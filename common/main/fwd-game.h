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

#pragma once

#include "dsx-ns.h"
#include "fwd-d_array.h"
#include "fwd-gr.h"
#include "fwd-object.h"
#include "fwd-player.h"
#include "kconfig.h"
#include "maths.h"
#include <chrono>
#include <cstdint>
#include <type_traits>

#ifdef DXX_BUILD_DESCENT
#include <physfs.h>
#endif

namespace dcx {

constexpr std::integral_constant<int, 5> NDL{};       // Number of difficulty levels.

constexpr std::integral_constant<unsigned, 30> DESIGNATED_GAME_FPS{};	// assuming the original intended Framerate was 30
constexpr std::integral_constant<int, F1_0 / DESIGNATED_GAME_FPS> DESIGNATED_GAME_FRAMETIME;

#ifdef NDEBUG
constexpr auto MINIMUM_FPS = DESIGNATED_GAME_FPS;
constexpr std::integral_constant<unsigned, 200> MAXIMUM_FPS{};
#else
constexpr std::integral_constant<unsigned, 1> MINIMUM_FPS{};
constexpr std::integral_constant<unsigned, 1000> MAXIMUM_FPS{};
#endif

// from mglobal.c
using d_time_fix = std::chrono::duration<uint32_t, std::ratio<1, F1_0>>;
extern fix FrameTime;           // time in seconds since last frame
extern fix64 GameTime64;            // time in game (sum of FrameTime)
extern int d_tick_count; // increments according to DESIGNATED_GAME_FRAMETIME
extern int d_tick_step;  // true once in interval of DESIGNATED_GAME_FRAMETIME
enum class gauge_inset_window_view : unsigned;
enum class game_mode_flag : uint16_t;
enum class game_mode_flags : uint16_t;
enum class cockpit_mode_t : uint8_t;

// The following bits define the game modes.
#define GM_NETWORK		game_mode_flag::network       // You are in network mode
#define GM_MULTI_ROBOTS	game_mode_flag::multi_robots       // You are in a multiplayer mode with robots.
#define GM_MULTI_COOP	game_mode_flag::multi_coop      // You are in a multiplayer mode and can't hurt other players.
#define GM_TEAM			game_mode_flag::team     // Team mode for network play
#define GM_BOUNTY		game_mode_flag::bounty     // New bounty mode by Matt1360
#define GM_CAPTURE		game_mode_flag::capture     // Capture the flag mode for D2
#define GM_HOARD		game_mode_flag::hoard    // New hoard mode for D2 Christmas
#define GM_NORMAL		game_mode_flags::normal       // You are in normal play mode, no multiplayer stuff
#define GM_MULTI        GM_NETWORK     // You are in some type of multiplayer game	(GM_NETWORK /* | GM_SERIAL | GM_MODEM */)
extern game_mode_flags Game_mode;
extern game_mode_flags Newdemo_game_mode;
extern screen_mode Game_screen_mode;

void calc_d_tick();

extern int Game_suspended;          // if non-zero, nothing moves but player

/* This must be a signed type.  Some sites, such as `bump_this_object`,
 * use Difficulty_level_type in arithmetic expressions, and those
 * expressions must be signed to produce the correct result.
 */
enum class Difficulty_level_type : signed int;

struct d_game_shared_state;
struct d_game_unique_state;

extern int Global_missile_firing_count;

extern int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

#if DXX_USE_STEREOSCOPIC_RENDER
// Stereo viewport formats
enum class StereoFormat : uint8_t;

extern StereoFormat VR_stereo;
extern fix  VR_eye_width;
extern int  VR_eye_offset;
extern int  VR_sync_width;
extern grs_subcanvas VR_hud_left;
extern grs_subcanvas VR_hud_right;
#endif

extern cockpit_mode_t last_drawn_cockpit;

class pause_game_world_time;

void stop_time();
void start_time();
void reset_time();       // called when starting level

#if DXX_USE_SCREENSHOT
/* The implementation is common, but is only called from similar code, so
 * omit the declaration when used in a common-only context.
 */
#ifdef DXX_BUILD_DESCENT
// If automap_flag == 1, then call automap routine to write message.
#if DXX_USE_SCREENSHOT_FORMAT_LEGACY
void write_bmp(PHYSFS_File *, unsigned w, unsigned h);
#endif
void save_screen_shot(int automap_flag);
#endif
#endif

// force cockpit redraw next time. call this if you've trashed the screen
void fly_init(object_base &obj);
void reset_cockpit();       // called if you've trashed the screen

// functions to save, clear, and resture palette flash effects
void reset_palette_add(void);

void game_init_render_sub_buffers(grs_canvas &, int x, int y, int w, int h);
// Sets up the canvases we will be rendering to

extern int netplayerinfo_on;
extern int force_cockpit_redraw;
}

// Suspend flags
#define SUSP_ROBOTS     1           // Robot AI doesn't move
#define	SHOW_EXIT_PATH	1

//sets the rgb values for palette flash
#define PALETTE_FLASH_SET(_r,_g,_b) (static_cast<void>(PaletteRedAdd = {_r}), static_cast<void>(PaletteGreenAdd = {_g}), static_cast<void>(PaletteBlueAdd = {_b}))

// from game.c
void close_game(void);
void calc_frame_time(void);

#ifdef DXX_BUILD_DESCENT
namespace dsx {

enum class next_level_request_secret_flag : bool;

struct game_window;
extern game_window *Game_wind;

void game();
void init_game();
#if DXX_USE_STEREOSCOPIC_RENDER
void init_stereo();
#endif
void init_cockpit();
void PALETTE_FLASH_ADD(int dr, int dg, int db);

struct d_game_shared_state;

#if DXX_BUILD_DESCENT == 2
struct d_game_unique_state;
struct d_level_shared_seismic_state;
struct d_level_unique_seismic_state;

extern d_level_shared_seismic_state LevelSharedSeismicState;
extern d_level_unique_seismic_state LevelUniqueSeismicState;
#endif

extern d_game_shared_state GameSharedState;
extern d_game_unique_state GameUniqueState;

void game_flush_respawn_inputs(control_info &Controls);
void game_flush_inputs(control_info &Controls);    // clear all inputs
void palette_restore(void);
void show_help();
void show_netgame_help();

// put up the help message
void show_newdemo_help();
#if DXX_BUILD_DESCENT == 2
void full_palette_save();	// all of the above plus gr_palette_load(gr_palette)

//Flickering light system
struct flickering_light;
struct d_flickering_light_state;

extern d_flickering_light_state Flickering_light_state;

extern int BigWindowSwitch;
void compute_slide_segs();

// turn flickering off (because light has been turned off)
void disable_flicker(d_flickering_light_state &fls, vmsegidx_t segnum, sidenum_t sidenum);

// turn flickering off (because light has been turned on)
void enable_flicker(d_flickering_light_state &fls, vmsegidx_t segnum, sidenum_t sidenum);

/*
 * reads a flickering_light structure from a PHYSFS_File
 */
void flickering_light_read(flickering_light &fl, NamedPHYSFS_File fp);
void flickering_light_write(const flickering_light &fl, PHYSFS_File *fp);
#endif

//Cheats
struct game_cheats;
extern game_cheats cheats;

game_window *game_setup();
bool allowed_to_fire_laser(const player_info &);
void reset_globals_for_new_game();
void check_rear_view(control_info &Controls);
int create_special_path();
#if DXX_BUILD_DESCENT == 2
extern uint8_t DemoDoingRight, DemoDoingLeft;
extern fix64	Time_flash_last_played;

playernum_t get_marker_owner(game_mode_flags, game_marker_index, unsigned max_numplayers);

extern struct object *Missile_viewer;
extern object_signature_t Missile_viewer_sig;

extern enumerated_array<unsigned, 2, gauge_inset_window_view> Coop_view_player;     // left & right
extern enumerated_array<game_marker_index, 2, gauge_inset_window_view> Marker_viewer_num;    // left & right
#endif

#if DXX_USE_EDITOR
void dump_used_textures_all();
void move_player_2_segment(vmsegptridx_t seg, sidenum_t side);
#endif
}

#if DXX_BUILD_DESCENT == 1
void palette_save();
#endif
#endif

#ifndef NDEBUG      // if debugging, these are variables

extern int Slew_on;                 // in slew or sim mode?

#else               // if not debugging, these are constants

#define Slew_on             0       // no slewing in real game

#endif

extern int Rear_view;           // if true, looking back.

// selects a given cockpit (or lack of one).
void select_cockpit(cockpit_mode_t mode);

// show a message in a nice little box
void show_boxed_message(grs_canvas &, const char *msg);

// turns off rear view & rear view cockpit
void reset_rear_view(void);

void game_leave_menus(void);

int8_t cheats_enabled();
void game_disable_cheats();
void toggle_cockpit(void);
extern fix Show_view_text_timer;
extern d_time_fix ThisLevelTime;

