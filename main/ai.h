/* $Id: ai.h,v 1.4 2003-03-14 21:28:29 btb Exp $ */
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
 * Header file for AI system.
 *
 * Old Log:
 * Revision 1.3  1995/10/15  16:28:07  allender
 * added flag to player_is_visible function
 *
 * Revision 1.2  1995/10/10  11:48:32  allender
 * PC ai header
 *
 * Revision 1.1  1995/05/16  15:54:00  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:07  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.57  1995/02/04  17:28:31  mike
 * make station guys return better.
 *
 * Revision 1.56  1995/02/04  10:03:23  mike
 * Fly to exit cheat.
 *
 * Revision 1.55  1995/02/01  19:23:52  rob
 * Externed a boss var.
 *
 * Revision 1.54  1995/01/30  13:00:58  mike
 * Make robots fire at player other than one they are controlled by sometimes.
 *
 * Revision 1.53  1995/01/26  15:09:16  rob
 * Changed robot gating to accomodate multiplayer.
 *
 * Revision 1.52  1995/01/26  12:23:12  rob
 * Added new externs needed for multiplayer.
 *
 * Revision 1.51  1995/01/21  21:22:14  mike
 * Kill prototype of init_boss_segments, which didn't need to be public
 * and had changed.
 *
 * Revision 1.50  1995/01/16  19:24:29  mike
 * Publicize BOSS_GATE_MATCEN_NUM and Boss_been_hit.
 *
 * Revision 1.49  1995/01/02  16:17:35  mike
 * prototype some super boss function for gameseq.
 *
 * Revision 1.48  1994/12/19  17:08:06  mike
 * deal with new ai_multiplayer_awareness which returns a value saying whether this object can be moved by this player.
 *
 * Revision 1.47  1994/12/12  17:18:04  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.46  1994/12/08  15:46:16  mike
 * better robot behavior.
 *
 * Revision 1.45  1994/11/27  23:16:08  matt
 * Made debug code go away when debugging turned off
 *
 * Revision 1.44  1994/11/16  23:38:41  mike
 * new improved boss teleportation behavior.
 *
 * Revision 1.43  1994/11/10  17:45:11  mike
 * debugging.
 *
 * Revision 1.42  1994/11/07  10:37:42  mike
 * hooks for rob's network code.
 *
 * Revision 1.41  1994/11/06  15:10:50  mike
 * prototype a debug function for dumping ai info.
 *
 * Revision 1.40  1994/11/02  17:57:30  rob
 * Added extern of Believe_player_pos needed to get control centers
 * locating people.
 *
 * Revision 1.39  1994/10/28  19:43:39  mike
 * Prototype Boss_cloak_start_time, Boss_cloak_end_time.
 *
 * Revision 1.38  1994/10/22  14:14:42  mike
 * Prototype ai_reset_all_paths.
 *
 * Revision 1.37  1994/10/21  20:42:01  mike
 * Define MAX_PATH_LENGTH: maximum allowed length of a path.
 *
 * Revision 1.36  1994/10/20  09:49:18  mike
 * Prototype something.
 *
 *
 * Revision 1.35  1994/10/18  15:37:52  mike
 * Define ROBOT_BOSS1.
 *
 * Revision 1.34  1994/10/13  11:12:25  mike
 * Prototype some door functions.
 *
 * Revision 1.33  1994/10/12  21:28:51  mike
 * Prototype create_n_segment_path_to_door
 * Prototype ai_open_doors_in_segment
 * Prototype ai_door_is_openable.
 *
 * Revision 1.32  1994/10/11  15:59:41  mike
 * Prototype Robot_firing_enabled.
 *
 * Revision 1.31  1994/10/09  22:02:48  mike
 * Adapt create_path_points and create_n_segment_path prototypes to use avoid_seg for player evasion.
 *
 * Revision 1.30  1994/09/18  18:07:44  mike
 * Update prototypes for create_path_points and create_path_to_player.
 *
 * Revision 1.29  1994/09/15  16:34:08  mike
 * Prototype do_ai_robot_hit_attack.
 *
 * Revision 1.28  1994/09/12  19:12:35  mike
 * Prototype attempt_to_resume_path.
 *
 * Revision 1.27  1994/08/25  21:55:32  mike
 * Add some prototypes.
 *
 * Revision 1.26  1994/08/10  19:53:24  mike
 * Prototype create_path_to_player and init_robots_for_level.
 *
 * Revision 1.25  1994/08/04  16:32:58  mike
 * prototype create_path_to_player.
 *
 * Revision 1.24  1994/08/03  15:17:20  mike
 * Prototype make_random_vector.
 *
 * Revision 1.23  1994/07/31  18:10:34  mike
 * Update prototype for create_path_points.
 *
 * Revision 1.22  1994/07/28  12:36:14  matt
 * Cleaned up object bumping code
 *
 */

#ifndef _AI_H
#define _AI_H

#include <stdio.h>

#include "object.h"
#include "fvi.h"
#include "robot.h"

#define PLAYER_AWARENESS_INITIAL_TIME   (3*F1_0)
#define MAX_PATH_LENGTH                 30          // Maximum length of path in ai path following.
#define MAX_DEPTH_TO_SEARCH_FOR_PLAYER  10
#define BOSS_GATE_MATCEN_NUM            -1
#define MAX_BOSS_TELEPORT_SEGS          100
#define BOSS_ECLIP_NUM                  53

#define ROBOT_BRAIN 7
#define ROBOT_BOSS1 17

#define ROBOT_FIRE_AGITATION 94

#define BOSS_D2     21 // Minimum D2 boss value.
#define BOSS_COOL   21
#define BOSS_WATER  22
#define BOSS_FIRE   23
#define BOSS_ICE    24
#define BOSS_ALIEN1 25
#define BOSS_ALIEN2 26

#define NUM_D2_BOSSES 8

extern ubyte Boss_teleports[NUM_D2_BOSSES];     // Set byte if this boss can teleport
extern ubyte Boss_spew_more[NUM_D2_BOSSES];     // Set byte if this boss can teleport
//extern ubyte Boss_cloaks[NUM_D2_BOSSES];        // Set byte if this boss can cloak
extern ubyte Boss_spews_bots_energy[NUM_D2_BOSSES];     // Set byte if boss spews bots when hit by energy weapon.
extern ubyte Boss_spews_bots_matter[NUM_D2_BOSSES];     // Set byte if boss spews bots when hit by matter weapon.
extern ubyte Boss_invulnerable_energy[NUM_D2_BOSSES];   // Set byte if boss is invulnerable to energy weapons.
extern ubyte Boss_invulnerable_matter[NUM_D2_BOSSES];   // Set byte if boss is invulnerable to matter weapons.
extern ubyte Boss_invulnerable_spot[NUM_D2_BOSSES];     // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

extern fix Boss_cloak_start_time, Boss_cloak_end_time;
extern int Num_boss_teleport_segs;
extern short Boss_teleport_segs[MAX_BOSS_TELEPORT_SEGS];
extern fix Last_teleport_time;
extern fix Boss_cloak_duration;

extern ai_local Ai_local_info[MAX_OBJECTS];
extern vms_vector Believed_player_pos;
extern int Believed_player_seg;

extern void move_towards_segment_center(object *objp);
extern int gate_in_robot(int type, int segnum);
extern void do_ai_movement(object *objp);
extern void ai_move_to_new_segment( object * obj, short newseg, int first_time );
// extern void ai_follow_path( object * obj, short newseg, int first_time );
extern void ai_recover_from_wall_hit(object *obj, int segnum);
extern void ai_move_one(object *objp);
extern void do_ai_frame(object *objp);
extern void init_ai_object(int objnum, int initial_mode, int hide_segment);
extern void update_player_awareness(object *objp, fix new_awareness);
extern void create_awareness_event(object *objp, int type);         // object *objp can create awareness of player, amount based on "type"
extern void do_ai_frame_all(void);
extern void init_ai_system(void);
extern void reset_ai_states(object *objp);
extern int create_path_points(object *objp, int start_seg, int end_seg, point_seg *point_segs, short *num_points, int max_depth, int random_flag, int safety_flag, int avoid_seg);
extern void create_all_paths(void);
extern void create_path_to_station(object *objp, int max_length);
extern void ai_follow_path(object *objp, int player_visibility, int previous_visibility, vms_vector *vec_to_player);
extern void ai_turn_towards_vector(vms_vector *vec_to_player, object *obj, fix rate);
extern void ai_turn_towards_vel_vec(object *objp, fix rate);
extern void init_ai_objects(void);
extern void do_ai_robot_hit(object *robot, int type);
extern void create_n_segment_path(object *objp, int path_length, int avoid_seg);
extern void create_n_segment_path_to_door(object *objp, int path_length, int avoid_seg);
extern void make_random_vector(vms_vector *vec);
extern void init_robots_for_level(void);
extern int ai_behavior_to_mode(int behavior);
extern int Robot_firing_enabled;
extern void create_path_to_segment(object *objp, int goalseg, int max_length, int safety_flag);
extern int ready_to_fire(robot_info *robptr, ai_local *ailp);
extern int polish_path(object *objp, point_seg *psegs, int num_points);
extern void move_towards_player(object *objp, vms_vector *vec_to_player);

// max_length is maximum depth of path to create.
// If -1, use default: MAX_DEPTH_TO_SEARCH_FOR_PLAYER
extern void create_path_to_player(object *objp, int max_length, int safety_flag);
extern void attempt_to_resume_path(object *objp);

// When a robot and a player collide, some robots attack!
extern void do_ai_robot_hit_attack(object *robot, object *player, vms_vector *collision_point);
extern void ai_open_doors_in_segment(object *robot);
extern int ai_door_is_openable(object *objp, segment *segp, int sidenum);
extern int player_is_visible_from_object(object *objp, vms_vector *pos, fix field_of_view, vms_vector *vec_to_player);
extern void ai_reset_all_paths(void);   // Reset all paths.  Call at the start of a level.
extern int ai_multiplayer_awareness(object *objp, int awareness_level);

// In escort.c
extern void do_escort_frame(object *objp, fix dist_to_player, int player_visibility);
extern void do_snipe_frame(object *objp, fix dist_to_player, int player_visibility, vms_vector *vec_to_player);
extern void do_thief_frame(object *objp, fix dist_to_player, int player_visibility, vms_vector *vec_to_player);

#ifndef NDEBUG
extern void force_dump_ai_objects_all(char *msg);
#else
#define force_dump_ai_objects_all(msg)
#endif

extern void start_boss_death_sequence(object *objp);
extern void ai_init_boss_for_ship(void);
extern int Boss_been_hit;
extern fix AI_proc_time;

// Stuff moved from ai.c by MK on 05/25/95.
#define ANIM_RATE       (F1_0/16)
#define DELTA_ANG_SCALE 16

#define OVERALL_AGITATION_MAX   100
#define MAX_AI_CLOAK_INFO       8   // Must be a power of 2!

typedef struct {
	fix         last_time;
	int         last_segment;
	vms_vector  last_position;
} ai_cloak_info;

#define BOSS_CLOAK_DURATION (F1_0*7)
#define BOSS_DEATH_DURATION (F1_0*6)

#define CHASE_TIME_LENGTH   (F1_0*8)
#define DEFAULT_ROBOT_SOUND_VOLUME F1_0

extern fix Dist_to_last_fired_upon_player_pos;
extern vms_vector Last_fired_upon_player_pos;
extern int Laser_rapid_fire;

#define MAX_AWARENESS_EVENTS 64
typedef struct awareness_event {
	short       segnum; // segment the event occurred in
	short       type;   // type of event, defines behavior
	vms_vector  pos;    // absolute 3 space location of event
} awareness_event;

#define AIS_MAX 8
#define AIE_MAX 4

#define ESCORT_GOAL_UNSPECIFIED -1

#define ESCORT_GOAL_UNSPECIFIED -1
#define ESCORT_GOAL_BLUE_KEY    1
#define ESCORT_GOAL_GOLD_KEY    2
#define ESCORT_GOAL_RED_KEY     3
#define ESCORT_GOAL_CONTROLCEN  4
#define ESCORT_GOAL_EXIT        5

// Custom escort goals.
#define ESCORT_GOAL_ENERGY      6
#define ESCORT_GOAL_ENERGYCEN   7
#define ESCORT_GOAL_SHIELD      8
#define ESCORT_GOAL_POWERUP     9
#define ESCORT_GOAL_ROBOT       10
#define ESCORT_GOAL_HOSTAGE     11
#define ESCORT_GOAL_PLAYER_SPEW 12
#define ESCORT_GOAL_SCRAM       13
#define ESCORT_GOAL_EXIT2       14
#define ESCORT_GOAL_BOSS        15
#define ESCORT_GOAL_MARKER1     16
#define ESCORT_GOAL_MARKER2     17
#define ESCORT_GOAL_MARKER3     18
#define ESCORT_GOAL_MARKER4     19
#define ESCORT_GOAL_MARKER5     20
#define ESCORT_GOAL_MARKER6     21
#define ESCORT_GOAL_MARKER7     22
#define ESCORT_GOAL_MARKER8     23
#define ESCORT_GOAL_MARKER9     24

#define MAX_ESCORT_GOALS        25

#define MAX_ESCORT_DISTANCE     (F1_0*80)
#define MIN_ESCORT_DISTANCE     (F1_0*40)

#define FUELCEN_CHECK           1000

extern fix Escort_last_path_created;
extern int Escort_goal_object, Escort_special_goal, Escort_goal_index;

#define GOAL_WIDTH 11

#define SNIPE_RETREAT_TIME  (F1_0*5)
#define SNIPE_ABORT_RETREAT_TIME (SNIPE_RETREAT_TIME/2) // Can abort a retreat with this amount of time left in retreat
#define SNIPE_ATTACK_TIME   (F1_0*10)
#define SNIPE_WAIT_TIME     (F1_0*5)
#define SNIPE_FIRE_TIME     (F1_0*2)

#define THIEF_PROBABILITY   16384   // 50% chance of stealing an item at each attempt
#define MAX_STOLEN_ITEMS    10      // Maximum number kept track of, will keep stealing, causes stolen weapons to be lost!

extern int   Max_escort_length;
extern int   Escort_kill_object;
extern ubyte Stolen_items[MAX_STOLEN_ITEMS];
extern fix   Escort_last_path_created;
extern int   Escort_goal_object, Escort_special_goal, Escort_goal_index;

extern void  create_buddy_bot(void);

extern int   Max_escort_length;

extern char  *Escort_goal_text[MAX_ESCORT_GOALS];

extern void  ai_multi_send_robot_position(int objnum, int force);

extern int   Flinch_scale;
extern int   Attack_scale;
extern byte  Mike_to_matt_xlate[];

// Amount of time since the current robot was last processed for things such as movement.
// It is not valid to use FrameTime because robots do not get moved every frame.

extern int   Num_boss_teleport_segs;
extern short Boss_teleport_segs[];
extern int   Num_boss_gate_segs;
extern short Boss_gate_segs[];


// --------- John: These variables must be saved as part of gamesave. ---------
extern int              Ai_initialized;
extern int              Overall_agitation;
extern ai_local         Ai_local_info[MAX_OBJECTS];
extern point_seg        Point_segs[MAX_POINT_SEGS];
extern point_seg        *Point_segs_free_ptr;
extern ai_cloak_info    Ai_cloak_info[MAX_AI_CLOAK_INFO];
extern fix              Boss_cloak_start_time;
extern fix              Boss_cloak_end_time;
extern fix              Last_teleport_time;
extern fix              Boss_teleport_interval;
extern fix              Boss_cloak_interval;        // Time between cloaks
extern fix              Boss_cloak_duration;
extern fix              Last_gate_time;
extern fix              Gate_interval;
extern fix              Boss_dying_start_time;
extern byte             Boss_dying, Boss_dying_sound_playing;
extern fix              Boss_hit_time;
// -- extern int              Boss_been_hit;
// ------ John: End of variables which must be saved as part of gamesave. -----

extern int  ai_evaded;

extern byte Super_boss_gate_list[];
#define MAX_GATE_INDEX  25

extern int  Ai_info_enabled;
extern int  Robot_firing_enabled;


// These globals are set by a call to find_vector_intersection, which is a slow routine,
// so we don't want to call it again (for this object) unless we have to.
extern vms_vector   Hit_pos;
extern int          Hit_type, Hit_seg;
extern fvi_info     Hit_data;

extern int              Num_awareness_events;
extern awareness_event  Awareness_events[MAX_AWARENESS_EVENTS];

extern vms_vector       Believed_player_pos;

#ifndef NDEBUG
// Index into this array with ailp->mode
extern char *mode_text[18];

// Index into this array with aip->behavior
extern char behavior_text[6][9];

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
extern char state_text[8][5];

extern int Do_ai_flag, Break_on_object;

extern void mprintf_animation_info(object *objp);

#endif //ifndef NDEBUG

extern int Stolen_item_index;   // Used in ai.c for controlling rate of Thief flare firing.

extern void ai_frame_animation(object *objp);
extern int do_silly_animation(object *objp);
extern int openable_doors_in_segment(int segnum);
extern void compute_vis_and_vec(object *objp, vms_vector *pos, ai_local *ailp, vms_vector *vec_to_player, int *player_visibility, robot_info *robptr, int *flag);
extern void do_firing_stuff(object *obj, int player_visibility, vms_vector *vec_to_player);
extern int maybe_ai_do_actual_firing_stuff(object *obj, ai_static *aip);
extern void ai_do_actual_firing_stuff(object *obj, ai_static *aip, ai_local *ailp, robot_info *robptr, vms_vector *vec_to_player, fix dist_to_player, vms_vector *gun_point, int player_visibility, int object_animates, int gun_num);
extern void do_super_boss_stuff(object *objp, fix dist_to_player, int player_visibility);
extern void do_boss_stuff(object *objp, int player_visibility);
// -- unused, 08/07/95 -- extern void ai_turn_randomly(vms_vector *vec_to_player, object *obj, fix rate, int previous_visibility);
extern void ai_move_relative_to_player(object *objp, ai_local *ailp, fix dist_to_player, vms_vector *vec_to_player, fix circle_distance, int evade_only, int player_visibility);
extern void move_away_from_player(object *objp, vms_vector *vec_to_player, int attack_type);
extern void move_towards_vector(object *objp, vms_vector *vec_goal, int dot_based);
extern void init_ai_frame(void);
extern void detect_escort_goal_accomplished(int index);
extern void set_escort_special_goal(int key);

extern void create_bfs_list(int start_seg, short bfs_list[], int *length, int max_segs);
extern void init_thief_for_level();


extern int Escort_goal_object;

extern int ai_save_state( FILE * fp );
extern int ai_restore_state( FILE * fp, int version );

extern int Buddy_objnum, Buddy_allowed_to_talk;

extern void start_robot_death_sequence(object *objp);
extern int do_any_robot_dying_frame(object *objp);
extern void buddy_message(char * format, ... );

#define SPECIAL_REACTOR_ROBOT   65
extern void special_reactor_stuff(void);

#endif /* _AI_H */
