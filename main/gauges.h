/* $Id: gauges.h,v 1.1.1.1 2006/03/17 19:57:43 zicodxx Exp $ */
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
 * Prototypes and defines for gauges
 *
 */


#ifndef _GAUGES_H
#define _GAUGES_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "object.h"
#include "hudmsg.h"

//from gauges.c

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern bitmap_index Gauges[MAX_GAUGE_BMS];      // Array of all gauge bitmaps.
extern bitmap_index Gauges_hires[MAX_GAUGE_BMS];    // hires gauges

// Flags for gauges/hud stuff

extern void show_score();
extern void show_score_added();
extern void add_points_to_score();
extern void add_bonus_points_to_score();

void render_gauges(void);
void init_gauges(void);
void close_gauges(void);
void cockpit_decode_alpha(grs_bitmap *bm);
void show_reticle(int reticle_type, int secondary_display);
void show_mousefs_indicator(int mx, int my, int mz, int x, int y, int size);
extern void check_erase_message(void);

extern void draw_hud();     // draw all the HUD stuff

extern void player_dead_message(void);
//extern void say_afterburner_status(void);

// fills in the coords of the hostage video window
void get_hostage_window_coords(int *x, int *y, int *w, int *h);

// from testgaug.c

void gauge_frame(void);
extern void update_laser_weapon_info(void);
extern void play_homing_warning(void);
extern void show_HUD_names();

typedef struct {
	ubyte r,g,b;
} rgb;

extern rgb player_rgb[];

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
void do_cockpit_window_view(int win, object *viewer, int rear_view_flag, int user, const char *label);

#define GAUGE_HUD_NUMMODES 3

typedef struct span {
	int l,r;
} span;

extern span weapon_window_left[],weapon_window_left_hires[],weapon_window_right[],weapon_window_right_hires[];


#define WinBoxLeft (HIRESMODE?weapon_window_left_hires:weapon_window_left)
#define WinBoxRight (HIRESMODE?weapon_window_right_hires:weapon_window_right)

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

#endif /* _GAUGES_H */
