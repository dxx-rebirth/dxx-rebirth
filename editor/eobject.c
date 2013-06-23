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

#include "objpage.h"
#include "fix.h"
#include "dxxerror.h"
#include "kdefs.h"
#include	"object.h"
#include "polyobj.h"
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

#define	OBJ_SCALE		(F1_0/2)
#define	OBJ_DEL_SIZE	(F1_0/2)

#define ROTATION_UNIT (4096/4)

//segment		*Cur_object_seg = -1;

void show_objects_in_segment(segment *sp)
{
	short		objid;

	objid = sp->objects;
	while (objid != -1) {
		objid = Objects[objid].next;
	}
}

//returns the number of the first object in a segment, skipping the player
int get_first_object(segment *seg)
{
	int id;

	id = seg->objects;

	if (id == (ConsoleObject-Objects))
		id = Objects[id].next;

	return id;
}

//returns the number of the next object in a segment, skipping the player
int get_next_object(segment *seg,int id)
{
	if (id==-1 || (id=Objects[id].next)==-1)
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
int place_object(segment *segp, vms_vector *object_pos, short object_type, short object_id)
{
        short objnum=0;
	object *obj;
	vms_matrix seg_matrix;

	med_extract_matrix_from_segment(segp,&seg_matrix);

	switch (object_type)
	{

		case OBJ_HOSTAGE:

			objnum = obj_create(OBJ_HOSTAGE, -1, 
					segp-Segments,object_pos,&seg_matrix,HOSTAGE_SIZE,
					CT_NONE,MT_NONE,RT_HOSTAGE);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			// Fill in obj->id and other hostage info
			// hostage_init_info( objnum );	//don't need to anymore
		
			obj->control_type = CT_POWERUP;
			
			obj->rtype.vclip_info.vclip_num = Hostage_vclip_num[object_id];
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
			obj->rtype.vclip_info.framenum = 0;

			break;

		case OBJ_ROBOT:

			objnum = obj_create(OBJ_ROBOT, object_id, segp - Segments, object_pos,
				&seg_matrix, Polygon_models[Robot_info[object_id].model_num].rad,
				CT_AI, MT_PHYSICS, RT_POLYOBJ);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;

			//set Physics info
		
			obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
			obj->mtype.phys_info.drag = Robot_info[obj->id].drag;

			obj->mtype.phys_info.flags |= (PF_LEVELLING);

			obj->shields = Robot_info[obj->id].strength;

			{	int	hide_segment;
			if (Markedsegp)
				hide_segment = Markedsegp-Segments;
			else
				hide_segment = -1;
			//	robots which lunge forward to attack cannot have behavior type still.
			if (Robot_info[obj->id].attack_type)
				init_ai_object(obj-Objects, AIB_NORMAL, hide_segment);
			else
				init_ai_object(obj-Objects, AIB_STILL, hide_segment);
			}
			break;

		case OBJ_POWERUP:

			objnum = obj_create(OBJ_POWERUP, object_id,
					segp - Segments, object_pos, &seg_matrix, Powerup_info[object_id].size,
					CT_POWERUP, MT_NONE, RT_POWERUP);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			//set powerup-specific data

			obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].play_time/Vclip[obj->rtype.vclip_info.vclip_num].num_frames;
			obj->rtype.vclip_info.framenum = 0;

			if (obj->id == POW_VULCAN_WEAPON)
				obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
			else
				obj->ctype.powerup_info.count = 1;

			break;

		case OBJ_CNTRLCEN: 
		{
			objnum = obj_create(OBJ_CNTRLCEN, object_id, segp - Segments, object_pos,
					&seg_matrix, Polygon_models[object_id].rad,
					CT_CNTRLCEN, MT_NONE, RT_POLYOBJ);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			//Set polygon-object-specific data 
			obj->shields = 0;	// stored in Reactor_strength or calculated
			obj->rtype.pobj_info.model_num = Reactors[object_id].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;

			break;
		}

		case OBJ_PLAYER:	{
			objnum = obj_create(OBJ_PLAYER, object_id, segp - Segments, object_pos,
				&seg_matrix, Polygon_models[Player_ship->model_num].rad,
				CT_NONE, MT_PHYSICS, RT_POLYOBJ);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = Player_ship->model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//for (i=0;i<MAX_SUBMODELS;i++)
			//	vm_angvec_zero(&obj->rtype.pobj_info.anim_angles[i]);

			//set Physics info

			vm_vec_zero(&obj->mtype.phys_info.velocity);
			obj->mtype.phys_info.mass = Player_ship->mass;
			obj->mtype.phys_info.drag = Player_ship->drag;
			obj->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE;
			obj->shields = i2f(100);

			break;
		}
		default:
			break;	
		}

	Cur_object_index = objnum;
	//Cur_object_seg = Cursegp;

	show_objects_in_segment(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Count number of player objects, return value.
int compute_num_players(void)
{
	int	i, count = 0;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_PLAYER)
			count++;

	return count;

}

int ObjectMakeCoop(void)
{
	Assert(Cur_object_index != -1);
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

	vms_vector	cur_object_loc;

#ifdef SHAREWARE
	if (Cur_object_type == OBJ_PLAYER)
	{
		int num_players = compute_num_players();
		Assert(num_players <= MAX_PLAYERS);
		if (num_players == MAX_PLAYERS) {
			editor_status("Can't place player object.  Already %i players.", MAX_PLAYERS);
			return -1;
		}
	}
#endif

#ifndef SHAREWARE
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
#endif

	//update_due_to_new_segment();
	compute_segment_center(&cur_object_loc, Cursegp);

	old_cur_object_index = Cur_object_index;
	rval = place_object(Cursegp, &cur_object_loc, Cur_object_type, Cur_object_id);

	if (old_cur_object_index != Cur_object_index)
		Objects[Cur_object_index].rtype.pobj_info.tmap_override = -1;

	return rval;

}

//	------------------------------------------------------------------------------------------------------
//	Place current object at center of current segment.
int ObjectPlaceObjectTmap(void)
{
	int	rval, old_cur_object_index;
	vms_vector	cur_object_loc;

	//update_due_to_new_segment();
	compute_segment_center(&cur_object_loc, Cursegp);

	old_cur_object_index = Cur_object_index;
	rval = place_object(Cursegp, &cur_object_loc, Cur_object_type, Cur_object_id);

	if ((Cur_object_index != old_cur_object_index) && (Objects[Cur_object_index].render_type == RT_POLYOBJ))
		Objects[Cur_object_index].rtype.pobj_info.tmap_override = CurrentTexture;
	else
		editor_status("Unable to apply current texture map to this object.");
	
	return rval;
}

//	------------------------------------------------------------------------------------------------------
int ObjectSelectNextinSegment(void)
{
	int	id;
	segment *objsegp;


	//update_due_to_new_segment();

	//Assert(Cur_object_seg == Cursegp);

	if (Cur_object_index == -1) {
		objsegp = Cursegp;
		Cur_object_index = objsegp->objects;
	} else {
		objsegp = Cursegp;
		if (Objects[Cur_object_index].segnum != Cursegp-Segments)
			Cur_object_index = objsegp->objects;
	}


	//Debug: make sure current object is in current segment
	for (id=objsegp->objects;(id != Cur_object_index)  && (id != -1);id=Objects[id].next);
	Assert(id == Cur_object_index);		//should have found object

	//	Select the next object, wrapping back to start if we are at the end of the linked list for this segment.
	if (id != -1)
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
	Cur_object_index = -1;

	Update_flags |= UF_WORLD_CHANGED;

	return 0;
}

//Moves to next object in the mine, skipping the player
int ObjectSelectPrevInMine()
{	int i;
	for (i=0;i<MAX_OBJECTS;i++) {
		Cur_object_index--;
		if (Cur_object_index < 0 )
			Cur_object_index = MAX_OBJECTS-1;

		if ((Objects[Cur_object_index ].type != OBJ_NONE) && (Cur_object_index != (ConsoleObject-Objects)) )	{
			Cursegp = &Segments[Objects[Cur_object_index ].segnum];
			med_create_new_segment_from_cursegp();
			//Cur_object_seg = Cursegp;
			return 1;
		}
	}
	Cur_object_index = -1;

	Update_flags |= UF_WORLD_CHANGED;

	return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Delete current object, if it exists.
//	If it doesn't exist, reformat Matt's hard disk, even if he is in Boston.
int ObjectDelete(void)
{

	if (Cur_object_index != -1) {
		int delete_objnum;

		delete_objnum = Cur_object_index;

		ObjectSelectNextinSegment();

		obj_delete(delete_objnum);

		if (delete_objnum == Cur_object_index)
			Cur_object_index = -1;

		Update_flags |= UF_WORLD_CHANGED;
	}

	return 1;
}

//	-----------------------------------------------------------------------------------------------------------------
//	Object has moved to another segment, (or at least poked through).
//	If still in mine, that is legal, so relink into new segment.
//	Return value:	0 = in mine, 1 = not in mine
int move_object_within_mine(object * obj, vms_vector *newpos )
{
	int segnum;

	for (segnum=0;segnum <= Highest_segment_index; segnum++) {
		segmasks result = get_seg_masks(&obj->pos, segnum, 0, __FILE__, __LINE__);

		if (result.centermask == 0) {
			int	fate;
			fvi_info	hit_info;
			fvi_query fq;

			//	See if the radius pokes through any wall.
			fq.p0						= &obj->pos;
			fq.startseg				= obj->segnum;
			fq.p1						= newpos;
			fq.rad					= obj->size;
			fq.thisobjnum			= -1;
			fq.ignore_obj_list	= NULL;
			fq.flags					= 0;

			fate = find_vector_intersection(&fq,&hit_info);

			if (fate != HIT_WALL) {
				if ( segnum != obj->segnum )
					obj_relink( obj-Objects, segnum);
				obj->pos = *newpos;
				return 0;
			}
		}
	}

	return 1;

}


//	Return 0 if object is in expected segment, else return 1
int verify_object_seg(object *objp, vms_vector *newpos)
{
	segmasks result = get_seg_masks(newpos, objp->segnum, objp->size, __FILE__, __LINE__);

	if (result.facemask == 0)
		return 0;
	else
		return move_object_within_mine(objp, newpos);

}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveForward(void)
{
	object *obj;
	vms_vector	fvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_forward_vector_from_segment(&Segments[obj->segnum], &fvec);
	vm_vec_normalize(&fvec);

	vm_vec_add(&newpos, &obj->pos, vm_vec_scale(&fvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveBack(void)
{
	object *obj;
	vms_vector	fvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_forward_vector_from_segment(&Segments[obj->segnum], &fvec);
	vm_vec_normalize(&fvec);

	vm_vec_sub(&newpos, &obj->pos, vm_vec_scale(&fvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveLeft(void)
{
	object *obj;
	vms_vector	rvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_right_vector_from_segment(&Segments[obj->segnum], &rvec);
	vm_vec_normalize(&rvec);

	vm_vec_sub(&newpos, &obj->pos, vm_vec_scale(&rvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveRight(void)
{
	object *obj;
	vms_vector	rvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_right_vector_from_segment(&Segments[obj->segnum], &rvec);
	vm_vec_normalize(&rvec);

	vm_vec_add(&newpos, &obj->pos, vm_vec_scale(&rvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectSetDefault(void)
{
	//update_due_to_new_segment();

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	compute_segment_center(&Objects[Cur_object_index].pos, &Segments[Objects[Cur_object_index].segnum]);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


//	------------------------------------------------------------------------------------------------------
int	ObjectMoveUp(void)
{
	object *obj;
	vms_vector	uvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_up_vector_from_segment(&Segments[obj->segnum], &uvec);
	vm_vec_normalize(&uvec);

	vm_vec_add(&newpos, &obj->pos, vm_vec_scale(&uvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveDown(void)
{
	object *obj;
	vms_vector	uvec;
	vms_vector	newpos;

	if (Cur_object_index == -1) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	obj = &Objects[Cur_object_index];

	extract_up_vector_from_segment(&Segments[obj->segnum], &uvec);
	vm_vec_normalize(&uvec);

	vm_vec_sub(&newpos, &obj->pos, vm_vec_scale(&uvec, OBJ_SCALE));

	if (!verify_object_seg(obj, &newpos))
		obj->pos = newpos;

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMakeSmaller(void)
{
	fix	cur_size;

	//update_due_to_new_segment();

	cur_size = Objects[Cur_object_index].size;

	cur_size -= OBJ_DEL_SIZE;
	if (cur_size < OBJ_DEL_SIZE)
		cur_size = OBJ_DEL_SIZE;

	Objects[Cur_object_index].size = cur_size;
	
	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMakeLarger(void)
{
	fix	cur_size;

	//update_due_to_new_segment();

	cur_size = Objects[Cur_object_index].size;

	cur_size += OBJ_DEL_SIZE;

	Objects[Cur_object_index].size = cur_size;
	
	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------

int rotate_object(short objnum, int p, int b, int h)
{
	object *obj = &Objects[objnum];
	vms_angvec ang;
	vms_matrix rotmat,tempm;
	
//	vm_extract_angles_matrix( &ang,&obj->orient);

//	ang.p += p;
//	ang.b += b;
//	ang.h += h;

	ang.p = p;
	ang.b = b;
	ang.h = h;

	vm_angles_2_matrix(&rotmat, &ang);
	vm_matrix_x_matrix(&tempm, &obj->orient, &rotmat);
	obj->orient = tempm;

//   vm_angles_2_matrix(&obj->orient, &ang);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


void reset_object(short objnum)
{
	object *obj = &Objects[objnum];

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

	vm_vec_negate(&m->uvec);
	vm_vec_negate(&m->rvec);

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

void move_object_to_position(int objnum, vms_vector *newpos)
{
	object	*objp = &Objects[objnum];

	segmasks result = get_seg_masks(newpos, objp->segnum, objp->size, __FILE__, __LINE__);

	if (result.facemask == 0) {
		objp->pos = *newpos;
	} else {
		if (verify_object_seg(&Objects[objnum], newpos)) {
			int		fate, count;
			int		viewer_segnum;
			object	temp_viewer_obj;
			fvi_query fq;
			fvi_info	hit_info;
			vms_vector	last_outside_pos;

			temp_viewer_obj = *Viewer;
			viewer_segnum = find_object_seg(&temp_viewer_obj);
			temp_viewer_obj.segnum = viewer_segnum;

			//	If the viewer is outside the mine, get him in the mine!
			if (viewer_segnum == -1) {
				//	While outside mine, move towards object
				count = 0;
				while (viewer_segnum == -1) {
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
				while (viewer_segnum != -1) {

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
			}

			fq.p0						= &temp_viewer_obj.pos;
			fq.startseg				= temp_viewer_obj.segnum;
			fq.p1						= newpos;
			fq.rad					= temp_viewer_obj.size;
			fq.thisobjnum			= -1;
			fq.ignore_obj_list	= NULL;
			fq.flags					= 0;

			fate = find_vector_intersection(&fq,&hit_info);
			if (fate == HIT_WALL) {
				int	new_segnum;

				objp->pos = hit_info.hit_pnt;
				new_segnum = find_object_seg(objp);
				Assert(new_segnum != -1);
				obj_relink(objp-Objects, new_segnum);
			} else {
				editor_status("Attempted to move object out of mine.  Object not moved.");
			}
		}
	}

	Update_flags |= UF_WORLD_CHANGED;
}

void move_object_to_vector(vms_vector *vec_through_screen, fix delta_distance)
{
	vms_vector	result;

	vm_vec_scale_add(&result, &Viewer->pos, vec_through_screen, vm_vec_dist(&Viewer->pos,&Objects[Cur_object_index].pos)+delta_distance);

	move_object_to_position(Cur_object_index, &result);

}

void move_object_to_mouse_click_delta(fix delta_distance)
{
	short			xcrd,ycrd;
	vms_vector	vec_through_screen;

	if (Cur_object_index == -1) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return;
	}

	xcrd = GameViewBox->b1_drag_x1;
	ycrd = GameViewBox->b1_drag_y1;

	med_point_2_vec(&_canv_editor_game, &vec_through_screen, xcrd, ycrd);

	move_object_to_vector(&vec_through_screen, delta_distance);

}

void move_object_to_mouse_click(void)
{
	move_object_to_mouse_click_delta(0);
}

int	ObjectMoveNearer(void)
{
	vms_vector	result;

	if (Cur_object_index == -1) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(-4*F1_0);		//	Move four units closer to eye

	vm_vec_sub(&result, &Objects[Cur_object_index].pos, &Viewer->pos);
	vm_vec_normalize(&result);
	move_object_to_vector(&result, -4*F1_0);

	return 1;	
}

int	ObjectMoveFurther(void)
{
	vms_vector	result;

	if (Cur_object_index == -1) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(+4*F1_0);		//	Move four units further from eye

	vm_vec_sub(&result, &Objects[Cur_object_index].pos, &Viewer->pos);
	vm_vec_normalize(&result);
	move_object_to_vector(&result, 4*F1_0);

	return 1;	
}

