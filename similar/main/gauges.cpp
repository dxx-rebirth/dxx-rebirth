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
 * Inferno gauge drivers
 *
 */

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hudmsg.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "dxxerror.h"
#include "menu.h"			// For the font.
#include "collide.h"
#include "object.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "bm.h"
#include "text.h"
#include "powerup.h"
#include "sounds.h"
#include "multi.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "controls.h"
#include "text.h"
#include "render.h"
#include "piggy.h"
#include "laser.h"
#include "common/3d/globvars.h"
#include "playsave.h"
#include "rle.h"
#include "byteutil.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-exchange.h"

using std::min;

//bitmap numbers for gauges
#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%,90%...0%)
#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES		10
#define GAUGE_ENERGY_LEFT		21
#define GAUGE_ENERGY_RIGHT		22
#define GAUGE_NUMERICAL			23
#define GAUGE_BLUE_KEY			24
#define GAUGE_GOLD_KEY			25
#define GAUGE_RED_KEY			26
#define GAUGE_BLUE_KEY_OFF		27
#define GAUGE_GOLD_KEY_OFF		28
#define GAUGE_RED_KEY_OFF		29
#define SB_GAUGE_BLUE_KEY		30
#define SB_GAUGE_GOLD_KEY		31
#define SB_GAUGE_RED_KEY		32
#define SB_GAUGE_BLUE_KEY_OFF		33
#define SB_GAUGE_GOLD_KEY_OFF		34
#define SB_GAUGE_RED_KEY_OFF		35
#define SB_GAUGE_ENERGY			36
#define GAUGE_LIVES			37
#define GAUGE_SHIPS			38
#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define GAUGE_HOMING_WARNING_ON	56
#define GAUGE_HOMING_WARNING_OFF	57
#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED			70

//Coordinats for gauges
#if defined(DXX_BUILD_DESCENT_I)
#define GAUGE_BLUE_KEY_X_L		45
#define GAUGE_BLUE_KEY_X_H		91
#define GAUGE_GOLD_KEY_X_L		44
#define GAUGE_GOLD_KEY_X_H		89
#define GAUGE_RED_KEY_X_L		43
#define GAUGE_RED_KEY_X_H		87
#define GAUGE_RED_KEY_Y_H		417
#define LEFT_ENERGY_GAUGE_X_H 	137
#define RIGHT_ENERGY_GAUGE_X 	(HIRESMODE?380:190)
#elif defined(DXX_BUILD_DESCENT_II)
#define GAUGE_AFTERBURNER		20
#define SB_GAUGE_AFTERBURNER		71
#define FLAG_ICON_RED			72
#define FLAG_ICON_BLUE			73
#define GAUGE_BLUE_KEY_X_L		272
#define GAUGE_BLUE_KEY_X_H		535
#define GAUGE_GOLD_KEY_X_L		273
#define GAUGE_GOLD_KEY_X_H		537
#define GAUGE_RED_KEY_X_L		274
#define GAUGE_RED_KEY_X_H		539
#define GAUGE_RED_KEY_Y_H		416
#define LEFT_ENERGY_GAUGE_X_H 	138
#define RIGHT_ENERGY_GAUGE_X 	(HIRESMODE?379:190)
#endif

#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		(HIRESMODE?GAUGE_BLUE_KEY_X_H:GAUGE_BLUE_KEY_X_L)
#define GAUGE_BLUE_KEY_Y		(HIRESMODE?GAUGE_BLUE_KEY_Y_H:GAUGE_BLUE_KEY_Y_L)
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		(HIRESMODE?GAUGE_GOLD_KEY_X_H:GAUGE_GOLD_KEY_X_L)
#define GAUGE_GOLD_KEY_Y		(HIRESMODE?GAUGE_GOLD_KEY_Y_H:GAUGE_GOLD_KEY_Y_L)
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X			(HIRESMODE?GAUGE_RED_KEY_X_H:GAUGE_RED_KEY_X_L)
#define GAUGE_RED_KEY_Y			(HIRESMODE?GAUGE_RED_KEY_Y_H:GAUGE_RED_KEY_Y_L)
#define SB_GAUGE_KEYS_X_L		11
#define SB_GAUGE_KEYS_X_H		26
#define SB_GAUGE_KEYS_X			(HIRESMODE?SB_GAUGE_KEYS_X_H:SB_GAUGE_KEYS_X_L)
#define SB_GAUGE_BLUE_KEY_Y_L		153
#define SB_GAUGE_GOLD_KEY_Y_L		169
#define SB_GAUGE_RED_KEY_Y_L		185
#define SB_GAUGE_BLUE_KEY_Y_H		390
#define SB_GAUGE_GOLD_KEY_Y_H		422
#define SB_GAUGE_RED_KEY_Y_H		454
#define SB_GAUGE_BLUE_KEY_Y		(HIRESMODE?SB_GAUGE_BLUE_KEY_Y_H:SB_GAUGE_BLUE_KEY_Y_L)
#define SB_GAUGE_GOLD_KEY_Y		(HIRESMODE?SB_GAUGE_GOLD_KEY_Y_H:SB_GAUGE_GOLD_KEY_Y_L)
#define SB_GAUGE_RED_KEY_Y		(HIRESMODE?SB_GAUGE_RED_KEY_Y_H:SB_GAUGE_RED_KEY_Y_L)
#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21
#define LEFT_ENERGY_GAUGE_X 	(HIRESMODE?LEFT_ENERGY_GAUGE_X_H:LEFT_ENERGY_GAUGE_X_L)
#define LEFT_ENERGY_GAUGE_Y 	(HIRESMODE?LEFT_ENERGY_GAUGE_Y_H:LEFT_ENERGY_GAUGE_Y_L)
#define LEFT_ENERGY_GAUGE_W 	(HIRESMODE?LEFT_ENERGY_GAUGE_W_H:LEFT_ENERGY_GAUGE_W_L)
#define LEFT_ENERGY_GAUGE_H 	(HIRESMODE?LEFT_ENERGY_GAUGE_H_H:LEFT_ENERGY_GAUGE_H_L)
#define RIGHT_ENERGY_GAUGE_Y 	(HIRESMODE?314:131)
#define RIGHT_ENERGY_GAUGE_W 	(HIRESMODE?133:64)
#define RIGHT_ENERGY_GAUGE_H 	(HIRESMODE?21:8)

#if defined(DXX_BUILD_DESCENT_I)
#define SB_ENERGY_GAUGE_Y 		(HIRESMODE?390:155)
#define SB_ENERGY_GAUGE_H 		(HIRESMODE?82:41)
#define SB_ENERGY_NUM_Y 		(HIRESMODE?457:190)
#define SHIELD_GAUGE_Y			(HIRESMODE?374:155)
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-(HIRESMODE?16:7))			//156 -- MWA used to be hard coded to 156
#define NUMERICAL_GAUGE_Y		(HIRESMODE?316:130)
#define PRIMARY_W_PIC_X			(HIRESMODE?135:64)
#define SECONDARY_W_PIC_X		(HIRESMODE?405:234)
#define SECONDARY_W_PIC_Y		(HIRESMODE?370:154)
#define SECONDARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?462:207)
#define SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define SECONDARY_AMMO_X		HUD_SCALE_X(HIRESMODE?475:213)
#define SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?425:171)
#define SB_LIVES_X			(HIRESMODE?550:266)
#define SB_LIVES_Y			(HIRESMODE?450:185)
#define SB_SCORE_RIGHT_H		605
#define BOMB_COUNT_X			(HIRESMODE?468:210)
#define PRIMARY_W_BOX_RIGHT_H		241
#define SECONDARY_W_BOX_RIGHT_L		264	//(SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_LEFT_H		403
#define SECONDARY_W_BOX_TOP_H		364
#define SECONDARY_W_BOX_RIGHT_H		531
#define SB_PRIMARY_W_BOX_TOP_L		154
#define SB_PRIMARY_W_BOX_BOT_L		(195)
#define SB_SECONDARY_W_BOX_TOP_L	154
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54)
#define SB_SECONDARY_W_BOX_BOT_L	(153+42)
#define SB_SECONDARY_AMMO_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+(HIRESMODE?(14):11))	//(212+9)
#define GET_GAUGE_INDEX(x)		(Gauges[x].index)

#elif defined(DXX_BUILD_DESCENT_II)
#define AFTERBURNER_GAUGE_X_L	45-1
#define AFTERBURNER_GAUGE_Y_L	158
#define AFTERBURNER_GAUGE_H_L	32
#define AFTERBURNER_GAUGE_X_H	88
#define AFTERBURNER_GAUGE_Y_H	378
#define AFTERBURNER_GAUGE_H_H	65
#define AFTERBURNER_GAUGE_X	(HIRESMODE?AFTERBURNER_GAUGE_X_H:AFTERBURNER_GAUGE_X_L)
#define AFTERBURNER_GAUGE_Y	(HIRESMODE?AFTERBURNER_GAUGE_Y_H:AFTERBURNER_GAUGE_Y_L)
#define AFTERBURNER_GAUGE_H	(HIRESMODE?AFTERBURNER_GAUGE_H_H:AFTERBURNER_GAUGE_H_L)
#define SB_AFTERBURNER_GAUGE_X 		(HIRESMODE?196:98)
#define SB_AFTERBURNER_GAUGE_Y 		(HIRESMODE?445:184)
#define SB_AFTERBURNER_GAUGE_W 		(HIRESMODE?33:16)
#define SB_AFTERBURNER_GAUGE_H 		(HIRESMODE?29:13)
#define SB_ENERGY_GAUGE_Y 		(HIRESMODE?381:(155-2))
#define SB_ENERGY_GAUGE_H 		(HIRESMODE?60:29)
#define SB_ENERGY_NUM_Y 		(HIRESMODE?457:175)
#define SHIELD_GAUGE_Y			(HIRESMODE?375:155)
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-(HIRESMODE?16:8))			//156 -- MWA used to be hard coded to 156
#define NUMERICAL_GAUGE_Y		(HIRESMODE?314:130)
#define PRIMARY_W_PIC_X			(HIRESMODE?(135-10):64)
#define SECONDARY_W_PIC_X		(HIRESMODE?466:234)
#define SECONDARY_W_PIC_Y		(HIRESMODE?374:154)
#define SECONDARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?413:207)
#define SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?378:157)
#define SECONDARY_AMMO_X		HUD_SCALE_X(HIRESMODE?428:213)
#define SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?407:171)
#define SB_LIVES_X			(HIRESMODE?(550-10-3):266)
#define SB_LIVES_Y			(HIRESMODE?450-3:185)
#define SB_SCORE_RIGHT_H		(605+8)
#define BOMB_COUNT_X			(HIRESMODE?546:275)
#define PRIMARY_W_BOX_RIGHT_H		242
#define SECONDARY_W_BOX_RIGHT_L		263	//(SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_LEFT_H		404
#define SECONDARY_W_BOX_TOP_H		363
#define SECONDARY_W_BOX_RIGHT_H		529
#define SB_PRIMARY_W_BOX_TOP_L		153
#define SB_PRIMARY_W_BOX_BOT_L		(195+1)
#define SB_SECONDARY_W_BOX_TOP_L	153
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54+1)
#define SB_SECONDARY_W_BOX_BOT_L	(SB_SECONDARY_W_BOX_TOP_L+43)
#define SB_SECONDARY_AMMO_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+(HIRESMODE?(14-4):11))	//(212+9)
#define GET_GAUGE_INDEX(x)		((HIRESMODE)?Gauges_hires[x].index:Gauges[x].index)

#endif

// MOVEME
#define HOMING_WARNING_X		(HIRESMODE?13:7)
#define HOMING_WARNING_Y		(HIRESMODE?416:171)

#define SB_ENERGY_GAUGE_X 		(HIRESMODE?196:98)
#define SB_ENERGY_GAUGE_W 		(HIRESMODE?32:16)
#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SHIP_GAUGE_Y			(SHIELD_GAUGE_Y+(HIRESMODE?10:5))
#define SHIELD_GAUGE_X 			(HIRESMODE?292:146)
#define SB_SHIELD_GAUGE_X 		(HIRESMODE?247:123)		//139
#define SB_SHIELD_GAUGE_Y 		(HIRESMODE?395:163)
#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+(HIRESMODE?10:5))
#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?21:12))	//151
#define NUMERICAL_GAUGE_X		(HIRESMODE?308:154)
#define PRIMARY_W_PIC_Y			(HIRESMODE?370:154)
#define PRIMARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?182:87)
#define PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define PRIMARY_AMMO_X			HUD_SCALE_X(HIRESMODE?186:93)
#define PRIMARY_AMMO_Y			HUD_SCALE_Y(HIRESMODE?420:171)
#define SB_LIVES_LABEL_X		(HIRESMODE?475:237)
#define SB_SCORE_RIGHT_L		301
#define SB_SCORE_RIGHT			(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)
#define SB_SCORE_Y			(HIRESMODE?398:158)
#define SB_SCORE_LABEL_X		(HIRESMODE?475:237)
#define SB_SCORE_ADDED_RIGHT		(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)
#define SB_SCORE_ADDED_Y		(HIRESMODE?413:165)
#define BOMB_COUNT_Y			(HIRESMODE?445:186)
#define SB_BOMB_COUNT_X			(HIRESMODE?342:171)
#define SB_BOMB_COUNT_Y			(HIRESMODE?458:191)

// defining box boundries for weapon pictures
#define PRIMARY_W_BOX_LEFT_L		63
#define PRIMARY_W_BOX_TOP_L		151		//154
#define PRIMARY_W_BOX_RIGHT_L		(PRIMARY_W_BOX_LEFT_L+58)
#define PRIMARY_W_BOX_BOT_L		(PRIMARY_W_BOX_TOP_L+42)
#define PRIMARY_W_BOX_LEFT_H		121
#define PRIMARY_W_BOX_TOP_H		364
#define PRIMARY_W_BOX_BOT_H		(PRIMARY_W_BOX_TOP_H+106)		//470
#define PRIMARY_W_BOX_LEFT		(HIRESMODE?PRIMARY_W_BOX_LEFT_H:PRIMARY_W_BOX_LEFT_L)
#define PRIMARY_W_BOX_TOP		(HIRESMODE?PRIMARY_W_BOX_TOP_H:PRIMARY_W_BOX_TOP_L)
#define PRIMARY_W_BOX_RIGHT		(HIRESMODE?PRIMARY_W_BOX_RIGHT_H:PRIMARY_W_BOX_RIGHT_L)
#define PRIMARY_W_BOX_BOT		(HIRESMODE?PRIMARY_W_BOX_BOT_H:PRIMARY_W_BOX_BOT_L)
#define SECONDARY_W_BOX_LEFT_L		202	//207
#define SECONDARY_W_BOX_TOP_L		151
#define SECONDARY_W_BOX_BOT_L		(SECONDARY_W_BOX_TOP_L+42)
#define SECONDARY_W_BOX_BOT_H		(SECONDARY_W_BOX_TOP_H+106)		//470
#define SECONDARY_W_BOX_LEFT		(HIRESMODE?SECONDARY_W_BOX_LEFT_H:SECONDARY_W_BOX_LEFT_L)
#define SECONDARY_W_BOX_TOP		(HIRESMODE?SECONDARY_W_BOX_TOP_H:SECONDARY_W_BOX_TOP_L)
#define SECONDARY_W_BOX_RIGHT		(HIRESMODE?SECONDARY_W_BOX_RIGHT_H:SECONDARY_W_BOX_RIGHT_L)
#define SECONDARY_W_BOX_BOT		(HIRESMODE?SECONDARY_W_BOX_BOT_H:SECONDARY_W_BOX_BOT_L)
#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_RIGHT_L	(SB_PRIMARY_W_BOX_LEFT_L+55)
#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H	179
#define SB_PRIMARY_W_BOX_BOT_H		473
#define SB_PRIMARY_W_BOX_LEFT		(HIRESMODE?SB_PRIMARY_W_BOX_LEFT_H:SB_PRIMARY_W_BOX_LEFT_L)
#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H	381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H	473
#define SB_SECONDARY_W_BOX_LEFT		(HIRESMODE?SB_SECONDARY_W_BOX_LEFT_H:SB_SECONDARY_W_BOX_LEFT_L)	//210
#define SB_PRIMARY_W_PIC_X		(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_PRIMARY_W_TEXT_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?50:24))	//(51+23)
#define SB_PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?390:157)
#define SB_PRIMARY_AMMO_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?58:30))	//((SB_PRIMARY_W_BOX_LEFT+33)-3)	//(51+32)
#define SB_PRIMARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?410:171)
#define SB_SECONDARY_W_PIC_X		(HIRESMODE?385:(SB_SECONDARY_W_BOX_LEFT+27))	//(212+27)
#define SB_SECONDARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_SECONDARY_W_TEXT_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+2)	//212
#define SB_SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?390:157)
#define SB_SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?414:171)

#define WS_SET				0		//in correct state
#define WS_FADING_OUT			1
#define WS_FADING_IN			2
#define FADE_SCALE			(2*i2f(GR_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)

#define COCKPIT_PRIMARY_BOX		(!HIRESMODE?0:4)
#define COCKPIT_SECONDARY_BOX		(!HIRESMODE?1:5)
#define SB_PRIMARY_BOX			(!HIRESMODE?2:6)
#define SB_SECONDARY_BOX		(!HIRESMODE?3:7)

// scaling gauges
#define BASE_WIDTH (HIRESMODE? 640 : 320)
#define BASE_HEIGHT	(HIRESMODE? 480 : 200)
#ifdef OGL
#define HUD_SCALE_X(x)		static_cast<int>(static_cast<double>(x) * (static_cast<double>(grd_curscreen->get_screen_width()) / BASE_WIDTH) + 0.5)
#define HUD_SCALE_Y(y)		static_cast<int>(static_cast<double>(y) * (static_cast<double>(grd_curscreen->get_screen_height()) / BASE_HEIGHT) + 0.5)
#define HUD_SCALE_X_AR(x)	(HUD_SCALE_X(100) > HUD_SCALE_Y(100) ? HUD_SCALE_Y(x) : HUD_SCALE_X(x))
#define HUD_SCALE_Y_AR(y)	(HUD_SCALE_Y(100) > HUD_SCALE_X(100) ? HUD_SCALE_X(y) : HUD_SCALE_Y(y))
#else
#define HUD_SCALE_X(x)		(x)
#define HUD_SCALE_Y(y)		(y)
#define HUD_SCALE_X_AR(x)	(x)
#define HUD_SCALE_Y_AR(y)	(y)
#endif

#if defined(DXX_BUILD_DESCENT_I)
array<bitmap_index, MAX_GAUGE_BMS_MAC> Gauges; // Array of all gauge bitmaps.
#elif defined(DXX_BUILD_DESCENT_II)
array<bitmap_index, MAX_GAUGE_BMS> Gauges,   // Array of all gauge bitmaps.
	Gauges_hires;   // hires gauges
static array<int, 2> weapon_box_user{{WBU_WEAPON, WBU_WEAPON}};		//see WBU_ constants in gauges.h
#endif
grs_bitmap deccpt;
static array<grs_subbitmap_ptr, 2> WinBoxOverlay; // Overlay subbitmaps for both weapon boxes

#define PAGE_IN_GAUGE(x) _page_in_gauge(x)
static inline void _page_in_gauge(int x)
{
#if defined(DXX_BUILD_DESCENT_II)
    if (HIRESMODE)
        PIGGY_PAGE_IN(Gauges_hires[x]);
	else
#endif
		PIGGY_PAGE_IN(Gauges[x]);
}

static void draw_ammo_info(int x,int y,int ammo_count);

static int score_display;
static fix score_time;
static array<int, 2> old_weapon{{-1, -1}};
static int old_laser_level		= -1;
static int invulnerable_frame = 0;
static array<int, 2> weapon_box_states;
static_assert(WS_SET == 0, "weapon_box_states must start at zero");
static array<fix, 2> weapon_box_fade_values;
int	Color_0_31_0 = -1;

struct gauge_box
{
	int left,top;
	int right,bot;		//maximal box
};

const gauge_box gauge_boxes[] = {

// primary left/right low res
		{PRIMARY_W_BOX_LEFT_L,PRIMARY_W_BOX_TOP_L,PRIMARY_W_BOX_RIGHT_L,PRIMARY_W_BOX_BOT_L},
		{SECONDARY_W_BOX_LEFT_L,SECONDARY_W_BOX_TOP_L,SECONDARY_W_BOX_RIGHT_L,SECONDARY_W_BOX_BOT_L},

//sb left/right low res
		{SB_PRIMARY_W_BOX_LEFT_L,SB_PRIMARY_W_BOX_TOP_L,SB_PRIMARY_W_BOX_RIGHT_L,SB_PRIMARY_W_BOX_BOT_L},
		{SB_SECONDARY_W_BOX_LEFT_L,SB_SECONDARY_W_BOX_TOP_L,SB_SECONDARY_W_BOX_RIGHT_L,SB_SECONDARY_W_BOX_BOT_L},

// primary left/right hires
		{PRIMARY_W_BOX_LEFT_H,PRIMARY_W_BOX_TOP_H,PRIMARY_W_BOX_RIGHT_H,PRIMARY_W_BOX_BOT_H},
		{SECONDARY_W_BOX_LEFT_H,SECONDARY_W_BOX_TOP_H,SECONDARY_W_BOX_RIGHT_H,SECONDARY_W_BOX_BOT_H},

// sb left/right hires
		{SB_PRIMARY_W_BOX_LEFT_H,SB_PRIMARY_W_BOX_TOP_H,SB_PRIMARY_W_BOX_RIGHT_H,SB_PRIMARY_W_BOX_BOT_H},
		{SB_SECONDARY_W_BOX_LEFT_H,SB_SECONDARY_W_BOX_TOP_H,SB_SECONDARY_W_BOX_RIGHT_H,SB_SECONDARY_W_BOX_BOT_H},
	};

struct span
{
	unsigned l, r;
};

//store delta x values from left of box
const span weapon_window_left[] = {
	{71,114},
	{69,116},
	{68,117},
	{66,118},
	{66,119},
	{66,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{65,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{64,119},
	{63,119},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,118},
	{63,117},
	{63,117},
	{64,116},
	{65,115},
	{66,113},
	{68,111},
};


//store delta x values from left of box
const span weapon_window_right[] = {
	{208,255},
	{206,257},
	{205,258},
	{204,259},
	{203,260},
	{203,260},
	{203,260},
	{203,260},
	{203,260},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,261},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{203,262},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{204,263},
	{205,263},
	{206,262},
	{207,261},
	{208,260},
	{211,255},
};

//store delta x values from left of box
const span weapon_window_left_hires[] = {
	{141,231},
	{139,234},
	{137,235},
	{136,237},
	{135,238},
	{134,239},
	{133,240},
	{132,240},
	{131,241},
	{131,241},
	{130,242},
	{129,242},
	{129,242},
	{129,243},
	{128,243},
	{128,243},
	{128,243},
	{128,243},
	{128,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{127,243},
	{126,243},
	{126,243},
	{126,243},
	{126,243},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{126,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{125,242},
	{124,242},
	{124,242},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{124,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{123,241},
	{122,241},
	{122,241},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{122,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,240},
	{121,239},
	{121,239},
	{121,239},
	{121,238},
	{121,238},
	{121,238},
	{122,237},
	{122,237},
	{123,236},
	{123,235},
	{124,234},
	{125,233},
	{126,232},
	{126,231},
	{128,230},
	{130,228},
	{131,226},
	{133,223},
};


//store delta x values from left of box
const span weapon_window_right_hires[] = {
	{416,509},
	{413,511},
	{412,513},
	{410,514},
	{409,515},
	{408,516},
	{407,517},
	{407,518},
	{406,519},
	{406,519},
	{405,520},
	{405,521},
	{405,521},
	{404,521},
	{404,522},
	{404,522},
	{404,522},
	{404,522},
	{404,522},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,523},
	{404,524},
	{404,524},
	{404,524},
	{404,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,524},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,525},
	{405,526},
	{405,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,526},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,527},
	{406,528},
	{406,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,528},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{407,529},
	{408,529},
	{408,529},
	{408,529},
	{409,529},
	{409,529},
	{409,529},
	{410,529},
	{410,528},
	{411,527},
	{412,527},
	{413,526},
	{414,525},
	{415,524},
	{416,524},
	{417,522},
	{419,521},
	{422,519},
	{424,518},
};

static inline void hud_bitblt_free (int x, int y, int w, int h, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,w,h,*bm,-1,F1_0);
#else
	gr_ubitmapm(x, y, *bm);
#endif
}

static inline void hud_bitblt (int x, int y, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,HUD_SCALE_X (bm->bm_w),HUD_SCALE_Y (bm->bm_h),*bm,-1,F1_0);
#else
	gr_ubitmapm(x, y, *bm);
#endif
}

static void hud_show_score()
{
	char	score_str[20];
	int	w, h, aw;

	if (HUD_toolong)
		return;

	gr_set_curfont( GAME_FONT );

	if ( ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) ) {
		sprintf(score_str, "%s: %5d", TXT_KILLS, Players[Player_num].net_kills_total);
	} else {
		sprintf(score_str, "%s: %5d", TXT_SCORE, Players[Player_num].score);
  	}

	gr_get_string_size(score_str, &w, &h, &aw );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1);

	gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), FSPACY(1), score_str);
}

static void hud_show_timer_count()
{
	char	score_str[20];
	int	w, h, aw,i;
	fix timevar=0;

	if (HUD_toolong)
		return;

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed)
	{
		timevar=i2f (Netgame.PlayTimeAllowed*5*60);
		i=f2i(timevar-ThisLevelTime);
		i++;

		sprintf(score_str, "T - %5d", i);
		gr_get_string_size(score_str, &w, &h, &aw );

		if (Color_0_31_0 == -1)
			Color_0_31_0 = BM_XRGB(0,31,0);

		gr_set_fontcolor(Color_0_31_0, -1);

		if (i>-1 && !Control_center_destroyed)
			gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(12), LINE_SPACING+FSPACY(1), score_str);
	}
}

static void hud_show_score_added()
{
	int	color;
	int	w, h, aw;
	char	score_str[20];

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		color = f2i(score_time * 20) + 12;

		if (color < 10) color = 12;
		if (color > 31) color = 30;

		color = color - (color % 4);

		if (cheats.enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(12), LINE_SPACING+FSPACY(1), score_str);
	} else {
		score_time = 0;
		score_display = 0;
	}
}

static void sb_show_score()
{
	char	score_str[20];
	int x,y;
	int	w, h, aw;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,20,0),-1 );

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		gr_printf(HUD_SCALE_X(SB_SCORE_LABEL_X),HUD_SCALE_Y(SB_SCORE_Y),"%s:", TXT_KILLS);
	else
		gr_printf(HUD_SCALE_X(SB_SCORE_LABEL_X),HUD_SCALE_Y(SB_SCORE_Y),"%s:", TXT_SCORE);

	gr_set_curfont( GAME_FONT );
	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		sprintf(score_str, "%5d", Players[Player_num].net_kills_total);
	else
		sprintf(score_str, "%5d", Players[Player_num].score);
	gr_get_string_size(score_str, &w, &h, &aw );

	x = HUD_SCALE_X(SB_SCORE_RIGHT)-w-FSPACX(1);
	y = HUD_SCALE_Y(SB_SCORE_Y);

	//erase old score
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(x,y,HUD_SCALE_X(SB_SCORE_RIGHT),y+LINE_SPACING);

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
	else
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	gr_string(x,y,score_str);
}

static void sb_show_score_added()
{
	int	color;
	int w, h, aw;
	char	score_str[32];
	static int x;
	static	int last_score_display = -1;

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		return;

	if (score_display == 0)
		return;

	gr_set_curfont( GAME_FONT );

	score_time -= FrameTime;
	if (score_time > 0) {
		if (score_display != last_score_display)
			last_score_display = score_display;

		color = f2i(score_time * 20) + 10;

		if (color < 10) color = 10;
		if (color > 31) color = 31;

		if (cheats.enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		x = HUD_SCALE_X(SB_SCORE_ADDED_RIGHT)-w-FSPACX(1);
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_string(x, HUD_SCALE_Y(SB_SCORE_ADDED_Y), score_str);
	} else {
		//erase old score
		gr_setcolor(BM_XRGB(0,0,0));
		gr_rect(x,HUD_SCALE_Y(SB_SCORE_ADDED_Y),HUD_SCALE_X(SB_SCORE_ADDED_RIGHT),HUD_SCALE_Y(SB_SCORE_ADDED_Y)+LINE_SPACING);
		score_time = 0;
		score_display = 0;
	}
}

//	-----------------------------------------------------------------------------
void play_homing_warning(void)
{
	fix beep_delay;
	static fix64 Last_warning_beep_time = 0; // Time we last played homing missile warning beep.

	if (Endlevel_sequence || Player_is_dead)
		return;

	if (Players[Player_num].homing_object_dist >= 0) {
		beep_delay = Players[Player_num].homing_object_dist/128;
		if (beep_delay > F1_0)
			beep_delay = F1_0;
		else if (beep_delay < F1_0/8)
			beep_delay = F1_0/8;

		if (GameTime64 - Last_warning_beep_time > beep_delay/2 || Last_warning_beep_time > GameTime64) {
			digi_play_sample( SOUND_HOMING_WARNING, F1_0 );
			Last_warning_beep_time = GameTime64;
		}
	}
}

//	-----------------------------------------------------------------------------
static void show_homing_warning(void)
{
	if (Endlevel_sequence)
	{
		PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
		hud_bitblt( HUD_SCALE_X(HOMING_WARNING_X), HUD_SCALE_Y(HOMING_WARNING_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF) ]);
		return;
	}

	gr_set_current_canvas( NULL );

	if (Players[Player_num].homing_object_dist >= 0)
	{
		if (GameTime64 & 0x4000)
		{
			PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_ON );
			hud_bitblt( HUD_SCALE_X(HOMING_WARNING_X), HUD_SCALE_Y(HOMING_WARNING_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_ON) ]);
		}
		else
		{
			PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
			hud_bitblt( HUD_SCALE_X(HOMING_WARNING_X), HUD_SCALE_Y(HOMING_WARNING_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF) ]);
		}
	}
	else
	{
		PAGE_IN_GAUGE( GAUGE_HOMING_WARNING_OFF );
		hud_bitblt( HUD_SCALE_X(HOMING_WARNING_X), HUD_SCALE_Y(HOMING_WARNING_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_HOMING_WARNING_OFF) ]);
	}
}

static void hud_show_homing_warning(void)
{
	if (Players[Player_num].homing_object_dist >= 0) {
		if (GameTime64 & 0x4000) {
			gr_set_curfont( GAME_FONT );
			gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
			gr_string(0x8000, grd_curcanv->cv_bitmap.bm_h-LINE_SPACING,TXT_LOCK);
		}
	}
}

static void hud_show_keys(void)
{
	grs_bitmap *blue,*yellow,*red;
	int y=HUD_SCALE_Y_AR(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h+2)+FSPACY(1);

	PAGE_IN_GAUGE( KEY_ICON_BLUE );
	PAGE_IN_GAUGE( KEY_ICON_YELLOW );
	PAGE_IN_GAUGE( KEY_ICON_RED );

	blue=&GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_BLUE) ];
	yellow=&GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_YELLOW) ];
	red=&GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_RED) ];

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY)
		hud_bitblt_free(FSPACX(2),y,HUD_SCALE_X_AR(blue->bm_w),HUD_SCALE_Y_AR(blue->bm_h),blue);

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+3),y,HUD_SCALE_X_AR(yellow->bm_w),HUD_SCALE_Y_AR(yellow->bm_h),yellow);

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+yellow->bm_w+6),y,HUD_SCALE_X_AR(red->bm_w),HUD_SCALE_Y_AR(red->bm_h),red);

}

#if defined(DXX_BUILD_DESCENT_II)
static void hud_show_orbs (void)
{
	if (game_mode_hoard()) {
		int x=0,y=LINE_SPACING+FSPACY(1);
		grs_bitmap *bm;

		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
			x = (SWIDTH/18);
		}
		else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
			x = FSPACX(2);
		}
		else if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
			y=HUD_SCALE_Y_AR(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h+GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_RED) ].bm_h+4)+FSPACY(1);
			x = FSPACX(2);
		}
		else
			Int3();		//what sort of cockpit?

		bm = &Orb_icons[HIRESMODE];

		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

		hud_bitblt_free(x,y,HUD_SCALE_Y_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(x+HUD_SCALE_X_AR(bm->bm_w), y, " x %d", Players[Player_num].secondary_ammo[PROXIMITY_INDEX]);
	}
}

static void hud_show_flag(void)
{
	if (game_mode_capture_flag() && (Players[Player_num].flags & PLAYER_FLAGS_FLAG)) {
		int x=0,y=0,icon;
		grs_bitmap *bm;

		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
			y=HUD_SCALE_Y_AR(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h+2)+FSPACY(1);
			x = (SWIDTH/10);
		}
		else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
			y=HUD_SCALE_Y_AR(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h+2)+FSPACY(1);
			x = FSPACX(2);
		}
		else if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
			y=HUD_SCALE_Y_AR(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h+GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_RED) ].bm_h+4)+FSPACY(1);
			x = FSPACX(2);
		}
		else
			Int3();		//what sort of cockpit?


		icon = (get_team(Player_num) == TEAM_BLUE)?FLAG_ICON_RED:FLAG_ICON_BLUE;
		bm = &GameBitmaps[ GET_GAUGE_INDEX(icon) ];

		PAGE_IN_GAUGE( icon );
		hud_bitblt_free(x,y,HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);

	}
}
#endif

static void hud_show_energy(void)
{
	if (PlayerCfg.HudMode<2) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		if (Game_mode & GM_MULTI)
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*5)),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
		else
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-LINE_SPACING),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
	}

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_energy(f2ir(Players[Player_num].energy));
}

#if defined(DXX_BUILD_DESCENT_I)
static inline void hud_show_afterburner(void)
{
}
#define convert_1s(s)
#elif defined(DXX_BUILD_DESCENT_II)
static void hud_show_afterburner(void)
{
	int y;

	if (! (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER))
		return;		//don't draw if don't have

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	y = (Game_mode & GM_MULTI)?(-7*LINE_SPACING):(-3*LINE_SPACING);

	gr_printf(FSPACX(1), grd_curcanv->cv_bitmap.bm_h+y, "burn: %d%%" , fixmul(Afterburner_charge,100));

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_afterburner(Afterburner_charge);
}

//convert '1' characters to special wide ones
#define convert_1s(s) do {char *p=s; while ((p=strchr(p,'1')) != NULL) *p=132;} while(0)
#endif

static inline const char *SECONDARY_WEAPON_NAMES_VERY_SHORT(const unsigned u)
{
	switch(u)
	{
		default:
			Int3();
		case CONCUSSION_INDEX:	return TXT_CONCUSSION;
		case HOMING_INDEX:		return TXT_HOMING;
		case PROXIMITY_INDEX:	return TXT_PROXBOMB;
		case SMART_INDEX:		return TXT_SMART;
		case MEGA_INDEX:		return TXT_MEGA;
#if defined(DXX_BUILD_DESCENT_II)
		case SMISSILE1_INDEX:	return "Flash";
		case GUIDED_INDEX:		return "Guided";
		case SMART_MINE_INDEX:	return "SmrtMine";
		case SMISSILE4_INDEX:	return "Mercury";
		case SMISSILE5_INDEX:	return "Shaker";
#endif
	}
}

static void show_bomb_count(int x,int y,int bg_color,int always_show,int right_align)
{
	int bomb,count,w=0,h=0,aw=0;
	char txt[5],*t;

#if defined(DXX_BUILD_DESCENT_I)
	if (!PlayerCfg.BombGauge)
		return;
#endif

	bomb = which_bomb();
	count = Players[Player_num].secondary_ammo[bomb];

	count = min(count,99);	//only have room for 2 digits - cheating give 200

	if (always_show && count == 0)		//no bombs, draw nothing on HUD
		return;

	if (count)
		gr_set_fontcolor((bomb==PROXIMITY_INDEX)?gr_find_closest_color(55,0,0):BM_XRGB(59,50,21),bg_color);
	else
		gr_set_fontcolor(bg_color,bg_color);	//erase by drawing in background color

	snprintf(txt, sizeof(txt), "B:%02d", count);

	while ((t=strchr(txt,'1')) != NULL)
		*t = '\x84';	//convert to wide '1'

	if (right_align)
		gr_get_string_size(txt, &w, &h, &aw );

	gr_string(x-w,y,txt);
}

static void draw_primary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		draw_ammo_info(SB_PRIMARY_AMMO_X,SB_PRIMARY_AMMO_Y,ammo_count);
	else
		draw_ammo_info(PRIMARY_AMMO_X,PRIMARY_AMMO_Y,ammo_count);
}

static void hud_set_fontcolor_red()
{
	gr_set_fontcolor(BM_XRGB(20,0,0),-1);
}

static void hud_set_fontcolor_yellow()
{
	gr_set_fontcolor(BM_XRGB(15,15,0),-1);
}

static void hud_set_fontcolor_green()
{
	gr_set_fontcolor(BM_XRGB(0,15,0),-1);
}

static void hud_set_fontcolor_dimgreen()
{
	gr_set_fontcolor(BM_XRGB(0,6,0),-1);
}

static void hud_set_fontcolor_gray()
{
	gr_set_fontcolor(BM_XRGB(3,3,3),-1);
}

static void hud_set_primary_weapon_fontcolor(const int consider_weapon)
{
	if (Primary_weapon==consider_weapon)
		hud_set_fontcolor_red();
	else{
		if (player_has_primary_weapon(consider_weapon).has_weapon())
		{
			const int is_super = (consider_weapon >= 5);
#if defined(DXX_BUILD_DESCENT_II)
			const int base_weapon = is_super ? consider_weapon - 5 : consider_weapon;
			if (Primary_last_was_super[base_weapon])
			{
				if (is_super)
					hud_set_fontcolor_green();
				else
					hud_set_fontcolor_yellow();
			}
			else
#endif
			{
				if (is_super)
					hud_set_fontcolor_yellow();
				else
					hud_set_fontcolor_green();
			}
		}
		else
			hud_set_fontcolor_gray();
	}
}

static void hud_set_secondary_weapon_fontcolor(const int consider_weapon)
{
	if (Secondary_weapon==consider_weapon)
		hud_set_fontcolor_red();
	else{
		if (Players[Player_num].secondary_ammo[consider_weapon]>0)
		{
			const int is_super = (consider_weapon >= 5);
#if defined(DXX_BUILD_DESCENT_II)
			const int base_weapon = is_super ? consider_weapon - 5 : consider_weapon;
			if (Secondary_last_was_super[base_weapon])
			{
				if (is_super)
					hud_set_fontcolor_green();
				else
					hud_set_fontcolor_yellow();
			}
			else
#endif
			{
				if (is_super)
					hud_set_fontcolor_yellow();
				else
					hud_set_fontcolor_green();
			}
		}
		else
			hud_set_fontcolor_dimgreen();
	}
}

static void hud_printf_vulcan_ammo(const int x, const int y)
{
	if (PlayerCfg.CockpitMode[1]!=CM_FULL_SCREEN)
		return;
	const unsigned primary_weapon_flags = Players[Player_num].primary_weapon_flags;
	const auto vulcan_mask = HAS_VULCAN_FLAG;
#if defined(DXX_BUILD_DESCENT_I)
	const auto gauss_mask = vulcan_mask;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto gauss_mask = HAS_GAUSS_FLAG;
#endif
	const int fmt_vulcan_ammo = f2i((unsigned int)Players[Player_num].vulcan_ammo * VULCAN_AMMO_SCALE);
	const unsigned has_weapon_uses_vulcan_ammo = (primary_weapon_flags & (gauss_mask | vulcan_mask));
	if (!has_weapon_uses_vulcan_ammo && !fmt_vulcan_ammo)
		return;
	if (weapon_index_uses_vulcan_ammo(Primary_weapon))
		hud_set_fontcolor_red();
	else if (has_weapon_uses_vulcan_ammo)
		hud_set_fontcolor_green();
	else
		hud_set_fontcolor_gray();
	char c;
#if defined(DXX_BUILD_DESCENT_II)
	if ((primary_weapon_flags & gauss_mask) && (Primary_last_was_super[VULCAN_INDEX] || !(primary_weapon_flags & vulcan_mask)))
		c = 'G';
	else
#endif
		if (primary_weapon_flags & vulcan_mask)
		c = 'V';
	else
		c = 'A';
	gr_printf(x,y,"%c:%i", c, fmt_vulcan_ammo);
}

static void hud_show_weapons_mode(int type,int vertical,int orig_x,int orig_y){
	int i,w,h,aw,x=orig_x,y=orig_y;

	if (vertical){
		y=y+(LINE_SPACING*4);
	}

	if (type==0){
		for (i=4;i>=0;i--){
			const char *txtweapon;
			char weapon_str[10];
			hud_set_primary_weapon_fontcolor(i);
			switch(i){
				case 0:
					snprintf(weapon_str, sizeof(weapon_str), "%c%i", (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)?'Q':'L', Players[Player_num].laser_level+1);
					txtweapon = weapon_str;
					break;
				case 1:
#if defined(DXX_BUILD_DESCENT_I)
				if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
				{
					sprintf(weapon_str,"V%i", f2i(Players[Player_num].vulcan_ammo * VULCAN_AMMO_SCALE));
					txtweapon = weapon_str;
				}
				else
#endif
					txtweapon = "V";
					break;
				case 2:
					txtweapon = "S";
					break;
				case 3:
					txtweapon = "P";
					break;
				case 4:
					txtweapon = "F";
					break;
				default:
					continue;
			}
			gr_get_string_size(txtweapon, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACY(2);
			}else
				x-=w+FSPACX(3);
			gr_string(x, y, txtweapon);
			if (i == 1 && !vertical)
			{
				hud_printf_vulcan_ammo(x, y - (LINE_SPACING * 1));
			}
		}
	} else {
		for (i=4;i>=0;i--){
			char weapon_str[10];
			hud_set_secondary_weapon_fontcolor(i);
			snprintf(weapon_str,sizeof(weapon_str),"%i",Players[Player_num].secondary_ammo[i]);
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACY(2);
			}else
				x-=w+FSPACX(3);
			gr_string(x, y, weapon_str);
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	x = orig_x;
	y = orig_y;
	if (vertical)
	{
		x=x+FSPACX(15);
		y=y+(LINE_SPACING*4);
	}
	else
	{
		y=y+LINE_SPACING;
	}

	if (type==0) {
		for (i=9;i>=5;i--){
			const char *txtweapon;
			char weapon_str[10];
			hud_set_primary_weapon_fontcolor(i);
			switch(i){
				case 5:
					txtweapon = " ";
					break;
				case 6:
					txtweapon = "G";
					break;
				case 7:
					txtweapon = "H";
					break;
				case 8:
					txtweapon = "P";
					break;
				case 9:
					if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
					{
						snprintf(weapon_str, sizeof(weapon_str), "O%03i", Omega_charge * 100/MAX_OMEGA_CHARGE);
						txtweapon = weapon_str;
					}
					else
						txtweapon = "O";
					break;
				default:
					continue;
			}
			gr_get_string_size(txtweapon, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACY(2);
			}else
				x-=w+FSPACX(3);
			if (i == 5)
			{
				if (vertical)
					hud_printf_vulcan_ammo(x, y);
				continue;
			}
			gr_string(x, y, txtweapon);
		}
	} else {
		for (i=9;i>=5;i--){
			char weapon_str[10];
			hud_set_secondary_weapon_fontcolor(i);
			snprintf(weapon_str,sizeof(weapon_str),"%i",Players[Player_num].secondary_ammo[i]);
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACY(2);
			}else
				x-=w+FSPACX(3);
			gr_string(x, y, weapon_str);
		}
	}
#endif

	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
}

static void hud_show_weapons(void)
{
	int	y;
	const char	*weapon_name;
	char	weapon_str[32];

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	y = grd_curcanv->cv_bitmap.bm_h;

	if (Game_mode & GM_MULTI)
		y -= LINE_SPACING*4;

	if (PlayerCfg.HudMode==1){
#if defined(DXX_BUILD_DESCENT_I)
		unsigned multiplier = 1;
#elif defined(DXX_BUILD_DESCENT_II)
		unsigned multiplier = 2;
#endif
		hud_show_weapons_mode(0,0,grd_curcanv->cv_bitmap.bm_w,y-(LINE_SPACING*2*multiplier));
		hud_show_weapons_mode(1,0,grd_curcanv->cv_bitmap.bm_w,y-(LINE_SPACING*multiplier));
	}
	else if (PlayerCfg.HudMode==2){
		int x1,x2;
		int w, aw;
		gr_get_string_size("V1000", &w, &x1, &aw );
		gr_get_string_size("0 ", &x2, &x1, &aw);
		y=grd_curcanv->cv_bitmap.bm_h/1.75;
		x1=grd_curcanv->cv_bitmap.bm_w/2.1-(FSPACX(40)+w);
		x2=grd_curcanv->cv_bitmap.bm_w/1.9+(FSPACX(42)+x2);
		hud_show_weapons_mode(0,1,x1,y);
		hud_show_weapons_mode(1,1,x2,y);
		gr_set_fontcolor(BM_XRGB(14,14,23),-1 );
		gr_printf(x2, y-(LINE_SPACING*4),"%i", f2ir(Players[Player_num].shields));
		gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
		gr_printf(x1, y-(LINE_SPACING*4),"%i", f2ir(Players[Player_num].energy));
	}
	else
	{
		const char *disp_primary_weapon_name;
		int	w, h, aw;

		weapon_name = PRIMARY_WEAPON_NAMES_SHORT(Primary_weapon);
		switch (Primary_weapon) {
			case LASER_INDEX:
				if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
					sprintf(weapon_str, "%s %s %i", TXT_QUAD, weapon_name, Players[Player_num].laser_level+1);
				else
					sprintf(weapon_str, "%s %i", weapon_name, Players[Player_num].laser_level+1);
				disp_primary_weapon_name = weapon_str;
				break;

			case VULCAN_INDEX:
#if defined(DXX_BUILD_DESCENT_II)
			case GAUSS_INDEX:
#endif
				sprintf(weapon_str, "%s: %i", weapon_name, f2i((unsigned) Players[Player_num].vulcan_ammo * (unsigned) VULCAN_AMMO_SCALE));
				convert_1s(weapon_str);
				disp_primary_weapon_name = weapon_str;
				break;

			case SPREADFIRE_INDEX:
			case PLASMA_INDEX:
			case FUSION_INDEX:
				disp_primary_weapon_name = weapon_name;
				break;
#if defined(DXX_BUILD_DESCENT_II)
			case HELIX_INDEX:
			case PHOENIX_INDEX:
				disp_primary_weapon_name = weapon_name;
				break;
			case OMEGA_INDEX:
				sprintf(weapon_str, "%s: %03i", weapon_name, Omega_charge * 100/MAX_OMEGA_CHARGE);
				convert_1s(weapon_str);
				disp_primary_weapon_name = weapon_str;
				break;
#endif

#if defined(DXX_BUILD_DESCENT_II)
			case SUPER_LASER_INDEX:	//no such thing as super laser
#endif
			default:
				Int3();
				disp_primary_weapon_name = "";
				break;
		}

		gr_get_string_size(disp_primary_weapon_name, &w, &h, &aw );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-(LINE_SPACING*2), disp_primary_weapon_name);
		const char *disp_secondary_weapon_name;

		disp_secondary_weapon_name = SECONDARY_WEAPON_NAMES_VERY_SHORT(Secondary_weapon);

		sprintf(weapon_str, "%s %d",disp_secondary_weapon_name,Players[Player_num].secondary_ammo[Secondary_weapon]);
		gr_get_string_size(weapon_str, &w, &h, &aw );
		gr_string(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-LINE_SPACING, weapon_str);

		show_bomb_count(grd_curcanv->cv_bitmap.bm_w-FSPACX(1), y-(LINE_SPACING*3),-1,1,1);
	}
}

static void hud_show_cloak_invuln(void)
{
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*8;
		else
			y -= LINE_SPACING*4;

		const fix64 effect_end = Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime64;
		if (effect_end > F1_0*3 || GameTime64 & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s: %lu", TXT_CLOAKED, static_cast<unsigned long>(effect_end / F1_0));
		}
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*9;
		else
			y -= LINE_SPACING*5;

		const fix64 effect_end = Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime64;
		if (effect_end > F1_0*4 || GameTime64 & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s: %lu", TXT_INVULNERABLE, static_cast<unsigned long>(effect_end / F1_0));
		}
	}

}

static void hud_show_shield(void)
{
	if (PlayerCfg.HudMode<2) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

		if ( Players[Player_num].shields >= 0 )	{
			if (Game_mode & GM_MULTI)
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*6)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
			else
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*2)),"%s: %i", TXT_SHIELD, f2ir(Players[Player_num].shields));
		} else {
			if (Game_mode & GM_MULTI)
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*6)),"%s: 0", TXT_SHIELD );
			else
				gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*2)),"%s: 0", TXT_SHIELD );
		}
	}

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_shields(f2ir(Players[Player_num].shields));
}

//draw the icons for number of lives
static void hud_show_lives()
{
	int x;

	if (HUD_toolong)
		return;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
		x = HUD_SCALE_X(7);
	else
		x = FSPACX(2);

	if (Game_mode & GM_MULTI) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_printf(x, FSPACY(1), "%s: %d", TXT_DEATHS, Players[Player_num].net_killed_total);
	}
	else if (Players[Player_num].lives > 1)  {
		PAGE_IN_GAUGE( GAUGE_LIVES );
		grs_bitmap * bm = &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ];
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		hud_bitblt_free(x,FSPACY(1),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X_AR(bm->bm_w)+x, FSPACY(1), " x %d", Players[Player_num].lives-1);
	}

}

static void sb_show_lives()
{
	int x,y;
	grs_bitmap * bm = &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ];
	x = SB_LIVES_X;
	y = SB_LIVES_Y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
	if (Game_mode & GM_MULTI)
		gr_printf(HUD_SCALE_X(SB_LIVES_LABEL_X),HUD_SCALE_Y(y),"%s:", TXT_DEATHS);
	else
		gr_printf(HUD_SCALE_X(SB_LIVES_LABEL_X),HUD_SCALE_Y(y),"%s:", TXT_LIVES);

	if (Game_mode & GM_MULTI)
	{
		char killed_str[20];
		int w, h, aw;
		static array<int, 4> last_x{{SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H}};

		sprintf(killed_str, "%5d", Players[Player_num].net_killed_total);
		gr_get_string_size(killed_str, &w, &h, &aw);
		gr_setcolor(BM_XRGB(0,0,0));
		const auto x = HUD_SCALE_X(SB_SCORE_RIGHT)-w-FSPACX(1);
		gr_rect(exchange(last_x[HIRESMODE], x), HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y)+LINE_SPACING);
		gr_set_fontcolor(BM_XRGB(0,20,0),-1);
		gr_string(x, HUD_SCALE_Y(y), killed_str);
		return;
	}

	//erase old icons
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(HUD_SCALE_X(x), HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y+bm->bm_h));

	if (Players[Player_num].lives-1 > 0) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		PAGE_IN_GAUGE( GAUGE_LIVES );
		hud_bitblt_free(HUD_SCALE_X(x),HUD_SCALE_Y(y),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X(x)+HUD_SCALE_X_AR(bm->bm_w), HUD_SCALE_Y(y), " x %d", Players[Player_num].lives-1);
	}
}

#ifndef RELEASE
static void show_time()
{
	int secs = f2i(Players[Player_num].time_level) % 60;
	int mins = f2i(Players[Player_num].time_level) / 60;

	gr_set_curfont( GAME_FONT );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1 );

	gr_printf(SWIDTH-FSPACX(30),GHEIGHT-(LINE_SPACING*11),"%d:%02d", mins, secs);
}
#endif

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

void add_points_to_score(int points)
{
	int prev_score;

	score_time += f1_0*2;
	score_display += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	if (points == 0 || cheats.enabled)
		return;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);

	if (Game_mode & GM_MULTI_COOP)
		multi_send_score();

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		powerup_basic_str(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

void add_bonus_points_to_score(int points)
{
	int prev_score;

	if (points == 0 || cheats.enabled)
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;


	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

// Decode cockpit bitmap to deccpt and add alpha fields to weapon boxes (as it should have always been) so we later can render sub bitmaps over the window canvases
static void cockpit_decode_alpha(grs_bitmap *bm)
{

	static unsigned char *cur=NULL;
	static short cur_w=0, cur_h=0;
	static unsigned char cockpitbuf[1024*1024];

	// check if we processed this bitmap already
	if (cur==bm->bm_data && cur_w == bm->bm_w && cur_h == bm->bm_h)
		return;

	memset(cockpitbuf,0,1024*1024);

	// decode the bitmap
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		auto sbits = &bm->get_bitmap_data()[4 + (bm->bm_h * data_offset)];
		dbits = cockpitbuf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode({sbits, dbits}, rle_end(*bm, cockpitbuf));
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(&cockpitbuf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	// add alpha color to the pixels which are inside the window box spans
	const unsigned lower_y = (HIRESMODE?364:151);
	const unsigned upper_y = (HIRESMODE?469:193) - lower_y;
	unsigned i = bm->bm_w * lower_y;
	const auto *wbl = (HIRESMODE?weapon_window_left_hires:weapon_window_left);
	const auto *wbr = (HIRESMODE?weapon_window_right_hires:weapon_window_right);
	for (unsigned y=0;y < upper_y;y++)
	{
		const auto &wbly = wbl[y];
		const auto &wbry = wbr[y];
		for (unsigned x=0;x < bm->bm_w;x++)
		{
			if ((x >= wbly.l && x <= wbly.r) || (x >= wbry.l && x <= wbry.r))
				cockpitbuf[i]=TRANSPARENCY_COLOR;
			i++;
		}
	}
#ifdef OGL
	ogl_freebmtexture(*bm);
#endif
	gr_init_bitmap(deccpt, 0, 0, 0, bm->bm_w, bm->bm_h, bm->bm_w, cockpitbuf);
	gr_set_transparent(deccpt,1);
#ifdef OGL
	ogl_ubitmapm_cs (0, 0, -1, -1, deccpt, 255, F1_0); // render one time to init the texture
#endif
	WinBoxOverlay[0] = gr_create_sub_bitmap(deccpt,(PRIMARY_W_BOX_LEFT)-2,(PRIMARY_W_BOX_TOP)-2,(PRIMARY_W_BOX_RIGHT-PRIMARY_W_BOX_LEFT+4),(PRIMARY_W_BOX_BOT-PRIMARY_W_BOX_TOP+4));
	WinBoxOverlay[1] = gr_create_sub_bitmap(deccpt,(SECONDARY_W_BOX_LEFT)-2,(SECONDARY_W_BOX_TOP)-2,(SECONDARY_W_BOX_RIGHT-SECONDARY_W_BOX_LEFT)+4,(SECONDARY_W_BOX_BOT-SECONDARY_W_BOX_TOP)+4);

	cur = bm->get_bitmap_data();
	cur_w = bm->bm_w;
	cur_h = bm->bm_h;
}

static void draw_wbu_overlay()
{
#if defined(DXX_BUILD_DESCENT_I)
	unsigned cockpit_idx = PlayerCfg.CockpitMode[1];
#elif defined(DXX_BUILD_DESCENT_II)
	unsigned cockpit_idx = PlayerCfg.CockpitMode[1]+(HIRESMODE?(Num_cockpits/2):0);
#endif
	PIGGY_PAGE_IN(cockpit_bitmap[cockpit_idx]);
	grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[cockpit_idx].index];

	cockpit_decode_alpha(bm);

	if (WinBoxOverlay[0])
		hud_bitblt(HUD_SCALE_X(PRIMARY_W_BOX_LEFT-2),HUD_SCALE_Y(PRIMARY_W_BOX_TOP-2),WinBoxOverlay[0].get());
	if (WinBoxOverlay[1])
		hud_bitblt(HUD_SCALE_X(SECONDARY_W_BOX_LEFT-2),HUD_SCALE_Y(SECONDARY_W_BOX_TOP-2),WinBoxOverlay[1].get());
}

void close_gauges()
{
	WinBoxOverlay = {};
}

void init_gauges()
{
	old_weapon[0] = old_weapon[1] = -1;
	old_laser_level	= -1;
#if defined(DXX_BUILD_DESCENT_II)
	weapon_box_user[0] = weapon_box_user[1] = WBU_WEAPON;
#endif
}

static void draw_energy_bar(int energy)
{
	int x1, x2, y;
	int not_energy = (HIRESMODE?(HUD_SCALE_X(125 - (energy*125)/100)):(HUD_SCALE_X(63 - (energy*63)/100)));
	double aplitscale=((double)(HUD_SCALE_X(65)/HUD_SCALE_Y(8))/(65/8)); //scale aplitude of energy bar to current resolution aspect

	// Draw left energy bar
	PAGE_IN_GAUGE( GAUGE_ENERGY_LEFT );
	hud_bitblt (HUD_SCALE_X(LEFT_ENERGY_GAUGE_X), HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_ENERGY_LEFT) ]);

	gr_setcolor(BM_XRGB(0,0,0));

	if (energy < 100)
		for (y=0; y < HUD_SCALE_Y(LEFT_ENERGY_GAUGE_H); y++) {
			x1 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_H - 2) - y*(aplitscale);
			x2 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_H - 2) - y*(aplitscale) + not_energy;

			if (x2 > HUD_SCALE_X(LEFT_ENERGY_GAUGE_W) - (y*aplitscale)/3)
				x2 = HUD_SCALE_X(LEFT_ENERGY_GAUGE_W) - (y*aplitscale)/3;

			if (x2 > x1) gr_uline( i2f(x1+HUD_SCALE_X(LEFT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y)), i2f(x2+HUD_SCALE_X(LEFT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y)) );
		}

	gr_set_current_canvas( NULL );

	// Draw right energy bar
	PAGE_IN_GAUGE( GAUGE_ENERGY_RIGHT );
	hud_bitblt (HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X), HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_ENERGY_RIGHT) ]);

	if (energy < 100)
		for (y=0; y < HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_H); y++) {
			x1 = HUD_SCALE_X(RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H + 2 ) + y*(aplitscale) - not_energy;
			x2 = HUD_SCALE_X(RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H + 2 ) + y*(aplitscale);

			if (x1 < (y*aplitscale)/3)
				x1 = (y*aplitscale)/3;

			if (x2 > x1) gr_uline( i2f(x1+HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y)), i2f(x2+HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X)), i2f(y+HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y)) );
		}

	gr_set_current_canvas( NULL );
}

#if defined(DXX_BUILD_DESCENT_II)
static void draw_afterburner_bar(int afterburner)
{
	int not_afterburner;
	int i, j, y;
	static const ubyte afterburner_bar_table[AFTERBURNER_GAUGE_H_L*2] = { 3,11, 3,11, 3,11, 3,11, 3,11, 3,11, 2,11, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10, 1,10, 1,10, 1,10, 1,9, 1,9, 1,9, 1,9, 0,9, 0,9, 0,8, 0,8, 0,8, 0,8, 1,8, 2,8, 3,8, 4,8, 5,8, 6,7 };
	static const ubyte afterburner_bar_table_hires[AFTERBURNER_GAUGE_H_H*2] = { 5,20, 5,20, 5,19, 5,19, 5,19, 5,19, 4,19, 4,19, 4,19, 4,19, 4,19, 4,18, 4,18, 4,18, 4,18, 3,18, 3,18, 3,18, 3,18, 3,18, 3,18, 3,17, 3,17, 2,17, 2,17, 2,17, 2,17, 2,17, 2,17, 2,17, 2,17, 2,16, 2,16, 1,16, 1,16, 1,16, 1,16, 1,16, 1,16, 1,16, 1,16, 1,15, 1,15, 1,15, 0,15, 0,15, 0,15, 0,15, 0,15, 0,15, 0,14, 0,14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,13, 7,13, 8,13, 9,13, 10,13, 11,13, 12,13 };
	const ubyte *pabt = (HIRESMODE ? afterburner_bar_table_hires : afterburner_bar_table);

	// Draw afterburner bar
	PAGE_IN_GAUGE( GAUGE_AFTERBURNER );
	hud_bitblt( HUD_SCALE_X(AFTERBURNER_GAUGE_X), HUD_SCALE_Y(AFTERBURNER_GAUGE_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_AFTERBURNER) ]);
	gr_setcolor( BM_XRGB(0,0,0) );
	not_afterburner = fixmul(f1_0 - afterburner,AFTERBURNER_GAUGE_H);

	for (y = 0; y < not_afterburner; y++) {
		for (i = HUD_SCALE_Y (y), j = HUD_SCALE_Y (y + 1); i < j; i++) {
			gr_rect (
				HUD_SCALE_X (AFTERBURNER_GAUGE_X + pabt [y * 2]),
				HUD_SCALE_Y (AFTERBURNER_GAUGE_Y-1) + i,
				HUD_SCALE_X (AFTERBURNER_GAUGE_X + pabt [y * 2 + 1] + 1),
				HUD_SCALE_Y (AFTERBURNER_GAUGE_Y) + i);
			}
		}
	gr_set_current_canvas( NULL );
}
#endif

static void draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	PAGE_IN_GAUGE( GAUGE_SHIELDS+9-bm_num	);
	hud_bitblt( HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num) ]);
}

#define CLOAK_FADE_WAIT_TIME  0x400

static void draw_player_ship(int cloak_state,int x, int y)
{
	static fix cloak_fade_timer=0;
	static int cloak_fade_value=GR_FADE_LEVELS-1;
	const auto color = get_player_or_team_color(Player_num);
	PAGE_IN_GAUGE(GAUGE_SHIPS+color);
	grs_bitmap *const bm = &GameBitmaps[GET_GAUGE_INDEX(GAUGE_SHIPS+color)];

	if (cloak_state)
	{
		static int step = 0;

		if (GameTime64-Players[Player_num].cloak_time < F1_0)
		{
			step = -2;
		}
		else if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime64 <= F1_0*3)
		{
			if (cloak_fade_value >= (GR_FADE_LEVELS-1))
			{
				step = -2;
			}
			else if (cloak_fade_value <= 0)
			{
				step = 2;
			}
		}
		else
		{
			step = 0;
			cloak_fade_value = 0;
		}

		cloak_fade_timer -= FrameTime;

		while (cloak_fade_timer < 0)
		{
			cloak_fade_timer += CLOAK_FADE_WAIT_TIME;
			cloak_fade_value += step;
		}

		if (cloak_fade_value > (GR_FADE_LEVELS-1))
			cloak_fade_value = (GR_FADE_LEVELS-1);
		if (cloak_fade_value <= 0)
			cloak_fade_value = 0;
	}
	else
	{
		cloak_fade_timer = 0;
		cloak_fade_value = GR_FADE_LEVELS-1;
	}

	gr_set_current_canvas(NULL);
	hud_bitblt( HUD_SCALE_X(x), HUD_SCALE_Y(y), bm);
	gr_settransblend(cloak_fade_value, GR_BLEND_NORMAL);
	gr_rect(HUD_SCALE_X(x-3), HUD_SCALE_Y(y-3), HUD_SCALE_X(x+bm->bm_w+3), HUD_SCALE_Y(y+bm->bm_h+3));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	gr_set_current_canvas( NULL );
}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

static void draw_numerical_display(int shield, int energy)
{
	int sw,sh,saw,ew,eh,eaw;

	gr_set_curfont( GAME_FONT );
#ifndef OGL
	PAGE_IN_GAUGE( GAUGE_NUMERICAL );
	hud_bitblt( HUD_SCALE_X(NUMERICAL_GAUGE_X), HUD_SCALE_Y(NUMERICAL_GAUGE_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_NUMERICAL) ]);
#endif
	// cockpit is not 100% geometric so we need to divide shield and energy X position by 1.951 which should be most accurate
	// gr_get_string_size is used so we can get the numbers finally in the correct position with sw and ew
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );
	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf((grd_curscreen->get_screen_width() / 1.951) - (sw / 2),
			(grd_curscreen->get_screen_height() / 1.365), "%d", shield);

	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf((grd_curscreen->get_screen_width() / 1.951) - (ew / 2),
			(grd_curscreen->get_screen_height() / 1.5), "%d", energy);

	gr_set_current_canvas( NULL );
}


static void draw_keys()
{
	gr_set_current_canvas( NULL );

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY )	{
		PAGE_IN_GAUGE( GAUGE_BLUE_KEY );
		hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_BLUE_KEY) ]);
	} else {
		PAGE_IN_GAUGE( GAUGE_BLUE_KEY_OFF );
		hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_BLUE_KEY_OFF) ]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)	{
		PAGE_IN_GAUGE( GAUGE_GOLD_KEY );
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_GOLD_KEY) ]);
	} else {
		PAGE_IN_GAUGE( GAUGE_GOLD_KEY_OFF );
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_GOLD_KEY_OFF) ]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)	{
		PAGE_IN_GAUGE( GAUGE_RED_KEY );
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_RED_KEY) ]);
	} else {
		PAGE_IN_GAUGE( GAUGE_RED_KEY_OFF );
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[ GET_GAUGE_INDEX(GAUGE_RED_KEY_OFF) ]);
	}
}


static void draw_weapon_info_sub(int info_index,const gauge_box *box,int pic_x,int pic_y,const char *name,int text_x,int text_y)
{
	grs_bitmap *bm;

	//clear the window
	gr_setcolor(BM_XRGB(0,0,0));
#if defined(DXX_BUILD_DESCENT_I)
	gr_rect(HUD_SCALE_X(box->left),HUD_SCALE_Y(box->top),HUD_SCALE_X(box->right),HUD_SCALE_Y(box->bot+1));
	PIGGY_PAGE_IN( Weapon_info[info_index].picture );
	bm=&GameBitmaps[Weapon_info[info_index].picture.index];
#elif defined(DXX_BUILD_DESCENT_II)
	gr_rect(HUD_SCALE_X(box->left),HUD_SCALE_Y(box->top),HUD_SCALE_X(box->right),HUD_SCALE_Y(box->bot));

	if (Piggy_hamfile_version >= 3 // !SHAREWARE
		&& HIRESMODE)
	{
		PIGGY_PAGE_IN( Weapon_info[info_index].hires_picture );
		bm=&GameBitmaps[Weapon_info[info_index].hires_picture.index];
	}
	else
	{
		PIGGY_PAGE_IN( Weapon_info[info_index].picture );
		bm=&GameBitmaps[Weapon_info[info_index].picture.index];
	}
#endif

	Assert(bm != NULL);

	hud_bitblt(HUD_SCALE_X(pic_x), HUD_SCALE_Y(pic_y), bm);

	if (PlayerCfg.HudMode == 0)
	{
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );

		gr_string(text_x,text_y,name);

		//	For laser, show level and quadness
#if defined(DXX_BUILD_DESCENT_I)
		if (info_index == LASER_INDEX)
#elif defined(DXX_BUILD_DESCENT_II)
		if (info_index == LASER_ID || info_index == SUPER_LASER_ID)
#endif
		{
			gr_printf(text_x,text_y+LINE_SPACING, "%s: %i", TXT_LVL, Players[Player_num].laser_level+1);
			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
				gr_string(text_x,text_y+(LINE_SPACING*2), TXT_QUAD);
		}
	}
}


static void draw_weapon_info(int weapon_type,int weapon_num,int laser_level)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)laser_level;
#endif
	int x,y;
	int info_index;

	if (weapon_type == 0) {
		info_index = Primary_weapon_to_weapon_info[weapon_num];

#if defined(DXX_BUILD_DESCENT_II)
		if (info_index == LASER_ID && laser_level > MAX_LASER_LEVEL)
			info_index = SUPER_LASER_ID;
#endif

		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(info_index,
				&gauge_boxes[SB_PRIMARY_BOX],
				SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,
				PRIMARY_WEAPON_NAMES_SHORT(weapon_num),
				SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y);
			x=SB_PRIMARY_AMMO_X;
			y=SB_PRIMARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(info_index,
				&gauge_boxes[COCKPIT_PRIMARY_BOX],
				PRIMARY_W_PIC_X,PRIMARY_W_PIC_Y,
				PRIMARY_WEAPON_NAMES_SHORT(weapon_num),
				PRIMARY_W_TEXT_X,PRIMARY_W_TEXT_Y);
			x=PRIMARY_AMMO_X;
			y=PRIMARY_AMMO_Y;
		}
		if (PlayerCfg.HudMode!=0)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (weapon_box_user[weapon_type] == WBU_WEAPON)
#endif
				hud_show_weapons_mode(weapon_type,1,x,y);
		}
	}
	else
	{
		info_index = Secondary_weapon_to_weapon_info[weapon_num];

		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(info_index,
				&gauge_boxes[SB_SECONDARY_BOX],
				SB_SECONDARY_W_PIC_X,SB_SECONDARY_W_PIC_Y,
				SECONDARY_WEAPON_NAMES_SHORT(weapon_num),
				SB_SECONDARY_W_TEXT_X,SB_SECONDARY_W_TEXT_Y);
			x=SB_SECONDARY_AMMO_X;
			y=SB_SECONDARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(info_index,
				&gauge_boxes[COCKPIT_SECONDARY_BOX],
				SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,
				SECONDARY_WEAPON_NAMES_SHORT(weapon_num),
				SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y);
			x=SECONDARY_AMMO_X;
			y=SECONDARY_AMMO_Y;
		}
		if (PlayerCfg.HudMode!=0)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (weapon_box_user[weapon_type] == WBU_WEAPON)
#endif
				hud_show_weapons_mode(weapon_type,1,x,y);
		}
	}
}

static void draw_ammo_info(int x,int y,int ammo_count)
{
	if (!PlayerCfg.HudMode)
	{
		gr_setcolor(BM_XRGB(0,0,0));
		gr_set_fontcolor(BM_XRGB(20,0,0),-1 );
		gr_printf(x,y,"%03d",ammo_count);
	}
}

static void draw_secondary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		draw_ammo_info(SB_SECONDARY_AMMO_X,SB_SECONDARY_AMMO_Y,ammo_count);
	else
		draw_ammo_info(SECONDARY_AMMO_X,SECONDARY_AMMO_Y,ammo_count);
}

static void draw_weapon_box(int weapon_type,int weapon_num)
{
	int laser_level_changed;

	gr_set_current_canvas(NULL);

	gr_set_curfont( GAME_FONT );

	laser_level_changed = (weapon_type==0 && weapon_num==LASER_INDEX && (Players[Player_num].laser_level != old_laser_level));

	if ((weapon_num != old_weapon[weapon_type] || laser_level_changed) && weapon_box_states[weapon_type] == WS_SET && (old_weapon[weapon_type] != -1) && !PlayerCfg.HudMode)
	{
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type]=i2f(GR_FADE_LEVELS-1);
	}

	if (old_weapon[weapon_type] == -1)
	{
		draw_weapon_info(weapon_type,weapon_num,Players[Player_num].laser_level);
		old_weapon[weapon_type] = weapon_num;
		weapon_box_states[weapon_type] = WS_SET;
	}

	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		draw_weapon_info(weapon_type,old_weapon[weapon_type],old_laser_level);
		weapon_box_fade_values[weapon_type] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0) {
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type] = weapon_num;
			old_laser_level = Players[Player_num].laser_level;
			weapon_box_fade_values[weapon_type] = 0;
		}
	}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN) {
		if (weapon_num != old_weapon[weapon_type]) {
			weapon_box_states[weapon_type] = WS_FADING_OUT;
		}
		else {
			draw_weapon_info(weapon_type,weapon_num,Players[Player_num].laser_level);
			weapon_box_fade_values[weapon_type] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_FADE_LEVELS-1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type] = -1;
			}
		}
	} else
	{
		draw_weapon_info(weapon_type, weapon_num, Players[Player_num].laser_level);
		old_weapon[weapon_type] = weapon_num;
		old_laser_level = Players[Player_num].laser_level;
	}

	if (weapon_box_states[weapon_type] != WS_SET)		//fade gauge
	{
		int fade_value = f2i(weapon_box_fade_values[weapon_type]);
		int boxofs = (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;

		gr_settransblend(fade_value, GR_BLEND_NORMAL);
		gr_rect(HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].left),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].top),HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].right),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].bot));

		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	}

	gr_set_current_canvas(NULL);
}

#if defined(DXX_BUILD_DESCENT_II)
static array<fix, 2> static_time;

static void draw_static(int win)
{
	vclip *vc = &Vclip[VCLIP_MONITOR_STATIC];
	grs_bitmap *bmp;
	int framenum;
	int boxofs = (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
#ifndef OGL
	int x,y;
#endif

	static_time[win] += FrameTime;
	if (static_time[win] >= vc->play_time) {
		weapon_box_user[win] = WBU_WEAPON;
		return;
	}

	framenum = static_time[win] * vc->num_frames / vc->play_time;

	PIGGY_PAGE_IN(vc->frames[framenum]);

	bmp = &GameBitmaps[vc->frames[framenum].index];

	gr_set_current_canvas(NULL);
#ifndef OGL
	for (x=gauge_boxes[boxofs+win].left;x<gauge_boxes[boxofs+win].right;x+=bmp->bm_w)
		for (y=gauge_boxes[boxofs+win].top;y<gauge_boxes[boxofs+win].bot;y+=bmp->bm_h)
			gr_bitmap(x,y,*bmp);
#else
	if (HIRESMODE)
	{
		hud_bitblt(HUD_SCALE_X(gauge_boxes[boxofs+win].left),HUD_SCALE_Y(gauge_boxes[boxofs+win].top),bmp);
		hud_bitblt(HUD_SCALE_X(gauge_boxes[boxofs+win].left),HUD_SCALE_Y(gauge_boxes[boxofs+win].bot-bmp->bm_h),bmp);
		hud_bitblt(HUD_SCALE_X(gauge_boxes[boxofs+win].right-bmp->bm_w),HUD_SCALE_Y(gauge_boxes[boxofs+win].top),bmp);
		hud_bitblt(HUD_SCALE_X(gauge_boxes[boxofs+win].right-bmp->bm_w),HUD_SCALE_Y(gauge_boxes[boxofs+win].bot-bmp->bm_h),bmp);
	}

#endif

	gr_set_current_canvas(NULL);
}
#endif

static void draw_weapon_boxes()
{
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_box_user[0] == WBU_WEAPON)
#endif
	{
		draw_weapon_box(0,Primary_weapon);

		if (weapon_box_states[0] == WS_SET) {
			int is_vulcan_ammo_weapon = weapon_index_uses_vulcan_ammo(Primary_weapon);
			if (is_vulcan_ammo_weapon)
			{
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_primary_ammo(Players[Player_num].vulcan_ammo);
				draw_primary_ammo_info(f2i((unsigned) VULCAN_AMMO_SCALE * (unsigned) Players[Player_num].vulcan_ammo));
			}

#if defined(DXX_BUILD_DESCENT_II)
			if (Primary_weapon == OMEGA_INDEX)
			{
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_primary_ammo(Omega_charge);
				draw_primary_ammo_info(Omega_charge * 100/MAX_OMEGA_CHARGE);
			}
#endif
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (weapon_box_user[0] == WBU_STATIC)
		draw_static(0);

	if (weapon_box_user[1] == WBU_WEAPON)
#endif
	{
		draw_weapon_box(1,Secondary_weapon);

		if (weapon_box_states[1] == WS_SET)
		{
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_secondary_ammo(Players[Player_num].secondary_ammo[Secondary_weapon]);
			draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (weapon_box_user[1] == WBU_STATIC)
		draw_static(1);
#endif
}


static void sb_draw_energy_bar(int energy)
{
	int erase_height,i;
	int ew, eh, eaw;

	PAGE_IN_GAUGE( SB_GAUGE_ENERGY );
	hud_bitblt(HUD_SCALE_X(SB_ENERGY_GAUGE_X), HUD_SCALE_Y(SB_ENERGY_GAUGE_Y), &GameBitmaps[GET_GAUGE_INDEX(SB_GAUGE_ENERGY)]);

	erase_height = HUD_SCALE_Y((100 - energy) * SB_ENERGY_GAUGE_H / 100);
	gr_setcolor( 0 );
	for (i=0;i<erase_height;i++)
		gr_uline( i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X-1)), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i), i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X+(SB_ENERGY_GAUGE_W))), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i) );

	//draw numbers
	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
#if defined(DXX_BUILD_DESCENT_I)
	unsigned y = SB_ENERGY_NUM_Y;
#elif defined(DXX_BUILD_DESCENT_II)
	unsigned y = SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4);
#endif
	gr_printf((grd_curscreen->get_screen_width() / 3) - (ew / 2), HUD_SCALE_Y(y), "%d", energy);

	gr_set_current_canvas(NULL);
}

#if defined(DXX_BUILD_DESCENT_II)
static void sb_draw_afterburner()
{
	int erase_height, w, h, aw, i;
	auto &ab_str = "AB";

	PAGE_IN_GAUGE( SB_GAUGE_AFTERBURNER );
	hud_bitblt(HUD_SCALE_X(SB_AFTERBURNER_GAUGE_X), HUD_SCALE_Y(SB_AFTERBURNER_GAUGE_Y), &GameBitmaps[GET_GAUGE_INDEX(SB_GAUGE_AFTERBURNER)]);

	erase_height = HUD_SCALE_Y(fixmul((f1_0 - Afterburner_charge),SB_AFTERBURNER_GAUGE_H-1));
	gr_setcolor( 0 );
	for (i=0;i<erase_height;i++)
		gr_uline( i2f(HUD_SCALE_X(SB_AFTERBURNER_GAUGE_X-1)), i2f(HUD_SCALE_Y(SB_AFTERBURNER_GAUGE_Y)+i), i2f(HUD_SCALE_X(SB_AFTERBURNER_GAUGE_X+(SB_AFTERBURNER_GAUGE_W))), i2f(HUD_SCALE_Y(SB_AFTERBURNER_GAUGE_Y)+i) );

	//draw legend
	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		gr_set_fontcolor(BM_XRGB(45,0,0),-1 );
	else
		gr_set_fontcolor(BM_XRGB(12,12,12),-1 );

	gr_get_string_size(ab_str, &w, &h, &aw );
	gr_string(HUD_SCALE_X(SB_AFTERBURNER_GAUGE_X+(SB_AFTERBURNER_GAUGE_W+1)/2)-(w/2), HUD_SCALE_Y(SB_AFTERBURNER_GAUGE_Y+(SB_AFTERBURNER_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4))), "AB");
	gr_set_current_canvas(NULL);
}
#endif

static void sb_draw_shield_num(int shield)
{
	//draw numbers
	int sw, sh, saw;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );

	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf((grd_curscreen->get_screen_width() / 2.266) - (sw / 2), HUD_SCALE_Y(SB_SHIELD_NUM_Y), "%d", shield);
}

static void sb_draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	gr_set_current_canvas(NULL);
	PAGE_IN_GAUGE( GAUGE_SHIELDS+9-bm_num );
	hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[GET_GAUGE_INDEX(GAUGE_SHIELDS+9-bm_num) ]);
}

static void sb_draw_keys()
{
	grs_bitmap * bm;
	int flags = Players[Player_num].flags;

	gr_set_current_canvas(NULL);
	bm = &GameBitmaps[ GET_GAUGE_INDEX((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF) ];
	PAGE_IN_GAUGE( (flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF );
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_BLUE_KEY_Y), bm);
	bm = &GameBitmaps[ GET_GAUGE_INDEX((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF) ];
	PAGE_IN_GAUGE( (flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF );
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_GOLD_KEY_Y), bm);
	bm = &GameBitmaps[ GET_GAUGE_INDEX((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF) ];
	PAGE_IN_GAUGE( (flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF );
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_RED_KEY_Y), bm);
}

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
static void draw_invulnerable_ship()
{
	static fix time=0;

	gr_set_current_canvas(NULL);

	if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime64 > F1_0*4 || GameTime64 & 0x8000)
	{

		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)	{
			PAGE_IN_GAUGE( GAUGE_INVULNERABLE+invulnerable_frame );
			hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame) ]);
		} else {
			PAGE_IN_GAUGE( GAUGE_INVULNERABLE+invulnerable_frame );
			hud_bitblt( HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[GET_GAUGE_INDEX(GAUGE_INVULNERABLE+invulnerable_frame)]);
		}

		time += FrameTime;

		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		sb_draw_shield_bar(f2ir(Players[Player_num].shields));
	else
		draw_shield_bar(f2ir(Players[Player_num].shields));
}

const rgb_array_t player_rgb_normal{{
							{15,15,23},
							{27,0,0},
							{0,23,0},
							{30,11,31},
							{31,16,0},
							{24,17,6},
							{14,21,12},
							{29,29,0},
}};

struct xy {
	sbyte x, y;
};

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
const array<xy, 4> cross_offsets{{
	{-8,-5},
	{-4,-2},
	{-4,-2},
	{-2,-1}
}},
	primary_offsets{{
	{-30,14},
	{-16,6},
	{-15,6},
	{-8, 2}
}},
	secondary_offsets{{
	{-24,2},
	{-12,0},
	{-12,1},
	{-6,-2}
}};

//draw the reticle
void show_reticle(int reticle_type, int secondary_display)
{
	int x,y,size;
	int laser_ready,missile_ready;
	int cross_bm_num,primary_bm_num,secondary_bm_num;
	int use_hires_reticle,ofs,gauge_index;

#if defined(DXX_BUILD_DESCENT_II)
	if (Newdemo_state==ND_STATE_PLAYBACK && Viewer->type != OBJ_PLAYER)
		 return;
#endif

	x = grd_curcanv->cv_bitmap.bm_w/2;
	y = grd_curcanv->cv_bitmap.bm_h/2;
	size = (grd_curcanv->cv_bitmap.bm_h / (32-(PlayerCfg.ReticleSize*4)));

	laser_ready = allowed_to_fire_laser();
	missile_ready = allowed_to_fire_missile();

	primary_bm_num = (laser_ready && player_has_primary_weapon(Primary_weapon).has_all());
	secondary_bm_num = (missile_ready && player_has_secondary_weapon(Secondary_weapon).has_all());

	if (primary_bm_num && Primary_weapon==LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
		primary_bm_num++;

	if (Secondary_weapon_to_gun_num[Secondary_weapon]==7)
		secondary_bm_num += 3;		//now value is 0,1 or 3,4
	else if (secondary_bm_num && !(Missile_gun&1))
			secondary_bm_num++;

	cross_bm_num = ((primary_bm_num > 0) || (secondary_bm_num > 0));

	Assert(primary_bm_num <= 2);
	Assert(secondary_bm_num <= 4);
	Assert(cross_bm_num <= 1);

	gr_setcolor(BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]));
	gr_settransblend(PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);

	switch (reticle_type)
	{
		case RET_TYPE_CLASSIC:
		{
			grs_bitmap *cross, *primary, *secondary;

			use_hires_reticle = (HIRESMODE != 0);
			ofs = (use_hires_reticle?0:2);
			gauge_index = RETICLE_CROSS + cross_bm_num;
			PAGE_IN_GAUGE( gauge_index );
			cross = &GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			hud_bitblt_free(x+HUD_SCALE_X_AR(cross_offsets[ofs].x),y+HUD_SCALE_Y_AR(cross_offsets[ofs].y), HUD_SCALE_X_AR(cross->bm_w), HUD_SCALE_Y_AR(cross->bm_h), cross);

			gauge_index = RETICLE_PRIMARY + primary_bm_num;
			PAGE_IN_GAUGE( gauge_index );
			primary = &GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			hud_bitblt_free(x+HUD_SCALE_X_AR(primary_offsets[ofs].x),y+HUD_SCALE_Y_AR(primary_offsets[ofs].y), HUD_SCALE_X_AR(primary->bm_w), HUD_SCALE_Y_AR(primary->bm_h), primary);

			gauge_index = RETICLE_SECONDARY + secondary_bm_num;
			PAGE_IN_GAUGE( gauge_index );
			secondary = &GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			hud_bitblt_free(x+HUD_SCALE_X_AR(secondary_offsets[ofs].x),y+HUD_SCALE_Y_AR(secondary_offsets[ofs].y), HUD_SCALE_X_AR(secondary->bm_w), HUD_SCALE_Y_AR(secondary->bm_h), secondary);
			break;
		}
		case RET_TYPE_CLASSIC_REBOOT:
		{
#ifdef OGL
			ogl_draw_vertex_reticle(cross_bm_num,primary_bm_num,secondary_bm_num,BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]),PlayerCfg.ReticleRGBA[3],PlayerCfg.ReticleSize);
#endif
			break;

		}
		case RET_TYPE_X:
			gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5))); // top-left
			gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5))); // top-right
			gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5))); // bottom-left
			gr_uline(i2f(x+(size/2)), i2f(y+(size/2)), i2f(x+(size/5)), i2f(y+(size/5))); // bottom-right
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x+(size/2)), i2f(y+(size/2)+(size/5)), i2f(x+(size/5)), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_DOT:
			gr_disk(i2f(x),i2f(y),i2f(size/5));
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x), i2f(y+(size/2)+(size/5)), i2f(x), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_CIRCLE:
			gr_ucircle(i2f(x),i2f(y),i2f(size/4));
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)-(size/5)), i2f(y-(size/2)), i2f(x-(size/5)-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)+(size/5)), i2f(y-(size/2)), i2f(x+(size/5)+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x), i2f(y+(size/2)+(size/5)), i2f(x), i2f(y+(size/5)+(size/5)));
			break;
		case RET_TYPE_CROSS_V1:
			gr_uline(i2f(x),i2f(y-(size/2)),i2f(x),i2f(y+(size/2)+1)); // horiz
			gr_uline(i2f(x-(size/2)),i2f(y),i2f(x+(size/2)+1),i2f(y)); // vert
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_CROSS_V2:
			gr_uline(i2f(x), i2f(y-(size/2)), i2f(x), i2f(y-(size/6))); // vert-top
			gr_uline(i2f(x), i2f(y+(size/2)), i2f(x), i2f(y+(size/6))); // vert-bottom
			gr_uline(i2f(x-(size/2)), i2f(y), i2f(x-(size/6)), i2f(y)); // horiz-left
			gr_uline(i2f(x+(size/2)), i2f(y), i2f(x+(size/6)), i2f(y)); // horiz-right
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_ANGLE:
			gr_uline(i2f(x),i2f(y),i2f(x),i2f(y+(size/2))); // vert
			gr_uline(i2f(x),i2f(y),i2f(x+(size/2)),i2f(y)); // horiz
			if (secondary_display && secondary_bm_num == 1)
				gr_uline(i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 2)
				gr_uline(i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)));
			else if (secondary_display && secondary_bm_num == 4)
				gr_uline(i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)));
			break;
		case RET_TYPE_NONE:
			break;
		default:
			break;
	}
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
}

void show_mousefs_indicator(int mx, int my, int mz, int x, int y, int size)
{
	int axscale = (MOUSEFS_DELTA_RANGE*2)/size, xaxpos = x+(mx/axscale), yaxpos = y+(my/axscale), zaxpos = y+(mz/axscale);

	gr_setcolor(BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]));
	gr_settransblend(PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);
	gr_uline(i2f(xaxpos), i2f(y-(size/2)), i2f(xaxpos), i2f(y-(size/4)));
	gr_uline(i2f(xaxpos), i2f(y+(size/2)), i2f(xaxpos), i2f(y+(size/4)));
	gr_uline(i2f(x-(size/2)), i2f(yaxpos), i2f(x-(size/4)), i2f(yaxpos));
	gr_uline(i2f(x+(size/2)), i2f(yaxpos), i2f(x+(size/4)), i2f(yaxpos));
	gr_uline(i2f(x+(size/2)+HUD_SCALE_X_AR(2)), i2f(y), i2f(x+(size/2)+HUD_SCALE_X_AR(2)), i2f(zaxpos));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
}

static void hud_show_kill_list()
{
	playernum_t n_players;
	playernum_array_t player_list;
	int n_left,i,x0,x1,y,save_y;

	if (Show_kill_list_timer > 0)
	{
		Show_kill_list_timer -= FrameTime;
		if (Show_kill_list_timer < 0)
			Show_kill_list = 0;
	}

	gr_set_curfont( GAME_FONT );

	n_players = multi_get_kill_list(player_list);

	if (Show_kill_list == 3)
		n_players = 2;

	if (n_players <= 4)
		n_left = n_players;
	else
		n_left = (n_players+1)/2;

	x0 = FSPACX(1); x1 = FSPACX(43);

	if (Game_mode & GM_MULTI_COOP)
		x1 = FSPACX(31);

	save_y = y = grd_curcanv->cv_bitmap.bm_h - n_left*(LINE_SPACING);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		save_y = y -= FSPACX(6);
		if (Game_mode & GM_MULTI_COOP)
			x1 = FSPACX(33);
		else
			x1 = FSPACX(43);
	}

	for (i=0;i<n_players;i++) {
		playernum_t player_num;
		callsign_t name;
		int sw,sh,aw;

		if (i>=n_left) {
			if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(53);
			else
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(60);
			if (Game_mode & GM_MULTI_COOP)
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(27);
			else
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(15);  // Right edge of name, change this for width problems
			if (i==n_left)
				y = save_y;

			if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
				x1-=FSPACX(18);
		}
		else  if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
		{
			x1 = FSPACX(43);
			x1 -=FSPACX(18);
		}

		if (Show_kill_list == 3)
			player_num = i;
		else
			player_num = player_list[i];

		color_t fontcolor;
		if (Show_kill_list == 1 || Show_kill_list==2)
		{
			if (Players[player_num].connected != CONNECT_PLAYING)
				fontcolor = BM_XRGB(12, 12, 12);
			else {
				auto &color = player_rgb[get_player_or_team_color(player_num)];
				fontcolor = BM_XRGB(color.r, color.g, color.b);
			}
		}
		else
		{
			auto &color = player_rgb_normal[player_num];
			fontcolor = BM_XRGB(color.r, color.g, color.b);
		}
		gr_set_fontcolor(fontcolor, -1);

		if (Show_kill_list == 3)
			name = Netgame.team_name[i];
		else if (Game_mode & GM_BOUNTY && player_num == Bounty_target && GameTime64&0x10000)
		{
			name = "[TARGET]";
		}
		else
			name = Players[player_num].callsign;	// Note link to above if!!
		gr_get_string_size(static_cast<const char *>(name),&sw,&sh,&aw);
		while (sw > (x1-x0-FSPACX(2))) {
			name.buffer()[strlen(name)-1]=0;
			gr_get_string_size(name,&sw,&sh,&aw);
		}
		gr_string(x0,y,name);

		if (Show_kill_list==2)
		{
			if (Players[player_num].net_killed_total+Players[player_num].net_kills_total==0)
				gr_string (x1,y,"NA");
		else
			gr_printf (x1,y,"%d%%",(int)((float)((float)Players[player_num].net_kills_total/((float)Players[player_num].net_killed_total+(float)Players[player_num].net_kills_total))*100.0));
		}
		else if (Show_kill_list == 3)
			gr_printf(x1,y,"%3d",team_kills[i]);
		else if (Game_mode & GM_MULTI_COOP)
			gr_printf(x1,y,"%-6d",Players[player_num].score);
		else if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
			gr_printf(x1,y,"%3d(%d)",Players[player_num].net_kills_total,Players[player_num].KillGoalCount);
		else
			gr_printf(x1,y,"%3d",Players[player_num].net_kills_total);

		y += LINE_SPACING;
	}
}

//returns true if viewer can see object
static int see_object(int objnum)
{
	fvi_query fq;
	int hit_type;
	fvi_info hit_data;

	//see if we can see this player

	fq.p0 					= &Viewer->pos;
	fq.p1 					= &Objects[objnum].pos;
	fq.rad 					= 0;
	fq.thisobjnum			= Viewer - Objects;
	fq.flags 				= FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_GET_SEGLIST;
	fq.startseg				= Viewer->segnum;
	fq.ignore_obj_list.first = nullptr;

	hit_type = find_vector_intersection(fq, hit_data);

	return (hit_type == HIT_OBJECT && hit_data.hit_object == objnum);
}

//show names of teammates & players carrying flags
void show_HUD_names()
{
	int is_friend = 0, show_friend_name = 0, show_enemy_name = 0, show_name = 0, show_typing = 0, show_indi = 0, objnum = 0;
	
	for (playernum_t pnum=0;pnum<N_players;pnum++)
	{
		if (pnum == Player_num || Players[pnum].connected != CONNECT_PLAYING)
			continue;
		// ridiculusly complex to check if we want to show something... but this is readable at least.
		is_friend = (Game_mode & GM_MULTI_COOP || (Game_mode & GM_TEAM && get_team(pnum) == get_team(Player_num)));
		show_friend_name = Show_reticle_name;
		show_enemy_name = Show_reticle_name && Netgame.ShowEnemyNames && !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED);
		show_name = ((is_friend && show_friend_name) || (!is_friend && show_enemy_name));
		show_typing = is_friend || !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED);
#if defined(DXX_BUILD_DESCENT_I)
		show_indi = ((/*(Game_mode & ( GM_CAPTURE | GM_HOARD ) && Players[pnum].flags & PLAYER_FLAGS_FLAG) || */(Game_mode & GM_BOUNTY &&  pnum == Bounty_target)) && (is_friend || !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED)));
#elif defined(DXX_BUILD_DESCENT_II)
		show_indi = ((((game_mode_capture_flag() || game_mode_hoard()) && Players[pnum].flags & PLAYER_FLAGS_FLAG) || (Game_mode & GM_BOUNTY &&  pnum == Bounty_target)) && (is_friend || !(Players[pnum].flags & PLAYER_FLAGS_CLOAKED)));
#endif

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			//if this is a demo, the objnum in the player struct is wrong, so we search the object list for the objnum
			for (objnum=0;objnum<=Highest_object_index;objnum++)
				if (Objects[objnum].type==OBJ_PLAYER && get_player_id(&Objects[objnum]) == pnum)
					break;
			if (objnum > Highest_object_index)	//not in list, thus not visible
				continue;			//..so don't show name
		}
		else
			objnum = Players[pnum].objnum;

		if ((show_name || show_typing || show_indi) && see_object(objnum))
		{
			auto player_point = g3_rotate_point(Objects[objnum].pos);
			if (player_point.p3_codes == 0) //on screen
			{
				g3_project_point(player_point);
				if (!(player_point.p3_flags & PF_OVERFLOW))
				{
					fix x,y,dx,dy;
					char s[CALLSIGN_LEN+10];
					int w, h, aw, x1, y1;

					x = player_point.p3_sx;
					y = player_point.p3_sy;
					dy = -fixmuldiv(fixmul(Objects[objnum].size,Matrix_scale.y),i2f(grd_curcanv->cv_bitmap.bm_h)/2,player_point.p3_z);
					dx = fixmul(dy,grd_curscreen->sc_aspect);
					/* Set the text to show */
					const char *name = NULL;
					if( Game_mode & GM_BOUNTY && pnum == Bounty_target )
						name = "Target";
					else if (show_name)
						name = static_cast<const char *>(Players[pnum].callsign);
					const char *trailer = NULL;
					if (show_typing)
					{
						if (multi_sending_message[pnum] == msgsend_typing)
							trailer = "Typing";
						else if (multi_sending_message[pnum] == msgsend_automap)
							trailer = "Map";
					}
					int written = snprintf(s, sizeof(s), "%s%s%s", name ? name : "", name && trailer ? ", " : "", trailer ? trailer : "");
					if (written)
					{
						gr_get_string_size(s, &w, &h, &aw);
						const auto color = get_player_or_team_color(pnum);
						gr_set_fontcolor(BM_XRGB(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b), -1);
						x1 = f2i(x)-w/2;
						y1 = f2i(y-dy)+FSPACY(1);
						gr_string (x1, y1, s);
					}

					/* Draw box on HUD */
					if (show_indi)
					{
						fix w,h;

						w = dx/4;
						h = dy/4;

#if defined(DXX_BUILD_DESCENT_II)
						if (game_mode_capture_flag())
							gr_setcolor((get_team(pnum) == TEAM_BLUE)?BM_XRGB(31,0,0):BM_XRGB(0,0,31));
						else if (game_mode_hoard())
						{
							if (Game_mode & GM_TEAM)
								gr_setcolor((get_team(pnum) == TEAM_RED)?BM_XRGB(31,0,0):BM_XRGB(0,0,31));
							else
								gr_setcolor(BM_XRGB(0,31,0));
						}
						else if( Game_mode & GM_BOUNTY )
#endif
							gr_setcolor( BM_XRGB( player_rgb[pnum].r, player_rgb[pnum].g, player_rgb[pnum].b ) );

						gr_line(x+dx-w,y-dy,x+dx,y-dy);
						gr_line(x+dx,y-dy,x+dx,y-dy+h);
						gr_line(x-dx,y-dy,x-dx+w,y-dy);
						gr_line(x-dx,y-dy,x-dx,y-dy+h);
						gr_line(x+dx-w,y+dy,x+dx,y+dy);
						gr_line(x+dx,y+dy,x+dx,y+dy-h);
						gr_line(x-dx,y+dy,x-dx+w,y+dy);
						gr_line(x-dx,y+dy,x-dx,y+dy-h);
					}
				}
			}
		}
	}
}


//draw all the things on the HUD
void draw_hud()
{
	if (Newdemo_state == ND_STATE_RECORDING)
	{
		if (Primary_weapon == VULCAN_INDEX)
			newdemo_record_primary_ammo(Players[Player_num].vulcan_ammo);
#if defined(DXX_BUILD_DESCENT_II)
		if (Primary_weapon == OMEGA_INDEX)
			newdemo_record_primary_ammo(Omega_charge);
#endif
		newdemo_record_secondary_ammo(Players[Player_num].secondary_ammo[Secondary_weapon]);
	}
	if (PlayerCfg.HudMode==3) // no hud, "immersion mode"
		return;

	// Cruise speed
	if (Viewer->type==OBJ_PLAYER && get_player_id(Viewer)==Player_num && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)	{
		int	x = FSPACX(1);
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(0, 31, 0), -1 );
		if (Cruise_speed > 0) {
			if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 10;
				else
					y -= LINE_SPACING * 6;
			} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 6;
				else
					y -= LINE_SPACING * 1;
			} else {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 7;
				else
					y -= LINE_SPACING * 2;
			}

			gr_printf( x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}

	//	Show score so long as not in rearview
	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW && PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR) {
		hud_show_score();
		if (score_time)
			hud_show_score_added();
	}

	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
		hud_show_timer_count();

	//	Show other stuff if not in rearview or letterbox.
	if (!Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
	{
		show_HUD_names();

		if (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR || PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
			hud_show_homing_warning();

		if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
			hud_show_energy();
			hud_show_shield();
			hud_show_afterburner();
			hud_show_weapons();
#if defined(DXX_BUILD_DESCENT_I)
			if (!PCSharePig)
#endif
			hud_show_keys();
			hud_show_cloak_invuln();

			if (Newdemo_state==ND_STATE_RECORDING)
				newdemo_record_player_flags(Players[Player_num].flags);
		}

#ifndef RELEASE
		if (!(Game_mode&GM_MULTI && Show_kill_list))
			show_time();
#endif

#if defined(DXX_BUILD_DESCENT_II)
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
		{
			hud_show_flag();
			hud_show_orbs();
		}
#endif
		HUD_render_message_frame();

		if (PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR)
			hud_show_lives();
		if (Game_mode&GM_MULTI && Show_kill_list)
			hud_show_kill_list();
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
			show_reticle(PlayerCfg.ReticleType, 1);
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX && Newdemo_state != ND_STATE_PLAYBACK && PlayerCfg.MouseFlightSim && PlayerCfg.MouseFSIndicator)
			show_mousefs_indicator(Controls.raw_mouse_axis[0], Controls.raw_mouse_axis[1], Controls.raw_mouse_axis[2], GWIDTH/2, GHEIGHT/2, GHEIGHT/4);
	}

	if (Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW) {
		HUD_render_message_frame();
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_string(0x8000,GHEIGHT-LINE_SPACING,TXT_REAR_VIEW);
	}
}

//print out some player statistics
void render_gauges()
{
	int energy = f2ir(Players[Player_num].energy);
	int shields = f2ir(Players[Player_num].shields);
	int cloak = ((Players[Player_num].flags&PLAYER_FLAGS_CLOAKED) != 0);

	Assert(PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR);

	if (shields < 0 ) shields = 0;

	gr_set_current_canvas(NULL);
	gr_set_curfont( GAME_FONT );

	if (Newdemo_state == ND_STATE_RECORDING)
		if (Players[Player_num].homing_object_dist >= 0)
			newdemo_record_homing_distance(Players[Player_num].homing_object_dist);

	draw_weapon_boxes();

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		draw_energy_bar(energy);
		draw_numerical_display(shields, energy);
#if defined(DXX_BUILD_DESCENT_I)
		if (!PlayerCfg.HudMode)
#elif defined(DXX_BUILD_DESCENT_II)
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_player_afterburner(Afterburner_charge);
		draw_afterburner_bar(Afterburner_charge);
#endif
		show_bomb_count(HUD_SCALE_X(BOMB_COUNT_X), HUD_SCALE_Y(BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);
		draw_player_ship(cloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship();
		else
			draw_shield_bar(shields);
		draw_numerical_display(shields, energy);

		if (Newdemo_state==ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(Players[Player_num].flags);
		}
		draw_keys();

		show_homing_warning();
		draw_wbu_overlay();

	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		sb_draw_energy_bar(energy);
#if defined(DXX_BUILD_DESCENT_I)
		if (!PlayerCfg.HudMode)
#elif defined(DXX_BUILD_DESCENT_II)
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_player_afterburner(Afterburner_charge);
		sb_draw_afterburner();
		if (!PlayerCfg.HudMode && weapon_box_user[1] == WBU_WEAPON)
#endif
			show_bomb_count(HUD_SCALE_X(SB_BOMB_COUNT_X), HUD_SCALE_Y(SB_BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);

		draw_player_ship(cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship();
		else
			sb_draw_shield_bar(shields);
		sb_draw_shield_num(shields);

		if (Newdemo_state==ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(Players[Player_num].flags);
		}
		sb_draw_keys();

		sb_show_lives();

		if ((Game_mode&GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			sb_show_score();
		}
		else
		{
			sb_show_score();
			sb_show_score_added();
		}
	}
#if defined(DXX_BUILD_DESCENT_I)
	else
		draw_player_ship(cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
#endif
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
void update_laser_weapon_info(void)
{
	if (old_weapon[0] == 0)
#if defined(DXX_BUILD_DESCENT_II)
		if (! (Players[Player_num].laser_level > MAX_LASER_LEVEL && old_laser_level <= MAX_LASER_LEVEL))
#endif
			old_weapon[0] = -1;
}

#if defined(DXX_BUILD_DESCENT_II)
static array<int, 2> overlap_dirty;

//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  viewer is object.  NULL object means give up window
//user is one of the WBU_ constants.  If rear_view_flag is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.
void do_cockpit_window_view(int win,int user)
{
	Assert(user == WBU_WEAPON || user == WBU_STATIC);
	if (user == WBU_STATIC && weapon_box_user[win] != WBU_STATIC)
		static_time[win] = 0;
	if (weapon_box_user[win] == WBU_WEAPON || weapon_box_user[win] == WBU_STATIC)
		return;		//already set
	weapon_box_user[win] = user;
	if (overlap_dirty[win]) {
		gr_set_current_canvas(NULL);
		overlap_dirty[win] = 0;
	}
}

void do_cockpit_window_view(int win,const vobjptridx_t viewer,int rear_view_flag,int user,const char *label)
{
	grs_canvas window_canv;
	static grs_canvas overlap_canv;
	object *viewer_save = Viewer;
	int boxnum;
	static int window_x,window_y;
	const gauge_box *box;
	int rear_view_save = Rear_view;
	int w,h,dx;

	box = NULL;

	window_rendered_data window;
	update_rendered_data(window, viewer, rear_view_flag);

	weapon_box_user[win] = user;						//say who's using window

	Viewer = viewer;
	Rear_view = rear_view_flag;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN)
	{
		w = HUD_SCALE_X_AR(HIRESMODE?106:44);
		h = HUD_SCALE_Y_AR(HIRESMODE?106:44);

		dx = (win==0)?-(w+(w/10)):(w/10);

		window_x = grd_curscreen->get_screen_width() / 2 + dx;
		window_y = grd_curscreen->get_screen_height() - h - (SHEIGHT / 15);

		gr_init_sub_canvas(window_canv, grd_curscreen->sc_canvas, window_x, window_y, w, h);
	}
	else {
		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
			boxnum = (COCKPIT_PRIMARY_BOX)+win;
		else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
			boxnum = (SB_PRIMARY_BOX)+win;
		else
			goto abort;

		box = &gauge_boxes[boxnum];
		gr_init_sub_canvas(window_canv, grd_curscreen->sc_canvas, HUD_SCALE_X(box->left), HUD_SCALE_Y(box->top), HUD_SCALE_X(box->right-box->left+1), HUD_SCALE_Y(box->bot-box->top+1));
	}

	gr_set_current_canvas(&window_canv);

	render_frame(0, window);

	//	HACK! If guided missile, wake up robots as necessary.
	if (viewer->type == OBJ_WEAPON) {
		// -- Used to require to be GUIDED -- if (viewer->id == GUIDEDMISS_ID)
		wake_up_rendered_objects(viewer, window);
	}

	if (label) {
		gr_set_curfont( GAME_FONT );
		if (Color_0_31_0 == -1)
			Color_0_31_0 = BM_XRGB(0,31,0);
		gr_set_fontcolor(Color_0_31_0, -1);
		gr_string(0x8000,FSPACY(1),label);
	}

	if (user == WBU_GUIDED)
		show_reticle(RET_TYPE_CROSS_V1, 0);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		int small_window_bottom,big_window_bottom,extra_part_h;

		{
			gr_setcolor(BM_XRGB(0,0,32));
			gr_ubox(0,0,grd_curcanv->cv_bitmap.bm_w,grd_curcanv->cv_bitmap.bm_h);
		}

		//if the window only partially overlaps the big 3d window, copy
		//the extra part to the visible screen

		big_window_bottom = SHEIGHT - 1;

		if (window_y > big_window_bottom) {

			//the small window is completely outside the big 3d window, so
			//copy it to the visible screen

			gr_set_current_canvas(NULL);

			gr_bitmap(window_x,window_y,window_canv.cv_bitmap);

			overlap_dirty[win] = 1;
		}
		else {

			small_window_bottom = window_y + window_canv.cv_bitmap.bm_h - 1;

			extra_part_h = small_window_bottom - big_window_bottom;

			if (extra_part_h > 0) {
				gr_init_sub_canvas(overlap_canv, window_canv, 0, window_canv.cv_bitmap.bm_h-extra_part_h, window_canv.cv_bitmap.bm_w, extra_part_h);
				gr_set_current_canvas(NULL);
				gr_bitmap(window_x,big_window_bottom+1,overlap_canv.cv_bitmap);
				overlap_dirty[win] = 1;
			}
		}
	}
	else {

		gr_set_current_canvas(NULL);
	}

	//force redraw when done
	old_weapon[win] = -1;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
		draw_wbu_overlay();

abort:;

	Viewer = viewer_save;

	Rear_view = rear_view_save;
}
#endif
