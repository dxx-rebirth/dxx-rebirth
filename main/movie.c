/* $Id: movie.c,v 1.11 2002-08-31 12:14:01 btb Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: movie.c,v 1.11 2002-08-31 12:14:01 btb Exp $";
#endif

#define DEBUG_LEVEL CON_NORMAL

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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
#include "mvelib.h"
#include "text.h"
#include "fileutil.h"

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

int MVEPaletteCalls = 0;

//do we have the robot movies available
int robot_movies = 0; //0 means none, 1 means lowres, 2 means hires

int MovieHires = 0;   //default for now is lores

int RoboFile = 0;
char Robo_filename[FILENAME_LEN];
MVESTREAM *Robo_mve;
grs_bitmap *Robo_bitmap;


//      Function Prototypes
int RunMovie(char *filename, int highres_flag, int allow_abort,int dx,int dy);

int open_movie_file(char *filename,int must_have);

void change_filename_ext( char *dest, char *src, char *ext );
void decode_text_line(char *p);
void draw_subtitles(int frame_num);


//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
int PlayMovie(const char *filename, int must_have)
{
	char name[FILENAME_LEN],*p;
	int c, ret;
#if 0
	int save_sample_rate;
#endif

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

#if 0
	save_sample_rate = digi_sample_rate;
	digi_sample_rate = SAMPLE_RATE_22K;		//always 22K for movies
	digi_reset(); digi_reset();
#else
	digi_close();
#endif

	ret = RunMovie(name,MovieHires,must_have,-1,-1);

#if 0
	gr_palette_clear();		//clear out palette in case movie aborted
#endif

#if 0
	digi_sample_rate = save_sample_rate;		//restore rate for game
	digi_reset(); digi_reset();
#else
	digi_init();
#endif

	Screen_mode = -1;		//force screen reset

	return ret;
}

void initializeMovie(MVESTREAM *mve, grs_bitmap *mve_bitmap);
void shutdownMovie(MVESTREAM *mve);

//returns status.  see movie.h
int RunMovie(char *filename, int hires_flag, int must_have,int dx,int dy)
{
	int filehndl;
	int result=1,aborted=0;
	int frame_num;
	MVESTREAM *mve;
	grs_bitmap *mve_bitmap;

	result=1;
	con_printf(DEBUG_LEVEL, "movie: RunMovie: %s %d %d %d %d\n", filename, hires_flag, must_have, dx, dy);

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = open_movie_file(filename,must_have);

	if (filehndl == -1) {
#ifndef EDITOR
		if (must_have)
			Warning("movie: RunMovie: Cannot open movie <%s>\n",filename);
#endif
		return MOVIE_NOT_PLAYED;
	}

	if (hires_flag)
		gr_set_mode(SM(640,480));
	else
		gr_set_mode(SM(320,200));

	frame_num = 0;

	FontHires = FontHiresAvailable && hires_flag;

    mve = mve_open(filehndl);
    if (mve == NULL)
    {
        fprintf(stderr, "can't open MVE file '%s'\n", filename);
        return 1;
    }

	mve_bitmap = gr_create_bitmap(hires_flag?592:320, hires_flag?312:200);

    initializeMovie(mve, mve_bitmap);

	while((result = mve_play_next_chunk(mve))) {
		int key;

		gr_bitmap(hires_flag?24:12, hires_flag?84:42, mve_bitmap);

		draw_subtitles(frame_num);

		gr_update();

		key = key_inkey();

		// If ESCAPE pressed, then quit movie.
		if (key == KEY_ESC) {
			result = 0;
			aborted = 1;
			break;
		}

		// If PAUSE pressed, then pause movie
		if (key == KEY_PAUSE) {
			//MVE_rmHoldMovie();
			//show_pause_message(TXT_PAUSE);
			while (!key_inkey()) ;
			//clear_pause_message();
		}

		frame_num++;
	}

	Assert(aborted || !result);	 ///movie should be over

    shutdownMovie(mve);

	gr_free_bitmap(mve_bitmap);

    mve_close(mve);

	close(filehndl);                           // Close Movie File

	// Restore old graphic state

	Screen_mode=-1;  //force reset of screen mode

	return (aborted?MOVIE_ABORTED:MOVIE_PLAYED_FULL);
}


int InitMovieBriefing()
{
	if (MenuHires)
		gr_set_mode(SM(640,480));
	else
		gr_set_mode(SM(320,200));

	return 1;
}


//returns 1 if frame updated ok
int RotateRobot()
{
	int ret;

	ret = mve_play_next_chunk(Robo_mve);
	gr_bitmap(MenuHires?280:200, MenuHires?140:80, Robo_bitmap);

	if (!ret) {
		DeInitRobotMovie();
		InitRobotMovie(Robo_filename);
	}

	return 1;
}


void DeInitRobotMovie(void)
{
	con_printf(DEBUG_LEVEL, "movie: DeInitRobotMovie\n");

	shutdownMovie(Robo_mve);

	gr_free_bitmap(Robo_bitmap);

	close(RoboFile);
}


int InitRobotMovie(char *filename)
{
	con_printf(DEBUG_LEVEL, "movie: InitRobotMovie: %s\n", filename);

	RoboFile = open_movie_file(filename, 1);

	if (RoboFile == -1) {
		Warning("movie: InitRobotMovie: Cannot open movie file <%s>",filename);
		return MOVIE_NOT_PLAYED;
	}

	strcpy(Robo_filename, filename);

	Robo_mve = mve_open(RoboFile);
    if (Robo_mve == NULL)
    {
        fprintf(stderr, "can't open MVE file '%s'\n", filename);
        return 0;
    }

	Robo_bitmap = gr_create_bitmap(MenuHires?320:160, MenuHires?200:100);

	initializeMovie(Robo_mve, Robo_bitmap);

	return 1;
}


/*
 *		Subtitle system code
 */

ubyte *subtitle_raw_data;


//search for next field following whitespace 
ubyte *next_field(ubyte *p)
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
	ubyte *p;
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
   
	MALLOC (subtitle_raw_data, ubyte, size+1);

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


int request_cd(void)
{
	con_printf(DEBUG_LEVEL, "STUB: movie: request_cd\n");
	return 0;
}


movielib *init_new_movie_lib(char *filename,FILE *fp)
{
	int nfiles,offset;
	int i,n;
	movielib *table;

	//read movie file header

	nfiles = file_read_int(fp);		//get number of files

	//table = d_malloc(sizeof(*table) + sizeof(ml_entry)*nfiles);
	MALLOC(table, movielib, 1);
	MALLOC(table->movies, ml_entry, nfiles);

	strcpy(table->name,filename);
	table->n_movies = nfiles;

	offset = 4+4+nfiles*(13+4);	//id + nfiles + nfiles * (filename + size)

	for (i=0;i<nfiles;i++) {
		int len;

		n = fread( table->movies[i].name, 13, 1, fp );
		if ( n != 1 )
			break;		//end of file (probably)

		len = file_read_int(fp);

		table->movies[i].len = len;
		table->movies[i].offset = offset;

		offset += table->movies[i].len;

	}

	fclose(fp);

	table->flags = 0;

	return table;

}


movielib *init_old_movie_lib(char *filename,FILE *fp)
{
	int nfiles,size;
	int i;
	movielib *table,*table2;

	nfiles = 0;

	//allocate big table
	table = d_malloc(sizeof(*table) + sizeof(ml_entry)*MAX_MOVIES_PER_LIB);

	while( 1 ) {
		int len;

		i = fread( table->movies[nfiles].name, 13, 1, fp );
		if ( i != 1 )
			break;		//end of file (probably)

		i = fread( &len, 4, 1, fp );
		if ( i != 1 )
			Error("error reading movie library <%s>",filename);

		table->movies[nfiles].len = INTEL_INT(len);
		table->movies[nfiles].offset = ftell( fp );

		fseek( fp, INTEL_INT(len), SEEK_CUR );		//skip data

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

	fclose(fp);

	table->flags = 0;

	return table;

}


//find the specified movie library, and read in list of movies in it
movielib *init_movie_lib(char *filename)
{
	//note: this based on cfile_init_hogfile()

	char id[4];
	FILE * fp;

	fp = fopen( filename, "rb" );

	if ((fp == NULL) && (AltHogdir_initialized)) {
		char temp[128];
		strcpy(temp, AltHogDir);
		strcat(temp, "/");
		strcat(temp, filename);
		fp = fopen(temp, "rb");
	}

	if ( fp == NULL )
		return NULL;

	fread( id, 4, 1, fp );
	if ( !strncmp( id, "DMVL", 4 ) )
		return init_new_movie_lib(filename,fp);
	else if ( !strncmp( id, "DHF", 3 ) ) {
		fseek(fp,-1,SEEK_CUR);		//old file had 3 char id
		return init_old_movie_lib(filename,fp);
	}
	else {
		fclose(fp);
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


void init_movie(char *filename,int libnum,int is_robots,int required)
{
	int high_res;
	int try = 0;

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
		strchr(filename,'.')[-1] = 'h';

try_again:;

	if ((movie_libs[libnum] = init_movie_lib(filename)) == NULL) {
		char name2[100];

		strcpy(name2,CDROM_dir);
		strcat(name2,filename);
		movie_libs[libnum] = init_movie_lib(name2);

		if (movie_libs[libnum] != NULL)
			movie_libs[libnum]->flags |= MLF_ON_CD;
		else {
			if (required) {
				Warning("Cannot open movie file <%s>\n",filename);
			}

			if (!try) {                                         // first try
				if (strchr(filename, '.')[-1] == 'h') {         // try again with lowres
					strchr(filename, '.')[-1] = 'l';
					Warning("Trying to open movie file <%s> instead\n", filename);
					try++;
					goto try_again;
				} else if (strchr(filename, '.')[-1] == 'l') {  // try again with highres
					strchr(filename, '.')[-1] = 'h';
					Warning("Trying to open movie file <%s> instead\n", filename);
					try++;
					goto try_again;
				}
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
	con_printf(DEBUG_LEVEL, "movie: init_extra_robot_movie: %s\n", filename);

	close_movie(EXTRA_ROBOT_LIB);
	init_movie(filename,EXTRA_ROBOT_LIB,1,0);
}


//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
int search_movie_lib(movielib *lib,char *filename,int must_have)
{
	int i;
	int filehandle;

	if (lib == NULL)
		return -1;

	for (i=0;i<lib->n_movies;i++)
		if (!stricmp(filename,lib->movies[i].name)) {	//found the movie in a library 
			int from_cd;

			from_cd = (lib->flags & MLF_ON_CD);

			if (from_cd)
				songs_stop_redbook();		//ready to read from CD

			do {		//keep trying until we get the file handle

				/* movie_handle = */ filehandle = open(lib->name, O_RDONLY);

				if ((filehandle == -1) && (AltHogdir_initialized)) {
					char temp[128];
					strcpy(temp, AltHogDir);
					strcat(temp, "/");
					strcat(temp, lib->name);
					filehandle = open(temp, O_RDONLY);
				}

				if (must_have && from_cd && filehandle == -1) {		//didn't get file!

					if (request_cd() == -1)		//ESC from requester
						break;						//bail from here. will get error later
				}

			} while (must_have && from_cd && filehandle == -1);

			if (filehandle != -1)
				lseek(filehandle,(/* movie_start = */ lib->movies[i].offset),SEEK_SET);

			return filehandle;
		}

	return -1;
}


//returns file handle
int open_movie_file(char *filename,int must_have)
{
	int filehandle,i;

	for (i=0;i<N_MOVIE_LIBS;i++) {
		if ((filehandle = search_movie_lib(movie_libs[i],filename,must_have)) != -1)
			return filehandle;
	}

	return -1;		//couldn't find it
}


