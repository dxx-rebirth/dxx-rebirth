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
 * $Source: /cvs/cvsroot/d2x/main/movie.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-01-18 07:26:54 $
 *
 * Movie stuff (converts mve's to exe files, and plays them externally (e.g. with wine)
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "inferno.h"
#include "args.h"
#include "movie.h"
#include "key.h"
#include "songs.h"
#include "strutil.h"
#include "mono.h"
#include "error.h"
#include "digi.h"
#include "u_mem.h"
#include "byteswap.h"
#include "cfile.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"

int RoboFile=0,MVEPaletteCalls=0;

//      Function Prototypes
int RunMovie(char *filename, int highres_flag, int allow_abort,int dx,int dy);

// Subtitle data
typedef struct {
	short first_frame,last_frame;
	char *msg;
} subtitle;


// #define BUFFER_MOVIE 

#define MAX_SUBTITLES 500
subtitle Subtitles[MAX_SUBTITLES];
int Num_subtitles;

int MovieHires = 0;		//default for now is lores

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

	ret = RunMovie(name,MovieHires,must_have,-1,-1);

	return ret;
}
 
int open_movie_file(char *filename,int must_have, int *lenp);

//returns status.  see movie.h
int RunMovie(char *filename, int hires_flag, int must_have,int dx,int dy)
{
	int filehndl, size;
	int aborted=0;

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = open_movie_file(filename,must_have, &size);

	if (filehndl == -1) {
#ifndef EDITOR
		if (must_have)
			{
				strupr(filename);
				Error("Cannot open movie file <%s>",filename);
			}	
		else
			return MOVIE_NOT_PLAYED;
#else
		return MOVIE_NOT_PLAYED;
#endif
	}

#if 0
	if (hires_flag)
		gr_set_mode(SM_640x480V);
	else
		gr_set_mode(SM_320x200C);
#endif

	// play!
	{
		char *buf;
		int len;
		FILE *fil;
		struct stat stats;
		char *stubfile;
		char *execcmd;

		strcpy(filename+strlen(filename)-4,".exe"); //change extension
		if (stat(filename, &stats)) {
			stubfile = "fstrailw.stub";
			if (stat(stubfile, &stats)) {
				con_printf(CON_NORMAL, "Error loading %s, aborting movie.\n", stubfile);
				return MOVIE_NOT_PLAYED;
			}
			
			len = stats.st_size;
			buf = d_malloc(len);
			fil = fopen(stubfile, "r");
			fread(buf, len, 1, fil);
			fclose(fil);
			
			fil = fopen(filename, "w");
			fwrite(buf, len, 1, fil);
			d_free(buf);
			
			len = size;
			buf = d_malloc(len);
			read(filehndl, buf, len);
			fwrite(buf, len, 1, fil);
			d_free(buf);
			fclose(fil);
		}
		sprintf(execcmd, "wine %s", filename);
		if(system(execcmd) == -1) {
			con_printf(CON_NORMAL, "Error executing %s, movie aborted.\n", filename);
			return MOVIE_NOT_PLAYED;
		}
			

	}

	close(filehndl);                           // Close Movie File
 
	Screen_mode=-1;		//force reset of screen mode
	    
	return (aborted?MOVIE_ABORTED:MOVIE_PLAYED_FULL);
}

int InitMovieBriefing ()
{
	return 1;
}

//returns 1 if frame updated ok
int RotateRobot ()
{
	return 1;
}

void DeInitRobotMovie()
{
	close(RoboFile);                           // Close Movie File
}

int InitRobotMovie (char *filename)
{
	int len;

	if (FindArg("-nomovies"))
		return MOVIE_NOT_PLAYED; 

//	digi_stop_all();

	mprintf ((0,"RoboFile=%s\n",filename));

	RoboFile = open_movie_file(filename,1, &len);

	if (RoboFile == -1) {
		#ifdef RELEASE
			Error("Cannot open movie file <%s>",filename);
		#else
			return MOVIE_NOT_PLAYED;
		#endif
	}
	
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

void change_filename_ext( char *dest, char *src, char *ext );
void decode_text_line(char *p);

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
		free(subtitle_raw_data);
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
		free(subtitle_raw_data);
	subtitle_raw_data = NULL;
	Num_subtitles = 0;
}

typedef struct {
	char name[FILENAME_LEN];
	int offset,len;
} ml_entry;

#define MLF_ON_CD		0

typedef struct {
	char		name[100];	//[FILENAME_LEN];
	int		n_movies;
	ubyte		flags,pad[3];
	ml_entry	movies[1];
} movielib;

#define MAX_MOVIES_PER_LIB		50		//determines size of malloc

movielib *init_new_movie_lib(char *filename,FILE *fp)
{
	int nfiles,offset;
	int i,n;
	movielib *table;

	//read movie file header

	fread(&nfiles,4,1,fp);		//get number of files

	table = malloc(sizeof(*table) + sizeof(ml_entry)*nfiles);

	strcpy(table->name,filename);
	table->n_movies = nfiles;

	offset = 4+4+nfiles*(13+4);	//id + nfiles + nfiles * (filename + size)

	for (i=0;i<nfiles;i++) {
		int len;

		n = fread( table->movies[i].name, 13, 1, fp );
		if ( n != 1 )
			break;		//end of file (probably)

		n = fread( &len, 4, 1, fp );
		if ( n != 1 )
			Error("error reading movie library <%s>",filename);

		table->movies[i].len = INTEL_INT(len);
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
	table = malloc(sizeof(*table) + sizeof(ml_entry)*MAX_MOVIES_PER_LIB);

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
	table2 = malloc(size);
	memcpy(table2,table,size);
	free(table);
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

#ifdef D2_OEM
char *movielib_files[] = {"intro-l.mvl","other-l.mvl","robots-l.mvl","oem-l.mvl"};
#else
char *movielib_files[] = {"intro-l.mvl","other-l.mvl","robots-l.mvl"};
#endif

#define N_BUILTIN_MOVIE_LIBS (sizeof(movielib_files)/sizeof(*movielib_files))
#define N_MOVIE_LIBS (N_BUILTIN_MOVIE_LIBS+1)
#define EXTRA_ROBOT_LIB N_BUILTIN_MOVIE_LIBS
movielib *movie_libs[N_MOVIE_LIBS];

void close_movie(int i)
{
	if (movie_libs[i])
		free(movie_libs[i]);
}

void close_movies()
{
	int i;

	for (i=0;i<N_MOVIE_LIBS;i++)
		close_movie(i);
}

#include "gamepal.h"

extern char CDROM_dir[];
extern int MenuHiresAvailable;

extern ubyte last_palette_for_color_fonts[];

extern int force_rb_register;

//ask user to put the D2 CD in.
//returns -1 if ESC pressed, 0 if OK chosen
//CD may not have been inserted
int request_cd()
{
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

	force_rb_register = 1;	//disc has changed; force register new CD    
	
	gr_palette_clear();

	memcpy(gr_palette,save_pal,sizeof(save_pal));
	
	gr_ubitmap(0,0,&tcanv->cv_bitmap);

	if (!was_faded)
		gr_palette_load(gr_palette);

	gr_free_canvas(tcanv);

	return ret;
}

//do we have the robot movies available
int robot_movies=0;	//0 means none, 1 means lowres, 2 means hires

void init_movie(char *filename,int libnum,int is_robots,int required)
{
	int high_res;

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

#if defined(D2_OEM)
try_again:;
#endif

	if ((movie_libs[libnum] = init_movie_lib(filename)) == NULL) {
		char name2[100];
		
		strcpy(name2,CDROM_dir);
		strcat(name2,filename);
		movie_libs[libnum] = init_movie_lib(name2);

		if (movie_libs[libnum] != NULL)
			movie_libs[libnum]->flags |= MLF_ON_CD;
		else {
			if (required) {
				#if defined(RELEASE) && !defined(D2_OEM)		//allow no movies if not release
					strupr(filename);
					Error("Cannot open movie file <%s>",filename);
				#endif
			}
			#if defined(D2_OEM)		//if couldn't get higres, try low
			if (is_robots == 1) {	//first try, try again with lowres
				strchr(filename,'.')[-1] = 'l';
				high_res = 0;
				is_robots++;
				goto try_again;
			}
			else if (is_robots == 2) {		//failed twice. bail with error
				strupr(filename);
				Error("Cannot open movie file <%s>",filename);
			}
			#endif
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

int movie_handle,movie_start;

//looks through a movie library for a movie file
//returns filehandle, with fileposition at movie, or -1 if can't find
int search_movie_lib(movielib *lib,char *filename,int must_have, int *lenp)
{
	int i;
	int filehandle;

	if (lib == NULL)
		return -1;

	for (i=0;i<lib->n_movies;i++)
		if (!stricmp(filename,lib->movies[i].name)) {	//found the movie in a library 
			int from_cd;

			*lenp = lib->movies[i].len;

			from_cd = (lib->flags & MLF_ON_CD);

			if (from_cd)
				songs_stop_redbook();		//ready to read from CD

			do {		//keep trying until we get the file handle

				movie_handle = filehandle = open(lib->name, O_RDONLY);

				if (must_have && from_cd && filehandle == -1) {		//didn't get file!

					if (request_cd() == -1)		//ESC from requester
						break;						//bail from here. will get error later
				}

			} while (must_have && from_cd && filehandle == -1);

			if (filehandle != -1)
				lseek(filehandle,(movie_start=lib->movies[i].offset),SEEK_SET);

			return filehandle;
		}

	return -1;
}

//returns file handle
int open_movie_file(char *filename,int must_have, int *lenp)
{
	int filehandle,i;

	for (i=0;i<N_MOVIE_LIBS;i++) {

		if ((filehandle = search_movie_lib(movie_libs[i],filename,must_have, lenp)) != -1)
			return filehandle;
	}

	return -1;		//couldn't find it
}
