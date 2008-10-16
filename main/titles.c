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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#ifdef RCS
static char rcsid[] = "$Id: titles.c,v 1.2 2006/03/18 23:08:13 michaelstather Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include "compbit.h"
#include "text.h"
#include "kmatrix.h"
#include "piggy.h"
#include "songs.h"
#include "newmenu.h"
#include "state.h"
#include "gameseq.h"
#include "console.h"

#ifdef OGL
#include "ogl_init.h"
#endif

void title_save_game();
void set_briefing_fontcolor ();

#define MAX_BRIEFING_COLORS 7
#define	SHAREWARE_ENDING_FILENAME "ending.tex"
char * Briefing_text;
int	Briefing_text_colors[MAX_BRIEFING_COLORS];
int	Current_color = 0;
int	Erase_color;
grs_bitmap briefing_bm;

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
	int rval;
	rval = key_inkey();

	if ( rval==KEY_ALTED+KEY_F2 )	{
	 	title_save_game();
		return 0;
	}

	if (rval == KEY_PRINT_SCREEN) {
		save_screen_shot(0);
		return 0; //say no key pressed
	}

	return rval;
}

int show_title_screen( char * filename, int allow_keys )	
{
	fix timer;
	int pcx_error;
	grs_bitmap title_bm;

	gr_init_bitmap_data (&title_bm);

	if ((pcx_error=pcx_read_bitmap( filename, &title_bm, BM_LINEAR, gr_palette ))!=PCX_ERROR_NONE)	{
		Int3();
		gr_free_bitmap_data (&title_bm);
		return 0;
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

typedef struct {
	char	bs_name[255]; // filename, eg merc01.  Assumes .lbm suffix.
	sbyte	level_num;
	sbyte	message_num;
	short	text_ulx, text_uly; // upper left x,y of text window
	short	text_width, text_height; // width and height of text window
} briefing_screen;

#define BRIEFING_SECRET_NUM	31 // This must correspond to the first secret level which must come at the end of the list.
#define BRIEFING_OFFSET_NUM	4 // This must correspond to the first level screen (ie, past the bald guy briefing screens)
#define	SHAREWARE_ENDING_LEVEL_NUM	0x7f
#define	REGISTERED_ENDING_LEVEL_NUM	0x7e
#define Briefing_screens_LH ((SWIDTH >= 640 && SHEIGHT >= 480 && cfexist( "brief01h.pcx"))?Briefing_screens_h:Briefing_screens)

briefing_screen Briefing_screens[] = {
	{	"brief01.pcx",   0,  1,  13, 140, 290,  59 },
	{	"brief02.pcx",   0,  2,  27,  34, 257, 177 },
	{	"brief03.pcx",   0,  3,  20,  22, 257, 177 },
	{	"brief02.pcx",   0,  4,  27,  34, 257, 177 },

	{	"moon01.pcx",    1,  5,  10,  10, 300, 170 },	// level 1
	{	"moon01.pcx",    2,  6,  10,  10, 300, 170 },	// level 2
	{	"moon01.pcx",    3,  7,  10,  10, 300, 170 },	// level 3

	{	"venus01.pcx",   4,  8,  15,  15, 300, 200 },	// level 4
	{	"venus01.pcx",   5,  9,  15,  15, 300, 200 },	// level 5

	{	"brief03.pcx",   6, 10,  20,  22, 257, 177 },
	{	"merc01.pcx",    6, 11,  10,  15, 300, 200 },	// level 6
	{	"merc01.pcx",    7, 12,  10,  15, 300, 200 },	// level 7

#ifndef SHAREWARE
	{	"brief03.pcx",   8, 13,  20,  22, 257, 177 },
	{	"mars01.pcx",    8, 14,  10, 100, 300, 200 },	// level 8
	{	"mars01.pcx",    9, 15,  10, 100, 300, 200 },	// level 9
	{	"brief03.pcx",  10, 16,  20,  22, 257, 177 },
	{	"mars01.pcx",   10, 17,  10, 100, 300, 200 },	// level 10

	{	"jup01.pcx",    11, 18,  10,  40, 300, 200 },	// level 11
	{	"jup01.pcx",    12, 19,  10,  40, 300, 200 },	// level 12
	{	"brief03.pcx",  13, 20,  20,  22, 257, 177 },
	{	"jup01.pcx",    13, 21,  10,  40, 300, 200 },	// level 13
	{	"jup01.pcx",    14, 22,  10,  40, 300, 200 },	// level 14

	{	"saturn01.pcx", 15, 23,  10,  40, 300, 200 },	// level 15
	{	"brief03.pcx",  16, 24,  20,  22, 257, 177 },
	{	"saturn01.pcx", 16, 25,  10,  40, 300, 200 },	// level 16
	{	"brief03.pcx",  17, 26,  20,  22, 257, 177 },
	{	"saturn01.pcx", 17, 27,  10,  40, 300, 200 },	// level 17

	{	"uranus01.pcx", 18, 28, 100, 100, 300, 200 },	// level 18
	{	"uranus01.pcx", 19, 29, 100, 100, 300, 200 },	// level 19
	{	"uranus01.pcx", 20, 30, 100, 100, 300, 200 },	// level 20
	{	"uranus01.pcx", 21, 31, 100, 100, 300, 200 },	// level 21

	{	"neptun01.pcx", 22, 32,  10,  20, 300, 200 },	// level 22
	{	"neptun01.pcx", 23, 33,  10,  20, 300, 200 },	// level 23
	{	"neptun01.pcx", 24, 34,  10,  20, 300, 200 },	// level 24

	{	"pluto01.pcx",  25, 35,  10,  20, 300, 200 },	// level 25
	{	"pluto01.pcx",  26, 36,  10,  20, 300, 200 },	// level 26
	{	"pluto01.pcx",  27, 37,  10,  20, 300, 200 },	// level 27

	{	"aster01.pcx",  -1, 38,  10,  90, 300, 200 },	// secret level -1
	{	"aster01.pcx",  -2, 39,  10,  90, 300, 200 },	// secret level -2
	{	"aster01.pcx",  -3, 40,  10,  90, 300, 200 }, 	// secret level -3
#endif

	{	"end01.pcx",   SHAREWARE_ENDING_LEVEL_NUM,   1, 23, 40, 320, 200 }, 	// shareware end
#ifndef SHAREWARE
	{	"end02.pcx",   REGISTERED_ENDING_LEVEL_NUM,  1,  5,  5, 300, 200 }, 	// registered end
	{	"end01.pcx",   REGISTERED_ENDING_LEVEL_NUM,  2, 23, 40, 320, 200 }, 	// registered end
	{	"end03.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,  5,  5, 300, 200 }, 	// registered end
#endif

};

briefing_screen Briefing_screens_h[] = { // hires screens
	{  "brief01h.pcx",   0,  1,  13, 140, 290,  59 },
	{  "brief02h.pcx",   0,  2,  27,  34, 257, 177 },
	{  "brief03h.pcx",   0,  3,  20,  22, 257, 177 },
	{  "brief02h.pcx",   0,  4,  27,  34, 257, 177 },

	{  "moon01h.pcx",    1,  5,  10,  10, 300, 170 },	// level 1
	{  "moon01h.pcx",    2,  6,  10,  10, 300, 170 },	// level 2
	{  "moon01h.pcx",    3,  7,  10,  10, 300, 170 },	// level 3

	{  "venus01h.pcx",   4,  8,  15,  15, 300, 200 },	// level 4
	{  "venus01h.pcx",   5,  9,  15,  15, 300, 200 },	// level 5

	{  "brief03h.pcx",   6, 10,  20,  22, 257, 177 },
	{  "merc01h.pcx",    6, 11,  10,  15, 300, 200 },	// level 6
	{  "merc01h.pcx",    7, 12,  10,  15, 300, 200 },	// level 7

#ifndef SHAREWARE
	{  "brief03h.pcx",   8, 13,  20,  22, 257, 177 },
	{  "mars01h.pcx",    8, 14,  10, 100, 300, 200 },	// level 8
	{  "mars01h.pcx",    9, 15,  10, 100, 300, 200 },	// level 9
	{  "brief03h.pcx",  10, 16,  20,  22, 257, 177 },
	{  "mars01h.pcx",   10, 17,  10, 100, 300, 200 },	// level 10

	{  "jup01h.pcx",    11, 18,  10,  40, 300, 200 },	// level 11
	{  "jup01h.pcx",    12, 19,  10,  40, 300, 200 },	// level 12
	{  "brief03h.pcx",  13, 20,  20,  22, 257, 177 },
	{  "jup01h.pcx",    13, 21,  10,  40, 300, 200 },	// level 13
	{  "jup01h.pcx",    14, 22,  10,  40, 300, 200 },	// level 14

	{  "saturn01h.pcx", 15, 23,  10,  40, 300, 200 },	// level 15
	{  "brief03h.pcx",  16, 24,  20,  22, 257, 177 },
	{  "saturn01h.pcx", 16, 25,  10,  40, 300, 200 },	// level 16
	{  "brief03h.pcx",  17, 26,  20,  22, 257, 177 },
	{  "saturn01h.pcx", 17, 27,  10,  40, 300, 200 },	// level 17

	{  "uranus01h.pcx", 18, 28, 100, 100, 300, 200 },	// level 18
	{  "uranus01h.pcx", 19, 29, 100, 100, 300, 200 },	// level 19
	{  "uranus01h.pcx", 20, 30, 100, 100, 300, 200 },	// level 20
	{  "uranus01h.pcx", 21, 31, 100, 100, 300, 200 },	// level 21

	{  "neptun01h.pcx", 22, 32,  10,  20, 300, 200 },	// level 22
	{  "neptun01h.pcx", 23, 33,  10,  20, 300, 200 },	// level 23
	{  "neptun01h.pcx", 24, 34,  10,  20, 300, 200 },	// level 24

	{  "pluto01h.pcx",  25, 35,  10,  20, 300, 200 },	// level 25
	{  "pluto01h.pcx",  26, 36,  10,  20, 300, 200 },	// level 26
	{  "pluto01h.pcx",  27, 37,  10,  20, 300, 200 },	// level 27

	{  "aster01h.pcx",  -1, 38,  10, 90, 300,  200 },	// secret level -1
	{  "aster01h.pcx",  -2, 39,  10, 90, 300,  200 },	// secret level -2
	{  "aster01h.pcx",  -3, 40,  10, 90, 300,  200 }, 	// secret level -3
#endif

	{  "end01h.pcx",   SHAREWARE_ENDING_LEVEL_NUM,   1,  23, 40, 320, 200 }, 	// shareware end
#ifndef SHAREWARE
	{  "end02h.pcx",   REGISTERED_ENDING_LEVEL_NUM,  1,   5,  5, 300, 200 }, 	// registered end
	{  "end01h.pcx",   REGISTERED_ENDING_LEVEL_NUM,  2,  23, 40, 320, 200 }, 	// registered end
	{  "end03h.pcx",   REGISTERED_ENDING_LEVEL_NUM,  3,   5,  5, 300, 200 }, 	// registered end
#endif

};

#define	MAX_BRIEFING_SCREEN (sizeof(Briefing_screens) / sizeof(Briefing_screens[0]))

char * get_briefing_screen( int level_num )
{
	int i, found_level=0, last_level=0;
	for (i = 0; i < MAX_BRIEFING_SCREEN; i++)	{
		if ( found_level && Briefing_screens_LH[i].level_num != level_num )
			return Briefing_screens_LH[last_level].bs_name;
		if (Briefing_screens[i].level_num == level_num )	{
			found_level=1;
			last_level = i;
		}
	}
	return NULL;
}

int	Briefing_text_x, Briefing_text_y;

void init_char_pos(int x, int y)
{
	Briefing_text_x = x;
	Briefing_text_y = y;
}

grs_canvas *Robot_canv = NULL;
vms_angvec Robot_angles;

char	Bitmap_name[32] = "";
#define	EXIT_DOOR_MAX	14
#define	OTHER_THING_MAX	10	// Adam: This is the number of frames in your new animating thing.
#define	DOOR_DIV_INIT	6
sbyte	Door_dir=1, Door_div_count=0, Animating_bitmap_type=0;

//	-----------------------------------------------------------------------------
void show_animated_bitmap(void)
{
        grs_canvas      *curcanv_save, *bitmap_canv=0;
	grs_bitmap	*bitmap_ptr;

	// Only plot every nth frame.
	if (Door_div_count) {

		if (Bitmap_name[0] != 0) {
			bitmap_index bi;
			bi = piggy_find_bitmap(Bitmap_name);
			bitmap_ptr = &GameBitmaps[bi.index];
			PIGGY_PAGE_IN( bi );
#ifdef OGL
			ogl_ubitmapm_cs(rescale_x(220), rescale_y(45),(bitmap_ptr->bm_w*(SWIDTH/320)),(bitmap_ptr->bm_h*(SHEIGHT/200)),bitmap_ptr,255,F1_0);
#else
			gr_bitmapm(rescale_x(220), rescale_y(45), bitmap_ptr);
#endif
		}
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

//	-----------------------------------------------------------------------------
void show_briefing_bitmap(grs_bitmap *bmp)
{
	grs_canvas	*curcanv_save, *bitmap_canv;

	bitmap_canv = gr_create_sub_canvas(grd_curcanv, 220*((double)SWIDTH/320), 45*((double)SHEIGHT/200), 166, 138);
	curcanv_save = grd_curcanv;
	grd_curcanv = bitmap_canv;	
#ifdef OGL
	ogl_ubitmapm_cs(0,0,(bmp->bm_w*(SWIDTH/320)),(bmp->bm_h*(SHEIGHT/200)),bmp,255,F1_0);
#else
	gr_bitmapm(0, 0, bmp);
#endif
	grd_curcanv = curcanv_save;
	d_free(bitmap_canv);
}

//	-----------------------------------------------------------------------------
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

//	-----------------------------------------------------------------------------
void init_spinning_robot(void)
{
	int x = rescale_x(138);
	int y = rescale_y(55);
	int w = rescale_x(162);
	int h = rescale_y(134);

	Robot_canv = gr_create_sub_canvas(grd_curcanv, x, y, w, h);
}

//	-----------------------------------------------------------------------------
//	Returns char width.
//	If show_robot_flag set, then show a frame of the spinning robot.
int show_char_delay(char the_char, int delay, int robot_num, int cursor_flag)
{
	int	w, h, aw;
	char	message[2];
	fix	start_time;
	int	i;

	start_time = timer_get_fixed_seconds();

	message[0] = the_char;
	message[1] = 0;

	gr_get_string_size(message, &w, &h, &aw );

	Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));

     if (delay)
      {
	if (cursor_flag && delay) {
		gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
		gr_printf(Briefing_text_x, Briefing_text_y, "_" );
	}

	for (i=0; i<2; i++) {
                if (robot_num != -1)
			show_spinning_robot_frame(robot_num);
                show_animated_bitmap();

		while (timer_get_fixed_seconds() < start_time + delay/2);

		start_time = timer_get_fixed_seconds();
	}

	// Erase cursor
        if (cursor_flag) {
		gr_set_fontcolor(Erase_color, -1);
		gr_printf(Briefing_text_x, Briefing_text_y, "_" );
	}
      }
// end additions/changed - adb

	// Draw the character
	gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
	gr_printf(Briefing_text_x+1, Briefing_text_y, message );

	return w;
}

ubyte baldguy_cheat = 0;
static ubyte bald_guy_cheat_index = 0;
char new_baldguy_pcx[] = "btexture.xxx";

#define	BALD_GUY_CHEAT_SIZE	7
#define NEW_END_GUY1	1
#define NEW_END_GUY2	3

ubyte	bald_guy_cheat_1[BALD_GUY_CHEAT_SIZE] = { KEY_B ^ 0xF0 ^ 0xab, 
	KEY_A ^ 0xE0 ^ 0xab, 
	KEY_L ^ 0xD0 ^ 0xab, 
	KEY_D ^ 0xC0 ^ 0xab, 
	KEY_G ^ 0xB0 ^ 0xab, 
	KEY_U ^ 0xA0 ^ 0xab,
	KEY_Y ^ 0x90 ^ 0xab };

void bald_guy_cheat(int key)
{
	if (key == (bald_guy_cheat_1[bald_guy_cheat_index] ^ (0xf0 - (bald_guy_cheat_index << 4)) ^ 0xab)) {
		bald_guy_cheat_index++;
		if (bald_guy_cheat_index == BALD_GUY_CHEAT_SIZE)	{
			baldguy_cheat = 1;
			bald_guy_cheat_index = 0;
		}
	} else
		bald_guy_cheat_index = 0;
}

//	-----------------------------------------------------------------------------
//	loads a briefing screen
int load_briefing_screen( int screen_num )
{
	int	pcx_error;

	if (briefing_bm.bm_data != NULL)
		gr_free_bitmap_data(&briefing_bm);

	gr_init_bitmap_data(&briefing_bm);

	if ( ((screen_num == NEW_END_GUY1) || (screen_num == NEW_END_GUY2)) && baldguy_cheat) {
		if ( bald_guy_load(new_baldguy_pcx, &briefing_bm, BM_LINEAR, gr_palette) == 0)
			return 0;
	}
	if ((pcx_error=pcx_read_bitmap( Briefing_screens_LH[screen_num].bs_name, &briefing_bm, BM_LINEAR, gr_palette ))!=PCX_ERROR_NONE) {
		Int3();
		return 0;
	}

	show_fullscr(&briefing_bm);

	gr_palette_load(gr_palette);

	set_briefing_fontcolor();

	return 0;
}

#define	KEY_DELAY_DEFAULT ((F1_0*28)/1000)

//	-----------------------------------------------------------------------------
int get_message_num(char **message)
{
	int	num=0;

	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;

	while (strlen(*message) > 0 && (**message >= '0') && (**message <= '9')) {
		num = 10*num + **message-'0';
		(*message)++;
	}

	while (strlen(*message) > 0 && *(*message)++ != 10) // Get and drop eoln
		;

	return num;
}

void title_save_game()
{
	grs_canvas * save_canv;
	grs_canvas * save_canv_data;
	grs_font * save_font;
	ubyte palette[768];

	if ( Next_level_num == 0 ) return;
	
	save_canv = grd_curcanv;
	save_font = grd_curcanv->cv_font;

	save_canv_data = gr_create_canvas( grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h );
	gr_set_current_canvas(save_canv_data);
	gr_ubitmap(0,0,&save_canv->cv_bitmap);
	gr_set_current_canvas(save_canv);
	gr_clear_canvas(gr_find_closest_color_current( 0, 0, 0));
	gr_palette_read( palette );
	gr_palette_load( gr_palette );
#ifndef SHAREWARE
	state_save_all(1);
#endif
	
	gr_set_current_canvas(save_canv);
	gr_ubitmap(0,0,&save_canv_data->cv_bitmap);
	gr_palette_load( palette );
	gr_set_curfont(save_font);
}


//	-----------------------------------------------------------------------------
void get_message_name(char **message, char *result)
{
	while (strlen(*message) > 0 && **message == ' ')
		(*message)++;

	while (strlen(*message) > 0 && (**message != ' ') && (**message != 10)) {
		if (**message != 13)
			*result++ = **message;
		(*message)++;
	}

	if (**message != 10)
		while (strlen(*message) > 0 && *(*message)++ != 10) // Get and drop eoln
			;

	*result = 0;
}

//	-----------------------------------------------------------------------------
void flash_cursor(int cursor_flag)
{
	if (cursor_flag == 0)
		return;

	if ((timer_get_fixed_seconds() % (F1_0/2) ) > (F1_0/4))
		gr_set_fontcolor(Briefing_text_colors[Current_color], -1);
	else
		gr_set_fontcolor(Erase_color, -1);

	gr_printf(Briefing_text_x, Briefing_text_y, "_" );

}

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

//	-----------------------------------------------------------------------------
//	Return true if message got aborted by user (pressed ESC), else return false.
//	Draws text, images and bitmaps actually
int show_briefing(int screen_num, char *message)
{
	int	prev_ch=-1;
	int	ch, done=0;
	briefing_screen	*bsp = &Briefing_screens[screen_num];
	int	delay_count = KEY_DELAY_DEFAULT;
	int	key_check;
	int	robot_num=-1;
	int	rval=0;
	int	tab_stop=0;
	int	flashing_cursor=0;
	int	new_page=0;
	int text_ulx = rescale_x(bsp->text_ulx);
	int text_uly = rescale_y(bsp->text_uly);
	grs_bitmap	guy_bitmap;
	int streamcount=0, guy_bitmap_show=0;

	Bitmap_name[0] = 0;

	Current_color = 0;

	gr_set_curfont( GAME_FONT );

	init_char_pos(text_ulx, text_uly);

	while (!done) {
		ch = *message++;
		if (ch == '$') {
			ch = *message++;
			if (ch == 'C') {
				Current_color = get_message_num(&message)-1;
				Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				prev_ch = 10;
			} else if (ch == 'F') { // toggle flashing cursor
				flashing_cursor = !flashing_cursor;
				prev_ch = 10;
				while (*message++ != 10)
					;
			} else if (ch == 'T') {
				tab_stop = get_message_num(&message);
				tab_stop*=(1+HIRESMODE);
				prev_ch = 10; // read to eoln
			} else if (ch == 'R') {
				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				init_spinning_robot();
				robot_num = get_message_num(&message);
				prev_ch = 10; // read to eoln
			} else if (ch == 'N') {
				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 0;
				prev_ch = 10;
			} else if (ch == 'O') {
				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, Bitmap_name);
				strcat(Bitmap_name, "#0");
				Animating_bitmap_type = 1;
				prev_ch = 10;
			} else if (ch == 'B') {
				char		bitmap_name[32];
				ubyte		temp_palette[768];
				int		iff_error;

				if (Robot_canv != NULL)
					{d_free(Robot_canv); Robot_canv=NULL;}

				get_message_name(&message, bitmap_name);
				strcat(bitmap_name, ".bbm");
				gr_init_bitmap_data (&guy_bitmap);
				iff_error = iff_read_bitmap(bitmap_name, &guy_bitmap, BM_LINEAR, temp_palette);
				Assert(iff_error == IFF_NO_ERROR);
				show_briefing_bitmap(&guy_bitmap);
				guy_bitmap_show=1;
				prev_ch = 10;
			} else if (ch == 'S') {
				int	keypress;
				fix	start_time;
				fix 	time_out_value;

				start_time = timer_get_fixed_seconds();
				start_time = timer_get_approx_seconds();
				time_out_value = start_time + i2f(60*5); // Wait 1 minute...

				while ( (keypress = local_key_inkey()) == 0 ) { // Wait for a key
					if ( timer_get_approx_seconds() > time_out_value ) {
						keypress = 0;
						break; // Time out after 1 minute..
					}
					while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
						;
					timer_delay2(50);
					gr_flip();
					show_fullscr(&briefing_bm);
					redraw_messagestream(streamcount);
					if (guy_bitmap_show)
						show_briefing_bitmap(&guy_bitmap);
					flash_cursor(flashing_cursor);
					show_spinning_robot_frame(robot_num);
					show_animated_bitmap();

					start_time += KEY_DELAY_DEFAULT/2;
				}

#ifndef NDEBUG
				if (keypress == KEY_BACKSP)
					Int3();
#endif
				if (keypress == KEY_ESC)
					rval = 1;

				flashing_cursor = 0;
				done = 1;
			} else if (ch == 'P') { // New page.

				new_page = 1;
				while (*message != 10) {
					message++; // drop carriage return after special escape sequence
				}
				message++;
				prev_ch = 10;
			} else if (ch == '$' || ch == ';') { // Print a $/;
				prev_ch = ch;
				Briefing_text_x += show_char_delay(ch, delay_count, robot_num, flashing_cursor);
			}
		} else if (ch == '\t') {		//	Tab
			if (Briefing_text_x - text_ulx < tab_stop)
				Briefing_text_x = text_ulx + tab_stop;
		} else if ((ch == ';') && (prev_ch == 10)) {
			while (*message++ != 10)
				;
			prev_ch = 10;
		} else if (ch == '\\') {
			prev_ch = ch;
		} else if (ch == 10) {
			if (prev_ch != '\\') {
				prev_ch = ch;
				Briefing_text_y += FSPACY(5)+FSPACY(5)*3/5;
				Briefing_text_x = text_ulx;
				if (Briefing_text_y > text_uly + rescale_y(bsp->text_height)) {
					Briefing_text_x = text_ulx;
					Briefing_text_y = text_uly;
				}
			} else {
				if (ch == 13)
					Int3();
				prev_ch = ch;
			}
		} else {
			messagestream[streamcount].x = Briefing_text_x;
			messagestream[streamcount].y = Briefing_text_y;
			messagestream[streamcount].color = Briefing_text_colors[Current_color];
			messagestream[streamcount].ch = ch;
			if (delay_count) {
				gr_flip();
				show_fullscr(&briefing_bm);
				redraw_messagestream(streamcount);
				if (flashing_cursor)
					gr_printf(Briefing_text_x + FSPACX(7),Briefing_text_y,"_");
			}
			if (guy_bitmap_show)
				show_briefing_bitmap(&guy_bitmap);
			streamcount++;
			prev_ch = ch;
			Briefing_text_x += show_char_delay(ch, delay_count, robot_num, flashing_cursor);

		}

// Check for Esc -> abort.
		if(delay_count)
			key_check=local_key_inkey();
		else
			key_check=0;

		if ( key_check == KEY_ESC ) {
			rval = 1;
			done = 1;
		}

		if ( key_check == KEY_ALTED+KEY_F2 )	
			title_save_game();

		if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER))
			delay_count = 0;

		if (Briefing_text_x > text_ulx + rescale_x(bsp->text_width)) {
			Briefing_text_x = text_ulx;
			Briefing_text_y += GAME_FONT->ft_h+GAME_FONT->ft_h*3/5;
		}

		if ((new_page) || (Briefing_text_y > text_uly + rescale_y(bsp->text_height))) {
			fix	start_time = 0;
			fix	time_out_value = 0;
			int	keypress;

			new_page = 0;
			start_time = timer_get_approx_seconds();
			time_out_value = start_time + i2f(60*5); // Wait 1 minute...

			while ( (keypress = local_key_inkey()) == 0 ) { // Wait for a key
				if ( timer_get_approx_seconds() > time_out_value ) {
					keypress = 0;
					break; // Time out after 1 minute..
				}
				while (timer_get_approx_seconds() < start_time + KEY_DELAY_DEFAULT/2)
					;
				timer_delay2(50);
				gr_flip();
				show_fullscr(&briefing_bm);
				redraw_messagestream(streamcount);
				if (guy_bitmap_show)
					show_briefing_bitmap(&guy_bitmap);
				flash_cursor(flashing_cursor);
				show_spinning_robot_frame(robot_num);
				show_animated_bitmap();

				start_time += KEY_DELAY_DEFAULT/2;
			}

			robot_num = -1;

#ifndef NDEBUG
			if (keypress == KEY_BACKSP)
				Int3();
#endif
			if (keypress == KEY_ESC) {
				rval = 1;
				done = 1;
			}

			Briefing_text_x = text_ulx;
			Briefing_text_y = text_uly;
			streamcount=0;
			if (guy_bitmap_show) {
				gr_free_bitmap_data (&guy_bitmap);
				guy_bitmap_show=0;
			}
			delay_count = KEY_DELAY_DEFAULT;
		}
	}


	if (briefing_bm.bm_data != NULL)
		gr_free_bitmap_data (&briefing_bm);

	if (Robot_canv != NULL)
		{d_free(Robot_canv); Robot_canv=NULL;}

	return rval;
}

//	-----------------------------------------------------------------------------
//	Return a pointer to the start of text for screen #screen_num.
char * get_briefing_message(int screen_num)
{
	char	*tptr = Briefing_text;
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

	return tptr;
}

// -----------------------------------------------------------------------------
//	Load Descent briefing text.
void load_screen_text(char *filename, char **buf)
{
	CFILE	*tfile;
	CFILE *ifile;
	int	len;
	int	have_binary = 0;

	if ((tfile = cfopen(filename,"rb")) == NULL) {
		char nfilename[30], *ptr;

		strcpy(nfilename, filename);
		if ((ptr = strrchr(nfilename, '.')))
			*ptr = '\0';
		strcat(nfilename, ".txb");
		if ((ifile = cfopen(nfilename, "rb")) == NULL)
			Error("Cannot open file %s or %s", filename, nfilename);
		have_binary = 1;

		len = cfilelength(ifile);
		MALLOC(*buf,char, len+1);
		cfread(*buf, 1, len, ifile);
		cfclose(ifile);
	} else {
		len = cfilelength(tfile);
		MALLOC(*buf, char, len+1);
		cfread(*buf, 1, len, tfile);
		cfclose(tfile);
	}

	if (have_binary)
		decode_text(*buf, len);

	*(*buf+len)='\0';
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int show_briefing_text(int screen_num)
{
	char	*message_ptr;

	message_ptr = get_briefing_message(Briefing_screens[screen_num].message_num);

	if (message_ptr==NULL)
		return (0);

	set_briefing_fontcolor();

	return show_briefing(screen_num, message_ptr);
}

void set_briefing_fontcolor ()
{
	Briefing_text_colors[0] = gr_find_closest_color_current( 0, 40, 0);

	Briefing_text_colors[1] = gr_find_closest_color_current( 40, 33, 35);

	Briefing_text_colors[2] = gr_find_closest_color_current( 8, 31, 54);

	//green
	Briefing_text_colors[0] = gr_find_closest_color_current( 0, 54, 0);
	//white
	Briefing_text_colors[1] = gr_find_closest_color_current( 42, 38, 32);

	//Begin D1X addition
	//red
	Briefing_text_colors[2] = gr_find_closest_color_current( 63, 0, 0);

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

//	-----------------------------------------------------------------------------
//	Return true if screen got aborted by user, else return false.
int show_briefing_screen( int screen_num, int allow_keys)
{
	int	rval=0;

	gr_init_bitmap_data (&briefing_bm);

	load_briefing_screen(screen_num);

	gr_palette_load(gr_palette);

	rval = show_briefing_text(screen_num);

	return rval;
}


//	-----------------------------------------------------------------------------
void do_briefing_screens(int level_num)
{
	int	abort_briefing_screens = 0;
	int	cur_briefing_screen = 0;

	if (!Briefing_text_filename[0]) //no filename?
		return;

	songs_play_song( SONG_BRIEFING, 1 );

	set_screen_mode( SCREEN_MENU );
	gr_set_current_canvas(NULL);

	key_flush();

	load_screen_text(Briefing_text_filename, &Briefing_text);

	if (level_num == 1) {
		while ((!abort_briefing_screens) && (Briefing_screens[cur_briefing_screen].level_num == 0)) {
			abort_briefing_screens = show_briefing_screen(cur_briefing_screen, 0);
			cur_briefing_screen++;
		}
	}

	if (!abort_briefing_screens) {
		for (cur_briefing_screen = 0; cur_briefing_screen < MAX_BRIEFING_SCREEN; cur_briefing_screen++)
			if (Briefing_screens[cur_briefing_screen].level_num == level_num)
				if (show_briefing_screen(cur_briefing_screen, 0))
					break;
	}

	d_free(Briefing_text);

	key_flush();
}

void show_order_form()
{
	char    exit_screen[16];

	strcpy(exit_screen, "apple.pcx");	// D1 Mac OEM Demo
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "order01.pcx"); // D1 Demo
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "warning.pcx");	// D1 Registered
	show_title_screen(exit_screen, 1);
}

#ifndef SHAREWARE
void do_registered_end_game(void)
{
	int	cur_briefing_screen;

	if (!Ending_text_filename[0]) //no filename?
		return;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
	{
		// Special ending for deathmatch!!
		int len = 40;
		
		MALLOC(Briefing_text, char, len);
		sprintf(Briefing_text, "Test");
	}
		
	load_screen_text(Ending_text_filename, &Briefing_text);

	for (cur_briefing_screen = 0; cur_briefing_screen < MAX_BRIEFING_SCREEN; cur_briefing_screen++)
		if (Briefing_screens[cur_briefing_screen].level_num == REGISTERED_ENDING_LEVEL_NUM)
			if (show_briefing_screen(cur_briefing_screen, 0))
				break;

	d_free(Briefing_text);
}
#endif

void do_shareware_end_game(void)
{
	int	cur_briefing_screen;

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
	{
		MALLOC(Briefing_text, char, 4); // Dummy
		kmatrix_view();
		return;
	}
	else 
#endif
	{
#ifdef DEST_SAT
		load_screen_text(Ending_text_filename, &Briefing_text);
#else
		load_screen_text(SHAREWARE_ENDING_FILENAME, &Briefing_text);
#endif
	}

	for (cur_briefing_screen = 0; cur_briefing_screen < MAX_BRIEFING_SCREEN; cur_briefing_screen++)
		if (Briefing_screens[cur_briefing_screen].level_num == SHAREWARE_ENDING_LEVEL_NUM)
			if (show_briefing_screen(cur_briefing_screen, 0))
				break;

	d_free(Briefing_text);
}

void do_end_game(void)
{
	set_screen_mode( SCREEN_MENU );
	gr_set_current_canvas(NULL);

	key_flush();

	#ifdef SHAREWARE
	do_shareware_end_game(); //hurrah! you win!
	#else
		#ifdef DEST_SAT
			do_shareware_end_game(); //hurrah! you win!
		#else
			do_registered_end_game(); //hurrah! you win!
		#endif
	#endif

	if (Briefing_text) {
		d_free(Briefing_text);
		Briefing_text = NULL;
	}

	key_flush();

	Function_mode = FMODE_MENU;

	Game_mode = GM_GAME_OVER;

#ifdef DEST_SAT
	show_order_form();
#endif

#ifdef SHAREWARE
	show_order_form();
#endif

}

