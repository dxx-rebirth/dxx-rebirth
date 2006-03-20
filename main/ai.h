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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/ai.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:41 $
 * 
 * Header file for AI system.
 * 
 * $Log: ai.h,v $
 * Revision 1.1.1.1  2006/03/17 19:41:41  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:05  donut
 * Import of d1x 1.37 source.
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

#include "object.h"

#define	PLAYER_AWARENESS_INITIAL_TIME		(3*F1_0)
#define	MAX_PATH_LENGTH						30			//	Maximum length of path in ai path following.
#define	MAX_DEPTH_TO_SEARCH_FOR_PLAYER	10
#define	BOSS_GATE_MATCEN_NUM					-1
#define	MAX_BOSS_TELEPORT_SEGS	100
#define	BOSS_ECLIP_NUM						53

#define	ROBOT_BRAIN	7
#define	ROBOT_BOSS1	17

extern fix Boss_cloak_start_time, Boss_cloak_end_time;
extern	int	Boss_hit_this_frame;
extern int	Num_boss_teleport_segs;
extern short	Boss_teleport_segs[MAX_BOSS_TELEPORT_SEGS];
extern fix	Last_teleport_time;
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
extern void init_ai_system(void);
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
extern int Robot_firing_enabled;

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

extern void HUD_init_message(char * format, ... );
#endif
