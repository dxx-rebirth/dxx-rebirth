/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <cstddef>
#include <stdexcept>
#include <functional>

#include "dxxerror.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "console.h"
#include "palette.h"
#include "physfsx.h"
#include "game.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "kconfig.h"
#include "gauges.h"
#include "rbaudio.h"
#include "render.h"
#include "digi.h"
#include "key.h"
#include "mouse.h"
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

#include "compiler-lengthof.h"
#include "compiler-range_for.h"

#ifndef RELEASE
#define TABLE_CREATION 1
#endif

using std::min;
using std::max;
using std::plus;
using std::minus;

// Array used to 'blink' the cursor while waiting for a keypress.
const array<sbyte, 64> fades{{
	1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,
	16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,
	31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,
	16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1
}};

const array<char[2], 2> invert_text{{"N", "Y"}};
joybutton_text_t joybutton_text;
joyaxis_text_t joyaxis_text;
static const char mouseaxis_text[][8] = { "L/R", "F/B", "WHEEL" };
static const char mousebutton_text[][8] = { "LEFT", "RIGHT", "MID", "M4", "M5", "M6", "M7", "M8", "M9", "M10","M11","M12","M13","M14","M15","M16" };

const array<uint8_t, 19> system_keys{{
	KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN,
	// KEY_*LOCK should always be last since we wanna skip these if -nostickykeys
	KEY_CAPSLOCK, KEY_SCROLLOCK, KEY_NUMLOCK
}};

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

struct kc_item
{
	const short x, y;              // x, y pos of label
	const short xinput;                // x pos of input field
	const short w2;                // length of input field
	const short u,d,l,r;           // neighboring field ids for cursor navigation
	const ubyte type;
	const int state_bit;
	union {
		ubyte control_info::state_controls_t::*ci_state_ptr;
		ubyte control_info::state_controls_t::*ci_count_ptr;
	};
};

struct kc_mitem {
	ubyte value;		// what key,button,etc
};

struct kc_menu : embed_window_pointer_t
{
	const char *litems;
	const kc_item	*items;
	kc_mitem	*mitems;
	const char	*title;
	unsigned	nitems;
	unsigned	citem;
	array<int, JOY_MAX_AXES>	old_jaxis;
	array<int, 3>	old_maxis;
	ubyte	changing;
	ubyte	q_fade_i;	// for flashing the question mark
	ubyte	mouse_state;
};

const array<array<ubyte, MAX_CONTROLS>, 3> DefaultKeySettings{{
#if defined(DXX_BUILD_DESCENT_I)
	{{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x33,0x0,0x34,0x0}},
	{{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0}},
	{{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}},
#elif defined(DXX_BUILD_DESCENT_II)
	{{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0x1f,0xff,0x33,0xff,0x34,0xff,0x23,0xff,0x14,0xff,0xff,0xff,0x0,0x0}},
	{{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0}},
	{{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0}},
#endif
}};
const array<ubyte, MAX_DXX_REBIRTH_CONTROLS> DefaultKeySettingsRebirth{{ 0x2,0xff,0xff,0x3,0xff,0xff,0x4,0xff,0xff,0x5,0xff,0xff,0x6,0xff,0xff,0x7,0xff,0xff,0x8,0xff,0xff,0x9,0xff,0xff,0xa,0xff,0xff,0xb,0xff,0xff }};

//	  id,  x,  y, w1, w2,  u,  d,   l, r,     text,   type, value
static const kc_item kc_keyboard[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ 15, 49, 86, 26, 43,  2, 49,  1, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_pitch_forward} },
	{ 15, 49,115, 26, 48,  3,  0, 24, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_pitch_forward} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 15, 49, 86, 26, 55,  2, 56,  1, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_pitch_forward} },
	{ 15, 49,115, 26, 50,  3,  0, 24, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_pitch_forward} },
#endif
	{ 15, 57, 86, 26,  0,  4, 25,  3, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_pitch_backward} },
	{ 15, 57,115, 26,  1,  5,  2, 26, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_pitch_backward} },
	{ 15, 65, 86, 26,  2,  6, 27,  5, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_heading_left} },
	{ 15, 65,115, 26,  3,  7,  4, 28, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_heading_left} },
	{ 15, 73, 86, 26,  4,  8, 29,  7, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_heading_right} },
	{ 15, 73,115, 26,  5,  9,  6, 34, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_heading_right} },
	{ 15, 85, 86, 26,  6, 10, 35,  9, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::slide_on} },
	{ 15, 85,115, 26,  7, 11,  8, 36, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::slide_on} },
	{ 15, 93, 86, 26,  8, 12, 37, 11, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_left} },
	{ 15, 93,115, 26,  9, 13, 10, 44, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_left} },
	{ 15,101, 86, 26, 10, 14, 45, 13, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_right} },
	{ 15,101,115, 26, 11, 15, 12, 30, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_right} },
	{ 15,109, 86, 26, 12, 16, 31, 15, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_up} },
	{ 15,109,115, 26, 13, 17, 14, 32, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_up} },
	{ 15,117, 86, 26, 14, 18, 33, 17, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_down} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 15,117,115, 26, 15, 19, 16, 38, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_down} },
	{ 15,129, 86, 26, 16, 20, 39, 19, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::bank_on} },
	{ 15,129,115, 26, 17, 21, 18, 40, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::bank_on} },
	{ 15,137, 86, 26, 18, 22, 41, 21, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_left} },
	{ 15,137,115, 26, 19, 23, 20, 42, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_left} },
	{ 15,145, 86, 26, 20, 46, 43, 23, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_right} },
	{ 15,145,115, 26, 21, 47, 22, 46, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_right} },
	{158, 49,241, 26, 49, 26,  1, 25, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::fire_primary} },
	{158, 49,270, 26, 42, 27, 24,  2, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::fire_primary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 15,117,115, 26, 15, 19, 16, 46, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_down} },
	{ 15,129, 86, 26, 16, 20, 47, 19, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::bank_on} },
	{ 15,129,115, 26, 17, 21, 18, 38, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::bank_on} },
	{ 15,137, 86, 26, 18, 22, 39, 21, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_left} },
	{ 15,137,115, 26, 19, 23, 20, 40, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_left} },
	{ 15,145, 86, 26, 20, 48, 41, 23, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_right} },
	{ 15,145,115, 26, 21, 49, 22, 42, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_right} },
	{158, 49,241, 26, 51, 26,  1, 25, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::fire_primary} },
	{158, 49,270, 26, 56, 27, 24,  2, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::fire_primary} },
#endif
	{158, 57,241, 26, 24, 28,  3, 27, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::fire_secondary} },
	{158, 57,270, 26, 25, 29, 26,  4, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::fire_secondary} },
	{158, 65,241, 26, 26, 34,  5, 29, BT_KEY, 0, {&control_info::state_controls_t::fire_flare} },
	{158, 65,270, 26, 27, 35, 28,  6, BT_KEY, 0, {&control_info::state_controls_t::fire_flare} },
	{158,105,241, 26, 44, 32, 13, 31, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::accelerate} },
	{158,105,270, 26, 45, 33, 30, 14, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::accelerate} },
#if defined(DXX_BUILD_DESCENT_I)
	{158,113,241, 26, 30, 38, 15, 33, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::reverse} },
	{158,113,270, 26, 31, 39, 32, 16, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::reverse} },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,113,241, 26, 30, 46, 15, 33, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::reverse} },
	{158,113,270, 26, 31, 47, 32, 16, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::reverse} },
#endif
	{158, 73,241, 26, 28, 36,  7, 35, BT_KEY, 0, {&control_info::state_controls_t::drop_bomb} },
	{158, 73,270, 26, 29, 37, 34,  8, BT_KEY, 0, {&control_info::state_controls_t::drop_bomb} },
	{158, 85,241, 26, 34, 44,  9, 37, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::rear_view} },
	{158, 85,270, 26, 35, 45, 36, 10, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::rear_view} },
#if defined(DXX_BUILD_DESCENT_I)
	{158,125,241, 26, 32, 40, 17, 39, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_plus} },
	{158,125,270, 26, 33, 41, 38, 18, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_plus} },
	{158,133,241, 26, 38, 42, 19, 41, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_minus} },
	{158,133,270, 26, 39, 43, 40, 20, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_minus} },
	{158,141,241, 26, 40, 25, 21, 43, BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
	{158,141,270, 26, 41,  0, 42, 22, BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,133,241, 26, 46, 40, 19, 39, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_plus} },
	{158,133,270, 26, 47, 41, 38, 20, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_plus} },
	{158,141,241, 26, 38, 42, 21, 41, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_minus} },
	{158,141,270, 26, 39, 43, 40, 22, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_minus} },
	{158,149,241, 26, 40, 52, 23, 43, BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
	{158,149,270, 26, 41, 53, 42, 48, BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
#endif
	{158, 93,241, 26, 36, 30, 11, 45, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::automap} },
	{158, 93,270, 26, 37, 31, 44, 12, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::automap} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 15,157, 86, 26, 22, 48, 23, 47, BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,157,115, 26, 23, 49, 46, 48, BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,165, 86, 26, 46,  1, 47, 49, BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 15,165,115, 26, 47, 24, 48,  0, BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,121,241, 26, 32, 38, 17, 47, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::afterburner} },
	{158,121,270, 26, 33, 39, 46, 18, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::afterburner} },
	{ 15,161, 86, 26, 22, 50, 43, 49, BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,161,115, 26, 23, 51, 48, 52, BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,169, 86, 26, 48,  1, 53, 51, BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 15,169,115, 26, 49, 24, 50, 54, BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{158,163,241, 26, 42, 54, 49, 53, BT_KEY, 0, {&control_info::state_controls_t::headlight} },
	{158,163,270, 26, 43, 55, 52, 50, BT_KEY, 0, {&control_info::state_controls_t::headlight} },
	{158,171,241, 26, 52, 56, 51, 55, BT_KEY, STATE_BIT1, {&control_info::state_controls_t::energy_to_shield} },
	{158,171,270, 26, 53,  0, 54, 56, BT_KEY, STATE_BIT2, {&control_info::state_controls_t::energy_to_shield} },
	{158,179,241, 26, 54, 25, 55,  0, BT_KEY, 0, {&control_info::state_controls_t::toggle_bomb} },
#endif
};
static const char *const kcl_keyboard =
	"Pitch forward\0"
	"Pitch backward\0"
	"Turn left\0"
	"Turn right\0"
	"Slide on\0"
	"Slide left\0"
	"Slide right\0"
	"Slide up\0"
	"Slide down\0"
	"Bank on\0"
	"Bank left\0"
	"Bank right\0"
	"Fire primary\0"
	"Fire secondary\0"
	"Fire flare\0"
	"Accelerate\0"
	"Reverse\0"
	"Drop Bomb\0"
	"REAR VIEW\0"
	"Cruise Faster\0"
	"Cruise Slower\0"
	"Cruise Off\0"
	"Automap\0"
#if defined(DXX_BUILD_DESCENT_II)
	"Afterburner\0"
#endif
	"Cycle Primary\0"
	"Cycle Second.\0"
#if defined(DXX_BUILD_DESCENT_II)
	"Headlight\0"
	"Energy->Shield\0"
	"Toggle Bomb\0"
#endif
;
static kc_mitem kcm_keyboard[lengthof(kc_keyboard)];

static const kc_item kc_joystick[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ 22, 46,104, 26, 15,  1, 24, 29, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,104, 26,  0,  4, 34, 30, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,104, 26, 26,  3, 37, 31, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,104, 26,  2, 25, 38, 32, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::reverse} },
	{ 22, 62,104, 26,  1, 26, 35, 33, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{174, 46,248, 26, 23,  6, 29, 34, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::slide_on} },
	{174, 54,248, 26,  5,  7, 30, 35, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,248, 26,  6,  8, 33, 36, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,248, 26,  7,  9, 43, 37, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,248, 26,  8, 10, 31, 38, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,248, 26,  9, 11, 32, 39, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::bank_on} },
	{174, 94,248, 26, 10, 12, 42, 40, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,248, 26, 11, 44, 28, 41, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_right} },
	{ 22,154, 73, 26, 47, 15, 47, 14, BT_JOY_AXIS, 0, {NULL} },
	{ 22,154,121,  8, 27, 16, 13, 17, BT_INVERT, 0, {NULL} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22, 46,102, 26, 15,  1, 24, 31, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,102, 26,  0,  4, 36, 32, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,102, 26, 26,  3, 39, 33, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,102, 26,  2, 25, 40, 34, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::reverse} },
	{ 22, 62,102, 26,  1, 26, 37, 35, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{174, 46,248, 26, 23,  6, 31, 36, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::slide_on} },
	{174, 54,248, 26,  5,  7, 32, 37, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,248, 26,  6,  8, 35, 38, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,248, 26,  7,  9, 45, 39, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,248, 26,  8, 10, 33, 40, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,248, 26,  9, 11, 34, 41, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::bank_on} },
	{174, 94,248, 26, 10, 12, 44, 42, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,248, 26, 11, 28, 46, 43, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_right} },
	{ 22,154, 73, 26, 55, 15, 55, 14, BT_JOY_AXIS, 0, {NULL} },
	{ 22,154,121,  8, 50, 16, 13, 17, BT_INVERT, 0, {NULL} },
#endif
	{ 22,162, 73, 26, 13,  0, 18, 16, BT_JOY_AXIS, 0, {NULL} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 22,162,121,  8, 14, 29, 15, 19, BT_INVERT, 0, {NULL} },
	{164,154,222, 26, 28, 19, 14, 18, BT_JOY_AXIS, 0, {NULL} },
	{164,154,270,  8, 45, 20, 17, 15, BT_INVERT, 0, {NULL} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22,162,121,  8, 14, 31, 15, 19, BT_INVERT, 0, {NULL} },
	{164,154,222, 26, 51, 19, 14, 18, BT_JOY_AXIS, 0, {NULL} },
	{164,154,270,  8, 54, 20, 17, 15, BT_INVERT, 0, {NULL} },
#endif
	{164,162,222, 26, 17, 21, 16, 20, BT_JOY_AXIS, 0, {NULL} },
	{164,162,270,  8, 18, 22, 19, 21, BT_INVERT, 0, {NULL} },
	{164,170,222, 26, 19, 23, 20, 22, BT_JOY_AXIS, 0, {NULL} },
	{164,170,270,  8, 20, 24, 21, 23, BT_INVERT, 0, {NULL} },
	{164,178,222, 26, 21,  5, 22, 24, BT_JOY_AXIS, 0, {NULL} },
#if defined(DXX_BUILD_DESCENT_I)
	{164,178,270,  8, 22, 34, 23,  0, BT_INVERT, 0, {NULL} },
	{ 22, 94,104, 26,  3, 27, 39, 42, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,104, 26,  4,  2, 36, 43, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,104, 26, 25, 14, 40, 28, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::automap} },
	{ 22,102,133, 26, 42, 17, 27, 12, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::automap} },
	{ 22, 46,133, 26, 16, 30,  0,  5, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,133, 26, 29, 33,  1,  6, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,133, 26, 43, 32,  2,  9, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,133, 26, 31, 42,  3, 10, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::reverse} },
	{ 22, 62,133, 26, 30, 43,  4,  7, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{174, 46,278, 26, 24, 35,  5,  1, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::slide_on} },
	{174, 54,278, 26, 34, 36,  6,  4, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,278, 26, 35, 37,  7, 26, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,278, 26, 36, 38,  8,  2, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,278, 26, 37, 39,  9,  3, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,278, 26, 38, 40, 10, 25, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::bank_on} },
	{174, 94,278, 26, 39, 41, 11, 27, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,278, 26, 40, 46, 12, 44, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_right} },
	{ 22, 94,133, 26, 32, 28, 25, 11, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,133, 26, 33, 31, 26,  8, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{174,110,248, 26, 12, 45, 41, 46, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,248, 26, 44, 18, 46, 47, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
	{174,110,278, 26, 41, 47, 44, 45, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,278, 26, 46, 13, 45, 13, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{164,178,270,  8, 22, 36, 23,  0, BT_INVERT, 0, {NULL} },
	{ 22, 94,102, 26,  3, 27, 41, 44, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,102, 26,  4,  2, 38, 45, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,102, 26, 25, 30, 42, 46, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::afterburner} },
	{174,110,248, 26, 12, 29, 49, 47, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,248, 26, 28, 54, 53, 48, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 22,110,102, 26, 27, 52, 43, 49, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::headlight} },
	{ 22, 46,132, 26, 16, 32,  0,  5, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,132, 26, 31, 35,  1,  6, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,132, 26, 45, 34,  2,  9, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,132, 26, 33, 44,  3, 10, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::reverse} },
	{ 22, 62,132, 26, 32, 45,  4,  7, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{174, 46,278, 26, 24, 37,  5,  1, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::slide_on} },
	{174, 54,278, 26, 36, 38,  6,  4, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,278, 26, 37, 39,  7, 26, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,278, 26, 38, 40,  8,  2, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,278, 26, 39, 41,  9,  3, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,278, 26, 40, 42, 10, 25, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::bank_on} },
	{174, 94,278, 26, 41, 43, 11, 27, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,278, 26, 42, 47, 12, 30, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_right} },
	{ 22, 94,132, 26, 34, 46, 25, 11, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,132, 26, 35, 33, 26,  8, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,132, 26, 44, 49, 27, 12, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::afterburner} },
	{174,110,278, 26, 43, 48, 28, 52, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,278, 26, 47, 55, 29, 50, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 22,110,132, 26, 46, 53, 30, 28, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::headlight} },
	{ 22,126,102, 26, 52, 14, 48, 51, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::automap} },
	{ 22,126,132, 26, 53, 17, 50, 54, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::automap} },
	{ 22,118,102, 26, 30, 50, 47, 53, BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::energy_to_shield} },
	{ 22,118,132, 26, 49, 51, 52, 29, BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::energy_to_shield} },
	{174,126,248, 26, 29, 18, 51, 55, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::toggle_bomb} },
	{174,126,278, 26, 48, 13, 54, 13, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::toggle_bomb} },
#endif
};
static const char *const kcl_joystick =
	"Fire primary\0"
	"Fire secondary\0"
	"Accelerate\0"
	"Reverse\0"
	"Fire flare\0"
	"Slide on\0"
	"Slide left\0"
	"Slide right\0"
	"Slide up\0"
	"Slide down\0"
	"Bank on\0"
	"Bank left\0"
	"Bank right\0"
	"Pitch U/D\0"
	"Turn L/R\0"
	"Slide L/R\0"
	"Slide U/D\0"
	"Bank L/R\0"
	"Throttle\0"
	"Rear view\0"
	"Drop bomb\0"
#if defined(DXX_BUILD_DESCENT_I)
	"Automap\0"
#elif defined(DXX_BUILD_DESCENT_II)
	"Afterburner\0"
	"Cycle Primary\0"
	"Cycle Secondary\0"
	"Headlight\0"
#endif
	"Fire primary\0"
	"Fire secondary\0"
	"Accelerate\0"
	"Reverse\0"
	"Fire flare\0"
	"Slide on\0"
	"Slide left\0"
	"Slide right\0"
	"Slide up\0"
	"Slide down\0"
	"Bank on\0"
	"Bank left\0"
	"Bank right\0"
	"Rear view\0"
	"Drop bomb\0"
#if defined(DXX_BUILD_DESCENT_I)
	"Cycle Primary\0"
	"Cycle Secondary\0"
	"\0"	// Cycle Primary
	"\0"	// Cycle Secondary
#elif defined(DXX_BUILD_DESCENT_II)
	"\0"	// Afterburner
	"\0"	// Cycle Primary
	"\0"	// Cycle Secondary
	"\0"	// Headlight
	"Automap\0"
	"Energy->Shield\0"
	"Toggle Bomb\0"
#endif
;
static kc_mitem kcm_joystick[lengthof(kc_joystick)];

static const kc_item kc_mouse[] = {
	{ 25, 46,110, 26, 19,  1, 20,  5, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::fire_primary} },
	{ 25, 54,110, 26,  0,  4,  5,  6, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::fire_secondary} },
	{ 25, 78,110, 26, 26,  3,  8,  9, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::accelerate} },
	{ 25, 86,110, 26,  2, 25,  9, 10, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::reverse} },
	{ 25, 62,110, 26,  1, 26,  6,  7, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{180, 46,239, 26, 23,  6,  0,  1, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::slide_on} },
	{180, 54,239, 26,  5,  7,  1,  4, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_left} },
	{180, 62,239, 26,  6,  8,  4, 26, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_right} },
	{180, 70,239, 26,  7,  9, 26,  2, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_up} },
	{180, 78,239, 26,  8, 10,  2,  3, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_down} },
	{180, 86,239, 26,  9, 11,  3, 25, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::bank_on} },
	{180, 94,239, 26, 10, 12, 25, 27, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_bank_left} },
	{180,102,239, 26, 11, 22, 27, 28, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_bank_right} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 25,154, 83, 26, 24, 15, 28, 14, BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,154,131,  8, 28, 16, 13, 21, BT_INVERT, 0, {NULL} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 25,154, 83, 26, 24, 15, 29, 14, BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,154,131,  8, 29, 16, 13, 21, BT_INVERT, 0, {NULL} },
#endif
	{ 25,162, 83, 26, 13, 17, 22, 16, BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,162,131,  8, 14, 18, 15, 23, BT_INVERT, 0, {NULL} },
	{ 25,170, 83, 26, 15, 19, 24, 18, BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,170,131,  8, 16, 20, 17, 19, BT_INVERT, 0, {NULL} },
	{ 25,178, 83, 26, 17,  0, 18, 20, BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,178,131,  8, 18, 21, 19,  0, BT_INVERT, 0, {NULL} },
	{180,154,238, 26, 20, 23, 14, 22, BT_MOUSE_AXIS, 0, {NULL} },
	{180,154,286,  8, 12, 24, 21, 15, BT_INVERT, 0, {NULL} },
	{180,162,238, 26, 21,  5, 16, 24, BT_MOUSE_AXIS, 0, {NULL} },
	{180,162,286,  8, 22, 13, 23, 17, BT_INVERT, 0, {NULL} },
	{ 25, 94,110, 26,  3, 27, 10, 11, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::rear_view} },
	{ 25, 70,110, 26,  4,  2,  7,  8, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 25,102,110, 26, 25, 28, 11, 12, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 25,110,110, 26, 27, 14, 12, 13, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 25,102,110, 26, 25, 28, 11, 12, BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::afterburner} },
	{ 25,110,110, 26, 27, 29, 12, 29, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 25,118,110, 26, 28, 14, 28, 13, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#endif
};
static const char *const kcl_mouse =
	"Fire primary\0"
	"Fire secondary\0"
	"Accelerate\0"
	"reverse\0"
	"Fire flare\0"
	"Slide on\0"
	"Slide left\0"
	"Slide right\0"
	"Slide up\0"
	"Slide down\0"
	"Bank on\0"
	"Bank left\0"
	"Bank right\0"
	"Pitch U/D\0"
	"Turn L/R\0"
	"Slide L/R\0"
	"Slide U/D\0"
	"Bank L/R\0"
	"Throttle\0"
	"REAR VIEW\0"
	"Drop Bomb\0"
#if defined(DXX_BUILD_DESCENT_II)
	"Afterburner\0"
#endif
	"Cycle Primary\0"
	"Cycle Secondary\0"
;
static kc_mitem kcm_mouse[lengthof(kc_mouse)];

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

static const kc_item kc_rebirth[] = {
	{ 15, 69,157, 26, 29,  3, 29,  1, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 69,215, 26, 27,  4,  0,  2, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 69,273, 26, 28,  5,  1,  3, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,157, 26,  0,  6,  2,  4, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,215, 26,  1,  7,  3,  5, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,273, 26,  2,  8,  4,  6, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,157, 26,  3,  9,  5,  7, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,215, 26,  4, 10,  6,  8, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,273, 26,  5, 11,  7,  9, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,157, 26,  6, 12,  8, 10, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,215, 26,  7, 13,  9, 11, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,273, 26,  8, 14, 10, 12, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,157, 26,  9, 15, 11, 13, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,215, 26, 10, 16, 12, 14, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,273, 26, 11, 17, 13, 15, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,157, 26, 12, 18, 14, 16, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,215, 26, 13, 19, 15, 17, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,273, 26, 14, 20, 16, 18, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,157, 26, 15, 21, 17, 19, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,215, 26, 16, 22, 18, 20, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,273, 26, 17, 23, 19, 21, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,157, 26, 18, 24, 20, 22, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,215, 26, 19, 25, 21, 23, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,273, 26, 20, 26, 22, 24, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,157, 26, 21, 27, 23, 25, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,215, 26, 22, 28, 24, 26, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,273, 26, 23, 29, 25, 27, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,157, 26, 24,  1, 26, 28, BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,215, 26, 25,  2, 27, 29, BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,273, 26, 26,  0, 28,  0, BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
};
static const char *const kcl_rebirth =
	WEAPON_STRING_LASER	"\0"
	WEAPON_STRING_VULCAN	"\0"
	WEAPON_STRING_SPREADFIRE	"\0"
	WEAPON_STRING_PLASMA	"\0"
	WEAPON_STRING_FUSION	"\0"
	WEAPON_STRING_CONCUSSION	"\0"
	WEAPON_STRING_HOMING	"\0"
	WEAPON_STRING_PROXIMITY	"\0"
	WEAPON_STRING_SMART	"\0"
	WEAPON_STRING_MEGA	"\0"
;
static kc_mitem kcm_rebirth[lengthof(kc_rebirth)];

static void kc_drawinput( const kc_item &item, kc_mitem& mitem, int is_current, const char *label );
static void kc_change_key( kc_menu &menu,const d_event &event, kc_mitem& mitem );
static void kc_change_joybutton( kc_menu &menu,const d_event &event, kc_mitem& mitem );
static void kc_change_mousebutton( kc_menu &menu,const d_event &event, kc_mitem& mitem );
static void kc_change_joyaxis( kc_menu &menu,const d_event &event, kc_mitem& mitem );
static void kc_change_mouseaxis( kc_menu &menu,const d_event &event, kc_mitem& mitem );
static void kc_change_invert( kc_menu *menu, kc_mitem * item );
static void kc_drawquestion( kc_menu *menu, const kc_item *item );

#ifdef TABLE_CREATION
static const kc_item *find_item_at(const kc_item *const ib, const kc_item *const ie, int x, int y)
{
	const auto predicate = [=](const kc_item &i) {
		return i.xinput == x && i.y == y;
	};
	return std::find_if(ib, ie, predicate);
}

namespace {

class find_item_single
{
	uint_fast32_t current;
	const uint_fast32_t limit;
public:
	find_item_single(uint_fast32_t c, uint_fast32_t l) :
		current(c), limit(l)
	{
	}
	uint_fast32_t value() const { return current; }
	/* Return true on reset, false otherwise */
	bool decrement()
	{
		if (current)
			return -- current, false;
		return current = limit - 1, true;
	}
	/* Return true on reset, false otherwise */
	bool increment()
	{
		if (current != limit)
			return ++ current, false;
		return current = 0, true;
	}
};

class find_item_state
{
	find_item_single x, y;
public:
	find_item_state(const kc_item &citem, const uint_fast32_t xl, const uint_fast32_t yl) :
		x(citem.xinput, xl), y(citem.y, yl)
	{
	}
	uint_fast32_t x_value() const { return x.value(); }
	uint_fast32_t y_value() const { return y.value(); }
	bool x_decrement() { return x.decrement(); }
	bool y_decrement() { return y.decrement(); }
	bool x_increment() { return x.increment(); }
	bool y_increment() { return y.increment(); }
};

}

template <bool (find_item_state::*outer_step)(), bool (find_item_state::*inner_step)()>
static inline std::ptrdiff_t find_next_item(const kc_item *const ib, const kc_item *const ie, find_item_state state)
{
	bool looped = false;
	for (;;)
	{
		if (unlikely((state.*outer_step)()))
			if (unlikely((state.*inner_step)()))
			{
				if (unlikely(looped))
					/* Sanity check.  If looped is true and inner_step
					 * returned true again, then the entire area has
					 * already been searched.
					 */
					return std::distance(ib, ie);
				looped = true;
			}
		auto i = find_item_at(ib, ie, state.x_value(), state.y_value());
		if (i != ie)
			return std::distance(ib, i);
	}
}

static std::ptrdiff_t find_next_item_up(const kc_item *const ib, const kc_item *const ie, const find_item_state &state)
{
	return find_next_item<&find_item_state::y_decrement, &find_item_state::x_decrement>(ib, ie, state);
}

static std::ptrdiff_t find_next_item_down(const kc_item *const ib, const kc_item *const ie, const find_item_state &state)
{
	return find_next_item<&find_item_state::y_increment, &find_item_state::x_increment>(ib, ie, state);
}

static std::ptrdiff_t find_next_item_right(const kc_item *const ib, const kc_item *const ie, const find_item_state &state)
{
	return find_next_item<&find_item_state::x_increment, &find_item_state::y_increment>(ib, ie, state);
}

static std::ptrdiff_t find_next_item_left(const kc_item *const ib, const kc_item *const ie, const find_item_state &state)
{
	return find_next_item<&find_item_state::x_decrement, &find_item_state::y_decrement>(ib, ie, state);
}

static const char btype_text[][13] = {
	"KEY",
	"MOUSE_BUTTON",
	"MOUSE_AXIS",
	"JOY_BUTTON",
	"JOY_AXIS",
	"INVERT"
};

template <std::size_t N>
static void print_create_table_items(PHYSFS_file *fp, const char *type, const char *litems, const kc_item (&items)[N])
{
	PHYSFSX_printf( fp, "\nstatic const kc_item kc_%s[] = {\n", type );
	const grs_bitmap &cv_bitmap = grd_curcanv->cv_bitmap;
	const uint_fast32_t bm_w = cv_bitmap.bm_w, bm_h = cv_bitmap.bm_h;
	range_for (auto &i, items)
	{
		short u,d,l,r;
		const auto ib = std::begin(items);
		const auto ie = std::end(items);
		const find_item_state s{i, bm_w, bm_h};
		u = find_next_item_up(ib, ie, s);
		d = find_next_item_down(ib, ie, s);
		l = find_next_item_left(ib, ie, s);
		r = find_next_item_right(ib, ie, s);
		PHYSFSX_printf( fp, "\t{ %3d,%3d,%3d,%3d,%3hd,%3hd,%3hd,%3hd, BT_%s },\n", 
					   i.x, i.y, i.xinput, i.w2,
					   u, d, l, r,
					   btype_text[i.type] );
	}
	PHYSFSX_printf( fp, "};\n"
		"static const char *const kcl_%s =\n", type);
	const char *litem = litems;
	for (unsigned i=0; i < N; ++i )
	{
		PHYSFSX_printf( fp, "\t\"%s\\0\"\n", litem);
		litem += strlen(litem) + 1;
	}
	PHYSFSX_printf( fp, ";\n"
		"static kc_mitem kcm_%1$s[lengthof(kc_%1$s)];\n", type );
}
#endif

static const char *get_item_text(const kc_item &item, const kc_mitem &mitem, char (&buf)[10])
{
	if (mitem.value==255) {
		return "";
	} else {
		switch( item.type )	{
			case BT_KEY:
				return key_properties[mitem.value].key_text;
			case BT_MOUSE_BUTTON:
				return mousebutton_text[mitem.value];
			case BT_MOUSE_AXIS:
				return mouseaxis_text[mitem.value];
			case BT_JOY_BUTTON:
				if (joybutton_text.size() > mitem.value)
					return &joybutton_text[mitem.value][0];
				else
				{
					snprintf(buf, sizeof(buf), "BTN%2d", mitem.value + 1);
					return buf;
				}
				break;
			case BT_JOY_AXIS:
				if (joyaxis_text.size() > mitem.value)
					return &joyaxis_text[mitem.value][0];
				else
				{
					snprintf(buf, sizeof(buf), "AXIS%2d", mitem.value + 1);
					return buf;
				}
				break;
			case BT_INVERT:
				return invert_text[mitem.value];
			default:
				return NULL;
		}
	}
}

static int get_item_height(const kc_item &item, const kc_mitem &mitem)
{
	int w, h, aw;
	char buf[10];
	const char *btext;

	btext = get_item_text(item, mitem, buf);
	if (!btext)
		return 0;
	gr_get_string_size(btext, &w, &h, &aw  );

	return h;
}

static void kconfig_draw(kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	int w = FSPACX(290), h = FSPACY(170);

	gr_set_current_canvas(NULL);
	nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

	gr_set_current_canvas(window_get_canvas(*menu->wind));

	const grs_font *save_font = grd_curcanv->cv_font;
	gr_set_curfont(MEDIUM3_FONT);

	Assert(!strchr( menu->title, '\n' ));
	gr_string( 0x8000, FSPACY(8), menu->title );

	gr_set_curfont(GAME_FONT);
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
	
	unsigned citem = menu->citem;
	const char *current_label = NULL;
	const char *litem = menu->litems;
	for (unsigned i=0; i < menu->nitems; i++ )	{
		int next_label = (i + 1 >= menu->nitems || menu->items[i + 1].y != menu->items[i].y);
		if (i == citem)
			current_label = litem;
		else
			kc_drawinput( menu->items[i], menu->mitems[i], 0, next_label ? litem : NULL );
		if (next_label)
			litem += strlen(litem) + 1;
	}
	kc_drawinput( menu->items[citem], menu->mitems[citem], 1, current_label );
	
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
		kc_change_invert(menu, &menu->mitems[menu->citem]);
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

static window_event_result kconfig_mouse(window *wind,const d_event &event, kc_menu *menu)
{
	grs_canvas * save_canvas = grd_curcanv;
	int mx, my, mz, x1, y1;
	window_event_result rval = window_event_result::ignored;

	gr_set_current_canvas(window_get_canvas(*wind));
	
	if (menu->mouse_state)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		for (unsigned i=0; i<menu->nitems; i++ )	{
			item_height = get_item_height( menu->items[i], menu->mitems[i] );
			x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[i].xinput);
			y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[i].y);
			if (in_bounds(mx, my, x1, FSPACX(menu->items[i].w2), y1, item_height)) {
				menu->citem = i;
				rval = window_event_result::handled;
				break;
			}
		}
	}
	else if (event.type == EVENT_MOUSE_BUTTON_UP)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		item_height = get_item_height( menu->items[menu->citem], menu->mitems[menu->citem] );
		x1 = grd_curcanv->cv_bitmap.bm_x + FSPACX(menu->items[menu->citem].xinput);
		y1 = grd_curcanv->cv_bitmap.bm_y + FSPACY(menu->items[menu->citem].y);
		if (in_bounds(mx, my, x1, FSPACX(menu->items[menu->citem].w2), y1, item_height)) {
			kconfig_start_changing(menu);
			rval = window_event_result::handled;
		}
		else
		{
			// Click out of changing mode - kreatordxx
			menu->changing = 0;
			rval = window_event_result::handled;
		}
	}
	
	gr_set_current_canvas(save_canvas);
	
	return rval;
}

template <std::size_t M, std::size_t C>
static void reset_mitem_values(kc_mitem (&m)[M], const array<ubyte, C> &c)
{
	for (unsigned i=0; i < min(lengthof(m), C); i++)
		m[i].value = c[i];
}

static window_event_result kconfig_key_command(window *, const d_event &event, kc_menu *menu)
{
	int k;

	k = event_key_get(event);

	// when changing, process no keys instead of ESC
	if (menu->changing && (k != -2 && k != KEY_ESC))
		return window_event_result::ignored;
	switch (k)
	{
		case KEY_CTRLED+KEY_D:
			menu->mitems[menu->citem].value = 255;
			return window_event_result::handled;
		case KEY_CTRLED+KEY_R:	
			if ( menu->items==kc_keyboard )
				reset_mitem_values(kcm_keyboard, DefaultKeySettings[0]);

			if ( menu->items==kc_joystick )
				reset_mitem_values(kcm_joystick, DefaultKeySettings[1]);

			if ( menu->items==kc_mouse )
				reset_mitem_values(kcm_mouse, DefaultKeySettings[2]);
			if ( menu->items==kc_rebirth )
				reset_mitem_values(kcm_rebirth, DefaultKeySettingsRebirth);
			return window_event_result::handled;
		case KEY_DELETE:
			menu->mitems[menu->citem].value=255;
			return window_event_result::handled;
		case KEY_UP: 		
		case KEY_PAD8:
			menu->citem = menu->items[menu->citem].u; 
			return window_event_result::handled;
		case KEY_DOWN:
		case KEY_PAD2:
			menu->citem = menu->items[menu->citem].d; 
			return window_event_result::handled;
		case KEY_LEFT:
		case KEY_PAD4:
			menu->citem = menu->items[menu->citem].l; 
			return window_event_result::handled;
		case KEY_RIGHT:
		case KEY_PAD6:
			menu->citem = menu->items[menu->citem].r; 
			return window_event_result::handled;
		case KEY_ENTER:
		case KEY_PADENTER:
			kconfig_start_changing(menu);
			return window_event_result::handled;
		case -2:	
		case KEY_ESC:
			if (menu->changing)
				menu->changing = 0;
			else
			{
				return window_event_result::close;
			}
			return window_event_result::handled;
#ifdef TABLE_CREATION
		case KEY_F12:
			if (auto fp = PHYSFSX_openWriteBuffered("kconfig.cod"))
			{
				PHYSFSX_printf( fp, "const ubyte DefaultKeySettings[3][MAX_CONTROLS] = {\n" );
				for (unsigned i=0; i<3; i++ )	{
					PHYSFSX_printf( fp, "{0x%2x", PlayerCfg.KeySettings[i][0] );
					for (int j=1; j<MAX_CONTROLS; j++ )
						PHYSFSX_printf( fp, ",0x%2x", PlayerCfg.KeySettings[i][j] );
					PHYSFSX_printf( fp, "},\n" );
				}
				PHYSFSX_printf( fp, "};\n" );

				print_create_table_items(fp, "keyboard", kcl_keyboard, kc_keyboard);
				print_create_table_items(fp, "joystick", kcl_joystick, kc_joystick);
				print_create_table_items(fp, "mouse", kcl_mouse, kc_mouse);
				print_create_table_items(fp, "rebirth", kcl_rebirth, kc_rebirth);
			}
			return window_event_result::handled;
#endif
		case 0:		// some other event
			break;
			
		default:
			break;
	}
	return window_event_result::ignored;
}

static window_event_result kconfig_handler(window *wind,const d_event &event, kc_menu *menu)
{
	switch (event.type)
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
			if (menu->changing && (menu->items[menu->citem].type == BT_MOUSE_BUTTON) && (event.type == EVENT_MOUSE_BUTTON_DOWN))
#elif defined(DXX_BUILD_DESCENT_II)
			if (menu->changing && (menu->items[menu->citem].type == BT_MOUSE_BUTTON) && (event.type == EVENT_MOUSE_BUTTON_UP))
#endif
			{
				kc_change_mousebutton(*menu, event, menu->mitems[menu->citem] );
				menu->mouse_state = (event.type == EVENT_MOUSE_BUTTON_DOWN);
				return window_event_result::handled;
			}

			if (event_mouse_get_button(event) == MBTN_RIGHT)
			{
				if (!menu->changing)
				{
					return window_event_result::close;
				}
				return window_event_result::handled;
			}
			else if (event_mouse_get_button(event) != MBTN_LEFT)
				return window_event_result::ignored;

			menu->mouse_state = (event.type == EVENT_MOUSE_BUTTON_DOWN);
			return kconfig_mouse(wind, event, menu);

		case EVENT_MOUSE_MOVED:
			if (menu->changing && menu->items[menu->citem].type == BT_MOUSE_AXIS) kc_change_mouseaxis(*menu, event, menu->mitems[menu->citem]);
			else
				event_mouse_get_delta( event, &menu->old_maxis[0], &menu->old_maxis[1], &menu->old_maxis[2]);
			break;

		case EVENT_JOYSTICK_BUTTON_DOWN:
			if (menu->changing && menu->items[menu->citem].type == BT_JOY_BUTTON) kc_change_joybutton(*menu, event, menu->mitems[menu->citem]);
			break;

		case EVENT_JOYSTICK_MOVED:
			if (menu->changing && menu->items[menu->citem].type == BT_JOY_AXIS) kc_change_joyaxis(*menu, event, menu->mitems[menu->citem]);
			else
			{
				int axis, value;
				event_joystick_get_axis( event, &axis, &value );
				menu->old_jaxis[axis] = value;
			}
			break;

		case EVENT_KEY_COMMAND:
		{
			window_event_result rval = kconfig_key_command(wind, event, menu);
			if (rval != window_event_result::ignored)
				return rval;
			if (menu->changing && menu->items[menu->citem].type == BT_KEY) kc_change_key(*menu, event, menu->mitems[menu->citem]);
			return window_event_result::ignored;
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
			delete menu;
			
			// Update save values...
			
			for (unsigned i=0; i < lengthof(kc_keyboard); i++ ) 
				PlayerCfg.KeySettings[0][i] = kcm_keyboard[i].value;
			
			for (unsigned i=0; i < lengthof(kc_joystick); i++ ) 
				PlayerCfg.KeySettings[1][i] = kcm_joystick[i].value;

			for (unsigned i=0; i < lengthof(kc_mouse); i++ ) 
				PlayerCfg.KeySettings[2][i] = kcm_mouse[i].value;
			
			for (unsigned i=0; i < lengthof(kc_rebirth); i++)
				PlayerCfg.KeySettingsRebirth[i] = kcm_rebirth[i].value;
			return window_event_result::ignored;	// continue closing
		default:
			return window_event_result::ignored;
			break;
	}
	return window_event_result::handled;
}

static void kconfig_sub(const char *litems, const kc_item * items,kc_mitem *mitems,int nitems, const char *title)
{
	kc_menu *menu = new kc_menu{};
	menu->items = items;
	menu->litems = litems;
	menu->mitems = mitems;
	menu->nitems = nitems;
	menu->title = title;
	menu->citem = 0;
	menu->changing = 0;
	menu->mouse_state = 0;

	if (!(menu->wind = window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200),
					   kconfig_handler, menu)))
		delete menu;
}

template <std::size_t N>
static void kconfig_sub(const char *litems, const kc_item (&items)[N], kc_mitem (&mitems)[N], const char *title)
{
	kconfig_sub(litems, items, mitems, N, title);
}

static void kc_drawinput(const kc_item &item, kc_mitem& mitem, int is_current, const char *label )
{
	int x, w, h, aw;
	char buf[10];
	const char *btext;
	if (label)
	{
		if (is_current)
			gr_set_fontcolor( BM_XRGB(20,20,29), -1 );
		else
			gr_set_fontcolor( BM_XRGB(15,15,24), -1 );

		gr_string( FSPACX(item.x), FSPACY(item.y), label );
	}

	btext = get_item_text(item, mitem, buf);
	if (!btext)
		return;
	{
		gr_get_string_size(btext, &w, &h, &aw  );

		if (is_current)
			gr_setcolor( BM_XRGB(21,0,24) );
		else
			gr_setcolor( BM_XRGB(16,0,19) );
		gr_urect( FSPACX(item.xinput), FSPACY(item.y-1), FSPACX(item.xinput+item.w2), FSPACY(item.y)+h );
		
		gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

		x = FSPACX(item.xinput)+((FSPACX(item.w2)-w)/2);
	
		gr_string( x, FSPACY(item.y), btext );
	}
}


static void kc_drawquestion( kc_menu *menu, const kc_item *item )
{
	int x, w, h, aw;

	gr_get_string_size("?", &w, &h, &aw  );

#if defined(DXX_BUILD_DESCENT_I)
	int c = BM_XRGB(21,0,24);

	gr_setcolor( gr_fade_table[fades[menu->q_fade_i]][c] );
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

static void kc_set_exclusive_binding(kc_menu &menu, kc_mitem &mitem, unsigned type, unsigned value)
{
	for (unsigned i=0; i < menu.nitems; i++ )
	{
		if ( (&menu.mitems[i] != &mitem) && (menu.items[i].type==type) && (menu.mitems[i].value==value) )
		{
			menu.mitems[i].value = 255;
		}
	}
	mitem.value = value;
	menu.changing = 0;
}

static void kc_change_key( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	ubyte keycode = 255;

	Assert(event.type == EVENT_KEY_COMMAND);
	keycode = event_key_get_raw(event);

	auto e = end(system_keys);
	if (unlikely(GameArg.CtlNoStickyKeys))
		e = std::prev(e, 3);
	const auto predicate = [keycode](uint8_t k) { return keycode == k; };
	if (std::any_of(begin(system_keys), e, predicate))
		return;

	kc_set_exclusive_binding(menu, mitem, BT_KEY, keycode);
}

static void kc_change_joybutton( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	int button = 255;

	Assert(event.type == EVENT_JOYSTICK_BUTTON_DOWN);
	button = event_joystick_get_button(event);

	kc_set_exclusive_binding(menu, mitem, BT_JOY_BUTTON, button);
}

static void kc_change_mousebutton( kc_menu &menu,const d_event &event, kc_mitem &mitem)
{
	int button;

	Assert(event.type == EVENT_MOUSE_BUTTON_DOWN || event.type == EVENT_MOUSE_BUTTON_UP);
	button = event_mouse_get_button(event);

	kc_set_exclusive_binding(menu, mitem, BT_MOUSE_BUTTON, button);
}

static void kc_change_joyaxis( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	int axis, value;

	Assert(event.type == EVENT_JOYSTICK_MOVED);
	event_joystick_get_axis( event, &axis, &value );

	if ( abs(value-menu.old_jaxis[axis])<32 )
		return;
	con_printf(CON_DEBUG, "Axis Movement detected: Axis %i", axis);

	kc_set_exclusive_binding(menu, mitem, BT_JOY_AXIS, axis);
}

static void kc_change_mouseaxis( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	int dx, dy, dz;

	Assert(event.type == EVENT_MOUSE_MOVED);
	event_mouse_get_delta( event, &dx, &dy, &dz );
	uint8_t code;
	if (abs(dz) > 5)
		code = 2;
	else if (abs(dy) > 5)
		code = 1;
	else if (abs(dx) > 5)
		code = 0;
	else
		return;
		kc_set_exclusive_binding(menu, mitem, BT_MOUSE_AXIS, code);
}

static void kc_change_invert( kc_menu *menu, kc_mitem * item )
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
		case 0:kconfig_sub( kcl_keyboard, kc_keyboard,kcm_keyboard,title); break;
		case 1:kconfig_sub( kcl_joystick, kc_joystick,kcm_joystick,title); break;
		case 2:kconfig_sub( kcl_mouse,	  kc_mouse,   kcm_mouse,   title); break;
		case 3:kconfig_sub( kcl_rebirth,  kc_rebirth, kcm_rebirth, title); break;
		default:
			Int3();
			return;
	}
}

static void input_button_matched(const kc_item& item, int down)
{
	if (item.state_bit)
	{
		if (!item.ci_state_ptr)
			throw std::logic_error("NULL state pointer with non-zero state bit");
		if (down)
			Controls.state.*item.ci_state_ptr |= item.state_bit;
		else
			Controls.state.*item.ci_state_ptr &= ~item.state_bit;
	}
	else
	{
		if (item.ci_count_ptr != NULL && down)
			Controls.state.*item.ci_count_ptr += 1;
	}
}

template <template<typename> class F>
static void adjust_ramped_keyboard_field(float& keydown_time, ubyte& state, fix& time, const int& sensitivity, const int& speed_factor, const int& speed_divisor = 1)
#define adjust_ramped_keyboard_field(F, M, ...)	\
	(adjust_ramped_keyboard_field<F>(Controls.down_time.M, Controls.state.M, __VA_ARGS__))
{
	if (state)
	{
		if (keydown_time < F1_0)
			keydown_time += (!keydown_time)?F1_0*((float)sensitivity/16)+1:FrameTime/4;
		time = F<fix>()(time, speed_factor*FrameTime/speed_divisor*(keydown_time/F1_0));
	}
	else
		keydown_time = 0;
}

template <std::size_t N>
static void adjust_axis_field(fix& time, const array<fix, N> &axes, unsigned value, unsigned invert, const int& sensitivity)
{
	if (value == 255)
		return;
	fix amount = (axes.at(value) * sensitivity) / 8;
	if ( !invert ) // If not inverted...
		time -= amount;
	else
		time += amount;
}

static void clamp_value(fix& value, const fix& lower, const fix& upper)
{
	value = min(max(value, lower), upper);
}

static void clamp_symmetric_value(fix& value, const fix& bound)
{
	clamp_value(value, -bound, bound);
}

void convert_raw_joy_axis(int kcm_index, int player_cfg_index, int i)
{
	if (i == kcm_joystick[kcm_index].value) {
		if (abs(Controls.raw_joy_axis[i]) <= (128 * PlayerCfg.JoystickLinear[player_cfg_index]) / 16) {
			Controls.joy_axis[i] = (Controls.raw_joy_axis[i]*(FrameTime * PlayerCfg.JoystickSpeed[player_cfg_index]) / 16)/128;
		}
		else {
			Controls.joy_axis[i] = (Controls.raw_joy_axis[i]*FrameTime)/128;
		}
	}
}

void kconfig_read_controls(const d_event &event, int automap_flag)
{
	int speed_factor = cheats.turbo?2:1;
	static fix64 mouse_delta_time = 0;

#ifndef NDEBUG
	// --- Don't do anything if in debug mode ---
	if ( keyd_pressed[KEY_DELETE] )
	{
		Controls = {};
		return;
	}
#endif

	Controls.pitch_time = Controls.vertical_thrust_time = Controls.heading_time = Controls.sideways_thrust_time = Controls.bank_time = Controls.forward_thrust_time = 0;

	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
			for (uint_fast32_t i = 0; i < lengthof(kc_keyboard); i++)
			{
				if (kcm_keyboard[i].value < 255 && kcm_keyboard[i].value == event_key_get_raw(event))
				{
					input_button_matched(kc_keyboard[i], (event.type==EVENT_KEY_COMMAND));
				}
			}
			if (!automap_flag && event.type == EVENT_KEY_COMMAND)
				for (uint_fast32_t i = 0, j = 0; i < 28; i += 3, j++)
					if (kcm_rebirth[i].value < 255 && kcm_rebirth[i].value == event_key_get_raw(event))
					{
						Controls.state.select_weapon = j+1;
						break;
					}
			break;
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			for (uint_fast32_t i = 0; i < lengthof(kc_joystick); i++)
			{
				if (kcm_joystick[i].value < 255 && kc_joystick[i].type == BT_JOY_BUTTON && kcm_joystick[i].value == event_joystick_get_button(event))
				{
					input_button_matched(kc_joystick[i], (event.type==EVENT_JOYSTICK_BUTTON_DOWN));
				}
			}
			if (!automap_flag && event.type == EVENT_JOYSTICK_BUTTON_DOWN)
				for (uint_fast32_t i = 1, j = 0; i < 29; i += 3, j++)
					if (kcm_rebirth[i].value < 255 && kcm_rebirth[i].value == event_joystick_get_button(event))
					{
						Controls.state.select_weapon = j+1;
						break;
					}
			break;
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			for (uint_fast32_t i = 0; i < lengthof(kc_mouse); i++)
			{
				if (kcm_mouse[i].value < 255 && kc_mouse[i].type == BT_MOUSE_BUTTON && kcm_mouse[i].value == event_mouse_get_button(event))
				{
					input_button_matched(kc_mouse[i], (event.type==EVENT_MOUSE_BUTTON_DOWN));
				}
			}
			if (!automap_flag && event.type == EVENT_MOUSE_BUTTON_DOWN)
				for (uint_fast32_t i = 2, j = 0; i < 30; i += 3, j++)
					if (kcm_rebirth[i].value < 255 && kcm_rebirth[i].value == event_mouse_get_button(event))
					{
						Controls.state.select_weapon = j+1;
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

			if (axis == kcm_joystick[13].value) // Pitch U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[1]*8;
			if (axis == kcm_joystick[15].value) // Turn L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[0]*8;
			if (axis == kcm_joystick[17].value) // Slide L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[2]*8;
			if (axis == kcm_joystick[19].value) // Slide U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[3]*8;
			if (axis == kcm_joystick[21].value) // Bank Deadzone
				joy_null_value = PlayerCfg.JoystickDead[4]*8;
			if (axis == kcm_joystick[23].value) // Throttle - default deadzone
				joy_null_value = PlayerCfg.JoystickDead[5]*3;

			if (Controls.raw_joy_axis[axis] > joy_null_value) 
				Controls.raw_joy_axis[axis] = ((Controls.raw_joy_axis[axis]-joy_null_value)*128)/(128-joy_null_value);
			else if (Controls.raw_joy_axis[axis] < -joy_null_value)
				Controls.raw_joy_axis[axis] = ((Controls.raw_joy_axis[axis]+joy_null_value)*128)/(128-joy_null_value);
			else
				Controls.raw_joy_axis[axis] = 0;
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
				for (uint_fast32_t i = 0; i <= 2; i++)
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
				Controls.mouse_axis[0] = (Controls.raw_mouse_axis[0]*FrameTime)/4;
				Controls.mouse_axis[1] = (Controls.raw_mouse_axis[1]*FrameTime)/4;
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
	
	for (int i = 0; i < JOY_MAX_AXES; i++) {
		convert_raw_joy_axis(15, 0, i); // Turn L/R
		convert_raw_joy_axis(13, 1, i); // Pitch U/D
		convert_raw_joy_axis(17, 2, i); // Slide L/R
		convert_raw_joy_axis(19, 3, i); // Slide U/D
		convert_raw_joy_axis(21, 4, i); // Bank
		convert_raw_joy_axis(23, 5, i); // Throttle
	}


	//------------ Read pitch_time -----------
	if ( !Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_pitch_forward, Controls.pitch_time, PlayerCfg.KeyboardSens[1], speed_factor, 2);
		adjust_ramped_keyboard_field(minus, key_pitch_backward, Controls.pitch_time, PlayerCfg.KeyboardSens[1], speed_factor, 2);
		// From joystick...
		adjust_axis_field(Controls.pitch_time, Controls.joy_axis, kcm_joystick[13].value, kcm_joystick[14].value, PlayerCfg.JoystickSens[1]);
		// From mouse...
		adjust_axis_field(Controls.pitch_time, Controls.mouse_axis, kcm_mouse[13].value, kcm_mouse[14].value, PlayerCfg.MouseSens[1]);
	}
	else Controls.pitch_time = 0;


	//----------- Read vertical_thrust_time -----------------
	if ( Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_pitch_forward, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
		adjust_ramped_keyboard_field(minus, key_pitch_backward, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
		// From joystick...
		// NOTE: Use Slide U/D invert setting
		adjust_axis_field(Controls.vertical_thrust_time, Controls.joy_axis, kcm_joystick[13].value, !kcm_joystick[20].value, PlayerCfg.JoystickSens[3]);
		// From mouse...
		adjust_axis_field(Controls.vertical_thrust_time, Controls.mouse_axis, kcm_mouse[13].value, kcm_mouse[20].value, PlayerCfg.MouseSens[3]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_slide_up, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
	adjust_ramped_keyboard_field(minus, key_slide_down, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
	// From buttons...
	if ( Controls.state.btn_slide_up ) Controls.vertical_thrust_time += speed_factor*FrameTime;
	if ( Controls.state.btn_slide_down ) Controls.vertical_thrust_time -= speed_factor*FrameTime;
	// From joystick...
	adjust_axis_field(Controls.vertical_thrust_time, Controls.joy_axis, kcm_joystick[19].value, !kcm_joystick[20].value, PlayerCfg.JoystickSens[3]);
	// From mouse...
	adjust_axis_field(Controls.vertical_thrust_time, Controls.mouse_axis, kcm_mouse[19].value, !kcm_mouse[20].value, PlayerCfg.MouseSens[3]);

	//---------- Read heading_time -----------
	if (!Controls.state.slide_on && !Controls.state.bank_on)
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_right, Controls.heading_time, PlayerCfg.KeyboardSens[0], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_left, Controls.heading_time, PlayerCfg.KeyboardSens[0], speed_factor);
		// From joystick...
		adjust_axis_field(Controls.heading_time, Controls.joy_axis, kcm_joystick[15].value, !kcm_joystick[16].value, PlayerCfg.JoystickSens[0]);
		// From mouse...
		adjust_axis_field(Controls.heading_time, Controls.mouse_axis, kcm_mouse[15].value, !kcm_mouse[16].value, PlayerCfg.MouseSens[0]);
	}
	else Controls.heading_time = 0;

	//----------- Read sideways_thrust_time -----------------
	if ( Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_right, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_left, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
		// From joystick...
		adjust_axis_field(Controls.sideways_thrust_time, Controls.joy_axis, kcm_joystick[15].value, !kcm_joystick[18].value, PlayerCfg.JoystickSens[2]);
		// From mouse...
		adjust_axis_field(Controls.sideways_thrust_time, Controls.mouse_axis, kcm_mouse[15].value, !kcm_mouse[18].value, PlayerCfg.MouseSens[2]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_slide_right, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
	adjust_ramped_keyboard_field(minus, key_slide_left, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
	// From buttons...
	if ( Controls.state.btn_slide_left ) Controls.sideways_thrust_time -= speed_factor*FrameTime;
	if ( Controls.state.btn_slide_right ) Controls.sideways_thrust_time += speed_factor*FrameTime;
	// From joystick...
	adjust_axis_field(Controls.sideways_thrust_time, Controls.joy_axis, kcm_joystick[17].value, !kcm_joystick[18].value, PlayerCfg.JoystickSens[2]);
	// From mouse...
	adjust_axis_field(Controls.sideways_thrust_time, Controls.mouse_axis, kcm_mouse[17].value, !kcm_mouse[18].value, PlayerCfg.MouseSens[2]);

	//----------- Read bank_time -----------------
	if ( Controls.state.bank_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_left, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_right, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
		// From joystick...
		adjust_axis_field(Controls.bank_time, Controls.joy_axis, kcm_joystick[15].value, kcm_joystick[22].value, PlayerCfg.JoystickSens[4]);
		// From mouse...
		adjust_axis_field(Controls.bank_time, Controls.mouse_axis, kcm_mouse[15].value, !kcm_mouse[22].value, PlayerCfg.MouseSens[4]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_bank_left, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
	adjust_ramped_keyboard_field(minus, key_bank_right, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
	// From buttons...
	if ( Controls.state.btn_bank_left ) Controls.bank_time += speed_factor*FrameTime;
	if ( Controls.state.btn_bank_right ) Controls.bank_time -= speed_factor*FrameTime;
	// From joystick...
	adjust_axis_field(Controls.bank_time, Controls.joy_axis, kcm_joystick[21].value, kcm_joystick[22].value, PlayerCfg.JoystickSens[4]);
	// From mouse...
	adjust_axis_field(Controls.bank_time, Controls.mouse_axis, kcm_mouse[21].value, !kcm_mouse[22].value, PlayerCfg.MouseSens[4]);

	//----------- Read forward_thrust_time -------------
	// From keyboard/buttons...
	if ( Controls.state.accelerate ) Controls.forward_thrust_time += speed_factor*FrameTime;
	if ( Controls.state.reverse ) Controls.forward_thrust_time -= speed_factor*FrameTime;
	// From joystick...
	adjust_axis_field(Controls.forward_thrust_time, Controls.joy_axis, kcm_joystick[23].value, kcm_joystick[24].value, PlayerCfg.JoystickSens[5]);
	// From mouse...
	adjust_axis_field(Controls.forward_thrust_time, Controls.mouse_axis, kcm_mouse[23].value, kcm_mouse[24].value, PlayerCfg.MouseSens[5]);

	//----------- Read cruise-control-type of throttle.
	if ( Controls.state.cruise_plus ) Cruise_speed += speed_factor*FrameTime*80;
	if ( Controls.state.cruise_minus ) Cruise_speed -= speed_factor*FrameTime*80;
	if ( Controls.state.cruise_off > 0 ) Controls.state.cruise_off = Cruise_speed = 0;
	clamp_value(Cruise_speed, 0, i2f(100));
	if (Controls.forward_thrust_time==0) Controls.forward_thrust_time = fixmul(Cruise_speed,FrameTime)/100;

	//----------- Clamp values between -FrameTime and FrameTime
	clamp_symmetric_value(Controls.pitch_time, FrameTime/2);
	clamp_symmetric_value(Controls.heading_time, FrameTime);
	clamp_symmetric_value(Controls.vertical_thrust_time, FrameTime);
	clamp_symmetric_value(Controls.sideways_thrust_time, FrameTime);
	clamp_symmetric_value(Controls.bank_time, FrameTime);
	clamp_symmetric_value(Controls.forward_thrust_time, FrameTime);
}

void reset_cruise(void)
{
	Cruise_speed=0;
}


void kc_set_controls()
{
	for (unsigned i=0; i < lengthof(kc_keyboard); i++ )
		kcm_keyboard[i].value = PlayerCfg.KeySettings[0][i];

	for (unsigned i=0; i < lengthof(kc_joystick); i++ )
	{
		kcm_joystick[i].value = PlayerCfg.KeySettings[1][i];
		if (kc_joystick[i].type == BT_INVERT )
		{
			if (kcm_joystick[i].value!=1)
				kcm_joystick[i].value = 0;
			PlayerCfg.KeySettings[1][i] = kcm_joystick[i].value;
		}
	}

	for (unsigned i=0; i < lengthof(kc_mouse); i++ )
	{
		kcm_mouse[i].value = PlayerCfg.KeySettings[2][i];
		if (kc_mouse[i].type == BT_INVERT )
		{
			if (kcm_mouse[i].value!=1)
				kcm_mouse[i].value = 0;
			PlayerCfg.KeySettings[2][i] = kcm_mouse[i].value;
		}
	}

	for (unsigned i=0; i < lengthof(kc_rebirth); i++ )
		kcm_rebirth[i].value = PlayerCfg.KeySettingsRebirth[i];
}
