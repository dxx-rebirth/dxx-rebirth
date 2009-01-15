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
 * Constants & prototypes which pertain to the game only
 * 
 */

#ifndef _GAME_H
#define _GAME_H

#include "vecmat.h"
#include "object.h"
#include "checker.h"

#ifdef NDEBUG
#define MAXIMUM_FPS 200
#else
#define MAXIMUM_FPS 1000
#endif

//from mglobal.c
extern fix FrameTime;		//time in seconds since last frame
extern fix GameTime;		//time in game (sum of FrameTime)
extern int FrameCount;		//how many frames rendered
extern int FixedStep;		//fixed time bytes stored here
extern fix Next_laser_fire_time;	//      Time at which player can next fire his selected laser.
extern fix Last_laser_fired_time;
extern fix Next_missile_fire_time;	//      Time at which player can next fire his selected missile.
extern fix Laser_delay_time;	//      Delay between laser fires.
extern int Cheats_enabled;

// bits for FixedStep
#define EPS4	1
#define EPS20	2
#define EPS30	4

//constants for ft_preference
#define FP_RIGHT		0
#define FP_UP			1
#define FP_FORWARD	2	//this is the default
#define FP_LEFT		3
#define FP_DOWN		4
#define FP_FIRST_TIME	5

extern int ft_preference;

//      The following bits define the game modes.
#define GM_EDITOR				1	//        You came into the game from the editor
// #define GM_SERIAL				2	// You are in serial mode // OBSOLETE
#define GM_NETWORK				4	// You are in network mode
#define GM_MULTI_ROBOTS				8	//        You are in a multiplayer mode with robots.
#define GM_MULTI_COOP				16	//        You are in a multiplayer mode and can't hurt other players.
// #define GM_MODEM				32	// You are in a modem (serial) game // OBSOLETE
#define GM_UNKNOWN				64	//        You are not in any mode, kind of dangerous...
#define GM_GAME_OVER				128	//        Game has been finished
#define GM_TEAM					256	// Team mode for network play
#define GM_NORMAL				0	//        You are in normal play mode, no multiplayer stuff
#define GM_MULTI				38	//        You are in some type of multiplayer game

#define	NDL	                5		//        Number of difficulty levels.

extern int Game_mode;
//added 3/24/99 by Owen Evans
extern u_int32_t Game_screen_mode;
//end added - OE

extern int Game_paused;
extern int gauge_message_on;

#ifndef NDEBUG			//if debugging, these are variables
extern int Slew_on;		//in slew or sim mode?

#else //if not debugging, these are constants

#define Slew_on 		0	//no slewing in real game
#define Game_double_buffer	1	//always double buffer in real game
#endif	/*
 */

//Suspend flags

#define SUSP_NONE		0	//Everything moving normally
#define SUSP_ROBOTS		1	//Robot AI doesn't move
#define SUSP_WEAPONS		2	//Lasers, etc. don't move         

extern int Game_suspended;	//if non-zero, nothing moves but player


//from game.c
void init_game (void);
void game (void);
void close_game (void);
void calc_frame_time (void);
void FixedStepCalc();
void do_flythrough (object * obj, int first_time);

extern jmp_buf LeaveGame;	// Do a long jump to this when game is over.
extern int Difficulty_level;	//      Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int Global_laser_firing_count;
extern int Global_missile_firing_count;
extern int Render_depth;
extern fix Auto_fire_fusion_cannon_time, Fusion_charge;

extern int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

#define	MAX_PALETTE_ADD	30

// DPH (17/9/98): Change to static inline function under linux as linux
// has problems with linefeed characters.
#ifdef __LINUX__
static inline void PALETTE_FLASH_ADD(int _dr, int _dg, int _db)
{
	do {
		if ((PaletteRedAdd+=(_dr)) > MAX_PALETTE_ADD)
			PaletteRedAdd = MAX_PALETTE_ADD;
		if ((PaletteGreenAdd+=(_dg)) > MAX_PALETTE_ADD)
			PaletteGreenAdd = MAX_PALETTE_ADD;
		if ((PaletteBlueAdd+=(_db)) > MAX_PALETTE_ADD)
			PaletteBlueAdd = MAX_PALETTE_ADD;
		if (PaletteRedAdd < -MAX_PALETTE_ADD)
			PaletteRedAdd = -MAX_PALETTE_ADD;
		if (PaletteGreenAdd < -MAX_PALETTE_ADD)
			PaletteGreenAdd = -MAX_PALETTE_ADD;
		if (PaletteBlueAdd < -MAX_PALETTE_ADD)
			PaletteBlueAdd = -MAX_PALETTE_ADD;
	} while (0);
}
#else
//adds to rgb values for palette flash
#define PALETTE_FLASH_ADD(_dr,_dg,_db)                                  \
	do {                                                            \
		if ((PaletteRedAdd+=(_dr)) > MAX_PALETTE_ADD)           \
			PaletteRedAdd = MAX_PALETTE_ADD;                \
		if ((PaletteGreenAdd+=(_dg)) > MAX_PALETTE_ADD)         \
			PaletteGreenAdd = MAX_PALETTE_ADD;              \
		if ((PaletteBlueAdd+=(_db)) > MAX_PALETTE_ADD)          \
			PaletteBlueAdd = MAX_PALETTE_ADD;               \
		if (PaletteRedAdd < -MAX_PALETTE_ADD)                   \
			PaletteRedAdd = -MAX_PALETTE_ADD;               \
		if (PaletteGreenAdd < -MAX_PALETTE_ADD)                 \
			PaletteGreenAdd = -MAX_PALETTE_ADD;             \
		if (PaletteBlueAdd < -MAX_PALETTE_ADD)                  \
			PaletteBlueAdd = -MAX_PALETTE_ADD;              \
	} while (0)
#endif

//sets the rgb values for palette flash
#define PALETTE_FLASH_SET(_r,_g,_b) PaletteRedAdd=(_r), PaletteGreenAdd=(_g), PaletteBlueAdd=(_b)

extern int draw_gauges_on;

extern void init_game_screen (void);

extern void game_flush_inputs ();	// clear all inputs
extern int Playing_game;	// True if playing game
extern int Auto_flythrough;	//if set, start flythough automatically
extern int Mark_count;		// number of debugging marks set
extern char faded_in;
extern int last_drawn_cockpit;

extern void stop_time (void);
extern void start_time (void);
extern void reset_time (void);		//called when starting level
extern void save_screen_shot (int automap_flag);

//valid modes for cockpit
#define CM_FULL_COCKPIT 	0	//normal screen with cockput
#define CM_REAR_VIEW			1	//looking back with bitmap
#define CM_STATUS_BAR		2	//small status bar, w/ reticle
#define CM_FULL_SCREEN		3	//full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX			4	//half-height window (for cutscenes)

extern int Game_window_w,	//width and height of player's game window
  Game_window_h, max_window_h;

extern int Rear_view;		//if true, looking back.

//selects a given cockpit (or lack of one).
void select_cockpit (int mode);

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit (void);	//called if you've trashed the screen

//functions to save, clear, and resture palette flash effects
void palette_save (void);
void reset_palette_add (void);
void palette_restore (void);

//put up the help message
void show_help();
void show_netgame_help();
void show_newdemo_help();

//show a message in a nice little box
void show_boxed_message (char *msg, int RenderFlag);

//turns off rear view & rear view cockpit
void reset_rear_view (void);

void init_cockpit ();

extern int Game_turbo_mode;

#define VR_NONE			0	//viewing the game screen
#define VR_AREA_DET		1	//viewing with the stereo area determined method
#define VR_INTERLACED	        2	//viewing with the stereo interlaced method

extern ubyte VR_switch_eyes;
extern fix VR_eye_width;
extern int VR_render_mode;
extern int VR_compatible_menus;
extern grs_canvas *VR_offscreen_buffer;		// The offscreen data buffer
extern grs_canvas VR_render_buffer[2];	//  Two offscreen buffers for left/right eyes.
extern grs_canvas VR_render_sub_buffer[2];	//  Two sub buffers for left/right eyes.
extern grs_canvas VR_screen_pages[2];	//  Two pages of VRAM if paging is available
extern grs_canvas VR_screen_sub_pages[2];	//  Two sub pages of VRAM if paging is available
extern grs_canvas *VR_offscreen_menu;		// The offscreen data buffer for menus

void game_init_render_buffers (int render_max_w, int render_max_h, int render_method);
void game_do_render_frame(int flip);
extern int Allow_primary_cycle;
extern int Allow_secondary_cycle;

#endif

