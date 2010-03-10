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

#ifdef RCS
static char rcsid[] = "$Id: titles.c,v 1.2 2006/03/18 23:08:13 michaelstather Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef OGL
#include "ogl_init.h"
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
#include "console.h"

extern unsigned RobSX,RobSY,RobDX,RobDY; // Robot movie coords

struct briefing;
void set_briefing_fontcolor (struct briefing *br);
int get_new_message_num(char **message);
int DefineBriefingBox (char **buf);

#define MAX_BRIEFING_COLORS     7
#define	SHAREWARE_ENDING_FILENAME	"ending.tex"
#define DEFAULT_BRIEFING_BKG		"brief03.pcx"

int	Briefing_text_colors[MAX_BRIEFING_COLORS];
int	Current_color = 0;
int	Erase_color;

// added by Jan Bobrowski for variable-size menu screen
static int rescale_x(int x)
{
	return x * GWIDTH / 320;
}

static int rescale_y(int y)
{
	return y * GHEIGHT / 200;
}

int local_key_inkey(void)
{
	int	rval;

	rval = key_inkey();

	if (rval == KEY_PRINT_SCREEN) {
		save_screen_shot(0);
		return 0;				//say no key pressed
	}

	else if (mouse_button_state(0))
		rval = KEY_SPACEBAR;

	return rval;
}

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

	gr_init_bitmap_data (&title_bm);

	if ((pcx_error=pcx_read_bitmap( filename, &title_bm, BM_LINEAR, gr_palette ))!=PCX_ERROR_NONE)	{
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",filename, pcx_errormsg(pcx_error), pcx_error);
	}

	gr_set_current_canvas( NULL );

	timer = timer_get_fixed_seconds() + i2f(3);

	gr_palette_load( gr_palette );

	while (1) {
		gr_flip();
		show_fullscr(&title_bm);

		if (( local_key_inkey() && allow_keys ) || ( timer_get_fixed_seconds() > timer ))
		{
			gr_free_bitmap_data (&title_bm);
			break;
		}
		timer_delay2(50);
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
			strcpy(filename,HIRESMODE?"pre_i1b.pcx":"pre_i1.pcx");

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

			strcpy(filename, HIRESMODE?"iplogo1b.pcx":"iplogo1.pcx"); // OEM
			if (! cfexist(filename))
				strcpy(filename, "iplogo1.pcx"); // SHAREWARE
			if (! cfexist(filename))
				strcpy(filename, "mplogo.pcx"); // MAC SHAREWARE
			if (cfexist(filename))
				show_title_screen(filename, 1, 1);

			strcpy(filename, HIRESMODE?"logob.pcx":"logo.pcx"); // OEM
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
			strcpy(filename,HIRESMODE?"oem1b.pcx":"oem1.pcx");

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

typedef struct msgstream {
	int x;
	int y;
	int color;
	int ch;
} __pack__ msgstream;

typedef struct briefing
{
	short	level_num;
	short	cur_screen;
	briefing_screen	*screen;
	grs_bitmap background;
	char	background_name[16];
	int		got_z;
	int		hum_channel, printing_channel;
	char	*text;
	char	*message;
	int		text_x, text_y;
	msgstream messagestream[2048];
	int		streamcount;
	short	tab_stop;
	ubyte	flashing_cursor;
	ubyte	new_page;
	ubyte	dumb_adjust;
	ubyte	line_adjustment;
	short	chattering;
	int		delay_count;
	int		robot_num;
	grs_canvas	*robot_canv;
	vms_angvec	robot_angles;
	char	robot_playing;
	char    bitmap_name[32];
	grs_bitmap  guy_bitmap;
	sbyte	guy_bitmap_show;
	sbyte   door_dir, door_div_count, animating_bitmap_type;
	sbyte	prev_ch;
} briefing;

void briefing_init(briefing *br, short level_num)
{
	br->level_num = level_num;
	br->cur_screen = 0;
	strcpy(br->background_name, DEFAULT_BRIEFING_BKG);
	br->robot_canv = NULL;
	br->robot_playing = 0;
	br->bitmap_name[0] = '\0';
	br->door_dir = 1;
	br->door_div_count = 0;
	br->animating_bitmap_type = 0;
}

void init_char_pos(briefing *br, int x, int y)
{
	br->text_x = x;
	br->text_y = y;
}

#define EXIT_DOOR_MAX   14
#define OTHER_THING_MAX 10      // Adam: This is the number of frames in your new animating thing.
#define DOOR_DIV_INIT   6

//-----------------------------------------------------------------------------
void show_animated_bitmap(briefing *br)
{
	grs_canvas  *curcanv_save, *bitmap_canv=0;
	grs_bitmap	*bitmap_ptr;

	// Only plot every nth frame.
	if (br->door_div_count) {
		if (br->bitmap_name[0] != 0) {
			bitmap_index bi;
			bi = piggy_find_bitmap(br->bitmap_name);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
#ifdef OGL
			ogl_ubitmapm_cs(rescale_x(220), rescale_y(45),(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
			gr_bitmapm(rescale_x(220), rescale_y(45), bitmap_ptr);
#endif
		}
		br->door_div_count--;
		return;
	}

	br->door_div_count = DOOR_DIV_INIT;

	if (br->bitmap_name[0] != 0) {
		char		*pound_signp;
		int		num, dig1, dig2;
		bitmap_index bi;

		switch (br->animating_bitmap_type) {
			case 0:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 64, 64);	break;
			case 1:		bitmap_canv = gr_create_sub_canvas(grd_curcanv, rescale_x(220), rescale_y(45), 94, 94);	break; // Adam: Change here for your new animating bitmap thing. 94, 94 are bitmap size.
			default:	Int3(); // Impossible, illegal value for br->animating_bitmap_type
		}

		curcanv_save = grd_curcanv;
		grd_curcanv = bitmap_canv;

		pound_signp = strchr(br->bitmap_name, '#');
		Assert(pound_signp != NULL);

		dig1 = *(pound_signp+1);
		dig2 = *(pound_signp+2);
		if (dig2 == 0)
			num = dig1-'0';
		else
			num = (dig1-'0')*10 + (dig2-'0');

		switch (br->animating_bitmap_type) {
			case 0:
				num += br->door_dir;
				if (num > EXIT_DOOR_MAX) {
					num = EXIT_DOOR_MAX;
					br->door_dir = -1;
				} else if (num < 0) {
					num = 0;
					br->door_dir = 1;
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

		bi = piggy_find_bitmap(br->bitmap_name);
		bitmap_ptr = &GameBitmaps[bi.index];
		PIGGY_PAGE_IN( bi );
#ifdef OGL
		ogl_ubitmapm_cs(0,0,(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
		gr_bitmapm(0, 0, bitmap_ptr);
#endif
		grd_curcanv = curcanv_save;
		d_free(bitmap_canv);

		switch (br->animating_bitmap_type) {
			case 0:
				if (num == EXIT_DOOR_MAX) {
					br->door_dir = -1;
					br->door_div_count = 64;
				} else if (num == 0) {
					br->door_dir = 1;
					br->door_div_count = 64;
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

	bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220*((double)SWIDTH/(HIRESMODE ? 640 : 320)), 45*((double)SHEIGHT/(HIRESMODE ? 480 : 200)),
									   bmp->bm_w, bmp->bm_h);
	curcanv_save = grd_curcanv;
	gr_set_current_canvas(bitmap_canv);
#ifdef OGL
	ogl_ubitmapm_cs(0,0,(bmp->bm_w*(SWIDTH/(HIRESMODE ? 640 : 320))),(bmp->bm_h*(SHEIGHT/(HIRESMODE ? 480 : 200))),bmp,255,F1_0);
#else
	gr_bitmapm(0, 0, bmp);
#endif
	gr_set_current_canvas(curcanv_save);

	d_free(bitmap_canv);
}

//-----------------------------------------------------------------------------
void show_spinning_robot_frame(briefing *br, int robot_num)
{
	grs_canvas	*curcanv_save;

	if (robot_num != -1) {
		br->robot_angles.h += 150;

		curcanv_save = grd_curcanv;
		grd_curcanv = br->robot_canv;
		Assert(Robot_info[robot_num].model_num != -1);
		draw_model_picture(Robot_info[robot_num].model_num, &br->robot_angles);
		grd_curcanv = curcanv_save;
	}

}

//-----------------------------------------------------------------------------
void init_spinning_robot(briefing *br) //(int x,int y,int w,int h)
{
	int x = rescale_x(138);
	int y = rescale_y(55);
	int w = rescale_x(166);
	int h = rescale_y(138);

	br->robot_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
}

//---------------------------------------------------------------------------
// Returns char width.
// If show_robot_flag set, then show a frame of the spinning robot.
int show_char_delay(briefing *br, char the_char, int delay, int robot_num, int cursor_flag)
{
	int	w, h, aw;
	char	message[2];
	static fix start_time = 0;

	message[0] = the_char;
	message[1] = 0;

	if (start_time==0 && timer_get_fixed_seconds()<0)
		start_time=timer_get_fixed_seconds();

	gr_get_string_size(message, &w, &h, &aw );

	Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));

	//	Draw cursor if there is some delay and caller says to draw cursor
	if (delay)
	{
		if (cursor_flag)
		{
			gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
			gr_printf(br->text_x, br->text_y, "_" );
		}

		if (br->bitmap_name[0] != 0)
			show_animated_bitmap(br);

		if (br->robot_playing)
			RotateRobot();

		while (timer_get_fixed_seconds() < (start_time + delay/2))
		{
			if (br->robot_playing)
				RotateRobot();
		}
		if (robot_num != -1)
			show_spinning_robot_frame(br, robot_num);

		start_time = timer_get_fixed_seconds();

		// Erase cursor
		if (cursor_flag)
		{
			gr_set_fontcolor(Erase_color, -1);
			gr_printf(br->text_x, br->text_y, "_" );
		}
	}

	// Draw the character
	gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
	gr_printf(br->text_x+1, br->text_y, message );

	return w;
}

//	-----------------------------------------------------------------------------
//	loads a briefing screen
int load_briefing_screen(briefing *br, char *fname)
{
	int pcx_error;

	if (br->background.bm_data != NULL)
		gr_free_bitmap_data(&br->background);

	gr_init_bitmap_data(&br->background);

	strcpy (br->background_name,fname);

	if ((pcx_error = pcx_read_bitmap(fname, &br->background, BM_LINEAR, gr_palette))!=PCX_ERROR_NONE)
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",fname, pcx_errormsg(pcx_error), pcx_error);

	show_fullscr(&br->background);

	if (EMULATING_D1) // HACK, FIXME: D1 missions should use their own palette (PALETTE.256), but texture replacements not complete
		gr_use_palette_table("groupa.256");

	gr_palette_load(gr_palette);

	set_briefing_fontcolor(br);

	return 1;
}



#define KEY_DELAY_DEFAULT       ((F1_0*20)/1000)

//-----------------------------------------------------------------------------
int get_message_num(char **message)
{
	int	num=0;

	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;

	while (strlen(*message) > 0 && (**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

	while (strlen(*message) > 0 && *(*message)++ != 10)		//	Get and drop eoln
		;

	return num;
}

//-----------------------------------------------------------------------------
void get_message_name(char **message, char *result)
{
	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;

	while (strlen(*message) > 0 && (**message != ' ') && (**message != 10)) {
		if (**message != '\n')
			*result++ = **message;
		(*message)++;
	}

	if (**message != 10)
		while (strlen(*message) > 0 && *(*message)++ != 10)		//	Get and drop eoln
			;

	*result = 0;
}

//-----------------------------------------------------------------------------
void flash_cursor(briefing *br, int cursor_flag)
{
	if (cursor_flag == 0)
		return;

	if ((timer_get_fixed_seconds() % (F1_0/2) ) > (F1_0/4))
		gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
	else
		gr_set_fontcolor(Erase_color, -1);

	gr_printf(br->text_x, br->text_y, "_" );
}

void redraw_messagestream(msgstream *stream, int count)
{
	char msgbuf[2];
	int i;

	for (i=0; i<count; i++) {
		msgbuf[0] = stream[i].ch;
		msgbuf[1] = 0;
		if (stream[i-1].color != stream[i].color)
			gr_set_fontcolor(stream[i].color,-1);
		gr_printf(stream[i].x+1,stream[i].y,"%s",msgbuf);
	}
}

//	-----------------------------------------------------------------------------
//	Return true if message got aborted by user (pressed ESC), else return false.
//	Draws text, images and bitmaps actually
int show_briefing(briefing *br)
{
	int	ch, done=0;
	int	key_check;
	int	rval=0;

	br->got_z = 0;
	br->hum_channel = br->printing_channel = -1;
	Current_color = 0;
	br->streamcount = 0;
	br->tab_stop = 0;
	br->flashing_cursor = 0;
	br->new_page = 0;
	br->dumb_adjust = 0;
	br->line_adjustment = 1;
	br->chattering = 0;
	br->delay_count = KEY_DELAY_DEFAULT;
	br->robot_num = -1;
	br->robot_playing=0;
	br->bitmap_name[0] = 0;
	br->guy_bitmap_show = 0;
	br->prev_ch = -1;

	#ifndef SHAREWARE
	br->hum_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_HUM), F1_0/2, 0xFFFF/2, 1, -1, -1, -1 );
	#endif

	gr_set_curfont( GAME_FONT );

	if (EMULATING_D1) {
		br->got_z = 1;
		MALLOC(br->screen, briefing_screen, 1);
		memcpy(br->screen, &Briefing_screens[br->cur_screen], sizeof(briefing_screen));
		br->screen->text_ulx = rescale_x(br->screen->text_ulx);
		br->screen->text_uly = rescale_y(br->screen->text_uly);
		br->screen->text_width = rescale_x(br->screen->text_width);
		br->screen->text_height = rescale_y(br->screen->text_height);
		init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
	} else {
		br->screen=&Briefing_screens[0];
		init_char_pos(br, br->screen->text_ulx, br->screen->text_uly-(8*(1+HIRESMODE)));
	}

	while (!done) {
		ch = *br->message++;
		if (ch == '$') {
			ch = *br->message++;
			if (ch=='D') {
				br->cur_screen=DefineBriefingBox (&br->message);
				br->screen = &Briefing_screens[br->cur_screen];
				init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
				br->line_adjustment=0;
				br->prev_ch = 10;                                   // read to eoln
			} else if (ch=='U') {
				br->cur_screen=get_message_num(&br->message);
				br->screen = &Briefing_screens[br->cur_screen];
				init_char_pos(br, br->screen->text_ulx, br->screen->text_uly);
				br->prev_ch = 10;                                   // read to eoln
			} else if (ch == 'C') {
				Current_color = get_message_num(&br->message)-1;
				Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				br->prev_ch = 10;
			} else if (ch == 'F') {     // toggle flashing cursor
				br->flashing_cursor = !br->flashing_cursor;
				br->prev_ch = 10;
				while (*br->message++ != 10)
					;
			} else if (ch == 'T') {
				br->tab_stop = get_message_num(&br->message);
				br->tab_stop*=(1+HIRESMODE);
				br->prev_ch = 10;							//	read to eoln
			} else if (ch == 'R') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}
				if (br->robot_playing) {
					DeInitRobotMovie();
					br->robot_playing=0;
				}

				if (EMULATING_D1) {
					init_spinning_robot(br);
					br->robot_num = get_message_num(&br->message);
					while (*br->message++ != 10)
						;
				} else {
					char spinRobotName[]="rba.mve",kludge;  // matt don't change this!

					kludge=*br->message++;
					spinRobotName[2]=kludge; // ugly but proud

					br->robot_playing=InitRobotMovie(spinRobotName);

					// gr_remap_bitmap_good( &grd_curcanv->cv_bitmap, pal, -1, -1 );

					if (br->robot_playing) {
						RotateRobot();
						set_briefing_fontcolor (br);
					}
				}
				br->prev_ch = 10;                           // read to eoln
			} else if (ch == 'N') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				get_message_name(&br->message, br->bitmap_name);
				strcat(br->bitmap_name, "#0");
				br->animating_bitmap_type = 0;
				br->prev_ch = 10;
			} else if (ch == 'O') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				get_message_name(&br->message, br->bitmap_name);
				strcat(br->bitmap_name, "#0");
				br->animating_bitmap_type = 1;
				br->prev_ch = 10;
			} else if (ch=='A') {
				br->line_adjustment=1-br->line_adjustment;
			} else if (ch=='Z') {
				char fname[15];
				int i;

				br->got_z=1;
				br->dumb_adjust=1;
				i=0;
				while ((fname[i]=*br->message) != '\n') {
					i++;
					br->message++;
				}
				fname[i]=0;
				if (*br->message != 10)
					while (*br->message++ != 10)    //  Get and drop eoln
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

					if ((HIRESMODE && cfexist(fname2)) || !cfexist(fname))
						strcpy(fname,fname2);
					load_briefing_screen (br, fname);
				}

			} else if (ch == 'B') {
				char		bitmap_name[32];
				ubyte		temp_palette[768];
				int		iff_error;

				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				get_message_name(&br->message, bitmap_name);
				strcat(bitmap_name, ".bbm");
				gr_init_bitmap_data (&br->guy_bitmap);
				iff_error = iff_read_bitmap(bitmap_name, &br->guy_bitmap, BM_LINEAR, temp_palette);
				Assert(iff_error == IFF_NO_ERROR);
				gr_remap_bitmap_good( &br->guy_bitmap, temp_palette, -1, -1 );

				show_briefing_bitmap(&br->guy_bitmap);
				br->guy_bitmap_show=1;
				br->prev_ch = 10;
			} else if (ch == 'S') {
				int keypress;
				fix start_time;

				br->chattering=0;
				if (br->printing_channel>-1)
					digi_stop_sound( br->printing_channel );
				br->printing_channel=-1;


				start_time = timer_get_fixed_seconds();
				while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key

					while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
						;
					timer_delay2(50);
					if (!br->robot_playing)
						gr_flip();
					show_fullscr(&br->background);
					redraw_messagestream(br->messagestream, br->streamcount);
					if (br->guy_bitmap_show)
						show_briefing_bitmap(&br->guy_bitmap);
					flash_cursor(br, br->flashing_cursor);

					if (br->robot_playing)
						RotateRobot ();
					if (br->robot_num != -1)
						show_spinning_robot_frame(br, br->robot_num);

					if (br->bitmap_name[0] != 0)
						show_animated_bitmap(br);
					start_time += KEY_DELAY_DEFAULT/2;
				}

#ifndef NDEBUG
				if (keypress == KEY_BACKSP)
					Int3();
#endif
				if (keypress == KEY_ESC)
					rval = 1;

				br->flashing_cursor = 0;
				done = 1;
			} else if (ch == 'P') {		//	New page.
				if (!br->got_z) {
					Int3(); // Hey ryan!!!! You gotta load a screen before you start
					        // printing to it! You know, $Z !!!
		  		    load_briefing_screen (br, HIRESMODE?"end01b.pcx":"end01.pcx");
				}

				br->new_page = 1;

				while (*br->message != 10) {
					br->message++;	//	drop carriage return after special escape sequence
				}
				br->message++;
				br->prev_ch = 10;
			}
		} else if (ch == '\t') {		//	Tab
			if (br->text_x - br->screen->text_ulx < br->tab_stop)
				br->text_x = br->screen->text_ulx + br->tab_stop;
		} else if ((ch == ';') && (br->prev_ch == 10)) {
			while (*br->message++ != 10)
				;
			br->prev_ch = 10;
		} else if (ch == '\\') {
			br->prev_ch = ch;
		} else if (ch == 10) {
			if (br->prev_ch != '\\') {
				br->prev_ch = ch;
				if (br->dumb_adjust==0)
					br->text_y += FSPACY(5)+FSPACY(5)*3/5;
				else
					br->dumb_adjust--;
				br->text_x = br->screen->text_ulx;
				if (br->text_y > br->screen->text_uly + br->screen->text_height) {
					load_briefing_screen(br, Briefing_screens[br->cur_screen].bs_name);
					br->text_x = br->screen->text_ulx;
					br->text_y = br->screen->text_uly;
				}
			} else {
				if (ch == 13)		//Can this happen? Above says ch==10
					Int3();
				br->prev_ch = ch;
			}
		} else {
			if (!br->got_z) {
				Int3(); // Hey ryan!!!! You gotta load a screen before you start
				        // printing to it! You know, $Z !!!
				load_briefing_screen (br, HIRESMODE?"end01b.pcx":"end01.pcx");
			}

			br->messagestream[br->streamcount].x = br->text_x;
			br->messagestream[br->streamcount].y = br->text_y;
			br->messagestream[br->streamcount].color = Briefing_text_colors[Current_color];
			br->messagestream[br->streamcount].ch = ch;
			if (br->delay_count) {
				if (!br->robot_playing)
					gr_flip();
				show_fullscr(&br->background);
				redraw_messagestream(br->messagestream, br->streamcount);
			}
			if (br->guy_bitmap_show)
				show_briefing_bitmap(&br->guy_bitmap);
			br->streamcount++;

			br->prev_ch = ch;

			if (!br->chattering) {
		 		br->printing_channel  = digi_start_sound( digi_xlat_sound(SOUND_BRIEFING_PRINTING), F1_0, 0xFFFF/2, 1, -1, -1, -1 );
				br->chattering=1;
			}

			br->text_x += show_char_delay(br, ch, br->delay_count, br->robot_num, br->flashing_cursor);

		}

		//	Check for Esc -> abort.
		if(br->delay_count)
			key_check=local_key_inkey();
		else
			key_check=0;

		if ( key_check == KEY_ESC ) {
			rval = 1;
			done = 1;
		}

		if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER))
			br->delay_count = 0;

		if (br->text_x > br->screen->text_ulx + br->screen->text_width) {
			br->text_x = br->screen->text_ulx;
			br->text_y += br->screen->text_uly;
		}

		if ((br->new_page) || (br->text_y > br->screen->text_uly + br->screen->text_height)) {
			fix	start_time = 0;
			int	keypress;

			br->new_page = 0;

			if (br->printing_channel>-1)
				digi_stop_sound( br->printing_channel );
			br->printing_channel=-1;

			br->chattering=0;

			start_time = timer_get_fixed_seconds();
			while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key
				while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
					;
				timer_delay2(50);
				if (!br->robot_playing)
					gr_flip();
				show_fullscr(&br->background);
				redraw_messagestream(br->messagestream, br->streamcount);
				if (br->guy_bitmap_show)
					show_briefing_bitmap(&br->guy_bitmap);
				flash_cursor(br, br->flashing_cursor);
				if (br->robot_playing)
					RotateRobot();
				if (br->robot_num != -1)
					show_spinning_robot_frame(br, br->robot_num);
				if (br->bitmap_name[0] != 0)
					show_animated_bitmap(br);
				start_time += KEY_DELAY_DEFAULT/2;
			}

			if (br->robot_playing)
				DeInitRobotMovie();
			br->robot_playing=0;
			br->robot_num = -1;

#ifndef NDEBUG
			if (keypress == KEY_BACKSP)
				Int3();
#endif
			if (keypress == KEY_ESC) {
				rval = 1;
				done = 1;
			}
			load_briefing_screen(br, br->background_name);
			br->text_x = br->screen->text_ulx;
			br->text_y = br->screen->text_uly;

			br->streamcount=0;
			if (br->guy_bitmap_show) {
				gr_free_bitmap_data (&br->guy_bitmap);
				br->guy_bitmap_show=0;
			}

			br->delay_count = KEY_DELAY_DEFAULT;
		}
	}

	if (br->robot_playing) {
		DeInitRobotMovie();
		br->robot_playing=0;
	}

	if (br->robot_canv != NULL)
		{d_free(br->robot_canv); br->robot_canv=NULL;}

	if (br->hum_channel>-1)
		digi_stop_sound( br->hum_channel );
	if (br->printing_channel>-1)
		digi_stop_sound( br->printing_channel );
	if (EMULATING_D1) {
		d_free(br->screen);
	}

	if (br->background.bm_data != NULL)
		gr_free_bitmap_data (&br->background);

	return rval;
}

//-----------------------------------------------------------------------------
// Return a pointer to the start of text for screen #screen_num.
char * get_briefing_message(briefing *br, int screen_num)
{
	char	*tptr = br->text;
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
         		return (0);
				//Error("Cannot open file %s or %s", filename, nfilename);
		}

		have_binary = 1;

		len = cfilelength(ifile);
		MALLOC(*buf, char, len+1);
		for (x=0, i=0; i < len; i++, x++) {
			cfread (*buf+x,1,1,ifile);
			if (*(*buf+x)==13)
				x--;
		}
		cfclose(ifile);
	} else {
		len = cfilelength(tfile);
		MALLOC(*buf, char, len+1);
		for (x=0, i=0; i < len; i++, x++) {
			cfread (*buf+x,1,1,tfile);
			if (*(*buf+x)==13)
				x--;
		}
		cfclose(tfile);
	}

	if (have_binary)
		decode_text(*buf, len);

	*(*buf+len)='\0';

	return (1);
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int show_briefing_text(briefing *br)
{
	br->message = get_briefing_message
		(br, EMULATING_D1 ? Briefing_screens[br->cur_screen].message_num : br->cur_screen);

	if (br->message==NULL)
		return (0);

	set_briefing_fontcolor(br);

	return show_briefing(br);
}

void set_briefing_fontcolor (briefing *br)
{
	Briefing_text_colors[0] = gr_find_closest_color_current( 0, 40, 0);
	Briefing_text_colors[1] = gr_find_closest_color_current( 40, 33, 35);
	Briefing_text_colors[2] = gr_find_closest_color_current( 8, 31, 54);

	if (EMULATING_D1) {
		//green
		Briefing_text_colors[0] = gr_find_closest_color_current( 0, 54, 0);
		//white
		Briefing_text_colors[1] = gr_find_closest_color_current( 42, 38, 32);
		//Begin D1X addition
		//red
		Briefing_text_colors[2] = gr_find_closest_color_current( 63, 0, 0);
	}

	if (br->robot_playing)
	{
		Briefing_text_colors[0] = gr_find_closest_color_current( 0, 31, 0);
	}

	//blue
	Briefing_text_colors[3] = gr_find_closest_color_current( 0, 0, 54);
	//gray
	Briefing_text_colors[4] = gr_find_closest_color_current( 14, 14, 14);
	//yellow
	Briefing_text_colors[5] = gr_find_closest_color_current( 54, 54, 0);
	//purple
	Briefing_text_colors[6] = gr_find_closest_color_current( 0, 54, 54);
	//End D1X addition

	Erase_color = gr_find_closest_color_current(0, 0, 0);
}

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int show_briefing_screen(briefing *br)
{
	int     rval=0;

	if (EMULATING_D1) {
		gr_init_bitmap_data (&br->background);

		load_briefing_screen(br, Briefing_screens[br->cur_screen].bs_name);
		show_fullscr(&br->background );
		gr_palette_load(gr_palette);
	}

	rval = show_briefing_text(br);

	return rval;
}


//-----------------------------------------------------------------------------
void do_briefing_screens(char *filename, int level_num)
{
	briefing *br;
	int abort_briefing_screens = 0;

	if (!filename || !*filename)
		return;

	MALLOC(br, briefing, 1);
	if (!br)
		return;

	briefing_init(br, level_num);
	
	if (!load_screen_text(filename, &br->text))
	{
		d_free(br);
		return;
	}

	#ifdef SHAREWARE
	songs_play_song( SONG_BRIEFING, 1 );
	#else
	songs_stop_all();
	#endif

	// set screen correctly for robot movies
	set_screen_mode( SCREEN_MOVIE );

	gr_set_current_canvas(NULL);

	key_flush();

	if (EMULATING_D1) {
		int i;

		for (i = 0; i < NUM_D1_BRIEFING_SCREENS; i++)
			memcpy(&Briefing_screens[i], &D1_Briefing_screens[i], sizeof(briefing_screen));

		if (br->level_num == 1) {
			while ((!abort_briefing_screens) && (Briefing_screens[br->cur_screen].level_num == 0)) {
				abort_briefing_screens = show_briefing_screen(br);
				br->cur_screen++;
			}
		}

		if (!abort_briefing_screens) {
			for (br->cur_screen = 0; br->cur_screen < NUM_D1_BRIEFING_SCREENS; br->cur_screen++)
				if (Briefing_screens[br->cur_screen].level_num == br->level_num)
					if (show_briefing_screen(br))
						break;
		}

	} else
	{
		br->cur_screen = br->level_num;
		show_briefing_screen(br);
	}

	d_free(br->text);
	d_free(br);

	key_flush();
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

	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;

	while (strlen(*message) > 0 && (**message >= '0') && (**message <= '9')) {
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

	key_flush();

	strcpy(exit_screen, HIRESMODE?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HIRESMODE?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HIRESMODE?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! cfexist(exit_screen))
		strcpy(exit_screen, HIRESMODE?"warningb.pcx":"warning.pcx"); // D1
	if (! cfexist(exit_screen))
		return; // D2 registered

	show_title_screen(exit_screen,1,0);

#endif
}
