/* $Id: aistruct.h,v 1.2 2003-10-04 03:14:47 btb Exp $ */
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
 * Old Log:
 * Revision 1.2  1995/10/10  11:49:04  allender
 * PC ai code
 *
 * Revision 1.1  1995/05/16  15:54:08  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:30:19  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.34  1995/01/25  13:50:46  mike
 * Robots make angry sounds.
 *
 * Revision 1.33  1994/12/29  12:44:56  rob
 * Added new coop robot flag.
 *
 * Revision 1.32  1994/12/20  20:41:54  rob
 * Added new ai flag for multiplayer robots.
 *
 * Revision 1.31  1994/12/19  16:37:39  rob
 * Added a new flag for remote controlled objects.
 *
 * Revision 1.30  1994/12/07  00:36:07  mike
 * fix phys_apply_rot for robots -- ai was bashing effect in next frame.
 *
 * Revision 1.29  1994/12/02  22:06:28  mike
 * add fields to allow robots to make awareness sounds every so often, not every damn blasted frame
 *
 * Revision 1.28  1994/11/04  17:18:35  yuan
 * Increased MAX_SEG_POINTS to 2500.
 *
 * Revision 1.27  1994/10/17  21:19:22  mike
 * robot cloaking.
 *
 * Revision 1.26  1994/10/12  21:28:38  mike
 * Add new ai mode: AIM_OPEN_DOOR.
 * Add GOALSIDE to aip.
 *
 * Revision 1.25  1994/09/25  23:41:08  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeleft is now a property
 * of all objects, and the object structure has been otherwise cleaned up.
 *
 * Revision 1.24  1994/09/21  12:28:11  mike
 * Change AI behavior for when player cloaked
 *
 * Revision 1.23  1994/09/19  21:43:00  mike
 * Add follow_path_start_seg and follow_path_end_seg to aistruct.h.
 *
 * Revision 1.22  1994/09/18  18:06:14  mike
 * Add Last_uncloaked_time and Last_uncloaked_position variables.
 *
 * Revision 1.21  1994/09/15  16:31:38  mike
 * Define GREEN_GUY
 * Add previous_visibility to ai_local struct.
 *
 * Revision 1.20  1994/09/12  19:12:45  mike
 * Change some bytes to ints in ai_local so I could set watchpoints.
 *
 * Revision 1.19  1994/08/25  21:53:31  mike
 * Add behavior, taking place of what used to be mode.
 *
 * Revision 1.18  1994/08/23  16:38:09  mike
 * rapidfire_count in ai_local.
 *
 * Revision 1.17  1994/08/19  17:38:23  mike
 * *** empty log message ***
 *
 * Revision 1.16  1994/08/17  22:18:58  mike
 * add time_since_processed to ai_local.
 *
 * Revision 1.15  1994/08/10  19:52:25  mike
 * Add Overall_agitation.
 *
 * Revision 1.14  1994/08/04  16:32:32  mike
 * Add time_player_seen.
 *
 * Revision 1.13  1994/07/28  16:58:11  mike
 * Move constants from ai.c
 *
 * Revision 1.12  1994/07/19  15:26:24  mike
 * New ai_static and ai_local structures.
 *
 * Revision 1.11  1994/07/15  15:17:19  matt
 * Changes MAX_AI_FLAGS for better alignment
 *
 */

#ifndef _AISTRUCT_H
#define _AISTRUCT_H

#include "inferno.h"
//#include "polyobj.h"

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

// This is the stuff that is permanent for an AI object and is
// therefore saved to disk.
typedef struct ai_static {
	ubyte   behavior;               //
	sbyte   flags[MAX_AI_FLAGS];    // various flags, meaning defined by constants
	short   hide_segment;           // Segment to go to for hiding.
	short   hide_index;             // Index in Path_seg_points
	short   path_length;            // Length of hide path.
	sbyte   cur_path_index;         // Current index in path.
	sbyte   dying_sound_playing;    // !0 if this robot is playing its dying sound.

	// -- not needed! -- short   follow_path_start_seg;  // Start segment for robot which follows path.
	// -- not needed! -- short   follow_path_end_seg;    // End segment for robot which follows path.

	short   danger_laser_num;
	int     danger_laser_signature;
	fix     dying_start_time;       // Time at which this robot started dying.

	//sbyte   extras[28];             // 32 extra bytes for storing stuff so we don't have to change versions on disk
} __pack__ ai_static;

// This is the stuff which doesn't need to be saved to disk.
typedef struct ai_local {
// These used to be bytes, changed to ints so I could set watchpoints on them.
// player_awareness_type..rapidfire_count used to be bytes
// goal_segment used to be short.
	int     player_awareness_type;  // type of awareness of player
	int     retry_count;            // number of retries in physics last time this object got moved.
	int     consecutive_retries;    // number of retries in consecutive frames (ie, without a retry_count of 0)
	int     mode;                   // current mode within behavior
	int     previous_visibility;    // Visibility of player last time we checked.
	int     rapidfire_count;        // number of shots fired rapidly
	int     goal_segment;           // goal segment for current path

	// -- MK, 10/21/95, unused -- fix     last_see_time, last_attack_time; // For sound effects, time at which player last seen, attacked

	fix     next_action_time;           // time in seconds until something happens, mode dependent
	fix     next_fire;                  // time in seconds until can fire again
	fix     next_fire2;                 // time in seconds until can fire again from second weapon
	fix     player_awareness_time;      // time in seconds robot will be aware of player, 0 means not aware of player
	fix     time_player_seen;           // absolute time in seconds at which player was last seen, might cause to go into follow_path mode
	fix     time_player_sound_attacked; // absolute time in seconds at which player was last seen with visibility of 2.
	fix     next_misc_sound_time;       // absolute time in seconds at which this robot last made an angry or lurking sound.
	fix     time_since_processed;       // time since this robot last processed in do_ai_frame
	vms_angvec goal_angles[MAX_SUBMODELS];  // angles for each subobject
	vms_angvec delta_angles[MAX_SUBMODELS]; // angles for each subobject
	sbyte   goal_state[MAX_SUBMODELS];      // Goal state for this sub-object
	sbyte   achieved_state[MAX_SUBMODELS];  // Last achieved state
} ai_local;

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
