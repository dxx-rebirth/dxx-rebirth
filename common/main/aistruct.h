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
 * Structs and constants for AI system.
 * object.h depends on this.
 * ai.h depends on object.h.
 * Get it?
 *
 */

#pragma once

#include <physfs.h>

#ifdef __cplusplus
#include "polyobj.h"
#include "pack.h"
#include "objnum.h"
#include "fwd-segment.h"

#define GREEN_GUY   1

#define MAX_SEGMENTS_PER_PATH       20

namespace dcx {

enum class player_awareness_type_t : uint8_t
{
	PA_NONE,
	PA_NEARBY_ROBOT_FIRED		= 1,  // Level of robot awareness after nearby robot fires a weapon
	PA_WEAPON_WALL_COLLISION	= 2,  // Level of robot awareness after player weapon hits nearby wall
//	PA_PLAYER_VISIBLE			= 2,  // Level of robot awareness if robot is looking towards player, and player not hidden
	PA_PLAYER_COLLISION			= 3,  // Level of robot awareness after player bumps into robot
	PA_WEAPON_ROBOT_COLLISION	= 4,  // Level of robot awareness after player weapon hits nearby robot
};

enum class player_visibility_state : int8_t
{
	no_line_of_sight,
	visible_not_in_field_of_view,
	visible_and_in_field_of_view,
};

static inline unsigned player_is_visible(const player_visibility_state s)
{
	return static_cast<unsigned>(s) > 0;
}

}

// Constants indicating currently moving forward or backward through
// path.  Note that you can add aip->direction to aip_path_index to
// get next segment on path.
#define AI_DIR_FORWARD  1
#define AI_DIR_BACKWARD (-AI_DIR_FORWARD)

#ifdef dsx
namespace dsx {

enum class ai_behavior : uint8_t
{
// Behaviors
	AIB_STILL = 0x80,
	AIB_NORMAL = 0x81,
	AIB_RUN_FROM = 0x83,
	AIB_STATION = 0x85,
#if defined(DXX_BUILD_DESCENT_I)
	AIB_HIDE = 0x82,
	AIB_FOLLOW_PATH = 0x84,
#elif defined(DXX_BUILD_DESCENT_II)
	AIB_BEHIND = 0x82,
	AIB_SNIPE = 0x84,
	AIB_FOLLOW = 0x86,
#endif
};

#define MIN_BEHAVIOR    0x80
#if defined(DXX_BUILD_DESCENT_I)
#define	MAX_BEHAVIOR	0x85
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_BEHAVIOR    0x86
#endif

enum class ai_mode : uint8_t
{
//  Modes
	AIM_STILL = 0,
	AIM_WANDER = 1,
	AIM_FOLLOW_PATH = 2,
	AIM_CHASE_OBJECT = 3,
	AIM_RUN_FROM_OBJECT = 4,
	AIM_FOLLOW_PATH_2 = 6,
	AIM_OPEN_DOOR = 7,
#if defined(DXX_BUILD_DESCENT_I)
	AIM_HIDE = 5,
#elif defined(DXX_BUILD_DESCENT_II)
	AIM_BEHIND = 5,
	AIM_GOTO_PLAYER = 8,   //  Only for escort behavior
	AIM_GOTO_OBJECT = 9,   //  Only for escort behavior

	AIM_SNIPE_ATTACK = 10,
	AIM_SNIPE_FIRE = 11,
	AIM_SNIPE_RETREAT = 12,
	AIM_SNIPE_RETREAT_BACKWARDS = 13,
	AIM_SNIPE_WAIT = 14,

	AIM_THIEF_ATTACK = 15,
	AIM_THIEF_RETREAT = 16,
	AIM_THIEF_WAIT = 17,
#endif
};

}
#endif

#define AISM_GOHIDE                 0
#define AISM_HIDING                 1

#define AI_MAX_STATE    7
#define AI_MAX_EVENT    4

#define AIS_NONE        0
#define AIS_REST        1
#define AIS_SRCH        2
#define AIS_LOCK        3
#define AIS_FLIN        4
#define AIS_FIRE        5
#define AIS_RECO        6
#define AIS_ERR_        7

#define AIE_FIRE        0
#define AIE_HITT        1
#define AIE_COLL        2
#define AIE_HURT        3

//typedef struct opath {
//	sbyte   path_index;     // current index of path
//	sbyte   path_direction; // current path direction
//	sbyte   path_length;    // length of current path
//	sbyte   nothing;
//	short   path[MAX_SEGMENTS_PER_PATH];
//	short   always_0xabc;   // If this is ever not 0xabc, then someone overwrote
//} opath;
//
//typedef struct oai_state {
//	short   mode;               //
//	short   counter;            // kind of a hack, frame countdown until switch modes
//	opath   paths[2];
//	vms_vector movement_vector; // movement vector for one second
//} oai_state;

#if defined(DXX_BUILD_DESCENT_II)
#define SUB_FLAGS_GUNSEG        0x01
#define SUB_FLAGS_SPROX         0x02    // If set, then this bot drops a super prox, not a prox, when it's time to drop something
#define SUB_FLAGS_CAMERA_AWAKE  0x04    // If set, a camera (on a missile) woke this robot up, so don't fire at player.  Can look real stupid!
#endif

//  Constants defining meaning of flags in ai_state
#define MAX_AI_FLAGS    11          // This MUST cause word (4 bytes) alignment in ai_static, allowing for one byte mode

#define CURRENT_GUN     flags[0]    // This is the last gun the object fired from
#define CURRENT_STATE   flags[1]    // current behavioral state
#define GOAL_STATE      flags[2]    // goal state
#define PATH_DIR        flags[3]    // direction traveling path, 1 = forward, -1 = backward, other = error!
#if defined(DXX_BUILD_DESCENT_I)
#define	SUBMODE			flags[4]			//	submode, eg AISM_HIDING if mode == AIM_HIDE
#elif defined(DXX_BUILD_DESCENT_II)
#define SUB_FLAGS       flags[4]    // bit 0: Set -> Robot's current gun in different segment than robot's center.
#endif
#define GOALSIDE        flags[5]    // for guys who open doors, this is the side they are going after.
#define CLOAKED         flags[6]    // Cloaked now.
#define SKIP_AI_COUNT   flags[7]    // Skip AI this frame, but decrement in do_ai_frame.
#define  REMOTE_OWNER   flags[8]    // Who is controlling this remote AI object (multiplayer use only)
#define  REMOTE_SLOT_NUM flags[9]   // What slot # is this robot in for remote control purposes (multiplayer use only)

// This is the stuff that is permanent for an AI object.
#ifdef dsx
namespace dsx {

// Rather temporal AI stuff.
struct ai_local : public prohibit_void_ptr<ai_local>
{
// These used to be bytes, changed to ints so I could set watchpoints on them.
	player_awareness_type_t player_awareness_type = player_awareness_type_t::PA_NONE;           // type of awareness of player
	uint8_t retry_count = 0;                     // number of retries in physics last time this object got moved.
	uint8_t consecutive_retries = 0;             // number of retries in consecutive frames (ie, without a retry_count of 0)
	player_visibility_state previous_visibility{};             // Visibility of player last time we checked.
	uint8_t rapidfire_count = 0;                 // number of shots fired rapidly
	ai_mode mode{};                            // current mode within behavior
	segnum_t      goal_segment{};                    // goal segment for current path
	fix        next_action_time = 0;              // time in seconds until something happens, mode dependent
	fix        next_fire = 0;                     // time in seconds until can fire again
#if defined(DXX_BUILD_DESCENT_II)
	fix        next_fire2 = 0;                    // time in seconds until can fire again from second weapon
#endif
	fix        player_awareness_time = 0;         // time in seconds robot will be aware of player, 0 means not aware of player
	fix        time_since_processed = 0;          // time since this robot last processed in do_ai_frame
	fix64      time_player_seen = 0;              // absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix64      time_player_sound_attacked = 0;    // absolute time in seconds at which player was last seen with visibility of 2.
	fix64      next_misc_sound_time = 0;          // absolute time in seconds at which this robot last made an angry or lurking sound.
	std::array<vms_angvec, MAX_SUBMODELS> goal_angles{};    // angles for each subobject
	std::array<vms_angvec, MAX_SUBMODELS> delta_angles{};   // angles for each subobject
	std::array<sbyte, MAX_SUBMODELS> goal_state{};     // Goal state for this sub-object
	std::array<sbyte, MAX_SUBMODELS> achieved_state{}; // Last achieved state
};

struct ai_static : public prohibit_void_ptr<ai_static>
{
	ai_behavior behavior = static_cast<ai_behavior>(0);               //
	std::array<sbyte, MAX_AI_FLAGS> flags{};    // various flags, meaning defined by constants
	segnum_t hide_segment{};           // Segment to go to for hiding.
	short hide_index{};             // Index in Path_seg_points
	short path_length{};            // Length of hide path.
#if defined(DXX_BUILD_DESCENT_I)
	short cur_path_index{};         // Current index in path.
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte cur_path_index{};         // Current index in path.
	sbyte dying_sound_playing{};    // !0 if this robot is playing its dying sound.
#endif
	objnum_t danger_laser_num{};
	object_signature_t danger_laser_signature;
#if defined(DXX_BUILD_DESCENT_II)
	fix64 dying_start_time{};       // Time at which this robot started dying.
#endif
	ai_local ail;
};

// Same as above but structure Savegames/Multiplayer objects expect
struct ai_static_rw
{
	ubyte   behavior;               //
	sbyte   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	short   hide_segment;           // Segment to go to for hiding.
	short   hide_index;             // Index in Path_seg_points
	short   path_length;            // Length of hide path.
#if defined(DXX_BUILD_DESCENT_I)
	short   cur_path_index;         // Current index in path.
	short   follow_path_start_seg;  // Start segment for robot which follows path.
	short   follow_path_end_seg;    // End segment for robot which follows path.
	int     danger_laser_signature;
	short   danger_laser_num;
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte   cur_path_index;         // Current index in path.
	sbyte   dying_sound_playing;    // !0 if this robot is playing its dying sound.
	short   danger_laser_num;
	int     danger_laser_signature;
	fix     dying_start_time;       // Time at which this robot started dying.
#endif
} __pack__;

// Same as above but structure Savegames expect
struct ai_local_rw
{
// These used to be bytes, changed to ints so I could set watchpoints on them.
#if defined(DXX_BUILD_DESCENT_I)
	sbyte      player_awareness_type;           // type of awareness of player
	sbyte      retry_count;                     // number of retries in physics last time this object got moved.
	sbyte      consecutive_retries;             // number of retries in consecutive frames (ie, without a retry_count of 0)
	sbyte      mode;                            // current mode within behavior
	sbyte      previous_visibility;             // Visibility of player last time we checked.
	sbyte      rapidfire_count;                 // number of shots fired rapidly
	short      goal_segment;                    // goal segment for current path
	fix        last_see_time, last_attack_time; // For sound effects, time at which player last seen, attacked
#elif defined(DXX_BUILD_DESCENT_II)
	int        player_awareness_type;         // type of awareness of player
	int        retry_count;                   // number of retries in physics last time this object got moved.
	int        consecutive_retries;           // number of retries in consecutive frames (ie, without a retry_count of 0)
	int        mode;                          // current mode within behavior
	int        previous_visibility;           // Visibility of player last time we checked.
	int        rapidfire_count;               // number of shots fired rapidly
	int        goal_segment;                  // goal segment for current path
#endif
	fix        next_action_time;              // time in seconds until something happens, mode dependent
	fix        next_fire;                     // time in seconds until can fire again
#if defined(DXX_BUILD_DESCENT_II)
	fix        next_fire2;                    // time in seconds until can fire again from second weapon
#endif
	fix        player_awareness_time;         // time in seconds robot will be aware of player, 0 means not aware of player
	fix        time_player_seen;              // absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix        time_player_sound_attacked;    // absolute time in seconds at which player was last seen with visibility of 2.
	fix        next_misc_sound_time;          // absolute time in seconds at which this robot last made an angry or lurking sound.
	fix        time_since_processed;          // time since this robot last processed in do_ai_frame
	vms_angvec goal_angles[MAX_SUBMODELS];    // angles for each subobject
	vms_angvec delta_angles[MAX_SUBMODELS];   // angles for each subobject
	sbyte      goal_state[MAX_SUBMODELS];     // Goal state for this sub-object
	sbyte      achieved_state[MAX_SUBMODELS]; // Last achieved state
};

struct ai_cloak_info : public prohibit_void_ptr<ai_cloak_info>
{
	fix64       last_time;
#if defined(DXX_BUILD_DESCENT_II)
	segnum_t         last_segment;
#endif
	vms_vector  last_position;
};

// Same as above but structure Savegames expect
struct ai_cloak_info_rw
{
	fix         last_time;
#if defined(DXX_BUILD_DESCENT_II)
	int         last_segment;
#endif
	vms_vector  last_position;
};

#if defined(DXX_BUILD_DESCENT_II)

// Maximum number kept track of, will keep stealing, causes stolen weapons to be lost!
struct d_thief_unique_state
{
	unsigned Stolen_item_index;   // Used in ai.c for controlling rate of Thief flare firing.
	std::array<uint8_t, 10> Stolen_items;
};

#endif
}
#endif

namespace dcx {

struct point_seg : prohibit_void_ptr<point_seg> {
	segnum_t         segnum;
	vms_vector  point;
};

struct seg_seg
{
	segnum_t       start, end;
};

constexpr std::integral_constant<unsigned, 2500> MAX_POINT_SEGS{};

}

// These are the information for a robot describing the location of
// the player last time he wasn't cloaked, and the time at which he
// was uncloaked.  We should store this for each robot, but that's
// memory expensive.
//extern fix        Last_uncloaked_time;
//extern vms_vector Last_uncloaked_position;

#ifdef dsx
namespace dsx {
extern void ai_do_cloak_stuff(void);
}
#endif

#endif
