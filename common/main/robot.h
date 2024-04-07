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

#include <ranges>
#include "fwd-robot.h"
#include "game.h"

#include "pack.h"
#include "aistruct.h"
#include "weapon_id.h"
#include "object.h"
#include "fwd-partial_range.h"
#include "d_array.h"
#include "digi.h"

namespace dcx {

enum class robot_animation_state : uint8_t
{
	rest,
	alert,
	fire,
	recoil,
	flinch,
};

enum class robot_id : uint8_t
{
	brain = 7,
	toaster = 10,
	baby_spider = 14,
	/* if DXX_BUILD_DESCENT_II */
	energy_bandit = 44,
	special_reactor = 65,
	/* endif */
	None = UINT8_MAX,
};

enum class boss_robot_id : uint8_t;

//describes the position of a certain joint
struct jointpos : prohibit_void_ptr<jointpos>
{
	uint16_t jointnum;
	vms_angvec angles;
};

//describes a list of joint positions
struct jointlist
{
	short n_joints;
	short offset;
};

struct d_level_shared_robot_joint_state {
	unsigned N_robot_joints;
};

}

#ifdef dsx
constexpr auto weapon_none = weapon_id_type::unspecified;

namespace dsx {

//  Robot information
struct robot_info : prohibit_void_ptr<robot_info>
{
	polygon_model_index model_num;                  // which polygon model?
	enumerated_array<vms_vector, MAX_GUNS, robot_gun_number>  gun_points;   // where each gun model is
	enumerated_array<uint8_t, MAX_GUNS, robot_gun_number>   gun_submodels;    // which submodel is each gun in?
	uint16_t score_value;						//	Score from this robot.
	vclip_index exp1_vclip_num;
	vclip_index exp2_vclip_num;
	short   exp1_sound_num;
	short   exp2_sound_num;
	weapon_id_type weapon_type;
	uint8_t   n_guns;         // how many different gun positions
	sbyte   contains_prob;  //  Probability that this instance will contain something in N/16
	contained_object_parameters contains;  //  Type of thing contained, robot or powerup, in bitmaps.tbl, !0=robot, 0=powerup
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

	enumerated_array<fix, NDL, Difficulty_level_type>     field_of_view, // compare this value with forward_vector.dot.vector_to_player, if field_of_view <, then robot can see player
		firing_wait,   //  time in seconds between shots
#if defined(DXX_BUILD_DESCENT_II)
		firing_wait2,  //  time in seconds between shots
#endif
		turn_time,     // time in seconds to rotate 360 degrees in a dimension
		max_speed,         //  maximum speed attainable by this robot
		circle_distance;   //  distance at which robot circles player

	enumerated_array<int8_t, NDL, Difficulty_level_type>   rapidfire_count,   //  number of shots fired rapidly
		evade_speed;       //  rate at which robot can evade shots, 0=none, 4=very fast
	sbyte   cloak_type;     //  0=never, 1=always, 2=except-when-firing
	sbyte   attack_type;    //  0=firing, 1=charge (like green guy)

	ubyte   see_sound;      //  sound robot makes when it first sees the player
	ubyte   attack_sound;   //  sound robot makes when it attacks the player
	ubyte   claw_sound;     //  sound robot makes as it claws you (attack_type should be 1)
	boss_robot_id boss_flag;      //  0 = not boss, 1 = boss.  Is that surprising?
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
	enumerated_array<enumerated_array<jointlist, N_ANIM_STATES, robot_animation_state>, MAX_GUNS + 1, robot_gun_number> anim_states;
	int     always_0xabcd;      // debugging
};

#if defined(DXX_BUILD_DESCENT_I)
static inline int robot_is_companion(const robot_info &)
{
	return 0;
}

static inline int robot_is_thief(const robot_info &)
{
	return 0;
}
#elif defined(DXX_BUILD_DESCENT_II)
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
struct d_level_shared_robot_info_state
{
	//how many kinds of robots
	unsigned N_robot_types;      // Number of robot types.  We used to assume this was the same as N_polygon_models.
	// Robot info for AI system, loaded from bitmaps.tbl.
	d_robot_info_array Robot_info;
};

#if defined(DXX_BUILD_DESCENT_II)
// returns ptr to escort robot, or NULL
imobjptridx_t find_escort(fvmobjptridx &vmobjptridx, const d_robot_info_array &Robot_info);
#endif

imobjptridx_t robot_create(const d_robot_info_array &Robot_info, robot_id id, vmsegptridx_t segnum, const vms_vector &pos, const vms_matrix *orient, fix size, ai_behavior behavior, const imsegidx_t hide_segment = segment_none);
void recreate_thief(const d_robot_info_array &Robot_info, robot_id thief_id);

// Drops objects contained in objp.
bool object_create_robot_egg(const d_robot_info_array &Robot_info, object_base &objp);
bool object_create_robot_egg(const d_robot_info_array &Robot_info, contained_object_type type, contained_object_id id, unsigned num, const vms_vector &init_vel, const vms_vector &pos, vmsegptridx_t segnum);

// Create a matcen robot
imobjptridx_t create_morph_robot(const d_robot_info_array &Robot_info, vmsegptridx_t segp, const vms_vector &object_pos, robot_id object_id);

// do whatever this thing does in a frame
void do_controlcen_frame(const d_robot_info_array &Robot_info, vmobjptridx_t obj);

window_event_result multi_message_input_sub(const d_robot_info_array &Robot_info, int key, control_info &Controls);

/* Robot joints can be customized by hxm files, which are per-level.
 */
struct d_level_shared_robot_joint_state : ::dcx::d_level_shared_robot_joint_state
{
	//Big array of joint positions.  All robots index into this array
	std::array<jointpos, MAX_ROBOT_JOINTS> Robot_joints;
};

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
//                          robot_animation_state::rest, robot_animation_state::alert, robot_animation_state::fire, robot_animation_state::recoil, robot_animation_state::flinch

//  On exit:
//      Returns number of joints in list.
//      jp_list_ptr is stuffed with a pointer to a static array of joint positions.  This pointer is valid forever.
std::ranges::subrange<const jointpos *> robot_get_anim_state(const d_robot_info_array &, const std::array<jointpos, MAX_ROBOT_JOINTS> &, robot_id robot_type, robot_gun_number gun_num, robot_animation_state state);

/*
 * reads n robot_info structs from a PHYSFS_File
 */
}
#endif

#if 0
void jointpos_write(PHYSFS_File *fp, const jointpos &jp);
#endif
#ifdef dsx
namespace dsx {

static inline void boss_link_see_sound(const d_robot_info_array &Robot_info, const vcobjptridx_t objp)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)Robot_info;
	constexpr unsigned soundnum = SOUND_BOSS_SHARE_SEE;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned soundnum = Robot_info[get_robot_id(objp)].see_sound;
#endif
	digi_link_sound_to_object2(soundnum, objp, 1, F1_0, sound_stack::allow_stacking, vm_distance{F1_0*512});	//	F1_0*512 means play twice as loud
}
}
#endif
