/* $Id: gauges.h,v 1.3 2004-08-28 23:17:45 schaffner Exp $ */
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

//from gauges.c

#define MAX_GAUGE_BMS 100   // increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern bitmap_index Gauges[MAX_GAUGE_BMS];      // Array of all gauge bitmaps.
extern bitmap_index Gauges_hires[MAX_GAUGE_BMS];    // hires gauges

// Flags for gauges/hud stuff
extern ubyte Reticle_on;

extern void init_gauge_canvases();
extern void close_gauge_canvases();

extern void show_score();
extern void show_score_added();
extern void add_points_to_score();
extern void add_bonus_points_to_score();

void render_gauges(void);
void init_gauges(void);
extern void check_erase_message(void);

extern void HUD_render_message_frame();
extern void HUD_clear_messages();

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
extern int HUD_init_message(char *format, ... );

#define gauge_message HUD_init_message

extern void draw_hud();     // draw all the HUD stuff

extern void player_dead_message(void);
//extern void say_afterburner_status(void);

// fills in the coords of the hostage video window
void get_hostage_window_coords(int *x, int *y, int *w, int *h);

// from testgaug.c

void gauge_frame(void);
extern void update_laser_weapon_info(void);
extern void play_homing_warning(void);

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
void do_cockpit_window_view(int win, object *viewer, int rear_view_flag, int user, char *label);

#ifdef MACINTOSH
# include "QuickDraw.h"
# include "RAVE.h"
void calculate_sub_view_window_bounds(int inSubWindowNum, TQARect* outBoundsRect);
#endif

#endif /* _GAUGES_H */
