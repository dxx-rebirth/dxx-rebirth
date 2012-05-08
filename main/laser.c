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
 * This will contain the laser code
 *
 */

#include <stdlib.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "bm.h"
#include "object.h"
#include "laser.h"
#include "segment.h"
#include "fvi.h"
#include "segpoint.h"
#include "error.h"
#include "key.h"
#include "texmap.h"
#include "textures.h"
#include "render.h"
#include "vclip.h"
#include "fireball.h"
#include "polyobj.h"
#include "robot.h"
#include "weapon.h"
#include "timer.h"
#include "player.h"
#include "sounds.h"
#include "ai.h"
#include "powerup.h"
#include "multi.h"
#include "physics.h"
#include "hudmsg.h"

#define NEWHOMER

int Network_laser_track = -1;

int find_homing_object_complete(vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2);

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
int laser_are_related( int o1, int o2 )
{
	if ( (o1<0) || (o2<0) )
		return 0;

	// See if o2 is the parent of o1
	if ( Objects[o1].type == OBJ_WEAPON  )
		if ( (Objects[o1].ctype.laser_info.parent_num==o2) && (Objects[o1].ctype.laser_info.parent_signature==Objects[o2].signature) )
                 {
			//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
			if ((Objects[o1].id != PROXIMITY_ID) || (Objects[o1].ctype.laser_info.creation_time + F1_0*2 >= GameTime64)) {
				return 1;
			} else
				return 0;
                 }

	// See if o1 is the parent of o2
	if ( Objects[o2].type == OBJ_WEAPON  )
		if ( (Objects[o2].ctype.laser_info.parent_num==o1) && (Objects[o2].ctype.laser_info.parent_signature==Objects[o1].signature) )
			return 1;

	// They must both be weapons
	if ( Objects[o1].type != OBJ_WEAPON || Objects[o2].type != OBJ_WEAPON )
		return 0;

	//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
	// See if they're siblings...
	if ( Objects[o1].ctype.laser_info.parent_signature==Objects[o2].ctype.laser_info.parent_signature )
         {
		if (Objects[o1].id == PROXIMITY_ID  || Objects[o2].id == PROXIMITY_ID)
			return 0;		//if either is proximity, then can blow up, so say not related
		else
			return 1;
         }
	return 0;
}

//--unused-- int Muzzle_scale=2;
int Laser_offset=0;

void do_muzzle_stuff(int segnum, vms_vector *pos)
{
	Muzzle_data[Muzzle_queue_index].create_time = timer_query();
	Muzzle_data[Muzzle_queue_index].segnum = segnum;
	Muzzle_data[Muzzle_queue_index].pos = *pos;
	Muzzle_queue_index++;
	if (Muzzle_queue_index >= MUZZLE_QUEUE_MAX)
		Muzzle_queue_index = 0;
}

/*
 * In effort to reduce weapon fire traffic in Multiplayer games artificially decrease the fire rate down to 100ms between shots.
 * This will work for all weapons, even if game is modded.
 */
float weapon_rate_scale(int wp_id)
{
	if ( !(Game_mode & GM_MULTI) )
		return 1.0;
	if ( Weapon_info[wp_id].fire_wait >= f0_1 || Weapon_info[wp_id].fire_wait <= 0 )
		return 1.0;
	return (f0_1/Weapon_info[wp_id].fire_wait);
}

//---------------------------------------------------------------------------------
// Initializes a laser after Fire is pressed

//	Returns object number.
int Laser_create_new( vms_vector * direction, vms_vector * position, int segnum, int parent, int weapon_type, int make_sound )
{
	int objnum;
	object *obj;
	int rtype=-1;
	fix parent_speed, weapon_speed;
	fix volume;
	fix laser_radius = -1;
        fix laser_length = 0;

	Assert( weapon_type < N_weapon_types );

	if ( (weapon_type<0) || (weapon_type>=N_weapon_types) )
		weapon_type = 0;

	//	Don't let homing blobs make muzzle flash.
	if (Objects[parent].type == OBJ_ROBOT)
		do_muzzle_stuff(segnum, position);

	switch( Weapon_info[weapon_type].render_type )	{
	case WEAPON_RENDER_BLOB:
		rtype = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
		laser_radius = Weapon_info[weapon_type].blob_size;
		laser_length = 0;
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
		laser_length = 0;
		break;
	case WEAPON_RENDER_VCLIP:
		rtype = RT_WEAPON_VCLIP;
		laser_radius = Weapon_info[weapon_type].blob_size;
		laser_length = 0;
		break;
	default:
		Error( "Invalid weapon render type in Laser_create_new\n" );
	}

	// Add to object list
	Assert(laser_radius != -1);
	Assert(rtype != -1);
	objnum = obj_create( OBJ_WEAPON, weapon_type, segnum, position, NULL, laser_radius, CT_WEAPON, MT_PHYSICS, rtype );

	if ( objnum < 0 ) 	{
		Int3();
		return -1;
	}

	obj = &Objects[objnum];

	if (Objects[parent].type == OBJ_PLAYER) {
		if (weapon_type == FUSION_ID) {
			int	fusion_scale;

			if (Game_mode & GM_MULTI)
				fusion_scale = 2;
			else
				fusion_scale = 4;

			if (Fusion_charge <= 0)
				obj->ctype.laser_info.multiplier = F1_0;
			else if (Fusion_charge <= F1_0*fusion_scale)
				obj->ctype.laser_info.multiplier = F1_0 + Fusion_charge/2;
			else
				obj->ctype.laser_info.multiplier = F1_0*fusion_scale;

			//	Fusion damage was boosted by mk on 3/27 (for reg 1.1 release), but we only want it to apply to single player games.
			if (Game_mode & GM_MULTI)
				obj->ctype.laser_info.multiplier /= 2;
		} else if ((weapon_type == LASER_ID) && (Players[Objects[parent].id].flags & PLAYER_FLAGS_QUAD_LASERS))
			obj->ctype.laser_info.multiplier = F1_0*3/4;
	}


	//	This is strange:  Make children of smart bomb bounce so if they hit a wall right away, they
	//	won't detonate.  The frame interval code will clear this bit after 1/2 second.
	if ((weapon_type == PLAYER_SMART_HOMING_ID) || (weapon_type == ROBOT_SMART_HOMING_ID))
		obj->mtype.phys_info.flags |= PF_BOUNCE;

	if (Weapon_info[weapon_type].render_type == WEAPON_RENDER_POLYMODEL) {
		obj->rtype.pobj_info.model_num = Weapon_info[obj->id].model_num;
		laser_radius = fixdiv(Polygon_models[obj->rtype.pobj_info.model_num].rad,Weapon_info[obj->id].po_len_to_width_ratio);
		laser_length = Polygon_models[obj->rtype.pobj_info.model_num].rad * 2;
		obj->size = laser_radius;
	}

	obj->mtype.phys_info.mass = Weapon_info[weapon_type].mass;
	obj->mtype.phys_info.drag = Weapon_info[weapon_type].drag;
	if (Weapon_info[weapon_type].bounce)
		obj->mtype.phys_info.flags |= PF_BOUNCE;

	vm_vec_zero(&obj->mtype.phys_info.thrust);

	if (weapon_type == FLARE_ID)
		obj->mtype.phys_info.flags |= PF_STICK;		//this obj sticks to walls

	obj->shields = Weapon_info[obj->id].strength[Difficulty_level]*weapon_rate_scale(obj->id);

	// Fill in laser-specific data

	obj->lifeleft							= Weapon_info[obj->id].lifetime;
	obj->ctype.laser_info.parent_type		= Objects[parent].type;
	obj->ctype.laser_info.parent_signature = Objects[parent].signature;
	obj->ctype.laser_info.parent_num			= parent;

	//	Assign parent type to highest level creator.  This propagates parent type down from
	//	the original creator through weapons which create children of their own (ie, smart missile)
	if (Objects[parent].type == OBJ_WEAPON) {
		int	highest_parent = parent;

		while (Objects[highest_parent].type == OBJ_WEAPON) {
			highest_parent							= Objects[highest_parent].ctype.laser_info.parent_num;
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

//	Re-enable, 09/09/94:
	volume = F1_0;
	if (Weapon_info[obj->id].flash_sound > -1 )	{
		if (make_sound)	{
			if ( parent == (Viewer-Objects) )	{
				if (weapon_type == VULCAN_ID)			// Make your own vulcan gun  1/2 as loud.
					volume = F1_0 / 2;
				digi_play_sample( Weapon_info[obj->id].flash_sound, volume );
			} else {
				digi_link_sound_to_pos( Weapon_info[obj->id].flash_sound, obj->segnum, 0, &obj->pos, 0, volume );
			}
		}
	}

	//	WARNING! NOTE! HEY! DEBUG! ETC! --MK 10/26/94
	//	This is John's new code to fire the laser from the gun tip so that the back end of the laser bolt is
	//	at the gun tip.  A problem that needs to be fixed is that the laser bolt might be in another segment.
	//	Use find_vector_interesection to detect the segment.

	// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
	// This also jitters the laser a bit so that it doesn't alias.
	//	Don't do for weapons created by weapons.
	if ((Objects[parent].type != OBJ_WEAPON) && (Weapon_info[weapon_type].render_type != WEAPON_RENDER_NONE) && (weapon_type != FLARE_ID)) {
//	if ((Objects[parent].type != OBJ_WEAPON) && (weapon_type != FLARE_ID) ) {
		vms_vector	end_pos;
		int			end_segnum;

	 	vm_vec_scale_add( &end_pos, &obj->pos, direction, Laser_offset+(laser_length/2) );
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
        if (weapon_type == PROXIMITY_ID) {
		parent_speed = vm_vec_mag_quick(&Objects[parent].mtype.phys_info.velocity);
		if (vm_vec_dot(&Objects[parent].mtype.phys_info.velocity, &Objects[parent].orient.fvec) < 0)
			parent_speed = -parent_speed;
//                if(parent_speed>(F1_0*60))
//                 parent_speed=F1_0*1600;
/*                 { char *murp;
                   sprintf(murp,"%i.%i",parent_speed>>16,(parent_speed<<16)>>16);
                   nm_messagebox(NULL,1,"OK",murp);
                 }*/
        } else
		parent_speed = 0;

	weapon_speed = Weapon_info[obj->id].speed[Difficulty_level];

	//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
	if ((obj->id == PLAYER_SMART_HOMING_ID) || (obj->id == ROBOT_SMART_HOMING_ID))
		weapon_speed /= 4;

	if (Weapon_info[obj->id].thrust != 0)
		weapon_speed /= 2;

	vm_vec_copy_scale( &obj->mtype.phys_info.velocity, direction, weapon_speed + parent_speed );
////Debug 101594
//if ((vm_vec_mag(&obj->mtype.phys_info.velocity) == 0) && (obj->id != PROXIMITY_ID))
//	Int3();	//	Curious.  This weapon starts with a velocity of 0 and it's not a proximity bomb.

	//	Set thrust
	if (Weapon_info[weapon_type].thrust != 0) {
		obj->mtype.phys_info.thrust = obj->mtype.phys_info.velocity;
		vm_vec_scale(&obj->mtype.phys_info.thrust, fixdiv(Weapon_info[obj->id].thrust, weapon_speed+parent_speed));
	}

// THIS CODE MAY NOT BE NEEDED... it was used to move the lasers out of the gun, since the
// laser pos is acutally the head of the laser, and we want the tail to be at the starting
// point, not the head.
//	object_move_one( obj );
//	This next, apparently redundant line, appears necessary due to a hack in render.c
//	obj->lifeleft = Weapon_info[obj->id].lifetime;

	if ((obj->type == OBJ_WEAPON) && (obj->id == FLARE_ID))
                obj->lifeleft += (d_rand()-16384) << 2;           //      add in -2..2 seconds

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

fix	Min_trackable_dot = 3*(F1_0 - MIN_TRACKABLE_DOT)/4 + MIN_TRACKABLE_DOT; //MIN_TRACKABLE_DOT;

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
	if (objp->type == OBJ_ROBOT)
		if (objp->ctype.ai_info.CLOAKED)
			return 0;

	vm_vec_sub(&vector_to_goal, &objp->pos, &tracker->pos);
	vm_vec_normalize_quick(&vector_to_goal);
	*dot = vm_vec_dot(&vector_to_goal, &tracker->orient.fvec);

	if ((*dot < Min_trackable_dot) && (*dot > F1_0*9/10)) {
		vm_vec_normalize(&vector_to_goal);
		*dot = vm_vec_dot(&vector_to_goal, &tracker->orient.fvec);
	}

	if (*dot >= Min_trackable_dot) {
		int	rval;
		//	dot is in legal range, now see if object is visible
		rval =  object_to_object_visibility(tracker, objp, FQ_TRANSWALL);
		return rval;
	} else {
		return 0;
	}
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
int find_homing_object(vms_vector *curpos, object *tracker)
{
	int	i;
	fix	max_dot = -F1_0*2;
	int	best_objnum = -1;

	if (!Weapon_info[tracker->id].homing_flag) {
		Int3();		//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
		return 0;	//	Track the damn stupid player for causing this problem!
	}

	//	Find an object to track based on game mode (eg, whether in network play) and who fired it.
	if (Game_mode & GM_MULTI) {
		//	In network mode.
		if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER) {
			//	It's fired by a player, so if robots present, track robot, else track player.
			if (Game_mode & GM_MULTI_COOP)
				return find_homing_object_complete( curpos, tracker, OBJ_ROBOT, -1);
			else
				return find_homing_object_complete( curpos, tracker, OBJ_PLAYER, OBJ_ROBOT);
		} else {
			Assert(tracker->ctype.laser_info.parent_type == OBJ_ROBOT);
			return find_homing_object_complete(curpos, tracker, OBJ_PLAYER, -1);
		}
	}
	else {
		//	Not in network mode.  If not fired by player, then track player.
		if (tracker->ctype.laser_info.parent_num != Players[Player_num].objnum) {
			if (!(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
				best_objnum = ConsoleObject - Objects;
		} else {
			//	Not in network mode and fired by player.
			for (i=Num_rendered_objects-1; i>=0; i--) {
				fix			dot; //, dist;
				vms_vector	vec_to_curobj;
				int			objnum = Ordered_rendered_object_list[i];
				object		*curobjp = &Objects[objnum];

				if (objnum == Players[Player_num].objnum)
					continue;

				//	Can't track AI object if he's cloaked.
				if (curobjp->type == OBJ_ROBOT)
					if (curobjp->ctype.ai_info.CLOAKED)
						continue;

				vm_vec_sub(&vec_to_curobj, &curobjp->pos, curpos);
				vm_vec_normalize_quick(&vec_to_curobj);
				dot = vm_vec_dot(&vec_to_curobj, &tracker->orient.fvec);

				//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
				//	to determine if an object is initially trackable.  find_homing_object is called on subsequent
				//	frames to determine if the object remains trackable.
				if (dot > MIN_TRACKABLE_DOT) {
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

	return best_objnum;
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of objects.  If you are only interested in one type, set track_obj_type2 to NULL
int find_homing_object_complete(vms_vector *curpos, object *tracker, int track_obj_type1, int track_obj_type2)
{
	int	objnum;
	fix     max_dot = -F1_0*2;
	int	best_objnum = -1;

	fix max_trackable_dist = MAX_TRACKABLE_DIST;
	fix min_trackable_dot = MIN_TRACKABLE_DOT;

	if (!Weapon_info[tracker->id].homing_flag) {
		Int3();		//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
		return 0;	//	Track the damn stupid player for causing this problem!
	}

	for (objnum=0; objnum<=Highest_object_index; objnum++) {
		int		is_proximity = 0;
                fix             dot, dist;
		vms_vector	vec_to_curobj;
		object		*curobjp = &Objects[objnum];

		if ((curobjp->type != track_obj_type1) && (curobjp->type != track_obj_type2))
		{
			if ((curobjp->type == OBJ_WEAPON) && (curobjp->id == PROXIMITY_ID)) {
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
		if (curobjp->type == OBJ_ROBOT)
			if (curobjp->ctype.ai_info.CLOAKED)
				continue;

		vm_vec_sub(&vec_to_curobj, &curobjp->pos, curpos);
		dist = vm_vec_mag(&vec_to_curobj);

		if (dist < max_trackable_dist) {
			vm_vec_normalize(&vec_to_curobj);
			dot = vm_vec_dot(&vec_to_curobj, &tracker->orient.fvec);
			if (is_proximity)
				dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

			//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
			//	to determine if an object is initially trackable.  find_homing_object is called on subsequent
			//	frames to determine if the object remains trackable.
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
int track_track_goal(int track_goal, object *tracker, fix *dot)
{
#ifdef NEWHOMER
	if (object_is_trackable(track_goal, tracker, dot) && (tracker-Objects)) {
		return track_goal;
	} else if (tracker-Objects)
#else
	//	Every 8 frames for each object, scan all objects.
	if (object_is_trackable(track_goal, tracker, dot) && ((((tracker-Objects) ^ FrameCount) % 8) != 0)) {
		return track_goal;
	} else if ((((tracker-Objects) ^ FrameCount) % 4) == 0)
#endif
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
			int	goal_type;

			if (track_goal == -1)
				rval = find_homing_object_complete(&tracker->pos, tracker, OBJ_PLAYER, -1);
			else {
				goal_type = Objects[tracker->ctype.laser_info.track_goal].type;
				rval = find_homing_object_complete(&tracker->pos, tracker, goal_type, -1);
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
	fq.flags					= FQ_CHECK_OBJS;

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
	if (objnum == -1)
		return;

	//	If this weapon is supposed to be silent, set that bit!
	if (!make_sound)
		Objects[objnum].flags |= OF_SILENT;

        //      If this weapon is supposed to be harmless, set that bit!
	if (harmless)
		Objects[objnum].flags |= OF_HARMLESS;

	//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
	if (obj == ConsoleObject)
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

	energy_usage = Weapon_info[FLARE_ID].energy_usage*weapon_rate_scale(FLARE_ID);

	if (Difficulty_level < 2)
		energy_usage = fixmul(energy_usage, i2f(Difficulty_level+2)/4);

	if (Players[Player_num].energy > 0) {
		Players[Player_num].energy -= energy_usage;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
			auto_select_weapon(0);
		}

		Laser_player_fire( obj, FLARE_ID, 6, 1, 0);

		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_fire(FLARE_ID+MISSILE_ADJUST, 0, 0, 1, -1);
		#endif
	}

}

#define	HOMING_MISSILE_SCALE	8

//-------------------------------------------------------------------------------------------
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

#ifdef NEWHOMER
/* 
 * In the original game homers turned sharper in higher FPS-values. We do not want that so we need to scale vector_to_object to FrameTime.
 * For each difficulty setting we have a base value the homers will align to. This we express in a FPS value representing the homers turn radius of the original game (i.e. "The homer will turn like on XXFPS"). 
 * NOTE: Old homers only get valid track_goal every 8 frames. This does not apply anymore so these values are divided by 4 to compensate this.
 */
fix homing_turn_base[NDL] = { 4, 5, 6, 7, 8 };
#endif

//-------------------------------------------------------------------------------------------
//sequence this laser object for this _frame_ (underscores added here to aid MK in his searching!)
void Laser_do_weapon_sequence(object *obj)
{
	Assert(obj->control_type == CT_WEAPON);

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( Weapon_info[obj->id].damage_radius )
			explode_badass_weapon(obj);
		return;
	}

	//delete weapons that are not moving
	if (	!((FrameCount ^ obj->signature) & 3) &&
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

	//	For homing missiles, turn towards target.
	if (Weapon_info[obj->id].homing_flag) {
		vms_vector		vector_to_object, temp_vec;
		fix				dot=F1_0;
		fix				speed, max_speed;

		//	For first 1/2 second of life, missile flies straight.
		if (obj->ctype.laser_info.creation_time + HOMING_MISSILE_STRAIGHT_TIME < GameTime64) {

			int	track_goal = obj->ctype.laser_info.track_goal;

			//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
			if ((obj->id == ROBOT_SMART_HOMING_ID) || (obj->id == PLAYER_SMART_HOMING_ID)) {
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
				vm_vec_sub(&vector_to_object, &Objects[track_goal].pos, &obj->pos);

				vm_vec_normalize_quick(&vector_to_object);
				temp_vec = obj->mtype.phys_info.velocity;
				speed = vm_vec_normalize(&temp_vec);
				max_speed = Weapon_info[obj->id].speed[Difficulty_level];
				if (speed+F1_0 < max_speed) {
					speed += fixmul(max_speed, FrameTime/2);
					if (speed > max_speed)
						speed = max_speed;
				}

				// Scale vector to object to current FrameTime.
				vm_vec_scale(&vector_to_object, F1_0/((float)(F1_0/homing_turn_base[Difficulty_level])/FrameTime));

				vm_vec_add2(&temp_vec, &vector_to_object);
				//	The boss' smart children track better...
				if (Weapon_info[obj->id].render_type != WEAPON_RENDER_POLYMODEL)
					vm_vec_add2(&temp_vec, &vector_to_object);
				vm_vec_normalize(&temp_vec);
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
					homing_missile_turn_towards_velocity(obj, &temp_vec);		//	temp_vec is normalized velocity.
#else // OLD - ORIGINAL - MISSILE TRACKING CODE
				vm_vec_sub(&vector_to_object, &Objects[track_goal].pos, &obj->pos);

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

					if (absdot > F1_0/8) {
						if (absdot > F1_0/4)
							absdot = F1_0/4;
						lifelost = fixmul(absdot*16, FrameTime);
						obj->lifeleft -= lifelost;
					}
					//added 8/14/98 by Victor Rachels to make homers lose life while going straight, too
					obj->lifeleft -= fixmul(F1_0, FrameTime);
					//end addition - Victor Rachels
				}

				//	Only polygon objects have visible orientation, so only they should turn.
				if (Weapon_info[obj->id].render_type == WEAPON_RENDER_POLYMODEL)
					homing_missile_turn_towards_velocity(obj, &temp_vec);		//	temp_vec is normalized velocity.
#endif
			}
		}
	}

	//	Make sure weapon is not moving faster than allowed speed.
	if (Weapon_info[obj->id].thrust != 0) {
		fix	weapon_speed;

		weapon_speed = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
		if (weapon_speed > Weapon_info[obj->id].speed[Difficulty_level]) {
			fix	scale_factor;

			scale_factor = fixdiv(Weapon_info[obj->id].speed[Difficulty_level], weapon_speed);
			vm_vec_scale(&obj->mtype.phys_info.velocity, scale_factor);
		}

	}
}

int	Spreadfire_toggle=0;
fix64	Last_laser_fired_time = 0;

extern int Player_fired_laser_this_frame;

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for
//   network players

int do_laser_firing_player(void)
{
	player	*plp = &Players[Player_num];
	fix		energy_used;
	int		ammo_used;
	int		weapon_index;
	int		rval = 0;
	int 		nfires = 1;
//        fix             addval;

	if (Player_is_dead)
		return 0;

	weapon_index = Primary_weapon_to_weapon_info[Primary_weapon];
	energy_used = Weapon_info[weapon_index].energy_usage*weapon_rate_scale(weapon_index);

	if (Difficulty_level < 2)
		energy_used = fixmul(energy_used, i2f(Difficulty_level+2)/4);

	ammo_used = Weapon_info[weapon_index].ammo_usage*weapon_rate_scale(weapon_index);

//        addval = 2*FrameTime;
//        if (addval > F1_0)
//                addval = F1_0;

	if (Last_laser_fired_time + 2*FrameTime < GameTime64)
		Next_laser_fire_time = GameTime64;

        while (Next_laser_fire_time <= GameTime64) {
		if	((plp->energy >= energy_used) || ((Primary_weapon == VULCAN_INDEX) && (plp->primary_ammo[Primary_weapon] >= ammo_used)) ) {
			int	laser_level, flags;

                        //added/moved on 8/16/98 by Victor Rachels (also from Arne de Bruijn) to fix EBlB
                        Last_laser_fired_time = GameTime64;
                        //end move - Victor Rachels

			if (!cheats.rapidfire)
				Next_laser_fire_time += Weapon_info[weapon_index].fire_wait*weapon_rate_scale(weapon_index);
			else
				Next_laser_fire_time += F1_0/25;

			laser_level = Players[Player_num].laser_level;

			flags = 0;

			if (Primary_weapon == SPREADFIRE_INDEX) {
				if (Spreadfire_toggle)
					flags |= LASER_SPREADFIRE_TOGGLED;
				Spreadfire_toggle = !Spreadfire_toggle;
			}

			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
				flags |= LASER_QUAD;

			rval += do_laser_firing(Players[Player_num].objnum, Primary_weapon, laser_level, flags, nfires);

			plp->energy -= (energy_used * rval) / Weapon_info[weapon_index].fire_count;
			if (plp->energy < 0)
				plp->energy = 0;

                        if (ammo_used > plp->primary_ammo[Primary_weapon])
				plp->primary_ammo[Primary_weapon] = 0;
			else
				plp->primary_ammo[Primary_weapon] -= ammo_used;

			auto_select_weapon(0);		//	Make sure the player can fire from this weapon.

		} else
                   break;  //      Couldn't fire weapon, so abort.
	}

	Global_laser_firing_count = 0;

	return rval;
}

//	--------------------------------------------------------------------------------------------------
//	Object "objnum" fires weapon "weapon_num" of level "level".  (Right now (9/24/94) level is used only for type 0 laser.
//	Flags are the player flags.  For network mode, set to 0.
//	It is assumed that this is a player object (as in multiplayer), and therefore the gun positions are known.
//	Returns number of times a weapon was fired.  This is typically 1, but might be more for low frame rates.
//      More than one shot is fired with a pseudo-delay so that players on slow machines can fire (for themselves
//	or other players) often enough for things like the vulcan cannon.
int do_laser_firing(int objnum, int weapon_num, int level, int flags, int nfires)
{
	object	*objp = &Objects[objnum];

	switch (weapon_num) {
		case LASER_INDEX: {
			// The Laser_offset is used to "jitter" the laser fire so that lasers don't always appear
			// right in front of your face.   I put it here instead of laser_create_new because I want
			// both of the dual laser beams to be fired from the same distance.
			Laser_offset = ((F1_0*2)*(d_rand()%8))/8;

			Laser_player_fire( objp, level, 0, 1, 0);
			Laser_player_fire( objp, level, 1, 0, 0);

			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				Laser_player_fire( objp, level, 2, 0, 0);
				Laser_player_fire( objp, level, 3, 0, 0);
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

#ifndef SHAREWARE
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
#endif

		default:
			Int3();	//	Contact Mike: Unknown Primary weapon type, setting to 0.
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
#define	NUM_SMART_CHILDREN	6

typedef	struct {
	int	objnum;
	fix	dist;
} objdist;

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
		vm_vec_sub(&vector_to_goal, &Objects[goal_obj].pos, &objp->pos);
		vm_vec_normalize_quick(&vector_to_goal);
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

//	-------------------------------------------------------------------------------------------
//	Create the children of a smart bomb, which is a bunch of homing missiles.
void create_smart_children(object *objp)
{
	int parent_type;
	int numobjs=0, objnum = 0, sel_objnum, last_sel_objnum = -1;
	int objlist[MAX_OBJDISTS];
	int blob_id;

	parent_type = objp->ctype.laser_info.parent_type;

	if (objp->id == SMART_ID) {
		int i;

		if (Game_mode & GM_MULTI)
			d_srand(8321L);

		for (objnum=0; objnum<=Highest_object_index; objnum++) {
			object *curobjp = &Objects[objnum];

			if ((((curobjp->type == OBJ_ROBOT) && (!curobjp->ctype.ai_info.CLOAKED)) || (curobjp->type == OBJ_PLAYER)) && (objnum != objp->ctype.laser_info.parent_num)) {
				fix dist;

				if (curobjp->type == OBJ_PLAYER)
				{
					if ((parent_type == OBJ_PLAYER) && (Game_mode & GM_MULTI_COOP))
						continue;
					if ((Game_mode & GM_TEAM) && (get_team(curobjp->id) == get_team(Objects[objp->ctype.laser_info.parent_num].id)))
						continue;
					if (Players[curobjp->id].flags & PLAYER_FLAGS_CLOAKED)
						continue;
				}

				//	Robot blobs can't track robots.
				if (curobjp->type == OBJ_ROBOT)
					if (parent_type == OBJ_ROBOT)
						continue;

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
		if (parent_type == OBJ_PLAYER) {
			blob_id = PLAYER_SMART_HOMING_ID;
			Assert(blob_id != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
		} else {
			Assert(objp->type == OBJ_ROBOT);
			blob_id = ROBOT_SMART_HOMING_ID;
		}

		for (i=0; i<NUM_SMART_CHILDREN; i++) {
			sel_objnum = (numobjs==0)?-1:objlist[(d_rand() * numobjs) >> 15];
			if (numobjs > 1)
				while (sel_objnum == last_sel_objnum)
					sel_objnum = objlist[(d_rand() * numobjs) >> 15];
			create_homing_missile(objp, sel_objnum, blob_id, (i==0)?1:0);
			last_sel_objnum = sel_objnum;
		}
	}
}

#define	CONCUSSION_GUN		4
#define	HOMING_GUN			4

#define	PROXIMITY_GUN		7
#define	SMART_GUN			7
#define	MEGA_GUN				7


int Missile_gun=0, Proximity_dropped = 0;

//	-------------------------------------------------------------------------------------------
//changed on 9/16/98 by adb to distinguish between drop bomb and secondary fire
void do_missile_firing(int drop_bomb)
{
	int weapon = (drop_bomb) ? PROXIMITY_INDEX : Secondary_weapon;
	
	Network_laser_track = -1;

	Assert(weapon < MAX_SECONDARY_WEAPONS);

	if (!Player_is_dead && (Players[Player_num].secondary_ammo[weapon] > 0))      {

		int	weapon_index;

		Players[Player_num].secondary_ammo[weapon]--;

		weapon_index = Secondary_weapon_to_weapon_info[weapon];
		if (!cheats.rapidfire)
			Next_missile_fire_time = GameTime64 + Weapon_info[weapon_index].fire_wait*weapon_rate_scale(weapon_index);
		else
			Next_missile_fire_time = GameTime64 + F1_0/25;

		switch (weapon) {
			case CONCUSSION_INDEX:
				Laser_player_fire( ConsoleObject, CONCUSSION_ID, CONCUSSION_GUN+(Missile_gun & 1), 1, 0 );
				Missile_gun++;
				break;

			case PROXIMITY_INDEX:
				Proximity_dropped ++;
				if (Proximity_dropped == 4)
				{
					Proximity_dropped = 0;
					#ifdef NETWORK
					maybe_drop_net_powerup(POW_PROXIMITY_WEAPON);
					#endif
				}
				Laser_player_fire( ConsoleObject, PROXIMITY_ID, PROXIMITY_GUN, 1, 0);
				break;

			case HOMING_INDEX:
				Laser_player_fire( ConsoleObject, HOMING_ID, HOMING_GUN+(Missile_gun & 1), 1, 0 );
				Missile_gun++;
				#ifdef NETWORK
				maybe_drop_net_powerup(POW_HOMING_AMMO_1);
				#endif
				break;

#ifndef SHAREWARE
			case SMART_INDEX:
				Laser_player_fire( ConsoleObject, SMART_ID, SMART_GUN, 1, 0);
				#ifdef NETWORK
				maybe_drop_net_powerup(POW_SMARTBOMB_WEAPON);
				#endif
				break;

			case MEGA_INDEX:
				Laser_player_fire( ConsoleObject, MEGA_ID, MEGA_GUN, 1, 0);
				#ifdef NETWORK
				maybe_drop_net_powerup(POW_MEGA_WEAPON);
				#endif

				{ vms_vector force_vec;
				force_vec.x = -(ConsoleObject->orient.fvec.x << 7);
				force_vec.y = -(ConsoleObject->orient.fvec.y << 7);
				force_vec.z = -(ConsoleObject->orient.fvec.z << 7);
				phys_apply_force(ConsoleObject, &force_vec);

                                force_vec.x = (force_vec.x >> 4) + d_rand() - 16384;
                                force_vec.y = (force_vec.y >> 4) + d_rand() - 16384;
                                force_vec.z = (force_vec.z >> 4) + d_rand() - 16384;
				phys_apply_rot(ConsoleObject, &force_vec);
				}
				break;
#endif
		}

		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_fire(weapon+MISSILE_ADJUST, 0, (Missile_gun-1), 1, Network_laser_track);
		#endif

		// don't autoselect if dropping prox and prox not current weapon
		if (!drop_bomb || Secondary_weapon == PROXIMITY_INDEX)
			auto_select_weapon(1);		//select next missile, if this one out of ammo
	}
}

#ifdef NETWORK
void net_missile_firing(int player, int gun, int flags)
{

	switch (gun-MISSILE_ADJUST) {
		case CONCUSSION_INDEX:
			Laser_player_fire( Objects+Players[player].objnum, CONCUSSION_ID, CONCUSSION_GUN+(flags & 1), 1, 0 );
			break;

		case PROXIMITY_INDEX:
			Laser_player_fire( Objects+Players[player].objnum, PROXIMITY_ID, PROXIMITY_GUN, 1, 0);
			break;

		case HOMING_INDEX:
			Laser_player_fire( Objects+Players[player].objnum, HOMING_ID, HOMING_GUN+(flags & 1), 1, 0);
			break;

		case SMART_INDEX:
			Laser_player_fire( Objects+Players[player].objnum, SMART_ID, SMART_GUN, 1, 0);
			break;

		case MEGA_INDEX:
			Laser_player_fire( Objects+Players[player].objnum, MEGA_ID, MEGA_GUN, 1, 0);
			break;

		case FLARE_ID:
			Laser_player_fire( Objects+Players[player].objnum, FLARE_ID, 6, 1, 0);
			break;

		default:
			break;
	}

}
#endif



