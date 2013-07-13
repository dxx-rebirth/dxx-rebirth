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
 * Autonomous Individual movement.
 *
 */


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
#include "laser.h"
#include "fvi.h"
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
#include "cntrlcen.h"
#include "multibot.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "gameseq.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"
#include "fuelcen.h"
#include "controls.h"
#include "kconfig.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "string.h"

#ifndef NDEBUG
#include <time.h>
#endif

// ---------- John: These variables must be saved as part of gamesave. --------
int             Ai_initialized = 0;
int             Overall_agitation;
ai_local        Ai_local_info[MAX_OBJECTS];
point_seg       Point_segs[MAX_POINT_SEGS];
point_seg       *Point_segs_free_ptr = Point_segs;
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
fix64           Boss_hit_time;
int           Boss_dying;
sbyte           Boss_dying_sound_playing, unused123, unused234;

// -- MK, 10/21/95, unused! -- int             Boss_been_hit=0;


// ------ John: End of variables which must be saved as part of gamesave. -----


// -- ubyte Boss_cloaks[NUM_D2_BOSSES]              = {1,1,1,1,1,1};      // Set byte if this boss can cloak

const ubyte Boss_teleports[NUM_D2_BOSSES]           = {1,1,1,1,1,1, 1,1}; // Set byte if this boss can teleport
const ubyte Boss_spew_more[NUM_D2_BOSSES]           = {0,1,0,0,0,0, 0,0}; // If set, 50% of time, spew two bots.
const ubyte Boss_spews_bots_energy[NUM_D2_BOSSES]   = {1,1,0,1,0,1, 1,1}; // Set byte if boss spews bots when hit by energy weapon.
const ubyte Boss_spews_bots_matter[NUM_D2_BOSSES]   = {0,0,1,1,1,1, 0,1}; // Set byte if boss spews bots when hit by matter weapon.
const ubyte Boss_invulnerable_energy[NUM_D2_BOSSES] = {0,0,1,1,0,0, 0,0}; // Set byte if boss is invulnerable to energy weapons.
const ubyte Boss_invulnerable_matter[NUM_D2_BOSSES] = {0,0,0,0,1,1, 1,0}; // Set byte if boss is invulnerable to matter weapons.
const ubyte Boss_invulnerable_spot[NUM_D2_BOSSES]   = {0,0,0,0,0,1, 0,1}; // Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

int ai_evaded=0;

// -- sbyte Super_boss_gate_list[MAX_GATE_INDEX] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};

int Animation_enabled = 1;

#ifndef NDEBUG
int Ai_info_enabled=0;
#endif


// These globals are set by a call to find_vector_intersection, which is a slow routine,
// so we don't want to call it again (for this object) unless we have to.
vms_vector  Hit_pos;
int         Hit_type, Hit_seg;
fvi_info    Hit_data;

int             Num_awareness_events = 0;
awareness_event Awareness_events[MAX_AWARENESS_EVENTS];

vms_vector      Believed_player_pos;
int             Believed_player_seg;

#ifndef NDEBUG
// Index into this array with ailp->mode
static const char mode_text[18][16] = {
	"STILL",
	"WANDER",
	"FOL_PATH",
	"CHASE_OBJ",
	"RUN_FROM",
	"BEHIND",
	"FOL_PATH2",
	"OPEN_DOOR",
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



fix Dist_to_last_fired_upon_player_pos = 0;

// ----------------------------------------------------------------------------
void init_ai_frame(void)
{
	int ab_state;

	Dist_to_last_fired_upon_player_pos = vm_vec_dist_quick(&Last_fired_upon_player_pos, &Believed_player_pos);

	ab_state = Afterburner_charge && Controls.afterburner_state && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER);

	if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON) || ab_state) {
		ai_do_cloak_stuff();
	}
}

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if next_fire <= 0 or next_fire2 <= 0.
int ready_to_fire(robot_info *robptr, ai_local *ailp)
{
	if (robptr->weapon_type2 != -1)
		return (ailp->next_fire <= 0) || (ailp->next_fire2 <= 0);
	else
		return (ailp->next_fire <= 0);
}

// ----------------------------------------------------------------------------
// Make a robot near the player snipe.
#define	MNRS_SEG_MAX	70
void make_nearby_robot_snipe(void)
{
	int bfs_length, i;
	short bfs_list[MNRS_SEG_MAX];

	create_bfs_list(ConsoleObject->segnum, bfs_list, &bfs_length, MNRS_SEG_MAX);

	for (i=0; i<bfs_length; i++) {
		int objnum = Segments[bfs_list[i]].objects;

		while (objnum != -1) {
			object *objp = &Objects[objnum];
			robot_info *robptr = &Robot_info[objp->id];

			if ((objp->type == OBJ_ROBOT) && (objp->id != ROBOT_BRAIN)) {
				if ((objp->ctype.ai_info.behavior != AIB_SNIPE) && (objp->ctype.ai_info.behavior != AIB_RUN_FROM) && !Robot_info[objp->id].boss_flag && !robptr->companion) {
					objp->ctype.ai_info.behavior = AIB_SNIPE;
					Ai_local_info[objnum].mode = AIM_SNIPE_ATTACK;
					return;
				}
			}
			objnum = objp->next;
		}
	}
}

int Ai_last_missile_camera = -1;

// --------------------------------------------------------------------------------------------------------------------
void do_ai_frame(object *obj)
{
	int         objnum = obj-Objects;
	ai_static   *aip = &obj->ctype.ai_info;
	ai_local    *ailp = &Ai_local_info[objnum];
	fix         dist_to_player;
	vms_vector  vec_to_player;
	fix         dot;
	robot_info  *robptr;
	int         player_visibility=-1;
	int         obj_ref;
	int         object_animates;
	int         new_goal_state;
	int         visibility_and_vec_computed = 0;
	int         previous_visibility;
	vms_vector  gun_point;
	vms_vector  vis_vec_pos;

	ailp->next_action_time -= FrameTime;

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {
			obj->mtype.phys_info.rotthrust.x = (obj->mtype.phys_info.rotthrust.x * 15)/16;
			obj->mtype.phys_info.rotthrust.y = (obj->mtype.phys_info.rotthrust.y * 15)/16;
			obj->mtype.phys_info.rotthrust.z = (obj->mtype.phys_info.rotthrust.z * 15)/16;
			if (!aip->SKIP_AI_COUNT)
				obj->mtype.phys_info.flags &= ~PF_USES_THRUST;
		}
		return;
	}

	robptr = &Robot_info[obj->id];
	Assert(robptr->always_0xabcd == 0xabcd);

	if (do_any_robot_dying_frame(obj))
		return;

	// Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	// Else, you can turn a big nasty robot into a wimp by firing flares at it.
	// This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && ready_to_fire(robptr, ailp)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3(); // This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	if (!Do_ai_flag)
		return;

	if (Break_on_object != -1)
		if ((obj-Objects) == Break_on_object)
			Int3(); // Contact Mike: This is a debug break
#endif

	//Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR));
	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		aip->behavior = AIB_NORMAL;
	}

	Assert(obj->segnum != -1);
	Assert(obj->id < N_robot_types);

	obj_ref = objnum ^ d_tick_count;

	if (ailp->next_fire > -F1_0*8)
		ailp->next_fire -= FrameTime;

	if (robptr->weapon_type2 != -1) {
		if (ailp->next_fire2 > -F1_0*8)
			ailp->next_fire2 -= FrameTime;
	} else
		ailp->next_fire2 = F1_0*8;

	if (ailp->time_since_processed < F1_0*256)
		ailp->time_since_processed += FrameTime;

	previous_visibility = ailp->previous_visibility;    //  Must get this before we toast the master copy!

	// -- (No robots have this behavior...)
	// -- // Deal with cloaking for robots which are cloaked except just before firing.
	// -- if (robptr->cloak_type == RI_CLOAKED_EXCEPT_FIRING)
	// -- 	if (ailp->next_fire < F1_0/2)
	// -- 		aip->CLOAKED = 1;
	// -- 	else
	// -- 		aip->CLOAKED = 0;

	// If only awake because of a camera, make that the believed player position.
	if ((aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE) && (Ai_last_missile_camera != -1))
		Believed_player_pos = Objects[Ai_last_missile_camera].pos;
	else {
		if (cheats.robotskillrobots) {
			vis_vec_pos = obj->pos;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility) {
				int ii, min_obj = -1;
				fix min_dist = F1_0*200, cur_dist;

				for (ii=0; ii<=Highest_object_index; ii++)
					if ((Objects[ii].type == OBJ_ROBOT) && (ii != objnum)) {
						cur_dist = vm_vec_dist_quick(&obj->pos, &Objects[ii].pos);

						if (cur_dist < F1_0*100)
							if (object_to_object_visibility(obj, &Objects[ii], FQ_TRANSWALL))
								if (cur_dist < min_dist) {
									min_obj = ii;
									min_dist = cur_dist;
								}
					}
				if (min_obj != -1) {
					Believed_player_pos = Objects[min_obj].pos;
					Believed_player_seg = Objects[min_obj].segnum;
					vm_vec_normalized_dir_quick(&vec_to_player, &Believed_player_pos, &obj->pos);
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
	dist_to_player = vm_vec_dist_quick(&Believed_player_pos, &obj->pos);

	// If this robot can fire, compute visibility from gun position.
	// Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	if ((previous_visibility || !(obj_ref & 3)) && ready_to_fire(robptr, ailp) && (dist_to_player < F1_0*200) && (robptr->n_guns) && !(robptr->attack_type)) {
		// Since we passed ready_to_fire(), either next_fire or next_fire2 <= 0.  calc_gun_point from relevant one.
		// If both are <= 0, we will deal with the mess in ai_do_actual_firing_stuff
		if (ailp->next_fire <= 0)
			calc_gun_point(&gun_point, obj, aip->CURRENT_GUN);
		else
			calc_gun_point(&gun_point, obj, 0);
		vis_vec_pos = gun_point;
	} else {
		vis_vec_pos = obj->pos;
		vm_vec_zero(&gun_point);
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - 
	// Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(Game_mode & GM_MULTI) && (robptr->companion != 1) && (robptr->thief != 1))
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
				case AIM_GOTO_PLAYER:
					// -- Buddy_got_stuck = 1;
					move_towards_segment_center(obj);
					create_path_to_player(obj, 100, 1);
					// -- Buddy_got_stuck = 0;
					break;
				case AIM_GOTO_OBJECT:
					Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
					//if (obj->segnum == ConsoleObject->segnum) {
					//	if (Point_segs[aip->hide_index + aip->cur_path_index].segnum == obj->segnum)
					//		if ((aip->cur_path_index + aip->PATH_DIR >= 0) && (aip->cur_path_index + aip->PATH_DIR < aip->path_length-1))
					//			aip->cur_path_index += aip->PATH_DIR;
					//}
					break;
				case AIM_CHASE_OBJECT:
					create_path_to_player(obj, 4 + Overall_agitation/8 + Difficulty_level, 1);
					break;
				case AIM_STILL:
					if (robptr->attack_type)
						move_towards_segment_center(obj);
					else if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION) || (aip->behavior == AIB_FOLLOW)))    // Behavior is still, so don't follow path.
						attempt_to_resume_path(obj);
					break;
				case AIM_FOLLOW_PATH:
					if (Game_mode & GM_MULTI) {
						ailp->mode = AIM_STILL;
					} else
						attempt_to_resume_path(obj);
					break;
				case AIM_RUN_FROM_OBJECT:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					create_n_segment_path(obj, 5, -1);
					ailp->mode = AIM_RUN_FROM_OBJECT;
					break;
				case AIM_BEHIND:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					break;
				case AIM_OPEN_DOOR:
					create_n_segment_path_to_door(obj, 5, -1);
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
	if (!(Game_mode & GM_MULTI) && (Segment2s[obj->segnum].special == SEGMENT_IS_ROBOTMAKER)) {
		if (Station[Segment2s[obj->segnum].value].Enabled) {
			ai_follow_path(obj, 1, 1, NULL);    // 1 = player is visible, which might be a lie, but it works.
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
				#ifndef SHAREWARE
				ai_multi_send_robot_position(objnum, -1);
				#endif

				if (!((ailp->mode == AIM_FOLLOW_PATH) && (aip->cur_path_index < aip->path_length-1)))
					if ((aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM)) {
						if (dist_to_player < F1_0*30)
							create_n_segment_path(obj, 5, 1);
						else
							create_path_to_player(obj, 20, 1);
					}
			}
		}

	// -- // Make sure that if this guy got hit or bumped, then he's chasing player.
	// -- if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
	// -- 	if ((ailp->mode != AIM_BEHIND) && (aip->behavior != AIB_STILL) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_RUN_FROM) && (!robptr->companion) && (!robptr->thief) && (obj->id != ROBOT_BRAIN)) {
	// -- 		ailp->mode = AIM_CHASE_OBJECT;
	// -- 		ailp->player_awareness_type = 0;
	// -- 		ailp->player_awareness_time = 0;
	// -- 	}
	// -- }

	// Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		if (player_visibility == 1) // Only increase visibility if unobstructed, else claw guys attack through doors.
			player_visibility = 2;
	} else if (((obj_ref&3) == 0) && !previous_visibility && (dist_to_player < F1_0*100)) {
		fix sval, rval;

		rval = d_rand();
		sval = (dist_to_player * (Difficulty_level+1))/64;

		if ((fixmul(rval, sval) < FrameTime) || (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
			ailp->player_awareness_time = F1_0*3;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
			if (player_visibility == 1) {
				player_visibility = 2;
			}
		}
	}


	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
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

	switch (Robot_info[obj->id].boss_flag) {
	case 0:
		break;

	case 1:
	case 2:
		// FIXME!!!!
		break;

	default:
		{
			int	pv;

			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			pv = player_visibility;

			// If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
			}

			do_boss_stuff(obj, pv);
		}
		break;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Time-slice, don't process all the time, purely an efficiency hack.
	// Guys whose behavior is station and are not at their hide segment get processed anyway.
	if (!((aip->behavior == AIB_SNIPE) && (ailp->mode != AIM_SNIPE_WAIT)) && !robptr->companion && !robptr->thief && (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION-1)) { // If robot got hit, he gets to attack player always!
#ifndef NDEBUG
		if (Break_on_object != objnum) {    // don't time slice if we're interested in this object.
#endif
			if ((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != obj->segnum)) {
				if (dist_to_player > F1_0*250)  // station guys not at home always processed until 250 units away.
					return;
			} else if ((!ailp->previous_visibility) && ((dist_to_player >> 7) > ailp->time_since_processed)) {  // 128 units away (6.4 segments) processed after 1 second.
				return;
			}
#ifndef NDEBUG
		}
#endif
	}

	// Reset time since processed, but skew objects so not everything
	// processed synchronously, else we get fast frames with the
	// occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp->time_since_processed = - ((objnum & 0x03) * FrameTime ) / 2;

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	//	Perform special ability
	switch (obj->id) {
		case ROBOT_BRAIN:
			// Robots function nicely if behavior is Station.  This
			// means they won't move until they can see the player, at
			// which time they will start wandering about opening doors.
			if (ConsoleObject->segnum == obj->segnum) {
				if (!ai_multiplayer_awareness(obj, 97))
					return;
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				move_away_from_player(obj, &vec_to_player, 0);
				ai_multi_send_robot_position(objnum, -1);
			} else if (ailp->mode != AIM_STILL) {
				int r;

				r = openable_doors_in_segment(obj->segnum);
				if (r != -1) {
					ailp->mode = AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}

				if (ailp->next_action_time < 0) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if (player_visibility) {
						make_nearby_robot_snipe();
						ailp->next_action_time = (NDL - Difficulty_level) * 2*F1_0;
					}
				}
			} else {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, -1);     // third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		default:
			break;
	}

	if (aip->behavior == AIB_SNIPE) {
		if ((Game_mode & GM_MULTI) && !robptr->thief) {
			aip->behavior = AIB_NORMAL;
			ailp->mode = AIM_CHASE_OBJECT;
			return;
		}

		if (!(obj_ref & 3) || previous_visibility) {
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// If this sniper is in still mode, if he was hit or can see player, switch to snipe mode.
			if (ailp->mode == AIM_STILL)
				if (player_visibility || (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION))
					ailp->mode = AIM_SNIPE_ATTACK;

			if (!robptr->thief && (ailp->mode != AIM_STILL))
				do_snipe_frame(obj, dist_to_player, player_visibility, &vec_to_player);
		} else if (!robptr->thief && !robptr->companion)
			return;
	}

	// More special ability stuff, but based on a property of a robot, not its ID.
	if (robptr->companion) {

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_escort_frame(obj, dist_to_player, player_visibility);

		if (obj->ctype.ai_info.danger_laser_num != -1) {
			object *dobjp = &Objects[obj->ctype.ai_info.danger_laser_num];

			if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == obj->ctype.ai_info.danger_laser_signature)) {
				fix circle_distance;
				circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
				ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, circle_distance, 1, player_visibility);
			}
		}

		if (ready_to_fire(robptr, ailp)) {
			int do_stuff = 0;
			if (openable_doors_in_segment(obj->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + 2*aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if ((ailp->mode == AIM_GOTO_PLAYER) && (dist_to_player < 3*MIN_ESCORT_DISTANCE/2) && (vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player) > -F1_0/4)) {
				do_stuff = 1;
			}

			if (do_stuff) {
				Laser_create_new_easy( &obj->orient.fvec, &obj->pos, obj-Objects, FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (!Buddy_allowed_to_talk) // If buddy not talking, make him fire flares less often.
					ailp->next_fire += d_rand()*4;
			}

		}
	}

	if (robptr->thief) {

		compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
		do_thief_frame(obj, dist_to_player, player_visibility, &vec_to_player);

		if (ready_to_fire(robptr, ailp)) {
			int do_stuff = 0;
			if (openable_doors_in_segment(obj->segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;
			else if (openable_doors_in_segment(Point_segs[aip->hide_index + aip->cur_path_index + 2*aip->PATH_DIR].segnum) != -1)
				do_stuff = 1;

			if (do_stuff) {
				// @mk, 05/08/95: Firing flare from center of object, this is dumb...
				Laser_create_new_easy( &obj->orient.fvec, &obj->pos, obj-Objects, FLARE_ID, 1);
				ailp->next_fire = F1_0/2;
				if (Stolen_item_index == 0)     // If never stolen an item, fire flares less often (bad: Stolen_item_index wraps, but big deal)
					ailp->next_fire += d_rand()*4;
			}
		}
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	switch (ailp->mode) {
		case AIM_CHASE_OBJECT: {        // chasing player, sort of, chase if far, back off if close, circle in between
			fix circle_distance;

			circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
			// Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robptr->attack_type != 1)
				circle_distance += (objnum&0xf) * F1_0/2;

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			// @mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (previous_visibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(obj, 53)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				create_path_to_player(obj, 8, 1);
				ai_multi_send_robot_position(objnum, -1);
			} else if ((player_visibility == 0) && (dist_to_player > F1_0*80) && (!(Game_mode & GM_MULTI))) {
				// If pretty far from the player, player cannot be seen
				// (obstructed) and in chase mode, switch to follow path mode.
				// This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->goal_segment = aip->hide_segment;
					create_path_to_station(obj, 15);
				} // -- this looks like a dumb thing to do...robots following paths far away from you! else create_n_segment_path(obj, 5, -1);
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
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!ai_multiplayer_awareness(obj, 70)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
					return;
				}
				ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, circle_distance, 0, player_visibility);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				}

				if (ai_evaded) {
					ai_multi_send_robot_position(objnum, 1);
					ai_evaded = 0;
				} else
					ai_multi_send_robot_position(objnum, -1);

				do_firing_stuff(obj, player_visibility, &vec_to_player);
			}
			break;
		}

		case AIM_RUN_FROM_OBJECT:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility) {
				if (ailp->player_awareness_type == 0)
					ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;

			}

			// If in multiplayer, only do if player visible.  If not multiplayer, do always.
			if (!(Game_mode & GM_MULTI) || player_visibility)
				if (ai_multiplayer_awareness(obj, 75)) {
					ai_follow_path(obj, player_visibility, previous_visibility, &vec_to_player);
					ai_multi_send_robot_position(objnum, -1);
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
			if ((ailp->next_fire <= 0) && (player_visibility)) {
				vms_vector fire_vec, fire_pos;

				if (!ai_multiplayer_awareness(obj, 75))
					return;

				fire_vec = obj->orient.fvec;
				vm_vec_negate(&fire_vec);
				vm_vec_add(&fire_pos, &obj->pos, &fire_vec);

				if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
					Laser_create_new_easy( &fire_vec, &fire_pos, obj-Objects, ROBOT_SUPERPROX_ID, 1);
				else
					Laser_create_new_easy( &fire_vec, &fire_pos, obj-Objects, PROXIMITY_ID, 1);

				ailp->next_fire = (F1_0/2)*(NDL+5 - Difficulty_level);      // Drop a proximity bomb every 5 seconds.

#ifdef NETWORK
#ifndef SHAREWARE
				if (Game_mode & GM_MULTI)
				{
					ai_multi_send_robot_position(obj-Objects, -1);
					if (aip->SUB_FLAGS & SUB_FLAGS_SPROX)
						multi_send_robot_fire(obj-Objects, -2, &fire_vec);
					else
						multi_send_robot_fire(obj-Objects, -1, &fire_vec);
				}
#endif
#endif
			}
			break;

		case AIM_GOTO_PLAYER:
		case AIM_GOTO_OBJECT:
			ai_follow_path(obj, 2, previous_visibility, &vec_to_player);    // Follows path as if player can see robot.
			ai_multi_send_robot_position(objnum, -1);
			break;

		case AIM_FOLLOW_PATH: {
			int anger_level = 65;

			if (aip->behavior == AIB_STATION)
				if (Point_segs[aip->hide_index + aip->path_length - 1].segnum == aip->hide_segment) {
					anger_level = 64;
				}

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!ai_multiplayer_awareness(obj, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			ai_follow_path(obj, player_visibility, previous_visibility, &vec_to_player);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			if (aip->behavior != AIB_RUN_FROM)
				do_firing_stuff(obj, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_SNIPE) && (aip->behavior != AIB_FOLLOW) && (aip->behavior != AIB_RUN_FROM) && (obj->id != ROBOT_BRAIN) && (robptr->companion != 1) && (robptr->thief != 1)) {
				if (robptr->attack_type == 0)
					ailp->mode = AIM_CHASE_OBJECT;
				// This should not just be distance based, but also time-since-player-seen based.
			} else if ((dist_to_player > F1_0*(20*(2*Difficulty_level + robptr->pursuit)))
						&& (GameTime64 - ailp->time_player_seen > (F1_0/2*(Difficulty_level+robptr->pursuit)))
						&& (player_visibility == 0)
						&& (aip->behavior == AIB_NORMAL)
						&& (ailp->mode == AIM_FOLLOW_PATH)) {
				ailp->mode = AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}

			ai_multi_send_robot_position(objnum, -1);

			break;
		}

		case AIM_BEHIND:
			if (!ai_multiplayer_awareness(obj, 71)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				}
				return;
			}

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				// Get behind the player.
				// Method:
				// If vec_to_player dot player_rear_vector > 0, behind is goal.
				// Else choose goal with larger dot from left, right.
				vms_vector  goal_point, goal_vector, vec_to_goal, rand_vec;
				fix         dot;

				dot = vm_vec_dot(&ConsoleObject->orient.fvec, &vec_to_player);
				if (dot > 0) {          // Remember, we're interested in the rear vector dot being < 0.
					goal_vector = ConsoleObject->orient.fvec;
					vm_vec_negate(&goal_vector);
				} else {
					fix dot;
					dot = vm_vec_dot(&ConsoleObject->orient.rvec, &vec_to_player);
					goal_vector = ConsoleObject->orient.rvec;
					if (dot > 0) {
						vm_vec_negate(&goal_vector);
					}
				}

				vm_vec_scale(&goal_vector, 2*(ConsoleObject->size + obj->size + (((objnum*4 + d_tick_count) & 63) << 12)));
				vm_vec_add(&goal_point, &ConsoleObject->pos, &goal_vector);
				make_random_vector(&rand_vec);
				vm_vec_scale_add2(&goal_point, &rand_vec, F1_0*8);
				vm_vec_sub(&vec_to_goal, &goal_point, &obj->pos);
				vm_vec_normalize_quick(&vec_to_goal);
				move_towards_vector(obj, &vec_to_goal, 0);
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
			}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_send_robot_position(objnum, -1);
			break;

		case AIM_STILL:
			if ((dist_to_player < F1_0*120+Difficulty_level*F1_0*20) || (ailp->player_awareness_type >= PA_WEAPON_ROBOT_COLLISION-1)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				// turn towards vector if visible this time or last time, or rand
				// new!
				if ((player_visibility == 2) || (previous_visibility == 2)) { // -- MK, 06/09/95:  || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI))) {
					if (!ai_multiplayer_awareness(obj, 71)) {
						if (maybe_ai_do_actual_firing_stuff(obj, aip))
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				}

				do_firing_stuff(obj, player_visibility, &vec_to_player);
				if (player_visibility == 2) {  // Changed @mk, 09/21/95: Require that they be looking to evade.  Change, MK, 01/03/95 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (robptr->attack_type == 1) {
						aip->behavior = AIB_NORMAL;
						if (!ai_multiplayer_awareness(obj, 80)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
						if (ai_evaded) {
							ai_multi_send_robot_position(objnum, 1);
							ai_evaded = 0;
						}
						else
							ai_multi_send_robot_position(objnum, -1);
					} else {
						// Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(obj, 81)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 1, player_visibility);
						if (ai_evaded) {
							ai_multi_send_robot_position(objnum, -1);
							ai_evaded = 0;
						}
						else
							ai_multi_send_robot_position(objnum, -1);
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
			vms_vector center_point, goal_vector;
			Assert(obj->id == ROBOT_BRAIN);     // Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(obj, 62))
				return;
			compute_center_point_on_side(&center_point, &Segments[obj->segnum], aip->GOALSIDE);
			vm_vec_sub(&goal_vector, &center_point, &obj->pos);
			vm_vec_normalize_quick(&goal_vector);
			ai_turn_towards_vector(&goal_vector, obj, robptr->turn_time[Difficulty_level]);
			move_towards_vector(obj, &goal_vector, 0);
			ai_multi_send_robot_position(objnum, -1);

			break;
		}

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
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
				if (robptr->thief)
					ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 0, player_visibility);
				break;
			}
			break;

		case AIM_THIEF_WAIT:
		case AIM_THIEF_ATTACK:
		case AIM_THIEF_RETREAT:
		case AIM_WANDER:    // Used for Buddy Bot
			break;

		default:
			ailp->mode = AIM_CHASE_OBJECT;
			break;
	}       // end: switch (ailp->mode) {

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// If the robot can see you, increase his awareness of you.
	// This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1); // Means it didn't get initialized!
	compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
	if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW) && (!robptr->thief)) {
		if ((ailp->player_awareness_type == 0) && (aip->SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE))
			aip->SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		else if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;
	}

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
		int i,num_guns;
		num_guns = Robot_info[obj->id].n_guns;
		for (i=0; i<num_guns; i++)
			ailp->goal_state[i] = AIS_FIRE;
	}

	// - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  -
	// Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if (ready_to_fire(robptr, ailp) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN)  && (obj->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
		case AIS_NONE:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			dot = vm_vec_dot(&obj->orient.fvec, &vec_to_player);
			if (dot >= F1_0/2)
				if (aip->GOAL_STATE == AIS_REST)
					aip->GOAL_STATE = AIS_SRCH;
			break;
		case AIS_REST:
			if (aip->GOAL_STATE == AIS_REST) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (ready_to_fire(robptr, ailp) && (player_visibility)) {
					aip->GOAL_STATE = AIS_FIRE;
				}
			}
			break;
		case AIS_SRCH:
			if (!ai_multiplayer_awareness(obj, 60))
				return;

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_multi_send_robot_position(objnum, -1);
			}
			break;
		case AIS_LOCK:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!(Game_mode & GM_MULTI) || (player_visibility)) {
				if (!ai_multiplayer_awareness(obj, 68))
					return;

				if (player_visibility == 2) {   // @mk, 09/21/95, require that they be looking towards you to turn towards you.
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		case AIS_FIRE:
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (player_visibility == 2) {
				if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) {
					if (Game_mode & GM_MULTI) {
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);
						return;
					}
				}
				ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
				ai_multi_send_robot_position(objnum, -1);
			}

			// Fire at player, if appropriate.
			ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates, aip->CURRENT_GUN);

			break;
		case AIS_RECO:
			if (!(obj_ref & 3)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility == 2) {
					if (!ai_multiplayer_awareness(obj, 69))
						return;
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				} // -- MK, 06/09/95: else if (!(Game_mode & GM_MULTI)) {
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
		if (aip->CURRENT_GUN >= Robot_info[obj->id].n_guns)
		{
			if ((robptr->n_guns == 1) || (robptr->weapon_type2 == -1))  // Two weapon types hack.
				aip->CURRENT_GUN = 0;
			else
				aip->CURRENT_GUN = 1;
		}
	}

}

// ----------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		Ai_cloak_info[i].last_position = ConsoleObject->pos;
		Ai_cloak_info[i].last_segment = ConsoleObject->segnum;
		Ai_cloak_info[i].last_time = GameTime64;
	}

	// Make work for control centers.
	Believed_player_pos = Ai_cloak_info[0].last_position;
	Believed_player_seg = Ai_cloak_info[0].last_segment;

}

// ----------------------------------------------------------------------------
// Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(object *objp, int type)
{
	// If player cloaked and hit a robot, then increase awareness
	if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_WEAPON_WALL_COLLISION) || (type == PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (Num_awareness_events < MAX_AWARENESS_EVENTS) {
		if ((type == PA_WEAPON_WALL_COLLISION) || (type == PA_WEAPON_ROBOT_COLLISION))
			if (objp->id == VULCAN_ID)
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
void create_awareness_event(object *objp, int type)
{
	// If not in multiplayer, or in multiplayer with robots, do this, else unnecessary!
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS)) {
		if (add_awareness_event(objp, type)) {
			if (((d_rand() * (type+4)) >> 15) > 4)
				Overall_agitation++;
			if (Overall_agitation > OVERALL_AGITATION_MAX)
				Overall_agitation = OVERALL_AGITATION_MAX;
		}
	}
}

sbyte New_awareness[MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux(int segnum, int type, int level)
{
	int j;

	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;

	// Process children.
	for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
		if (IS_CHILD(Segments[segnum].children[j]))
		{
			if (level <= 3)
			{
				if (type == 4)
					pae_aux(Segments[segnum].children[j], type-1, level+1);
				else
					pae_aux(Segments[segnum].children[j], type, level+1);
			}
		}
}


// ----------------------------------------------------------------------------------
void process_awareness_events(void)
{
	int i;

	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS)) {
		memset(New_awareness, 0, sizeof(New_awareness[0]) * (Highest_segment_index+1));

		for (i=0; i<Num_awareness_events; i++)
			pae_aux(Awareness_events[i].segnum, Awareness_events[i].type, 1);

	}

	Num_awareness_events = 0;
}

// ----------------------------------------------------------------------------------
void set_player_awareness_all(void)
{
	int i;

	process_awareness_events();

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].control_type == CT_AI) {
			if (New_awareness[Objects[i].segnum] > Ai_local_info[i].player_awareness_type) {
				Ai_local_info[i].player_awareness_type = New_awareness[Objects[i].segnum];
				Ai_local_info[i].player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
			}

			// Clear the bit that says this robot is only awake because a camera woke it up.
			if (New_awareness[Objects[i].segnum] > Ai_local_info[i].player_awareness_type)
				Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
}

#ifndef NDEBUG
int Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char Ai_error_message[128] = "";

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(char *msg)
{
	int tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf(Ai_error_message, "%s\n", msg);
	//dump_ai_objects_all();
	Ai_error_message[0] = 0;

	Ai_dump_enable = tsave;
}

// ----------------------------------------------------------------------------------
void turn_off_ai_dump(void)
{
	if (Ai_dump_file != NULL)
		fclose(Ai_dump_file);

	Ai_dump_file = NULL;
}

#endif

extern void do_boss_dying_frame(object *objp);

// ----------------------------------------------------------------------------------
// Do things which need to get done for all AI objects each frame.
// This includes:
//  Setting player_awareness (a fix, time in seconds which object is aware of player)
void do_ai_frame_all(void)
{
#ifndef NDEBUG
	//dump_ai_objects_all();
#endif

	set_player_awareness_all();

	if (Ai_last_missile_camera > -1) {
		// Clear if supposed misisle camera is not a weapon, or just every so often, just in case.
		if (((d_tick_count & 0x0f) == 0) || (Objects[Ai_last_missile_camera].type != OBJ_WEAPON)) {
			int i;

			Ai_last_missile_camera = -1;
			for (i=0; i<=Highest_object_index; i++)
				if (Objects[i].type == OBJ_ROBOT)
					Objects[i].ctype.ai_info.SUB_FLAGS &= ~SUB_FLAGS_CAMERA_AWAKE;
		}
	}

	// (Moved here from do_boss_stuff() because that only gets called if robot aware of player.)
	if (Boss_dying) {
		int i;

		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[Objects[i].id].boss_flag)
					do_boss_dying_frame(&Objects[i]);
	}
}


extern int Final_boss_is_dead;
extern fix Boss_invulnerable_dot;

// Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	Overall_agitation = 0;
	Final_boss_is_dead=0;

	Buddy_objnum = 0;
	Buddy_allowed_to_talk = 0;

	Boss_invulnerable_dot = F1_0/4 - i2f(Difficulty_level)/8;
	Boss_dying_start_time = 0;
	
	Ai_last_missile_camera = -1;
}

// Following functions convert ai_local/ai_cloak_info to ai_local/ai_cloak_info_rw to be written to/read from Savegames. Convertin back is not done here - reading is done specifically together with swapping (if necessary). These structs differ in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
void state_ai_local_to_ai_local_rw(ai_local *ail, ai_local_rw *ail_rw)
{
	int i = 0;

	ail_rw->player_awareness_type      = ail->player_awareness_type;
	ail_rw->retry_count                = ail->retry_count;
	ail_rw->consecutive_retries        = ail->consecutive_retries;
	ail_rw->mode                       = ail->mode;
	ail_rw->previous_visibility        = ail->previous_visibility;
	ail_rw->rapidfire_count            = ail->rapidfire_count;
	ail_rw->goal_segment               = ail->goal_segment;
	ail_rw->next_action_time           = ail->next_action_time;
	ail_rw->next_fire                  = ail->next_fire;
	ail_rw->next_fire2                 = ail->next_fire2;
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

void state_ai_cloak_info_to_ai_cloak_info_rw(ai_cloak_info *aic, ai_cloak_info_rw *aic_rw)
{
	if (aic->last_time - GameTime64 < F1_0*(-18000))
		aic_rw->last_time = F1_0*(-18000);
	else
		aic_rw->last_time = aic->last_time - GameTime64;
	aic_rw->last_segment    = aic->last_segment;
	aic_rw->last_position.x = aic->last_position.x;
	aic_rw->last_position.x = aic->last_position.y;
	aic_rw->last_position.z = aic->last_position.z;
}

int ai_save_state(PHYSFS_file *fp)
{
	int i = 0;
	fix tmptime32 = 0;

	PHYSFS_write(fp, &Ai_initialized, sizeof(int), 1);
	PHYSFS_write(fp, &Overall_agitation, sizeof(int), 1);
	//PHYSFS_write(fp, Ai_local_info, sizeof(ai_local) * MAX_OBJECTS, 1);
	for (i = 0; i < MAX_OBJECTS; i++)
	{
		ai_local_rw *ail_rw;
		CALLOC(ail_rw, ai_local_rw, 1);
		state_ai_local_to_ai_local_rw(&Ai_local_info[i], ail_rw);
		PHYSFS_write(fp, ail_rw, sizeof(ai_local_rw), 1);
		d_free(ail_rw);
	}
	PHYSFS_write(fp, Point_segs, sizeof(point_seg) * MAX_POINT_SEGS, 1);
	//PHYSFS_write(fp, Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1);
	for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	{
		ai_cloak_info_rw *aic_rw;
		CALLOC(aic_rw, ai_cloak_info_rw, 1);
		state_ai_cloak_info_to_ai_cloak_info_rw(&Ai_cloak_info[i], aic_rw);
		PHYSFS_write(fp, aic_rw, sizeof(ai_cloak_info_rw), 1);
		d_free(aic_rw);
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
	PHYSFS_write(fp, &Boss_dying_sound_playing, sizeof(int), 1);
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
	PHYSFS_write(fp, &Escort_goal_index, sizeof(Escort_goal_index), 1);
	PHYSFS_write(fp, &Stolen_items, sizeof(Stolen_items[0])*MAX_STOLEN_ITEMS, 1);

	{
		int temp;
		temp = Point_segs_free_ptr - Point_segs;
		PHYSFS_write(fp, &temp, sizeof(int), 1);
	}

	PHYSFS_write(fp, &Num_boss_teleport_segs, sizeof(Num_boss_teleport_segs), 1);
	PHYSFS_write(fp, &Num_boss_gate_segs, sizeof(Num_boss_gate_segs), 1);

	if (Num_boss_gate_segs)
		PHYSFS_write(fp, Boss_gate_segs, sizeof(Boss_gate_segs[0]), Num_boss_gate_segs);

	if (Num_boss_teleport_segs)
		PHYSFS_write(fp, Boss_teleport_segs, sizeof(Boss_teleport_segs[0]), Num_boss_teleport_segs);

	return 1;
}

void ai_local_read_n_swap(ai_local *ail, int n, int swap, PHYSFS_file *fp)
{
	int i;
	
	for (i = 0; i < n; i++, ail++)
	{
		int j;
		fix tmptime32 = 0;

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
		ail->player_awareness_time = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_seen = (fix64)tmptime32;
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->time_player_sound_attacked = (fix64)tmptime32;
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ail->next_misc_sound_time = (fix64)tmptime32;
		ail->time_since_processed = PHYSFSX_readSXE32(fp, swap);
		
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readAngleVecX(fp, &ail->goal_angles[j], swap);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readAngleVecX(fp, &ail->delta_angles[j], swap);
		for (j = 0; j < MAX_SUBMODELS; j++)
			ail->goal_state[j] = PHYSFSX_readByte(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			ail->achieved_state[j] = PHYSFSX_readByte(fp);
	}
}

void point_seg_read_n_swap(point_seg *ps, int n, int swap, PHYSFS_file *fp)
{
	int i;
	
	for (i = 0; i < n; i++, ps++)
	{
		ps->segnum = PHYSFSX_readSXE32(fp, swap);
		PHYSFSX_readVectorX(fp, &ps->point, swap);
	}
}

void ai_cloak_info_read_n_swap(ai_cloak_info *ci, int n, int swap, PHYSFS_file *fp)
{
	int i;
	fix tmptime32 = 0;
	
	for (i = 0; i < n; i++, ci++)
	{
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		ci->last_time = (fix64)tmptime32;
		ci->last_segment = PHYSFSX_readSXE32(fp, swap);
		PHYSFSX_readVectorX(fp, &ci->last_position, swap);
	}
}

int ai_restore_state(PHYSFS_file *fp, int version, int swap)
{
	fix tmptime32 = 0;

	Ai_initialized = PHYSFSX_readSXE32(fp, swap);
	Overall_agitation = PHYSFSX_readSXE32(fp, swap);
	ai_local_read_n_swap(Ai_local_info, MAX_OBJECTS, swap, fp);
	point_seg_read_n_swap(Point_segs, MAX_POINT_SEGS, swap, fp);
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
		int i;

		Escort_kill_object = -1;
		Escort_last_path_created = 0;
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_special_goal = -1;
		Escort_goal_index = -1;

		for (i=0; i<MAX_STOLEN_ITEMS; i++) {
			Stolen_items[i] = 255;
		}

	}

	if (version >= 15) {
		int temp;
		temp = PHYSFSX_readSXE32(fp, swap);
		Point_segs_free_ptr = &Point_segs[temp];
	} else
		ai_reset_all_paths();

	if (version >= 21) {
		int i;
												
		Num_boss_teleport_segs = PHYSFSX_readSXE32(fp, swap);
		Num_boss_gate_segs = PHYSFSX_readSXE32(fp, swap);

		for (i = 0; i < Num_boss_gate_segs; i++)
			Boss_gate_segs[i] = PHYSFSX_readSXE16(fp, swap);

		for (i = 0; i < Num_boss_teleport_segs; i++)
			Boss_teleport_segs[i] = PHYSFSX_readSXE16(fp, swap);
	}

	return 1;
}
