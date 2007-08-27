/* $Id: titles.c,v 1.1.1.1 2006/03/17 19:57:46 zicodxx Exp $ */
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
 * Routines to display title screens...
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#ifdef MACINTOSH
#include <Events.h>
#endif

#include "pstypes.h"
#include "timer.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "joy.h"
#include "mono.h"
#include "gamefont.h"
#include "cfile.h"
#include "error.h"
#include "polyobj.h"
#include "textures.h"
#include "screens.h"
#include "multi.h"
#include "player.h"
#include "digi.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#include "movie.h"
#include "menu.h"
#include "mouse.h"


void set_briefing_fontcolor ();
int get_new_message_num(char **message);
int DefineBriefingBox (char **buf);

extern unsigned RobSX,RobSY,RobDX,RobDY; // Robot movie coords

char CurBriefScreenName[15]="brief03.pcx";
char	* Briefing_text;
char RobotPlaying=0;
grs_bitmap briefing_bm;

//Begin D1X modification
#define MAX_BRIEFING_COLORS     7
//End D1X modification

#define	SHAREWARE_ENDING_FILENAME	"ending.tex"

int	Briefing_foreground_colors[MAX_BRIEFING_COLORS], Briefing_background_colors[MAX_BRIEFING_COLORS];
int	Current_color = 0;
int	Erase_color;

extern int check_button_press();

#ifdef MACINTOSH
extern void macintosh_quit(void);
#endif

// added by Jan Bobrowski for variable-size menu screen
static int rescale_x(int x)
{
	return x * GWIDTH / 320;
}

static int rescale_y(int y)
{
	return y * GHEIGHT / 200;
}

#ifndef MACINTOSH
int local_key_inkey(void)
{
	int	rval;

	rval = key_inkey();

	if (rval == KEY_PRINT_SCREEN) {
		save_screen_shot(0);
		return 0;				//say no key pressed
	}

	if (check_button_press())		//joystick or mouse button pressed?
		rval = KEY_SPACEBAR;
	else if (mouse_button_state(0))
		rval = KEY_SPACEBAR;

	#ifdef MACINTOSH
	if ( rval == KEY_Q+KEY_COMMAND )
		macintosh_quit();
	#endif

	return rval;
}
#else
int local_key_inkey(void)
{
	EventRecord event;
	int	rval;

	if (!GetOSEvent(everyEvent, &event))
		return 0;

	if (event.what != keyDown)
		return 0;

	rval = (int)((event.message & keyCodeMask) >> 8);

	if (rval == KEY_PRINT_SCREEN) {
		save_screen_shot(0);
		return 0;				//say no key pressed
	}

	if (check_button_press())		//joystick or mouse button pressed?
		rval = KEY_SPACEBAR;

	#ifdef MACINTOSH
	if ( rval == KEY_Q+KEY_COMMAND )
		macintosh_quit();
	#endif

	return rval;
}
#endif

int show_title_screen( char * filename, int allow_keys, int from_hog_only )
{
	fix timer;
	int pcx_error;
	grs_bitmap title_bm;
	char new_filename[FILENAME_LEN+1] = "";

#ifdef RELEASE
	if (from_hog_only)
		strcpy(new_filename,"\x01");	//only read from hog file
#endif

	strcat(new_filename,filename);
	filename = new_filename;

	title_bm.bm_data=NULL;
	if ((pcx_error=pcx_read_bitmap( filename, &title_bm, BM_LINEAR, gr_palette ))!=PCX_ERROR_NONE)	{
		printf( "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n",filename, pcx_errormsg(pcx_error), pcx_error);
		mprintf((0, "File '%s', PCX load error: %s (%i)\n  (No big deal, just no title screen.)\n",filename, pcx_errormsg(pcx_error), pcx_error));
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",filename, pcx_errormsg(pcx_error), pcx_error);
	}

	gr_set_current_canvas( NULL );

	timer = timer_get_fixed_seconds() + i2f(3);

	gr_palette_load( gr_palette );

	while (1) {
		gr_flip();
		show_fullscr(&title_bm);
		gr_update();

		if (allow_keys) {
			gr_free_bitmap_data (&title_bm);
			return 1;
		}

		if ( local_key_inkey() && allow_keys ) break;
		if ( timer_get_fixed_seconds() > timer ) break;

		timer_delay(400);
        }

	gr_free_bitmap_data (&title_bm);

	return 0;
}

int intro_played = 0;

void show_titles(void)
{
#ifndef SHAREWARE
	int played=MOVIE_NOT_PLAYED;    //default is not played
#endif
	int song_playing = 0;

#ifdef D2_OEM
#define MOVIE_REQUIRED 0
#else
#define MOVIE_REQUIRED 1
#endif

	{       //show bundler screens
		char filename[FILENAME_LEN];

		played=MOVIE_NOT_PLAYED;        //default is not played

		played = PlayMovie("pre_i.mve",0);

		if (!played) {
			strcpy(filename,HiresGFX?"pre_i1b.pcx":"pre_i1.pcx");

			while (PHYSFS_exists(filename))
			{
				show_title_screen( filename, 1, 0 );
				filename[5]++;
			}
		}
	}

#ifndef SHAREWARE
	init_subtitles("intro.tex");
	played = PlayMovie("intro.mve",MOVIE_REQUIRED);
	close_subtitles();
#endif

	if (played != MOVIE_NOT_PLAYED)
		intro_played = 1;
	else
	{                                               //didn't get intro movie, try titles

		played = PlayMovie("titles.mve",MOVIE_REQUIRED);

		if (played == MOVIE_NOT_PLAYED)
		{
			char filename[FILENAME_LEN];

			con_printf( CON_DEBUG, "\nPlaying title song..." );
			songs_play_song( SONG_TITLE, 1);
			song_playing = 1;
			con_printf( CON_DEBUG, "\nShowing logo screens..." );

			strcpy(filename, HiresGFX?"iplogo1b.pcx":"iplogo1.pcx"); // OEM
			if (! cfexist(filename))
				strcpy(filename, "iplogo1.pcx"); // SHAREWARE
			if (! cfexist(filename))
				strcpy(filename, "mplogo.pcx"); // MAC SHAREWARE
			if (cfexist(filename))
				show_title_screen(filename, 1, 1);

			strcpy(filename, HiresGFX?"logob.pcx":"logo.pcx"); // OEM
			if (! cfexist(filename))
				strcpy(filename, "logo.pcx"); // SHAREWARE
			if (! cfexist(filename))
				strcpy(filename, "plogo.pcx"); // MAC SHAREWARE
			if (cfexist(filename))
				show_title_screen(filename, 1, 1);
		}
	}

	{       //show bundler movie or screens

		char filename[FILENAME_LEN];
		PHYSFS_file *movie_handle;

		played=MOVIE_NOT_PLAYED;        //default is not played

		//check if OEM movie exists, so we don't stop the music if it doesn't
		movie_handle = PHYSFS_openRead("oem.mve");
		if (movie_handle)
		{
			PHYSFS_close(movie_handle);
			played = PlayMovie("oem.mve",0);
			song_playing = 0;               //movie will kill sound
		}

		if (!played)
		{
			strcpy(filename,HiresGFX?"oem1b.pcx":"oem1.pcx");

			while (PHYSFS_exists(filename))
			{
				show_title_screen( filename, 1, 0 );
				filename[3]++;
			}
		}
	}

	if (!song_playing)
		songs_play_song( SONG_TITLE, 1);
}

typedef struct {
	char    bs_name[14];                //  filename, eg merc01.  Assumes .lbm suffix.
	sbyte   level_num;
	sbyte   message_num;
	short   text_ulx, text_uly;         //  upper left x,y of text window
	short   text_width, text_height;    //  width and height of text window
} briefing_screen;

#define BRIEFING_SECRET_NUM 31          //  This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM 4           // This must correspond to the first level screen (ie, past the bald guy briefing screens)

#define	SHAREWARE_ENDING_LEVEL_NUM  0x7f
#define	REGISTERED_ENDING_LEVEL_NUM 0x7e

#ifdef SHAREWARE
#define ENDING_LEVEL_NUM 	SHAREWARE_ENDING_LEVEL_NUM
#else
#define ENDING_LEVEL_NUM 	REGISTERED_ENDING_LEVEL_NUM
#endif

#define MAX_BRIEFING_SCREENS 60

briefing_screen Briefing_screens[MAX_BRIEFING_SCREENS]=
 {{"brief03.pcx",0,3,8,8,257,177}}; // default=0!!!

briefing_screen D1_Briefing_screens[] = {
	{ "brief01.pcx",   0,  1,  13, 140, 290,  59 },
	{ "brief02.pcx",   0,  2,  27,  34, 257, 177 },
	{ "brief03.pcx",   0,  3,  20,  22, 257, 177 },
	{ "brief02.pcx",   0,  4,  27,  34, 257, 177 },

	{ "moon01.pcx",    1,  5,  10,  10, 300, 170 }, // level 1
	{ "moon01.pcx",    2,  6,  10,  10, 300, 170 }, // level 2
	{ "moon01.pcx",    3,  7,  10,  10, 300, 170 }, // level 3

	{ "venus01.pcx",   4,  8,  15, 15, 300,  200 }, // level 4
	{ "venus01.pcx",   5,  9,  15, 15, 300,  200 }, // level 5

	{ "brief03.pcx",   6, 10,  20,  22, 257, 177 },
	{ "merc01.pcx",    6, 11,  10, 15, 300, 200 },  // level 6
	{ "merc01.pcx",    7, 12,  10, 15, 300, 200 },  // level 7

#ifndef SHAREWARE
	{ "brief03.pcx",   8, 13,  20,  22, 257, 177 },
	{ "mars01.pcx",    8, 14,  10, 100, 300,  200 }, // level 8
	{ "mars01.pcx",    9, 15,  10, 100, 300,  200 }, // level 9
	{ "brief03.pcx",  10, 16,  20,  22, 257, 177 },
	{ "mars01.pcx",   10, 17,  10, 100, 300,  200 }, // level 10

	{ "jup01.pcx",    11, 18,  10, 40, 300,  200 }, // level 11
	{ "jup01.pcx",    12, 19,  10, 40, 300,  200 }, // level 12
	{ "brief03.pcx",  13, 20,  20,  22, 257, 177 },
	{ "jup01.pcx",    13, 21,  10, 40, 300,  200 }, // level 13
	{ "jup01.pcx",    14, 22,  10, 40, 300,  200 }, // level 14

	{ "saturn01.pcx", 15, 23,  10, 40, 300,  200 }, // level 15
	{ "brief03.pcx",  16, 24,  20,  22, 257, 177 },
	{ "saturn01.pcx", 16, 25,  10, 40, 300,  200 }, // level 16
	{ "brief03.pcx",  17, 26,  20,  22, 257, 177 },
	{ "saturn01.pcx", 17, 27,  10, 40, 300,  200 }, // level 17

	{ "uranus01.pcx", 18, 28,  100, 100, 300,  200 }, // level 18
	{ "uranus01.pcx", 19, 29,  100, 100, 300,  200 }, // level 19
	{ "uranus01.pcx", 20, 30,  100, 100, 300,  200 }, // level 20
	{ "uranus01.pcx", 21, 31,  100, 100, 300,  200 }, // level 21

	{ "neptun01.pcx", 22, 32,  10, 20, 300,  200 }, // level 22
	{ "neptun01.pcx", 23, 33,  10, 20, 300,  200 }, // level 23
	{ "neptun01.pcx", 24, 34,  10, 20, 300,  200 }, // level 24

	{ "pluto01.pcx",  25, 35,  10, 20, 300,  200 }, // level 25
	{ "pluto01.pcx",  26, 36,  10, 20, 300,  200 }, // level 26
	{ "pluto01.pcx",  27, 37,  10, 20, 300,  200 }, // level 27

	{ "aster01.pcx",  -1, 38,  10, 90, 300,  200 }, // secret level -1
	{ "aster01.pcx",  -2, 39,  10, 90, 300,  200 }, // secret level -2
	{ "aster01.pcx",  -3, 40,  10, 90, 300,  200 }, // secret level -3
#endif

	{ "end01.pcx",   SHAREWARE_ENDING_LEVEL_NUM,  1,  23, 40, 320, 200 },   // shareware end
#ifndef SHAREWARE
	{ "end02.pcx",   REGISTERED_ENDING_LEVEL_NUM,  1,  5, 5, 300, 200 },    // registered end
	{ "end01.pcx",   REGISTERED_ENDING_LEVEL_NUM,  2,  23, 40, 320, 200 },  // registered end
	{ "end03.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,  5, 5, 300, 200 },    // registered end
#endif

};

#define NUM_D1_BRIEFING_SCREENS (sizeof(D1_Briefing_screens)/sizeof(briefing_screen))

int	Briefing_text_x, Briefing_text_y;

void init_char_pos(int x, int y)
{
	Briefing_text_x = x;
	Briefing_text_y = y;
	mprintf ((0,"Setting init x=%d y=%d\n",x,y));
}

grs_canvas	*Robot_canv = NULL;
vms_angvec	Robot_angles;

char    Bitmap_name[32] = "";
#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      // Adam: This is the number of frames in your new animating thing.
#define DOOR_DIV_INIT   6
sbyte   Door_dir=1, Door_div_count=0, Animating_bitmap_type=0;

//-----------------------------------------------------------------------------
void show_animated_bitmap(void)
{
        grs_canvas      *curcanv_save, *bitmap_canv=0;
	grs_bitmap	*bitmap_ptr;

// 	gr_use_palette_table( "groupa.256" );

	// Only plot every nth frame.
	if (Door_div_count) {
#ifdef OGL
		if (Bitmap_name[0] != 0) {
			bitmap_index bi;
			bi = piggy_find_bitmap(Bitmap_name);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
			ogl_ubitmapm_cs(rescale_x(220), rescale_y(45),(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
		}
#endif
		Door_div_count--;
		return;
	}

	Door_div_count = DOOR_DIV_INIT;

	if (Bitmap_name[0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;
		bitmap_index bi;

		switch (Animating_bitmap_type) {
			case 0:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 64, 64);	break;
			case 1:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 94, 94);	break; // Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:	Int3(); // Impossible, illegal value for Animating_bitmap_type
		}

		curcanv_save = grd_curcanv;
		grd_curcanv = bitmap_canv;

		pound_signp = strchr(Bitmap_name, '#');
		Assert(pound_signp != NULL);

		dig1 = *(pound_signp+1);
		dig2 = *(pound_signp+2);
		if (dig2 == 0)
			num = dig1-'0';
		else
			num = (dig1-'0')*10 + (dig2-'0');

		switch (Animating_bitmap_type) {
			case 0:
				num += Door_dir;
				if (num > EXIT_DOOR_MAX) {
					num = EXIT_DOOR_MAX;
					Door_dir = -1;
				} else if (num < 0) {
					num = 0;
					Door_dir = 1;
				}
				break;
			case 1:
				num++;
				if (num > OTHER_THING_MAX)
					num = 0;
				break;
		}

		Assert(num < 100);
		if (num >= 10) {
			*(pound_signp+1) = (num / 10) + '0';
			*(pound_signp+2) = (num % 10) + '0';
			*(pound_signp+3) = 0;
		} else {
			*(pound_signp+1) = (num % 10) + '0';
			*(pound_signp+2) = 0;
		}

		bi = piggy_find_bitmap(Bitmap_name);
		bitmap_ptr = &GameBitmaps[bi.index];
		PIGGY_PAGE_IN( bi );
#ifdef OGL
		ogl_ubitmapm_cs(0,0,(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
		gr_bitmapm(0, 0, bitmap_ptr);
#endif
		grd_curcanv = curcanv_save;
		d_free(bitmap_canv);

		switch (Animating_bitmap_type) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					Door_dir = -1;
					Door_div_count = 64;
				} else if (num == 0) {
					Door_dir = 1;
					Door_div_count = 64;
				}
				break;
			case 1:
				break;
		}
	}
}

//-----------------------------------------------------------------------------
void show_briefing_bitmap(grs_bitmap *bmp)
{
	grs_canvas	*curcanv_save, *bitmap_canv;

	bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220, 45, bmp->bm_w, bmp->bm_h);
	curcanv_save = grd_curcanv;
	gr_set_current_canvas(bitmap_canv);
	gr_bitmapm(0, 0, bmp);
	gr_set_current_canvas(curcanv_save);

	d_free(bitmap_canv);
}

//-----------------------------------------------------------------------------
void show_spinning_robot_frame(int robot_num)
{
	grs_canvas	*curcanv_save;

	if (robot_num != -1) {
		Robot_angles.h += 150;

		curcanv_save = grd_curcanv;
		grd_curcanv = Robot_canv;
		Assert(Robot_info[robot_num].model_num != -1);
		draw_model_picture(Robot_info[robot_num].model_num, &Robot_angles);
		grd_curcanv = curcanv_save;
	}

}

//-----------------------------------------------------------------------------
void init_spinning_robot(void) //(int x,int y,int w,int h)
{
	int x = rescale_x(138);
	int y = rescale_y(55);
	int w = rescale_x(166);
	int h = rescale_y(138);

	Robot_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
}

//---------------------------------------------------------------------------
// Returns char width.
// If show_robot_flag set, then show a frame of the spinning robot.
int show_char_delay(char the_char, int delay, int robot_num, int cursor_flag)
{
	int w, h, aw;
	char message[2];
	static fix	start_time=0;

	message[0] = the_char;
	message[1] = 0;

	if (start_time==0 && timer_get_fixed_seconds()<0)
		start_time=timer_get_fixed_seconds();

	gr_get_string_size(message, &w, &h, &aw );

	Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));

	//	Draw cursor if there is some delay and caller says to draw cursor
	if (cursor_flag && delay) {
		gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
		gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
		gr_update();
	}

	if ((Bitmap_name[0] != 0) && (delay != 0))
		show_animated_bitmap();

	if (RobotPlaying && (delay != 0))
		RotateRobot();

	while (timer_get_fixed_seconds() < (start_time + delay/2)) {
		if (RobotPlaying && delay != 0)
			RotateRobot();
	}
	if (robot_num != -1)
		show_spinning_robot_frame(robot_num);

	start_time = timer_get_fixed_seconds();

	//	Erase cursor
	if (cursor_flag && delay) {
		gr_set_fontcolor(Erase_color, -1);
		gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
	}

	//	Draw the character
	gr_set_fontcolor(Briefing_background_colors[Current_color], -1);
	gr_printf(Briefing_text_x, Briefing_text_y, message );

	gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
	gr_printf(Briefing_text_x+1, Briefing_text_y, message );

	if (delay) gr_update();

	return w;
}

//	-----------------------------------------------------------------------------
//	loads a briefing screen
//	also takes care for D1 emulation palette remapping in robot screens
//	for OGL, redrawing of briefing_bm is done in show_briefing
int load_briefing_screen( char *fname )
{
	int pcx_error;
	ubyte old_pal[256*3];

	if (briefing_bm.bm_data != NULL)
		gr_free_bitmap_data(&briefing_bm);

	gr_init_bitmap_data(&briefing_bm);

	strcpy (CurBriefScreenName,fname);

	if ((pcx_error = pcx_read_bitmap( fname, &briefing_bm, BM_LINEAR,gr_palette))!=PCX_ERROR_NONE)   {
		printf( "File '%s', PCX load error: %s (%i)\n  (It's a briefing screen.  Does this cause you pain?)\n",fname, pcx_errormsg(pcx_error), pcx_error);
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",fname, pcx_errormsg(pcx_error), pcx_error);
	}

	memcpy(old_pal,gr_palette,sizeof(old_pal));

	if (EMULATING_D1 && stricmp(fname,"brief03.pcx") == 0)
	{
		gr_use_palette_table( "groupa.256" );
		gr_remap_bitmap_good( &briefing_bm, old_pal, -1, -1 );
	}

	show_fullscr(&briefing_bm);

	gr_palette_load(gr_palette);

	set_briefing_fontcolor();

	return 1;
}



#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

//-----------------------------------------------------------------------------
int get_message_num(char **message)
{
	int	num=0;

	while (**message == ' ')
		(*message)++;

	while ((**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

	while (*(*message)++ != 10)		//	Get and drop eoln
		;

	return num;
}

//-----------------------------------------------------------------------------
void get_message_name(char **message, char *result)
{
	while (**message == ' ')
		(*message)++;

	while ((**message != ' ') && (**message != 10)) {
		if (**message != '\n')
			*result++ = **message;
		(*message)++;
	}

	if (**message != 10)
		while (*(*message)++ != 10)		//	Get and drop eoln
			;

	*result = 0;
}

//-----------------------------------------------------------------------------
void flash_cursor(int cursor_flag)
{
	if (cursor_flag == 0)
		return;

	if ((timer_get_fixed_seconds() % (F1_0/2) ) > (F1_0/4))
		gr_set_fontcolor(Briefing_foreground_colors[Current_color], -1);
	else
		gr_set_fontcolor(Erase_color, -1);

	gr_printf(Briefing_text_x+1, Briefing_text_y, "_" );
	gr_update();
}

#ifdef OGL
typedef struct msgstream {
	int x;
	int y;
	int color;
	int ch;
} __pack__ msgstream;

msgstream messagestream[2048];

void redraw_messagestream(int count)
{
	char msgbuf[2];
	int i;

	for (i=0; i<count; i++) {
		msgbuf[0] = messagestream[i].ch;
		msgbuf[1] = 0;
		if (messagestream[i-1].color != messagestream[i].color)
			gr_set_fontcolor(messagestream[i].color,-1);
		gr_printf(messagestream[i].x+1,messagestream[i].y,"%s",msgbuf);
	}
}
#endif

//	-----------------------------------------------------------------------------
//	Return true if message got aborted by user (pressed ESC), else return false.
//	Draws text, images and bitmaps actually
int show_briefing(int screen_num, char *message)
{
	int	prev_ch=-1;
	int	ch, done=0,i;
	briefing_screen	*bsp;
	int	delay_count = KEY_DELAY_DEFAULT;
	int	key_check;
	int	robot_num=-1;
	int	rval=0;
	static int tab_stop=0;
	int	flashing_cursor=0;
	int	new_page=0,GotZ=0;
	char spinRobotName[]="rba.mve",kludge;  // matt don't change this!
	char fname[15];
	char DumbAdjust=0;
	char chattering=0;
	int hum_channel=-1,printing_channel=-1;
	int LineAdjustment=1;
	grs_bitmap  guy_bitmap;
#ifdef OGL
	int streamcount=0, guy_bitmap_show=0;
#endif

// 	if (EMULATING_D1)
// 		fname = Briefing_screens[screen_num].bs_name;
// 	else
// 		fname = CurBriefScreenName;
	if (EMULATING_D1)
		strncpy(fname, Briefing_screens[screen_num].bs_name, 15);//fname = Briefing_screens[screen_num].bs_name;
	else
		strncpy(fname, CurBriefScreenName, 15);//fname = CurBriefScreenName;


	Bitmap_name[0] = 0;
	Current_color = 0;
	RobotPlaying=0;

	#ifndef SHAREWARE
	hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
	#endif

	// mprintf((0, "Going to print message [%s] at x=%i, y=%i\n", message, x, y));
	gr_set_curfont( GAME_FONT );

	if (EMULATING_D1) {
		GotZ = 1;
		MALLOC(bsp, briefing_screen, 1);
		memcpy(bsp, &Briefing_screens[screen_num], sizeof(briefing_screen));
		bsp->text_ulx = rescale_x(bsp->text_ulx);
		bsp->text_uly = rescale_y(bsp->text_uly);
		bsp->text_width = rescale_x(bsp->text_width);
		bsp->text_height = rescale_y(bsp->text_height);
		init_char_pos(bsp->text_ulx, bsp->text_uly);
	} else {
		bsp=&Briefing_screens[0];
		init_char_pos(bsp->text_ulx, bsp->text_uly-(8*(1+HiresGFX)));
	}

	while (!done) {
		ch = *message++;
		if (ch == '$') {
			ch = *message++;
			if (ch=='D') {
				screen_num=DefineBriefingBox (&message);
				bsp = &Briefing_screens[screen_num];
				init_char_pos(bsp->text_ulx, bsp->text_uly);
				LineAdjustment=0;
				prev_ch = 10;                                   // read to eoln
			} else if (ch=='U') {
				screen_num=get_message_num(&message);
				bsp = &Briefing_screens[screen_num];
				init_char_pos(bsp->text_ulx, bsp->text_uly);
				prev_ch = 10;                                   // read to eoln
			} else if (ch == 'C') {
				Current_color = get_message_num(&message)-1;
				Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				prev_ch = 10;
			} else if (ch == 'F') {     // toggle flashing cursor
				flashing_cursor = !flashing_cursor;
				prev_ch = 10;
				while (*message++ != 10)
					;
			} else if (ch == 'T') {
				tab_stop = get_message_num(&message);
				tab_stop*=(1+HiresGFX);
				prev_ch = 10;							//	read to eoln
			} else if (ch == 'R') {
				if (Robot_canv != NULL) {
					d_free(Robot_canv);
					Robot_canv=NULL;
				}
				if (RobotPlaying) {
					DeInitRobotMovie();
					RobotPlaying=0;
				}

				if (EMULATING_D1) {
					init_spinning_robot();
					robot_num = get_message_num(&message);
					while (*message++ != 10)
						;
				} else {
					kludge=*message++;
					spinRobotName[2]=kludge; // ugly but proud

					RobotPlaying=InitRobotMovie(spinRobotName);

					// gr_remap_bitmap_good( &grd_curcanv->cv_bitmap, pal, -1, -1 );

					if (RobotPlaying) {
						RotateRobot();
						set_briefing_fontcolor ();
						mprintf ((0,"Robot playing is %d!!!",RobotPlaying));
					}
				}
				prev_ch = 10;                           // read to eoln
			} else if (ch == 'N') {
				//--grs_bitmap *bitmap_ptr;
				if (Robot_canv != NULL) {
					d_free(Robot_canv);
					Robot_canv=NULL;
				}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 0;
				prev_ch = 10;
			} else if (ch == 'O') {
				if (Robot_canv != NULL) {
						d_free(Robot_canv);
						Robot_canv=NULL;
				}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 1;
				prev_ch = 10;
			} else if (ch=='A') {
				LineAdjustment=1-LineAdjustment;
			} else if (ch=='Z') {
				//mprintf ((0,"Got a Z!\n"));
				GotZ=1;
				DumbAdjust=1;
				i=0;
				while ((fname[i]=*message) != '\n') {
					i++;
					message++;
				}
				fname[i]=0;
				if (*message != 10)
					while (*message++ != 10)    //  Get and drop eoln
						;

				{
					char fname2[15];

					i=0;
					while (fname[i]!='.') {
						fname2[i] = fname[i];
						i++;
					}
					fname2[i++]='b';
					fname2[i++]='.';
					fname2[i++]='p';
					fname2[i++]='c';
					fname2[i++]='x';
					fname2[i++]=0;

					if ((HiresGFX && cfexist(fname2)) || !cfexist(fname))
						strcpy(fname,fname2);
					load_briefing_screen (fname);
				}

			} else if (ch == 'B') {
				char        bitmap_name[32];
				ubyte       temp_palette[768];
				int         iff_error;

				if (Robot_canv != NULL) {
					d_free(Robot_canv);
					Robot_canv=NULL;
				}

				get_message_name(&message, bitmap_name);
				strcat(bitmap_name, ".bbm");
				guy_bitmap.bm_data = NULL;
				iff_error = iff_read_bitmap(bitmap_name, &guy_bitmap, BM_LINEAR, temp_palette);
				Assert(iff_error == IFF_NO_ERROR);
				gr_remap_bitmap_good( &guy_bitmap, temp_palette, -1, -1 );

				show_briefing_bitmap(&guy_bitmap);
#ifndef OGL
				d_free(guy_bitmap.bm_data);
#else
				guy_bitmap_show=1;
#endif
				prev_ch = 10;
			} else if (ch == 'S') {
				int keypress;
				fix start_time;

				chattering=0;
				if (printing_channel>-1)
					digi_stop_sound( printing_channel );
				printing_channel=-1;

				gr_update();

				start_time = timer_get_fixed_seconds();
				while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key

					while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
						;
#ifdef OGL
					if (!RobotPlaying)
						gr_flip();
					show_fullscr(&briefing_bm);
					redraw_messagestream(streamcount);
					if (guy_bitmap_show)
						show_briefing_bitmap(&guy_bitmap);
#endif
					flash_cursor(flashing_cursor);

					if (RobotPlaying)
						RotateRobot ();
					if (robot_num != -1)
						show_spinning_robot_frame(robot_num);

					if (Bitmap_name[0] != 0)
						show_animated_bitmap();
					start_time += KEY_DELAY_DEFAULT/2;
					timer_delay(400);
				}

#ifndef NDEBUG
				if (keypress == KEY_BACKSP)
					Int3();
#endif
				if (keypress == KEY_ESC)
					rval = 1;

				flashing_cursor = 0;
				done = 1;
			} else if (ch == 'P') {		//	New page.
				if (!GotZ) {
					Int3(); // Hey ryan!!!! You gotta load a screen before you start
					        // printing to it! You know, $Z !!!
		  		    load_briefing_screen (HiresGFX?"end01b.pcx":"end01.pcx");
				}

				new_page = 1;

				while (*message != 10) {
					message++;	//	drop carriage return after special escape sequence
				}
				message++;
				prev_ch = 10;
				gr_update();
			}
		} else if (ch == '\t') {		//	Tab
			if (Briefing_text_x - bsp->text_ulx < tab_stop)
				Briefing_text_x = bsp->text_ulx + tab_stop;
		} else if ((ch == ';') && (prev_ch == 10)) {
			while (*message++ != 10)
				;
			prev_ch = 10;
		} else if (ch == '\\') {
			prev_ch = ch;
		} else if (ch == 10) {
			if (prev_ch != '\\') {
				prev_ch = ch;
				if (DumbAdjust==0)
					Briefing_text_y += FONTSCALE_Y(8*(HiresGFX+1));
				else
					DumbAdjust--;
				Briefing_text_x = bsp->text_ulx;
				if (Briefing_text_y > bsp->text_uly + bsp->text_height) {
#ifndef OGL
					load_briefing_screen(Briefing_screens[screen_num].bs_name);
#endif
					Briefing_text_x = bsp->text_ulx;
					Briefing_text_y = bsp->text_uly;
				}
			} else {
				if (ch == 13)		//Can this happen? Above says ch==10
					Int3();
				prev_ch = ch;
			}

		} else {
			if (!GotZ) {
				Int3(); // Hey ryan!!!! You gotta load a screen before you start
				        // printing to it! You know, $Z !!!
				load_briefing_screen (HiresGFX?"end01b.pcx":"end01.pcx");
			}

#ifdef OGL
			messagestream[streamcount].x = Briefing_text_x;
			messagestream[streamcount].y = Briefing_text_y;
			messagestream[streamcount].color = Briefing_foreground_colors[Current_color];
			messagestream[streamcount].ch = ch;
			if (delay_count) {
				if (!RobotPlaying)
					gr_flip();
				show_fullscr(&briefing_bm);
				redraw_messagestream(streamcount);
			}
			if (guy_bitmap_show)
				show_briefing_bitmap(&guy_bitmap);
			streamcount++;
#endif

			prev_ch = ch;

			if (!chattering) {
		 		printing_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_PRINTING), F1_0, 0xFFFF/2, 1, -1, -1, -1 );
				chattering=1;
			}

			Briefing_text_x += show_char_delay(ch, delay_count, robot_num, flashing_cursor);

		}

		//	Check for Esc -> abort.
		if(delay_count)
			key_check=local_key_inkey();
		else
			key_check=0;

		if ( key_check == KEY_ESC ) {
			rval = 1;
			done = 1;
		}

		if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER))
			delay_count = 0;

		if ((key_check == KEY_ALTED+KEY_ENTER) ||
			(key_check == KEY_ALTED+KEY_PADENTER))
			gr_toggle_fullscreen();

		if (Briefing_text_x > bsp->text_ulx + bsp->text_width) {
			Briefing_text_x = bsp->text_ulx;
			Briefing_text_y += bsp->text_uly;
		}

		if ((new_page) || (Briefing_text_y > bsp->text_uly + bsp->text_height)) {
			fix	start_time = 0;
			int	keypress;

			new_page = 0;

			if (printing_channel>-1)
				digi_stop_sound( printing_channel );
			printing_channel=-1;

			chattering=0;

			start_time = timer_get_fixed_seconds();
			while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key
				while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
					;
#ifdef OGL
				if (!RobotPlaying)
					gr_flip();
				show_fullscr(&briefing_bm);
				redraw_messagestream(streamcount);
				if (guy_bitmap_show)
					show_briefing_bitmap(&guy_bitmap);
#endif
				flash_cursor(flashing_cursor);
				if (RobotPlaying)
					RotateRobot();
				if (robot_num != -1)
					show_spinning_robot_frame(robot_num);
				if (Bitmap_name[0] != 0)
					show_animated_bitmap();
				start_time += KEY_DELAY_DEFAULT/2;
				timer_delay(400);
			}

			if (RobotPlaying)
				DeInitRobotMovie();
			RobotPlaying=0;
			robot_num = -1;

#ifndef NDEBUG
			if (keypress == KEY_BACKSP)
				Int3();
#endif
			if (keypress == KEY_ESC) {
				rval = 1;
				done = 1;
			}
			load_briefing_screen(fname);
			Briefing_text_x = bsp->text_ulx;
			Briefing_text_y = bsp->text_uly;

#ifdef OGL
			streamcount=0;
			if (guy_bitmap_show) {
				gr_free_bitmap_data (&guy_bitmap);
				guy_bitmap_show=0;
			}
#endif

			delay_count = KEY_DELAY_DEFAULT;
		}
	}

	if (RobotPlaying) {
		DeInitRobotMovie();
		RobotPlaying=0;
	}

	if (Robot_canv != NULL)
		{d_free(Robot_canv); Robot_canv=NULL;}

	if (hum_channel>-1)
		digi_stop_sound( hum_channel );
	if (printing_channel>-1)
		digi_stop_sound( printing_channel );
	if (EMULATING_D1) {
		d_free(bsp);
	}

#ifdef OGL
	if (briefing_bm.bm_data != NULL)
		gr_free_bitmap_data (&briefing_bm);
#endif

	return rval;
}

//-----------------------------------------------------------------------------
// Return a pointer to the start of text for screen #screen_num.
char * get_briefing_message(int screen_num)
{
	char *tptr = Briefing_text;
	int	cur_screen=0;
	int	ch;

	Assert(screen_num >= 0);

	while ( (*tptr != 0 ) && (screen_num != cur_screen)) {
		ch = *tptr++;
		if (ch == '$') {
			ch = *tptr++;
			if (ch == 'S')
				cur_screen = get_message_num(&tptr);
		}
	}

	if (screen_num!=cur_screen)
		return (NULL);

	return tptr;
}

//-----------------------------------------------------------------------------
//	Load Descent briefing text.
int load_screen_text(char *filename, char **buf)
{
	CFILE *tfile;
	CFILE *ifile;
	int	len, i,x;
	int	have_binary = 0;

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		char nfilename[30], *ptr;

		strcpy(nfilename, filename);
		if ((ptr = strrchr(nfilename, '.')))
			*ptr = '\0';
		strcat(nfilename, ".txb");
		if ((ifile = cfopen(nfilename, "rb")) == NULL) {
			mprintf ((0,"can't open %s!\n",nfilename));
         		return (0);
				//Error("Cannot open file %s or %s", filename, nfilename);
		}

		mprintf ((0,"reading...\n"));
		have_binary = 1;

		len = cfilelength(ifile);
		MALLOC(*buf, char, len+500);
		mprintf ((0,"len=%d\n",len));
		for (x=0, i=0; i < len; i++, x++) {
			cfread (*buf+x,1,1,ifile);
			//  mprintf ((0,"%c",*(*buf+x)));
			if (*(*buf+x)==13)
				x--;
		}

		cfclose(ifile);
	} else {
		len = cfilelength(tfile);
		MALLOC(*buf, char, len+500);
		for (x=0, i=0; i < len; i++, x++) {
			cfread (*buf+x,1,1,tfile);
			// mprintf ((0,"%c",*(*buf+x)));
			if (*(*buf+x)==13)
				x--;
		}


		//cfread(*buf, 1, len, tfile);
		cfclose(tfile);
	}

	if (have_binary)
		decode_text(*buf, len);

	return (1);
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int show_briefing_text(int screen_num)
{
	char	*message_ptr;

	message_ptr = get_briefing_message
		(EMULATING_D1 ? Briefing_screens[screen_num].message_num : screen_num);

	if (message_ptr==NULL)
		return (0);

	set_briefing_fontcolor();

	return show_briefing(screen_num, message_ptr);
}

void set_briefing_fontcolor ()
{
	Briefing_foreground_colors[0] = gr_find_closest_color_current( 0, 40, 0);
	Briefing_background_colors[0] = gr_find_closest_color_current( 0, 6, 0);

	Briefing_foreground_colors[1] = gr_find_closest_color_current( 40, 33, 35);
	Briefing_background_colors[1] = gr_find_closest_color_current( 5, 5, 5);

	Briefing_foreground_colors[2] = gr_find_closest_color_current( 8, 31, 54);
	Briefing_background_colors[2] = gr_find_closest_color_current( 1, 4, 7);

	if (EMULATING_D1) {
		//green
		Briefing_foreground_colors[0] = gr_find_closest_color_current( 0, 54, 0);
		Briefing_background_colors[0] = gr_find_closest_color_current( 0, 19, 0);
		//white
		Briefing_foreground_colors[1] = gr_find_closest_color_current( 42, 38, 32);
		Briefing_background_colors[1] = gr_find_closest_color_current( 14, 14, 14);

		//Begin D1X addition
		//red
		Briefing_foreground_colors[2] = gr_find_closest_color_current( 63, 0, 0);
		Briefing_background_colors[2] = gr_find_closest_color_current( 31, 0, 0);
	}

	if (RobotPlaying)
	{
		Briefing_foreground_colors[0] = gr_find_closest_color_current( 0, 31, 0);
		Briefing_background_colors[0] = gr_find_closest_color_current( 0, 19, 0);
	}

	//blue
	Briefing_foreground_colors[3] = gr_find_closest_color_current( 0, 0, 54);
	Briefing_background_colors[3] = gr_find_closest_color_current( 0, 0, 19);
	//gray
	Briefing_foreground_colors[4] = gr_find_closest_color_current( 14, 14, 14);
	Briefing_background_colors[4] = gr_find_closest_color_current( 0, 0, 0);
	//yellow
	Briefing_foreground_colors[5] = gr_find_closest_color_current( 54, 54, 0);
	Briefing_background_colors[5] = gr_find_closest_color_current( 19, 19, 0);
	//purple
	Briefing_foreground_colors[6] = gr_find_closest_color_current( 0, 54, 54);
	Briefing_background_colors[6] = gr_find_closest_color_current( 0, 19, 19);
	//End D1X addition

	Erase_color = gr_find_closest_color_current(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int show_briefing_screen( int screen_num, int allow_keys)
{
	int     rval=0;

	if (EMULATING_D1) {
		load_briefing_screen(Briefing_screens[screen_num].bs_name);

		gr_palette_clear();
#ifndef OGL
		show_fullscr(&briefing_bm );
#endif
		gr_palette_load(gr_palette);
		gr_update();

	}

	rval = show_briefing_text(screen_num);

	return rval;
}


//-----------------------------------------------------------------------------
void do_briefing_screens(char *filename,int level_num)
{
	int	abort_briefing_screens = 0;
	int	cur_briefing_screen = 0;

	#ifdef APPLE_DEMO
	return;			// no briefing screens at all for demo

	#endif

	mprintf ((0,"Trying briefing screen <%s>\n",filename));

	if (!filename || !*filename)
		return;

	if (!load_screen_text(filename, &Briefing_text))
		return;

	#ifdef SHAREWARE
	songs_play_song( SONG_BRIEFING, 1 );
	#else
	songs_stop_all();
	#endif

	set_screen_mode( SCREEN_MENU );

	gr_set_current_canvas(NULL);

	mprintf ((0,"Playing briefing screen <%s>, level %d\n",filename,level_num));

	key_flush();

	if (EMULATING_D1) {
		int i;

		for (i = 0; i < NUM_D1_BRIEFING_SCREENS; i++)
			memcpy(&Briefing_screens[i], &D1_Briefing_screens[i], sizeof(briefing_screen));

		if (level_num == 1) {
			while ((!abort_briefing_screens) && (Briefing_screens[cur_briefing_screen].level_num == 0)) {
				abort_briefing_screens = show_briefing_screen(cur_briefing_screen, 0);
				cur_briefing_screen++;
			}
		}

		if (!abort_briefing_screens) {
			for (cur_briefing_screen = 0; cur_briefing_screen < NUM_D1_BRIEFING_SCREENS; cur_briefing_screen++)
				if (Briefing_screens[cur_briefing_screen].level_num == level_num)
					if (show_briefing_screen(cur_briefing_screen, 0))
						break;
		}

	} else
		show_briefing_screen(level_num,0);

	d_free (Briefing_text);
	key_flush();

	return;

}

int DefineBriefingBox (char **buf)
{
	int n,i=0;
	char name[20];

	n=get_new_message_num (buf);

	Assert(n < MAX_BRIEFING_SCREENS);

	while (**buf!=' ') {
		name[i++]=**buf;
		(*buf)++;
	}

	name[i]='\0';   // slap a delimiter on this guy

	strcpy (Briefing_screens[n].bs_name,name);
	Briefing_screens[n].level_num=get_new_message_num (buf);
	Briefing_screens[n].message_num=get_new_message_num (buf);
	Briefing_screens[n].text_ulx=get_new_message_num (buf);
	Briefing_screens[n].text_uly=get_new_message_num (buf);
	Briefing_screens[n].text_width=get_new_message_num (buf);
	Briefing_screens[n].text_height=get_message_num (buf);  // NOTICE!!!

	Briefing_screens[n].text_ulx = rescale_x(Briefing_screens[n].text_ulx);
	Briefing_screens[n].text_uly = rescale_y(Briefing_screens[n].text_uly);
	Briefing_screens[n].text_width = rescale_x(Briefing_screens[n].text_width);
	Briefing_screens[n].text_height = rescale_y(Briefing_screens[n].text_height);

	return (n);
}

int get_new_message_num(char **message)
{
	int	num=0;

	while (**message == ' ')
		(*message)++;

	while ((**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

       (*message)++;

	return num;
}

void show_order_form()
{
#ifndef EDITOR
	char    exit_screen[16];

	gr_set_current_canvas( NULL );
	gr_palette_clear();

	key_flush();

	strcpy(exit_screen, HiresGFX?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HiresGFX?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HiresGFX?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HiresGFX?"warningb.pcx":"warning.pcx"); // D1
	if (! cfexist(exit_screen))
		return; // D2 registered

	show_title_screen(exit_screen,1,0);

#endif
}
