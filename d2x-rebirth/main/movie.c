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

#include <string.h>
#ifndef macintosh
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# ifndef _MSC_VER
#  include <unistd.h>
# endif
#endif // ! macintosh
#include <ctype.h>

#include "movie.h"
#include "window.h"
#include "console.h"
#include "config.h"
#include "key.h"
#include "mouse.h"
#include "digi.h"
#include "songs.h"
#include "inferno.h"
#include "palette.h"
#include "strutil.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "byteswap.h"
#include "gr.h"
#include "gamefont.h"
#include "menu.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"
#include "physfsrwops.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "args.h"

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

SDL_RWops *RoboFile;

// Function Prototypes
int RunMovie(char *filename, int highres_flag, int allow_abort,int dx,int dy);

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
	int ret;

	if (GameArg.SysNoMovies)
		return MOVIE_NOT_PLAYED;

	strcpy(name,filename);

	if ((p=strchr(name,'.')) == NULL)		//add extension, if missing
		strcat(name,".mve");

	// Stop all digital sounds currently playing.
	digi_stop_digi_sounds();

	// Stop all songs
	songs_stop_all();

	// MD2211: if using SDL_Mixer, we never reinit the sound system
#ifdef USE_SDLMIXER
	if (GameArg.SndDisableSdlMixer)
#endif
		digi_close();

	// Start sound
	MVE_sndInit(!GameArg.SndNoSound ? 1 : -1);

	ret = RunMovie(name, GameArg.GfxMovieHires, must_have, -1, -1);

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (!GameArg.SndNoSound
#ifdef USE_SDLMIXER
		&& GameArg.SndDisableSdlMixer
#endif
	)
		digi_init();

	Screen_mode = -1;		//force screen reset

	return ret;
}

void MovieShowFrame(ubyte *buf, int dstx, int dsty, int bufw, int bufh, int sw, int sh)
{
	grs_bitmap source_bm;
	static ubyte old_pal[768];
	float scale = 1.0;

	if (memcmp(old_pal,gr_palette,768))
	{
		memcpy(old_pal,gr_palette,768);
		return;
	}
	memcpy(old_pal,gr_palette,768);

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = bufw;
	source_bm.bm_h = bufh;
	source_bm.bm_type = BM_LINEAR;
	source_bm.bm_flags = 0;
	source_bm.bm_data = buf;

	if (dstx == -1 && dsty == -1) // Fullscreen movie so set scale to fit the actual screen size
	{
		if (((float)SWIDTH/SHEIGHT) < ((float)sw/bufh))
			scale = ((float)SWIDTH/sw);
		else
			scale = ((float)SHEIGHT/bufh);
	}
	else // Other (robot) movie so set scale to min. screen dimension
	{
		if (((float)SWIDTH/bufw) < ((float)SHEIGHT/bufh))
			scale = ((float)SWIDTH/sw);
		else
			scale = ((float)SHEIGHT/sh);
	}

	if (dstx == -1) // center it
		dstx = (SWIDTH/2)-((bufw*scale)/2);
	if (dsty == -1) // center it
		dsty = (SHEIGHT/2)-((bufh*scale)/2);

#ifdef OGL
	glDisable (GL_BLEND);

	ogl_ubitblt_i(
		bufw*scale, bufh*scale,
		dstx, dsty,
		bufw, bufh, 0, 0, &source_bm,&grd_curcanv->cv_bitmap,GameCfg.MovieTexFilt);

	glEnable (GL_BLEND);
#else
	gr_bm_ubitbltm(bufw,bufh,dstx,dsty,0,0,&source_bm,&grd_curcanv->cv_bitmap);
#endif
}

//our routine to set the pallete, called from the movie code
void MovieSetPalette(unsigned char *p, unsigned start, unsigned count)
{
	if (count == 0)
		return;

	//Color 0 should be black, and we get color 255
	Assert(start>=1 && start+count-1<=254);

	//Set color 0 to be black
	gr_palette[0] = gr_palette[1] = gr_palette[2] = 0;

	//Set color 255 to be our subtitle color
	gr_palette[765] = gr_palette[766] = gr_palette[767] = 50;

	//movie libs palette into our array
	memcpy(gr_palette+start*3,p+start*3,count*3);
}


typedef struct movie
{
	int result, aborted;
	int frame_num;
	int paused;
} movie;

int show_pause_message(window *wind, d_event *event, void *userdata)
{
	userdata = userdata;

	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
			if (event_mouse_get_button(event) != 0)
				return 0;
			// else fall through

		case EVENT_KEY_COMMAND:
			if (!call_default_handler(event))
				window_close(wind);
			return 1;

		case EVENT_WINDOW_DRAW:
		{
			char *msg = TXT_PAUSE;
			int w,h,aw;
			int y;

			gr_set_current_canvas(NULL);
			gr_set_curfont( GAME_FONT );

			gr_get_string_size(msg,&w,&h,&aw);

			y = (grd_curscreen->sc_h-h)/2;

			gr_set_fontcolor( 255, -1 );

			gr_ustring( 0x8000, y, msg );
			break;
		}

		default:
			break;
	}

	return 0;
}

int MovieHandler(window *wind, d_event *event, movie *m)
{
	int key;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			m->paused = 0;
			break;

		case EVENT_WINDOW_DEACTIVATED:
			m->paused = 1;
			MVE_rmHoldMovie();
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			// If ESCAPE pressed, then quit movie.
			if (key == KEY_ESC) {
				m->result = m->aborted = 1;
				window_close(wind);
				return 1;
			}

			// If PAUSE pressed, then pause movie
			if ((key == KEY_PAUSE) || (key == KEY_COMMAND + KEY_P))
			{
				if (window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, show_pause_message, NULL))
					MVE_rmHoldMovie();
				return 1;
			}
			break;

		case EVENT_WINDOW_DRAW:
			if (!m->paused)
			{
				m->result = MVE_rmStepMovie();
				if (m->result)
				{
					window_close(wind);
					return 1;
				}
			}

			draw_subtitles(m->frame_num);

			gr_palette_load(gr_palette);

			if (!m->paused)
				m->frame_num++;
			break;

		case EVENT_WINDOW_CLOSE:
			if (Quitting)
				m->result = m->aborted = 1;
			break;
			
		default:
			break;
	}

	return 0;
}

//returns status.  see movie.h
int RunMovie(char *filename, int hires_flag, int must_have,int dx,int dy)
{
	window *wind;
	movie *m;
	SDL_RWops *filehndl;
	int track = 0;
	int aborted = 0;
	int reshow = 0;
#ifdef OGL
	ubyte pal_save[768];
#endif

	MALLOC(m, movie, 1);
	if (!m)
		return MOVIE_NOT_PLAYED;

	m->result = 1;
	m->aborted = 0;
	m->frame_num = 0;
	m->paused = 0;

	reshow = hide_menus();

	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))MovieHandler, m);
	if (!wind)
	{
		if (reshow)
			show_menus();
		d_free(m);
		return MOVIE_NOT_PLAYED;
	}

	// Open Movie file.  If it doesn't exist, no movie, just return.

	filehndl = PHYSFSRWOPS_openRead(filename);

	if (!filehndl)
	{
		if (must_have)
			con_printf(CON_URGENT, "Can't open movie <%s>: %s\n", filename, PHYSFS_getLastError());
		window_close(wind);
		if (reshow)
			show_menus();
		d_free(m);
		return MOVIE_NOT_PLAYED;
	}

	MVE_memCallbacks(MPlayAlloc, MPlayFree);
	MVE_ioCallbacks(FileRead);

#ifdef OGL
	set_screen_mode(SCREEN_MOVIE);
	gr_copy_palette(pal_save, gr_palette, 768);
	memset(gr_palette, 0, 768);
	gr_palette_load(gr_palette);
#else
	gr_set_mode(SM((hires_flag?640:320),(hires_flag?480:200)));
#endif
	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);

	if (MVE_rmPrepMovie((void *)filehndl, dx, dy, track)) {
		Int3();
		SDL_FreeRW(filehndl);
		window_close(wind);
		if (reshow)
			show_menus();
		d_free(m);
		return MOVIE_NOT_PLAYED;
	}

	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);

	while (window_exists(wind))
		event_process();

	Assert(m->aborted || m->result == MVE_ERR_EOF);	 ///movie should be over

    MVE_rmEndMovie();

	SDL_FreeRW(filehndl);                           // Close Movie File
	if (reshow)
		show_menus();
	aborted = m->aborted;
	d_free(m);

	// Restore old graphic state

	Screen_mode=-1;  //force reset of screen mode
#ifdef OGL
	gr_copy_palette(gr_palette, pal_save, 768);
	gr_palette_load(pal_save);
#endif

	return (aborted?MOVIE_ABORTED:MOVIE_PLAYED_FULL);
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
		if (MVE_rmPrepMovie(RoboFile, SWIDTH/2.3, SHEIGHT/2.3, 0))
		{
			Int3();
			return 0;
		}
		err = MVE_rmStepMovie();
	}
	if (err) {
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
	if (GameArg.SysNoMovies)
		return 0;

	con_printf(CON_DEBUG, "RoboFile=%s\n", filename);

	MVE_memCallbacks(MPlayAlloc, MPlayFree);
	MVE_ioCallbacks(FileRead);
	MVE_sfCallbacks(MovieShowFrame);
	MVE_palCallbacks(MovieSetPalette);
	MVE_sndInit(-1);        //tell movies to play no sound for robots

	RoboFile = PHYSFSRWOPS_openRead(filename);

	if (!RoboFile)
	{
		con_printf(CON_URGENT, "Can't open movie <%s>: %s\n", filename, PHYSFS_getLastError());
		return MOVIE_NOT_PLAYED;
	}

	Vid_State = VID_PLAY;

	if (MVE_rmPrepMovie((void *)RoboFile, SWIDTH/2.3, SHEIGHT/2.3, 0)) {
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
	PHYSFS_file *ifile;
	int size,read_count;
	char *p;
	int have_binary = 0;

	Num_subtitles = 0;

	if (!GameCfg.MovieSubtitles)
		return 0;

	ifile = PHYSFSX_openReadBuffered(filename);		//try text version

	if (!ifile) {								//no text version, try binary version
		char filename2[FILENAME_LEN];
		change_filename_extension(filename2, filename, ".txb");
		ifile = PHYSFSX_openReadBuffered(filename2);
		if (!ifile)
			return 0;
		have_binary = 1;
	}

	size = PHYSFS_fileLength(ifile);

	MALLOC (subtitle_raw_data, char, size+1);

	read_count = PHYSFS_read(ifile, subtitle_raw_data, 1, size);

	PHYSFS_close(ifile);

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
	static int num_active_subtitles,next_subtitle;
	int t,y;
	int must_erase=0;

	if (frame_num == 0) {
		num_active_subtitles = 0;
		next_subtitle = 0;
		gr_set_curfont( GAME_FONT );
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
	y = grd_curcanv->cv_bitmap.bm_h-((LINE_SPACING)*(MAX_ACTIVE_SUBTITLES+2));

	//erase old subtitles if necessary
	if (must_erase) {
		gr_setcolor(0);
		gr_rect(0,y,grd_curcanv->cv_bitmap.bm_w-1,grd_curcanv->cv_bitmap.bm_h-1);
	}

	//now draw the current subtitles
	for (t=0;t<num_active_subtitles;t++)
		if (active_subtitles[t] != -1) {
			gr_string(0x8000,y,Subtitles[active_subtitles[t]].msg);
			y += LINE_SPACING;
		}
}

void init_movie(char *movielib, int required)
{
	char filename[FILENAME_LEN+2];

	snprintf(filename, FILENAME_LEN+2, "%s-%s.mvl", movielib, GameArg.GfxMovieHires?"h":"l");

	if (!PHYSFSX_contfile_init(filename, 0))
	{
		if (required)
			con_printf(CON_URGENT, "Can't open movielib <%s>: %s\n", filename, PHYSFS_getLastError());
	}
}

//find and initialize the movie libraries
void init_movies()
{
	int i;

	if (GameArg.SysNoMovies)
		return;

	for (i=0;i<N_BUILTIN_MOVIE_LIBS;i++) {
		init_movie(movielib_files[i], 1);
	}
}


void close_extra_robot_movie(void)
{
	char filename[FILENAME_LEN+2];

	if (strcmp(movielib_files[EXTRA_ROBOT_LIB],"")) {
		snprintf(filename,FILENAME_LEN+2, "%s-%s.mvl", movielib_files[EXTRA_ROBOT_LIB], GameArg.GfxMovieHires?"h":"l");

		if (!PHYSFSX_contfile_close(filename))
		{
			con_printf(CON_URGENT, "Can't close movielib <%s>: %s\n", filename, PHYSFS_getLastError());
			snprintf(filename, FILENAME_LEN+2, "%s-%s.mvl", movielib_files[EXTRA_ROBOT_LIB], GameArg.GfxMovieHires?"l":"h");

			if (!PHYSFSX_contfile_close(filename))
				con_printf(CON_URGENT, "Can't close movielib <%s>: %s\n", filename, PHYSFS_getLastError());
		}
	}
}

void init_extra_robot_movie(char *movielib)
{
	if (GameArg.SysNoMovies)
		return;

	close_extra_robot_movie();
	init_movie(movielib, 0);
}
