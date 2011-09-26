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
 * Routines to configure keyboard, joystick, etc..
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "console.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "kconfig.h"
#include "gauges.h"
#include "rbaudio.h"
#include "render.h"
#include "digi.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "timer.h"
#include "text.h"
#include "player.h"
#include "menu.h"
#include "automap.h"
#include "args.h"
#include "lighting.h"
#include "ai.h"
#include "cntrlcen.h"
#include "collide.h"
#include "playsave.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#define TABLE_CREATION 1

// Array used to 'blink' the cursor while waiting for a keypress.
sbyte fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

char *invert_text[2] = { "N", "Y" };
char *joybutton_text[JOY_MAX_BUTTONS];
char *joyaxis_text[JOY_MAX_AXES];
char *mouseaxis_text[3] = { "L/R", "F/B", "WHEEL" };
char *mousebutton_text[16] = { "LEFT", "RIGHT", "MID", "M4", "M5", "M6", "M7", "M8", "M9", "M10","M11","M12","M13","M14","M15","M16" };

ubyte system_keys[19] = { KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN, KEY_CAPSLOCK, KEY_SCROLLOCK, KEY_NUMLOCK }; // KEY_*LOCK should always be last since we wanna skip these if -nostickykeys

control_info Controls;

fix Cruise_speed=0;

#define BT_KEY 			0
#define BT_MOUSE_BUTTON 	1
#define BT_MOUSE_AXIS		2
#define BT_JOY_BUTTON 		3
#define BT_JOY_AXIS		4
#define BT_INVERT		5
#define STATE_BIT1		1
#define STATE_BIT2		2
#define STATE_BIT3		4
#define STATE_BIT4		8
#define STATE_BIT5		16

char *btype_text[] = { "BT_KEY", "BT_MOUSE_BUTTON", "BT_MOUSE_AXIS", "BT_JOY_BUTTON", "BT_JOY_AXIS", "BT_INVERT" };

#define INFO_Y (188)

typedef struct kc_item {
	short id;				// The id of this item
	short x, y;              // x, y pos of label
	short w1;                // x pos of input field
	short w2;                // length of input field
	short u,d,l,r;           // neighboring field ids for cursor navigation
        //short text_num1;
        char *text;
	ubyte type;
	ubyte value;		// what key,button,etc
	ubyte *ci_state_ptr;
	int state_bit;
	ubyte *ci_count_ptr;
} kc_item;

typedef struct kc_menu
{
	window	*wind;
	kc_item	*items;
	char	*title;
	int	nitems;
	int	citem;
	int	old_jaxis[JOY_MAX_AXES];
	int	old_maxis[3];
	ubyte	changing;
	ubyte	q_fade_i;	// for flashing the question mark
	ubyte	mouse_state;
} kc_menu;

ubyte DefaultKeySettings[3][MAX_CONTROLS] = {
{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x15,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x0,0x0,0x0,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
};
ubyte DefaultKeySettingsD1X[MAX_D1X_CONTROLS] = { 0x2,0xff,0xff,0x3,0xff,0xff,0x4,0xff,0xff,0x5,0xff,0xff,0x6,0xff,0xff,0x7,0xff,0xff,0x8,0xff,0xff,0x9,0xff,0xff,0xa,0xff,0xff,0xb,0xff,0xff };

//	  id,  x,  y, w1, w2,  u,  d,   l, r,     text,   type, value
kc_item kc_keyboard[NUM_KEY_CONTROLS] = {
	{  0, 15, 49, 71, 26, 43,  2, 49,  1,"Pitch forward", BT_KEY, 255, &Controls.pitch_forward_state, STATE_BIT1, NULL },
	{  1, 15, 49,100, 26, 48,  3,  0, 24,"Pitch forward", BT_KEY, 255, &Controls.pitch_forward_state, STATE_BIT2, NULL },
	{  2, 15, 57, 71, 26,  0,  4, 25,  3,"Pitch backward", BT_KEY, 255, &Controls.pitch_backward_state, STATE_BIT1, NULL },
	{  3, 15, 57,100, 26,  1,  5,  2, 26,"Pitch backward", BT_KEY, 255, &Controls.pitch_backward_state, STATE_BIT2, NULL },
	{  4, 15, 65, 71, 26,  2,  6, 27,  5,"Turn left", BT_KEY, 255, &Controls.heading_left_state, STATE_BIT1, NULL },
	{  5, 15, 65,100, 26,  3,  7,  4, 28,"Turn left", BT_KEY, 255, &Controls.heading_left_state, STATE_BIT2, NULL },
	{  6, 15, 73, 71, 26,  4,  8, 29,  7,"Turn right", BT_KEY, 255, &Controls.heading_right_state, STATE_BIT1, NULL },
	{  7, 15, 73,100, 26,  5,  9,  6, 34,"Turn right", BT_KEY, 255, &Controls.heading_right_state, STATE_BIT2, NULL },
	{  8, 15, 85, 71, 26,  6, 10, 35,  9,"Slide on", BT_KEY, 255, &Controls.slide_on_state, STATE_BIT1, NULL },
	{  9, 15, 85,100, 26,  7, 11,  8, 36,"Slide on", BT_KEY, 255, &Controls.slide_on_state, STATE_BIT2, NULL },
	{ 10, 15, 93, 71, 26,  8, 12, 37, 11,"Slide left", BT_KEY, 255, &Controls.slide_left_state, STATE_BIT1, NULL },
	{ 11, 15, 93,100, 26,  9, 13, 10, 44,"Slide left", BT_KEY, 255, &Controls.slide_left_state, STATE_BIT2, NULL },
	{ 12, 15,101, 71, 26, 10, 14, 45, 13,"Slide right", BT_KEY, 255, &Controls.slide_right_state, STATE_BIT1, NULL },
	{ 13, 15,101,100, 26, 11, 15, 12, 30,"Slide right", BT_KEY, 255, &Controls.slide_right_state, STATE_BIT2, NULL },
	{ 14, 15,109, 71, 26, 12, 16, 31, 15,"Slide up", BT_KEY, 255, &Controls.slide_up_state, STATE_BIT1, NULL },
	{ 15, 15,109,100, 26, 13, 17, 14, 32,"Slide up", BT_KEY, 255, &Controls.slide_up_state, STATE_BIT2, NULL },
	{ 16, 15,117, 71, 26, 14, 18, 33, 17,"Slide down", BT_KEY, 255, &Controls.slide_down_state, STATE_BIT1, NULL },
	{ 17, 15,117,100, 26, 15, 19, 16, 38,"Slide down", BT_KEY, 255, &Controls.slide_down_state, STATE_BIT2, NULL },
	{ 18, 15,129, 71, 26, 16, 20, 39, 19,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT1, NULL },
	{ 19, 15,129,100, 26, 17, 21, 18, 40,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT2, NULL },
	{ 20, 15,137, 71, 26, 18, 22, 41, 21,"Bank left", BT_KEY, 255, &Controls.bank_left_state, STATE_BIT1, NULL },
	{ 21, 15,137,100, 26, 19, 23, 20, 42,"Bank left", BT_KEY, 255, &Controls.bank_left_state, STATE_BIT2, NULL },
	{ 22, 15,145, 71, 26, 20, 46, 43, 23,"Bank right", BT_KEY, 255, &Controls.bank_right_state, STATE_BIT1, NULL },
	{ 23, 15,145,100, 26, 21, 47, 22, 46,"Bank right", BT_KEY, 255, &Controls.bank_right_state, STATE_BIT2, NULL },
	{ 24,158, 49, 83, 26, 49, 26,  1, 25,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT1, &Controls.fire_primary_count },
	{ 25,158, 49,112, 26, 42, 27, 24,  2,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT2, &Controls.fire_primary_count },
	{ 26,158, 57, 83, 26, 24, 28,  3, 27,"Fire secondary", BT_KEY, 255, &Controls.fire_secondary_state, STATE_BIT1, &Controls.fire_secondary_count },
	{ 27,158, 57,112, 26, 25, 29, 26,  4,"Fire secondary", BT_KEY, 255, &Controls.fire_secondary_state, STATE_BIT2, &Controls.fire_secondary_count },
	{ 28,158, 65, 83, 26, 26, 34,  5, 29,"Fire flare", BT_KEY, 255, NULL, 0, &Controls.fire_flare_count },
	{ 29,158, 65,112, 26, 27, 35, 28,  6,"Fire flare", BT_KEY, 255, NULL, 0, &Controls.fire_flare_count },
	{ 30,158,105, 83, 26, 44, 32, 13, 31,"Accelerate", BT_KEY, 255, &Controls.accelerate_state, STATE_BIT1, NULL },
	{ 31,158,105,112, 26, 45, 33, 30, 14,"Accelerate", BT_KEY, 255, &Controls.accelerate_state, STATE_BIT2, NULL },
	{ 32,158,113, 83, 26, 30, 38, 15, 33,"Reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT1, NULL },
	{ 33,158,113,112, 26, 31, 39, 32, 16,"Reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT2, NULL },
	{ 34,158, 73, 83, 26, 28, 36,  7, 35,"Drop bomb", BT_KEY, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 35,158, 73,112, 26, 29, 37, 34,  8,"Drop bomb", BT_KEY, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 36,158, 85, 83, 26, 34, 44,  9, 37,"Rear view", BT_KEY, 255, &Controls.rear_view_state, STATE_BIT1, &Controls.rear_view_count },
	{ 37,158, 85,112, 26, 35, 45, 36, 10,"Rear view", BT_KEY, 255, &Controls.rear_view_state, STATE_BIT2, &Controls.rear_view_count },
	{ 38,158,125, 83, 26, 32, 40, 17, 39,"Cruise faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT1, NULL },
	{ 39,158,125,112, 26, 33, 41, 38, 18,"Cruise faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT2, NULL },
	{ 40,158,133, 83, 26, 38, 42, 19, 41,"Cruise slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT1, NULL },
	{ 41,158,133,112, 26, 39, 43, 40, 20,"Cruise slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT2, NULL },
	{ 42,158,141, 83, 26, 40, 25, 21, 43,"Cruise off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
	{ 43,158,141,112, 26, 41,  0, 42, 22,"Cruise off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
	{ 44,158, 93, 83, 26, 36, 30, 11, 45,"Automap", BT_KEY, 255, &Controls.automap_state, STATE_BIT1, &Controls.automap_count },
	{ 45,158, 93,112, 26, 37, 31, 44, 12,"Automap", BT_KEY, 255, &Controls.automap_state, STATE_BIT2, &Controls.automap_count },
	{ 46, 15,157, 71, 26, 22, 48, 23, 47,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 47, 15,157,100, 26, 23, 49, 46, 48,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 48, 15,165, 71, 26, 46,  1, 47, 49,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 49, 15,165,100, 26, 47, 24, 48,  0,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
};
kc_item kc_joystick[NUM_JOYSTICK_CONTROLS] = {
	{  0, 22, 46, 82, 26, 15,  1, 24, 29,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT3, &Controls.fire_primary_count },
	{  1, 22, 54, 82, 26,  0,  4, 34, 30,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT3, &Controls.fire_secondary_count },
	{  2, 22, 78, 82, 26, 26,  3, 37, 31,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT3, NULL },
	{  3, 22, 86, 82, 26,  2, 25, 38, 32,"Reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT3, NULL },
	{  4, 22, 62, 82, 26,  1, 26, 35, 33,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{  5,174, 46, 74, 26, 23,  6, 29, 34,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT3, NULL },
	{  6,174, 54, 74, 26,  5,  7, 30, 35,"Slide left", BT_JOY_BUTTON, 255, &Controls.slide_left_state, STATE_BIT3, NULL },
	{  7,174, 62, 74, 26,  6,  8, 33, 36,"Slide right", BT_JOY_BUTTON, 255, &Controls.slide_right_state, STATE_BIT3, NULL },
	{  8,174, 70, 74, 26,  7,  9, 43, 37,"Slide up", BT_JOY_BUTTON, 255, &Controls.slide_up_state, STATE_BIT3, NULL },
	{  9,174, 78, 74, 26,  8, 10, 31, 38,"Slide down", BT_JOY_BUTTON, 255, &Controls.slide_down_state, STATE_BIT3, NULL },
	{ 10,174, 86, 74, 26,  9, 11, 32, 39,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT3, NULL },
	{ 11,174, 94, 74, 26, 10, 12, 42, 40,"Bank left", BT_JOY_BUTTON, 255, &Controls.bank_left_state, STATE_BIT3, NULL },
	{ 12,174,102, 74, 26, 11, 44, 28, 41,"Bank right", BT_JOY_BUTTON, 255, &Controls.bank_right_state, STATE_BIT3, NULL },
	{ 13, 22,154, 51, 26, 47, 15, 47, 14,"Pitch U/D", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 14, 22,154, 99,  8, 27, 16, 13, 17,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
	{ 15, 22,162, 51, 26, 13,  0, 18, 16,"Turn L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 16, 22,162, 99,  8, 14, 29, 15, 19,"Turn L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 17,164,154, 58, 26, 28, 19, 14, 18,"Slide L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 18,164,154,106,  8, 45, 20, 17, 15,"Slide L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 19,164,162, 58, 26, 17, 21, 16, 20,"Slide U/D", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 20,164,162,106,  8, 18, 22, 19, 21,"Slide U/D", BT_INVERT, 255, NULL, 0, NULL },
	{ 21,164,170, 58, 26, 19, 23, 20, 22,"Bank L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 22,164,170,106,  8, 20, 24, 21, 23,"Bank L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 23,164,178, 58, 26, 21,  5, 22, 24,"Throttle", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 24,164,178,106,  8, 22, 34, 23,  0,"Throttle", BT_INVERT, 255, NULL, 0, NULL },
	{ 25, 22, 94, 82, 26,  3, 27, 39, 42,"Rear view", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT3, &Controls.rear_view_count },
	{ 26, 22, 70, 82, 26,  4,  2, 36, 43,"Drop bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 27, 22,102, 82, 26, 25, 14, 40, 28,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT3, &Controls.automap_count },
	{ 28, 22,102,111, 26, 42, 17, 27, 12,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT4, &Controls.automap_count },
	{ 29, 22, 46,111, 26, 16, 30,  0,  5,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT4, &Controls.fire_primary_count },
	{ 30, 22, 54,111, 26, 29, 33,  1,  6,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT4, &Controls.fire_secondary_count },
	{ 31, 22, 78,111, 26, 43, 32,  2,  9,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT4, NULL },
	{ 32, 22, 86,111, 26, 31, 42,  3, 10,"Reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT4, NULL },
	{ 33, 22, 62,111, 26, 30, 43,  4,  7,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{ 34,174, 46,104, 26, 24, 35,  5,  1,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT4, NULL },
	{ 35,174, 54,104, 26, 34, 36,  6,  4,"Slide left", BT_JOY_BUTTON, 255, &Controls.slide_left_state, STATE_BIT4, NULL },
	{ 36,174, 62,104, 26, 35, 37,  7, 26,"Slide right", BT_JOY_BUTTON, 255, &Controls.slide_right_state, STATE_BIT4, NULL },
	{ 37,174, 70,104, 26, 36, 38,  8,  2,"Slide up", BT_JOY_BUTTON, 255, &Controls.slide_up_state, STATE_BIT4, NULL },
	{ 38,174, 78,104, 26, 37, 39,  9,  3,"Slide down", BT_JOY_BUTTON, 255, &Controls.slide_down_state, STATE_BIT4, NULL },
	{ 39,174, 86,104, 26, 38, 40, 10, 25,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT4, NULL },
	{ 40,174, 94,104, 26, 39, 41, 11, 27,"Bank left", BT_JOY_BUTTON, 255, &Controls.bank_left_state, STATE_BIT4, NULL },
	{ 41,174,102,104, 26, 40, 46, 12, 44,"Bank right", BT_JOY_BUTTON, 255, &Controls.bank_right_state, STATE_BIT4, NULL },
	{ 42, 22, 94,111, 26, 32, 28, 25, 11,"Rear view", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT4, &Controls.rear_view_count },
	{ 43, 22, 70,111, 26, 33, 31, 26,  8,"Drop bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 44,174,110, 74, 26, 12, 45, 41, 46,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 45,174,118, 74, 26, 44, 18, 46, 47,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 46,174,110,104, 26, 41, 47, 44, 45,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 47,174,118,104, 26, 46, 13, 45, 13,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
};
kc_item kc_mouse[NUM_MOUSE_CONTROLS] = {
	{  0, 25, 46, 85, 26, 19,  1, 20,  5,"Fire primary", BT_MOUSE_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT5, &Controls.fire_primary_count },
	{  1, 25, 54, 85, 26,  0,  4,  5,  6,"Fire secondary", BT_MOUSE_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT5, &Controls.fire_secondary_count },
	{  2, 25, 78, 85, 26, 26,  3,  8,  9,"Accelerate", BT_MOUSE_BUTTON, 255, &Controls.accelerate_state, STATE_BIT5, NULL },
	{  3, 25, 86, 85, 26,  2, 25,  9, 10,"Reverse", BT_MOUSE_BUTTON, 255, &Controls.reverse_state, STATE_BIT5, NULL },
	{  4, 25, 62, 85, 26,  1, 26,  6,  7,"Fire flare", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{  5,180, 46, 59, 26, 23,  6,  0,  1,"Slide on", BT_MOUSE_BUTTON, 255, &Controls.slide_on_state, STATE_BIT5, NULL },
	{  6,180, 54, 59, 26,  5,  7,  1,  4,"Slide left", BT_MOUSE_BUTTON, 255, &Controls.slide_left_state, STATE_BIT5, NULL },
	{  7,180, 62, 59, 26,  6,  8,  4, 26,"Slide right", BT_MOUSE_BUTTON, 255, &Controls.slide_right_state, STATE_BIT5, NULL },
	{  8,180, 70, 59, 26,  7,  9, 26,  2,"Slide up", BT_MOUSE_BUTTON, 255, &Controls.slide_up_state, STATE_BIT5, NULL },
	{  9,180, 78, 59, 26,  8, 10,  2,  3,"Slide down", BT_MOUSE_BUTTON, 255, &Controls.slide_down_state, STATE_BIT5, NULL },
	{ 10,180, 86, 59, 26,  9, 11,  3, 25,"Bank on", BT_MOUSE_BUTTON, 255, &Controls.bank_on_state, STATE_BIT5, NULL },
	{ 11,180, 94, 59, 26, 10, 12, 25, 27,"Bank left", BT_MOUSE_BUTTON, 255, &Controls.bank_left_state, STATE_BIT5, NULL },
	{ 12,180,102, 59, 26, 11, 22, 27, 28,"Bank right", BT_MOUSE_BUTTON, 255, &Controls.bank_right_state, STATE_BIT5, NULL },
	{ 13, 25,154, 58, 26, 24, 15, 28, 14,"Pitch U/D", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 14, 25,154,106,  8, 28, 16, 13, 21,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
	{ 15, 25,162, 58, 26, 13, 17, 22, 16,"Turn L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 16, 25,162,106,  8, 14, 18, 15, 23,"Turn L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 17, 25,170, 58, 26, 15, 19, 24, 18,"Slide L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 18, 25,170,106,  8, 16, 20, 17, 19,"Slide L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 19, 25,178, 58, 26, 17,  0, 18, 20,"Slide U/D", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 20, 25,178,106,  8, 18, 21, 19,  0,"Slide U/D", BT_INVERT, 255, NULL, 0, NULL },
	{ 21,180,154, 58, 26, 20, 23, 14, 22,"Bank L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 22,180,154,106,  8, 12, 24, 21, 15,"Bank L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 23,180,162, 58, 26, 21,  5, 16, 24,"Throttle", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 24,180,162,106,  8, 22, 13, 23, 17,"Throttle", BT_INVERT, 255, NULL, 0, NULL },
	{ 25, 25, 94, 85, 26,  3, 27, 10, 11,"Rear view", BT_MOUSE_BUTTON, 255, &Controls.rear_view_state, STATE_BIT5, &Controls.rear_view_count },
	{ 26, 25, 70, 85, 26,  4,  2,  7,  8,"Drop bomb", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 27, 25,102, 85, 26, 25, 28, 11, 12,"Cycle Primary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 28, 25,110, 85, 26, 27, 14, 12, 13,"Cycle Secondary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
};
kc_item kc_d1x[NUM_D1X_CONTROLS] = {
	{  0, 15, 69,142, 26, 29,  3, 29,  1,"LASER CANNON", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{  1, 15, 69,200, 26, 27,  4,  0,  2,"LASER CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  2, 15, 69,258, 26, 28,  5,  1,  3,"LASER CANNON", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  3, 15, 77,142, 26,  0,  6,  2,  4,"VULCAN CANNON", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{  4, 15, 77,200, 26,  1,  7,  3,  5,"VULCAN CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  5, 15, 77,258, 26,  2,  8,  4,  6,"VULCAN CANNON", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  6, 15, 85,142, 26,  3,  9,  5,  7,"SPREADFIRE CANNON", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{  7, 15, 85,200, 26,  4, 10,  6,  8,"SPREADFIRE CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  8, 15, 85,258, 26,  5, 11,  7,  9,"SPREADFIRE CANNON", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{  9, 15, 93,142, 26,  6, 12,  8, 10,"PLASMA CANNON", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 10, 15, 93,200, 26,  7, 13,  9, 11,"PLASMA CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 11, 15, 93,258, 26,  8, 14, 10, 12,"PLASMA CANNON", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 12, 15,101,142, 26,  9, 15, 11, 13,"FUSION CANNON", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 13, 15,101,200, 26, 10, 16, 12, 14,"FUSION CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 14, 15,101,258, 26, 11, 17, 13, 15,"FUSION CANNON", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 15,109,142, 26, 12, 18, 14, 16,"CONCUSSION MISSILE", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 16, 15,109,200, 26, 13, 19, 15, 17,"CONCUSSION MISSILE", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 17, 15,109,258, 26, 14, 20, 16, 18,"CONCUSSION MISSILE", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 18, 15,117,142, 26, 15, 21, 17, 19,"HOMING MISSILE", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 19, 15,117,200, 26, 16, 22, 18, 20,"HOMING MISSILE", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 20, 15,117,258, 26, 17, 23, 19, 21,"HOMING MISSILE", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 21, 15,125,142, 26, 18, 24, 20, 22,"PROXIMITY BOMB", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 22, 15,125,200, 26, 19, 25, 21, 23,"PROXIMITY BOMB", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 23, 15,125,258, 26, 20, 26, 22, 24,"PROXIMITY BOMB", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 24, 15,133,142, 26, 21, 27, 23, 25,"SMART MISSILE", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 25, 15,133,200, 26, 22, 28, 24, 26,"SMART MISSILE", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 26, 15,133,258, 26, 23, 29, 25, 27,"SMART MISSILE", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 27, 15,141,142, 26, 24,  1, 26, 28,"MEGA MISSILE", BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 28, 15,141,200, 26, 25,  2, 27, 29,"MEGA MISSILE", BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 29, 15,141,258, 26, 26,  0, 28,  0,"MEGA MISSILE", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
};

void kc_drawitem( kc_item *item, int is_current );
void kc_change_key( kc_menu *menu, d_event *event, kc_item * item );
void kc_change_joybutton( kc_menu *menu, d_event *event, kc_item * item );
void kc_change_mousebutton( kc_menu *menu, d_event *event, kc_item * item );
void kc_change_joyaxis( kc_menu *menu, d_event *event, kc_item * item );
void kc_change_mouseaxis( kc_menu *menu, d_event *event, kc_item * item );
void kc_change_invert( kc_menu *menu, kc_item * item );

#ifdef TABLE_CREATION
int find_item_at( kc_item * items, int nitems, int x, int y )
{
	int i;
	
	for (i=0; i<nitems; i++ )	{
		if ( ((items[i].x+items[i].w1)==x) && (items[i].y==y))
			return i;
	}
	return -1;
}

int find_next_item_up( kc_item * items, int nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x+items[citem].w1;
	
	do {	
		y--;
		if ( y < 0 ) {
			y = grd_curcanv->cv_bitmap.bm_h-1;
			x--;
			if ( x < 0 ) {
				x = grd_curcanv->cv_bitmap.bm_w-1;
			}
		}
		i = find_item_at( items, nitems, x, y );
	} while ( i < 0 );
	
	return i;
}

int find_next_item_down( kc_item * items, int nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x+items[citem].w1;
	
	do {	
		y++;
		if ( y > grd_curcanv->cv_bitmap.bm_h-1 ) {
			y = 0;
			x++;
			if ( x > grd_curcanv->cv_bitmap.bm_w-1 ) {
				x = 0;
			}
		}
		i = find_item_at( items, nitems, x, y );
	} while ( i < 0 );
	
	return i;
}

int find_next_item_right( kc_item * items, int nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x+items[citem].w1;
	
	do {	
		x++;
		if ( x > grd_curcanv->cv_bitmap.bm_w-1 ) {
			x = 0;
			y++;
			if ( y > grd_curcanv->cv_bitmap.bm_h-1 ) {
				y = 0;
			}
		}
		i = find_item_at( items, nitems, x, y );
	} while ( i < 0 );
	
	return i;
}

int find_next_item_left( kc_item * items, int nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x+items[citem].w1;
	
	do {	
		x--;
		if ( x < 0 ) {
			x = grd_curcanv->cv_bitmap.bm_w-1;
			y--;
			if ( y < 0 ) {
				y = grd_curcanv->cv_bitmap.bm_h-1;
			}
		}
		i = find_item_at( items, nitems, x, y );
	} while ( i < 0 );
	
	return i;
}
#endif

int get_item_height(kc_item *item)
{
	int w, h, aw;
	char btext[10];

	if (item->value==255) {
		strcpy(btext, "");
	} else {
		switch( item->type )	{
			case BT_KEY:
				strncpy( btext, key_text[item->value], 10 ); break;
			case BT_MOUSE_BUTTON:
				strncpy( btext, mousebutton_text[item->value], 10); break;
			case BT_MOUSE_AXIS:
				strncpy( btext, mouseaxis_text[item->value], 10 ); break;
			case BT_JOY_BUTTON:
				if (joybutton_text[item->value])
					strncpy(btext, joybutton_text[item->value], 10);
				else
					sprintf(btext, "BTN%2d", item->value + 1);
				break;
			case BT_JOY_AXIS:
				if (joyaxis_text[item->value])
					strncpy(btext, joyaxis_text[item->value], 10);
				else
					sprintf(btext, "AXIS%2d", item->value + 1);
				break;
			case BT_INVERT:
				strncpy( btext, invert_text[item->value], 10 ); break;
		}
	}
	gr_get_string_size(btext, &w, &h, &aw  );

	return h;
}

void kc_drawquestion( kc_menu *menu, kc_item *item );

void kconfig_draw(kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	grs_font * save_font;
	char * p;
	int i;
	int w = FSPACX(290), h = FSPACY(170);

	gr_set_current_canvas(NULL);
	nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

	gr_set_current_canvas(window_get_canvas(menu->wind));

	save_font = grd_curcanv->cv_font;
	grd_curcanv->cv_font = MEDIUM3_FONT;

	p = strchr( menu->title, '\n' );
	if ( p ) *p = 32;
	gr_string( 0x8000, FSPACY(8), menu->title );
	if ( p ) *p = '\n';

	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
	gr_string( 0x8000, FSPACY(21), "Enter changes, ctrl-d deletes, ctrl-r resets defaults, ESC exits");
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

	if ( menu->items == kc_keyboard )
	{
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );
		
		gr_rect( FSPACX( 98), FSPACY(42), FSPACX(106), FSPACY(42) ); // horiz/left
		gr_rect( FSPACX(120), FSPACY(42), FSPACX(128), FSPACY(42) ); // horiz/right
		gr_rect( FSPACX( 98), FSPACY(42), FSPACX( 98), FSPACY(44) ); // vert/left
		gr_rect( FSPACX(128), FSPACY(42), FSPACX(128), FSPACY(44) ); // vert/right
		
		gr_string( FSPACX(109), FSPACY(40), "OR" );

		gr_rect( FSPACX(253), FSPACY(42), FSPACX(261), FSPACY(42) ); // horiz/left
		gr_rect( FSPACX(275), FSPACY(42), FSPACX(283), FSPACY(42) ); // horiz/right
		gr_rect( FSPACX(253), FSPACY(42), FSPACX(253), FSPACY(44) ); // vert/left
		gr_rect( FSPACX(283), FSPACY(42), FSPACX(283), FSPACY(44) ); // vert/right

		gr_string( FSPACX(264), FSPACY(40), "OR" );
	}
	else if ( menu->items == kc_joystick )
	{
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );
		gr_string( 0x8000, FSPACY(30), TXT_BUTTONS );
		gr_string( 0x8000,FSPACY(137), TXT_AXES );
		gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
		gr_string( FSPACX( 81), FSPACY(145), TXT_AXIS );
		gr_string( FSPACX(111), FSPACY(145), TXT_INVERT );
		gr_string( FSPACX(230), FSPACY(145), TXT_AXIS );
		gr_string( FSPACX(260), FSPACY(145), TXT_INVERT );
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );

		gr_rect( FSPACX(115), FSPACY(40), FSPACX(123), FSPACY(40) ); // horiz/left
		gr_rect( FSPACX(137), FSPACY(40), FSPACX(145), FSPACY(40) ); // horiz/right
		gr_rect( FSPACX(115), FSPACY(40), FSPACX(115), FSPACY(42) ); // vert/left
		gr_rect( FSPACX(145), FSPACY(40), FSPACX(145), FSPACY(42) ); // vert/right

		gr_string( FSPACX(126), FSPACY(38), "OR" );

		gr_rect( FSPACX(261), FSPACY(40), FSPACX(269), FSPACY(40) ); // horiz/left
		gr_rect( FSPACX(283), FSPACY(40), FSPACX(291), FSPACY(40) ); // horiz/right
		gr_rect( FSPACX(261), FSPACY(40), FSPACX(261), FSPACY(42) ); // vert/left
		gr_rect( FSPACX(291), FSPACY(40), FSPACX(291), FSPACY(42) ); // vert/right

		gr_string( FSPACX(272), FSPACY(38), "OR" );
	}
	else if ( menu->items == kc_mouse )
	{
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );
		gr_string( 0x8000, FSPACY(35), TXT_BUTTONS );
		gr_string( 0x8000,FSPACY(137), TXT_AXES );
		gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
		gr_string( FSPACX( 87), FSPACY(145), TXT_AXIS );
		gr_string( FSPACX(120), FSPACY(145), TXT_INVERT );
		gr_string( FSPACX(242), FSPACY(145), TXT_AXIS );
		gr_string( FSPACX(274), FSPACY(145), TXT_INVERT );
	}
	else if ( menu->items == kc_d1x )
	{
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );

		gr_string(FSPACX(152), FSPACY(60), "KEYBOARD");
		gr_string(FSPACX(210), FSPACY(60), "JOYSTICK");
		gr_string(FSPACX(273), FSPACY(60), "MOUSE");
	}
	
	for (i=0; i<menu->nitems; i++ )	{
		kc_drawitem( &menu->items[i], 0 );
	}
	kc_drawitem( &menu->items[menu->citem], 1 );
	
	if (menu->changing)
	{
		switch( menu->items[menu->citem].type )
		{
			case BT_KEY:            gr_string( 0x8000, FSPACY(INFO_Y), TXT_PRESS_NEW_KEY ); break;
			case BT_MOUSE_BUTTON:   gr_string( 0x8000, FSPACY(INFO_Y), TXT_PRESS_NEW_MBUTTON ); break;
			case BT_MOUSE_AXIS:     gr_string( 0x8000, FSPACY(INFO_Y), TXT_MOVE_NEW_MSE_AXIS ); break;
			case BT_JOY_BUTTON:     gr_string( 0x8000, FSPACY(INFO_Y), TXT_PRESS_NEW_JBUTTON ); break;
			case BT_JOY_AXIS:       gr_string( 0x8000, FSPACY(INFO_Y), TXT_MOVE_NEW_JOY_AXIS ); break;
		}
		kc_drawquestion( menu, &menu->items[menu->citem] );
	}
	
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
	grd_curcanv->cv_font	= save_font;
	gr_set_current_canvas( save_canvas );
}

void kconfig_start_changing(kc_menu *menu)
{
	if (menu->items[menu->citem].type == BT_INVERT)
	{
		kc_change_invert(menu, &menu->items[menu->citem]);
		return;
	}
	
	menu->q_fade_i = 0;	// start question mark flasher
	menu->changing = 1;
}

int kconfig_mouse(window *wind, d_event *event, kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	int mx, my, mz, x1, x2, y1, y2;
	int i;
	int rval = 0;

	gr_set_current_canvas(window_get_canvas(wind));
	
	if (menu->mouse_state)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		for (i=0; i<menu->nitems; i++ )	{
			item_height = get_item_height( &menu->items[i] );
			x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[i].x) + FSPACX(menu->items[i].w1);
			x2 = x1 + FSPACX(menu->items[i].w2);
			y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[i].y);
			y2 = y1 + item_height;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				menu->citem = i;
				rval = 1;
				break;
			}
		}
	}
	else if (event->type == EVENT_MOUSE_BUTTON_UP)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		item_height = get_item_height( &menu->items[menu->citem] );
		x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[menu->citem].x) + FSPACX(menu->items[menu->citem].w1);
		x2 = x1 + FSPACX(menu->items[menu->citem].w2);
		y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[menu->citem].y);
		y2 = y1 + item_height;
		if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
			kconfig_start_changing(menu);
			rval = 1;
		}
		else
		{
			// Click out of changing mode - kreatordxx
			menu->changing = 0;
			rval = 1;
		}
	}
	
	gr_set_current_canvas(save_canvas);
	
	return rval;
}

int kconfig_key_command(window *wind, d_event *event, kc_menu *menu)
{
	int i,k;

	k = event_key_get(event);

	// when changing, process no keys instead of ESC
	if (menu->changing && (k != -2 && k != KEY_ESC))
		return 0;
	
	switch (k)
	{
		case KEY_CTRLED+KEY_D:
			menu->items[menu->citem].value = 255;
			return 1;
		case KEY_CTRLED+KEY_R:	
			if ( menu->items==kc_keyboard )
				for (i=0; i<NUM_KEY_CONTROLS; i++ )
					menu->items[i].value=DefaultKeySettings[0][i];

			if ( menu->items==kc_joystick )
				for (i=0; i<NUM_JOYSTICK_CONTROLS; i++ )
					menu->items[i].value = DefaultKeySettings[1][i];

			if ( menu->items==kc_mouse )
				for (i=0; i<NUM_MOUSE_CONTROLS; i++ )
					menu->items[i].value = DefaultKeySettings[2][i];

			if ( menu->items==kc_d1x )
				for(i=0;i<NUM_D1X_CONTROLS;i++)
					menu->items[i].value=DefaultKeySettingsD1X[i];
			return 1;
		case KEY_DELETE:
			menu->items[menu->citem].value=255;
			return 1;
		case KEY_UP: 		
		case KEY_PAD8:
#ifdef TABLE_CREATION
			if (menu->items[menu->citem].u==-1) menu->items[menu->citem].u=find_next_item_up( menu->items,menu->nitems, menu->citem);
#endif
			menu->citem = menu->items[menu->citem].u; 
			return 1;
		case KEY_DOWN:
		case KEY_PAD2:
#ifdef TABLE_CREATION
			if (menu->items[menu->citem].d==-1) menu->items[menu->citem].d=find_next_item_down( menu->items,menu->nitems, menu->citem);
#endif
			menu->citem = menu->items[menu->citem].d; 
			return 1;
		case KEY_LEFT:
		case KEY_PAD4:
#ifdef TABLE_CREATION
			if (menu->items[menu->citem].l==-1) menu->items[menu->citem].l=find_next_item_left( menu->items,menu->nitems, menu->citem);
#endif
			menu->citem = menu->items[menu->citem].l; 
			return 1;
		case KEY_RIGHT:
		case KEY_PAD6:
#ifdef TABLE_CREATION
			if (menu->items[menu->citem].r==-1) menu->items[menu->citem].r=find_next_item_right( menu->items,menu->nitems, menu->citem);
#endif
			menu->citem = menu->items[menu->citem].r; 
			return 1;
		case KEY_ENTER:
		case KEY_PADENTER:
			kconfig_start_changing(menu);
			return 1;
		case -2:	
		case KEY_ESC:
			if (menu->changing)
				menu->changing = 0;
			else
				window_close(wind);
			return 1;
#ifdef TABLE_CREATION
		case KEY_F12:	{
				PHYSFS_file * fp;
				for (i=0; i<NUM_KEY_CONTROLS; i++ )	{
					kc_keyboard[i].u = find_next_item_up( kc_keyboard,NUM_KEY_CONTROLS, i);
					kc_keyboard[i].d = find_next_item_down( kc_keyboard,NUM_KEY_CONTROLS, i);
					kc_keyboard[i].l = find_next_item_left( kc_keyboard,NUM_KEY_CONTROLS, i);
					kc_keyboard[i].r = find_next_item_right( kc_keyboard,NUM_KEY_CONTROLS, i);
				}
				for (i=0; i<NUM_JOYSTICK_CONTROLS; i++ )	{
					kc_joystick[i].u = find_next_item_up( kc_joystick,NUM_JOYSTICK_CONTROLS, i);
					kc_joystick[i].d = find_next_item_down( kc_joystick,NUM_JOYSTICK_CONTROLS, i);
					kc_joystick[i].l = find_next_item_left( kc_joystick,NUM_JOYSTICK_CONTROLS, i);
					kc_joystick[i].r = find_next_item_right( kc_joystick,NUM_JOYSTICK_CONTROLS, i);
				}
				for (i=0; i<NUM_MOUSE_CONTROLS; i++ )	{
					kc_mouse[i].u = find_next_item_up( kc_mouse,NUM_MOUSE_CONTROLS, i);
					kc_mouse[i].d = find_next_item_down( kc_mouse,NUM_MOUSE_CONTROLS, i);
					kc_mouse[i].l = find_next_item_left( kc_mouse,NUM_MOUSE_CONTROLS, i);
					kc_mouse[i].r = find_next_item_right( kc_mouse,NUM_MOUSE_CONTROLS, i);
				}
				for (i=0; i<NUM_D1X_CONTROLS; i++ )	{
					kc_d1x[i].u = find_next_item_up( kc_d1x,NUM_D1X_CONTROLS, i);
					kc_d1x[i].d = find_next_item_down( kc_d1x,NUM_D1X_CONTROLS, i);
					kc_d1x[i].l = find_next_item_left( kc_d1x,NUM_D1X_CONTROLS, i);
					kc_d1x[i].r = find_next_item_right( kc_d1x,NUM_D1X_CONTROLS, i);
				}
				fp = PHYSFSX_openWriteBuffered( "kconfig.cod" );
				
				PHYSFSX_printf( fp, "ubyte DefaultKeySettings[3][MAX_CONTROLS] = {\n" );
				for (i=0; i<3; i++ )	{
					int j;
					PHYSFSX_printf( fp, "{0x%2x", PlayerCfg.KeySettings[i][0] );
					for (j=1; j<MAX_CONTROLS; j++ )
						PHYSFSX_printf( fp, ",0x%2x", PlayerCfg.KeySettings[i][j] );
					PHYSFSX_printf( fp, "},\n" );
				}
				PHYSFSX_printf( fp, "};\n" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_keyboard[NUM_KEY_CONTROLS] = {\n" );
				for (i=0; i<NUM_KEY_CONTROLS; i++ )	{
					PHYSFSX_printf( fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_keyboard[i].id, kc_keyboard[i].x, kc_keyboard[i].y, kc_keyboard[i].w1, kc_keyboard[i].w2,
							kc_keyboard[i].u, kc_keyboard[i].d, kc_keyboard[i].l, kc_keyboard[i].r,
							34, kc_keyboard[i].text, 34, btype_text[kc_keyboard[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_joystick[NUM_JOYSTICK_CONTROLS] = {\n" );
				for (i=0; i<NUM_JOYSTICK_CONTROLS; i++ )	{
					PHYSFSX_printf( fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_joystick[i].id, kc_joystick[i].x, kc_joystick[i].y, kc_joystick[i].w1, kc_joystick[i].w2,
							kc_joystick[i].u, kc_joystick[i].d, kc_joystick[i].l, kc_joystick[i].r,
							34, kc_joystick[i].text, 34, btype_text[kc_joystick[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_mouse[NUM_MOUSE_CONTROLS] = {\n" );
				for (i=0; i<NUM_MOUSE_CONTROLS; i++ )	{
					PHYSFSX_printf( fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_mouse[i].id, kc_mouse[i].x, kc_mouse[i].y, kc_mouse[i].w1, kc_mouse[i].w2,
							kc_mouse[i].u, kc_mouse[i].d, kc_mouse[i].l, kc_mouse[i].r,
							34, kc_mouse[i].text, 34, btype_text[kc_mouse[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_d1x[NUM_D1X_CONTROLS] = {\n" );
				for (i=0; i<NUM_D1X_CONTROLS; i++ )	{
					PHYSFSX_printf( fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_d1x[i].id, kc_d1x[i].x, kc_d1x[i].y, kc_d1x[i].w1, kc_d1x[i].w2,
							kc_d1x[i].u, kc_d1x[i].d, kc_d1x[i].l, kc_d1x[i].r,
							34, kc_d1x[i].text, 34, btype_text[kc_d1x[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFS_close(fp);
				
			}
			return 1;
#endif
		case 0:		// some other event
			break;
			
		default:
			break;
	}
	
	return 0;
}

int kconfig_handler(window *wind, d_event *event, kc_menu *menu)
{
	int i;
	
	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_WINDOW_DEACTIVATED:
			menu->mouse_state = 0;
			break;
			
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (menu->changing && (menu->items[menu->citem].type == BT_MOUSE_BUTTON) && (event->type == EVENT_MOUSE_BUTTON_DOWN))
			{
				kc_change_mousebutton( menu, event, &menu->items[menu->citem] );
				menu->mouse_state = 1;
				return 1;
			}

			if (event_mouse_get_button(event) == MBTN_RIGHT)
			{
				if (!menu->changing)
					window_close(wind);
				return 1;
			}
			else if (event_mouse_get_button(event) != MBTN_LEFT)
				return 0;

			menu->mouse_state = (event->type == EVENT_MOUSE_BUTTON_DOWN);
			return kconfig_mouse(wind, event, menu);

		case EVENT_MOUSE_MOVED:
			if (menu->changing && menu->items[menu->citem].type == BT_MOUSE_AXIS) kc_change_mouseaxis(menu, event, &menu->items[menu->citem]);
			else
				event_mouse_get_delta( event, &menu->old_maxis[0], &menu->old_maxis[1], &menu->old_maxis[2]);
			break;

		case EVENT_JOYSTICK_BUTTON_DOWN:
			if (menu->changing && menu->items[menu->citem].type == BT_JOY_BUTTON) kc_change_joybutton(menu, event, &menu->items[menu->citem]);
			break;

		case EVENT_JOYSTICK_MOVED:
			if (menu->changing && menu->items[menu->citem].type == BT_JOY_AXIS) kc_change_joyaxis(menu, event, &menu->items[menu->citem]);
			else
			{
				int axis, value;
				event_joystick_get_axis( event, &axis, &value );
				menu->old_jaxis[axis] = value;
			}
			break;

		case EVENT_KEY_COMMAND:
		{
			int rval = kconfig_key_command(wind, event, menu);
			if (rval)
				return rval;
			if (menu->changing && menu->items[menu->citem].type == BT_KEY) kc_change_key(menu, event, &menu->items[menu->citem]);
			return 0;
		}

		case EVENT_IDLE:
			kconfig_mouse(wind, event, menu);
			break;
			
		case EVENT_WINDOW_DRAW:
			if (menu->changing)
				timer_delay(f0_1/10);
			else
				timer_delay2(50);
			kconfig_draw(menu);
			break;
			
		case EVENT_WINDOW_CLOSE:
			d_free(menu);
			
			// Update save values...
			
			for (i=0; i<NUM_KEY_CONTROLS; i++ ) 
				PlayerCfg.KeySettings[0][i] = kc_keyboard[i].value;
			
			for (i=0; i<NUM_JOYSTICK_CONTROLS; i++ ) 
				PlayerCfg.KeySettings[1][i] = kc_joystick[i].value;

			for (i=0; i<NUM_MOUSE_CONTROLS; i++ ) 
				PlayerCfg.KeySettings[2][i] = kc_mouse[i].value;
			
			for (i=0; i<NUM_D1X_CONTROLS; i++)
				PlayerCfg.KeySettingsD1X[i] = kc_d1x[i].value;
			return 0;	// continue closing
			break;
			
		default:
			return 0;
			break;
	}
	
	return 1;
}

void kconfig_sub(kc_item * items,int nitems, char *title)
{
	kc_menu *menu;

	MALLOC(menu, kc_menu, 1);
	
	if (!menu)
		return;

	memset(menu, 0, sizeof(kc_menu));
	menu->items = items;
	menu->nitems = nitems;
	menu->title = title;
	menu->citem = 0;
	menu->changing = 0;
	menu->mouse_state = 0;

	if (!(menu->wind = window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200),
					   (int (*)(window *, d_event *, void *))kconfig_handler, menu)))
		d_free(menu);
}


void kc_drawitem( kc_item *item, int is_current )
{
	int x, w, h, aw;
	char btext[10];

	if (is_current)
		gr_set_fontcolor( BM_XRGB(20,20,29), -1 );
	else
		gr_set_fontcolor( BM_XRGB(15,15,24), -1 );

	gr_string( FSPACX(item->x), FSPACY(item->y), item->text );

	if (item->value==255) {
		strcpy( btext, "" );
	} else {
		switch( item->type )	{
			case BT_KEY:
				strncpy( btext, key_text[item->value], 10 ); break;
			case BT_MOUSE_BUTTON:
				strncpy( btext, mousebutton_text[item->value], 10 ); break;
			case BT_MOUSE_AXIS:
				strncpy( btext, mouseaxis_text[item->value], 10 ); break;
			case BT_JOY_BUTTON:
				if (joybutton_text[item->value])
					strncpy(btext, joybutton_text[item->value], 10);
				else
					sprintf(btext, "BTN%2d", item->value + 1);
				break;
			case BT_JOY_AXIS:
				if (joyaxis_text[item->value])
					strncpy(btext, joyaxis_text[item->value], 10);
				else
					sprintf(btext, "AXIS%2d", item->value + 1);
				break;
			case BT_INVERT:
				strncpy( btext, invert_text[item->value], 10 ); break;
		}
	}
	gr_get_string_size(btext, &w, &h, &aw  );

	if (is_current)
		gr_setcolor( BM_XRGB(21,0,24) );
	else
		gr_setcolor( BM_XRGB(16,0,19) );
	gr_urect( FSPACX(item->w1+item->x), FSPACY(item->y-1), FSPACX(item->w1+item->x+item->w2), FSPACY(item->y)+h );
	
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

	x = FSPACX(item->w1+item->x)+((FSPACX(item->w2)-w)/2);

	gr_string( x, FSPACY(item->y), btext );
}


void kc_drawquestion( kc_menu *menu, kc_item *item )
{
	int c, x, w, h, aw;

	gr_get_string_size("?", &w, &h, &aw  );

	c = BM_XRGB(21,0,24);

	gr_setcolor( gr_fade_table[fades[menu->q_fade_i]*256+c] );
	menu->q_fade_i++;
	if (menu->q_fade_i>63) menu->q_fade_i=0;

	gr_urect( FSPACX(item->w1+item->x), FSPACY(item->y-1), FSPACX(item->w1+item->x+item->w2), FSPACY(item->y)+h );
	
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

	x = FSPACX(item->w1+item->x)+((FSPACX(item->w2)-w)/2);

	gr_string( x, FSPACY(item->y), "?" );
}

void kc_change_key( kc_menu *menu, d_event *event, kc_item * item )
{
	int i,n;
	ubyte keycode = 255;

	Assert(event->type == EVENT_KEY_COMMAND);
	keycode = event_key_get_raw(event);

	if (strlen(key_text[keycode])<=0)
		return;

	for (n=0; n<(GameArg.CtlNoStickyKeys?sizeof(system_keys)-3:sizeof(system_keys)); n++ )
		if ( system_keys[n] == keycode )
			return;

	for (i=0; i<menu->nitems; i++ )
	{
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_KEY) && (menu->items[i].value==keycode) )
		{
			menu->items[i].value = 255;
		}
	}
	item->value = keycode;
	menu->changing = 0;
}

void kc_change_joybutton( kc_menu *menu, d_event *event, kc_item * item )
{
	int n,i,button = 255;

	Assert(event->type == EVENT_JOYSTICK_BUTTON_DOWN);
	button = event_joystick_get_button(event);

	for (i=0; i<menu->nitems; i++ )
	{
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_JOY_BUTTON) && (menu->items[i].value==button) )
			menu->items[i].value = 255;
	}
	item->value = button;
	menu->changing = 0;
}

void kc_change_mousebutton( kc_menu *menu, d_event *event, kc_item * item )
{
	int n,i,button;

	Assert(event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP);
	button = event_mouse_get_button(event);

	for (i=0; i<menu->nitems; i++)
	{
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_MOUSE_BUTTON) && (menu->items[i].value==button) )
			menu->items[i].value = 255;
	}
	item->value = button;
	menu->changing = 0;
}

void kc_change_joyaxis( kc_menu *menu, d_event *event, kc_item * item )
{
	int i, n, axis, value;

	Assert(event->type == EVENT_JOYSTICK_MOVED);
	event_joystick_get_axis( event, &axis, &value );

	if ( abs(value-menu->old_jaxis[axis])<32 )
		return;
	con_printf(CON_DEBUG, "Axis Movement detected: Axis %i\n", axis);

	for (i=0; i<menu->nitems; i++ )
	{
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_JOY_AXIS) && (menu->items[i].value==axis) )
			menu->items[i].value = 255;
	}
	item->value = axis;
	menu->changing = 0;
}

void kc_change_mouseaxis( kc_menu *menu, d_event *event, kc_item * item )
{
	int i, n, dx, dy, dz;
	ubyte code = 255;

	Assert(event->type == EVENT_MOUSE_MOVED);
	event_mouse_get_delta( event, &dx, &dy, &dz );
	if ( abs(dx)>5 ) code = 0;
	if ( abs(dy)>5 ) code = 1;
	if ( abs(dz)>5 ) code = 2;

	if (code!=255)
	{
		for (i=0; i<menu->nitems; i++ )
		{
			n = item - menu->items;
			if ( (i!=n) && (menu->items[i].type==BT_MOUSE_AXIS) && (menu->items[i].value==code) )
				menu->items[i].value = 255;
		}
		item->value = code;
		menu->changing = 0;
	}
}

void kc_change_invert( kc_menu *menu, kc_item * item )
{
	if (item->value)
		item->value = 0;
	else 
		item->value = 1;

	menu->changing = 0;		// in case we were changing something else
}

#include "screens.h"

void kconfig(int n, char * title)
{
	set_screen_mode( SCREEN_MENU );
	kc_set_controls();

	switch(n)
    	{
		case 0:kconfig_sub( kc_keyboard,NUM_KEY_CONTROLS,  title); break;
		case 1:kconfig_sub( kc_joystick,NUM_JOYSTICK_CONTROLS,title); break;
		case 2:kconfig_sub( kc_mouse,   NUM_MOUSE_CONTROLS,    title); break;
		case 3:kconfig_sub( kc_d1x, NUM_D1X_CONTROLS, title ); break;
		default:
			Int3();
			return;
	}
}

void kconfig_read_controls(d_event *event, int automap_flag)
{
	int i = 0, j = 0, speed_factor = cheats.turbo?2:1;
	static fix64 mouse_delta_time = 0;

#ifndef NDEBUG
	// --- Don't do anything if in debug mode ---
	if ( keyd_pressed[KEY_DELETE] )
	{
		memset( &Controls, 0, sizeof(control_info) );
		return;
	}
#endif

	Controls.pitch_time = Controls.vertical_thrust_time = Controls.heading_time = Controls.sideways_thrust_time = Controls.bank_time = Controls.forward_thrust_time = 0;

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
			for (i = 0; i < NUM_KEY_CONTROLS; i++)
			{
				if (kc_keyboard[i].value < 255 && kc_keyboard[i].value == event_key_get_raw(event))
				{
					if (kc_keyboard[i].ci_state_ptr != NULL)
					{
						if (event->type==EVENT_KEY_COMMAND)
							*kc_keyboard[i].ci_state_ptr |= kc_keyboard[i].state_bit;
						else
							*kc_keyboard[i].ci_state_ptr &= ~kc_keyboard[i].state_bit;
					}
					if (kc_keyboard[i].ci_count_ptr != NULL && event->type==EVENT_KEY_COMMAND)
						*kc_keyboard[i].ci_count_ptr += 1;
				}
			}
			if (!automap_flag && event->type == EVENT_KEY_COMMAND)
				for (i = 0, j = 0; i < 28; i += 3, j++)
					if (kc_d1x[i].value < 255 && kc_d1x[i].value == event_key_get_raw(event))
					{
						Controls.select_weapon_count = j+1;
						break;
					}
			break;
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			for (i = 0; i < NUM_JOYSTICK_CONTROLS; i++)
			{
				if (kc_joystick[i].value < 255 && kc_joystick[i].type == BT_JOY_BUTTON && kc_joystick[i].value == event_joystick_get_button(event))
				{
					if (kc_joystick[i].ci_state_ptr != NULL)
					{
						if (event->type==EVENT_JOYSTICK_BUTTON_DOWN)
							*kc_joystick[i].ci_state_ptr |= kc_joystick[i].state_bit;
						else
							*kc_joystick[i].ci_state_ptr &= ~kc_joystick[i].state_bit;
					}
					if (kc_joystick[i].ci_count_ptr != NULL && event->type==EVENT_JOYSTICK_BUTTON_DOWN)
						*kc_joystick[i].ci_count_ptr += 1;
				}
			}
			if (!automap_flag && event->type == EVENT_JOYSTICK_BUTTON_DOWN)
				for (i = 1, j = 0; i < 29; i += 3, j++)
					if (kc_d1x[i].value < 255 && kc_d1x[i].value == event_joystick_get_button(event))
					{
						Controls.select_weapon_count = j+1;
						break;
					}
			break;
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			for (i = 0; i < NUM_MOUSE_CONTROLS; i++)
			{
				if (kc_mouse[i].value < 255 && kc_mouse[i].type == BT_MOUSE_BUTTON && kc_mouse[i].value == event_mouse_get_button(event))
				{
					if (kc_mouse[i].ci_state_ptr != NULL)
					{
						if (event->type==EVENT_MOUSE_BUTTON_DOWN)
							*kc_mouse[i].ci_state_ptr |= kc_mouse[i].state_bit;
						else
							*kc_mouse[i].ci_state_ptr &= ~kc_mouse[i].state_bit;
					}
					if (kc_mouse[i].ci_count_ptr != NULL && event->type==EVENT_MOUSE_BUTTON_DOWN)
						*kc_mouse[i].ci_count_ptr += 1;
				}
			}
			if (!automap_flag && event->type == EVENT_MOUSE_BUTTON_DOWN)
				for (i = 2, j = 0; i < 30; i += 3, j++)
					if (kc_d1x[i].value < 255 && kc_d1x[i].value == event_mouse_get_button(event))
					{
						Controls.select_weapon_count = j+1;
						break;
					}
			break;
		case EVENT_JOYSTICK_MOVED:
		{
			int axis = 0, value = 0, joy_null_value = 0;
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			event_joystick_get_axis(event, &axis, &value);

			Controls.raw_joy_axis[axis] = value;

			if (axis == kc_joystick[13].value) // Pitch U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[1]*8;
			if (axis == kc_joystick[15].value) // Turn L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[0]*8;
			if (axis == kc_joystick[17].value) // Slide L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[2]*8;
			if (axis == kc_joystick[19].value) // Slide U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[3]*8;
			if (axis == kc_joystick[21].value) // Bank Deadzone
				joy_null_value = PlayerCfg.JoystickDead[4]*8;
			if (axis == kc_joystick[23].value) // Throttle - default deadzone
				joy_null_value = PlayerCfg.JoystickDead[5]*3;

			if (Controls.raw_joy_axis[axis] > joy_null_value) 
				Controls.raw_joy_axis[axis] = ((Controls.raw_joy_axis[axis]-joy_null_value)*128)/(128-joy_null_value);
			else if (Controls.raw_joy_axis[axis] < -joy_null_value)
				Controls.raw_joy_axis[axis] = ((Controls.raw_joy_axis[axis]+joy_null_value)*128)/(128-joy_null_value);
			else
				Controls.raw_joy_axis[axis] = 0;
			Controls.joy_axis[axis] = (Controls.raw_joy_axis[axis]*FrameTime)/128;
			break;
		}
		case EVENT_MOUSE_MOVED:
		{
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			if (PlayerCfg.MouseFlightSim)
			{
				int ax[3];
				event_mouse_get_delta( event, &ax[0], &ax[1], &ax[2] );
				for (i = 0; i <= 2; i++)
				{
					int mouse_null_value = (i==2?16:PlayerCfg.MouseFSDead*8);
					Controls.raw_mouse_axis[i] += ax[i];
					if (Controls.raw_mouse_axis[i] < -MOUSEFS_DELTA_RANGE)
						Controls.raw_mouse_axis[i] = -MOUSEFS_DELTA_RANGE;
					if (Controls.raw_mouse_axis[i] > MOUSEFS_DELTA_RANGE)
						Controls.raw_mouse_axis[i] = MOUSEFS_DELTA_RANGE;
					if (Controls.raw_mouse_axis[i] > mouse_null_value) 
						Controls.mouse_axis[i] = (((Controls.raw_mouse_axis[i]-mouse_null_value)*MOUSEFS_DELTA_RANGE)/(MOUSEFS_DELTA_RANGE-mouse_null_value)*FrameTime)/MOUSEFS_DELTA_RANGE;
					else if (Controls.raw_mouse_axis[i] < -mouse_null_value)
						Controls.mouse_axis[i] = (((Controls.raw_mouse_axis[i]+mouse_null_value)*MOUSEFS_DELTA_RANGE)/(MOUSEFS_DELTA_RANGE-mouse_null_value)*FrameTime)/MOUSEFS_DELTA_RANGE;
					else
						Controls.mouse_axis[i] = 0;
				}
			}
			else
			{
				event_mouse_get_delta( event, &Controls.raw_mouse_axis[0], &Controls.raw_mouse_axis[1], &Controls.raw_mouse_axis[2] );
				Controls.mouse_axis[0] = (Controls.raw_mouse_axis[0]*FrameTime)/8;
				Controls.mouse_axis[1] = (Controls.raw_mouse_axis[1]*FrameTime)/8;
				Controls.mouse_axis[2] = (Controls.raw_mouse_axis[2]*FrameTime);
				mouse_delta_time = timer_query() + (F1_0/30);
			}
			break;
		}
		case EVENT_IDLE:
		default:
			if (!PlayerCfg.MouseFlightSim && mouse_delta_time < timer_query())
			{
				Controls.mouse_axis[0] = Controls.mouse_axis[1] = Controls.mouse_axis[2] = 0;
				mouse_delta_time = timer_query() + (F1_0/30);
			}
			break;
	}

	//------------ Read pitch_time -----------
	if ( !Controls.slide_on_state )
	{
		int kp = 0;
		// From keyboard/buttons...
		if ( Controls.pitch_forward_state ) kp += speed_factor*FrameTime/2;
		if ( Controls.pitch_backward_state ) kp -= speed_factor*FrameTime/2;
		if (kp == 0)
			Controls.pitch_time = 0;
		else if (kp > 0) {
			if (Controls.pitch_time < 0)
				Controls.pitch_time = 0;
		} else // kp < 0
			if (Controls.pitch_time > 0)
				Controls.pitch_time = 0;
		Controls.pitch_time += kp;
		// From joystick...
		if ( !kc_joystick[14].value ) // If not inverted...
			Controls.pitch_time -= (Controls.joy_axis[kc_joystick[13].value]*PlayerCfg.JoystickSens[1])/8;
		else
			Controls.pitch_time += (Controls.joy_axis[kc_joystick[13].value]*PlayerCfg.JoystickSens[1])/8;
		// From mouse...
		if ( !kc_mouse[14].value ) // If not inverted...
			Controls.pitch_time -= (Controls.mouse_axis[kc_mouse[13].value]*PlayerCfg.MouseSens[1])/8;
		else
			Controls.pitch_time += (Controls.mouse_axis[kc_mouse[13].value]*PlayerCfg.MouseSens[1])/8;
	}
	else Controls.pitch_time = 0;

	//----------- Read vertical_thrust_time -----------------
	if ( Controls.slide_on_state )
	{
		// From keyboard/buttons...
		if ( Controls.pitch_forward_state ) Controls.vertical_thrust_time += speed_factor*FrameTime;
		if ( Controls.pitch_backward_state ) Controls.vertical_thrust_time -= speed_factor*FrameTime;
		// From joystick...
		if ( !kc_joystick[20].value /*!kc_joystick[14].value*/ )		// If not inverted... NOTE: Use Slide U/D invert setting
			Controls.vertical_thrust_time += (Controls.joy_axis[kc_joystick[13].value]*PlayerCfg.JoystickSens[3])/8;
		else
			Controls.vertical_thrust_time -= (Controls.joy_axis[kc_joystick[13].value]*PlayerCfg.JoystickSens[3])/8;
		// From mouse...
		if ( !kc_mouse[20].value /*!kc_mouse[14].value*/ )		// If not inverted... NOTE: Use Slide U/D invert setting
			Controls.vertical_thrust_time -= (Controls.mouse_axis[kc_mouse[13].value]*PlayerCfg.MouseSens[3])/8;
		else
			Controls.vertical_thrust_time += (Controls.mouse_axis[kc_mouse[13].value]*PlayerCfg.MouseSens[3])/8;
	}
	// From keyboard/buttons...
	if ( Controls.slide_up_state ) Controls.vertical_thrust_time += speed_factor*FrameTime;
	if ( Controls.slide_down_state ) Controls.vertical_thrust_time -= speed_factor*FrameTime;
	// From joystick...
	if ( !kc_joystick[20].value )		// If not inverted...
		Controls.vertical_thrust_time += (Controls.joy_axis[kc_joystick[19].value]*PlayerCfg.JoystickSens[3])/8;
	else
		Controls.vertical_thrust_time -= (Controls.joy_axis[kc_joystick[19].value]*PlayerCfg.JoystickSens[3])/8;
	// From mouse...
	if ( !kc_mouse[20].value )		// If not inverted...
		Controls.vertical_thrust_time += (Controls.mouse_axis[kc_mouse[19].value]*PlayerCfg.MouseSens[3])/8;
	else
		Controls.vertical_thrust_time -= (Controls.mouse_axis[kc_mouse[19].value]*PlayerCfg.MouseSens[3])/8;

	//---------- Read heading_time -----------
	if (!Controls.slide_on_state && !Controls.bank_on_state)
	{
		int kh = 0;
		// From keyboard/buttons...
		if ( Controls.heading_right_state ) kh += speed_factor*FrameTime;
		if ( Controls.heading_left_state ) kh -= speed_factor*FrameTime;
		if (kh == 0)
			Controls.heading_time = 0;
		else if (kh > 0) {
			if (Controls.heading_time < 0)
				Controls.heading_time = 0;
		} else // kh < 0
			if (Controls.heading_time > 0)
				Controls.heading_time = 0;
		Controls.heading_time += kh;
		// From joystick...
		if ( !kc_joystick[16].value )		// If not inverted...
			Controls.heading_time += (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[0])/8;
		else
			Controls.heading_time -= (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[0])/8;
		// From mouse...
		if ( !kc_mouse[16].value )		// If not inverted...
			Controls.heading_time += (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[0])/8;
		else
			Controls.heading_time -= (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[0])/8;
	}
	else Controls.heading_time = 0;

	//----------- Read sideways_thrust_time -----------------
	if ( Controls.slide_on_state )
	{
		// From keyboard/buttons...
		if ( Controls.heading_right_state ) Controls.sideways_thrust_time += speed_factor*FrameTime;
		if ( Controls.heading_left_state ) Controls.sideways_thrust_time -= speed_factor*FrameTime;
		// From joystick...
		if ( !kc_joystick[18].value /*!kc_joystick[16].value*/ )		// If not inverted... NOTE: Use Slide L/R invert setting
			Controls.sideways_thrust_time += (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[2])/8;
		else
			Controls.sideways_thrust_time -= (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[2])/8;
		// From mouse...
		if ( !kc_mouse[18].value /*!kc_mouse[16].value*/ )		// If not inverted... NOTE: Use Slide L/R invert setting
			Controls.sideways_thrust_time += (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[2])/8;
		else
			Controls.sideways_thrust_time -= (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[2])/8;
	}
	// From keyboard/buttons...
	if ( Controls.slide_left_state ) Controls.sideways_thrust_time -= speed_factor*FrameTime;
	if ( Controls.slide_right_state ) Controls.sideways_thrust_time += speed_factor*FrameTime;
	// From joystick...
	if ( !kc_joystick[18].value )		// If not inverted...
		Controls.sideways_thrust_time += (Controls.joy_axis[kc_joystick[17].value]*PlayerCfg.JoystickSens[2])/8;
	else
		Controls.sideways_thrust_time -= (Controls.joy_axis[kc_joystick[17].value]*PlayerCfg.JoystickSens[2])/8;
	// From mouse...
	if ( !kc_mouse[18].value )		// If not inverted...
		Controls.sideways_thrust_time += (Controls.mouse_axis[kc_mouse[17].value]*PlayerCfg.MouseSens[2])/8;
	else
		Controls.sideways_thrust_time -= (Controls.mouse_axis[kc_mouse[17].value]*PlayerCfg.MouseSens[2])/8;

	//----------- Read bank_time -----------------
	if ( Controls.bank_on_state )
	{
		// From keyboard/buttons...
		if ( Controls.heading_left_state ) Controls.bank_time += speed_factor*FrameTime;
		if ( Controls.heading_right_state ) Controls.bank_time -= speed_factor*FrameTime;
		// From joystick...
		if ( !kc_joystick[22].value /*!kc_joystick[16].value*/ )		// If not inverted... NOTE: Use Bank L/R invert setting
			Controls.bank_time -= (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[4])/8;
		else
			Controls.bank_time += (Controls.joy_axis[kc_joystick[15].value]*PlayerCfg.JoystickSens[4])/8;
		// From mouse...
		if ( !kc_mouse[22].value /*!kc_mouse[16].value*/ )		// If not inverted... NOTE: Use Bank L/R invert setting
			Controls.bank_time += (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[4])/8;
		else
			Controls.bank_time -= (Controls.mouse_axis[kc_mouse[15].value]*PlayerCfg.MouseSens[4])/8;
	}

	// From keyboard/buttons...
	if ( Controls.bank_left_state ) Controls.bank_time += speed_factor*FrameTime;
	if ( Controls.bank_right_state ) Controls.bank_time -= speed_factor*FrameTime;
	// From joystick...
	if ( !kc_joystick[22].value )		// If not inverted...
		Controls.bank_time -= (Controls.joy_axis[kc_joystick[21].value]*PlayerCfg.JoystickSens[4])/8;
	else
		Controls.bank_time += (Controls.joy_axis[kc_joystick[21].value]*PlayerCfg.JoystickSens[4])/8;
	// From mouse...
	if ( !kc_mouse[22].value )		// If not inverted...
		Controls.bank_time += (Controls.mouse_axis[kc_mouse[21].value]*PlayerCfg.MouseSens[4])/8;
	else
		Controls.bank_time -= (Controls.mouse_axis[kc_mouse[21].value]*PlayerCfg.MouseSens[4])/8;

	//----------- Read forward_thrust_time -------------
	// From keyboard/buttons...
	if ( Controls.accelerate_state ) Controls.forward_thrust_time += speed_factor*FrameTime;
	if ( Controls.reverse_state ) Controls.forward_thrust_time -= speed_factor*FrameTime;
	// From joystick...
	if ( !kc_joystick[24].value )		// If not inverted...
		Controls.forward_thrust_time -= (Controls.joy_axis[kc_joystick[23].value]*PlayerCfg.JoystickSens[5])/8;
	else
		Controls.forward_thrust_time += (Controls.joy_axis[kc_joystick[23].value]*PlayerCfg.JoystickSens[5])/8;
	// From mouse...
	if ( !kc_mouse[24].value )		// If not inverted...
		Controls.forward_thrust_time -= (Controls.mouse_axis[kc_mouse[23].value]*PlayerCfg.MouseSens[5])/8;
	else
		Controls.forward_thrust_time += (Controls.mouse_axis[kc_mouse[23].value]*PlayerCfg.MouseSens[5])/8;

	//----------- Read cruise-control-type of throttle.
	if ( Controls.cruise_plus_state ) Cruise_speed += speed_factor*FrameTime*80;
	if ( Controls.cruise_minus_state ) Cruise_speed -= speed_factor*FrameTime*80;
	if ( Controls.cruise_off_count > 0 ) Controls.cruise_off_count = Cruise_speed = 0;
	if (Cruise_speed > i2f(100) ) Cruise_speed = i2f(100);
	if (Cruise_speed < 0 ) Cruise_speed = 0;
	if (Controls.forward_thrust_time==0) Controls.forward_thrust_time = fixmul(Cruise_speed,FrameTime)/100;

	//----------- Clamp values between -FrameTime and FrameTime
	if (Controls.pitch_time > FrameTime/2 ) Controls.pitch_time = FrameTime/2;
	if (Controls.heading_time > FrameTime ) Controls.heading_time = FrameTime;
	if (Controls.pitch_time < -FrameTime/2 ) Controls.pitch_time = -FrameTime/2;
	if (Controls.heading_time < -FrameTime ) Controls.heading_time = -FrameTime;
	if (Controls.vertical_thrust_time > FrameTime ) Controls.vertical_thrust_time = FrameTime;
	if (Controls.sideways_thrust_time > FrameTime ) Controls.sideways_thrust_time = FrameTime;
	if (Controls.bank_time > FrameTime ) Controls.bank_time = FrameTime;
	if (Controls.forward_thrust_time > FrameTime ) Controls.forward_thrust_time = FrameTime;
	if (Controls.vertical_thrust_time < -FrameTime ) Controls.vertical_thrust_time = -FrameTime;
	if (Controls.sideways_thrust_time < -FrameTime ) Controls.sideways_thrust_time = -FrameTime;
	if (Controls.bank_time < -FrameTime ) Controls.bank_time = -FrameTime;
	if (Controls.forward_thrust_time < -FrameTime ) Controls.forward_thrust_time = -FrameTime;
}

void reset_cruise(void)
{
	Cruise_speed=0;
}


void kc_set_controls()
{
	int i;

	for (i=0; i<NUM_KEY_CONTROLS; i++ )
		kc_keyboard[i].value = PlayerCfg.KeySettings[0][i];

	for (i=0; i<NUM_JOYSTICK_CONTROLS; i++ )
	{
		kc_joystick[i].value = PlayerCfg.KeySettings[1][i];
		if (kc_joystick[i].type == BT_INVERT )
		{
			if (kc_joystick[i].value!=1)
				kc_joystick[i].value = 0;
			PlayerCfg.KeySettings[1][i] = kc_joystick[i].value;
		}
	}

	for (i=0; i<NUM_MOUSE_CONTROLS; i++ )
	{
		kc_mouse[i].value = PlayerCfg.KeySettings[2][i];
		if (kc_mouse[i].type == BT_INVERT )
		{
			if (kc_mouse[i].value!=1)
				kc_mouse[i].value = 0;
			PlayerCfg.KeySettings[2][i] = kc_mouse[i].value;
		}
	}

	for (i=0; i<NUM_D1X_CONTROLS; i++ )
		kc_d1x[i].value = PlayerCfg.KeySettingsD1X[i];
}
