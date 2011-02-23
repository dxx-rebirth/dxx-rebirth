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
 * Header file for AI system.
 *
 */


#ifndef _AI_H
#define _AI_H

#include "object.h"

#define	PLAYER_AWARENESS_INITIAL_TIME		(3*F1_0)
#define	MAX_PATH_LENGTH						30			//	Maximum length of path in ai path following.
#define	MAX_DEPTH_TO_SEARCH_FOR_PLAYER	10
#define	BOSS_GATE_MATCEN_NUM					-1
#define	MAX_BOSS_TELEPORT_SEGS	100

#define	ROBOT_BRAIN	7
#define	ROBOT_BOSS1	17

extern fix64 Boss_cloak_start_time, Boss_cloak_end_time;
extern	int	Boss_hit_this_frame;
extern int	Num_boss_teleport_segs;
extern short	Boss_teleport_segs[MAX_BOSS_TELEPORT_SEGS];
extern fix64	Last_teleport_time;
extern fix 	Boss_cloak_duration;
extern int Boss_dying;

extern ai_local	Ai_local_info[MAX_OBJECTS];
extern vms_vector	Believed_player_pos;

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
extern void create_awareness_event(object *objp, int type);			// object *objp can create awareness of player, amount based on "type"
extern void do_ai_frame_all(void);
extern void reset_ai_states(object *objp);
extern int create_path_points(object *objp, int start_seg, int end_seg, point_seg *point_segs, short *num_points, int max_depth, int random_flag, int safety_flag, int avoid_seg);
extern void create_all_paths(void);
extern void create_path_to_station(object *objp, int max_length);
extern void ai_follow_path(object *objp, int player_visibility);
extern void ai_turn_towards_vector(vms_vector *vec_to_player, object *obj, fix rate);
extern void ai_turn_towards_vel_vec(object *objp, fix rate);
extern void init_ai_objects(void);
extern void do_ai_robot_hit(object *robot, int type);
extern void create_n_segment_path(object *objp, int path_length, int avoid_seg);
extern void create_n_segment_path_to_door(object *objp, int path_length, int avoid_seg);
extern void make_random_vector(vms_vector *vec);
extern void init_robots_for_level(void);
extern int ai_behavior_to_mode(int behavior);

//	max_length is maximum depth of path to create.
//	If -1, use default:	MAX_DEPTH_TO_SEARCH_FOR_PLAYER
extern void create_path_to_player(object *objp, int max_length, int safety_flag);
extern void attempt_to_resume_path(object *objp);

//	When a robot and a player collide, some robots attack!
extern void do_ai_robot_hit_attack(object *robot, object *player, vms_vector *collision_point);
extern void ai_open_doors_in_segment(object *robot);
extern int ai_door_is_openable(object *objp, segment *segp, int sidenum);
extern int player_is_visible_from_object(object *objp, vms_vector *pos, fix field_of_view, vms_vector *vec_to_player);
extern void ai_reset_all_paths(void);	//	Reset all paths.  Call at the start of a level.
extern int ai_multiplayer_awareness(object *objp, int awareness_level);

#ifndef NDEBUG
extern void force_dump_ai_objects_all(char *msg);
#else
#define force_dump_ai_objects_all(msg)
#endif

extern void start_boss_death_sequence(object *objp);
extern void ai_init_boss_for_ship(void);
extern int Boss_been_hit;
extern fix AI_proc_time;

extern int ai_save_state(PHYSFS_file * fp);
extern int ai_restore_state(PHYSFS_file *fp, int swap);

#endif
