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
 * Prototypes and defines for gauges
 *
 */

#pragma once

#include "fwd-player.h"

struct bitmap_index;

#ifdef __cplusplus
#include "fwd-object.h"

//from gauges.c

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_GAUGE_BMS_PC 80u		//	increased from 56 to 80 by a very unhappy MK on 10/24/94.
#define MAX_GAUGE_BMS_MAC 85u
#define MAX_GAUGE_BMS (MacPig ? MAX_GAUGE_BMS_MAC : MAX_GAUGE_BMS_PC)

extern array<bitmap_index, MAX_GAUGE_BMS_MAC> Gauges;   // Array of all gauge bitmaps.
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_GAUGE_BMS 100u   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern array<bitmap_index, MAX_GAUGE_BMS> Gauges;      // Array of all gauge bitmaps.
extern array<bitmap_index, MAX_GAUGE_BMS> Gauges_hires;    // hires gauges
#endif

// Flags for gauges/hud stuff

#ifdef dsx
namespace dsx {
void add_points_to_score(player_info &, int points);
void add_bonus_points_to_score(player_info &, int points);
void render_gauges(void);
void init_gauges(void);
void draw_hud(grs_canvas &, const object &);     // draw all the HUD stuff
}
#endif
void close_gauges(void);
#ifdef dsx
namespace dsx {
void show_reticle(grs_canvas &canvas, const player_info &, int reticle_type, int secondary_display);
void show_HUD_names(grs_canvas &);
}
#endif
void show_mousefs_indicator(grs_canvas &canvas, int mx, int my, int mz, int x, int y, int size);

void player_dead_message(grs_canvas &);
//extern void say_afterburner_status(void);

// from testgaug.c

#ifdef dsx
namespace dsx {
extern void update_laser_weapon_info(void);
void play_homing_warning(const player_info &);
}
#endif

struct rgb {
	ubyte r,g,b;
};

typedef const array<rgb, MAX_PLAYERS> rgb_array_t;
extern const rgb_array_t player_rgb_normal;

/* Stub for mods that provide switchable player colors */
class rgb_array_wrapper
{
public:
	const rgb &operator[](std::size_t i) const
	{
		return player_rgb_normal[i];
	}
};

constexpr rgb_array_wrapper player_rgb{};

#if defined(DXX_BUILD_DESCENT_II)
#define WBU_WEAPON      0       // the weapons display
#define WBU_MISSILE     1       // the missile view
#define WBU_ESCORT      2       // the "buddy bot"
#define WBU_REAR        3       // the rear view
#define WBU_COOP        4       // coop or team member view
#define WBU_GUIDED      5       // the guided missile
#define WBU_MARKER      6       // a dropped marker
#define WBU_STATIC      7       // playing static after missile hits

// draws a 3d view into one of the cockpit windows.  win is 0 for
// left, 1 for right.  viewer is object.  NULL object means give up
// window user is one of the WBU_ constants.  If rear_view_flag is
// set, show a rear view.  If label is non-NULL, print the label at
// the top of the window.
void do_cockpit_window_view(int win, vobjptr_t viewer, int rear_view_flag, int user, const char *label, const player_info * = nullptr);
void do_cockpit_window_view(int win, int user);
#endif

#define GAUGE_HUD_NUMMODES 4

extern int	Color_0_31_0;

// defines for the reticle(s)
#define RET_TYPE_CLASSIC        0
#define RET_TYPE_CLASSIC_REBOOT 1
#define RET_TYPE_NONE           2
#define RET_TYPE_X              3
#define RET_TYPE_DOT            4
#define RET_TYPE_CIRCLE         5
#define RET_TYPE_CROSS_V1       6
#define RET_TYPE_CROSS_V2       7
#define RET_TYPE_ANGLE          8

#define RET_COLOR_DEFAULT_R     0
#define RET_COLOR_DEFAULT_G     32
#define RET_COLOR_DEFAULT_B     0
#define RET_COLOR_DEFAULT_A     0

#endif
