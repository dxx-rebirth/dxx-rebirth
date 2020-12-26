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
 * header for demo playback system
 *
 */

#pragma once

#ifdef __cplusplus
#include "physfsx.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-weapon.h"
#include "fwd-window.h"

#define ND_STATE_NORMAL			0
#define ND_STATE_RECORDING		1
#define ND_STATE_PLAYBACK		2
#define ND_STATE_PAUSED			3
#define ND_STATE_REWINDING		4
#define ND_STATE_FASTFORWARD		5
#define ND_STATE_ONEFRAMEFORWARD	6
#define ND_STATE_ONEFRAMEBACKWARD	7

#define DEMO_DIR                "demos/"
#define DEMO_EXT		"dem"
extern const std::array<file_extension_t, 1> demo_file_extensions;

#if DXX_WORDS_BIGENDIAN
#define DEMO_BACKUP_EXT			"386"
#else
#define DEMO_BACKUP_EXT			"ppc"
#endif

// Gives state of recorder
extern int Newdemo_state;
extern int NewdemoFrameCount;

extern int Newdemo_vcr_state;
extern int Newdemo_game_mode;
extern sbyte Newdemo_do_interpolate;
extern int Newdemo_show_percentage;

//Does demo start automatically?
extern int Auto_demo;
extern int Newdemo_num_written;

#if defined(DXX_BUILD_DESCENT_II)
extern ubyte DemoDoRight,DemoDoLeft;
extern struct object DemoRightExtra,DemoLeftExtra;
#endif

// Functions called during recording process...
#ifdef dsx
namespace dsx {
extern void newdemo_record_start_demo();
extern void newdemo_record_start_frame(fix frame_time );
void newdemo_record_render_object(vmobjptridx_t  obj);
void newdemo_record_viewer_object(vcobjptridx_t obj);
icobjptridx_t newdemo_find_object(object_signature_t signature);
void newdemo_record_kill_sound_linked_to_object(vcobjptridx_t);
void newdemo_start_playback(const char *filename);
void newdemo_record_morph_frame(vcobjptridx_t);
}
#endif
extern void newdemo_record_sound_3d_once( int soundno, int angle, int volume );
extern void newdemo_record_sound_once( int soundno );
extern void newdemo_record_sound( int soundno );
void newdemo_record_wall_hit_process( segnum_t segnum, int side, int damage, int playernum );
extern void newdemo_record_player_stats(int shields, int energy, int score );
void newdemo_record_wall_toggle(segnum_t segnum, int side );
extern void newdemo_record_control_center_destroyed();
extern void newdemo_record_hud_message(const char *s);
extern void newdemo_record_palette_effect(short r, short g, short b);
extern void newdemo_record_player_energy(int);
extern void newdemo_record_player_shields(int);
extern void newdemo_record_player_flags(unsigned);
extern void newdemo_record_player_weapon(int, int);
void newdemo_record_effect_blowup(segnum_t, int, const vms_vector &);
extern void newdemo_record_homing_distance(fix);
extern void newdemo_record_letterbox(void);
extern void newdemo_record_rearview(void);
extern void newdemo_record_restore_cockpit(void);
#ifdef dsx
void newdemo_record_wall_set_tmap_num1(vcsegidx_t seg, unsigned side, vcsegidx_t cseg, unsigned cside, texture1_value tmap);
void newdemo_record_wall_set_tmap_num2(vcsegidx_t seg, unsigned side, vcsegidx_t cseg, unsigned cside, texture2_value tmap);
#endif
extern void newdemo_record_multi_cloak(int pnum);
extern void newdemo_record_multi_decloak(int pnum);
#ifdef dsx
namespace dsx {
extern void newdemo_set_new_level(int level_num);
}
#endif
extern void newdemo_record_restore_rearview(void);

extern void newdemo_record_multi_death(int pnum);
extern void newdemo_record_multi_kill(int pnum, sbyte kill);
void newdemo_record_multi_connect(unsigned pnum, unsigned new_player, const char *new_callsign);
extern void newdemo_record_multi_reconnect(int pnum);
extern void newdemo_record_multi_disconnect(int pnum);
extern void newdemo_record_player_score(int score);
void newdemo_record_multi_score(unsigned pnum, int score);
extern void newdemo_record_primary_ammo(int new_ammo);
extern void newdemo_record_secondary_ammo(int new_ammo);
void newdemo_record_door_opening(segnum_t segnum, int side);
void newdemo_record_laser_level(laser_level old_level, laser_level new_level);

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
void newdemo_record_player_afterburner(fix afterburner);
void newdemo_record_cloaking_wall(wallnum_t front_wall_num, wallnum_t back_wall_num, ubyte type, ubyte state, fix cloak_value, fix l0, fix l1, fix l2, fix l3);
void newdemo_record_guided_start();
void newdemo_record_guided_end();
void newdemo_record_secret_exit_blown(int truth);
void newdemo_record_trigger(vcsegidx_t segnum, unsigned side, objnum_t objnum, unsigned shot);
#endif
}
#endif

// Functions called during playback process...
extern void newdemo_object_move_all();
extern window_event_result newdemo_playback_one_frame();
#ifdef dsx
namespace dsx {
extern window_event_result newdemo_goto_end(int to_rewrite);
}
#endif
extern window_event_result newdemo_goto_beginning();

// Interactive functions to control playback/record;
#ifdef dsx
namespace dsx {
extern void newdemo_stop_playback();
}
#endif
extern void newdemo_start_recording();
extern void newdemo_stop_recording();

extern int newdemo_swap_endian(const char *filename);

extern int newdemo_get_percent_done();

extern void newdemo_record_link_sound_to_object3( int soundno, objnum_t objnum, fix max_volume, fix  max_distance, int loop_start, int loop_end );
int newdemo_count_demos();

#endif
