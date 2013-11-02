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
 * Routines to configure keyboard, joystick, etc..
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <cstddef>

#include "dxxerror.h"
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
static const sbyte fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

static const char invert_text[2][2] = { "N", "Y" };
char *joybutton_text[JOY_MAX_BUTTONS];
char *joyaxis_text[JOY_MAX_AXES];
static const char mouseaxis_text[][8] = { "L/R", "F/B", "WHEEL" };
static const char mousebutton_text[][8] = { "LEFT", "RIGHT", "MID", "M4", "M5", "M6", "M7", "M8", "M9", "M10","M11","M12","M13","M14","M15","M16" };

static const ubyte system_keys[19] = { KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN, KEY_CAPSLOCK, KEY_SCROLLOCK, KEY_NUMLOCK }; // KEY_*LOCK should always be last since we wanna skip these if -nostickykeys

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

#define INFO_Y (188)

typedef struct kc_item {
	const short x, y;              // x, y pos of label
	const short xinput;                // x pos of input field
	const short w2;                // length of input field
#ifndef TABLE_CREATION
	const
#endif
	short u,d,l,r;           // neighboring field ids for cursor navigation
        //short text_num1;
        const char *const text;
	const ubyte type;
	ubyte value;		// what key,button,etc
	ubyte *const ci_state_ptr;
	const int state_bit;
	ubyte *const ci_count_ptr;
} kc_item;

typedef struct kc_menu
{
	window	*wind;
	kc_item	*items;
	const char	*title;
	unsigned	nitems;
	unsigned	citem;
	int	old_jaxis[JOY_MAX_AXES];
	int	old_maxis[3];
	ubyte	changing;
	ubyte	q_fade_i;	// for flashing the question mark
	ubyte	mouse_state;
} kc_menu;

const ubyte DefaultKeySettings[3][MAX_CONTROLS] = {
#if defined(DXX_BUILD_DESCENT_I)
{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x33,0x0,0x34,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
#elif defined(DXX_BUILD_DESCENT_II)
{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x1f,0xff,0x33,0xff,0x34,0xff,0x23,0xff,0x14,0xff,0xff,0xff,0x0,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0},
#endif
};
const ubyte DefaultKeySettingsRebirth[MAX_DXX_REBIRTH_CONTROLS] = { 0x2,0xff,0xff,0x3,0xff,0xff,0x4,0xff,0xff,0x5,0xff,0xff,0x6,0xff,0xff,0x7,0xff,0xff,0x8,0xff,0xff,0x9,0xff,0xff,0xa,0xff,0xff,0xb,0xff,0xff };

//	  id,  x,  y, w1, w2,  u,  d,   l, r,     text,   type, value
kc_item kc_keyboard[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ 15, 49, 86, 26, 43,  2, 49,  1,"Pitch forward", BT_KEY, 255, &Controls.key_pitch_forward_state, STATE_BIT1, NULL },
	{ 15, 49,115, 26, 48,  3,  0, 24,"Pitch forward", BT_KEY, 255, &Controls.key_pitch_forward_state, STATE_BIT2, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 15, 49, 86, 26, 55,  2, 56,  1,"Pitch forward", BT_KEY, 255, &Controls.key_pitch_forward_state, STATE_BIT1, NULL },
	{ 15, 49,115, 26, 50,  3,  0, 24,"Pitch forward", BT_KEY, 255, &Controls.key_pitch_forward_state, STATE_BIT2, NULL },
#endif
	{ 15, 57, 86, 26,  0,  4, 25,  3,"Pitch backward", BT_KEY, 255, &Controls.key_pitch_backward_state, STATE_BIT1, NULL },
	{ 15, 57,115, 26,  1,  5,  2, 26,"Pitch backward", BT_KEY, 255, &Controls.key_pitch_backward_state, STATE_BIT2, NULL },
	{ 15, 65, 86, 26,  2,  6, 27,  5,"Turn left", BT_KEY, 255, &Controls.key_heading_left_state, STATE_BIT1, NULL },
	{ 15, 65,115, 26,  3,  7,  4, 28,"Turn left", BT_KEY, 255, &Controls.key_heading_left_state, STATE_BIT2, NULL },
	{ 15, 73, 86, 26,  4,  8, 29,  7,"Turn right", BT_KEY, 255, &Controls.key_heading_right_state, STATE_BIT1, NULL },
	{ 15, 73,115, 26,  5,  9,  6, 34,"Turn right", BT_KEY, 255, &Controls.key_heading_right_state, STATE_BIT2, NULL },
	{ 15, 85, 86, 26,  6, 10, 35,  9,"Slide on", BT_KEY, 255, &Controls.slide_on_state, STATE_BIT1, NULL },
	{ 15, 85,115, 26,  7, 11,  8, 36,"Slide on", BT_KEY, 255, &Controls.slide_on_state, STATE_BIT2, NULL },
	{ 15, 93, 86, 26,  8, 12, 37, 11,"Slide left", BT_KEY, 255, &Controls.key_slide_left_state, STATE_BIT1, NULL },
	{ 15, 93,115, 26,  9, 13, 10, 44,"Slide left", BT_KEY, 255, &Controls.key_slide_left_state, STATE_BIT2, NULL },
	{ 15,101, 86, 26, 10, 14, 45, 13,"Slide right", BT_KEY, 255, &Controls.key_slide_right_state, STATE_BIT1, NULL },
	{ 15,101,115, 26, 11, 15, 12, 30,"Slide right", BT_KEY, 255, &Controls.key_slide_right_state, STATE_BIT2, NULL },
	{ 15,109, 86, 26, 12, 16, 31, 15,"Slide up", BT_KEY, 255, &Controls.key_slide_up_state, STATE_BIT1, NULL },
	{ 15,109,115, 26, 13, 17, 14, 32,"Slide up", BT_KEY, 255, &Controls.key_slide_up_state, STATE_BIT2, NULL },
	{ 15,117, 86, 26, 14, 18, 33, 17,"Slide down", BT_KEY, 255, &Controls.key_slide_down_state, STATE_BIT1, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{ 15,117,115, 26, 15, 19, 16, 38,"Slide down", BT_KEY, 255, &Controls.key_slide_down_state, STATE_BIT2, NULL },
	{ 15,129, 86, 26, 16, 20, 39, 19,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT1, NULL },
	{ 15,129,115, 26, 17, 21, 18, 40,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT2, NULL },
	{ 15,137, 86, 26, 18, 22, 41, 21,"Bank left", BT_KEY, 255, &Controls.key_bank_left_state, STATE_BIT1, NULL },
	{ 15,137,115, 26, 19, 23, 20, 42,"Bank left", BT_KEY, 255, &Controls.key_bank_left_state, STATE_BIT2, NULL },
	{ 15,145, 86, 26, 20, 46, 43, 23,"Bank right", BT_KEY, 255, &Controls.key_bank_right_state, STATE_BIT1, NULL },
	{ 15,145,115, 26, 21, 47, 22, 46,"Bank right", BT_KEY, 255, &Controls.key_bank_right_state, STATE_BIT2, NULL },
	{158, 49, 241, 26, 49, 26,  1, 25,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT1, NULL },
	{158, 49,270, 26, 42, 27, 24,  2,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT2, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 15,117,115, 26, 15, 19, 16, 46,"Slide down", BT_KEY, 255, &Controls.key_slide_down_state, STATE_BIT2, NULL },
	{ 15,129, 86, 26, 16, 20, 47, 19,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT1, NULL },
	{ 15,129,115, 26, 17, 21, 18, 38,"Bank on", BT_KEY, 255, &Controls.bank_on_state, STATE_BIT2, NULL },
	{ 15,137, 86, 26, 18, 22, 39, 21,"Bank left", BT_KEY, 255, &Controls.key_bank_left_state, STATE_BIT1, NULL },
	{ 15,137,115, 26, 19, 23, 20, 40,"Bank left", BT_KEY, 255, &Controls.key_bank_left_state, STATE_BIT2, NULL },
	{ 15,145, 86, 26, 20, 48, 41, 23,"Bank right", BT_KEY, 255, &Controls.key_bank_right_state, STATE_BIT1, NULL },
	{ 15,145,115, 26, 21, 49, 22, 42,"Bank right", BT_KEY, 255, &Controls.key_bank_right_state, STATE_BIT2, NULL },
	{158, 49, 241, 26, 51, 26,  1, 25,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT1, NULL },
	{158, 49,270, 26, 56, 27, 24,  2,"Fire primary", BT_KEY, 255, &Controls.fire_primary_state, STATE_BIT2, NULL },
#endif
	{158, 57, 241, 26, 24, 28,  3, 27,"Fire secondary", BT_KEY, 255, &Controls.fire_secondary_state, STATE_BIT1, NULL },
	{158, 57,270, 26, 25, 29, 26,  4,"Fire secondary", BT_KEY, 255, &Controls.fire_secondary_state, STATE_BIT2, NULL },
	{158, 65, 241, 26, 26, 34,  5, 29,"Fire flare", BT_KEY, 255, NULL, 0, &Controls.fire_flare_count },
	{158, 65,270, 26, 27, 35, 28,  6,"Fire flare", BT_KEY, 255, NULL, 0, &Controls.fire_flare_count },
	{158,105, 241, 26, 44, 32, 13, 31,"Accelerate", BT_KEY, 255, &Controls.accelerate_state, STATE_BIT1, NULL },
	{158,105,270, 26, 45, 33, 30, 14,"Accelerate", BT_KEY, 255, &Controls.accelerate_state, STATE_BIT2, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{158,113, 241, 26, 30, 38, 15, 33,"Reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT1, NULL },
	{158,113,270, 26, 31, 39, 32, 16,"Reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT2, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,113, 241, 26, 30, 46, 15, 33,"reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT1, NULL },
	{158,113,270, 26, 31, 47, 32, 16,"reverse", BT_KEY, 255, &Controls.reverse_state, STATE_BIT2, NULL },
#endif
	{158, 73, 241, 26, 28, 36,  7, 35,"Drop Bomb", BT_KEY, 255, NULL, 0, &Controls.drop_bomb_count },
	{158, 73,270, 26, 29, 37, 34,  8,"Drop Bomb", BT_KEY, 255, NULL, 0, &Controls.drop_bomb_count },
	{158, 85, 241, 26, 34, 44,  9, 37,"REAR VIEW", BT_KEY, 255, &Controls.rear_view_state, STATE_BIT1, NULL },
	{158, 85,270, 26, 35, 45, 36, 10,"REAR VIEW", BT_KEY, 255, &Controls.rear_view_state, STATE_BIT2, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{158,125, 241, 26, 32, 40, 17, 39,"Cruise faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT1, NULL },
	{158,125,270, 26, 33, 41, 38, 18,"Cruise faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT2, NULL },
	{158,133, 241, 26, 38, 42, 19, 41,"Cruise slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT1, NULL },
	{158,133,270, 26, 39, 43, 40, 20,"Cruise slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT2, NULL },
	{158,141, 241, 26, 40, 25, 21, 43,"Cruise off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
	{158,141,270, 26, 41,  0, 42, 22,"Cruise off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,133, 241, 26, 46, 40, 19, 39,"Cruise Faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT1, NULL },
	{158,133,270, 26, 47, 41, 38, 20,"Cruise Faster", BT_KEY, 255, &Controls.cruise_plus_state, STATE_BIT2, NULL },
	{158,141, 241, 26, 38, 42, 21, 41,"Cruise Slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT1, NULL },
	{158,141,270, 26, 39, 43, 40, 22,"Cruise Slower", BT_KEY, 255, &Controls.cruise_minus_state, STATE_BIT2, NULL },
	{158,149, 241, 26, 40, 52, 23, 43,"Cruise Off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
	{158,149,270, 26, 41, 53, 42, 48,"Cruise Off", BT_KEY, 255, NULL, 0, &Controls.cruise_off_count },
#endif
	{158, 93, 241, 26, 36, 30, 11, 45,"Automap", BT_KEY, 255, &Controls.automap_state, STATE_BIT1, NULL },
	{158, 93,270, 26, 37, 31, 44, 12,"Automap", BT_KEY, 255, &Controls.automap_state, STATE_BIT2, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{ 15,157, 86, 26, 22, 48, 23, 47,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 15,157,115, 26, 23, 49, 46, 48,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 15,165, 86, 26, 46,  1, 47, 49,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 15,165,115, 26, 47, 24, 48,  0,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,121, 241, 26, 32, 38, 17, 47,"Afterburner", BT_KEY, 255, &Controls.afterburner_state, STATE_BIT1, NULL },
	{158,121,270, 26, 33, 39, 46, 18,"Afterburner", BT_KEY, 255, &Controls.afterburner_state, STATE_BIT2, NULL },
	{ 15,161, 86, 26, 22, 50, 43, 49,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 15,161,115, 26, 23, 51, 48, 52,"Cycle Primary", BT_KEY, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 15,169, 86, 26, 48,  1, 53, 51,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 15,169,115, 26, 49, 24, 50, 54,"Cycle Second.", BT_KEY, 255, NULL, 0, &Controls.cycle_secondary_count },
	{158,163, 241, 26, 42, 54, 49, 53,"Headlight", BT_KEY, 255, NULL, 0, &Controls.headlight_count },
	{158,163,270, 26, 43, 55, 52, 50,"Headlight", BT_KEY, 255, NULL, 0, &Controls.headlight_count },
	{158,171, 241, 26, 52, 56, 51, 55,"Energy->Shield", BT_KEY, 255, &Controls.energy_to_shield_state, STATE_BIT1, NULL },
	{158,171,270, 26, 53,  0, 54, 56,"Energy->Shield", BT_KEY, 255, &Controls.energy_to_shield_state, STATE_BIT2, NULL },
	{158,179, 241, 26, 54, 25, 55,  0,"Toggle Bomb", BT_KEY, 255, NULL, 0, &Controls.toggle_bomb_count },
#endif
};
kc_item kc_joystick[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ 22, 46, 104, 26, 15,  1, 24, 29,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT3, NULL },
	{ 22, 54, 104, 26,  0,  4, 34, 30,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT3, NULL },
	{ 22, 78, 104, 26, 26,  3, 37, 31,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT3, NULL },
	{ 22, 86, 104, 26,  2, 25, 38, 32,"Reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT3, NULL },
	{ 22, 62, 104, 26,  1, 26, 35, 33,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{174, 46, 248, 26, 23,  6, 29, 34,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT3, NULL },
	{174, 54, 248, 26,  5,  7, 30, 35,"Slide left", BT_JOY_BUTTON, 255, &Controls.btn_slide_left_state, STATE_BIT3, NULL },
	{174, 62, 248, 26,  6,  8, 33, 36,"Slide right", BT_JOY_BUTTON, 255, &Controls.btn_slide_right_state, STATE_BIT3, NULL },
	{174, 70, 248, 26,  7,  9, 43, 37,"Slide up", BT_JOY_BUTTON, 255, &Controls.btn_slide_up_state, STATE_BIT3, NULL },
	{174, 78, 248, 26,  8, 10, 31, 38,"Slide down", BT_JOY_BUTTON, 255, &Controls.btn_slide_down_state, STATE_BIT3, NULL },
	{174, 86, 248, 26,  9, 11, 32, 39,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT3, NULL },
	{174, 94, 248, 26, 10, 12, 42, 40,"Bank left", BT_JOY_BUTTON, 255, &Controls.btn_bank_left_state, STATE_BIT3, NULL },
	{174,102, 248, 26, 11, 44, 28, 41,"Bank right", BT_JOY_BUTTON, 255, &Controls.btn_bank_right_state, STATE_BIT3, NULL },
	{ 22,154, 73, 26, 47, 15, 47, 14,"Pitch U/D", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 22,154, 121,  8, 27, 16, 13, 17,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22, 46, 102, 26, 15,  1, 24, 31,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT3, NULL },
	{ 22, 54, 102, 26,  0,  4, 36, 32,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT3, NULL },
	{ 22, 78, 102, 26, 26,  3, 39, 33,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT3, NULL },
	{ 22, 86, 102, 26,  2, 25, 40, 34,"reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT3, NULL },
	{ 22, 62, 102, 26,  1, 26, 37, 35,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{174, 46, 248, 26, 23,  6, 31, 36,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT3, NULL },
	{174, 54, 248, 26,  5,  7, 32, 37,"Slide left", BT_JOY_BUTTON, 255, &Controls.btn_slide_left_state, STATE_BIT3, NULL },
	{174, 62, 248, 26,  6,  8, 35, 38,"Slide right", BT_JOY_BUTTON, 255, &Controls.btn_slide_right_state, STATE_BIT3, NULL },
	{174, 70, 248, 26,  7,  9, 45, 39,"Slide up", BT_JOY_BUTTON, 255, &Controls.btn_slide_up_state, STATE_BIT3, NULL },
	{174, 78, 248, 26,  8, 10, 33, 40,"Slide down", BT_JOY_BUTTON, 255, &Controls.btn_slide_down_state, STATE_BIT3, NULL },
	{174, 86, 248, 26,  9, 11, 34, 41,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT3, NULL },
	{174, 94, 248, 26, 10, 12, 44, 42,"Bank left", BT_JOY_BUTTON, 255, &Controls.btn_bank_left_state, STATE_BIT3, NULL },
	{174,102, 248, 26, 11, 28, 46, 43,"Bank right", BT_JOY_BUTTON, 255, &Controls.btn_bank_right_state, STATE_BIT3, NULL },
	{ 22,154, 73, 26, 55, 15, 55, 14,"Pitch U/D", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{ 22,154, 121,  8, 50, 16, 13, 17,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
#endif
	{ 22,162, 73, 26, 13,  0, 18, 16,"Turn L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{ 22,162, 121,  8, 14, 29, 15, 19,"Turn L/R", BT_INVERT, 255, NULL, 0, NULL },
	{164,154, 222, 26, 28, 19, 14, 18,"Slide L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{164,154,270,  8, 45, 20, 17, 15,"Slide L/R", BT_INVERT, 255, NULL, 0, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22,162, 121,  8, 14, 31, 15, 19,"Turn L/R", BT_INVERT, 255, NULL, 0, NULL },
	{164,154, 222, 26, 51, 19, 14, 18,"Slide L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{164,154,270,  8, 54, 20, 17, 15,"Slide L/R", BT_INVERT, 255, NULL, 0, NULL },
#endif
	{164,162, 222, 26, 17, 21, 16, 20,"Slide U/D", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{164,162,270,  8, 18, 22, 19, 21,"Slide U/D", BT_INVERT, 255, NULL, 0, NULL },
	{164,170, 222, 26, 19, 23, 20, 22,"Bank L/R", BT_JOY_AXIS, 255, NULL, 0, NULL },
	{164,170,270,  8, 20, 24, 21, 23,"Bank L/R", BT_INVERT, 255, NULL, 0, NULL },
	{164,178, 222, 26, 21,  5, 22, 24,"throttle", BT_JOY_AXIS, 255, NULL, 0, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{164,178,270,  8, 22, 34, 23,  0,"Throttle", BT_INVERT, 255, NULL, 0, NULL },
	{ 22, 94, 104, 26,  3, 27, 39, 42,"Rear view", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT3, NULL },
	{ 22, 70, 104, 26,  4,  2, 36, 43,"Drop bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 22,102, 104, 26, 25, 14, 40, 28,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT3, NULL },
	{ 22,102,133, 26, 42, 17, 27, 12,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT4, NULL },
	{ 22, 46,133, 26, 16, 30,  0,  5,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT4, NULL },
	{ 22, 54,133, 26, 29, 33,  1,  6,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT4, NULL },
	{ 22, 78,133, 26, 43, 32,  2,  9,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT4, NULL },
	{ 22, 86,133, 26, 31, 42,  3, 10,"Reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT4, NULL },
	{ 22, 62,133, 26, 30, 43,  4,  7,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{174, 46,278, 26, 24, 35,  5,  1,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT4, NULL },
	{174, 54,278, 26, 34, 36,  6,  4,"Slide left", BT_JOY_BUTTON, 255, &Controls.btn_slide_left_state, STATE_BIT4, NULL },
	{174, 62,278, 26, 35, 37,  7, 26,"Slide right", BT_JOY_BUTTON, 255, &Controls.btn_slide_right_state, STATE_BIT4, NULL },
	{174, 70,278, 26, 36, 38,  8,  2,"Slide up", BT_JOY_BUTTON, 255, &Controls.btn_slide_up_state, STATE_BIT4, NULL },
	{174, 78,278, 26, 37, 39,  9,  3,"Slide down", BT_JOY_BUTTON, 255, &Controls.btn_slide_down_state, STATE_BIT4, NULL },
	{174, 86,278, 26, 38, 40, 10, 25,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT4, NULL },
	{174, 94,278, 26, 39, 41, 11, 27,"Bank left", BT_JOY_BUTTON, 255, &Controls.btn_bank_left_state, STATE_BIT4, NULL },
	{174,102,278, 26, 40, 46, 12, 44,"Bank right", BT_JOY_BUTTON, 255, &Controls.btn_bank_right_state, STATE_BIT4, NULL },
	{ 22, 94,133, 26, 32, 28, 25, 11,"Rear view", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT4, NULL },
	{ 22, 70,133, 26, 33, 31, 26,  8,"Drop bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{174,110, 248, 26, 12, 45, 41, 46,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{174,118, 248, 26, 44, 18, 46, 47,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
	{174,110,278, 26, 41, 47, 44, 45,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{174,118,278, 26, 46, 13, 45, 13,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
#elif defined(DXX_BUILD_DESCENT_II)
	{164,178,270,  8, 22, 36, 23,  0,"throttle", BT_INVERT, 255, NULL, 0, NULL },
	{ 22, 94, 102, 26,  3, 27, 41, 44,"REAR VIEW", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT3, NULL },
	{ 22, 70, 102, 26,  4,  2, 38, 45,"Drop Bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 22,102, 102, 26, 25, 30, 42, 46,"Afterburner", BT_JOY_BUTTON, 255, &Controls.afterburner_state, STATE_BIT3, NULL },
	{174,110, 248, 26, 12, 29, 49, 47,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{174,118, 248, 26, 28, 54, 53, 48,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 22,110, 102, 26, 27, 52, 43, 49,"Headlight", BT_JOY_BUTTON, 255, NULL, 0, &Controls.headlight_count },
	{ 22, 46,132, 26, 16, 32,  0,  5,"Fire primary", BT_JOY_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT4, NULL },
	{ 22, 54,132, 26, 31, 35,  1,  6,"Fire secondary", BT_JOY_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT4, NULL },
	{ 22, 78,132, 26, 45, 34,  2,  9,"Accelerate", BT_JOY_BUTTON, 255, &Controls.accelerate_state, STATE_BIT4, NULL },
	{ 22, 86,132, 26, 33, 44,  3, 10,"reverse", BT_JOY_BUTTON, 255, &Controls.reverse_state, STATE_BIT4, NULL },
	{ 22, 62,132, 26, 32, 45,  4,  7,"Fire flare", BT_JOY_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{174, 46,278, 26, 24, 37,  5,  1,"Slide on", BT_JOY_BUTTON, 255, &Controls.slide_on_state, STATE_BIT4, NULL },
	{174, 54,278, 26, 36, 38,  6,  4,"Slide left", BT_JOY_BUTTON, 255, &Controls.btn_slide_left_state, STATE_BIT4, NULL },
	{174, 62,278, 26, 37, 39,  7, 26,"Slide right", BT_JOY_BUTTON, 255, &Controls.btn_slide_right_state, STATE_BIT4, NULL },
	{174, 70,278, 26, 38, 40,  8,  2,"Slide up", BT_JOY_BUTTON, 255, &Controls.btn_slide_up_state, STATE_BIT4, NULL },
	{174, 78,278, 26, 39, 41,  9,  3,"Slide down", BT_JOY_BUTTON, 255, &Controls.btn_slide_down_state, STATE_BIT4, NULL },
	{174, 86,278, 26, 40, 42, 10, 25,"Bank on", BT_JOY_BUTTON, 255, &Controls.bank_on_state, STATE_BIT4, NULL },
	{174, 94,278, 26, 41, 43, 11, 27,"Bank left", BT_JOY_BUTTON, 255, &Controls.btn_bank_left_state, STATE_BIT4, NULL },
	{174,102,278, 26, 42, 47, 12, 30,"Bank right", BT_JOY_BUTTON, 255, &Controls.btn_bank_right_state, STATE_BIT4, NULL },
	{ 22, 94,132, 26, 34, 46, 25, 11,"REAR VIEW", BT_JOY_BUTTON, 255, &Controls.rear_view_state, STATE_BIT4, NULL },
	{ 22, 70,132, 26, 35, 33, 26,  8,"Drop Bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
	{ 22,102,132, 26, 44, 49, 27, 12,"Afterburner", BT_JOY_BUTTON, 255, &Controls.afterburner_state, STATE_BIT4, NULL },
	{174,110,278, 26, 43, 48, 28, 52,"Cycle Primary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{174,118,278, 26, 47, 55, 29, 50,"Cycle Secondary", BT_JOY_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
	{ 22,110,132, 26, 46, 53, 30, 28,"Headlight", BT_JOY_BUTTON, 255, NULL, 0, &Controls.headlight_count },
	{ 22,126, 102, 26, 52, 14, 48, 51,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT3, NULL },
	{ 22,126,132, 26, 53, 17, 50, 54,"Automap", BT_JOY_BUTTON, 255, &Controls.automap_state, STATE_BIT4, NULL },
	{ 22,118, 102, 26, 30, 50, 47, 53,"Energy->Shield", BT_JOY_BUTTON, 255, &Controls.energy_to_shield_state, STATE_BIT3, NULL },
	{ 22,118,132, 26, 49, 51, 52, 29,"Energy->Shield", BT_JOY_BUTTON, 255, &Controls.energy_to_shield_state, STATE_BIT4, NULL },
	{174,126, 248, 26, 29, 18, 51, 55,"Toggle Bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.toggle_bomb_count },
	{174,126,278, 26, 48, 13, 54, 13,"Toggle Bomb", BT_JOY_BUTTON, 255, NULL, 0, &Controls.toggle_bomb_count },
#endif
};
kc_item kc_mouse[] = {
	{ 25, 46, 110, 26, 19,  1, 20,  5,"Fire primary", BT_MOUSE_BUTTON, 255, &Controls.fire_primary_state, STATE_BIT5, NULL },
	{ 25, 54, 110, 26,  0,  4,  5,  6,"Fire secondary", BT_MOUSE_BUTTON, 255, &Controls.fire_secondary_state, STATE_BIT5, NULL },
	{ 25, 78, 110, 26, 26,  3,  8,  9,"Accelerate", BT_MOUSE_BUTTON, 255, &Controls.accelerate_state, STATE_BIT5, NULL },
	{ 25, 86, 110, 26,  2, 25,  9, 10,"reverse", BT_MOUSE_BUTTON, 255, &Controls.reverse_state, STATE_BIT5, NULL },
	{ 25, 62, 110, 26,  1, 26,  6,  7,"Fire flare", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.fire_flare_count },
	{180, 46, 239, 26, 23,  6,  0,  1,"Slide on", BT_MOUSE_BUTTON, 255, &Controls.slide_on_state, STATE_BIT5, NULL },
	{180, 54, 239, 26,  5,  7,  1,  4,"Slide left", BT_MOUSE_BUTTON, 255, &Controls.btn_slide_left_state, STATE_BIT5, NULL },
	{180, 62, 239, 26,  6,  8,  4, 26,"Slide right", BT_MOUSE_BUTTON, 255, &Controls.btn_slide_right_state, STATE_BIT5, NULL },
	{180, 70, 239, 26,  7,  9, 26,  2,"Slide up", BT_MOUSE_BUTTON, 255, &Controls.btn_slide_up_state, STATE_BIT5, NULL },
	{180, 78, 239, 26,  8, 10,  2,  3,"Slide down", BT_MOUSE_BUTTON, 255, &Controls.btn_slide_down_state, STATE_BIT5, NULL },
	{180, 86, 239, 26,  9, 11,  3, 25,"Bank on", BT_MOUSE_BUTTON, 255, &Controls.bank_on_state, STATE_BIT5, NULL },
	{180, 94, 239, 26, 10, 12, 25, 27,"Bank left", BT_MOUSE_BUTTON, 255, &Controls.btn_bank_left_state, STATE_BIT5, NULL },
	{180,102, 239, 26, 11, 22, 27, 28,"Bank right", BT_MOUSE_BUTTON, 255, &Controls.btn_bank_right_state, STATE_BIT5, NULL },
#if defined(DXX_BUILD_DESCENT_I)
	{ 25,154, 83, 26, 24, 15, 28, 14,"Pitch U/D", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 25,154,131,  8, 28, 16, 13, 21,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 25,154, 83, 26, 24, 15, 29, 14,"Pitch U/D", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 25,154,131,  8, 29, 16, 13, 21,"Pitch U/D", BT_INVERT, 255, NULL, 0, NULL },
#endif
	{ 25,162, 83, 26, 13, 17, 22, 16,"Turn L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 25,162,131,  8, 14, 18, 15, 23,"Turn L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 25,170, 83, 26, 15, 19, 24, 18,"Slide L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 25,170,131,  8, 16, 20, 17, 19,"Slide L/R", BT_INVERT, 255, NULL, 0, NULL },
	{ 25,178, 83, 26, 17,  0, 18, 20,"Slide U/D", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{ 25,178,131,  8, 18, 21, 19,  0,"Slide U/D", BT_INVERT, 255, NULL, 0, NULL },
	{180,154, 238, 26, 20, 23, 14, 22,"Bank L/R", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{180,154,286,  8, 12, 24, 21, 15,"Bank L/R", BT_INVERT, 255, NULL, 0, NULL },
	{180,162, 238, 26, 21,  5, 16, 24,"Throttle", BT_MOUSE_AXIS, 255, NULL, 0, NULL },
	{180,162,286,  8, 22, 13, 23, 17,"Throttle", BT_INVERT, 255, NULL, 0, NULL },
	{ 25, 94, 110, 26,  3, 27, 10, 11,"REAR VIEW", BT_MOUSE_BUTTON, 255, &Controls.rear_view_state, STATE_BIT5, NULL },
	{ 25, 70, 110, 26,  4,  2,  7,  8,"Drop Bomb", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.drop_bomb_count },
#if defined(DXX_BUILD_DESCENT_I)
	{ 25,102, 110, 26, 25, 28, 11, 12,"Cycle Primary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 25,110, 110, 26, 27, 14, 12, 13,"Cycle Secondary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 25,102, 110, 26, 25, 28, 11, 12,"Afterburner", BT_MOUSE_BUTTON, 255, &Controls.afterburner_state, STATE_BIT5, NULL },
	{ 25,110, 110, 26, 27, 29, 12, 29,"Cycle Primary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_primary_count },
	{ 25,118, 110, 26, 28, 14, 28, 13,"Cycle Secondary", BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.cycle_secondary_count },
#endif
};

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_EXTENDED_WEAPON_STRING(X)
#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_EXTENDED_WEAPON_STRING(X)	X
#endif

#define WEAPON_STRING_LASER	D2X_EXTENDED_WEAPON_STRING("(SUPER)") "LASER CANNON"
#define WEAPON_STRING_VULCAN	"VULCAN" D2X_EXTENDED_WEAPON_STRING("/GAUSS") " CANNON"
#define WEAPON_STRING_SPREADFIRE	"SPREADFIRE" D2X_EXTENDED_WEAPON_STRING("/HELIX") " CANNON"
#define WEAPON_STRING_PLASMA	"PLASMA" D2X_EXTENDED_WEAPON_STRING("/PHOENIX") " CANNON"
#define WEAPON_STRING_FUSION	"FUSION" D2X_EXTENDED_WEAPON_STRING("/OMEGA") " CANNON"
#define WEAPON_STRING_CONCUSSION	"CONCUSSION" D2X_EXTENDED_WEAPON_STRING("/FLASH") " MISSILE"
#define WEAPON_STRING_HOMING	"HOMING" D2X_EXTENDED_WEAPON_STRING("/GUIDED") " MISSILE"
#define WEAPON_STRING_PROXIMITY	"PROXIMITY BOMB" D2X_EXTENDED_WEAPON_STRING("/SMART MINE")
#define WEAPON_STRING_SMART	"SMART" D2X_EXTENDED_WEAPON_STRING("/MERCURY") " MISSILE"
#define WEAPON_STRING_MEGA	"MEGA" D2X_EXTENDED_WEAPON_STRING("/EARTHSHAKER") " MISSILE"

kc_item kc_rebirth[] = {
	{ 15, 69,157, 26, 29,  3, 29,  1,WEAPON_STRING_LASER, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 69,215, 26, 27,  4,  0,  2,WEAPON_STRING_LASER, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 69,273, 26, 28,  5,  1,  3,WEAPON_STRING_LASER, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 77,157, 26,  0,  6,  2,  4,WEAPON_STRING_VULCAN, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 77,215, 26,  1,  7,  3,  5,WEAPON_STRING_VULCAN, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 77,273, 26,  2,  8,  4,  6,WEAPON_STRING_VULCAN, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 85,157, 26,  3,  9,  5,  7,WEAPON_STRING_SPREADFIRE, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 85,215, 26,  4, 10,  6,  8,WEAPON_STRING_SPREADFIRE, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 85,273, 26,  5, 11,  7,  9,WEAPON_STRING_SPREADFIRE, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 93,157, 26,  6, 12,  8, 10,WEAPON_STRING_PLASMA, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 93,215, 26,  7, 13,  9, 11,WEAPON_STRING_PLASMA, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15, 93,273, 26,  8, 14, 10, 12,WEAPON_STRING_PLASMA, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,101,157, 26,  9, 15, 11, 13,WEAPON_STRING_FUSION, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,101,215, 26, 10, 16, 12, 14,WEAPON_STRING_FUSION, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,101,273, 26, 11, 17, 13, 15,WEAPON_STRING_FUSION, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,109,157, 26, 12, 18, 14, 16,WEAPON_STRING_CONCUSSION, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,109,215, 26, 13, 19, 15, 17,WEAPON_STRING_CONCUSSION, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,109,273, 26, 14, 20, 16, 18,WEAPON_STRING_CONCUSSION, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,117,157, 26, 15, 21, 17, 19,WEAPON_STRING_HOMING, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,117,215, 26, 16, 22, 18, 20,WEAPON_STRING_HOMING, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,117,273, 26, 17, 23, 19, 21,WEAPON_STRING_HOMING, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,125,157, 26, 18, 24, 20, 22,WEAPON_STRING_PROXIMITY, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,125,215, 26, 19, 25, 21, 23,WEAPON_STRING_PROXIMITY, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,125,273, 26, 20, 26, 22, 24,WEAPON_STRING_PROXIMITY, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,133,157, 26, 21, 27, 23, 25,WEAPON_STRING_SMART, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,133,215, 26, 22, 28, 24, 26,WEAPON_STRING_SMART, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,133,273, 26, 23, 29, 25, 27,WEAPON_STRING_SMART, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,141,157, 26, 24,  1, 26, 28,WEAPON_STRING_MEGA, BT_KEY, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,141,215, 26, 25,  2, 27, 29,WEAPON_STRING_MEGA, BT_JOY_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
	{ 15,141,273, 26, 26,  0, 28,  0,WEAPON_STRING_MEGA, BT_MOUSE_BUTTON, 255, NULL, 0, &Controls.select_weapon_count },
};

static void kc_drawitem( kc_item *item, int is_current );
static void kc_change_key( kc_menu *menu, d_event *event, kc_item * item );
static void kc_change_joybutton( kc_menu *menu, d_event *event, kc_item * item );
static void kc_change_mousebutton( kc_menu *menu, d_event *event, kc_item * item );
static void kc_change_joyaxis( kc_menu *menu, d_event *event, kc_item * item );
static void kc_change_mouseaxis( kc_menu *menu, d_event *event, kc_item * item );
static void kc_change_invert( kc_menu *menu, kc_item * item );
static void kc_drawquestion( kc_menu *menu, kc_item *item );

#ifdef TABLE_CREATION
static int find_item_at( const kc_item * items, unsigned nitems, int x, int y )
{
	for (unsigned i=0; i<nitems; i++ )	{
		if ( ((items[i].xinput)==x) && (items[i].y==y))
			return i;
	}
	return -1;
}

static int find_next_item_up( const kc_item * items, unsigned nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].xinput;
	
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

static int find_next_item_down( const kc_item * items, unsigned nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].xinput;
	
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

static int find_next_item_right( const kc_item * items, unsigned nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].xinput;
	
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

static int find_next_item_left( const kc_item * items, unsigned nitems, int citem )
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].xinput;
	
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

template <unsigned nitems>
static int find_next_item_up( const kc_item (&items)[nitems], int citem )
{
	return find_next_item_up(items, nitems, citem);
}

template <unsigned nitems>
static int find_next_item_down( const kc_item (&items)[nitems], int citem )
{
	return find_next_item_down(items, nitems, citem);
}

template <unsigned nitems>
static int find_next_item_right( const kc_item (&items)[nitems], int citem )
{
	return find_next_item_right(items, nitems, citem);
}

template <unsigned nitems>
static int find_next_item_left( const kc_item (&items)[nitems], int citem )
{
	return find_next_item_left(items, nitems, citem);
}
#endif

static const char *get_item_text(const kc_item *item, char (&buf)[10])
{
	if (item->value==255) {
		return "";
	} else {
		switch( item->type )	{
			case BT_KEY:
				return key_properties[item->value].key_text;
			case BT_MOUSE_BUTTON:
				return mousebutton_text[item->value];
			case BT_MOUSE_AXIS:
				return mouseaxis_text[item->value];
			case BT_JOY_BUTTON:
				if (joybutton_text[item->value])
					return joybutton_text[item->value];
				else
				{
					snprintf(buf, sizeof(buf), "BTN%2d", item->value + 1);
					return buf;
				}
				break;
			case BT_JOY_AXIS:
				if (joyaxis_text[item->value])
					return joyaxis_text[item->value];
				else
				{
					snprintf(buf, sizeof(buf), "AXIS%2d", item->value + 1);
					return buf;
				}
				break;
			case BT_INVERT:
				return invert_text[item->value];
			default:
				return NULL;
		}
	}
}

static int get_item_height(kc_item *item)
{
	int w, h, aw;
	char buf[10];
	const char *btext;

	btext = get_item_text(item, buf);
	if (!btext)
		return 0;
	gr_get_string_size(btext, &w, &h, &aw  );

	return h;
}

static void kconfig_draw(kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	grs_font * save_font;
	int w = FSPACX(290), h = FSPACY(170);

	gr_set_current_canvas(NULL);
	nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

	gr_set_current_canvas(window_get_canvas(menu->wind));

	save_font = grd_curcanv->cv_font;
	grd_curcanv->cv_font = MEDIUM3_FONT;

	Assert(!strchr( menu->title, '\n' ));
	gr_string( 0x8000, FSPACY(8), menu->title );

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
	else if ( menu->items == kc_rebirth )
	{
		gr_set_fontcolor( BM_XRGB(31,27,6), -1 );
		gr_setcolor( BM_XRGB(31,27,6) );

		gr_string(FSPACX(152), FSPACY(60), "KEYBOARD");
		gr_string(FSPACX(210), FSPACY(60), "JOYSTICK");
		gr_string(FSPACX(273), FSPACY(60), "MOUSE");
	}
	
	for (unsigned i=0; i<menu->nitems; i++ )	{
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

static void kconfig_start_changing(kc_menu *menu)
{
	if (menu->items[menu->citem].type == BT_INVERT)
	{
		kc_change_invert(menu, &menu->items[menu->citem]);
		return;
	}

	menu->q_fade_i = 0;	// start question mark flasher
	menu->changing = 1;
}

static inline int in_bounds(unsigned mx, unsigned my, unsigned x1, unsigned xw, unsigned y1, unsigned yh)
{
	if (mx <= x1)
		return 0;
	if (my <= y1)
		return 0;
	if (mx >= x1 + xw)
		return 0;
	if (my >= y1 + yh)
		return 0;
	return 1;
}

static int kconfig_mouse(window *wind, d_event *event, kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	int mx, my, mz, x1, y1;
	int rval = 0;

	gr_set_current_canvas(window_get_canvas(wind));
	
	if (menu->mouse_state)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		for (unsigned i=0; i<menu->nitems; i++ )	{
			item_height = get_item_height( &menu->items[i] );
			x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[i].xinput);
			y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[i].y);
			if (in_bounds(mx, my, x1, FSPACX(menu->items[i].w2), y1, item_height)) {
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
		x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[menu->citem].xinput);
		y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[menu->citem].y);
		if (in_bounds(mx, my, x1, FSPACX(menu->items[menu->citem].w2), y1, item_height)) {
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

static int kconfig_key_command(window *wind, d_event *event, kc_menu *menu)
{
	int k;

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
				for (unsigned i=0; i< (sizeof(kc_keyboard) / sizeof(kc_keyboard[0])); i++ )
					menu->items[i].value=DefaultKeySettings[0][i];

			if ( menu->items==kc_joystick )
				for (unsigned i=0; i<(sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++ )
					menu->items[i].value = DefaultKeySettings[1][i];

			if ( menu->items==kc_mouse )
				for (unsigned i=0; i<(sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++ )
					menu->items[i].value = DefaultKeySettings[2][i];
			if ( menu->items==kc_rebirth )
				for(unsigned i=0;i<sizeof(kc_rebirth) / sizeof(kc_rebirth[0]);i++)
					menu->items[i].value=DefaultKeySettingsRebirth[i];
			return 1;
		case KEY_DELETE:
			menu->items[menu->citem].value=255;
			return 1;
		case KEY_UP: 		
		case KEY_PAD8:
			menu->citem = menu->items[menu->citem].u; 
			return 1;
		case KEY_DOWN:
		case KEY_PAD2:
			menu->citem = menu->items[menu->citem].d; 
			return 1;
		case KEY_LEFT:
		case KEY_PAD4:
			menu->citem = menu->items[menu->citem].l; 
			return 1;
		case KEY_RIGHT:
		case KEY_PAD6:
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
			static const char *const btype_text[] = { "BT_KEY", "BT_MOUSE_BUTTON", "BT_MOUSE_AXIS", "BT_JOY_BUTTON", "BT_JOY_AXIS", "BT_INVERT" };
				PHYSFS_file * fp;
				for (unsigned i=0; i< (sizeof(kc_keyboard) / sizeof(kc_keyboard[0])); i++ )	{
					kc_keyboard[i].u = find_next_item_up( kc_keyboard, i);
					kc_keyboard[i].d = find_next_item_down( kc_keyboard, i);
					kc_keyboard[i].l = find_next_item_left( kc_keyboard, i);
					kc_keyboard[i].r = find_next_item_right( kc_keyboard, i);
				}
				for (unsigned i=0; i<(sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++ )	{
					kc_joystick[i].u = find_next_item_up( kc_joystick, i);
					kc_joystick[i].d = find_next_item_down( kc_joystick, i);
					kc_joystick[i].l = find_next_item_left( kc_joystick, i);
					kc_joystick[i].r = find_next_item_right( kc_joystick, i);
				}
				for (unsigned i=0; i<(sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++ )	{
					kc_mouse[i].u = find_next_item_up( kc_mouse, i);
					kc_mouse[i].d = find_next_item_down( kc_mouse, i);
					kc_mouse[i].l = find_next_item_left( kc_mouse, i);
					kc_mouse[i].r = find_next_item_right( kc_mouse, i);
				}
				for (unsigned i=0; i<sizeof(kc_rebirth) / sizeof(kc_rebirth[0]); i++ )	{
					kc_rebirth[i].u = find_next_item_up( kc_rebirth, i);
					kc_rebirth[i].d = find_next_item_down( kc_rebirth, i);
					kc_rebirth[i].l = find_next_item_left( kc_rebirth, i);
					kc_rebirth[i].r = find_next_item_right( kc_rebirth, i);
				}
				fp = PHYSFSX_openWriteBuffered( "kconfig.cod" );
				
				PHYSFSX_printf( fp, "const ubyte DefaultKeySettings[3][MAX_CONTROLS] = {\n" );
				for (unsigned i=0; i<3; i++ )	{
					int j;
					PHYSFSX_printf( fp, "{0x%2x", PlayerCfg.KeySettings[i][0] );
					for (j=1; j<MAX_CONTROLS; j++ )
						PHYSFSX_printf( fp, ",0x%2x", PlayerCfg.KeySettings[i][j] );
					PHYSFSX_printf( fp, "},\n" );
				}
				PHYSFSX_printf( fp, "};\n" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_keyboard[] = {\n" );
				for (unsigned i=0; i<(sizeof(kc_keyboard)/sizeof(kc_keyboard[0])); i++ )	{
					PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_keyboard[i].x, kc_keyboard[i].y, kc_keyboard[i].xinput, kc_keyboard[i].w2,
							kc_keyboard[i].u, kc_keyboard[i].d, kc_keyboard[i].l, kc_keyboard[i].r,
							34, kc_keyboard[i].text, 34, btype_text[kc_keyboard[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_joystick[] = {\n" );
				for (unsigned i=0; i<(sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++ )	{
#if defined(DXX_BUILD_DESCENT_II)
					if (kc_joystick[i].type == BT_JOY_BUTTON)
						PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
								kc_joystick[i].x, kc_joystick[i].y, kc_joystick[i].xinput, kc_joystick[i].w2,
								kc_joystick[i].u, kc_joystick[i].d, kc_joystick[i].l, kc_joystick[i].r,
								34, kc_joystick[i].text, 34, btype_text[kc_joystick[i].type] );
					else
#endif
						PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
								kc_joystick[i].x, kc_joystick[i].y, kc_joystick[i].xinput, kc_joystick[i].w2,
								kc_joystick[i].u, kc_joystick[i].d, kc_joystick[i].l, kc_joystick[i].r,
								34, kc_joystick[i].text, 34, btype_text[kc_joystick[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_mouse[] = {\n" );
				for (unsigned i=0; i<(sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++ )	{
					PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_mouse[i].x, kc_mouse[i].y, kc_mouse[i].xinput, kc_mouse[i].w2,
							kc_mouse[i].u, kc_mouse[i].d, kc_mouse[i].l, kc_mouse[i].r,
							34, kc_mouse[i].text, 34, btype_text[kc_mouse[i].type] );
				}
				PHYSFSX_printf( fp, "};" );
				
				PHYSFSX_printf( fp, "\nkc_item kc_rebirth[] = {\n" );
				for (unsigned i=0; i<(sizeof(kc_rebirth) / sizeof(kc_rebirth[0])); i++ )	{
					PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n", 
							kc_rebirth[i].x, kc_rebirth[i].y, kc_rebirth[i].xinput, kc_rebirth[i].w2,
							kc_rebirth[i].u, kc_rebirth[i].d, kc_rebirth[i].l, kc_rebirth[i].r,
							34, kc_rebirth[i].text, 34, btype_text[kc_rebirth[i].type] );
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

static int kconfig_handler(window *wind, d_event *event, kc_menu *menu)
{
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
#if defined(DXX_BUILD_DESCENT_I)
			if (menu->changing && (menu->items[menu->citem].type == BT_MOUSE_BUTTON) && (event->type == EVENT_MOUSE_BUTTON_DOWN))
#elif defined(DXX_BUILD_DESCENT_II)
			if (menu->changing && (menu->items[menu->citem].type == BT_MOUSE_BUTTON) && (event->type == EVENT_MOUSE_BUTTON_UP))
#endif
			{
				kc_change_mousebutton( menu, event, &menu->items[menu->citem] );
				menu->mouse_state = (event->type == EVENT_MOUSE_BUTTON_DOWN);
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
			
			for (unsigned i=0; i<(sizeof(kc_keyboard)/sizeof(kc_keyboard[0])); i++ ) 
				PlayerCfg.KeySettings[0][i] = kc_keyboard[i].value;
			
			for (unsigned i=0; i<(sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++ ) 
				PlayerCfg.KeySettings[1][i] = kc_joystick[i].value;

			for (unsigned i=0; i<(sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++ ) 
				PlayerCfg.KeySettings[2][i] = kc_mouse[i].value;
			
			for (unsigned i=0; i<(sizeof(kc_rebirth) / sizeof(kc_rebirth[0])); i++)
				PlayerCfg.KeySettingsRebirth[i] = kc_rebirth[i].value;
			return 0;	// continue closing
			break;
			
		default:
			return 0;
			break;
	}
	
	return 1;
}

static void kconfig_sub(kc_item * items,int nitems, const char *title)
{
	kc_menu *menu;

	CALLOC(menu, kc_menu, 1);
	
	if (!menu)
		return;

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

template <std::size_t N>
static void kconfig_sub(kc_item (&items)[N], const char *title)
{
	kconfig_sub(items, N, title);
}

static void kc_drawitem( kc_item *item, int is_current )
{
	int x, w, h, aw;
	char buf[10];
	const char *btext;

	if (is_current)
		gr_set_fontcolor( BM_XRGB(20,20,29), -1 );
	else
		gr_set_fontcolor( BM_XRGB(15,15,24), -1 );

	gr_string( FSPACX(item->x), FSPACY(item->y), item->text );

	btext = get_item_text(item, buf);
	if (!btext)
		return;
	{
		gr_get_string_size(btext, &w, &h, &aw  );

		if (is_current)
			gr_setcolor( BM_XRGB(21,0,24) );
		else
			gr_setcolor( BM_XRGB(16,0,19) );
		gr_urect( FSPACX(item->xinput), FSPACY(item->y-1), FSPACX(item->xinput+item->w2), FSPACY(item->y)+h );
		
		gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

		x = FSPACX(item->xinput)+((FSPACX(item->w2)-w)/2);
	
		gr_string( x, FSPACY(item->y), btext );
	}
}


static void kc_drawquestion( kc_menu *menu, kc_item *item )
{
	int x, w, h, aw;

	gr_get_string_size("?", &w, &h, &aw  );

#if defined(DXX_BUILD_DESCENT_I)
	int c = BM_XRGB(21,0,24);

	gr_setcolor( gr_fade_table[fades[menu->q_fade_i]*256+c] );
#elif defined(DXX_BUILD_DESCENT_II)
	gr_setcolor(BM_XRGB(21*fades[menu->q_fade_i]/31,0,24*fades[menu->q_fade_i]/31));
#endif
	menu->q_fade_i++;
	if (menu->q_fade_i>63) menu->q_fade_i=0;

	gr_urect( FSPACX(item->xinput), FSPACY(item->y-1), FSPACX(item->xinput+item->w2), FSPACY(item->y)+h );
	
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

	x = FSPACX(item->xinput)+((FSPACX(item->w2)-w)/2);

	gr_string( x, FSPACY(item->y), "?" );
}

static void kc_change_key( kc_menu *menu, d_event *event, kc_item * item )
{
	ubyte keycode = 255;

	Assert(event->type == EVENT_KEY_COMMAND);
	keycode = event_key_get_raw(event);

	if (!(key_properties[keycode].key_text))
		return;

	for (unsigned n=0; n<(GameArg.CtlNoStickyKeys?sizeof(system_keys)-3:sizeof(system_keys)); n++ )
		if ( system_keys[n] == keycode )
			return;

	for (unsigned i=0; i<menu->nitems; i++ )
	{
		unsigned n;
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_KEY) && (menu->items[i].value==keycode) )
		{
			menu->items[i].value = 255;
		}
	}
	item->value = keycode;
	menu->changing = 0;
}

static void kc_change_joybutton( kc_menu *menu, d_event *event, kc_item * item )
{
	int button = 255;

	Assert(event->type == EVENT_JOYSTICK_BUTTON_DOWN);
	button = event_joystick_get_button(event);

	for (unsigned i=0; i<menu->nitems; i++ )
	{
		unsigned n;
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_JOY_BUTTON) && (menu->items[i].value==button) )
			menu->items[i].value = 255;
	}
	item->value = button;
	menu->changing = 0;
}

static void kc_change_mousebutton( kc_menu *menu, d_event *event, kc_item * item )
{
	int button;

	Assert(event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP);
	button = event_mouse_get_button(event);

	for (unsigned i=0; i<menu->nitems; i++)
	{
		unsigned n;
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_MOUSE_BUTTON) && (menu->items[i].value==button) )
			menu->items[i].value = 255;
	}
	item->value = button;
	menu->changing = 0;
}

static void kc_change_joyaxis( kc_menu *menu, d_event *event, kc_item * item )
{
	int axis, value;

	Assert(event->type == EVENT_JOYSTICK_MOVED);
	event_joystick_get_axis( event, &axis, &value );

	if ( abs(value-menu->old_jaxis[axis])<32 )
		return;
	con_printf(CON_DEBUG, "Axis Movement detected: Axis %i\n", axis);

	for (unsigned i=0; i<menu->nitems; i++ )
	{
		unsigned n;
		n = item - menu->items;
		if ( (i!=n) && (menu->items[i].type==BT_JOY_AXIS) && (menu->items[i].value==axis) )
			menu->items[i].value = 255;
	}
	item->value = axis;
	menu->changing = 0;
}

static void kc_change_mouseaxis( kc_menu *menu, d_event *event, kc_item * item )
{
	int dx, dy, dz;
	ubyte code = 255;

	Assert(event->type == EVENT_MOUSE_MOVED);
	event_mouse_get_delta( event, &dx, &dy, &dz );
	if ( abs(dx)>5 ) code = 0;
	if ( abs(dy)>5 ) code = 1;
	if ( abs(dz)>5 ) code = 2;

	if (code!=255)
	{
		for (unsigned i=0; i<menu->nitems; i++ )
		{
			unsigned n;
			n = item - menu->items;
			if ( (i!=n) && (menu->items[i].type==BT_MOUSE_AXIS) && (menu->items[i].value==code) )
				menu->items[i].value = 255;
		}
		item->value = code;
		menu->changing = 0;
	}
}

static void kc_change_invert( kc_menu *menu, kc_item * item )
{
	if (item->value)
		item->value = 0;
	else 
		item->value = 1;

	menu->changing = 0;		// in case we were changing something else
}

#include "screens.h"

void kconfig(int n, const char * title)
{
	set_screen_mode( SCREEN_MENU );
	kc_set_controls();

	switch(n)
    	{
		case 0:kconfig_sub( kc_keyboard,title); break;
		case 1:kconfig_sub( kc_joystick,title); break;
		case 2:kconfig_sub( kc_mouse,   title); break;
		case 3:kconfig_sub( kc_rebirth, title); break;
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
			for (i = 0; i < (sizeof(kc_keyboard) / sizeof(kc_keyboard[0])); i++)
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
					if (kc_rebirth[i].value < 255 && kc_rebirth[i].value == event_key_get_raw(event))
					{
						Controls.select_weapon_count = j+1;
						break;
					}
			break;
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			for (i = 0; i < (sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++)
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
					if (kc_rebirth[i].value < 255 && kc_rebirth[i].value == event_joystick_get_button(event))
					{
						Controls.select_weapon_count = j+1;
						break;
					}
			break;
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			for (i = 0; i < (sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++)
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
					if (kc_rebirth[i].value < 255 && kc_rebirth[i].value == event_mouse_get_button(event))
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
				mouse_delta_time = timer_query() + DESIGNATED_GAME_FRAMETIME;
			}
			break;
		}
		case EVENT_IDLE:
		default:
			if (!PlayerCfg.MouseFlightSim && mouse_delta_time < timer_query())
			{
				Controls.mouse_axis[0] = Controls.mouse_axis[1] = Controls.mouse_axis[2] = 0;
				mouse_delta_time = timer_query() + DESIGNATED_GAME_FRAMETIME;
			}
			break;
	}

	//------------ Read pitch_time -----------
	if ( !Controls.slide_on_state )
	{
		// From keyboard...
		if ( Controls.key_pitch_forward_state ) 
		{
			if ( Controls.key_pitch_forward_down_time < F1_0 )
				Controls.key_pitch_forward_down_time += (!Controls.key_pitch_forward_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[1]/16)+1:FrameTime/4;
			Controls.pitch_time += speed_factor*FrameTime/2*(Controls.key_pitch_forward_down_time/F1_0);
		}
		else
			Controls.key_pitch_forward_down_time = 0;
		if ( Controls.key_pitch_backward_state )
		{
			if ( Controls.key_pitch_backward_down_time < F1_0 )
				Controls.key_pitch_backward_down_time += (!Controls.key_pitch_backward_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[1]/16)+1:FrameTime/4;
			Controls.pitch_time -= speed_factor*FrameTime/2*(Controls.key_pitch_backward_down_time/F1_0);
		}
		else
			Controls.key_pitch_backward_down_time = 0;
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
		// From keyboard...
		if ( Controls.key_pitch_forward_state ) 
		{
			if (Controls.key_pitch_forward_down_time < F1_0)
				Controls.key_pitch_forward_down_time += (!Controls.key_pitch_forward_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[3]/16)+1:FrameTime/4;
			Controls.vertical_thrust_time += speed_factor*FrameTime*(Controls.key_pitch_forward_down_time/F1_0);
		}
		else
			Controls.key_pitch_forward_down_time = 0;
		if ( Controls.key_pitch_backward_state )
		{
			if ( Controls.key_pitch_backward_down_time < F1_0 )
				Controls.key_pitch_backward_down_time += (!Controls.key_pitch_backward_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[3]/16)+1:FrameTime/4;
			Controls.vertical_thrust_time -= speed_factor*FrameTime*(Controls.key_pitch_backward_down_time/F1_0);
		}
		else
			Controls.key_pitch_backward_down_time = 0;
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
	// From keyboard...
	if ( Controls.key_slide_up_state ) 
	{
		if (Controls.key_slide_up_down_time < F1_0)
			Controls.key_slide_up_down_time += (!Controls.key_slide_up_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[3]/16)+1:FrameTime/4;
		Controls.vertical_thrust_time += speed_factor*FrameTime*(Controls.key_slide_up_down_time/F1_0);
	}
	else
		Controls.key_slide_up_down_time = 0;
	if ( Controls.key_slide_down_state )
	{
		if ( Controls.key_slide_down_down_time < F1_0 )
			Controls.key_slide_down_down_time += (!Controls.key_slide_down_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[3]/16)+1:FrameTime/4;
		Controls.vertical_thrust_time -= speed_factor*FrameTime*(Controls.key_slide_down_down_time/F1_0);
	}
	else
		Controls.key_slide_down_down_time = 0;
	// From buttons...
	if ( Controls.btn_slide_up_state ) Controls.vertical_thrust_time += speed_factor*FrameTime;
	if ( Controls.btn_slide_down_state ) Controls.vertical_thrust_time -= speed_factor*FrameTime;
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
		// From keyboard...
		if ( Controls.key_heading_right_state ) 
		{
			if (Controls.key_heading_right_down_time < F1_0)
				Controls.key_heading_right_down_time += (!Controls.key_heading_right_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[0]/16)+1:FrameTime/4;
			Controls.heading_time += speed_factor*FrameTime*(Controls.key_heading_right_down_time/F1_0);
		}
		else
			Controls.key_heading_right_down_time = 0;
		if ( Controls.key_heading_left_state )
		{
			if ( Controls.key_heading_left_down_time < F1_0 )
				Controls.key_heading_left_down_time += (!Controls.key_heading_left_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[0]/16)+1:FrameTime/4;
			Controls.heading_time -= speed_factor*FrameTime*(Controls.key_heading_left_down_time/F1_0);
		}
		else
			Controls.key_heading_left_down_time = 0;
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
		// From keyboard...
		if ( Controls.key_heading_right_state ) 
		{
			if (Controls.key_heading_right_down_time < F1_0)
				Controls.key_heading_right_down_time += (!Controls.key_heading_right_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[2]/16)+1:FrameTime/4;
			Controls.sideways_thrust_time += speed_factor*FrameTime*(Controls.key_heading_right_down_time/F1_0);
		}
		else
			Controls.key_heading_right_down_time = 0;
		if ( Controls.key_heading_left_state )
		{
			if ( Controls.key_heading_left_down_time < F1_0 )
				Controls.key_heading_left_down_time += (!Controls.key_heading_left_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[2]/16)+1:FrameTime/4;
			Controls.sideways_thrust_time -= speed_factor*FrameTime*(Controls.key_heading_left_down_time/F1_0);
		}
		else
			Controls.key_heading_left_down_time = 0;
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
	// From keyboard...
	if ( Controls.key_slide_right_state ) 
	{
		if (Controls.key_slide_right_down_time < F1_0)
			Controls.key_slide_right_down_time += (!Controls.key_slide_right_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[2]/16)+1:FrameTime/4;
		Controls.sideways_thrust_time += speed_factor*FrameTime*(Controls.key_slide_right_down_time/F1_0);
	}
	else
		Controls.key_slide_right_down_time = 0;
	if ( Controls.key_slide_left_state )
	{
		if ( Controls.key_slide_left_down_time < F1_0 )
			Controls.key_slide_left_down_time += (!Controls.key_slide_left_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[1]/16)+1:FrameTime/4;
		Controls.sideways_thrust_time -= speed_factor*FrameTime*(Controls.key_slide_left_down_time/F1_0);
	}
	else
		Controls.key_slide_left_down_time = 0;
	// From buttons...
	if ( Controls.btn_slide_left_state ) Controls.sideways_thrust_time -= speed_factor*FrameTime;
	if ( Controls.btn_slide_right_state ) Controls.sideways_thrust_time += speed_factor*FrameTime;
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
		// From keyboard...
		if ( Controls.key_heading_left_state )
		{
			if ( Controls.key_heading_left_down_time < F1_0 )
				Controls.key_heading_left_down_time += (!Controls.key_heading_left_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[4]/16)+1:FrameTime/4;
			Controls.bank_time += speed_factor*FrameTime*(Controls.key_heading_left_down_time/F1_0);
		}
		else
			Controls.key_bank_left_down_time = 0;
		if ( Controls.key_heading_right_state ) 
		{
			if (Controls.key_heading_right_down_time < F1_0)
				Controls.key_heading_right_down_time += (!Controls.key_heading_right_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[4]/16)+1:FrameTime/4;
			Controls.bank_time -= speed_factor*FrameTime*(Controls.key_heading_right_down_time/F1_0);
		}
		else
			Controls.key_heading_right_down_time = 0;
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
	// From keyboard...
	if ( Controls.key_bank_left_state )
	{
		if ( Controls.key_bank_left_down_time < F1_0 )
			Controls.key_bank_left_down_time += (!Controls.key_bank_left_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[4]/16)+1:FrameTime/4;
		Controls.bank_time += speed_factor*FrameTime*(Controls.key_bank_left_down_time/F1_0);
	}
	else
		Controls.key_bank_left_down_time = 0;
	if ( Controls.key_bank_right_state ) 
	{
		if (Controls.key_bank_right_down_time < F1_0)
			Controls.key_bank_right_down_time += (!Controls.key_bank_right_down_time)?F1_0*((float)PlayerCfg.KeyboardSens[4]/16)+1:FrameTime/4;
		Controls.bank_time -= speed_factor*FrameTime*(Controls.key_bank_right_down_time/F1_0);
	}
	else
		Controls.key_bank_right_down_time = 0;
	// From buttons...
	if ( Controls.btn_bank_left_state ) Controls.bank_time += speed_factor*FrameTime;
	if ( Controls.btn_bank_right_state ) Controls.bank_time -= speed_factor*FrameTime;
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
	for (unsigned i=0; i<(sizeof(kc_keyboard)/sizeof(kc_keyboard[0])); i++ )
		kc_keyboard[i].value = PlayerCfg.KeySettings[0][i];

	for (unsigned i=0; i<(sizeof(kc_joystick) / sizeof(kc_joystick[0])); i++ )
	{
		kc_joystick[i].value = PlayerCfg.KeySettings[1][i];
		if (kc_joystick[i].type == BT_INVERT )
		{
			if (kc_joystick[i].value!=1)
				kc_joystick[i].value = 0;
			PlayerCfg.KeySettings[1][i] = kc_joystick[i].value;
		}
	}

	for (unsigned i=0; i<(sizeof(kc_mouse) / sizeof(kc_mouse[0])); i++ )
	{
		kc_mouse[i].value = PlayerCfg.KeySettings[2][i];
		if (kc_mouse[i].type == BT_INVERT )
		{
			if (kc_mouse[i].value!=1)
				kc_mouse[i].value = 0;
			PlayerCfg.KeySettings[2][i] = kc_mouse[i].value;
		}
	}

	for (unsigned i=0; i<(sizeof(kc_rebirth) / sizeof(kc_rebirth[0])); i++ )
		kc_rebirth[i].value = PlayerCfg.KeySettingsRebirth[i];
}
