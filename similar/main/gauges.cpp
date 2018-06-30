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
#include <cmath>
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
#include "weapon.h"
#include "common/3d/globvars.h"
#include "playsave.h"
#include "rle.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "partial_range.h"

using std::min;

namespace {

class local_multires_gauge_graphic
{
	bool hiresmode;
public:
	local_multires_gauge_graphic() :
		hiresmode(HIRESMODE)
	{
	}
	bool is_hires() const
	{
		return hiresmode;
	}
	template <typename T>
		T get(T h, T l) const
	{
		return is_hires() ? h : l;
	}
	template <typename T>
		const T &rget(const T &h, const T &l) const
		{
			return is_hires() ? h : l;
		}
};

static bool show_cloak_invul_timer()
{
	return PlayerCfg.CloakInvulTimer && Newdemo_state != ND_STATE_PLAYBACK;
}

}

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
#define RIGHT_ENERGY_GAUGE_X 	((multires_gauge_graphic.get(380, 190)))
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
#define RIGHT_ENERGY_GAUGE_X 	((multires_gauge_graphic.get(379, 190)))
#endif

#define GAUGE_BLUE_KEY_Y_L		152
#define GAUGE_BLUE_KEY_Y_H		374
#define GAUGE_BLUE_KEY_X		((multires_gauge_graphic.get(GAUGE_BLUE_KEY_X_H, GAUGE_BLUE_KEY_X_L)))
#define GAUGE_BLUE_KEY_Y		((multires_gauge_graphic.get(GAUGE_BLUE_KEY_Y_H, GAUGE_BLUE_KEY_Y_L)))
#define GAUGE_GOLD_KEY_Y_L		162
#define GAUGE_GOLD_KEY_Y_H		395
#define GAUGE_GOLD_KEY_X		((multires_gauge_graphic.get(GAUGE_GOLD_KEY_X_H, GAUGE_GOLD_KEY_X_L)))
#define GAUGE_GOLD_KEY_Y		((multires_gauge_graphic.get(GAUGE_GOLD_KEY_Y_H, GAUGE_GOLD_KEY_Y_L)))
#define GAUGE_RED_KEY_Y_L		172
#define GAUGE_RED_KEY_X			((multires_gauge_graphic.get(GAUGE_RED_KEY_X_H, GAUGE_RED_KEY_X_L)))
#define GAUGE_RED_KEY_Y			((multires_gauge_graphic.get(GAUGE_RED_KEY_Y_H, GAUGE_RED_KEY_Y_L)))
#define SB_GAUGE_KEYS_X_L		11
#define SB_GAUGE_KEYS_X_H		26
#define SB_GAUGE_KEYS_X			((multires_gauge_graphic.get(SB_GAUGE_KEYS_X_H, SB_GAUGE_KEYS_X_L)))
#define SB_GAUGE_BLUE_KEY_Y_L		153
#define SB_GAUGE_GOLD_KEY_Y_L		169
#define SB_GAUGE_RED_KEY_Y_L		185
#define SB_GAUGE_BLUE_KEY_Y_H		390
#define SB_GAUGE_GOLD_KEY_Y_H		422
#define SB_GAUGE_RED_KEY_Y_H		454
#define SB_GAUGE_BLUE_KEY_Y		((multires_gauge_graphic.get(SB_GAUGE_BLUE_KEY_Y_H, SB_GAUGE_BLUE_KEY_Y_L)))
#define SB_GAUGE_GOLD_KEY_Y		((multires_gauge_graphic.get(SB_GAUGE_GOLD_KEY_Y_H, SB_GAUGE_GOLD_KEY_Y_L)))
#define SB_GAUGE_RED_KEY_Y		((multires_gauge_graphic.get(SB_GAUGE_RED_KEY_Y_H, SB_GAUGE_RED_KEY_Y_L)))
#define LEFT_ENERGY_GAUGE_X_L 	70
#define LEFT_ENERGY_GAUGE_Y_L 	131
#define LEFT_ENERGY_GAUGE_W_L 	64
#define LEFT_ENERGY_GAUGE_H_L 	8
#define LEFT_ENERGY_GAUGE_Y_H 	314
#define LEFT_ENERGY_GAUGE_W_H 	133
#define LEFT_ENERGY_GAUGE_H_H 	21
#define LEFT_ENERGY_GAUGE_X 	((multires_gauge_graphic.get(LEFT_ENERGY_GAUGE_X_H, LEFT_ENERGY_GAUGE_X_L)))
#define LEFT_ENERGY_GAUGE_Y 	((multires_gauge_graphic.get(LEFT_ENERGY_GAUGE_Y_H, LEFT_ENERGY_GAUGE_Y_L)))
#define LEFT_ENERGY_GAUGE_W 	((multires_gauge_graphic.get(LEFT_ENERGY_GAUGE_W_H, LEFT_ENERGY_GAUGE_W_L)))
#define LEFT_ENERGY_GAUGE_H 	((multires_gauge_graphic.get(LEFT_ENERGY_GAUGE_H_H, LEFT_ENERGY_GAUGE_H_L)))
#define RIGHT_ENERGY_GAUGE_Y 	((multires_gauge_graphic.get(314, 131)))
#define RIGHT_ENERGY_GAUGE_W 	((multires_gauge_graphic.get(133, 64)))
#define RIGHT_ENERGY_GAUGE_H 	((multires_gauge_graphic.get(21, 8)))

#if defined(DXX_BUILD_DESCENT_I)
#define SB_ENERGY_GAUGE_Y 		((multires_gauge_graphic.get(390, 155)))
#define SB_ENERGY_GAUGE_H 		((multires_gauge_graphic.get(82, 41)))
#define SB_ENERGY_NUM_Y 		((multires_gauge_graphic.get(457, 190)))
#define SHIELD_GAUGE_Y			((multires_gauge_graphic.get(374, 155)))
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-((multires_gauge_graphic.get(16, 7))))			//156 -- MWA used to be hard coded to 156
#define NUMERICAL_GAUGE_Y		((multires_gauge_graphic.get(316, 130)))
#define PRIMARY_W_PIC_X			((multires_gauge_graphic.get(135, 64)))
#define SECONDARY_W_PIC_X		((multires_gauge_graphic.get(405, 234)))
#define SECONDARY_W_PIC_Y		((multires_gauge_graphic.get(370, 154)))
#define SECONDARY_W_TEXT_X		multires_gauge_graphic.get(462, 207)
#define SECONDARY_W_TEXT_Y		multires_gauge_graphic.get(400, 157)
#define SECONDARY_AMMO_X		multires_gauge_graphic.get(475, 213)
#define SECONDARY_AMMO_Y		multires_gauge_graphic.get(425, 171)
#define SB_LIVES_X			((multires_gauge_graphic.get(550, 266)))
#define SB_LIVES_Y			((multires_gauge_graphic.get(450, 185)))
#define SB_SCORE_RIGHT_H		605
#define BOMB_COUNT_X			((multires_gauge_graphic.get(468, 210)))
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
#define SB_SECONDARY_AMMO_X		(SB_SECONDARY_W_BOX_LEFT + (multires_gauge_graphic.get(14, 11)))	//(212+9)
#define GET_GAUGE_INDEX(x)		(Gauges[x].index)

#elif defined(DXX_BUILD_DESCENT_II)
#define AFTERBURNER_GAUGE_X_L	45-1
#define AFTERBURNER_GAUGE_Y_L	158
#define AFTERBURNER_GAUGE_H_L	32
#define AFTERBURNER_GAUGE_X_H	88
#define AFTERBURNER_GAUGE_Y_H	378
#define AFTERBURNER_GAUGE_H_H	65
#define AFTERBURNER_GAUGE_X	((multires_gauge_graphic.get(AFTERBURNER_GAUGE_X_H, AFTERBURNER_GAUGE_X_L)))
#define AFTERBURNER_GAUGE_Y	((multires_gauge_graphic.get(AFTERBURNER_GAUGE_Y_H, AFTERBURNER_GAUGE_Y_L)))
#define AFTERBURNER_GAUGE_H	((multires_gauge_graphic.get(AFTERBURNER_GAUGE_H_H, AFTERBURNER_GAUGE_H_L)))
#define SB_AFTERBURNER_GAUGE_X 		((multires_gauge_graphic.get(196, 98)))
#define SB_AFTERBURNER_GAUGE_Y 		((multires_gauge_graphic.get(445, 184)))
#define SB_AFTERBURNER_GAUGE_W 		((multires_gauge_graphic.get(33, 16)))
#define SB_AFTERBURNER_GAUGE_H 		((multires_gauge_graphic.get(29, 13)))
#define SB_ENERGY_GAUGE_Y 		((multires_gauge_graphic.get(381, 155-2)))
#define SB_ENERGY_GAUGE_H 		((multires_gauge_graphic.get(60, 29)))
#define SB_ENERGY_NUM_Y 		((multires_gauge_graphic.get(457, 175)))
#define SHIELD_GAUGE_Y			((multires_gauge_graphic.get(375, 155)))
#define SB_SHIELD_NUM_Y 		(SB_SHIELD_GAUGE_Y-((multires_gauge_graphic.get(16, 8))))			//156 -- MWA used to be hard coded to 156
#define NUMERICAL_GAUGE_Y		((multires_gauge_graphic.get(314, 130)))
#define PRIMARY_W_PIC_X			((multires_gauge_graphic.get(135-10, 64)))
#define SECONDARY_W_PIC_X		((multires_gauge_graphic.get(466, 234)))
#define SECONDARY_W_PIC_Y		((multires_gauge_graphic.get(374, 154)))
#define SECONDARY_W_TEXT_X		multires_gauge_graphic.get(413, 207)
#define SECONDARY_W_TEXT_Y		multires_gauge_graphic.get(378, 157)
#define SECONDARY_AMMO_X		multires_gauge_graphic.get(428, 213)
#define SECONDARY_AMMO_Y		multires_gauge_graphic.get(407, 171)
#define SB_LIVES_X			((multires_gauge_graphic.get(550-10-3, 266)))
#define SB_LIVES_Y			((multires_gauge_graphic.get(450-3, 185)))
#define SB_SCORE_RIGHT_H		(605+8)
#define BOMB_COUNT_X			((multires_gauge_graphic.get(546, 275)))
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
#define SB_SECONDARY_AMMO_X		(SB_SECONDARY_W_BOX_LEFT + (multires_gauge_graphic.get(14 - 4, 11)))	//(212+9)
#define GET_GAUGE_INDEX(x)		((multires_gauge_graphic.rget(Gauges_hires, Gauges)[x].index))

#endif

// MOVEME
#define HOMING_WARNING_X		((multires_gauge_graphic.get(13, 7)))
#define HOMING_WARNING_Y		((multires_gauge_graphic.get(416, 171)))

#define SB_ENERGY_GAUGE_X 		((multires_gauge_graphic.get(196, 98)))
#define SB_ENERGY_GAUGE_W 		((multires_gauge_graphic.get(32, 16)))
#define SHIP_GAUGE_X 			(SHIELD_GAUGE_X+((multires_gauge_graphic.get(11, 5))))
#define SHIP_GAUGE_Y			(SHIELD_GAUGE_Y+((multires_gauge_graphic.get(10, 5))))
#define SHIELD_GAUGE_X 			((multires_gauge_graphic.get(292, 146)))
#define SB_SHIELD_GAUGE_X 		((multires_gauge_graphic.get(247, 123)))		//139
#define SB_SHIELD_GAUGE_Y 		((multires_gauge_graphic.get(395, 163)))
#define SB_SHIP_GAUGE_X 		(SB_SHIELD_GAUGE_X+((multires_gauge_graphic.get(11, 5))))
#define SB_SHIP_GAUGE_Y 		(SB_SHIELD_GAUGE_Y+((multires_gauge_graphic.get(10, 5))))
#define SB_SHIELD_NUM_X 		(SB_SHIELD_GAUGE_X+((multires_gauge_graphic.get(21, 12))))	//151
#define NUMERICAL_GAUGE_X		((multires_gauge_graphic.get(308, 154)))
#define PRIMARY_W_PIC_Y			((multires_gauge_graphic.get(370, 154)))
#define PRIMARY_W_TEXT_X		multires_gauge_graphic.get(182, 87)
#define PRIMARY_W_TEXT_Y		multires_gauge_graphic.get(378, 157)
#define PRIMARY_AMMO_X			multires_gauge_graphic.get(186, 93)
#define PRIMARY_AMMO_Y			multires_gauge_graphic.get(407, 171)
#define SB_LIVES_LABEL_X		((multires_gauge_graphic.get(475, 237)))
#define SB_SCORE_RIGHT_L		301
#define SB_SCORE_RIGHT			((multires_gauge_graphic.get(SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_L)))
#define SB_SCORE_Y			((multires_gauge_graphic.get(398, 158)))
#define SB_SCORE_LABEL_X		((multires_gauge_graphic.get(475, 237)))
#define SB_SCORE_ADDED_RIGHT		((multires_gauge_graphic.get(SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_L)))
#define SB_SCORE_ADDED_Y		((multires_gauge_graphic.get(413, 165)))
#define BOMB_COUNT_Y			((multires_gauge_graphic.get(445, 186)))
#define SB_BOMB_COUNT_X			((multires_gauge_graphic.get(342, 171)))
#define SB_BOMB_COUNT_Y			((multires_gauge_graphic.get(458, 191)))

// defining box boundries for weapon pictures
#define PRIMARY_W_BOX_LEFT_L		63
#define PRIMARY_W_BOX_TOP_L		151		//154
#define PRIMARY_W_BOX_RIGHT_L		(PRIMARY_W_BOX_LEFT_L+58)
#define PRIMARY_W_BOX_BOT_L		(PRIMARY_W_BOX_TOP_L+42)
#define PRIMARY_W_BOX_LEFT_H		121
#define PRIMARY_W_BOX_TOP_H		364
#define PRIMARY_W_BOX_BOT_H		(PRIMARY_W_BOX_TOP_H+106)		//470
#define PRIMARY_W_BOX_LEFT		((multires_gauge_graphic.get(PRIMARY_W_BOX_LEFT_H, PRIMARY_W_BOX_LEFT_L)))
#define PRIMARY_W_BOX_TOP		((multires_gauge_graphic.get(PRIMARY_W_BOX_TOP_H, PRIMARY_W_BOX_TOP_L)))
#define PRIMARY_W_BOX_RIGHT		((multires_gauge_graphic.get(PRIMARY_W_BOX_RIGHT_H, PRIMARY_W_BOX_RIGHT_L)))
#define PRIMARY_W_BOX_BOT		((multires_gauge_graphic.get(PRIMARY_W_BOX_BOT_H, PRIMARY_W_BOX_BOT_L)))
#define SECONDARY_W_BOX_LEFT_L		202	//207
#define SECONDARY_W_BOX_TOP_L		151
#define SECONDARY_W_BOX_BOT_L		(SECONDARY_W_BOX_TOP_L+42)
#define SECONDARY_W_BOX_BOT_H		(SECONDARY_W_BOX_TOP_H+106)		//470
#define SECONDARY_W_BOX_LEFT		((multires_gauge_graphic.get(SECONDARY_W_BOX_LEFT_H, SECONDARY_W_BOX_LEFT_L)))
#define SECONDARY_W_BOX_TOP		((multires_gauge_graphic.get(SECONDARY_W_BOX_TOP_H, SECONDARY_W_BOX_TOP_L)))
#define SECONDARY_W_BOX_RIGHT		((multires_gauge_graphic.get(SECONDARY_W_BOX_RIGHT_H, SECONDARY_W_BOX_RIGHT_L)))
#define SECONDARY_W_BOX_BOT		((multires_gauge_graphic.get(SECONDARY_W_BOX_BOT_H, SECONDARY_W_BOX_BOT_L)))
#define SB_PRIMARY_W_BOX_LEFT_L		34		//50
#define SB_PRIMARY_W_BOX_RIGHT_L	(SB_PRIMARY_W_BOX_LEFT_L+55)
#define SB_PRIMARY_W_BOX_LEFT_H		68
#define SB_PRIMARY_W_BOX_TOP_H		381
#define SB_PRIMARY_W_BOX_RIGHT_H	179
#define SB_PRIMARY_W_BOX_BOT_H		473
#define SB_PRIMARY_W_BOX_LEFT		((multires_gauge_graphic.get(SB_PRIMARY_W_BOX_LEFT_H, SB_PRIMARY_W_BOX_LEFT_L)))
#define SB_SECONDARY_W_BOX_LEFT_L	169
#define SB_SECONDARY_W_BOX_LEFT_H	338
#define SB_SECONDARY_W_BOX_TOP_H	381
#define SB_SECONDARY_W_BOX_RIGHT_H	449
#define SB_SECONDARY_W_BOX_BOT_H	473
#define SB_SECONDARY_W_BOX_LEFT		((multires_gauge_graphic.get(SB_SECONDARY_W_BOX_LEFT_H, SB_SECONDARY_W_BOX_LEFT_L)))	//210
#define SB_PRIMARY_W_PIC_X		(SB_PRIMARY_W_BOX_LEFT+1)	//51
#define SB_PRIMARY_W_PIC_Y		((multires_gauge_graphic.get(382, 154)))
#define SB_PRIMARY_W_TEXT_X		(SB_PRIMARY_W_BOX_LEFT + multires_gauge_graphic.get(50, 24))	//(51+23)
#define SB_PRIMARY_W_TEXT_Y		(multires_gauge_graphic.get(390, 157))
#define SB_PRIMARY_AMMO_X		(SB_PRIMARY_W_BOX_LEFT + multires_gauge_graphic.get(58, 30))	//((SB_PRIMARY_W_BOX_LEFT+33)-3)	//(51+32)
#define SB_PRIMARY_AMMO_Y		multires_gauge_graphic.get(410, 171)
#define SB_SECONDARY_W_PIC_X		((multires_gauge_graphic.get(385, (SB_SECONDARY_W_BOX_LEFT+27))))	//(212+27)
#define SB_SECONDARY_W_PIC_Y		((multires_gauge_graphic.get(382, 154)))
#define SB_SECONDARY_W_TEXT_X		(SB_SECONDARY_W_BOX_LEFT + 2)	//212
#define SB_SECONDARY_W_TEXT_Y		multires_gauge_graphic.get(390, 157)
#define SB_SECONDARY_AMMO_Y		multires_gauge_graphic.get(414, 171)

#define WS_SET				0		//in correct state
#define WS_FADING_OUT			1
#define WS_FADING_IN			2
#define FADE_SCALE			(2*i2f(GR_FADE_LEVELS)/REARM_TIME)		// fade out and back in REARM_TIME, in fade levels per seconds (int)

#define COCKPIT_PRIMARY_BOX		((multires_gauge_graphic.get(4, 0)))
#define COCKPIT_SECONDARY_BOX		((multires_gauge_graphic.get(5, 1)))
#define SB_PRIMARY_BOX			((multires_gauge_graphic.get(6, 2)))
#define SB_SECONDARY_BOX		((multires_gauge_graphic.get(7, 3)))

// scaling gauges
#define BASE_WIDTH(G) ((G).get(640, 320))
#define BASE_HEIGHT(G)	((G).get(480, 200))
namespace {

#if DXX_USE_OGL
template <char tag>
class hud_scale_float;
#else
struct hud_unscaled_int
{
	long operator()(const unsigned i) const
	{
		return i;
	}
};

template <char tag>
using hud_scale_float = hud_unscaled_int;
#endif

using hud_ar_scale_float = hud_scale_float<'a'>;
using hud_x_scale_float = hud_scale_float<'x'>;
using hud_y_scale_float = hud_scale_float<'y'>;

#if DXX_USE_OGL
class base_hud_scaled_int
{
	const long v;
public:
	explicit constexpr base_hud_scaled_int(const long l) :
		v(l)
	{
	}
	operator long() const
	{
		return v;
	}
};

template <char>
class hud_scaled_int : public base_hud_scaled_int
{
public:
	DXX_INHERIT_CONSTRUCTORS(hud_scaled_int, base_hud_scaled_int);
};

class base_hud_scale_float
{
protected:
	const double scale;
	long operator()(const int i) const
	{
		return (this->scale * static_cast<double>(i)) + 0.5;
	}
	double get() const
	{
		return scale;
	}
public:
	constexpr base_hud_scale_float(const double s) :
		scale(s)
	{
	}
};

template <char tag>
class hud_scale_float : base_hud_scale_float
{
public:
	using scaled = hud_scaled_int<tag>;
	using base_hud_scale_float::get;
	DXX_INHERIT_CONSTRUCTORS(hud_scale_float, base_hud_scale_float);
	scaled operator()(const int i) const
	{
		return scaled(this->base_hud_scale_float::operator()(i));
	}
};

static hud_x_scale_float HUD_SCALE_X(const unsigned screen_width, const local_multires_gauge_graphic multires_gauge_graphic)
{
	return static_cast<double>(screen_width) / BASE_WIDTH(multires_gauge_graphic);
}

static hud_y_scale_float HUD_SCALE_Y(const unsigned screen_height, const local_multires_gauge_graphic multires_gauge_graphic)
{
	return static_cast<double>(screen_height) / BASE_HEIGHT(multires_gauge_graphic);
}

static hud_ar_scale_float HUD_SCALE_AR(const hud_x_scale_float x, const hud_y_scale_float y)
{
	return std::min(x.get(), y.get());
}

static hud_ar_scale_float HUD_SCALE_AR(const unsigned screen_width, const unsigned screen_height, const local_multires_gauge_graphic multires_gauge_graphic)
{
	return HUD_SCALE_AR(HUD_SCALE_X(screen_width, multires_gauge_graphic), HUD_SCALE_Y(screen_height, multires_gauge_graphic));
}

#define draw_numerical_display_draw_context	hud_draw_context_hs
#else
#define hud_bitblt_free(canvas,x,y,w,h,bm)	hud_bitblt_free(canvas,x,y,bm)
#define draw_numerical_display_draw_context	hud_draw_context_hs_mr

static hud_ar_scale_float HUD_SCALE_AR(hud_x_scale_float, hud_y_scale_float)
{
	return {};
}

static hud_ar_scale_float HUD_SCALE_AR(unsigned, unsigned, local_multires_gauge_graphic)
{
	return {};
}
#endif

}

#if defined(DXX_BUILD_DESCENT_I)
#define PAGE_IN_GAUGE(x,g)	PAGE_IN_GAUGE(x)
array<bitmap_index, MAX_GAUGE_BMS_MAC> Gauges; // Array of all gauge bitmaps.
#elif defined(DXX_BUILD_DESCENT_II)
#define PAGE_IN_GAUGE	PAGE_IN_GAUGE
array<bitmap_index, MAX_GAUGE_BMS> Gauges,   // Array of all gauge bitmaps.
	Gauges_hires;   // hires gauges
static array<int, 2> weapon_box_user{{WBU_WEAPON, WBU_WEAPON}};		//see WBU_ constants in gauges.h
#endif
static grs_bitmap deccpt;
static array<grs_subbitmap_ptr, 2> WinBoxOverlay; // Overlay subbitmaps for both weapon boxes

namespace dsx {
static inline void PAGE_IN_GAUGE(int x, const local_multires_gauge_graphic multires_gauge_graphic)
{
	const auto &g =
#if defined(DXX_BUILD_DESCENT_II)
		multires_gauge_graphic.is_hires() ? Gauges_hires :
#endif
		Gauges;
	PIGGY_PAGE_IN(g[x]);
}
}

static void draw_ammo_info(grs_canvas &, unsigned x, unsigned y, unsigned ammo_count);

static int score_display;
static fix score_time;
static array<int, 2> old_weapon{{-1, -1}};
static int old_laser_level		= -1;
static int invulnerable_frame;
static array<int, 2> weapon_box_states;
static_assert(WS_SET == 0, "weapon_box_states must start at zero");
static array<fix, 2> weapon_box_fade_values;
int	Color_0_31_0 = -1;

namespace dcx {

namespace {

struct hud_draw_context_canvas
{
	grs_canvas &canvas;
	hud_draw_context_canvas(grs_canvas &c) :
		canvas(c)
	{
	}
};

struct hud_draw_context_multires
{
	const local_multires_gauge_graphic multires_gauge_graphic;
	hud_draw_context_multires(const local_multires_gauge_graphic mr) :
		multires_gauge_graphic(mr)
	{
	}
};

struct hud_draw_context_mr : hud_draw_context_canvas, hud_draw_context_multires
{
	hud_draw_context_mr(grs_canvas &c, const local_multires_gauge_graphic mr) :
		hud_draw_context_canvas(c), hud_draw_context_multires(mr)
	{
	}
};

struct hud_draw_context_xyscale
{
	const hud_x_scale_float xscale;
	const hud_y_scale_float yscale;
#if DXX_USE_OGL
	constexpr hud_draw_context_xyscale(const hud_x_scale_float x, const hud_y_scale_float y) :
		xscale(x), yscale(y)
	{
	}
#else
	constexpr hud_draw_context_xyscale() :
		xscale{}, yscale{}
	{
	}
#endif
};

struct hud_draw_context_hs_mr : hud_draw_context_mr, hud_draw_context_xyscale
{
#if DXX_USE_OGL
	hud_draw_context_hs_mr(grs_canvas &c, const unsigned screen_width, const unsigned screen_height, const local_multires_gauge_graphic multires_gauge_graphic) :
		hud_draw_context_mr(c, multires_gauge_graphic),
		hud_draw_context_xyscale(HUD_SCALE_X(screen_width, multires_gauge_graphic), HUD_SCALE_Y(screen_height, multires_gauge_graphic))
	{
	}
#else
	hud_draw_context_hs_mr(grs_canvas &c, unsigned, unsigned, const local_multires_gauge_graphic multires_gauge_graphic) :
		hud_draw_context_mr(c, multires_gauge_graphic)
	{
	}
#endif
};

struct hud_draw_context_hs : hud_draw_context_canvas, hud_draw_context_xyscale
{
	hud_draw_context_hs(const hud_draw_context_hs_mr &hudctx) :
		hud_draw_context_canvas(hudctx.canvas), hud_draw_context_xyscale(hudctx)
	{
	}
};

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

struct dspan
{
	span l, r;
};

//store delta x values from left of box
const array<dspan, 43> weapon_windows_lowres = {{
	{{71,114},		{208,255}},
	{{69,116},		{206,257}},
	{{68,117},		{205,258}},
	{{66,118},		{204,259}},
	{{66,119},		{203,260}},
	{{66,119},		{203,260}},
	{{65,119},		{203,260}},
	{{65,119},		{203,260}},
	{{65,119},		{203,260}},
	{{65,119},		{203,261}},
	{{65,119},		{203,261}},
	{{65,119},		{203,261}},
	{{65,119},		{203,261}},
	{{65,119},		{203,261}},
	{{65,119},		{203,261}},
	{{64,119},		{203,261}},
	{{64,119},		{203,261}},
	{{64,119},		{203,261}},
	{{64,119},		{203,262}},
	{{64,119},		{203,262}},
	{{64,119},		{203,262}},
	{{64,119},		{203,262}},
	{{64,119},		{203,262}},
	{{64,119},		{203,262}},
	{{63,119},		{203,262}},
	{{63,118},		{203,262}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,118},		{204,263}},
	{{63,117},		{204,263}},
	{{63,117},		{205,263}},
	{{64,116},		{206,262}},
	{{65,115},		{207,261}},
	{{66,113},		{208,260}},
	{{68,111},		{211,255}},
}};

//store delta x values from left of box
const array<dspan, 107> weapon_windows_hires = {{
	{{141,231},		{416,509}},
	{{139,234},		{413,511}},
	{{137,235},		{412,513}},
	{{136,237},		{410,514}},
	{{135,238},		{409,515}},
	{{134,239},		{408,516}},
	{{133,240},		{407,517}},
	{{132,240},		{407,518}},
	{{131,241},		{406,519}},
	{{131,241},		{406,519}},
	{{130,242},		{405,520}},
	{{129,242},		{405,521}},
	{{129,242},		{405,521}},
	{{129,243},		{404,521}},
	{{128,243},		{404,522}},
	{{128,243},		{404,522}},
	{{128,243},		{404,522}},
	{{128,243},		{404,522}},
	{{128,243},		{404,522}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{127,243},		{404,523}},
	{{126,243},		{404,524}},
	{{126,243},		{404,524}},
	{{126,243},		{404,524}},
	{{126,243},		{404,524}},
	{{126,242},		{405,524}},
	{{126,242},		{405,524}},
	{{126,242},		{405,524}},
	{{126,242},		{405,524}},
	{{126,242},		{405,524}},
	{{126,242},		{405,524}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{125,242},		{405,525}},
	{{124,242},		{405,526}},
	{{124,242},		{405,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,526}},
	{{124,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{123,241},		{406,527}},
	{{122,241},		{406,528}},
	{{122,241},		{406,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,528}},
	{{122,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,240},		{407,529}},
	{{121,239},		{408,529}},
	{{121,239},		{408,529}},
	{{121,239},		{408,529}},
	{{121,238},		{409,529}},
	{{121,238},		{409,529}},
	{{121,238},		{409,529}},
	{{122,237},		{410,529}},
	{{122,237},		{410,528}},
	{{123,236},		{411,527}},
	{{123,235},		{412,527}},
	{{124,234},		{413,526}},
	{{125,233},		{414,525}},
	{{126,232},		{415,524}},
	{{126,231},		{416,524}},
	{{128,230},		{417,522}},
	{{130,228},		{419,521}},
	{{131,226},		{422,519}},
	{{133,223},		{424,518}},
}};

static inline void hud_bitblt_free(grs_canvas &canvas, const unsigned x, const unsigned y, const unsigned w, const unsigned h, grs_bitmap &bm)
{
#if DXX_USE_OGL
	ogl_ubitmapm_cs(canvas, x, y, w, h, bm, ogl_colors::white, F1_0);
#else
	gr_ubitmapm(canvas, x, y, bm);
#endif
}

static void hud_bitblt_scaled_xy(const hud_draw_context_hs hudctx, const unsigned x, const unsigned y, grs_bitmap &bm)
{
	hud_bitblt_free(hudctx.canvas, x, y, hudctx.xscale(bm.bm_w), hudctx.yscale(bm.bm_h), bm);
}

static void hud_bitblt(const hud_draw_context_hs hudctx, const unsigned x, const unsigned y, grs_bitmap &bm)
{
	hud_bitblt_scaled_xy(hudctx, hudctx.xscale(x), hudctx.yscale(y), bm);
}

}

}

namespace dsx {

namespace {

#if defined(DXX_BUILD_DESCENT_I)
#define hud_gauge_bitblt_draw_context	hud_draw_context_hs
#elif defined(DXX_BUILD_DESCENT_II)
#define hud_gauge_bitblt_draw_context	hud_draw_context_hs_mr
#endif
static void hud_gauge_bitblt(const hud_gauge_bitblt_draw_context hudctx, const unsigned x, const unsigned y, const unsigned gauge)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
#endif
	PAGE_IN_GAUGE(gauge, multires_gauge_graphic);
	hud_bitblt(hudctx, x, y, GameBitmaps[GET_GAUGE_INDEX(gauge)]);
}

class draw_keys_state
{
	const hud_draw_context_hs_mr hudctx;
	const player_flags player_key_flags;
public:
	draw_keys_state(const hud_draw_context_hs_mr hc, const player_flags f) :
		hudctx(hc), player_key_flags(f)
	{
	}
	void draw_all_cockpit_keys();
	void draw_all_statusbar_keys();
protected:
	void draw_one_key(const unsigned x, const unsigned y, const unsigned gauge, const PLAYER_FLAG flag) const
	{
		hud_gauge_bitblt(hudctx, x, y, (player_key_flags & flag) ? gauge : (gauge + 3));
	}
};

}

}

static void hud_show_score(grs_canvas &canvas, const player_info &player_info)
{
	char	score_str[20];

	if (HUD_toolong)
		return;

	const char *label;
	int value;
	if ( ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) ) {
		label = TXT_KILLS;
		value = player_info.net_kills_total;
	} else {
		label = TXT_SCORE;
		value = player_info.mission.score;
  	}
	snprintf(score_str, sizeof(score_str), "%s: %5d", label, value);

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(canvas, Color_0_31_0, -1);

	int	w, h;
	auto &game_font = *GAME_FONT;
	gr_get_string_size(game_font, score_str, &w, &h, nullptr);
	gr_string(canvas, game_font, canvas.cv_bitmap.bm_w - w - FSPACX(1), FSPACY(1), score_str, w, h);
}

static void hud_show_timer_count(grs_canvas &canvas)
{
	int	i;
	fix timevar=0;

	if (HUD_toolong)
		return;

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed)
	{
		timevar=i2f (Netgame.PlayTimeAllowed*5*60);
		i=f2i(timevar-ThisLevelTime);
		i++;

		if (i>-1 && !Control_center_destroyed)
		{
		if (Color_0_31_0 == -1)
			Color_0_31_0 = BM_XRGB(0,31,0);

		gr_set_fontcolor(canvas, Color_0_31_0, -1);

			char score_str[20];
			snprintf(score_str, sizeof(score_str), "T - %5d", i);
			int w, h;
			gr_get_string_size(*canvas.cv_font, score_str, &w, &h, nullptr);
			gr_string(canvas, *canvas.cv_font, canvas.cv_bitmap.bm_w - w - FSPACX(12), LINE_SPACING(*canvas.cv_font, *GAME_FONT) + FSPACY(1), score_str, w, h);
		}
	}
}

static void hud_show_score_added(grs_canvas &canvas)
{
	int	color;

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		return;

	if (score_display == 0)
		return;

	score_time -= FrameTime;
	if (score_time > 0) {
		color = f2i(score_time * 20) + 12;

		if (color < 10) color = 12;
		if (color > 31) color = 30;

		color = color - (color % 4);

		char score_buf[32];
		const auto score_str = cheats.enabled
			? TXT_CHEATER
			: (snprintf(score_buf, sizeof(score_buf), "%5d", score_display), score_buf);

		gr_set_fontcolor(canvas, BM_XRGB(0, color, 0), -1);
		int w, h;
		auto &game_font = *GAME_FONT;
		gr_get_string_size(game_font, score_str, &w, &h, nullptr);
		gr_string(canvas, game_font, canvas.cv_bitmap.bm_w - w - FSPACX(PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN ? 1 : 12), LINE_SPACING(game_font, game_font) + FSPACY(1), score_str, w, h);
	} else {
		score_time = 0;
		score_display = 0;
	}
}

static void sb_show_score(const hud_draw_context_hs_mr hudctx, const player_info &player_info)
{
	char	score_str[20];

	auto &canvas = hudctx.canvas;
	gr_set_fontcolor(canvas, BM_XRGB(0, 20, 0), -1);

	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const auto y = hudctx.yscale(SB_SCORE_Y);
	auto &game_font = *GAME_FONT;
	gr_printf(canvas, game_font, hudctx.xscale(SB_SCORE_LABEL_X), y, "%s:", (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) ? TXT_KILLS : TXT_SCORE);

	snprintf(score_str, sizeof(score_str), "%5d",
			(Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)
			? player_info.net_kills_total
			: (gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1), player_info.mission.score));
	int	w, h;
	gr_get_string_size(game_font, score_str, &w, &h, nullptr);

	const auto scaled_score_right = hudctx.xscale(SB_SCORE_RIGHT);
	const auto x = scaled_score_right - w - FSPACX(1);

	//erase old score
	const uint8_t color = BM_XRGB(0, 0, 0);
	gr_rect(canvas, x, y, scaled_score_right, y + LINE_SPACING(game_font, game_font), color);

	gr_string(canvas, game_font, x, y, score_str, w, h);
}

static void sb_show_score_added(const hud_draw_context_hs_mr hudctx)
{
	static int x;
	static	int last_score_display = -1;

	if ( (Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) )
		return;

	if (score_display == 0)
		return;

	auto &canvas = hudctx.canvas;
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;

	score_time -= FrameTime;
	if (score_time > 0) {
		if (score_display != last_score_display)
			last_score_display = score_display;

		int	color;
		color = f2i(score_time * 20) + 10;

		if (color < 10) color = 10;
		if (color > 31) color = 31;

		char score_buf[32];
		const auto score_str = cheats.enabled
			? TXT_CHEATER
			: (snprintf(score_buf, sizeof(score_buf), "%5d", score_display), score_buf);

		int w, h;
		auto &game_font = *GAME_FONT;
		gr_get_string_size(game_font, score_str, &w, &h, nullptr);
		x = hudctx.xscale(SB_SCORE_ADDED_RIGHT) - w - FSPACX(1);
		gr_set_fontcolor(canvas, BM_XRGB(0, color, 0), -1);
		gr_string(canvas, game_font, x, hudctx.yscale(SB_SCORE_ADDED_Y), score_str, w, h);
	} else {
		//erase old score
		const uint8_t color = BM_XRGB(0, 0, 0);
		const auto scaled_score_y = hudctx.yscale(SB_SCORE_ADDED_Y);
		gr_rect(canvas, x, scaled_score_y, hudctx.xscale(SB_SCORE_ADDED_RIGHT), scaled_score_y + LINE_SPACING(*canvas.cv_font, *GAME_FONT), color);
		score_time = 0;
		score_display = 0;
	}
}

namespace dsx {

//	-----------------------------------------------------------------------------
void play_homing_warning(const player_info &player_info)
{
	fix beep_delay;
	static fix64 Last_warning_beep_time = 0; // Time we last played homing missile warning beep.

	if (Endlevel_sequence || Player_dead_state != player_dead_state::no)
		return;

	const auto homing_object_dist = player_info.homing_object_dist;
	if (homing_object_dist >= 0) {
		beep_delay = homing_object_dist / 128;
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

}

//	-----------------------------------------------------------------------------
static void show_homing_warning(const hud_draw_context_hs_mr hudctx, const int homing_object_dist)
{
	unsigned gauge;
	if (Endlevel_sequence)
	{
		gauge = GAUGE_HOMING_WARNING_OFF;
	}
	else
	{
		gauge = ((GameTime64 & 0x4000) && homing_object_dist >= 0)
			? GAUGE_HOMING_WARNING_ON
			: GAUGE_HOMING_WARNING_OFF;
	}
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, HOMING_WARNING_X, HOMING_WARNING_Y, gauge);
}

static void hud_show_homing_warning(grs_canvas &canvas, const int homing_object_dist)
{
	if (homing_object_dist >= 0)
	{
		if (GameTime64 & 0x4000) {
			gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
			auto &game_font = *GAME_FONT;
			gr_string(canvas, game_font, 0x8000, canvas.cv_bitmap.bm_h - LINE_SPACING(*canvas.cv_font, *GAME_FONT), TXT_LOCK);
		}
	}
}

static void hud_show_keys(const hud_draw_context_mr hudctx, const hud_ar_scale_float hud_scale_ar, const player_info &player_info)
{
	const auto player_key_flags = player_info.powerup_flags;
	if (!(player_key_flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY | PLAYER_FLAGS_RED_KEY)))
		return;
	class gauge_key
	{
		grs_bitmap *const bm;
	public:
		gauge_key(const unsigned key_icon, const local_multires_gauge_graphic multires_gauge_graphic) :
			bm(&GameBitmaps[static_cast<void>(multires_gauge_graphic), PAGE_IN_GAUGE(key_icon, multires_gauge_graphic), GET_GAUGE_INDEX(key_icon)])
		{
		}
		grs_bitmap *operator->() const
		{
			return bm;
		}
		operator grs_bitmap &() const
		{
			return *bm;
		}
	};
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const gauge_key blue(KEY_ICON_BLUE, multires_gauge_graphic);
	const unsigned y = hud_scale_ar(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h + 2) + FSPACY(1);

	const unsigned blue_bitmap_width = blue->bm_w;
	const auto &&fspacx2 = FSPACX(2);
	auto &canvas = hudctx.canvas;
	if (player_key_flags & PLAYER_FLAGS_BLUE_KEY)
		hud_bitblt_free(canvas, fspacx2, y, hud_scale_ar(blue_bitmap_width), hud_scale_ar(blue->bm_h), blue);

	if (!(player_key_flags & (PLAYER_FLAGS_GOLD_KEY | PLAYER_FLAGS_RED_KEY)))
		return;
	const gauge_key yellow(KEY_ICON_YELLOW, multires_gauge_graphic);
	const unsigned yellow_bitmap_width = yellow->bm_w;
	if (player_key_flags & PLAYER_FLAGS_GOLD_KEY)
		hud_bitblt_free(canvas, fspacx2 + hud_scale_ar(blue_bitmap_width + 3), y, hud_scale_ar(yellow_bitmap_width), hud_scale_ar(yellow->bm_h), yellow);

	if (player_key_flags & PLAYER_FLAGS_RED_KEY)
	{
		const gauge_key red(KEY_ICON_RED, multires_gauge_graphic);
		hud_bitblt_free(canvas, fspacx2 + hud_scale_ar(blue_bitmap_width + yellow_bitmap_width + 6), y, hud_scale_ar(red->bm_w), hud_scale_ar(red->bm_h), red);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
static void hud_show_orbs(grs_canvas &canvas, const player_info &player_info, const local_multires_gauge_graphic multires_gauge_graphic)
{
	if (game_mode_hoard()) {
		const auto &&fspacy1 = FSPACY(1);
		int x, y = LINE_SPACING(*canvas.cv_font, *GAME_FONT) + fspacy1;
		const auto &&hud_scale_ar = HUD_SCALE_AR(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
			x = (SWIDTH/18);
		}
		else
		{
			x = FSPACX(2);
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		}
		else if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
			y = hud_scale_ar(GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h + GameBitmaps[ GET_GAUGE_INDEX(KEY_ICON_RED) ].bm_h + 4) + fspacy1;
		}
		else
		{
			Int3();		//what sort of cockpit?
			return;
		}
		}

		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
		auto &bm = Orb_icons[multires_gauge_graphic.is_hires()];
		const auto &&scaled_width = hud_scale_ar(bm.bm_w);
		hud_bitblt_free(canvas, x, y, scaled_width, hud_scale_ar(bm.bm_h), bm);
		gr_printf(canvas, *canvas.cv_font, x + scaled_width, y, " x %d", player_info.hoard.orbs);
	}
}

static void hud_show_flag(grs_canvas &canvas, const player_info &player_info, const local_multires_gauge_graphic multires_gauge_graphic)
{
	if (game_mode_capture_flag() && (player_info.powerup_flags & PLAYER_FLAGS_FLAG)) {
		int x, y = GameBitmaps[ GET_GAUGE_INDEX(GAUGE_LIVES) ].bm_h + 2, icon;
		const auto &&fspacy1 = FSPACY(1);
		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
			x = (SWIDTH/10);
		}
		else
		{
			x = FSPACX(2);
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		}
		else if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
			y += GameBitmaps[GET_GAUGE_INDEX(KEY_ICON_RED)].bm_h + 2;
		}
		else
		{
			Int3();		//what sort of cockpit?
			return;
		}
		}

		icon = (get_team(Player_num) == TEAM_BLUE)?FLAG_ICON_RED:FLAG_ICON_BLUE;
		auto &bm = GameBitmaps[GET_GAUGE_INDEX(icon)];
		PAGE_IN_GAUGE(icon, multires_gauge_graphic);
		const auto &&hud_scale_ar = HUD_SCALE_AR(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
		hud_bitblt_free(canvas, x, hud_scale_ar(y) + fspacy1, hud_scale_ar(bm.bm_w), hud_scale_ar(bm.bm_h), bm);
	}
}
#endif

static void hud_show_energy(grs_canvas &canvas, const player_info &player_info)
{
	auto &energy = player_info.energy;
	if (PlayerCfg.HudMode == HudType::Standard || PlayerCfg.HudMode == HudType::Alternate1)
	{
		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
		auto &game_font = *GAME_FONT;
		const auto &&line_spacing = LINE_SPACING(game_font, game_font);
		gr_printf(canvas, game_font, FSPACX(1), canvas.cv_bitmap.bm_h - ((Game_mode & GM_MULTI) ? (line_spacing * (5 + (N_players > 3))) : line_spacing),"%s: %i", TXT_ENERGY, f2ir(energy));
	}

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_energy(f2ir(energy));
}

#if defined(DXX_BUILD_DESCENT_I)
static inline void hud_show_afterburner(grs_canvas &, const player_info &)
{
}
#define convert_1s(s)
#elif defined(DXX_BUILD_DESCENT_II)
static void hud_show_afterburner(grs_canvas &canvas, const player_info &player_info)
{
	int y;
	if (! (player_info.powerup_flags & PLAYER_FLAGS_AFTERBURNER))
		return;		//don't draw if don't have

	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);

	auto &game_font = *GAME_FONT;
	const auto &&line_spacing = LINE_SPACING(game_font, game_font);
	y = (Game_mode & GM_MULTI) ? (-7 * line_spacing) : (-3 * line_spacing);

	gr_printf(canvas, game_font, FSPACX(1), canvas.cv_bitmap.bm_h + y, "burn: %d%%" , fixmul(Afterburner_charge, 100));

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
			/*-fallthrough*/
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

namespace dsx {

static void show_bomb_count(grs_canvas &canvas, const player_info &player_info, const int x, const int y, const int bg_color, const int always_show, const int right_align)
{
#if defined(DXX_BUILD_DESCENT_I)
	if (!PlayerCfg.BombGauge)
		return;
#endif

	const auto bomb = which_bomb();
	int count = player_info.secondary_ammo[bomb];

	count = min(count,99);	//only have room for 2 digits - cheating give 200

	if (always_show && count == 0)		//no bombs, draw nothing on HUD
		return;

	gr_set_fontcolor(canvas, count
		? (bomb == PROXIMITY_INDEX
			? gr_find_closest_color(55, 0, 0)
			: BM_XRGB(59, 50, 21)
		)
		: bg_color,	//erase by drawing in background color
		bg_color);

	char txt[5];
	snprintf(txt, sizeof(txt), "B:%02d", count);
	//convert to wide '1'
	std::replace(&txt[2], &txt[4], '1', '\x84');

	int w, h;
	gr_get_string_size(*canvas.cv_font, txt, &w, &h, nullptr);
	gr_string(canvas, *canvas.cv_font, right_align ? x - w : x, y, txt, w, h);
}
}

static void draw_primary_ammo_info(const hud_draw_context_hs_mr hudctx, const unsigned ammo_count)
{
	int x, y;
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		x = SB_PRIMARY_AMMO_X, y = SB_PRIMARY_AMMO_Y;
	else
		x = PRIMARY_AMMO_X, y = PRIMARY_AMMO_Y;
	draw_ammo_info(hudctx.canvas, hudctx.xscale(x), hudctx.yscale(y), ammo_count);
}

namespace dcx {
namespace {

enum weapon_type
{
	primary,
	secondary,
};

constexpr rgb_t hud_rgb_red = {40, 0, 0};
constexpr rgb_t hud_rgb_green = {0, 30, 0};
constexpr rgb_t hud_rgb_dimgreen = {0, 12, 0};
constexpr rgb_t hud_rgb_gray = {6, 6, 6};
}
}

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
constexpr rgb_t hud_rgb_yellow = {30, 30, 0};
#endif

__attribute_warn_unused_result
static rgb_t hud_get_primary_weapon_fontcolor(const player_info &player_info, const int consider_weapon)
{
	if (player_info.Primary_weapon == consider_weapon)
		return hud_rgb_red;
	else{
		if (player_has_primary_weapon(player_info, consider_weapon).has_weapon())
		{
#if defined(DXX_BUILD_DESCENT_II)
			const auto is_super = (consider_weapon >= 5);
			const int base_weapon = is_super ? consider_weapon - 5 : consider_weapon;
			if (player_info.Primary_last_was_super & (1 << base_weapon))
			{
				if (is_super)
					return hud_rgb_green;
				else
					return hud_rgb_yellow;
			}
			else if (is_super)
				return hud_rgb_yellow;
			else
#endif
				return hud_rgb_green;
		}
		else
			return hud_rgb_gray;
	}
}

static void hud_set_primary_weapon_fontcolor(const player_info &player_info, const unsigned consider_weapon, grs_canvas &canvas)
{
	auto rgb = hud_get_primary_weapon_fontcolor(player_info, consider_weapon);
	gr_set_fontcolor(canvas, gr_find_closest_color(rgb.r, rgb.g, rgb.b), -1);
}

__attribute_warn_unused_result
static rgb_t hud_get_secondary_weapon_fontcolor(const player_info &player_info, const int consider_weapon)
{
	if (player_info.Secondary_weapon == consider_weapon)
		return hud_rgb_red;
	else{
		if (player_info.secondary_ammo[consider_weapon])
		{
#if defined(DXX_BUILD_DESCENT_II)
			const auto is_super = (consider_weapon >= 5);
			const int base_weapon = is_super ? consider_weapon - 5 : consider_weapon;
			if (player_info.Secondary_last_was_super & (1 << base_weapon))
			{
				if (is_super)
					return hud_rgb_green;
				else
					return hud_rgb_yellow;
			}
			else if (is_super)
				return hud_rgb_yellow;
			else
#endif
				return hud_rgb_green;
		}
		else
			return hud_rgb_dimgreen;
	}
}

static void hud_set_secondary_weapon_fontcolor(const player_info &player_info, const unsigned consider_weapon, grs_canvas &canvas)
{
	auto rgb = hud_get_secondary_weapon_fontcolor(player_info, consider_weapon);
	gr_set_fontcolor(canvas, gr_find_closest_color(rgb.r, rgb.g, rgb.b), -1);
}

__attribute_warn_unused_result
static rgb_t hud_get_vulcan_ammo_fontcolor(const player_info &player_info, const unsigned has_weapon_uses_vulcan_ammo)
{
	if (weapon_index_uses_vulcan_ammo(player_info.Primary_weapon))
		return hud_rgb_red;
	else if (has_weapon_uses_vulcan_ammo)
		return hud_rgb_green;
	else
		return hud_rgb_gray;
}

static void hud_set_vulcan_ammo_fontcolor(const player_info &player_info, const unsigned has_weapon_uses_vulcan_ammo, grs_canvas &canvas)
{
	auto rgb = hud_get_vulcan_ammo_fontcolor(player_info, has_weapon_uses_vulcan_ammo);
	gr_set_fontcolor(canvas, gr_find_closest_color(rgb.r, rgb.g, rgb.b), -1);
}

static void hud_printf_vulcan_ammo(grs_canvas &canvas, const player_info &player_info, const int x, const int y)
{
	const unsigned primary_weapon_flags = player_info.primary_weapon_flags;
	const auto vulcan_mask = HAS_VULCAN_FLAG;
#if defined(DXX_BUILD_DESCENT_I)
	const auto gauss_mask = vulcan_mask;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto gauss_mask = HAS_GAUSS_FLAG;
#endif
	const auto fmt_vulcan_ammo = vulcan_ammo_scale(player_info.vulcan_ammo);
	const unsigned has_weapon_uses_vulcan_ammo = (primary_weapon_flags & (gauss_mask | vulcan_mask));
	if (!has_weapon_uses_vulcan_ammo && !fmt_vulcan_ammo)
		return;
	hud_set_vulcan_ammo_fontcolor(player_info, has_weapon_uses_vulcan_ammo, canvas);
	const char c =
#if defined(DXX_BUILD_DESCENT_II)
		((primary_weapon_flags & gauss_mask) && ((player_info.Primary_last_was_super & (1 << primary_weapon_index_t::VULCAN_INDEX)) || !(primary_weapon_flags & vulcan_mask)))
		? 'G'
		: 
#endif
		(primary_weapon_flags & vulcan_mask)
			? 'V'
			: 'A'
	;
	gr_printf(canvas, *canvas.cv_font, x, y, "%c:%u", c, fmt_vulcan_ammo);
}

static void hud_show_primary_weapons_mode(grs_canvas &canvas, const player_info &player_info, const int vertical, const int orig_x, const int orig_y)
{
	int x=orig_x,y=orig_y;

	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	if (vertical){
		y += line_spacing * 4;
	}

	const auto &&fspacx = FSPACX();
	const auto &&fspacx3 = fspacx(3);
	const auto &&fspacy2 = FSPACY(2);
	{
		for (uint_fast32_t ui = 5; ui --;)
		{
			const auto i = static_cast<primary_weapon_index_t>(ui);
			const char *txtweapon;
			char weapon_str[10];
			hud_set_primary_weapon_fontcolor(player_info, i, canvas);
			switch(i)
			{
				case primary_weapon_index_t::LASER_INDEX:
					{
						snprintf(weapon_str, sizeof(weapon_str), "%c%i", (player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS) ? 'Q' : 'L', player_info.laser_level + 1);
					txtweapon = weapon_str;
					}
					break;
				case primary_weapon_index_t::VULCAN_INDEX:
					txtweapon = "V";
					break;
				case primary_weapon_index_t::SPREADFIRE_INDEX:
					txtweapon = "S";
					break;
				case primary_weapon_index_t::PLASMA_INDEX:
					txtweapon = "P";
					break;
				case primary_weapon_index_t::FUSION_INDEX:
					txtweapon = "F";
					break;
				default:
					continue;
			}
			int w, h;
			gr_get_string_size(*canvas.cv_font, txtweapon, &w, &h, nullptr);
			if (vertical){
				y -= h + fspacy2;
			}else
				x -= w + fspacx3;
			gr_string(canvas, *canvas.cv_font, x, y, txtweapon, w, h);
			if (i == primary_weapon_index_t::VULCAN_INDEX)
			{
				/*
				 * In Descent 1, this will always draw the ammo, but the
				 * position depends on fullscreen and, if in fullscreen,
				 * whether in vertical mode.
				 *
				 * In Descent 2, this will draw in non-fullscreen and in
				 * fullscreen non-vertical, but not in fullscreen
				 * vertical.  The fullscreen vertical case is handled
				 * specially in a large Descent2 block below.
				 */
				int vx, vy;
				if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN ? (
						vertical ? (
#if defined(DXX_BUILD_DESCENT_I)
							vx = x, vy = y, true
#else
							false
#endif
						) : (
							vx = x, vy = y - line_spacing, true
						)
					) : (
						vx = x - (w + fspacx3), vy = y - ((h + fspacy2) * 2), true
					)
				)
					hud_printf_vulcan_ammo(canvas, player_info, vx, vy);
			}
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	x = orig_x;
	y = orig_y;
	if (vertical)
	{
		x += fspacx(15);
		y += line_spacing * 4;
	}
	else
	{
		y += line_spacing;
	}

	{
		for (uint_fast32_t ui = 10; ui -- != 5;)
		{
			const auto i = static_cast<primary_weapon_index_t>(ui);
			const char *txtweapon;
			char weapon_str[10];
			hud_set_primary_weapon_fontcolor(player_info, i, canvas);
			switch(i)
			{
				case primary_weapon_index_t::SUPER_LASER_INDEX:
					txtweapon = " ";
					break;
				case primary_weapon_index_t::GAUSS_INDEX:
					txtweapon = "G";
					break;
				case primary_weapon_index_t::HELIX_INDEX:
					txtweapon = "H";
					break;
				case primary_weapon_index_t::PHOENIX_INDEX:
					txtweapon = "P";
					break;
				case primary_weapon_index_t::OMEGA_INDEX:
					if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN && (player_info.primary_weapon_flags & HAS_OMEGA_FLAG))
					{
						snprintf(weapon_str, sizeof(weapon_str), "O%3i", player_info.Omega_charge * 100 / MAX_OMEGA_CHARGE);
						txtweapon = weapon_str;
					}
					else
						txtweapon = "O";
					break;
				default:
					continue;
			}
			int w, h;
			gr_get_string_size(*canvas.cv_font, txtweapon, &w, &h, nullptr);
			if (vertical){
				y -= h + fspacy2;
			}else
				x -= w + fspacx3;
			if (i == primary_weapon_index_t::SUPER_LASER_INDEX)
			{
				if (vertical && (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN))
					hud_printf_vulcan_ammo(canvas, player_info, x, y);
				continue;
			}
			gr_string(canvas, *canvas.cv_font, x, y, txtweapon, w, h);
		}
	}
#endif
	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
}

static void hud_show_secondary_weapons_mode(grs_canvas &canvas, const player_info &player_info, const unsigned vertical, const int orig_x, const int orig_y)
{
	int x=orig_x,y=orig_y;

	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	if (vertical){
		y += line_spacing * 4;
	}

	const auto &&fspacx = FSPACX();
	const auto &&fspacx3 = fspacx(3);
	const auto &&fspacy2 = FSPACY(2);
	auto &secondary_ammo = player_info.secondary_ammo;
	{
		for (uint_fast32_t ui = 5; ui --;)
		{
			const auto i = static_cast<secondary_weapon_index_t>(ui);
			char weapon_str[10];
			hud_set_secondary_weapon_fontcolor(player_info, i, canvas);
			snprintf(weapon_str, sizeof(weapon_str), "%i", secondary_ammo[i]);
			int w, h;
			gr_get_string_size(*canvas.cv_font, weapon_str, &w, &h, nullptr);
			if (vertical){
				y -= h + fspacy2;
			}else
				x -= w + fspacx3;
			gr_string(canvas, *canvas.cv_font, x, y, weapon_str, w, h);
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	x = orig_x;
	y = orig_y;
	if (vertical)
	{
		x += fspacx(15);
		y += line_spacing * 4;
	}
	else
	{
		y += line_spacing;
	}

	{
		for (uint_fast32_t ui = 10; ui -- != 5;)
		{
			const auto i = static_cast<secondary_weapon_index_t>(ui);
			char weapon_str[10];
			hud_set_secondary_weapon_fontcolor(player_info, i, canvas);
			snprintf(weapon_str, sizeof(weapon_str), "%u", secondary_ammo[i]);
			int w, h;
			gr_get_string_size(*canvas.cv_font, weapon_str, &w, &h, nullptr);
			if (vertical){
				y -= h + fspacy2;
			}else
				x -= w + fspacx3;
			gr_string(canvas, *canvas.cv_font, x, y, weapon_str, w, h);
		}
	}
#endif
	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
}

static void hud_show_weapons(grs_canvas &canvas, const object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	int	y;
	const char	*weapon_name;
	char	weapon_str[32];

	auto &game_font = *GAME_FONT;
	gr_set_curfont(canvas, GAME_FONT);
	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);

	y = canvas.cv_bitmap.bm_h;

	const auto &&line_spacing = LINE_SPACING(game_font, game_font);
	if (Game_mode & GM_MULTI)
		y -= line_spacing * 4;

	if (PlayerCfg.HudMode == HudType::Alternate1)
	{
#if defined(DXX_BUILD_DESCENT_I)
		constexpr unsigned multiplier = 1;
#elif defined(DXX_BUILD_DESCENT_II)
		constexpr unsigned multiplier = 2;
#endif
		hud_show_primary_weapons_mode(canvas, player_info, 0, canvas.cv_bitmap.bm_w, y - (line_spacing * 2 * multiplier));
		hud_show_secondary_weapons_mode(canvas, player_info, 0, canvas.cv_bitmap.bm_w, y - (line_spacing * multiplier));
		return;
	}
	const auto &&fspacx = FSPACX();
	if (PlayerCfg.HudMode == HudType::Alternate2)
	{
		int x1,x2;
		int w;
		gr_get_string_size(game_font, "V1000", &w, nullptr, nullptr);
		gr_get_string_size(game_font, "0 ", &x2, nullptr, nullptr);
		y = canvas.cv_bitmap.bm_h / 1.75;
		x1 = canvas.cv_bitmap.bm_w / 2.1 - (fspacx(40) + w);
		x2 = canvas.cv_bitmap.bm_w / 1.9 + (fspacx(42) + x2);
		hud_show_primary_weapons_mode(canvas, player_info, 1, x1, y);
		hud_show_secondary_weapons_mode(canvas, player_info, 1, x2, y);
		gr_set_fontcolor(canvas, BM_XRGB(14, 14, 23), -1);
		gr_printf(canvas, game_font, x2, y - (line_spacing * 4), "%i", f2ir(plrobj.shields));
		gr_set_fontcolor(canvas, BM_XRGB(25, 18, 6), -1);
		gr_printf(canvas, game_font, x1, y - (line_spacing * 4), "%i", f2ir(player_info.energy));
	}
	else
	{
		const char *disp_primary_weapon_name;
		const auto Primary_weapon = player_info.Primary_weapon;

		weapon_name = PRIMARY_WEAPON_NAMES_SHORT(Primary_weapon);
		switch (Primary_weapon) {
			case primary_weapon_index_t::LASER_INDEX:
				{
				if (player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS)
					snprintf(weapon_str, sizeof(weapon_str), "%s %s %i", TXT_QUAD, weapon_name, player_info.laser_level + 1);
				else
					snprintf(weapon_str, sizeof(weapon_str), "%s %i", weapon_name, player_info.laser_level + 1);
				}
				disp_primary_weapon_name = weapon_str;
				break;

			case primary_weapon_index_t::VULCAN_INDEX:
#if defined(DXX_BUILD_DESCENT_II)
			case primary_weapon_index_t::GAUSS_INDEX:
#endif
				snprintf(weapon_str, sizeof(weapon_str), "%s: %u", weapon_name, vulcan_ammo_scale(player_info.vulcan_ammo));
				convert_1s(weapon_str);
				disp_primary_weapon_name = weapon_str;
				break;

			case primary_weapon_index_t::SPREADFIRE_INDEX:
			case primary_weapon_index_t::PLASMA_INDEX:
			case primary_weapon_index_t::FUSION_INDEX:
#if defined(DXX_BUILD_DESCENT_II)
			case primary_weapon_index_t::HELIX_INDEX:
			case primary_weapon_index_t::PHOENIX_INDEX:
#endif
				disp_primary_weapon_name = weapon_name;
				break;
#if defined(DXX_BUILD_DESCENT_II)
			case primary_weapon_index_t::OMEGA_INDEX:
				snprintf(weapon_str, sizeof(weapon_str), "%s: %03i", weapon_name, player_info.Omega_charge * 100 / MAX_OMEGA_CHARGE);
				convert_1s(weapon_str);
				disp_primary_weapon_name = weapon_str;
				break;

			case primary_weapon_index_t::SUPER_LASER_INDEX:	//no such thing as super laser
#endif
			default:
				Int3();
				disp_primary_weapon_name = "";
				break;
		}

		int	w, h;
		gr_get_string_size(game_font, disp_primary_weapon_name, &w, &h, nullptr);
		const auto &&bmwx = canvas.cv_bitmap.bm_w - fspacx(1);
		gr_string(canvas, game_font, bmwx - w, y - (line_spacing * 2), disp_primary_weapon_name, w, h);
		const char *disp_secondary_weapon_name;

		auto &Secondary_weapon = player_info.Secondary_weapon;
		disp_secondary_weapon_name = SECONDARY_WEAPON_NAMES_VERY_SHORT(Secondary_weapon);

		snprintf(weapon_str, sizeof(weapon_str), "%s %u", disp_secondary_weapon_name, player_info.secondary_ammo[Secondary_weapon]);
		gr_get_string_size(game_font, weapon_str, &w, &h, nullptr);
		gr_string(canvas, game_font, bmwx - w, y - line_spacing, weapon_str, w, h);

		show_bomb_count(canvas, player_info, bmwx, y - (line_spacing * 3), -1, 1, 1);
	}
}
}

static void hud_show_cloak_invuln(grs_canvas &canvas, const player_flags player_flags, const fix64 cloak_time, const fix64 invulnerable_time)
{
	if (!(player_flags & (PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE)))
		return;
	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	const auto base_y = canvas.cv_bitmap.bm_h - ((Game_mode & GM_MULTI) ? line_spacing * 8 : line_spacing * 4);
	const auto gametime64 = GameTime64;
	const auto &&fspacx1 = FSPACX(1);

	const auto cloak_invul_timer = show_cloak_invul_timer();
	const auto a = [&](const fix64 effect_end, int y, const char *txt) {
		if (cloak_invul_timer)
			gr_printf(canvas, *canvas.cv_font, fspacx1, y, "%s: %lu", txt, static_cast<unsigned long>(effect_end / F1_0));
		else
			gr_string(canvas, *canvas.cv_font, fspacx1, y, txt);
	};

	if (player_flags & PLAYER_FLAGS_CLOAKED)
	{
		const fix64 effect_end = cloak_time + CLOAK_TIME_MAX - gametime64;
		if (effect_end > F1_0*3 || gametime64 & 0x8000)
		{
			a(effect_end, base_y, TXT_CLOAKED);
		}
	}

	if (player_flags & PLAYER_FLAGS_INVULNERABLE)
	{
		const fix64 effect_end = invulnerable_time + INVULNERABLE_TIME_MAX - gametime64;
		if (effect_end > F1_0*4 || gametime64 & 0x8000)
		{
			a(effect_end, base_y - line_spacing, TXT_INVULNERABLE);
		}
	}
}

static void hud_show_cloak_invuln(grs_canvas &canvas, const player_info &player_info)
{
	hud_show_cloak_invuln(canvas, player_info.powerup_flags, player_info.cloak_time, player_info.invulnerable_time);
}

static void hud_show_shield(grs_canvas &canvas, const object &plrobj)
{
	if (PlayerCfg.HudMode == HudType::Standard || PlayerCfg.HudMode == HudType::Alternate1)
	{
		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);

		auto &game_font = *GAME_FONT;
		const auto &&line_spacing = LINE_SPACING(game_font, game_font);
		const auto shields = plrobj.shields;
		gr_printf(canvas, game_font, FSPACX(1), canvas.cv_bitmap.bm_h - ((Game_mode & GM_MULTI) ? line_spacing * (6 + (N_players > 3)) : line_spacing * 2), "%s: %i", TXT_SHIELD, shields >= 0 ? f2ir(shields) : 0);
	}

	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_shields(f2ir(plrobj.shields));
}

//draw the icons for number of lives
static void hud_show_lives(const hud_draw_context_hs_mr hudctx, const hud_ar_scale_float hud_scale_ar, const player_info &player_info)
{
	if (HUD_toolong)
		return;

	const int x = (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
		? static_cast<int>(hudctx.xscale(7))
		: static_cast<int>(FSPACX(2));

	auto &canvas = hudctx.canvas;
	if (Game_mode & GM_MULTI) {
		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
		auto &game_font = *GAME_FONT;
		gr_printf(canvas, game_font, x, FSPACY(1), "%s: %d", TXT_DEATHS, player_info.net_killed_total);
	}
	else if (const unsigned lives = get_local_player().lives - 1)
	{
		gr_set_curfont(canvas, GAME_FONT);
		gr_set_fontcolor(canvas, BM_XRGB(0, 20, 0), -1);
#if defined(DXX_BUILD_DESCENT_II)
		auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
#endif
		PAGE_IN_GAUGE(GAUGE_LIVES, multires_gauge_graphic);
		auto &bm = GameBitmaps[GET_GAUGE_INDEX(GAUGE_LIVES)];
		const auto &&fspacy1 = FSPACY(1);
		hud_bitblt_free(canvas, x, fspacy1, hud_scale_ar(bm.bm_w), hud_scale_ar(bm.bm_h), bm);
		auto &game_font = *GAME_FONT;
		gr_printf(canvas, game_font, hud_scale_ar(bm.bm_w) + x, fspacy1, " x %u", lives);
	}
}

static void sb_show_lives(const hud_draw_context_hs_mr hudctx, const hud_ar_scale_float hud_scale_ar, const player_info &player_info)
{
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const auto y = SB_LIVES_Y;

	auto &canvas = hudctx.canvas;
	gr_set_fontcolor(canvas, BM_XRGB(0, 20, 0), -1);
	const auto scaled_y = hudctx.yscale(y);
	auto &game_font = *GAME_FONT;
	gr_printf(canvas, game_font, hudctx.xscale(SB_LIVES_LABEL_X), scaled_y, "%s:", (Game_mode & GM_MULTI) ? TXT_DEATHS : TXT_LIVES);

	const uint8_t color = BM_XRGB(0,0,0);
	const auto scaled_score_right = hudctx.xscale(SB_SCORE_RIGHT);
	if (Game_mode & GM_MULTI)
	{
		char killed_str[20];
		static array<int, 4> last_x{{SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H}};

		snprintf(killed_str, sizeof(killed_str), "%5d", player_info.net_killed_total);
		int w, h;
		gr_get_string_size(game_font, killed_str, &w, &h, nullptr);
		const auto x = scaled_score_right - w - FSPACX(1);
		gr_rect(canvas, exchange(last_x[multires_gauge_graphic.is_hires()], x), scaled_y, scaled_score_right, scaled_y + LINE_SPACING(game_font, game_font), color);
		gr_string(canvas, game_font, x, scaled_y, killed_str, w, h);
		return;
	}

	const int x = SB_LIVES_X;
	//erase old icons
	auto &bm = GameBitmaps[GET_GAUGE_INDEX(GAUGE_LIVES)];
	const auto scaled_x = hudctx.xscale(x);
	gr_rect(canvas, scaled_x, scaled_y, scaled_score_right, hudctx.yscale(y + bm.bm_h), color);

	const auto lives = get_local_player().lives - 1;
	if (lives > 0) {
		PAGE_IN_GAUGE(GAUGE_LIVES, multires_gauge_graphic);
		const auto scaled_width = hud_scale_ar(bm.bm_w);
		hud_bitblt_free(canvas, scaled_x, scaled_y, scaled_width, hud_scale_ar(bm.bm_h), bm);
		gr_printf(canvas, game_font, scaled_x + scaled_width, scaled_y, " x %d", lives);
	}
}

#ifndef RELEASE
static void show_time(grs_canvas &canvas)
{
	auto &plr = get_local_player();
	const unsigned secs = f2i(plr.time_level) % 60;
	const unsigned mins = f2i(plr.time_level) / 60;

	if (Color_0_31_0 == -1)
		Color_0_31_0 = BM_XRGB(0,31,0);
	gr_set_fontcolor(canvas, Color_0_31_0, -1);
	auto &game_font = *GAME_FONT;
	gr_printf(canvas, game_font, FSPACX(2), (LINE_SPACING(*canvas.cv_font, *GAME_FONT) * 15), "%d:%02d", mins, secs);
}
#endif

#define EXTRA_SHIP_SCORE	50000		//get new ship every this many points

static void common_add_points_to_score(const int points, int &score)
{
	if (points == 0 || cheats.enabled)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_score(points);

	const auto prev_score = score;
	score += points;

	if (Game_mode & GM_MULTI)
		return;

	const auto current_ship_score = score / EXTRA_SHIP_SCORE;
	const auto previous_ship_score = prev_score / EXTRA_SHIP_SCORE;
	if (current_ship_score != previous_ship_score)
	{
		int snd;
		get_local_player().lives += current_ship_score - previous_ship_score;
		powerup_basic_str(20, 20, 20, 0, TXT_EXTRA_LIFE);
		if ((snd=Powerup_info[POW_EXTRA_LIFE].hit_sound) > -1 )
			digi_play_sample( snd, F1_0 );
	}
}

namespace dsx {

void add_points_to_score(player_info &player_info, int points)
{
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;
	score_time += f1_0*2;
	score_display += points;
	if (score_time > f1_0*4) score_time = f1_0*4;

	common_add_points_to_score(points, player_info.mission.score);
	if (Game_mode & GM_MULTI_COOP)
		multi_send_score();
}

/* This is only called in single player when the player is between
 * levels.
 */
void add_bonus_points_to_score(player_info &player_info, int points)
{
	assert(!(Game_mode & GM_MULTI));
	common_add_points_to_score(points, player_info.mission.score);
}

}

// Decode cockpit bitmap to deccpt and add alpha fields to weapon boxes (as it should have always been) so we later can render sub bitmaps over the window canvases
static void cockpit_decode_alpha(const hud_draw_context_mr hudctx, grs_bitmap *const bm)
{
	static const uint8_t *cur;
	static uint16_t cur_w, cur_h;
#ifndef DXX_MAX_COCKPIT_BITMAP_SIZE
	/* 640 wide by 480 high should be enough for all bitmaps shipped
	 * with shareware or commercial data.
	 * Use a #define so that the value can be easily overridden at build
	 * time.
	 */
#define DXX_MAX_COCKPIT_BITMAP_SIZE	(640 * 480)
#endif
	static array<uint8_t, DXX_MAX_COCKPIT_BITMAP_SIZE> cockpitbuf;

	const unsigned bm_h = bm->bm_h;
	if (unlikely(!bm_h))
		/* Invalid, but later code has undefined results if bm_h==0 */
		return;
	const unsigned bm_w = bm->bm_w;
	// check if we processed this bitmap already
	if (cur == bm->bm_data && cur_w == bm_w && cur_h == bm_h)
		return;

	cockpitbuf = {};

	// decode the bitmap
	if (bm->get_flag_mask(BM_FLAG_RLE))
	{
		if (!bm_rle_expand(*bm).loop(bm_w, bm_rle_expand_range(cockpitbuf)))
		{
				/* Out of space.  Return without adjusting the bitmap.
				 * The result will look ugly, but run correctly.
				 */
				con_printf(CON_URGENT, __FILE__ ":%u: BUG: RLE-encoded bitmap with size %hux%hu exceeds decode buffer size %" DXX_PRI_size_type, __LINE__, static_cast<uint16_t>(bm_w), static_cast<uint16_t>(bm_h), cockpitbuf.size());
				return;
		}
	}
	else
	{
		const std::size_t len = bm_w * bm_h;
		if (len > cockpitbuf.size())
		{
			con_printf(CON_URGENT, __FILE__ ":%u: BUG: RLE-encoded bitmap with size %hux%hu exceeds decode buffer size %" DXX_PRI_size_type, __LINE__, static_cast<uint16_t>(bm_w), static_cast<uint16_t>(bm_h), cockpitbuf.size());
			return;
		}
		memcpy(cockpitbuf.data(), bm->bm_data, len);
	}

	// add alpha color to the pixels which are inside the window box spans
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const unsigned lower_y = ((multires_gauge_graphic.get(364, 151)));
	unsigned i = bm_w * lower_y;
	const auto fill_alpha_one_line = [](unsigned o, const span &s) {
		std::fill_n(&cockpitbuf[o + s.l], s.r - s.l + 1, TRANSPARENCY_COLOR);
	};
	range_for (auto &s,
		multires_gauge_graphic.is_hires()
		? make_range(weapon_windows_hires)
		: make_range(weapon_windows_lowres)
	)
	{
		fill_alpha_one_line(i, s.l);
		fill_alpha_one_line(i, s.r);
		i += bm_w;
	}
#if DXX_USE_OGL
	ogl_freebmtexture(*bm);
#endif
	gr_init_bitmap(deccpt, bm_mode::linear, 0, 0, bm_w, bm_h, bm_w, cockpitbuf.data());
	gr_set_transparent(deccpt,1);
#if DXX_USE_OGL
	ogl_ubitmapm_cs(hudctx.canvas, 0, 0, -1, -1, deccpt, 255, F1_0); // render one time to init the texture
#endif
	WinBoxOverlay[0] = gr_create_sub_bitmap(deccpt,(PRIMARY_W_BOX_LEFT)-2,(PRIMARY_W_BOX_TOP)-2,(PRIMARY_W_BOX_RIGHT-PRIMARY_W_BOX_LEFT+4),(PRIMARY_W_BOX_BOT-PRIMARY_W_BOX_TOP+4));
	WinBoxOverlay[1] = gr_create_sub_bitmap(deccpt,(SECONDARY_W_BOX_LEFT)-2,(SECONDARY_W_BOX_TOP)-2,(SECONDARY_W_BOX_RIGHT-SECONDARY_W_BOX_LEFT)+4,(SECONDARY_W_BOX_BOT-SECONDARY_W_BOX_TOP)+4);

	cur = bm->get_bitmap_data();
	cur_w = bm_w;
	cur_h = bm_h;
}

namespace dsx {
static void draw_wbu_overlay(const hud_draw_context_hs_mr hudctx)
{
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
#if defined(DXX_BUILD_DESCENT_I)
	unsigned cockpit_idx = PlayerCfg.CockpitMode[1];
#elif defined(DXX_BUILD_DESCENT_II)
	unsigned cockpit_idx = PlayerCfg.CockpitMode[1]+(multires_gauge_graphic.is_hires() ? (Num_cockpits / 2) : 0);
#endif
	PIGGY_PAGE_IN(cockpit_bitmap[cockpit_idx]);
	grs_bitmap *bm = &GameBitmaps[cockpit_bitmap[cockpit_idx].index];

	cockpit_decode_alpha(hudctx, bm);

	if (WinBoxOverlay[0])
		hud_bitblt(hudctx, PRIMARY_W_BOX_LEFT - 2, PRIMARY_W_BOX_TOP - 2, *WinBoxOverlay[0].get());
	if (WinBoxOverlay[1])
		hud_bitblt(hudctx, SECONDARY_W_BOX_LEFT - 2, SECONDARY_W_BOX_TOP - 2, *WinBoxOverlay[1].get());
}
}

void close_gauges()
{
	WinBoxOverlay = {};
}

namespace dsx {
void init_gauges()
{
	old_weapon[0] = old_weapon[1] = -1;
	old_laser_level	= -1;
#if defined(DXX_BUILD_DESCENT_II)
	weapon_box_user[0] = weapon_box_user[1] = WBU_WEAPON;
#endif
}
}

static void draw_energy_bar(const hud_draw_context_hs_mr hudctx, const int energy)
{
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const int not_energy = hudctx.xscale(multires_gauge_graphic.is_hires() ? (125 - (energy * 125) / 100) : (63 - (energy * 63) / 100));
	const double aplitscale = static_cast<double>(hudctx.xscale(65) / hudctx.yscale(8)) / (65 / 8); //scale amplitude of energy bar to current resolution aspect

	// Draw left energy bar
	hud_gauge_bitblt(hudctx, LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y, GAUGE_ENERGY_LEFT);

	const auto color = BM_XRGB(0, 0, 0);

	if (energy < 100)
	{
		const auto xscale_energy_gauge_x = hudctx.xscale(LEFT_ENERGY_GAUGE_X);
		const auto xscale_energy_gauge_w = hudctx.xscale(LEFT_ENERGY_GAUGE_W);
		const auto xscale_energy_gauge_h2 = hudctx.xscale(LEFT_ENERGY_GAUGE_H - 2);
		const auto yscale_energy_gauge_y = hudctx.yscale(LEFT_ENERGY_GAUGE_Y);
		const auto yscale_energy_gauge_h = hudctx.yscale(LEFT_ENERGY_GAUGE_H);
		for (unsigned y = 0; y < yscale_energy_gauge_h; ++y)
		{
			const auto bound = xscale_energy_gauge_w - (y * aplitscale) / 3;
			const auto x1 = xscale_energy_gauge_h2 - y * aplitscale;
			const auto x2 = std::min(x1 + not_energy, bound);

			if (x2 > x1)
			{
				const auto ly = i2f(y + yscale_energy_gauge_y);
				gr_uline(*grd_curcanv, i2f(x1 + xscale_energy_gauge_x), ly, i2f(x2 + xscale_energy_gauge_x), ly, color);
			}
		}
	}

	// Draw right energy bar
	hud_gauge_bitblt(hudctx, RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y, GAUGE_ENERGY_RIGHT);

	if (energy < 100)
	{
		const auto xscale_energy_gauge_x = hudctx.xscale(RIGHT_ENERGY_GAUGE_X);
		const auto yscale_energy_gauge_y = hudctx.yscale(RIGHT_ENERGY_GAUGE_Y);
		const auto yscale_energy_gauge_h = hudctx.yscale(RIGHT_ENERGY_GAUGE_H);
		const auto xscale_right_energy = hudctx.xscale(RIGHT_ENERGY_GAUGE_W - RIGHT_ENERGY_GAUGE_H + 2);
		for (unsigned y = 0; y < yscale_energy_gauge_h; ++y)
		{
			const auto bound = (y * aplitscale) / 3;
			const auto x2 = xscale_right_energy + y * aplitscale;
			auto x1 = x2 - not_energy;

			if (x1 < bound)
				x1 = bound;

			if (x2 > x1)
			{
				const auto ly = i2f(y + yscale_energy_gauge_y);
				gr_uline(*grd_curcanv, i2f(x1 + xscale_energy_gauge_x), ly, i2f(x2 + xscale_energy_gauge_x), ly, color);
			}
		}
	}

	gr_set_default_canvas();
}

#if defined(DXX_BUILD_DESCENT_II)
static void draw_afterburner_bar(const hud_draw_context_hs_mr hudctx, const int afterburner)
{
	struct lr
	{
		uint8_t l, r;
	};
	static const array<lr, AFTERBURNER_GAUGE_H_L> afterburner_bar_table = {{
		{3, 11},
		{3, 11},
		{3, 11},
		{3, 11},
		{3, 11},
		{3, 11},
		{2, 11},
		{2, 10},
		{2, 10},
		{2, 10},
		{2, 10},
		{2, 10},
		{2, 10},
		{1, 10},
		{1, 10},
		{1, 10},
		{1, 9},
		{1, 9},
		{1, 9},
		{1, 9},
		{0, 9},
		{0, 9},
		{0, 8},
		{0, 8},
		{0, 8},
		{0, 8},
		{1, 8},
		{2, 8},
		{3, 8},
		{4, 8},
		{5, 8},
		{6, 7}
	}};
	static const array<lr, AFTERBURNER_GAUGE_H_H> afterburner_bar_table_hires = {{
		{5, 20},
		{5, 20},
		{5, 19},
		{5, 19},
		{5, 19},
		{5, 19},
		{4, 19},
		{4, 19},
		{4, 19},
		{4, 19},
		{4, 19},
		{4, 18},
		{4, 18},
		{4, 18},
		{4, 18},
		{3, 18},
		{3, 18},
		{3, 18},
		{3, 18},
		{3, 18},
		{3, 18},
		{3, 17},
		{3, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 17},
		{2, 16},
		{2, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 16},
		{1, 15},
		{1, 15},
		{1, 15},
		{0, 15},
		{0, 15},
		{0, 15},
		{0, 15},
		{0, 15},
		{0, 15},
		{0, 14},
		{0, 14},
		{0, 14},
		{1, 14},
		{2, 14},
		{3, 14},
		{4, 14},
		{5, 14},
		{6, 13},
		{7, 13},
		{8, 13},
		{9, 13},
		{10, 13},
		{11, 13},
		{12, 13}
	}};

	// Draw afterburner bar
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	const auto afterburner_gauge_x = AFTERBURNER_GAUGE_X;
	const auto afterburner_gauge_y = AFTERBURNER_GAUGE_Y;
	const auto &&table = multires_gauge_graphic.is_hires()
		? std::make_pair(afterburner_bar_table_hires.data(), afterburner_bar_table_hires.size())
		: std::make_pair(afterburner_bar_table.data(), afterburner_bar_table.size());
	hud_gauge_bitblt(hudctx, afterburner_gauge_x, afterburner_gauge_y, GAUGE_AFTERBURNER);
	const unsigned not_afterburner = fixmul(f1_0 - afterburner, table.second);
	if (not_afterburner > table.second)
		return;
	const uint8_t color = BM_XRGB(0, 0, 0);
	const int base_top = hudctx.yscale(afterburner_gauge_y - 1);
	const int base_bottom = hudctx.yscale(afterburner_gauge_y);
	int y = 0;
	range_for (auto &ab, unchecked_partial_range(table.first, not_afterburner))
	{
		const int left = hudctx.xscale(afterburner_gauge_x + ab.l);
		const int right = hudctx.xscale(afterburner_gauge_x + ab.r + 1);
		for (int i = hudctx.yscale(y), j = hudctx.yscale(++y); i < j; ++i)
		{
			gr_rect(hudctx.canvas, left, base_top + i, right, base_bottom + i, color);
		}
	}
}
#endif

static void draw_shield_bar(const hud_draw_context_hs_mr hudctx, const int shield)
{
	int bm_num = shield>=100?9:(shield / 10);
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, SHIELD_GAUGE_X, SHIELD_GAUGE_Y, GAUGE_SHIELDS + 9 - bm_num);
}

static void show_cockpit_cloak_invul_timer(grs_canvas &canvas, const fix64 effect_end, const int y)
{
	char countdown[8];
	int ow, oh;
	snprintf(countdown, sizeof(countdown), "%lu", static_cast<unsigned long>(effect_end / F1_0));
	gr_set_fontcolor(canvas, BM_XRGB(31, 31, 31), -1);
	gr_get_string_size(*canvas.cv_font, countdown, &ow, &oh, nullptr);
	const int x = grd_curscreen->get_screen_width() / (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR
		? 2.266
		: 1.951
	);
	gr_string(canvas, *canvas.cv_font, x - (ow / 2), y, countdown, ow, oh);
}

#define CLOAK_FADE_WAIT_TIME  0x400

namespace dsx {

static void draw_player_ship(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const int cloak_state, const int x, const int y)
{
	static fix cloak_fade_timer=0;
	static int cloak_fade_value=GR_FADE_LEVELS-1;

	if (cloak_state)
	{
		static int step = 0;
		const auto cloak_time = player_info.cloak_time;

		if (GameTime64 - cloak_time < F1_0)
		{
			step = -2;
		}
		else if (cloak_time + CLOAK_TIME_MAX - GameTime64 <= F1_0*3)
		{
			if (cloak_fade_value >= static_cast<signed>(GR_FADE_LEVELS-1))
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

		if (cloak_fade_value > static_cast<signed>(GR_FADE_LEVELS-1))
			cloak_fade_value = (GR_FADE_LEVELS-1);
		if (cloak_fade_value <= 0)
			cloak_fade_value = 0;
	}
	else
	{
		cloak_fade_timer = 0;
		cloak_fade_value = GR_FADE_LEVELS-1;
	}

	const auto color = get_player_or_team_color(Player_num);
#if defined(DXX_BUILD_DESCENT_II)
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
#endif
	PAGE_IN_GAUGE(GAUGE_SHIPS+color, multires_gauge_graphic);
	auto &bm = GameBitmaps[GET_GAUGE_INDEX(GAUGE_SHIPS+color)];
	hud_bitblt(hudctx, x, y, bm);
	auto &canvas = hudctx.canvas;
	gr_settransblend(canvas, cloak_fade_value, GR_BLEND_NORMAL);
	gr_rect(canvas, hudctx.xscale(x - 3), hudctx.yscale(y - 3), hudctx.xscale(x + bm.bm_w + 3), hudctx.yscale(y + bm.bm_h + 3), 0);
	gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
        // Show Cloak Timer if enabled
	if (cloak_fade_value < GR_FADE_LEVELS/2 && show_cloak_invul_timer())
		show_cockpit_cloak_invul_timer(canvas, player_info.cloak_time + CLOAK_TIME_MAX - GameTime64, hudctx.yscale(y + (bm.bm_h / 2)));
}

}

#define INV_FRAME_TIME	(f1_0/10)		//how long for each frame

namespace dcx {

static const char *get_gauge_width_string(const unsigned v)
{
	if (v > 199)
		return "200";
	return &"100"[(v > 99)
		? 0
		: (v > 9) ? 1 : 2
	];
}

}

static void draw_numerical_display(const draw_numerical_display_draw_context hudctx, const int shield, const int energy)
{
	auto &canvas = hudctx.canvas;
#if !DXX_USE_OGL
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y, GAUGE_NUMERICAL);
#endif
	// cockpit is not 100% geometric so we need to divide shield and energy X position by 1.951 which should be most accurate
	// gr_get_string_size is used so we can get the numbers finally in the correct position with sw and ew
	const int xb = grd_curscreen->get_screen_width() / 1.951;
	auto &game_font = *GAME_FONT;
	const auto a = [&canvas, &game_font, xb](int v, int y) {
		int w;
		gr_get_string_size(game_font, get_gauge_width_string(v), &w, nullptr, nullptr);
		gr_printf(canvas, game_font, xb - (w / 2), y, "%d", v);
	};
	gr_set_fontcolor(canvas, BM_XRGB(14, 14, 23), -1);
	const auto screen_height = grd_curscreen->get_screen_height();
	a(shield, screen_height / 1.365);

	gr_set_fontcolor(canvas, BM_XRGB(25, 18, 6), -1);
	a(energy, screen_height / 1.5);
}

void draw_keys_state::draw_all_cockpit_keys()
{
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	draw_one_key(GAUGE_BLUE_KEY_X, GAUGE_BLUE_KEY_Y, GAUGE_BLUE_KEY, PLAYER_FLAGS_BLUE_KEY);
	draw_one_key(GAUGE_GOLD_KEY_X, GAUGE_GOLD_KEY_Y, GAUGE_GOLD_KEY, PLAYER_FLAGS_GOLD_KEY);
	draw_one_key(GAUGE_RED_KEY_X, GAUGE_RED_KEY_Y, GAUGE_RED_KEY, PLAYER_FLAGS_RED_KEY);
}

namespace dsx {

static void draw_weapon_info_sub(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const int info_index, const gauge_box *const box, const int pic_x, const int pic_y, const char *const name, const int text_x, const int text_y)
{
	//clear the window
	const uint8_t color = BM_XRGB(0, 0, 0);
	{
#if defined(DXX_BUILD_DESCENT_I)
		constexpr unsigned bottom_bias = 1;
#elif defined(DXX_BUILD_DESCENT_II)
		constexpr unsigned bottom_bias = 0;
#endif
		gr_rect(hudctx.canvas, hudctx.xscale(box->left), hudctx.yscale(box->top), hudctx.xscale(box->right), hudctx.yscale(box->bot + bottom_bias), color);
	}
	const auto &picture = 
#if defined(DXX_BUILD_DESCENT_II)
	// !SHAREWARE
		(Piggy_hamfile_version >= 3 && hudctx.multires_gauge_graphic.is_hires()) ?
			Weapon_info[info_index].hires_picture :
#endif
			Weapon_info[info_index].picture;
	PIGGY_PAGE_IN(picture);
	auto &bm = GameBitmaps[picture.index];

	hud_bitblt(hudctx, pic_x, pic_y, bm);

	if (PlayerCfg.HudMode == HudType::Standard)
	{
		auto &canvas = hudctx.canvas;
		gr_set_fontcolor(canvas, BM_XRGB(0, 20, 0), -1);

		gr_string(canvas, *canvas.cv_font, text_x, text_y, name);

		//	For laser, show level and quadness
#if defined(DXX_BUILD_DESCENT_I)
		if (info_index == primary_weapon_index_t::LASER_INDEX)
#elif defined(DXX_BUILD_DESCENT_II)
		if (info_index == weapon_id_type::LASER_ID || info_index == weapon_id_type::SUPER_LASER_ID)
#endif
		{
			const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
			gr_printf(canvas, *canvas.cv_font, text_x, text_y + line_spacing, "%s: %i", TXT_LVL, player_info.laser_level + 1);
			if (player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS)
				gr_string(canvas, *canvas.cv_font, text_x, text_y + (line_spacing * 2), TXT_QUAD);
		}
	}
}

static void draw_primary_weapon_info(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const unsigned weapon_num, const unsigned laser_level)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)laser_level;
#endif
	int x,y;

	{
		const auto weapon_id = Primary_weapon_to_weapon_info[weapon_num];
		const auto info_index = 
#if defined(DXX_BUILD_DESCENT_II)
			(weapon_id == weapon_id_type::LASER_ID && laser_level > MAX_LASER_LEVEL)
			? weapon_id_type::SUPER_LASER_ID
			:
#endif
			weapon_id;

		const gauge_box *box;
		int pic_x, pic_y, text_x, text_y;
		auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			box = &gauge_boxes[SB_PRIMARY_BOX];
			pic_x = SB_PRIMARY_W_PIC_X;
			pic_y = SB_PRIMARY_W_PIC_Y;
			text_x = SB_PRIMARY_W_TEXT_X;
			text_y = SB_PRIMARY_W_TEXT_Y;
			x=SB_PRIMARY_AMMO_X;
			y=SB_PRIMARY_AMMO_Y;
		}
		else
		{
			box = &gauge_boxes[COCKPIT_PRIMARY_BOX];
			pic_x = PRIMARY_W_PIC_X;
			pic_y = PRIMARY_W_PIC_Y;
			text_x = PRIMARY_W_TEXT_X;
			text_y = PRIMARY_W_TEXT_Y;
			x=PRIMARY_AMMO_X;
			y=PRIMARY_AMMO_Y;
		}
		draw_weapon_info_sub(hudctx, player_info, info_index, box, pic_x, pic_y, PRIMARY_WEAPON_NAMES_SHORT(weapon_num), hudctx.xscale(text_x), hudctx.yscale(text_y));
		if (PlayerCfg.HudMode != HudType::Standard)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (weapon_box_user[0] == WBU_WEAPON)
#endif
				hud_show_primary_weapons_mode(hudctx.canvas, player_info, 1, hudctx.xscale(x), hudctx.yscale(y));
		}
	}
}

static void draw_secondary_weapon_info(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const unsigned weapon_num)
{
	int x,y;
	int info_index;

	{
		info_index = Secondary_weapon_to_weapon_info[weapon_num];
		const gauge_box *box;
		int pic_x, pic_y, text_x, text_y;
		auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
		if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		{
			box = &gauge_boxes[SB_SECONDARY_BOX];
			pic_x = SB_SECONDARY_W_PIC_X;
			pic_y = SB_SECONDARY_W_PIC_Y;
			text_x = SB_SECONDARY_W_TEXT_X;
			text_y = SB_SECONDARY_W_TEXT_Y;
			x=SB_SECONDARY_AMMO_X;
			y=SB_SECONDARY_AMMO_Y;
		}
		else
		{
			box = &gauge_boxes[COCKPIT_SECONDARY_BOX];
			pic_x = SECONDARY_W_PIC_X;
			pic_y = SECONDARY_W_PIC_Y;
			text_x = SECONDARY_W_TEXT_X;
			text_y = SECONDARY_W_TEXT_Y;
			x=SECONDARY_AMMO_X;
			y=SECONDARY_AMMO_Y;
		}
		draw_weapon_info_sub(hudctx, player_info, info_index, box, pic_x, pic_y, SECONDARY_WEAPON_NAMES_SHORT(weapon_num), hudctx.xscale(text_x), hudctx.yscale(text_y));
		if (PlayerCfg.HudMode != HudType::Standard)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (weapon_box_user[1] == WBU_WEAPON)
#endif
				hud_show_secondary_weapons_mode(hudctx.canvas, player_info, 1, hudctx.xscale(x), hudctx.yscale(y));
		}
	}
}

static void draw_weapon_info(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const unsigned weapon_num, const unsigned laser_level, const weapon_type wt)
{
	if (wt == weapon_type::primary)
		draw_primary_weapon_info(hudctx, player_info, weapon_num, laser_level);
	else
		draw_secondary_weapon_info(hudctx, player_info, weapon_num);
}

}

static void draw_ammo_info(grs_canvas &canvas, const unsigned x, const unsigned y, const unsigned ammo_count)
{
	if (PlayerCfg.HudMode == HudType::Standard)
	{
		gr_set_fontcolor(canvas, BM_XRGB(20, 0, 0), -1);
		gr_printf(canvas, *canvas.cv_font, x, y, "%03u", ammo_count);
	}
}

static void draw_secondary_ammo_info(const hud_draw_context_hs_mr hudctx, const unsigned ammo_count)
{
	int x, y;
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR)
		x = SB_SECONDARY_AMMO_X, y = SB_SECONDARY_AMMO_Y;
	else
		x = SECONDARY_AMMO_X, y = SECONDARY_AMMO_Y;
	draw_ammo_info(hudctx.canvas, hudctx.xscale(x), hudctx.yscale(y), ammo_count);
}

static void draw_weapon_box(const hud_draw_context_hs_mr hudctx, const player_info &player_info, const unsigned weapon_num, const weapon_type wt)
{
	auto &canvas = hudctx.canvas;
	gr_set_curfont(canvas, GAME_FONT);

	const auto laser_level_changed = (wt == weapon_type::primary && weapon_num == primary_weapon_index_t::LASER_INDEX && (player_info.laser_level != old_laser_level));

	if ((weapon_num != old_weapon[wt] || laser_level_changed) && weapon_box_states[wt] == WS_SET && (old_weapon[wt] != -1) && PlayerCfg.HudMode == HudType::Standard)
	{
		weapon_box_states[wt] = WS_FADING_OUT;
		weapon_box_fade_values[wt]=i2f(GR_FADE_LEVELS-1);
	}

	const local_multires_gauge_graphic multires_gauge_graphic{};
	if (old_weapon[wt] == -1)
	{
		draw_weapon_info(hudctx, player_info, weapon_num, player_info.laser_level, wt);
		old_weapon[wt] = weapon_num;
		weapon_box_states[wt] = WS_SET;
	}

	if (weapon_box_states[wt] == WS_FADING_OUT) {
		draw_weapon_info(hudctx, player_info, old_weapon[wt], old_laser_level, wt);
		weapon_box_fade_values[wt] -= FrameTime * FADE_SCALE;
		if (weapon_box_fade_values[wt] <= 0) {
			weapon_box_states[wt] = WS_FADING_IN;
			old_weapon[wt] = weapon_num;
			old_laser_level = player_info.laser_level;
			weapon_box_fade_values[wt] = 0;
		}
	}
	else if (weapon_box_states[wt] == WS_FADING_IN) {
		if (weapon_num != old_weapon[wt]) {
			weapon_box_states[wt] = WS_FADING_OUT;
		}
		else {
			draw_weapon_info(hudctx, player_info, weapon_num, player_info.laser_level, wt);
			weapon_box_fade_values[wt] += FrameTime * FADE_SCALE;
			if (weapon_box_fade_values[wt] >= i2f(GR_FADE_LEVELS-1)) {
				weapon_box_states[wt] = WS_SET;
				old_weapon[wt] = -1;
			}
		}
	} else
	{
		draw_weapon_info(hudctx, player_info, weapon_num, player_info.laser_level, wt);
		old_weapon[wt] = weapon_num;
		old_laser_level = player_info.laser_level;
	}

	if (weapon_box_states[wt] != WS_SET)		//fade gauge
	{
		int fade_value = f2i(weapon_box_fade_values[wt]);
		int boxofs = (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;

		gr_settransblend(canvas, fade_value, GR_BLEND_NORMAL);
		auto &g = gauge_boxes[boxofs + wt];
		auto &canvas = hudctx.canvas;
		gr_rect(canvas, hudctx.xscale(g.left), hudctx.yscale(g.top), hudctx.xscale(g.right), hudctx.yscale(g.bot), 0);

		gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
static array<fix, 2> static_time;

static void draw_static(const hud_draw_context_hs_mr hudctx, const unsigned win)
{
	vclip *vc = &Vclip[VCLIP_MONITOR_STATIC];
	int framenum;
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	int boxofs = (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)?SB_PRIMARY_BOX:COCKPIT_PRIMARY_BOX;
#if !DXX_USE_OGL
	int x,y;
#endif

	static_time[win] += FrameTime;
	if (static_time[win] >= vc->play_time) {
		weapon_box_user[win] = WBU_WEAPON;
		return;
	}

	framenum = static_time[win] * vc->num_frames / vc->play_time;

	PIGGY_PAGE_IN(vc->frames[framenum]);

	auto &bmp = GameBitmaps[vc->frames[framenum].index];
	auto &box = gauge_boxes[boxofs+win];
#if !DXX_USE_OGL
	for (x = box.left; x < box.right; x += bmp.bm_w)
		for (y = box.top; y < box.bot; y += bmp.bm_h)
			gr_bitmap(hudctx.canvas, x, y, bmp);
#else
	if (multires_gauge_graphic.is_hires())
	{
		const auto scaled_left = hudctx.xscale(box.left);
		const auto scaled_top = hudctx.yscale(box.top);
		const auto scaled_bottom = hudctx.yscale(box.bot - bmp.bm_h);
		hud_bitblt_scaled_xy(hudctx, scaled_left, scaled_top, bmp);
		hud_bitblt_scaled_xy(hudctx, scaled_left, scaled_bottom, bmp);
		const auto scaled_right = hudctx.xscale(box.right - bmp.bm_w);
		hud_bitblt_scaled_xy(hudctx, scaled_right, scaled_top, bmp);
		hud_bitblt_scaled_xy(hudctx, scaled_right, scaled_bottom, bmp);
	}
#endif
}
#endif

namespace dsx {

static void draw_weapon_box0(const hud_draw_context_hs_mr hudctx, const player_info &player_info)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_box_user[0] == WBU_WEAPON)
#endif
	{
		const auto Primary_weapon = player_info.Primary_weapon;
		draw_weapon_box(hudctx, player_info, Primary_weapon, weapon_type::primary);

		if (weapon_box_states[0] == WS_SET) {
			unsigned nd_ammo;
			unsigned ammo_count;
			if (weapon_index_uses_vulcan_ammo(Primary_weapon))
			{
				nd_ammo = player_info.vulcan_ammo;
				ammo_count = vulcan_ammo_scale(nd_ammo);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (Primary_weapon == primary_weapon_index_t::OMEGA_INDEX)
			{
				auto &Omega_charge = player_info.Omega_charge;
				nd_ammo = Omega_charge;
				ammo_count = Omega_charge * 100/MAX_OMEGA_CHARGE;
			}
#endif
			else
				return;
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_primary_ammo(nd_ammo);
			draw_primary_ammo_info(hudctx, ammo_count);
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (weapon_box_user[0] == WBU_STATIC)
		draw_static(hudctx, 0);
#endif
}

static void draw_weapon_box1(const hud_draw_context_hs_mr hudctx, const player_info &player_info)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_box_user[1] == WBU_WEAPON)
#endif
	{
		auto &Secondary_weapon = player_info.Secondary_weapon;
		draw_weapon_box(hudctx, player_info, Secondary_weapon, weapon_type::secondary);
		if (weapon_box_states[1] == WS_SET)
		{
			const auto ammo = player_info.secondary_ammo[Secondary_weapon];
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_secondary_ammo(ammo);
			draw_secondary_ammo_info(hudctx, ammo);
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (weapon_box_user[1] == WBU_STATIC)
		draw_static(hudctx, 1);
#endif
}

static void draw_weapon_boxes(const hud_draw_context_hs_mr hudctx, const player_info &player_info)
{
	draw_weapon_box0(hudctx, player_info);
	draw_weapon_box1(hudctx, player_info);
}

static void sb_draw_energy_bar(const hud_draw_context_hs_mr hudctx, const unsigned energy)
{
	int ew;

	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, SB_GAUGE_ENERGY);

	auto &canvas = hudctx.canvas;
	if (energy <= 100)
	{
		const auto color = 0;
		const int erase_x0 = i2f(hudctx.xscale(SB_ENERGY_GAUGE_X));
		const int erase_x1 = i2f(hudctx.xscale(SB_ENERGY_GAUGE_X + SB_ENERGY_GAUGE_W));
		const int erase_y_base = hudctx.yscale(SB_ENERGY_GAUGE_Y);
		for (int i = hudctx.yscale((100 - energy) * SB_ENERGY_GAUGE_H / 100); i-- > 0;)
		{
		const int erase_y = i2f(erase_y_base + i);
		gr_uline(canvas, erase_x0, erase_y, erase_x1, erase_y, color);
		}
	}

	//draw numbers
	gr_set_fontcolor(canvas, BM_XRGB(25, 18, 6), -1);
	gr_get_string_size(*canvas.cv_font, get_gauge_width_string(energy), &ew, nullptr, nullptr);
#if defined(DXX_BUILD_DESCENT_I)
	unsigned y = SB_ENERGY_NUM_Y;
#elif defined(DXX_BUILD_DESCENT_II)
	unsigned y = SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4);
#endif
	gr_printf(canvas, *canvas.cv_font, (grd_curscreen->get_screen_width() / 3) - (ew / 2), hudctx.yscale(y), "%d", energy);
}

}

#if defined(DXX_BUILD_DESCENT_II)
static void sb_draw_afterburner(const hud_draw_context_hs_mr hudctx, const player_info &player_info)
{
	auto &ab_str = "AB";

	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y, SB_GAUGE_AFTERBURNER);

	const auto color = 0;
	const int erase_x0 = i2f(hudctx.xscale(SB_AFTERBURNER_GAUGE_X));
	const int erase_x1 = i2f(hudctx.xscale(SB_AFTERBURNER_GAUGE_X + (SB_AFTERBURNER_GAUGE_W)));
	const int erase_y_base = hudctx.yscale(SB_AFTERBURNER_GAUGE_Y);
	for (int i = hudctx.yscale(fixmul((f1_0 - Afterburner_charge), SB_AFTERBURNER_GAUGE_H)); i-- > 0;)
	{
		const int erase_y = i2f(erase_y_base + i);
		gr_uline(hudctx.canvas, erase_x0, erase_y, erase_x1, erase_y, color);
	}

	//draw legend
	unsigned r, g, b;
	if (player_info.powerup_flags & PLAYER_FLAGS_AFTERBURNER)
		r = 90, g = b = 0;
	else
		r = g = b = 24;
	auto &canvas = hudctx.canvas;
	gr_set_fontcolor(canvas, gr_find_closest_color(r, g, b), -1);

	int w, h;
	gr_get_string_size(*canvas.cv_font, ab_str, &w, &h, nullptr);
	gr_string(canvas, *canvas.cv_font, hudctx.xscale(SB_AFTERBURNER_GAUGE_X + (SB_AFTERBURNER_GAUGE_W + 1) / 2) - (w / 2), hudctx.yscale(SB_AFTERBURNER_GAUGE_Y + (SB_AFTERBURNER_GAUGE_H - GAME_FONT->ft_h - (GAME_FONT->ft_h / 4))), ab_str, w, h);
}
#endif

static void sb_draw_shield_num(const hud_draw_context_hs_mr hudctx, const unsigned shield)
{
	//draw numbers
	int sw;

	auto &canvas = hudctx.canvas;
	gr_set_fontcolor(canvas, BM_XRGB(14, 14, 23), -1);

	auto &game_font = *GAME_FONT;
	gr_get_string_size(game_font, get_gauge_width_string(shield), &sw, nullptr, nullptr);
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	gr_printf(canvas, game_font, (grd_curscreen->get_screen_width() / 2.266) - (sw / 2), hudctx.yscale(SB_SHIELD_NUM_Y), "%d", shield);
}

static void sb_draw_shield_bar(const hud_draw_context_hs_mr hudctx, const int shield)
{
	int bm_num = shield>=100?9:(shield / 10);
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	hud_gauge_bitblt(hudctx, SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, GAUGE_SHIELDS+9-bm_num);
}

void draw_keys_state::draw_all_statusbar_keys()
{
	auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
	draw_one_key(SB_GAUGE_KEYS_X, SB_GAUGE_BLUE_KEY_Y, SB_GAUGE_BLUE_KEY, PLAYER_FLAGS_BLUE_KEY);
	draw_one_key(SB_GAUGE_KEYS_X, SB_GAUGE_GOLD_KEY_Y, SB_GAUGE_GOLD_KEY, PLAYER_FLAGS_GOLD_KEY);
	draw_one_key(SB_GAUGE_KEYS_X, SB_GAUGE_RED_KEY_Y, SB_GAUGE_RED_KEY, PLAYER_FLAGS_RED_KEY);
}

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
static void draw_invulnerable_ship(const hud_draw_context_hs_mr hudctx, const object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	const auto cmmode = PlayerCfg.CockpitMode[1];
	const auto t = player_info.invulnerable_time;
	if (t + INVULNERABLE_TIME_MAX - GameTime64 > F1_0*4 || GameTime64 & 0x8000)
	{
		static fix time;
		auto ltime = time + FrameTime;
		const auto old_invulnerable_frame = invulnerable_frame;
		while (ltime > INV_FRAME_TIME)
		{
			ltime -= INV_FRAME_TIME;
			if (++invulnerable_frame == N_INVULNERABLE_FRAMES)
				invulnerable_frame=0;
		}
		time = ltime;
		unsigned x, y;
		auto &multires_gauge_graphic = hudctx.multires_gauge_graphic;
		if (cmmode == CM_STATUS_BAR)
		{
			x = SB_SHIELD_GAUGE_X;
			y = SB_SHIELD_GAUGE_Y;
		}
		else
		{
			x = SHIELD_GAUGE_X;
			y = SHIELD_GAUGE_Y;
		}
		hud_gauge_bitblt(hudctx, x, y, GAUGE_INVULNERABLE + old_invulnerable_frame);

                // Show Invulnerability Timer if enabled
		if (show_cloak_invul_timer())
			show_cockpit_cloak_invul_timer(hudctx.canvas, t + INVULNERABLE_TIME_MAX - GameTime64, hudctx.yscale(y));
	}
	else
	{
		const auto shields_ir = f2ir(plrobj.shields);
		if (cmmode == CM_STATUS_BAR)
			sb_draw_shield_bar(hudctx, shields_ir);
		else
			draw_shield_bar(hudctx, shields_ir);
	}
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
namespace dsx {
void show_reticle(grs_canvas &canvas, const player_info &player_info, int reticle_type, int secondary_display)
{
	int x,y,size;
	int laser_ready,missile_ready;
	int cross_bm_num,primary_bm_num,secondary_bm_num;
	int gauge_index;

#if defined(DXX_BUILD_DESCENT_II)
	if (Newdemo_state==ND_STATE_PLAYBACK && Viewer->type != OBJ_PLAYER)
		 return;
#endif

	x = canvas.cv_bitmap.bm_w/2;
	y = canvas.cv_bitmap.bm_h/2;
	size = (canvas.cv_bitmap.bm_h / (32-(PlayerCfg.ReticleSize*4)));

	laser_ready = allowed_to_fire_laser(player_info);

	missile_ready = allowed_to_fire_missile(player_info);
	auto &Primary_weapon = player_info.Primary_weapon;
	primary_bm_num = (laser_ready && player_has_primary_weapon(player_info, Primary_weapon).has_all());
	auto &Secondary_weapon = player_info.Secondary_weapon;
	secondary_bm_num = (missile_ready && player_has_secondary_weapon(player_info, Secondary_weapon).has_all());

	if (primary_bm_num && Primary_weapon == primary_weapon_index_t::LASER_INDEX && (player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS))
		primary_bm_num++;

	if (Secondary_weapon_to_gun_num[Secondary_weapon]==7)
		secondary_bm_num += 3;		//now value is 0,1 or 3,4
	else if (secondary_bm_num && !(player_info.missile_gun & 1))
			secondary_bm_num++;

	cross_bm_num = ((primary_bm_num > 0) || (secondary_bm_num > 0));

	Assert(primary_bm_num <= 2);
	Assert(secondary_bm_num <= 4);
	Assert(cross_bm_num <= 1);

	const auto color = BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]);
	gr_settransblend(canvas, PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);

	[&]{
		int x0, x1, y0, y1;
	switch (reticle_type)
	{
		case RET_TYPE_CLASSIC:
		{
			const local_multires_gauge_graphic multires_gauge_graphic{};
			const hud_draw_context_hs_mr hudctx(canvas, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
			const auto use_hires_reticle = multires_gauge_graphic.is_hires();
			const unsigned ofs = (use_hires_reticle ? 0 : 2);
			gauge_index = RETICLE_CROSS + cross_bm_num;
			PAGE_IN_GAUGE(gauge_index, multires_gauge_graphic);
			auto &cross = GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			const auto &&hud_scale_ar = HUD_SCALE_AR(hudctx.xscale, hudctx.yscale);
			hud_bitblt_free(canvas, x + hud_scale_ar(cross_offsets[ofs].x), y + hud_scale_ar(cross_offsets[ofs].y), hud_scale_ar(cross.bm_w), hud_scale_ar(cross.bm_h), cross);

			gauge_index = RETICLE_PRIMARY + primary_bm_num;
			PAGE_IN_GAUGE(gauge_index, multires_gauge_graphic);
			auto &primary = GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			hud_bitblt_free(canvas, x + hud_scale_ar(primary_offsets[ofs].x), y + hud_scale_ar(primary_offsets[ofs].y), hud_scale_ar(primary.bm_w), hud_scale_ar(primary.bm_h), primary);

			gauge_index = RETICLE_SECONDARY + secondary_bm_num;
			PAGE_IN_GAUGE(gauge_index, multires_gauge_graphic);
			auto &secondary = GameBitmaps[GET_GAUGE_INDEX(gauge_index)];
			hud_bitblt_free(canvas, x + hud_scale_ar(secondary_offsets[ofs].x), y + hud_scale_ar(secondary_offsets[ofs].y), hud_scale_ar(secondary.bm_w), hud_scale_ar(secondary.bm_h), secondary);
			return;
		}
		case RET_TYPE_CLASSIC_REBOOT:
#if DXX_USE_OGL
			ogl_draw_vertex_reticle(cross_bm_num,primary_bm_num,secondary_bm_num,BM_XRGB(PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2]),PlayerCfg.ReticleRGBA[3],PlayerCfg.ReticleSize);
#endif
			return;
		case RET_TYPE_X:
			{
			gr_uline(canvas, i2f(x-(size/2)), i2f(y-(size/2)), i2f(x-(size/5)), i2f(y-(size/5)), color); // top-left
			gr_uline(canvas, i2f(x+(size/2)), i2f(y-(size/2)), i2f(x+(size/5)), i2f(y-(size/5)), color); // top-right
			gr_uline(canvas, i2f(x-(size/2)), i2f(y+(size/2)), i2f(x-(size/5)), i2f(y+(size/5)), color); // bottom-left
			gr_uline(canvas, i2f(x+(size/2)), i2f(y+(size/2)), i2f(x+(size/5)), i2f(y+(size/5)), color); // bottom-right
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)-(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)+(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x+(size/2)), y0 = i2f(y+(size/2)+(size/5)), x1 = i2f(x+(size/5)), y1 = i2f(y+(size/5)+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_DOT:
			{
				gr_disk(canvas, i2f(x), i2f(y), i2f(size/5), color);
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)-(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)+(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x), y0 = i2f(y+(size/2)+(size/5)), x1 = i2f(x), y1 = i2f(y+(size/5)+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_CIRCLE:
			{
				gr_ucircle(canvas, i2f(x), i2f(y), i2f(size/4), color);
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)-(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)+(size/5)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x), y0 = i2f(y+(size/2)+(size/5)), x1 = i2f(x), y1 = i2f(y+(size/5)+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_CROSS_V1:
			{
			gr_uline(canvas, i2f(x),i2f(y-(size/2)),i2f(x),i2f(y+(size/2)+1), color); // horiz
			gr_uline(canvas, i2f(x-(size/2)),i2f(y),i2f(x+(size/2)+1),i2f(y), color); // vert
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x-(size/2)), y0 = i2f(y+(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_CROSS_V2:
			{
			gr_uline(canvas, i2f(x), i2f(y-(size/2)), i2f(x), i2f(y-(size/6)), color); // vert-top
			gr_uline(canvas, i2f(x), i2f(y+(size/2)), i2f(x), i2f(y+(size/6)), color); // vert-bottom
			gr_uline(canvas, i2f(x-(size/2)), i2f(y), i2f(x-(size/6)), i2f(y), color); // horiz-left
			gr_uline(canvas, i2f(x+(size/2)), i2f(y), i2f(x+(size/6)), i2f(y), color); // horiz-right
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x-(size/2)), y0 = i2f(y+(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_ANGLE:
			{
			gr_uline(canvas, i2f(x),i2f(y),i2f(x),i2f(y+(size/2)), color); // vert
			gr_uline(canvas, i2f(x),i2f(y),i2f(x+(size/2)),i2f(y), color); // horiz
			if (secondary_display && secondary_bm_num == 1)
				x0 = i2f(x-(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 2)
				x0 = i2f(x+(size/2)), y0 = i2f(y-(size/2)), x1 = i2f(x+(size/5)), y1 = i2f(y-(size/5));
			else if (secondary_display && secondary_bm_num == 4)
				x0 = i2f(x-(size/2)), y0 = i2f(y+(size/2)), x1 = i2f(x-(size/5)), y1 = i2f(y+(size/5));
			else
				return;
			}
			break;
		case RET_TYPE_NONE:
		default:
			return;
	}
	gr_uline(canvas, x0, y0, x1, y1, color);
	}();
	gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
}
}

void show_mousefs_indicator(grs_canvas &canvas, int mx, int my, int mz, int x, int y, int size)
{
	int axscale = (MOUSEFS_DELTA_RANGE*2)/size, xaxpos = x+(mx/axscale), yaxpos = y+(my/axscale), zaxpos = y+(mz/axscale);

	gr_settransblend(canvas, PlayerCfg.ReticleRGBA[3], GR_BLEND_NORMAL);
	auto &rgba = PlayerCfg.ReticleRGBA;
	const auto color = BM_XRGB(rgba[0], rgba[1], rgba[2]);
	gr_uline(canvas, i2f(xaxpos), i2f(y-(size/2)), i2f(xaxpos), i2f(y-(size/4)), color);
	gr_uline(canvas, i2f(xaxpos), i2f(y+(size/2)), i2f(xaxpos), i2f(y+(size/4)), color);
	gr_uline(canvas, i2f(x-(size/2)), i2f(yaxpos), i2f(x-(size/4)), i2f(yaxpos), color);
	gr_uline(canvas, i2f(x+(size/2)), i2f(yaxpos), i2f(x+(size/4)), i2f(yaxpos), color);
	const local_multires_gauge_graphic multires_gauge_graphic{};
	auto &&hud_scale_ar = HUD_SCALE_AR(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
	auto &&hud_scale_ar2 = hud_scale_ar(2);
	gr_uline(canvas, i2f(x + (size / 2) + hud_scale_ar2), i2f(y), i2f(x + (size / 2) + hud_scale_ar2), i2f(zaxpos), color);
	gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
}

static void hud_show_kill_list(fvcobjptr &vcobjptr, grs_canvas &canvas)
{
	playernum_t n_players;
	playernum_array_t player_list;
	int n_left,i,x0,x1,x2,y,save_y;

	if (Show_kill_list_timer > 0)
	{
		Show_kill_list_timer -= FrameTime;
		if (Show_kill_list_timer < 0)
			Show_kill_list = 0;
	}

	n_players = multi_get_kill_list(player_list);

	if (Show_kill_list == 3)
		n_players = 2;

	if (n_players <= 4)
		n_left = n_players;
	else
		n_left = (n_players+1)/2;

	const auto &&fspacx = FSPACX();
	const auto &&fspacx43 = fspacx(43);

	x1 = fspacx43;

	if (Game_mode & GM_MULTI_COOP)
		x1 = fspacx(31);

	auto &game_font = *GAME_FONT;
	const auto &&line_spacing = LINE_SPACING(game_font, game_font);
	save_y = y = canvas.cv_bitmap.bm_h - n_left * line_spacing;

	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		save_y = y -= fspacx(6);
		if (Game_mode & GM_MULTI_COOP)
			x1 = fspacx(33);
	}

	const auto bm_w = canvas.cv_bitmap.bm_w;
	const auto &&bmw_x0_cockpit = bm_w - fspacx(PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT ? 53 : 60);
	// Right edge of name, change this for width problems
	const auto &&bmw_x1_multi = bm_w - fspacx((Game_mode & GM_MULTI_COOP) ? 27 : 15);
	const auto &&fspacx1 = fspacx(1);
	const auto &&fspacx2 = fspacx(2);
	const auto &&fspacx18 = fspacx(18);
        const auto &&fspacx35 = fspacx(35);
        const auto &&fspacx64 = fspacx(64);
	x0 = fspacx1;
	for (i=0;i<n_players;i++) {
		playernum_t player_num;
		callsign_t name;

		if (i>=n_left) {
			x0 = bmw_x0_cockpit;
			x1 = bmw_x1_multi;
                        if (PlayerCfg.MultiPingHud)
                        {
                                x0 -= fspacx35;
                                x1 -= fspacx35;
                        }
			if (i==n_left)
				y = save_y;

			if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
				x1 -= fspacx18;
		}
		else  if (Netgame.KillGoal || Netgame.PlayTimeAllowed)
		{
			x1 = fspacx43;
			x1 -= fspacx18;
		}

		if (Show_kill_list == 3)
			player_num = i;
		else
			player_num = player_list[i];
		auto &p = *vcplayerptr(player_num);

		color_t fontcolor;
		rgb color;
		if (Show_kill_list == 1 || Show_kill_list==2)
		{
			if (vcplayerptr(player_num)->connected != CONNECT_PLAYING)
				color.r = color.g = color.b = 12;
			else {
				color = player_rgb[get_player_or_team_color(player_num)];
			}
		}
		else
		{
			color = player_rgb_normal[player_num];
		}
		fontcolor = BM_XRGB(color.r, color.g, color.b);
		gr_set_fontcolor(canvas, fontcolor, -1);

		if (Show_kill_list == 3)
			name = Netgame.team_name[i];
		else if (Game_mode & GM_BOUNTY && player_num == Bounty_target && GameTime64&0x10000)
		{
			name = "[TARGET]";
		}
		else
			name = vcplayerptr(player_num)->callsign;	// Note link to above if!!
		int sw, sh;
		gr_get_string_size(game_font, static_cast<const char *>(name), &sw, &sh, nullptr);
		{
			const auto b = x1 - x0 - fspacx2;
			if (sw > b)
				for (char *e = &name.buffer()[strlen(name)];;)
				{
					 *--e = 0;
					 gr_get_string_size(game_font, name, &sw, nullptr, nullptr);
					 if (!(sw > b))
						 break;
				}
		}
		gr_string(canvas, game_font, x0, y, name, sw, sh);

		auto &player_info = vcobjptr(p.objnum)->ctype.player_info;
		if (Show_kill_list==2)
		{
			const int eff = (player_info.net_killed_total + player_info.net_kills_total <= 0)
				? 0
				: static_cast<int>(
					static_cast<float>(player_info.net_kills_total) / (
						static_cast<float>(player_info.net_killed_total) + static_cast<float>(player_info.net_kills_total)
					) * 100.0
				);
			gr_printf(canvas, game_font, x1, y, "%i%%", eff <= 0 ? 0 : eff);
		}
		else if (Show_kill_list == 3)
			gr_printf(canvas, game_font, x1, y, "%3d", team_kills[i]);
		else if (Game_mode & GM_MULTI_COOP)
			gr_printf(canvas, game_font, x1, y, "%-6d", player_info.mission.score);
		else if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
			gr_printf(canvas, game_font, x1, y, "%3d(%d)", player_info.net_kills_total, player_info.KillGoalCount);
		else
			gr_printf(canvas, game_font, x1, y, "%3d", player_info.net_kills_total);

                if (PlayerCfg.MultiPingHud && Show_kill_list != 3)
                {
                        if (Game_mode & GM_MULTI_COOP)
                                x2 = SWIDTH - (fspacx64/2);
                        else
                                x2 = x0 + fspacx64;
			gr_printf(canvas, game_font, x2, y, "%4dms", Netgame.players[player_num].ping);
                }

		y += line_spacing;
	}
}

//returns true if viewer can see object
static int see_object(fvcobjptridx &vcobjptridx, const vcobjptridx_t objnum)
{
	fvi_query fq;
	int hit_type;
	fvi_info hit_data;

	//see if we can see this player

	fq.p0 					= &Viewer->pos;
	fq.p1 					= &objnum->pos;
	fq.rad 					= 0;
	fq.thisobjnum			= vcobjptridx(Viewer);
	fq.flags 				= FQ_TRANSWALL | FQ_CHECK_OBJS | FQ_GET_SEGLIST;
	fq.startseg				= Viewer->segnum;
	fq.ignore_obj_list.first = nullptr;

	hit_type = find_vector_intersection(fq, hit_data);

	return (hit_type == HIT_OBJECT && hit_data.hit_object == objnum);
}

//show names of teammates & players carrying flags
namespace dsx {

void show_HUD_names(grs_canvas &canvas)
{
	for (playernum_t pnum = 0;pnum < N_players; ++pnum)
	{
		if (pnum == Player_num || vcplayerptr(pnum)->connected != CONNECT_PLAYING)
			continue;
		// ridiculusly complex to check if we want to show something... but this is readable at least.

		objnum_t objnum;
		if (Newdemo_state == ND_STATE_PLAYBACK) {
			//if this is a demo, the objnum in the player struct is wrong, so we search the object list for the objnum
			for (objnum=0;objnum<=Highest_object_index;objnum++)
			{
				auto &objp = *vcobjptr(objnum);
				if (objp.type == OBJ_PLAYER && get_player_id(objp) == pnum)
					break;
			}
			if (objnum > Highest_object_index)	//not in list, thus not visible
				continue;			//..so don't show name
		}
		else
			objnum = vcplayerptr(pnum)->objnum;

		const auto &&objp = vcobjptridx(objnum);
		const auto &pl_flags = objp->ctype.player_info.powerup_flags;
		const auto is_friend = (Game_mode & GM_MULTI_COOP || (Game_mode & GM_TEAM && get_team(pnum) == get_team(Player_num)));
		const auto show_friend_name = Show_reticle_name;
		const auto is_cloaked = pl_flags & PLAYER_FLAGS_CLOAKED;
		const auto show_enemy_name = Show_reticle_name && Netgame.ShowEnemyNames && !is_cloaked;
		const auto show_name = is_friend ? show_friend_name : show_enemy_name;
		const auto show_typing = is_friend || !is_cloaked;
		const auto is_bounty_target = (Game_mode & GM_BOUNTY) && pnum == Bounty_target;
		const auto show_indi = (is_friend || !is_cloaked) &&
#if defined(DXX_BUILD_DESCENT_I)
			is_bounty_target;
#elif defined(DXX_BUILD_DESCENT_II)
			(is_bounty_target || ((game_mode_capture_flag() || game_mode_hoard()) && (pl_flags & PLAYER_FLAGS_FLAG)));
#endif

		if ((show_name || show_typing || show_indi) && see_object(vcobjptridx, objp))
		{
			auto player_point = g3_rotate_point(objp->pos);
			if (player_point.p3_codes == 0) //on screen
			{
				g3_project_point(player_point);
				if (!(player_point.p3_flags & PF_OVERFLOW))
				{
					fix x,y,dx,dy;
					char s[CALLSIGN_LEN+10];
					int x1, y1;

					x = player_point.p3_sx;
					y = player_point.p3_sy;
					dy = -fixmuldiv(fixmul(objp->size, Matrix_scale.y), i2f(canvas.cv_bitmap.bm_h) / 2, player_point.p3_z);
					dx = fixmul(dy,grd_curscreen->sc_aspect);
					/* Set the text to show */
					const char *name = NULL;
					if(is_bounty_target)
						name = "Target";
					else if (show_name)
						name = static_cast<const char *>(vcplayerptr(pnum)->callsign);
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
						int w, h;
						gr_get_string_size(*canvas.cv_font, s, &w, &h, nullptr);
						const auto color = get_player_or_team_color(pnum);
						gr_set_fontcolor(canvas, BM_XRGB(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b), -1);
						x1 = f2i(x)-w/2;
						y1 = f2i(y-dy)+FSPACY(1);
						gr_string(canvas, *canvas.cv_font, x1, y1, s, w, h);
					}

					/* Draw box on HUD */
					if (show_indi)
					{
						fix w,h;

						w = dx/4;
						h = dy/4;

							struct {
								int r, g, b;
							} c{};
#if defined(DXX_BUILD_DESCENT_II)
						if (game_mode_capture_flag())
								((get_team(pnum) == TEAM_BLUE) ? c.r : c.b) = 31;
						else if (game_mode_hoard())
						{
								((Game_mode & GM_TEAM)
									? ((get_team(pnum) == TEAM_RED) ? c.r : c.b)
									: c.g
								) = 31;
						}
						else
#endif
							{
								auto &color = player_rgb[get_player_color(pnum)];
								c = {color.r, color.g, color.b};
							}
						const uint8_t color = BM_XRGB(c.r, c.g, c.b);

						gr_line(canvas, x + dx - w, y - dy, x + dx, y - dy, color);
						gr_line(canvas, x + dx, y - dy, x + dx, y - dy + h, color);
						gr_line(canvas, x - dx, y - dy, x - dx + w, y - dy, color);
						gr_line(canvas, x - dx, y - dy, x - dx, y - dy + h, color);
						gr_line(canvas, x + dx - w, y + dy, x + dx, y + dy, color);
						gr_line(canvas, x + dx, y + dy, x + dx, y + dy - h, color);
						gr_line(canvas, x - dx, y + dy, x - dx + w, y + dy, color);
						gr_line(canvas, x - dx, y + dy, x - dx, y + dy - h, color);
					}
				}
			}
		}
	}
}

//draw all the things on the HUD
void draw_hud(grs_canvas &canvas, const object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	if (Newdemo_state == ND_STATE_RECORDING)
	{
		int ammo;
		auto &Primary_weapon = player_info.Primary_weapon;
		if ((Primary_weapon == primary_weapon_index_t::VULCAN_INDEX && (ammo = player_info.vulcan_ammo, true))
#if defined(DXX_BUILD_DESCENT_II)
			||
			(Primary_weapon == primary_weapon_index_t::OMEGA_INDEX && (ammo = player_info.Omega_charge, true))
#endif
		)
			newdemo_record_primary_ammo(ammo);
		newdemo_record_secondary_ammo(player_info.secondary_ammo[player_info.Secondary_weapon]);
	}
	if (PlayerCfg.HudMode == HudType::Hidden) // no hud, "immersion mode"
		return;

	// Cruise speed
	if (Viewer->type == OBJ_PLAYER && get_player_id(vcobjptr(Viewer)) == Player_num && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
	{
		int	x = FSPACX(1);
		int	y = canvas.cv_bitmap.bm_h;

		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
		if (Cruise_speed > 0) {
			auto &game_font = *GAME_FONT;
			const auto &&line_spacing = LINE_SPACING(game_font, game_font);
			if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= line_spacing * 10;
				else
					y -= line_spacing * 6;
			} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= line_spacing * 6;
				else
					y -= line_spacing * 1;
			} else {
				if (Game_mode & GM_MULTI)
					y -= line_spacing * 7;
				else
					y -= line_spacing * 2;
			}

			gr_printf(canvas, game_font, x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}

	//	Show score so long as not in rearview
	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW && PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR) {
		hud_show_score(canvas, player_info);
		if (score_time)
			hud_show_score_added(canvas);
	}

	if ( !Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
		hud_show_timer_count(canvas);

	//	Show other stuff if not in rearview or letterbox.
	if (!Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW)
	{
		show_HUD_names(canvas);

		if (PlayerCfg.CockpitMode[1]==CM_STATUS_BAR || PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN)
			hud_show_homing_warning(canvas, player_info.homing_object_dist);

		const local_multires_gauge_graphic multires_gauge_graphic = {};
		const hud_draw_context_hs_mr hudctx(canvas, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
		if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
			hud_show_energy(canvas, player_info);
			hud_show_shield(canvas, plrobj);
			hud_show_afterburner(canvas, player_info);
			hud_show_weapons(canvas, plrobj);
#if defined(DXX_BUILD_DESCENT_I)
			if (!PCSharePig)
#endif
			hud_show_keys(hudctx, HUD_SCALE_AR(hudctx.xscale, hudctx.yscale), player_info);
			hud_show_cloak_invuln(canvas, player_info);

			if (Newdemo_state==ND_STATE_RECORDING)
				newdemo_record_player_flags(player_info.powerup_flags.get_player_flags());
		}

#ifndef RELEASE
		if (!(Game_mode&GM_MULTI && Show_kill_list))
			show_time(canvas);
#endif

#if defined(DXX_BUILD_DESCENT_II)
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
		{
			hud_show_flag(canvas, player_info, multires_gauge_graphic);
			hud_show_orbs(canvas, player_info, multires_gauge_graphic);
		}
#endif
		HUD_render_message_frame(canvas);

		if (PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR)
			hud_show_lives(hudctx, HUD_SCALE_AR(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic), player_info);
		if (Game_mode&GM_MULTI && Show_kill_list)
			hud_show_kill_list(vcobjptr, canvas);
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
			show_reticle(canvas, player_info, PlayerCfg.ReticleType, 1);
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX && Newdemo_state != ND_STATE_PLAYBACK && PlayerCfg.MouseFlightSim && PlayerCfg.MouseFSIndicator)
		{
			const auto gwidth = canvas.cv_bitmap.bm_w;
			const auto gheight = canvas.cv_bitmap.bm_h;
			auto &raw_mouse_axis = Controls.raw_mouse_axis;
			show_mousefs_indicator(canvas, raw_mouse_axis[0], raw_mouse_axis[1], raw_mouse_axis[2], gwidth / 2, gheight / 2, gheight / 4);
		}
	}

	if (Rear_view && PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW) {
		HUD_render_message_frame(canvas);
		gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0), -1);
		auto &game_font = *GAME_FONT;
		gr_string(canvas, game_font, 0x8000, canvas.cv_bitmap.bm_h - LINE_SPACING(game_font, game_font), TXT_REAR_VIEW);
	}
}
}

//print out some player statistics
namespace dsx {
void render_gauges()
{
	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	const auto energy = f2ir(player_info.energy);
	auto &pl_flags = player_info.powerup_flags;
	const auto cloak = (pl_flags & PLAYER_FLAGS_CLOAKED);

	Assert(PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR);

	auto shields = f2ir(plrobj.shields);
	if (shields < 0 ) shields = 0;

	gr_set_default_canvas();
	gr_set_curfont(*grd_curcanv, GAME_FONT);

	if (Newdemo_state == ND_STATE_RECORDING)
	{
		const auto homing_object_dist = player_info.homing_object_dist;
		if (homing_object_dist >= 0)
			newdemo_record_homing_distance(homing_object_dist);
	}

	const local_multires_gauge_graphic multires_gauge_graphic{};
	const hud_draw_context_hs_mr hudctx(*grd_curcanv, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
	draw_weapon_boxes(hudctx, player_info);
	if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		draw_energy_bar(hudctx, energy);
#if defined(DXX_BUILD_DESCENT_I)
		if (PlayerCfg.HudMode == HudType::Standard)
#elif defined(DXX_BUILD_DESCENT_II)
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_player_afterburner(Afterburner_charge);
		draw_afterburner_bar(hudctx, Afterburner_charge);
#endif
		show_bomb_count(hudctx.canvas, player_info, hudctx.xscale(BOMB_COUNT_X), hudctx.yscale(BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);
		draw_player_ship(hudctx, player_info, cloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);

		if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship(hudctx, plrobj);
		else
			draw_shield_bar(hudctx, shields);
		draw_numerical_display(hudctx, shields, energy);

		if (Newdemo_state==ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(player_info.powerup_flags.get_player_flags());
		}
		draw_keys_state(hudctx, player_info.powerup_flags).draw_all_cockpit_keys();

		show_homing_warning(hudctx, player_info.homing_object_dist);
		draw_wbu_overlay(hudctx);

	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_player_energy(energy);
		sb_draw_energy_bar(hudctx, energy);
#if defined(DXX_BUILD_DESCENT_I)
		if (PlayerCfg.HudMode == HudType::Standard)
#elif defined(DXX_BUILD_DESCENT_II)
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_player_afterburner(Afterburner_charge);
		sb_draw_afterburner(hudctx, player_info);
		if (PlayerCfg.HudMode == HudType::Standard && weapon_box_user[1] == WBU_WEAPON)
#endif
			show_bomb_count(hudctx.canvas, player_info, hudctx.xscale(SB_BOMB_COUNT_X), hudctx.yscale(SB_BOMB_COUNT_Y), gr_find_closest_color(0, 0, 0), 0, 0);

		draw_player_ship(hudctx, player_info, cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);

		if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
			draw_invulnerable_ship(hudctx, plrobj);
		else
			sb_draw_shield_bar(hudctx, shields);
		sb_draw_shield_num(hudctx, shields);

		if (Newdemo_state==ND_STATE_RECORDING)
		{
			newdemo_record_player_shields(shields);
			newdemo_record_player_flags(player_info.powerup_flags.get_player_flags());
		}
		draw_keys_state(hudctx, player_info.powerup_flags).draw_all_statusbar_keys();

		sb_show_lives(hudctx, HUD_SCALE_AR(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), hudctx.multires_gauge_graphic), player_info);
		sb_show_score(hudctx, player_info);

		if ((Game_mode&GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
		}
		else
		{
			sb_show_score_added(hudctx);
		}
	}
#if defined(DXX_BUILD_DESCENT_I)
	else
		draw_player_ship(hudctx, player_info, cloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
#endif
}
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set old_weapon[0] to -1 to force redraw.
namespace dsx {
void update_laser_weapon_info(void)
{
	if (old_weapon[0] == 0)
#if defined(DXX_BUILD_DESCENT_II)
		if (!(get_local_plrobj().ctype.player_info.laser_level > MAX_LASER_LEVEL && old_laser_level <= MAX_LASER_LEVEL))
#endif
			old_weapon[0] = -1;
}
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
		gr_set_default_canvas();
		overlap_dirty[win] = 0;
	}
}

void do_cockpit_window_view(const int win, const vmobjptr_t viewer, const int rear_view_flag, const int user, const char *const label, const player_info *const player_info)
{
	grs_canvas window_canv;
	static grs_canvas overlap_canv;
	object *viewer_save = Viewer;
	int boxnum;
	static int window_x,window_y;
	const gauge_box *box;
	int rear_view_save = Rear_view;

	box = NULL;

	window_rendered_data window;
	update_rendered_data(window, viewer, rear_view_flag);

	weapon_box_user[win] = user;						//say who's using window

	Viewer = viewer;
	Rear_view = rear_view_flag;

	const local_multires_gauge_graphic multires_gauge_graphic{};
	const hud_draw_context_hs_mr hudctx(window_canv, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic);
	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN)
	{
		const unsigned w = HUD_SCALE_AR(hudctx.xscale, hudctx.yscale)(multires_gauge_graphic.get(106, 44));
		const unsigned h = w;

		const int dx = (win == 0) ? -(w + (w / 10)) : (w / 10);

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
		gr_init_sub_canvas(window_canv, grd_curscreen->sc_canvas, hudctx.xscale(box->left), hudctx.yscale(box->top), hudctx.xscale(box->right - box->left + 1), hudctx.yscale(box->bot - box->top + 1));
	}

	gr_set_current_canvas(window_canv);

	render_frame(*grd_curcanv, 0, window);

	//	HACK! If guided missile, wake up robots as necessary.
	if (viewer->type == OBJ_WEAPON) {
		// -- Used to require to be GUIDED -- if (viewer->id == GUIDEDMISS_ID)
		wake_up_rendered_objects(viewer, window);
	}

	if (label) {
		if (Color_0_31_0 == -1)
			Color_0_31_0 = BM_XRGB(0,31,0);
		gr_set_fontcolor(*grd_curcanv, Color_0_31_0, -1);
		auto &game_font = *GAME_FONT;
		gr_string(*grd_curcanv, game_font, 0x8000, FSPACY(1), label);
	}

	if (player_info)	// only non-nullptr for WBU_GUIDED
	{
		show_reticle(*grd_curcanv, *player_info, RET_TYPE_CROSS_V1, 0);
	}

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		int small_window_bottom,big_window_bottom,extra_part_h;

		gr_ubox(*grd_curcanv, 0, 0, grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, BM_XRGB(0,0,32));

		//if the window only partially overlaps the big 3d window, copy
		//the extra part to the visible screen

		big_window_bottom = SHEIGHT - 1;

		if (window_y > big_window_bottom) {

			//the small window is completely outside the big 3d window, so
			//copy it to the visible screen

			gr_set_default_canvas();

			gr_bitmap(*grd_curcanv, window_x, window_y, window_canv.cv_bitmap);

			overlap_dirty[win] = 1;
		}
		else {

			small_window_bottom = window_y + window_canv.cv_bitmap.bm_h - 1;

			extra_part_h = small_window_bottom - big_window_bottom;

			if (extra_part_h > 0) {
				gr_init_sub_canvas(overlap_canv, window_canv, 0, window_canv.cv_bitmap.bm_h-extra_part_h, window_canv.cv_bitmap.bm_w, extra_part_h);
				gr_set_default_canvas();
				gr_bitmap(*grd_curcanv, window_x, big_window_bottom + 1, overlap_canv.cv_bitmap);
				overlap_dirty[win] = 1;
			}
		}
	}
	else if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
	{
		/* `draw_wbu_overlay` has hard-coded x/y coordinates with their
		 * origin at the root of the screen canvas, not the window
		 * canvas.  Reset to screen canvas so that the coordinates are
		 * interpreted properly.
		 */
		gr_set_default_canvas();
		draw_wbu_overlay(hud_draw_context_hs_mr(*grd_curcanv, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height(), multires_gauge_graphic));
	}

	//force redraw when done
	old_weapon[win] = -1;

abort:;

	Viewer = viewer_save;

	Rear_view = rear_view_save;
	/* grd_curcanv may point to `window_canv`; if so, grd_curcanv
	 * would become a dangling pointer when `window_canv` goes out of
	 * scope at the end of the block.  Redirect it to the default screen
	 * to avoid pointing to freed memory.  Setting grd_curcanv to
	 * nullptr would be better, but some code assumes that grd_curcanv
	 * is never nullptr, so instead set it to the default canvas.
	 * Eventually, grd_curcanv will be removed entirely.
	 */
	gr_set_default_canvas();
}
#endif
