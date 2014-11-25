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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "console.h"
#include "3d.h"

#include "object.h"
#include "render.h"
#include "dxxerror.h"
#include "ai.h"
#include "escort.h"
#include "laser.h"
#include "fvi.h"
#include "physfsx.h"
#include "physfs-serial.h"
#include "polyobj.h"
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
#include "timer.h"
#include "sounds.h"
#include "gameseg.h"
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "gameseq.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "args.h"
#include "fuelcen.h"
#include "controls.h"
#include "kconfig.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#include "editor/kdefs.h"
#endif

//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "segiter.h"
#include "partial_range.h"

using std::min;

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

#if defined(DXX_BUILD_DESCENT_I)
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds
#elif defined(DXX_BUILD_DESCENT_II)
#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	(F1_0*40)

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		(F1_0*4)
#define	MAX_LEAD_DISTANCE	(F1_0*200)
#define	LEAD_RANGE			(F1_0/2)

#define	MAX_SPEW_BOT		3

static const int	Spew_bots[NUM_D2_BOSSES][MAX_SPEW_BOT] = {
	{38, 40, -1},
	{37, -1, -1},
	{43, 57, -1},
	{26, 27, 58},
	{59, 58, 54},
	{60, 61, 54},

	{69, 29, 24},
	{72, 60, 73} 
};

static const int	Max_spew_bots[NUM_D2_BOSSES] = {2, 1, 2, 3, 3, 3,  3, 3};
#endif

static void init_boss_segments(boss_special_segment_array_t &segptr, int size_check, int one_wall_hack);

enum {
	Flinch_scale = 4,
	Attack_scale = 24,
};
#define	ANIM_RATE		(F1_0/16)
#define	DELTA_ANG_SCALE	16

static void ai_multi_send_robot_position(const vobjptridx_t objnum, int force);

static const sbyte Mike_to_matt_xlate[] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

#define	OVERALL_AGITATION_MAX	100

#define		MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#define	BOSS_CLOAK_DURATION	(F1_0*7)
#define	BOSS_DEATH_DURATION	(F1_0*6)
//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use FrameTime because robots do not get moved every frame.

boss_teleport_segment_array_t	Boss_teleport_segs;

boss_gate_segment_array_t Boss_gate_segs;

// ---------- John: These variables must be saved as part of gamesave. --------
int             Ai_initialized = 0;
int             Overall_agitation;
point_seg_array_t       Point_segs;
point_seg_array_t::iterator       Point_segs_free_ptr;
ai_cloak_info   Ai_cloak_info[MAX_AI_CLOAK_INFO];
fix64           Boss_cloak_start_time = 0;
fix64           Boss_cloak_end_time = 0;
fix64           Last_teleport_time = 0;
fix             Boss_teleport_interval = F1_0*8;
fix             Boss_cloak_interval = F1_0*10;                    //    Time between cloaks
fix             Boss_cloak_duration = BOSS_CLOAK_DURATION;
fix64           Last_gate_time = 0;
fix             Gate_interval = F1_0*6;
fix64           Boss_dying_start_time;
#if defined(DXX_BUILD_DESCENT_I)
int				Boss_dying, Boss_dying_sound_playing, Boss_hit_this_frame;
int				Boss_been_hit=0;

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
static const sbyte	Super_boss_gate_list[] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};
#define	MAX_GATE_INDEX	( sizeof(Super_boss_gate_list) / sizeof(Super_boss_gate_list[0]) )

#elif defined(DXX_BUILD_DESCENT_II)
fix64           Boss_hit_time;
int           Boss_dying;
sbyte           Boss_dying_sound_playing;


// ------ John: End of variables which must be saved as part of gamesave. -----

vms_vector	Last_fired_upon_player_pos;


// -- ubyte Boss_cloaks[NUM_D2_BOSSES]              = {1,1,1,1,1,1};      // Set byte if this boss can cloak

const ubyte Boss_teleports[NUM_D2_BOSSES]           = {1,1,1,1,1,1, 1,1}; // Set byte if this boss can teleport
const ubyte Boss_spew_more[NUM_D2_BOSSES]           = {0,1,0,0,0,0, 0,0}; // If set, 50% of time, spew two bots.
const ubyte Boss_spews_bots_energy[NUM_D2_BOSSES]   = {1,1,0,1,0,1, 1,1}; // Set byte if boss spews bots when hit by energy weapon.
const ubyte Boss_spews_bots_matter[NUM_D2_BOSSES]   = {0,0,1,1,1,1, 0,1}; // Set byte if boss spews bots when hit by matter weapon.
const ubyte Boss_invulnerable_energy[NUM_D2_BOSSES] = {0,0,1,1,0,0, 0,0}; // Set byte if boss is invulnerable to energy weapons.
const ubyte Boss_invulnerable_matter[NUM_D2_BOSSES] = {0,0,0,0,1,1, 1,0}; // Set byte if boss is invulnerable to matter weapons.
const ubyte Boss_invulnerable_spot[NUM_D2_BOSSES]   = {0,0,0,0,0,1, 0,1}; // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

segnum_t             Believed_player_seg;
#endif

static const std::size_t MAX_AWARENESS_EVENTS = 64;
struct awareness_event
{
	segnum_t	segnum;				// segment the event occurred in
	short			type;					// type of event, defines behavior
	vms_vector	pos;					// absolute 3 space location of event
};

int ai_evaded=0;


#if defined(DXX_BUILD_DESCENT_I)
static const
#endif
int Animation_enabled = 1;

// These globals are set by a call to find_vector_intersection, which is a slow routine,
// so we don't want to call it again (for this object) unless we have to.
vms_vector  Hit_pos;
int         Hit_type;
fvi_info    Hit_data;

static unsigned             Num_awareness_events;
awareness_event Awareness_events[MAX_AWARENESS_EVENTS];

vms_vector      Believed_player_pos;

#define	AIS_MAX	8
#define	AIE_MAX	4

#ifndef NDEBUG
// Index into this array with ailp->mode
static const char mode_text[][16] = {
	"STILL",
	"WANDER",
	"FOL_PATH",
	"CHASE_OBJ",
	"RUN_FROM",
#if defined(DXX_BUILD_DESCENT_I)
	"HIDE",
#elif defined(DXX_BUILD_DESCENT_II)
	"BEHIND",
#endif
	"FOL_PATH2",
	"OPEN_DOOR",
#if defined(DXX_BUILD_DESCENT_II)
	"GOTO_PLR",
	"GOTO_OBJ",
	"SN_ATT",
	"SN_FIRE",
	"SN_RETR",
	"SN_RTBK",
	"SN_WAIT",
	"TH_ATTACK",
	"TH_RETREAT",
	"TH_WAIT",
#endif
};

//	Index into this array with aip->behavior
static const char behavior_text[6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

// Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
static const char state_text[8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR_",
};


#endif

// Current state indicates where the robot current is, or has just done.
// Transition table between states for an AI object.
// First dimension is trigger event.
// Second dimension is current state.
// Third dimension is goal state.
// Result is new goal state.
// ERR_ means something impossible has happened.
static const sbyte Ai_transition_table[AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
		// Event = AIE_FIRE, a nearby object fired
		// none     rest      srch      lock      flin      fire      reco        // CURRENT is rows, GOAL is columns
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // none
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // rest
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // search
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO }, // lock
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO }, // flinch
		{ AIS_ERR_, AIS_FIRE, AIS_FIRE, AIS_FIRE, AIS_FLIN, AIS_FIRE, AIS_RECO }, // fire
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE }  // recoil
	},

	// Event = AIE_HITT, a nearby object was hit (or a wall was hit)
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_COLL, player collided with robot
	{
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_LOCK, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_REST, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FIRE, AIS_RECO},
		{ AIS_ERR_, AIS_LOCK, AIS_LOCK, AIS_LOCK, AIS_FLIN, AIS_FIRE, AIS_FIRE}
	},

	// Event = AIE_HURT, player hurt robot (by firing at and hitting it)
	// Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN},
		{ AIS_ERR_, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN, AIS_FLIN}
	}
};

static int ready_to_fire_weapon1(const ai_local *ailp, fix threshold)
{
	return (ailp->next_fire <= threshold);
}

static int ready_to_fire_weapon2(const robot_info *robptr, const ai_local *ailp, fix threshold)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)robptr;
	(void)ailp;
	(void)threshold;
	return 0;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robptr->weapon_type2 == weapon_none)
		return 0;
	return (ailp->next_fire2 <= threshold);
#endif
}

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if next_fire <= 0 or next_fire2 <= 0.
static int ready_to_fire_any_weapon(const robot_info *robptr, const ai_local *ailp, fix threshold)
{
	return ready_to_fire_weapon1(ailp, threshold) || ready_to_fire_weapon2(robptr, ailp, threshold);
}

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int ai_behavior_to_mode(int behavior)
{
	switch (behavior) {
		case AIB_STILL:			return AIM_STILL;
		case AIB_NORMAL:			return AIM_CHASE_OBJECT;
		case AIB_RUN_FROM:		return AIM_RUN_FROM_OBJECT;
		case AIB_STATION:			return AIM_STILL;
#if defined(DXX_BUILD_DESCENT_I)
		case AIB_HIDE:				return AIM_HIDE;
		case AIB_FOLLOW_PATH:	return AIM_FOLLOW_PATH;
#elif defined(DXX_BUILD_DESCENT_II)
		case AIB_BEHIND:			return AIM_BEHIND;
		case AIB_SNIPE:			return AIM_STILL;	//	Changed, 09/13/95, MK, snipers are still until they see you or are hit.
		case AIB_FOLLOW:			return AIM_FOLLOW_PATH;
#endif
		default:	Int3();	//	Contact Mike: Error, illegal behavior type
	}

	return AIM_STILL;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the player starts a new ship.
void ai_init_boss_for_ship(void)
{
#if defined(DXX_BUILD_DESCENT_I)
	Boss_been_hit = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	Boss_hit_time = -F1_0*10;
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void init_ai_object(const vobjptr_t objp, int behavior, segnum_t hide_segment)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	*ailp = {};

	if (behavior == 0) {
		behavior = AIB_NORMAL;
		aip->behavior = behavior;
	}

	//	mode is now set from the Robot dialog, so this should get overwritten.
	ailp->mode = AIM_STILL;

	ailp->previous_visibility = 0;

	if (behavior != -1) {
		aip->behavior = behavior;
		ailp->mode = ai_behavior_to_mode(aip->behavior);
	} else if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		aip->behavior = AIB_NORMAL;
	}

	robot_info	*robptr = &Robot_info[get_robot_id(objp)];
#if defined(DXX_BUILD_DESCENT_II)
	if (robot_is_companion(robptr)) {
		ailp->mode = AIM_GOTO_PLAYER;
		Escort_kill_object = -1;
	}

	if (robot_is_thief(robptr)) {
		aip->behavior = AIB_SNIPE;
		ailp->mode = AIM_THIEF_WAIT;
	}

	if (robptr->attack_type) {
		aip->behavior = AIB_NORMAL;
		ailp->mode = ai_behavior_to_mode(aip->behavior);
	}
#endif

	// This is astonishingly stupid!  This routine gets called by matcens! KILL KILL KILL!!! Point_segs_free_ptr = Point_segs;

	vm_vec_zero(objp->mtype.phys_info.velocity);
	ailp->player_awareness_time = 0;
	ailp->player_awareness_type = 0;
	aip->GOAL_STATE = AIS_SRCH;
	aip->CURRENT_STATE = AIS_REST;
	ailp->time_player_seen = GameTime64;
	ailp->next_misc_sound_time = GameTime64;
	ailp->time_player_sound_attacked = GameTime64;

#if defined(DXX_BUILD_DESCENT_I)
	if ((behavior == AIB_HIDE) || (behavior == AIB_FOLLOW_PATH) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((behavior == AIB_SNIPE) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM) || (behavior == AIB_FOLLOW))
#endif
	{
		aip->hide_segment = hide_segment;
		ailp->goal_segment = hide_segment;
		aip->hide_index = -1;			// This means the path has not yet been created.
		aip->cur_path_index = 0;
	}

	aip->SKIP_AI_COUNT = 0;

	if (robptr->cloak_type == RI_CLOAKED_ALWAYS)
		aip->CLOAKED = 1;
	else
		aip->CLOAKED = 0;

	objp->mtype.phys_info.flags |= (PF_BOUNCE | PF_TURNROLL);
	
	aip->REMOTE_OWNER = -1;

#if defined(DXX_BUILD_DESCENT_II)
	aip->dying_sound_playing = 0;
	aip->dying_start_time = 0;
#endif
	aip->danger_laser_num = object_none;
}

// ---------------------------------------------------------------------------------------------------------------------
void init_ai_objects(void)
{
	Point_segs_free_ptr = Point_segs.begin();

	range_for (auto &obj, Objects)
	{
		if (obj.control_type == CT_AI)
			init_ai_object(&obj, obj.ctype.ai_info.behavior, obj.ctype.ai_info.hide_segment);
	}

	Boss_dying_sound_playing = 0;
	Boss_dying = 0;

	Ai_initialized = 1;

	init_boss_segments(Boss_gate_segs, 0, 0);

	init_boss_segments(Boss_teleport_segs, 1, 0);
#if defined(DXX_BUILD_DESCENT_I)
	Boss_been_hit = 0;
	Gate_interval = F1_0*5 - Difficulty_level*F1_0/2;
#elif defined(DXX_BUILD_DESCENT_II)
	Gate_interval = F1_0*4 - Difficulty_level*i2f(2)/3;
	if (Boss_teleport_segs.count() == 1)
		init_boss_segments(Boss_teleport_segs, 1, 1);

	ai_do_cloak_stuff();

	init_buddy_for_level();

	if (Current_mission && (Current_level_num == Last_level))
	{
		Boss_teleport_interval = F1_0*10;
		Boss_cloak_interval = F1_0*15;					//	Time between cloaks
	} else
	{
		Boss_teleport_interval = F1_0*7;
		Boss_cloak_interval = F1_0*10;					//	Time between cloaks
	}
#endif
}

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector(const vms_vector &goal_vector, const vobjptr_t objp, fix rate)
{
	vms_vector	new_fvec;
	fix			dot;

	//	Not all robots can turn, eg, SPECIAL_REACTOR_ROBOT
	if (rate == 0)
		return;

	if ((objp->type == OBJ_ROBOT) && (get_robot_id(objp) == BABY_SPIDER_ID)) {
		physics_turn_towards_vector(goal_vector, objp, rate);
		return;
	}

	new_fvec = goal_vector;

	dot = vm_vec_dot(goal_vector, objp->orient.fvec);

	if (dot < (F1_0 - FrameTime/2)) {
		fix	mag;
		fix	new_scale = fixdiv(FrameTime * AI_TURN_SCALE, rate);
		vm_vec_scale(new_fvec, new_scale);
		vm_vec_add2(new_fvec, objp->orient.fvec);
		mag = vm_vec_normalize_quick(new_fvec);
		if (mag < F1_0/256) {
			new_fvec = goal_vector;		//	if degenerate vector, go right to goal
		}
	}

	if (Seismic_tremor_magnitude) {
		fix			scale;
		const auto rand_vec = make_random_vector();
		scale = fixdiv(2*Seismic_tremor_magnitude, Robot_info[get_robot_id(objp)].mass);
		vm_vec_scale_add2(new_fvec, rand_vec, scale);
	}

	vm_vector_2_matrix(objp->orient, new_fvec, nullptr, &objp->orient.rvec);
}

#if defined(DXX_BUILD_DESCENT_I)
static void ai_turn_randomly(const vms_vector &vec_to_player, const vobjptr_t obj, fix rate, int previous_visibility)
{
	vms_vector	curvec;

	//	Random turning looks too stupid, so 1/4 of time, cheat.
	if (previous_visibility)
                if (d_rand() > 0x7400) {
			ai_turn_towards_vector(vec_to_player, obj, rate);
			return;
		}
//--debug--     if (d_rand() > 0x6000)
//--debug-- 		Prevented_turns++;

	curvec = obj->mtype.phys_info.rotvel;

	curvec.y += F1_0/64;

	curvec.x += curvec.y/6;
	curvec.y += curvec.z/4;
	curvec.z += curvec.x/10;

	if (abs(curvec.x) > F1_0/8) curvec.x /= 4;
	if (abs(curvec.y) > F1_0/8) curvec.y /= 4;
	if (abs(curvec.z) > F1_0/8) curvec.z /= 4;

	obj->mtype.phys_info.rotvel = curvec;

}

#elif defined(DXX_BUILD_DESCENT_II)
fix Dist_to_last_fired_upon_player_pos = 0;
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
int player_is_visible_from_object(const vobjptridx_t objp, vms_vector &pos, fix field_of_view, const vms_vector &vec_to_player)
{
	fix			dot;
	fvi_query	fq;

#if defined(DXX_BUILD_DESCENT_II)
	//	Assume that robot's gun tip is in same segment as robot's center.
	if (objp->control_type == CT_AI)
		objp->ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_GUNSEG;
#endif

	fq.p0						= &pos;
	if ((pos.x != objp->pos.x) || (pos.y != objp->pos.y) || (pos.z != objp->pos.z)) {
		auto segnum = find_point_seg(pos, objp->segnum);
		if (segnum == segment_none) {
			fq.startseg = objp->segnum;
			pos = objp->pos;
			move_towards_segment_center(objp);
		} else
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (segnum != objp->segnum) {
				if (objp->control_type == CT_AI)
					objp->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_GUNSEG;
			}
#endif
			fq.startseg = segnum;
		}
	} else
		fq.startseg			= objp->segnum;
	fq.p1						= &Believed_player_pos;
	fq.rad					= F1_0/4;
	fq.thisobjnum			= objp;
	fq.ignore_obj_list	= NULL;
#if defined(DXX_BUILD_DESCENT_I)
	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???
#elif defined(DXX_BUILD_DESCENT_II)
	fq.flags					= FQ_TRANSWALL; // -- Why were we checking objects? | FQ_CHECK_OBJS;		//what about trans walls???
#endif

	Hit_type = find_vector_intersection(&fq,&Hit_data);

	Hit_pos = Hit_data.hit_pnt;

#if defined(DXX_BUILD_DESCENT_I)
	if ((Hit_type == HIT_NONE) || ((Hit_type == HIT_OBJECT) && (Hit_data.hit_object == Players[Player_num].objnum)))
#elif defined(DXX_BUILD_DESCENT_II)
	// -- when we stupidly checked objects -- if ((Hit_type == HIT_NONE) || ((Hit_type == HIT_OBJECT) && (Hit_data.hit_object == Players[Player_num].objnum))) {
	if (Hit_type == HIT_NONE)
#endif
	{
		dot = vm_vec_dot(vec_to_player, objp->orient.fvec);
		if (dot > field_of_view - (Overall_agitation << 9)) {
			return 2;
		} else {
			return 1;
		}
	} else {
		return 0;
	}
}

// ------------------------------------------------------------------------------------------------------------------
//	Return 1 if animates, else return 0
static int do_silly_animation(const vobjptr_t objp)
{
	const jointpos 		*jp_list;
	int				robot_type, gun_num, robot_state, num_joint_positions;
	polyobj_info	*pobj_info = &objp->rtype.pobj_info;
	ai_static		*aip = &objp->ctype.ai_info;
	int				num_guns, at_goal;
	int				attack_type;
	int				flinch_attack_scale = 1;

	robot_type = get_robot_id(objp);
	const robot_info *robptr = &Robot_info[robot_type];
	num_guns = robptr->n_guns;
	attack_type = robptr->attack_type;

	if (num_guns == 0) {
		return 0;
	}

	//	This is a hack.  All positions should be based on goal_state, not GOAL_STATE.
	robot_state = Mike_to_matt_xlate[aip->GOAL_STATE];
	// previous_robot_state = Mike_to_matt_xlate[aip->CURRENT_STATE];

	if (attack_type) // && ((robot_state == AS_FIRE) || (robot_state == AS_RECOIL)))
		flinch_attack_scale = Attack_scale;
	else if ((robot_state == AS_FLINCH) || (robot_state == AS_RECOIL))
		flinch_attack_scale = Flinch_scale;

	at_goal = 1;
	for (gun_num=0; gun_num <= num_guns; gun_num++) {
		int	joint;

		num_joint_positions = robot_get_anim_state(&jp_list, robot_type, gun_num, robot_state);

		for (joint=0; joint<num_joint_positions; joint++) {
			fix			delta_angle, delta_2;
			unsigned jointnum = jp_list[joint].jointnum;
			const vms_angvec	*jp = &jp_list[joint].angles;
			vms_angvec	*pobjp = &pobj_info->anim_angles[jointnum];

			if (jointnum >= Polygon_models[objp->rtype.pobj_info.model_num].n_models) {
				Int3();		// Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if (jp->p != pobjp->p) {
				if (gun_num == 0)
					at_goal = 0;
				ai_local		*ailp = &objp->ctype.ai_info.ail;
				ailp->goal_angles[jointnum].p = jp->p;

				delta_angle = jp->p - pobjp->p;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				ailp->delta_angles[jointnum].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pobjp->b) {
				if (gun_num == 0)
					at_goal = 0;
				ai_local		*ailp = &objp->ctype.ai_info.ail;
				ailp->goal_angles[jointnum].b = jp->b;

				delta_angle = jp->b - pobjp->b;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				ailp->delta_angles[jointnum].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pobjp->h) {
				if (gun_num == 0)
					at_goal = 0;
				ai_local		*ailp = &objp->ctype.ai_info.ail;
				ailp->goal_angles[jointnum].h = jp->h;

				delta_angle = jp->h - pobjp->h;
				if (delta_angle >= F1_0/2)
					delta_2 = -ANIM_RATE;
				else if (delta_angle >= 0)
					delta_2 = ANIM_RATE;
				else if (delta_angle >= -F1_0/2)
					delta_2 = -ANIM_RATE;
				else
					delta_2 = ANIM_RATE;

				if (flinch_attack_scale != 1)
					delta_2 *= flinch_attack_scale;

				ailp->delta_angles[jointnum].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//ai_static	*aip = &objp->ctype.ai_info;
			ai_local		*ailp = &objp->ctype.ai_info.ail;
			ailp->achieved_state[gun_num] = ailp->goal_state[gun_num];
			if (ailp->achieved_state[gun_num] == AIS_RECO)
				ailp->goal_state[gun_num] = AIS_FIRE;

			if (ailp->achieved_state[gun_num] == AIS_FLIN)
				ailp->goal_state[gun_num] = AIS_LOCK;

		}
	}

	if (at_goal == 1) //num_guns)
		aip->CURRENT_STATE = aip->GOAL_STATE;

	return 1;
}

//	------------------------------------------------------------------------------------------
//	Move all sub-objects in an object towards their goals.
//	Current orientation of object is at:	pobj_info.anim_angles
//	Goal orientation of object is at:		ai_info.goal_angles
//	Delta orientation of object is at:		ai_info.delta_angles
static void ai_frame_animation(const vobjptr_t objp)
{
	int	joint;
	auto num_joints = Polygon_models[objp->rtype.pobj_info.model_num].n_models;
	for (joint=1; joint<num_joints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vms_angvec	*curangp = &objp->rtype.pobj_info.anim_angles[joint];
		ai_local		*ailp = &objp->ctype.ai_info.ail;
		vms_angvec	*goalangp = &ailp->goal_angles[joint];
		vms_angvec	*deltaangp = &ailp->delta_angles[joint];

		delta_to_goal = goalangp->p - curangp->p;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = fixmul(deltaangp->p, FrameTime) * DELTA_ANG_SCALE;
			curangp->p += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->p = goalangp->p;
		}

		delta_to_goal = goalangp->b - curangp->b;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = fixmul(deltaangp->b, FrameTime) * DELTA_ANG_SCALE;
			curangp->b += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->b = goalangp->b;
		}

		delta_to_goal = goalangp->h - curangp->h;
		if (delta_to_goal > 32767)
			delta_to_goal = delta_to_goal - 65536;
		else if (delta_to_goal < -32767)
			delta_to_goal = 65536 + delta_to_goal;

		if (delta_to_goal) {
			scaled_delta_angle = fixmul(deltaangp->h, FrameTime) * DELTA_ANG_SCALE;
			curangp->h += scaled_delta_angle;
			if (abs(delta_to_goal) < abs(scaled_delta_angle))
				curangp->h = goalangp->h;
		}

	}

}

// ----------------------------------------------------------------------------------
static void set_next_fire_time(const vobjptr_t objp, ai_local *ailp, robot_info *robptr, int gun_num)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)objp;
	(void)gun_num;
	ailp->rapidfire_count++;

	if (ailp->rapidfire_count < robptr->rapidfire_count[Difficulty_level]) {
		ailp->next_fire = min(F1_0/8, robptr->firing_wait[Difficulty_level]/2);
	} else {
		ailp->rapidfire_count = 0;
		ailp->next_fire = robptr->firing_wait[Difficulty_level];
	}
#elif defined(DXX_BUILD_DESCENT_II)
	//	For guys in snipe mode, they have a 50% shot of getting this shot in free.
	if ((gun_num != 0) || (robptr->weapon_type2 == weapon_none))
		if ((objp->ctype.ai_info.behavior != AIB_SNIPE) || (d_rand() > 16384))
			ailp->rapidfire_count++;

	//	Old way, 10/15/95: Continuous rapidfire if rapidfire_count set.
// -- 	if (((robptr->weapon_type2 == -1) || (gun_num != 0)) && (ailp->rapidfire_count < robptr->rapidfire_count[Difficulty_level])) {
// -- 		ailp->next_fire = min(F1_0/8, robptr->firing_wait[Difficulty_level]/2);
// -- 	} else {
// -- 		if ((robptr->weapon_type2 == -1) || (gun_num != 0)) {
// -- 			ailp->rapidfire_count = 0;
// -- 			ailp->next_fire = robptr->firing_wait[Difficulty_level];
// -- 		} else
// -- 			ailp->next_fire2 = robptr->firing_wait2[Difficulty_level];
// -- 	}

	if (((gun_num != 0) || (robptr->weapon_type2 == weapon_none)) && (ailp->rapidfire_count < robptr->rapidfire_count[Difficulty_level])) {
		ailp->next_fire = min(F1_0/8, robptr->firing_wait[Difficulty_level]/2);
	} else {
		if ((robptr->weapon_type2 == weapon_none) || (gun_num != 0)) {
			ailp->next_fire = robptr->firing_wait[Difficulty_level];
			if (ailp->rapidfire_count >= robptr->rapidfire_count[Difficulty_level])
				ailp->rapidfire_count = 0;
		} else
			ailp->next_fire2 = robptr->firing_wait2[Difficulty_level];
	}
#endif
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void do_ai_robot_hit_attack(const vobjptridx_t robot, const objptridx_t playerobj, const vms_vector &collision_point)
{
	ai_local		*ailp = &robot->ctype.ai_info.ail;
	robot_info *robptr = &Robot_info[get_robot_id(robot)];

//#ifndef NDEBUG
	if (cheats.robotfiringsuspended)
		return;
//#endif

	//	If player is dead, stop firing.
	if (Objects[Players[Player_num].objnum].type == OBJ_GHOST)
		return;

	if (robptr->attack_type == 1) {
		if (ready_to_fire_weapon1(ailp, 0)) {
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				if (vm_vec_dist_quick(ConsoleObject->pos, robot->pos) < robot->size + ConsoleObject->size + F1_0*2)
				{
					collide_player_and_nasty_robot( playerobj, robot, collision_point );
#if defined(DXX_BUILD_DESCENT_II)
					if (robptr->energy_drain && Players[Player_num].energy) {
						Players[Player_num].energy -= robptr->energy_drain * F1_0;
						if (Players[Player_num].energy < 0)
							Players[Player_num].energy = 0;
					}
#endif
				}

			robot->ctype.ai_info.GOAL_STATE = AIS_RECO;
			set_next_fire_time(robot, ailp, robptr, 1);	//	1 = gun_num: 0 is special (uses next_fire2)
		}
	}

}

#if defined(DXX_BUILD_DESCENT_II)
// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit player given positions, player vel, elapsed time
static fix compute_lead_component(fix player_pos, fix robot_pos, fix player_vel, fix elapsed_time)
{
	return fixdiv(player_pos - robot_pos, elapsed_time) + player_vel;
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the player, returning point to fire at in fire_point.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot(vector_to_player, player_direction) must be in -LEAD_RANGE..LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
static int lead_player(const vobjptr_t objp, const vms_vector &fire_point, const vms_vector &believed_player_pos, int gun_num, vms_vector &fire_vec)
{
	fix			dot, player_speed, dist_to_player, max_weapon_speed, projected_time;
	vms_vector	player_movement_dir;
	int			weapon_type;
	weapon_info	*wptr;
	robot_info	*robptr;

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		return 0;

	player_movement_dir = ConsoleObject->mtype.phys_info.velocity;
	player_speed = vm_vec_normalize_quick(player_movement_dir);

	if (player_speed < MIN_LEAD_SPEED)
		return 0;

	auto vec_to_player = vm_vec_sub(believed_player_pos, fire_point);
	dist_to_player = vm_vec_normalize_quick(vec_to_player);
	if (dist_to_player > MAX_LEAD_DISTANCE)
		return 0;

	dot = vm_vec_dot(vec_to_player, player_movement_dir);

	if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
		return 0;

	//	Looks like it might be worth trying to lead the player.
	robptr = &Robot_info[get_robot_id(objp)];
	weapon_type = robptr->weapon_type;
	if (robptr->weapon_type2 != weapon_none)
		if (gun_num == 0)
			weapon_type = robptr->weapon_type2;

	wptr = &Weapon_info[weapon_type];
	max_weapon_speed = wptr->speed[Difficulty_level];
	if (max_weapon_speed < F1_0)
		return 0;

	//	Matter weapons:
	//	At Rookie or Trainee, don't lead at all.
	//	At higher skill levels, don't lead as well.  Accomplish this by screwing up max_weapon_speed.
	if (wptr->matter)
	{
		if (Difficulty_level <= 1)
			return 0;
		else
			max_weapon_speed *= (NDL-Difficulty_level);
	}

	projected_time = fixdiv(dist_to_player, max_weapon_speed);

	fire_vec.x = compute_lead_component(believed_player_pos.x, fire_point.x, ConsoleObject->mtype.phys_info.velocity.x, projected_time);
	fire_vec.y = compute_lead_component(believed_player_pos.y, fire_point.y, ConsoleObject->mtype.phys_info.velocity.y, projected_time);
	fire_vec.z = compute_lead_component(believed_player_pos.z, fire_point.z, ConsoleObject->mtype.phys_info.velocity.z, projected_time);

	vm_vec_normalize_quick(fire_vec);

	Assert(vm_vec_dot(fire_vec, objp->orient.fvec) < 3*F1_0/2);

	//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
	if (vm_vec_dot(fire_vec, objp->orient.fvec) < F1_0/2) {
		vm_vec_add2(fire_vec, vec_to_player);
		vm_vec_scale(fire_vec, F1_0/2);
		if (vm_vec_dot(fire_vec, objp->orient.fvec) < F1_0/2) {
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
static void ai_fire_laser_at_player(const vobjptridx_t obj, const vms_vector &fire_point, int gun_num
#if defined(DXX_BUILD_DESCENT_II)
									, const vms_vector &believed_player_pos
#endif
									)
{
	ai_local		*ailp = &obj->ctype.ai_info.ail;
	robot_info	*robptr = &Robot_info[get_robot_id(obj)];
	vms_vector	fire_vec;
	vms_vector	bpp_diff;

	Assert(robptr->attack_type == 0);	//	We should never be coming here for the green guy, as he has no laser!

	if (cheats.robotfiringsuspended)
		return;

	if (obj->control_type == CT_MORPH)
		return;

	//	If player is exploded, stop firing.
	if (Player_exploded)
		return;

#if defined(DXX_BUILD_DESCENT_II)
	//	If this robot is only awake because a camera woke it up, don't fire.
	if (obj->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
		return;

	if (obj->ctype.ai_info.dying_start_time)
		return;		//	No firing while in death roll.

	//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
	//	If you try to key the boss off obj->ctype.ai_info.dying_start_time, it will hose the endlevel stuff.
	if (Boss_dying_start_time && Robot_info[get_robot_id(obj)].boss_flag)
		return;
#endif

	//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		fix64	cloak_time = Ai_cloak_info[obj % MAX_AI_CLOAK_INFO].last_time;

		if (GameTime64 - cloak_time > CLOAK_TIME_MAX/4)
			if (d_rand() > fixdiv(GameTime64 - cloak_time, CLOAK_TIME_MAX)/2) {
				set_next_fire_time(obj, ailp, robptr, gun_num);
				return;
			}
	}

#if defined(DXX_BUILD_DESCENT_I)
	//	Set position to fire at based on difficulty level.
	bpp_diff.x = Believed_player_pos.x + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;
	bpp_diff.y = Believed_player_pos.y + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;
	bpp_diff.z = Believed_player_pos.z + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;

	//	Half the time fire at the player, half the time lead the player.
        if (d_rand() > 16384) {

		vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);

	} else {
		const auto player_direction_vector = vm_vec_sub(bpp_diff, bpp_diff);

		// If player is not moving, fire right at him!
		//	Note: If the robot fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the robot; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the player.  For now, just fire right at the player.
		if ((abs(player_direction_vector.x < 0x10000)) && (abs(player_direction_vector.y < 0x10000)) && (abs(player_direction_vector.z < 0x10000))) {

			vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);

		// Player is moving.  Determine where the player will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the player.
		//	So, that's why we write games, instead of guiding missiles...
		} else {
			vm_vec_sub(fire_vec, bpp_diff, fire_point);
			vm_vec_scale(fire_vec,fixmul(Weapon_info[Robot_info[get_robot_id(obj)].weapon_type].speed[Difficulty_level], FrameTime));

			vm_vec_add2(fire_vec, player_direction_vector);
			vm_vec_normalize_quick(fire_vec);

		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	//	Handle problem of a robot firing through a wall because its gun tip is on the other
	//	side of the wall than the robot's center.  For speed reasons, we normally only compute
	//	the vector from the gun point to the player.  But we need to know whether the gun point
	//	is separated from the robot's center by a wall.  If so, don't fire!
	if (obj->ctype.ai_info.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
		//	Well, the gun point is in a different segment than the robot's center.
		//	This is almost always ok, but it is not ok if something solid is in between.
		auto gun_segnum = find_point_seg(fire_point, obj->segnum);

		//	See if these segments are connected, which should almost always be the case.
		auto conn_side = find_connect_side(gun_segnum, &Segments[obj->segnum]);
		if (conn_side != -1) {
			//	They are connected via conn_side in segment obj->segnum.
			//	See if they are unobstructed.
			if (!(WALL_IS_DOORWAY(&Segments[obj->segnum], conn_side) & WID_FLY_FLAG)) {
				//	Can't fly through, so don't let this bot fire through!
				return;
			}
		} else {
			//	Well, they are not directly connected, so use find_vector_intersection to see if they are unobstructed.
			fvi_query	fq;
			fvi_info		hit_data;
			int			fate;

			fq.startseg				= obj->segnum;
			fq.p0						= &obj->pos;
			fq.p1						= &fire_point;
			fq.rad					= 0;
			fq.thisobjnum			= obj;
			fq.ignore_obj_list	= NULL;
			fq.flags					= FQ_TRANSWALL;

			fate = find_vector_intersection(&fq, &hit_data);
			if (fate != HIT_NONE) {
				Int3();		//	This bot's gun is poking through a wall, so don't fire.
				move_towards_segment_center(obj);		//	And decrease chances it will happen again.
				return;
			}
		}
	}

	//	Set position to fire at based on difficulty level and robot's aiming ability
	fix aim = FIRE_K*F1_0 - (FIRE_K-1)*(robptr->aim << 8);	//	F1_0 in bitmaps.tbl = same as used to be.  Worst is 50% more error.

	//	Robots aim more poorly during seismic disturbance.
	if (Seismic_tremor_magnitude) {
		fix	temp;

		temp = F1_0 - abs(Seismic_tremor_magnitude);
		if (temp < F1_0/2)
			temp = F1_0/2;

		aim = fixmul(aim, temp);
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
	while ((count < 4) && (dot < F1_0/4)) {
		bpp_diff.x = believed_player_pos.x + fixmul((d_rand()-16384) * (NDL-Difficulty_level-1) * 4, aim);
		bpp_diff.y = believed_player_pos.y + fixmul((d_rand()-16384) * (NDL-Difficulty_level-1) * 4, aim);
		bpp_diff.z = believed_player_pos.z + fixmul((d_rand()-16384) * (NDL-Difficulty_level-1) * 4, aim);

		vm_vec_normalized_dir_quick(fire_vec, bpp_diff, fire_point);
		dot = vm_vec_dot(obj->orient.fvec, fire_vec);
		count++;
	}
	}
player_led: ;
#endif

	int			weapon_type;
	weapon_type = robptr->weapon_type;
#if defined(DXX_BUILD_DESCENT_II)
	if (robptr->weapon_type2 != weapon_none)
		if (gun_num == 0)
			weapon_type = robptr->weapon_type2;
#endif

	Laser_create_new_easy( fire_vec, fire_point, obj, static_cast<weapon_type_t>(weapon_type), 1);

	if (Game_mode & GM_MULTI)
	{
		ai_multi_send_robot_position(obj, -1);
		multi_send_robot_fire(obj, obj->ctype.ai_info.CURRENT_GUN, fire_vec);
	}

	create_awareness_event(obj, PA_NEARBY_ROBOT_FIRED);

	set_next_fire_time(obj, ailp, robptr, gun_num);

#if defined(DXX_BUILD_DESCENT_I)
	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (robptr->boss_flag)
		Last_teleport_time -= Boss_teleport_interval/2;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
//	if dot_based set, then speed is based on direction of movement relative to heading
static void move_towards_vector(const vobjptr_t objp, const vms_vector &vec_goal, int dot_based)
{
	physics_info	*pptr = &objp->mtype.phys_info;
	fix				speed, dot, max_speed;
	robot_info		*robptr = &Robot_info[get_robot_id(objp)];

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards player as usual.

	const auto vel = vm_vec_normalized_quick(pptr->velocity);
	dot = vm_vec_dot(vel, objp->orient.fvec);

#if defined(DXX_BUILD_DESCENT_I)
	dot_based = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robot_is_thief(robptr))
		dot = (F1_0+dot)/2;
#endif

	if (dot_based && (dot < 3*F1_0/4)) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		pptr->velocity.x = pptr->velocity.x/2 + fixmul(vec_goal.x, FrameTime*32);
		pptr->velocity.y = pptr->velocity.y/2 + fixmul(vec_goal.y, FrameTime*32);
		pptr->velocity.z = pptr->velocity.z/2 + fixmul(vec_goal.z, FrameTime*32);
	} else {
		pptr->velocity.x += fixmul(vec_goal.x, FrameTime*64) * (Difficulty_level+5)/4;
		pptr->velocity.y += fixmul(vec_goal.y, FrameTime*64) * (Difficulty_level+5)/4;
		pptr->velocity.z += fixmul(vec_goal.z, FrameTime*64) * (Difficulty_level+5)/4;
	}

	speed = vm_vec_mag_quick(pptr->velocity);
	max_speed = robptr->max_speed[Difficulty_level];

	//	Green guy attacks twice as fast as he moves away.
#if defined(DXX_BUILD_DESCENT_I)
	if (robptr->attack_type == 1)
#elif defined(DXX_BUILD_DESCENT_II)
	if ((robptr->attack_type == 1) || robot_is_thief(robptr) || robptr->kamikaze)
#endif
		max_speed *= 2;

	if (speed > max_speed) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static
#endif
void move_towards_player(const vobjptr_t objp, const vms_vector &vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	move_towards_vector(objp, vec_to_player, 1);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means normal slide about.  fast_flag = 0 means no evasion.
static void move_around_player(const vobjptridx_t objp, const vms_vector &vec_to_player, int fast_flag)
{
	physics_info	*pptr = &objp->mtype.phys_info;
	fix				speed;
	int				dir;
	vms_vector		evade_vector;

	if (fast_flag == 0)
		return;

	dir = ((objp) ^ ((d_tick_count + 3*(objp)) >> 5)) & 3;

	Assert((dir >= 0) && (dir <= 3));

	switch (dir) {
		case 0:
			evade_vector.x = fixmul(vec_to_player.z, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player.y, FrameTime*32);
			evade_vector.z = fixmul(-vec_to_player.x, FrameTime*32);
			break;
		case 1:
			evade_vector.x = fixmul(-vec_to_player.z, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player.y, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player.x, FrameTime*32);
			break;
		case 2:
			evade_vector.x = fixmul(-vec_to_player.y, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player.x, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player.z, FrameTime*32);
			break;
		case 3:
			evade_vector.x = fixmul(vec_to_player.y, FrameTime*32);
			evade_vector.y = fixmul(-vec_to_player.x, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player.z, FrameTime*32);
			break;
		default:
			Error("Function move_around_player: Bad case.");
	}

	const robot_info		*robptr = &Robot_info[get_robot_id(objp)];
	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = vm_vec_dot(vec_to_player, objp->orient.fvec);
		if ((dot > robptr->field_of_view[Difficulty_level]) && !(ConsoleObject->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			if (!robptr->strength)
				damage_scale = F1_0;
			else
				damage_scale = fixdiv(objp->shields, robptr->strength);
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

	speed = vm_vec_mag_quick(pptr->velocity);
	if (speed > robptr->max_speed[Difficulty_level]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
static void move_away_from_player(const vobjptridx_t objp, const vms_vector &vec_to_player, int attack_type)
{
	fix				speed;
	physics_info	*pptr = &objp->mtype.phys_info;
	int				objref;

	pptr->velocity.x -= fixmul(vec_to_player.x, FrameTime*16);
	pptr->velocity.y -= fixmul(vec_to_player.y, FrameTime*16);
	pptr->velocity.z -= fixmul(vec_to_player.z, FrameTime*16);

	if (attack_type) {
		//	Get value in 0..3 to choose evasion direction.
		objref = ((objp) ^ ((d_tick_count + 3*(objp)) >> 5)) & 3;

		switch (objref) {
			case 0:
				vm_vec_scale_add2(pptr->velocity, objp->orient.uvec, FrameTime << 5);
				break;
			case 1:
				vm_vec_scale_add2(pptr->velocity, objp->orient.uvec, -FrameTime << 5);
				break;
			case 2:
				vm_vec_scale_add2(pptr->velocity, objp->orient.rvec, FrameTime << 5);
				break;
			case 3:
				vm_vec_scale_add2(pptr->velocity, objp->orient.rvec, -FrameTime << 5);
				break;
			default:	Int3();	//	Impossible, bogus value on objref, must be in 0..3
		}
	}


	speed = vm_vec_mag_quick(pptr->velocity);

	const robot_info		*robptr = &Robot_info[get_robot_id(objp)];
	if (speed > robptr->max_speed[Difficulty_level]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_STILL).
static void ai_move_relative_to_player(const vobjptridx_t objp, ai_local *ailp, fix dist_to_player, const vms_vector &vec_to_player, fix circle_distance, int evade_only, int player_visibility)
{
	const robot_info	*robptr = &Robot_info[get_robot_id(objp)];

	Assert(player_visibility != -1);

	//	See if should take avoidance.

	// New way, green guys don't evade:	if ((robptr->attack_type == 0) && (objp->ctype.ai_info.danger_laser_num != -1)) {
	if (objp->ctype.ai_info.danger_laser_num != object_none) {
		const vobjptridx_t dobjp = vobjptridx(objp->ctype.ai_info.danger_laser_num);

		if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == objp->ctype.ai_info.danger_laser_signature)) {
			fix			dot, dist_to_laser, field_of_view;
			vms_vector	laser_fvec;

			field_of_view = robptr->field_of_view[Difficulty_level];

			auto vec_to_laser = vm_vec_sub(dobjp->pos, objp->pos);
			dist_to_laser = vm_vec_normalize_quick(vec_to_laser);
			dot = vm_vec_dot(vec_to_laser, objp->orient.fvec);

			if (dot > field_of_view || robot_is_companion(robptr))
			{
				fix			laser_robot_dot;

				//	The laser is seen by the robot, see if it might hit the robot.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (dobjp->render_type == RT_POLYOBJ)
					laser_fvec = dobjp->orient.fvec;
				else {		//	Not a polyobj, get velocity and normalize.
					laser_fvec = vm_vec_normalized_quick(dobjp->mtype.phys_info.velocity);	//dobjp->orient.fvec;
				}
				const auto laser_vec_to_robot = vm_vec_normalized_quick(vm_vec_sub(objp->pos, dobjp->pos));
				laser_robot_dot = vm_vec_dot(laser_fvec, laser_vec_to_robot);

				if ((laser_robot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
					int	evade_speed;

					ai_evaded = 1;
					evade_speed = robptr->evade_speed[Difficulty_level];
					move_around_player(objp, vec_to_player, evade_speed);
				}
			}
			return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if ((!robptr->attack_type) && (!robot_is_thief(robptr)) && evade_only)
		return;

	//	If we fall out of above, then no object to be avoided.
	objp->ctype.ai_info.danger_laser_num = object_none;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (robptr->attack_type == 1) {
		if ((!ready_to_fire_weapon1(ailp, robptr->firing_wait[Difficulty_level]/4) && (dist_to_player < F1_0*30)) || Player_is_dead) {
			//	1/4 of time, move around player, 3/4 of time, move away from player
			if (d_rand() < 8192) {
				move_around_player(objp, vec_to_player, -1);
			} else {
				move_away_from_player(objp, vec_to_player, 1);
			}
		} else {
			move_towards_player(objp, vec_to_player);
		}
	}
	else if (robot_is_thief(robptr))
	{
		move_towards_player(objp, vec_to_player);
	}
	else {
#if defined(DXX_BUILD_DESCENT_I)
		if (dist_to_player < circle_distance)
			move_away_from_player(objp, vec_to_player, 0);
		else if (dist_to_player < circle_distance*2)
			move_around_player(objp, vec_to_player, -1);
		else
			move_towards_player(objp, vec_to_player);
#elif defined(DXX_BUILD_DESCENT_II)
		int	objval = ((objp) & 0x0f) ^ 0x0a;

		//	Changes here by MK, 12/29/95.  Trying to get rid of endless circling around bots in a large room.
		if (robptr->kamikaze) {
			move_towards_player(objp, vec_to_player);
		} else if (dist_to_player < circle_distance)
			move_away_from_player(objp, vec_to_player, 0);
		else if ((dist_to_player < (3+objval)*circle_distance/2) && !ready_to_fire_weapon1(ailp, -F1_0)) {
			move_around_player(objp, vec_to_player, -1);
		} else {
			if (ready_to_fire_weapon1(ailp, -(F1_0 + (objval << 12))) && player_visibility) {
				//	Usually move away, but sometimes move around player.
				if ((((GameTime64 >> 18) & 0x0f) ^ objval) > 4) {
					move_away_from_player(objp, vec_to_player, 0);
				} else {
					move_around_player(objp, vec_to_player, -1);
				}
			} else
				move_towards_player(objp, vec_to_player);
		}
#endif
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
void make_random_vector(vms_vector &vec)
{
	vec.x = (d_rand() - 16384) | 1;	// make sure we don't create null vector
	vec.y = d_rand() - 16384;
	vec.z = d_rand() - 16384;
	vm_vec_normalize_quick(vec);
}

//	-------------------------------------------------------------------------------------------------------------------
objnum_t	Break_on_object = object_none;

static void do_firing_stuff(const vobjptr_t obj, int player_visibility, const vms_vector &vec_to_player)
{
#if defined(DXX_BUILD_DESCENT_I)
	if (player_visibility >= 1)
#elif defined(DXX_BUILD_DESCENT_II)
	if ((Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD ) || (player_visibility >= 1))
#endif
	{
		//	Now, if in robot's field of view, lock onto player
		fix	dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
		if ((dot >= 7*F1_0/8) || (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
			ai_static	*aip = &obj->ctype.ai_info;
			ai_local		*ailp = &obj->ctype.ai_info.ail;

			switch (aip->GOAL_STATE) {
				case AIS_NONE:
				case AIS_REST:
				case AIS_SRCH:
				case AIS_LOCK:
					aip->GOAL_STATE = AIS_FIRE;
					if (ailp->player_awareness_type <= PA_NEARBY_ROBOT_FIRED) {
						ailp->player_awareness_type = PA_NEARBY_ROBOT_FIRED;
						ailp->player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
					}
					break;
			}
		} else if (dot >= F1_0/2) {
			ai_static	*aip = &obj->ctype.ai_info;
			switch (aip->GOAL_STATE) {
				case AIS_NONE:
				case AIS_REST:
				case AIS_SRCH:
					aip->GOAL_STATE = AIS_LOCK;
					break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void do_ai_robot_hit(const vobjptridx_t objp, int type)
{
	if (objp->control_type == CT_AI) {
		if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_PLAYER_COLLISION))
			switch (objp->ctype.ai_info.behavior) {
#if defined(DXX_BUILD_DESCENT_I)
				case AIM_HIDE:
					objp->ctype.ai_info.SUBMODE = AISM_GOHIDE;
					break;
				case AIM_STILL:
					objp->ctype.ai_info.ail.mode = AIM_CHASE_OBJECT;
					break;
#elif defined(DXX_BUILD_DESCENT_II)
				case AIB_STILL:
				{
					int	r;

					//	Attack robots (eg, green guy) shouldn't have behavior = still.
					Assert(Robot_info[get_robot_id(objp)].attack_type == 0);

					r = d_rand();
					//	1/8 time, charge player, 1/4 time create path, rest of time, do nothing
					ai_local		*ailp = &objp->ctype.ai_info.ail;
					if (r < 4096) {
						create_path_to_player(objp, 10, 1);
						objp->ctype.ai_info.behavior = AIB_STATION;
						objp->ctype.ai_info.hide_segment = objp->segnum;
						ailp->mode = AIM_CHASE_OBJECT;
					} else if (r < 4096+8192) {
						create_n_segment_path(objp, d_rand()/8192 + 2, segment_none);
						ailp->mode = AIM_FOLLOW_PATH;
					}
					break;
				}
#endif
			}
	}

}
#ifndef NDEBUG
int	Do_ai_flag=1;
#endif

#define	CHASE_TIME_LENGTH		(F1_0*8)
#define	DEFAULT_ROBOT_SOUND_VOLUME		F1_0
int		Robot_sound_volume=DEFAULT_ROBOT_SOUND_VOLUME;

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
static void compute_vis_and_vec(const vobjptridx_t objp, vms_vector &pos, ai_local *ailp, vms_vector &vec_to_player, int *player_visibility, const robot_info *robptr, int *flag)
{
	if (!*flag) {
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
			fix			delta_time, dist;
			int			cloak_index = (objp) % MAX_AI_CLOAK_INFO;

			delta_time = GameTime64 - Ai_cloak_info[cloak_index].last_time;
			if (delta_time > F1_0*2) {
				Ai_cloak_info[cloak_index].last_time = GameTime64;
				const auto randvec = make_random_vector();
				vm_vec_scale_add2(Ai_cloak_info[cloak_index].last_position, randvec, 8*delta_time );
			}

			dist = vm_vec_normalized_dir_quick(vec_to_player, Ai_cloak_info[cloak_index].last_position, pos);
			*player_visibility = player_is_visible_from_object(objp, pos, robptr->field_of_view[Difficulty_level], vec_to_player);
			// *player_visibility = 2;

			if ((ailp->next_misc_sound_time < GameTime64) && (ready_to_fire_any_weapon(robptr, ailp, F1_0)) && (dist < F1_0*20))
			{
				ailp->next_misc_sound_time = GameTime64 + (d_rand() + F1_0) * (7 - Difficulty_level) / 1;
				digi_link_sound_to_pos( robptr->see_sound, objp->segnum, 0, pos, 0 , Robot_sound_volume);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			vm_vec_normalized_dir_quick(vec_to_player, Believed_player_pos, pos);
			if ((vec_to_player.x == 0) && (vec_to_player.y == 0) && (vec_to_player.z == 0)) {
				vec_to_player.x = F1_0;
			}
			*player_visibility = player_is_visible_from_object(objp, pos, robptr->field_of_view[Difficulty_level], vec_to_player);

			//	This horrible code added by MK in desperation on 12/13/94 to make robots wake up as soon as they
			//	see you without killing frame rate.
			{
				ai_static	*aip = &objp->ctype.ai_info;
			if ((*player_visibility == 2) && (ailp->previous_visibility != 2))
				if ((aip->GOAL_STATE == AIS_REST) || (aip->CURRENT_STATE == AIS_REST)) {
					aip->GOAL_STATE = AIS_FIRE;
					aip->CURRENT_STATE = AIS_FIRE;
				}
			}

#if defined(DXX_BUILD_DESCENT_I)
			if (!Player_exploded)
#endif
			if ((ailp->previous_visibility != *player_visibility) && (*player_visibility == 2))
			{
				if (ailp->previous_visibility == 0) {
					if (ailp->time_player_seen + F1_0/2 < GameTime64) {
						digi_link_sound_to_pos( robptr->see_sound, objp->segnum, 0, pos, 0 , Robot_sound_volume);
						ailp->time_player_sound_attacked = GameTime64;
						ailp->next_misc_sound_time = GameTime64 + F1_0 + d_rand()*4;
					}
				} else if (ailp->time_player_sound_attacked + F1_0/4 < GameTime64) {
					digi_link_sound_to_pos( robptr->attack_sound, objp->segnum, 0, pos, 0 , Robot_sound_volume);
					ailp->time_player_sound_attacked = GameTime64;
				}
			} 

			if ((*player_visibility == 2) && (ailp->next_misc_sound_time < GameTime64)) {
				ailp->next_misc_sound_time = GameTime64 + (d_rand() + F1_0) * (7 - Difficulty_level) / 2;
				digi_link_sound_to_pos( robptr->attack_sound, objp->segnum, 0, pos, 0 , Robot_sound_volume);
			}
			ailp->previous_visibility = *player_visibility;
		}

		*flag = 1;

#if defined(DXX_BUILD_DESCENT_II)
		//	@mk, 09/21/95: If player view is not obstructed and awareness is at least as high as a nearby collision,
		//	act is if robot is looking at player.
		if (ailp->player_awareness_type >= PA_NEARBY_ROBOT_FIRED)
			if (*player_visibility == 1)
				*player_visibility = 2;
#endif

		if (*player_visibility) {
			ailp->time_player_seen = GameTime64;
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move object one object radii from current position towards segment center.
//	If segment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(const vobjptr_t objp)
{
/* ZICO's change of 20081103:
   Make move to segment center smoother by using move_towards vector.
   Bot's should not jump around and maybe even intersect with each other!
   In case it breaks something what I do not see, yet, old code is still there. */
	auto segnum = objp->segnum;
	vms_vector	vec_to_center;

	const auto segment_center = compute_segment_center(&Segments[segnum]);
	vm_vec_normalized_dir_quick(vec_to_center, segment_center, objp->pos);
	move_towards_vector(objp, vec_to_center, 1);
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robot.
//	Brains, avoid robots, companions can open doors.
//	objp == NULL means treat as buddy.
int ai_door_is_openable(_ai_door_is_openable_objptr objp, const vcsegptr_t segp, int sidenum)
{
	if (!IS_CHILD(segp->children[sidenum]))
		return 0;		//trap -2 (exit side)

	auto wall_num = segp->sides[sidenum].wall_num;

	if (wall_num == wall_none)		//if there's no door at all...
		return 0;				//..then say it can't be opened

	//	The mighty console object can open all doors (for purposes of determining paths).
	if (objp == ConsoleObject) {

		if (Walls[wall_num].type == WALL_DOOR)
			return 1;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ((get_robot_id(objp) == ROBOT_BRAIN) || (objp->ctype.ai_info.behavior == AIB_RUN_FROM))
	{

		if (wall_num != wall_none)
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED))
				return 1;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	wall	*wallp;
	wallp = &Walls[wall_num];

	if (objp == nullptr || Robot_info[get_robot_id(objp)].companion == 1)
	{
		int	ailp_mode;

		if (wallp->flags & WALL_BUDDY_PROOF) {
			if ((wallp->type == WALL_DOOR) && (wallp->state == WALL_DOOR_CLOSED))
				return 0;
			else if (wallp->type == WALL_CLOSED)
				return 0;
			else if ((wallp->type == WALL_ILLUSION) && !(wallp->flags & WALL_ILLUSION_OFF))
				return 0;
		}
			
		if (wallp->keys != KEY_NONE) {
			if (wallp->keys == KEY_BLUE)
				return (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY);
			else if (wallp->keys == KEY_GOLD)
				return (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY);
			else if (wallp->keys == KEY_RED)
				return (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY);
		}

		if ((wallp->type != WALL_DOOR) && (wallp->type != WALL_CLOSED))
			return 1;

		//	If Buddy is returning to player, don't let him think he can get through triggered doors.
		//	It's only valid to think that if the player is going to get him through.  But if he's
		//	going to the player, the player is probably on the opposite side.
		if (objp == nullptr)
			ailp_mode = Objects[Buddy_objnum].ctype.ai_info.ail.mode;
		else
			ailp_mode = objp->ctype.ai_info.ail.mode;

		// -- if (Buddy_got_stuck) {
		if (ailp_mode == AIM_GOTO_PLAYER) {
			if ((wallp->type == WALL_BLASTABLE) && (wallp->state != WALL_BLASTED))
				return 0;
			if (wallp->type == WALL_CLOSED)
				return 0;
			if (wallp->type == WALL_DOOR) {
				if ((wallp->flags & WALL_DOOR_LOCKED) && (wallp->state == WALL_DOOR_CLOSED))
					return 0;
			}
		}
		// -- }

		if ((ailp_mode != AIM_GOTO_PLAYER) && (wallp->controlling_trigger != -1)) {
			int	clip_num = wallp->clip_num;

			if (clip_num == -1)
				return 1;
			else if (WallAnims[clip_num].flags & WCF_HIDDEN) {
				if (wallp->state == WALL_DOOR_CLOSED)
					return 0;
				else
					return 1;
			} else
				return 1;
		}

		if (wallp->type == WALL_DOOR)  {
			if (wallp->type == WALL_BLASTABLE)
				return 1;
			else {
				int	clip_num = wallp->clip_num;

				if (clip_num == -1)
					return 1;
				//	Buddy allowed to go through secret doors to get to player.
				else if ((ailp_mode != AIM_GOTO_PLAYER) && (WallAnims[clip_num].flags & WCF_HIDDEN)) {
					if (wallp->state == WALL_DOOR_CLOSED)
						return 0;
					else
						return 1;
				} else
					return 1;
			}
		}
	} else if ((get_robot_id(objp) == ROBOT_BRAIN) || (objp->ctype.ai_info.behavior == AIB_RUN_FROM) || (objp->ctype.ai_info.behavior == AIB_SNIPE)) {
		if (wall_num != wall_none)
		{
			if ((wallp->type == WALL_DOOR) && (wallp->keys == KEY_NONE) && !(wallp->flags & WALL_DOOR_LOCKED))
				return 1;
			else if (wallp->keys != KEY_NONE) {	//	Allow bots to open doors to which player has keys.
				if (wallp->keys & Players[Player_num].flags)
					return 1;
			}
		}
	}
#endif
	return 0;
}

//	-----------------------------------------------------------------------------------------------------------
//	Return side of openable door in segment, if any.  If none, return -1.
static int openable_doors_in_segment(const vcsegptr_t segp)
{
	int	i;
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (segp->sides[i].wall_num != wall_none) {
			int	wall_num = segp->sides[i].wall_num;
#if defined(DXX_BUILD_DESCENT_I)
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED))
#elif defined(DXX_BUILD_DESCENT_II)
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED) && !(WallAnims[Walls[wall_num].clip_num].flags & WCF_HIDDEN))
#endif
				return i;
		}
	}

	return -1;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an object of size size at pos *pos intersects a (player or robot or control center) in segment *segp.
static int check_object_object_intersection(const vms_vector &pos, fix size, const vcsegptr_t segp)
{
	//	If this would intersect with another object (only check those in this segment), then try to move.
	range_for (auto curobjp, objects_in(*segp))
	{
		if ((curobjp->type == OBJ_PLAYER) || (curobjp->type == OBJ_ROBOT) || (curobjp->type == OBJ_CNTRLCEN)) {
			if (vm_vec_dist_quick(pos, curobjp->pos) < size + curobjp->size)
				return 1;
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return objnum if object created, else return -1.
//	If pos == NULL, pick random spot in segment.
static objptridx_t create_gated_robot(const vsegptridx_t segp, int object_id, const vms_vector *pos)
{
	const robot_info	*robptr = &Robot_info[object_id];
	int		count=0;
	fix		objsize = Polygon_models[robptr->model_num].rad;
	int		default_behavior;
#if defined(DXX_BUILD_DESCENT_I)
	const int maximum_gated_robots = 2*Difficulty_level + 3;
#elif defined(DXX_BUILD_DESCENT_II)
	const int maximum_gated_robots = 2*Difficulty_level + 6;
#endif
#if defined(DXX_BUILD_DESCENT_II)
	if (GameTime64 - Last_gate_time < Gate_interval)
		return object_none;
#endif

	range_for (auto i, highest_valid(Objects))
		if (Objects[i].type == OBJ_ROBOT)
			if (Objects[i].matcen_creator == BOSS_GATE_MATCEN_NUM)
				count++;

	if (count > maximum_gated_robots)
	{
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return object_none;
	}

	const auto object_pos = pos ? *pos : pick_random_point_in_seg(segp);

	//	See if legal to place object here.  If not, move about in segment and try again.
	if (check_object_object_intersection(object_pos, objsize, segp)) {
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return object_none;
	}

	auto objp = obj_create(OBJ_ROBOT, object_id, segp, object_pos, &vmd_identity_matrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ);

	if ( objp == object_none ) {
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return objp;
	}

	Net_create_objnums[0] = objp; // A convenient global to get objnum back to caller for multiplayer

	//Set polygon-object-specific data

	objp->rtype.pobj_info.model_num = robptr->model_num;
	objp->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	objp->mtype.phys_info.mass = robptr->mass;
	objp->mtype.phys_info.drag = robptr->drag;

	objp->mtype.phys_info.flags |= (PF_LEVELLING);

	objp->shields = robptr->strength;
	objp->matcen_creator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.

#if defined(DXX_BUILD_DESCENT_I)
	default_behavior = AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = AIB_RUN_FROM;
#elif defined(DXX_BUILD_DESCENT_II)
	objp->lifeleft = F1_0*30;	//	Gated in robots only live 30 seconds.
	default_behavior = Robot_info[get_robot_id(objp)].behavior;
#endif
	init_ai_object(objp, default_behavior, segment_none );		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	object_create_explosion(segp, object_pos, i2f(10), VCLIP_MORPHING_ROBOT );
	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, segp, 0, object_pos, 0 , F1_0);
	morph_start(objp);

	Last_gate_time = GameTime64;

	Players[Player_num].num_robots_level++;
	Players[Player_num].num_robots_total++;

	return objp;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make object objp gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return objnum if robot successfully created, else return -1
objptridx_t gate_in_robot(int type, segnum_t segnum)
{
	Assert((segnum >= 0) && (segnum <= Highest_segment_index));
	return create_gated_robot(segnum, type, NULL);
}

#if defined(DXX_BUILD_DESCENT_I)
static objptridx_t gate_in_robot(int type)
{
	auto segnum = Boss_gate_segs[(d_rand() * Boss_gate_segs.count()) >> 15];
	return gate_in_robot(type, segnum);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
static int boss_fits_in_seg(const vobjptridx_t boss_objp, const vsegptridx_t segp)
{
	int			posnum;
	const auto segcenter = compute_segment_center(segp);
	for (posnum=0; posnum<9; posnum++) {
		if (posnum > 0) {
			vms_vector	vertex_pos;

			Assert((posnum-1 >= 0) && (posnum-1 < 8));
			vertex_pos = Vertices[segp->verts[posnum-1]];
			vm_vec_avg(boss_objp->pos, vertex_pos, segcenter);
		} else
			boss_objp->pos = segcenter;

		obj_relink(boss_objp, segp);
		if (!object_intersects_wall(boss_objp))
			return 1;
	}

	return 0;
}

#if defined(DXX_BUILD_DESCENT_II)
// --------------------------------------------------------------------------------------------------------------------
//	Create a Buddy bot.
//	This automatically happens when you bring up the Buddy menu in a debug version.
//	It is available as a cheat in a non-debug (release) version.
void create_buddy_bot(void)
{
	int	buddy_id;
	for (buddy_id=0;; buddy_id++)
	{
		if (!(buddy_id < N_robot_types))
			return;
		const robot_info *robptr = &Robot_info[buddy_id];
		if (robptr->companion)
			break;
	}
	const auto object_pos = compute_segment_center(&Segments[ConsoleObject->segnum]);
	create_morph_robot( &Segments[ConsoleObject->segnum], object_pos, buddy_id);
}
#endif

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segptr.
//	Set *num_segs.
//	Boss is allowed to teleport to segments he fits in (calls object_intersects_wall) and
//	he can reach from his initial position (calls find_connected_distance).
//	If size_check is set, then only add segment if boss can fit in it, else any segment is legal.
//	one_wall_hack added by MK, 10/13/95: A mega-hack!  Set to !0 to ignore the 
static void init_boss_segments(boss_special_segment_array_t &segptr, int size_check, int one_wall_hack)
{
#if defined(DXX_BUILD_DESCENT_I)
	one_wall_hack = 0;
#endif
	objnum_t			boss_objnum=object_none;

	segptr.clear();
#ifdef EDITOR
	Selected_segs.clear();
#endif

	//	See if there is a boss.  If not, quick out.
	range_for (auto i, highest_valid(Objects))
		if ((Objects[i].type == OBJ_ROBOT) && (Robot_info[get_robot_id(&Objects[i])].boss_flag))
		{
			boss_objnum = i; // if != 1 then there is more than one boss here.
			break;
		}

	if (boss_objnum != object_none) {
		vms_vector	original_boss_pos;
		const vobjptridx_t boss_objp = vobjptridx(boss_objnum);
		int			head, tail;
		int			seg_queue[QUEUE_SIZE];
		fix			boss_size_save;

		boss_size_save = boss_objp->size;
#if defined(DXX_BUILD_DESCENT_I)
		boss_objp->size = fixmul((F1_0/4)*3, boss_objp->size);
#endif
		// -- Causes problems!!	-- boss_objp->size = fixmul((F1_0/4)*3, boss_objp->size);
		auto original_boss_seg = boss_objp->segnum;
		original_boss_pos = boss_objp->pos;
		head = 0;
		tail = 0;
		seg_queue[head++] = original_boss_seg;

		segptr.emplace_back(original_boss_seg);
		#ifdef EDITOR
		Selected_segs.emplace_back(original_boss_seg);
		#endif

		visited_segment_bitarray_t visited;

		while (tail != head) {
			int		sidenum;
			segment	*segp = &Segments[seg_queue[tail++]];

			tail &= QUEUE_SIZE-1;

			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
				auto w = WALL_IS_DOORWAY(segp, sidenum);
				if ((w & WID_FLY_FLAG) || one_wall_hack)
				{
#if defined(DXX_BUILD_DESCENT_II)
					//	If we get here and w == WID_WALL, then we want to process through this wall, else not.
					if (IS_CHILD(segp->children[sidenum])) {
						if (one_wall_hack)
							one_wall_hack--;
					} else
						continue;
#endif

					if (!visited[segp->children[sidenum]]) {
						seg_queue[head++] = segp->children[sidenum];
						visited[segp->children[sidenum]] = true;
						head &= QUEUE_SIZE-1;
						if (head > tail) {
							if (head == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
						} else
							if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
	
						if ((!size_check) || boss_fits_in_seg(boss_objp, segp->children[sidenum])) {
							segptr.emplace_back(segp->children[sidenum]);
							#ifdef EDITOR
							Selected_segs.emplace_back(segp->children[sidenum]);
							#endif
							if (segptr.count() >= MAX_BOSS_TELEPORT_SEGS) {
								tail = head;
								sidenum=MAX_SIDES_PER_SEGMENT;
								break;
							}
						}
					}
				}
			}

		}

		boss_objp->size = boss_size_save;
		boss_objp->pos = original_boss_pos;
		obj_relink(boss_objp, original_boss_seg);

	}

}

// --------------------------------------------------------------------------------------------------------------------
static void teleport_boss(const vobjptridx_t objp)
{
	segnum_t			rand_segnum;
	int			rand_index;
	Assert(Boss_teleport_segs.count() > 0);

	//	Pick a random segment from the list of boss-teleportable-to segments.
	rand_index = (d_rand() * Boss_teleport_segs.count()) >> 15;
	rand_segnum = Boss_teleport_segs[rand_index];
	Assert((rand_segnum >= 0) && (rand_segnum <= Highest_segment_index));

	if (Game_mode & GM_MULTI)
		multi_send_boss_teleport(objp, rand_segnum);

	compute_segment_center(objp->pos, &Segments[rand_segnum]);
	obj_relink(objp, rand_segnum);

	Last_teleport_time = GameTime64;

	//	make boss point right at player
	const auto boss_dir = vm_vec_sub(Objects[Players[Player_num].objnum].pos, objp->pos);
	vm_vector_2_matrix(objp->orient, boss_dir, nullptr, nullptr);

	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, rand_segnum, 0, objp->pos, 0 , F1_0);
	digi_kill_sound_linked_to_object( objp);

	//	After a teleport, boss can fire right away.
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	ailp->next_fire = 0;
#if defined(DXX_BUILD_DESCENT_I)
	digi_link_sound_to_object2( SOUND_BOSS_SHARE_SEE, objp, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud
#elif defined(DXX_BUILD_DESCENT_II)
	ailp->next_fire2 = 0;
	digi_link_sound_to_object2( Robot_info[get_robot_id(objp)].see_sound, objp, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud
#endif

}

//	----------------------------------------------------------------------
void start_boss_death_sequence(const vobjptr_t objp)
{
	const robot_info	*robptr = &Robot_info[get_robot_id(objp)];
	if (robptr->boss_flag) {
		Boss_dying = 1;
		Boss_dying_start_time = GameTime64;
	}

}

//	----------------------------------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static void do_boss_dying_frame(const vobjptridx_t objp)
{
	fix	boss_roll_val, temp;

	boss_roll_val = fixdiv(GameTime64 - Boss_dying_start_time, BOSS_DEATH_DURATION);

	fix_sincos(fixmul(boss_roll_val, boss_roll_val), &temp, &objp->mtype.phys_info.rotvel.x);
	fix_sincos(boss_roll_val, &temp, &objp->mtype.phys_info.rotvel.y);
	fix_sincos(boss_roll_val-F1_0/8, &temp, &objp->mtype.phys_info.rotvel.z);

	objp->mtype.phys_info.rotvel.x = (GameTime64 - Boss_dying_start_time)/9;
	objp->mtype.phys_info.rotvel.y = (GameTime64 - Boss_dying_start_time)/5;
	objp->mtype.phys_info.rotvel.z = (GameTime64 - Boss_dying_start_time)/7;

	if (Boss_dying_start_time + BOSS_DEATH_DURATION - BOSS_DEATH_SOUND_DURATION < GameTime64) {
		if (!Boss_dying_sound_playing) {
			Boss_dying_sound_playing = 1;
			digi_link_sound_to_object2( SOUND_BOSS_SHARE_DIE, objp, 0, F1_0*4, F1_0*1024 );	//	F1_0*512 means play twice as loud
                } else if (d_rand() < FrameTime*16)
                        create_small_fireball_on_object(objp, (F1_0 + d_rand()) * 8, 0);
        } else if (d_rand() < FrameTime*8)
                create_small_fireball_on_object(objp, (F1_0/2 + d_rand()) * 8, 1);

	if (Boss_dying_start_time + BOSS_DEATH_DURATION < GameTime64 || GameTime64+(F1_0*2) < Boss_dying_start_time)
	{
		Boss_dying_start_time=GameTime64; // make sure following only happens one time!
		do_controlcen_destroyed_stuff(object_none);
		explode_object(objp, F1_0/4);
		digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, F1_0*512);
	}
}

static int do_any_robot_dying_frame(const vobjptridx_t)
{
	return 0;
}
#elif defined(DXX_BUILD_DESCENT_II)
//	objp points at a boss.  He was presumably just hit and he's supposed to create a bot at the hit location *pos.
objnum_t boss_spew_robot(const vobjptr_t objp, const vms_vector &pos)
{
	int		boss_index;

	boss_index = Robot_info[get_robot_id(objp)].boss_flag - BOSS_D2;

	Assert((boss_index >= 0) && (boss_index < NUM_D2_BOSSES));

	auto segnum = find_point_seg(pos, objp->segnum);
	if (segnum == segment_none) {
		return object_none;
	}	

	const objptridx_t newobjp = create_gated_robot( segnum, Spew_bots[boss_index][(Max_spew_bots[boss_index] * d_rand()) >> 15], &pos);
 
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
			vm_vec_sub(newobjp->mtype.phys_info.velocity, pos, objp->pos);
			vm_vec_normalize_quick(newobjp->mtype.phys_info.velocity);
			vm_vec_scale(newobjp->mtype.phys_info.velocity, F1_0*128);
		}
	}

	return newobjp;
}

// --------------------------------------------------------------------------------------------------------------------
//	Call this each time the player starts a new ship.
void init_ai_for_ship(void)
{
	range_for (auto &i, Ai_cloak_info) {
		i.last_time = GameTime64;
		i.last_segment = ConsoleObject->segnum;
		i.last_position = ConsoleObject->pos;
	}
}

//	----------------------------------------------------------------------
void start_robot_death_sequence(const vobjptr_t objp)
{
	objp->ctype.ai_info.dying_start_time = GameTime64;
	objp->ctype.ai_info.dying_sound_playing = 0;
	objp->ctype.ai_info.SKIP_AI_COUNT = 0;

}

//	----------------------------------------------------------------------
//	General purpose robot-dies-with-death-roll-and-groan code.
//	Return true if object just died.
//	scale: F1_0*4 for boss, much smaller for much smaller guys
static int do_robot_dying_frame(const vobjptridx_t objp, fix64 start_time, fix roll_duration, sbyte *dying_sound_playing, int death_sound, fix expl_scale, fix sound_scale)
{
	fix	roll_val, temp;
	fix	sound_duration;

	if (!roll_duration)
		roll_duration = F1_0/4;

	roll_val = fixdiv(GameTime64 - start_time, roll_duration);

	fix_sincos(fixmul(roll_val, roll_val), &temp, &objp->mtype.phys_info.rotvel.x);
	fix_sincos(roll_val, &temp, &objp->mtype.phys_info.rotvel.y);
	fix_sincos(roll_val-F1_0/8, &temp, &objp->mtype.phys_info.rotvel.z);

	objp->mtype.phys_info.rotvel.x = (GameTime64 - start_time)/9;
	objp->mtype.phys_info.rotvel.y = (GameTime64 - start_time)/5;
	objp->mtype.phys_info.rotvel.z = (GameTime64 - start_time)/7;

	if (GameArg.SndDigiSampleRate)
		sound_duration = fixdiv(GameSounds[digi_xlat_sound(death_sound)].length,GameArg.SndDigiSampleRate);
	else
		sound_duration = F1_0;

	if (start_time + roll_duration - sound_duration < GameTime64) {
		if (!*dying_sound_playing) {
			*dying_sound_playing = 1;
			digi_link_sound_to_object2( death_sound, objp, 0, sound_scale, sound_scale*256 );	//	F1_0*512 means play twice as loud
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
static void do_boss_dying_frame(const vobjptridx_t objp)
{
	int	rval;

	rval = do_robot_dying_frame(objp, Boss_dying_start_time, BOSS_DEATH_DURATION, &Boss_dying_sound_playing, Robot_info[get_robot_id(objp)].deathroll_sound, F1_0*4, F1_0*4);

	if (rval)
	{
		Boss_dying_start_time=GameTime64; // make sure following only happens one time!
		do_controlcen_destroyed_stuff(object_none);
		explode_object(objp, F1_0/4);
		digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, F1_0*512);
	}
}

//	----------------------------------------------------------------------
static int do_any_robot_dying_frame(const vobjptridx_t objp)
{
	if (objp->ctype.ai_info.dying_start_time) {
		int	rval, death_roll;

		death_roll = Robot_info[get_robot_id(objp)].death_roll;
		rval = do_robot_dying_frame(objp, objp->ctype.ai_info.dying_start_time, min(death_roll/2+1,6)*F1_0, &objp->ctype.ai_info.dying_sound_playing, Robot_info[get_robot_id(objp)].deathroll_sound, death_roll*F1_0/8, death_roll*F1_0/2);

		if (rval) {
			objp->ctype.ai_info.dying_start_time = GameTime64; // make sure following only happens one time!
			explode_object(objp, F1_0/4);
			digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp, 0, F2_0, F1_0*512);
			if ((Current_level_num < 0) && (Robot_info[get_robot_id(objp)].thief))
				recreate_thief(objp);
		}

		return 1;
	}

	return 0;
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
int ai_multiplayer_awareness(const vobjptridx_t objp, int awareness_level)
{
	int	rval=1;

	if (Game_mode & GM_MULTI) {
		if (awareness_level == 0)
			return 0;
		rval = multi_can_move_robot(objp, awareness_level);
	}

	return rval;

}

#ifndef NDEBUG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
#if defined(DXX_BUILD_DESCENT_I)
static void do_boss_stuff(const vobjptridx_t objp)
{
#ifndef NDEBUG
	if (objp->shields != Prev_boss_shields) {
		Prev_boss_shields = objp->shields;
	}
#endif

	if (!Boss_dying) {
		if (objp->ctype.ai_info.CLOAKED == 1) {
			if ((GameTime64 - Boss_cloak_start_time > BOSS_CLOAK_DURATION/3) && (Boss_cloak_end_time - GameTime64 > BOSS_CLOAK_DURATION/3) && (GameTime64 - Last_teleport_time > Boss_teleport_interval)) {
				if (ai_multiplayer_awareness(objp, 98))
					teleport_boss(objp);
			} else if (Boss_hit_this_frame) {
				Boss_hit_this_frame = 0;
				Last_teleport_time -= Boss_teleport_interval/4;
			}

			if (GameTime64 > Boss_cloak_end_time)
				objp->ctype.ai_info.CLOAKED = 0;
		} else {
			if ((GameTime64 - Boss_cloak_end_time > Boss_cloak_interval) || Boss_hit_this_frame) {
				if (ai_multiplayer_awareness(objp, 95))
				{
					Boss_hit_this_frame = 0;
					Boss_cloak_start_time = GameTime64;
					Boss_cloak_end_time = GameTime64+Boss_cloak_duration;
					objp->ctype.ai_info.CLOAKED = 1;
					if (Game_mode & GM_MULTI)
						multi_send_boss_cloak(objp);
				}
			}
		}
	} else
		do_boss_dying_frame(objp);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(F1_0*150)


// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
static void do_super_boss_stuff(const vobjptridx_t objp, fix dist_to_player, int player_visibility)
{
	static int eclip_state = 0;
	do_boss_stuff(objp);

	// Only master player can cause gating to occur.
	if ((Game_mode & GM_MULTI) && !multi_i_am_master())
                return;

	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (Game_mode & GM_MULTI)) {
		if (GameTime64 - Last_gate_time > Gate_interval/2) {
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

		if (GameTime64 - Last_gate_time > Gate_interval)
			if (ai_multiplayer_awareness(objp, 99)) {
				uint_fast32_t randtype = (d_rand() * MAX_GATE_INDEX) >> 15;

				Assert(randtype < MAX_GATE_INDEX);
				randtype = Super_boss_gate_list[randtype];
				Assert(randtype < N_robot_types);

				const objptridx_t rtval = gate_in_robot(randtype);
				if (rtval != object_none && (Game_mode & GM_MULTI))
				{
					multi_send_boss_create_robot(objp, randtype, rtval);
					map_objnum_local_to_local(Net_create_objnums[0]);
				}
			}	
	}
}

#elif defined(DXX_BUILD_DESCENT_II)
static void do_boss_stuff(const vobjptridx_t objp, int player_visibility)
{
	int	boss_id, boss_index;

	boss_id = Robot_info[get_robot_id(objp)].boss_flag;

	Assert((boss_id >= BOSS_D2) && (boss_id < BOSS_D2 + NUM_D2_BOSSES));

	boss_index = boss_id - BOSS_D2;

#ifndef NDEBUG
	if (objp->shields != Prev_boss_shields) {
		Prev_boss_shields = objp->shields;
	}
#endif

	//	@mk, 10/13/95:  Reason:
	//		Level 4 boss behind locked door.  But he's allowed to teleport out of there.  So he
	//		teleports out of there right away, and blasts player right after first door.
	if (!player_visibility && (GameTime64 - Boss_hit_time > F1_0*2))
		return;

	if (!Boss_dying && Boss_teleports[boss_index]) {
		if (objp->ctype.ai_info.CLOAKED == 1) {
			Boss_hit_time = GameTime64;	//	Keep the cloak:teleport process going.
			if ((GameTime64 - Boss_cloak_start_time > BOSS_CLOAK_DURATION/3) && (Boss_cloak_end_time - GameTime64 > BOSS_CLOAK_DURATION/3) && (GameTime64 - Last_teleport_time > Boss_teleport_interval)) {
				if (ai_multiplayer_awareness(objp, 98))
					teleport_boss(objp);
			} else if (GameTime64 - Boss_hit_time > F1_0*2) {
				Last_teleport_time -= Boss_teleport_interval/4;
			}

			if (GameTime64 > Boss_cloak_end_time || GameTime64 < Boss_cloak_start_time)
				objp->ctype.ai_info.CLOAKED = 0;
		} else if ((GameTime64 - Boss_cloak_end_time > Boss_cloak_interval) || (GameTime64 - Boss_cloak_end_time < -Boss_cloak_duration)) {
			if (ai_multiplayer_awareness(objp, 95)) {
				Boss_cloak_start_time = GameTime64;
				Boss_cloak_end_time = GameTime64+Boss_cloak_duration;
				objp->ctype.ai_info.CLOAKED = 1;
				if (Game_mode & GM_MULTI)
					multi_send_boss_cloak(objp);
			}
		}
	}

}
#endif


static void ai_multi_send_robot_position(const vobjptridx_t obj, int force)
{
	if (Game_mode & GM_MULTI) 
	{
		if (force != -1)
			multi_send_robot_position(obj, 1);
		else
			multi_send_robot_position(obj, 0);
	}
	return;
}

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this object should be allowed to fire at the player.
static int maybe_ai_do_actual_firing_stuff(const vobjptr_t obj, ai_static *aip)
{
	if (Game_mode & GM_MULTI)
		if ((aip->GOAL_STATE != AIS_FLIN) && (get_robot_id(obj) != ROBOT_BRAIN))
			if (aip->CURRENT_STATE == AIS_FIRE)
				return 1;

	return 0;
}

#if defined(DXX_BUILD_DESCENT_I)
static void ai_do_actual_firing_stuff(const vobjptridx_t obj, ai_static *aip, ai_local *ailp, const robot_info *robptr, const vms_vector &vec_to_player, fix dist_to_player, const vms_vector &gun_point, int player_visibility, int object_animates, int gun_num)
{
	fix	dot;

	if (player_visibility == 2) {
		//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
		// if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE)) && (ailp->next_fire <= 0))
		if (!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) {
			dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < robptr->n_guns) {
					if (robptr->attack_type == 1) {
						if (!Player_exploded && (dist_to_player < obj->size + ConsoleObject->size + F1_0*2)) {		// robptr->circle_distance[Difficulty_level] + ConsoleObject->size)
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(obj, ConsoleObject, obj->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point.x == 0) && (gun_point.y == 0) && (gun_point.z == 0)) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							ai_fire_laser_at_player(obj, gun_point, 0);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_FOLLOW_PATH) && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_STILL)))
						ailp->mode = AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->n_guns)
					aip->CURRENT_GUN = 0;
			}
		}
	} else if (Weapon_info[robptr->weapon_type].homing_flag == 1) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE))
			&& ready_to_fire_weapon1(ailp, 0)
			&& (vm_vec_dist_quick(Hit_pos, obj->pos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(obj, gun_point, 0);

			aip->GOAL_STATE = AIS_RECO;
			ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		}
	}
}
#elif defined(DXX_BUILD_DESCENT_II)

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if player is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
static void ai_do_actual_firing_stuff(const vobjptridx_t obj, ai_static *aip, ai_local *ailp, const robot_info *robptr, const vms_vector &vec_to_player, fix dist_to_player, vms_vector &gun_point, int player_visibility, int object_animates, int gun_num)
{
	fix	dot;

	if ((player_visibility == 2) || (Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD )) {
		vms_vector	fire_pos;

		fire_pos = Believed_player_pos;

		//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
		//	So, fire at Last_fired_upon_player_pos instead of the player position.
		if (!robptr->attack_type && (player_visibility != 2))
			fire_pos = Last_fired_upon_player_pos;

		//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
		//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
		if (!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) {
			dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
			if ((dot >= 7*F1_0/8) || ((dot > F1_0/4) &&  robptr->boss_flag)) {

				if (gun_num < robptr->n_guns) {
					if (robptr->attack_type == 1) {
						if (!Player_exploded && (dist_to_player < obj->size + ConsoleObject->size + F1_0*2)) {		// robptr->circle_distance[Difficulty_level] + ConsoleObject->size)
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(obj, ConsoleObject, obj->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point.x == 0) && (gun_point.y == 0) && (gun_point.z == 0)) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping away...)
							if (gun_num != 0) {
								if (ready_to_fire_weapon1(ailp, 0)) {
									ai_fire_laser_at_player(obj, gun_point, gun_num, fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

								if (ready_to_fire_weapon2(robptr, ailp, 0)) {
									calc_gun_point(gun_point, obj, 0);
									ai_fire_laser_at_player(obj, gun_point, 0, fire_pos);
									Last_fired_upon_player_pos = fire_pos;
								}

							} else if (ready_to_fire_weapon1(ailp, 0)) {
								ai_fire_laser_at_player(obj, gun_point, gun_num, fire_pos);
								Last_fired_upon_player_pos = fire_pos;
							}
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aip->behavior != AIB_RUN_FROM)
						 && (aip->behavior != AIB_STILL)
						 && (aip->behavior != AIB_SNIPE)
						 && (aip->behavior != AIB_FOLLOW)
						 && (!robptr->attack_type)
						 && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_STILL)))
						ailp->mode = AIM_CHASE_OBJECT;
				}

				aip->GOAL_STATE = AIS_RECO;
				ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.  If has 2 gun types, select gun #1, if exists.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->n_guns)
				{
					if ((robptr->n_guns == 1) || (robptr->weapon_type2 == weapon_none))
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
				}
			}
		}
	} else if ( ((!robptr->attack_type) && (Weapon_info[Robot_info[get_robot_id(obj)].weapon_type].homing_flag == 1)) || (((Robot_info[get_robot_id(obj)].weapon_type2 != weapon_none) && (Weapon_info[Robot_info[get_robot_id(obj)].weapon_type2].homing_flag == 1))) ) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE))
			&& (((ready_to_fire_weapon1(ailp, 0)) && (aip->CURRENT_GUN != 0)) || ((ready_to_fire_weapon2(robptr, ailp, 0)) && (aip->CURRENT_GUN == 0)))
			 && (vm_vec_dist_quick(Hit_pos, obj->pos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(obj, gun_point, gun_num, Believed_player_pos);

			aip->GOAL_STATE = AIS_RECO;
			ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= robptr->n_guns)
				aip->CURRENT_GUN = 0;
		}
	} else {


	//	---------------------------------------------------------------

		vms_vector	vec_to_last_pos;

		if (d_rand()/2 < fixmul(FrameTime, (Difficulty_level << 12) + 0x4000)) {
		if ((!object_animates || ready_to_fire_any_weapon(robptr, ailp, 0)) && (Dist_to_last_fired_upon_player_pos < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			vm_vec_normalized_dir_quick(vec_to_last_pos, Believed_player_pos, obj->pos);
			dot = vm_vec_dot(obj->orient.fvec, vec_to_last_pos);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < robptr->n_guns) {
					if (robptr->attack_type == 1) {
						if (!Player_exploded && (dist_to_player < obj->size + ConsoleObject->size + F1_0*2)) {		// robptr->circle_distance[Difficulty_level] + ConsoleObject->size) {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(obj, ConsoleObject, obj->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point.x == 0) && (gun_point.y == 0) && (gun_point.z == 0)) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-type system, 06/05/95 (life is slipping away...)
							if (gun_num != 0) {
								if (ready_to_fire_weapon1(ailp, 0))
									ai_fire_laser_at_player(obj, gun_point, gun_num, Last_fired_upon_player_pos);

								if (ready_to_fire_weapon2(robptr, ailp, 0)) {
									calc_gun_point(gun_point, obj, 0);
									ai_fire_laser_at_player(obj, gun_point, 0, Last_fired_upon_player_pos);
								}

							} else if (ready_to_fire_weapon1(ailp, 0))
								ai_fire_laser_at_player(obj, gun_point, gun_num, Last_fired_upon_player_pos);
						}
					}

					//	Wants to fire, so should go into chase mode, probably.
					if ( (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && ((ailp->mode == AIM_FOLLOW_PATH) || (ailp->mode == AIM_STILL)))
						ailp->mode = AIM_CHASE_OBJECT;
				}
				aip->GOAL_STATE = AIS_RECO;
				ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

				// Switch to next gun for next fire.
				aip->CURRENT_GUN++;
				if (aip->CURRENT_GUN >= robptr->n_guns)
				{
					if (robptr->n_guns == 1)
						aip->CURRENT_GUN = 0;
					else
						aip->CURRENT_GUN = 1;
				}
			}
		}
		}


	//	---------------------------------------------------------------


	}

}



// ----------------------------------------------------------------------------
void init_ai_frame(void)
{
	int ab_state;

	Dist_to_last_fired_upon_player_pos = vm_vec_dist_quick(Last_fired_upon_player_pos, Believed_player_pos);

	ab_state = Afterburner_charge && Controls.state.afterburner && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER);

	if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON) || ab_state) {
		ai_do_cloak_stuff();
	}
}

// ----------------------------------------------------------------------------
// Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70
static void make_nearby_robot_snipe(void)
{
	unsigned bfs_length;
	segnum_t bfs_list[MNRS_SEG_MAX];

	create_bfs_list(ConsoleObject->segnum, bfs_list, bfs_length);

	range_for (auto &i, partial_range(bfs_list, bfs_length)) {
		range_for (auto objp, objects_in(Segments[i]))
		{
			robot_info *robptr = &Robot_info[get_robot_id(objp)];

			if ((objp->type == OBJ_ROBOT) && (get_robot_id(objp) != ROBOT_BRAIN)) {
				if ((objp->ctype.ai_info.behavior != AIB_SNIPE) && (objp->ctype.ai_info.behavior != AIB_RUN_FROM) && !Robot_info[get_robot_id(objp)].boss_flag && !robot_is_companion(robptr)) {
					objp->ctype.ai_info.behavior = AIB_SNIPE;
					objp->ctype.ai_info.ail.mode = AIM_SNIPE_ATTACK;
					return;
				}
			}
		}
	}
}

objnum_t Ai_last_missile_camera = object_none;

static int openable_door_on_near_path(const object &obj, const ai_static &aip)
{
	if (aip.path_length < 1)
		return 0;
	if (openable_doors_in_segment(vsegptr(obj.segnum)) != -1)
		return 1;
	if (aip.path_length < 2)
		return 0;
	size_t idx;
	idx = aip.hide_index + aip.cur_path_index + aip.PATH_DIR;
	if (idx < sizeof(Point_segs) / sizeof(Point_segs[0]) && openable_doors_in_segment(vsegptr(Point_segs[idx].segnum)) != -1)
		return 1;
	if (aip.path_length < 3)
		return 0;
	idx = aip.hide_index + aip.cur_path_index + 2*aip.PATH_DIR;
	if (idx < sizeof(Point_segs) / sizeof(Point_segs[0]) && openable_doors_in_segment(vsegptr(Point_segs[idx].segnum)) != -1)
		return 1;
	return 0;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
void do_ai_frame(const vobjptridx_t obj)
{
	const objnum_t &objnum = obj;
	ai_static	*aip = &obj->ctype.ai_info;
	ai_local		*ailp = &obj->ctype.ai_info.ail;
	fix			dist_to_player;
	vms_vector	vec_to_player;
	fix			dot;
	int			player_visibility=-1;
	int			obj_ref;
	int			object_animates;
	int			new_goal_state;
	int			visibility_and_vec_computed = 0;
	int			previous_visibility;
	vms_vector	gun_point;
	vms_vector	vis_vec_pos;

#if defined(DXX_BUILD_DESCENT_II)
	ailp->next_action_time -= FrameTime;
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

	const robot_info *robptr = &Robot_info[get_robot_id(obj)];
	Assert(robptr->always_0xabcd == 0xabcd);

	if (do_any_robot_dying_frame(obj))
		return;

	// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	// Else, you can turn a big nasty robot into a wimp by firing flares at it.
	// This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ready_to_fire_any_weapon(robptr, ailp, 0)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3(); // This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	if (!Do_ai_flag)
		return;

	if (Break_on_object != object_none)
		if ((obj) == Break_on_object)
			Int3(); // Contact Mike: This is a debug break
#endif

	//Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR));
	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		aip->behavior = AIB_NORMAL;
	}

	Assert(obj->segnum != segment_none);
	Assert(get_robot_id(obj) < N_robot_types);

	obj_ref = objnum ^ d_tick_count;

	if (!ready_to_fire_weapon1(ailp, -F1_0*8))
		ailp->next_fire -= FrameTime;

#if defined(DXX_BUILD_DESCENT_I)
	Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;
#elif defined(DXX_BUILD_DESCENT_II)
	if (robptr->weapon_type2 != weapon_none) {
		if (!ready_to_fire_weapon2(robptr, ailp, -F1_0*8))
			ailp->next_fire2 -= FrameTime;
	} else
		ailp->next_fire2 = F1_0*8;
#endif

	if (ailp->time_since_processed < F1_0*256)
		ailp->time_since_processed += FrameTime;

	previous_visibility = ailp->previous_visibility;    //  Must get this before we toast the master copy!

#if defined(DXX_BUILD_DESCENT_I)
	if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
		Believed_player_pos = ConsoleObject->pos;
#elif defined(DXX_BUILD_DESCENT_II)
	// If only awake because of a camera, make that the believed player position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (Ai_last_missile_camera != object_none))
		Believed_player_pos = Objects[Ai_last_missile_camera].pos;
	else {
		if (cheats.robotskillrobots) {
			vis_vec_pos = obj->pos;
			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility) {
				objnum_t min_obj = object_none;
				fix min_dist = F1_0*200, cur_dist;

				range_for (auto ii, highest_valid(Objects))
					if ((Objects[ii].type == OBJ_ROBOT) && (ii != objnum)) {
						cur_dist = vm_vec_dist_quick(obj->pos, Objects[ii].pos);

						if (cur_dist < F1_0*100)
							if (object_to_object_visibility(obj, &Objects[ii], FQ_TRANSWALL))
								if (cur_dist < min_dist) {
									min_obj = ii;
									min_dist = cur_dist;
								}
					}
				if (min_obj != object_none) {
					Believed_player_pos = Objects[min_obj].pos;
					Believed_player_seg = Objects[min_obj].segnum;
					vm_vec_normalized_dir_quick(vec_to_player, Believed_player_pos, obj->pos);
				} else
					goto _exit_cheat;
			} else
				goto _exit_cheat;
		} else {
_exit_cheat:
			visibility_and_vec_computed = 0;
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				Believed_player_pos = ConsoleObject->pos;
			else
				Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;
		}
	}
#endif
	dist_to_player = vm_vec_dist_quick(Believed_player_pos, obj->pos);

	// If this robot can fire, compute visibility from gun position.
	// Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
#if defined(DXX_BUILD_DESCENT_I)
	if (ready_to_fire_any_weapon(robptr, ailp, 0) && (dist_to_player < F1_0*200) && (robptr->n_guns) && !(robptr->attack_type))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((previous_visibility || !(obj_ref & 3)) && ready_to_fire_any_weapon(robptr, ailp, 0) && (dist_to_player < F1_0*200) && (robptr->n_guns) && !(robptr->attack_type))
#endif
	{
		// Since we passed ready_to_fire_any_weapon(), either next_fire or next_fire2 <= 0.  calc_gun_point from relevant one.
		// If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
#if defined(DXX_BUILD_DESCENT_II)
		if (!ready_to_fire_weapon1(ailp, 0))
			calc_gun_point(gun_point, obj, 0);
		else
#endif
			calc_gun_point(gun_point, obj, aip->CURRENT_GUN);
		vis_vec_pos = gun_point;
	} else {
		vis_vec_pos = obj->pos;
		vm_vec_zero(gun_point);
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - 
	// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
#if defined(DXX_BUILD_DESCENT_I)
	if ((aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(Game_mode & GM_MULTI))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(Game_mode & GM_MULTI) && (robot_is_companion(robptr) != 1) && (robot_is_thief(robptr) != 1))
#endif
		if (Overall_agitation > 70) {
			if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/4)) {
				if (d_rand() * (Overall_agitation - 40) > F1_0*5) {
					create_path_to_player(obj, 4 + Overall_agitation/8 + Difficulty_level, 1);
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
	if ((ailp->retry_count) && !(Game_mode & GM_MULTI)) {
		ailp->consecutive_retries += ailp->retry_count;
		ailp->retry_count = 0;
		if (ailp->consecutive_retries > 3) {
			switch (ailp->mode) {
#if defined(DXX_BUILD_DESCENT_II)
				case AIM_GOTO_PLAYER:
					move_towards_segment_center(obj);
					create_path_to_player(obj, 100, 1);
					break;
				case AIM_GOTO_OBJECT:
					Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
					break;
#endif
				case AIM_CHASE_OBJECT:
					create_path_to_player(obj, 4 + Overall_agitation/8 + Difficulty_level, 1);
					break;
				case AIM_STILL:
#if defined(DXX_BUILD_DESCENT_I)
					if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION)))	//	Behavior is still, so don't follow path.
#elif defined(DXX_BUILD_DESCENT_II)
					if (robptr->attack_type)
						move_towards_segment_center(obj);
					else if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION) || (aip->behavior == AIB_FOLLOW)))    // Behavior is still, so don't follow path.
#endif
						attempt_to_resume_path(obj);
					break;
				case AIM_FOLLOW_PATH:
					if (Game_mode & GM_MULTI)
						ailp->mode = AIM_STILL;
					else
						attempt_to_resume_path(obj);
					break;
				case AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					create_n_segment_path(obj, 5, segment_none);
					ailp->mode = AIM_RUN_FROM_OBJECT;
					break;
#if defined(DXX_BUILD_DESCENT_I)
				case AIM_HIDE:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					if (Overall_agitation > (50 - Difficulty_level*4))
						create_path_to_player(obj, 4 + Overall_agitation/8, 1);
					else {
						create_n_segment_path(obj, 5, segment_none);
					}
					break;
#elif defined(DXX_BUILD_DESCENT_II)
				case AIM_BEHIND:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					break;
#endif
				case AIM_OPEN_DOOR:
					create_n_segment_path_to_door(obj, 5, segment_none);
					break;
				#ifndef NDEBUG
				case AIM_FOLLOW_PATH_2:
					Int3(); // Should never happen!
					break;
				#endif
			}
			ailp->consecutive_retries = 0;
		}
	} else
		ailp->consecutive_retries /= 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If in materialization center, exit
	if (!(Game_mode & GM_MULTI) && (Segments[obj->segnum].special == SEGMENT_IS_ROBOTMAKER)) {
#if defined(DXX_BUILD_DESCENT_II)
		if (Station[Segments[obj->segnum].value].Enabled)
#endif
		{
			ai_follow_path(obj, 1, NULL);    // 1 = player is visible, which might be a lie, but it works.
			return;
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Decrease player awareness due to the passage of time.
	if (ailp->player_awareness_type) {
		if (ailp->player_awareness_time > 0) {
			ailp->player_awareness_time -= FrameTime;
			if (ailp->player_awareness_time <= 0) {
				ailp->player_awareness_time = F1_0*2;   //new: 11/05/94
				ailp->player_awareness_type--;          //new: 11/05/94
			}
		} else {
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*2;
			//aip->GOAL_STATE = AIS_REST;
		}
	} else
		aip->GOAL_STATE = AIS_REST;                     //new: 12/13/94


	if (Player_is_dead && (ailp->player_awareness_type == 0))
		if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/8)) {
			if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(obj, 30))
					return;
				ai_multi_send_robot_position(obj, -1);

				if (!((ailp->mode == AIM_FOLLOW_PATH) && (aip->cur_path_index < aip->path_length-1)))
#if defined(DXX_BUILD_DESCENT_II)
					if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM))
#endif
					{
						if (dist_to_player < F1_0*30)
							create_n_segment_path(obj, 5, segment_none);
						else
							create_path_to_player(obj, 20, 1);
					}
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION))
#if defined(DXX_BUILD_DESCENT_I)
	{
		if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM) && (get_robot_id(obj) != ROBOT_BRAIN))
			ailp->mode = AIM_CHASE_OBJECT;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	{
		compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		if (player_visibility == 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility = 2;
	} else if (((obj_ref&3) == 0) && !previous_visibility && (dist_to_player < F1_0*100)) {
		fix sval, rval;

		rval = d_rand();
		sval = (dist_to_player * (Difficulty_level+1))/64;

		if ((fixmul(rval, sval) < FrameTime) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
			ailp->player_awareness_time = F1_0*3;
			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility == 1) {
				player_visibility = 2;
			}
		}
	}
#endif

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aip->GOAL_STATE == AIS_FLIN) && (aip->CURRENT_STATE == AIS_FLIN))
		aip->GOAL_STATE = AIS_LOCK;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Note: Should only do these two function calls for objects which animate
	if (Animation_enabled && (dist_to_player < F1_0*100)) { // && !(Game_mode & GM_MULTI)) {
		object_animates = do_silly_animation(obj);
		if (object_animates)
			ai_frame_animation(obj);
	} else {
		// If Object is supposed to animate, but we don't let it animate due to distance, then
		// we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;        // If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (robptr->boss_flag) {
	case 0:
		break;

	case 1:
#if defined(DXX_BUILD_DESCENT_I)
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			dist_to_player /= 4;

			do_boss_stuff(obj);
			dist_to_player *= 4;
			break;
#endif
	case 2:
		// FIXME!!!!
#if defined(DXX_BUILD_DESCENT_I)
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			{	int pv = player_visibility;
				fix	dtp = dist_to_player/4;

			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = vm_vec_dist_quick(ConsoleObject->pos, obj->pos)/4;
			}

			do_super_boss_stuff(obj, dtp, pv);
			}
#endif
		break;

	default:
#if defined(DXX_BUILD_DESCENT_I)
			Int3();	//	Bogus boss flag value.
#elif defined(DXX_BUILD_DESCENT_II)
		{
			int	pv;

			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;

			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			pv = player_visibility;

			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
			}

			do_boss_stuff(obj, pv);
		}
		break;
#endif
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Time-slice, don't process all the time, purely an efficiency hack.
	// Guys whose behavior is station and are not at their hide segment get processed anyway.
#if defined(DXX_BUILD_DESCENT_I)
	if (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION-1) { // If robot got hit, he gets to attack player always!
#ifndef NDEBUG
		if (Break_on_object != objnum)
#endif
		{    // don't time slice if we're interested in this object.
			if ((dist_to_player > F1_0*250) && (ailp->time_since_processed <= F1_0*2))
				return;
			else if (!((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != obj->segnum))) {
				if ((dist_to_player > F1_0*150) && (ailp->time_since_processed <= F1_0))
					return;
				else if ((dist_to_player > F1_0*100) && (ailp->time_since_processed <= F1_0/2))
					return;
			}
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (!((aip->behavior == AIB_SNIPE) && (ailp->mode != AIM_SNIPE_WAIT)) && !robot_is_companion(robptr) && !robot_is_thief(robptr) && (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION-1)) { // If robot got hit, he gets to attack player always!
#ifndef NDEBUG
		if (Break_on_object != objnum)
#endif
		{    // don't time slice if we're interested in this object.
			if ((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != obj->segnum)) {
				if (dist_to_player > F1_0*250)  // station guys not at home always processed until 250 units away.
					return;
			} else if ((!ailp->previous_visibility) && ((dist_to_player >> 7) > ailp->time_since_processed)) {  // 128 units away (6.4 segments) processed after 1 second.
				return;
			}
		}
	}
#endif

	// Reset time since processed, but skew objects so not everything
	// processed synchronously, else we get fast frames with the
	// occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp->time_since_processed = - ((objnum & 0x03) * FrameTime ) / 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	//	Perform special ability
	switch (get_robot_id(obj)) {
		case ROBOT_BRAIN:
			// Robots function nicely if behavior is Station.  This
			// means they won't move until they can see the player, at
			// which time they will start wandering about opening doors.
			if (ConsoleObject->segnum == obj->segnum) {
				if (!ai_multiplayer_awareness(obj, 97))
					return;
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				move_away_from_player(obj, vec_to_player, 0);
				ai_multi_send_robot_position(obj, -1);
			} else if (ailp->mode != AIM_STILL) {
				int r;

				r = openable_doors_in_segment(vsegptr(obj->segnum));
				if (r != -1) {
					ailp->mode = AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, segment_none);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(obj, -1);
				}

#if defined(DXX_BUILD_DESCENT_II)
				if (ailp->next_action_time < 0) {
					compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if (player_visibility) {
						make_nearby_robot_snipe();
						ailp->next_action_time = (NDL - Difficulty_level) * 2*F1_0;
					}
				}
#endif
			} else {
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, segment_none);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(obj, -1);
				}
			}
			break;
		default:
			break;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (aip->behavior == AIB_SNIPE) {
		if ((Game_mode & GM_MULTI) && !robot_is_thief(robptr)) {
			aip->behavior = AIB_NORMAL;
			ailp->mode = AIM_CHASE_OBJECT;
			return;
		}

		if (!(obj_ref & 3) || previous_visibility) {
			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
			if (ailp->mode == AIM_STILL)
				if (player_visibility || (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION))
					ailp->mode = AIM_SNIPE_ATTACK;

			if (!robot_is_thief(robptr) && (ailp->mode != AIM_STILL))
				do_snipe_frame(obj, dist_to_player, player_visibility, vec_to_player);
		} else if (!robot_is_thief(robptr) && !robot_is_companion(robptr))
			return;
	}

	// More special ability stuff, but based on a property of a robot, not its ID.
	if (robot_is_companion(robptr)) {

		compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_escort_frame(obj, dist_to_player, player_visibility);

		if (obj->ctype.ai_info.danger_laser_num != object_none) {
			object *dobjp = &Objects[obj->ctype.ai_info.danger_laser_num];

			if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == obj->ctype.ai_info.danger_laser_signature)) {
				fix circle_distance;
				circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
				ai_move_relative_to_player(obj, ailp, dist_to_player, vec_to_player, circle_distance, 1, player_visibility);
			}
		}

		if (ready_to_fire_any_weapon(robptr, ailp, 0)) {
			int do_stuff = 0;
			if (openable_door_on_near_path(*obj, *aip))
				do_stuff = 1;
			else if ((ailp->mode == AIM_GOTO_PLAYER) && (dist_to_player < 3*MIN_ESCORT_DISTANCE/2) && (vm_vec_dot(ConsoleObject->orient.fvec, vec_to_player) > -F1_0/4)) {
				do_stuff = 1;
			}

			if (do_stuff) {
				Laser_create_new_easy( obj->orient.fvec, obj->pos, obj, FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (!Buddy_allowed_to_talk) // If buddy not talking, make him fire flares less often.
					ailp->next_fire += d_rand()*4;
			}

		}
	}

	if (robot_is_thief(robptr)) {

		compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_thief_frame(obj, dist_to_player, player_visibility, vec_to_player);

		if (ready_to_fire_any_weapon(robptr, ailp, 0)) {
			int do_stuff = 0;
			if (openable_door_on_near_path(*obj, *aip))
				do_stuff = 1;

			if (do_stuff) {
				// @mk, 05/08/95: Firing flare from center of object, this is dumb...
				Laser_create_new_easy( obj->orient.fvec, obj->pos, obj, FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (Stolen_item_index == 0)     // If never stolen an item, fire flares less often (bad: Stolen_item_index wraps, but big deal)
					ailp->next_fire += d_rand()*4;
			}
		}
	}
#endif

	switch (ailp->mode) {
		case AIM_CHASE_OBJECT: {        // chasing player, sort of, chase if far, back off if close, circle in between
			fix circle_distance;

			circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
			// Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robptr->attack_type != 1)
				circle_distance += (objnum&0xf) * F1_0/2;

			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// @mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (previous_visibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(obj, 53)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				create_path_to_player(obj, 8, 1);
				ai_multi_send_robot_position(obj, -1);
			} else if ((player_visibility == 0) && (dist_to_player > F1_0*80) && (!(Game_mode & GM_MULTI))) {
				// If pretty far from the player, player cannot be seen
				// (obstructed) and in chase mode, switch to follow path mode.
				// This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->goal_segment = aip->hide_segment;
					create_path_to_station(obj, 15);
				} // -- this looks like a dumb thing to do...robots following paths far away from you! else create_n_segment_path(obj, 5, -1);
#if defined(DXX_BUILD_DESCENT_I)
				else
					create_n_segment_path(obj, 5, segment_none);
#endif
				break;
			}

			if ((aip->CURRENT_STATE == AIS_REST) && (aip->GOAL_STATE == AIS_REST)) {
				if (player_visibility) {
					if (d_rand() < FrameTime*player_visibility) {
						if (dist_to_player/256 < d_rand()*player_visibility) {
							aip->GOAL_STATE = AIS_SRCH;
							aip->CURRENT_STATE = AIS_SRCH;
						}
					}
				}
			}

			if (GameTime64 - ailp->time_player_seen > CHASE_TIME_LENGTH) {

				if (Game_mode & GM_MULTI)
					if (!player_visibility && (dist_to_player > F1_0*70)) {
						ailp->mode = AIM_STILL;
						return;
					}

				if (!ai_multiplayer_awareness(obj, 64)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
#if defined(DXX_BUILD_DESCENT_I)
				create_path_to_player(obj, 10, 1);
				ai_multi_send_robot_position(obj, -1);
#endif
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!ai_multiplayer_awareness(obj, 70)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				ai_move_relative_to_player(obj, ailp, dist_to_player, vec_to_player, circle_distance, 0, player_visibility);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
#if defined(DXX_BUILD_DESCENT_I)
					else
						ai_turn_randomly(vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
#endif
				}

				if (ai_evaded) {
					ai_multi_send_robot_position(obj, 1);
					ai_evaded = 0;
				}
				else
					ai_multi_send_robot_position(obj, -1);

				do_firing_stuff(obj, player_visibility, vec_to_player);
			}
			break;
		}

		case AIM_RUN_FROM_OBJECT:
			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility) {
				if (ailp->player_awareness_type == 0)
					ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;

			}

			// If in multiplayer, only do if player visible.  If not multiplayer, do always.
			if (!(Game_mode & GM_MULTI) || player_visibility)
				if (ai_multiplayer_awareness(obj, 75)) {
					ai_follow_path(obj, player_visibility, &vec_to_player);
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
			if (ready_to_fire_weapon1(ailp, 0) && (player_visibility)) {
				if (!ai_multiplayer_awareness(obj, 75))
					return;

				const auto fire_vec = vm_vec_negated(obj->orient.fvec);
				const auto fire_pos = vm_vec_add(obj->pos, fire_vec);

#if defined(DXX_BUILD_DESCENT_I)
				ailp->next_fire = F1_0*5;		//	Drop a proximity bomb every 5 seconds.
#elif defined(DXX_BUILD_DESCENT_II)
				ailp->next_fire = (F1_0/2)*(NDL+5 - Difficulty_level);      // Drop a proximity bomb every 5 seconds.
				if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					Laser_create_new_easy( fire_vec, fire_pos, obj, ROBOT_SUPERPROX_ID, 1);
				else
#endif
					Laser_create_new_easy( fire_vec, fire_pos, obj, PROXIMITY_ID, 1);

				if (Game_mode & GM_MULTI)
				{
					ai_multi_send_robot_position(obj, -1);
#if defined(DXX_BUILD_DESCENT_II)
					if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
						multi_send_robot_fire(obj, -2, fire_vec);
					else
#endif
						multi_send_robot_fire(obj, -1, fire_vec);
				}
			}
			break;

#if defined(DXX_BUILD_DESCENT_II)
		case AIM_GOTO_PLAYER:
		case AIM_GOTO_OBJECT:
			ai_follow_path(obj, 2, &vec_to_player);    // Follows path as if player can see robot.
			ai_multi_send_robot_position(obj, -1);
			break;
#endif

		case AIM_FOLLOW_PATH: {
			int anger_level = 65;

			if (aip->behavior == AIB_STATION)
				if (Point_segs[aip->hide_index + aip->path_length - 1].segnum == aip->hide_segment) {
					anger_level = 64;
				}

			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!ai_multiplayer_awareness(obj, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			ai_follow_path(obj, player_visibility, &vec_to_player);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

#if defined(DXX_BUILD_DESCENT_I)
			if ((aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM))
				do_firing_stuff(obj, player_visibility, vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM) && (get_robot_id(obj) != ROBOT_BRAIN))
			{
				if (robptr->attack_type == 0)
					ailp->mode = AIM_CHASE_OBJECT;
			}
			else if ((player_visibility == 0)
				&& (aip->behavior == AIB_NORMAL)
				&& (ailp->mode == AIM_FOLLOW_PATH)
				&& (aip->behavior != AIB_RUN_FROM))
			{
				ailp->mode = AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}
#elif defined(DXX_BUILD_DESCENT_II)
			if (aip->behavior != AIB_RUN_FROM)
				do_firing_stuff(obj, player_visibility, vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && (aip->behavior != AIB_RUN_FROM) && (get_robot_id(obj) != ROBOT_BRAIN) && (robot_is_companion(robptr) != 1) && (robot_is_thief(robptr) != 1))
			{
				if (robptr->attack_type == 0)
					ailp->mode = AIM_CHASE_OBJECT;
				// This should not just be distance based, but also time-since-player-seen based.
			}
			else if ((dist_to_player > F1_0*(20*(2*Difficulty_level + robptr->pursuit)))
				&& (GameTime64 - ailp->time_player_seen > (F1_0/2*(Difficulty_level+robptr->pursuit)))
				&& (player_visibility == 0)
				&& (aip->behavior == AIB_NORMAL)
				&& (ailp->mode == AIM_FOLLOW_PATH))
			{
				ailp->mode = AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}
#endif

			ai_multi_send_robot_position(obj, -1);

			break;
		}

#if defined(DXX_BUILD_DESCENT_I)
		case AIM_HIDE:
#elif defined(DXX_BUILD_DESCENT_II)
		case AIM_BEHIND:
#endif
			if (!ai_multiplayer_awareness(obj, 71)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

#if defined(DXX_BUILD_DESCENT_I)
 			ai_follow_path(obj, player_visibility, NULL);
#elif defined(DXX_BUILD_DESCENT_II)
			if (player_visibility == 2) {
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
					auto &rvec = ConsoleObject->orient.rvec;
					goal_vector = (vm_vec_dot(rvec, vec_to_player) > 0) ? vm_vec_negated(rvec) : rvec;
				}

				vm_vec_scale(goal_vector, 2*(ConsoleObject->size + obj->size + (((objnum*4 + d_tick_count) & 63) << 12)));
				auto goal_point = vm_vec_add(ConsoleObject->pos, goal_vector);
				const auto rand_vec = make_random_vector();
				vm_vec_scale_add2(goal_point, rand_vec, F1_0*8);
				const auto vec_to_goal = vm_vec_normalized_quick(vm_vec_sub(goal_point, obj->pos));
				move_towards_vector(obj, vec_to_goal, 0);
				ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			}
#endif

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_send_robot_position(obj, -1);
			break;

		case AIM_STILL:
			if ((dist_to_player < F1_0*120+Difficulty_level*F1_0*20) || (ailp->player_awareness_type >= PA_WEAPON_ROBOT_COLLISION-1)) {
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				// turn towards vector if visible this time or last time, or rand
				// new!
#if defined(DXX_BUILD_DESCENT_I)
				if ((player_visibility) || (previous_visibility) || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI)))
#elif defined(DXX_BUILD_DESCENT_II)
				if ((player_visibility == 2) || (previous_visibility == 2)) // -- MK, 06/09/95:  || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI)))
#endif
				{
					if (!ai_multiplayer_awareness(obj, 71)) {
						if (maybe_ai_do_actual_firing_stuff(obj, aip))
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}

				do_firing_stuff(obj, player_visibility, vec_to_player);
#if defined(DXX_BUILD_DESCENT_I)
				if (player_visibility) //	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility == 2) // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
#endif
				{
					if (robptr->attack_type == 1) {
						aip->behavior = AIB_NORMAL;
						if (!ai_multiplayer_awareness(obj, 80)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, vec_to_player, 0, 0, player_visibility);
						if (ai_evaded) {
							ai_multi_send_robot_position(obj, 1);
							ai_evaded = 0;
						}
						else
							ai_multi_send_robot_position(obj, -1);
					} else {
						// Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(obj, 81)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, vec_to_player, 0, 1, player_visibility);
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
					if (aip->behavior == AIB_STATION) {
						ailp->goal_segment = aip->hide_segment;
						create_path_to_station(obj, 15);
					}
					break;
				}
			}

			break;
		case AIM_OPEN_DOOR: {       // trying to open a door.
			Assert(get_robot_id(obj) == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(obj, 62))
				return;
			const auto center_point = compute_center_point_on_side(&Segments[obj->segnum], aip->GOALSIDE);
			const auto goal_vector = vm_vec_normalized_quick(vm_vec_sub(center_point, obj->pos));
			ai_turn_towards_vector(goal_vector, obj, robptr->turn_time[Difficulty_level]);
			move_towards_vector(obj, goal_vector, 0);
			ai_multi_send_robot_position(obj, -1);

			break;
		}

#if defined(DXX_BUILD_DESCENT_II)
		case AIM_SNIPE_WAIT:
			break;
		case AIM_SNIPE_RETREAT:
			// -- if (ai_multiplayer_awareness(obj, 53))
			// -- 	if (ailp->next_fire < -F1_0)
			// -- 		ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			break;
		case AIM_SNIPE_RETREAT_BACKWARDS:
		case AIM_SNIPE_ATTACK:
		case AIM_SNIPE_FIRE:
			if (ai_multiplayer_awareness(obj, 53)) {
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				if (robot_is_thief(robptr))
					ai_move_relative_to_player(obj, ailp, dist_to_player, vec_to_player, 0, 0, player_visibility);
				break;
			}
			break;

		case AIM_THIEF_WAIT:
		case AIM_THIEF_ATTACK:
		case AIM_THIEF_RETREAT:
		case AIM_WANDER:    // Used for Buddy Bot
			break;
#endif

		default:
			ailp->mode = AIM_CHASE_OBJECT;
			break;
	}       // end: switch (ailp->mode) {

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If the robot can see you, increase his awareness of you.
	// This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1); // Means it didn't get initialized!
	compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
#if defined(DXX_BUILD_DESCENT_I)
	if (player_visibility == 2)
		if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
#elif defined(DXX_BUILD_DESCENT_II)
	if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW) && (!robot_is_thief(robptr))) {
		if ((ailp->player_awareness_type == 0) && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
	}
#endif

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	Assert(ailp->player_awareness_type <= AIE_MAX);
	Assert(aip->CURRENT_STATE < AIS_MAX);
	Assert(aip->GOAL_STATE < AIS_MAX);

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	if (ailp->player_awareness_type) {
		new_goal_state = Ai_transition_table[ailp->player_awareness_type-1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) {
			// Decrease awareness, else this robot will flinch every frame.
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*3;
		}

		if (new_goal_state == AIS_ERR_)
			new_goal_state = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = new_goal_state;

	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == AIS_FIRE) ) {
		uint_fast32_t num_guns = robptr->n_guns;
		for (uint_fast32_t i=0; i<num_guns; i++)
			ailp->goal_state[i] = AIS_FIRE;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if (ready_to_fire_any_weapon(robptr, ailp, 0) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN)  && (get_robot_id(obj) != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
			case AIS_NONE:
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				dot = vm_vec_dot(obj->orient.fvec, vec_to_player);
				if (dot >= F1_0/2)
					if (aip->GOAL_STATE == AIS_REST)
						aip->GOAL_STATE = AIS_SRCH;
				break;
			case AIS_REST:
				if (aip->GOAL_STATE == AIS_REST) {
					compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if (ready_to_fire_any_weapon(robptr, ailp, 0) && (player_visibility)) {
						aip->GOAL_STATE = AIS_FIRE;
					}
				}
				break;
			case AIS_SRCH:
				if (!ai_multiplayer_awareness(obj, 60))
					return;

				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

#if defined(DXX_BUILD_DESCENT_I)
				if (player_visibility) {
					ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				} else if (!(Game_mode & GM_MULTI))
					ai_turn_randomly(vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility == 2) {
					ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}
#endif
				break;
			case AIS_LOCK:
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				if (!(Game_mode & GM_MULTI) || (player_visibility)) {
					if (!ai_multiplayer_awareness(obj, 68))
						return;

#if defined(DXX_BUILD_DESCENT_I)
					if (player_visibility)
					{
						ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
					else if (!(Game_mode & GM_MULTI))
						ai_turn_randomly(vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
#elif defined(DXX_BUILD_DESCENT_II)
					if (player_visibility == 2)
					{   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
						ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
#endif
				}
				break;
			case AIS_FIRE:
				compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

#if defined(DXX_BUILD_DESCENT_I)
				if (player_visibility) {
					if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) 
					{
						if (Game_mode & GM_MULTI) {
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, 0);
							return;
						}
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				} else if (!(Game_mode & GM_MULTI)) {
					ai_turn_randomly(vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
				}
#elif defined(DXX_BUILD_DESCENT_II)
				if (player_visibility == 2) {
					if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) {
						if (Game_mode & GM_MULTI) {
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
					}
					ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(obj, -1);
				}
#endif

				// Fire at player, if appropriate.
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, vec_to_player, dist_to_player, gun_point, player_visibility, object_animates, aip->CURRENT_GUN);

				break;
			case AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec(obj, vis_vec_pos, ailp, vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
#if defined(DXX_BUILD_DESCENT_I)
					if (player_visibility) {
						if (!ai_multiplayer_awareness(obj, 69))
							return;
						ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
						ai_multi_send_robot_position(obj, -1);
					}
					else if (!(Game_mode & GM_MULTI)) {
						ai_turn_randomly(vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
					}
#elif defined(DXX_BUILD_DESCENT_II)
					if (player_visibility == 2) {
						if (!ai_multiplayer_awareness(obj, 69))
							return;
						ai_turn_towards_vector(vec_to_player, obj, robptr->turn_time[Difficulty_level]);
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
	} // end of: if (aip->GOAL_STATE != AIS_FLIN) {

	// Switch to next gun for next fire.
	if (player_visibility == 0) {
		aip->CURRENT_GUN++;
		if (aip->CURRENT_GUN >= robptr->n_guns)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (!((robptr->n_guns == 1) || (robptr->weapon_type2 == weapon_none)))  // Two weapon types hack.
				aip->CURRENT_GUN = 1;
			else
#endif
				aip->CURRENT_GUN = 0;
		}
	}

}

// ----------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		Ai_cloak_info[i].last_position = ConsoleObject->pos;
#if defined(DXX_BUILD_DESCENT_II)
		Ai_cloak_info[i].last_segment = ConsoleObject->segnum;
#endif
		Ai_cloak_info[i].last_time = GameTime64;
	}

	// Make work for control centers.
	Believed_player_pos = Ai_cloak_info[0].last_position;
#if defined(DXX_BUILD_DESCENT_II)
	Believed_player_seg = Ai_cloak_info[0].last_segment;
#endif

}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
static int add_awareness_event(const vobjptr_t objp, enum player_awareness_type_t type)
{
	// If player cloaked and hit a robot, then increase awareness
	if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_WEAPON_WALL_COLLISION) || (type == PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (Num_awareness_events < MAX_AWARENESS_EVENTS) {
		if ((type == PA_WEAPON_WALL_COLLISION) || (type == PA_WEAPON_ROBOT_COLLISION))
			if (get_weapon_id(objp) == VULCAN_ID)
				if (d_rand() > 3276)
					return 0;       // For vulcan cannon, only about 1/10 actually cause awareness

		Awareness_events[Num_awareness_events].segnum = objp->segnum;
		Awareness_events[Num_awareness_events].pos = objp->pos;
		Awareness_events[Num_awareness_events].type = type;
		Num_awareness_events++;
	} else {
		//Int3();   // Hey -- Overflowed Awareness_events, make more or something
		// This just gets ignored, so you can just
		// continue.
	}
	return 1;

}

// ----------------------------------------------------------------------------------
// Robots will become aware of the player based on something that occurred.
// The object (probably player or weapon) which created the awareness is objp.
void create_awareness_event(const vobjptr_t objp, enum player_awareness_type_t type)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
#if defined(DXX_BUILD_DESCENT_II)
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
#endif
	{
		if (add_awareness_event(objp, type)) {
			if (((d_rand() * (type+4)) >> 15) > 4)
				Overall_agitation++;
			if (Overall_agitation > OVERALL_AGITATION_MAX)
				Overall_agitation = OVERALL_AGITATION_MAX;
		}
	}
}

struct awareness_t : public array<ubyte, MAX_SEGMENTS> {};

// ----------------------------------------------------------------------------------
static void pae_aux(segnum_t segnum, int type, int level, awareness_t &New_awareness)
{
	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;

	// Process children.
#if defined(DXX_BUILD_DESCENT_I)
	if (level <= 4)
#elif defined(DXX_BUILD_DESCENT_II)
	if (level <= 3)
#endif
	{
		range_for (auto &j, Segments[segnum].children)
			if (IS_CHILD(j))
				pae_aux(j, type == 4 ? type-1 : type, level+1, New_awareness);
	}
}


// ----------------------------------------------------------------------------------
static void process_awareness_events(awareness_t &New_awareness)
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
	{
		New_awareness.fill(0);
		range_for (auto &i, partial_range(Awareness_events, Num_awareness_events))
			pae_aux(i.segnum, i.type, 1, New_awareness);
	}

	Num_awareness_events = 0;
}

// ----------------------------------------------------------------------------------
static void set_player_awareness_all(void)
{
	awareness_t New_awareness;

	process_awareness_events(New_awareness);

	range_for (auto i, highest_valid(Objects))
		if (Objects[i].type == OBJ_ROBOT && Objects[i].control_type == CT_AI)
		{
			ai_local		*ailp = &Objects[i].ctype.ai_info.ail;
			if (New_awareness[Objects[i].segnum] > ailp->player_awareness_type) {
				ailp->player_awareness_type = New_awareness[Objects[i].segnum];
				ailp->player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
			}

#if defined(DXX_BUILD_DESCENT_II)
			// Clear the bit that says this robot is only awake because a camera woke it up.
			if (New_awareness[Objects[i].segnum] > ailp->player_awareness_type)
				Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
#endif
		}
}

#if PARALLAX
int Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message[128] = "";

// ----------------------------------------------------------------------------------
static void dump_ai_objects_all()
{
#if defined(DXX_BUILD_DESCENT_I)
	int	total=0;
	time_t	time_of_day;

	time_of_day = time(NULL);

	if (!Ai_dump_enable)
		return;

	if (Ai_dump_file == NULL)
		Ai_dump_file = fopen("ai.out","a+t");

	fprintf(Ai_dump_file, "\nnum: seg distance __mode__ behav.    [velx vely velz] (Tick = %i)\n", d_tick_count);
	fprintf(Ai_dump_file, "Date & Time = %s\n", ctime(&time_of_day));

	if (Ai_error_message[0])
		fprintf(Ai_dump_file, "Error message: %s\n", Ai_error_message);

	range_for (auto objnum, highest_valid(Objects))
	{
		object		*objp = &Objects[objnum];
		ai_static	*aip = &objp->ctype.ai_info;
		ai_local		*ailp = &objp->ctype.ai_info.ail;
		fix			dist_to_player;

		dist_to_player = vm_vec_dist(objp->pos, ConsoleObject->pos);

		if (objp->control_type == CT_AI) {
			fprintf(Ai_dump_file, "%3i: %3i %8.3f %8s %8s [%3i %4i]\n",
				objnum, objp->segnum, f2fl(dist_to_player), mode_text[ailp->mode], behavior_text[aip->behavior-0x80], aip->hide_index, aip->path_length);
			if (aip->path_length)
				total += aip->path_length;
		}
	}

	fprintf(Ai_dump_file, "Total path length = %4i\n", total);
#endif
}

void force_dump_ai_objects_all(const char *msg)
{
	int tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf(Ai_error_message, "%s\n", msg);
	dump_ai_objects_all();
	Ai_error_message[0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
#else
static inline void dump_ai_objects_all()
{
}
#endif

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which object is aware of player)
void do_ai_frame_all(void)
{
#ifndef NDEBUG
	dump_ai_objects_all();
#endif

	set_player_awareness_all();

#if defined(DXX_BUILD_DESCENT_II)
	if (Ai_last_missile_camera != object_none) {
		// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
		if (((d_tick_count & 0x0f) == 0) || (Objects[Ai_last_missile_camera].type != OBJ_WEAPON)) {
			Ai_last_missile_camera = object_none;
			range_for (auto i, highest_valid(Objects))
				if (Objects[i].type == OBJ_ROBOT)
					Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
	}

	// (Moved here from do_boss_stuff() because that only gets called if robot aware of player.)
	if (Boss_dying) {
		range_for (auto i, highest_valid(Objects))
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[get_robot_id(&Objects[i])].boss_flag)
					do_boss_dying_frame(&Objects[i]);
	}
#endif
}


// Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	Overall_agitation = 0;
#if defined(DXX_BUILD_DESCENT_II)
	Final_boss_is_dead=0;

	Buddy_objnum = object_none;
	Buddy_allowed_to_talk = 0;

	Boss_dying_start_time = 0;
	
	Ai_last_missile_camera = object_none;
#endif
}

// Following functions convert ai_local/ai_cloak_info to ai_local/ai_cloak_info_rw to be written to/read from Savegames. Convertin back is not done here - reading is done specifically together with swapping (if necessary). These structs differ in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
static void state_ai_local_to_ai_local_rw(ai_local *ail, ai_local_rw *ail_rw)
{
	int i = 0;

	ail_rw->player_awareness_type      = ail->player_awareness_type;
	ail_rw->retry_count                = ail->retry_count;
	ail_rw->consecutive_retries        = ail->consecutive_retries;
	ail_rw->mode                       = ail->mode;
	ail_rw->previous_visibility        = ail->previous_visibility;
	ail_rw->rapidfire_count            = ail->rapidfire_count;
	ail_rw->goal_segment               = ail->goal_segment;
#if defined(DXX_BUILD_DESCENT_I)
	ail_rw->last_see_time              = ail->last_see_time;
	ail_rw->last_attack_time           = ail->last_attack_time;
#elif defined(DXX_BUILD_DESCENT_II)
	ail_rw->next_fire2                 = ail->next_fire2;
#endif
	ail_rw->next_action_time           = ail->next_action_time;
	ail_rw->next_fire                  = ail->next_fire;
	ail_rw->player_awareness_time      = ail->player_awareness_time;
	if (ail->time_player_seen - GameTime64 < F1_0*(-18000))
		ail_rw->time_player_seen = F1_0*(-18000);
	else
		ail_rw->time_player_seen = ail->time_player_seen - GameTime64;
	if (ail->time_player_sound_attacked - GameTime64 < F1_0*(-18000))
		ail_rw->time_player_sound_attacked = F1_0*(-18000);
	else
		ail_rw->time_player_sound_attacked = ail->time_player_sound_attacked - GameTime64;
	ail_rw->time_player_sound_attacked = ail->time_player_sound_attacked;
	ail_rw->next_misc_sound_time       = ail->next_misc_sound_time - GameTime64;
	ail_rw->time_since_processed       = ail->time_since_processed;
	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		ail_rw->goal_angles[i].p   = ail->goal_angles[i].p;
		ail_rw->goal_angles[i].b   = ail->goal_angles[i].b;
		ail_rw->goal_angles[i].h   = ail->goal_angles[i].h;
		ail_rw->delta_angles[i].p  = ail->delta_angles[i].p;
		ail_rw->delta_angles[i].b  = ail->delta_angles[i].b;
		ail_rw->delta_angles[i].h  = ail->delta_angles[i].h;
		ail_rw->goal_state[i]      = ail->goal_state[i];
		ail_rw->achieved_state[i]  = ail->achieved_state[i];
	}
}

static void state_ai_cloak_info_to_ai_cloak_info_rw(ai_cloak_info *aic, ai_cloak_info_rw *aic_rw)
{
	if (aic->last_time - GameTime64 < F1_0*(-18000))
		aic_rw->last_time = F1_0*(-18000);
	else
		aic_rw->last_time = aic->last_time - GameTime64;
#if defined(DXX_BUILD_DESCENT_II)
	aic_rw->last_segment    = aic->last_segment;
#endif
	aic_rw->last_position.x = aic->last_position.x;
	aic_rw->last_position.y = aic->last_position.y;
	aic_rw->last_position.z = aic->last_position.z;
}

DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE();
DEFINE_SERIAL_UDT_TO_MESSAGE(point_seg, p, (p.segnum, serial::pad<2>(), p.point));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(point_seg, 16);

DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(point_seg_array_t, p, (static_cast<array<point_seg, MAX_POINT_SEGS> &>(p)));
DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(point_seg_array_t, p, (static_cast<const array<point_seg, MAX_POINT_SEGS> &>(p)));

int ai_save_state(PHYSFS_file *fp)
{
	fix tmptime32 = 0;

	PHYSFS_write(fp, &Ai_initialized, sizeof(int), 1);
	PHYSFS_write(fp, &Overall_agitation, sizeof(int), 1);
	range_for (auto &i, Objects)
	{
		ai_local_rw ail_rw;
		ai_local		*ailp = &i.ctype.ai_info.ail;
		state_ai_local_to_ai_local_rw(ailp, &ail_rw);
		PHYSFS_write(fp, &ail_rw, sizeof(ail_rw), 1);
	}
	PHYSFSX_serialize_write(fp, Point_segs);
	//PHYSFS_write(fp, Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1);
	range_for (auto &i, Ai_cloak_info)
	{
		ai_cloak_info_rw aic_rw;
		state_ai_cloak_info_to_ai_cloak_info_rw(&i, &aic_rw);
		PHYSFS_write(fp, &aic_rw, sizeof(aic_rw), 1);
	}
	if (Boss_cloak_start_time - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Boss_cloak_start_time - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	if (Boss_cloak_end_time - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Boss_cloak_end_time - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	if (Last_teleport_time - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Last_teleport_time - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &Boss_teleport_interval, sizeof(fix), 1);
	PHYSFS_write(fp, &Boss_cloak_interval, sizeof(fix), 1);
	PHYSFS_write(fp, &Boss_cloak_duration, sizeof(fix), 1);
	if (Last_gate_time - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Last_gate_time - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &Gate_interval, sizeof(fix), 1);
	if (Boss_dying_start_time == 0) // if Boss not dead, yet we expect this to be 0, so do not convert!
	{
		tmptime32 = 0;
	}
	else
	{
		if (Boss_dying_start_time - GameTime64 < F1_0*(-18000))
			tmptime32 = F1_0*(-18000);
		else
			tmptime32 = Boss_dying_start_time - GameTime64;
		if (tmptime32 == 0) // now if our converted value went 0 we should do something against it
			tmptime32 = -1;
	}
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &Boss_dying, sizeof(int), 1);
	int boss_dying_sound_playing = Boss_dying_sound_playing;
	PHYSFS_write(fp, &boss_dying_sound_playing, sizeof(int), 1);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_write(fp, &Boss_hit_this_frame, sizeof(int), 1);
	PHYSFS_write(fp, &Boss_been_hit, sizeof(int), 1);
#elif defined(DXX_BUILD_DESCENT_II)
	if (Boss_hit_time - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Boss_hit_time - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &Escort_kill_object, sizeof(Escort_kill_object), 1);
	if (Escort_last_path_created - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Escort_last_path_created - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &Escort_goal_object, sizeof(Escort_goal_object), 1);
	PHYSFS_write(fp, &Escort_special_goal, sizeof(Escort_special_goal), 1);
	{
		int egi = Escort_goal_index;
		PHYSFS_write(fp, &egi, sizeof(int), 1);
	}
	PHYSFS_write(fp, &Stolen_items, sizeof(Stolen_items[0])*MAX_STOLEN_ITEMS, 1);

	{
		int temp;
		temp = Point_segs_free_ptr - Point_segs;
		PHYSFS_write(fp, &temp, sizeof(int), 1);
	}

	unsigned Num_boss_teleport_segs = Boss_teleport_segs.count();
	PHYSFS_write(fp, &Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1);
	unsigned Num_boss_gate_segs = Boss_gate_segs.count();
	PHYSFS_write(fp, &Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1);

	if (Num_boss_gate_segs)
		PHYSFS_write(fp, &Boss_gate_segs[0], sizeof(Boss_gate_segs[0]), Num_boss_gate_segs);

	if (Num_boss_teleport_segs)
		PHYSFS_write(fp, &Boss_teleport_segs[0], sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs);
#endif

	return 1;
}

static void PHYSFSX_readAngleVecX(PHYSFS_file *file, vms_angvec &v, int swap)
{
	v.p = PHYSFSX_readSXE16(file, swap);
	v.b = PHYSFSX_readSXE16(file, swap);
	v.h = PHYSFSX_readSXE16(file, swap);
}

static void ai_local_read_swap(ai_local *ail, int swap, PHYSFS_file *fp)
{
	{
		fix tmptime32 = 0;

#if defined(DXX_BUILD_DESCENT_I)
		ail->player_awareness_type = PHYSFSX_readByte(fp);
		ail->retry_count = PHYSFSX_readByte(fp);
		ail->consecutive_retries = PHYSFSX_readByte(fp);
		ail->mode = PHYSFSX_readByte(fp);
		ail->previous_visibility = PHYSFSX_readByte(fp);
		ail->rapidfire_count = PHYSFSX_readByte(fp);
		ail->goal_segment = PHYSFSX_readSXE16(fp, swap);
		ail->last_see_time = PHYSFSX_readSXE32(fp, swap);
		ail->last_attack_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_action_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire = PHYSFSX_readSXE32(fp, swap);
#elif defined(DXX_BUILD_DESCENT_II)
		ail->player_awareness_type = PHYSFSX_readSXE32(fp, swap);
		ail->retry_count = PHYSFSX_readSXE32(fp, swap);
		ail->consecutive_retries = PHYSFSX_readSXE32(fp, swap);
		ail->mode = PHYSFSX_readSXE32(fp, swap);
		ail->previous_visibility = PHYSFSX_readSXE32(fp, swap);
		ail->rapidfire_count = PHYSFSX_readSXE32(fp, swap);
		ail->goal_segment = PHYSFSX_readSXE32(fp, swap);
		ail->next_action_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire2 = PHYSFSX_readSXE32(fp, swap);
#endif
		ail->player_awareness_time = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_seen = (fix64)tmptime32;
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_sound_attacked = (fix64)tmptime32;
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->next_misc_sound_time = (fix64)tmptime32;
		ail->time_since_processed = PHYSFSX_readSXE32(fp, swap);
		
		range_for (auto &j, ail->goal_angles)
			PHYSFSX_readAngleVecX(fp, j, swap);
		range_for (auto &j, ail->delta_angles)
			PHYSFSX_readAngleVecX(fp, j, swap);
		range_for (auto &j, ail->goal_state)
			j = PHYSFSX_readByte(fp);
		range_for (auto &j, ail->achieved_state)
			j = PHYSFSX_readByte(fp);
	}
}

static void PHYSFSX_readVectorX(PHYSFS_file *file, vms_vector &v, int swap)
{
	v.x = PHYSFSX_readSXE32(file, swap);
	v.y = PHYSFSX_readSXE32(file, swap);
	v.z = PHYSFSX_readSXE32(file, swap);
}

static void ai_cloak_info_read_n_swap(ai_cloak_info *ci, int n, int swap, PHYSFS_file *fp)
{
	int i;
	fix tmptime32 = 0;
	
	for (i = 0; i < n; i++, ci++)
	{
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ci->last_time = (fix64)tmptime32;
#if defined(DXX_BUILD_DESCENT_II)
		ci->last_segment = PHYSFSX_readSXE32(fp, swap);
#endif
		PHYSFSX_readVectorX(fp, ci->last_position, swap);
	}
}

int ai_restore_state(PHYSFS_file *fp, int version, int swap)
{
	fix tmptime32 = 0;

	Ai_initialized = PHYSFSX_readSXE32(fp, swap);
	Overall_agitation = PHYSFSX_readSXE32(fp, swap);
	range_for (object &obj, Objects)
		ai_local_read_swap(&obj.ctype.ai_info.ail, swap, fp);
	PHYSFSX_serialize_read(fp, Point_segs);
	ai_cloak_info_read_n_swap(Ai_cloak_info, MAX_AI_CLOAK_INFO, swap, fp);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Boss_cloak_start_time = (fix64)tmptime32;
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Boss_cloak_end_time = (fix64)tmptime32;
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Last_teleport_time = (fix64)tmptime32;
	Boss_teleport_interval = PHYSFSX_readSXE32(fp, swap);
	Boss_cloak_interval = PHYSFSX_readSXE32(fp, swap);
	Boss_cloak_duration = PHYSFSX_readSXE32(fp, swap);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Last_gate_time = (fix64)tmptime32;
	Gate_interval = PHYSFSX_readSXE32(fp, swap);
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Boss_dying_start_time = (fix64)tmptime32;
	Boss_dying = PHYSFSX_readSXE32(fp, swap);
	Boss_dying_sound_playing = PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_I)
	(void)version;
	Boss_hit_this_frame = PHYSFSX_readSXE32(fp, swap);
	Boss_been_hit = PHYSFSX_readSXE32(fp, swap);
#elif defined(DXX_BUILD_DESCENT_II)
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	Boss_hit_time = (fix64)tmptime32;
	// -- MK, 10/21/95, unused! -- PHYSFS_read(fp, &Boss_been_hit, sizeof(int), 1);

	if (version >= 8) {
		Escort_kill_object = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		Escort_last_path_created = (fix64)tmptime32;
		Escort_goal_object = PHYSFSX_readSXE32(fp, swap);
		Escort_special_goal = PHYSFSX_readSXE32(fp, swap);
		Escort_goal_index = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_read(fp, &Stolen_items, sizeof(Stolen_items[0]) * MAX_STOLEN_ITEMS, 1);
	} else {
		Escort_kill_object = -1;
		Escort_last_path_created = 0;
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_special_goal = -1;
		Escort_goal_index = -1;

		Stolen_items.fill(255);
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
		unsigned Num_boss_teleport_segs = PHYSFSX_readSXE32(fp, swap);
		unsigned Num_boss_gate_segs = PHYSFSX_readSXE32(fp, swap);

		Boss_gate_segs.clear();
		for (unsigned i = 0; i < Num_boss_gate_segs; i++)
			Boss_gate_segs.emplace_back(PHYSFSX_readSXE16(fp, swap));

		Boss_teleport_segs.clear();
		for (unsigned i = 0; i < Num_boss_teleport_segs; i++)
			Boss_teleport_segs.emplace_back(PHYSFSX_readSXE16(fp, swap));
	}
#endif

	return 1;
}
