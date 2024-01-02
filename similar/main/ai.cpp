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
 * Autonomous Individual movement.
 *
 */

#include <algorithm>
#include <cstdlib>
#include <stdio.h>
#include <time.h>
#include <optional>

#include "inferno.h"
#include "game.h"
#include "console.h"
#include "3d.h"

#include "object.h"
#include "dxxerror.h"
#include "ai.h"
#include "escort.h"
#include "laser.h"
#include "fvi.h"
#include "physfsx.h"
#include "physfs-serial.h"
#include "robot.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "sounds.h"
#include "gameseg.h"
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "gameseq.h"
#include "powerup.h"
#include "args.h"
#include "fuelcen.h"
#include "controls.h"
#include "kconfig.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include <utility>

using std::min;

#define	AI_TURN_SCALE	1

namespace dsx {

const object *Ai_last_missile_camera;

namespace {
static void init_boss_segments(const segment_array &segments, const object &boss_objnum, d_level_shared_boss_state::special_segment_array_t &a, int size_check, int one_wall_hack);
static void ai_multi_send_robot_position(object &objnum, int force);

/* Computing the robot gun point is moderately expensive.  Defer computing it
 * until needed, and store the computed value in a std::optional.  When the
 * optional is valueless, the gun point has not yet been computed.
 */
class robot_gun_point : std::optional<vms_vector>
{
	using base_type = std::optional<vms_vector>;
public:
	using base_type::has_value;
	const vms_vector &build(const robot_info &robptr, const object_base &obj, robot_gun_number gun_num);
};

/* If the gun point was previously computed, return a reference to the
 * previously computed value.
 *
 * If the gun point was not yet computed, compute it here, and return a
 * reference to the computed value.
 */
const vms_vector &robot_gun_point::build(const robot_info &robptr, const object_base &obj, const robot_gun_number gun_num)
{
	if (!has_value())
	{
		vms_vector gun_point;
		calc_gun_point(robptr, gun_point, obj, gun_num);
		emplace(gun_point);
	}
	return **this;
}

#if defined(DXX_BUILD_DESCENT_I)
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds

#elif defined(DXX_BUILD_DESCENT_II)
#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	(F1_0*40)

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		(F1_0*4)
#define	MAX_LEAD_DISTANCE	(F1_0*200)
#define	LEAD_RANGE			(F1_0/2)

/* Invalid robot ID numbers in Spew_bots must be at the end of their row.  The
 * corresponding count in Max_spew_bots will prevent the game from rolling a
 * random index that loads the invalid robot ID number.
 */
constexpr enumerated_array<std::array<robot_id, 3>, NUM_D2_BOSSES, boss_robot_index> Spew_bots{{{
	{{robot_id{38}, robot_id{40}, robot_id{UINT8_MAX}}},
	{{robot_id{37}, robot_id{UINT8_MAX}, robot_id{UINT8_MAX}}},
	{{robot_id{43}, robot_id{57}, robot_id{UINT8_MAX}}},
	{{robot_id{26}, robot_id{27}, robot_id{58}}},
	{{robot_id{59}, robot_id{58}, robot_id{54}}},
	{{robot_id{60}, robot_id{61}, robot_id{54}}},

	{{robot_id{69}, robot_id{29}, robot_id{24}}},
	{{robot_id{72}, robot_id{60}, robot_id{73}}} 
}}};

constexpr enumerated_array<uint8_t, NUM_D2_BOSSES, boss_robot_index> Max_spew_bots{{{2, 1, 2, 3, 3, 3, 3, 3}}};
static fix Dist_to_last_fired_upon_player_pos;
#endif

static bool robot_gun_number_valid(const robot_info &robptr, const robot_gun_number current_gun)
{
	return static_cast<uint8_t>(current_gun) < robptr.n_guns;
}

static robot_gun_number robot_advance_current_gun(const robot_info &robptr, const robot_gun_number gun)
{
	const uint8_t next_gun = 1 + static_cast<uint8_t>(gun);
	if (next_gun >= robptr.n_guns)
		return robot_gun_number::_0;
	return robot_gun_number{next_gun};
}

static robot_gun_number robot_advance_current_gun_prefer_second(const robot_info &robptr, const robot_gun_number gun)
{
	const uint8_t next_gun = 1 + static_cast<uint8_t>(gun);
	if (next_gun >= robptr.n_guns)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (robptr.n_guns != 1 && robptr.weapon_type2 != weapon_none)
			return robot_gun_number::_1;
		else
#endif
			return robot_gun_number::_0;
	}
	return robot_gun_number{next_gun};
}

}
}

namespace dcx {
namespace {
constexpr std::integral_constant<int, F1_0 * 8> CHASE_TIME_LENGTH{};
constexpr std::integral_constant<int, F0_5> Robot_sound_volume{};
enum {
	Flinch_scale = 4,
	Attack_scale = 24,
};
#define	ANIM_RATE		(F1_0/16)
#define	DELTA_ANG_SCALE	16

constexpr std::array<robot_animation_state, 8> Mike_to_matt_xlate{{
	robot_animation_state::rest, robot_animation_state::rest, robot_animation_state::alert, robot_animation_state::alert, robot_animation_state::flinch, robot_animation_state::fire, robot_animation_state::recoil, robot_animation_state::rest
}};

#define	OVERALL_AGITATION_MAX	100

#define		MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#define	BOSS_CLOAK_DURATION	Boss_cloak_duration
#define	BOSS_DEATH_DURATION	(F1_0*6)
//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use FrameTime because robots do not get moved every frame.

// ---------- John: These variables must be saved as part of gamesave. --------
static int             Overall_agitation;
static std::array<ai_cloak_info, MAX_AI_CLOAK_INFO>   Ai_cloak_info;

fix build_savegametime_from_gametime(const fix64 gametime, const fix64 t)
{
	const auto delta_time = t - gametime;
	if (delta_time < F1_0 * (-18000))
		return fix{F1_0 * (-18000)};
	return static_cast<fix>(delta_time);
}

}
point_seg_array_t       Point_segs;
point_seg_array_t::iterator       Point_segs_free_ptr;

// ------ John: End of variables which must be saved as part of gamesave. -----

//	0	mech
//	1	green claw
//	2	spider
//	3	josh
//	4	violet
//	5	cloak vulcan
//	6	cloak mech
//	7	brain
//	8	onearm
//	9	plasma
//	10	toaster
//	11	bird
//	12	missile bird
//	13	polyhedron
//	14	baby spider
//	15	mini boss
//	16	super mech
//	17	shareware boss
//	18	cloak-green	; note, gating in this guy benefits player, cloak objects
//	19	vulcan
//	20	toad
//	21	4-claw
//	22	quad-laser
// 23 super boss

// byte	Super_boss_gate_list[] = {0, 1, 2, 9, 11, 16, 18, 19, 21, 22, 0, 9, 9, 16, 16, 18, 19, 19, 22, 22};
constexpr std::array<robot_id, 21> Super_boss_gate_list{{
	robot_id{0},
	robot_id{1},
	robot_id{8},
	robot_id{9},
	robot_id{10},
	robot_id{11},
	robot_id{12},
	robot_id{15},
	robot_id{16},
	robot_id{18},
	robot_id{19},
	robot_id{20},
	robot_id{22},
	robot_id{0},
	robot_id{8},
	robot_id{11},
	robot_id{19},
	robot_id{20},
	robot_id{8},
	robot_id{20},
	robot_id{8},
}};

std::optional<ai_static_state> build_ai_state_from_untrusted(const uint8_t untrusted)
{
	switch (untrusted)
	{
		case static_cast<uint8_t>(ai_static_state::AIS_NONE):
		case static_cast<uint8_t>(ai_static_state::AIS_REST):
		case static_cast<uint8_t>(ai_static_state::AIS_SRCH):
		case static_cast<uint8_t>(ai_static_state::AIS_LOCK):
		case static_cast<uint8_t>(ai_static_state::AIS_FLIN):
		case static_cast<uint8_t>(ai_static_state::AIS_FIRE):
		case static_cast<uint8_t>(ai_static_state::AIS_RECO):
		case static_cast<uint8_t>(ai_static_state::AIS_ERR_):
			return ai_static_state{untrusted};
		default:
			return std::nullopt;
	}
}

}
#define	MAX_GATE_INDEX	(Super_boss_gate_list.size())

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {


// ------ John: End of variables which must be saved as part of gamesave. -----

vms_vector	Last_fired_upon_player_pos;


// -- ubyte Boss_cloaks[NUM_D2_BOSSES]              = {1,1,1,1,1,1};      // Set byte if this boss can cloak

constexpr boss_flags_t Boss_teleports{{{1,1,1,1,1,1, 1,1}}}; // Set byte if this boss can teleport
const boss_flags_t Boss_spew_more{{{0,1,0,0,0,0, 0,0}}}; // If set, 50% of time, spew two bots.
const boss_flags_t Boss_spews_bots_energy{{{1,1,0,1,0,1, 1,1}}}; // Set byte if boss spews bots when hit by energy weapon.
const boss_flags_t Boss_spews_bots_matter{{{0,0,1,1,1,1, 0,1}}}; // Set byte if boss spews bots when hit by matter weapon.
const boss_flags_t Boss_invulnerable_energy{{{0,0,1,1,0,0, 0,0}}}; // Set byte if boss is invulnerable to energy weapons.
const boss_flags_t Boss_invulnerable_matter{{{0,0,0,0,1,1, 1,0}}}; // Set byte if boss is invulnerable to matter weapons.
const boss_flags_t Boss_invulnerable_spot{{{0,0,0,0,0,1, 0,1}}}; // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

segnum_t             Believed_player_seg;
}
#endif

namespace dcx {

vms_vector      Believed_player_pos;

namespace {

struct robot_to_player_visibility_state
{
	vms_vector vec_to_player;
	player_visibility_state visibility;
	uint8_t initialized = 0;
};

struct awareness_t : enumerated_array<player_awareness_type_t, MAX_SEGMENTS, segnum_t>
{
};

static int ai_evaded;

// This is set by a call to find_vector_intersection, which is a slow routine,
// so we don't want to call it again (for this object) unless we have to.
static vms_vector  Hit_pos;

static bool silly_animation_angle(fixang vms_angvec::*const a, const vms_angvec &jp, const vms_angvec &pobjp, const int flinch_attack_scale, vms_angvec &goal_angles, vms_angvec &delta_angles)
{
	const fix delta_angle = jp.*a - pobjp.*a;
	if (!delta_angle)
		return false;
	goal_angles.*a = jp.*a;
	const fix delta_anim_rate = (delta_angle >= F1_0/2)
		? -ANIM_RATE
		: (delta_angle >= 0)
			? ANIM_RATE
			: (delta_angle >= -F1_0/2)
				? -ANIM_RATE
				: ANIM_RATE
	;
	const fix delta_2 = (flinch_attack_scale != 1)
		? delta_anim_rate * flinch_attack_scale
		: delta_anim_rate;
	delta_angles.*a = delta_2 / DELTA_ANG_SCALE;		// complete revolutions per second
	return true;
}

static void frame_animation_angle(fixang vms_angvec::*const a, const fix frametime, const vms_angvec &deltaang, const vms_angvec &goalang, vms_angvec &curang)
{
	fix delta_to_goal = goalang.*a - curang.*a;
	if (delta_to_goal > 32767)
		delta_to_goal = delta_to_goal - 65536;
	else if (delta_to_goal < -32767)
		delta_to_goal = 65536 + delta_to_goal;
	if (delta_to_goal)
	{
		// Due to deltaang.*a being usually small values and frametime getting smaller with higher FPS, the usual use of fixmul will have scaled_delta_angle result in 0 way too often and long, making the robot animation run circles around itself. So multiply deltaang by DELTA_ANG_SCALE when passed to fixmul.
		const fix scaled_delta_angle = fixmul(deltaang.*a * DELTA_ANG_SCALE, frametime); // fixmul(deltaang.*a, frametime) * DELTA_ANG_SCALE;
		auto &ca = curang.*a;
		if (abs(delta_to_goal) < abs(scaled_delta_angle))
			ca = goalang.*a;
		else
			ca += scaled_delta_angle;
	}
}

static void move_toward_vector_component_assign(fix vms_vector::*const p, const vms_vector &vec_goal, const fix frametime32, vms_vector &velocity)
{
	velocity.*p = (velocity.*p / 2) + fixmul(vec_goal.*p, frametime32);
}

static void move_toward_vector_component_add(fix vms_vector::*const p, const vms_vector &vec_goal, const fix frametime64, const fix difficulty_scale, vms_vector &velocity)
{
	velocity.*p += fixmul(vec_goal.*p, frametime64) * difficulty_scale / 4;
}

}

}

#define	AIS_MAX	8
#define	AIE_MAX	4

// Current state indicates where the robot current is, or has just done.
// Transition table between states for an AI object.
// First dimension is trigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
constexpr std::array<
	enumerated_array<
		enumerated_array<ai_static_state, AI_MAX_STATE, ai_static_state>,
		AI_MAX_STATE,
		ai_static_state
		>,
		4> Ai_transition_table = {{
	{{{
		// Event = AIE_FIRE, a nearby object fired
		// none     rest      srch      lock      flin      fire      reco        // CURRENT is rows, GOAL is columns
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}}, // none
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}}, // rest
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}}, // search
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}}, // lock
		{{{AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO}}}, // flinch
		{{{AIS_ERR_, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLIN, AIS_FIRE, AIS_RECO}}}, // fire
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}}},  // recoil
	}}},

	// Event = AIE_HITT, a nearby object was hit (or a wall was hit)
	{{{
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}}},
	}}},

	// Event = AIE_COLL, player collided with robot
	{{{
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_LOCK, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO}}},
		{{{AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}}},
	}}},

	// Event = AIE_HURT, player hurt robot (by firing at and hitting it)
	// Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{{{
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
		{{{AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}}},
	}}}
}};

namespace dsx {

weapon_id_type get_robot_weapon(const robot_info &ri, const robot_gun_number gun_num)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)gun_num;
#elif defined(DXX_BUILD_DESCENT_II)
	if (ri.weapon_type2 != weapon_none && gun_num == robot_gun_number::_0)
		return ri.weapon_type2;
#endif
	return ri.weapon_type;
}

namespace {

static int ready_to_fire_weapon1(const ai_local &ailp, fix threshold)
{
	return ailp.next_fire <= threshold;
}

static int ready_to_fire_weapon2(const robot_info &robptr, const ai_local &ailp, fix threshold)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)robptr;
	(void)ailp;
	(void)threshold;
	return 0;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robptr.weapon_type2 == weapon_none)
		return 0;
	return ailp.next_fire2 <= threshold;
#endif
}

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if next_fire <= 0 or next_fire2 <= 0.
static int ready_to_fire_any_weapon(const robot_info &robptr, const ai_local &ailp, fix threshold)
{
	return ready_to_fire_weapon1(ailp, threshold) || ready_to_fire_weapon2(robptr, ailp, threshold);
}

}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
ai_mode ai_behavior_to_mode(ai_behavior behavior)
{
	switch (behavior) {
		case ai_behavior::AIB_STILL:
			return ai_mode::AIM_STILL;
		case ai_behavior::AIB_NORMAL:
			return ai_mode::AIM_CHASE_OBJECT;
		case ai_behavior::AIB_RUN_FROM:
			return ai_mode::AIM_RUN_FROM_OBJECT;
		case ai_behavior::AIB_STATION:
			return ai_mode::AIM_STILL;
#if defined(DXX_BUILD_DESCENT_I)
		case ai_behavior::AIB_HIDE:
			return ai_mode::AIM_HIDE;
		case ai_behavior::AIB_FOLLOW_PATH:
			return ai_mode::AIM_FOLLOW_PATH;
#elif defined(DXX_BUILD_DESCENT_II)
		case ai_behavior::AIB_BEHIND:
			return ai_mode::AIM_BEHIND;
		case ai_behavior::AIB_SNIPE:
			return ai_mode::AIM_STILL;	//	Changed, 09/13/95, MK, snipers are still until they see you or are hit.
		case ai_behavior::AIB_FOLLOW:
			return ai_mode::AIM_FOLLOW_PATH;
#endif
		default:	Int3();	//	Contact Mike: Error, illegal behavior type
	}

	return ai_mode::AIM_STILL;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the player starts a new ship.
void ai_init_boss_for_ship(void)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	BossUniqueState.Boss_hit_time = -F1_0*10;
#endif
}

namespace {

static void boss_init_all_segments(const segment_array &Segments, const object &boss_objnum)
{
	auto &Boss_gate_segs = LevelSharedBossState.Gate_segs;
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	if (!Boss_teleport_segs.empty())
		return;	// already have boss segs

	init_boss_segments(Segments, boss_objnum, Boss_gate_segs, 0, 0);
	
	init_boss_segments(Segments, boss_objnum, Boss_teleport_segs, 1, 0);
#if defined(DXX_BUILD_DESCENT_II)
	if (Boss_teleport_segs.size() < 2)
		init_boss_segments(Segments, boss_objnum, Boss_teleport_segs, 1, 1);
#endif
}

}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void init_ai_object(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, ai_behavior behavior, const imsegidx_t hide_segment)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;
	ai_local		*const ailp = &aip->ail;

	*ailp = {};

	if (static_cast<unsigned>(behavior) == 0) {
		behavior = ai_behavior::AIB_NORMAL;
		aip->behavior = behavior;
	}

	//	mode is now set from the Robot dialog, so this should get overwritten.
	ailp->mode = ai_mode::AIM_STILL;

	ailp->previous_visibility = player_visibility_state::no_line_of_sight;

	{
		aip->behavior = behavior;
		ailp->mode = ai_behavior_to_mode(aip->behavior);
	}

	auto &robptr = Robot_info[get_robot_id(obj)];
#if defined(DXX_BUILD_DESCENT_II)
	if (robot_is_companion(robptr)) {
		auto &BuddyState = LevelUniqueObjectState.BuddyState;
		BuddyState.Buddy_objnum = objp;
		ailp->mode = ai_mode::AIM_GOTO_PLAYER;
	}

	if (robot_is_thief(robptr)) {
		aip->behavior = ai_behavior::AIB_SNIPE;
		ailp->mode = ai_mode::AIM_THIEF_WAIT;
	}

	if (robptr.attack_type) {
		aip->behavior = ai_behavior::AIB_NORMAL;
		ailp->mode = ai_behavior_to_mode(aip->behavior);
	}
#endif

	// This is astonishingly stupid!  This routine gets called by matcens! KILL KILL KILL!!! Point_segs_free_ptr = Point_segs;

	obj.mtype.phys_info.velocity = {};
	ailp->player_awareness_time = 0;
	ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
	aip->GOAL_STATE = AIS_SRCH;
	if constexpr (DXX_HAVE_POISON || static_cast<uint8_t>(robot_gun_number::_0) != 0)
		/* If memory poisoning is enabled, then CURRENT_GUN is undefined.  Set
		 * it to a reasonable value here, because nothing else will assign it
		 * before use.
		 *
		 * If memory poisoning is disabled, then obj_create initialized
		 * CURRENT_GUN to 0 as part of clearing the newly allocated structure.
		 * If the value that would be written by an explicit initialization is
		 * 0, then avoid actually writing it.
		 */
		aip->CURRENT_GUN = robot_gun_number::_0;
	aip->CURRENT_STATE = AIS_REST;
	ailp->time_player_seen = {GameTime64};
	ailp->next_misc_sound_time = {GameTime64};
	ailp->time_player_sound_attacked = {GameTime64};

	aip->hide_segment = hide_segment;
	ailp->goal_segment = hide_segment;
	aip->hide_index = -1;			// This means the path has not yet been created.
	aip->cur_path_index = 0;

	aip->SKIP_AI_COUNT = 0;

	if (robptr.cloak_type == RI_CLOAKED_ALWAYS)
		aip->CLOAKED = 1;
	else
		aip->CLOAKED = 0;

	obj.mtype.phys_info.flags |= (PF_BOUNCE | PF_TURNROLL);
	
	aip->REMOTE_OWNER = -1;

#if defined(DXX_BUILD_DESCENT_II)
	aip->dying_sound_playing = 0;
	aip->dying_start_time = 0;
#endif
	aip->danger_laser_num = object_none;

	if (robptr.boss_flag != boss_robot_id::None
#if DXX_USE_EDITOR
		&& !EditorWindow
#endif
		)
	{
		BossUniqueState = {};
		boss_init_all_segments(Segments, obj);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void init_ai_objects(const d_robot_info_array &Robot_info)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Boss_gate_segs = LevelSharedBossState.Gate_segs;
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	Point_segs_free_ptr = Point_segs.begin();
	Boss_gate_segs.clear();
	Boss_teleport_segs.clear();

	range_for (const auto &&o, vmobjptridx)
	{
		auto &obj = *o;
		if (obj.type == OBJ_ROBOT && obj.control_source == object::control_type::ai)
			init_ai_object(Robot_info, o, obj.ctype.ai_info.behavior, obj.ctype.ai_info.hide_segment);
	}

	BossUniqueState.Boss_dying_sound_playing = 0;
	BossUniqueState.Boss_dying = 0;

	const auto Difficulty_level = underlying_value(GameUniqueState.Difficulty_level);
#define D1_Boss_gate_interval	F1_0*5 - Difficulty_level*F1_0/2
#if defined(DXX_BUILD_DESCENT_I)
	GameUniqueState.Boss_gate_interval = D1_Boss_gate_interval;
#elif defined(DXX_BUILD_DESCENT_II)
	ai_do_cloak_stuff();

	init_buddy_for_level();

	if (EMULATING_D1)
	{
		LevelSharedBossState.Boss_cloak_interval = d_level_shared_boss_state::D1_Boss_cloak_interval::value;
		LevelSharedBossState.Boss_teleport_interval = d_level_shared_boss_state::D1_Boss_teleport_interval::value;
		GameUniqueState.Boss_gate_interval = D1_Boss_gate_interval;
	}
	else
	{
		GameUniqueState.Boss_gate_interval = F1_0*4 - Difficulty_level*i2f(2)/3;
		if (Current_level_num == Current_mission->last_level)
		{
		LevelSharedBossState.Boss_teleport_interval = F1_0*10;
		LevelSharedBossState.Boss_cloak_interval = F1_0*15;					//	Time between cloaks
		} else
		{
		LevelSharedBossState.Boss_teleport_interval = F1_0*7;
		LevelSharedBossState.Boss_cloak_interval = F1_0*10;					//	Time between cloaks
		}
	}
#endif
#undef D1_Boss_gate_interval
}

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector(const vms_vector &goal_vector, object_base &objp, fix rate)
{
	//	Not all robots can turn, eg, robot_id::special_reactor
	if (rate == 0)
		return;

	if (objp.type == OBJ_ROBOT && get_robot_id(objp) == robot_id::baby_spider)
	{
		physics_turn_towards_vector(goal_vector, objp, rate);
		return;
	}

	auto new_fvec = goal_vector;

	const fix dot = vm_vec_dot(goal_vector, objp.orient.fvec);

	if (dot < (F1_0 - FrameTime/2)) {
		fix	new_scale = fixdiv(FrameTime * AI_TURN_SCALE, rate);
		vm_vec_scale(new_fvec, new_scale);
		vm_vec_add2(new_fvec, objp.orient.fvec);
		auto mag = vm_vec_normalize_quick(new_fvec);
		if (mag < F1_0/256) {
			new_fvec = goal_vector;		//	if degenerate vector, go right to goal
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (const auto Seismic_tremor_magnitude = LevelUniqueSeismicState.Seismic_tremor_magnitude)
	{
		fix			scale;
		scale = fixdiv(2*Seismic_tremor_magnitude, Robot_info[get_robot_id(objp)].mass);
		vm_vec_scale_add2(new_fvec, make_random_vector(), scale);
	}
#endif

	vm_vector_2_matrix(objp.orient, new_fvec, nullptr, &objp.orient.rvec);
}

#if defined(DXX_BUILD_DESCENT_I)
namespace {
static void ai_turn_randomly(const vms_vector &vec_to_player, object_base &obj, const fix rate, const player_visibility_state previous_visibility)
{
	vms_vector	curvec;

	//	Random turning looks too stupid, so 1/4 of time, cheat.
	if (player_is_visible(previous_visibility))
                if (d_rand() > 0x7400) {
			ai_turn_towards_vector(vec_to_player, obj, rate);
			return;
		}
//--debug--     if (d_rand() > 0x6000)
//--debug-- 		Prevented_turns++;

	curvec = obj.mtype.phys_info.rotvel;

	curvec.y += F1_0/64;

	curvec.x += curvec.y/6;
	curvec.y += curvec.z/4;
	curvec.z += curvec.x/10;

	if (abs(curvec.x) > F1_0/8) curvec.x /= 4;
	if (abs(curvec.y) > F1_0/8) curvec.y /= 4;
	if (abs(curvec.z) > F1_0/8) curvec.z /= 4;

	obj.mtype.phys_info.rotvel = curvec;

}
}
#endif

//	Overall_agitation affects:
//		Widens field of view.  Field of view is in range 0..1 (specified in bitmaps.tbl as N/360 degrees).
//			Overall_agitation/128 subtracted from field of view, making robots see wider.
//		Increases distance to which robot will search to create path to player by Overall_agitation/8 segments.
//		Decreases wait between fire times by Overall_agitation/64 seconds.


// --------------------------------------------------------------------------------------------------------------------
//	Returns:
//		0		Player is not visible from object, obstruction or something.
//		1		Player is visible, but not in field of view.
//		2		Player is visible and in field of view.
//	Note: Uses Believed_player_pos as player's position for cloak effect.
//	NOTE: Will destructively modify *pos if *pos is outside the mine.
player_visibility_state player_is_visible_from_object(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, vms_vector &pos, const fix field_of_view, const vms_vector &vec_to_player)
{
	auto &obj = *objp;
	fix			dot;

#if defined(DXX_BUILD_DESCENT_II)
	//	Assume that robot's gun tip is in same segment as robot's center.
	if (obj.control_source == object::control_type::ai)
		obj.ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
#endif

	segnum_t startseg;
	if (pos.x != obj.pos.x || pos.y != obj.pos.y || pos.z != obj.pos.z)
	{
		auto &Segments = LevelSharedSegmentState.get_segments();
		const auto &&segnum = find_point_seg(LevelSharedSegmentState, pos, Segments.vcptridx(obj.segnum));
		if (segnum == segment_none) {
			startseg = obj.segnum;
			pos = obj.pos;
			auto &robptr = Robot_info[get_robot_id(obj)];
			move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
		} else
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (segnum != obj.segnum) {
				if (obj.control_source == object::control_type::ai)
					obj.ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
			}
#endif
			startseg = segnum;
		}
	} else
		startseg			= obj.segnum;
	fvi_info Hit_data;
	const auto Hit_type = find_vector_intersection(fvi_query{
		pos,
		Believed_player_pos,
		fvi_query::unused_ignore_obj_list,
		fvi_query::unused_LevelUniqueObjectState,
		fvi_query::unused_Robot_info,
		FQ_TRANSWALL, // -- Why were we checking objects? | FQ_CHECK_OBJS;		//what about trans walls???
		objp,
	}, startseg, F1_0 / 4, Hit_data);
	Hit_pos = Hit_data.hit_pnt;

	if (Hit_type == fvi_hit_type::None)
	{
		dot = vm_vec_dot(vec_to_player, obj.orient.fvec);
		if (dot > field_of_view - (Overall_agitation << 9)) {
			return player_visibility_state::visible_and_in_field_of_view;
		} else {
			return player_visibility_state::visible_not_in_field_of_view;
		}
	} else {
		return player_visibility_state::no_line_of_sight;
	}
}

namespace {
// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
static int do_silly_animation(const d_robot_info_array &Robot_info, object &objp)
{
	polyobj_info	*const pobj_info = &objp.rtype.pobj_info;
	auto &aip = objp.ctype.ai_info;
	int				at_goal;
	int				attack_type;
	int				flinch_attack_scale = 1;

	const auto robot_type{get_robot_id(objp)};
	auto &Robot_joints = LevelSharedRobotJointState.Robot_joints;
	auto &robptr = Robot_info[robot_type];
	attack_type = robptr.attack_type;
	const auto num_guns = robptr.n_guns;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goal_state, not GOAL_STATE.
	const auto robot_state = Mike_to_matt_xlate[aip.GOAL_STATE];
	// previous_robot_state = Mike_to_matt_xlate[aip->CURRENT_STATE];

	if (attack_type) // && ((robot_state == robot_animation_state::fire) || (robot_state == robot_animation_state::recoil)))
		flinch_attack_scale = Attack_scale;
	else if (robot_state == robot_animation_state::flinch || robot_state == robot_animation_state::recoil)
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	for (const uint8_t gun_num_idx : xrange(1u + num_guns))
	{
		const auto gun_num = robot_gun_number{gun_num_idx};
		auto &ail = aip.ail;
		for (auto &jr : robot_get_anim_state(Robot_info, Robot_joints, robot_type, gun_num, robot_state))
		{
			unsigned jointnum = jr.jointnum;
			auto &jp = jr.angles;
			vms_angvec	*pobjp = &pobj_info->anim_angles[jointnum];

			if (jointnum >= Polygon_models[objp.rtype.pobj_info.model_num].n_models) {
				Int3();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			auto &goal_angles = ail.goal_angles[jointnum];
			auto &delta_angles = ail.delta_angles[jointnum];
			const auto animate_p = silly_animation_angle(&vms_angvec::p, jp, *pobjp, flinch_attack_scale, goal_angles, delta_angles);
			const auto animate_b = silly_animation_angle(&vms_angvec::b, jp, *pobjp, flinch_attack_scale, goal_angles, delta_angles);
			const auto animate_h = silly_animation_angle(&vms_angvec::h, jp, *pobjp, flinch_attack_scale, goal_angles, delta_angles);
			if (gun_num == robot_gun_number::_0)
			{
				if (animate_p || animate_b || animate_h)
					at_goal = 0;
			}
		}

		if (at_goal) {
			//ai_static	*aip = &objp->ctype.ai_info;
			ail.achieved_state[gun_num] = ail.goal_state[gun_num];
			if (ail.achieved_state[gun_num] == AIS_RECO)
				ail.goal_state[gun_num] = AIS_FIRE;
			else if (ail.achieved_state[gun_num] == AIS_FLIN)
				ail.goal_state[gun_num] = AIS_LOCK;
		}
	}

	if (at_goal == 1) //num_guns)
		aip.CURRENT_STATE = aip.GOAL_STATE;

	return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-objects in an object towards their goals.
//	Current orientation of object is at:	pobj_info.anim_angles
//	Goal orientation of object is at:		ai_info.goal_angles
//	Delta orientation of object is at:		ai_info.delta_angles
static void ai_frame_animation(object &objp)
{
	int	joint;
	auto &pobj_info = objp.rtype.pobj_info;
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto num_joints = Polygon_models[pobj_info.model_num].n_models;
	const auto &ail = objp.ctype.ai_info.ail;
	for (joint=1; joint<num_joints; joint++) {
		auto &curang = pobj_info.anim_angles[joint];
		auto &goalang = ail.goal_angles[joint];
		auto &deltaang = ail.delta_angles[joint];

		const fix frametime = FrameTime;
		frame_animation_angle(&vms_angvec::p, frametime, deltaang, goalang, curang);
		frame_animation_angle(&vms_angvec::b, frametime, deltaang, goalang, curang);
		frame_animation_angle(&vms_angvec::h, frametime, deltaang, goalang, curang);
	}
}

// ----------------------------------------------------------------------------------
static void set_next_fire_time(object &objp, ai_local &ailp, const robot_info &robptr, const robot_gun_number gun_num)
{
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
#if defined(DXX_BUILD_DESCENT_I)
	(void)objp;
	(void)gun_num;
	ailp.rapidfire_count++;

	if (ailp.rapidfire_count < robptr.rapidfire_count[Difficulty_level]) {
		ailp.next_fire = min(F1_0/8, robptr.firing_wait[Difficulty_level]/2);
	} else {
		ailp.rapidfire_count = 0;
		ailp.next_fire = robptr.firing_wait[Difficulty_level];
	}
#elif defined(DXX_BUILD_DESCENT_II)
	//	For guys in snipe mode, they have a 50% shot of getting this shot in free.
	if (gun_num != robot_gun_number::_0 || robptr.weapon_type2 == weapon_none)
		if ((objp.ctype.ai_info.behavior != ai_behavior::AIB_SNIPE) || (d_rand() > 16384))
			ailp.rapidfire_count++;

	//	Old way, 10/15/95: Continuous rapidfire if rapidfire_count set.
// -- 	if (((robptr.weapon_type2 == -1) || (gun_num != 0)) && (ailp->rapidfire_count < robptr.rapidfire_count[Difficulty_level])) {
// -- 		ailp->next_fire = min(F1_0/8, robptr.firing_wait[Difficulty_level]/2);
// -- 	} else {
// -- 		if ((robptr.weapon_type2 == -1) || (gun_num != 0)) {
// -- 			ailp->rapidfire_count = 0;
// -- 			ailp->next_fire = robptr.firing_wait[Difficulty_level];
// -- 		} else
// -- 			ailp->next_fire2 = robptr.firing_wait2[Difficulty_level];
// -- 	}

	if ((gun_num != robot_gun_number::_0 || robptr.weapon_type2 == weapon_none) && ailp.rapidfire_count < robptr.rapidfire_count[Difficulty_level])
	{
		ailp.next_fire = min(F1_0/8, robptr.firing_wait[Difficulty_level]/2);
	} else {
		if (gun_num != robot_gun_number::_0 || robptr.weapon_type2 == weapon_none)
		{
			ailp.next_fire = robptr.firing_wait[Difficulty_level];
			if (ailp.rapidfire_count >= robptr.rapidfire_count[Difficulty_level])
				ailp.rapidfire_count = 0;
		} else
			ailp.next_fire2 = robptr.firing_wait2[Difficulty_level];
	}
#endif
}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void do_ai_robot_hit_attack(const d_robot_info_array &Robot_info, const vmobjptridx_t robot, const vmobjptridx_t playerobj, const vms_vector &collision_point)
{
	ai_local &ailp = robot->ctype.ai_info.ail;
	auto &robptr = Robot_info[get_robot_id(robot)];

//#ifndef NDEBUG
	if (cheats.robotfiringsuspended)
		return;
//#endif

	//	If player is dead, stop firing.
	object &plrobj = *playerobj;
	if (plrobj.type == OBJ_GHOST)
		return;

	if (robptr.attack_type == 1) {
		if (ready_to_fire_weapon1(ailp, 0)) {
			auto &player_info = plrobj.ctype.player_info;
			if (!(player_info.powerup_flags & PLAYER_FLAGS_CLOAKED))
				if (vm_vec_dist_quick(plrobj.pos, robot->pos) < robot->size + plrobj.size + F1_0*2)
				{
					collide_player_and_nasty_robot(Robot_info, playerobj, robot, collision_point);
#if defined(DXX_BUILD_DESCENT_II)
					auto &energy = player_info.energy;
					if (robptr.energy_drain && energy) {
						energy -= robptr.energy_drain * F1_0;
						if (energy < 0)
							energy = 0;
					}
#endif
				}

			robot->ctype.ai_info.GOAL_STATE = AIS_RECO;
			set_next_fire_time(robot, ailp, robptr, robot_gun_number::_1);	//	1 = gun_num: 0 is special (uses next_fire2)
		}
	}
}

namespace {
#if defined(DXX_BUILD_DESCENT_II)
// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit player given positions, player vel, elapsed time
static fix compute_lead_component(fix player_pos, fix robot_pos, fix player_vel, fix elapsed_time)
{
	return fixdiv(player_pos - robot_pos, elapsed_time) + player_vel;
}

static void compute_lead_component(fix vms_vector::*const m, vms_vector &out, const vms_vector &believed_player_pos, const vms_vector &fire_point, const vms_vector &velocity, const fix projected_time)
{
	out.*m = compute_lead_component(believed_player_pos.*m, fire_point.*m, velocity.*m, projected_time);
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the player, returning point to fire at in fire_point.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot(vector_to_player, player_direction) must be in -LEAD_RANGE..LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
static int lead_player(const object_base &objp, const vms_vector &fire_point, const vms_vector &believed_player_pos, const robot_gun_number gun_num, vms_vector &fire_vec)
{
	const auto &plrobj = *ConsoleObject;
	if (plrobj.ctype.player_info.powerup_flags & PLAYER_FLAGS_CLOAKED)
		return 0;

	const auto &velocity = plrobj.mtype.phys_info.velocity;
	const auto &&[player_speed, player_movement_dir] = vm_vec_normalize_quick_with_magnitude(velocity);

	if (player_speed < MIN_LEAD_SPEED)
		return 0;

	const auto &&[dist_to_player, vec_to_player] = vm_vec_normalize_quick_with_magnitude(vm_vec_sub(believed_player_pos, fire_point));
	if (dist_to_player > MAX_LEAD_DISTANCE)
		return 0;

	const fix dot = vm_vec_dot(vec_to_player, player_movement_dir);

	if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
		return 0;

	//	Looks like it might be worth trying to lead the player.
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	const auto weapon_type = get_robot_weapon(Robot_info[get_robot_id(objp)], gun_num);

	const weapon_info *const wptr = &Weapon_info[weapon_type];
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	fix max_weapon_speed = wptr->speed[Difficulty_level];
	if (max_weapon_speed < F1_0)
		return 0;

	//	Matter weapons:
	//	At Rookie or Trainee, don't lead at all.
	//	At higher skill levels, don't lead as well.  Accomplish this by screwing up max_weapon_speed.
	if (wptr->matter != weapon_info::matter_flag::energy)
	{
		switch (Difficulty_level)
		{
			case Difficulty_level_type::_0:
			case Difficulty_level_type::_1:
				return 0;
			case Difficulty_level_type::_2:
			case Difficulty_level_type::_3:
				max_weapon_speed *= NDL - underlying_value(Difficulty_level);
				break;
			case Difficulty_level_type::_4:
				break;
		}
	}

	const fix projected_time = fixdiv(dist_to_player, max_weapon_speed);

	compute_lead_component(&vms_vector::x, fire_vec, believed_player_pos, fire_point, velocity, projected_time);
	compute_lead_component(&vms_vector::y, fire_vec, believed_player_pos, fire_point, velocity, projected_time);
	compute_lead_component(&vms_vector::z, fire_vec, believed_player_pos, fire_point, velocity, projected_time);

	vm_vec_normalize_quick(fire_vec);

	Assert(vm_vec_dot(fire_vec, objp.orient.fvec) < 3*F1_0/2);

	//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
	if (vm_vec_dot(fire_vec, objp.orient.fvec) < F1_0/2) {
		vm_vec_add2(fire_vec, vec_to_player);
		vm_vec_scale(fire_vec, F1_0/2);
		if (vm_vec_dot(fire_vec, objp.orient.fvec) < F1_0/2) {
			return 0;
		}
	}

	return 1;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
static void ai_fire_laser_at_player(const d_robot_info_array &Robot_info, const d_level_shared_segment_state &LevelSharedSegmentState, const vmobjptridx_t obj, const player_info &player_info, robot_gun_point &gun_point, const robot_gun_number gun_num
#if defined(DXX_BUILD_DESCENT_II)
									, const vms_vector &believed_player_pos
#endif
									)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	const auto Difficulty_level = underlying_value(GameUniqueState.Difficulty_level);
	const auto powerup_flags = player_info.powerup_flags;
	ai_local &ailp = obj->ctype.ai_info.ail;
	auto &robptr = Robot_info[get_robot_id(obj)];
	vms_vector	fire_vec;

	Assert(robptr.attack_type == 0);	//	We should never be coming here for the green guy, as he has no laser!

	if (cheats.robotfiringsuspended)
		return;

	if (obj->control_source == object::control_type::morph)
		return;

	//	If player is exploded, stop firing.
	if (Player_dead_state == player_dead_state::exploded)
		return;

#if defined(DXX_BUILD_DESCENT_II)
	//	If this robot is only awake because a camera woke it up, don't fire.
	if (obj->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
		return;

	if (obj->ctype.ai_info.dying_start_time)
		return;		//	No firing while in death roll.

	//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
	//	If you try to key the boss off obj->ctype.ai_info.dying_start_time, it will hose the endlevel stuff.
	if (BossUniqueState.Boss_dying_start_time && robptr.boss_flag != boss_robot_id::None)
		return;
#endif

	//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (powerup_flags & PLAYER_FLAGS_CLOAKED) {
		fix64	cloak_time = Ai_cloak_info[static_cast<imobjptridx_t::index_type>(obj) % MAX_AI_CLOAK_INFO].last_time;

		if (GameTime64 - cloak_time > CLOAK_TIME_MAX/4)
			if (d_rand() > fixdiv(GameTime64 - cloak_time, CLOAK_TIME_MAX)/2) {
				set_next_fire_time(obj, ailp, robptr, gun_num);
				return;
			}
	}

	auto &fire_point = gun_point.build(robptr, obj, gun_num);
#if defined(DXX_BUILD_DESCENT_I)
	(void)LevelSharedSegmentState;
	//	Set position to fire at based on difficulty level.
	vms_vector bpp_diff = Believed_player_pos;
	const auto m = (NDL - Difficulty_level - 1) * 4;
	bpp_diff.x += (d_rand() - 16384) * m;
	bpp_diff.y += (d_rand() - 16384) * m;
	bpp_diff.z += (d_rand() - 16384) * m;

	//	Half the time fire at the player, half the time lead the player.
        if (d_rand() > 16384) {

		vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);

	} else {
		// If player is not moving, fire right at him!
		//	Note: If the robot fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the robot; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the player.  For now, just fire right at the player.
		{
			vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);
		// Player is moving.  Determine where the player will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the player.
		//	So, that's why we write games, instead of guiding missiles...
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	//	Handle problem of a robot firing through a wall because its gun tip is on the other
	//	side of the wall than the robot's center.  For speed reasons, we normally only compute
	//	the vector from the gun point to the player.  But we need to know whether the gun point
	//	is separated from the robot's center by a wall.  If so, don't fire!
	if (obj->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
		//	Well, the gun point is in a different segment than the robot's center.
		//	This is almost always ok, but it is not ok if something solid is in between.
		//	See if these segments are connected, which should almost always be the case.
		auto &Segments = LevelSharedSegmentState.get_segments();
		const auto &&csegp = Segments.vcptridx(obj->segnum);
		const auto &&gun_segnum = find_point_seg(LevelSharedSegmentState, fire_point, csegp);
		const auto conn_side = find_connect_side(gun_segnum, csegp);
		if (conn_side != side_none)
		{
			//	They are connected via conn_side in segment obj->segnum.
			//	See if they are unobstructed.
			if (!(WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, csegp, conn_side) & WALL_IS_DOORWAY_FLAG::fly))
			{
				//	Can't fly through, so don't let this bot fire through!
				return;
			}
		} else {
			//	Well, they are not directly connected, so use find_vector_intersection to see if they are unobstructed.
			fvi_info		hit_data;

			const auto fate = find_vector_intersection(fvi_query{
				obj->pos,
				fire_point,
				fvi_query::unused_ignore_obj_list,
				fvi_query::unused_LevelUniqueObjectState,
				fvi_query::unused_Robot_info,
				FQ_TRANSWALL,
				obj,
			}, obj->segnum, 0, hit_data);
			if (fate != fvi_hit_type::None)
			{
				Int3();		//	This bot's gun is poking through a wall, so don't fire.
				move_towards_segment_center(robptr, LevelSharedSegmentState, obj);		//	And decrease chances it will happen again.
				return;
			}
		}
	}

	//	Set position to fire at based on difficulty level and robot's aiming ability
	fix aim = FIRE_K*F1_0 - (FIRE_K-1)*(robptr.aim << 8);	//	F1_0 in bitmaps.tbl = same as used to be.  Worst is 50% more error.

	//	Robots aim more poorly during seismic disturbance.
	if (const auto Seismic_tremor_magnitude = LevelUniqueSeismicState.Seismic_tremor_magnitude)
	{
		aim = fixmul(aim, std::max(F1_0 - abs(Seismic_tremor_magnitude), F1_0 / 2));
	}

	//	Lead the player half the time.
	//	Note that when leading the player, aim is perfect.  This is probably acceptable since leading is so hacked in.
	//	Problem is all robots will lead equally badly.
	if (d_rand() < 16384) {
		if (lead_player(obj, fire_point, believed_player_pos, gun_num, fire_vec))		//	Stuff direction to fire at in fire_point.
			goto player_led;
	}

	{
	fix dot = 0;
	unsigned count = 0;			//	Don't want to sit in this loop forever...
	const auto m = (NDL - Difficulty_level - 1) * 4;
	while ((count < 4) && (dot < F1_0/4)) {
		vms_vector bpp_diff = believed_player_pos;
		bpp_diff.x += fixmul((d_rand() - 16384) * m, aim);
		bpp_diff.y += fixmul((d_rand() - 16384) * m, aim);
		bpp_diff.z += fixmul((d_rand() - 16384) * m, aim);

		vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);
		dot = vm_vec_dot(obj->orient.fvec, fire_vec);
		count++;
	}
	}
player_led: ;
#endif

	const auto weapon_type = get_robot_weapon(robptr, gun_num);

	Laser_create_new_easy(Robot_info, fire_vec, fire_point, obj, weapon_type, weapon_sound_flag::audible);

	if (Game_mode & GM_MULTI)
	{
		ai_multi_send_robot_position(obj, -1);
		multi_send_robot_fire(obj, obj->ctype.ai_info.CURRENT_GUN, fire_vec);
	}

	create_awareness_event(obj, player_awareness_type_t::PA_NEARBY_ROBOT_FIRED, LevelUniqueRobotAwarenessState);

	set_next_fire_time(obj, ailp, robptr, gun_num);

	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (robptr.boss_flag == boss_robot_id::d1_1 || robptr.boss_flag == boss_robot_id::d1_superboss)
		BossUniqueState.Last_teleport_time -= LevelSharedBossState.Boss_teleport_interval / 2;
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
//	if dot_based set, then speed is based on direction of movement relative to heading
static void move_towards_vector(const robot_info &robptr, object_base &objp, const vms_vector &vec_goal, int dot_based)
{
	auto &velocity = objp.mtype.phys_info.velocity;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards player as usual.

	const auto vel = vm_vec_normalized_quick(velocity);
	fix dot = vm_vec_dot(vel, objp.orient.fvec);

#if defined(DXX_BUILD_DESCENT_I)
	dot_based = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robot_is_thief(robptr))
		dot = (F1_0+dot)/2;
#endif

	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	if (dot_based && (dot < 3*F1_0/4)) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		const fix frametime32 = FrameTime * 32;
		move_toward_vector_component_assign(&vms_vector::x, vec_goal, frametime32, velocity);
		move_toward_vector_component_assign(&vms_vector::y, vec_goal, frametime32, velocity);
		move_toward_vector_component_assign(&vms_vector::z, vec_goal, frametime32, velocity);
	} else {
		const fix frametime64 = FrameTime * 64;
		const fix difficulty_scale = underlying_value(Difficulty_level) + 5;
		move_toward_vector_component_add(&vms_vector::x, vec_goal, frametime64, difficulty_scale, velocity);
		move_toward_vector_component_add(&vms_vector::y, vec_goal, frametime64, difficulty_scale, velocity);
		move_toward_vector_component_add(&vms_vector::z, vec_goal, frametime64, difficulty_scale, velocity);
	}

	const auto speed = vm_vec_mag_quick(velocity);
	fix max_speed = robptr.max_speed[Difficulty_level];

	//	Green guy attacks twice as fast as he moves away.
#if defined(DXX_BUILD_DESCENT_I)
	if (robptr.attack_type == 1)
#elif defined(DXX_BUILD_DESCENT_II)
	if (robptr.attack_type == 1 || robot_is_thief(robptr) || robptr.kamikaze)
#endif
		max_speed *= 2;

	if (speed > max_speed) {
		velocity.x = (velocity.x * 3) / 4;
		velocity.y = (velocity.y * 3) / 4;
		velocity.z = (velocity.z * 3) / 4;
	}
}

}

// --------------------------------------------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static
#endif
void move_towards_player(const d_robot_info_array &Robot_info, object &objp, const vms_vector &vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	auto &robptr = Robot_info[get_robot_id(objp)];
	move_towards_vector(robptr, objp, vec_to_player, 1);
}

namespace {

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means normal slide about.  fast_flag = 0 means no evasion.
static void move_around_player(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, const player_flags powerup_flags, const vms_vector &vec_to_player, int fast_flag)
{
	physics_info	*pptr = &objp->mtype.phys_info;
	vms_vector		evade_vector;

	if (fast_flag == 0)
		return;

	const unsigned dir = ((objp) ^ ((d_tick_count + 3*(objp)) >> 5)) & 3;
	switch (dir) {
		case 0:
			evade_vector.x = vec_to_player.z;
			evade_vector.y = vec_to_player.y;
			evade_vector.z = -vec_to_player.x;
			break;
		case 1:
			evade_vector.x = -vec_to_player.z;
			evade_vector.y = vec_to_player.y;
			evade_vector.z = vec_to_player.x;
			break;
		case 2:
			evade_vector.x = -vec_to_player.y;
			evade_vector.y = vec_to_player.x;
			evade_vector.z = vec_to_player.z;
			break;
		case 3:
			evade_vector.x = vec_to_player.y;
			evade_vector.y = -vec_to_player.x;
			evade_vector.z = vec_to_player.z;
			break;
		default:
			throw std::runtime_error("move_around_player: bad case");
	}
	const auto frametime32 = FrameTime * 32;
	evade_vector.x = fixmul(evade_vector.x, frametime32);
	evade_vector.y = fixmul(evade_vector.y, frametime32);
	evade_vector.z = fixmul(evade_vector.z, frametime32);

	auto &robptr = Robot_info[get_robot_id(objp)];
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = vm_vec_dot(vec_to_player, objp->orient.fvec);
		if (dot > robptr.field_of_view[Difficulty_level] && !(powerup_flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (!robptr.strength)
				damage_scale = F1_0;
			else
				damage_scale = fixdiv(objp->shields, robptr.strength);
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in case...
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in case...

			vm_vec_scale(evade_vector, i2f(fast_flag) + damage_scale);
		}
	}

	pptr->velocity.x += evade_vector.x;
	pptr->velocity.y += evade_vector.y;
	pptr->velocity.z += evade_vector.z;

	const auto speed = vm_vec_mag_quick(pptr->velocity);
	if (speed > robptr.max_speed[Difficulty_level]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
static void move_away_from_player(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, const vms_vector &vec_to_player, int attack_type)
{
	physics_info	*pptr = &objp->mtype.phys_info;

	const auto frametime = FrameTime;
	pptr->velocity.x -= fixmul(vec_to_player.x, frametime*16);
	pptr->velocity.y -= fixmul(vec_to_player.y, frametime*16);
	pptr->velocity.z -= fixmul(vec_to_player.z, frametime*16);

	if (attack_type) {
		//	Get value in 0..3 to choose evasion direction.
		const int objref = objp ^ ((d_tick_count + 3 * objp) >> 5);
		vm_vec_scale_add2(pptr->velocity, (objref & 2) ? objp->orient.rvec : objp->orient.uvec, ((objref & 1) ? -frametime : frametime) << 5);
	}


	auto speed = vm_vec_mag_quick(pptr->velocity);

	auto &robptr = Robot_info[get_robot_id(objp)];
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	if (speed > robptr.max_speed[Difficulty_level])
	{
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_STILL).
static void ai_move_relative_to_player(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, ai_local &ailp, const fix dist_to_player, const fix circle_distance, const int evade_only, const robot_to_player_visibility_state &player_visibility, const player_info &player_info)
{
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	auto &vec_to_player = player_visibility.vec_to_player;
	auto &robptr = Robot_info[get_robot_id(objp)];

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((robptr.attack_type == 0) && (objp->ctype.ai_info.danger_laser_num != -1))
	if (objp->ctype.ai_info.danger_laser_num != object_none) {
		const auto &&dobjp = objp.absolute_sibling(objp->ctype.ai_info.danger_laser_num);

		if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == objp->ctype.ai_info.danger_laser_signature)) {
			vms_vector	laser_fvec;

			const fix field_of_view = robptr.field_of_view[Difficulty_level];

			const auto &&[dist_to_laser, vec_to_laser] = vm_vec_normalize_quick_with_magnitude(vm_vec_sub(dobjp->pos, objp->pos));
			const fix dot = vm_vec_dot(vec_to_laser, objp->orient.fvec);

			if (dot > field_of_view || robot_is_companion(robptr))
			{
				fix			laser_robot_dot;

				//	The laser is seen by the robot, see if it might hit the robot.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (dobjp->render_type == render_type::RT_POLYOBJ)
					laser_fvec = dobjp->orient.fvec;
				else {		//	Not a polyobj, get velocity and normalize.
					laser_fvec = vm_vec_normalized_quick(dobjp->mtype.phys_info.velocity);	//dobjp->orient.fvec;
				}
				const auto laser_vec_to_robot = vm_vec_normalized_quick(vm_vec_sub(objp->pos, dobjp->pos));
				laser_robot_dot = vm_vec_dot(laser_fvec, laser_vec_to_robot);

				if ((laser_robot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
					int	evade_speed;

					ai_evaded = 1;
					evade_speed = robptr.evade_speed[Difficulty_level];
					move_around_player(Robot_info, objp, player_info.powerup_flags, vec_to_player, evade_speed);
				}
			}
			return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if (!robptr.attack_type && !robot_is_thief(robptr) && evade_only)
		return;

	//	If we fall out of above, then no object to be avoided.
	objp->ctype.ai_info.danger_laser_num = object_none;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (robptr.attack_type == 1) {
		if ((!ready_to_fire_weapon1(ailp, robptr.firing_wait[Difficulty_level]/4) && dist_to_player < F1_0*30) ||
			Player_dead_state != player_dead_state::no)
		{
			//	1/4 of time, move around player, 3/4 of time, move away from player
			if (d_rand() < 8192) {
				move_around_player(Robot_info, objp, player_info.powerup_flags, vec_to_player, -1);
			} else {
				move_away_from_player(Robot_info, objp, vec_to_player, 1);
			}
		} else {
			move_towards_player(Robot_info, objp, vec_to_player);
		}
	}
	else if (robot_is_thief(robptr))
	{
		move_towards_player(Robot_info, objp, vec_to_player);
	}
	else {
#if defined(DXX_BUILD_DESCENT_I)
		if (dist_to_player < circle_distance)
			move_away_from_player(Robot_info, objp, vec_to_player, 0);
		else if (dist_to_player < circle_distance*2)
			move_around_player(Robot_info, objp, player_info.powerup_flags, vec_to_player, -1);
		else
			move_towards_player(Robot_info, objp, vec_to_player);
#elif defined(DXX_BUILD_DESCENT_II)
		int	objval = ((objp) & 0x0f) ^ 0x0a;

		//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
		if (robptr.kamikaze) {
			move_towards_player(Robot_info, objp, vec_to_player);
		} else if (dist_to_player < circle_distance)
			move_away_from_player(Robot_info, objp, vec_to_player, 0);
		else if ((dist_to_player < (3+objval)*circle_distance/2) && !ready_to_fire_weapon1(ailp, -F1_0)) {
			move_around_player(Robot_info, objp, player_info.powerup_flags, vec_to_player, -1);
		} else {
			if (ready_to_fire_weapon1(ailp, -(F1_0 + (objval << 12))) && player_is_visible(player_visibility.visibility))
			{
				//	Usually move away, but sometimes move around player.
				if ((((GameTime64 >> 18) & 0x0f) ^ objval) > 4) {
					move_away_from_player(Robot_info, objp, vec_to_player, 0);
				} else {
					move_around_player(Robot_info, objp, player_info.powerup_flags, vec_to_player, -1);
				}
			} else
				move_towards_player(Robot_info, objp, vec_to_player);
		}
#endif
	}
}

}

}

namespace dcx {

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
void make_random_vector(vms_vector &vec)
{
	vec.x = (d_rand() - 16384) | 1;	// make sure we don't create null vector
	vec.y = d_rand() - 16384;
	vec.z = d_rand() - 16384;
	vm_vec_normalize_quick(vec);
}

}

namespace dsx {
namespace {

//	-------------------------------------------------------------------------------------------------------------------
static void do_firing_stuff(object &obj, const player_flags powerup_flags, const robot_to_player_visibility_state &player_visibility)
{
#if defined(DXX_BUILD_DESCENT_I)
	if (player_is_visible(player_visibility.visibility))
#elif defined(DXX_BUILD_DESCENT_II)
	if (Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD || player_is_visible(player_visibility.visibility))
#endif
	{
		//	Now, if in robot's field of view, lock onto player
		const fix dot = vm_vec_dot(obj.orient.fvec, player_visibility.vec_to_player);
		if ((dot >= 7*F1_0/8) || (powerup_flags & PLAYER_FLAGS_CLOAKED)) {
			ai_static *const aip = &obj.ctype.ai_info;
			ai_local *const ailp = &obj.ctype.ai_info.ail;

			switch (aip->GOAL_STATE) {
				case AIS_NONE:
				case AIS_REST:
				case AIS_SRCH:
				case AIS_LOCK:
					aip->GOAL_STATE = AIS_FIRE;
					if (ailp->player_awareness_type <= player_awareness_type_t::PA_NEARBY_ROBOT_FIRED) {
						ailp->player_awareness_type = player_awareness_type_t::PA_NEARBY_ROBOT_FIRED;
						ailp->player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
				case ai_static_state::AIS_FLIN:
				case ai_static_state::AIS_FIRE:
				case ai_static_state::AIS_RECO:
				case ai_static_state::AIS_ERR_:
					break;
			}
		} else if (dot >= F1_0/2) {
			ai_static *const aip = &obj.ctype.ai_info;
			switch (aip->GOAL_STATE) {
				case AIS_NONE:
				case AIS_REST:
				case AIS_SRCH:
					aip->GOAL_STATE = AIS_LOCK;
					break;
				case ai_static_state::AIS_LOCK:
				case ai_static_state::AIS_FLIN:
				case ai_static_state::AIS_FIRE:
				case ai_static_state::AIS_RECO:
				case ai_static_state::AIS_ERR_:
					break;
			}
		}
	}
}

}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void do_ai_robot_hit(const vmobjptridx_t objp, const robot_info &robptr, player_awareness_type_t type)
{
	if (objp->control_source == object::control_type::ai)
	{
		if (type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION || type == player_awareness_type_t::PA_PLAYER_COLLISION)
			switch (objp->ctype.ai_info.behavior) {
#if defined(DXX_BUILD_DESCENT_I)
				case ai_behavior::AIB_HIDE:
					objp->ctype.ai_info.SUBMODE = AISM_GOHIDE;
					break;
				case ai_behavior::AIB_STILL:
                                        // objp->ctype.ai_info.ail.mode = ai_mode::AIM_CHASE_OBJECT; // NOTE: Should be triggered but causes unwanted movements with bosses. I leave this here for future reference.
					break;
				case ai_behavior::AIB_FOLLOW_PATH:
#elif defined(DXX_BUILD_DESCENT_II)
				case ai_behavior::AIB_STILL:
				{
					int	r;

					r = d_rand();
					//	1/8 time, charge player, 1/4 time create path, rest of time, do nothing
					ai_local		*ailp = &objp->ctype.ai_info.ail;
					if (r < 4096) {
						create_path_to_believed_player_segment(objp, robptr, 10, create_path_safety_flag::safe);
						objp->ctype.ai_info.behavior = ai_behavior::AIB_STATION;
						objp->ctype.ai_info.hide_segment = objp->segnum;
						ailp->mode = ai_mode::AIM_CHASE_OBJECT;
					} else if (r < 4096+8192) {
						create_n_segment_path(objp, robptr, d_rand() / 8192 + 2, segment_none);
						ailp->mode = ai_mode::AIM_FOLLOW_PATH;
					}
					break;
				}
				case ai_behavior::AIB_BEHIND:
				case ai_behavior::AIB_SNIPE:
				case ai_behavior::AIB_FOLLOW:
#endif
				case ai_behavior::AIB_NORMAL:
				case ai_behavior::AIB_RUN_FROM:
				case ai_behavior::AIB_STATION:
					break;
			}
	}
}

namespace {

// --------------------------------------------------------------------------------------------------------------------
//	Note: This function could be optimized.  Surely player_is_visible_from_object would benefit from the
//	information of a normalized vec_to_player.
//	Return player visibility:
//		0		not visible
//		1		visible, but robot not looking at player (ie, on an unobstructed vector)
//		2		visible and in robot's field of view
//		-1		player is cloaked
//	If the player is cloaked, set vec_to_player based on time player cloaked and last uncloaked position.
//	Updates ailp->previous_visibility if player is not cloaked, in which case the previous visibility is left unchanged
//	and is copied to player_visibility

static void compute_vis_and_vec(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, const player_info &player_info, vms_vector &pos, ai_local &ailp, robot_to_player_visibility_state &player_visibility, const robot_info &robptr)
{
	if (player_visibility.initialized)
		return;
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	const auto powerup_flags = player_info.powerup_flags;
		if (powerup_flags & PLAYER_FLAGS_CLOAKED)
		{
			const unsigned cloak_index = (objp) % MAX_AI_CLOAK_INFO;
			const fix delta_time = GameTime64 - Ai_cloak_info[cloak_index].last_time;
			if (delta_time > F1_0*2) {
				Ai_cloak_info[cloak_index].last_time = {GameTime64};
				vm_vec_scale_add2(Ai_cloak_info[cloak_index].last_position, make_random_vector(), 8 * delta_time);
			}

			const auto dist = vm_vec_normalized_dir_quick(player_visibility.vec_to_player, Ai_cloak_info[cloak_index].last_position, pos);
			player_visibility.visibility = player_is_visible_from_object(Robot_info, objp, pos, robptr.field_of_view[Difficulty_level], player_visibility.vec_to_player);
			if (ailp.next_misc_sound_time < GameTime64 && ready_to_fire_any_weapon(robptr, ailp, F1_0) && dist < F1_0 * 20)
			{
				ailp.next_misc_sound_time = GameTime64 + (d_rand() + F1_0) * (7 - underlying_value(Difficulty_level)) / 1;
				digi_link_sound_to_object(robptr.see_sound, objp, 0, Robot_sound_volume, sound_stack::allow_stacking);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			vm_vec_normalized_dir_quick(player_visibility.vec_to_player, Believed_player_pos, pos);
			if (player_visibility.vec_to_player.x == 0 && player_visibility.vec_to_player.y == 0 && player_visibility.vec_to_player.z == 0)
			{
				player_visibility.vec_to_player.x = F1_0;
			}
			player_visibility.visibility = player_is_visible_from_object(Robot_info, objp, pos, robptr.field_of_view[Difficulty_level], player_visibility.vec_to_player);

			//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
			//	see you without killing frame rate.
			{
				ai_static	*aip = &objp->ctype.ai_info;
			if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view && ailp.previous_visibility != player_visibility_state::visible_and_in_field_of_view)
				if ((aip->GOAL_STATE == AIS_REST) || (aip->CURRENT_STATE == AIS_REST)) {
					aip->GOAL_STATE = AIS_FIRE;
					aip->CURRENT_STATE = AIS_FIRE;
				}
			}

#if defined(DXX_BUILD_DESCENT_I)
			if (Player_dead_state != player_dead_state::exploded)
#endif
			if (ailp.previous_visibility != player_visibility.visibility && player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
			{
				if (ailp.previous_visibility == player_visibility_state::no_line_of_sight)
				{
					if (ailp.time_player_seen + F1_0/2 < GameTime64)
					{
						digi_link_sound_to_object(robptr.see_sound, objp, 0, Robot_sound_volume, sound_stack::allow_stacking);
						ailp.time_player_sound_attacked = {GameTime64};
						ailp.next_misc_sound_time = GameTime64 + F1_0 + d_rand()*4;
					}
				}
				else if (ailp.time_player_sound_attacked + F1_0/4 < GameTime64)
				{
					digi_link_sound_to_object(robptr.attack_sound, objp, 0, Robot_sound_volume, sound_stack::allow_stacking);
					ailp.time_player_sound_attacked = {GameTime64};
				}
			} 

			if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view && ailp.next_misc_sound_time < GameTime64)
			{
				ailp.next_misc_sound_time = GameTime64 + (d_rand() + F1_0) * (7 - underlying_value(Difficulty_level)) / 2;
				digi_link_sound_to_object(robptr.attack_sound, objp, 0, Robot_sound_volume, sound_stack::allow_stacking);
			}
			ailp.previous_visibility = player_visibility.visibility;
		}

#if defined(DXX_BUILD_DESCENT_II)
		//	@mk, 09/21/95: If player view is not obstructed and awareness is at least as high as a nearby collision,
		//	act is if robot is looking at player.
		if (ailp.player_awareness_type >= player_awareness_type_t::PA_NEARBY_ROBOT_FIRED)
			if (player_visibility.visibility == player_visibility_state::visible_not_in_field_of_view)
				player_visibility.visibility = player_visibility_state::visible_and_in_field_of_view;
#endif

		if (player_is_visible(player_visibility.visibility))
		{
			ailp.time_player_seen = {GameTime64};
		}
	player_visibility.initialized = 1;
}

#if defined(DXX_BUILD_DESCENT_II)
static void compute_buddy_vis_vec(const vmobjptridx_t buddy_obj, const vms_vector &buddy_pos, robot_to_player_visibility_state &player_visibility, const robot_info &robptr)
{
	if (player_visibility.initialized)
		return;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
	{
		player_visibility.vec_to_player = {};
		player_visibility.visibility = player_visibility_state::no_line_of_sight;
		player_visibility.initialized = 1;
		return;
	}
	auto &plrobj = *Objects.vcptr(plr.objnum);
	/* Buddy ignores cloaking */
	vm_vec_normalized_dir_quick(player_visibility.vec_to_player, plrobj.pos, buddy_pos);
	if (player_visibility.vec_to_player.x == 0 && player_visibility.vec_to_player.y == 0 && player_visibility.vec_to_player.z == 0)
		player_visibility.vec_to_player.x = F1_0;

	fvi_info hit_data;
	const auto hit_type = find_vector_intersection(fvi_query{
		buddy_pos,
		plrobj.pos,
		fvi_query::unused_ignore_obj_list,
		fvi_query::unused_LevelUniqueObjectState,
		fvi_query::unused_Robot_info,
		FQ_TRANSWALL,
		buddy_obj,
	}, buddy_obj->segnum, F1_0 / 4, hit_data);

	auto &ailp = buddy_obj->ctype.ai_info.ail;
	player_visibility.visibility = (hit_type == fvi_hit_type::None)
		? ((ailp.time_player_seen = GameTime64, vm_vec_dot(player_visibility.vec_to_player, buddy_obj->orient.fvec) > robptr.field_of_view[GameUniqueState.Difficulty_level])
		   ? player_visibility_state::visible_and_in_field_of_view
		   : player_visibility_state::visible_not_in_field_of_view
		)
		: player_visibility_state::no_line_of_sight;
	ailp.previous_visibility = player_visibility.visibility;
	player_visibility.initialized = 1;
}
#endif

}

// --------------------------------------------------------------------------------------------------------------------
//	Move object one object radii from current position towards segment center.
//	If segment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(const robot_info &robptr, const d_level_shared_segment_state &LevelSharedSegmentState, object_base &objp)
{
/* ZICO's change of 20081103:
   Make move to segment center smoother by using move_towards vector.
   Bot's should not jump around and maybe even intersect with each other!
   In case it breaks something what I do not see, yet, old code is still there. */
	const auto segnum = objp.segnum;
	vms_vector	vec_to_center;

	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &SSegments = LevelSharedSegmentState.get_segments();
	const auto &&segment_center = compute_segment_center(Vertices.vcptr, SSegments.vcptr(segnum));
	vm_vec_normalized_dir_quick(vec_to_center, segment_center, objp.pos);
	move_towards_vector(robptr, objp, vec_to_center, 1);
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robot.
//	Brains, avoid robots, companions can open doors.
//	objp == NULL means treat as buddy.
int ai_door_is_openable(
	const object &obj, const robot_info *const robptr,
#if defined(DXX_BUILD_DESCENT_II)
	const player_flags powerup_flags,
#endif
	const shared_segment &segp, const sidenum_t sidenum)
{
	if (!IS_CHILD(segp.children[sidenum]))
		return 0;		//trap -2 (exit side)

	const auto wall_num = segp.sides[sidenum].wall_num;

	if (wall_num == wall_none)		//if there's no door at all...
		return 0;				//..then say it can't be opened

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	auto &wall = *vcwallptr(wall_num);
	//	The mighty console object can open all doors (for purposes of determining paths).
	if (&obj == ConsoleObject) {
		const auto wt = wall.type;
		if (wt == WALL_DOOR)
		{
			static_assert(WALL_DOOR != 0, "WALL_DOOR must be nonzero for this shortcut to work properly.");
			return wt;
		}
	}
	if (!robptr)
		return 0;

#if defined(DXX_BUILD_DESCENT_I)
	if (get_robot_id(obj) == robot_id::brain || obj.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM)
	{

		if (wall_num != wall_none)
		{
			const auto wt = wall.type;
			if (wt == WALL_DOOR && wall.keys == wall_key::none && !(wall.flags & wall_flag::door_locked))
			{
				static_assert(WALL_DOOR != 0, "WALL_DOOR must be nonzero for this shortcut to work properly.");
				return wt;
			}
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	auto &WallAnims = GameSharedState.WallAnims;
	if (robptr->companion)
	{
		const auto wt = wall.type;
		if (wall.flags & wall_flag::buddy_proof) {
			if (wt == WALL_DOOR && wall.state == wall_state::closed)
				return 0;
			else if (wt == WALL_CLOSED)
				return 0;
			else if (wt == WALL_ILLUSION && !(wall.flags & wall_flag::illusion_off))
				return 0;
		}
		switch (const auto wall_keys = wall.keys)
		{
			case wall_key::blue:
			case wall_key::gold:
			case wall_key::red:
				{
					return powerup_flags & static_cast<PLAYER_FLAG>(wall_keys);
				}
			default:
				break;
		}

		if (wt != WALL_DOOR && wt != WALL_CLOSED)
			return 1;

		//	If Buddy is returning to player, don't let him think he can get through triggered doors.
		//	It's only valid to think that if the player is going to get him through.  But if he's
		//	going to the player, the player is probably on the opposite side.
		const ai_mode ailp_mode = obj.ctype.ai_info.ail.mode;

		// -- if (Buddy_got_stuck) {
		if (ailp_mode == ai_mode::AIM_GOTO_PLAYER) {
			if (wt == WALL_BLASTABLE && !(wall.flags & wall_flag::blasted))
				return 0;
			if (wt == WALL_CLOSED)
				return 0;
			if (wt == WALL_DOOR) {
				if ((wall.flags & wall_flag::door_locked) && (wall.state == wall_state::closed))
					return 0;
			}
		}
		// -- }

		if (ailp_mode != ai_mode::AIM_GOTO_PLAYER && wall.controlling_trigger != trigger_none)
		{
			const auto clip_num = wall.clip_num;
			if (clip_num == -1)
				return clip_num;
			else if (WallAnims[clip_num].flags & WCF_HIDDEN) {
				static_assert(static_cast<unsigned>(wall_state::closed) == 0, "wall_state::closed must be zero for this shortcut to work properly.");
				return underlying_value(wall.state);
			} else
				return 1;
		}

		if (wt == WALL_DOOR)  {
				const auto clip_num = wall.clip_num;

				if (clip_num == -1)
					return clip_num;
				//	Buddy allowed to go through secret doors to get to player.
				else if ((ailp_mode != ai_mode::AIM_GOTO_PLAYER) && (WallAnims[clip_num].flags & WCF_HIDDEN)) {
					static_assert(static_cast<unsigned>(wall_state::closed) == 0, "wall_state::closed must be zero for this shortcut to work properly.");
					return underlying_value(wall.state);
				} else
					return 1;
		}
	}
	else if (get_robot_id(obj) == robot_id::brain || obj.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM || obj.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
	{
		if (wall_num != wall_none)
		{
			const auto wt = wall.type;
			if (wt == WALL_DOOR && (wall.keys == wall_key::none) && !(wall.flags & wall_flag::door_locked))
			{
				static_assert(WALL_DOOR != 0, "WALL_DOOR must be nonzero for this shortcut to work properly.");
				return wt;
			}
			else if (wall.keys != wall_key::none) {	//	Allow bots to open doors to which player has keys.
				return powerup_flags & static_cast<PLAYER_FLAG>(wall.keys);
			}
		}
	}
#endif
	return 0;
}

namespace {

//	-----------------------------------------------------------------------------------------------------------
//	Return side of openable door in segment, if any.  If none, return side_none.
static std::optional<sidenum_t> openable_doors_in_segment(fvcwallptr &vcwallptr, const shared_segment &segp)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &WallAnims = GameSharedState.WallAnims;
#endif
	for (const auto &&[idx, value] : enumerate(segp.sides))
	{
		const auto wall_num = value.wall_num;
		if (wall_num != wall_none)
		{
			auto &w = *vcwallptr(wall_num);
			if (w.type != WALL_DOOR)
				continue;
			if (w.keys != wall_key::none)
				continue;
			if (w.state != wall_state::closed)
				continue;
			if (w.flags & wall_flag::door_locked)
				continue;
#if defined(DXX_BUILD_DESCENT_II)
			if (WallAnims[w.clip_num].flags & WCF_HIDDEN)
				continue;
#endif
			return static_cast<sidenum_t>(idx);
		}
	}
	return std::nullopt;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an object of size size at pos *pos intersects a (player or robot or control center) in segment *segp.
static int check_object_object_intersection(const vms_vector &pos, fix size, const unique_segment &segp)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	//	If this would intersect with another object (only check those in this segment), then try to move.
	range_for (const object_base &curobj, objects_in(segp, vcobjptridx, vcsegptr))
	{
		if (curobj.type == OBJ_PLAYER || curobj.type == OBJ_ROBOT || curobj.type == OBJ_CNTRLCEN)
		{
			if (vm_vec_dist_quick(pos, curobj.pos) < size + curobj.size)
				return 1;
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return objnum if object created, else return -1.
//	If pos == NULL, pick random spot in segment.
static imobjptridx_t create_gated_robot(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, fvcobjptr &vcobjptr, const vmsegptridx_t segp, const robot_id object_id, const vms_vector *const pos)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	const auto Gate_interval = GameUniqueState.Boss_gate_interval;
#if defined(DXX_BUILD_DESCENT_I)
	const unsigned maximum_gated_robots = 2 * underlying_value(Difficulty_level) + 3;
#elif defined(DXX_BUILD_DESCENT_II)
	if (GameTime64 - BossUniqueState.Last_gate_time < Gate_interval)
		return object_none;
	const unsigned maximum_gated_robots = 2 * underlying_value(Difficulty_level) + 6;
#endif

	unsigned count = 0;
	range_for (const auto &&objp, vcobjptr)
	{
		auto &obj = *objp;
		if (obj.type == OBJ_ROBOT)
			if (obj.matcen_creator == BOSS_GATE_MATCEN_NUM)
				count++;
	}

	if (count > maximum_gated_robots)
	{
		BossUniqueState.Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return object_none;
	}

	auto &vcvertptr = Vertices.vcptr;
	const auto object_pos = pos ? *pos : pick_random_point_in_seg(vcvertptr, segp, std::minstd_rand(d_rand()));

	//	See if legal to place object here.  If not, move about in segment and try again.
	auto &robptr = Robot_info[object_id];
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const fix objsize = Polygon_models[robptr.model_num].rad;
	if (check_object_object_intersection(object_pos, objsize, segp)) {
		BossUniqueState.Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return object_none;
	}

#if defined(DXX_BUILD_DESCENT_I)
	const ai_behavior default_behavior = (object_id == robot_id::toaster) //	This is a toaster guy!
	? ai_behavior::AIB_RUN_FROM
	: ai_behavior::AIB_NORMAL;
#elif defined(DXX_BUILD_DESCENT_II)
	const ai_behavior default_behavior = robptr.behavior;
#endif
	auto objp = robot_create(Robot_info, object_id, segp, object_pos, &vmd_identity_matrix, objsize, default_behavior);

	if ( objp == object_none ) {
		BossUniqueState.Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return object_none;
	}

	//Set polygon-object-specific data

	objp->rtype.pobj_info.model_num = robptr.model_num;
	objp->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	objp->mtype.phys_info.mass = robptr.mass;
	objp->mtype.phys_info.drag = robptr.drag;

	objp->mtype.phys_info.flags |= (PF_LEVELLING);

	objp->shields = robptr.strength;
	objp->matcen_creator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.

#if defined(DXX_BUILD_DESCENT_II)
	objp->lifeleft = F1_0*30;	//	Gated in robots only live 30 seconds.
#endif

	object_create_explosion_without_damage(Vclip, segp, object_pos, i2f(10), vclip_index::morphing_robot);
	digi_link_sound_to_pos(Vclip[vclip_index::morphing_robot].sound_num, segp, sidenum_t::WLEFT, object_pos, 0, F1_0);
	morph_start(LevelUniqueMorphObjectState, LevelSharedPolygonModelState, objp);

	BossUniqueState.Last_gate_time = {GameTime64};
	++LevelUniqueObjectState.accumulated_robots;
	++GameUniqueState.accumulated_robots;

	return objp;
}

}

// --------------------------------------------------------------------------------------------------------------------
//	Make object objp gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return objnum if robot successfully created, else return -1
imobjptridx_t gate_in_robot(const d_robot_info_array &Robot_info, const robot_id type, const vmsegptridx_t segnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	return create_gated_robot(Robot_info, Vclip, vcobjptr, segnum, type, nullptr);
}

namespace {

static imobjptridx_t gate_in_robot(const d_robot_info_array &Robot_info, fvmsegptridx &vmsegptridx, const robot_id type)
{
	auto &Boss_gate_segs = LevelSharedBossState.Gate_segs;
	auto segnum = Boss_gate_segs[(d_rand() * Boss_gate_segs.size()) >> 15];
	return gate_in_robot(Robot_info, type, vmsegptridx(segnum));
}

}

}

namespace dcx {

namespace {

static const shared_segment *boss_intersects_wall(fvcvertptr &vcvertptr, const object_base &boss_objp, const vcsegptridx_t segp)
{
	const auto size = boss_objp.size;
	const auto &&segcenter = compute_segment_center(vcvertptr, segp);
	const auto &&r = ranges::subrange(segp->verts);
	const auto re = r.end();
	auto pos = segcenter;
	for (auto ri = r.begin();; ++ri)
	{
		const auto seg = sphere_intersects_wall(vcsegptridx, vcvertptr, pos, segp, size).seg;
		if (!seg)
			return seg;
		if (ri == re)
			return seg;
		auto &vertex_pos = *vcvertptr(*ri);
		pos = vm_vec_avg(vertex_pos, segcenter);
	}
}

// ----------------------------------------------------------------------------------
static void pae_aux(const vcsegptridx_t segnum, const player_awareness_type_t type, awareness_t &New_awareness, const unsigned allowed_recursions_remaining)
{
	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;
	const auto sub_allowed_recursions_remaining = allowed_recursions_remaining - 1;
	if (!sub_allowed_recursions_remaining)
		return;
	// Process children.
	const auto subtype = (type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION)
		? player_awareness_type_t::PA_PLAYER_COLLISION
		: type;
	for (const auto j : segnum->shared_segment::children)
		if (IS_CHILD(j))
			pae_aux(segnum.absolute_sibling(j), subtype, New_awareness, sub_allowed_recursions_remaining);
}

}

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void create_buddy_bot(const d_level_shared_robot_info_state &LevelSharedRobotInfoState)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	for (auto &&[idx, ri] : enumerate(partial_const_range(LevelSharedRobotInfoState.Robot_info, LevelSharedRobotInfoState.N_robot_types)))
	{
		if (!ri.companion)
			continue;
		const auto &&segp = vmsegptridx(ConsoleObject->segnum);
		const auto &&object_pos = compute_segment_center(Vertices.vcptr, segp);
		create_morph_robot(LevelSharedRobotInfoState.Robot_info, segp, object_pos, idx);
		break;
	}
}
#endif

namespace {

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at imsegptr.
//	Set *num_segs.
//	Boss is allowed to teleport to segments he fits in (calls object_intersects_wall) and
//	he can reach from his initial position (calls find_connected_distance).
//	If size_check is set, then only add segment if boss can fit in it, else any segment is legal.
//	one_wall_hack added by MK, 10/13/95: A mega-hack!  Set to !0 to ignore the 
static void init_boss_segments(const segment_array &segments, const object &boss_objp, d_level_shared_boss_state::special_segment_array_t &a, const int size_check, int one_wall_hack)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	constexpr unsigned QUEUE_SIZE = 256;
	auto &vcsegptridx = segments.vcptridx;
	auto &vmsegptr = segments.vmptr;
#if defined(DXX_BUILD_DESCENT_I)
	one_wall_hack = 0;
#endif

	a.clear();
#if DXX_USE_EDITOR
	Selected_segs.clear();
#endif

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	{
		int			head, tail;
		std::array<segnum_t, QUEUE_SIZE> seg_queue;

		const auto original_boss_seg = boss_objp.segnum;
		head = 0;
		tail = 0;
		seg_queue[head++] = original_boss_seg;
		auto &vcvertptr = Vertices.vcptr;

		if (!size_check || !boss_intersects_wall(vcvertptr, boss_objp, vcsegptridx(original_boss_seg)))
		{
			a.emplace_back(original_boss_seg);
#if DXX_USE_EDITOR
			Selected_segs.emplace_back(original_boss_seg);
#endif
		}

		visited_segment_bitarray_t visited;

		while (tail != head) {
			const cscusegment segp = *vmsegptr(seg_queue[tail++]);

			tail &= QUEUE_SIZE-1;

			for (const auto &&[sidenum, csegnum] : enumerate(segp.s.children))
			{
				const auto w = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, static_cast<sidenum_t>(sidenum));
				if ((w & WALL_IS_DOORWAY_FLAG::fly) || one_wall_hack)
				{
#if defined(DXX_BUILD_DESCENT_II)
					//	If we get here and w == wall_is_doorway_result::wall, then we want to process through this wall, else not.
					if (IS_CHILD(csegnum)) {
						if (one_wall_hack)
							one_wall_hack--;
					} else
						continue;
#endif

					if (auto &&v = visited[csegnum])
					{
					}
					else
					{
						v = true;
						seg_queue[head++] = csegnum;
						head &= QUEUE_SIZE-1;
						if (head > tail) {
							if (head == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
						} else
							if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
	
						if (!size_check || !boss_intersects_wall(vcvertptr, boss_objp, vcsegptridx(csegnum))) {
							a.emplace_back(csegnum);
#if DXX_USE_EDITOR
							Selected_segs.emplace_back(csegnum);
							#endif
							if (a.size() >= a.max_size())
							{
								tail = head;
								break;
							}
						}
					}
				}
			}
		}
		// Last resort - add original seg even if boss doesn't fit in it
		if (a.empty())
			a.emplace_back(original_boss_seg);
	}
}

// --------------------------------------------------------------------------------------------------------------------
static void teleport_boss(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, const vms_vector &target_pos)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	segnum_t			rand_segnum;
	int			rand_index;
	assert(!Boss_teleport_segs.empty());
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;

	//	Pick a random segment from the list of boss-teleportable-to segments.
	rand_index = (d_rand() * Boss_teleport_segs.size()) >> 15;
	rand_segnum = Boss_teleport_segs[rand_index];
	Assert(rand_segnum <= Highest_segment_index);

	if (Game_mode & GM_MULTI)
		multi_send_boss_teleport(objp, rand_segnum);

	const auto &&rand_segp = vmsegptridx(rand_segnum);
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	compute_segment_center(vcvertptr, objp->pos, rand_segp);
	obj_relink(vmobjptr, vmsegptr, objp, rand_segp);

	BossUniqueState.Last_teleport_time = {GameTime64};

	//	make boss point right at player
	const auto boss_dir = vm_vec_sub(target_pos, objp->pos);
	vm_vector_2_matrix(objp->orient, boss_dir, nullptr, nullptr);

	digi_link_sound_to_pos(Vclip[vclip_index::morphing_robot].sound_num, rand_segp, sidenum_t::WLEFT, objp->pos, 0, F1_0);
	digi_kill_sound_linked_to_object( objp);

	//	After a teleport, boss can fire right away.
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	ailp->next_fire = 0;
#if defined(DXX_BUILD_DESCENT_II)
	ailp->next_fire2 = 0;
#endif
	boss_link_see_sound(Robot_info, objp);
}

}

//	----------------------------------------------------------------------
void start_boss_death_sequence(d_level_unique_boss_state &BossUniqueState, const d_robot_info_array &Robot_info, object &objp)
{
	auto &robptr = Robot_info[get_robot_id(objp)];
	if (robptr.boss_flag != boss_robot_id::None)
	{
		BossUniqueState.Boss_dying = 1;
		BossUniqueState.Boss_dying_start_time = {GameTime64};
	}

}

//	----------------------------------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
namespace {
static void do_boss_dying_frame(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	const auto time_boss_dying = GameTime64 - BossUniqueState.Boss_dying_start_time;
	{
		auto &rotvel = objp->mtype.phys_info.rotvel;
		rotvel.x = time_boss_dying / 9;
		rotvel.y = time_boss_dying / 5;
		rotvel.z = time_boss_dying / 7;
	}

	if (BossUniqueState.Boss_dying_start_time + BOSS_DEATH_DURATION - BOSS_DEATH_SOUND_DURATION < GameTime64) {
		if (!BossUniqueState.Boss_dying_sound_playing)
		{
			BossUniqueState.Boss_dying_sound_playing = 1;
			digi_link_sound_to_object2(SOUND_BOSS_SHARE_DIE, objp, 0, F1_0*4, sound_stack::allow_stacking, vm_distance{F1_0*1024});	//	F1_0*512 means play twice as loud
                } else if (d_rand() < FrameTime*16)
                        create_small_fireball_on_object(objp, (F1_0 + d_rand()) * 8, 0);
        } else if (d_rand() < FrameTime*8)
                create_small_fireball_on_object(objp, (F1_0/2 + d_rand()) * 8, 1);

	if (BossUniqueState.Boss_dying_start_time + BOSS_DEATH_DURATION < GameTime64 || GameTime64+(F1_0*2) < BossUniqueState.Boss_dying_start_time)
	{
		BossUniqueState.Boss_dying_start_time = {GameTime64}; // make sure following only happens one time!
		do_controlcen_destroyed_stuff(object_none);
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, objp, F1_0/4);
		digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, sound_stack::allow_stacking, vm_distance{F1_0*512});
	}
}

static int do_any_robot_dying_frame(const d_robot_info_array &, vmobjptridx_t)
{
	return 0;
}
}
#elif defined(DXX_BUILD_DESCENT_II)
//	objp points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
imobjptridx_t boss_spew_robot(const d_robot_info_array &Robot_info, const object_base &objp, const vms_vector &pos)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	const auto boss_index = build_boss_robot_index_from_boss_robot_id(Robot_info[get_robot_id(objp)].boss_flag);
	assert(Boss_spew_more.valid_index(boss_index));

	auto &Segments = LevelUniqueSegmentState.get_segments();
	const auto &&segnum = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos, Segments.vmptridx(objp.segnum));
	if (segnum == segment_none) {
		return object_none;
	}	

	const auto &&newobjp = create_gated_robot(Robot_info, Vclip, vcobjptr, segnum, Spew_bots[boss_index][(Max_spew_bots[boss_index] * d_rand()) >> 15], &pos);
 
	//	Make spewed robot come tumbling out as if blasted by a flash missile.
	if (newobjp != object_none) {
		int		force_val;

		force_val = F1_0/FrameTime;

		if (force_val) {
			newobjp->ctype.ai_info.SKIP_AI_COUNT += force_val;
			newobjp->mtype.phys_info.rotthrust.x = ((d_rand() - 16384) * force_val)/16;
			newobjp->mtype.phys_info.rotthrust.y = ((d_rand() - 16384) * force_val)/16;
			newobjp->mtype.phys_info.rotthrust.z = ((d_rand() - 16384) * force_val)/16;
			newobjp->mtype.phys_info.flags |= PF_USES_THRUST;

			//	Now, give a big initial velocity to get moving away from boss.
			vm_vec_sub(newobjp->mtype.phys_info.velocity, pos, objp.pos);
			vm_vec_normalize_quick(newobjp->mtype.phys_info.velocity);
			vm_vec_scale(newobjp->mtype.phys_info.velocity, F1_0*128);
		}
	}

	return newobjp;
}

//	----------------------------------------------------------------------
void start_robot_death_sequence(object &obj)
{
	auto &ai_info = obj.ctype.ai_info;
	ai_info.dying_start_time = {GameTime64};
	ai_info.dying_sound_playing = 0;
	ai_info.SKIP_AI_COUNT = 0;
}

namespace {

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if object just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
static int do_robot_dying_frame(const vmobjptridx_t objp, fix64 start_time, fix roll_duration, sbyte *dying_sound_playing, int death_sound, fix expl_scale, fix sound_scale)
{
	fix	sound_duration;

	if (!roll_duration)
		roll_duration = F1_0/4;

	objp->mtype.phys_info.rotvel.x = (GameTime64 - start_time)/9;
	objp->mtype.phys_info.rotvel.y = (GameTime64 - start_time)/5;
	objp->mtype.phys_info.rotvel.z = (GameTime64 - start_time)/7;

	if (const auto SndDigiSampleRate = underlying_value(GameArg.SndDigiSampleRate))
		sound_duration = fixdiv(GameSounds[digi_xlat_sound(death_sound)].length, SndDigiSampleRate);
	else
		sound_duration = F1_0;

	if (start_time + roll_duration - sound_duration < GameTime64) {
		if (!*dying_sound_playing) {
			*dying_sound_playing = 1;
			digi_link_sound_to_object2(death_sound, objp, 0, sound_scale, sound_stack::allow_stacking, vm_distance{sound_scale*256});	//	F1_0*512 means play twice as loud
		} else if (d_rand() < FrameTime*16)
			create_small_fireball_on_object(objp, (F1_0 + d_rand()) * (16 * expl_scale/F1_0)/8, 0);
	} else if (d_rand() < FrameTime*8)
		create_small_fireball_on_object(objp, (F1_0/2 + d_rand()) * (16 * expl_scale/F1_0)/8, 1);

	if (start_time + roll_duration < GameTime64 || GameTime64+(F1_0*2) < start_time)
		return 1;
	else
		return 0;
}

//	----------------------------------------------------------------------
static void do_boss_dying_frame(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	int	rval;

	rval = do_robot_dying_frame(objp, BossUniqueState.Boss_dying_start_time, BOSS_DEATH_DURATION, &BossUniqueState.Boss_dying_sound_playing, Robot_info[get_robot_id(objp)].deathroll_sound, F1_0*4, F1_0*4);

	if (rval)
	{
		BossUniqueState.Boss_dying_start_time = {GameTime64}; // make sure following only happens one time!
		do_controlcen_destroyed_stuff(object_none);
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, objp, F1_0/4);
		digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, sound_stack::allow_stacking, vm_distance{F1_0*512});
	}
}

//	----------------------------------------------------------------------
static int do_any_robot_dying_frame(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	if (objp->ctype.ai_info.dying_start_time) {
		int	rval, death_roll;

		death_roll = Robot_info[get_robot_id(objp)].death_roll;
		rval = do_robot_dying_frame(objp, objp->ctype.ai_info.dying_start_time, min(death_roll/2+1,6)*F1_0, &objp->ctype.ai_info.dying_sound_playing, Robot_info[get_robot_id(objp)].deathroll_sound, death_roll*F1_0/8, death_roll*F1_0/2);

		if (rval) {
			objp->ctype.ai_info.dying_start_time = {GameTime64}; // make sure following only happens one time!
			explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, objp, F1_0/4);
			digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, sound_stack::allow_stacking, vm_distance{F1_0*512});
			if (Current_level_num < 0)
			{
				const auto id = get_robot_id(objp);
				if (Robot_info[id].thief)
					recreate_thief(Robot_info, id);
			}
		}

		return 1;
	}

	return 0;
}

}
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Called for an AI object if it is fairly aware of the player.
//	awareness_level is in 0..100.  Larger numbers indicate greater awareness (eg, 99 if firing at player).
//	In a given frame, might not get called for an object, or might be called more than once.
//	The fact that this routine is not called for a given object does not mean that object is not interested in the player.
//	Objects are moved by physics, so they can move even if not interested in a player.  However, if their velocity or
//	orientation is changing, this routine will be called.
//	Return value:
//		0	this player IS NOT allowed to move this robot.
//		1	this player IS allowed to move this robot.
int ai_multiplayer_awareness(const vmobjptridx_t objp, int awareness_level)
{
	int	rval=1;

	if (Game_mode & GM_MULTI) {
		if (awareness_level == 0)
			return 0;
		rval = multi_can_move_robot(objp, awareness_level);
	}

	return rval;
}

}

#ifndef NDEBUG
namespace {
fix	Prev_boss_shields = -1;
}
#endif

namespace dsx {
namespace {

#if defined(DXX_BUILD_DESCENT_I)
#define do_d1_boss_stuff(Robot_info,FS,FO,PV)	do_d1_boss_stuff(Robot_info,FS,FO)
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
static void do_d1_boss_stuff(const d_robot_info_array &Robot_info, fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, const player_visibility_state player_visibility)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
#ifndef NDEBUG
	if (objp->shields != Prev_boss_shields) {
		Prev_boss_shields = objp->shields;
	}
#endif

#if defined(DXX_BUILD_DESCENT_II)
	if (!EMULATING_D1 && !player_is_visible(player_visibility) && !BossUniqueState.Boss_hit_this_frame)
		return;
#endif

	if (!BossUniqueState.Boss_dying) {
		const auto Boss_cloak_start_time = BossUniqueState.Boss_cloak_start_time;
		if (objp->ctype.ai_info.CLOAKED == 1) {
			if (GameTime64 - Boss_cloak_start_time > Boss_cloak_duration / 3 &&
				(Boss_cloak_start_time + Boss_cloak_duration) - GameTime64 > Boss_cloak_duration / 3 &&
				GameTime64 - BossUniqueState.Last_teleport_time > LevelSharedBossState.Boss_teleport_interval)
			{
				if (ai_multiplayer_awareness(objp, 98))
					teleport_boss(Robot_info, Vclip, vmsegptridx, objp, get_local_plrobj().pos);
			} else if (BossUniqueState.Boss_hit_this_frame) {
				BossUniqueState.Boss_hit_this_frame = 0;
				BossUniqueState.Last_teleport_time -= LevelSharedBossState.Boss_teleport_interval / 4;
			}

			if (GameTime64 > (Boss_cloak_start_time + Boss_cloak_duration) ||
				GameTime64 < Boss_cloak_start_time)
				objp->ctype.ai_info.CLOAKED = 0;
		} else {
			if (BossUniqueState.Boss_hit_this_frame ||
				GameTime64 - (Boss_cloak_start_time + Boss_cloak_duration) > LevelSharedBossState.Boss_cloak_interval)
			{
				if (ai_multiplayer_awareness(objp, 95))
				{
					BossUniqueState.Boss_hit_this_frame = 0;
					BossUniqueState.Boss_cloak_start_time = {GameTime64};
					objp->ctype.ai_info.CLOAKED = 1;
					if (Game_mode & GM_MULTI)
						multi_send_boss_cloak(objp);
				}
			}
		}
	} else
		do_boss_dying_frame(Robot_info, objp);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(F1_0*150)


// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
static void do_super_boss_stuff(const d_robot_info_array &Robot_info, fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, const fix dist_to_player, const player_visibility_state player_visibility)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	static int eclip_state = 0;
	do_d1_boss_stuff(Robot_info, vmsegptridx, objp, player_visibility);

	// Only master player can cause gating to occur.
	if ((Game_mode & GM_MULTI) && !multi_i_am_master())
                return;

	if (dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE || player_is_visible(player_visibility) || (Game_mode & GM_MULTI)) {
		const auto Gate_interval = GameUniqueState.Boss_gate_interval;
		if (GameTime64 - BossUniqueState.Last_gate_time > Gate_interval/2) {
			restart_effect(ECLIP_NUM_BOSS);
			if (eclip_state == 0) {
				multi_send_boss_start_gate(objp);
				eclip_state = 1;
			}
		}
		else {
			stop_effect(ECLIP_NUM_BOSS);
			if (eclip_state == 1) {
				multi_send_boss_stop_gate(objp);
				eclip_state = 0;
			}
		}

		if (GameTime64 - BossUniqueState.Last_gate_time > Gate_interval)
			if (ai_multiplayer_awareness(objp, 99)) {
				uint_fast32_t randtype = (d_rand() * MAX_GATE_INDEX) >> 15;

				Assert(randtype < MAX_GATE_INDEX);
				const auto &&rtval = gate_in_robot(Robot_info, vmsegptridx, Super_boss_gate_list[randtype]);
				if (rtval != object_none && (Game_mode & GM_MULTI))
				{
					multi_send_boss_create_robot(objp, rtval);
				}
			}	
	}
}

#if defined(DXX_BUILD_DESCENT_II)
static void do_d2_boss_stuff(fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, const player_visibility_state player_visibility)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	const auto boss_index = build_boss_robot_index_from_boss_robot_id(Robot_info[get_robot_id(objp)].boss_flag);
	assert(Boss_spew_more.valid_index(boss_index));

#ifndef NDEBUG
	if (objp->shields != Prev_boss_shields) {
		Prev_boss_shields = objp->shields;
	}
#endif

	//	@mk, 10/13/95:  Reason:
	//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
	//		teleports out of there right away, and blasts player right after first door.
	if (!player_is_visible(player_visibility) && GameTime64 - BossUniqueState.Boss_hit_time > F1_0 * 2)
		return;

	if (!BossUniqueState.Boss_dying && Boss_teleports[boss_index]) {
		const auto Boss_cloak_start_time = BossUniqueState.Boss_cloak_start_time;
		if (objp->ctype.ai_info.CLOAKED == 1) {
			BossUniqueState.Boss_hit_time = {GameTime64};	//	Keep the cloak:teleport process going.
			if (GameTime64 - Boss_cloak_start_time > Boss_cloak_duration / 3 &&
				(Boss_cloak_start_time + Boss_cloak_duration) - GameTime64 > Boss_cloak_duration / 3 &&
				GameTime64 - BossUniqueState.Last_teleport_time > LevelSharedBossState.Boss_teleport_interval)
			{
				if (ai_multiplayer_awareness(objp, 98))
					teleport_boss(LevelSharedRobotInfoState.Robot_info, Vclip, vmsegptridx, objp, get_local_plrobj().pos);
			} else if (GameTime64 - BossUniqueState.Boss_hit_time > F1_0*2) {
				BossUniqueState.Last_teleport_time -= LevelSharedBossState.Boss_teleport_interval / 4;
			}

			if (GameTime64 > (Boss_cloak_start_time + Boss_cloak_duration) ||
				GameTime64 < Boss_cloak_start_time)
				objp->ctype.ai_info.CLOAKED = 0;
		} else if (GameTime64 - (Boss_cloak_start_time + Boss_cloak_duration) > LevelSharedBossState.Boss_cloak_interval ||
				GameTime64 - (Boss_cloak_start_time + Boss_cloak_duration) < -Boss_cloak_duration) {
			if (ai_multiplayer_awareness(objp, 95)) {
				BossUniqueState.Boss_cloak_start_time = {GameTime64};
				objp->ctype.ai_info.CLOAKED = 1;
				if (Game_mode & GM_MULTI)
					multi_send_boss_cloak(objp);
			}
		}
	}

}
#endif


static void ai_multi_send_robot_position(object &obj, int force)
{
	if (Game_mode & GM_MULTI) 
		multi_send_robot_position(obj, force != -1 ? multi_send_robot_position_priority::_2 : multi_send_robot_position_priority::_1);
	return;
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this object should be allowed to fire at the player.
static int maybe_ai_do_actual_firing_stuff(object &obj)
{
	if (Game_mode & GM_MULTI)
	{
		auto &aip = obj.ctype.ai_info;
		if (aip.GOAL_STATE != AIS_FLIN && get_robot_id(obj) != robot_id::brain)
		{
			const auto s = aip.CURRENT_STATE;
			if (s == AIS_FIRE)
			{
				static_assert(AIS_FIRE != 0, "AIS_FIRE must be nonzero for this shortcut to work properly.");
				return s;
			}
		}
	}

	return 0;
}

#if defined(DXX_BUILD_DESCENT_I)
static void ai_do_actual_firing_stuff(const d_robot_info_array &Robot_info, fvmobjptridx &vmobjptridx, const vmobjptridx_t obj, ai_static *const aip, ai_local &ailp, const robot_info &robptr, const fix dist_to_player, robot_gun_point &gun_point, const robot_to_player_visibility_state &player_visibility, const int object_animates, const player_info &player_info, robot_gun_number)
{
	auto &vec_to_player = player_visibility.vec_to_player;
	fix	dot;

	if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
	{
		//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
		// if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE)) && (ailp->next_fire <= 0))
		if (!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) {
			dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
			if (dot >= 7*F1_0/8) {
				if (robot_gun_number_valid(robptr, aip->CURRENT_GUN))
				{
					if (robptr.attack_type == 1) {
						if (Player_dead_state != player_dead_state::exploded &&
							dist_to_player < obj->size + ConsoleObject->size + F1_0*2)
						{
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(Robot_info, obj, vmobjptridx(ConsoleObject), obj->pos);
						} else {
							return;
						}
					} else {
						if (!gun_point.has_value()) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, robot_gun_number::_0);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if (aip->behavior != ai_behavior::AIB_RUN_FROM && aip->behavior != ai_behavior::AIB_STILL && aip->behavior != ai_behavior::AIB_FOLLOW_PATH && (ailp.mode == ai_mode::AIM_FOLLOW_PATH || ailp.mode == ai_mode::AIM_STILL))
						ailp.mode = ai_mode::AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp.goal_state[aip->CURRENT_GUN] = AIS_RECO;
				// Switch to next gun for next fire.
				aip->CURRENT_GUN = robot_advance_current_gun(robptr, aip->CURRENT_GUN);
			}
		}
	} else if (Weapon_info[robptr.weapon_type].homing_flag) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if ((!object_animates || ailp.achieved_state[aip->CURRENT_GUN] == AIS_FIRE)
			&& ready_to_fire_weapon1(ailp, 0)
			&& (vm_vec_dist_quick(Hit_pos, obj->pos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, robot_gun_number::_0);

			aip->GOAL_STATE = AIS_RECO;
			ailp.goal_state[aip->CURRENT_GUN] = AIS_RECO;
		}
			// Switch to next gun for next fire.
		aip->CURRENT_GUN = robot_advance_current_gun(robptr, aip->CURRENT_GUN);
	}
}
#elif defined(DXX_BUILD_DESCENT_II)

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if player is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
static void ai_do_actual_firing_stuff(const d_robot_info_array &Robot_info, fvmobjptridx &vmobjptridx, const vmobjptridx_t obj, ai_static *const aip, ai_local &ailp, const robot_info &robptr, const fix dist_to_player, robot_gun_point &gun_point, const robot_to_player_visibility_state &player_visibility, const int object_animates, const player_info &player_info, const robot_gun_number gun_num)
{
	auto &vec_to_player = player_visibility.vec_to_player;
	fix	dot;

	if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view || Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD)
	{
		vms_vector	fire_pos;

		fire_pos = Believed_player_pos;

		//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
		//	So, fire at Last_fired_upon_player_pos instead of the player position.
		if (!robptr.attack_type && player_visibility.visibility != player_visibility_state::visible_and_in_field_of_view)
			fire_pos = Last_fired_upon_player_pos;

		//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
		//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
		if (!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) {
			dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
			if ((dot >= 7*F1_0/8) || ((dot > F1_0/4) && robptr.boss_flag != boss_robot_id::None))
			{
				if (robot_gun_number_valid(robptr, gun_num))
				{
					if (robptr.attack_type == 1) {
						if (Player_dead_state != player_dead_state::exploded &&
							dist_to_player < obj->size + ConsoleObject->size + F1_0*2)
						{
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(Robot_info, obj, vmobjptridx(ConsoleObject), obj->pos);
						} else {
							return;
						}
					} else {
						if (!gun_point.has_value()) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping away...)
							if (ready_to_fire_weapon1(ailp, 0)) {
								ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, gun_num, fire_pos);
								Last_fired_upon_player_pos = fire_pos;
							}
							if (gun_num != robot_gun_number::_0)
							{
								if (ready_to_fire_weapon2(robptr, ailp, 0)) {
									ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, robot_gun_number::_0, fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}
							}
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aip->behavior != ai_behavior::AIB_RUN_FROM)
						 && (aip->behavior != ai_behavior::AIB_STILL)
						 && (aip->behavior != ai_behavior::AIB_SNIPE)
						 && (aip->behavior != ai_behavior::AIB_FOLLOW)
						 && (!robptr.attack_type)
						 && (ailp.mode == ai_mode::AIM_FOLLOW_PATH || ailp.mode == ai_mode::AIM_STILL))
						ailp.mode = ai_mode::AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp.goal_state[aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.  If has 2 gun types, select gun #1, if exists.
				aip->CURRENT_GUN = robot_advance_current_gun_prefer_second(robptr, aip->CURRENT_GUN);
			}
		}
	}
	else if ((!robptr.attack_type && Weapon_info[Robot_info[get_robot_id(obj)].weapon_type].homing_flag) || ((Robot_info[get_robot_id(obj)].weapon_type2 != weapon_none && Weapon_info[Robot_info[get_robot_id(obj)].weapon_type2].homing_flag)))
	{
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if ((!object_animates || ailp.achieved_state[aip->CURRENT_GUN] == AIS_FIRE)
			&& (
				(aip->CURRENT_GUN != robot_gun_number::_0 && ready_to_fire_weapon1(ailp, 0)) ||
				(aip->CURRENT_GUN == robot_gun_number::_0 && ready_to_fire_weapon2(robptr, ailp, 0)))
			 && (vm_vec_dist_quick(Hit_pos, obj->pos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
				return;
			robot_gun_point gun_point;
			ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, gun_num, Believed_player_pos);

			aip->GOAL_STATE = AIS_RECO;
			ailp.goal_state[aip->CURRENT_GUN] = AIS_RECO;
		}
			// Switch to next gun for next fire.
		aip->CURRENT_GUN = robot_advance_current_gun(robptr, aip->CURRENT_GUN);
	} else {
	//	---------------------------------------------------------------
		vms_vector	vec_to_last_pos;
		if (d_rand()/2 < fixmul(FrameTime, (underlying_value(GameUniqueState.Difficulty_level) << 12) + 0x4000)) {
		if ((!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) && (Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			vm_vec_normalized_dir_quick(vec_to_last_pos, Believed_player_pos, obj->pos);
			dot = vm_vec_dot(obj->orient.fvec, vec_to_last_pos);
			if (dot >= 7*F1_0/8) {
				if (robot_gun_number_valid(robptr, aip->CURRENT_GUN))
				{
					if (robptr.attack_type == 1) {
						if (Player_dead_state != player_dead_state::exploded &&
							dist_to_player < obj->size + ConsoleObject->size + F1_0*2)
						{
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(Robot_info, obj, vmobjptridx(ConsoleObject), obj->pos);
						} else {
							return;
						}
					} else {
						if (!gun_point.has_value()) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping away...)
							if (ready_to_fire_weapon1(ailp, 0))
							{
								ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, gun_num, Last_fired_upon_player_pos);
							}
							if (gun_num != robot_gun_number::_0)
							{
								if (ready_to_fire_weapon2(robptr, ailp, 0)) {
									ai_fire_laser_at_player(Robot_info, LevelSharedSegmentState, obj, player_info, gun_point, robot_gun_number::_0, Last_fired_upon_player_pos);
								}
							}
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if (aip->behavior != ai_behavior::AIB_RUN_FROM && aip->behavior != ai_behavior::AIB_STILL && aip->behavior != ai_behavior::AIB_SNIPE && aip->behavior != ai_behavior::AIB_FOLLOW && (ailp.mode == ai_mode::AIM_FOLLOW_PATH || ailp.mode == ai_mode::AIM_STILL))
						ailp.mode = ai_mode::AIM_CHASE_OBJECT;
				}
				aip->GOAL_STATE = AIS_RECO;
				ailp.goal_state[aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN = robot_advance_current_gun_prefer_second(robptr, aip->CURRENT_GUN);
			}
		}
		}

	//	---------------------------------------------------------------
	}
}

}

// ----------------------------------------------------------------------------
void init_ai_frame(const player_flags powerup_flags, const control_info &Controls)
{
	Dist_to_last_fired_upon_player_pos = vm_vec_dist_quick(Last_fired_upon_player_pos, Believed_player_pos);

	if (!(powerup_flags & PLAYER_FLAGS_CLOAKED) ||
		(powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON) ||
		(Afterburner_charge && Controls.state.afterburner && (powerup_flags & PLAYER_FLAGS_AFTERBURNER)))
	{
		ai_do_cloak_stuff();
	}
}

namespace {

// ----------------------------------------------------------------------------
// Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70
static void make_nearby_robot_snipe(fvmsegptr &vmsegptr, const object &robot, const robot_info &robptr, const player_flags powerup_flags)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	std::array<segnum_t, MNRS_SEG_MAX> bfs_list;
	/* Passing powerup_flags here seems wrong.  Sniping robots do not
	 * open doors, so they should not care what doors the player can
	 * open.  However, passing powerup_flags here maintains the
	 * semantics that past versions used.
	 */
	const auto bfs_length = create_bfs_list(robot, robptr, ConsoleObject->segnum, powerup_flags, bfs_list);

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	range_for (auto &i, partial_const_range(bfs_list, bfs_length)) {
		range_for (object &objp, objects_in(vmsegptr(i), vmobjptridx, vmsegptr))
		{
			object &obj = objp;
			if (obj.type != OBJ_ROBOT)
				continue;
			if (obj.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
				continue;
			if (obj.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM)
				continue;
			const auto rid = get_robot_id(obj);
			if (rid == robot_id::brain)
				continue;
			auto &robptr = Robot_info[rid];
			if (robptr.boss_flag == boss_robot_id::None && !robot_is_companion(robptr))
			{
					obj.ctype.ai_info.behavior = ai_behavior::AIB_SNIPE;
					obj.ctype.ai_info.ail.mode = ai_mode::AIM_SNIPE_ATTACK;
					return;
			}
		}
	}
}

static int openable_door_on_near_path(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, const object &obj, const ai_static &aip)
{
	const auto path_length = aip.path_length;
	if (path_length < 1)
		return 0;
	if (const auto r = openable_doors_in_segment(vcwallptr, vcsegptr(obj.segnum)).has_value())
		return r;
	if (path_length < 2)
		return 0;
	size_t idx;
	idx = aip.hide_index + aip.cur_path_index + aip.PATH_DIR;
	/* Hack: Point_segs[idx].segnum should never be none here, but
	 * sometimes the guidebot has a none.  Guard against the bogus none
	 * until someone can track down why the guidebot does this.
	 */
	if (idx < Point_segs.size() && Point_segs[idx].segnum != segment_none)
	{
		if (const auto r = openable_doors_in_segment(vcwallptr, vcsegptr(Point_segs[idx].segnum)).has_value())
			return r;
	}
	if (path_length < 3)
		return 0;
	idx = aip.hide_index + aip.cur_path_index + 2*aip.PATH_DIR;
	if (idx < Point_segs.size() && Point_segs[idx].segnum != segment_none)
	{
		if (const auto r = openable_doors_in_segment(vcwallptr, vcsegptr(Point_segs[idx].segnum)).has_value())
			return r;
	}
	return 0;
}

static unsigned guidebot_should_fire_flare(fvcobjptr &vcobjptr, fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, d_unique_buddy_state &BuddyState, const robot_info &robptr, const object &buddy_obj)
{
	auto &ais = buddy_obj.ctype.ai_info;
	auto &ail = ais.ail;
	if (const auto r = ready_to_fire_any_weapon(robptr, ail, 0))
	{
	}
	else
		return r;
	if (const auto r = openable_door_on_near_path(vcsegptr, vcwallptr, buddy_obj, ais))
		return r;
	if (ail.mode != ai_mode::AIM_GOTO_PLAYER)
		return 0;
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		/* should never happen */
		return 0;
	auto &plrobj = *vcobjptr(plr.objnum);
	if (plrobj.type != OBJ_PLAYER)
		return 0;
	vms_vector vec_to_controller;
	const auto dist_to_controller = vm_vec_normalized_dir_quick(vec_to_controller, plrobj.pos, buddy_obj.pos);
	if (dist_to_controller >= 3 * MIN_ESCORT_DISTANCE / 2)
		return 0;
	if (vm_vec_dot(plrobj.orient.fvec, vec_to_controller) <= -F1_0 / 4)
		return 0;
	return 1;
}
#endif

#ifdef NDEBUG
static bool is_break_object(vcobjidx_t)
{
	return false;
}
#else
__attribute_used
static objnum_t Break_on_object = object_none;

static bool is_break_object(const vcobjidx_t robot)
{
	return Break_on_object == robot;
}
#endif

static bool skip_ai_for_time_splice(const vcobjptridx_t robot, const robot_info &robptr, const vm_distance &dist_to_player)
{
	if (unlikely(is_break_object(robot)))
		// don't time slice if we're interested in this object.
		return false;
	const auto &aip = robot->ctype.ai_info;
	const auto &ailp = aip.ail;
#if defined(DXX_BUILD_DESCENT_I)
	(void)robptr;
	if (static_cast<uint8_t>(ailp.player_awareness_type) < static_cast<uint8_t>(player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION) - 1)
	{ // If robot got hit, he gets to attack player always!
		{
			if ((dist_to_player > F1_0*250) && (ailp.time_since_processed <= F1_0*2))
				return true;
			else if (!((aip.behavior == ai_behavior::AIB_STATION) && (ailp.mode == ai_mode::AIM_FOLLOW_PATH) && (aip.hide_segment != robot->segnum))) {
				if ((dist_to_player > F1_0*150) && (ailp.time_since_processed <= F1_0))
					return true;
				else if ((dist_to_player > F1_0*100) && (ailp.time_since_processed <= F1_0/2))
					return true;
			}
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (!((aip.behavior == ai_behavior::AIB_SNIPE) && (ailp.mode != ai_mode::AIM_SNIPE_WAIT)) && !robot_is_companion(robptr) && !robot_is_thief(robptr) && static_cast<uint8_t>(ailp.player_awareness_type) < static_cast<uint8_t>(player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION) - 1)
	{ // If robot got hit, he gets to attack player always!
		{
			if ((aip.behavior == ai_behavior::AIB_STATION) && (ailp.mode == ai_mode::AIM_FOLLOW_PATH) && (aip.hide_segment != robot->segnum)) {
				if (dist_to_player > F1_0*250)  // station guys not at home always processed until 250 units away.
					return true;
			}
			else if (!player_is_visible(ailp.previous_visibility) && ((dist_to_player >> 7) > ailp.time_since_processed))
			{  // 128 units away (6.4 segments) processed after 1 second.
				return true;
			}
		}
	}
#endif
	return false;
}

}

// --------------------------------------------------------------------------------------------------------------------
void do_ai_frame(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const vmobjptridx_t obj)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	const objnum_t &objnum = obj;
	ai_static	*const aip = &obj->ctype.ai_info;
	ai_local &ailp = obj->ctype.ai_info.ail;
	int			obj_ref;
	int			object_animates;
	vms_vector	vis_vec_pos;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;

#if defined(DXX_BUILD_DESCENT_II)
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &vcobjptr = Objects.vcptr;
	ailp.next_action_time -= FrameTime;
#endif

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
#if defined(DXX_BUILD_DESCENT_II)
		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {
			obj->mtype.phys_info.rotthrust.x = (obj->mtype.phys_info.rotthrust.x * 15)/16;
			obj->mtype.phys_info.rotthrust.y = (obj->mtype.phys_info.rotthrust.y * 15)/16;
			obj->mtype.phys_info.rotthrust.z = (obj->mtype.phys_info.rotthrust.z * 15)/16;
			if (!aip->SKIP_AI_COUNT)
				obj->mtype.phys_info.flags &= ~PF_USES_THRUST;
		}
#endif
		return;
	}

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Station = LevelUniqueFuelcenterState.Station;
#endif
	auto &robptr = Robot_info[get_robot_id(obj)];
	Assert(robptr.always_0xabcd == 0xabcd);

	if (do_any_robot_dying_frame(Robot_info, obj))
		return;

	// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	// Else, you can turn a big nasty robot into a wimp by firing flares at it.
	// This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ready_to_fire_any_weapon(robptr, ailp, 0)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if (aip->behavior == ai_behavior::AIB_RUN_FROM && ailp.mode != ai_mode::AIM_RUN_FROM_OBJECT)
		Int3(); // This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	if (Break_on_object != object_none)
		if ((obj) == Break_on_object)
			Int3(); // Contact Mike: This is a debug break
#endif

	//Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR));
	switch (aip->behavior)
	{
		case ai_behavior::AIB_STILL:
		case ai_behavior::AIB_NORMAL:
		case ai_behavior::AIB_RUN_FROM:
		case ai_behavior::AIB_STATION:
#if defined(DXX_BUILD_DESCENT_I)
		case ai_behavior::AIB_HIDE:
		case ai_behavior::AIB_FOLLOW_PATH:
#elif defined(DXX_BUILD_DESCENT_II)
		case ai_behavior::AIB_BEHIND:
		case ai_behavior::AIB_SNIPE:
		case ai_behavior::AIB_FOLLOW:
#endif
			break;
		default:
			aip->behavior = ai_behavior::AIB_NORMAL;
			break;
	}

	Assert(obj->segnum != segment_none);
	assert(underlying_value(get_robot_id(obj)) < LevelSharedRobotInfoState.N_robot_types);

	obj_ref = objnum ^ d_tick_count;

	if (!ready_to_fire_weapon1(ailp, -F1_0*8))
		ailp.next_fire -= FrameTime;

#if defined(DXX_BUILD_DESCENT_I)
	Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robptr.weapon_type2 != weapon_none) {
		if (!ready_to_fire_weapon2(robptr, ailp, -F1_0*8))
			ailp.next_fire2 -= FrameTime;
	} else
		ailp.next_fire2 = F1_0*8;
#endif

	if (ailp.time_since_processed < F1_0*256)
		ailp.time_since_processed += FrameTime;

	const auto previous_visibility = ailp.previous_visibility;    //  Must get this before we toast the master copy!

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	robot_to_player_visibility_state player_visibility;
	auto &vec_to_player = player_visibility.vec_to_player;
#if defined(DXX_BUILD_DESCENT_I)
	if (!(player_info.powerup_flags & PLAYER_FLAGS_CLOAKED))
		Believed_player_pos = ConsoleObject->pos;
#elif defined(DXX_BUILD_DESCENT_II)
	// If only awake because of a camera, make that the believed player position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && Ai_last_missile_camera)
		Believed_player_pos = Ai_last_missile_camera->pos;
	else {
		if (cheats.robotskillrobots) {
			vis_vec_pos = obj->pos;
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
			if (player_is_visible(player_visibility.visibility))
			{
				icobjptr_t min_obj = nullptr;
				fix min_dist = F1_0*200, cur_dist;

				range_for (const auto &&objp, vcobjptr)
				{
					if (objp->type == OBJ_ROBOT && objp != obj)
					{
						cur_dist = vm_vec_dist_quick(obj->pos, objp->pos);
						if (cur_dist < F1_0*100)
							if (object_to_object_visibility(obj, objp, FQ_TRANSWALL))
								if (cur_dist < min_dist) {
									min_obj = objp;
									min_dist = cur_dist;
								}
					}
				}

				if (min_obj != nullptr)
				{
					Believed_player_pos = min_obj->pos;
					Believed_player_seg = min_obj->segnum;
					vm_vec_normalized_dir_quick(vec_to_player, Believed_player_pos, obj->pos);
				} else
					goto _exit_cheat;
			} else
				goto _exit_cheat;
		} else {
_exit_cheat:
			DXX_MAKE_VAR_UNDEFINED(player_visibility);
			player_visibility.initialized = 0;
			if (!(player_info.powerup_flags & PLAYER_FLAGS_CLOAKED))
				Believed_player_pos = ConsoleObject->pos;
			else
				Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;
		}
	}
#endif
	auto dist_to_player = vm_vec_dist_quick(Believed_player_pos, obj->pos);

	// If this robot can fire, compute visibility from gun position.
	// Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	robot_gun_point gun_point;
#if defined(DXX_BUILD_DESCENT_I)
	if (ready_to_fire_any_weapon(robptr, ailp, 0) && (dist_to_player < F1_0*200) && (robptr.n_guns) && !(robptr.attack_type))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((player_is_visible(previous_visibility) || !(obj_ref & 3)) && ready_to_fire_any_weapon(robptr, ailp, 0) && (dist_to_player < F1_0*200) && (robptr.n_guns) && !(robptr.attack_type))
#endif
	{
		// Since we passed ready_to_fire_any_weapon(), either next_fire or next_fire2 <= 0.  calc_gun_point from relevant one.
		// If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
		const auto gun_num =
#if defined(DXX_BUILD_DESCENT_II)
			(!ready_to_fire_weapon1(ailp, 0))
			? robot_gun_number::_0
			:
#endif
			aip->CURRENT_GUN;
		vis_vec_pos = gun_point.build(robptr, obj, gun_num);
	} else {
		vis_vec_pos = obj->pos;
	}

	switch (const auto boss_flag = robptr.boss_flag) {
		case boss_robot_id::None:
			break;
			
		case boss_robot_id::d1_superboss:
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
			
			{
				auto pv = player_visibility.visibility;
			auto dtp = dist_to_player/4;
			
			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
				pv = player_visibility_state::no_line_of_sight;
				dtp = vm_vec_dist_quick(ConsoleObject->pos, obj->pos)/4;
			}
			
			do_super_boss_stuff(Robot_info, vmsegptridx, obj, dtp, pv);
		}
			break;
			
		default:
#if defined(DXX_BUILD_DESCENT_I)
			(void)boss_flag;
			Int3();	//	Bogus boss flag value.
			break;
#endif
		case boss_robot_id::d1_1:
		{
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
			
#if defined(DXX_BUILD_DESCENT_II)
			auto pv = player_visibility.visibility;
			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
				pv = player_visibility_state::no_line_of_sight;
			}

			if (boss_flag != boss_robot_id::d1_1)
			{
				do_d2_boss_stuff(vmsegptridx, obj, pv);
				break;
			}
#endif
			do_d1_boss_stuff(Robot_info, vmsegptridx, obj, pv);
		}
			break;
	}
	
	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - 
	// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
#if defined(DXX_BUILD_DESCENT_I)
	if ((aip->behavior != ai_behavior::AIB_RUN_FROM) && (aip->behavior != ai_behavior::AIB_STILL) && !(Game_mode & GM_MULTI))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((aip->behavior != ai_behavior::AIB_SNIPE) && (aip->behavior != ai_behavior::AIB_RUN_FROM) && (aip->behavior != ai_behavior::AIB_STILL) && !(Game_mode & GM_MULTI) && (robot_is_companion(robptr) != 1) && (robot_is_thief(robptr) != 1))
#endif
		if (Overall_agitation > 70) {
			if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/4)) {
				if (d_rand() * (Overall_agitation - 40) > F1_0*5) {
					create_path_to_believed_player_segment(obj, robptr, 4 + Overall_agitation/8 + underlying_value(Difficulty_level), create_path_safety_flag::safe);
					return;
				}
			}
		}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If retry count not 0, then add it into consecutive_retries.
	// If it is 0, cut down consecutive_retries.
	// This is largely a hack to speed up physics and deal with stupid
	// AI.  This is low level communication between systems of a sort
	// that should not be done.
	if (ailp.retry_count && !(Game_mode & GM_MULTI))
	{
		ailp.consecutive_retries += ailp.retry_count;
		ailp.retry_count = 0;
		if (ailp.consecutive_retries > 3)
		{
			switch (ailp.mode) {
#if defined(DXX_BUILD_DESCENT_II)
				case ai_mode::AIM_GOTO_PLAYER:
					move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
					create_path_to_guidebot_player_segment(obj, robptr, 100, create_path_safety_flag::safe);
					break;
				case ai_mode::AIM_GOTO_OBJECT:
					BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
					break;
#endif
				case ai_mode::AIM_CHASE_OBJECT:
					create_path_to_believed_player_segment(obj, robptr, 4 + Overall_agitation/8 + underlying_value(Difficulty_level), create_path_safety_flag::safe);
					break;
				case ai_mode::AIM_STILL:
#if defined(DXX_BUILD_DESCENT_I)
					if (!((aip->behavior == ai_behavior::AIB_STILL) || (aip->behavior == ai_behavior::AIB_STATION)))	//	Behavior is still, so don't follow path.
#elif defined(DXX_BUILD_DESCENT_II)
					if (robptr.attack_type)
						move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
					else if (!((aip->behavior == ai_behavior::AIB_STILL) || (aip->behavior == ai_behavior::AIB_STATION) || (aip->behavior == ai_behavior::AIB_FOLLOW)))    // Behavior is still, so don't follow path.
#endif
						attempt_to_resume_path(Robot_info, obj);
					break;
				case ai_mode::AIM_FOLLOW_PATH:
					if (Game_mode & GM_MULTI)
						ailp.mode = ai_mode::AIM_STILL;
					else
						attempt_to_resume_path(Robot_info, obj);
					break;
				case ai_mode::AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
					obj->mtype.phys_info.velocity = {};
					create_n_segment_path(obj, robptr, 5, segment_none);
					ailp.mode = ai_mode::AIM_RUN_FROM_OBJECT;
					break;
#if defined(DXX_BUILD_DESCENT_I)
				case ai_mode::AIM_HIDE:
					move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
					obj->mtype.phys_info.velocity = {};
					if (Overall_agitation > (50 - underlying_value(Difficulty_level) * 4))
						create_path_to_believed_player_segment(obj, robptr, 4 + Overall_agitation/8, create_path_safety_flag::safe);
					else {
						create_n_segment_path(obj, robptr, 5, segment_none);
					}
					break;
#elif defined(DXX_BUILD_DESCENT_II)
				case ai_mode::AIM_BEHIND:
					move_towards_segment_center(robptr, LevelSharedSegmentState, obj);
					obj->mtype.phys_info.velocity = {};
					break;
				case ai_mode::AIM_SNIPE_ATTACK:
				case ai_mode::AIM_SNIPE_FIRE:
				case ai_mode::AIM_SNIPE_RETREAT:
				case ai_mode::AIM_SNIPE_RETREAT_BACKWARDS:
				case ai_mode::AIM_SNIPE_WAIT:
				case ai_mode::AIM_THIEF_ATTACK:
				case ai_mode::AIM_THIEF_RETREAT:
				case ai_mode::AIM_THIEF_WAIT:
					break;
#endif
				case ai_mode::AIM_OPEN_DOOR:
					create_n_segment_path_to_door(obj, robptr, 5);
					break;
				case ai_mode::AIM_FOLLOW_PATH_2:
					Int3(); // Should never happen!
					break;
				case ai_mode::AIM_WANDER:
					break;
			}
			ailp.consecutive_retries = 0;
		}
	} else
		ailp.consecutive_retries /= 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If in materialization center, exit
	if (!(Game_mode & GM_MULTI))
	{
		const shared_segment &seg = *vcsegptr(obj->segnum);
		if (seg.special == segment_special::robotmaker)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (Station[seg.station_idx].Enabled)
#endif
		{
			ai_follow_path(Robot_info, obj, player_visibility_state::visible_not_in_field_of_view, nullptr);    // 1 = player is visible, which might be a lie, but it works.
			return;
		}
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Decrease player awareness due to the passage of time.
	if (ailp.player_awareness_type != player_awareness_type_t::PA_NONE)
	{
		if (ailp.player_awareness_time > 0)
		{
			ailp.player_awareness_time -= FrameTime;
			if (ailp.player_awareness_time <= 0)
			{
				ailp.player_awareness_time = F1_0*2;   //new: 11/05/94
				ailp.player_awareness_type = static_cast<player_awareness_type_t>(static_cast<unsigned>(ailp.player_awareness_type) - 1);          //new: 11/05/94
			}
		} else {
			ailp.player_awareness_time = F1_0*2;
			ailp.player_awareness_type = static_cast<player_awareness_type_t>(static_cast<unsigned>(ailp.player_awareness_type) - 1);
			//aip->GOAL_STATE = AIS_REST;
		}
	} else
		aip->GOAL_STATE = AIS_REST;                     //new: 12/13/94


	if (Player_dead_state != player_dead_state::no &&
		ailp.player_awareness_type == player_awareness_type_t::PA_NONE)
		if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/8)) {
			if ((aip->behavior != ai_behavior::AIB_STILL) && (aip->behavior != ai_behavior::AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(obj, 30))
					return;
				ai_multi_send_robot_position(obj, -1);

				if (!(ailp.mode == ai_mode::AIM_FOLLOW_PATH && aip->cur_path_index < aip->path_length - 1))
#if defined(DXX_BUILD_DESCENT_II)
					if ((aip->behavior != ai_behavior::AIB_SNIPE) && (aip->behavior != ai_behavior::AIB_RUN_FROM))
#endif
					{
						if (dist_to_player < F1_0*30)
							create_n_segment_path(obj, robptr, 5, segment_none);
						else
							create_path_to_believed_player_segment(obj, robptr, 20, create_path_safety_flag::safe);
					}
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing player.
	if (ailp.player_awareness_type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION || ailp.player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION)
#if defined(DXX_BUILD_DESCENT_I)
	{
		if (aip->behavior != ai_behavior::AIB_STILL && aip->behavior != ai_behavior::AIB_FOLLOW_PATH && aip->behavior != ai_behavior::AIB_RUN_FROM && get_robot_id(obj) != robot_id::brain)
			ailp.mode = ai_mode::AIM_CHASE_OBJECT;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	{
		compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
		if (player_visibility.visibility == player_visibility_state::visible_not_in_field_of_view) // Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility.visibility = player_visibility_state::visible_and_in_field_of_view;
	} else if (((obj_ref&3) == 0) && !player_is_visible(previous_visibility) && (dist_to_player < F1_0*100)) {
		fix sval, rval;

		rval = d_rand();
		sval = (dist_to_player * (static_cast<int>(Difficulty_level) + 1)) / 64;

		if ((fixmul(rval, sval) < FrameTime) || (player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp.player_awareness_type = player_awareness_type_t::PA_PLAYER_COLLISION;
			ailp.player_awareness_time = F1_0*3;
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
			if (player_visibility.visibility == player_visibility_state::visible_not_in_field_of_view)
				player_visibility.visibility = player_visibility_state::visible_and_in_field_of_view;
		}
	}
#endif

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aip->GOAL_STATE == AIS_FLIN) && (aip->CURRENT_STATE == AIS_FLIN))
		aip->GOAL_STATE = AIS_LOCK;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Note: Should only do these two function calls for objects which animate
	if (dist_to_player < F1_0*100) { // && !(Game_mode & GM_MULTI))
		object_animates = do_silly_animation(Robot_info, obj);
		if (object_animates)
			ai_frame_animation(obj);
	} else {
		// If Object is supposed to animate, but we don't let it animate due to distance, then
		// we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Time-slice, don't process all the time, purely an efficiency hack.
	// Guys whose behavior is station and are not at their hide segment get processed anyway.
	if (skip_ai_for_time_splice(obj, robptr, dist_to_player))
		return;

	// Reset time since processed, but skew objects so not everything
	// processed synchronously, else we get fast frames with the
	// occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp.time_since_processed = - ((objnum & 0x03) * FrameTime ) / 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	//	Perform special ability
	switch (get_robot_id(obj)) {
		case robot_id::brain:
			// Robots function nicely if behavior is Station.  This
			// means they won't move until they can see the player, at
			// which time they will start wandering about opening doors.
			if (ConsoleObject->segnum == obj->segnum) {
				if (!ai_multiplayer_awareness(obj, 97))
					return;
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
				move_away_from_player(Robot_info, obj, vec_to_player, 0);
				ai_multi_send_robot_position(obj, -1);
			}
			else if (ailp.mode != ai_mode::AIM_STILL)
			{
				auto &Walls = LevelUniqueWallSubsystemState.Walls;
				auto &vcwallptr = Walls.vcptr;
				if (const auto r = openable_doors_in_segment(vcwallptr, vcsegptr(obj->segnum)); r.has_value())
				{
					ailp.mode = ai_mode::AIM_OPEN_DOOR;
					aip->GOALSIDE = *r;
				}
				else if (ailp.mode != ai_mode::AIM_FOLLOW_PATH)
				{
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, robptr, 8 + underlying_value(Difficulty_level));
					ai_multi_send_robot_position(obj, -1);
				}

#if defined(DXX_BUILD_DESCENT_II)
				if (ailp.next_action_time < 0)
				{
					compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
					if (player_is_visible(player_visibility.visibility))
					{
						const auto powerup_flags = player_info.powerup_flags;
						make_nearby_robot_snipe(vmsegptr, obj, robptr, powerup_flags);
						ailp.next_action_time = (NDL - underlying_value(Difficulty_level)) * 2*F1_0;
					}
				}
#endif
			} else {
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
				if (player_is_visible(player_visibility.visibility))
				{
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, robptr, 8 + underlying_value(Difficulty_level));
					ai_multi_send_robot_position(obj, -1);
				}
			}
			break;
		default:
			break;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (aip->behavior == ai_behavior::AIB_SNIPE) {
		if ((Game_mode & GM_MULTI) && !robot_is_thief(robptr)) {
			aip->behavior = ai_behavior::AIB_NORMAL;
			ailp.mode = ai_mode::AIM_CHASE_OBJECT;
			return;
		}

		if (!(obj_ref & 3) || player_is_visible(previous_visibility))
		{
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

			// If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
			if (ailp.mode == ai_mode::AIM_STILL)
				if (player_is_visible(player_visibility.visibility) || ailp.player_awareness_type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION)
					ailp.mode = ai_mode::AIM_SNIPE_ATTACK;

			if (!robot_is_thief(robptr) && ailp.mode != ai_mode::AIM_STILL)
				do_snipe_frame(obj, robptr, dist_to_player, player_visibility.visibility, vec_to_player);
		} else if (!robot_is_thief(robptr) && !robot_is_companion(robptr))
			return;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	// More special ability stuff, but based on a property of a robot, not its ID.
	if (robot_is_companion(robptr)) {
		compute_buddy_vis_vec(obj, obj->pos, player_visibility, robptr);
		auto &player_controlling_guidebot = get_player_controlling_guidebot(BuddyState, Players);
		if (player_controlling_guidebot.objnum != object_none)
		{
			auto &plrobj_controlling_guidebot = *Objects.vcptr(player_controlling_guidebot.objnum);
			do_escort_frame(obj, robptr, plrobj_controlling_guidebot, player_visibility.visibility);
		}

		if (obj->ctype.ai_info.danger_laser_num != object_none) {
			auto &dobjp = *vcobjptr(obj->ctype.ai_info.danger_laser_num);
			if ((dobjp.type == OBJ_WEAPON) && (dobjp.signature == obj->ctype.ai_info.danger_laser_signature))
			{
				fix circle_distance;
				circle_distance = robptr.circle_distance[Difficulty_level] + ConsoleObject->size;
				ai_move_relative_to_player(Robot_info, obj, ailp, dist_to_player, circle_distance, 1, player_visibility, player_info);
			}
		}

		if (guidebot_should_fire_flare(vcobjptr, vcsegptr, vcwallptr, BuddyState, robptr, *obj))
		{
			Laser_create_new_easy(Robot_info, obj->orient.fvec, obj->pos, obj, weapon_id_type::FLARE_ID, weapon_sound_flag::audible);
				ailp.next_fire = F1_0/2;
				if (!BuddyState.Buddy_allowed_to_talk) // If buddy not talking, make him fire flares less often.
					ailp.next_fire += d_rand()*4;
		}
	}

	if (robot_is_thief(robptr)) {

		compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
		do_thief_frame(obj, robptr, dist_to_player, player_visibility.visibility, vec_to_player);

		if (ready_to_fire_any_weapon(robptr, ailp, 0)) {
			if (openable_door_on_near_path(vmsegptr, vcwallptr, *obj, *aip))
			{
				// @mk, 05/08/95: Firing flare from center of object, this is dumb...
				Laser_create_new_easy(Robot_info, obj->orient.fvec, obj->pos, obj, weapon_id_type::FLARE_ID, weapon_sound_flag::audible);
				ailp.next_fire = F1_0/2;
				if (LevelUniqueObjectState.ThiefState.Stolen_item_index == 0)     // If never stolen an item, fire flares less often (bad: Stolen_item_index wraps, but big deal)
					ailp.next_fire += d_rand()*4;
			}
		}
	}
#endif

	switch (ailp.mode)
	{
		case ai_mode::AIM_CHASE_OBJECT: {        // chasing player, sort of, chase if far, back off if close, circle in between
			fix circle_distance;

			circle_distance = robptr.circle_distance[Difficulty_level] + ConsoleObject->size;
			// Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robptr.attack_type != 1)
				circle_distance += (objnum&0xf) * F1_0/2;

			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

			// @mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if (player_visibility.visibility != player_visibility_state::visible_and_in_field_of_view && previous_visibility == player_visibility_state::visible_and_in_field_of_view)
			{ // this is redundant: mk, 01/15/95: && (ailp->mode == ai_mode::AIM_CHASE_OBJECT))
				if (!ai_multiplayer_awareness(obj, 53)) {
					if (maybe_ai_do_actual_firing_stuff(obj))
						ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
					return;
				}
				create_path_to_believed_player_segment(obj, robptr, 8, create_path_safety_flag::safe);
				ai_multi_send_robot_position(obj, -1);
			} else if (!player_is_visible(player_visibility.visibility) && dist_to_player > F1_0 * 80 && !(Game_mode & GM_MULTI))
			{
				// If pretty far from the player, player cannot be seen
				// (obstructed) and in chase mode, switch to follow path mode.
				// This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == ai_behavior::AIB_STATION) {
					ailp.goal_segment = aip->hide_segment;
					create_path_to_station(obj, robptr, 15);
				} // -- this looks like a dumb thing to do...robots following paths far away from you! else create_n_segment_path(obj, 5, -1);
#if defined(DXX_BUILD_DESCENT_I)
				else
					create_n_segment_path(obj, robptr, 5, segment_none);
#endif
				break;
			}

			if ((aip->CURRENT_STATE == AIS_REST) && (aip->GOAL_STATE == AIS_REST)) {
				const auto pv = player_visibility.visibility;
				if (player_is_visible(pv))
				{
					const auto upv = static_cast<unsigned>(pv);
					if (d_rand() < FrameTime * upv)
						if (dist_to_player/256 < static_cast<fix>(d_rand() * upv))
						{
							aip->GOAL_STATE = AIS_SRCH;
							aip->CURRENT_STATE = AIS_SRCH;
						}
				}
			}

			if (GameTime64 - ailp.time_player_seen > CHASE_TIME_LENGTH)
			{

				if (Game_mode & GM_MULTI)
				{
					if (!player_is_visible(player_visibility.visibility) && dist_to_player > F1_0 * 70)
					{
						ailp.mode = ai_mode::AIM_STILL;
						return;
					}
				}

				if (!ai_multiplayer_awareness(obj, 64)) {
					if (maybe_ai_do_actual_firing_stuff(obj))
						ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
					return;
				}
#if defined(DXX_BUILD_DESCENT_I)
				create_path_to_believed_player_segment(obj, robptr, 10, create_path_safety_flag::safe);
				ai_multi_send_robot_position(obj, -1);
#endif
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!ai_multiplayer_awareness(obj, 70)) {
					if (maybe_ai_do_actual_firing_stuff(obj))
						ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
					return;
				}
				ai_move_relative_to_player(Robot_info, obj, ailp, dist_to_player, circle_distance, 0, player_visibility, player_info);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_is_visible(player_visibility.visibility))
						ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
#if defined(DXX_BUILD_DESCENT_I)
					else
						ai_turn_randomly(vec_to_player, obj, robptr.turn_time[Difficulty_level], previous_visibility);
#endif
				}

				ai_multi_send_robot_position(obj, ai_evaded ? (ai_evaded = 0, 1) : -1);

				do_firing_stuff(obj, player_info.powerup_flags, player_visibility);
			}
			break;
		}

		case ai_mode::AIM_RUN_FROM_OBJECT:
			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

			{
				const auto pv = player_visibility.visibility;
				const auto player_is_in_line_of_sight = player_is_visible(pv);
				if (player_is_in_line_of_sight)
				{
				if (ailp.player_awareness_type == player_awareness_type_t::PA_NONE)
					ailp.player_awareness_type = player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION;
				}

			// If in multiplayer, only do if player visible.  If not multiplayer, do always.
				if (!(Game_mode & GM_MULTI) || player_is_in_line_of_sight)
				if (ai_multiplayer_awareness(obj, 75)) {
					ai_follow_path(Robot_info, obj, player_visibility.visibility, &vec_to_player);
					ai_multi_send_robot_position(obj, -1);
				}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			// Bad to let run_from robot fire at player because it
			// will cause a war in which it turns towards the player
			// to fire and then towards its goal to move.
			// do_firing_stuff(obj, player_visibility, &vec_to_player);
			// Instead, do this:
			// (Note, only drop if player is visible.  This prevents
			// the bombs from being a giveaway, and also ensures that
			// the robot is moving while it is dropping.  Also means
			// fewer will be dropped.)
			if (ready_to_fire_weapon1(ailp, 0) && player_is_in_line_of_sight)
			{
				if (!ai_multiplayer_awareness(obj, 75))
					return;

				const auto fire_vec = vm_vec_negated(obj->orient.fvec);
				const auto fire_pos = vm_vec_add(obj->pos, fire_vec);

#if defined(DXX_BUILD_DESCENT_I)
				ailp.next_fire = F1_0*5;		//	Drop a proximity bomb every 5 seconds.
				const auto weapon_id =
#elif defined(DXX_BUILD_DESCENT_II)
				ailp.next_fire = (F1_0 / 2) * (NDL + 5 - underlying_value(Difficulty_level));      // Drop a proximity bomb every 5 seconds.
				const auto weapon_id = (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					? weapon_id_type::ROBOT_SUPERPROX_ID
					:
#endif
					weapon_id_type::PROXIMITY_ID;
				Laser_create_new_easy(Robot_info, fire_vec, fire_pos, obj, weapon_id, weapon_sound_flag::audible);

				if (Game_mode & GM_MULTI)
				{
					ai_multi_send_robot_position(obj, -1);
					const auto gun_num =
#if defined(DXX_BUILD_DESCENT_II)
						(aip->SUB_FLAGS & SUB_FLAGS_SPROX)
						? robot_gun_number::smart_mine
						:
#endif
					robot_gun_number::proximity;
					multi_send_robot_fire(obj, gun_num, fire_vec);
				}
			}
			}
			break;

#if defined(DXX_BUILD_DESCENT_II)
		case ai_mode::AIM_GOTO_PLAYER:
		case ai_mode::AIM_GOTO_OBJECT:
			ai_follow_path(Robot_info, obj, player_visibility_state::visible_and_in_field_of_view, &vec_to_player);    // Follows path as if player can see robot.
			ai_multi_send_robot_position(obj, -1);
			break;
#endif

		case ai_mode::AIM_FOLLOW_PATH: {
			int anger_level = 65;

			if (aip->behavior == ai_behavior::AIB_STATION)
			{
				const std::size_t idx = aip->hide_index + aip->path_length - 1;
				if (idx < Point_segs.size() && Point_segs[idx].segnum == aip->hide_segment)
				{
					anger_level = 64;
				}
			}

			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

			if (!ai_multiplayer_awareness(obj, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(obj)) {
					ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
				}
				return;
			}

			ai_follow_path(Robot_info, obj, player_visibility.visibility, &vec_to_player);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

#if defined(DXX_BUILD_DESCENT_I)
			if ((aip->behavior != ai_behavior::AIB_FOLLOW_PATH) && (aip->behavior != ai_behavior::AIB_RUN_FROM))
				do_firing_stuff(obj, player_info.powerup_flags, player_visibility);

			if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view && aip->behavior != ai_behavior::AIB_FOLLOW_PATH && aip->behavior != ai_behavior::AIB_RUN_FROM && get_robot_id(obj) != robot_id::brain)
			{
				if (robptr.attack_type == 0)
					ailp.mode = ai_mode::AIM_CHASE_OBJECT;
			}
			else if (!player_is_visible(player_visibility.visibility)
				&& (aip->behavior == ai_behavior::AIB_NORMAL)
				&& (ailp.mode == ai_mode::AIM_FOLLOW_PATH)
				&& (aip->behavior != ai_behavior::AIB_RUN_FROM))
			{
				ailp.mode = ai_mode::AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}
#elif defined(DXX_BUILD_DESCENT_II)
			if (aip->behavior != ai_behavior::AIB_RUN_FROM)
				do_firing_stuff(obj, player_info.powerup_flags, player_visibility);

			if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view &&
				aip->behavior != ai_behavior::AIB_SNIPE &&
				aip->behavior != ai_behavior::AIB_FOLLOW &&
				aip->behavior != ai_behavior::AIB_RUN_FROM &&
				get_robot_id(obj) != robot_id::brain &&
				robot_is_companion(robptr) != 1 &&
				robot_is_thief(robptr) != 1)
			{
				if (robptr.attack_type == 0)
					ailp.mode = ai_mode::AIM_CHASE_OBJECT;
				// This should not just be distance based, but also time-since-player-seen based.
			}
			else if ((dist_to_player > F1_0 * (20 * (2 * underlying_value(Difficulty_level) + robptr.pursuit)))
				&& (GameTime64 - ailp.time_player_seen > (F1_0 / 2 * (underlying_value(Difficulty_level) + robptr.pursuit)))
				&& !player_is_visible(player_visibility.visibility)
				&& (aip->behavior == ai_behavior::AIB_NORMAL)
				&& (ailp.mode == ai_mode::AIM_FOLLOW_PATH))
			{
				ailp.mode = ai_mode::AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}
#endif

			ai_multi_send_robot_position(obj, -1);

			break;
		}

#if defined(DXX_BUILD_DESCENT_I)
		case ai_mode::AIM_HIDE:
#elif defined(DXX_BUILD_DESCENT_II)
		case ai_mode::AIM_BEHIND:
#endif
			if (!ai_multiplayer_awareness(obj, 71)) {
				if (maybe_ai_do_actual_firing_stuff(obj)) {
					compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
					ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
				}
				return;
			}

			compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

#if defined(DXX_BUILD_DESCENT_I)
			ai_follow_path(Robot_info, obj, player_visibility.visibility, nullptr);
#elif defined(DXX_BUILD_DESCENT_II)
			if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
			{
				// Get behind the player.
				// Method:
				// If vec_to_player dot player_rear_vector > 0, behind is goal.
				// Else choose goal with larger dot from left, right.
				vms_vector  goal_vector;
				fix         dot;

				dot = vm_vec_dot(ConsoleObject->orient.fvec, vec_to_player);
				if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
					goal_vector = vm_vec_negated(ConsoleObject->orient.fvec);
				} else {
					auto &orient = ConsoleObject->orient;
					constexpr unsigned choice_count = 15;
					const unsigned selector = (ConsoleObject->id ^ obj.get_unchecked_index() ^ ConsoleObject->segnum ^ obj->segnum) % (choice_count + 1);
					if (selector == 0)
						goal_vector = orient.rvec;
					else if (selector == choice_count)
						goal_vector = orient.uvec;
					else
					{
						vm_vec_scale_add2(goal_vector, orient.rvec, (choice_count - selector) * F1_0);
						vm_vec_copy_scale(goal_vector, orient.uvec, selector * F1_0);
						vm_vec_normalize_quick(goal_vector);
					}
					if (vm_vec_dot(goal_vector, vec_to_player) > 0)
						vm_vec_negate(goal_vector);
				}

				vm_vec_scale(goal_vector, 2*(ConsoleObject->size + obj->size + (((objnum*4 + d_tick_count) & 63) << 12)));
				auto goal_point = vm_vec_add(ConsoleObject->pos, goal_vector);
				vm_vec_scale_add2(goal_point, make_random_vector(), F1_0*8);
				const auto vec_to_goal = vm_vec_normalized_quick(vm_vec_sub(goal_point, obj->pos));
				move_towards_vector(robptr, obj, vec_to_goal, 0);
				ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
				ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
			}
#endif

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_send_robot_position(obj, -1);
			break;

		case ai_mode::AIM_STILL:
			if ((dist_to_player < F1_0 * 120 + static_cast<int>(Difficulty_level) * F1_0 * 20) || (static_cast<unsigned>(ailp.player_awareness_type) >= static_cast<unsigned>(player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION) - 1))
			{
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

				// turn towards vector if visible this time or last time, or rand
				// new!
#if defined(DXX_BUILD_DESCENT_I)
				if (player_is_visible(player_visibility.visibility) || player_is_visible(previous_visibility) || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI)))
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view || previous_visibility == player_visibility_state::visible_and_in_field_of_view) // -- MK, 06/09/95:  || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI)))
#endif
				{
					if (!ai_multiplayer_awareness(obj, 71)) {
						if (maybe_ai_do_actual_firing_stuff(obj))
							ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
						return;
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}

				do_firing_stuff(obj, player_info.powerup_flags, player_visibility);
#if defined(DXX_BUILD_DESCENT_I)
				if (player_is_visible(player_visibility.visibility)) //	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view) // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
#endif
				{
					if (robptr.attack_type == 1) {
						aip->behavior = ai_behavior::AIB_NORMAL;
						if (!ai_multiplayer_awareness(obj, 80)) {
							if (maybe_ai_do_actual_firing_stuff(obj))
								ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(Robot_info, obj, ailp, dist_to_player, 0, 0, player_visibility, player_info);
						ai_multi_send_robot_position(obj, ai_evaded ? (ai_evaded = 0, 1) : -1);
					} else {
						// Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(obj, 81)) {
							if (maybe_ai_do_actual_firing_stuff(obj))
								ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(Robot_info, obj, ailp, dist_to_player, 0, 1, player_visibility, player_info);
						if (ai_evaded) {
							ai_multi_send_robot_position(obj, -1);
							ai_evaded = 0;
						}
						else
							ai_multi_send_robot_position(obj, -1);
					}
				} else if ((obj->segnum != aip->hide_segment) && (dist_to_player > F1_0*80) && (!(Game_mode & GM_MULTI))) {
					// If pretty far from the player, player cannot be
					// seen (obstructed) and in chase mode, switch to
					// follow path mode.
					// This has one desirable benefit of avoiding physics retries.
					if (aip->behavior == ai_behavior::AIB_STATION) {
						ailp.goal_segment = aip->hide_segment;
						create_path_to_station(obj, robptr, 15);
					}
					break;
				}
			}

			break;
		case ai_mode::AIM_OPEN_DOOR: {       // trying to open a door.
			assert(get_robot_id(obj) == robot_id::brain);     // Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(obj, 62))
				return;
			auto &vcvertptr = Vertices.vcptr;
			const auto &&center_point = compute_center_point_on_side(vcvertptr, vcsegptr(obj->segnum), aip->GOALSIDE);
			const auto goal_vector = vm_vec_normalized_quick(vm_vec_sub(center_point, obj->pos));
			ai_turn_towards_vector(goal_vector, obj, robptr.turn_time[Difficulty_level]);
			move_towards_vector(robptr, obj, goal_vector, 0);
			ai_multi_send_robot_position(obj, -1);

			break;
		}

#if defined(DXX_BUILD_DESCENT_II)
		case ai_mode::AIM_SNIPE_WAIT:
			break;
		case ai_mode::AIM_SNIPE_RETREAT:
			// -- if (ai_multiplayer_awareness(obj, 53))
			// -- 	if (ailp->next_fire < -F1_0)
			// -- 		ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			break;
		case ai_mode::AIM_SNIPE_RETREAT_BACKWARDS:
		case ai_mode::AIM_SNIPE_ATTACK:
		case ai_mode::AIM_SNIPE_FIRE:
			if (ai_multiplayer_awareness(obj, 53)) {
				ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
				if (robot_is_thief(robptr))
					ai_move_relative_to_player(Robot_info, obj, ailp, dist_to_player, 0, 0, player_visibility, player_info);
				break;
			}
			break;

		case ai_mode::AIM_THIEF_WAIT:
		case ai_mode::AIM_THIEF_ATTACK:
		case ai_mode::AIM_THIEF_RETREAT:
		case ai_mode::AIM_WANDER:    // Used for Buddy Bot
			break;
#endif

		default:
			ailp.mode = ai_mode::AIM_CHASE_OBJECT;
			break;
	}       // end: switch (ailp->mode)

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If the robot can see you, increase his awareness of you.
	// This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1); // Means it didn't get initialized!
	compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
	if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
	{
#if defined(DXX_BUILD_DESCENT_I)
		if (ailp.player_awareness_type == player_awareness_type_t::PA_NONE)
			ailp.player_awareness_type = player_awareness_type_t::PA_PLAYER_COLLISION;
#elif defined(DXX_BUILD_DESCENT_II)
	if (aip->behavior != ai_behavior::AIB_FOLLOW && !robot_is_thief(robptr))
	{
		if (ailp.player_awareness_type == player_awareness_type_t::PA_NONE && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp.player_awareness_type == player_awareness_type_t::PA_NONE)
			ailp.player_awareness_type = player_awareness_type_t::PA_PLAYER_COLLISION;
	}
#endif
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	assert(static_cast<unsigned>(ailp.player_awareness_type) <= AIE_MAX);
	Assert(aip->CURRENT_STATE < AIS_MAX);
	Assert(aip->GOAL_STATE < AIS_MAX);

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (ailp.player_awareness_type != player_awareness_type_t::PA_NONE) {
		const auto i = static_cast<unsigned>(ailp.player_awareness_type) - 1;
		if (ailp.player_awareness_type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION)
		{
			// Decrease awareness, else this robot will flinch every frame.
			ailp.player_awareness_type = player_awareness_type_t::PA_PLAYER_COLLISION;
			ailp.player_awareness_time = F1_0*3;
		}

		auto new_goal_state = (i < Ai_transition_table.size())
			? Ai_transition_table[i][aip->CURRENT_STATE][aip->GOAL_STATE]
			: AIS_REST;
		if (new_goal_state == AIS_ERR_)
			new_goal_state = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = new_goal_state;

	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If new state = fire, then set all gun states to fire.
	if (aip->GOAL_STATE == AIS_FIRE)
	{
		std::fill_n(ailp.goal_state.begin(), robptr.n_guns, AIS_FIRE);
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if (ready_to_fire_any_weapon(robptr, ailp, 0) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if (aip->GOAL_STATE != AIS_FLIN && get_robot_id(obj) != robot_id::brain)
	{
		switch (aip->CURRENT_STATE) {
			case AIS_NONE:
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

				{
				const fix dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
				if (dot >= F1_0/2)
					if (aip->GOAL_STATE == AIS_REST)
						aip->GOAL_STATE = AIS_SRCH;
				}
				break;
			case AIS_REST:
				if (aip->GOAL_STATE == AIS_REST) {
					compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
					if (ready_to_fire_any_weapon(robptr, ailp, 0) && player_is_visible(player_visibility.visibility))
					{
						aip->GOAL_STATE = AIS_FIRE;
					}
				}
				break;
			case AIS_SRCH:
				if (!ai_multiplayer_awareness(obj, 60))
					return;

				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

#if defined(DXX_BUILD_DESCENT_I)
				if (player_is_visible(player_visibility.visibility))
				{
					ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				} else if (!(Game_mode & GM_MULTI))
					ai_turn_randomly(vec_to_player, obj, robptr.turn_time[Difficulty_level], previous_visibility);
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
				{
					ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}
#endif
				break;
			case AIS_LOCK:
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

				if (!(Game_mode & GM_MULTI) || player_is_visible(player_visibility.visibility))
				{
					if (!ai_multiplayer_awareness(obj, 68))
						return;

#if defined(DXX_BUILD_DESCENT_I)
					if (player_is_visible(player_visibility.visibility))
					{
						ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
					else if (!(Game_mode & GM_MULTI))
						ai_turn_randomly(vec_to_player, obj, robptr.turn_time[Difficulty_level], previous_visibility);
#elif defined(DXX_BUILD_DESCENT_II)
					if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
					{   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
						ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
#endif
				}
				break;
			case AIS_FIRE:
				compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);

#if defined(DXX_BUILD_DESCENT_I)
				if (player_is_visible(player_visibility.visibility))
				{
					if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) 
					{
						if (Game_mode & GM_MULTI) {
							ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, robot_gun_number::_0);
							return;
						}
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				} else if (!(Game_mode & GM_MULTI)) {
					ai_turn_randomly(vec_to_player, obj, robptr.turn_time[Difficulty_level], previous_visibility);
				}
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
				{
					if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) {
						if (Game_mode & GM_MULTI) {
							ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);
							return;
						}
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}
#endif

				// Fire at player, if appropriate.
				ai_do_actual_firing_stuff(Robot_info, vmobjptridx, obj, aip, ailp, robptr, dist_to_player, gun_point, player_visibility, object_animates, player_info, aip->CURRENT_GUN);

				break;
			case AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec(Robot_info, obj, player_info, vis_vec_pos, ailp, player_visibility, robptr);
#if defined(DXX_BUILD_DESCENT_I)
					if (player_is_visible(player_visibility.visibility))
					{
						if (!ai_multiplayer_awareness(obj, 69))
							return;
						ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
					else if (!(Game_mode & GM_MULTI)) {
						ai_turn_randomly(vec_to_player, obj, robptr.turn_time[Difficulty_level], previous_visibility);
					}
#elif defined(DXX_BUILD_DESCENT_II)
					if (player_visibility.visibility == player_visibility_state::visible_and_in_field_of_view)
					{
						if (!ai_multiplayer_awareness(obj, 69))
							return;
						ai_turn_towards_vector(vec_to_player, obj, robptr.turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
#endif
				}
				break;
			case AIS_FLIN:
				break;
			default:
				aip->GOAL_STATE = AIS_REST;
				aip->CURRENT_STATE = AIS_REST;
				break;
		}
	} // end of: if (aip->GOAL_STATE != AIS_FLIN)

	// Switch to next gun for next fire.
	if (!player_is_visible(player_visibility.visibility))
	{
		aip->CURRENT_GUN = robot_advance_current_gun_prefer_second(robptr, aip->CURRENT_GUN);
	}

}

// ----------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	range_for (auto &i, Ai_cloak_info) {
		i.last_time = {GameTime64};
#if defined(DXX_BUILD_DESCENT_II)
		i.last_segment = ConsoleObject->segnum;
#endif
		i.last_position = ConsoleObject->pos;
	}

	// Make work for control centers.
	Believed_player_pos = Ai_cloak_info[0].last_position;
#if defined(DXX_BUILD_DESCENT_II)
	Believed_player_seg = Ai_cloak_info[0].last_segment;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the player starts a new ship.
void init_ai_for_ship()
{
	ai_do_cloak_stuff();
}

namespace {

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
static int add_awareness_event(const object_base &objp, player_awareness_type_t type, d_level_unique_robot_awareness_state &awareness)
{
	// If player cloaked and hit a robot, then increase awareness
	if (type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION ||
		type == player_awareness_type_t::PA_WEAPON_WALL_COLLISION ||
		type == player_awareness_type_t::PA_PLAYER_COLLISION)
		ai_do_cloak_stuff();

	if (awareness.Num_awareness_events < awareness.Awareness_events.size())
	{
		if (type == player_awareness_type_t::PA_WEAPON_WALL_COLLISION ||
			type == player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION)
			if (objp.type == OBJ_WEAPON && get_weapon_id(objp) == weapon_id_type::VULCAN_ID)
				if (d_rand() > 3276)
					return 0;       // For vulcan cannon, only about 1/10 actually cause awareness

		auto &e = awareness.Awareness_events[awareness.Num_awareness_events++];
		e.segnum = objp.segnum;
		e.pos = objp.pos;
		e.type = type;
	} else {
		//Int3();   // Hey -- Overflowed Awareness_events, make more or something
		// This just gets ignored, so you can just
		// continue.
	}
	return 1;

}

}

// ----------------------------------------------------------------------------------
// Robots will become aware of the player based on something that occurred.
// The object (probably player or weapon) which created the awareness is objp.
void create_awareness_event(object &objp, player_awareness_type_t type, d_level_unique_robot_awareness_state &LevelUniqueRobotAwarenessState)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
	{
		if (add_awareness_event(objp, type, LevelUniqueRobotAwarenessState))
		{
			if (((d_rand() * (static_cast<unsigned>(type) + 4)) >> 15) > 4)
				Overall_agitation++;
			if (Overall_agitation > OVERALL_AGITATION_MAX)
				Overall_agitation = OVERALL_AGITATION_MAX;
		}
	}
}

namespace {

// ----------------------------------------------------------------------------------
static unsigned process_awareness_events(fvcsegptridx &vcsegptridx, d_level_unique_robot_awareness_state &LevelUniqueRobotAwarenessState, awareness_t &New_awareness)
{
	unsigned result = 0;
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
	{
		const auto Num_awareness_events = std::exchange(LevelUniqueRobotAwarenessState.Num_awareness_events, 0);
		if (!Num_awareness_events)
			return Num_awareness_events;
		result = Num_awareness_events;
		New_awareness.fill(player_awareness_type_t::PA_NONE);
		const unsigned allowed_recursions_remaining =
#if defined(DXX_BUILD_DESCENT_II)
			!EMULATING_D1 ? 3 :
#endif
			4;
		range_for (auto &i, partial_const_range(LevelUniqueRobotAwarenessState.Awareness_events, Num_awareness_events))
			pae_aux(vcsegptridx(i.segnum), i.type, New_awareness, allowed_recursions_remaining);
	}
	return result;
}

// ----------------------------------------------------------------------------------
static void set_player_awareness_all(fvmobjptr &vmobjptr, fvcsegptridx &vcsegptridx, d_level_unique_robot_awareness_state &LevelUniqueRobotAwarenessState)
{
	awareness_t New_awareness;

	if (!process_awareness_events(vcsegptridx, LevelUniqueRobotAwarenessState, New_awareness))
		return;

	range_for (const auto &&objp, vmobjptr)
	{
		object &obj = objp;
		if (obj.type == OBJ_ROBOT && obj.control_source == object::control_type::ai)
		{
			auto &ailp = obj.ctype.ai_info.ail;
			auto &na = New_awareness[obj.segnum];
			if (ailp.player_awareness_type < na)
			{
				ailp.player_awareness_type = na;
				ailp.player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;

#if defined(DXX_BUILD_DESCENT_II)
			// Clear the bit that says this robot is only awake because a camera woke it up.
				obj.ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
#endif
			}
		}
	}
}

}

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which object is aware of player)
void do_ai_frame_all(const d_robot_info_array &Robot_info)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;

	set_player_awareness_all(vmobjptr, vcsegptridx, LevelUniqueRobotAwarenessState);

#if defined(DXX_BUILD_DESCENT_II)
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &vmobjptridx = Objects.vmptridx;
	if (Ai_last_missile_camera)
	{
		// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
		if (((d_tick_count & 0x0f) == 0) || (Ai_last_missile_camera->type != OBJ_WEAPON)) {
			Ai_last_missile_camera = nullptr;
			range_for (const auto &&objp, vmobjptr)
			{
				if (objp->type == OBJ_ROBOT)
					objp->ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
			}
		}
	}

	// (Moved here from do_d2_boss_stuff() because that only gets called if robot aware of player.)
	if (BossUniqueState.Boss_dying) {
		range_for (const auto &&objp, vmobjptridx)
		{
			if (objp->type == OBJ_ROBOT)
				if (Robot_info[get_robot_id(objp)].boss_flag != boss_robot_id::None)
					do_boss_dying_frame(Robot_info, objp);
		}
	}
#endif
}


// Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	BossUniqueState.Boss_dying_start_time = 0;
	Overall_agitation = 0;
#if defined(DXX_BUILD_DESCENT_II)
	GameUniqueState.Final_boss_countdown_time = 0;
	Ai_last_missile_camera = nullptr;
#endif
}

namespace {

// Following functions convert ai_local/ai_cloak_info to ai_local/ai_cloak_info_rw to be written to/read from Savegames. Convertin back is not done here - reading is done specifically together with swapping (if necessary). These structs differ in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
static void state_ai_local_to_ai_local_rw(const ai_local *ail, ai_local_rw *ail_rw)
{
	ail_rw->player_awareness_type      = static_cast<int8_t>(ail->player_awareness_type);
	ail_rw->retry_count                = ail->retry_count;
	ail_rw->consecutive_retries        = ail->consecutive_retries;
	ail_rw->mode                       = static_cast<uint8_t>(ail->mode);
	ail_rw->previous_visibility        = static_cast<int8_t>(ail->previous_visibility);
	ail_rw->rapidfire_count            = ail->rapidfire_count;
	ail_rw->goal_segment               = underlying_value(ail->goal_segment);
#if defined(DXX_BUILD_DESCENT_I)
	ail_rw->last_see_time              = 0;
	ail_rw->last_attack_time           = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	ail_rw->next_fire2                 = ail->next_fire2;
#endif
	ail_rw->next_action_time           = ail->next_action_time;
	ail_rw->next_fire                  = ail->next_fire;
	ail_rw->player_awareness_time      = ail->player_awareness_time;
	const auto gametime{GameTime64};
	ail_rw->time_player_seen = build_savegametime_from_gametime(gametime, ail->time_player_seen);
	ail_rw->time_player_sound_attacked = build_savegametime_from_gametime(gametime, ail->time_player_sound_attacked);
	ail_rw->next_misc_sound_time = build_savegametime_from_gametime(gametime, ail->next_misc_sound_time);
	ail_rw->time_since_processed       = ail->time_since_processed;
	for (uint8_t i = 0; i < MAX_SUBMODELS; i++)
	{
		ail_rw->goal_angles[i].p   = ail->goal_angles[i].p;
		ail_rw->goal_angles[i].b   = ail->goal_angles[i].b;
		ail_rw->goal_angles[i].h   = ail->goal_angles[i].h;
		ail_rw->delta_angles[i].p  = ail->delta_angles[i].p;
		ail_rw->delta_angles[i].b  = ail->delta_angles[i].b;
		ail_rw->delta_angles[i].h  = ail->delta_angles[i].h;
		ail_rw->goal_state[i]      = ail->goal_state[(robot_gun_number{i})];
		ail_rw->achieved_state[i]  = ail->achieved_state[(robot_gun_number{i})];
	}
}

static void state_ai_cloak_info_to_ai_cloak_info_rw(const ai_cloak_info *const aic, ai_cloak_info_rw *const aic_rw)
{
	aic_rw->last_time = build_savegametime_from_gametime(GameTime64, aic->last_time);
#if defined(DXX_BUILD_DESCENT_II)
	aic_rw->last_segment    = aic->last_segment;
#endif
	aic_rw->last_position.x = aic->last_position.x;
	aic_rw->last_position.y = aic->last_position.y;
	aic_rw->last_position.z = aic->last_position.z;
}

}

}

namespace dcx {

DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE();
DEFINE_SERIAL_UDT_TO_MESSAGE(point_seg, p, (p.segnum, serial::pad<2>(), p.point));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(point_seg, 16);

DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(point_seg_array_t, p, (static_cast<std::array<point_seg, MAX_POINT_SEGS> &>(p)));
DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(point_seg_array_t, p, (static_cast<const std::array<point_seg, MAX_POINT_SEGS> &>(p)));

}

namespace dsx {

int ai_save_state(PHYSFS_File *fp)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Boss_gate_segs = LevelSharedBossState.Gate_segs;
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
#endif
	auto &Objects = LevelUniqueObjectState.Objects;
	fix tmptime32 = 0;

	const int Ai_initialized = 0;
	PHYSFS_write(fp, &Ai_initialized, sizeof(int), 1);
	PHYSFS_write(fp, &Overall_agitation, sizeof(int), 1);
	{
		ai_local_rw zero{};
		range_for (const auto &i, Objects)
		{
			ai_local_rw ail_rw;
			PHYSFS_write(fp, i.type == OBJ_ROBOT ? (state_ai_local_to_ai_local_rw(&i.ctype.ai_info.ail, &ail_rw), &ail_rw) : &zero, sizeof(ail_rw), 1);
		}
	}
	PHYSFSX_serialize_write(fp, Point_segs);
	//PHYSFS_write(fp, Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1);
	range_for (auto &i, Ai_cloak_info)
	{
		ai_cloak_info_rw aic_rw;
		state_ai_cloak_info_to_ai_cloak_info_rw(&i, &aic_rw);
		PHYSFS_write(fp, &aic_rw, sizeof(aic_rw), 1);
	}
	const auto gametime{GameTime64};
	{
		const auto Boss_cloak_start_time = BossUniqueState.Boss_cloak_start_time;
		tmptime32 = build_savegametime_from_gametime(gametime, Boss_cloak_start_time);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
		tmptime32 = build_savegametime_from_gametime(gametime, Boss_cloak_start_time + Boss_cloak_duration);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	}
	{
		const auto Last_teleport_time = BossUniqueState.Last_teleport_time;
		tmptime32 = build_savegametime_from_gametime(gametime, Last_teleport_time);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	}
	{
		const fix Boss_teleport_interval = LevelSharedBossState.Boss_teleport_interval;
		PHYSFS_write(fp, &Boss_teleport_interval, sizeof(fix), 1);
		const fix Boss_cloak_interval = LevelSharedBossState.Boss_cloak_interval;
		PHYSFS_write(fp, &Boss_cloak_interval, sizeof(fix), 1);
	}
	PHYSFS_write(fp, &Boss_cloak_duration, sizeof(fix), 1);
	tmptime32 = build_savegametime_from_gametime(gametime, BossUniqueState.Last_gate_time);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &GameUniqueState.Boss_gate_interval, sizeof(fix), 1);
	{
		const auto Boss_dying_start_time = BossUniqueState.Boss_dying_start_time;
	if (Boss_dying_start_time == 0) // if Boss not dead, yet we expect this to be 0, so do not convert!
	{
		tmptime32 = 0;
	}
	else
	{
		tmptime32 = build_savegametime_from_gametime(gametime, Boss_dying_start_time);
		if (tmptime32 == 0) // now if our converted value went 0 we should do something against it
			tmptime32 = -1;
	}
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	}
	{
	const int boss_dying = BossUniqueState.Boss_dying;
	PHYSFS_write(fp, &boss_dying, sizeof(int), 1);
	}
	const int boss_dying_sound_playing = BossUniqueState.Boss_dying_sound_playing;
	PHYSFS_write(fp, &boss_dying_sound_playing, sizeof(int), 1);
#if defined(DXX_BUILD_DESCENT_I)
	const int boss_hit_this_frame = BossUniqueState.Boss_hit_this_frame;
	PHYSFS_write(fp, &boss_hit_this_frame, sizeof(int), 1);
	const int Boss_been_hit = 0;
	PHYSFS_write(fp, &Boss_been_hit, sizeof(int), 1);
#elif defined(DXX_BUILD_DESCENT_II)
	tmptime32 = build_savegametime_from_gametime(gametime, BossUniqueState.Boss_hit_time);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_writeSLE32(fp, -1);
	tmptime32 = build_savegametime_from_gametime(gametime, BuddyState.Escort_last_path_created);
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	{
		const uint32_t Escort_goal_object = BuddyState.Escort_goal_object;
	PHYSFS_write(fp, &Escort_goal_object, sizeof(Escort_goal_object), 1);
	}
	{
		const uint32_t Escort_special_goal = BuddyState.Escort_special_goal;
	PHYSFS_write(fp, &Escort_special_goal, sizeof(Escort_special_goal), 1);
	}
	{
		const int egi = BuddyState.Escort_goal_reachable == d_unique_buddy_state::Escort_goal_reachability::unreachable ? -2 : BuddyState.Escort_goal_objidx.get_unchecked_index();
		PHYSFS_write(fp, &egi, sizeof(int), 1);
	}
	{
		auto &Stolen_items = LevelUniqueObjectState.ThiefState.Stolen_items;
		PHYSFS_write(fp, &Stolen_items, sizeof(Stolen_items[0]) * Stolen_items.size(), 1);
	}

	{
		int temp;
		temp = Point_segs_free_ptr - Point_segs;
		PHYSFS_write(fp, &temp, sizeof(int), 1);
	}

	unsigned Num_boss_teleport_segs = Boss_teleport_segs.size();
	PHYSFS_write(fp, &Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1);
	unsigned Num_boss_gate_segs = Boss_gate_segs.size();
	PHYSFS_write(fp, &Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1);

	if (Num_boss_gate_segs)
		PHYSFS_write(fp, &Boss_gate_segs[0], sizeof(Boss_gate_segs[0]), Num_boss_gate_segs);

	if (Num_boss_teleport_segs)
		PHYSFS_write(fp, &Boss_teleport_segs[0], sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs);
#endif

	return 1;
}

}

namespace dcx {

static void PHYSFSX_readAngleVecX(PHYSFS_File *file, vms_angvec &v, int swap)
{
	v.p = PHYSFSX_readSXE16(file, swap);
	v.b = PHYSFSX_readSXE16(file, swap);
	v.h = PHYSFSX_readSXE16(file, swap);
}

}

namespace dsx {
namespace {

static void ai_local_read_swap(ai_local *ail, int swap, PHYSFS_File *fp)
{
	{
		fix tmptime32 = 0;

#if defined(DXX_BUILD_DESCENT_I)
		ail->player_awareness_type = static_cast<player_awareness_type_t>(PHYSFSX_readByte(fp));
		ail->retry_count = PHYSFSX_readByte(fp);
		ail->consecutive_retries = PHYSFSX_readByte(fp);
		ail->mode = static_cast<ai_mode>(PHYSFSX_readByte(fp));
		ail->previous_visibility = static_cast<player_visibility_state>(PHYSFSX_readByte(fp));
		ail->rapidfire_count = PHYSFSX_readByte(fp);
		{
			const auto s = segnum_t{static_cast<uint16_t>(PHYSFSX_readSXE16(fp, swap))};
			ail->goal_segment = imsegidx_t::check_nothrow_index(s) ? s : segment_none;
		}
		PHYSFSX_readSXE32(fp, swap);
		PHYSFSX_readSXE32(fp, swap);
		ail->next_action_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire = PHYSFSX_readSXE32(fp, swap);
#elif defined(DXX_BUILD_DESCENT_II)
		ail->player_awareness_type = static_cast<player_awareness_type_t>(PHYSFSX_readSXE32(fp, swap));
		ail->retry_count = PHYSFSX_readSXE32(fp, swap);
		ail->consecutive_retries = PHYSFSX_readSXE32(fp, swap);
		ail->mode = static_cast<ai_mode>(PHYSFSX_readSXE32(fp, swap));
		ail->previous_visibility = static_cast<player_visibility_state>(PHYSFSX_readSXE32(fp, swap));
		ail->rapidfire_count = PHYSFSX_readSXE32(fp, swap);
		{
			const auto s = segnum_t{static_cast<uint16_t>(PHYSFSX_readSXE32(fp, swap))};
			ail->goal_segment = imsegidx_t::check_nothrow_index(s) ? s : segment_none;
		}
		ail->next_action_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire2 = PHYSFSX_readSXE32(fp, swap);
#endif
		ail->player_awareness_time = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_seen = static_cast<fix64>(tmptime32);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_sound_attacked = static_cast<fix64>(tmptime32);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->next_misc_sound_time = static_cast<fix64>(tmptime32);
		ail->time_since_processed = PHYSFSX_readSXE32(fp, swap);
		
		range_for (auto &j, ail->goal_angles)
			PHYSFSX_readAngleVecX(fp, j, swap);
		range_for (auto &j, ail->delta_angles)
			PHYSFSX_readAngleVecX(fp, j, swap);
		range_for (auto &j, ail->goal_state)
			j = build_ai_state_from_untrusted(PHYSFSX_readByte(fp)).value();
		range_for (auto &j, ail->achieved_state)
			j = build_ai_state_from_untrusted(PHYSFSX_readByte(fp)).value();
	}
}

}

}

namespace dcx {

static void PHYSFSX_readVectorX(PHYSFS_File *file, vms_vector &v, int swap)
{
	v.x = PHYSFSX_readSXE32(file, swap);
	v.y = PHYSFSX_readSXE32(file, swap);
	v.z = PHYSFSX_readSXE32(file, swap);
}

}

namespace dsx {
namespace {

static void ai_cloak_info_read_n_swap(ai_cloak_info *ci, int n, int swap, PHYSFS_File *fp)
{
	int i;
	fix tmptime32 = 0;
	
	for (i = 0; i < n; i++, ci++)
	{
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ci->last_time = static_cast<fix64>(tmptime32);
#if defined(DXX_BUILD_DESCENT_II)
		{
			const auto s = segnum_t{static_cast<uint16_t>(PHYSFSX_readSXE32(fp, swap))};
			ci->last_segment = imsegidx_t::check_nothrow_index(s) ? s : segment_none;
		}
#endif
		PHYSFSX_readVectorX(fp, ci->last_position, swap);
	}
}

}

int ai_restore_state(const d_robot_info_array &Robot_info, PHYSFS_File *fp, int version, int swap)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Boss_gate_segs = LevelSharedBossState.Gate_segs;
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
#endif
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	fix tmptime32 = 0;

	PHYSFSX_readSXE32(fp, swap);
	Overall_agitation = PHYSFSX_readSXE32(fp, swap);
	range_for (object &obj, Objects)
	{
		ai_local discard;
		ai_local_read_swap(obj.type == OBJ_ROBOT ? &obj.ctype.ai_info.ail : &discard, swap, fp);
	}
	PHYSFSX_serialize_read(fp, Point_segs);
	ai_cloak_info_read_n_swap(Ai_cloak_info.data(), Ai_cloak_info.size(), swap, fp);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Boss_cloak_start_time = static_cast<fix64>(tmptime32);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Last_teleport_time = static_cast<fix64>(tmptime32);

	// If boss teleported, set the looping 'see' sound -kreatordxx
	// Also make sure any bosses that were generated/released during the game have teleport segs
#if DXX_USE_EDITOR
	if (!EditorWindow)
#endif
		range_for (const auto &&o, vmobjptridx)
		{
			if (o->type == OBJ_ROBOT)
			{
				const auto boss_id = Robot_info[get_robot_id(o)].boss_flag;
				if (boss_id != boss_robot_id::None)
				{
					const auto Last_teleport_time = BossUniqueState.Last_teleport_time;
					if (Last_teleport_time != 0 && Last_teleport_time != BossUniqueState.Boss_cloak_start_time)
						boss_link_see_sound(Robot_info, o);
					boss_init_all_segments(Segments, o);
				}
			}
		}
	
#if defined(DXX_BUILD_DESCENT_II)
	LevelSharedBossState.Boss_teleport_interval =
#endif
		PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_II)
	LevelSharedBossState.Boss_cloak_interval =
#endif
		PHYSFSX_readSXE32(fp, swap);
	PHYSFSX_readSXE32(fp, swap);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Last_gate_time = static_cast<fix64>(tmptime32);
	GameUniqueState.Boss_gate_interval = PHYSFSX_readSXE32(fp, swap);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Boss_dying_start_time = static_cast<fix64>(tmptime32);
	BossUniqueState.Boss_dying = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Boss_dying_sound_playing = PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_I)
	(void)version;
	BossUniqueState.Boss_hit_this_frame = PHYSFSX_readSXE32(fp, swap);
	PHYSFSX_readSXE32(fp, swap);
#elif defined(DXX_BUILD_DESCENT_II)
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	BossUniqueState.Boss_hit_time = static_cast<fix64>(tmptime32);

	if (version >= 8) {
		PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		BuddyState.Escort_last_path_created = static_cast<fix64>(tmptime32);
		BuddyState.Escort_goal_object = static_cast<escort_goal_t>(PHYSFSX_readSXE32(fp, swap));
		BuddyState.Escort_special_goal = static_cast<escort_goal_t>(PHYSFSX_readSXE32(fp, swap));
		const int egi = PHYSFSX_readSXE32(fp, swap);
		if (static_cast<unsigned>(egi) < Objects.size())
		{
			BuddyState.Escort_goal_objidx = egi;
			BuddyState.Escort_goal_reachable = d_unique_buddy_state::Escort_goal_reachability::reachable;
		}
		else
		{
			BuddyState.Escort_goal_objidx = object_none;
			BuddyState.Escort_goal_reachable = d_unique_buddy_state::Escort_goal_reachability::unreachable;
		}
		{
			auto &Stolen_items = LevelUniqueObjectState.ThiefState.Stolen_items;
			PHYSFS_read(fp, &Stolen_items, sizeof(Stolen_items[0]) * Stolen_items.size(), 1);
		}
	} else {
		BuddyState.Escort_last_path_created = 0;
		BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
		BuddyState.Escort_goal_objidx = object_none;
		BuddyState.Escort_goal_reachable = d_unique_buddy_state::Escort_goal_reachability::unreachable;

		LevelUniqueObjectState.ThiefState.Stolen_items.fill(255);
	}

	if (version >= 15) {
		unsigned temp;
		temp = PHYSFSX_readSXE32(fp, swap);
		if (temp > Point_segs.size())
			throw std::out_of_range("too many points");
		Point_segs_free_ptr = Point_segs.begin() + temp;
	} else
		ai_reset_all_paths();

	if (version >= 21) {
		const xrange Num_boss_teleport_segs = static_cast<unsigned>(PHYSFSX_readSXE32(fp, swap));
		const xrange Num_boss_gate_segs = static_cast<unsigned>(PHYSFSX_readSXE32(fp, swap));

		Boss_gate_segs.clear();
		for (const auto i : Num_boss_gate_segs)
		{
			(void)i;
			const auto s = segnum_t{static_cast<uint16_t>(PHYSFSX_readSXE16(fp, swap))};
			Boss_gate_segs.emplace_back(s);
		}

		Boss_teleport_segs.clear();
		for (const auto i : Num_boss_teleport_segs)
		{
			(void)i;
			const auto s = segnum_t{static_cast<uint16_t>(PHYSFSX_readSXE16(fp, swap))};
			Boss_teleport_segs.emplace_back(s);
		}
	}
#endif

	return 1;
}

}
