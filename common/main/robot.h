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
 * Header for robot.c
 *
 */

#pragma once

#include "vecmat.h"
#include "game.h"

#ifdef __cplusplus
#include "pack.h"
#include "ai.h"
#include "aistruct.h"
#include "polyobj.h"
#include "weapon_id.h"
#include "object.h"

#define MAX_GUNS 8      //should be multiple of 4 for ubyte array

//Animation states
#define AS_REST         0
#define AS_ALERT        1
#define AS_FIRE         2
#define AS_RECOIL       3
#define AS_FLINCH       4
#define N_ANIM_STATES   5

#define RI_CLOAKED_ALWAYS           1

namespace dcx {

//describes the position of a certain joint
struct jointpos : prohibit_void_ptr<jointpos>
{
	short jointnum;
	vms_angvec angles;
} __pack__;

//describes a list of joint positions
struct jointlist
{
	short n_joints;
	short offset;
};

constexpr std::integral_constant<unsigned, 16> ROBOT_NAME_LENGTH{};
}

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
//robot info flags
#define RIF_BIG_RADIUS  1   //pad the radius to fix robots firing through walls
#define RIF_THIEF       2   //this guy steals!
#endif

//  Robot information
struct robot_info : prohibit_void_ptr<robot_info>
{
	int     model_num;                  // which polygon model?
	array<vms_vector, MAX_GUNS>  gun_points;   // where each gun model is
	array<uint8_t, MAX_GUNS>   gun_submodels;    // which submodel is each gun in?
	uint16_t score_value;						//	Score from this robot.
	short   exp1_vclip_num;
	short   exp1_sound_num;
	short   exp2_vclip_num;
	short   exp2_sound_num;
	weapon_id_type weapon_type;
	uint8_t   n_guns;         // how many different gun positions
	sbyte   contains_id;    //  ID of powerup this robot can contain.

	sbyte   contains_count; //  Max number of things this instance can contain.
	sbyte   contains_prob;  //  Probability that this instance will contain something in N/16
	sbyte   contains_type;  //  Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte   kamikaze;       //  !0 means commits suicide when hits you, strength thereof. 0 means no.

	sbyte   badass;         //  Dies with badass explosion, and strength thereof, 0 means NO.
	sbyte   energy_drain;   //  Points of energy drained at each collision.
	weapon_id_type   weapon_type2;   //  Secondary weapon number, -1 means none, otherwise gun #0 fires this weapon.
#endif
	fix     lighting;       // should this be here or with polygon model?
	fix     strength;       // Initial shields of robot

	fix     mass;           // how heavy is this thing?
	fix     drag;           // how much drag does it have?

	array<fix, NDL>     field_of_view, // compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
		firing_wait,   //  time in seconds between shots
#if defined(DXX_BUILD_DESCENT_II)
		firing_wait2,  //  time in seconds between shots
#endif
		turn_time,     // time in seconds to rotate 360 degrees in a dimension
		max_speed,         //  maximum speed attainable by this robot
		circle_distance;   //  distance at which robot circles player

	array<int8_t, NDL>   rapidfire_count,   //  number of shots fired rapidly
		evade_speed;       //  rate at which robot can evade shots, 0=none, 4=very fast
	sbyte   cloak_type;     //  0=never, 1=always, 2=except-when-firing
	sbyte   attack_type;    //  0=firing, 1=charge (like green guy)

	ubyte   see_sound;      //  sound robot makes when it first sees the player
	ubyte   attack_sound;   //  sound robot makes when it attacks the player
	ubyte   claw_sound;     //  sound robot makes as it claws you (attack_type should be 1)
	sbyte   boss_flag;      //  0 = not boss, 1 = boss.  Is that surprising?
#if defined(DXX_BUILD_DESCENT_II)
	ubyte   taunt_sound;    //  sound robot makes after you die

	sbyte   companion;      //  Companion robot, leads you to things.
	sbyte   smart_blobs;    //  how many smart blobs are emitted when this guy dies!
	sbyte   energy_blobs;   //  how many smart blobs are emitted when this guy gets hit by energy weapon!

	sbyte   thief;          //  !0 means this guy can steal when he collides with you!
	sbyte   pursuit;        //  !0 means pursues player after he goes around a corner.  4 = 4/2 pursue up to 4/2 seconds after becoming invisible if up to 4 segments away
	sbyte   lightcast;      //  Amount of light cast. 1 is default.  10 is very large.
	sbyte   death_roll;     //  0 = dies without death roll. !0 means does death roll, larger = faster and louder

	//boss_flag, companion, thief, & pursuit probably should also be bits in the flags byte.
	ubyte   flags;          // misc properties

	ubyte   deathroll_sound;    // if has deathroll, what sound?
	ubyte   glow;               // apply this light to robot itself. stored as 4:4 fixed-point
	ai_behavior behavior;           //  Default behavior.
	ubyte   aim;                //  255 = perfect, less = more likely to miss.  0 != random, would look stupid.  0=45 degree spread.  Specify in bitmaps.tbl in range 0.0..1.0
#endif
	//animation info
	array<array<jointlist, N_ANIM_STATES>, MAX_GUNS+1> anim_states;
	int     always_0xabcd;      // debugging
};
}

constexpr auto weapon_none = weapon_id_type::unspecified;

namespace dsx {
static inline imobjptridx_t robot_create(ubyte id, vmsegptridx_t segnum, const vms_vector &pos, const vms_matrix *orient, fix size, ai_behavior behavior, const imsegidx_t hide_segment = segment_none)
{
	auto objp = obj_create(OBJ_ROBOT, id, segnum, pos, orient, size, CT_AI, MT_PHYSICS, RT_POLYOBJ);
	if (objp)
		init_ai_object(objp, behavior, hide_segment);
	return objp;
}

#if defined(DXX_BUILD_DESCENT_I)
// maximum number of robot types
constexpr std::integral_constant<unsigned, 30> MAX_ROBOT_TYPES{};
constexpr std::integral_constant<unsigned, 600> MAX_ROBOT_JOINTS{};

static inline int robot_is_companion(const robot_info &)
{
	return 0;
}

static inline int robot_is_thief(const robot_info &)
{
	return 0;
}
#elif defined(DXX_BUILD_DESCENT_II)
// maximum number of robot types
constexpr std::integral_constant<unsigned, 85> MAX_ROBOT_TYPES{};
constexpr std::integral_constant<unsigned, 1600> MAX_ROBOT_JOINTS{};

static inline int robot_is_companion(const robot_info &robptr)
{
	return robptr.companion;
}

static inline int robot_is_thief(const robot_info &robptr)
{
	return robptr.thief;
}
#endif

//the array of robots types
extern array<robot_info, MAX_ROBOT_TYPES> Robot_info;     // Robot info for AI system, loaded from bitmaps.tbl.
#if DXX_USE_EDITOR
using robot_names_array = array<array<char, ROBOT_NAME_LENGTH>, MAX_ROBOT_TYPES>;
extern robot_names_array Robot_names;
#endif
extern array<jointpos, MAX_ROBOT_JOINTS> Robot_joints;
}

namespace dcx {
//how many kinds of robots
extern unsigned N_robot_types;      // Number of robot types.  We used to assume this was the same as N_polygon_models.

extern unsigned N_robot_joints;
}

namespace dsx {
//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector &gun_point, const object_base &obj, unsigned gun_num);

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
partial_range_t<const jointpos *> robot_get_anim_state(const array<robot_info, MAX_ROBOT_TYPES> &, const array<jointpos, MAX_ROBOT_JOINTS> &, unsigned robot_type, unsigned gun_num, unsigned state);

/*
 * reads n robot_info structs from a PHYSFS_File
 */
void robot_info_read(PHYSFS_File *fp, robot_info &r);
}
#endif

/*
 * reads n jointpos structs from a PHYSFS_File
 */
void jointpos_read(PHYSFS_File *fp, jointpos &jp);
#if 0
void jointpos_write(PHYSFS_File *fp, const jointpos &jp);
#endif
#ifdef dsx
namespace dsx {
void robot_set_angles(robot_info *r,polymodel *pm, array<array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES> &angs);
weapon_id_type get_robot_weapon(const robot_info &ri, const unsigned gun_num);
}

static inline void boss_link_see_sound(const vcobjptridx_t objp)
{
#if defined(DXX_BUILD_DESCENT_I)
	constexpr unsigned soundnum = SOUND_BOSS_SHARE_SEE;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned soundnum = Robot_info[get_robot_id(objp)].see_sound;
#endif
	digi_link_sound_to_object2(soundnum, objp, 1, F1_0, sound_stack::allow_stacking, vm_distance{F1_0*512});	//	F1_0*512 means play twice as loud
}
#endif

#endif
