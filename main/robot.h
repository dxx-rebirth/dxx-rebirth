/* $Id: robot.h,v 1.4 2003-10-04 03:14:47 btb Exp $ */
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
 * Header for robot.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:01:59  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/07  16:52:00  john
 * Fixed robots not moving without edtiro bug.
 *
 * Revision 2.0  1995/02/27  11:30:59  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.25  1994/11/30  14:02:44  mike
 * fields for see/attack/claw sounds.
 *
 * Revision 1.24  1994/10/27  15:55:41  adam
 * *** empty log message ***
 *
 * Revision 1.23  1994/10/20  15:17:03  mike
 * Add boss flag.
 *
 * Revision 1.22  1994/10/20  09:51:00  adam
 * *** empty log message ***
 *
 * Revision 1.21  1994/10/18  10:52:54  mike
 * Support robots lunging as an attack_type.
 *
 * Revision 1.20  1994/10/17  21:19:02  mike
 * robot cloaking.
 *
 * Revision 1.19  1994/09/27  00:03:39  mike
 * Add score_value to robot_info struct.
 *
 * Revision 1.18  1994/09/22  19:01:12  mike
 * Move NDL from here to game.h
 *
 * Revision 1.17  1994/09/22  15:46:55  mike
 * Add default contained objects for robots.
 *
 * Revision 1.16  1994/09/22  10:46:57  mike
 * Add difficulty levels.
 *
 * Revision 1.15  1994/09/15  16:34:16  mike
 * Change rapidfire_count to a byte, add evade_speed, dum1, dum2.
 *
 * Revision 1.14  1994/09/09  14:21:58  matt
 * Increased maximum number of games
 *
 * Revision 1.13  1994/08/25  18:12:13  matt
 * Made player's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.12  1994/08/23  16:37:24  mike
 * Add rapidfire_count to robot_info.
 *
 * Revision 1.11  1994/07/27  19:45:01  mike
 * Objects containing objects.
 *
 * Revision 1.10  1994/07/12  12:40:01  matt
 * Revamped physics system
 *
 * Revision 1.9  1994/06/21  12:17:12  mike
 * Add circle_distance to robot_info.
 *
 * Revision 1.8  1994/06/09  16:22:28  matt
 * Moved header for calc_gun_point() here, where it belongs
 *
 * Revision 1.7  1994/06/08  18:16:23  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 *
 * Revision 1.6  1994/06/03  11:38:09  john
 * Made robots get their strength for RobotInfo->strength, which
 * is read in from bitmaps.tbl
 *
 * Revision 1.5  1994/05/30  19:43:31  mike
 * Add voluminous comment for robot_get_anim_state.
 *
 * Revision 1.4  1994/05/30  00:03:18  matt
 * Got rid of robot render type, and generally cleaned up polygon model
 * render objects.
 *
 * Revision 1.3  1994/05/29  18:46:37  matt
 * Added stuff for getting robot animation info for different states
 *
 * Revision 1.2  1994/05/26  21:09:18  matt
 * Moved robot stuff out of polygon model and into robot_info struct
 * Made new file, robot.c, to deal with robots
 *
 * Revision 1.1  1994/05/26  18:02:12  matt
 * Initial revision
 *
 *
 */

#ifndef _ROBOT_H
#define _ROBOT_H

#include "vecmat.h"
#include "object.h"
#include "game.h"
#include "cfile.h"

#define MAX_GUNS 8      //should be multiple of 4 for ubyte array

//Animation states
#define AS_REST         0
#define AS_ALERT        1
#define AS_FIRE         2
#define AS_RECOIL       3
#define AS_FLINCH       4
#define N_ANIM_STATES   5

#define RI_CLOAKED_NEVER            0
#define RI_CLOAKED_ALWAYS           1
#define RI_CLOAKED_EXCEPT_FIRING    2

//describes the position of a certain joint
typedef struct jointpos {
	short jointnum;
	vms_angvec angles;
} __pack__ jointpos;

//describes a list of joint positions
typedef struct jointlist {
	short n_joints;
	short offset;
} jointlist;

//robot info flags
#define RIF_BIG_RADIUS  1   //pad the radius to fix robots firing through walls
#define RIF_THIEF       2   //this guy steals!

//  Robot information
typedef struct robot_info {
	int     model_num;                  // which polygon model?
	vms_vector  gun_points[MAX_GUNS];   // where each gun model is
	ubyte   gun_submodels[MAX_GUNS];    // which submodel is each gun in?

	short   exp1_vclip_num;
	short   exp1_sound_num;

	short   exp2_vclip_num;
	short   exp2_sound_num;

	sbyte   weapon_type;
	sbyte   weapon_type2;   //  Secondary weapon number, -1 means none, otherwise gun #0 fires this weapon.
	sbyte   n_guns;         // how many different gun positions
	sbyte   contains_id;    //  ID of powerup this robot can contain.

	sbyte   contains_count; //  Max number of things this instance can contain.
	sbyte   contains_prob;  //  Probability that this instance will contain something in N/16
	sbyte   contains_type;  //  Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
	sbyte   kamikaze;       //  !0 means commits suicide when hits you, strength thereof. 0 means no.

	short   score_value;    //  Score from this robot.
	sbyte   badass;         //  Dies with badass explosion, and strength thereof, 0 means NO.
	sbyte   energy_drain;   //  Points of energy drained at each collision.

	fix     lighting;       // should this be here or with polygon model?
	fix     strength;       // Initial shields of robot

	fix     mass;           // how heavy is this thing?
	fix     drag;           // how much drag does it have?

	fix     field_of_view[NDL]; // compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
	fix     firing_wait[NDL];   //  time in seconds between shots
	fix     firing_wait2[NDL];  //  time in seconds between shots
	fix     turn_time[NDL];     // time in seconds to rotate 360 degrees in a dimension
// -- unused, mk, 05/25/95  fix fire_power[NDL];    //  damage done by a hit from this robot
// -- unused, mk, 05/25/95  fix shield[NDL];        //  shield strength of this robot
	fix     max_speed[NDL];         //  maximum speed attainable by this robot
	fix     circle_distance[NDL];   //  distance at which robot circles player

	sbyte   rapidfire_count[NDL];   //  number of shots fired rapidly
	sbyte   evade_speed[NDL];       //  rate at which robot can evade shots, 0=none, 4=very fast
	sbyte   cloak_type;     //  0=never, 1=always, 2=except-when-firing
	sbyte   attack_type;    //  0=firing, 1=charge (like green guy)

	ubyte   see_sound;      //  sound robot makes when it first sees the player
	ubyte   attack_sound;   //  sound robot makes when it attacks the player
	ubyte   claw_sound;     //  sound robot makes as it claws you (attack_type should be 1)
	ubyte   taunt_sound;    //  sound robot makes after you die

	sbyte   boss_flag;      //  0 = not boss, 1 = boss.  Is that surprising?
	sbyte   companion;      //  Companion robot, leads you to things.
	sbyte   smart_blobs;    //  how many smart blobs are emitted when this guy dies!
	sbyte   energy_blobs;   //  how many smart blobs are emitted when this guy gets hit by energy weapon!

	sbyte   thief;          //  !0 means this guy can steal when he collides with you!
	sbyte   pursuit;        //  !0 means pursues player after he goes around a corner.  4 = 4/2 pursue up to 4/2 seconds after becoming invisible if up to 4 segments away
	sbyte   lightcast;      //  Amount of light cast. 1 is default.  10 is very large.
	sbyte   death_roll;     //  0 = dies without death roll. !0 means does death roll, larger = faster and louder

	//boss_flag, companion, thief, & pursuit probably should also be bits in the flags byte.
	ubyte   flags;          // misc properties
	ubyte   pad[3];         // alignment

	ubyte   deathroll_sound;    // if has deathroll, what sound?
	ubyte   glow;               // apply this light to robot itself. stored as 4:4 fixed-point
	ubyte   behavior;           //  Default behavior.
	ubyte   aim;                //  255 = perfect, less = more likely to miss.  0 != random, would look stupid.  0=45 degree spread.  Specify in bitmaps.tbl in range 0.0..1.0

	//animation info
	jointlist anim_states[MAX_GUNS+1][N_ANIM_STATES];

	int     always_0xabcd;      // debugging

} __pack__ robot_info;


#define MAX_ROBOT_TYPES 85      // maximum number of robot types

#define ROBOT_NAME_LENGTH   16
extern char Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//the array of robots types
extern robot_info Robot_info[];     // Robot info for AI system, loaded from bitmaps.tbl.

//how many kinds of robots
extern  int N_robot_types;      // Number of robot types.  We used to assume this was the same as N_polygon_models.

//test data for one robot
#define MAX_ROBOT_JOINTS 1600
extern jointpos Robot_joints[MAX_ROBOT_JOINTS];
extern int  N_robot_joints;

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector *gun_point,object *obj,int gun_num);
//void calc_gun_point(vms_vector *gun_point,int objnum,int gun_num);

//  Tells joint positions for a gun to be in a specified state.
//  A gun can have associated with it any number of joints.  In order to tell whether a gun is a certain
//  state (such as FIRE or ALERT), you should call this function and check the returned joint positions
//  against the robot's gun's joint positions.  This function should also be called to determine how to
//  move a gun into a desired position.
//  For now (May 30, 1994), it is assumed that guns will linearly interpolate from one joint position to another.
//  There is no ordering of joint movement, so it's impossible to guarantee that a strange starting position won't
//  cause a gun to move through a robot's body, for example.

//  Given:
//      jp_list_ptr     pointer to list of joint angles, on exit, this is pointing at a static array
//      robot_type      type of robot for which to get joint information.  A particular type, not an instance of a robot.
//      gun_num         gun number.  If in 0..Robot_info[robot_type].n_guns-1, then it is a gun, else it refers to non-animating parts of robot.
//      state           state about which to get information.  Legal states in range 0..N_ANIM_STATES-1, defined in robot.h, are:
//                          AS_REST, AS_ALERT, AS_FIRE, AS_RECOIL, AS_FLINCH

//  On exit:
//      Returns number of joints in list.
//      jp_list_ptr is stuffed with a pointer to a static array of joint positions.  This pointer is valid forever.
extern int robot_get_anim_state(jointpos **jp_list_ptr,int robot_type,int gun_num,int state);

#ifdef FAST_FILE_IO
#define robot_info_read_n(ri, n, fp) cfread(ri, sizeof(robot_info), n, fp)
#define jointpos_read_n(jp, n, fp) cfread(jp, sizeof(jointpos), n, fp)
#else
/*
 * reads n robot_info structs from a CFILE
 */
extern int robot_info_read_n(robot_info *ri, int n, CFILE *fp);

/*
 * reads n jointpos structs from a CFILE
 */
extern int jointpos_read_n(jointpos *jp, int n, CFILE *fp);
#endif

#endif
