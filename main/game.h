/* $Id: game.h,v 1.8 2004-05-22 06:56:38 btb Exp $ */
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
 * Constants & prototypes which pertain to the game only
 *
 * Old Log:
 * Revision 1.3  1995/09/14  13:44:34  allender
 * fixed function prototypes to have void return value to satify
 * moto compiler
 *
 * Revision 1.2  1995/08/17  09:25:05  allender
 * externed Scanline_double variable
 *
 * Revision 1.1  1995/05/16  15:56:46  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/06  15:23:22  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:28:21  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.79  1995/02/13  10:37:17  john
 * Saved Buggin' cheat mode to save file.
 *
 * Revision 1.78  1995/02/01  16:34:12  john
 * Linted.
 *
 * Revision 1.77  1995/01/29  21:37:14  mike
 * initialize variables on game load so you don't drain your energy when you fire.
 *
 * Revision 1.76  1995/01/26  22:11:36  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.75  1995/01/26  16:45:31  mike
 * Add autofire fusion cannon stuff.
 *
 * Revision 1.74  1994/12/11  23:18:06  john
 * Added -nomusic.
 * Added RealFrameTime.
 * Put in a pause when sound initialization error.
 * Made controlcen countdown and framerate use RealFrameTime.
 *
 * Revision 1.73  1994/12/09  00:41:24  mike
 * fix hang in automap print screen
 *
 * Revision 1.72  1994/12/04  13:47:00  mike
 * enhance custom detail level support.
 *
 * Revision 1.71  1994/12/02  15:05:44  matt
 * Added new "official" cheats
 *
 * Revision 1.70  1994/11/28  18:14:09  rob
 * Added game_mode flag for team games.
 *
 * Revision 1.69  1994/11/15  16:51:13  matt
 * Made rear view only switch to rear cockpit if cockpit on in front view
 *
 * Revision 1.68  1994/11/04  16:26:10  john
 * Fixed bug with letterbox mode game after you finish a game.
 *
 * Revision 1.67  1994/11/02  11:59:48  john
 * Moved menu out of game into inferno main loop.
 *
 * Revision 1.66  1994/10/26  23:02:19  matt
 * Made palette flash saturate negative values
 *
 * Revision 1.65  1994/10/26  15:21:05  mike
 * Detail level stuff.  Make Render_depth public.
 *
 * Revision 1.64  1994/10/19  00:13:01  matt
 * Added prototypes
 *
 * Revision 1.63  1994/10/09  14:54:39  matt
 * Made player cockpit state & window size save/restore with saved games & automap
 *
 * Revision 1.62  1994/10/08  19:56:32  matt
 * Added prototype
 *
 * Revision 1.61  1994/10/07  22:19:32  mike
 * Increase number of difficulty levels from 4 to 5.
 *
 * Revision 1.60  1994/10/06  14:14:11  matt
 * Added new function to reset time (to prevent big FrameTime) at start of level
 *
 * Revision 1.59  1994/10/05  17:08:43  matt
 * Changed order of cockpit bitmaps, since there's no longer a full-screen cockpit
 *
 * Revision 1.58  1994/10/03  23:44:13  matt
 * Save & restore palette effect around menus & pause message
 *
 * Revision 1.57  1994/09/29  17:42:12  matt
 * Cleaned up game_mode a little
 *
 * Revision 1.56  1994/09/28  23:12:01  matt
 * Macroized palette flash system
 *
 * Revision 1.55  1994/09/24  16:56:13  rob
 * Added new fields for the Game_mode bitvector for modem play.
 *
 * Revision 1.54  1994/09/24  14:16:20  mike
 * Added new game mode constants.
 *
 * Revision 1.53  1994/09/22  19:00:57  mike
 * Move NDL from robot.h to here.
 *
 * Revision 1.52  1994/09/22  10:46:51  mike
 * Add difficulty levels.
 *
 * Revision 1.51  1994/09/17  01:39:52  matt
 * Added status bar/sizable window mode, and in the process revamped the
 * whole cockpit mode system.
 *
 * Revision 1.50  1994/09/15  21:23:10  matt
 * Changed system to keep track of whether & what cockpit is up
 *
 * Revision 1.49  1994/09/15  16:11:33  john
 * Added support for VFX1 head tracking. Fixed bug with memory over-
 * write when using stereo mode.
 *
 * Revision 1.48  1994/09/13  16:40:10  mike
 * Prototype Global_missile_firing_count.
 *
 * Revision 1.47  1994/09/13  11:19:05  mike
 * Add Next_missile_fire_time.
 *
 * Revision 1.46  1994/09/12  09:52:50  john
 * Made global flush function that flushes keyboard,mouse, and joystick.
 *
 * Revision 1.45  1994/09/03  15:24:14  mike
 * Make global Global_laser_firing_count.
 *
 * Revision 1.44  1994/08/31  19:26:57  mike
 * Prototypes for Next_laser_fire_time, Laser_delay_time.
 *
 * Revision 1.43  1994/08/18  10:47:22  john
 * Cleaned up game sequencing and player death stuff
 * in preparation for making the player explode into
 * pieces when dead.
 *
 * Revision 1.42  1994/08/11  18:03:53  matt
 * Added prototype
 *
 * Revision 1.41  1994/06/29  20:41:38  matt
 * Added new pause mode; cleaned up countdown & game startup code
 *
 * Revision 1.40  1994/06/24  17:03:49  john
 * Added VFX support. Also took all game sequencing stuff like
 * EndGame out and put it into gameseq.c
 *
 * Revision 1.39  1994/06/20  15:01:08  yuan
 * Added death when mine blows up...
 * Continues onto next level.
 *
 * Revision 1.38  1994/06/17  18:07:20  matt
 * Moved some vars out of ifdef
 *
 * Revision 1.37  1994/06/15  11:09:22  yuan
 * Moved gauge_message to mono screen for now.
 *
 * Revision 1.36  1994/05/30  20:22:11  yuan
 * New triggers.
 *
 * Revision 1.35  1994/05/27  10:32:48  yuan
 * New dialog boxes (Walls and Triggers) added.
 *
 *
 * Revision 1.34  1994/05/20  11:56:45  matt
 * Cleaned up find_vector_intersection() interface
 * Killed check_point_in_seg(), check_player_seg(), check_object_seg()
 *
 * Revision 1.33  1994/05/19  21:45:21  matt
 * Removed unused prototypes
 *
 * Revision 1.32  1994/05/19  18:53:17  yuan
 * Changing player structure...
 *
 * Revision 1.31  1994/05/16  16:38:35  yuan
 * Fixed palette add so it doesn't show up in the menu.
 *
 * Revision 1.30  1994/05/16  09:28:17  matt
 * Renamed init_player() to be init_player_stats(), added new funtion
 * init_player_object()
 *
 * Revision 1.29  1994/05/14  17:14:57  matt
 * Got rid of externs in source (non-header) files
 *
 */

#ifndef _GAME_H
#define _GAME_H

#include <setjmp.h>

#include "vecmat.h"
#include "object.h"

//#include "segment.h"

#ifdef MACINTOSH
extern ubyte Scanline_double;
#endif

#ifdef WINDOWS
typedef struct cockpit_span_line {
	short num;
	struct {
		short xmin, xmax;
	} span[5];
} cockpit_span_line;

extern cockpit_span_line win_cockpit_mask[480];
#endif


// from mglobal.c
extern fix FrameTime;               // time in seconds since last frame
extern fix RealFrameTime;           // time in seconds since last frame
extern fix GameTime;                // time in game (sum of FrameTime)
extern int FrameCount;              // how many frames rendered
extern fix Next_laser_fire_time;    // Time at which player can next fire his selected laser.
extern fix Last_laser_fired_time;
extern fix Next_missile_fire_time;  // Time at which player can next fire his selected missile.
extern fix Laser_delay_time;        // Delay between laser fires.
extern int Cheats_enabled;

extern int Missile_view_enabled;

extern object *Missile_viewer;

#define CV_NONE     0
#define CV_ESCORT   1
#define CV_REAR     2
#define CV_COOP     3
#define CV_MARKER   4

extern int Cockpit_3d_view[2];      // left & right
extern int Coop_view_player[2];     // left & right
extern int Marker_viewer_num[2];    // left & right

// constants for ft_preference
#define FP_RIGHT        0
#define FP_UP           1
#define FP_FORWARD      2       // this is the default
#define FP_LEFT         3
#define FP_DOWN         4
#define FP_FIRST_TIME   5

extern int ft_preference;

// The following bits define the game modes.
#define GM_EDITOR       1       // You came into the game from the editor
#define GM_SERIAL       2       // You are in serial mode
#define GM_NETWORK      4       // You are in network mode
#define GM_MULTI_ROBOTS 8       // You are in a multiplayer mode with robots.
#define GM_MULTI_COOP   16      // You are in a multiplayer mode and can't hurt other players.
#define GM_MODEM        32      // You are in a modem (serial) game

#define GM_UNKNOWN      64      // You are not in any mode, kind of dangerous...
#define GM_GAME_OVER    128     // Game has been finished

#define GM_TEAM         256     // Team mode for network play
#define GM_CAPTURE      512     // Capture the flag mode for D2
#define GM_HOARD        1024    // New hoard mode for D2 Christmas

#define GM_NORMAL       0       // You are in normal play mode, no multiplayer stuff
#define GM_MULTI        38      // You are in some type of multiplayer game

// Examples:
// Deathmatch mode on a network is GM_NETWORK
// Deathmatch mode via modem with robots is GM_MODEM | GM_MULTI_ROBOTS
// Cooperative mode via serial link is GM_SERIAL | GM_MULTI_COOP

#define NDL 5       // Number of difficulty levels.
#define NUM_DETAIL_LEVELS   6

extern int Game_mode;
//added 3/24/99 by Owen Evans
extern u_int32_t Game_screen_mode;
//end added - OE

extern int Game_paused;
extern int gauge_message_on;

#ifndef NDEBUG      // if debugging, these are variables

extern int Slew_on;                 // in slew or sim mode?
extern int Game_double_buffer;      // double buffering?


#else               // if not debugging, these are constants

#define Slew_on             0       // no slewing in real game
#define Game_double_buffer  1       // always double buffer in real game

#endif

#ifndef MACINTOSH

#define Scanline_double     0       // PC doesn't do scanline doubling

#else

extern ubyte Scanline_double;       // but the Macintosh does

#endif

// Suspend flags

#define SUSP_NONE       0           // Everything moving normally
#define SUSP_ROBOTS     1           // Robot AI doesn't move
#define SUSP_WEAPONS    2           // Lasers, etc. don't move

extern int Game_suspended;          // if non-zero, nothing moves but player

// from game.c
void init_game(void);
void game(void);
void close_game(void);
void init_cockpit(void);
void calc_frame_time(void);

int do_flythrough(object *obj,int first_time);

extern jmp_buf LeaveGame;       // Do a long jump to this when game is over.
extern int Difficulty_level;    // Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int Detail_level;        // Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest
extern int Global_laser_firing_count;
extern int Global_missile_firing_count;
extern int Render_depth;
extern fix Auto_fire_fusion_cannon_time, Fusion_charge;

extern int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

#define MAX_PALETTE_ADD 30

extern void PALETTE_FLASH_ADD(int dr, int dg, int db);

//sets the rgb values for palette flash
#define PALETTE_FLASH_SET(_r,_g,_b) PaletteRedAdd=(_r), PaletteGreenAdd=(_g), PaletteBlueAdd=(_b)

extern int draw_gauges_on;

extern void init_game_screen(void);

extern void game_flush_inputs();    // clear all inputs

extern int Playing_game;    // True if playing game
extern int Auto_flythrough; // if set, start flythough automatically
extern int Mark_count;      // number of debugging marks set
extern char faded_in;

extern void stop_time(void);
extern void start_time(void);
extern void reset_time(void);       // called when starting level

// If automap_flag == 1, then call automap routine to write message.
extern void save_screen_shot(int automap_flag);

#ifndef WINDOWS
extern grs_canvas * get_current_game_screen();
#endif

//valid modes for cockpit
#define CM_FULL_COCKPIT     0   // normal screen with cockput
#define CM_REAR_VIEW        1   // looking back with bitmap
#define CM_STATUS_BAR       2   // small status bar, w/ reticle
#define CM_FULL_SCREEN      3   // full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX        4   // half-height window (for cutscenes)

extern int Cockpit_mode;        // what sort of cockpit or window is up?
extern int Game_window_w,       // width and height of player's game window
           Game_window_h;

extern int Rear_view;           // if true, looking back.
extern fix Rear_view_leave_time;// how long until we decide key is down

// initalize flying
void fly_init(object *obj);

// selects a given cockpit (or lack of one).
void select_cockpit(int mode);

// force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit(void);       // called if you've trashed the screen

// functions to save, clear, and resture palette flash effects
void palette_save(void);
void reset_palette_add(void);
void palette_restore(void);

// put up the help message
void do_show_help();

// show a message in a nice little box
void show_boxed_message(char *msg);

// erases message drawn with show_boxed_message()
void clear_boxed_message();

// turns off rear view & rear view cockpit
void reset_rear_view(void);

extern int Game_turbo_mode;

// returns ptr to escort robot, or NULL
object *find_escort();

extern void apply_modified_palette(void);

//Flickering light system
typedef struct  {
	short segnum, sidenum;
	unsigned long mask;     // determines flicker pattern
	fix timer;              // time until next change
	fix delay;              // time between changes
} flickering_light;

#define MAX_FLICKERING_LIGHTS 100

extern flickering_light Flickering_lights[MAX_FLICKERING_LIGHTS];
extern int Num_flickering_lights;

// returns ptr to flickering light structure, or NULL if can't find
flickering_light *find_flicker(int segnum, int sidenum);

// turn flickering off (because light has been turned off)
void disable_flicker(int segnum, int sidenum);

// turn flickering off (because light has been turned on)
void enable_flicker(int segnum, int sidenum);

// returns 1 if ok, 0 if error
int add_flicker(int segnum, int sidenum, fix delay, unsigned long mask);

int gr_toggle_fullscreen_game(void);

/*
 * reads a flickering_light structure from a CFILE
 */
void flickering_light_read(flickering_light *fl, CFILE *fp);

extern int maxfps;

#endif /* _GAME_H */
