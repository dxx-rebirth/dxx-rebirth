/* $Id: movie.c,v 1.36 2004-08-06 20:36:02 schaffner Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Movie Playing Stuff
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: movie.c,v 1.36 2004-08-06 20:36:02 schaffner Exp $";
#endif

#define DEBUG_LEVEL CON_NORMAL

#include <string.h>
#ifndef macintosh
# ifndef _WIN32_WCE
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
# endif
# ifndef _MSC_VER
#  include <unistd.h>
# endif
#endif // ! macintosh
#include <ctype.h>

#include "movie.h"
#include "console.h"
#include "args.h"
#include "key.h"
#include "digi.h"
#include "songs.h"
#include "inferno.h"
#include "palette.h"
#include "strutil.h"
#include "error.h"
#include "u_mem.h"
#include "byteswap.h"
#include "gr.h"
#include "gamefont.h"
#include "cfile.h"
#include "menu.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"

extern int MenuHiresAvailable;
extern char CDROM_dir[];

#define VID_PLAY 0
#define VID_PAUSE 1

int Vid_State;


// Subtitle data
typedef struct {
	short first_frame,last_frame;
	char *msg;
} subtitle;

#define MAX_SUBTITLES 500
#define MAX_ACTIVE_SUBTITLES 3
subtitle Subtitles[MAX_SUBTITLES];
int Num_subtitles;

// Movielib data
typedef struct {
	char name[FILENAME_LEN];
	int offset,len;
} ml_entry;

#define MLF_ON_CD    1
#define MAX_MOVIES_PER_LIB    50    //determines size of malloc

typedef struct {
	char     name[100]; //[FILENAME_LEN];
	int      n_movies;
	ubyte    flags,pad[3];
	ml_entry *movies;
} movielib;

#ifdef D2_OEM
char movielib_files[][FILENAME_LEN] = {"intro-l.mvl","other-l.mvl","robots-l.mvl","oem-l.mvl"};
#else
char movielib_files[][FILENAME_LEN] = {"intro-l.mvl","other-l.mvl","robots-l.mvl"};
#endif

#define N_BUILTIN_MOVIE_LIBS (sizeof(movielib_files)/sizeof(*movielib_files))
#define N_MOVIE_LIBS (N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB N_BUILTIN_MOVIE_LIBS
movielib *movie_libs[N_MOVIE_LIBS];


//do we have the robot movies available
int robot_movies = 0; //0 means none, 1 means lowres, 2 means hires

int MovieHires = 1;   //default is highres

CFILE *RoboFile = NULL;
int RoboFilePos = 0;

// Function Prototypes
int RunMovie(char *filename, int highres_flag, int allow_abort,int dx,int dy);

CFILE *open_movie_file(char *filename, int must_have);
int reset_movie_file(CFILE *handle);

void change_filename_ext( char *dest, char *src, char *ext );
void decode_text_line(char *p);
void draw_subtitles(int frame_num);


// ----------------------------------------------------------------------
void* MPlayAlloc(unsigned size)
{
    return d_malloc(size);
}

void MPlayFree(void *p)
{
    d_free(p);
}


//-----------------------------------------------------------------------

unsigned int FileRead(void *handle, void *buf, unsigned int count)
{
    unsigned numread;
    numread = cfread(buf, 1, count, (CFILE *)handle);
    return (numread == count);
}


//-----------------------------------------------------------------------


//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
int PlayMovie(const char *filename, int must_have)
{
	char name[FILENAME_LEN],*p;
	int c, ret;

#ifndef RELEASE
	if (FindArg("-nomovies"))
		return MOVIE_NOT_PLAYED;
#endif

	strcpy(name,filename);

	if ((p=strchr(name,'.')) == NULL)		//add extension, if missing
		strcat(name,".mve");

	//check for escape already pressed & abort if so
	while ((c=key_inkey()) != 0)
		if (c == KEY_ESC)
			return MOVIE_ABORTED;

	// Stop all digital sounds currently playing.
	digi_stop_all();

	// Stop all songs
	songs_stop_all();

	digi_close();

	// Start sound
	if (!FindArg("-nosound"))
		MVE_sndInit(1);
	else
		MVE_sndInit(-1);

	ret = RunMovie(name,MovieHires,must_have,-1,-1);

	if (!FindArg("-nosound"))
		digi_init();

	Screen_mode = -1;		//force screen reset

	return ret;
}


void MovieShowFrame(ubyte *buf, uint bufw, uint bufh, uint sx, uint sy,
					uint w, uint h, uint dstx, uint dsty)
{
	grs_bitmap source_bm;

	//mprintf((0,"MovieShowFrame %d,%d  %d,%d  %d,%d  %d,%d\n",bufw,bufh,sx,sy,w,h,dstx,dsty));

	Assert(bufw == w && bufh == h);

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = bufw;
	source_bm.bm_h = bufh;
	source_bm.bm_type = BM_LINEAR;
	source_bm.bm_flags = 0;
	source_bm.bm_data = buf;

	gr_bm_ubitblt(bufw,bufh,dstx,dsty,sx,sy,&source_bm,&grd_curcanv->cv_bitmap);
}

//our routine to set the pallete, called from the movie code
void MovieSetPalette(unsigned char *p, unsigned start, unsigned count)
{
	if (count == 0)
		return;

	//mprintf((0,"SetPalette p=%x, start=%d, count=%d\n",p,start,count));

	//Color 0 should be black, and we get color 255
	Assert(start>=1 && start+count-1<=254);

	//Set color 0 to be black
	gr_palette[0] = gr_palette[1] = gr_palette[2] = 0;

	//Set color 255 to be our subtitle color
	gr_palette[765] = gr_palette[766] = gr_palette[767] = 50;

	//movie libs palette into our array
	memcpy(gr_palette+start*3,p+start*3,count*3);

	//finally set the palette in the hardware
	//gr_palette_load(gr_palette);

	//MVE_SetPalette(p, start, count);
}


#if 0
typedef struct bkg {
	short x, y, w, h;           // The location of the menu.
	grs_bitmap * bmp;       	// The background under the menu.
} bkg;

bkg movie_bg = {0,0,0,0,NULL};
#endif

#define BOX_BORDER (MenuHires?40:20)


void show_pause_message(char *msg)
{
	int w,h,aw;
	int x,y;

	gr_set_current_canvas(NULL);
	gr_set_curfont( SMALL_FONT );

	gr_get_string_size(msg,&w,&h,&aw);

	x = (grd_curscreen->sc_w-w)/2;
	y = (grd_curscreen->sc_h-h)/2;

#if 0
	if (movie_bg.bmp) {
		gr_free_bitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}

	// Save the background of the display
	movie_bg.x=x; movie_bg.y=y; movie_bg.w=w; movie_bg.h=h;

	movie_bg.bmp = gr_create_bitmap( w+BOX_BORDER, h+BOX_BORDER );

	gr_bm_ubitblt(w+BOX_BORDER, h+BOX_BORDER, 0, 0, x-BOX_BORDER/2, y-BOX_BORDER/2, &(grd_curcanv->cv_bitmap), movie_bg.bmp );
#endif

	gr_setcolor(0);
	gr_rect(x-BOX_BORDER/2,y-BOX_BORDER/2,x+w+BOX_BORDER/2-1,y+h+BOX_BORDER/2-1);

	gr_set_fontcolor( 255, -1 );

	gr_ustring( 0x8000, y, msg );

	gr_update();
}

void clear_pause_message()
{
#if 0
	if (movie_bg.bmp) {

		gr_bitmap(movie_bg.x-BOX_BORDER/2, movie_bg.y-BOX_BORDER/2, movie_bg.bmp);

		gr_free_bitmap(movie_bg.bmp);
		movie_bg.bmp = NULL;
	}
#endif
}


//returns status.  see movie.h
int RunMovie(char *filename, int hires_flag, int must_have,int dx,int dy)
{
	CFILE *filehndl;
	int result=1,aborted=0;
	int track = 0;
	int frame_num;
	int key;
#ifdef OGL
	ubyte pal_save[768];
#endif

	result=1;

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = open_movie_file(filename,must_have);

	if (filehndl == NULL)
	{
		if (must_have)
			con_printf(CON_NORMAL, "movie: RunMovie: Cannot open movie <%s>\n",filename);
		return MOVIE_NOT_PLAYED;
	}

	MVE_memCallbacks(MPlayAlloc, MPlayFree);
	MVE_ioCallbacks(FileRead);

	if (hires_flag) {
		gr_set_mode(SM(640,480));
	} else {
		gr_set_mode(SM(320,200));
	}
#ifdef OGL
	set_screen_mode(SCREEN_MENU);
	gr_copy_palette(pal_save, gr_palette, 768);
	memset(gr_palette, 0, 768);
	gr_palette_load(gr_palette);
#endif

#if !defined(POLY_ACC)
	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);
#endif

	if (MVE_rmPrepMovie((void *)filehndl, dx, dy, track)) {
		Int3();
		return MOVIE_NOT_PLAYED;
	}

	frame_num = 0;

	FontHires = FontHiresAvailable && hires_flag;

	while((result = MVE_rmStepMovie()) == 0) {

		draw_subtitles(frame_num);

		gr_palette_load(gr_palette); // moved this here because of flashing

		gr_update();

		key = key_inkey();

		// If ESCAPE pressed, then quit movie.
		if (key == KEY_ESC) {
			result = aborted = 1;
			break;
		}

		// If PAUSE pressed, then pause movie
		if (key == KEY_PAUSE) {
			MVE_rmHoldMovie();
			show_pause_message(TXT_PAUSE);
			while (!key_inkey()) ;
			clear_pause_message();
		}

#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
		if ((key == KEY_ALTED+KEY_ENTER) ||
		    (key == KEY_ALTED+KEY_PADENTER))
			gr_toggle_fullscreen();
#endif

		frame_num++;
	}

	Assert(aborted || result == MVE_ERR_EOF);	 ///movie should be over

    MVE_rmEndMovie();

	cfclose(filehndl);                           // Close Movie File

	// Restore old graphic state

	Screen_mode=-1;  //force reset of screen mode
#ifdef OGL
	gr_copy_palette(gr_palette, pal_save, 768);
	gr_palette_load(pal_save);
#endif

	return (aborted?MOVIE_ABORTED:MOVIE_PLAYED_FULL);
}


int InitMovieBriefing()
{
#if 0
	if (MenuHires)
		gr_set_mode(SM(640,480));
	else
		gr_set_mode(SM(320,200));

	gr_init_sub_canvas( &VR_screen_pages[0], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
	gr_init_sub_canvas( &VR_screen_pages[1], &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
#endif

	return 1;
}


//returns 1 if frame updated ok
int RotateRobot()
{
	int err;

	err = MVE_rmStepMovie();

	gr_palette_load(gr_palette);

	if (err == MVE_ERR_EOF)     //end of movie, so reset
	{
		reset_movie_file(RoboFile);
		if (MVE_rmPrepMovie((void *)RoboFile, MenuHires?280:140, MenuHires?200:80, 0)) {
			Int3();
			return 0;
		}
	}
	else if (err) {
		Int3();
		return 0;
	}

	return 1;
}


void DeInitRobotMovie(void)
{
	MVE_rmEndMovie();
	cfclose(RoboFile);                           // Close Movie File
}


int InitRobotMovie(char *filename)
{
	if (FindArg("-nomovies"))
		return 0;

	con_printf(DEBUG_LEVEL, "RoboFile=%s\n", filename);

	MVE_sndInit(-1);        //tell movies to play no sound for robots

	RoboFile = open_movie_file(filename, 1);

	if (RoboFile == NULL)
	{
		Warning("movie: InitRobotMovie: Cannot open movie file <%s>",filename);
		return MOVIE_NOT_PLAYED;
	}

	Vid_State = VID_PLAY;

	if (MVE_rmPrepMovie((void *)RoboFile, MenuHires?280:140, MenuHires?200:80, 0)) {
		Int3();
		return 0;
	}

	RoboFilePos = cfseek(RoboFile, 0L, SEEK_CUR);

	con_printf(DEBUG_LEVEL, "RoboFilePos=%d!\n", RoboFilePos);

	return 1;
}


/*
 *		Subtitle system code
 */

char *subtitle_raw_data;


//search for next field following whitespace 
char *next_field (char *p)
{
	while (*p && !isspace(*p))
		p++;

	if (!*p)
		return NULL;

	while (*p && isspace(*p))
		p++;

	if (!*p)
		return NULL;

	return p;
}


int init_subtitles(char *filename)
{
	CFILE *ifile;
	int size,read_count;
	char *p;
	int have_binary = 0;

	Num_subtitles = 0;

	if (! FindArg("-subtitles"))
		return 0;

	ifile = cfopen(filename,"rb");		//try text version

	if (!ifile) {								//no text version, try binary version
		char filename2[FILENAME_LEN];
		change_filename_ext(filename2,filename,".TXB");
		ifile = cfopen(filename2,"rb");
		if (!ifile)
			return 0;
		have_binary = 1;
	}

	size = cfilelength(ifile);

	MALLOC (subtitle_raw_data, char, size+1);

	read_count = cfread(subtitle_raw_data, 1, size, ifile);

	cfclose(ifile);

	subtitle_raw_data[size] = 0;

	if (read_count != size) {
		d_free(subtitle_raw_data);
		return 0;
	}

	p = subtitle_raw_data;

	while (p && p < subtitle_raw_data+size) {
		char *endp;

		endp = strchr(p,'\n'); 
		if (endp) {
			if (endp[-1] == '\r')
				endp[-1] = 0;		//handle 0d0a pair
			*endp = 0;			//string termintor
		}

		if (have_binary)
			decode_text_line(p);

		if (*p != ';') {
			Subtitles[Num_subtitles].first_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			Subtitles[Num_subtitles].last_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			Subtitles[Num_subtitles].msg = p;

			Assert(Num_subtitles==0 || Subtitles[Num_subtitles].first_frame >= Subtitles[Num_subtitles-1].first_frame);
			Assert(Subtitles[Num_subtitles].last_frame >= Subtitles[Num_subtitles].first_frame);

			Num_subtitles++;
		}

		p = endp+1;

	}

	return 1;
}


void close_subtitles()
{
	if (subtitle_raw_data)
		d_free(subtitle_raw_data);
	subtitle_raw_data = NULL;
	Num_subtitles = 0;
}


//draw the subtitles for this frame
void draw_subtitles(int frame_num)
{
	static int active_subtitles[MAX_ACTIVE_SUBTITLES];
	static int num_active_subtitles,next_subtitle,line_spacing;
	int t,y;
	int must_erase=0;

	if (frame_num == 0) {
		num_active_subtitles = 0;
		next_subtitle = 0;
		gr_set_curfont( GAME_FONT );
		line_spacing = grd_curcanv->cv_font->ft_h + (grd_curcanv->cv_font->ft_h >> 2);
		gr_set_fontcolor(255,-1);
	}

	//get rid of any subtitles that have expired
	for (t=0;t<num_active_subtitles;)
		if (frame_num > Subtitles[active_subtitles[t]].last_frame) {
			int t2;
			for (t2=t;t2<num_active_subtitles-1;t2++)
				active_subtitles[t2] = active_subtitles[t2+1];
			num_active_subtitles--;
			must_erase = 1;
		}
		else
			t++;

	//get any subtitles new for this frame 
	while (next_subtitle < Num_subtitles && frame_num >= Subtitles[next_subtitle].first_frame) {
		if (num_active_subtitles >= MAX_ACTIVE_SUBTITLES)
			Error("Too many active subtitles!");
		active_subtitles[num_active_subtitles++] = next_subtitle;
		next_subtitle++;
	}

	//find y coordinate for first line of subtitles
	y = grd_curcanv->cv_bitmap.bm_h-((line_spacing+1)*MAX_ACTIVE_SUBTITLES+2);

	//erase old subtitles if necessary
	if (must_erase) {
		gr_setcolor(0);
		gr_rect(0,y,grd_curcanv->cv_bitmap.bm_w-1,grd_curcanv->cv_bitmap.bm_h-1);
	}

	//now draw the current subtitles
	for (t=0;t<num_active_subtitles;t++)
		if (active_subtitles[t] != -1) {
			gr_string(0x8000,y,Subtitles[active_subtitles[t]].msg);
			y += line_spacing+1;
		}
}


movielib *init_new_movie_lib(char *filename, CFILE *fp)
{
	int nfiles,offset;
	int i,n;
	movielib *table;

	//read movie file header

	nfiles = cfile_read_int(fp);        //get number of files

	//table = d_malloc(sizeof(*table) + sizeof(ml_entry)*nfiles);
	MALLOC(table, movielib, 1);
	MALLOC(table->movies, ml_entry, nfiles);

	strcpy(table->name,filename);
	table->n_movies = nfiles;

	offset = 4+4+nfiles*(13+4);	//id + nfiles + nfiles * (filename + size)

	for (i=0;i<nfiles;i++) {
		int len;

		n = cfread(table->movies[i].name, 13, 1, fp);
		if ( n != 1 )
			break;		//end of file (probably)

		len = cfile_read_int(fp);

		table->movies[i].len = len;
		table->movies[i].offset = offset;

		offset += table->movies[i].len;

	}

	cfclose(fp);

	table->flags = 0;

	return table;

}


movielib *init_old_movie_lib(char *filename, CFILE *fp)
{
	int nfiles,size;
	int i;
	movielib *table,*table2;

	nfiles = 0;

	//allocate big table
	table = d_malloc(sizeof(*table) + sizeof(ml_entry)*MAX_MOVIES_PER_LIB);

	while( 1 ) {
		int len;

		i = cfread(table->movies[nfiles].name, 13, 1, fp);
		if ( i != 1 )
			break;		//end of file (probably)

		i = cfread(&len, 4, 1, fp);
		if ( i != 1 )
			Error("error reading movie library <%s>",filename);

		table->movies[nfiles].len = INTEL_INT(len);
		table->movies[nfiles].offset = cftell(fp);

		cfseek(fp, INTEL_INT(len), SEEK_CUR);       //skip data

		nfiles++;
	}

	//allocate correct-sized table
	size = sizeof(*table) + sizeof(ml_entry)*nfiles;
	table2 = d_malloc(size);
	memcpy(table2,table,size);
	d_free(table);
	table = table2;

	strcpy(table->name,filename);

	table->n_movies = nfiles;

	cfclose(fp);

	table->flags = 0;

	return table;

}


//find the specified movie library, and read in list of movies in it
movielib *init_movie_lib(char *filename)
{
	//note: this based on cfile_init_hogfile()

	char id[4];
	CFILE *fp;

	fp = cfopen(filename, "rb");

	if ( fp == NULL )
		return NULL;

	cfread(id, 4, 1, fp);
	if ( !strncmp( id, "DMVL", 4 ) )
		return init_new_movie_lib(filename,fp);
	else if ( !strncmp( id, "DHF", 3 ) ) {
		cfseek(fp,-1,SEEK_CUR);		//old file had 3 char id
		return init_old_movie_lib(filename,fp);
	}
	else {
		cfclose(fp);
		return NULL;
	}
}


void close_movie(int i)
{
	if (movie_libs[i]) {
		d_free(movie_libs[i]->movies);
		d_free(movie_libs[i]);
	}
}


void close_movies()
{
	int i;

	for (i=0;i<N_MOVIE_LIBS;i++)
		close_movie(i);
}


//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int request_cd(void)
{
#if 0
	ubyte save_pal[256*3];
	grs_canvas *save_canv,*tcanv;
	int ret,was_faded=gr_palette_faded_out;

	gr_palette_clear();

	save_canv = grd_curcanv;
	tcanv = gr_create_canvas(grd_curcanv->cv_w,grd_curcanv->cv_h);

	gr_set_current_canvas(tcanv);
	gr_ubitmap(0,0,&save_canv->cv_bitmap);
	gr_set_current_canvas(save_canv);

	gr_clear_canvas(BM_XRGB(0,0,0));

	memcpy(save_pal,gr_palette,sizeof(save_pal));

	memcpy(gr_palette,last_palette_for_color_fonts,sizeof(gr_palette));

 try_again:;

	ret = nm_messagebox( "CD ERROR", 1, "Ok", "Please insert your Descent II CD");

	if (ret == -1) {
		int ret2;

		ret2 = nm_messagebox( "CD ERROR", 2, "Try Again", "Leave Game", "You must insert your\nDescent II CD to Continue");

		if (ret2 == -1 || ret2 == 0)
			goto try_again;
	}

	force_rb_register = 1;  //disc has changed; force register new CD

	gr_palette_clear();

	memcpy(gr_palette,save_pal,sizeof(save_pal));

	gr_ubitmap(0,0,&tcanv->cv_bitmap);

	if (!was_faded)
		gr_palette_load(gr_palette);

	gr_free_canvas(tcanv);

	return ret;
#else
	con_printf(DEBUG_LEVEL, "STUB: movie: request_cd\n");
	return 0;
#endif
}


void init_movie(char *filename, int libnum, int is_robots, int required)
{
	int high_res, try;
	char *res = strchr(filename, '.') - 1; // 'h' == high resolution, 'l' == low

#ifndef RELEASE
	if (FindArg("-nomovies")) {
		movie_libs[libnum] = NULL;
		return;
	}
#endif

	//for robots, load highres versions if highres menus set
	if (is_robots)
		high_res = MenuHiresAvailable;
	else
		high_res = MovieHires;

	if (high_res)
		*res = 'h';

	for (try = 0; (movie_libs[libnum] = init_movie_lib(filename)) == NULL; try++) {
		char name2[100];

		strcpy(name2,CDROM_dir);
		strcat(name2,filename);
		movie_libs[libnum] = init_movie_lib(name2);

		if (movie_libs[libnum] != NULL) {
			movie_libs[libnum]->flags |= MLF_ON_CD;
			break; // we found our movie on the CD
		} else {
			if (try == 0) { // first try
				if (*res == 'h') { // try low res instead
					*res = 'l';
					high_res = 0;
				} else if (*res == 'l') { // try high
					*res = 'h';
					high_res = 1;
				} else {
					if (required)
						Warning("Cannot open movie file <%s>",filename);
					break;
				}
			} else { // try == 1
				if (required) {
					*res = '*';
					Warning("Cannot open any movie file <%s>", filename);
				}
				break;
			}
		}
	}

	if (is_robots && movie_libs[libnum]!=NULL)
		robot_movies = high_res?2:1;
}


//find and initialize the movie libraries
void init_movies()
{
	int i;
	int is_robots;

	for (i=0;i<N_BUILTIN_MOVIE_LIBS;i++) {

		if (!strnicmp(movielib_files[i],"robot",5))
			is_robots = 1;
		else
			is_robots = 0;

		init_movie(movielib_files[i],i,is_robots,1);
	}

	movie_libs[EXTRA_ROBOT_LIB] = NULL;

	atexit(close_movies);
}


void init_extra_robot_movie(char *filename)
{
	close_movie(EXTRA_ROBOT_LIB);
	init_movie(filename,EXTRA_ROBOT_LIB,1,0);
}


CFILE *movie_handle;
int movie_start;

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
CFILE *search_movie_lib(movielib *lib, char *filename, int must_have)
{
	int i;
	CFILE *filehandle;

	if (lib == NULL)
		return NULL;

	for (i=0;i<lib->n_movies;i++)
		if (!stricmp(filename,lib->movies[i].name)) {	//found the movie in a library 
			int from_cd;

			from_cd = (lib->flags & MLF_ON_CD);

			if (from_cd)
				songs_stop_redbook();		//ready to read from CD

			do {		//keep trying until we get the file handle

				movie_handle = filehandle = cfopen(lib->name, "rb");

				if (must_have && from_cd && filehandle == NULL)
				{   //didn't get file!

					if (request_cd() == -1)		//ESC from requester
						break;						//bail from here. will get error later
				}

			} while (must_have && from_cd && filehandle == NULL);

			if (filehandle)
				cfseek(filehandle, (movie_start = lib->movies[i].offset), SEEK_SET);

			return filehandle;
		}

	return NULL;
}


//returns file handle
CFILE *open_movie_file(char *filename, int must_have)
{
	CFILE *filehandle;
	int i;

	for (i=0;i<N_MOVIE_LIBS;i++) {
		if ((filehandle = search_movie_lib(movie_libs[i], filename, must_have)) != NULL)
			return filehandle;
	}

	return NULL;    //couldn't find it
}

//sets the file position to the start of this already-open file
int reset_movie_file(CFILE *handle)
{
	Assert(handle == movie_handle);

	cfseek(handle, movie_start, SEEK_SET);

	return 0;       //everything is cool
}
