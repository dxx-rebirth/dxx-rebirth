/* $Id: movie.c,v 1.39 2005-07-30 01:50:17 chris Exp $ */
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
static char rcsid[] = "$Id: movie.c,v 1.39 2005-07-30 01:50:17 chris Exp $";
#endif

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
#include "physfsrwops.h"

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

#ifdef D2_OEM
char movielib_files[5][FILENAME_LEN] = {"intro","other","robots","oem"};
#else
char movielib_files[4][FILENAME_LEN] = {"intro","other","robots"};
#endif

#define N_MOVIE_LIBS (sizeof(movielib_files) / sizeof(*movielib_files))
#define N_BUILTIN_MOVIE_LIBS (N_MOVIE_LIBS - 1)
#define EXTRA_ROBOT_LIB N_BUILTIN_MOVIE_LIBS

int MovieHires = 1;   //default is highres

SDL_RWops *RoboFile;

// Function Prototypes
int RunMovie(char *filename, int highres_flag, int allow_abort,int dx,int dy);

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
    numread = SDL_RWread((SDL_RWops *)handle, buf, 1, count);
    return (numread == count);
}


//-----------------------------------------------------------------------


//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
int PlayMovie(const char *filename, int must_have)
{
	char name[FILENAME_LEN],*p;
	int c, ret;

	if (FindArg("-nomovies"))
		return MOVIE_NOT_PLAYED;

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
	SDL_RWops *filehndl;
	int result=1,aborted=0;
	int track = 0;
	int frame_num;
	int key;
#ifdef OGL
	ubyte pal_save[768];
#endif

	result=1;

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = PHYSFSRWOPS_openRead(filename);

	if (!filehndl)
	{
		if (must_have)
			con_printf(CON_URGENT, "Can't open movie <%s>: %s\n", filename, PHYSFS_getLastError());
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

	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);

	if (MVE_rmPrepMovie((void *)filehndl, dx, dy, track)) {
		Int3();
		return MOVIE_NOT_PLAYED;
	}

	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);

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

	SDL_FreeRW(filehndl);                           // Close Movie File

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
		SDL_RWseek(RoboFile, 0, SEEK_SET);
		if (MVE_rmPrepMovie(RoboFile, MenuHires?280:140, MenuHires?200:80, 0))
		{
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
	SDL_FreeRW(RoboFile);                           // Close Movie File
}


int InitRobotMovie(char *filename)
{
	if (FindArg("-nomovies"))
		return 0;

	con_printf(CON_DEBUG, "RoboFile=%s\n", filename);

	MVE_sndInit(-1);        //tell movies to play no sound for robots

	RoboFile = PHYSFSRWOPS_openRead(filename);

	if (!RoboFile)
	{
		con_printf(CON_URGENT, "Can't open movie <%s>: %s\n", filename, PHYSFS_getLastError());
		return MOVIE_NOT_PLAYED;
	}

	Vid_State = VID_PLAY;

	if (MVE_rmPrepMovie((void *)RoboFile, MenuHires?280:140, MenuHires?200:80, 0)) {
		Int3();
		return 0;
	}

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


void close_movie(char *movielib, int is_robots)
{
	int high_res;
	char filename[FILENAME_LEN];

	if (is_robots)
		high_res = MenuHiresAvailable;
	else
		high_res = MovieHires;

	sprintf(filename, "%s-%s.mvl", movielib, high_res?"h":"l");

	if (!cfile_close(filename))
	{
		con_printf(CON_URGENT, "Can't close movielib <%s>: %s\n", filename, PHYSFS_getLastError());
		sprintf(filename, "%s-%s.mvl", movielib, high_res?"l":"h");

		if (!cfile_close(filename))
			con_printf(CON_URGENT, "Can't close movielib <%s>: %s\n", filename, PHYSFS_getLastError());
	}
}

void close_movies()
{
	int i, is_robots;

	for (i = 0 ; i < N_BUILTIN_MOVIE_LIBS ; i++)
	{
		if (!strnicmp(movielib_files[i], "robot", 5))
			is_robots = 1;
		else
			is_robots = 0;

		close_movie(movielib_files[i], is_robots);
	}
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
	con_printf(CON_DEBUG, "STUB: movie: request_cd\n");
	return 0;
#endif
}


void init_movie(char *movielib, int is_robots, int required)
{
	int high_res;
	char filename[FILENAME_LEN];

	//for robots, load highres versions if highres menus set
	if (is_robots)
		high_res = MenuHiresAvailable;
	else
		high_res = MovieHires;

	sprintf(filename, "%s-%s.mvl", movielib, high_res?"h":"l");

	if (!cfile_init(filename))
	{
		if (required)
			con_printf(CON_URGENT, "Can't open movielib <%s>: %s\n", filename, PHYSFS_getLastError());

		sprintf(filename, "%s-%s.mvl", movielib, high_res?"l":"h");

		if (!cfile_init(filename))
			if (required)
				con_printf(CON_URGENT, "Can't open movielib <%s>: %s\n", filename, PHYSFS_getLastError());
	}
}


//find and initialize the movie libraries
void init_movies()
{
	int i;
	int is_robots;

	if (FindArg("-nomovies"))
		return;

	for (i=0;i<N_BUILTIN_MOVIE_LIBS;i++) {

		if (!strnicmp(movielib_files[i],"robot",5))
			is_robots = 1;
		else
			is_robots = 0;

		init_movie(movielib_files[i], is_robots, 1);
	}

	atexit(close_movies);
}


void close_extra_robot_movie(void)
{
	if (strlen(movielib_files[EXTRA_ROBOT_LIB]))
		if (!cfile_close(movielib_files[EXTRA_ROBOT_LIB]))
			con_printf(CON_URGENT, "Can't close robot movielib: %s\n", PHYSFS_getLastError());
}

void init_extra_robot_movie(char *movielib)
{
	if (FindArg("-nomovies"))
		return;

	close_extra_robot_movie();
	init_movie(movielib, 1, 0);
	strcpy(movielib_files[EXTRA_ROBOT_LIB], movielib);
	atexit(close_extra_robot_movie);
}
