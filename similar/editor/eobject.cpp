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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Editor object functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "editor/eobject.h"

#include "objpage.h"
#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"
#include	"object.h"
#include "robot.h"
#include "game.h"
#include "ai.h"
#include "bm.h"
#include "3d.h"		//	For g3_point_to_vec
#include	"fvi.h"

#include "powerup.h"
#include "fuelcen.h"
#include "hostage.h"
#include "medrobot.h"
#include "player.h"
#include "gameseg.h"
#include "cntrlcen.h"

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "segiter.h"

#define	OBJ_SCALE		(F1_0/2)
#define	OBJ_DEL_SIZE	(F1_0/2)

#define ROTATION_UNIT (4096/4)

static void show_objects_in_segment(const vcsegptr_t sp)
{
	range_for (const auto i, objects_in(sp))
		(void)i;
}

//returns the number of the first object in a segment, skipping the player
static int get_first_object(const vsegptr_t seg)
{
	int id;

	id = seg->objects;

	if (id == (ConsoleObject-Objects))
		id = Objects[id].next;

	return id;
}

//returns the number of the next object in a segment, skipping the player
static int get_next_object(const vsegptr_t seg,objnum_t id)
{
	if (id==object_none || (id=Objects[id].next)==object_none)
		return get_first_object(seg);

	if (id == (ConsoleObject-Objects))
		return get_next_object(seg,id);

	return id;
}


//@@//	------------------------------------------------------------------------------------------------------
//@@// this should be called whenever the current segment may have changed
//@@//	If Cur_object_seg != Cursegp, then update various variables.
//@@// this used to be called update_due_to_new_segment()
//@@void ObjectUpdateCurrent(void)
//@@{
//@@	if (Cur_object_seg != Cursegp) {
//@@		Cur_object_seg = Cursegp;
//@@		Cur_object_index = get_first_object(Cur_object_seg);
//@@		Update_flags |= UF_WORLD_CHANGED;
//@@	}
//@@
//@@}

//	------------------------------------------------------------------------------------
int place_object(const vsegptridx_t segp, const vms_vector &object_pos, short object_type, short object_id)
{
	vms_matrix seg_matrix;

	med_extract_matrix_from_segment(segp,&seg_matrix);

	objptridx_t objnum = object_none;
	switch (object_type)
	{

		case OBJ_HOSTAGE:
		{
			objnum = obj_create(OBJ_HOSTAGE, -1, 
					segp,object_pos,&seg_matrix,HOSTAGE_SIZE,
					CT_NONE,MT_NONE,RT_HOSTAGE);

			if ( objnum == object_none)
				return 0;

			const vobjptridx_t obj = objnum;

			// Fill in obj->id and other hostage info
			hostage_init_info( objnum );
		
			obj->control_type = CT_POWERUP;
			
			obj->rtype.vclip_info.vclip_num = Hostage_vclip_num[object_id];
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
			obj->rtype.vclip_info.framenum = 0;
			break;
		}
		case OBJ_ROBOT:
		{
			objnum = obj_create(OBJ_ROBOT, object_id, segp, object_pos,
				&seg_matrix, Polygon_models[Robot_info[object_id].model_num].rad,
				CT_AI, MT_PHYSICS, RT_POLYOBJ);

			if ( objnum == object_none)
				return 0;

			const vobjptridx_t obj = objnum;

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;

			//set Physics info
		
			obj->mtype.phys_info.mass = Robot_info[get_robot_id(obj)].mass;
			obj->mtype.phys_info.drag = Robot_info[get_robot_id(obj)].drag;

			obj->mtype.phys_info.flags |= (PF_LEVELLING);

			obj->shields = Robot_info[get_robot_id(obj)].strength;

			{
				segnum_t hide_segment;
			if (Markedsegp)
				hide_segment = Markedsegp-Segments;
			else
				hide_segment = segment_none;
			//	robots which lunge forward to attack cannot have behavior type still.
			if (Robot_info[get_robot_id(obj)].attack_type)
				init_ai_object(obj, ai_behavior::AIB_NORMAL, hide_segment);
			else
				init_ai_object(obj, ai_behavior::AIB_STILL, hide_segment);
			}
			break;
		}
		case OBJ_POWERUP:
		{
			objnum = obj_create(OBJ_POWERUP, object_id,
					segp, object_pos, &seg_matrix, Powerup_info[object_id].size,
					CT_POWERUP, MT_NONE, RT_POWERUP);

			if ( objnum == object_none)
				return 0;

			const vobjptridx_t obj = objnum;

			//set powerup-specific data

			obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].play_time/Vclip[obj->rtype.vclip_info.vclip_num].num_frames;
			obj->rtype.vclip_info.framenum = 0;

			if (get_powerup_id(obj) == POW_VULCAN_WEAPON)
				obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
			else
				obj->ctype.powerup_info.count = 1;
			break;
		}
		case OBJ_CNTRLCEN: 
		{
			objnum = obj_create(OBJ_CNTRLCEN, object_id, segp, object_pos,
					&seg_matrix, Polygon_models[object_id].rad,
					CT_CNTRLCEN, MT_NONE, RT_POLYOBJ);

			if ( objnum == object_none)
				return 0;

			const vobjptridx_t obj = objnum;

			//Set polygon-object-specific data 
			obj->shields = 0;	// stored in Reactor_strength or calculated
#if defined(DXX_BUILD_DESCENT_I)
			obj->rtype.pobj_info.model_num = ObjId[object_type];
#elif defined(DXX_BUILD_DESCENT_II)
			obj->rtype.pobj_info.model_num = Reactors[object_id].model_num;
#endif
			obj->rtype.pobj_info.subobj_flags = 0;

			break;
		}
		case OBJ_PLAYER:	{
			objnum = obj_create(OBJ_PLAYER, object_id, segp, object_pos,
				&seg_matrix, Polygon_models[Player_ship->model_num].rad,
				CT_NONE, MT_PHYSICS, RT_POLYOBJ);

			if ( objnum == object_none)
				return 0;

			const vobjptridx_t obj = objnum;

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = Player_ship->model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//for (i=0;i<MAX_SUBMODELS;i++)
			//	vm_angvec_zero(&obj->rtype.pobj_info.anim_angles[i]);

			//set Physics info

			vm_vec_zero(obj->mtype.phys_info.velocity);
			obj->mtype.phys_info.mass = Player_ship->mass;
			obj->mtype.phys_info.drag = Player_ship->drag;
			obj->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE;
			obj->shields = i2f(100);
			break;
		}
		default:
			return 0;
		}

	Cur_object_index = objnum;
	//Cur_object_seg = Cursegp;

	show_objects_in_segment(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Count number of player objects, return value.
static int compute_num_players(void)
{
	int	count = 0;

	range_for (const auto i, highest_valid(Objects))
	{
		const auto &&objp = vcobjptr(static_cast<objnum_t>(i));
		if (objp->type == OBJ_PLAYER)
			count++;
	}

	return count;

}

int ObjectMakeCoop(void)
{
	Assert(Cur_object_index != object_none);
	Assert(Cur_object_index < MAX_OBJECTS);
//	Assert(Objects[Cur_object_index.type == OBJ_PLAYER);

	if (Objects[Cur_object_index].type == OBJ_PLAYER ) {
		Objects[Cur_object_index].type = OBJ_COOP;
		editor_status("You just made a player object COOPERATIVE");
	} else
		editor_status("This is not a player object");

	return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Place current object at center of current segment.
int ObjectPlaceObject(void)
{
	int	old_cur_object_index;
	int	rval;
	if (Cur_object_type == OBJ_PLAYER)
	{
		int num_players = compute_num_players();
		Assert(num_players <= MAX_MULTI_PLAYERS);
		if (num_players > MAX_PLAYERS)
			editor_status("You just placed a cooperative player object");
		if (num_players == MAX_MULTI_PLAYERS) {
			editor_status_fmt("Can't place player object.  Already %i players.", MAX_MULTI_PLAYERS);
			return -1;
		}
	}

	//update_due_to_new_segment();
	const auto cur_object_loc = compute_segment_center(Cursegp);

	old_cur_object_index = Cur_object_index;
	rval = place_object(Cursegp, cur_object_loc, Cur_object_type, Cur_object_id);

	if (old_cur_object_index != Cur_object_index)
		Objects[Cur_object_index].rtype.pobj_info.tmap_override = -1;

	return rval;

}

//	------------------------------------------------------------------------------------------------------
//	Place current object at center of current segment.
int ObjectPlaceObjectTmap(void)
{
	int	rval, old_cur_object_index;
	//update_due_to_new_segment();
	const auto cur_object_loc = compute_segment_center(Cursegp);

	old_cur_object_index = Cur_object_index;
	rval = place_object(Cursegp, cur_object_loc, Cur_object_type, Cur_object_id);

	if ((Cur_object_index != old_cur_object_index) && (Objects[Cur_object_index].render_type == RT_POLYOBJ))
		Objects[Cur_object_index].rtype.pobj_info.tmap_override = CurrentTexture;
	else
		editor_status("Unable to apply current texture map to this object.");
	
	return rval;
}

//	------------------------------------------------------------------------------------------------------
int ObjectSelectNextinSegment(void)
{
	segment *objsegp;


	//update_due_to_new_segment();

	//Assert(Cur_object_seg == Cursegp);

	if (Cur_object_index == object_none) {
		objsegp = Cursegp;
		Cur_object_index = objsegp->objects;
	} else {
		objsegp = Cursegp;
		if (Objects[Cur_object_index].segnum != Cursegp-Segments)
			Cur_object_index = objsegp->objects;
	}


	//Debug: make sure current object is in current segment
	objnum_t id;
	for (id=objsegp->objects;(id != Cur_object_index)  && (id != object_none);id=Objects[id].next);
	Assert(id == Cur_object_index);		//should have found object

	//	Select the next object, wrapping back to start if we are at the end of the linked list for this segment.
	if (id != object_none)
		Cur_object_index = get_next_object(objsegp,Cur_object_index);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;

}

//Moves to next object in the mine, skipping the player
int ObjectSelectNextInMine()
{	int i;
	for (i=0;i<MAX_OBJECTS;i++) {
		Cur_object_index++;
		if (Cur_object_index>= MAX_OBJECTS ) Cur_object_index= 0;

		if ((Objects[Cur_object_index ].type != OBJ_NONE) && (Cur_object_index != (ConsoleObject-Objects)) )	{
			Cursegp = &Segments[Objects[Cur_object_index ].segnum];
			med_create_new_segment_from_cursegp();
			//Cur_object_seg = Cursegp;
			return 1;
		}
	}
	Cur_object_index = object_none;

	Update_flags |= UF_WORLD_CHANGED;

	return 0;
}

//Moves to next object in the mine, skipping the player
int ObjectSelectPrevInMine()
{	int i;
	for (i=0;i<MAX_OBJECTS;i++) {
		if (!(Cur_object_index --))
			Cur_object_index = MAX_OBJECTS-1;

		if ((Objects[Cur_object_index ].type != OBJ_NONE) && (Cur_object_index != (ConsoleObject-Objects)) )	{
			Cursegp = &Segments[Objects[Cur_object_index ].segnum];
			med_create_new_segment_from_cursegp();
			//Cur_object_seg = Cursegp;
			return 1;
		}
	}
	Cur_object_index = object_none;

	Update_flags |= UF_WORLD_CHANGED;

	return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Delete current object, if it exists.
//	If it doesn't exist, reformat Matt's hard disk, even if he is in Boston.
int ObjectDelete(void)
{

	if (Cur_object_index != object_none) {
		auto delete_objnum = Cur_object_index;
		ObjectSelectNextinSegment();

		obj_delete(vobjptridx(delete_objnum));

		if (delete_objnum == Cur_object_index)
			Cur_object_index = object_none;

		Update_flags |= UF_WORLD_CHANGED;
	}

	return 1;
}

//	-----------------------------------------------------------------------------------------------------------------
//	Object has moved to another segment, (or at least poked through).
//	If still in mine, that is legal, so relink into new segment.
//	Return value:	0 = in mine, 1 = not in mine
static int move_object_within_mine(const vobjptridx_t obj, const vms_vector &newpos)
{
	range_for (const auto segnum, highest_valid(Segments))
	{
		const auto segp = vsegptridx(segnum);
		segmasks result = get_seg_masks(obj->pos, segp, 0);

		if (result.centermask == 0) {
			int	fate;
			fvi_info	hit_info;
			fvi_query fq;

			//	See if the radius pokes through any wall.
			fq.p0						= &obj->pos;
			fq.startseg				= obj->segnum;
			fq.p1						= &newpos;
			fq.rad					= obj->size;
			fq.thisobjnum			= object_none;
			fq.ignore_obj_list.first = nullptr;
			fq.flags					= 0;

			fate = find_vector_intersection(fq, hit_info);

			if (fate != HIT_WALL) {
				if ( segnum != obj->segnum )
					obj_relink( obj, segp);
				obj->pos = newpos;
				return 0;
			}
		}
	}

	return 1;

}


//	Return 0 if object is in expected segment, else return 1
static int verify_object_seg(const vobjptridx_t objp, const vms_vector &newpos)
{
	segmasks result = get_seg_masks(newpos, objp->segnum, objp->size);
	if (result.facemask == 0)
		return 0;
	else
		return move_object_within_mine(objp, newpos);
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveForward(void)
{
	vms_vector	fvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = vobjptridx(Cur_object_index);

	extract_forward_vector_from_segment(&Segments[obj->segnum], fvec);
	vm_vec_normalize(fvec);

	const auto newpos = vm_vec_add(obj->pos, vm_vec_scale(fvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveBack(void)
{
	vms_vector	fvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = vobjptridx(Cur_object_index);

	extract_forward_vector_from_segment(&Segments[obj->segnum], fvec);
	vm_vec_normalize(fvec);

	const auto newpos = vm_vec_sub(obj->pos, vm_vec_scale(fvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveLeft(void)
{
	vms_vector	rvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = vobjptridx(Cur_object_index);

	extract_right_vector_from_segment(&Segments[obj->segnum], rvec);
	vm_vec_normalize(rvec);

	const auto newpos = vm_vec_sub(obj->pos, vm_vec_scale(rvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveRight(void)
{
	vms_vector	rvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = vobjptridx(Cur_object_index);

	extract_right_vector_from_segment(&Segments[obj->segnum], rvec);
	vm_vec_normalize(rvec);

	const auto newpos = vm_vec_add(obj->pos, vm_vec_scale(rvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectSetDefault(void)
{
	//update_due_to_new_segment();

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	compute_segment_center(Objects[Cur_object_index].pos, &Segments[Objects[Cur_object_index].segnum]);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


//	------------------------------------------------------------------------------------------------------
int	ObjectMoveUp(void)
{
	vms_vector	uvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = vobjptridx(Cur_object_index);

	extract_up_vector_from_segment(&Segments[obj->segnum], uvec);
	vm_vec_normalize(uvec);

	const auto newpos = vm_vec_add(obj->pos, vm_vec_scale(uvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveDown(void)
{
	vms_vector	uvec;

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	auto obj = &Objects[Cur_object_index];

	extract_up_vector_from_segment(&Segments[obj->segnum], uvec);
	vm_vec_normalize(uvec);

	const auto newpos = vm_vec_sub(obj->pos, vm_vec_scale(uvec, OBJ_SCALE));

	if (!verify_object_seg(obj, newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------

static int rotate_object(const vobjptridx_t obj, int p, int b, int h)
{
	vms_angvec ang;
//	vm_extract_angles_matrix( &ang,&obj->orient);

//	ang.p += p;
//	ang.b += b;
//	ang.h += h;

	ang.p = p;
	ang.b = b;
	ang.h = h;

	const auto rotmat = vm_angles_2_matrix(ang);
	obj->orient = vm_matrix_x_matrix(obj->orient, rotmat);
//   vm_angles_2_matrix(&obj->orient, &ang);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

static void reset_object(const vobjptridx_t obj)
{
	med_extract_matrix_from_segment(&Segments[obj->segnum],&obj->orient);
}

int ObjectResetObject()
{
	reset_object(Cur_object_index);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


int ObjectFlipObject()
{
	vms_matrix *m=&Objects[Cur_object_index].orient;

	vm_vec_negate(m->uvec);
	vm_vec_negate(m->rvec);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

int ObjectDecreaseBank()		{return rotate_object(Cur_object_index, 0, -ROTATION_UNIT, 0);}
int ObjectIncreaseBank()		{return rotate_object(Cur_object_index, 0, ROTATION_UNIT, 0);}
int ObjectDecreasePitch()		{return rotate_object(Cur_object_index, -ROTATION_UNIT, 0, 0);}
int ObjectIncreasePitch()		{return rotate_object(Cur_object_index, ROTATION_UNIT, 0, 0);}
int ObjectDecreaseHeading()	{return rotate_object(Cur_object_index, 0, 0, -ROTATION_UNIT);}
int ObjectIncreaseHeading()	{return rotate_object(Cur_object_index, 0, 0, ROTATION_UNIT);}

int ObjectDecreaseBankBig()		{return rotate_object(Cur_object_index, 0, -(ROTATION_UNIT*4), 0);}
int ObjectIncreaseBankBig()		{return rotate_object(Cur_object_index, 0, (ROTATION_UNIT*4), 0);}
int ObjectDecreasePitchBig()		{return rotate_object(Cur_object_index, -(ROTATION_UNIT*4), 0, 0);}
int ObjectIncreasePitchBig()		{return rotate_object(Cur_object_index, (ROTATION_UNIT*4), 0, 0);}
int ObjectDecreaseHeadingBig()	{return rotate_object(Cur_object_index, 0, 0, -(ROTATION_UNIT*4));}
int ObjectIncreaseHeadingBig()	{return rotate_object(Cur_object_index, 0, 0, (ROTATION_UNIT*4));}

//	-----------------------------------------------------------------------------------------------------
//	Move object around based on clicks in 2d screen.
//	Slide an object parallel to the 2d screen, to a point on a vector.
//	The vector is defined by a point on the 2d screen and the eye.

//	V	=	vector from eye to 2d screen point.
//	E	=	eye
//	F	=	forward vector from eye
//	O	=	3-space location of object

//	D	=	depth of object given forward vector F
//		=	(OE dot norm(F))

//	Must solve intersection of:
//		E + tV		( equation of vector from eye through point on 2d screen)
//		Fs + D		( equation of plane parallel to 2d screen, at depth D)
//		=	Fx(Ex + tVx) + Fy(Ey + tVy) + Fz(Ez + tVz) + D = 0
//
//			      FxEx + FyEy + FzEz - D
//			t = - ----------------------
//					  VxFx + VyFy + VzFz

static void move_object_to_position(const vobjptridx_t objp, const vms_vector &newpos)
{
	segmasks result = get_seg_masks(newpos, objp->segnum, objp->size);

	if (result.facemask == 0) {
		objp->pos = newpos;
	} else {
		if (verify_object_seg(objp, newpos)) {
			int		fate;
			object	temp_viewer_obj;
			fvi_query fq;
			fvi_info	hit_info;

			temp_viewer_obj = *Viewer;
			auto viewer_segnum = find_object_seg(&temp_viewer_obj);
			temp_viewer_obj.segnum = viewer_segnum;

			//	If the viewer is outside the mine, get him in the mine!
			if (viewer_segnum == segment_none) {
				editor_status("Unable to move object, viewer not in mine.  Aborting");
				return;
#if 0
				vms_vector	last_outside_pos;
				//	While outside mine, move towards object
				count = 0;
				while (viewer_segnum == segment_none) {
					vms_vector	temp_vec;

					last_outside_pos = temp_viewer_obj.pos;

					vm_vec_avg(&temp_vec, &temp_viewer_obj.pos, newpos);
					temp_viewer_obj.pos = temp_vec;
					viewer_segnum = find_object_seg(&temp_viewer_obj);
					temp_viewer_obj.segnum = viewer_segnum;

					if (count > 5) {
						editor_status("Unable to move object, can't get viewer in mine.  Aborting");
						return;
					}
				}

				count = 0;
				//	While inside mine, move away from object.
				while (viewer_segnum != segment_none) {

					vms_vector	temp_vec;

					vm_vec_avg(&temp_vec, &temp_viewer_obj.pos, &last_outside_pos);
					temp_viewer_obj.pos = temp_vec;
					update_object_seg(&temp_viewer_obj);
					viewer_segnum = find_object_seg(&temp_viewer_obj);
					temp_viewer_obj.segnum = viewer_segnum;

					if (count > 5) {
						editor_status("Unable to move object, can't get viewer back out of mine.  Aborting");
						return;
					}
				}
#endif
			}

			fq.p0						= &temp_viewer_obj.pos;
			fq.startseg				= temp_viewer_obj.segnum;
			fq.p1						= &newpos;
			fq.rad					= temp_viewer_obj.size;
			fq.thisobjnum			= object_none;
			fq.ignore_obj_list.first = nullptr;
			fq.flags					= 0;

			fate = find_vector_intersection(fq, hit_info);
			if (fate == HIT_WALL) {

				objp->pos = hit_info.hit_pnt;
				auto new_segnum = find_object_seg(objp);
				Assert(new_segnum != segment_none);
				obj_relink(objp, new_segnum);
			} else {
				editor_status("Attempted to move object out of mine.  Object not moved.");
			}
		}
	}

	Update_flags |= UF_WORLD_CHANGED;
}

static void move_object_to_vector(const vms_vector &vec_through_screen, fix delta_distance)
{
	const auto result = vm_vec_scale_add(Viewer->pos, vec_through_screen, vm_vec_dist(Viewer->pos, Objects[Cur_object_index].pos) + delta_distance);
	move_object_to_position(vobjptridx(Cur_object_index), result);
}

static void move_object_to_mouse_click_delta(fix delta_distance)
{
	short			xcrd,ycrd;
	vms_vector	vec_through_screen;

	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return;
	}

	xcrd = GameViewBox->b1_drag_x1;
	ycrd = GameViewBox->b1_drag_y1;

	med_point_2_vec(&_canv_editor_game, vec_through_screen, xcrd, ycrd);

	move_object_to_vector(vec_through_screen, delta_distance);

}

void move_object_to_mouse_click(void)
{
	move_object_to_mouse_click_delta(0);
}

int	ObjectMoveNearer(void)
{
	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(-4*F1_0);		//	Move four units closer to eye

	const auto result = vm_vec_normalized(vm_vec_sub(Objects[Cur_object_index].pos, Viewer->pos));
	move_object_to_vector(result, -4*F1_0);

	return 1;	
}

int	ObjectMoveFurther(void)
{
	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(+4*F1_0);		//	Move four units further from eye

	const auto result = vm_vec_normalized(vm_vec_sub(Objects[Cur_object_index].pos, Viewer->pos));
	move_object_to_vector(result, 4*F1_0);

	return 1;	
}

