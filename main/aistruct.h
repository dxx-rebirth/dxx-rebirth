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
 * Structs and constants for AI system.
 * object.h depends on this.
 * ai.h depends on object.h.
 * Get it?
 *
 */

#ifndef _AISTRUCT_H
#define _AISTRUCT_H

#include "inferno.h"
#include "polyobj.h"

#define GREEN_GUY   1

#define MAX_SEGMENTS_PER_PATH       20

#define PA_WEAPON_WALL_COLLISION    2   // Level of robot awareness after player weapon hits nearby wall
//#define PA_PLAYER_VISIBLE           2   // Level of robot awareness if robot is looking towards player, and player not hidden
#define PA_NEARBY_ROBOT_FIRED       1   // Level of robot awareness after nearby robot fires a weapon
#define PA_PLAYER_COLLISION         3   // Level of robot awareness after player bumps into robot
#define PA_WEAPON_ROBOT_COLLISION   4   // Level of robot awareness after player weapon hits nearby robot

//#define PAE_WEAPON_HIT_WALL         1   // weapon hit wall, create player awareness
//#define PAE_WEAPON_HIT_ROBOT        2   // weapon hit wall, create player awareness

// Constants indicating currently moving forward or backward through
// path.  Note that you can add aip->direction to aip_path_index to
// get next segment on path.
#define AI_DIR_FORWARD  1
#define AI_DIR_BACKWARD (-AI_DIR_FORWARD)

// Behaviors
#define AIB_STILL                       0x80
#define AIB_NORMAL                      0x81
#define AIB_BEHIND                      0x82
#define AIB_RUN_FROM                    0x83
#define AIB_SNIPE                       0x84
#define AIB_STATION                     0x85
#define AIB_FOLLOW                      0x86

#define MIN_BEHAVIOR    0x80
#define MAX_BEHAVIOR    0x86

//  Modes
#define AIM_STILL                   0
#define AIM_WANDER                  1
#define AIM_FOLLOW_PATH             2
#define AIM_CHASE_OBJECT            3
#define AIM_RUN_FROM_OBJECT         4
#define AIM_BEHIND                  5
#define AIM_FOLLOW_PATH_2           6
#define AIM_OPEN_DOOR               7
#define AIM_GOTO_PLAYER             8   //  Only for escort behavior
#define AIM_GOTO_OBJECT             9   //  Only for escort behavior

#define AIM_SNIPE_ATTACK            10
#define AIM_SNIPE_FIRE              11
#define AIM_SNIPE_RETREAT           12
#define AIM_SNIPE_RETREAT_BACKWARDS 13
#define AIM_SNIPE_WAIT              14

#define AIM_THIEF_ATTACK            15
#define AIM_THIEF_RETREAT           16
#define AIM_THIEF_WAIT              17

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

#define SUB_FLAGS_GUNSEG        0x01
#define SUB_FLAGS_SPROX         0x02    // If set, then this bot drops a super prox, not a prox, when it's time to drop something
#define SUB_FLAGS_CAMERA_AWAKE  0x04    // If set, a camera (on a missile) woke this robot up, so don't fire at player.  Can look real stupid!

//  Constants defining meaning of flags in ai_state
#define MAX_AI_FLAGS    11          // This MUST cause word (4 bytes) alignment in ai_static, allowing for one byte mode

#define CURRENT_GUN     flags[0]    // This is the last gun the object fired from
#define CURRENT_STATE   flags[1]    // current behavioral state
#define GOAL_STATE      flags[2]    // goal state
#define PATH_DIR        flags[3]    // direction traveling path, 1 = forward, -1 = backward, other = error!
#define SUB_FLAGS       flags[4]    // bit 0: Set -> Robot's current gun in different segment than robot's center.
#define GOALSIDE        flags[5]    // for guys who open doors, this is the side they are going after.
#define CLOAKED         flags[6]    // Cloaked now.
#define SKIP_AI_COUNT   flags[7]    // Skip AI this frame, but decrement in do_ai_frame.
#define  REMOTE_OWNER   flags[8]    // Who is controlling this remote AI object (multiplayer use only)
#define  REMOTE_SLOT_NUM flags[9]   // What slot # is this robot in for remote control purposes (multiplayer use only)
#define  MULTI_ANGER    flags[10]   // How angry is a robot in multiplayer mode

// This is the stuff that is permanent for an AI object.
typedef struct ai_static {
	ubyte   behavior;               //
	sbyte   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	short   hide_segment;           // Segment to go to for hiding.
	short   hide_index;             // Index in Path_seg_points
	short   path_length;            // Length of hide path.
	sbyte   cur_path_index;         // Current index in path.
	sbyte   dying_sound_playing;    // !0 if this robot is playing its dying sound.
	short   danger_laser_num;
	int     danger_laser_signature;
	fix64   dying_start_time;       // Time at which this robot started dying.
} __pack__ ai_static;

// Same as above but structure Savegames/Multiplayer objects expect
typedef struct ai_static_rw {
	ubyte   behavior;               //
	sbyte   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	short   hide_segment;           // Segment to go to for hiding.
	short   hide_index;             // Index in Path_seg_points
	short   path_length;            // Length of hide path.
	sbyte   cur_path_index;         // Current index in path.
	sbyte   dying_sound_playing;    // !0 if this robot is playing its dying sound.
	short   danger_laser_num;
	int     danger_laser_signature;
	fix     dying_start_time;       // Time at which this robot started dying.
} __pack__ ai_static_rw;

// Rather temporal AI stuff.
typedef struct ai_local {
// These used to be bytes, changed to ints so I could set watchpoints on them.
	int        player_awareness_type;         // type of awareness of player
	int        retry_count;                   // number of retries in physics last time this object got moved.
	int        consecutive_retries;           // number of retries in consecutive frames (ie, without a retry_count of 0)
	int        mode;                          // current mode within behavior
	int        previous_visibility;           // Visibility of player last time we checked.
	int        rapidfire_count;               // number of shots fired rapidly
	int        goal_segment;                  // goal segment for current path
	fix        next_action_time;              // time in seconds until something happens, mode dependent
	fix        next_fire;                     // time in seconds until can fire again
	fix        next_fire2;                    // time in seconds until can fire again from second weapon
	fix        player_awareness_time;         // time in seconds robot will be aware of player, 0 means not aware of player
	fix64      time_player_seen;              // absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix64      time_player_sound_attacked;    // absolute time in seconds at which player was last seen with visibility of 2.
	fix64      next_misc_sound_time;          // absolute time in seconds at which this robot last made an angry or lurking sound.
	fix        time_since_processed;          // time since this robot last processed in do_ai_frame
	vms_angvec goal_angles[MAX_SUBMODELS];    // angles for each subobject
	vms_angvec delta_angles[MAX_SUBMODELS];   // angles for each subobject
	sbyte      goal_state[MAX_SUBMODELS];     // Goal state for this sub-object
	sbyte      achieved_state[MAX_SUBMODELS]; // Last achieved state
} ai_local;

// Same as above but structure Savegames expect
typedef struct ai_local_rw {
// These used to be bytes, changed to ints so I could set watchpoints on them.
	int        player_awareness_type;         // type of awareness of player
	int        retry_count;                   // number of retries in physics last time this object got moved.
	int        consecutive_retries;           // number of retries in consecutive frames (ie, without a retry_count of 0)
	int        mode;                          // current mode within behavior
	int        previous_visibility;           // Visibility of player last time we checked.
	int        rapidfire_count;               // number of shots fired rapidly
	int        goal_segment;                  // goal segment for current path
	fix        next_action_time;              // time in seconds until something happens, mode dependent
	fix        next_fire;                     // time in seconds until can fire again
	fix        next_fire2;                    // time in seconds until can fire again from second weapon
	fix        player_awareness_time;         // time in seconds robot will be aware of player, 0 means not aware of player
	fix        time_player_seen;              // absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix        time_player_sound_attacked;    // absolute time in seconds at which player was last seen with visibility of 2.
	fix        next_misc_sound_time;          // absolute time in seconds at which this robot last made an angry or lurking sound.
	fix        time_since_processed;          // time since this robot last processed in do_ai_frame
	vms_angvec goal_angles[MAX_SUBMODELS];    // angles for each subobject
	vms_angvec delta_angles[MAX_SUBMODELS];   // angles for each subobject
	sbyte      goal_state[MAX_SUBMODELS];     // Goal state for this sub-object
	sbyte      achieved_state[MAX_SUBMODELS]; // Last achieved state
} ai_local_rw;

typedef struct {
	fix64       last_time;
	int         last_segment;
	vms_vector  last_position;
} ai_cloak_info;

// Same as above but structure Savegames expect
typedef struct {
	fix         last_time;
	int         last_segment;
	vms_vector  last_position;
} ai_cloak_info_rw;

typedef struct {
	int         segnum;
	vms_vector  point;
} point_seg;

typedef struct {
	short       start, end;
} seg_seg;

#define MAX_POINT_SEGS  2500

extern point_seg    Point_segs[MAX_POINT_SEGS];
extern point_seg    *Point_segs_free_ptr;
extern int          Overall_agitation;

// These are the information for a robot describing the location of
// the player last time he wasn't cloaked, and the time at which he
// was uncloaked.  We should store this for each robot, but that's
// memory expensive.
//extern fix        Last_uncloaked_time;
//extern vms_vector Last_uncloaked_position;

extern void ai_do_cloak_stuff(void);

#endif /* _AISTRUCT_H */
