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
 * Code for the control center
 *
 */


#include <stdlib.h>
#include <stdio.h>
#if !defined(_WIN32) && !defined(macintosh)
#include <unistd.h>
#endif
#include "pstypes.h"
#include "dxxerror.h"
#include "inferno.h"
#include "cntrlcen.h"
#include "game.h"
#include "laser.h"
#include "gameseq.h"
#include "ai.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "wall.h"
#include "object.h"
#include "robot.h"
#include "vclip.h"
#include "fireball.h"
#include "endlevel.h"
#include "state.h"
#include "byteswap.h"
#include "args.h"

//@@vms_vector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@vms_vector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];

reactor Reactors[MAX_REACTORS];
int Num_reactors=0;

control_center_triggers ControlCenterTriggers;

int	Control_center_been_hit;
int	Control_center_player_been_seen;
int	Control_center_next_fire_time;
int	Control_center_present;

void do_countdown_frame();

//	-----------------------------------------------------------------------------
//return the position & orientation of a gun on the control center object
void calc_controlcen_gun_point(reactor *reactor, object *obj,int gun_num)
{
	vms_matrix m;
	vms_vector *gun_point = &obj->ctype.reactor_info.gun_pos[gun_num];
	vms_vector *gun_dir = &obj->ctype.reactor_info.gun_dir[gun_num];

	Assert(obj->type == OBJ_CNTRLCEN);
	Assert(obj->render_type==RT_POLYOBJ);

	Assert(gun_num < reactor->n_guns);

	//instance gun position & orientation

	vm_copy_transpose_matrix(&m,&obj->orient);

	vm_vec_rotate(gun_point,&reactor->gun_points[gun_num],&m);
	vm_vec_add2(gun_point,&obj->pos);
	vm_vec_rotate(gun_dir,&reactor->gun_dirs[gun_num],&m);
}

//	-----------------------------------------------------------------------------
//	Look at control center guns, find best one to fire at *objp.
//	Return best gun number (one whose direction dotted with vector to player is largest).
//	If best gun has negative dot, return -1, meaning no gun is good.
int calc_best_gun(int num_guns, const object *objreactor, const vms_vector *objpos)
{
	int	i;
	fix	best_dot;
	int	best_gun;
	const vms_vector (*const gun_pos)[MAX_CONTROLCEN_GUNS] = &objreactor->ctype.reactor_info.gun_pos;
	const vms_vector (*const gun_dir)[MAX_CONTROLCEN_GUNS] = &objreactor->ctype.reactor_info.gun_dir;

	best_dot = -F1_0*2;
	best_gun = -1;

	for (i=0; i<num_guns; i++) {
		fix			dot;
		vms_vector	gun_vec;

		vm_vec_sub(&gun_vec, objpos, &((*gun_pos)[i]));
		vm_vec_normalize_quick(&gun_vec);
		dot = vm_vec_dot(&((*gun_dir)[i]), &gun_vec);

		if (dot > best_dot) {
			best_dot = dot;
			best_gun = i;
		}
	}

	Assert(best_gun != -1);		// Contact Mike.  This is impossible.  Or maybe you're getting an unnormalized vector somewhere.

	if (best_dot < 0)
		return -1;
	else
		return best_gun;

}

int	Dead_controlcen_object_num=-1;

//how long to blow up on insane
int Base_control_center_explosion_time=DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

int Control_center_destroyed = 0;
fix Countdown_timer=0;
int Countdown_seconds_left=0, Total_countdown_time=0;		//in whole seconds

static const int	Alan_pavlish_reactor_times[NDL] = {90, 60, 45, 35, 30};

//	-----------------------------------------------------------------------------
//	Called every frame.  If control center been destroyed, then actually do something.
void do_controlcen_dead_frame(void)
{
	if ((Game_mode & GM_MULTI) && (Players[Player_num].connected != CONNECT_PLAYING)) // if out of level already there's no need for this
		return;

	if ((Dead_controlcen_object_num != -1) && (Countdown_seconds_left > 0))
		if (d_rand() < FrameTime*4)
			create_small_fireball_on_object(&Objects[Dead_controlcen_object_num], F1_0, 1);

	if (Control_center_destroyed && !Endlevel_sequence)
		do_countdown_frame();
}

#define COUNTDOWN_VOICE_TIME fl2f(12.75)

void do_countdown_frame()
{
	fix	old_time;
	int	fc, div_scale;

	if (!Control_center_destroyed)	return;

	if (!is_D2_OEM && !is_MAC_SHARE && !is_SHAREWARE)   // get countdown in OEM and SHAREWARE only
	{
		// On last level, we don't want a countdown.
		if (PLAYING_BUILTIN_MISSION && Current_level_num == Last_level)
		{
			if (!(Game_mode & GM_MULTI))
				return;
			if (Game_mode & GM_MULTI_ROBOTS)
				return;
		}
	}

	//	Control center destroyed, rock the player's ship.
	fc = Countdown_seconds_left;
	if (fc > 16)
		fc = 16;

	//	At Trainee, decrease rocking of ship by 4x.
	div_scale = 1;
	if (Difficulty_level == 0)
		div_scale = 4;

	if (d_tick_step)
	{
		ConsoleObject->mtype.phys_info.rotvel.x += (fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32))/div_scale;
		ConsoleObject->mtype.phys_info.rotvel.z += (fixmul(d_rand() - 16384, 3*F1_0/16 + (F1_0*(16-fc))/32))/div_scale;
	}
	//	Hook in the rumble sound effect here.

	old_time = Countdown_timer;
	Countdown_timer -= FrameTime;
	Countdown_seconds_left = f2i(Countdown_timer + F1_0*7/8);

	if ( (old_time > COUNTDOWN_VOICE_TIME ) && (Countdown_timer <= COUNTDOWN_VOICE_TIME) )	{
		digi_play_sample( SOUND_COUNTDOWN_13_SECS, F3_0 );
	}
	if ( f2i(old_time + F1_0*7/8) != Countdown_seconds_left )	{
		if ( (Countdown_seconds_left>=0) && (Countdown_seconds_left<10) )
			digi_play_sample( SOUND_COUNTDOWN_0_SECS+Countdown_seconds_left, F3_0 );
		if ( Countdown_seconds_left==Total_countdown_time-1)
			digi_play_sample( SOUND_COUNTDOWN_29_SECS, F3_0 );
	}						

	if (Countdown_timer > 0) {
		fix size,old_size;
		size = (i2f(Total_countdown_time)-Countdown_timer) / fl2f(0.65);
		old_size = (i2f(Total_countdown_time)-old_time) / fl2f(0.65);
		if (size != old_size && (Countdown_seconds_left < (Total_countdown_time-5) ))		{			// Every 2 seconds!
			//@@if (Dead_controlcen_object_num != -1) {
			//@@	vms_vector vp;	//,v,c;
			//@@	compute_segment_center(&vp, &Segments[Objects[Dead_controlcen_object_num].segnum]);
			//@@	object_create_explosion( Objects[Dead_controlcen_object_num].segnum, &vp, size*10, VCLIP_SMALL_EXPLOSION);
			//@@}

			digi_play_sample( SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0 );
		}
	}  else {
		int flash_value;

		if (old_time > 0)
			digi_play_sample( SOUND_MINE_BLEW_UP, F1_0 );

		flash_value = f2i(-Countdown_timer * (64 / 4));	// 4 seconds to total whiteness
		PALETTE_FLASH_SET(flash_value,flash_value,flash_value);

		if (PaletteBlueAdd > 64 )	{
			gr_set_current_canvas( NULL );
			gr_clear_canvas(BM_XRGB(31,31,31));				//make screen all white to match palette effect
			reset_palette_add();							//restore palette for death message
			//controlcen->MaxCapacity = Fuelcen_max_amount;
			//gauge_message( "Control Center Reset" );
			DoPlayerDead();		//kill_player();
		}																				
	}
}

//	-----------------------------------------------------------------------------
//	Called when control center gets destroyed.
//	This code is common to whether control center is implicitly imbedded in a boss,
//	or is an object of its own.
//	if objp == NULL that means the boss was the control center and don't set Dead_controlcen_object_num
void do_controlcen_destroyed_stuff(object *objp)
{
	int i;

	if ((Game_mode & GM_MULTI_ROBOTS) && Control_center_destroyed)
		return; // Don't allow resetting if control center and boss on same level

	// Must toggle walls whether it is a boss or control center.
	for (i=0;i<ControlCenterTriggers.num_links;i++)
		wall_toggle(ControlCenterTriggers.seg[i], ControlCenterTriggers.side[i]);

	// And start the countdown stuff.
	Control_center_destroyed = 1;

	// If a secret level, delete secret.sgc to indicate that we can't return to our secret level.
	if (Current_level_num < 0)
		PHYSFS_delete(SECRETC_FILENAME);

	if (Base_control_center_explosion_time != DEFAULT_CONTROL_CENTER_EXPLOSION_TIME)
		Total_countdown_time = Base_control_center_explosion_time + Base_control_center_explosion_time * (NDL-Difficulty_level-1)/2;
	else
		Total_countdown_time = Alan_pavlish_reactor_times[Difficulty_level];

	Countdown_timer = i2f(Total_countdown_time);

	if (!Control_center_present || objp==NULL)
		return;

	Dead_controlcen_object_num = objp-Objects;
}

fix64	Last_time_cc_vis_check = 0;

//	-----------------------------------------------------------------------------
//do whatever this thing does in a frame
void do_controlcen_frame(object *obj)
{
	int			best_gun_num;
	static fix controlcen_death_silence = 0;

	//	If a boss level, then Control_center_present will be 0.
	if (!Control_center_present)
		return;

#ifndef NDEBUG
	if (cheats.robotfiringsuspended || (Game_suspended & SUSP_ROBOTS))
		return;
#else
	if (cheats.robotfiringsuspended)
		return;
#endif

	if (!(Control_center_been_hit || Control_center_player_been_seen)) {
		if (!(d_tick_count % 8)) {		//	Do every so often...
			vms_vector	vec_to_player;
			fix			dist_to_player;
			int			i;
			segment		*segp = &Segments[obj->segnum];

			// This is a hack.  Since the control center is not processed by
			// ai_do_frame, it doesn't know to deal with cloaked dudes.  It
			// seems to work in single-player mode because it is actually using
			// the value of Believed_player_position that was set by the last
			// person to go through ai_do_frame.  But since a no-robots game
			// never goes through ai_do_frame, I'm making it so the control
			// center can spot cloaked dudes.

			if (Game_mode & GM_MULTI)
				Believed_player_pos = Objects[Players[Player_num].objnum].pos;

			//	Hack for special control centers which are isolated and not reachable because the
			//	real control center is inside the boss.
			for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
				if (IS_CHILD(segp->children[i]))
					break;
			if (i == MAX_SIDES_PER_SEGMENT)
				return;

			vm_vec_sub(&vec_to_player, &ConsoleObject->pos, &obj->pos);
			dist_to_player = vm_vec_normalize_quick(&vec_to_player);
			if (dist_to_player < F1_0*200) {
				Control_center_player_been_seen = player_is_visible_from_object(obj, &obj->pos, 0, &vec_to_player);
				Control_center_next_fire_time = 0;
			}
		}			

		return;
	}

	//	Periodically, make the reactor fall asleep if player not visible.
	if (Control_center_been_hit || Control_center_player_been_seen) {
		if ((Last_time_cc_vis_check + F1_0*5 < GameTime64) || (Last_time_cc_vis_check > GameTime64)) {
			vms_vector	vec_to_player;
			fix			dist_to_player;

			vm_vec_sub(&vec_to_player, &ConsoleObject->pos, &obj->pos);
			dist_to_player = vm_vec_normalize_quick(&vec_to_player);
			Last_time_cc_vis_check = GameTime64;
			if (dist_to_player < F1_0*120) {
				Control_center_player_been_seen = player_is_visible_from_object(obj, &obj->pos, 0, &vec_to_player);
				if (!Control_center_player_been_seen)
					Control_center_been_hit = 0;
			}
		}

	}

	if (Player_is_dead)
		controlcen_death_silence += FrameTime;
	else
		controlcen_death_silence = 0;

	if ((Control_center_next_fire_time < 0) && !(controlcen_death_silence > F1_0*2)) {
		reactor *reactor = get_reactor_definition(obj->id);
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
			best_gun_num = calc_best_gun(reactor->n_guns, obj, &Believed_player_pos);
		else
			best_gun_num = calc_best_gun(reactor->n_guns, obj, &ConsoleObject->pos);

		if (best_gun_num != -1) {
			int			rand_prob, count;
			vms_vector	vec_to_goal;
			fix			dist_to_player;
			fix			delta_fire_time;

			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				vm_vec_sub(&vec_to_goal, &Believed_player_pos, &obj->ctype.reactor_info.gun_pos[best_gun_num]);
				dist_to_player = vm_vec_normalize_quick(&vec_to_goal);
			} else {
				vm_vec_sub(&vec_to_goal, &ConsoleObject->pos, &obj->ctype.reactor_info.gun_pos[best_gun_num]);
				dist_to_player = vm_vec_normalize_quick(&vec_to_goal);
			}

			if (dist_to_player > F1_0*300)
			{
				Control_center_been_hit = 0;
				Control_center_player_been_seen = 0;
				return;
			}
	
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_controlcen_fire(&vec_to_goal, best_gun_num, obj-Objects);	
			#endif
			Laser_create_new_easy( &vec_to_goal, &obj->ctype.reactor_info.gun_pos[best_gun_num], obj-Objects, CONTROLCEN_WEAPON_NUM, 1);

			//	some of time, based on level, fire another thing, not directly at player, so it might hit him if he's constantly moving.
			rand_prob = F1_0/(abs(Current_level_num)/4+2);
			count = 0;
			while ((d_rand() > rand_prob) && (count < 4)) {
				vms_vector	randvec;

				make_random_vector(&randvec);
				vm_vec_scale_add2(&vec_to_goal, &randvec, F1_0/6);
				vm_vec_normalize_quick(&vec_to_goal);
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_controlcen_fire(&vec_to_goal, best_gun_num, obj-Objects);
				#endif
				Laser_create_new_easy( &vec_to_goal, &obj->ctype.reactor_info.gun_pos[best_gun_num], obj-Objects, CONTROLCEN_WEAPON_NUM, 0);
				count++;
			}

			delta_fire_time = (NDL - Difficulty_level) * F1_0/4;
			if (Difficulty_level == 0)
				delta_fire_time += F1_0/2;

			if (Game_mode & GM_MULTI) // slow down rate of fire in multi player
				delta_fire_time *= 2;

			Control_center_next_fire_time = delta_fire_time;

		}
	} else
		Control_center_next_fire_time -= FrameTime;

}

int Reactor_strength=-1;		//-1 mean not set by designer

//	-----------------------------------------------------------------------------
//	This must be called at the start of each level.
//	If this level contains a boss and mode != multiplayer, don't do control center stuff.  (Ghost out control center object.)
//	If this level contains a boss and mode == multiplayer, do control center stuff.
void init_controlcen_for_level(void)
{
	int		i;
	object	*objp;
	int		cntrlcen_objnum=-1, boss_objnum=-1;

	for (i=0; i<=Highest_object_index; i++) {
		objp = &Objects[i];
		if (objp->type == OBJ_CNTRLCEN)
		{
			if (cntrlcen_objnum != -1)
				;
			else
				cntrlcen_objnum = i;
		}

		if ((objp->type == OBJ_ROBOT) && (Robot_info[objp->id].boss_flag)) {
			if (boss_objnum != -1)
				;
			else
				boss_objnum = i;
		}
	}

#ifndef NDEBUG
	if (cntrlcen_objnum == -1) {
		Dead_controlcen_object_num = -1;
		return;
	}
#endif

	if ( (boss_objnum != -1) && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_ROBOTS)) ) {
		if (cntrlcen_objnum != -1) {
			Objects[cntrlcen_objnum].type = OBJ_GHOST;
			Objects[cntrlcen_objnum].render_type = RT_NONE;
			Control_center_present = 0;
		}
	} else if (cntrlcen_objnum != -1) {
		//	Compute all gun positions.
		objp = &Objects[cntrlcen_objnum];
		reactor *reactor = get_reactor_definition(objp->id);
		for (i=0; i<reactor->n_guns; i++)
			calc_controlcen_gun_point(reactor, objp, i);
		Control_center_present = 1;

		if (Reactor_strength == -1) {		//use old defaults
			//	Boost control center strength at higher levels.
			if (Current_level_num >= 0)
				objp->shields = F1_0*200 + (F1_0*200/4) * Current_level_num;
			else
				objp->shields = F1_0*200 - Current_level_num*F1_0*150;
		}
		else {
			objp->shields = i2f(Reactor_strength);
		}

	}

	//	Say the control center has not yet been hit.
	Control_center_been_hit = 0;
	Control_center_player_been_seen = 0;
	Control_center_next_fire_time = 0;
	
	Dead_controlcen_object_num = -1;
}

void special_reactor_stuff(void)
{
	if (Control_center_destroyed) {
		Countdown_timer += i2f(Base_control_center_explosion_time + (NDL-1-Difficulty_level)*Base_control_center_explosion_time/(NDL-1));
		Total_countdown_time = f2i(Countdown_timer)+2;	//	Will prevent "Self destruct sequence activated" message from replaying.
	}
}

/*
 * reads n reactor structs from a PHYSFS_file
 */
extern int reactor_read_n(reactor *r, int n, PHYSFS_file *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		r[i].model_num = PHYSFSX_readInt(fp);
		r[i].n_guns = PHYSFSX_readInt(fp);
		for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
			PHYSFSX_readVector(&(r[i].gun_points[j]), fp);
		for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
			PHYSFSX_readVector(&(r[i].gun_dirs[j]), fp);
	}
	return i;
}

/*
 * reads a control_center_triggers structure from a PHYSFS_file
 */
extern int control_center_triggers_read_n(control_center_triggers *cct, int n, PHYSFS_file *fp)
{
	int i;

	for (i = 0; i < n; i++)
	{
		cct->num_links = PHYSFSX_readShort(fp);
		for (unsigned j = 0; j < MAX_CONTROLCEN_LINKS; j++)
			cct->seg[j] = PHYSFSX_readShort(fp);
		for (unsigned j = 0; j < MAX_CONTROLCEN_LINKS; j++)
			cct->side[j] = PHYSFSX_readShort(fp);
	}
	return i;
}

void control_center_triggers_swap(control_center_triggers *cct, int swap)
{
	int i;
	
	if (!swap)
		return;
	
	cct->num_links = SWAPSHORT(cct->num_links);
	for (i = 0; i < MAX_CONTROLCEN_LINKS; i++)
		cct->seg[i] = SWAPSHORT(cct->seg[i]);
	for (i = 0; i < MAX_CONTROLCEN_LINKS; i++)
		cct->side[i] = SWAPSHORT(cct->side[i]);
}

/*
 * reads n control_center_triggers structs from a PHYSFS_file and swaps if specified
 */
void control_center_triggers_read_n_swap(control_center_triggers *cct, int n, int swap, PHYSFS_file *fp)
{
	int i;
	
	PHYSFS_read(fp, cct, sizeof(control_center_triggers), n);
	
	if (swap)
		for (i = 0; i < n; i++)
			control_center_triggers_swap(&cct[i], swap);
}

int control_center_triggers_write(control_center_triggers *cct, PHYSFS_file *fp)
{
	int j;

	PHYSFS_writeSLE16(fp, cct->num_links);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		PHYSFS_writeSLE16(fp, cct->seg[j]);
	for (j = 0; j < MAX_CONTROLCEN_LINKS; j++)
		PHYSFS_writeSLE16(fp, cct->side[j]);

	return 1;
}
