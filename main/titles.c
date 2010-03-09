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
#include "menu.h"
#include "mouse.h"
#include "console.h"

void title_save_game();
void set_briefing_fontcolor ();

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

	if ( rval==KEY_ALTED+KEY_F2 )	{
	 	title_save_game();
		return 0;
	}

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

void show_titles(void)
{
	char    publisher[16];
	
	if (GameArg.SysNoTitles)
		return;

	strcpy(publisher, "macplay.pcx");	// Mac Shareware
	if (!PHYSFS_exists(publisher))
		strcpy(publisher, "mplaycd.pcx");	// Mac Registered
	if (!PHYSFS_exists(publisher))
		strcpy(publisher, "iplogo1.pcx");	// PC. Only down here because it's lowres ;-)
	
	show_title_screen( publisher, 1, 1 );
	show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("logoh.pcx"))?"logoh.pcx":"logo.pcx"), 1, 1 );
	show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("descenth.pcx"))?"descenth.pcx":"descent.pcx"), 1, 1 );
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
#define Briefing_screens_LH ((SWIDTH >= 640 && SHEIGHT >= 480 && cfexist( "brief01h.pcx"))?Briefing_screens_h:Briefing_screens)

briefing_screen Briefing_screens[] = {
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
	grs_bitmap background;
	char	background_name[16];
	char	*text;
	char	*message;
	int		text_x, text_y;
	msgstream messagestream[2048];
	grs_canvas	*robot_canv;
	vms_angvec	robot_angles;
	char    bitmap_name[32];
	sbyte   door_dir, door_div_count, animating_bitmap_type;
} briefing;

void briefing_init(briefing *br, short level_num)
{
	br->level_num = level_num;
	br->cur_screen = 0;
	strcpy(br->background_name, DEFAULT_BRIEFING_BKG);
	br->robot_canv = NULL;
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
	int	i;

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

		for (i=0; i<2; i++)
		{
			if (br->bitmap_name[0] != 0)
				show_animated_bitmap(br);

			while (timer_get_fixed_seconds() < start_time + delay/2);
			if (robot_num != -1)
				show_spinning_robot_frame(br, robot_num);

			start_time = timer_get_fixed_seconds();
		}

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
int load_briefing_screen(briefing *br, char *fname)
{
	int pcx_error;

	if (br->background.bm_data != NULL)
		gr_free_bitmap_data(&br->background);

	gr_init_bitmap_data(&br->background);

	strcpy (br->background_name,fname);

	if (!stricmp(fname, "brief02h.pcx") && baldguy_cheat)
		if ( bald_guy_load(new_baldguy_pcx, &br->background, BM_LINEAR, gr_palette) == 0)
			return 0;

	if ((pcx_error = pcx_read_bitmap(fname, &br->background, BM_LINEAR, gr_palette))!=PCX_ERROR_NONE)
		Error( "Error loading briefing screen <%s>, PCX load error: %s (%i)\n",fname, pcx_errormsg(pcx_error), pcx_error);

	show_fullscr(&br->background);

	gr_palette_load(gr_palette);

	set_briefing_fontcolor();

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
	state_save_all(1, 0);
#endif
	
	gr_set_current_canvas(save_canv);
	gr_ubitmap(0,0,&save_canv_data->cv_bitmap);
	gr_palette_load( palette );
	gr_set_curfont(save_font);
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
	int	prev_ch=-1;
	int	ch, done=0;
	briefing_screen	*bsp;
	int	delay_count = KEY_DELAY_DEFAULT;
	int	key_check;
	int	robot_num=-1;
	int	rval=0;
	static int tab_stop=0;
	int	flashing_cursor=0;
	int	new_page=0;
	char fname[15];
	grs_bitmap  guy_bitmap;
	int streamcount=0, guy_bitmap_show=0;

	strncpy(fname, br->background_name, 15);//fname = br->background_name;
	br->bitmap_name[0] = 0;
	Current_color = 0;

	gr_set_curfont( GAME_FONT );

	MALLOC(bsp, briefing_screen, 1);
	memcpy(bsp, &Briefing_screens[br->cur_screen], sizeof(briefing_screen));
	bsp->text_ulx = rescale_x(bsp->text_ulx);
	bsp->text_uly = rescale_y(bsp->text_uly);
	bsp->text_width = rescale_x(bsp->text_width);
	bsp->text_height = rescale_y(bsp->text_height);
	init_char_pos(br, bsp->text_ulx, bsp->text_uly);

	while (!done) {
		ch = *br->message++;
		if (ch == '$') {
			ch = *br->message++;
			if (ch == 'C') {
				Current_color = get_message_num(&br->message)-1;
				Assert((Current_color >= 0) && (Current_color < MAX_BRIEFING_COLORS));
				prev_ch = 10;
			} else if (ch == 'F') {     // toggle flashing cursor
				flashing_cursor = !flashing_cursor;
				prev_ch = 10;
				while (*br->message++ != 10)
					;
			} else if (ch == 'T') {
				tab_stop = get_message_num(&br->message);
				tab_stop*=(1+HIRESMODE);
				prev_ch = 10;							//	read to eoln
			} else if (ch == 'R') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				init_spinning_robot(br);
				robot_num = get_message_num(&br->message);
				while (*br->message++ != 10)
					;
				prev_ch = 10;                           // read to eoln
			} else if (ch == 'N') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				get_message_name(&br->message, br->bitmap_name);
				strcat(br->bitmap_name, "#0");
				br->animating_bitmap_type = 0;
				prev_ch = 10;
			} else if (ch == 'O') {
				if (br->robot_canv != NULL)
				{
					d_free(br->robot_canv);
					br->robot_canv=NULL;
				}

				get_message_name(&br->message, br->bitmap_name);
				strcat(br->bitmap_name, "#0");
				br->animating_bitmap_type = 1;
				prev_ch = 10;
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
				gr_init_bitmap_data (&guy_bitmap);
				iff_error = iff_read_bitmap(bitmap_name, &guy_bitmap, BM_LINEAR, temp_palette);
				Assert(iff_error == IFF_NO_ERROR);
				gr_remap_bitmap_good( &guy_bitmap, temp_palette, -1, -1 );

				show_briefing_bitmap(&guy_bitmap);
				guy_bitmap_show=1;
				prev_ch = 10;
			} else if (ch == 'S') {
				int keypress;
				fix start_time;

				start_time = timer_get_fixed_seconds();
				while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key

					while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
						;
					timer_delay2(50);
					gr_flip();
					show_fullscr(&br->background);
					redraw_messagestream(br->messagestream, streamcount);
					if (guy_bitmap_show)
						show_briefing_bitmap(&guy_bitmap);
					flash_cursor(br, flashing_cursor);
					if (robot_num != -1)
						show_spinning_robot_frame(br, robot_num);

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

				flashing_cursor = 0;
				done = 1;
			} else if (ch == 'P') {		//	New page.

				new_page = 1;

				while (*br->message != 10) {
					br->message++;	//	drop carriage return after special escape sequence
				}
				br->message++;
				prev_ch = 10;
			} else if (ch == '$' || ch == ';') { // Print a $/;
				prev_ch = ch;
				br->text_x += show_char_delay(br, ch, delay_count, robot_num, flashing_cursor);
			}
		} else if (ch == '\t') {		//	Tab
			if (br->text_x - bsp->text_ulx < tab_stop)
				br->text_x = bsp->text_ulx + tab_stop;
		} else if ((ch == ';') && (prev_ch == 10)) {
			while (*br->message++ != 10)
				;
			prev_ch = 10;
		} else if (ch == '\\') {
			prev_ch = ch;
		} else if (ch == 10) {
			if (prev_ch != '\\') {
				prev_ch = ch;
				br->text_y += FSPACY(5)+FSPACY(5)*3/5;
				br->text_x = bsp->text_ulx;
				if (br->text_y > bsp->text_uly + bsp->text_height) {
					load_briefing_screen(br, Briefing_screens[br->cur_screen].bs_name);
					br->text_x = bsp->text_ulx;
					br->text_y = bsp->text_uly;
				}
			} else {
				if (ch == 13)		//Can this happen? Above says ch==10
					Int3();
				prev_ch = ch;
			}
		} else {
			br->messagestream[streamcount].x = br->text_x;
			br->messagestream[streamcount].y = br->text_y;
			br->messagestream[streamcount].color = Briefing_text_colors[Current_color];
			br->messagestream[streamcount].ch = ch;
			if (delay_count) {
				gr_flip();
				show_fullscr(&br->background);
				redraw_messagestream(br->messagestream, streamcount);
				if (flashing_cursor)
					gr_printf(br->text_x + FSPACX(7),br->text_y,"_");
			}
			if (guy_bitmap_show)
				show_briefing_bitmap(&guy_bitmap);
			streamcount++;

			prev_ch = ch;
			br->text_x += show_char_delay(br, ch, delay_count, robot_num, flashing_cursor);

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

		if ( key_check == KEY_ALTED+KEY_F2 )	
			title_save_game();

		if ((key_check == KEY_SPACEBAR) || (key_check == KEY_ENTER))
			delay_count = 0;

		if (br->text_x > bsp->text_ulx + bsp->text_width) {
			br->text_x = bsp->text_ulx;
			br->text_y += bsp->text_uly;
		}

		if ((new_page) || (br->text_y > bsp->text_uly + bsp->text_height)) {
			fix	start_time = 0;
			int	keypress;

			new_page = 0;
			start_time = timer_get_fixed_seconds();
			while ( (keypress = local_key_inkey()) == 0 ) {		//	Wait for a key
				while (timer_get_fixed_seconds() < start_time + KEY_DELAY_DEFAULT/2)
					;
				timer_delay2(50);
				gr_flip();
				show_fullscr(&br->background);
				redraw_messagestream(br->messagestream, streamcount);
				if (guy_bitmap_show)
					show_briefing_bitmap(&guy_bitmap);
				flash_cursor(br, flashing_cursor);
				if (robot_num != -1)
					show_spinning_robot_frame(br, robot_num);
				if (br->bitmap_name[0] != 0)
					show_animated_bitmap(br);
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
			load_briefing_screen(br, fname);
			br->text_x = bsp->text_ulx;
			br->text_y = bsp->text_uly;

			streamcount=0;
			if (guy_bitmap_show) {
				gr_free_bitmap_data (&guy_bitmap);
				guy_bitmap_show=0;
			}

			delay_count = KEY_DELAY_DEFAULT;
		}
	}

	if (br->robot_canv != NULL)
		{d_free(br->robot_canv); br->robot_canv=NULL;}

	d_free(bsp);
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
	int	len;
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

	return (1);
}

//-----------------------------------------------------------------------------
// Return true if message got aborted, else return false.
int show_briefing_text(briefing *br)
{
	br->message = get_briefing_message(br, Briefing_screens[br->cur_screen].message_num);

	if (br->message==NULL)
		return (0);

	set_briefing_fontcolor();

	return show_briefing(br);
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

//-----------------------------------------------------------------------------
// Return true if screen got aborted by user, else return false.
int show_briefing_screen(briefing *br)
{
	int     rval=0;

	gr_init_bitmap_data (&br->background);

	load_briefing_screen(br, Briefing_screens_LH[br->cur_screen].bs_name);
	show_fullscr(&br->background );
	gr_palette_load(gr_palette);

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

	songs_play_song( SONG_BRIEFING, 1 );

	set_screen_mode( SCREEN_MENU );
	gr_set_current_canvas(NULL);

	key_flush();

	if (br->level_num == 1) {
		while ((!abort_briefing_screens) && (Briefing_screens[br->cur_screen].level_num == 0)) {
			abort_briefing_screens = show_briefing_screen(br);
			br->cur_screen++;
		}
	}

	if (!abort_briefing_screens) {
		for (br->cur_screen = 0; br->cur_screen < MAX_BRIEFING_SCREEN; br->cur_screen++)
			if (Briefing_screens[br->cur_screen].level_num == br->level_num)
				if (show_briefing_screen(br))
					break;
	}

	d_free(br->text);
	d_free(br);

	key_flush();
}

#ifndef SHAREWARE
void do_registered_end_game(void)
{
	briefing *br;

	if (!Ending_text_filename[0]) //no filename?
		return;

	MALLOC(br, briefing, 1);
	if (!br)
		return;
	
	briefing_init(br, REGISTERED_ENDING_LEVEL_NUM);
	
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
	{
		// Special ending for deathmatch!!
		int len = 40;
		
		MALLOC(br->text, char, len);
		sprintf(br->text, "Test");
	}
		
	load_screen_text(Ending_text_filename, &br->text);

	for (br->cur_screen = 0; br->cur_screen < MAX_BRIEFING_SCREEN; br->cur_screen++)
		if (Briefing_screens[br->cur_screen].level_num == br->level_num)
			if (show_briefing_screen(br))
				break;

	d_free(br->text);
	d_free(br);
}
#endif

void do_shareware_end_game(void)
{
	briefing *br;

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
	{
		kmatrix_view(1);
		return;
	}
	else 
#endif
	{
		MALLOC(br, briefing, 1);
		if (!br)
			return;
		
		briefing_init(br, SHAREWARE_ENDING_LEVEL_NUM);
		
#ifdef DEST_SAT
		load_screen_text(Ending_text_filename, &br->text);
#else
		load_screen_text(SHAREWARE_ENDING_FILENAME, &br->text);
#endif
	}

	for (br->cur_screen = 0; br->cur_screen < MAX_BRIEFING_SCREEN; br->cur_screen++)
		if (Briefing_screens[br->cur_screen].level_num == br->level_num)
			if (show_briefing_screen(br))
				break;

	d_free(br->text);
	d_free(br);
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

void show_order_form()
{
	char    exit_screen[16];

	strcpy(exit_screen, "warning.pcx");	// D1 Registered
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "apple.pcx");	// D1 Mac OEM Demo
	if (! cfexist(exit_screen))
		strcpy(exit_screen, "order01.pcx"); // D1 Demo
	show_title_screen(exit_screen, 1, 1);
}

