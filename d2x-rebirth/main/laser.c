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
 * This will contain the laser code
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "bm.h"
#include "object.h"
#include "laser.h"
#include "args.h"
#include "segment.h"
#include "fvi.h"
#include "segpoint.h"
#include "dxxerror.h"
#include "key.h"
#include "texmap.h"
#include "textures.h"
#include "render.h"
#include "vclip.h"
#include "fireball.h"
#include "polyobj.h"
#include "robot.h"
#include "weapon.h"
#include "newdemo.h"
#include "timer.h"
#include "player.h"
#include "sounds.h"
#include "ai.h"
#include "powerup.h"
#include "multi.h"
#include "physics.h"
#include "multi.h"

#define NEWHOMER

object *Guided_missile[MAX_PLAYERS]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
int Guided_missile_sig[MAX_PLAYERS]={-1,-1,-1,-1,-1,-1,-1,-1};
int Network_laser_track = -1;

int find_homing_object_complete(vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2);

extern char Multi_is_guided;

extern void newdemo_record_guided_end();
extern void newdemo_record_guided_start();

int find_homing_object(vms_vector *curpos, object *tracker);

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// player and calls the appropriate routine.

void Laser_render(object *obj)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch( obj->id )	{
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPE_MISSILE:
		break;
	default:
		Error( "Invalid weapon type in Laser_render\n" );
	}
#endif

	switch( Weapon_info[obj->id].render_type )	{
	case WEAPON_RENDER_LASER:
		Int3();	// Not supported anymore!
					//Laser_draw_one(obj-Objects, Weapon_info[obj->id].bitmap );
		break;
	case WEAPON_RENDER_BLOB:
		draw_object_blob(obj, Weapon_info[obj->id].bitmap  );
		break;
	case WEAPON_RENDER_POLYMODEL:
		break;
	case WEAPON_RENDER_VCLIP:
		Int3();	//	Oops, not supported, type added by mk on 09/09/94, but not for lasers...
	default:
		Error( "Invalid weapon render type in Laser_render\n" );
	}

}

//---------------------------------------------------------------------------------
// Draws a texture-mapped laser bolt

//void Laser_draw_one( int objnum, grs_bitmap * bmp )
//{
//	int t1, t2, t3;
//	g3s_point p1, p2;
//	object *obj;
//	vms_vector start_pos,end_pos;
//
//	obj = &Objects[objnum];
//
//	start_pos = obj->pos;
//	vm_vec_scale_add(&end_pos,&start_pos,&obj->orient.fvec,-Laser_length);
//
//	g3_rotate_point(&p1,&start_pos);
//	g3_rotate_point(&p2,&end_pos);
//
//	t1 = Lighting_on;
//	t2 = Interpolation_method;
//	t3 = Transparency_on;
//
//	Lighting_on  = 0;
//	//Interpolation_method = 3;	// Full perspective
//	Interpolation_method = 1;	// Linear
//	Transparency_on = 1;
//
//	//gr_setcolor( gr_getcolor(31,15,0));
//	//g3_draw_line_ptrs(p1,p2);
//	//g3_draw_rod(p1,0x2000,p2,0x2000);
//	//g3_draw_rod(p1,Laser_width,p2,Laser_width);
//	g3_draw_rod_tmap(bmp,&p2,Laser_width,&p1,Laser_width,0);
//	Lighting_on = t1;
//	Interpolation_method = t2;
//	Transparency_on = t3;
//
//}

//	Changed by MK on 09/07/94
//	I want you to be able to blow up your own bombs.
//	AND...Your proximity bombs can blow you up if they're 2.0 seconds or more old.
//	Changed by MK on 06/06/95: Now must be 4.0 seconds old.  Much valid Net-complaining.
int laser_are_related( int o1, int o2 )
{
	if ( (o1<0) || (o2<0) )
		return 0;

	// See if o2 is the parent of o1
	if ( Objects[o1].type == OBJ_WEAPON  )
		if ( (Objects[o1].ctype.laser_info.parent_num==o2) && (Objects[o1].ctype.laser_info.parent_signature==Objects[o2].signature) )
		{
			//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
			if ((Objects[o1].id == PHOENIX_ID && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0/4)) ||
			   (Objects[o1].id == GUIDEDMISS_ID && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0*2)) ||
				(((Objects[o1].id == PROXIMITY_ID) || (Objects[o1].id == SUPERPROX_ID)) && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0*4))) {
				return 0;
			} else
				return 1;
		}

	// See if o1 is the parent of o2
	if ( Objects[o2].type == OBJ_WEAPON  )
	{
		if ( (Objects[o2].ctype.laser_info.parent_num==o1) && (Objects[o2].ctype.laser_info.parent_signature==Objects[o1].signature) )
		{
			//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is player, they are related only if o1 < 2.0 seconds old
			if ((Objects[o2].id == PHOENIX_ID && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0/4)) ||
			   (Objects[o2].id == GUIDEDMISS_ID && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0*2)) ||
				(((Objects[o2].id == PROXIMITY_ID) || (Objects[o2].id == SUPERPROX_ID)) && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0*4))) {
				return 0;
			} else
				return 1;
		}
	}

	// They must both be weapons
	if ( Objects[o1].type != OBJ_WEAPON || Objects[o2].type != OBJ_WEAPON )
		return 0;

	//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
	// See if they're siblings...
	//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
	if ( Objects[o1].ctype.laser_info.parent_signature==Objects[o2].ctype.laser_info.parent_signature )
	{
		if (is_proximity_bomb_or_smart_mine(Objects[o1].id) || is_proximity_bomb_or_smart_mine(Objects[o2].id)) {
			//	If neither is older than 1/2 second, then can't blow up!
			if ((GameTime64 > (Objects[o1].ctype.laser_info.creation_time + F1_0/2)) || (GameTime64 > (Objects[o2].ctype.laser_info.creation_time + F1_0/2)))
				return 0;
			else
				return 1;
		} else
			return 1;
	}

	//	Anything can cause a collision with a robot super prox mine.
	if (Objects[o1].id == ROBOT_SUPERPROX_ID || Objects[o2].id == ROBOT_SUPERPROX_ID ||
		 Objects[o1].id == PROXIMITY_ID || Objects[o2].id == PROXIMITY_ID ||
		 Objects[o1].id == SUPERPROX_ID || Objects[o2].id == SUPERPROX_ID ||
		 Objects[o1].id == PMINE_ID || Objects[o2].id == PMINE_ID)
		return 0;

	return 1;
}

void do_muzzle_stuff(int segnum, vms_vector *pos)
{
	Muzzle_data[Muzzle_queue_index].create_time = timer_query();
	Muzzle_data[Muzzle_queue_index].segnum = segnum;
	Muzzle_data[Muzzle_queue_index].pos = *pos;
	Muzzle_queue_index++;
	if (Muzzle_queue_index >= MUZZLE_QUEUE_MAX)
		Muzzle_queue_index = 0;
}

//creates a weapon object
int create_weapon_object(int weapon_type,int segnum,vms_vector *position)
{
	int rtype=-1;
	fix laser_radius = -1;
	int objnum;
	object *obj;

	switch( Weapon_info[weapon_type].render_type )	{

		case WEAPON_RENDER_BLOB:
			rtype = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
			laser_radius = Weapon_info[weapon_type].blob_size;
			break;
		case WEAPON_RENDER_POLYMODEL:
			laser_radius = 0;	//	Filled in below.
			rtype = RT_POLYOBJ;
			break;
		case WEAPON_RENDER_LASER:
			Int3(); 	// Not supported anymore
			break;
		case WEAPON_RENDER_NONE:
			rtype = RT_NONE;
			laser_radius = F1_0;
			break;
		case WEAPON_RENDER_VCLIP:
			rtype = RT_WEAPON_VCLIP;
			laser_radius = Weapon_info[weapon_type].blob_size;
			break;
		default:
			Error( "Invalid weapon render type in Laser_create_new\n" );
	}

	Assert(laser_radius != -1);
	Assert(rtype != -1);

	objnum = obj_create( OBJ_WEAPON, weapon_type, segnum, position, NULL, laser_radius, CT_WEAPON, MT_PHYSICS, rtype );
	if (objnum == -1)
		return -1;

	obj = &Objects[objnum];

	if (Weapon_info[weapon_type].render_type == WEAPON_RENDER_POLYMODEL) {
		obj->rtype.pobj_info.model_num = Weapon_info[obj->id].model_num;
		obj->size = fixdiv(Polygon_models[obj->rtype.pobj_info.model_num].rad,Weapon_info[obj->id].po_len_to_width_ratio);
	}

	obj->mtype.phys_info.mass = Weapon_info[weapon_type].mass;
	obj->mtype.phys_info.drag = Weapon_info[weapon_type].drag;
	vm_vec_zero(&obj->mtype.phys_info.thrust);

	if (Weapon_info[weapon_type].bounce==1)
		obj->mtype.phys_info.flags |= PF_BOUNCE;

	if (Weapon_info[weapon_type].bounce==2 || cheats.bouncyfire)
		obj->mtype.phys_info.flags |= PF_BOUNCE+PF_BOUNCES_TWICE;


	return objnum;
}

extern int Doing_lighting_hack_flag;

//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!! *****
//	Here are the constants you're looking for! --MK

//	Change the following constants to affect the look of the omega cannon.
//	Changing these constants will not affect the damage done.
//	WARNING: If you change DESIRED_OMEGA_DIST and MAX_OMEGA_BLOBS, you don't merely change the look of the cannon,
//	you change its range.  If you decrease DESIRED_OMEGA_DIST, you decrease how far the gun can fire.
#define OMEGA_BASE_TIME (F1_0/20) // How many blobs per second!! No FPS-based blob creation anymore, no FPS-based damage anymore!
#define	MIN_OMEGA_BLOBS		3				//	No matter how close the obstruction, at this many blobs created.
#define	MIN_OMEGA_DIST			(F1_0*3)		//	At least this distance between blobs, unless doing so would violate MIN_OMEGA_BLOBS
#define	DESIRED_OMEGA_DIST	(F1_0*5)		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			(MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	Additionally, several constants which apply to homing objects in general control the behavior of the Omega Cannon.
//	They are defined in laser.h.  They are copied here for reference.  These values are valid on 1/10/96:
//	If you want the Omega Cannon view cone to be different than the Homing Missile viewcone, contact MK to make the change.
//	(Unless you are a programmer, in which case, do it yourself!)
#define	OMEGA_MIN_TRACKABLE_DOT			(15*F1_0/16)		//	Larger values mean narrower cone.  F1_0 means damn near impossible.  0 means 180 degree field of view.
#define	OMEGA_MAX_TRACKABLE_DIST		MAX_OMEGA_DIST	//	An object must be at least this close to be tracked.

//	Note, you don't need to change these constants.  You can control damage and energy consumption by changing the
//	usual bitmaps.tbl parameters.
#define	OMEGA_DAMAGE_SCALE			32				//	Controls how much damage is done.  This gets multiplied by the damage specified in bitmaps.tbl in the $WEAPON line.
#define	OMEGA_ENERGY_CONSUMPTION	16				//	Controls how much energy is consumed.  This gets multiplied by the energy parameter from bitmaps.tbl.
//	-------------------------------------------------------------------------------------------------------------------------------

// Delete omega blobs further away than MAX_OMEGA_DIST
// Since last omega blob has VERY high velocity it's impossible to ensure a constant travel distance on varying FPS. So delete if they exceed their maximum distance.
int omega_cleanup(object *weapon)
{
	int parent_sig = weapon->ctype.laser_info.parent_signature, parent_num = weapon->ctype.laser_info.parent_num;

	if (weapon->type != OBJ_WEAPON || weapon->id != OMEGA_ID)
		return 0;

	if (Objects[parent_num].signature == parent_sig)
		if (vm_vec_dist(&weapon->pos, &Objects[parent_num].pos) > MAX_OMEGA_DIST)
		{
			obj_delete(weapon-Objects);
			return 1;
		}

	return 0;
}

// Return true if ok to do Omega damage. For Multiplayer games. See comment for omega_cleanup()
int ok_to_do_omega_damage(object *weapon)
{
	int parent_sig = weapon->ctype.laser_info.parent_signature, parent_num = weapon->ctype.laser_info.parent_num;

	if (weapon->type != OBJ_WEAPON || weapon->id != OMEGA_ID)
		return 1;
	if (!(Game_mode & GM_MULTI))
		return 1;

	if (Objects[parent_num].signature == parent_sig)
		if (vm_vec_dist(&Objects[parent_num].pos, &weapon->pos) > MAX_OMEGA_DIST)
			return 0;

	return 1;
}

// ---------------------------------------------------------------------------------
void create_omega_blobs(int firing_segnum, vms_vector *firing_pos, vms_vector *goal_pos, object *parent_objp)
{
	int		i = 0, last_segnum = 0, last_created_objnum = -1, num_omega_blobs = 0;
	vms_vector	vec_to_goal = ZERO_VECTOR, omega_delta_vector = ZERO_VECTOR, blob_pos = ZERO_VECTOR, perturb_vec = ZERO_VECTOR;
	fix		dist_to_goal = 0, omega_blob_dist = 0, perturb_array[MAX_OMEGA_BLOBS];

	memset(&perturb_array, 0, sizeof(fix)*MAX_OMEGA_BLOBS);

	vm_vec_sub(&vec_to_goal, goal_pos, firing_pos);
	dist_to_goal = vm_vec_normalize_quick(&vec_to_goal);

	if (dist_to_goal < MIN_OMEGA_BLOBS * MIN_OMEGA_DIST) {
		omega_blob_dist = MIN_OMEGA_DIST;
		num_omega_blobs = dist_to_goal/omega_blob_dist;
		if (num_omega_blobs == 0)
			num_omega_blobs = 1;
	} else {
		omega_blob_dist = DESIRED_OMEGA_DIST;
		num_omega_blobs = dist_to_goal / omega_blob_dist;
		if (num_omega_blobs > MAX_OMEGA_BLOBS) {
			num_omega_blobs = MAX_OMEGA_BLOBS;
			omega_blob_dist = dist_to_goal / num_omega_blobs;
		} else if (num_omega_blobs < MIN_OMEGA_BLOBS) {
			num_omega_blobs = MIN_OMEGA_BLOBS;
			omega_blob_dist = dist_to_goal / num_omega_blobs;
		}
	}

	omega_delta_vector = vec_to_goal;
	vm_vec_scale(&omega_delta_vector, omega_blob_dist);

	//	Now, create all the blobs
	blob_pos = *firing_pos;
	last_segnum = firing_segnum;

	//	If nearby, don't perturb vector.  If not nearby, start halfway out.
	if (dist_to_goal < MIN_OMEGA_DIST*4) {
		for (i=0; i<num_omega_blobs; i++)
			perturb_array[i] = 0;
	} else {
		vm_vec_scale_add2(&blob_pos, &omega_delta_vector, F1_0/2);	//	Put first blob half way out.
		for (i=0; i<num_omega_blobs/2; i++) {
			perturb_array[i] = F1_0*i + F1_0/4;
			perturb_array[num_omega_blobs-1-i] = F1_0*i;
		}
	}

	//	Create random perturbation vector, but favor _not_ going up in player's reference.
	make_random_vector(&perturb_vec);
	vm_vec_scale_add2(&perturb_vec, &parent_objp->orient.uvec, -F1_0/2);

	Doing_lighting_hack_flag = 1;	//	Ugly, but prevents blobs which are probably outside the mine from killing framerate.

	for (i=0; i<num_omega_blobs; i++) {
		vms_vector	temp_pos = ZERO_VECTOR;
		int		blob_objnum = -1, segnum = -1;

		//	This will put the last blob right at the destination object, causing damage.
		if (i == num_omega_blobs-1)
			vm_vec_scale_add2(&blob_pos, &omega_delta_vector, 15*F1_0/32);	//	Move last blob another (almost) half section

		//	Every so often, re-perturb blobs
		if ((i % 4) == 3) {
			vms_vector temp_vec = ZERO_VECTOR;

			make_random_vector(&temp_vec);
			vm_vec_scale_add2(&perturb_vec, &temp_vec, F1_0/4);
		}

		vm_vec_scale_add(&temp_pos, &blob_pos, &perturb_vec, perturb_array[i]);

		segnum = find_point_seg(&temp_pos, last_segnum);
		if (segnum != -1) {
			object *objp;

			last_segnum = segnum;
			blob_objnum = obj_create(OBJ_WEAPON, OMEGA_ID, segnum, &temp_pos, NULL, 0, CT_WEAPON, MT_PHYSICS, RT_WEAPON_VCLIP );
			if (blob_objnum == -1)
				break;

			last_created_objnum = blob_objnum;

			objp = &Objects[blob_objnum];

			objp->lifeleft = OMEGA_BASE_TIME+(d_rand()/8); // add little randomness so the lighting effect becomes a little more interesting
			objp->mtype.phys_info.velocity = vec_to_goal;

			//	Only make the last one move fast, else multiple blobs might collide with target.
			vm_vec_scale(&objp->mtype.phys_info.velocity, F1_0*4);

			objp->size = Weapon_info[objp->id].blob_size;

			objp->shields = fixmul(OMEGA_DAMAGE_SCALE*OMEGA_BASE_TIME, Weapon_info[objp->id].strength[Difficulty_level]);

			objp->ctype.laser_info.parent_type			= parent_objp->type;
			objp->ctype.laser_info.parent_signature	= parent_objp->signature;
			objp->ctype.laser_info.parent_num			= parent_objp-Objects;
			objp->movement_type = MT_NONE;	//	Only last one moves, that will get bashed below.

		}

		vm_vec_add2(&blob_pos, &omega_delta_vector);

	}

	//	Make last one move faster, but it's already moving at speed = F1_0*4.
	if (last_created_objnum != -1) {
		vm_vec_scale(&Objects[last_created_objnum].mtype.phys_info.velocity, Weapon_info[OMEGA_ID].speed[Difficulty_level]/4);
		Objects[last_created_objnum].movement_type = MT_PHYSICS;
	}

	Doing_lighting_hack_flag = 0;
}

#define	MIN_OMEGA_CHARGE	(MAX_OMEGA_CHARGE/8)
#define	OMEGA_CHARGE_SCALE	4			//	FrameTime / OMEGA_CHARGE_SCALE added to Omega_charge every frame.
fix	Omega_charge = MAX_OMEGA_CHARGE;

#define	OMEGA_CHARGE_SCALE	4

int	Last_omega_fire_time=0;

// ---------------------------------------------------------------------------------
//	Call this every frame to recharge the Omega Cannon.
void omega_charge_frame(void)
{
	fix	delta_charge, old_omega_charge;

	if (Omega_charge == MAX_OMEGA_CHARGE)
		return;

	if (!(player_has_weapon(OMEGA_INDEX, 0) & HAS_WEAPON_FLAG))
		return;

	if (Player_is_dead)
		return;

	//	Don't charge while firing. Wait 1/3 second after firing before recharging
	if (Last_omega_fire_time > GameTime64)
		Last_omega_fire_time = GameTime64;
	if (Last_omega_fire_time + F1_0/3 > GameTime64)
		return;

	if (Players[Player_num].energy) {
		fix	energy_used;

		old_omega_charge = Omega_charge;
		Omega_charge += FrameTime/OMEGA_CHARGE_SCALE;
		if (Omega_charge > MAX_OMEGA_CHARGE)
			Omega_charge = MAX_OMEGA_CHARGE;

		delta_charge = Omega_charge - old_omega_charge;

		energy_used = fixmul(F1_0*190/17, delta_charge);
		if (Difficulty_level < 2)
			energy_used = fixmul(energy_used, i2f(Difficulty_level+2)/4);

		Players[Player_num].energy -= energy_used;
		if (Players[Player_num].energy < 0)
			Players[Player_num].energy = 0;
	}


}

// -- fix	Last_omega_muzzle_flash_time;

// ---------------------------------------------------------------------------------
//	*objp is the object firing the omega cannon
//	*pos is the location from which the omega bolt starts
void do_omega_stuff(object *parent_objp, vms_vector *firing_pos, object *weapon_objp)
{
	int			lock_objnum, firing_segnum;
	vms_vector	goal_pos;
	int			pnum = parent_objp->id;
	fix fire_frame_overhead = 0;

	if (pnum == Player_num) {
		//	If charge >= min, or (some charge and zero energy), allow to fire.
		if (!((Omega_charge >= MIN_OMEGA_CHARGE) || (Omega_charge && !Players[pnum].energy))) {
			obj_delete(weapon_objp-Objects);
			return;
		}

		Omega_charge -= OMEGA_BASE_TIME;
		if (Omega_charge < 0)
			Omega_charge = 0;

		if (GameTime64 - Last_omega_fire_time + OMEGA_BASE_TIME <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that. Since Next_laser_firing_time is probably changed already (in do_laser_firing_player), we need to calculate the overhead slightly different. 
			fire_frame_overhead = GameTime64 - Last_omega_fire_time + OMEGA_BASE_TIME;

		Next_laser_fire_time = GameTime64+OMEGA_BASE_TIME-fire_frame_overhead;
		Last_omega_fire_time = GameTime64;
	}

	weapon_objp->ctype.laser_info.parent_type = OBJ_PLAYER;
	weapon_objp->ctype.laser_info.parent_num = Players[pnum].objnum;
	weapon_objp->ctype.laser_info.parent_signature = Objects[Players[pnum].objnum].signature;

	lock_objnum = find_homing_object(firing_pos, weapon_objp);

	firing_segnum = find_point_seg(firing_pos, parent_objp->segnum);

	//	Play sound.
	if ( parent_objp == Viewer )
		digi_play_sample( Weapon_info[weapon_objp->id].flash_sound, F1_0 );
	else
		digi_link_sound_to_pos( Weapon_info[weapon_objp->id].flash_sound, weapon_objp->segnum, 0, &weapon_objp->pos, 0, F1_0 );

	// -- if ((Last_omega_muzzle_flash_time + F1_0/4 < GameTime) || (Last_omega_muzzle_flash_time > GameTime)) {
	// -- 	do_muzzle_stuff(firing_segnum, firing_pos);
	// -- 	Last_omega_muzzle_flash_time = GameTime;
	// -- }

	//	Delete the original object.  Its only purpose in life was to determine which object to home in on.
	obj_delete(weapon_objp-Objects);

	//	If couldn't lock on anything, fire straight ahead.
	if (lock_objnum == -1) {
		fvi_query	fq;
		fvi_info		hit_data;
		int			fate;
		vms_vector	perturb_vec, perturbed_fvec;

		make_random_vector(&perturb_vec);
		vm_vec_scale_add(&perturbed_fvec, &parent_objp->orient.fvec, &perturb_vec, F1_0/16);

		vm_vec_scale_add(&goal_pos, firing_pos, &perturbed_fvec, MAX_OMEGA_DIST);
		fq.startseg = firing_segnum;
		if (fq.startseg == -1) {
			return;
		}
		fq.p0						= firing_pos;
		fq.p1						= &goal_pos;
		fq.rad					= 0;
		fq.thisobjnum			= parent_objp-Objects;
		fq.ignore_obj_list	= NULL;
		fq.flags					= FQ_IGNORE_POWERUPS | FQ_TRANSPOINT | FQ_CHECK_OBJS;		//what about trans walls???

		fate = find_vector_intersection(&fq, &hit_data);
		if (fate != HIT_NONE) {
			Assert(hit_data.hit_seg != -1);		//	How can this be?  We went from inside the mine to outside without hitting anything?
			goal_pos = hit_data.hit_pnt;
		}
	} else
		goal_pos = Objects[lock_objnum].pos;

	//	This is where we create a pile of omega blobs!
	create_omega_blobs(firing_segnum, firing_pos, &goal_pos, parent_objp);

}

// ---------------------------------------------------------------------------------
// Initializes a laser after Fire is pressed
//	Returns object number.
int Laser_create_new( vms_vector * direction, vms_vector * position, int segnum, int parent, int weapon_type, int make_sound )
{
	int objnum;
	object *obj;
	fix parent_speed, weapon_speed;
	fix volume;
	fix laser_length=0;

	Assert( weapon_type < N_weapon_types );

	if ( (weapon_type<0) || (weapon_type>=N_weapon_types) )
		weapon_type = 0;

	//	Don't let homing blobs make muzzle flash.
	if (Objects[parent].type == OBJ_ROBOT)
		do_muzzle_stuff(segnum, position);

	objnum = create_weapon_object(weapon_type,segnum,position);

	if ( objnum < 0 ) {
		return -1;
	}

	obj = &Objects[objnum];

	//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
	//	not apply to the Omega Cannon.
	if (weapon_type == OMEGA_ID) {
		// Create orientation matrix for tracking purposes.
		vm_vector_2_matrix( &obj->orient, direction, &Objects[parent].orient.uvec ,NULL);

		if (( &Objects[parent] != Viewer ) && (Objects[parent].type != OBJ_WEAPON))	{
			// Muzzle flash
			if (Weapon_info[obj->id].flash_vclip > -1 )
				object_create_muzzle_flash( obj->segnum, &obj->pos, Weapon_info[obj->id].flash_size, Weapon_info[obj->id].flash_vclip );
		}

		do_omega_stuff(&Objects[parent], position, obj);

		return objnum;
	}

	if (Objects[parent].type == OBJ_PLAYER) {
		if (weapon_type == FUSION_ID) {

			if (Fusion_charge <= 0)
				obj->ctype.laser_info.multiplier = F1_0;
			else if (Fusion_charge <= 4*F1_0)
				obj->ctype.laser_info.multiplier = F1_0 + Fusion_charge/2;
			else
				obj->ctype.laser_info.multiplier = 4*F1_0;

		} else if ((weapon_type >= LASER_ID && weapon_type <= MAX_SUPER_LASER_LEVEL) && (Players[Objects[parent].id].flags & PLAYER_FLAGS_QUAD_LASERS))
			obj->ctype.laser_info.multiplier = F1_0*3/4;
		else if (weapon_type == GUIDEDMISS_ID) {
			if (parent==Players[Player_num].objnum) {
				Guided_missile[Player_num]= obj;
				Guided_missile_sig[Player_num] = obj->signature;
				if (Newdemo_state==ND_STATE_RECORDING)
					newdemo_record_guided_start();
			}
		}
	}

	//	Make children of smart bomb bounce so if they hit a wall right away, they
	//	won't detonate.  The frame interval code will clear this bit after 1/2 second.
	if ((weapon_type == PLAYER_SMART_HOMING_ID) || (weapon_type == SMART_MINE_HOMING_ID) || (weapon_type == ROBOT_SMART_HOMING_ID) || (weapon_type == ROBOT_SMART_MINE_HOMING_ID) || (weapon_type == EARTHSHAKER_MEGA_ID))
		obj->mtype.phys_info.flags |= PF_BOUNCE;

	if (Weapon_info[weapon_type].render_type == WEAPON_RENDER_POLYMODEL)
		laser_length = Polygon_models[obj->rtype.pobj_info.model_num].rad * 2;

	if (weapon_type == FLARE_ID)
		obj->mtype.phys_info.flags |= PF_STICK;		//this obj sticks to walls

	obj->shields = Weapon_info[obj->id].strength[Difficulty_level];

	// Fill in laser-specific data

	obj->lifeleft							= Weapon_info[obj->id].lifetime;
	obj->ctype.laser_info.parent_type		= Objects[parent].type;
	obj->ctype.laser_info.parent_signature = Objects[parent].signature;
	obj->ctype.laser_info.parent_num			= parent;

	//	Assign parent type to highest level creator.  This propagates parent type down from
	//	the original creator through weapons which create children of their own (ie, smart missile)
	if (Objects[parent].type == OBJ_WEAPON) {
		int	highest_parent = parent;
		int	count;

		count = 0;
		while ((count++ < 10) && (Objects[highest_parent].type == OBJ_WEAPON)) {
			int	next_parent;

			next_parent = Objects[highest_parent].ctype.laser_info.parent_num;
			if (Objects[next_parent].signature != Objects[highest_parent].ctype.laser_info.parent_signature)
				break;	//	Probably means parent was killed.  Just continue.

			if (next_parent == highest_parent) {
				Int3();	//	Hmm, object is parent of itself.  This would seem to be bad, no?
				break;
			}

			highest_parent = next_parent;

			obj->ctype.laser_info.parent_num			= highest_parent;
			obj->ctype.laser_info.parent_type		= Objects[highest_parent].type;
			obj->ctype.laser_info.parent_signature = Objects[highest_parent].signature;
		}
	}

	// Create orientation matrix so we can look from this pov
	//	Homing missiles also need an orientation matrix so they know if they can make a turn.
	if ((obj->render_type == RT_POLYOBJ) || (Weapon_info[obj->id].homing_flag))
		vm_vector_2_matrix( &obj->orient,direction, &Objects[parent].orient.uvec ,NULL);

	if (( &Objects[parent] != Viewer ) && (Objects[parent].type != OBJ_WEAPON))	{
		// Muzzle flash
		if (Weapon_info[obj->id].flash_vclip > -1 )
			object_create_muzzle_flash( obj->segnum, &obj->pos, Weapon_info[obj->id].flash_size, Weapon_info[obj->id].flash_vclip );
	}

	volume = F1_0;
	if (Weapon_info[obj->id].flash_sound > -1 )	{
		if (make_sound)	{
			if ( parent == (Viewer-Objects) )	{
				if (weapon_type == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
					volume = F1_0 / 2;
				digi_play_sample( Weapon_info[obj->id].flash_sound, volume );
			} else {
				digi_link_sound_to_pos( Weapon_info[obj->id].flash_sound, obj->segnum, 0, &obj->pos, 0, volume );
			}
		}
	}

	//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
	// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
	// This also jitters the laser a bit so that it doesn't alias.
	//	Don't do for weapons created by weapons.
	if ((Objects[parent].type == OBJ_PLAYER) && (Weapon_info[weapon_type].render_type != WEAPON_RENDER_NONE) && (weapon_type != FLARE_ID)) {
		vms_vector	end_pos;
		int			end_segnum;

	 	vm_vec_scale_add( &end_pos, &obj->pos, direction, (laser_length/2) );
		end_segnum = find_point_seg(&end_pos, obj->segnum);
		if (end_segnum != obj->segnum) {
			if (end_segnum != -1) {
				obj->pos = end_pos;
				obj_relink(obj-Objects, end_segnum);
			}
		} else
			obj->pos = end_pos;
	}

	//	Here's where to fix the problem with objects which are moving backwards imparting higher velocity to their weaponfire.
	//	Find out if moving backwards.
	if (is_proximity_bomb_or_smart_mine(weapon_type)) {
		parent_speed = vm_vec_mag_quick(&Objects[parent].mtype.phys_info.velocity);
		if (vm_vec_dot(&Objects[parent].mtype.phys_info.velocity, &Objects[parent].orient.fvec) < 0)
			parent_speed = -parent_speed;
	} else
		parent_speed = 0;

	weapon_speed = Weapon_info[obj->id].speed[Difficulty_level];
	if (Weapon_info[obj->id].speedvar != 128) {
		fix	randval;

		//	Get a scale factor between speedvar% and 1.0.
		randval = F1_0 - ((d_rand() * Weapon_info[obj->id].speedvar) >> 6);
		weapon_speed = fixmul(weapon_speed, randval);
	}

	//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
	if ((obj->id == PLAYER_SMART_HOMING_ID) || (obj->id == SMART_MINE_HOMING_ID) || (obj->id == ROBOT_SMART_HOMING_ID) || (obj->id == ROBOT_SMART_MINE_HOMING_ID) || (obj->id == EARTHSHAKER_MEGA_ID))
		weapon_speed /= 4;

	if (Weapon_info[obj->id].thrust != 0)
		weapon_speed /= 2;

	vm_vec_copy_scale( &obj->mtype.phys_info.velocity, direction, weapon_speed + parent_speed );

	//	Set thrust
	if (Weapon_info[weapon_type].thrust != 0) {
		obj->mtype.phys_info.thrust = obj->mtype.phys_info.velocity;
		vm_vec_scale(&obj->mtype.phys_info.thrust, fixdiv(Weapon_info[obj->id].thrust, weapon_speed+parent_speed));
	}

	if ((obj->type == OBJ_WEAPON) && (obj->id == FLARE_ID))
		obj->lifeleft += (d_rand()-16384) << 2;		//	add in -2..2 seconds

	return objnum;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls Laser_create_new, but takes care of the segment and point computation for you.
int Laser_create_new_easy( vms_vector * direction, vms_vector * position, int parent, int weapon_type, int make_sound )
{
	fvi_query	fq;
	fvi_info		hit_data;
	object		*pobjp = &Objects[parent];
	int			fate;

	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
	//	which it fires may be in a different segment, which is bad news for find_vector_intersection.  So, cast
	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to Laser_create_new
	//	use the data returned from this call to find_vector_intersection.
	//	Note that while find_vector_intersection is pretty slow, it is not terribly slow if the destination point is
	//	in the same segment as the source point.

	fq.p0						= &pobjp->pos;
	fq.startseg				= pobjp->segnum;
	fq.p1						= position;
	fq.rad					= 0;
	fq.thisobjnum			= pobjp-Objects;
	fq.ignore_obj_list	= NULL;
	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	fate = find_vector_intersection(&fq, &hit_data);
	if (fate != HIT_NONE  || hit_data.hit_seg==-1) {
		return -1;
	}

	return Laser_create_new( direction, &hit_data.hit_pnt, hit_data.hit_seg, parent, weapon_type, make_sound );

}

int		Muzzle_queue_index = 0;

muzzle_info		Muzzle_data[MUZZLE_QUEUE_MAX];

//	-----------------------------------------------------------------------------------------------------------
//	Determine if two objects are on a line of sight.  If so, return true, else return false.
//	Calls fvi.
int object_to_object_visibility(object *obj1, object *obj2, int trans_type)
{
	fvi_query	fq;
	fvi_info		hit_data;
	int			fate;

	fq.p0						= &obj1->pos;
	fq.startseg				= obj1->segnum;
	fq.p1						= &obj2->pos;
	fq.rad					= 0x10;
	fq.thisobjnum			= obj1-Objects;
	fq.ignore_obj_list	= NULL;
	fq.flags					= trans_type;

	fate = find_vector_intersection(&fq, &hit_data);

	if (fate == HIT_WALL)
		return 0;
	else if (fate == HIT_NONE)
		return 1;
	else
		Int3();		//	Contact Mike: Oops, what happened?  What is fate?
						// 2 = hit object (impossible), 3 = bad starting point (bad)

	return 0;
}


//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *tracker is able to track object Objects[track_goal], else return false.
//	In order for the object to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a wall.
int object_is_trackable(int track_goal, object *tracker, fix *dot)
{
	vms_vector	vector_to_goal;
	object		*objp;

	if (track_goal == -1)
		return 0;

	if (Game_mode & GM_MULTI_COOP)
		return 0;

	objp = &Objects[track_goal];

	//	Don't track player if he's cloaked.
	if ((track_goal == Players[Player_num].objnum) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
		return 0;

	//	Can't track AI object if he's cloaked.
	if (objp->type == OBJ_ROBOT) {
		if (objp->ctype.ai_info.CLOAKED)
			return 0;
		//	Your missiles don't track your escort.
		if (Robot_info[objp->id].companion)
			if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
				return 0;
	}
	vm_vec_sub(&vector_to_goal, &objp->pos, &tracker->pos);
	vm_vec_normalize_quick(&vector_to_goal);
	*dot = vm_vec_dot(&vector_to_goal, &tracker->orient.fvec);

	if (*dot >= HOMING_MIN_TRACKABLE_DOT) {
		int	rval;
		//	dot is in legal range, now see if object is visible
		rval =  object_to_object_visibility(tracker, objp, FQ_TRANSWALL);
		return rval;
	} else {
		return 0;
	}
}

//	--------------------------------------------------------------------------------------------
int call_find_homing_object_complete(object *tracker, vms_vector *curpos)
{
	if (Game_mode & GM_MULTI) {
		if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER) {
			//	It's fired by a player, so if robots present, track robot, else track player.
			if (Game_mode & GM_MULTI_COOP)
				return find_homing_object_complete( curpos, tracker, OBJ_ROBOT, -1);
			else
				return find_homing_object_complete( curpos, tracker, OBJ_PLAYER, OBJ_ROBOT);
		} else {
			int	goal2_type = -1;

			if (cheats.robotskillrobots)
				goal2_type = OBJ_ROBOT;
			Assert(tracker->ctype.laser_info.parent_type == OBJ_ROBOT);
			return find_homing_object_complete(curpos, tracker, OBJ_PLAYER, goal2_type);
		}
	} else
		return find_homing_object_complete( curpos, tracker, OBJ_ROBOT, -1);
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
int find_homing_object(vms_vector *curpos, object *tracker)
{
	int	i;
	fix	max_dot = -F1_0*2;
	int	best_objnum = -1;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
	Assert((Weapon_info[tracker->id].homing_flag) || (tracker->id == OMEGA_ID));

	//	Find an object to track based on game mode (eg, whether in network play) and who fired it.

	if (Game_mode & GM_MULTI)
		return call_find_homing_object_complete(tracker, curpos);
	else {
		int cur_min_trackable_dot = HOMING_MAX_TRACKABLE_DOT;

		if ((tracker->type == OBJ_WEAPON) && (tracker->id == OMEGA_ID))
			cur_min_trackable_dot = OMEGA_MIN_TRACKABLE_DOT;

		//	Not in network mode.  If not fired by player, then track player.
		if (tracker->ctype.laser_info.parent_num != Players[Player_num].objnum) {
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				best_objnum = ConsoleObject - Objects;
		} else {
			int	window_num = -1;
			fix	dist, max_trackable_dist = HOMING_MAX_TRACKABLE_DIST;

			if (tracker->id == OMEGA_ID)
				max_trackable_dist = OMEGA_MAX_TRACKABLE_DIST;

			//	Find the window which has the forward view.
			for (i=0; i<MAX_RENDERED_WINDOWS; i++)
				if (Window_rendered_data[i].time >= timer_query()-1)
					if (Window_rendered_data[i].viewer == ConsoleObject)
						if (!Window_rendered_data[i].rear_view) {
							window_num = i;
							break;
						}

			//	Couldn't find suitable view from this frame, so do complete search.
			if (window_num == -1) {
				return call_find_homing_object_complete(tracker, curpos);
			}

			//	Not in network mode and fired by player.
			for (i=Window_rendered_data[window_num].num_objects-1; i>=0; i--) {
				fix			dot; //, dist;
				vms_vector	vec_to_curobj;
				int			objnum = Window_rendered_data[window_num].rendered_objects[i];
				object		*curobjp = &Objects[objnum];

				if (objnum == Players[Player_num].objnum)
					continue;

				//	Can't track AI object if he's cloaked.
				if (curobjp->type == OBJ_ROBOT) {
					if (curobjp->ctype.ai_info.CLOAKED)
						continue;

					//	Your missiles don't track your escort.
					if (Robot_info[curobjp->id].companion)
						if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
							continue;
				}

				vm_vec_sub(&vec_to_curobj, &curobjp->pos, curpos);
				dist = vm_vec_normalize_quick(&vec_to_curobj);
				if (dist < max_trackable_dist) {
					dot = vm_vec_dot(&vec_to_curobj, &tracker->orient.fvec);

					if (dot > cur_min_trackable_dot) {
						if (dot > max_dot) {
							if (object_to_object_visibility(tracker, &Objects[objnum], FQ_TRANSWALL)) {
								max_dot = dot;
								best_objnum = objnum;
							}
						}
					}
				}
			}
		}
	}

	return best_objnum;
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of objects.  If you are only interested in one type, set track_obj_type2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing objects not track parent's prox bombs.
int find_homing_object_complete(vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2)
{
	int	objnum;
	fix	max_dot = -F1_0*2;
	int	best_objnum = -1;
	fix	max_trackable_dist;
	fix	min_trackable_dot;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
	Assert((Weapon_info[tracker->id].homing_flag) || (tracker->id == OMEGA_ID));

	max_trackable_dist = HOMING_MAX_TRACKABLE_DIST;
	min_trackable_dot = HOMING_MAX_TRACKABLE_DOT;

	if (tracker->id == OMEGA_ID) {
		max_trackable_dist = OMEGA_MAX_TRACKABLE_DIST;
		min_trackable_dot = OMEGA_MIN_TRACKABLE_DOT;
	}

	for (objnum=0; objnum<=Highest_object_index; objnum++) {
		int			is_proximity = 0;
		fix			dot, dist;
		vms_vector	vec_to_curobj;
		object		*curobjp = &Objects[objnum];

		if ((curobjp->type != track_obj_type1) && (curobjp->type != track_obj_type2))
		{
			if ((curobjp->type == OBJ_WEAPON) && (is_proximity_bomb_or_smart_mine(curobjp->id))) {
				if (curobjp->ctype.laser_info.parent_signature != tracker->ctype.laser_info.parent_signature)
					is_proximity = 1;
				else
					continue;
			} else
				continue;
		}

		if (objnum == tracker->ctype.laser_info.parent_num) // Don't track shooter
			continue;

		//	Don't track cloaked players.
		if (curobjp->type == OBJ_PLAYER)
		{
			if (Players[curobjp->id].flags & PLAYER_FLAGS_CLOAKED)
				continue;
			// Don't track teammates in team games
			#ifdef NETWORK
			if ((Game_mode & GM_TEAM) && (Objects[tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) && (get_team(curobjp->id) == get_team(Objects[tracker->ctype.laser_info.parent_num].id)))
				continue;
			#endif
		}

		//	Can't track AI object if he's cloaked.
		if (curobjp->type == OBJ_ROBOT) {
			if (curobjp->ctype.ai_info.CLOAKED)
				continue;

			//	Your missiles don't track your escort.
			if (Robot_info[curobjp->id].companion)
				if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
					continue;
		}

		vm_vec_sub(&vec_to_curobj, &curobjp->pos, curpos);
		dist = vm_vec_mag(&vec_to_curobj);

		if (dist < max_trackable_dist) {
			vm_vec_normalize(&vec_to_curobj);
			dot = vm_vec_dot(&vec_to_curobj, &tracker->orient.fvec);
			if (is_proximity)
				dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

			if (dot > min_trackable_dot) {
				if (dot > max_dot) {
					if (object_to_object_visibility(tracker, &Objects[objnum], FQ_TRANSWALL)) {
						max_dot = dot;
						best_objnum = objnum;
					}
				}
			}
		}

	}

	return best_objnum;
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked object.  If not, see if another object is trackable.  If not, return -1,
//	else return object number of tracking object.
//	Computes and returns a fairly precise dot product.
int track_track_goal(int track_goal, object *tracker, fix *dot)
{
	if (object_is_trackable(track_goal, tracker, dot)) {
		return track_goal;
	} else if ((((tracker-Objects) ^ d_tick_count) % 4) == 0)
	{
		int	rval = -2;

		//	If player fired missile, then search for an object, if not, then give up.
		if (Objects[tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) {
			int	goal_type;

			if (track_goal == -1)
			{
				if (Game_mode & GM_MULTI)
				{
					if (Game_mode & GM_MULTI_COOP)
						rval = find_homing_object_complete( &tracker->pos, tracker, OBJ_ROBOT, -1);
					else if (Game_mode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or player
						rval = find_homing_object_complete( &tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
					else		//	Not cooperative and no robots, track only a player
						rval = find_homing_object_complete( &tracker->pos, tracker, OBJ_PLAYER, -1);
				}
				else
					rval = find_homing_object_complete(&tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
			}
			else
			{
				goal_type = Objects[tracker->ctype.laser_info.track_goal].type;
				if ((goal_type == OBJ_PLAYER) || (goal_type == OBJ_ROBOT))
					rval = find_homing_object_complete(&tracker->pos, tracker, goal_type, -1);
				else
					rval = -1;
			}
		}
		else {
			int	goal_type, goal2_type = -1;

			if (cheats.robotskillrobots)
				goal2_type = OBJ_ROBOT;

			if (track_goal == -1)
				rval = find_homing_object_complete(&tracker->pos, tracker, OBJ_PLAYER, goal2_type);
			else {
				goal_type = Objects[tracker->ctype.laser_info.track_goal].type;
				rval = find_homing_object_complete(&tracker->pos, tracker, goal_type, goal2_type);
			}
		}

		Assert(rval != -2);		//	This means it never got set which is bad!  Contact Mike.
		return rval;
	}

	return -1;
}

//-------------- Initializes a laser after Fire is pressed -----------------

void Laser_player_fire_spread_delay(object *obj, int laser_type, int gun_num, fix spreadr, fix spreadu, fix delay_time, int make_sound, int harmless)
{
	int			LaserSeg, Fate;
	vms_vector	LaserPos, LaserDir;
	fvi_query	fq;
	fvi_info		hit_data;
	vms_vector	gun_point, *pnt;
	vms_matrix	m;
	int			objnum;

	create_awareness_event(obj, PA_WEAPON_WALL_COLLISION);

	// Find the initial position of the laser
	pnt = &Player_ship->gun_points[gun_num];

	vm_copy_transpose_matrix(&m,&obj->orient);
	vm_vec_rotate(&gun_point,pnt,&m);

	vm_vec_add(&LaserPos,&obj->pos,&gun_point);

	//	If supposed to fire at a delayed time (delay_time), then move this point backwards.
	if (delay_time)
		vm_vec_scale_add2(&LaserPos, &obj->orient.fvec, -fixmul(delay_time, Weapon_info[laser_type].speed[Difficulty_level]));

//	do_muzzle_stuff(obj, &Pos);

	//--------------- Find LaserPos and LaserSeg ------------------
	fq.p0						= &obj->pos;
	fq.startseg				= obj->segnum;
	fq.p1						= &LaserPos;
	fq.rad					= 0x10;
	fq.thisobjnum			= obj-Objects;
	fq.ignore_obj_list	= NULL;
	fq.flags					= FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS;

	Fate = find_vector_intersection(&fq, &hit_data);

	LaserSeg = hit_data.hit_seg;

	if (LaserSeg == -1)		//some sort of annoying error
		return;

	//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
	if ( vm_vec_dist_quick(&LaserPos, &obj->pos) > 0x50000 )
		return;

	if (Fate==HIT_WALL) {
		return;
	}

	if (Fate==HIT_OBJECT) {
//		if ( Objects[hit_data.hit_object].type == OBJ_ROBOT )
//			Objects[hit_data.hit_object].flags |= OF_SHOULD_BE_DEAD;
//		if ( Objects[hit_data.hit_object].type != OBJ_POWERUP )
//			return;
	//as of 12/6/94, we don't care if the laser is stuck in an object. We
	//just fire away normally
	}

	//	Now, make laser spread out.
	LaserDir = obj->orient.fvec;
	if ((spreadr != 0) || (spreadu != 0)) {
		vm_vec_scale_add2(&LaserDir, &obj->orient.rvec, spreadr);
		vm_vec_scale_add2(&LaserDir, &obj->orient.uvec, spreadu);
	}

	objnum = Laser_create_new( &LaserDir, &LaserPos, LaserSeg, obj-Objects, laser_type, make_sound );

	//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
	if (laser_type == OMEGA_ID)
		return;

	if (objnum == -1)
		return;

#ifdef NETWORK
	if (laser_type==GUIDEDMISS_ID && Multi_is_guided) {
		Guided_missile[obj->id]=&Objects[objnum];
	}

	Multi_is_guided=0;
#endif

	if (laser_type == CONCUSSION_ID ||
			 laser_type == HOMING_ID ||
			 laser_type == SMART_ID ||
			 laser_type == MEGA_ID ||
			 laser_type == FLASH_ID ||
			 //laser_type == GUIDEDMISS_ID ||
			 //laser_type == SUPERPROX_ID ||
			 laser_type == MERCURY_ID ||
			 laser_type == EARTHSHAKER_ID)
		if (Missile_viewer == NULL && obj->id==Player_num)
			Missile_viewer = &Objects[objnum];

	//	If this weapon is supposed to be silent, set that bit!
	if (!make_sound)
		Objects[objnum].flags |= OF_SILENT;

	//	If this weapon is supposed to be silent, set that bit!
	if (harmless)
		Objects[objnum].flags |= OF_HARMLESS;

	//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
	//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
	if ((obj == ConsoleObject) && ((Objects[objnum].id != PROXIMITY_ID) && (Objects[objnum].id != SUPERPROX_ID)))
		Player_fired_laser_this_frame = objnum;

	if (Weapon_info[laser_type].homing_flag) {
		if (obj == ConsoleObject)
		{
			Objects[objnum].ctype.laser_info.track_goal = find_homing_object(&LaserPos, &Objects[objnum]);
			#ifdef NETWORK
			Network_laser_track = Objects[objnum].ctype.laser_info.track_goal;
			#endif
		}
		#ifdef NETWORK
		else // Some other player shot the homing thing
		{
			Assert(Game_mode & GM_MULTI);
			Objects[objnum].ctype.laser_info.track_goal = Network_laser_track;
		}
		#endif
		Objects[objnum].ctype.laser_info.track_turn_time = HOMING_TURN_TIME;
	}
}

//	-----------------------------------------------------------------------------------------------------------
void Laser_player_fire_spread(object *obj, int laser_type, int gun_num, fix spreadr, fix spreadu, int make_sound, int harmless)
{
	Laser_player_fire_spread_delay(obj, laser_type, gun_num, spreadr, spreadu, 0, make_sound, harmless);
}


//	-----------------------------------------------------------------------------------------------------------
void Laser_player_fire(object *obj, int laser_type, int gun_num, int make_sound, int harmless)
{
	Laser_player_fire_spread(obj, laser_type, gun_num, 0, 0, make_sound, harmless);
}

//	-----------------------------------------------------------------------------------------------------------
void Flare_create(object *obj)
{
	fix	energy_usage;

	energy_usage = Weapon_info[FLARE_ID].energy_usage;

	if (Difficulty_level < 2)
		energy_usage = fixmul(energy_usage, i2f(Difficulty_level+2)/4);

//	MK, 11/04/95: Allowed to fire flare even if no energy.
// -- 	if (Players[Player_num].energy >= energy_usage) {
		Players[Player_num].energy -= energy_usage;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
			// -- auto_select_weapon(0);
		}

		Laser_player_fire( obj, FLARE_ID, 6, 1, 0);

		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_fire(FLARE_ADJUST, 0, 0, 1, -1);
		#endif
// -- 	}

}

#define	HOMING_MISSILE_SCALE	16

//--------------------------------------------------------------------
//	Set object *objp's orientation to (or towards if I'm ambitious) its velocity.
void homing_missile_turn_towards_velocity(object *objp, vms_vector *norm_vel)
{
	vms_vector	new_fvec;

	new_fvec = *norm_vel;

	vm_vec_scale(&new_fvec, FrameTime * HOMING_MISSILE_SCALE);
	vm_vec_add2(&new_fvec, &objp->orient.fvec);
	vm_vec_normalize_quick(&new_fvec);

//	if ((norm_vel->x == 0) && (norm_vel->y == 0) && (norm_vel->z == 0))
//		return;

	vm_vector_2_matrix(&objp->orient, &new_fvec, NULL, NULL);
}


//-------------------------------------------------------------------------------------------
//sequence this laser object for this _frame_ (underscores added here to aid MK in his searching!)
void Laser_do_weapon_sequence(object *obj)
{
	Assert(obj->control_type == CT_WEAPON);

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( Weapon_info[obj->id].damage_radius )
			explode_badass_weapon(obj,&obj->pos);
		return;
	}

	if (omega_cleanup(obj))
		return;

	//delete weapons that are not moving
	if (	!((d_tick_count ^ obj->signature) & 3) &&
			(obj->id != FLARE_ID) &&
			(Weapon_info[obj->id].speed[Difficulty_level] > 0) &&
			(vm_vec_mag_quick(&obj->mtype.phys_info.velocity) < F2_0)) {
		obj_delete(obj-Objects);
		return;
	}

	if ( obj->id == FUSION_ID ) {		//always set fusion weapon to max vel

		vm_vec_normalize_quick(&obj->mtype.phys_info.velocity);

		vm_vec_scale(&obj->mtype.phys_info.velocity, Weapon_info[obj->id].speed[Difficulty_level]);
	}

	//	For homing missiles, turn towards target. (unless it's the guided missile)
	if (Weapon_info[obj->id].homing_flag && !(obj->id==GUIDEDMISS_ID && obj->ctype.laser_info.parent_type==OBJ_PLAYER && obj==Guided_missile[Objects[obj->ctype.laser_info.parent_num].id] && obj->signature==Guided_missile[Objects[obj->ctype.laser_info.parent_num].id]->signature))
	{
		vms_vector		vector_to_object, temp_vec;
		fix				dot=F1_0;
		fix				speed, max_speed;

		//	For first 125ms of life, missile flies straight.
		if (obj->ctype.laser_info.creation_time + HOMING_FLY_STRAIGHT_TIME < GameTime64) {

			int	track_goal = obj->ctype.laser_info.track_goal;

			//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
			if ((obj->id == ROBOT_SMART_MINE_HOMING_ID) || (obj->id == ROBOT_SMART_HOMING_ID) || (obj->id == SMART_MINE_HOMING_ID) || (obj->id == PLAYER_SMART_HOMING_ID) || (obj->id == EARTHSHAKER_MEGA_ID)) {
				obj->mtype.phys_info.flags &= ~PF_BOUNCE;
			}

			//	Make sure the object we are tracking is still trackable.
			track_goal = track_track_goal(track_goal, obj, &dot);

			if (track_goal == Players[Player_num].objnum) {
				fix	dist_to_player;

				dist_to_player = vm_vec_dist_quick(&obj->pos, &Objects[track_goal].pos);
				if ((dist_to_player < Players[Player_num].homing_object_dist) || (Players[Player_num].homing_object_dist < 0))
					Players[Player_num].homing_object_dist = dist_to_player;

			}

			if (track_goal != -1) {
#ifdef NEWHOMER
				// See if enough time (see HOMING_TURN_TIME) passed and if yes, allow a turn. If not, fly straight.
				if (obj->ctype.laser_info.track_turn_time >= HOMING_TURN_TIME)
				{
					vm_vec_sub(&vector_to_object, &Objects[track_goal].pos, &obj->pos);
					obj->ctype.laser_info.track_turn_time -= HOMING_TURN_TIME;
				}
				else
				{
					vms_vector straight;
					vm_vec_add(&straight, &obj->mtype.phys_info.velocity, &obj->pos);
					vm_vec_sub(&vector_to_object, &straight, &obj->pos);
				}
				obj->ctype.laser_info.track_turn_time += FrameTime;

				// Scale vector to object to current FrameTime if we run really low
				if (FrameTime > HOMING_TURN_TIME)
					vm_vec_scale(&vector_to_object, F1_0/((float)HOMING_TURN_TIME/FrameTime));
#else
				vm_vec_sub(&vector_to_object, &Objects[track_goal].pos, &obj->pos);
#endif
				vm_vec_normalize_quick(&vector_to_object);
				temp_vec = obj->mtype.phys_info.velocity;
				speed = vm_vec_normalize_quick(&temp_vec);
				max_speed = Weapon_info[obj->id].speed[Difficulty_level];
				if (speed+F1_0 < max_speed) {
					speed += fixmul(max_speed, FrameTime/2);
					if (speed > max_speed)
						speed = max_speed;
				}

				dot = vm_vec_dot(&temp_vec, &vector_to_object);

				vm_vec_add2(&temp_vec, &vector_to_object);
				//	The boss' smart children track better...
				if (Weapon_info[obj->id].render_type != WEAPON_RENDER_POLYMODEL)
					vm_vec_add2(&temp_vec, &vector_to_object);
				vm_vec_normalize_quick(&temp_vec);
				vm_vec_scale(&temp_vec, speed);
				obj->mtype.phys_info.velocity = temp_vec;

				//	Subtract off life proportional to amount turned.
				//	For hardest turn, it will lose 2 seconds per second.
				{
					fix	lifelost, absdot;

					absdot = abs(F1_0 - dot);

					lifelost = fixmul(absdot*32, FrameTime);
					obj->lifeleft -= lifelost;
				}

				//	Only polygon objects have visible orientation, so only they should turn.
				if (Weapon_info[obj->id].render_type == WEAPON_RENDER_POLYMODEL)
					homing_missile_turn_towards_velocity(obj, &temp_vec); // temp_vec is normalized velocity.
			}
		}
	}

	//	Make sure weapon is not moving faster than allowed speed.
	{
		fix	weapon_speed;

		weapon_speed = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
		if (weapon_speed > Weapon_info[obj->id].speed[Difficulty_level]) {
			//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
			if (Weapon_info[obj->id].speed[Difficulty_level]) {
				fix	scale_factor;

				scale_factor = fixdiv(Weapon_info[obj->id].speed[Difficulty_level], weapon_speed);
				vm_vec_scale(&obj->mtype.phys_info.velocity, scale_factor);
			}
		}
	}
}

fix64	Last_laser_fired_time = 0;

extern int Player_fired_laser_this_frame;

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for network players

int do_laser_firing_player(void)
{
	player	*plp = &Players[Player_num];
	fix		energy_used;
	int		ammo_used,primary_ammo;
	int		weapon_index;
	int		rval = 0;
	int 		nfires = 1;
	static int Spreadfire_toggle=0;
	static int Helix_orientation = 0;

	if (Player_is_dead)
		return 0;

	weapon_index = Primary_weapon_to_weapon_info[Primary_weapon];
	energy_used = Weapon_info[weapon_index].energy_usage;
	if (Primary_weapon == OMEGA_INDEX)
		energy_used = 0;	//	Omega consumes energy when recharging, not when firing.

	if (Difficulty_level < 2)
		energy_used = fixmul(energy_used, i2f(Difficulty_level+2)/4);

	//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
	if (weapon_index == HELIX_INDEX)
		if (Game_mode & GM_MULTI)
			energy_used *= 2;

	ammo_used = Weapon_info[weapon_index].ammo_usage;

	primary_ammo = (Primary_weapon == GAUSS_INDEX)?(plp->primary_ammo[VULCAN_INDEX]):(plp->primary_ammo[Primary_weapon]);

	if	(!((plp->energy >= energy_used) && (primary_ammo >= ammo_used)))
		auto_select_weapon(0);		//	Make sure the player can fire from this weapon.

	while (Next_laser_fire_time <= GameTime64) {
		if	((plp->energy >= energy_used) && (primary_ammo >= ammo_used)) {
			int	laser_level, flags, fire_frame_overhead = 0;

			if (GameTime64 - Next_laser_fire_time <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that.
				fire_frame_overhead = GameTime64 - Next_laser_fire_time;

                        Last_laser_fired_time = GameTime64;

			if (!cheats.rapidfire)
				Next_laser_fire_time = GameTime64 + Weapon_info[weapon_index].fire_wait - fire_frame_overhead;
			else
				Next_laser_fire_time = GameTime64 + (F1_0/25) - fire_frame_overhead;

			laser_level = Players[Player_num].laser_level;

			flags = 0;

			if (Primary_weapon == SPREADFIRE_INDEX) {
				if (Spreadfire_toggle)
					flags |= LASER_SPREADFIRE_TOGGLED;
				Spreadfire_toggle = !Spreadfire_toggle;
			}

			if (Primary_weapon == HELIX_INDEX) {
				Helix_orientation++;
				flags |= ((Helix_orientation & LASER_HELIX_MASK) << LASER_HELIX_SHIFT);
			}

			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
				flags |= LASER_QUAD;

			rval += do_laser_firing(Players[Player_num].objnum, Primary_weapon, laser_level, flags, nfires);

			plp->energy -= (energy_used * rval) / Weapon_info[weapon_index].fire_count;
			if (plp->energy < 0)
				plp->energy = 0;

			if ((Primary_weapon == VULCAN_INDEX) || (Primary_weapon == GAUSS_INDEX)) {
				if (ammo_used > plp->primary_ammo[VULCAN_INDEX])
					plp->primary_ammo[VULCAN_INDEX] = 0;
				else
					plp->primary_ammo[VULCAN_INDEX] -= ammo_used;
			}

			auto_select_weapon(0);		//	Make sure the player can fire from this weapon.

		} else {
			auto_select_weapon(0);		//	Make sure the player can fire from this weapon.
			Next_laser_fire_time = GameTime64;	//	Prevents shots-to-fire from building up.
			break;	//	Couldn't fire weapon, so abort.
		}
	}

	Global_laser_firing_count = 0;

	return rval;
}

//	--------------------------------------------------------------------------------------------------
//	Object "objnum" fires weapon "weapon_num" of level "level".  (Right now (9/24/94) level is used only for type 0 laser.
//	Flags are the player flags.  For network mode, set to 0.
//	It is assumed that this is a player object (as in multiplayer), and therefore the gun positions are known.
//	Returns number of times a weapon was fired.  This is typically 1, but might be more for low frame rates.
//	More than one shot is fired with a pseudo-delay so that players on show machines can fire (for themselves
//	or other players) often enough for things like the vulcan cannon.
int do_laser_firing(int objnum, int weapon_num, int level, int flags, int nfires)
{
	object	*objp = &Objects[objnum];

	switch (weapon_num) {
		case LASER_INDEX: {
			int weapon_num;

			if (level <= MAX_LASER_LEVEL)
				weapon_num = LASER_ID + level;
			else
				weapon_num = SUPER_LASER_ID + (level-MAX_LASER_LEVEL-1);

			Laser_player_fire( objp, weapon_num, 0, 1, 0);
			Laser_player_fire( objp, weapon_num, 1, 0, 0);

			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				Laser_player_fire( objp, weapon_num, 2, 0, 0);
				Laser_player_fire( objp, weapon_num, 3, 0, 0);
			}
			break;
		}
		case VULCAN_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand() > 24576)
			//	make_sound = 1;
			Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, make_sound, 0);
			if (nfires > 1) {
				Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, 0, 0);
				if (nfires > 2) {
					Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, 0, 0);
				}
			}
			break;
		}
		case SPREADFIRE_INDEX:
			if (flags & LASER_SPREADFIRE_TOGGLED) {
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, F1_0/16, 0, 0, 0);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, -F1_0/16, 0, 0, 0);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, 0, 1, 0);
			} else {
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, F1_0/16, 0, 0);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, -F1_0/16, 0, 0);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, 0, 1, 0);
			}
			break;

		case PLASMA_INDEX:
			Laser_player_fire( objp, PLASMA_ID, 0, 1, 0);
			Laser_player_fire( objp, PLASMA_ID, 1, 0, 0);
			if (nfires > 1) {
				Laser_player_fire_spread_delay( objp, PLASMA_ID, 0, 0, 0, FrameTime/2, 1, 0);
				Laser_player_fire_spread_delay( objp, PLASMA_ID, 1, 0, 0, FrameTime/2, 0, 0);
			}
			break;

		case FUSION_INDEX: {
			vms_vector	force_vec;

			Laser_player_fire( objp, FUSION_ID, 0, 1, 0);
			Laser_player_fire( objp, FUSION_ID, 1, 1, 0);

			flags = (sbyte)(Fusion_charge >> 12);

			Fusion_charge = 0;

			force_vec.x = -(objp->orient.fvec.x << 7);
			force_vec.y = -(objp->orient.fvec.y << 7);
			force_vec.z = -(objp->orient.fvec.z << 7);
			phys_apply_force(objp, &force_vec);

			force_vec.x = (force_vec.x >> 4) + d_rand() - 16384;
			force_vec.y = (force_vec.y >> 4) + d_rand() - 16384;
			force_vec.z = (force_vec.z >> 4) + d_rand() - 16384;
			phys_apply_rot(objp, &force_vec);

		}
			break;
		case SUPER_LASER_INDEX: {
			int super_level = 3;		//make some new kind of laser eventually
			Laser_player_fire( objp, super_level, 0, 1, 0);
			Laser_player_fire( objp, super_level, 1, 0, 0);

			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				Laser_player_fire( objp, super_level, 2, 0, 0);
				Laser_player_fire( objp, super_level, 3, 0, 0);
			}
			break;
		}
		case GAUSS_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand() > 24576)
			//	make_sound = 1;

			Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, make_sound, 0);
			if (nfires > 1) {
				Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, 0, 0);
				if (nfires > 2) {
					Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, 0, 0);
				}
			}
			break;
		}
		case HELIX_INDEX: {
			int helix_orient;
			fix spreadr,spreadu;
			helix_orient = (flags >> LASER_HELIX_SHIFT) & LASER_HELIX_MASK;
			switch(helix_orient) {

				case 0: spreadr =  F1_0/16; spreadu = 0;       break; // Vertical
				case 1: spreadr =  F1_0/17; spreadu = F1_0/42; break; //  22.5 degrees
				case 2: spreadr =  F1_0/22; spreadu = F1_0/22; break; //  45   degrees
				case 3: spreadr =  F1_0/42; spreadu = F1_0/17; break; //  67.5 degrees
				case 4: spreadr =  0;       spreadu = F1_0/16; break; //  90   degrees
				case 5: spreadr = -F1_0/42; spreadu = F1_0/17; break; // 112.5 degrees
				case 6: spreadr = -F1_0/22; spreadu = F1_0/22; break; // 135   degrees
				case 7: spreadr = -F1_0/17; spreadu = F1_0/42; break; // 157.5 degrees
				default:
					Error("Invalid helix_orientation value %x\n",helix_orient);
			}

			Laser_player_fire_spread( objp, HELIX_ID, 6,  0,  0, 1, 0);
			Laser_player_fire_spread( objp, HELIX_ID, 6,  spreadr,  spreadu, 0, 0);
			Laser_player_fire_spread( objp, HELIX_ID, 6, -spreadr, -spreadu, 0, 0);
			Laser_player_fire_spread( objp, HELIX_ID, 6,  spreadr*2,  spreadu*2, 0, 0);
			Laser_player_fire_spread( objp, HELIX_ID, 6, -spreadr*2, -spreadu*2, 0, 0);
			break;
		}

		case PHOENIX_INDEX:
			Laser_player_fire( objp, PHOENIX_ID, 0, 1, 0);
			Laser_player_fire( objp, PHOENIX_ID, 1, 0, 0);
			if (nfires > 1) {
				Laser_player_fire_spread_delay( objp, PHOENIX_ID, 0, 0, 0, FrameTime/2, 1, 0);
				Laser_player_fire_spread_delay( objp, PHOENIX_ID, 1, 0, 0, FrameTime/2, 0, 0);
			}
			break;

		case OMEGA_INDEX:
			Laser_player_fire( objp, OMEGA_ID, 1, 1, 0);
			break;

		default:
			Int3();	//	Contact Yuan: Unknown Primary weapon type, setting to 0.
			Primary_weapon = 0;
	}

	// Set values to be recognized during comunication phase, if we are the
	//  one shooting
#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && (objnum == Players[Player_num].objnum))
		multi_send_fire(weapon_num, level, flags, nfires, -1);
#endif

	return nfires;
}

#define	MAX_SMART_DISTANCE	(F1_0*150)
#define	MAX_OBJDISTS			30

//	-------------------------------------------------------------------------------------------
//	if goal_obj == -1, then create random vector
int create_homing_missile(object *objp, int goal_obj, int objtype, int make_sound)
{
	int			objnum;
	vms_vector	vector_to_goal;
	vms_vector	random_vector;
	//vms_vector	goal_pos;

	if (goal_obj == -1) {
		make_random_vector(&vector_to_goal);
	} else {
		vm_vec_normalized_dir_quick(&vector_to_goal, &Objects[goal_obj].pos, &objp->pos);
		make_random_vector(&random_vector);
		vm_vec_scale_add2(&vector_to_goal, &random_vector, F1_0/4);
		vm_vec_normalize_quick(&vector_to_goal);
	}

	//	Create a vector towards the goal, then add some noise to it.
	objnum = Laser_create_new(&vector_to_goal, &objp->pos, objp->segnum, objp-Objects, objtype, make_sound);
	if (objnum == -1)
		return -1;

	// Fixed to make sure the right person gets credit for the kill

//	Objects[objnum].ctype.laser_info.parent_num = objp->ctype.laser_info.parent_num;
//	Objects[objnum].ctype.laser_info.parent_type = objp->ctype.laser_info.parent_type;
//	Objects[objnum].ctype.laser_info.parent_signature = objp->ctype.laser_info.parent_signature;

	Objects[objnum].ctype.laser_info.track_goal = goal_obj;

	return objnum;
}

extern void blast_nearby_glass(object *objp, fix damage);

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void create_smart_children(object *objp, int num_smart_children)
{
	int parent_type, parent_num;
	int numobjs=0, objnum = 0, sel_objnum, last_sel_objnum = -1;
	int objlist[MAX_OBJDISTS];
	int blob_id;

	if (objp->type == OBJ_WEAPON) {
		parent_type = objp->ctype.laser_info.parent_type;
		parent_num = objp->ctype.laser_info.parent_num;
	} else if (objp->type == OBJ_ROBOT) {
		parent_type = OBJ_ROBOT;
		parent_num = objp-Objects;
	} else {
		Int3();	//	Hey, what kind of object is this!?
		parent_type = 0;
		parent_num = 0;
	}

#ifndef NDEBUG
	if ((objp->type == OBJ_WEAPON) && ((objp->id == SMART_ID) || (objp->id == SUPERPROX_ID) || (objp->id == ROBOT_SUPERPROX_ID) || (objp->id == EARTHSHAKER_ID)))
		Assert(Weapon_info[objp->id].children != -1);
#endif

	if (objp->type == OBJ_WEAPON && objp->id == EARTHSHAKER_ID)
		blast_nearby_glass(objp, Weapon_info[EARTHSHAKER_ID].strength[Difficulty_level]);

	if (((objp->type == OBJ_WEAPON) && (Weapon_info[objp->id].children != -1)) || (objp->type == OBJ_ROBOT)) {
		int i;

		if (Game_mode & GM_MULTI)
			d_srand(8321L);

		for (objnum=0; objnum<=Highest_object_index; objnum++) {
			object *curobjp = &Objects[objnum];

			if ((((curobjp->type == OBJ_ROBOT) && (!curobjp->ctype.ai_info.CLOAKED)) || (curobjp->type == OBJ_PLAYER)) && (objnum != parent_num)) {
				fix dist;

				if (curobjp->type == OBJ_PLAYER)
				{
					if ((parent_type == OBJ_PLAYER) && (Game_mode & GM_MULTI_COOP))
						continue;
					if ((Game_mode & GM_TEAM) && (get_team(curobjp->id) == get_team(Objects[parent_num].id)))
						continue;
					if (Players[curobjp->id].flags & PLAYER_FLAGS_CLOAKED)
						continue;
				}

				//	Robot blobs can't track robots.
				if (curobjp->type == OBJ_ROBOT) {
					if (parent_type == OBJ_ROBOT)
						continue;

					//	Your shots won't track the buddy.
					if (parent_type == OBJ_PLAYER)
						if (Robot_info[curobjp->id].companion)
							continue;
				}

				dist = vm_vec_dist_quick(&objp->pos, &curobjp->pos);
				if (dist < MAX_SMART_DISTANCE) {
					int oovis;

					oovis = object_to_object_visibility(objp, curobjp, FQ_TRANSWALL);

					if (oovis) { //object_to_object_visibility(objp, curobjp, FQ_TRANSWALL)) {
						objlist[numobjs] = objnum;
						numobjs++;
						if (numobjs >= MAX_OBJDISTS) {
							numobjs = MAX_OBJDISTS;
							break;
						}
					}
				}
			}
		}

		//	Get type of weapon for child from parent.
		if (objp->type == OBJ_WEAPON) {
			blob_id = Weapon_info[objp->id].children;
			Assert(blob_id != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
		} else {
			Assert(objp->type == OBJ_ROBOT);
			blob_id = ROBOT_SMART_HOMING_ID;
		}

		for (i=0; i<num_smart_children; i++) {
			sel_objnum = (numobjs==0)?-1:objlist[(d_rand() * numobjs) >> 15];
			if (numobjs > 1)
				while (sel_objnum == last_sel_objnum)
					sel_objnum = objlist[(d_rand() * numobjs) >> 15];
			create_homing_missile(objp, sel_objnum, blob_id, (i==0)?1:0);
			last_sel_objnum = sel_objnum;
		}
	}
}

int Missile_gun = 0;

//give up control of the guided missile
void release_guided_missile(int player_num)
{
	if (player_num == Player_num)
	 {
	  if (Guided_missile[player_num]==NULL)
			return;

		Missile_viewer = Guided_missile[player_num];
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
		 multi_send_guided_info (Guided_missile[Player_num],1);
#endif
		if (Newdemo_state==ND_STATE_RECORDING)
		 newdemo_record_guided_end();
	 }

	Guided_missile[player_num] = NULL;
}

int Proximity_dropped=0,Smartmines_dropped=0;

//	-------------------------------------------------------------------------------------------
//changed on 31/3/10 by kreatordxx to distinguish between drop bomb and secondary fire
void do_missile_firing(int drop_bomb)
{
	int gun_flag=0;
	int bomb = which_bomb();
	int weapon = (drop_bomb) ? bomb : Secondary_weapon;
	fix fire_frame_overhead = 0;

	Network_laser_track = -1;

	Assert(weapon < MAX_SECONDARY_WEAPONS);

	if (GameTime64 - Next_missile_fire_time <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that.
		fire_frame_overhead = GameTime64 - Next_missile_fire_time;

	if (Guided_missile[Player_num] && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num]) {
		release_guided_missile(Player_num);
		Next_missile_fire_time = GameTime64 + Weapon_info[Secondary_weapon_to_weapon_info[weapon]].fire_wait - fire_frame_overhead;
		return;
	}

	if (!Player_is_dead && (Players[Player_num].secondary_ammo[weapon] > 0))	{

		int weapon_index,weapon_gun;

		Players[Player_num].secondary_ammo[weapon]--;

		weapon_index = Secondary_weapon_to_weapon_info[weapon];

		if (!cheats.rapidfire)
			Next_missile_fire_time = GameTime64 + Weapon_info[weapon_index].fire_wait - fire_frame_overhead;
		else
			Next_missile_fire_time = GameTime64 + (F1_0/25) - fire_frame_overhead;

		weapon_gun = Secondary_weapon_to_gun_num[weapon];

		if (weapon_gun==4) {		//alternate left/right
			weapon_gun += (gun_flag = (Missile_gun & 1));
			Missile_gun++;
		}

		Laser_player_fire( ConsoleObject, weapon_index, weapon_gun, 1, 0);

		if (weapon == PROXIMITY_INDEX) {
			if (++Proximity_dropped == 4) {
				Proximity_dropped = 0;
#ifdef NETWORK
				maybe_drop_net_powerup(POW_PROXIMITY_WEAPON);
#endif
			}
		}
		else if (weapon == SMART_MINE_INDEX) {
			if (++Smartmines_dropped == 4) {
				Smartmines_dropped = 0;
#ifdef NETWORK
				maybe_drop_net_powerup(POW_SMART_MINE);
#endif
			}
		}
#ifdef NETWORK
		else if (weapon != CONCUSSION_INDEX)
			maybe_drop_net_powerup(Secondary_weapon_to_powerup[weapon]);
#endif

		if (weapon == MEGA_INDEX || weapon == SMISSILE5_INDEX) {
			vms_vector force_vec;

			force_vec.x = -(ConsoleObject->orient.fvec.x << 7);
			force_vec.y = -(ConsoleObject->orient.fvec.y << 7);
			force_vec.z = -(ConsoleObject->orient.fvec.z << 7);
			phys_apply_force(ConsoleObject, &force_vec);

			force_vec.x = (force_vec.x >> 4) + d_rand() - 16384;
			force_vec.y = (force_vec.y >> 4) + d_rand() - 16384;
			force_vec.z = (force_vec.z >> 4) + d_rand() - 16384;
			phys_apply_rot(ConsoleObject, &force_vec);
		}

#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_fire(weapon+MISSILE_ADJUST, 0, gun_flag, 1, Network_laser_track);
#endif

		// don't autoselect if dropping prox and prox not current weapon
		if (!drop_bomb || Secondary_weapon == bomb)
			auto_select_weapon(1);		//select next missile, if this one out of ammo
	}
}

