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
 * Inferno gauge drivers
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"
#include "menu.h"
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "hostage.h"
#include "bm.h"
#include "text.h"
#include "powerup.h"
#include "sounds.h"
#include "multi.h"
#include "network.h"
#include "endlevel.h"
#include "wall.h"
#include "text.h"
#include "render.h"
#include "piggy.h"
#include "playsave.h"
#include "rle.h"
#include "byteswap.h"
#ifdef OGL
#include "ogl_init.h"
#endif

//bitmap numbers for gauges
#define GAUGE_SHIELDS			0		//0..9, in decreasing order (100%,90%...0%)
#define GAUGE_INVULNERABLE		10		//10..19
#define N_INVULNERABLE_FRAMES		10
#define GAUGE_SPEED		   	20		//unused
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
#define GAUGE_SHIPS_LAST		45
#define RETICLE_CROSS			46
#define RETICLE_PRIMARY			48
#define RETICLE_SECONDARY		51
#define RETICLE_LAST			55
#define GAUGE_HOMING_WARNING_ON		56
#define GAUGE_HOMING_WARNING_OFF	57
#define SML_RETICLE_CROSS		58
#define SML_RETICLE_PRIMARY		60
#define SML_RETICLE_SECONDARY		63
#define SML_RETICLE_LAST		67
#define KEY_ICON_BLUE			68
#define KEY_ICON_YELLOW			69
#define KEY_ICON_RED			70

//Coordinats for gauges
// cockpit keys

#define GAUGE_BLUE_KEY_X_L		45
#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_X_H		91
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		(HIRESMODE?GAUGE_BLUE_KEY_X_H:GAUGE_BLUE_KEY_X_L)
#define GAUGE_BLUE_KEY_Y		(HIRESMODE?GAUGE_BLUE_KEY_Y_H:GAUGE_BLUE_KEY_Y_L)

#define GAUGE_GOLD_KEY_X_L		44
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_X_H		89
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		(HIRESMODE?GAUGE_GOLD_KEY_X_H:GAUGE_GOLD_KEY_X_L)
#define GAUGE_GOLD_KEY_Y		(HIRESMODE?GAUGE_GOLD_KEY_Y_H:GAUGE_GOLD_KEY_Y_L)

#define GAUGE_RED_KEY_X_L		43
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X_H		87
#define GAUGE_RED_KEY_Y_H		417
#define GAUGE_RED_KEY_X			(HIRESMODE?GAUGE_RED_KEY_X_H:GAUGE_RED_KEY_X_L)
#define GAUGE_RED_KEY_Y			(HIRESMODE?GAUGE_RED_KEY_Y_H:GAUGE_RED_KEY_Y_L)

// status bar keys

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

// cockpit enery gauges

#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8

#define LEFT_ENERGY_GAUGE_X_H 	137
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21

#define LEFT_ENERGY_GAUGE_X 	(HIRESMODE?LEFT_ENERGY_GAUGE_X_H:LEFT_ENERGY_GAUGE_X_L)
#define LEFT_ENERGY_GAUGE_Y 	(HIRESMODE?LEFT_ENERGY_GAUGE_Y_H:LEFT_ENERGY_GAUGE_Y_L)
#define LEFT_ENERGY_GAUGE_W 	(HIRESMODE?LEFT_ENERGY_GAUGE_W_H:LEFT_ENERGY_GAUGE_W_L)
#define LEFT_ENERGY_GAUGE_H 	(HIRESMODE?LEFT_ENERGY_GAUGE_H_H:LEFT_ENERGY_GAUGE_H_L)

#define RIGHT_ENERGY_GAUGE_X 	(HIRESMODE?380:190)
#define RIGHT_ENERGY_GAUGE_Y 	(HIRESMODE?314:131)
#define RIGHT_ENERGY_GAUGE_W 	(HIRESMODE?133:64)
#define RIGHT_ENERGY_GAUGE_H 	(HIRESMODE?21:8)

// sb energy gauge

#define SB_ENERGY_GAUGE_X 		(HIRESMODE?196:98)
#define SB_ENERGY_GAUGE_Y 		(HIRESMODE?390:155)
#define SB_ENERGY_GAUGE_W 		(HIRESMODE?32:16)
#define SB_ENERGY_GAUGE_H 		(HIRESMODE?82:41)

#define SB_ENERGY_NUM_X 		(SB_ENERGY_GAUGE_X+(HIRESMODE?4:2))
#define SB_ENERGY_NUM_Y 		(HIRESMODE?457:190)

#define SHIELD_GAUGE_X 			(HIRESMODE?292:146)
#define SHIELD_GAUGE_Y			(HIRESMODE?374:155)
#define SHIELD_GAUGE_W 			(HIRESMODE?70:35)
#define SHIELD_GAUGE_H			(HIRESMODE?77:32)

#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SHIP_GAUGE_Y			(SHIELD_GAUGE_Y+(HIRESMODE?10:5))

#define SB_SHIELD_GAUGE_X 		(HIRESMODE?247:123)		//139
#define SB_SHIELD_GAUGE_Y 		(HIRESMODE?395:163)

#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?11:5))
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+(HIRESMODE?10:5))

#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+(HIRESMODE?21:12))	//151
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-(HIRESMODE?16:7))			//156 -- MWA used to be hard coded to 156

#define NUMERICAL_GAUGE_X		(HIRESMODE?308:154)
#define NUMERICAL_GAUGE_Y		(HIRESMODE?316:130)
#define NUMERICAL_GAUGE_W		(HIRESMODE?38:19)
#define NUMERICAL_GAUGE_H		(HIRESMODE?55:22)

#define PRIMARY_W_PIC_X			(HIRESMODE?135:64)
#define PRIMARY_W_PIC_Y			(HIRESMODE?370:154)
#define PRIMARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?182:87)
#define PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define PRIMARY_AMMO_X			HUD_SCALE_X(HIRESMODE?186:(96-3))
#define PRIMARY_AMMO_Y			HUD_SCALE_Y(HIRESMODE?420:171)

#define SECONDARY_W_PIC_X		(HIRESMODE?405:234)
#define SECONDARY_W_PIC_Y		(HIRESMODE?370:154)
#define SECONDARY_W_TEXT_X		HUD_SCALE_X(HIRESMODE?462:207)
#define SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?400:157)
#define SECONDARY_AMMO_X		HUD_SCALE_X(HIRESMODE?475:213)
#define SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?425:171)

#define SB_LIVES_X			(HIRESMODE?550:266)
#define SB_LIVES_Y			(HIRESMODE?450:185)
#define SB_LIVES_LABEL_X		(HIRESMODE?475:237)
#define SB_LIVES_LABEL_Y		(SB_LIVES_Y+1)

#define SB_SCORE_RIGHT_L		301
#define SB_SCORE_RIGHT_H		605
#define SB_SCORE_RIGHT			(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)

#define SB_SCORE_Y			(HIRESMODE?398:158)
#define SB_SCORE_LABEL_X		(HIRESMODE?475:237)

#define SB_SCORE_ADDED_RIGHT		(HIRESMODE?SB_SCORE_RIGHT_H:SB_SCORE_RIGHT_L)
#define SB_SCORE_ADDED_Y		(HIRESMODE?413:165)

#define HOMING_WARNING_X		HUD_SCALE_X(HIRESMODE?14:7)
#define HOMING_WARNING_Y		HUD_SCALE_Y(HIRESMODE?415:171)

#define BOMB_COUNT_X			(HIRESMODE?468:210)
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
#define PRIMARY_W_BOX_RIGHT_H		241
#define PRIMARY_W_BOX_BOT_H		(PRIMARY_W_BOX_TOP_H+106)		//470

#define PRIMARY_W_BOX_LEFT		(HIRESMODE?PRIMARY_W_BOX_LEFT_H:PRIMARY_W_BOX_LEFT_L)
#define PRIMARY_W_BOX_TOP		(HIRESMODE?PRIMARY_W_BOX_TOP_H:PRIMARY_W_BOX_TOP_L)
#define PRIMARY_W_BOX_RIGHT		(HIRESMODE?PRIMARY_W_BOX_RIGHT_H:PRIMARY_W_BOX_RIGHT_L)
#define PRIMARY_W_BOX_BOT		(HIRESMODE?PRIMARY_W_BOX_BOT_H:PRIMARY_W_BOX_BOT_L)

#define SECONDARY_W_BOX_LEFT_L		202	//207
#define SECONDARY_W_BOX_TOP_L		151
#define SECONDARY_W_BOX_RIGHT_L		264	//(SECONDARY_W_BOX_LEFT+54)
#define SECONDARY_W_BOX_BOT_L		(SECONDARY_W_BOX_TOP_L+42)

#define SECONDARY_W_BOX_LEFT_H		403
#define SECONDARY_W_BOX_TOP_H		364
#define SECONDARY_W_BOX_RIGHT_H		531
#define SECONDARY_W_BOX_BOT_H		(SECONDARY_W_BOX_TOP_H+106)		//470

#define SECONDARY_W_BOX_LEFT		(HIRESMODE?SECONDARY_W_BOX_LEFT_H:SECONDARY_W_BOX_LEFT_L)
#define SECONDARY_W_BOX_TOP		(HIRESMODE?SECONDARY_W_BOX_TOP_H:SECONDARY_W_BOX_TOP_L)
#define SECONDARY_W_BOX_RIGHT		(HIRESMODE?SECONDARY_W_BOX_RIGHT_H:SECONDARY_W_BOX_RIGHT_L)
#define SECONDARY_W_BOX_BOT		(HIRESMODE?SECONDARY_W_BOX_BOT_H:SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_TOP_L		154
#define SB_PRIMARY_W_BOX_RIGHT_L	(SB_PRIMARY_W_BOX_LEFT_L+53)
#define SB_PRIMARY_W_BOX_BOT_L		(195)

#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H	179
#define SB_PRIMARY_W_BOX_BOT_H		473

#define SB_PRIMARY_W_BOX_LEFT		(HIRESMODE?SB_PRIMARY_W_BOX_LEFT_H:SB_PRIMARY_W_BOX_LEFT_L)
#define SB_PRIMARY_W_BOX_TOP		(HIRESMODE?SB_PRIMARY_W_BOX_TOP_H:SB_PRIMARY_W_BOX_TOP_L)
#define SB_PRIMARY_W_BOX_RIGHT		(HIRESMODE?SB_PRIMARY_W_BOX_RIGHT_H:SB_PRIMARY_W_BOX_RIGHT_L)
#define SB_PRIMARY_W_BOX_BOT		(HIRESMODE?SB_PRIMARY_W_BOX_BOT_H:SB_PRIMARY_W_BOX_BOT_L)

#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_TOP_L	154
#define SB_SECONDARY_W_BOX_RIGHT_L	(SB_SECONDARY_W_BOX_LEFT_L+54)
#define SB_SECONDARY_W_BOX_BOT_L	(153+43)

#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H	381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H	473

#define SB_SECONDARY_W_BOX_LEFT		(HIRESMODE?SB_SECONDARY_W_BOX_LEFT_H:SB_SECONDARY_W_BOX_LEFT_L)	//210
#define SB_SECONDARY_W_BOX_TOP		(HIRESMODE?SB_SECONDARY_W_BOX_TOP_H:SB_SECONDARY_W_BOX_TOP_L)
#define SB_SECONDARY_W_BOX_RIGHT	(HIRESMODE?SB_SECONDARY_W_BOX_RIGHT_H:SB_SECONDARY_W_BOX_RIGHT_L)
#define SB_SECONDARY_W_BOX_BOT		(HIRESMODE?SB_SECONDARY_W_BOX_BOT_H:SB_SECONDARY_W_BOX_BOT_L)

#define SB_PRIMARY_W_PIC_X		(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_PRIMARY_W_TEXT_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?50:24))	//(51+23)
#define SB_PRIMARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?390:157)
#define SB_PRIMARY_AMMO_X		HUD_SCALE_X(SB_PRIMARY_W_BOX_LEFT+(HIRESMODE?(38+20):30))	//((SB_PRIMARY_W_BOX_LEFT+33)-3)	//(51+32)
#define SB_PRIMARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?410:171)

#define SB_SECONDARY_W_PIC_X		(HIRESMODE?385:(SB_SECONDARY_W_BOX_LEFT+27))	//(212+27)
#define SB_SECONDARY_W_PIC_Y		(HIRESMODE?382:154)
#define SB_SECONDARY_W_TEXT_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+2)	//212
#define SB_SECONDARY_W_TEXT_Y		HUD_SCALE_Y(HIRESMODE?389:157)
#define SB_SECONDARY_AMMO_X		HUD_SCALE_X(SB_SECONDARY_W_BOX_LEFT+(HIRESMODE?(14):11))	//(212+9)
#define SB_SECONDARY_AMMO_Y		HUD_SCALE_Y(HIRESMODE?414:171)

#define WS_SET				0 //in correct state
#define WS_FADING_OUT			1
#define WS_FADING_IN			2
#define FADE_SCALE			(2*i2f(GR_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)
#define MAX_SHOWN_LIVES 4

#define COCKPIT_PRIMARY_BOX		(!HIRESMODE?0:4)
#define COCKPIT_SECONDARY_BOX		(!HIRESMODE?1:5)
#define SB_PRIMARY_BOX			(!HIRESMODE?2:6)
#define SB_SECONDARY_BOX		(!HIRESMODE?3:7)

// scaling gauges
#define BASE_WIDTH (HIRESMODE? 640 : 320)
#define BASE_HEIGHT	(HIRESMODE? 480 : 200)
#ifdef OGL
#define HUD_SCALE_X(x)		((int) ((double) (x) * ((double)grd_curscreen->sc_w/BASE_WIDTH) + 0.5))
#define HUD_SCALE_Y(y)		((int) ((double) (y) * ((double)grd_curscreen->sc_h/BASE_HEIGHT) + 0.5))
#define HUD_SCALE_X_AR(x)	(HUD_SCALE_X(100) > HUD_SCALE_Y(100) ? HUD_SCALE_Y(x) : HUD_SCALE_X(x))
#define HUD_SCALE_Y_AR(y)	(HUD_SCALE_Y(100) > HUD_SCALE_X(100) ? HUD_SCALE_X(y) : HUD_SCALE_Y(y))
#else
#define HUD_SCALE_X(x)		(x)
#define HUD_SCALE_Y(y)		(y)
#define HUD_SCALE_X_AR(x)	(x)
#define HUD_SCALE_Y_AR(y)	(y)
#endif

bitmap_index Gauges[MAX_GAUGE_BMS_MAC]; // Array of all gauge bitmaps.
grs_bitmap deccpt;
grs_bitmap *WinBoxOverlay[2] = { NULL, NULL }; // Overlay subbitmaps for both weapon boxes

static int score_display;
static fix score_time;
static int old_score		= -1;
static int old_energy		= -1;
static int old_shields		= -1;
static int old_flags		= -1;
static int old_weapon[2]	= {-1, -1};
static int old_ammo_count[2]	= {-1, -1};
static int old_cloak		=  0;
static int old_lives		= -1;
static int old_prox		= -1;
static int invulnerable_frame = 0;
static int cloak_fade_state;		//0=steady, -1 fading out, 1 fading in 
int weapon_box_states[2];
fix weapon_box_fade_values[2];
int Color_0_31_0 = -1;
fix Last_warning_beep_time = 0; // Time we last played homing missile warning beep.
int Last_homing_warning_shown=-1;
extern int HUD_nmessages, hud_first;
extern char HUD_messages[HUD_MAX_NUM][HUD_MESSAGE_LENGTH+5]; 

typedef struct gauge_box {
	int left,top;
	int right,bot;		//maximal box
} gauge_box;

//first two are primary & secondary
//seconds two are the same for the status bar
gauge_box gauge_boxes[] = {
	
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

//store delta x values from left of box
span weapon_window_left[] = {
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
span weapon_window_right[] = {
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
span weapon_window_left_hires[] = {
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
span weapon_window_right_hires[] = {
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

inline void hud_bitblt_free (int x, int y, int w, int h, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,w,h,bm,-1,F1_0);
#else
	gr_ubitmapm(x, y, bm);
#endif
}

inline void hud_bitblt (int x, int y, grs_bitmap *bm)
{
#ifdef OGL
	ogl_ubitmapm_cs (x,y,HUD_SCALE_X (bm->bm_w),HUD_SCALE_Y (bm->bm_h),bm,-1,F1_0);
#else
	gr_ubitmapm(x, y, bm);
#endif
}

void hud_show_score()
{
	char	score_str[20];
	int	w, h, aw;

	if ((HUD_nmessages > 0) && (strlen(HUD_messages[hud_first]) > 38))
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

	gr_printf(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), FSPACY(1), score_str);
}

void hud_show_score_added()
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

		if (Cheats_enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_printf(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(12), LINE_SPACING+FSPACY(1), score_str);
	} else {
		score_time = 0;
		score_display = 0;
	}
}

void sb_show_score()
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

	gr_printf(x,y,score_str);
}

void sb_show_score_added()
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

		if (Cheats_enabled)
			sprintf(score_str, "%s", TXT_CHEATER);
		else
			sprintf(score_str, "%5d", score_display);

		gr_get_string_size(score_str, &w, &h, &aw );
		x = HUD_SCALE_X(SB_SCORE_ADDED_RIGHT)-w-FSPACX(1);
		gr_set_fontcolor(BM_XRGB(0, color, 0),-1 );
		gr_printf(x, HUD_SCALE_Y(SB_SCORE_ADDED_Y), score_str);
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
	fix	beep_delay;

	if (Endlevel_sequence || Player_is_dead)
		return;

	if (Players[Player_num].homing_object_dist >= 0) {
		beep_delay = Players[Player_num].homing_object_dist/128;
		if (beep_delay > F1_0)
			beep_delay = F1_0;
		else if (beep_delay < F1_0/8)
			beep_delay = F1_0/8;

		if (GameTime - Last_warning_beep_time > beep_delay/2 || Last_warning_beep_time > GameTime) {
			digi_play_sample( SOUND_HOMING_WARNING, F1_0 );
			Last_warning_beep_time = GameTime;
		}
	}
}

//	-----------------------------------------------------------------------------
void show_homing_warning(void)
{
	if ((PlayerCfg.CockpitMode == CM_STATUS_BAR) || (Endlevel_sequence)) {
		if (Last_homing_warning_shown == 1) {
			PIGGY_PAGE_IN( Gauges[GAUGE_HOMING_WARNING_OFF] );
			hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
			Last_homing_warning_shown = 0;
		}
		return;
	}

	gr_set_current_canvas( NULL );

	if (Players[Player_num].homing_object_dist >= 0) {

		if (GameTime & 0x4000) {
			if (Last_homing_warning_shown != 1) {
				PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_ON]);
				hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_ON].index]);
				Last_homing_warning_shown = 1;
			}
		} else {
			if (Last_homing_warning_shown != 0) {
				PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
				hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
				Last_homing_warning_shown = 0;
			}
		}
	} else if (Last_homing_warning_shown != 0) {
		PIGGY_PAGE_IN(Gauges[GAUGE_HOMING_WARNING_OFF]);
		hud_bitblt( HOMING_WARNING_X, HOMING_WARNING_Y, &GameBitmaps[Gauges[GAUGE_HOMING_WARNING_OFF].index]);
		Last_homing_warning_shown = 0;
	}

}

void hud_show_homing_warning(void)
{
	if (Players[Player_num].homing_object_dist >= 0) {
		if (GameTime & 0x4000) {
			gr_set_curfont( GAME_FONT );
			gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
			gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h-LINE_SPACING,TXT_LOCK);
		}
	}
}

void hud_show_keys(void)
{
	grs_bitmap *blue,*yellow,*red;
	int y=HUD_SCALE_Y_AR(GameBitmaps[Gauges[GAUGE_LIVES].index].bm_h+2)+FSPACY(1);

	PIGGY_PAGE_IN(Gauges[KEY_ICON_BLUE]);
	PIGGY_PAGE_IN(Gauges[KEY_ICON_YELLOW]);
	PIGGY_PAGE_IN(Gauges[KEY_ICON_RED]);

	blue=&GameBitmaps[Gauges[KEY_ICON_BLUE].index];
	yellow=&GameBitmaps[Gauges[KEY_ICON_YELLOW].index];
	red=&GameBitmaps[Gauges[KEY_ICON_RED].index];

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY)
		hud_bitblt_free(FSPACX(2),y,HUD_SCALE_X_AR(blue->bm_w),HUD_SCALE_Y_AR(blue->bm_h),blue);

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+3),y,HUD_SCALE_X_AR(yellow->bm_w),HUD_SCALE_Y_AR(yellow->bm_h),yellow);

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)
		hud_bitblt_free(FSPACX(2)+HUD_SCALE_X_AR(blue->bm_w+yellow->bm_w+6),y,HUD_SCALE_X_AR(red->bm_w),HUD_SCALE_Y_AR(red->bm_h),red);
}

void hud_show_energy(void)
{
	if (PlayerCfg.HudMode<2){
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		if (Game_mode & GM_MULTI)
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-(LINE_SPACING*5)),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
		else
		     gr_printf(FSPACX(1), (grd_curcanv->cv_bitmap.bm_h-LINE_SPACING),"%s: %i", TXT_ENERGY, f2ir(Players[Player_num].energy));
	}

	if (Newdemo_state==ND_STATE_RECORDING ) {
		int energy = f2ir(Players[Player_num].energy);

		if (energy != old_energy) {
#ifdef SHAREWARE
			newdemo_record_player_energy(energy);
#else
			newdemo_record_player_energy(old_energy, energy);
#endif
			old_energy = energy;
	 	}
	}
}

void show_bomb_count(int x,int y,int bg_color,int always_show,int right_align)
{
	int bomb,count,countx,w=0,h=0,aw=0;
	char txt[5],*t;

	bomb = PROXIMITY_INDEX;
	count = Players[Player_num].secondary_ammo[bomb];

	#ifndef RELEASE
	count = min(count,99);	//only have room for 2 digits - cheating give 200
	#endif

	countx = (bomb==PROXIMITY_INDEX)?count:-count;

	if (always_show && count == 0)		//no bombs, draw nothing on HUD
		return;

	if (count)
		gr_set_fontcolor(gr_find_closest_color(55,0,0),bg_color);
	else
		gr_set_fontcolor(bg_color,bg_color);	//erase by drawing in background color

	sprintf(txt,"B:%02d",count);

	while ((t=strchr(txt,'1')) != NULL)
		*t = '\x84';	//convert to wide '1'

	if (right_align)
		gr_get_string_size(txt, &w, &h, &aw );

	gr_string(x-w,y,txt);
}

void hud_show_weapons_mode(int type,int vertical,int x,int y){
	int i,w,h,aw;
	char weapon_str[10];
	if (vertical){
		y=y+(LINE_SPACING*4);
	}
	if (type==0){
		for (i=4;i>=0;i--){
			if (Primary_weapon==i)
                             gr_set_fontcolor(BM_XRGB(20,0,0),-1);
			else{
				if (player_has_weapon(i,0) & HAS_WEAPON_FLAG)
				     gr_set_fontcolor(BM_XRGB(0,15,0),-1);
				else
				     gr_set_fontcolor(BM_XRGB(3,3,3),-1);
			}
			switch(i){
			  case 0:
				sprintf(weapon_str,"%c%i",
					  (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)?'Q':'L',
					  Players[Player_num].laser_level+1);
				break;
			  case 1:
				if (PlayerCfg.CockpitMode==CM_FULL_SCREEN)
					sprintf(weapon_str,"V%i", f2i(Players[Player_num].primary_ammo[1] * VULCAN_AMMO_SCALE));
				else
					sprintf(weapon_str,"V");
				break;
			  case 2:
				sprintf(weapon_str,"S");break;
			  case 3:
				sprintf(weapon_str,"P");break;
			  case 4:
				sprintf(weapon_str,"F");break;
				
			}
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACX(2);
			}else
			     x-=w+FSPACY(3);
			gr_printf(x, y, weapon_str);
		}
	}else{
		for (i=4;i>=0;i--){
			if (Secondary_weapon==i)
                             gr_set_fontcolor(BM_XRGB(20,0,0),-1);
			else{
				if (Players[Player_num].secondary_ammo[i]>0)
				     gr_set_fontcolor(BM_XRGB(0,15,0),-1);
				else
				     gr_set_fontcolor(BM_XRGB(0,6,0),-1);
			}
			sprintf(weapon_str,"%i",Players[Player_num].secondary_ammo[i]);
			gr_get_string_size(weapon_str, &w, &h, &aw );
			if (vertical){
				y-=h+FSPACX(2);
			}else
			     x-=w+FSPACY(3);
			gr_printf(x, y, weapon_str);
		}
	}
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
}

void hud_show_weapons(void)
{
	int	y;

	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	y = grd_curcanv->cv_bitmap.bm_h;

	if (Game_mode & GM_MULTI)
		y -= LINE_SPACING*4;

	if (PlayerCfg.HudMode==1){
		hud_show_weapons_mode(0,0,grd_curcanv->cv_bitmap.bm_w,y-(LINE_SPACING*2));
		hud_show_weapons_mode(1,0,grd_curcanv->cv_bitmap.bm_w,y-LINE_SPACING);
	}
	else if (PlayerCfg.HudMode==2){
		int x1,x2;
		int	w, aw;
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
		char    weapon_str[32];
		int	w, h, aw;

		switch (Primary_weapon) {
			case 0:
				if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
					sprintf(weapon_str, "%s %s %i", TXT_QUAD, TXT_LASER, Players[Player_num].laser_level+1);
				else
					sprintf(weapon_str, "%s %i", TXT_LASER, Players[Player_num].laser_level+1);
				break;
			case 1:
				sprintf(weapon_str, "%s: %i", TXT_W_VULCAN_S, f2i(Players[Player_num].primary_ammo[Primary_weapon] * VULCAN_AMMO_SCALE));
				break;
			case 2:
				strcpy(weapon_str, TXT_W_SPREADFIRE_S);
				break;
#ifndef SHAREWARE
			case 3:
				strcpy(weapon_str, TXT_W_PLASMA_S);
				break;
			case 4:
				strcpy(weapon_str, TXT_W_FUSION_S);
				break;
#endif
		}
		
		gr_get_string_size(weapon_str, &w, &h, &aw );
		gr_printf(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-(LINE_SPACING*2), weapon_str);//originally y-8

		switch (Secondary_weapon) {
			case 0:		strcpy(weapon_str, TXT_CONCUSSION);	break;
			case 1:		strcpy(weapon_str, TXT_HOMING);		break;
			case 2:		strcpy(weapon_str, TXT_PROXBOMB   );	break;
#ifndef SHAREWARE
			case 3:		strcpy(weapon_str, TXT_SMART);		break;
			case 4:		strcpy(weapon_str, TXT_MEGA);		break;
#endif
			default:	Int3();	weapon_str[0] = 0;		break;
		}

		sprintf(weapon_str, "%s %d",weapon_str,Players[Player_num].secondary_ammo[Secondary_weapon]);
		gr_get_string_size(weapon_str, &w,&h, &aw );
		gr_printf(grd_curcanv->cv_bitmap.bm_w-w-FSPACX(1), y-LINE_SPACING, weapon_str);

		show_bomb_count(grd_curcanv->cv_bitmap.bm_w-FSPACX(1), y-(LINE_SPACING*3),-1,1, 1);
	}

#ifndef SHAREWARE
	if (Primary_weapon == VULCAN_INDEX) {
		if (Players[Player_num].primary_ammo[Primary_weapon] != old_ammo_count[0]) {
			if (Newdemo_state == ND_STATE_RECORDING)
			     newdemo_record_primary_ammo(old_ammo_count[0], Players[Player_num].primary_ammo[Primary_weapon]);
			old_ammo_count[0] = Players[Player_num].primary_ammo[Primary_weapon];
		}
	}
	if (Players[Player_num].secondary_ammo[Secondary_weapon] != old_ammo_count[1]) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_secondary_ammo(old_ammo_count[1], Players[Player_num].secondary_ammo[Secondary_weapon]);
		old_ammo_count[1] = Players[Player_num].secondary_ammo[Secondary_weapon];
	}
#endif
}

void hud_show_cloak_invuln(void)
{
	gr_set_fontcolor(BM_XRGB(0,31,0),-1 );

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*7;
		else
			y -= LINE_SPACING*4;

		if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime > F1_0*3 ||
			Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime < 0 || 
			GameTime & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s", TXT_CLOAKED);
		}
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		int	y = grd_curcanv->cv_bitmap.bm_h;

		if (Game_mode & GM_MULTI)
			y -= LINE_SPACING*10;
		else
			y -= LINE_SPACING*5;

		if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime > F1_0*4 ||
			Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime < 0 || 
			GameTime & 0x8000)
		{
			gr_printf(FSPACX(1), y, "%s", TXT_INVULNERABLE);
		}
	}

}

void hud_show_shield(void)
{
	if (PlayerCfg.HudMode<2){
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

	if (Newdemo_state==ND_STATE_RECORDING ) {
		int shields = f2ir(Players[Player_num].shields);

		if (shields != old_shields) {
#ifdef SHAREWARE
			newdemo_record_player_shields(shields);
#else
			newdemo_record_player_shields(old_shields, shields);
#endif
			old_shields = shields;
		}
	}
}

//draw the icons for number of lives
void hud_show_lives()
{
	int x;

	if ((HUD_nmessages > 0) && (strlen(HUD_messages[hud_first]) > 38))
		return;

	if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT)
		x = HUD_SCALE_X(7);
	else
		x = FSPACX(2);

	if (Game_mode & GM_MULTI) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_printf(x, FSPACY(1), "%s: %d", TXT_DEATHS, Players[Player_num].net_killed_total);
	} 
	else if (Players[Player_num].lives > 1)  {
		grs_bitmap *bm;
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
		bm=&GameBitmaps[Gauges[GAUGE_LIVES].index];
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		hud_bitblt_free(x,FSPACY(1),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X_AR(bm->bm_w)+x, FSPACY(1), " x %d", Players[Player_num].lives-1);
	}
}

void sb_show_lives()
{
	int x,y;
	grs_bitmap * bm = &GameBitmaps[Gauges[GAUGE_LIVES].index];
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
		static int last_x[4] = {SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_L,SB_SCORE_RIGHT_H,SB_SCORE_RIGHT_H};
		int x;

		sprintf(killed_str, "%5d", Players[Player_num].net_killed_total);
		gr_get_string_size(killed_str, &w, &h, &aw);
		gr_setcolor(BM_XRGB(0,0,0));
		gr_rect(last_x[HIRESMODE], HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y)+LINE_SPACING);
		gr_set_fontcolor(BM_XRGB(0,20,0),-1);
		x = HUD_SCALE_X(SB_SCORE_RIGHT)-w-FSPACX(1);
		gr_printf(x, HUD_SCALE_Y(y), killed_str);
		last_x[HIRESMODE] = x;
		return;
	}

	//erase old icons
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(HUD_SCALE_X(x), HUD_SCALE_Y(y), HUD_SCALE_X(SB_SCORE_RIGHT), HUD_SCALE_Y(y+bm->bm_h));

	if (Players[Player_num].lives-1 > 0) {
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		PIGGY_PAGE_IN(Gauges[GAUGE_LIVES]);
		hud_bitblt_free(HUD_SCALE_X(x),HUD_SCALE_Y(y),HUD_SCALE_X_AR(bm->bm_w),HUD_SCALE_Y_AR(bm->bm_h),bm);
		gr_printf(HUD_SCALE_X(x)+HUD_SCALE_X_AR(bm->bm_w), HUD_SCALE_Y(y), " x %d", Players[Player_num].lives-1);
	}
}

#ifndef RELEASE

extern int Piggy_bitmap_cache_next;

void show_time()
{
	int secs = f2i(Players[Player_num].time_level) % 60;
	int mins = f2i(Players[Player_num].time_level) / 60;

	gr_set_curfont( GAME_FONT );

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(Color_0_31_0, -1 );

	gr_printf(SWIDTH-FSPACX(25),(SHEIGHT/2),"%d:%02d", mins, secs);
}
#endif

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

void add_points_to_score(int points) 
{
	int prev_score;

	score_time += f1_0*2;
	score_display += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	if (points == 0 || Cheats_enabled)
		return;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

#ifndef SHAREWARE
#ifdef NETWORK
	if (Game_mode & GM_MULTI_COOP)
		multi_send_score();
#endif
#endif

	if (Game_mode & GM_MULTI)
		return;

	if (Players[Player_num].score/EXTRA_SHIP_SCORE != prev_score/EXTRA_SHIP_SCORE) {
		int snd;
		Players[Player_num].lives += Players[Player_num].score/EXTRA_SHIP_SCORE - prev_score/EXTRA_SHIP_SCORE;
		powerup_basic(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

void add_bonus_points_to_score(int points) 
{
	int prev_score;

	if (points == 0 || Cheats_enabled)
		return;

	prev_score=Players[Player_num].score;

	Players[Player_num].score += points;


#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);
#endif

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
void cockpit_decode_alpha(grs_bitmap *bm)
{

	int i=0,x=0,y=0;
	static ubyte *cur=NULL;
	static unsigned char cockpitbuf[1024*1024];

	// check if we processed this bitmap already
	if (cur==bm->bm_data)
		return;

	memset(cockpitbuf,0,1024*1024);

	// decode the bitmap
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = cockpitbuf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}

	// add alpha color to the pixels which are inside the window box spans
	for (y=0;y<bm->bm_h;y++)
	{
		for (x=0;x<bm->bm_w;x++)
		{
			if (y >= (HIRESMODE?364:151) && y <= (HIRESMODE?469:193) && ((x >= WinBoxLeft[y-(HIRESMODE?364:151)].l && x <= WinBoxLeft[y-(HIRESMODE?364:151)].r) ||  (x >=WinBoxRight[y-(HIRESMODE?364:151)].l && x <= WinBoxRight[y-(HIRESMODE?364:151)].r)))
				cockpitbuf[i]=TRANSPARENCY_COLOR;
			i++;
		}
	}
#ifdef OGL
	ogl_freebmtexture(bm);
#endif
	gr_init_bitmap (&deccpt, 0, 0, 0, bm->bm_w, bm->bm_h, bm->bm_w, cockpitbuf);
	gr_set_transparent(&deccpt,1);
#ifdef OGL
	ogl_ubitmapm_cs (0, 0, -1, -1, &deccpt, 255, F1_0); // render one time to init the texture
#endif
	if (WinBoxOverlay[0] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[0]);
	if (WinBoxOverlay[1] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[1]);
	WinBoxOverlay[0] = gr_create_sub_bitmap(&deccpt,(PRIMARY_W_BOX_LEFT)-2,(PRIMARY_W_BOX_TOP)-2,(PRIMARY_W_BOX_RIGHT-PRIMARY_W_BOX_LEFT+4),(PRIMARY_W_BOX_BOT-PRIMARY_W_BOX_TOP+4));
	WinBoxOverlay[1] = gr_create_sub_bitmap(&deccpt,(SECONDARY_W_BOX_LEFT)-2,(SECONDARY_W_BOX_TOP)-2,(SECONDARY_W_BOX_RIGHT-SECONDARY_W_BOX_LEFT)+4,(SECONDARY_W_BOX_BOT-SECONDARY_W_BOX_TOP)+4);

	cur=bm->bm_data;
}

void draw_wbu_overlay()
{
	hud_bitblt(HUD_SCALE_X(PRIMARY_W_BOX_LEFT-2),HUD_SCALE_Y(PRIMARY_W_BOX_TOP-2),WinBoxOverlay[0]);
	hud_bitblt(HUD_SCALE_X(SECONDARY_W_BOX_LEFT-2),HUD_SCALE_Y(SECONDARY_W_BOX_TOP-2),WinBoxOverlay[1]);
}

void close_gauges()
{
	if (WinBoxOverlay[0] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[0]);
	if (WinBoxOverlay[1] != NULL)
		gr_free_sub_bitmap(WinBoxOverlay[1]);
	WinBoxOverlay[0] = NULL;
	WinBoxOverlay[1] = NULL;
}

void init_gauges()
{
	if ( ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) )
		old_score = -99;
	else
		old_score		= -1;
	old_energy			= -1;
	old_shields			= -1;
	old_flags			= -1;
	old_cloak			= -1;
	old_lives			= -1;
	
	old_weapon[0] = old_weapon[1] = -1;
	old_ammo_count[0] = old_ammo_count[1] = -1;

	cloak_fade_state = 0;
}

void draw_energy_bar(int energy)
{
	int x1, x2, y;
	int not_energy = (HIRESMODE?(HUD_SCALE_X(125 - (energy*125)/100)):(HUD_SCALE_X(63 - (energy*63)/100)));
	double aplitscale=((double)(HUD_SCALE_X(65)/HUD_SCALE_Y(8))/(65/8)); //scale aplitude of energy bar to current resolution aspect

	// Draw left energy bar
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_LEFT]);
	hud_bitblt (HUD_SCALE_X(LEFT_ENERGY_GAUGE_X), HUD_SCALE_Y(LEFT_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_ENERGY_LEFT].index]);

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
	PIGGY_PAGE_IN(Gauges[GAUGE_ENERGY_RIGHT]);
	hud_bitblt (HUD_SCALE_X(RIGHT_ENERGY_GAUGE_X), HUD_SCALE_Y(RIGHT_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_ENERGY_RIGHT].index]);

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

void draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	PIGGY_PAGE_IN(Gauges[GAUGE_SHIELDS+9-bm_num]);
	hud_bitblt (HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index]);
}

#define CLOAK_FADE_WAIT_TIME  0x400

void draw_player_ship(int cloak_state,int old_cloak_state,int x, int y)
{
	static fix cloak_fade_timer=0;
	static int cloak_fade_value=GR_FADE_LEVELS-1;
	static int refade = 0;
	grs_bitmap *bm = NULL;

#ifdef NETWORK
	if (Game_mode & GM_TEAM)
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+get_team(Player_num)]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+get_team(Player_num)].index];
	}
	else
#endif
	{
		PIGGY_PAGE_IN(Gauges[GAUGE_SHIPS+Player_num]);
		bm = &GameBitmaps[Gauges[GAUGE_SHIPS+Player_num].index];
	}
	

	if (old_cloak_state==-1 && cloak_state)
			cloak_fade_value=0;

	if (!cloak_state) {
		cloak_fade_value=GR_FADE_LEVELS-1;
		cloak_fade_state = 0;
	}

	if (cloak_state==1 && old_cloak_state==0)
		cloak_fade_state = -1;

	if ((Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime < F1_0*3 && //doing "about-to-uncloak" effect
		Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime > F1_0) &&
		cloak_state)
	{
		if (cloak_fade_state==0)
			cloak_fade_state = 2;
	}
	

	if (cloak_fade_state)
		cloak_fade_timer -= FrameTime;

	while (cloak_fade_state && cloak_fade_timer < 0) {

		cloak_fade_timer += CLOAK_FADE_WAIT_TIME;
		cloak_fade_value += cloak_fade_state;

		if (cloak_fade_value >= GR_FADE_LEVELS-1) {
			cloak_fade_value = GR_FADE_LEVELS-1;
			if (cloak_fade_state == 2 && cloak_state)
				cloak_fade_state = -2;
			else
				cloak_fade_state = 0;
		}
		else if (cloak_fade_value <= 0) {
			cloak_fade_value = 0;
			if (cloak_fade_state == -2)
				cloak_fade_state = 2;
			else
				cloak_fade_state = 0;
		}
	}

	// To fade out both pages in a paged mode.
	if (refade) refade = 0;
	else if (cloak_state && old_cloak_state && !cloak_fade_state && !refade) {
		cloak_fade_state = -1;
		refade = 1;
	}

	gr_set_current_canvas(NULL);

	hud_bitblt( HUD_SCALE_X(x), HUD_SCALE_Y(y), bm);

	Gr_scanline_darkening_level = cloak_fade_value;
	gr_rect(HUD_SCALE_X(x), HUD_SCALE_Y(y), HUD_SCALE_X(x+bm->bm_w), HUD_SCALE_Y(y+bm->bm_h));
	Gr_scanline_darkening_level = GR_FADE_LEVELS;

	gr_set_current_canvas( NULL );
}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

void draw_numerical_display(int shield, int energy)
{
	int sw,sh,saw,ew,eh,eaw;

	gr_set_curfont( GAME_FONT );
#ifndef OGL
	PIGGY_PAGE_IN(Gauges[GAUGE_NUMERICAL]);
	hud_bitblt (HUD_SCALE_X(NUMERICAL_GAUGE_X), HUD_SCALE_Y(NUMERICAL_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_NUMERICAL].index]);
#endif
	// cockpit is not 100% geometric so we need to divide shield and energy X position by 1.951 which should be most accurate
	// gr_get_string_size is used so we can get the numbers finally in the correct position with sw and ew
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );
	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(sw/2), 
			(grd_curscreen->sc_h/1.365),"%d",shield);

	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf(	(grd_curscreen->sc_w/1.951)-(ew/2),
			(grd_curscreen->sc_h/1.5),"%d",energy);

	gr_set_current_canvas( NULL );
}


void draw_keys()
{
	gr_set_current_canvas( NULL );

	if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY )	{
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY]);
                hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[Gauges[GAUGE_BLUE_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_BLUE_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_BLUE_KEY_X), HUD_SCALE_Y(GAUGE_BLUE_KEY_Y), &GameBitmaps[Gauges[GAUGE_BLUE_KEY_OFF].index]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY)	{
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY]);
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[Gauges[GAUGE_GOLD_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_GOLD_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_GOLD_KEY_X), HUD_SCALE_Y(GAUGE_GOLD_KEY_Y), &GameBitmaps[Gauges[GAUGE_GOLD_KEY_OFF].index]);
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY)	{
		PIGGY_PAGE_IN( Gauges[GAUGE_RED_KEY] );
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[Gauges[GAUGE_RED_KEY].index]);
	} else {
		PIGGY_PAGE_IN(Gauges[GAUGE_RED_KEY_OFF]);
		hud_bitblt( HUD_SCALE_X(GAUGE_RED_KEY_X), HUD_SCALE_Y(GAUGE_RED_KEY_Y), &GameBitmaps[Gauges[GAUGE_RED_KEY_OFF].index]);
	}
}


void draw_weapon_info_sub(int info_index,gauge_box *box,int pic_x,int pic_y,char *name,int text_x,int text_y)
{
	grs_bitmap *bm;
	char *p;

	//clear the window
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(HUD_SCALE_X(box->left),HUD_SCALE_Y(box->top),HUD_SCALE_X(box->right),HUD_SCALE_Y(box->bot+1));

	bm=&GameBitmaps[Weapon_info[info_index].picture.index];
	Assert(bm != NULL);

	PIGGY_PAGE_IN( Weapon_info[info_index].picture );
	hud_bitblt(HUD_SCALE_X(pic_x), HUD_SCALE_Y(pic_y), bm);

	if (PlayerCfg.HudMode == 0)
	{
		gr_set_fontcolor(BM_XRGB(0,20,0),-1 );
		
		if ((p=strchr(name,'\n'))!=NULL) {
			*p=0;
			gr_printf(text_x,text_y,name);
			gr_printf(text_x,text_y+LINE_SPACING,p+1);
			*p='\n';
		} else
			gr_printf(text_x,text_y,name);
		
		// For laser, show level and quadness
		if (info_index == 0) {
			char	temp_str[7];
			
			sprintf(temp_str, "%s: 0", TXT_LVL);
			
			temp_str[5] = Players[Player_num].laser_level+1 + '0';
			
			gr_printf(text_x,text_y+LINE_SPACING, temp_str);
			
			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS) {
				strcpy(temp_str, TXT_QUAD);
				gr_printf(text_x,text_y+(LINE_SPACING*2), temp_str);
			}
		}
	}
}

void draw_weapon_info(int weapon_type,int weapon_num)
{
	int x,y;
	
#ifdef SHAREWARE
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(weapon_type, weapon_num);
#endif
	
	if (weapon_type == 0)
	{
		if (PlayerCfg.CockpitMode == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[SB_PRIMARY_BOX],SB_PRIMARY_W_PIC_X,SB_PRIMARY_W_PIC_Y,Primary_weapon_names_short[weapon_num],SB_PRIMARY_W_TEXT_X,SB_PRIMARY_W_TEXT_Y);
			x=SB_PRIMARY_AMMO_X;
			y=SB_PRIMARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(Primary_weapon_to_weapon_info[weapon_num],&gauge_boxes[COCKPIT_PRIMARY_BOX],PRIMARY_W_PIC_X,PRIMARY_W_PIC_Y,	Primary_weapon_names_short[weapon_num],PRIMARY_W_TEXT_X,PRIMARY_W_TEXT_Y);
			x=PRIMARY_AMMO_X;
			y=PRIMARY_AMMO_Y;
		}
	}
	else
	{
		if (PlayerCfg.CockpitMode == CM_STATUS_BAR)
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[SB_SECONDARY_BOX],SB_SECONDARY_W_PIC_X,SB_SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SB_SECONDARY_W_TEXT_X,SB_SECONDARY_W_TEXT_Y);
			x=SB_SECONDARY_AMMO_X;
			y=SB_SECONDARY_AMMO_Y;
		}
		else
		{
			draw_weapon_info_sub(Secondary_weapon_to_weapon_info[weapon_num],&gauge_boxes[COCKPIT_SECONDARY_BOX],SECONDARY_W_PIC_X,SECONDARY_W_PIC_Y,SECONDARY_WEAPON_NAMES_SHORT(weapon_num),SECONDARY_W_TEXT_X,SECONDARY_W_TEXT_Y);
			x=SECONDARY_AMMO_X;
			y=SECONDARY_AMMO_Y;
		}
	}
	
	if (PlayerCfg.HudMode!=0)
		hud_show_weapons_mode(weapon_type,1,x,y);
}

void draw_ammo_info(int x,int y,int ammo_count,int primary)
{
	if (PlayerCfg.HudMode!=0)
		hud_show_weapons_mode(!primary,1,x,y);
	else
	{
		gr_setcolor(BM_XRGB(0,0,0));
		gr_set_fontcolor(BM_XRGB(20,0,0),-1 );
		gr_printf(x,y,"%03d",ammo_count);
	}
}

void draw_primary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode == CM_STATUS_BAR)
		draw_ammo_info(SB_PRIMARY_AMMO_X,SB_PRIMARY_AMMO_Y,ammo_count,1);
	else
		draw_ammo_info(PRIMARY_AMMO_X,PRIMARY_AMMO_Y,ammo_count,1);
}

void draw_secondary_ammo_info(int ammo_count)
{
	if (PlayerCfg.CockpitMode == CM_STATUS_BAR)
		draw_ammo_info(SB_SECONDARY_AMMO_X,SB_SECONDARY_AMMO_Y,ammo_count,0);
	else
                draw_ammo_info(SECONDARY_AMMO_X,SECONDARY_AMMO_Y,ammo_count,0);
}

void draw_weapon_box(int weapon_type,int weapon_num)
{
	gr_set_current_canvas(NULL);

	gr_set_curfont( GAME_FONT );

	if (weapon_num != old_weapon[weapon_type] && weapon_box_states[weapon_type] == WS_SET && (old_weapon[weapon_type] != -1) && !PlayerCfg.HudMode)
	{
		weapon_box_states[weapon_type] = WS_FADING_OUT;
		weapon_box_fade_values[weapon_type]=i2f(GR_FADE_LEVELS-1);
	}
			
	if (old_weapon[weapon_type] == -1)
	{
		draw_weapon_info(weapon_type,weapon_num);
		old_weapon[weapon_type] = weapon_num;
		old_ammo_count[weapon_type]=-1;
		weapon_box_states[weapon_type] = WS_SET;
	}
	
	if (weapon_box_states[weapon_type] == WS_FADING_OUT) {
		draw_weapon_info(weapon_type,old_weapon[weapon_type]);
		old_ammo_count[weapon_type]=-1;
		weapon_box_fade_values[weapon_type] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[weapon_type] <= 0)
		{
			weapon_box_states[weapon_type] = WS_FADING_IN;
			old_weapon[weapon_type] = weapon_num;
			old_weapon[weapon_type] = weapon_num;
			weapon_box_fade_values[weapon_type] = 0;
		}
	}
	else if (weapon_box_states[weapon_type] == WS_FADING_IN)
	{
		if (weapon_num != old_weapon[weapon_type])
		{
			weapon_box_states[weapon_type] = WS_FADING_OUT;
		}
		else
		{
			draw_weapon_info(weapon_type,weapon_num);
			old_ammo_count[weapon_type]=-1;
			weapon_box_fade_values[weapon_type] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[weapon_type] >= i2f(GR_FADE_LEVELS-1)) {
				weapon_box_states[weapon_type] = WS_SET;
				old_weapon[weapon_type] = -1;
			}
		}
	}
	else
	{
		draw_weapon_info(weapon_type, weapon_num);
		old_weapon[weapon_type] = weapon_num;
		old_ammo_count[weapon_type] = -1;
	}

	
	if (weapon_box_states[weapon_type] != WS_SET)         //fade gauge
	{
		int fade_value = f2i(weapon_box_fade_values[weapon_type]);
		int boxofs = (PlayerCfg.CockpitMode==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
		
		Gr_scanline_darkening_level = fade_value;
		gr_rect(HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].left),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].top),HUD_SCALE_X(gauge_boxes[boxofs+weapon_type].right),HUD_SCALE_Y(gauge_boxes[boxofs+weapon_type].bot));

		Gr_scanline_darkening_level = GR_FADE_LEVELS;
	}
	
	gr_set_current_canvas(NULL);

}

void draw_weapon_boxes()
{
	draw_weapon_box(0,Primary_weapon);
	
	if (weapon_box_states[0] == WS_SET)
		if (Players[Player_num].primary_ammo[Primary_weapon] != old_ammo_count[0])
			if (Primary_weapon == VULCAN_INDEX)
			{
	#ifndef SHAREWARE
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_primary_ammo(old_ammo_count[0], Players[Player_num].primary_ammo[Primary_weapon]);
	#endif
				draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
				old_ammo_count[0] = Players[Player_num].primary_ammo[Primary_weapon];
			}
	
	draw_weapon_box(1,Secondary_weapon);
	
	if (weapon_box_states[1] == WS_SET)
		if (Players[Player_num].secondary_ammo[Secondary_weapon] != old_ammo_count[1] || Players[Player_num].secondary_ammo[2] != old_prox)
		{
			old_prox=Players[Player_num].secondary_ammo[2];
	
#ifndef SHAREWARE
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_secondary_ammo(old_ammo_count[1], Players[Player_num].secondary_ammo[Secondary_weapon]);
#endif
			draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
			old_ammo_count[1] = Players[Player_num].secondary_ammo[Secondary_weapon];
		}
	
	if(PlayerCfg.HudMode!=0)
	{
		draw_primary_ammo_info(f2i(VULCAN_AMMO_SCALE * Players[Player_num].primary_ammo[Primary_weapon]));
		draw_secondary_ammo_info(Players[Player_num].secondary_ammo[Secondary_weapon]);
	}
}

void sb_draw_energy_bar(int energy)
{
	int erase_height,i;
	int ew,eh,eaw;

	PIGGY_PAGE_IN(Gauges[SB_GAUGE_ENERGY]);
	hud_bitblt( HUD_SCALE_X(SB_ENERGY_GAUGE_X), HUD_SCALE_Y(SB_ENERGY_GAUGE_Y), &GameBitmaps[Gauges[SB_GAUGE_ENERGY].index]);

	erase_height = HUD_SCALE_Y((100 - energy) * SB_ENERGY_GAUGE_H / 100);
	gr_setcolor( 0 );
	for (i=0;i<erase_height;i++)
		gr_uline( i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X-1)), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i), i2f(HUD_SCALE_X(SB_ENERGY_GAUGE_X+(SB_ENERGY_GAUGE_W))), i2f(HUD_SCALE_Y(SB_ENERGY_GAUGE_Y)+i) );

	gr_set_current_canvas( NULL );

	//draw numbers
	gr_set_fontcolor(BM_XRGB(25,18,6),-1 );
	gr_get_string_size((energy>199)?"200":(energy>99)?"100":(energy>9)?"00":"0",&ew,&eh,&eaw);
	gr_printf((grd_curscreen->sc_w/3)-(ew/2),HUD_SCALE_Y(SB_ENERGY_NUM_Y),"%d",energy);
}

void sb_draw_shield_num(int shield)
{
	grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode].index];
	int sw,sh,saw;

	//draw numbers
	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(BM_XRGB(14,14,23),-1 );

	//erase old one
	PIGGY_PAGE_IN( cockpit_bitmap[PlayerCfg.CockpitMode] );
	gr_setcolor(gr_gpixel(bm,SB_SHIELD_NUM_X,SB_SHIELD_NUM_Y-(SM_H(Game_screen_mode)-bm->bm_h)));
	gr_get_string_size((shield>199)?"200":(shield>99)?"100":(shield>9)?"00":"0",&sw,&sh,&saw);
	gr_printf((grd_curscreen->sc_w/2.267)-(sw/2),HUD_SCALE_Y(SB_SHIELD_NUM_Y),"%d",shield);
}

void sb_draw_shield_bar(int shield)
{
	int bm_num = shield>=100?9:(shield / 10);

	gr_set_current_canvas( NULL );

	PIGGY_PAGE_IN( Gauges[GAUGE_SHIELDS+9-bm_num] );
	hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_SHIELDS+9-bm_num].index]);
}

void sb_draw_keys()
{
	grs_bitmap * bm;
	int flags = Players[Player_num].flags;

	gr_set_current_canvas( NULL );
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_BLUE_KEY_Y), bm);
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_GOLD_KEY_Y), bm);
	bm = &GameBitmaps[Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF].index];
	PIGGY_PAGE_IN(Gauges[(flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF]);
	hud_bitblt( HUD_SCALE_X(SB_GAUGE_KEYS_X), HUD_SCALE_Y(SB_GAUGE_RED_KEY_Y), bm);
}

// Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void draw_invulnerable_ship()
{
	static fix time=0;

	gr_set_current_canvas( NULL );

	if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime > F1_0*4 ||
		Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime < 0 || 
		GameTime & 0x8000)
	{

		if (PlayerCfg.CockpitMode == CM_STATUS_BAR)	{
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( HUD_SCALE_X(SB_SHIELD_GAUGE_X), HUD_SCALE_Y(SB_SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index]);
		} else {
			PIGGY_PAGE_IN(Gauges[GAUGE_INVULNERABLE+invulnerable_frame]);
			hud_bitblt( HUD_SCALE_X(SHIELD_GAUGE_X), HUD_SCALE_Y(SHIELD_GAUGE_Y), &GameBitmaps[Gauges[GAUGE_INVULNERABLE+invulnerable_frame].index]);
		}

		time += FrameTime;

		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
	} else if (PlayerCfg.CockpitMode == CM_STATUS_BAR)
		sb_draw_shield_bar(f2ir(Players[Player_num].shields));
	else
		draw_shield_bar(f2ir(Players[Player_num].shields));
}

extern int Missile_gun;
extern int allowed_to_fire_laser(void);
extern int allowed_to_fire_missile(void);

rgb player_rgb[] = {	{15,15,23},
			{27,0,0},
			{0,23,0},
			{30,11,31},
			{31,16,0},
			{24,17,6},
			{14,21,12},
			{29,29,0}, };

typedef struct {
	sbyte x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy cross_offsets[4] = 		{ {-8,-5},	{-4,-2},	{-4,-2}, {-2,-1} };
xy primary_offsets[4] = 	{ {-30,14}, {-16,6},	{-15,6}, {-8, 2} };
xy secondary_offsets[4] =	{ {-24,2},	{-12,0}, {-12,1}, {-6,-2} };

//draw the reticle
void show_reticle()
{
	int x,y;
	int laser_ready,missile_ready,laser_ammo,missile_ammo;
	int cross_bm_num,primary_bm_num,secondary_bm_num;
	int use_hires_reticle,ofs;

	x = grd_curcanv->cv_bitmap.bm_w/2;
	y = grd_curcanv->cv_bitmap.bm_h/2;

	laser_ready = allowed_to_fire_laser();
	missile_ready = allowed_to_fire_missile();

	laser_ammo = player_has_weapon(Primary_weapon,0);
	missile_ammo = player_has_weapon(Secondary_weapon,1);

	primary_bm_num = (laser_ready && laser_ammo==HAS_ALL);
	secondary_bm_num = (missile_ready && missile_ammo==HAS_ALL);

	if (primary_bm_num && Primary_weapon==LASER_INDEX && (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS))
		primary_bm_num++;

	if (Secondary_weapon!=CONCUSSION_INDEX && Secondary_weapon!=HOMING_INDEX)
		secondary_bm_num += 3;
	else if (secondary_bm_num && !(Missile_gun&1))
			secondary_bm_num++;

	cross_bm_num = ((primary_bm_num > 0) || (secondary_bm_num > 0));

	Assert(primary_bm_num <= 2);
	Assert(secondary_bm_num <= 4);
	Assert(cross_bm_num <= 1);
#ifdef OGL
	if (PlayerCfg.OglReticle)
	{
		ogl_draw_reticle(cross_bm_num,primary_bm_num,secondary_bm_num);
	}
	else
#endif

	if (grd_curcanv->cv_bitmap.bm_w > 200) {
		grs_bitmap *cross, *primary, *secondary;

		use_hires_reticle = (HIRESMODE != 0);
		ofs = (use_hires_reticle?0:2);

		PIGGY_PAGE_IN(Gauges[RETICLE_CROSS + cross_bm_num]);
		cross = &GameBitmaps[Gauges[RETICLE_CROSS + cross_bm_num].index];
		hud_bitblt_free(x+HUD_SCALE_X_AR(cross_offsets[ofs].x),y+HUD_SCALE_Y_AR(cross_offsets[ofs].y), HUD_SCALE_X_AR(cross->bm_w), HUD_SCALE_Y_AR(cross->bm_h), cross);

		PIGGY_PAGE_IN(Gauges[RETICLE_PRIMARY + primary_bm_num]);
		primary = &GameBitmaps[Gauges[RETICLE_PRIMARY + primary_bm_num].index];
		hud_bitblt_free(x+HUD_SCALE_X_AR(primary_offsets[ofs].x),y+HUD_SCALE_Y_AR(primary_offsets[ofs].y), HUD_SCALE_X_AR(primary->bm_w), HUD_SCALE_Y_AR(primary->bm_h), primary);

		PIGGY_PAGE_IN(Gauges[RETICLE_SECONDARY + secondary_bm_num]);
		secondary = &GameBitmaps[Gauges[RETICLE_SECONDARY + secondary_bm_num].index];
		hud_bitblt_free(x+HUD_SCALE_X_AR(secondary_offsets[ofs].x),y+HUD_SCALE_Y_AR(secondary_offsets[ofs].y), HUD_SCALE_X_AR(secondary->bm_w), HUD_SCALE_Y_AR(secondary->bm_h), secondary);
	}

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Newdemo_state == ND_STATE_PLAYBACK) || (((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) && Show_reticle_name))
	{
		// Draw player callsign for player in sights 
		fvi_query fq;
		vms_vector orient;
		int Hit_type;
		fvi_info Hit_data;

		fq.p0 		= &ConsoleObject->pos;
		orient 		= ConsoleObject->orient.fvec;
		vm_vec_scale(&orient, F1_0*1024);
		vm_vec_add2(&orient, fq.p0);
		fq.p1 		= &orient;
		fq.rad 		= 0;
		fq.thisobjnum = ConsoleObject - Objects;
		fq.flags 	= FQ_TRANSWALL | FQ_CHECK_OBJS;
		fq.startseg	= ConsoleObject->segnum;
		fq.ignore_obj_list = NULL;

		Hit_type = find_vector_intersection(&fq, &Hit_data);
		if ((Hit_type == HIT_OBJECT) && (Objects[Hit_data.hit_object].type == OBJ_PLAYER))
		{
			// Draw callsign on HUD
			char s[CALLSIGN_LEN+1];
			int w, h, aw;
			int x1, y1;
			int pnum;
			int color_num;

			pnum = Objects[Hit_data.hit_object].id;

			if ((Game_mode & GM_TEAM) && (get_team(pnum) != get_team(Player_num)) && (Newdemo_state != ND_STATE_PLAYBACK))
				return;

			if (Game_mode & GM_TEAM)
				color_num = get_team(pnum);
			else
				color_num = pnum;
			sprintf(s, "%s", Players[pnum].callsign);
			gr_get_string_size(s, &w, &h, &aw);
			gr_set_fontcolor(BM_XRGB(player_rgb[color_num].r,player_rgb[color_num].g,player_rgb[color_num].b),-1 );
			x1 = x-(w/2);
			y1 = y+HUD_SCALE_Y(12);
			gr_string (x1, y1, s);
		}
#ifndef NDEBUG
		else if ((Hit_type == HIT_OBJECT) && (Objects[Hit_data.hit_object].type == OBJ_ROBOT))
		{
			char s[CALLSIGN_LEN+1];
			int w, h, aw;
			int x1, y1;
			int color_num = 0;

			sprintf(s, "%d", Hit_data.hit_object);
			gr_get_string_size(s, &w, &h, &aw);
			gr_set_fontcolor(BM_XRGB(player_rgb[color_num].r,player_rgb[color_num].g,player_rgb[color_num].b),-1 );
			x1 = x-(w/2);
			y1 = y+(HUD_SCALE_Y(12));
			gr_string (x1, y1, s);
		}
#endif
	}
#endif
#endif
}

#ifdef NETWORK
void hud_show_kill_list()
{
	int n_players,player_list[MAX_NUM_NET_PLAYERS];
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

	if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT) {
		save_y = y -= FSPACX(6);
		if (Game_mode & GM_MULTI_COOP)
			x1 = FSPACX(33);
		else
			x1 = FSPACX(43);
	}

	for (i=0;i<n_players;i++) {
		int player_num;
		char name[9];
		int sw,sh,aw;

		if (i>=n_left) {
			if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT)
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(53);
			else
				x0 = grd_curcanv->cv_bitmap.bm_w - FSPACX(60);
			if (Game_mode & GM_MULTI_COOP)
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(27);
			else
				x1 = grd_curcanv->cv_bitmap.bm_w - FSPACX(15);  // Right edge of name, change this for width problems
			if (i==n_left)
				y = save_y;

		}

		if (Show_kill_list == 3)
			player_num = i;
		else
			player_num = player_list[i];

		if (Show_kill_list == 1 || Show_kill_list==2)
		{
			int color;

			if (Players[player_num].connected != 1)
				gr_set_fontcolor(BM_XRGB(12, 12, 12), -1);
			else if (Game_mode & GM_TEAM) {
				color = get_team(player_num);
				gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
			}
			else {
				color = player_num;
				gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
			}
		}
		else
		{
			gr_set_fontcolor(BM_XRGB(player_rgb[player_num].r,player_rgb[player_num].g,player_rgb[player_num].b),-1 );
		}

		if (Show_kill_list == 3)
			strcpy(name, Netgame.team_name[i]);
		else
			strcpy(name,Players[player_num].callsign);	// Note link to above if!!
		gr_get_string_size(name,&sw,&sh,&aw);
		while (sw > (x1-x0-FSPACX(2))) {
			name[strlen(name)-1]=0;
			gr_get_string_size(name,&sw,&sh,&aw);
		}
		gr_printf(x0,y,"%s",name);

		if (Show_kill_list==2)
		{
			if (Players[player_num].net_killed_total+Players[player_num].net_kills_total==0)
				gr_printf (x1,y,"NA");
		else
			gr_printf (x1,y,"%d%%",(int)((float)((float)Players[player_num].net_kills_total/((float)Players[player_num].net_killed_total+(float)Players[player_num].net_kills_total))*100.0));		
		}
		else if (Show_kill_list == 3)	
			gr_printf(x1,y,"%3d",team_kills[i]);
		else if (Game_mode & GM_MULTI_COOP)
			gr_printf(x1,y,"%-6d",Players[player_num].score);
		else
			gr_printf(x1,y,"%3d",Players[player_num].net_kills_total);

		y += LINE_SPACING;
	}
}
#endif

//draw all the things on the HUD

void draw_hud()
{
	// Show score so long as not in rearview
	if ( !Rear_view && PlayerCfg.CockpitMode!=CM_REAR_VIEW && PlayerCfg.CockpitMode!=CM_STATUS_BAR) {
		hud_show_score();
		if (score_time)
			hud_show_score_added();
	}

	// Show other stuff if not in rearview or letterbox.
	if (!Rear_view && PlayerCfg.CockpitMode!=CM_REAR_VIEW) {
		if (PlayerCfg.CockpitMode==CM_STATUS_BAR || PlayerCfg.CockpitMode==CM_FULL_SCREEN)
			hud_show_homing_warning();

		if (PlayerCfg.CockpitMode==CM_FULL_SCREEN) {
			hud_show_energy();
			hud_show_shield();
			hud_show_weapons();
			hud_show_keys();
			hud_show_cloak_invuln();

			if ( ( Newdemo_state==ND_STATE_RECORDING ) && ( Players[Player_num].flags != old_flags )) {
				newdemo_record_player_flags(old_flags, Players[Player_num].flags);
				old_flags = Players[Player_num].flags;
			}
		}

#ifdef NETWORK
#ifndef RELEASE
		if (!(Game_mode&GM_MULTI && Show_kill_list))
			show_time();
#endif
#endif
		if (PlayerCfg.CockpitMode != CM_LETTERBOX && PlayerCfg.ReticleOn)
			show_reticle();
		HUD_render_message_frame();

		if (PlayerCfg.CockpitMode!=CM_STATUS_BAR)
			hud_show_lives();
#ifdef NETWORK
		if (Game_mode&GM_MULTI && Show_kill_list)
			hud_show_kill_list();
#endif
	}

	if (Rear_view && PlayerCfg.CockpitMode!=CM_REAR_VIEW) {
		HUD_render_message_frame();
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(BM_XRGB(0,31,0),-1 );
		gr_printf(0x8000,GHEIGHT-LINE_SPACING,TXT_REAR_VIEW);
	}

}

//print out some player statistics
void render_gauges()
{
	int energy = f2ir(Players[Player_num].energy);
	int shields = f2ir(Players[Player_num].shields);
	int cloak = ((Players[Player_num].flags&PLAYER_FLAGS_CLOAKED) != 0);

	Assert(PlayerCfg.CockpitMode==CM_FULL_COCKPIT || PlayerCfg.CockpitMode==CM_STATUS_BAR);

	if (shields < 0 ) shields = 0;

	gr_set_current_canvas(NULL);
	gr_set_curfont( GAME_FONT );

	if (Newdemo_state == ND_STATE_RECORDING)
		if (Players[Player_num].homing_object_dist >= 0)
			newdemo_record_homing_distance(Players[Player_num].homing_object_dist);

	draw_weapon_boxes();

	if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT) {

		if (Newdemo_state == ND_STATE_RECORDING && (energy != old_energy))
		{
			newdemo_record_player_energy(
#ifndef SHAREWARE
				old_energy,
#endif
				energy);
			old_energy = energy;
		}
		draw_energy_bar(energy);
		draw_numerical_display(shields, energy);
		if (!PlayerCfg.HudMode)
			show_bomb_count(HUD_SCALE_X(BOMB_COUNT_X), HUD_SCALE_Y(BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);
		draw_player_ship(cloak, old_cloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
			draw_invulnerable_ship();
			old_shields = shields ^ 1;
		} else {		// Draw the shield gauge
			if (Newdemo_state == ND_STATE_RECORDING && (shields != old_shields))
			{
				newdemo_record_player_shields(
#ifndef SHAREWARE
					old_shields,
#endif
					shields);
				old_shields = shields;
			}
			draw_shield_bar(shields);
		}
		draw_numerical_display(shields, energy);
	
		if (Newdemo_state == ND_STATE_RECORDING && (Players[Player_num].flags != old_flags))
		{
			newdemo_record_player_flags(old_flags, Players[Player_num].flags);
			old_flags = Players[Player_num].flags;
		}
		draw_keys();

		show_homing_warning();
		draw_wbu_overlay();

	} else if (PlayerCfg.CockpitMode == CM_STATUS_BAR) {

		if (Newdemo_state == ND_STATE_RECORDING && (energy != old_energy))
		{
			newdemo_record_player_energy(
#ifndef SHAREWARE
				old_energy,
#endif
				energy);
			old_energy = energy;
		}
		sb_draw_energy_bar(energy);
		if (!PlayerCfg.HudMode)
			show_bomb_count(HUD_SCALE_X(SB_BOMB_COUNT_X), HUD_SCALE_Y(SB_BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);

		draw_player_ship(cloak, old_cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		{
			draw_invulnerable_ship();
			old_shields = shields ^ 1;
		} 
		else
		{	// Draw the shield gauge
			if (Newdemo_state == ND_STATE_RECORDING && (shields != old_shields))
			{
				newdemo_record_player_shields(
#ifndef SHAREWARE
					old_shields,
#endif
					shields);
				old_shields = shields;
			}
			sb_draw_shield_bar(shields);
		}
		sb_draw_shield_num(shields);

		if (Newdemo_state == ND_STATE_RECORDING && (Players[Player_num].flags != old_flags))
		{
			newdemo_record_player_flags(old_flags, Players[Player_num].flags);
			old_flags = Players[Player_num].flags;
		}
		sb_draw_keys();
	

		// May want to record this in a demo...
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			sb_show_lives();
			old_lives = Players[Player_num].net_killed_total;
		}
		else
		{
			sb_show_lives();
			old_lives = Players[Player_num].lives;
		}

		if ((Game_mode&GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			sb_show_score();
			old_score = Players[Player_num].net_kills_total;
		}
		else
		{
			sb_show_score();
			old_score = Players[Player_num].score;
			sb_show_score_added();
		}
	} else
		draw_player_ship(cloak, old_cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

	old_cloak = cloak;
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
void update_laser_weapon_info(void)
{
	if (old_weapon[0] == 0)
		old_weapon[0] = -1;
}
