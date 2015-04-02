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
 * Constants & prototypes which pertain to the game only
 *
 */

#ifndef _GAME_H
#define _GAME_H

#include <physfs.h>
#include "pstypes.h"
#include "window.h"

#ifdef __cplusplus
#include <cstdint>
#include "pack.h"
#include "segnum.h"
#include "fwdvalptridx.h"

#define DESIGNATED_GAME_FPS 30 // assuming the original intended Framerate was 30
#define DESIGNATED_GAME_FRAMETIME (F1_0/DESIGNATED_GAME_FPS) 

#ifdef NDEBUG
#define MINIMUM_FPS DESIGNATED_GAME_FPS
#define MAXIMUM_FPS 200
#else
#define MINIMUM_FPS 1
#define MAXIMUM_FPS 1000
#endif

extern struct window *Game_wind;

// from mglobal.c
extern fix FrameTime;           // time in seconds since last frame
extern fix64 GameTime64;            // time in game (sum of FrameTime)
extern int d_tick_count; // increments according to DESIGNATED_GAME_FRAMETIME
extern int d_tick_step;  // true once in interval of DESIGNATED_GAME_FRAMETIME
extern fix64 Next_laser_fire_time;    // Time at which player can next fire his selected laser.
extern fix64 Last_laser_fired_time;
extern fix64 Next_missile_fire_time;  // Time at which player can next fire his selected missile.
extern fix64 Next_flare_fire_time;
extern fix Laser_delay_time;        // Delay between laser fires.

#if defined(DXX_BUILD_DESCENT_II)
class object_signature_t;
extern struct object *Missile_viewer;
extern object_signature_t Missile_viewer_sig;

#define CV_NONE     0
#define CV_ESCORT   1
#define CV_REAR     2
#define CV_COOP     3
#define CV_MARKER   4

extern array<int, 2> Coop_view_player;     // left & right
extern array<int, 2> Marker_viewer_num;    // left & right
#endif

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
// #define GM_SERIAL       2       // You are in serial mode // OBSOLETE
#define GM_NETWORK      4       // You are in network mode
#define GM_MULTI_ROBOTS 8       // You are in a multiplayer mode with robots.
#define GM_MULTI_COOP   16      // You are in a multiplayer mode and can't hurt other players.
// #define GM_MODEM        32      // You are in a modem (serial) game // OBSOLETE
#define GM_UNKNOWN      64      // You are not in any mode, kind of dangerous...
#define GM_GAME_OVER    128     // Game has been finished
#define GM_TEAM         256     // Team mode for network play
#if defined(DXX_BUILD_DESCENT_I)
#define GM_BOUNTY       512     // New bounty mode by Matt1360
#elif defined(DXX_BUILD_DESCENT_II)
#define GM_CAPTURE      512     // Capture the flag mode for D2
#define GM_HOARD        1024    // New hoard mode for D2 Christmas
#define GM_BOUNTY	2048	/* New bounty mode by Matt1360 */
#endif
#define GM_NORMAL       0       // You are in normal play mode, no multiplayer stuff
#define GM_MULTI        38      // You are in some type of multiplayer game


#define NDL 5       // Number of difficulty levels.

extern int Game_mode;
extern u_int32_t Game_screen_mode;

extern int gauge_message_on;

#ifndef NDEBUG      // if debugging, these are variables

extern int Slew_on;                 // in slew or sim mode?

#else               // if not debugging, these are constants

#define Slew_on             0       // no slewing in real game
#define Game_double_buffer  1       // always double buffer in real game

#endif

// Suspend flags

#define SUSP_NONE       0           // Everything moving normally
#define SUSP_ROBOTS     1           // Robot AI doesn't move
#define SUSP_WEAPONS    2           // Lasers, etc. don't move

extern int Game_suspended;          // if non-zero, nothing moves but player

#define	SHOW_EXIT_PATH	1


// from game.c
void init_game(void);
void game(void);
void close_game(void);
void init_cockpit(void);
void calc_frame_time(void);
void calc_d_tick();

extern int Difficulty_level;    // Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int Global_laser_firing_count;
extern int Global_missile_firing_count;
extern fix64 Auto_fire_fusion_cannon_time;
extern fix Fusion_charge;

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
extern int last_drawn_cockpit;

extern void stop_time(void);
extern void start_time(void);
extern void reset_time(void);       // called when starting level

// If automap_flag == 1, then call automap routine to write message.
extern void save_screen_shot(int automap_flag);

enum cockpit_mode_t
{
//valid modes for cockpit
	CM_FULL_COCKPIT,   // normal screen with cockpit
	CM_REAR_VIEW,   // looking back with bitmap
	CM_STATUS_BAR,   // small status bar, w/ reticle
	CM_FULL_SCREEN,   // full screen, no cockpit (w/ reticle)
	CM_LETTERBOX   // half-height window (for cutscenes)
};

extern int Game_window_w,       // width and height of player's game window
           Game_window_h;

extern int Rear_view;           // if true, looking back.

// initalize flying
void fly_init(vobjptr_t obj);

// selects a given cockpit (or lack of one).
void select_cockpit(cockpit_mode_t mode);

// force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit(void);       // called if you've trashed the screen

// functions to save, clear, and resture palette flash effects
void palette_save(void);
void reset_palette_add(void);
void palette_restore(void);
#if defined(DXX_BUILD_DESCENT_I)
static inline void full_palette_save(void)
{
	palette_save();
}
#elif defined(DXX_BUILD_DESCENT_II)
void full_palette_save(void);	// all of the above plus gr_palette_load(gr_palette)
#endif

// put up the help message
void show_help();
void show_netgame_help();
void show_newdemo_help();

// show a message in a nice little box
void show_boxed_message(const char *msg, int RenderFlag);

// turns off rear view & rear view cockpit
void reset_rear_view(void);

void game_init_render_sub_buffers(int x, int y, int w, int h);
// Sets up the canvases we will be rendering to
static inline void game_init_render_buffers (int render_max_w, int render_max_h)
{
	game_init_render_sub_buffers( 0, 0, render_max_w, render_max_h );
}

extern int netplayerinfo_on;
extern int	Slide_segs_computed;

#if defined(DXX_BUILD_DESCENT_I)
static inline int game_mode_capture_flag()
{
	return 0;
}
static inline int game_mode_hoard()
{
	return 0;
}
#elif defined(DXX_BUILD_DESCENT_II)
static inline int game_mode_capture_flag()
{
	return (Game_mode & GM_CAPTURE);
}
static inline int game_mode_hoard()
{
	return (Game_mode & GM_HOARD);
}
// returns ptr to escort robot, or NULL
objptridx_t find_escort();

//Flickering light system
struct flickering_light {
	segnum_t segnum;
	short sidenum;
	uint32_t mask;     // determines flicker pattern
	fix timer;              // time until next change
	fix delay;              // time between changes
};

#define MAX_FLICKERING_LIGHTS 100

typedef array<flickering_light, MAX_FLICKERING_LIGHTS> Flickering_light_array_t;
extern Flickering_light_array_t Flickering_lights;
extern unsigned Num_flickering_lights;
extern int BigWindowSwitch;

// turn flickering off (because light has been turned off)
void disable_flicker(segnum_t segnum, int sidenum);

// turn flickering off (because light has been turned on)
void enable_flicker(segnum_t segnum, int sidenum);

/*
 * reads a flickering_light structure from a PHYSFS_file
 */
void flickering_light_read(flickering_light *fl, PHYSFS_file *fp);

void flickering_light_write(flickering_light *fl, PHYSFS_file *fp);
#endif

void game_render_frame_mono();
static inline void game_render_frame_mono(int flip)
{
	game_render_frame_mono();
	if (flip)
		gr_flip();
}
void game_leave_menus(void);

//Cheats
struct game_cheats : prohibit_void_ptr<game_cheats>
{
	int enabled;
	int wowie;
	int allkeys;
	int invul;
	int shields;
	int killreactor;
	int exitpath;
	int levelwarp;
	int fullautomap;
	int ghostphysics;
	int rapidfire;
	int turbo;
	int robotfiringsuspended;
	int acid;
#if defined(DXX_BUILD_DESCENT_I)
	int wowie2;
	int cloak;
	int extralife;
	int baldguy;
#elif defined(DXX_BUILD_DESCENT_II)
	int lamer;
	int accessory;
	int bouncyfire;
	int killallrobots;
	int robotskillrobots;
	int monsterdamage;
	int buddyclone;
	int buddyangry;
#endif
};
extern game_cheats cheats;
void game_disable_cheats();
void move_player_2_segment(vsegptridx_t seg, int side);
int allowed_to_fire_laser(void);
int allowed_to_fire_flare(void);
int allowed_to_fire_missile(void);
void	check_rear_view(void);
window *game_setup(void);
int create_special_path(void);
window_event_result ReadControls(const d_event &event);
void toggle_cockpit(void);
void game_render_frame();
extern fix Show_view_text_timer;
extern fix ThisLevelTime;
extern int	Last_level_path_created;
extern int force_cockpit_redraw;
extern ubyte DemoDoingRight,DemoDoingLeft;
extern fix64	Time_flash_last_played;
window_event_result game_handler(window *wind,const d_event &event, const unused_window_userdata_t *);

#ifdef EDITOR
void dump_used_textures_all();
#endif

#endif

#endif /* _GAME_H */
