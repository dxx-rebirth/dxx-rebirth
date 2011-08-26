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
 * Autonomous Individual movement.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "inferno.h"
#include "game.h"
#include "console.h"
#include "3d.h"

#include "object.h"
#include "render.h"
#include "error.h"
#include "ai.h"
#include "laser.h"
#include "fvi.h"
#include "polyobj.h"
#include "bm.h"
#include "weapon.h"
#include "physics.h"
#include "collide.h"
#include "fuelcen.h"
#include "player.h"
#include "wall.h"
#include "vclip.h"
#include "digi.h"
#include "fireball.h"
#include "morph.h"
#include "effects.h"
#include "timer.h"
#include "sounds.h"
#include "cntrlcen.h"
#include "multibot.h"
#include "multi.h"
#include "gameseq.h"
#include "key.h"
#include "powerup.h"
#include "gauges.h"
#include "text.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifndef NDEBUG
#include "string.h"
#ifdef __MSDOS__
#include <time.h>
#endif
#endif

//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

void init_boss_segments(short segptr[], int *num_segs, int size_check);
void ai_multi_send_robot_position(int objnum, int force);

#define	PARALLAX	0		//	If !0, then special debugging info for Parallax eyes only enabled.

#define MIN_D 0x100

int	Flinch_scale = 4;
int	Attack_scale = 24;
#define	ANIM_RATE		(F1_0/16)
#define	DELTA_ANG_SCALE	16

sbyte Mike_to_matt_xlate[] = {AS_REST, AS_REST, AS_ALERT, AS_ALERT, AS_FLINCH, AS_FIRE, AS_RECOIL, AS_REST};

// int	No_ai_flag=0;

#define	OVERALL_AGITATION_MAX	100

#define		MAX_AI_CLOAK_INFO	8	//	Must be a power of 2!

#define	BOSS_CLOAK_DURATION	(F1_0*7)
#define	BOSS_DEATH_DURATION	(F1_0*6)
#define	BOSS_DEATH_SOUND_DURATION	0x2ae14		//	2.68 seconds

//	Amount of time since the current robot was last processed for things such as movement.
//	It is not valid to use FrameTime because robots do not get moved every frame.
//fix	AI_proc_time;

int	Num_boss_teleport_segs;
short	Boss_teleport_segs[MAX_BOSS_TELEPORT_SEGS];
#ifndef SHAREWARE
int	Num_boss_gate_segs;
short	Boss_gate_segs[MAX_BOSS_TELEPORT_SEGS];
#endif

//	---------- John: These variables must be saved as part of gamesave. ----------
int				Ai_initialized = 0;
int				Overall_agitation;
ai_local			Ai_local_info[MAX_OBJECTS];
point_seg		Point_segs[MAX_POINT_SEGS];
point_seg		*Point_segs_free_ptr = Point_segs;
ai_cloak_info	Ai_cloak_info[MAX_AI_CLOAK_INFO];
fix64				Boss_cloak_start_time = 0;
fix64				Boss_cloak_end_time = 0;
fix64				Last_teleport_time = 0;
fix				Boss_teleport_interval = F1_0*8;
fix				Boss_cloak_interval = F1_0*10;					//	Time between cloaks
fix				Boss_cloak_duration = BOSS_CLOAK_DURATION;
fix64				Last_gate_time = 0;
fix				Gate_interval = F1_0*6;
fix64				Boss_dying_start_time;
int				Boss_dying, Boss_dying_sound_playing, Boss_hit_this_frame;
int				Boss_been_hit=0;


//	---------- John: End of variables which must be saved as part of gamesave. ----------
int				ai_evaded=0;

#ifndef SHAREWARE
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
sbyte	Super_boss_gate_list[] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22, 0, 8, 11, 19, 20, 8, 20, 8};
#define	MAX_GATE_INDEX	( sizeof(Super_boss_gate_list) / sizeof(Super_boss_gate_list[0]) )
#endif

int	Ai_info_enabled=0;

#define	MAX_AWARENESS_EVENTS	64
typedef struct awareness_event {
	short 		segnum;				// segment the event occurred in
	short			type;					// type of event, defines behavior
	vms_vector	pos;					// absolute 3 space location of event
} awareness_event;


//	These globals are set by a call to find_vector_intersection, which is a slow routine,
//	so we don't want to call it again (for this object) unless we have to.
vms_vector	Hit_pos;
int			Hit_type, Hit_seg;
fvi_info		Hit_data;

int					Num_awareness_events = 0;
awareness_event	Awareness_events[MAX_AWARENESS_EVENTS];

vms_vector		Believed_player_pos;

#define	AIS_MAX	8
#define	AIE_MAX	4

//--unused-- int	Processed_this_frame, LastFrameCount;
#ifndef NDEBUG
//	Index into this array with ailp->mode
char	mode_text[8][9] = {
	"STILL   ",
	"WANDER  ",
	"FOL_PATH",
	"CHASE_OB",
	"RUN_FROM",
	"HIDE    ",
	"FOL_PAT2",
	"OPENDOR2"
};

//	Index into this array with aip->behavior
char	behavior_text[6][9] = {
	"STILL   ",
	"NORMAL  ",
	"HIDE    ",
	"RUN_FROM",
	"FOLPATH ",
	"STATION "
};

//	Index into this array with aip->GOAL_STATE or aip->CURRENT_STATE
char	state_text[8][5] = {
	"NONE",
	"REST",
	"SRCH",
	"LOCK",
	"FLIN",
	"FIRE",
	"RECO",
	"ERR_",
};


int	Ai_animation_test=0;
#endif

// Current state indicates where the robot current is, or has just done.
//	Transition table between states for an AI object.
//	 First dimension is trigger event.
//	Second dimension is current state.
//	 Third dimension is goal state.
//	Result is new goal state.
//	ERR_ means something impossible has happened.
sbyte Ai_transition_table[AI_MAX_EVENT][AI_MAX_STATE][AI_MAX_STATE] = {
	{
	//	Event = AIE_FIRE, a nearby object fired
	//	none			rest			srch			lock			flin			fire			reco				// CURRENT is rows, GOAL is columns
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	none
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	rest
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	search
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	lock
	{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},		//	flinch
	{	AIS_ERR_,	AIS_FIRE,	AIS_FIRE,	AIS_FIRE,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},		//	fire
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}		//	recoil
	},

	//	Event = AIE_HITT, a nearby object was hit (or a wall was hit)
	{
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}
	},

	//	Event = AIE_COLL, player collided with robot
	{
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_LOCK,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_REST,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FIRE,	AIS_RECO},
	{	AIS_ERR_,	AIS_LOCK,	AIS_LOCK,	AIS_LOCK,	AIS_FLIN,	AIS_FIRE,	AIS_FIRE}
	},

	//	Event = AIE_HURT, player hurt robot (by firing at and hitting it)
	//	Note, this doesn't necessarily mean the robot JUST got hit, only that that is the most recent thing that happened.
	{
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN},
	{	AIS_ERR_,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN,	AIS_FLIN}
	}
};

// ---------------------------------------------------------------------------------------------------------------------
//	Given a behavior, set initial mode.
int ai_behavior_to_mode(int behavior)
{
	switch (behavior) {
		case AIB_STILL:			return AIM_STILL;
		case AIB_NORMAL:			return AIM_CHASE_OBJECT;
		case AIB_HIDE:				return AIM_HIDE;
		case AIB_RUN_FROM:		return AIM_RUN_FROM_OBJECT;
		case AIB_FOLLOW_PATH:	return AIM_FOLLOW_PATH;
		case AIB_STATION:			return AIM_STILL;
		default:	Int3();	//	Contact Mike: Error, illegal behavior type
	}

	return AIM_STILL;
}

// ---------------------------------------------------------------------------------------------------------------------
//	Call every time the player starts a new ship.
void ai_init_boss_for_ship(void)
{
	Boss_been_hit = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
//	initial_mode == -1 means leave mode unchanged.
void init_ai_object(int objnum, int behavior, int hide_segment)
{
	object	*objp = &Objects[objnum];
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objnum];

	memset(ailp, 0, sizeof(ai_local));

#ifdef DEST_SAT
		if (!(Game_mode & GM_MULTI) && Robot_info[objp->id].boss_flag) {
			if (Current_level_num != Last_level) {
				objp->id = 0;
				objp->flags |= OF_SHOULD_BE_DEAD;
			}
		}
#endif

	if (behavior == 0) {
		behavior = AIB_NORMAL;
		objp->ctype.ai_info.behavior = behavior;
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

	// This is astonishingly stupid!  This routine gets called by matcens! KILL KILL KILL!!! Point_segs_free_ptr = Point_segs;

	vm_vec_zero(&objp->mtype.phys_info.velocity);
	// -- ailp->wait_time = F1_0*5;
	ailp->player_awareness_time = 0;
	ailp->player_awareness_type = 0;
	aip->GOAL_STATE = AIS_SRCH;
	aip->CURRENT_STATE = AIS_REST;
	ailp->time_player_seen = GameTime64;
	ailp->next_misc_sound_time = GameTime64;
	ailp->time_player_sound_attacked = GameTime64;

	if ((behavior == AIB_HIDE) || (behavior == AIB_FOLLOW_PATH) || (behavior == AIB_STATION) || (behavior == AIB_RUN_FROM)) {
		aip->hide_segment = hide_segment;
		ailp->goal_segment = hide_segment;
		aip->hide_index = -1;			// This means the path has not yet been created.
		aip->cur_path_index = 0;
	}

	aip->SKIP_AI_COUNT = 0;

	if (Robot_info[objp->id].cloak_type == RI_CLOAKED_ALWAYS)
		aip->CLOAKED = 1;
	else
		aip->CLOAKED = 0;

	objp->mtype.phys_info.flags |= (PF_BOUNCE | PF_TURNROLL);
	
	aip->REMOTE_OWNER = -1;
}

// ---------------------------------------------------------------------------------------------------------------------
void init_ai_objects(void)
{
	int	i;

	Point_segs_free_ptr = Point_segs;

	for (i=0; i<MAX_OBJECTS; i++) {
		object *objp = &Objects[i];

		if (objp->control_type == CT_AI)
			init_ai_object(i, objp->ctype.ai_info.behavior, objp->ctype.ai_info.hide_segment);
	}

	init_boss_segments(Boss_teleport_segs, &Num_boss_teleport_segs, 1);

	#ifndef SHAREWARE
		init_boss_segments(Boss_gate_segs, &Num_boss_gate_segs, 0);
	#endif

	Boss_dying_sound_playing = 0;
	Boss_dying = 0;
	Boss_been_hit = 0;
	#ifndef SHAREWARE
	Gate_interval = F1_0*5 - Difficulty_level*F1_0/2;
	#endif

	Ai_initialized = 1;
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
void set_rotvel_and_saturate(fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs(delta) < F1_0/8) {
			*dest = delta/4;
		} else
			*dest = delta;
	} else {
		*dest = delta;
	}
}

//--debug-- #ifndef NDEBUG
//--debug-- int	Total_turns=0;
//--debug-- int	Prevented_turns=0;
//--debug-- #endif

#define	AI_TURN_SCALE	1
#define	BABY_SPIDER_ID	14

extern void physics_turn_towards_vector(vms_vector *goal_vector, object *obj, fix rate);

//-------------------------------------------------------------------------------------------
void ai_turn_towards_vector(vms_vector *goal_vector, object *objp, fix rate)
{
	vms_vector	new_fvec;
	fix			dot;

	if ((objp->id == BABY_SPIDER_ID) && (objp->type == OBJ_ROBOT)) {
		physics_turn_towards_vector(goal_vector, objp, rate);
		return;
	}

	new_fvec = *goal_vector;

	dot = vm_vec_dot(goal_vector, &objp->orient.fvec);

	if (dot < (F1_0 - FrameTime/2)) {
		fix	mag;
		fix	new_scale = fixdiv(FrameTime * AI_TURN_SCALE, rate);
		vm_vec_scale(&new_fvec, new_scale);
		vm_vec_add2(&new_fvec, &objp->orient.fvec);
		mag = vm_vec_normalize_quick(&new_fvec);
		if (mag < F1_0/256) {
			new_fvec = *goal_vector;		//	if degenerate vector, go right to goal
		}
	}

	vm_vector_2_matrix(&objp->orient, &new_fvec, NULL, &objp->orient.rvec);
}

// --------------------------------------------------------------------------------------------------------------------
void ai_turn_randomly(vms_vector *vec_to_player, object *obj, fix rate, int previous_visibility)
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
int player_is_visible_from_object(object *objp, vms_vector *pos, fix field_of_view, vms_vector *vec_to_player)
{
	fix			dot;
	fvi_query	fq;

	fq.p0						= pos;
	if ((pos->x != objp->pos.x) || (pos->y != objp->pos.y) || (pos->z != objp->pos.z)) {
		int	segnum = find_point_seg(pos, objp->segnum);
		if (segnum == -1) {
			fq.startseg = objp->segnum;
			*pos = objp->pos;
			con_printf(CON_DEBUG, "Object %i, gun is outside mine, moving towards center.\n", objp-Objects);
			move_towards_segment_center(objp);
		} else
			fq.startseg = segnum;
	} else
		fq.startseg			= objp->segnum;
	fq.p1						= &Believed_player_pos;
	fq.rad					= F1_0/4;
	fq.thisobjnum			= objp-Objects;
	fq.ignore_obj_list	= NULL;
	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	Hit_type = find_vector_intersection(&fq,&Hit_data);

	Hit_pos = Hit_data.hit_pnt;
	Hit_seg = Hit_data.hit_seg;

	if ((Hit_type == HIT_NONE) || ((Hit_type == HIT_OBJECT) && (Hit_data.hit_object == Players[Player_num].objnum))) {
		dot = vm_vec_dot(vec_to_player, &objp->orient.fvec);
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
int do_silly_animation(object *objp)
{
	int				objnum = objp-Objects;
	jointpos 		*jp_list;
	int				robot_type, gun_num, robot_state, num_joint_positions;
	polyobj_info	*pobj_info = &objp->rtype.pobj_info;
	ai_static		*aip = &objp->ctype.ai_info;
	// ai_local			*ailp = &Ai_local_info[objnum];
	int				num_guns, at_goal;
	int				attack_type;
	int				flinch_attack_scale = 1;

	robot_type = objp->id;
	num_guns = Robot_info[robot_type].n_guns;
	attack_type = Robot_info[robot_type].attack_type;

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
			int			jointnum = jp_list[joint].jointnum;
			vms_angvec	*jp = &jp_list[joint].angles;
			vms_angvec	*pobjp = &pobj_info->anim_angles[jointnum];

			if (jointnum >= Polygon_models[objp->rtype.pobj_info.model_num].n_models) {
				//Int3();	  // Contact Mike: incompatible data, illegal jointnum, problem in pof file?
				continue;
			}
			if (jp->p != pobjp->p) {
				if (gun_num == 0)
					at_goal = 0;
				Ai_local_info[objnum].goal_angles[jointnum].p = jp->p;

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

				Ai_local_info[objnum].delta_angles[jointnum].p = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->b != pobjp->b) {
				if (gun_num == 0)
					at_goal = 0;
				Ai_local_info[objnum].goal_angles[jointnum].b = jp->b;

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

				Ai_local_info[objnum].delta_angles[jointnum].b = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}

			if (jp->h != pobjp->h) {
				if (gun_num == 0)
					at_goal = 0;
				Ai_local_info[objnum].goal_angles[jointnum].h = jp->h;

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

				Ai_local_info[objnum].delta_angles[jointnum].h = delta_2/DELTA_ANG_SCALE;		// complete revolutions per second
			}
		}

		if (at_goal) {
			//ai_static	*aip = &objp->ctype.ai_info;
			ai_local		*ailp = &Ai_local_info[objp-Objects];
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
void ai_frame_animation(object *objp)
{
	int	objnum = objp-Objects;
	int	joint;
	int	num_joints;

	num_joints = Polygon_models[objp->rtype.pobj_info.model_num].n_models;

	for (joint=1; joint<num_joints; joint++) {
		fix			delta_to_goal;
		fix			scaled_delta_angle;
		vms_angvec	*curangp = &objp->rtype.pobj_info.anim_angles[joint];
		vms_angvec	*goalangp = &Ai_local_info[objnum].goal_angles[joint];
		vms_angvec	*deltaangp = &Ai_local_info[objnum].delta_angles[joint];

#ifndef NDEBUG
if (Ai_animation_test) {
	con_printf(CON_DEBUG, "%i: [%7.3f %7.3f %7.3f]  [%7.3f %7.3f %7.3f]\n", joint, f2fl(curangp->p), f2fl(curangp->b), f2fl(curangp->h), f2fl(goalangp->p), f2fl(goalangp->b), f2fl(goalangp->h));
}
#endif
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
void set_next_fire_time(ai_local *ailp, robot_info *robptr)
{
	ailp->rapidfire_count++;

	if (ailp->rapidfire_count < robptr->rapidfire_count[Difficulty_level]) {
		ailp->next_fire = min(F1_0/8, robptr->firing_wait[Difficulty_level]/2);
	} else {
		ailp->rapidfire_count = 0;
		ailp->next_fire = robptr->firing_wait[Difficulty_level];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void do_ai_robot_hit_attack(object *robot, object *player, vms_vector *collision_point)
{
	ai_local		*ailp = &Ai_local_info[robot-Objects];
	robot_info *robptr = &Robot_info[robot->id];

//#ifndef NDEBUG
	if (cheats.robotfiringsuspended)
		return;
//#endif

	//	If player is dead, stop firing.
	if (Objects[Players[Player_num].objnum].type == OBJ_GHOST)
		return;

	if (robptr->attack_type == 1) {
		if (ailp->next_fire <= 0) {
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				if (vm_vec_dist_quick(&ConsoleObject->pos, &robot->pos) < robot->size + ConsoleObject->size + F1_0*2)
					collide_player_and_nasty_robot( player, robot, collision_point );

			robot->ctype.ai_info.GOAL_STATE = AIS_RECO;
			set_next_fire_time(ailp, robptr);
		}
	}

}

extern int Player_exploded;

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter vec_to_player is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter vec_to_player should not be necessary.
void ai_fire_laser_at_player(object *obj, vms_vector *fire_point)
{
	int			objnum = obj-Objects;
	ai_local		*ailp = &Ai_local_info[objnum];
	robot_info	*robptr = &Robot_info[obj->id];
	vms_vector	fire_vec;
	vms_vector	bpp_diff;

	if (cheats.robotfiringsuspended)
		return;

#ifndef NDEBUG
	//	We should never be coming here for the green guy, as he has no laser!
	if (robptr->attack_type == 1)
		Int3();	// Contact Mike: This is impossible.
#endif

	if (obj->control_type == CT_MORPH)
		return;

	//	If player is exploded, stop firing.
	if (Player_exploded)
		return;

	//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		fix64	cloak_time = Ai_cloak_info[objnum % MAX_AI_CLOAK_INFO].last_time;

		if (GameTime64 - cloak_time > CLOAK_TIME_MAX/4)
                        if (d_rand() > fixdiv(GameTime64 - cloak_time, CLOAK_TIME_MAX)/2) {
				set_next_fire_time(ailp, robptr);
				return;
			}
	}

//--	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
//--	//	which it fires may be in a different segment, which is bad news for find_vector_intersection.  So, cast
//--	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to Laser_create_new
//--	//	use the data returned from this call to find_vector_intersection.
//--	//	Note that while find_vector_intersection is pretty slow, it is not terribly slow if the destination point is
//--	//	in the same segment as the source point.
//--
//--	fq.p0						= &obj->pos;
//--	fq.startseg				= obj->segnum;
//--	fq.p1						= fire_point;
//--	fq.rad					= 0;
//--	fq.thisobjnum			= obj-Objects;
//--	fq.ignore_obj_list	= NULL;
//--	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???
//--
//--	fate = find_vector_intersection(&fq, &hit_data);
//--	if (fate != HIT_NONE)
//--		return;

	//	Set position to fire at based on difficulty level.
        bpp_diff.x = Believed_player_pos.x + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;
        bpp_diff.y = Believed_player_pos.y + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;
        bpp_diff.z = Believed_player_pos.z + (d_rand()-16384) * (NDL-Difficulty_level-1) * 4;

	//	Half the time fire at the player, half the time lead the player.
        if (d_rand() > 16384) {

		vm_vec_normalized_dir_quick(&fire_vec, &bpp_diff, fire_point);

	} else {
		vms_vector	player_direction_vector;

		vm_vec_sub(&player_direction_vector, &bpp_diff, &bpp_diff);

		// If player is not moving, fire right at him!
		//	Note: If the robot fires in the direction of its forward vector, this is bad because the weapon does not
		//	come out from the center of the robot; it comes out from the side.  So it is common for the weapon to miss
		//	its target.  Ideally, we want to point the guns at the player.  For now, just fire right at the player.
		if ((abs(player_direction_vector.x < 0x10000)) && (abs(player_direction_vector.y < 0x10000)) && (abs(player_direction_vector.z < 0x10000))) {

			vm_vec_normalized_dir_quick(&fire_vec, &bpp_diff, fire_point);

		// Player is moving.  Determine where the player will be at the end of the next frame if he doesn't change his
		//	behavior.  Fire at exactly that point.  This isn't exactly what you want because it will probably take the laser
		//	a different amount of time to get there, since it will probably be a different distance from the player.
		//	So, that's why we write games, instead of guiding missiles...
		} else {
			vm_vec_sub(&fire_vec, &bpp_diff, fire_point);
			vm_vec_scale(&fire_vec,fixmul(Weapon_info[Robot_info[obj->id].weapon_type].speed[Difficulty_level], FrameTime));

			vm_vec_add2(&fire_vec, &player_direction_vector);
			vm_vec_normalize_quick(&fire_vec);

		}
	}

	Laser_create_new_easy( &fire_vec, fire_point, obj-Objects, robptr->weapon_type, 1);

#ifndef SHAREWARE
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		ai_multi_send_robot_position(objnum, -1);
		multi_send_robot_fire(objnum, obj->ctype.ai_info.CURRENT_GUN, &fire_vec);
	}
#endif
#endif

	create_awareness_event(obj, PA_NEARBY_ROBOT_FIRED);

	set_next_fire_time(ailp, robptr);

	//	If the boss fired, allow him to teleport very soon (right after firing, cool!), pending other factors.
	if (robptr->boss_flag)
		Last_teleport_time -= Boss_teleport_interval/2;
}

// --------------------------------------------------------------------------------------------------------------------
//	vec_goal must be normalized, or close to it.
void move_towards_vector(object *objp, vms_vector *vec_goal)
{
	physics_info	*pptr = &objp->mtype.phys_info;
	fix				speed, dot, max_speed;
	robot_info		*robptr = &Robot_info[objp->id];
	vms_vector		vel;

	//	Trying to move towards player.  If forward vector much different than velocity vector,
	//	bash velocity vector twice as much towards player as usual.

	vel = pptr->velocity;
	vm_vec_normalize_quick(&vel);
	dot = vm_vec_dot(&vel, &objp->orient.fvec);

	if (dot < 3*F1_0/4) {
		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
		//	more quickly than the general code
		pptr->velocity.x = pptr->velocity.x/2 + fixmul(vec_goal->x, FrameTime*32);
		pptr->velocity.y = pptr->velocity.y/2 + fixmul(vec_goal->y, FrameTime*32);
		pptr->velocity.z = pptr->velocity.z/2 + fixmul(vec_goal->z, FrameTime*32);
	} else {
		pptr->velocity.x += fixmul(vec_goal->x, FrameTime*64) * (Difficulty_level+5)/4;
		pptr->velocity.y += fixmul(vec_goal->y, FrameTime*64) * (Difficulty_level+5)/4;
		pptr->velocity.z += fixmul(vec_goal->z, FrameTime*64) * (Difficulty_level+5)/4;
	}

	speed = vm_vec_mag_quick(&pptr->velocity);
	max_speed = robptr->max_speed[Difficulty_level];

	//	Green guy attacks twice as fast as he moves away.
	if (robptr->attack_type == 1)
		max_speed *= 2;

	if (speed > max_speed) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}
}

// --------------------------------------------------------------------------------------------------------------------
void move_towards_player(object *objp, vms_vector *vec_to_player)
//	vec_to_player must be normalized, or close to it.
{
	move_towards_vector(objp, vec_to_player);
}

// --------------------------------------------------------------------------------------------------------------------
//	I am ashamed of this: fast_flag == -1 means normal slide about.  fast_flag = 0 means no evasion.
void move_around_player(object *objp, vms_vector *vec_to_player, int fast_flag)
{
	physics_info	*pptr = &objp->mtype.phys_info;
	fix				speed;
	robot_info		*robptr = &Robot_info[objp->id];
	int				objnum = objp-Objects;
	int				dir;
	int				dir_change;
	fix				ft;
	vms_vector		evade_vector;
	int				count=0;

	if (fast_flag == 0)
		return;

	dir_change = 48;
	ft = FrameTime;
	if (ft < F1_0/32) {
		dir_change *= 8;
		count += 3;
	} else
		while (ft < F1_0/4) {
			dir_change *= 2;
			ft *= 2;
			count++;
		}

	dir = (/*FrameCount*/(GameTime64/2000==0?1:GameTime64/2000) + (count+1) * (objnum*8 + objnum*4 + objnum)) & dir_change;
	dir >>= (4+count);

	Assert((dir >= 0) && (dir <= 3));

	switch (dir) {
		case 0:
			evade_vector.x = fixmul(vec_to_player->z, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player->y, FrameTime*32);
			evade_vector.z = fixmul(-vec_to_player->x, FrameTime*32);
			break;
		case 1:
			evade_vector.x = fixmul(-vec_to_player->z, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player->y, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player->x, FrameTime*32);
			break;
		case 2:
			evade_vector.x = fixmul(-vec_to_player->y, FrameTime*32);
			evade_vector.y = fixmul(vec_to_player->x, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player->z, FrameTime*32);
			break;
		case 3:
			evade_vector.x = fixmul(vec_to_player->y, FrameTime*32);
			evade_vector.y = fixmul(-vec_to_player->x, FrameTime*32);
			evade_vector.z = fixmul(vec_to_player->z, FrameTime*32);
			break;
	}

	//	Note: -1 means normal circling about the player.  > 0 means fast evasion.
	if (fast_flag > 0) {
		fix	dot;

		//	Only take evasive action if looking at player.
		//	Evasion speed is scaled by percentage of shields left so wounded robots evade less effectively.

		dot = vm_vec_dot(vec_to_player, &objp->orient.fvec);
		if ((dot > robptr->field_of_view[Difficulty_level]) && !(ConsoleObject->flags & PLAYER_FLAGS_CLOAKED)) {
			fix	damage_scale;

			damage_scale = fixdiv(objp->shields, robptr->strength);
			if (damage_scale > F1_0)
				damage_scale = F1_0;		//	Just in case...
			else if (damage_scale < 0)
				damage_scale = 0;			//	Just in case...

			vm_vec_scale(&evade_vector, i2f(fast_flag) + damage_scale);
		}
	}

	pptr->velocity.x += evade_vector.x;
	pptr->velocity.y += evade_vector.y;
	pptr->velocity.z += evade_vector.z;

	speed = vm_vec_mag_quick(&pptr->velocity);
	if (speed > robptr->max_speed[Difficulty_level]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}

}

// --------------------------------------------------------------------------------------------------------------------
void move_away_from_player(object *objp, vms_vector *vec_to_player, int attack_type)
{
	fix				speed;
	physics_info	*pptr = &objp->mtype.phys_info;
	robot_info		*robptr = &Robot_info[objp->id];
	int				objref;

	pptr->velocity.x -= fixmul(vec_to_player->x, FrameTime*16);
	pptr->velocity.y -= fixmul(vec_to_player->y, FrameTime*16);
	pptr->velocity.z -= fixmul(vec_to_player->z, FrameTime*16);

	if (attack_type) {
		//	Get value in 0..3 to choose evasion direction.
		objref = ((objp-Objects) ^ ((FrameCount + 3*(objp-Objects)) >> 5)) & 3;

		switch (objref) {
			case 0:	vm_vec_scale_add2(&pptr->velocity, &objp->orient.uvec, FrameTime << 5);	break;
			case 1:	vm_vec_scale_add2(&pptr->velocity, &objp->orient.uvec, -FrameTime << 5);	break;
			case 2:	vm_vec_scale_add2(&pptr->velocity, &objp->orient.rvec, FrameTime << 5);	break;
			case 3:	vm_vec_scale_add2(&pptr->velocity, &objp->orient.rvec, -FrameTime << 5);	break;
			default:	Int3();	//	Impossible, bogus value on objref, must be in 0..3
		}
	}


	speed = vm_vec_mag_quick(&pptr->velocity);

	if (speed > robptr->max_speed[Difficulty_level]) {
		pptr->velocity.x = (pptr->velocity.x*3)/4;
		pptr->velocity.y = (pptr->velocity.y*3)/4;
		pptr->velocity.z = (pptr->velocity.z*3)/4;
	}

//--old--	fix				speed, dot;
//--old--	physics_info	*pptr = &objp->mtype.phys_info;
//--old--	robot_info		*robptr = &Robot_info[objp->id];
//--old--
//--old--	//	Trying to move away from player.  If forward vector much different than velocity vector,
//--old--	//	bash velocity vector twice as much away from player as usual.
//--old--	dot = vm_vec_dot(&pptr->velocity, &objp->orient.fvec);
//--old--	if (dot > -3*F1_0/4) {
//--old--		//	This funny code is supposed to slow down the robot and move his velocity towards his direction
//--old--		//	more quickly than the general code
//--old--		pptr->velocity.x = pptr->velocity.x/2 - fixmul(vec_to_player->x, FrameTime*16);
//--old--		pptr->velocity.y = pptr->velocity.y/2 - fixmul(vec_to_player->y, FrameTime*16);
//--old--		pptr->velocity.z = pptr->velocity.z/2 - fixmul(vec_to_player->z, FrameTime*16);
//--old--	} else {
//--old--		pptr->velocity.x -= fixmul(vec_to_player->x, FrameTime*16);
//--old--		pptr->velocity.y -= fixmul(vec_to_player->y, FrameTime*16);
//--old--		pptr->velocity.z -= fixmul(vec_to_player->z, FrameTime*16);
//--old--	}
//--old--
//--old--	speed = vm_vec_mag_quick(&pptr->velocity);
//--old--
//--old--	if (speed > robptr->max_speed[Difficulty_level]) {
//--old--		pptr->velocity.x = (pptr->velocity.x*3)/4;
//--old--		pptr->velocity.y = (pptr->velocity.y*3)/4;
//--old--		pptr->velocity.z = (pptr->velocity.z*3)/4;
//--old--	}
}

// --------------------------------------------------------------------------------------------------------------------
//	Move towards, away_from or around player.
//	Also deals with evasion.
//	If the flag evade_only is set, then only allowed to evade, not allowed to move otherwise (must have mode == AIM_STILL).
void ai_move_relative_to_player(object *objp, ai_local *ailp, fix dist_to_player, vms_vector *vec_to_player, fix circle_distance, int evade_only)
{
	object		*dobjp;
	robot_info	*robptr = &Robot_info[objp->id];

	//	See if should take avoidance.

// New way, green guys don't evade:	if ((robptr->attack_type == 0) && (objp->ctype.ai_info.danger_laser_num != -1)) {
	if (objp->ctype.ai_info.danger_laser_num != -1) {
		dobjp = &Objects[objp->ctype.ai_info.danger_laser_num];

		if ((dobjp->type == OBJ_WEAPON) && (dobjp->signature == objp->ctype.ai_info.danger_laser_signature)) {
			fix			dot, dist_to_laser, field_of_view;
			vms_vector	vec_to_laser, laser_fvec;

			field_of_view = Robot_info[objp->id].field_of_view[Difficulty_level];

			vm_vec_sub(&vec_to_laser, &dobjp->pos, &objp->pos);
			dist_to_laser = vm_vec_normalize_quick(&vec_to_laser);
			dot = vm_vec_dot(&vec_to_laser, &objp->orient.fvec);

			if (dot > field_of_view) {
				fix			laser_robot_dot;
				vms_vector	laser_vec_to_robot;

				//	The laser is seen by the robot, see if it might hit the robot.
				//	Get the laser's direction.  If it's a polyobj, it can be gotten cheaply from the orientation matrix.
				if (dobjp->render_type == RT_POLYOBJ)
					laser_fvec = dobjp->orient.fvec;
				else {		//	Not a polyobj, get velocity and normalize.
					laser_fvec = dobjp->mtype.phys_info.velocity;	//dobjp->orient.fvec;
					vm_vec_normalize_quick(&laser_fvec);
				}
				vm_vec_sub(&laser_vec_to_robot, &objp->pos, &dobjp->pos);
				vm_vec_normalize_quick(&laser_vec_to_robot);
				laser_robot_dot = vm_vec_dot(&laser_fvec, &laser_vec_to_robot);

				if ((laser_robot_dot > F1_0*7/8) && (dist_to_laser < F1_0*80)) {
					int	evade_speed;

					ai_evaded = 1;
					evade_speed = Robot_info[objp->id].evade_speed[Difficulty_level];

					move_around_player(objp, vec_to_player, evade_speed);
				}
			}
			return;
		}
	}

	//	If only allowed to do evade code, then done.
	//	Hmm, perhaps brilliant insight.  If want claw-type guys to keep coming, don't return here after evasion.
	if ((!robptr->attack_type) && evade_only)
		return;

	//	If we fall out of above, then no object to be avoided.
	objp->ctype.ai_info.danger_laser_num = -1;

	//	Green guy selects move around/towards/away based on firing time, not distance.
	if (robptr->attack_type == 1) {
		if (((ailp->next_fire > robptr->firing_wait[Difficulty_level]/4) && (dist_to_player < F1_0*30)) || Player_is_dead) {
			//	1/4 of time, move around player, 3/4 of time, move away from player
                        if (d_rand() < 8192) {
				move_around_player(objp, vec_to_player, -1);
			} else {
				move_away_from_player(objp, vec_to_player, 1);
			}
		} else {
			move_towards_player(objp, vec_to_player);
		}
	} else {
		if (dist_to_player < circle_distance)
			move_away_from_player(objp, vec_to_player, 0);
		else if (dist_to_player < circle_distance*2)
			move_around_player(objp, vec_to_player, -1);
		else
			move_towards_player(objp, vec_to_player);
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Compute a somewhat random, normalized vector.
void make_random_vector(vms_vector *vec)
{
        vec->x = (d_rand() - 16384) | 1;  // make sure we don't create null vector
        vec->y = d_rand() - 16384;
        vec->z = d_rand() - 16384;

	vm_vec_normalize_quick(vec);
}

//	-------------------------------------------------------------------------------------------------------------------
int	Break_on_object = -1;

void do_firing_stuff(object *obj, int player_visibility, vms_vector *vec_to_player)
{
	if (player_visibility >= 1) {
		//	Now, if in robot's field of view, lock onto player
		fix	dot = vm_vec_dot(&obj->orient.fvec, vec_to_player);
		if ((dot >= 7*F1_0/8) || (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
			ai_static	*aip = &obj->ctype.ai_info;
			ai_local		*ailp = &Ai_local_info[obj-Objects];

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
void do_ai_robot_hit(object *objp, int type)
{
	if (objp->control_type == CT_AI) {
		if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_PLAYER_COLLISION))
			switch (objp->ctype.ai_info.behavior) {
				case AIM_HIDE:
					objp->ctype.ai_info.SUBMODE = AISM_GOHIDE;
					break;
				case AIM_STILL:
					Ai_local_info[objp-Objects].mode = AIM_CHASE_OBJECT;
					break;
			}
	}

}
#ifndef NDEBUG
int	Do_ai_flag=1;
int	Cvv_test=0;
int	Cvv_last_time[MAX_OBJECTS];
int	Gun_point_hack=0;
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
void compute_vis_and_vec(object *objp, vms_vector *pos, ai_local *ailp, vms_vector *vec_to_player, int *player_visibility, robot_info *robptr, int *flag)
{
	if (!*flag) {
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
			fix			delta_time, dist;
			int			cloak_index = (objp-Objects) % MAX_AI_CLOAK_INFO;

			delta_time = GameTime64 - Ai_cloak_info[cloak_index].last_time;
			if (delta_time > F1_0*2) {
				vms_vector	randvec;

				Ai_cloak_info[cloak_index].last_time = GameTime64;
				make_random_vector(&randvec);
				vm_vec_scale_add2(&Ai_cloak_info[cloak_index].last_position, &randvec, 8*delta_time );
			}

			dist = vm_vec_normalized_dir_quick(vec_to_player, &Ai_cloak_info[cloak_index].last_position, pos);
			*player_visibility = player_is_visible_from_object(objp, pos, robptr->field_of_view[Difficulty_level], vec_to_player);
			// *player_visibility = 2;

			if ((ailp->next_misc_sound_time < GameTime64) && (ailp->next_fire < F1_0) && (dist < F1_0*20)) {
                                ailp->next_misc_sound_time = GameTime64 + (d_rand() + F1_0) * (7 - Difficulty_level) / 1;
				digi_link_sound_to_pos( robptr->see_sound, objp->segnum, 0, pos, 0 , Robot_sound_volume);
			}
		} else {
			//	Compute expensive stuff -- vec_to_player and player_visibility
			vm_vec_normalized_dir_quick(vec_to_player, &Believed_player_pos, pos);
			if ((vec_to_player->x == 0) && (vec_to_player->y == 0) && (vec_to_player->z == 0)) {
				con_printf(CON_DEBUG, "Warning: Player and robot at exactly the same location.\n");
				vec_to_player->x = F1_0;
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

			if (!Player_exploded && (ailp->previous_visibility != *player_visibility) && (*player_visibility == 2)) {
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

		if (*player_visibility) {
			ailp->time_player_seen = GameTime64;
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------
//	Move the object objp to a spot in which it doesn't intersect a wall.
//	It might mean moving it outside its current segment.
void move_object_to_legal_spot(object *objp)
{
	vms_vector	original_pos = objp->pos;
	int		i;
	segment	*segp = &Segments[objp->segnum];

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (WALL_IS_DOORWAY(segp, i) & WID_FLY_FLAG) {
			vms_vector	segment_center, goal_dir;

			compute_segment_center(&segment_center, &Segments[segp->children[i]]);
			vm_vec_sub(&goal_dir, &segment_center, &objp->pos);
			vm_vec_normalize_quick(&goal_dir);
			vm_vec_scale(&goal_dir, objp->size);
			vm_vec_add2(&objp->pos, &goal_dir);
			if (!object_intersects_wall(objp)) {
				int	new_segnum = find_point_seg(&objp->pos, objp->segnum);

				if (new_segnum != -1) {
					obj_relink(objp-Objects, new_segnum);
					return;
				}
			} else
				objp->pos = original_pos;
		}
	}

	// Int3();		//	Darn you John, you done it again!  (But contact Mike)
	con_printf(CON_DEBUG, "Note: Killing robot #%i because he's badly stuck outside the mine.\n", objp-Objects);

	apply_damage_to_robot(objp, objp->shields*2, objp-Objects);
}

// --------------------------------------------------------------------------------------------------------------------
//	Move object one object radii from current position towards segment center.
//	If segment center is nearer than 2 radii, move it to center.
void move_towards_segment_center(object *objp)
{
/* ZICO's change of 20081103:
   Make move to segment center smoother by using move_towards vector.
   Bot's should not jump around and maybe even intersect with each other!
   In case it breaks something what I do not see, yet, old code is still there. */
#if 1
	int		segnum = objp->segnum;
	vms_vector	vec_to_center, segment_center;

	compute_segment_center(&segment_center, &Segments[segnum]);
	vm_vec_normalized_dir_quick(&vec_to_center, &segment_center, &objp->pos);
	move_towards_vector(objp, &vec_to_center);
#else
	int			segnum = objp->segnum;
	fix			dist_to_center;
	vms_vector	segment_center, goal_dir;

	compute_segment_center(&segment_center, &Segments[segnum]);

	vm_vec_sub(&goal_dir, &segment_center, &objp->pos);
	dist_to_center = vm_vec_normalize_quick(&goal_dir);

	if (dist_to_center < objp->size) {
		//	Center is nearer than the distance we want to move, so move to center.
		objp->pos = segment_center;
		if (object_intersects_wall(objp)) {
			move_object_to_legal_spot(objp);
		}
	} else {
		int	new_segnum;
		//	Move one radii towards center.
		vm_vec_scale(&goal_dir, objp->size);
		vm_vec_add2(&objp->pos, &goal_dir);
		new_segnum = find_point_seg(&objp->pos, objp->segnum);
		if (new_segnum == -1) {
			objp->pos = segment_center;
			move_object_to_legal_spot(objp);
		}
	}
#endif
}

//	-----------------------------------------------------------------------------------------------------------
//	Return true if door can be flown through by a suitable type robot.
//	Only brains and avoid robots can open doors.
int ai_door_is_openable(object *objp, segment *segp, int sidenum)
{
	int	wall_num;

	//	The mighty console object can open all doors (for purposes of determining paths).
	if (objp == ConsoleObject) {
		int	wall_num = segp->sides[sidenum].wall_num;

		if (Walls[wall_num].type == WALL_DOOR)
			return 1;
	}

	if ((objp->id == ROBOT_BRAIN) || (objp->ctype.ai_info.behavior == AIB_RUN_FROM)) {
		wall_num = segp->sides[sidenum].wall_num;

		if (wall_num != -1)
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED))
				return 1;
	}

	return 0;
}

//--//	-----------------------------------------------------------------------------------------------------------
//--//	Return true if object *objp is allowed to open door at wall_num
//--int door_openable_by_robot(object *objp, int wall_num)
//--{
//--	if (objp->id == ROBOT_BRAIN)
//--		if (Walls[wall_num].keys == KEY_NONE)
//--			return 1;
//--
//--	return 0;
//--}

//	-----------------------------------------------------------------------------------------------------------
//	Return side of openable door in segment, if any.  If none, return -1.
int openable_doors_in_segment(object *objp)
{
	int	i;
	int	segnum = objp->segnum;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		if (Segments[segnum].sides[i].wall_num != -1) {
			int	wall_num = Segments[segnum].sides[i].wall_num;
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED))
				return i;
		}
	}

	return -1;

}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if a special object (player or control center) is in this segment.
int special_object_in_seg(int segnum)
{
	int	objnum;

	objnum = Segments[segnum].objects;

	while (objnum != -1) {
		if ((Objects[objnum].type == OBJ_PLAYER) || (Objects[objnum].type == OBJ_CNTRLCEN)) {
			return 1;
		} else
			objnum = Objects[objnum].next;
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	Randomly select a segment attached to *segp, reachable by flying.
int get_random_child(int segnum)
{
	int	sidenum;
	segment	*segp = &Segments[segnum];

        sidenum = (d_rand() * 6) >> 15;

	while (!(WALL_IS_DOORWAY(segp, sidenum) & WID_FLY_FLAG))
                sidenum = (d_rand() * 6) >> 15;

	segnum = segp->children[sidenum];

	return segnum;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if placing an object of size size at pos *pos intersects a (player or robot or control center) in segment *segp.
int check_object_object_intersection(vms_vector *pos, fix size, segment *segp)
{
	int		curobjnum;

	//	If this would intersect with another object (only check those in this segment), then try to move.
	curobjnum = segp->objects;
	while (curobjnum != -1) {
		object *curobjp = &Objects[curobjnum];
		if ((curobjp->type == OBJ_PLAYER) || (curobjp->type == OBJ_ROBOT) || (curobjp->type == OBJ_CNTRLCEN)) {
			if (vm_vec_dist_quick(pos, &curobjp->pos) < size + curobjp->size)
				return 1;
		}
		curobjnum = curobjp->next;
	}

	return 0;

}

#ifndef SHAREWARE

// --------------------------------------------------------------------------------------------------------------------
//	Return true if object created, else return false.
int create_gated_robot( int segnum, int object_id)
{
	int		objnum;
	object	*objp;
	segment	*segp = &Segments[segnum];
	vms_vector	object_pos;
	robot_info	*robptr = &Robot_info[object_id];
	int		i, count=0;
	fix		objsize = Polygon_models[robptr->model_num].rad;
	int		default_behavior;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Objects[i].matcen_creator == BOSS_GATE_MATCEN_NUM)
				count++;

	if (count > 2*Difficulty_level + 3) {
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return 0;
	}

	compute_segment_center(&object_pos, segp);
	pick_random_point_in_seg(&object_pos, segp-Segments);

	//	See if legal to place object here.  If not, move about in segment and try again.
	if (check_object_object_intersection(&object_pos, objsize, segp)) {
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return 0;
	}

	objnum = obj_create(OBJ_ROBOT, object_id, segnum, &object_pos, &vmd_identity_matrix, objsize, CT_AI, MT_PHYSICS, RT_POLYOBJ);

	if ( objnum < 0 ) {
		Last_gate_time = GameTime64 - 3*Gate_interval/4;
		return 0;
	}

	con_printf(CON_DEBUG, "Gating in object %i in segment %i\n", objnum, segp-Segments);

	#ifdef NETWORK
	Net_create_objnums[0] = objnum; // A convenient global to get objnum back to caller for multiplayer
	#endif

	objp = &Objects[objnum];

	//Set polygon-object-specific data

	objp->rtype.pobj_info.model_num = robptr->model_num;
	objp->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	objp->mtype.phys_info.mass = robptr->mass;
	objp->mtype.phys_info.drag = robptr->drag;

	objp->mtype.phys_info.flags |= (PF_LEVELLING);

	objp->shields = robptr->strength;
	objp->matcen_creator = BOSS_GATE_MATCEN_NUM;	//	flag this robot as having been created by the boss.

	default_behavior = AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = AIB_RUN_FROM;

	init_ai_object(objp-Objects, default_behavior, -1 );		//	Note, -1 = segment this robot goes to to hide, should probably be something useful

	object_create_explosion(segnum, &object_pos, i2f(10), VCLIP_MORPHING_ROBOT );
	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, segnum, 0, &object_pos, 0 , F1_0);
	morph_start(objp);

	Last_gate_time = GameTime64;

	Players[Player_num].num_robots_level++;
	Players[Player_num].num_robots_total++;

	return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Make object objp gate in a robot.
//	The process of him bringing in a robot takes one second.
//	Then a robot appears somewhere near the player.
//	Return true if robot successfully created, else return false
int gate_in_robot(int type, int segnum)
{
	if (segnum < 0)
                segnum = Boss_gate_segs[(d_rand() * Num_boss_gate_segs) >> 15];

	Assert((segnum >= 0) && (segnum <= Highest_segment_index));

	return create_gated_robot(segnum, type);
}

#endif

// --------------------------------------------------------------------------------------------------------------------
int boss_fits_in_seg(object *boss_objp, int segnum)
{
	vms_vector	segcenter;
	int			boss_objnum = boss_objp-Objects;
	int			posnum;

	compute_segment_center(&segcenter, &Segments[segnum]);

	for (posnum=0; posnum<9; posnum++) {
		if (posnum > 0) {
			vms_vector	vertex_pos;

			Assert((posnum-1 >= 0) && (posnum-1 < 8));
			vertex_pos = Vertices[Segments[segnum].verts[posnum-1]];
			vm_vec_avg(&boss_objp->pos, &vertex_pos, &segcenter);
		} else
			boss_objp->pos = segcenter;

		obj_relink(boss_objnum, segnum);
		if (!object_intersects_wall(boss_objp))
			return 1;
	}

	return 0;
}

#define	QUEUE_SIZE	256

// --------------------------------------------------------------------------------------------------------------------
//	Create list of segments boss is allowed to teleport to at segptr.
//	Set *num_segs.
//	Boss is allowed to teleport to segments he fits in (calls object_intersects_wall) and
//	he can reach from his initial position (calls find_connected_distance).
//	If size_check is set, then only add segment if boss can fit in it, else any segment is legal.
void init_boss_segments(short segptr[], int *num_segs, int size_check)
{
	int			boss_objnum=-1;
	int			i;

	*num_segs = 0;
#ifdef EDITOR
	N_selected_segs = 0;
#endif

	//	See if there is a boss.  If not, quick out.
	for (i=0; i<=Highest_object_index; i++)
		if ((Objects[i].type == OBJ_ROBOT) && (Robot_info[Objects[i].id].boss_flag))
			boss_objnum = i; // if != 1 then there is more than one boss here.

	if (boss_objnum != -1) {
		int			original_boss_seg;
		vms_vector	original_boss_pos;
		object		*boss_objp = &Objects[boss_objnum];
		int			head, tail;
		int			seg_queue[QUEUE_SIZE];
//ALREADY IN RENDER.H		byte			visited[MAX_SEGMENTS];
		fix			boss_size_save;

		boss_size_save = boss_objp->size;
		boss_objp->size = fixmul((F1_0/4)*3, boss_objp->size);
		original_boss_seg = boss_objp->segnum;
		original_boss_pos = boss_objp->pos;
		head = 0;
		tail = 0;
		seg_queue[head++] = original_boss_seg;

		segptr[(*num_segs)++] = original_boss_seg;
		#ifdef EDITOR
		Selected_segs[N_selected_segs++] = original_boss_seg;
		#endif

		for (i=0; i<=Highest_segment_index; i++)
			visited[i] = 0;

		while (tail != head) {
			int		sidenum;
			segment	*segp = &Segments[seg_queue[tail++]];

			tail &= QUEUE_SIZE-1;

			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
				if (WALL_IS_DOORWAY(segp, sidenum) & WID_FLY_FLAG) {
					if (visited[segp->children[sidenum]] == 0) {
						seg_queue[head++] = segp->children[sidenum];
						visited[segp->children[sidenum]] = 1;
						head &= QUEUE_SIZE-1;
						if (head > tail) {
							if (head == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
						} else
							if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
								Int3();	//	queue overflow.  Make it bigger!
	
						if ((!size_check) || boss_fits_in_seg(boss_objp, segp->children[sidenum])) {
							segptr[(*num_segs)++] = segp->children[sidenum];
							#ifdef EDITOR
							Selected_segs[N_selected_segs++] = segp->children[sidenum];
							#endif
							if (*num_segs >= MAX_BOSS_TELEPORT_SEGS) {
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
		obj_relink(boss_objnum, original_boss_seg);

	}

}

// --------------------------------------------------------------------------------------------------------------------
void teleport_boss(object *objp)
{
	int			rand_segnum;
	vms_vector	boss_dir;
	int			rand_seg;
	Assert(Num_boss_teleport_segs > 0);

	//	Pick a random segment from the list of boss-teleportable-to segments.
        rand_seg = (d_rand() * Num_boss_teleport_segs) >> 15; 
	rand_segnum = Boss_teleport_segs[rand_seg];
	Assert((rand_segnum >= 0) && (rand_segnum <= Highest_segment_index));

#ifndef SHAREWARE
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		multi_send_boss_actions(objp-Objects, 1, rand_seg, 0);
#endif
#endif

	compute_segment_center(&objp->pos, &Segments[rand_segnum]);
	obj_relink(objp-Objects, rand_segnum);

	Last_teleport_time = GameTime64;

	//	make boss point right at player
	vm_vec_sub(&boss_dir, &Objects[Players[Player_num].objnum].pos, &objp->pos);
	vm_vector_2_matrix(&objp->orient, &boss_dir, NULL, NULL);

	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, rand_segnum, 0, &objp->pos, 0 , F1_0);
	digi_kill_sound_linked_to_object( objp-Objects);
	digi_link_sound_to_object2( SOUND_BOSS_SHARE_SEE, objp-Objects, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud

	//	After a teleport, boss can fire right away.
	Ai_local_info[objp-Objects].next_fire = 0;

}

//	----------------------------------------------------------------------
void start_boss_death_sequence(object *objp)
{
	if (Robot_info[objp->id].boss_flag) {
		Boss_dying = 1;
		Boss_dying_start_time = GameTime64;
	}

}

//	----------------------------------------------------------------------
void do_boss_dying_frame(object *objp)
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
			digi_link_sound_to_object2( SOUND_BOSS_SHARE_DIE, objp-Objects, 0, F1_0*4, F1_0*1024 );	//	F1_0*512 means play twice as loud
                } else if (d_rand() < FrameTime*16)
                        create_small_fireball_on_object(objp, (F1_0 + d_rand()) * 8, 0);
        } else if (d_rand() < FrameTime*8)
                create_small_fireball_on_object(objp, (F1_0/2 + d_rand()) * 8, 1);

	if (Boss_dying_start_time + BOSS_DEATH_DURATION < GameTime64 || GameTime64+(F1_0*2) < Boss_dying_start_time) {
		Boss_dying_start_time=GameTime64; // make sure following only happens one time!
		do_controlcen_destroyed_stuff(NULL);
		explode_object(objp, F1_0/4);
		digi_link_sound_to_object2(SOUND_BADASS_EXPLOSION, objp-Objects, 0, F2_0, F1_0*512);
	}
}

#ifndef SHAREWARE 
#ifdef NETWORK
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
int ai_multiplayer_awareness(object *objp, int awareness_level)
{
	int	rval=1;

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI) {
		if (awareness_level == 0)
			return 0;
		rval = multi_can_move_robot(objp-Objects, awareness_level);
	}
#endif

	return rval;

}
#else
#define ai_multiplayer_awareness(a, b) 1
#endif
#else
#define ai_multiplayer_awareness(a, b) 1
#endif

#ifndef NDEBUG
fix	Prev_boss_shields = -1;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_boss_stuff(object *objp)
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
#ifndef SHAREWARE
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_boss_actions(objp-Objects, 2, 0, 0);
#endif
#endif
				}
			}
		}
	} else
		do_boss_dying_frame(objp);

}

#define	BOSS_TO_PLAYER_GATE_DISTANCE	(F1_0*150)

#ifndef SHAREWARE

// --------------------------------------------------------------------------------------------------------------------
//	Do special stuff for a boss.
void do_super_boss_stuff(object *objp, fix dist_to_player, int player_visibility)
{
#ifdef NETWORK
	static int eclip_state = 0;
#endif
	do_boss_stuff(objp);

	// Only master player can cause gating to occur.
	#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && !multi_i_am_master())
                return;
	#endif

	if ((dist_to_player < BOSS_TO_PLAYER_GATE_DISTANCE) || player_visibility || (Game_mode & GM_MULTI)) {
		if (GameTime64 - Last_gate_time > Gate_interval/2) {
			restart_effect(ECLIP_NUM_BOSS);
#ifndef SHAREWARE
#ifdef NETWORK
			if (eclip_state == 0) {
				multi_send_boss_actions(objp-Objects, 4, 0, 0);
				eclip_state = 1;
			}
#endif
#endif
		}
		else {
			stop_effect(ECLIP_NUM_BOSS);
#ifndef SHAREWARE
#ifdef NETWORK
			if (eclip_state == 1) {
				multi_send_boss_actions(objp-Objects, 5, 0, 0);
				eclip_state = 0;
			}
#endif
#endif
		}

		if (GameTime64 - Last_gate_time > Gate_interval)
			if (ai_multiplayer_awareness(objp, 99)) {
				int	rtval;
                                int     randtype = (d_rand() * MAX_GATE_INDEX) >> 15;

				Assert(randtype < MAX_GATE_INDEX);
				randtype = Super_boss_gate_list[randtype];
				Assert(randtype < N_robot_types);

				rtval = gate_in_robot(randtype, -1);
#ifndef SHAREWARE
#ifdef NETWORK
				if (rtval && (Game_mode & GM_MULTI))
				{
					multi_send_boss_actions(objp-Objects, 3, randtype, Net_create_objnums[0]);
					map_objnum_local_to_local(Net_create_objnums[0]);
				}
#endif
#endif
			}	
	}
}

#endif

//int multi_can_move_robot(object *objp, int awareness_level)
//{
//	return 0;
//}

#ifndef SHAREWARE
void ai_multi_send_robot_position(int objnum, int force)
{
#ifndef SHAREWARE
#ifdef NETWORK
	if (Game_mode & GM_MULTI) 
	{
		if (force != -1)
			multi_send_robot_position(objnum, 1);
		else
			multi_send_robot_position(objnum, 0);
	}
#endif
#endif
	return;
}
#else
#define ai_multi_send_robot_position(a, b)
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this object should be allowed to fire at the player.
int maybe_ai_do_actual_firing_stuff(object *obj, ai_static *aip)
{
	if (Game_mode & GM_MULTI)
		if ((aip->GOAL_STATE != AIS_FLIN) && (obj->id != ROBOT_BRAIN))
			if (aip->CURRENT_STATE == AIS_FIRE)
				return 1;

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------
void ai_do_actual_firing_stuff(object *obj, ai_static *aip, ai_local *ailp, robot_info *robptr, vms_vector *vec_to_player, fix dist_to_player, vms_vector *gun_point, int player_visibility, int object_animates)
{
	fix	dot;

	if (player_visibility == 2) {
		//	Changed by mk, 01/04/94, onearm would take about 9 seconds until he can fire at you.
		// if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE)) && (ailp->next_fire <= 0)) {
		if (!object_animates || (ailp->next_fire <= 0)) {
			dot = vm_vec_dot(&obj->orient.fvec, vec_to_player);
			if (dot >= 7*F1_0/8) {

				if (aip->CURRENT_GUN < Robot_info[obj->id].n_guns) {
					if (robptr->attack_type == 1) {
						if (!Player_exploded && (dist_to_player < obj->size + ConsoleObject->size + F1_0*2)) {		// robptr->circle_distance[Difficulty_level] + ConsoleObject->size) {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION-2))
								return;
							do_ai_robot_hit_attack(obj, ConsoleObject, &obj->pos);
						} else {
							return;
						}
					} else {
						if ((gun_point->x == 0) && (gun_point->y == 0) && (gun_point->z == 0)) {
							;
						} else {
							if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
								return;
							ai_fire_laser_at_player(obj, gun_point);
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
				if (aip->CURRENT_GUN >= Robot_info[obj->id].n_guns)
					aip->CURRENT_GUN = 0;
			}
		}
	} else if (Weapon_info[Robot_info[obj->id].weapon_type].homing_flag == 1) {
		//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
		if (((!object_animates) || (ailp->achieved_state[aip->CURRENT_GUN] == AIS_FIRE)) && (ailp->next_fire <= 0) && (vm_vec_dist_quick(&Hit_pos, &obj->pos) > F1_0*40)) {
			if (!ai_multiplayer_awareness(obj, ROBOT_FIRE_AGITATION))
				return;
			ai_fire_laser_at_player(obj, gun_point);

			aip->GOAL_STATE = AIS_RECO;
			ailp->goal_state[aip->CURRENT_GUN] = AIS_RECO;

			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= Robot_info[obj->id].n_guns)
				aip->CURRENT_GUN = 0;
		} else {
			// Switch to next gun for next fire.
			aip->CURRENT_GUN++;
			if (aip->CURRENT_GUN >= Robot_info[obj->id].n_guns)
				aip->CURRENT_GUN = 0;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
void do_ai_frame(object *obj)
{
	int			objnum = obj-Objects;
	ai_static	*aip = &obj->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objnum];
	fix			dist_to_player;
	vms_vector	vec_to_player;
	fix			dot;
	robot_info	*robptr;
	int			player_visibility=-1;
	int			obj_ref;
	int			object_animates;
	int			new_goal_state;
	int			visibility_and_vec_computed = 0;
	int			previous_visibility;
	vms_vector	gun_point;
	vms_vector	vis_vec_pos;

	if (aip->SKIP_AI_COUNT) {
		aip->SKIP_AI_COUNT--;
		return;
	}

	//	Kind of a hack.  If a robot is flinching, but it is time for it to fire, unflinch it.
	//	Else, you can turn a big nasty robot into a wimp by firing flares at it.
	//	This also allows the player to see the cool flinch effect for mechs without unbalancing the game.
	if ((aip->GOAL_STATE == AIS_FLIN) && (ailp->next_fire < 0)) {
		aip->GOAL_STATE = AIS_FIRE;
	}

#ifndef NDEBUG
	if ((aip->behavior == AIB_RUN_FROM) && (ailp->mode != AIM_RUN_FROM_OBJECT))
		Int3();	//	This is peculiar.  Behavior is run from, but mode is not.  Contact Mike.

	if (Ai_animation_test) {
		if (aip->GOAL_STATE == aip->CURRENT_STATE) {
			aip->GOAL_STATE++;
			if (aip->GOAL_STATE > AIS_RECO)
				aip->GOAL_STATE = AIS_REST;
		}

		object_animates = do_silly_animation(obj);
		if (object_animates)
			ai_frame_animation(obj);
		return;
	}

	if (!Do_ai_flag)
		return;

	if (Break_on_object != -1)
		if ((obj-Objects) == Break_on_object)
			Int3();	//	Contact Mike: This is a debug break
#endif

	Believed_player_pos = Ai_cloak_info[objnum & (MAX_AI_CLOAK_INFO-1)].last_position;

//	Assert((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR));
	if (!((aip->behavior >= MIN_BEHAVIOR) && (aip->behavior <= MAX_BEHAVIOR))) {
		aip->behavior = AIB_NORMAL;
	}

	Assert(obj->segnum != -1);
	Assert(obj->id < N_robot_types);

	robptr = &Robot_info[obj->id];
	Assert(robptr->always_0xabcd == 0xabcd);
	obj_ref = objnum ^ FrameCount;
	// -- if (ailp->wait_time > -F1_0*8)
	// -- 	ailp->wait_time -= FrameTime;
	if (ailp->next_fire > -F1_0*8)
		ailp->next_fire -= FrameTime;
	if (ailp->time_since_processed < F1_0*256)
		ailp->time_since_processed += FrameTime;
	previous_visibility = ailp->previous_visibility;	//	Must get this before we toast the master copy!

	//	Deal with cloaking for robots which are cloaked except just before firing.
	if (robptr->cloak_type == RI_CLOAKED_EXCEPT_FIRING)
         {
                if (ailp->next_fire < F1_0/2)
			aip->CLOAKED = 1;
		else
			aip->CLOAKED = 0;
         }

	if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
		Believed_player_pos = ConsoleObject->pos;

	dist_to_player = vm_vec_dist_quick(&Believed_player_pos, &obj->pos);

	//	If this robot can fire, compute visibility from gun position.
	//	Don't want to compute visibility twice, as it is expensive.  (So is call to calc_gun_point).
	if ((ailp->next_fire <= 0) && (dist_to_player < F1_0*200) && (robptr->n_guns) && !(robptr->attack_type)) {
		calc_gun_point(&gun_point, obj, aip->CURRENT_GUN);
		vis_vec_pos = gun_point;
	} else {
		vis_vec_pos = obj->pos;
		vm_vec_zero(&gun_point);
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Occasionally make non-still robots make a path to the player.  Based on agitation and distance from player.
	if ((aip->behavior != AIB_RUN_FROM) && (aip->behavior != AIB_STILL) && !(Game_mode & GM_MULTI))
		if (Overall_agitation > 70) {
                        if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/4)) {
                                if (d_rand() * (Overall_agitation - 40) > F1_0*5) {
					create_path_to_player(obj, 4 + Overall_agitation/8 + Difficulty_level, 1);
					// -- show_path_and_other(obj);
					return;
				}
			}
		}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If retry count not 0, then add it into consecutive_retries.
	//	If it is 0, cut down consecutive_retries.
	//	This is largely a hack to speed up physics and deal with stupid AI.  This is low level
	//	communication between systems of a sort that should not be done.
	if ((ailp->retry_count) && !(Game_mode & GM_MULTI)) {
		ailp->consecutive_retries += ailp->retry_count;
		ailp->retry_count = 0;
		if (ailp->consecutive_retries > 3) {
			switch (ailp->mode) {
				case AIM_CHASE_OBJECT:
					create_path_to_player(obj, 4 + Overall_agitation/8 + Difficulty_level, 1);
					break;
				case AIM_STILL:
					if (!((aip->behavior == AIB_STILL) || (aip->behavior == AIB_STATION)))	//	Behavior is still, so don't follow path.
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
					create_n_segment_path(obj, 5, -1);
					ailp->mode = AIM_RUN_FROM_OBJECT;
					break;
				case AIM_HIDE:
					move_towards_segment_center(obj);
					obj->mtype.phys_info.velocity.x = 0;
					obj->mtype.phys_info.velocity.y = 0;
					obj->mtype.phys_info.velocity.z = 0;
					if (Overall_agitation > (50 - Difficulty_level*4))
						create_path_to_player(obj, 4 + Overall_agitation/8, 1);
					else {
						create_n_segment_path(obj, 5, -1);
					}
					break;
				case AIM_OPEN_DOOR:
					create_n_segment_path_to_door(obj, 5, -1);
					break;
				#ifndef NDEBUG
				case AIM_FOLLOW_PATH_2:
					Int3();	//	Should never happen!
					break;
				#endif
			}
			ailp->consecutive_retries = 0;
		}
	} else
		ailp->consecutive_retries /= 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
 	//	If in materialization center, exit
 	if (!(Game_mode & GM_MULTI) && (Segments[obj->segnum].special == SEGMENT_IS_ROBOTMAKER)) {
 		ai_follow_path(obj, 1);		// 1 = player is visible, which might be a lie, but it works.
 		return;
 	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease player awareness due to the passage of time.
//-----old, faster way from about 11/06/94-----	if (ailp->player_awareness_type) {
//-----old, faster way from about 11/06/94-----		if (ailp->player_awareness_time > 0) {
//-----old, faster way from about 11/06/94-----			ailp->player_awareness_time -= FrameTime;
//-----old, faster way from about 11/06/94-----			if (ailp->player_awareness_time <= 0) {
//-----old, faster way from about 11/06/94----- 				ailp->player_awareness_time = F1_0*2;
//-----old, faster way from about 11/06/94----- 				ailp->player_awareness_type--;
//-----old, faster way from about 11/06/94-----				if (ailp->player_awareness_type < 0) {
//-----old, faster way from about 11/06/94-----	 				aip->GOAL_STATE = AIS_REST;
//-----old, faster way from about 11/06/94-----	 				ailp->player_awareness_type = 0;
//-----old, faster way from about 11/06/94-----				}
//-----old, faster way from about 11/06/94-----
//-----old, faster way from about 11/06/94-----			}
//-----old, faster way from about 11/06/94-----		} else {
//-----old, faster way from about 11/06/94----- 			ailp->player_awareness_type = 0;
//-----old, faster way from about 11/06/94----- 			aip->GOAL_STATE = AIS_REST;
//-----old, faster way from about 11/06/94-----		}
//-----old, faster way from about 11/06/94-----	} else
//-----old, faster way from about 11/06/94-----		aip->GOAL_STATE = AIS_REST;


	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Decrease player awareness due to the passage of time.
	if (ailp->player_awareness_type) {
		if (ailp->player_awareness_time > 0) {
			ailp->player_awareness_time -= FrameTime;
			if (ailp->player_awareness_time <= 0) {
 				ailp->player_awareness_time = F1_0*2;	//new: 11/05/94
 				ailp->player_awareness_type--;	//new: 11/05/94
			}
		} else {
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*2;
			// aip->GOAL_STATE = AIS_REST;
		}
	} else
		aip->GOAL_STATE = AIS_REST;							//new: 12/13/94


	if (Player_is_dead && (ailp->player_awareness_type == 0))
                if ((dist_to_player < F1_0*200) && (d_rand() < FrameTime/8)) {
			if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_RUN_FROM)) {
				if (!ai_multiplayer_awareness(obj, 30))
					return;
				ai_multi_send_robot_position(objnum, -1);

				if (!((ailp->mode == AIM_FOLLOW_PATH) && (aip->cur_path_index < aip->path_length-1)))
                                 {
					if (dist_to_player < F1_0*30)
						create_n_segment_path(obj, 5, 1);
					else
						create_path_to_player(obj, 20, 1);
                                 }
			}
		}

	//	Make sure that if this guy got hit or bumped, then he's chasing player.
	if ((ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) || (ailp->player_awareness_type >= PA_PLAYER_COLLISION)) {
		if ((aip->behavior != AIB_STILL) && (aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM) && (obj->id != ROBOT_BRAIN))
			ailp->mode = AIM_CHASE_OBJECT;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if ((aip->GOAL_STATE == AIS_FLIN) && (aip->CURRENT_STATE == AIS_FLIN))
		aip->GOAL_STATE = AIS_LOCK;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Note: Should only do these two function calls for objects which animate
	if ((dist_to_player < F1_0*100)) { // && !(Game_mode & GM_MULTI)) {
		object_animates = do_silly_animation(obj);
		if (object_animates)
			ai_frame_animation(obj);
	} else {
		//	If Object is supposed to animate, but we don't let it animate due to distance, then
		//	we must change its state, else it will never update.
		aip->CURRENT_STATE = aip->GOAL_STATE;
		object_animates = 0;		//	If we're not doing the animation, then should pretend it doesn't animate.
	}

	switch (Robot_info[obj->id].boss_flag) {
		case 0:
			break;
		case 1:
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			dist_to_player /= 4;

			do_boss_stuff(obj);
			dist_to_player *= 4;
			break;
#ifndef SHAREWARE
		case 2:
			if (aip->GOAL_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_FIRE;
			if (aip->CURRENT_STATE == AIS_FLIN)
				aip->CURRENT_STATE = AIS_FIRE;
			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			{	int pv = player_visibility;
				fix	dtp = dist_to_player/4;

			//	If player cloaked, visibility is screwed up and superboss will gate in robots when not supposed to.
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				pv = 0;
				dtp = vm_vec_dist_quick(&ConsoleObject->pos, &obj->pos)/4;
			}

			do_super_boss_stuff(obj, dtp, pv);
			}
			break;
#endif
		default:
			Int3();	//	Bogus boss flag value.
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Time-slice, don't process all the time, purely an efficiency hack.
	//	Guys whose behavior is station and are not at their hide segment get processed anyway.
	if (ailp->player_awareness_type < PA_WEAPON_ROBOT_COLLISION-1) { // If robot got hit, he gets to attack player always!
		#ifndef NDEBUG
		if (Break_on_object != objnum) {	//	don't time slice if we're interested in this object.
		#endif
			if ((dist_to_player > F1_0*250) && (ailp->time_since_processed <= F1_0*2))
				return;
			else if (!((aip->behavior == AIB_STATION) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->hide_segment != obj->segnum))) {
				if ((dist_to_player > F1_0*150) && (ailp->time_since_processed <= F1_0))
					return;
				else if ((dist_to_player > F1_0*100) && (ailp->time_since_processed <= F1_0/2))
					return;
			}
		#ifndef NDEBUG
		}
		#endif
	}

	//	Reset time since processed, but skew objects so not everything processed synchronously, else
	//	we get fast frames with the occasional very slow frame.
	// AI_proc_time = ailp->time_since_processed;
	ailp->time_since_processed = - ((objnum & 0x03) * FrameTime ) / 2;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Perform special ability
	switch (obj->id) {
		case ROBOT_BRAIN:
			//	Robots function nicely if behavior is Station.  This means they won't move until they
			//	can see the player, at which time they will start wandering about opening doors.
			if (ConsoleObject->segnum == obj->segnum) {
				if (!ai_multiplayer_awareness(obj, 97))
					return;
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				move_away_from_player(obj, &vec_to_player, 0);
				ai_multi_send_robot_position(objnum, -1);
			} else if (ailp->mode != AIM_STILL) {
				int	r;

				r = openable_doors_in_segment(obj);
				if (r != -1) {
					ailp->mode = AIM_OPEN_DOOR;
					aip->GOALSIDE = r;
				} else if (ailp->mode != AIM_FOLLOW_PATH) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}
			} else {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
				if (player_visibility) {
					if (!ai_multiplayer_awareness(obj, 50))
						return;
					create_n_segment_path_to_door(obj, 8+Difficulty_level, -1);		//	third parameter is avoid_seg, -1 means avoid nothing.
					ai_multi_send_robot_position(objnum, -1);
				}
			}
			break;
		default:
			break;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	switch (ailp->mode) {
		case AIM_CHASE_OBJECT: {		// chasing player, sort of, chase if far, back off if close, circle in between
			fix	circle_distance;

			circle_distance = robptr->circle_distance[Difficulty_level] + ConsoleObject->size;
			//	Green guy doesn't get his circle distance boosted, else he might never attack.
			if (robptr->attack_type != 1)
				circle_distance += (objnum&0xf) * F1_0/2;

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			//	@mk, 12/27/94, structure here was strange.  Would do both clauses of what are now this if/then/else.  Used to be if/then, if/then.
			if ((player_visibility < 2) && (previous_visibility == 2)) { // this is redundant: mk, 01/15/95: && (ailp->mode == AIM_CHASE_OBJECT)) {
				if (!ai_multiplayer_awareness(obj, 53)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				create_path_to_player(obj, 8, 1);
				// -- show_path_and_other(obj);
				ai_multi_send_robot_position(objnum, -1);
			} else if ((player_visibility == 0) && (dist_to_player > F1_0*80) && (!(Game_mode & GM_MULTI))) {
				//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
				//	This has one desirable benefit of avoiding physics retries.
				if (aip->behavior == AIB_STATION) {
					ailp->goal_segment = aip->hide_segment;
					create_path_to_station(obj, 15);
					// -- show_path_and_other(obj);
				} else
					create_n_segment_path(obj, 5, -1);
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
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				create_path_to_player(obj, 10, 1);
				// -- show_path_and_other(obj);
				ai_multi_send_robot_position(objnum, -1);
			} else if ((aip->CURRENT_STATE != AIS_REST) && (aip->GOAL_STATE != AIS_REST)) {
				if (!ai_multiplayer_awareness(obj, 70)) {
					if (maybe_ai_do_actual_firing_stuff(obj, aip))
						ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
					return;
				}
				ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, circle_distance, 0);

				if ((obj_ref & 1) && ((aip->GOAL_STATE == AIS_SRCH) || (aip->GOAL_STATE == AIS_LOCK))) {
					if (player_visibility) // == 2)
						ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					else
						ai_turn_randomly(&vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
				}

				if (ai_evaded) {
					ai_multi_send_robot_position(objnum, 1);
					ai_evaded = 0;
				}
				else 
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

			//	If in multiplayer, only do if player visible.  If not multiplayer, do always.
			if (!(Game_mode & GM_MULTI) || player_visibility)
				if (ai_multiplayer_awareness(obj, 75)) {
					ai_follow_path(obj, player_visibility);
					ai_multi_send_robot_position(objnum, -1);
				}

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			//	Bad to let run_from robot fire at player because it will cause a war in which it turns towards the
			//	player to fire and then towards its goal to move.
			// do_firing_stuff(obj, player_visibility, &vec_to_player);
			//	Instead, do this:
			//	(Note, only drop if player is visible.  This prevents the bombs from being a giveaway, and
			//	also ensures that the robot is moving while it is dropping.  Also means fewer will be dropped.)
			if ((ailp->next_fire <= 0) && (player_visibility)) {
				vms_vector	fire_vec, fire_pos;

				if (!ai_multiplayer_awareness(obj, 75))
					return;

				fire_vec = obj->orient.fvec;
				vm_vec_negate(&fire_vec);
				vm_vec_add(&fire_pos, &obj->pos, &fire_vec);

				Laser_create_new_easy( &fire_vec, &fire_pos, obj-Objects, PROXIMITY_ID, 1);
				ailp->next_fire = F1_0*5;		//	Drop a proximity bomb every 5 seconds.
				
				#ifdef NETWORK
				#ifndef SHAREWARE
				if (Game_mode & GM_MULTI)
				{
					ai_multi_send_robot_position(obj-Objects, -1);
					multi_send_robot_fire(obj-Objects, -1, &fire_vec);
				}
				#endif
				#endif	
			}
			break;

		case AIM_FOLLOW_PATH: {
			int	anger_level = 65;

			if (aip->behavior == AIB_STATION)
				if (Point_segs[aip->hide_index + aip->path_length - 1].segnum == aip->hide_segment) {
					anger_level = 64;
				}

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

			if (!ai_multiplayer_awareness(obj, anger_level)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
				}
				return;
			}

			ai_follow_path(obj, player_visibility);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			if ((aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM))
				do_firing_stuff(obj, player_visibility, &vec_to_player);

			if ((player_visibility == 2) && (aip->behavior != AIB_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM) && (obj->id != ROBOT_BRAIN)) {
				if (robptr->attack_type == 0)
					ailp->mode = AIM_CHASE_OBJECT;
			} else if ((player_visibility == 0) && (aip->behavior == AIB_NORMAL) && (ailp->mode == AIM_FOLLOW_PATH) && (aip->behavior != AIB_RUN_FROM)) {
				ailp->mode = AIM_STILL;
				aip->hide_index = -1;
				aip->path_length = 0;
			}

			ai_multi_send_robot_position(objnum, -1);

			break;
		}

		case AIM_HIDE:
			if (!ai_multiplayer_awareness(obj, 71)) {
				if (maybe_ai_do_actual_firing_stuff(obj, aip)) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
				}
				return;
			}

			compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

 			ai_follow_path(obj, player_visibility);

			if (aip->GOAL_STATE != AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;
			else if (aip->CURRENT_STATE == AIS_FLIN)
				aip->GOAL_STATE = AIS_LOCK;

			ai_multi_send_robot_position(objnum, -1);
			break;

		case AIM_STILL:
			if ((dist_to_player < F1_0*120+Difficulty_level*F1_0*20) || (ailp->player_awareness_type >= PA_WEAPON_ROBOT_COLLISION-1)) {
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				//	turn towards vector if visible this time or last time, or rand
				// new!
                                if ((player_visibility) || (previous_visibility) || ((d_rand() > 0x4000) && !(Game_mode & GM_MULTI))) {
					if (!ai_multiplayer_awareness(obj, 71)) {
						if (maybe_ai_do_actual_firing_stuff(obj, aip))
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
						return;
					}
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				}

				do_firing_stuff(obj, player_visibility, &vec_to_player);
				//	This is debugging code!  Remove it!  It's to make the green guy attack without doing other kinds of movement.
				if (player_visibility) {		//	Change, MK, 01/03/94 for Multiplayer reasons.  If robots can't see you (even with eyes on back of head), then don't do evasion.
					if (robptr->attack_type == 1) {
						aip->behavior = AIB_NORMAL;
						if (!ai_multiplayer_awareness(obj, 80)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 0);
						if (ai_evaded) {
							ai_multi_send_robot_position(objnum, 1);
							ai_evaded = 0;
						}
						else
							ai_multi_send_robot_position(objnum, -1);
					} else {
						//	Robots in hover mode are allowed to evade at half normal speed.
						if (!ai_multiplayer_awareness(obj, 81)) {
							if (maybe_ai_do_actual_firing_stuff(obj, aip))
								ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
						ai_move_relative_to_player(obj, ailp, dist_to_player, &vec_to_player, 0, 1);
						if (ai_evaded) {
							ai_multi_send_robot_position(objnum, -1);
							ai_evaded = 0;
						}
						else				
							ai_multi_send_robot_position(objnum, -1);
					}
				} else if ((obj->segnum != aip->hide_segment) && (dist_to_player > F1_0*80) && (!(Game_mode & GM_MULTI))) {
					//	If pretty far from the player, player cannot be seen (obstructed) and in chase mode, switch to follow path mode.
					//	This has one desirable benefit of avoiding physics retries.
					if (aip->behavior == AIB_STATION) {
						ailp->goal_segment = aip->hide_segment;
						create_path_to_station(obj, 15);
						// -- show_path_and_other(obj);
					}
					break;
				}
			}

			break;
		case AIM_OPEN_DOOR: {		// trying to open a door.
			vms_vector	center_point, goal_vector;
			Assert(obj->id == ROBOT_BRAIN);		//	Make sure this guy is allowed to be in this mode.

			if (!ai_multiplayer_awareness(obj, 62))
				return;
			compute_center_point_on_side(&center_point, &Segments[obj->segnum], aip->GOALSIDE);
			vm_vec_sub(&goal_vector, &center_point, &obj->pos);
			vm_vec_normalize_quick(&goal_vector);
			ai_turn_towards_vector(&goal_vector, obj, robptr->turn_time[Difficulty_level]);
			move_towards_vector(obj, &goal_vector);
			ai_multi_send_robot_position(objnum, -1);

			break;
		}

		default:
			ailp->mode = AIM_CHASE_OBJECT;
			break;
	}		// end:	switch (ailp->mode) {

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If the robot can see you, increase his awareness of you.
	//	This prevents the problem of a robot looking right at you but doing nothing.
	// Assert(player_visibility != -1);	//	Means it didn't get initialized!
	compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
	if (player_visibility == 2)
		if (ailp->player_awareness_type == 0)
			ailp->player_awareness_type = PA_PLAYER_COLLISION;

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (!object_animates) {
		aip->CURRENT_STATE = aip->GOAL_STATE;
	}

	Assert(ailp->player_awareness_type <= AIE_MAX);
	Assert(aip->CURRENT_STATE < AIS_MAX);
	Assert(aip->GOAL_STATE < AIS_MAX);

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	if (ailp->player_awareness_type) {
		new_goal_state = Ai_transition_table[ailp->player_awareness_type-1][aip->CURRENT_STATE][aip->GOAL_STATE];
		if (ailp->player_awareness_type == PA_WEAPON_ROBOT_COLLISION) {
			//	Decrease awareness, else this robot will flinch every frame.
			ailp->player_awareness_type--;
			ailp->player_awareness_time = F1_0*3;
		}

		if (new_goal_state == AIS_ERR_)
			new_goal_state = AIS_REST;

		if (aip->CURRENT_STATE == AIS_NONE)
			aip->CURRENT_STATE = AIS_REST;

		aip->GOAL_STATE = new_goal_state;

	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	If new state = fire, then set all gun states to fire.
	if ((aip->GOAL_STATE == AIS_FIRE) ) {
		int	i,num_guns;
		num_guns = Robot_info[obj->id].n_guns;
		for (i=0; i<num_guns; i++)
			ailp->goal_state[i] = AIS_FIRE;
	}

	//	- -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -  - -
	//	Hack by mk on 01/04/94, if a guy hasn't animated to the firing state, but his next_fire says ok to fire, bash him there
	if ((ailp->next_fire < 0) && (aip->GOAL_STATE == AIS_FIRE))
		aip->CURRENT_STATE = AIS_FIRE;

	if ((aip->GOAL_STATE != AIS_FLIN)  && (obj->id != ROBOT_BRAIN)) {
		switch (aip->CURRENT_STATE) {
			case	AIS_NONE:
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				dot = vm_vec_dot(&obj->orient.fvec, &vec_to_player);
				if (dot >= F1_0/2)
					if (aip->GOAL_STATE == AIS_REST)
						aip->GOAL_STATE = AIS_SRCH;
				break;
			case	AIS_REST:
				if (aip->GOAL_STATE == AIS_REST) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if ((ailp->next_fire <= 0) && (player_visibility)) {
						aip->GOAL_STATE = AIS_FIRE;
					}
				}
				break;
			case	AIS_SRCH:
				if (!ai_multiplayer_awareness(obj, 60))
					return;

				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				if (player_visibility) {
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				} else if (!(Game_mode & GM_MULTI))
					ai_turn_randomly(&vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
				break;
			case	AIS_LOCK:
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				if (!(Game_mode & GM_MULTI) || (player_visibility)) {
					if (!ai_multiplayer_awareness(obj, 68))
						return;

					if (player_visibility) {
						ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
						ai_multi_send_robot_position(objnum, -1);
					} else if (!(Game_mode & GM_MULTI))
						ai_turn_randomly(&vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
				}
				break;
			case	AIS_FIRE:
				compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);

				if (player_visibility) {
					if (!ai_multiplayer_awareness(obj, (ROBOT_FIRE_AGITATION-1))) 
					{
						if (Game_mode & GM_MULTI) {
							ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);
							return;
						}
					}
					ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
					ai_multi_send_robot_position(objnum, -1);
				} else if (!(Game_mode & GM_MULTI)) {
					ai_turn_randomly(&vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
				}

				//	Fire at player, if appropriate.
				ai_do_actual_firing_stuff(obj, aip, ailp, robptr, &vec_to_player, dist_to_player, &gun_point, player_visibility, object_animates);

				break;
			case	AIS_RECO:
				if (!(obj_ref & 3)) {
					compute_vis_and_vec(obj, &vis_vec_pos, ailp, &vec_to_player, &player_visibility, robptr, &visibility_and_vec_computed);
					if (player_visibility) {
						if (!ai_multiplayer_awareness(obj, 69))
							return;
						ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);
						ai_multi_send_robot_position(objnum, -1);
					} else if (!(Game_mode & GM_MULTI)) {
						ai_turn_randomly(&vec_to_player, obj, robptr->turn_time[Difficulty_level], previous_visibility);
					}
				}
				break;
			case	AIS_FLIN:
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
			aip->CURRENT_GUN = 0;
	}

}

//--mk, 121094 -- // ----------------------------------------------------------------------------------
//--mk, 121094 -- void spin_robot(object *robot, vms_vector *collision_point)
//--mk, 121094 -- {
//--mk, 121094 -- 	if (collision_point->x != 3) {
//--mk, 121094 -- 		robot->phys_info.rotvel.x = 0x1235;
//--mk, 121094 -- 		robot->phys_info.rotvel.y = 0x2336;
//--mk, 121094 -- 		robot->phys_info.rotvel.z = 0x3737;
//--mk, 121094 -- 	}
//--mk, 121094 -- 
//--mk, 121094 -- }

//	-----------------------------------------------------------------------------------
void ai_do_cloak_stuff(void)
{
	int	i;

	for (i=0; i<MAX_AI_CLOAK_INFO; i++) {
		Ai_cloak_info[i].last_position = ConsoleObject->pos;
		Ai_cloak_info[i].last_time = GameTime64;
	}

	//	Make work for control centers.
	Believed_player_pos = Ai_cloak_info[0].last_position;

}

//	-----------------------------------------------------------------------------------
//	Returns false if awareness is considered too puny to add, else returns true.
int add_awareness_event(object *objp, int type)
{
	//	If player cloaked and hit a robot, then increase awareness
	if ((type == PA_WEAPON_ROBOT_COLLISION) || (type == PA_WEAPON_WALL_COLLISION) || (type == PA_PLAYER_COLLISION))
		ai_do_cloak_stuff();

	if (Num_awareness_events < MAX_AWARENESS_EVENTS) {
		if ((type == PA_WEAPON_WALL_COLLISION) || (type == PA_WEAPON_ROBOT_COLLISION))
			if (objp->id == VULCAN_ID)
                                if (d_rand() > 3276)
					return 0;		//	For vulcan cannon, only about 1/10 actually cause awareness

		Awareness_events[Num_awareness_events].segnum = objp->segnum;
		Awareness_events[Num_awareness_events].pos = objp->pos;
		Awareness_events[Num_awareness_events].type = type;
		Num_awareness_events++;
	} else
		Assert(0);		// Hey -- Overflowed Awareness_events, make more or something
							// This just gets ignored, so you can just continue.
	return 1;

}

// ----------------------------------------------------------------------------------
//	Robots will become aware of the player based on something that occurred.
//	The object (probably player or weapon) which created the awareness is objp.
void create_awareness_event(object *objp, int type)
{
	if (add_awareness_event(objp, type)) {
                if (((d_rand() * (type+4)) >> 15) > 4)
			Overall_agitation++;
		if (Overall_agitation > OVERALL_AGITATION_MAX)
			Overall_agitation = OVERALL_AGITATION_MAX;
	}
}

sbyte	New_awareness[MAX_SEGMENTS];

// ----------------------------------------------------------------------------------
void pae_aux(int segnum, int type, int level)
{
	int	j;

	if (New_awareness[segnum] < type)
		New_awareness[segnum] = type;

	// Process children.
	if (level <= 4)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (IS_CHILD(Segments[segnum].children[j]))
                         {
				if (type == 4)
					pae_aux(Segments[segnum].children[j], type-1, level+1);
				else
					pae_aux(Segments[segnum].children[j], type, level+1);
                         }
}

// ----------------------------------------------------------------------------------
void process_awareness_events(void)
{
	int	i;

	for (i=0; i<=Highest_segment_index; i++)
		New_awareness[i] = 0;

	for (i=0; i<Num_awareness_events; i++)
		pae_aux(Awareness_events[i].segnum, Awareness_events[i].type, 1);

	Num_awareness_events = 0;
}

// ----------------------------------------------------------------------------------
void set_player_awareness_all(void)
{
	int	i;

	process_awareness_events();

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].control_type == CT_AI)
			if (New_awareness[Objects[i].segnum] > Ai_local_info[i].player_awareness_type) {
				Ai_local_info[i].player_awareness_type = New_awareness[Objects[i].segnum];
				Ai_local_info[i].player_awareness_time = PLAYER_AWARENESS_INITIAL_TIME;
			}
}

#ifndef NDEBUG
int	Ai_dump_enable = 0;

FILE *Ai_dump_file = NULL;

char	Ai_error_message[128] = "";

// ----------------------------------------------------------------------------------
void dump_ai_objects_all()
{
#if PARALLAX
	int	objnum;
	int	total=0;
	time_t	time_of_day;

	time_of_day = time(NULL);

	if (!Ai_dump_enable)
		return;

	if (Ai_dump_file == NULL)
		Ai_dump_file = fopen("ai.out","a+t");

	fprintf(Ai_dump_file, "\nnum: seg distance __mode__ behav.    [velx vely velz] (Frame = %i)\n", FrameCount);
	fprintf(Ai_dump_file, "Date & Time = %s\n", ctime(&time_of_day));

	if (Ai_error_message[0])
		fprintf(Ai_dump_file, "Error message: %s\n", Ai_error_message);

	for (objnum=0; objnum <= Highest_object_index; objnum++) {
		object		*objp = &Objects[objnum];
		ai_static	*aip = &objp->ctype.ai_info;
		ai_local		*ailp = &Ai_local_info[objnum];
		fix			dist_to_player;

		dist_to_player = vm_vec_dist(&objp->pos, &ConsoleObject->pos);

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

// ----------------------------------------------------------------------------------
void force_dump_ai_objects_all(char *msg)
{
	int	tsave;

	tsave = Ai_dump_enable;

	Ai_dump_enable = 1;

	sprintf(Ai_error_message, "%s\n", msg);
	dump_ai_objects_all();
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

// ----------------------------------------------------------------------------------
//	Do things which need to get done for all AI objects each frame.
//	This includes:
//		Setting player_awareness (a fix, time in seconds which object is aware of player)
void do_ai_frame_all(void)
{
#ifndef NDEBUG
	dump_ai_objects_all();
#endif

	set_player_awareness_all();
}

//	Initializations to be performed for all robots for a new level.
void init_robots_for_level(void)
{
	Overall_agitation = 0;
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
	ail_rw->last_see_time              = ail->last_see_time;
	ail_rw->last_attack_time           = ail->last_attack_time;
	ail_rw->wait_time                  = ail->wait_time;
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

void state_ai_cloak_info_to_ai_cloak_info_rw(ai_cloak_info *aic, ai_cloak_info_rw *aic_rw)
{
	if (aic->last_time - GameTime64 < F1_0*(-18000))
		aic_rw->last_time = F1_0*(-18000);
	else
		aic_rw->last_time = aic->last_time - GameTime64;
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
		MALLOC(ail_rw, ai_local_rw, 1);
		memset(ail_rw, 0, sizeof(ai_local_rw));
		state_ai_local_to_ai_local_rw(&Ai_local_info[i], ail_rw);
		PHYSFS_write(fp, ail_rw, sizeof(ai_local_rw), 1);
		d_free(ail_rw);
	}
	PHYSFS_write(fp, Point_segs, sizeof(point_seg) * MAX_POINT_SEGS, 1);
	//PHYSFS_write(fp, Ai_cloak_info, sizeof(ai_cloak_info) * MAX_AI_CLOAK_INFO, 1);
	for (i = 0; i < MAX_AI_CLOAK_INFO; i++)
	{
		ai_cloak_info_rw *aic_rw;
		MALLOC(aic_rw, ai_cloak_info_rw, 1);
		memset(aic_rw, 0, sizeof(ai_cloak_info_rw));
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
	PHYSFS_write(fp, &Boss_hit_this_frame, sizeof(int), 1);
	PHYSFS_write(fp, &Boss_been_hit, sizeof(int), 1);

	return 1;
}

void ai_local_read_n_swap(ai_local *ail, int n, int swap, PHYSFS_file *fp)
{
	int i;
	
	for (i = 0; i < n; i++, ail++)
	{
		int j;
		fix tmptime32 = 0;

		ail->player_awareness_type = PHYSFSX_readByte(fp);
		ail->retry_count = PHYSFSX_readByte(fp);
		ail->consecutive_retries = PHYSFSX_readByte(fp);
		ail->mode = PHYSFSX_readByte(fp);
		ail->previous_visibility = PHYSFSX_readByte(fp);
		ail->rapidfire_count = PHYSFSX_readByte(fp);
		ail->goal_segment = PHYSFSX_readSXE16(fp, swap);
		ail->last_see_time = PHYSFSX_readSXE32(fp, swap);
		ail->last_attack_time = PHYSFSX_readSXE32(fp, swap);
		ail->wait_time = PHYSFSX_readSXE32(fp, swap);
		ail->next_fire = PHYSFSX_readSXE32(fp, swap);
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
		PHYSFSX_readVectorX(fp, &ci->last_position, swap);
	}
}

int ai_restore_state(PHYSFS_file *fp, int swap)
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
	Boss_hit_this_frame = PHYSFSX_readSXE32(fp, swap);
	Boss_been_hit = PHYSFSX_readSXE32(fp, swap);

	return 1;
}
