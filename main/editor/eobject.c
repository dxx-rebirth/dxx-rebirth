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
 * $Source: /cvs/cvsroot/d2x/main/editor/eobject.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Editor object functions.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:00  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:14  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.93  1995/02/22  15:09:04  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.92  1995/01/12  12:10:32  yuan
 * Added coop object capability.
 * 
 * Revision 1.91  1994/12/20  17:57:02  yuan
 * Multiplayer object stuff.
 * 
 * Revision 1.90  1994/11/27  23:17:49  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.89  1994/11/17  14:48:06  mike
 * validation functions moved from editor to game.
 * 
 * Revision 1.88  1994/11/14  11:40:03  mike
 * fix default robot behavior.
 * 
 * Revision 1.87  1994/10/25  10:51:31  matt
 * Vulcan cannon powerups now contain ammo count
 * 
 * Revision 1.86  1994/10/23  02:11:40  matt
 * Got rid of obsolete hostage_info stuff
 * 
 * Revision 1.85  1994/10/17  21:35:32  matt
 * Added support for new Control Center/Main Reactor
 * 
 * Revision 1.84  1994/10/10  17:23:13  mike
 * Verify that not placing too many player objects.
 * 
 * Revision 1.83  1994/09/24  14:15:35  mike
 * Custom colored object support.
 * 
 * Revision 1.82  1994/09/15  22:58:12  matt
 * Made new objects be oriented to their segment
 * Added keypad function to flip an object upside-down
 * 
 * Revision 1.81  1994/09/01  10:58:41  matt
 * Sizes for powerups now specified in bitmaps.tbl; blob bitmaps now plot
 * correctly if width & height of bitmap are different.
 * 
 * Revision 1.80  1994/08/25  21:58:14  mike
 * Write ObjectSelectPrevInMine and something else, I think...
 * 
 * Revision 1.79  1994/08/16  20:19:54  mike
 * Make STILL default (from CHASE_OBJECT).
 * 
 * Revision 1.78  1994/08/14  23:15:45  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 * 
 * Revision 1.77  1994/08/13  14:58:43  matt
 * Finished adding support for miscellaneous objects
 * 
 * Revision 1.76  1994/08/12  22:24:58  matt
 * Generalized polygon objects (such as control center)
 * 
 * Revision 1.75  1994/08/09  16:06:11  john
 * Added the ability to place players.  Made old
 * Player variable be ConsoleObject.
 * 
 * Revision 1.74  1994/08/05  18:18:55  matt
 * Made object rotation have 4x resolution, and SHIFT+rotate do old resolution.
 * 
 * Revision 1.73  1994/08/01  13:30:56  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: eobject.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "segment.h"
#include "editor.h"

#include "objpage.h"
#include "fix.h"
#include "mono.h"
#include "error.h"
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

#define	OBJ_SCALE		(F1_0/2)
#define	OBJ_DEL_SIZE	(F1_0/2)

#define ROTATION_UNIT (4096/4)

//segment		*Cur_object_seg = -1;

void show_objects_in_segment(segment *sp)
{
	short		objid;

	mprintf((0,"Objects in segment #%i: ",sp-Segments));

	objid = sp->objects;
	while (objid != -1) {
		mprintf((0,"%2i ",objid));
		objid = Objects[objid].next;
	}
	mprintf((0,"\n"));
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
int place_object(segment *segp, vms_vector *object_pos, int object_type)
{
        short objnum=0;
	object *obj;
	vms_matrix seg_matrix;

	med_extract_matrix_from_segment(segp,&seg_matrix);

	switch(ObjType[object_type]) {

		case OL_HOSTAGE:

			objnum = obj_create(OBJ_HOSTAGE, -1, 
					segp-Segments,object_pos,&seg_matrix,HOSTAGE_SIZE,
					CT_NONE,MT_NONE,RT_HOSTAGE);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			// Fill in obj->id and other hostage info
			hostage_init_info( objnum );
		
			obj->control_type = CT_POWERUP;
			
			obj->rtype.vclip_info.vclip_num = Hostage_vclip_num[ObjId[object_type]];
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
			obj->rtype.vclip_info.framenum = 0;

			break;

		case OL_ROBOT:

			objnum = obj_create(OBJ_ROBOT,ObjId[object_type],segp-Segments,object_pos,
				&seg_matrix,Polygon_models[Robot_info[ObjId[object_type]].model_num].rad,
				CT_AI,MT_PHYSICS,RT_POLYOBJ);

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

		case OL_POWERUP:

			objnum = obj_create(OBJ_POWERUP,ObjId[object_type],
					segp-Segments,object_pos,&seg_matrix,Powerup_info[ObjId[object_type]].size,
					CT_POWERUP,MT_NONE,RT_POWERUP);

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

		case OL_CLUTTER:
		case OL_CONTROL_CENTER: 
		{
			int obj_type,control_type;

			if (ObjType[object_type]==OL_CONTROL_CENTER) {
				obj_type = OBJ_CNTRLCEN;
				control_type = CT_CNTRLCEN;
			}
			else {
				obj_type = OBJ_CLUTTER;
				control_type = CT_NONE;
			}

			objnum = obj_create(obj_type,object_type,segp-Segments,object_pos,
					&seg_matrix,Polygon_models[ObjId[object_type]].rad,
					control_type,MT_NONE,RT_POLYOBJ);

			if ( objnum < 0 )
				return 0;

			obj = &Objects[objnum];

			obj->shields = ObjStrength[object_type];

			//Set polygon-object-specific data 
			obj->shields = ObjStrength[object_type];
			obj->rtype.pobj_info.model_num = ObjId[object_type];
			obj->rtype.pobj_info.subobj_flags = 0;

			break;
		}

		case OL_PLAYER:	{
			objnum = obj_create(OBJ_PLAYER,ObjId[object_type],segp-Segments,object_pos,
				&seg_matrix,Polygon_models[Player_ship->model_num].rad,
				CT_NONE,MT_PHYSICS,RT_POLYOBJ);

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

	show_objects_in_segment(Cursegp);		//mprintf the objects

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
	if (ObjType[Cur_robot_type] == OL_PLAYER) {
		int num_players = compute_num_players();
		Assert(num_players <= MAX_PLAYERS);
		if (num_players == MAX_PLAYERS) {
			editor_status("Can't place player object.  Already %i players.", MAX_PLAYERS);
			return -1;
		}
	}
#endif

#ifndef SHAREWARE
	if (ObjType[Cur_robot_type] == OL_PLAYER) {
		int num_players = compute_num_players();
		Assert(num_players <= MAX_MULTI_PLAYERS);
		if (num_players > MAX_PLAYERS)
			editor_status("You just placed a cooperative player object");
		if (num_players == MAX_MULTI_PLAYERS) {
			editor_status("Can't place player object.  Already %i players.", MAX_MULTI_PLAYERS);
			return -1;
		}
	}
#endif

	//update_due_to_new_segment();
	compute_segment_center(&cur_object_loc, Cursegp);

	old_cur_object_index = Cur_object_index;
	rval = place_object(Cursegp, &cur_object_loc, Cur_robot_type);

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
	rval = place_object(Cursegp, &cur_object_loc, Cur_robot_type);

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

	//mprintf((0,"Cur_object_index == %i\n", Cur_object_index));

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
		segmasks	result = get_seg_masks(&obj->pos,segnum,0);
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
			} //else
				//mprintf((0, "Hit wall seg:side = %i:%i\n", hit_info.hit_seg, hit_info.hit_side));
		}
	}

	return 1;

}


//	Return 0 if object is in expected segment, else return 1
int verify_object_seg(object *objp, vms_vector *newpos)
{
	segmasks	result = get_seg_masks(newpos, objp->segnum, objp->size);

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


//void print_vec(vms_vector *vec, char *text)
//{
//	mprintf((0, "%10s = %9.5f %9.5f %9.5f\n", text, f2fl(vec->x), f2fl(vec->y), f2fl(vec->z)));
//}
//
// void solve(vms_vector *result, vms_vector *E, vms_vector *V, vms_vector *O, vms_vector *F)
// {
// 	fix	t, D;
// 	vms_vector	Fnorm, Vnorm;
// 	fix			num, denom;
// 	// float			test_plane;
// 
// 	print_vec(E, "E");
// 	print_vec(V, "V");
// 	print_vec(O, "O");
// 	print_vec(F, "F");
// 
// 	Fnorm = *F;	vm_vec_normalize(&Fnorm);
// 	Vnorm = *V;	vm_vec_normalize(&Vnorm);
// 
// 	D = (fixmul(O->x, Fnorm.x) + fixmul(O->y, Fnorm.y) + fixmul(O->z, Fnorm.z));
// 	mprintf((0, "D = %9.5f\n", f2fl(D)));
// 
// 	num = fixmul(Fnorm.x, E->x) + fixmul(Fnorm.y, E->y) + fixmul(Fnorm.z, E->z) - D;
// 	denom = vm_vec_dot(&Vnorm, &Fnorm);
// 	t = - num/denom;
// 
// 	mprintf((0, "num = %9.5f, denom = %9.5f, t = %9.5f\n", f2fl(num), f2fl(denom), f2fl(t)));
// 
// 	result->x = E->x + fixmul(t, Vnorm.x);
// 	result->y = E->y + fixmul(t, Vnorm.y);
// 	result->z = E->z + fixmul(t, Vnorm.z);
// 
// 	print_vec(result, "result");
// 
// 	// test_plane = fixmul(result->x, Fnorm.x) + fixmul(result->y, Fnorm.y) + fixmul(result->z, Fnorm.z) - D;
// 	// if (abs(test_plane) > .001)
// 	// 	printf("OOPS: test_plane = %9.5f\n", test_plane);
// }

void move_object_to_position(int objnum, vms_vector *newpos)
{
	object	*objp = &Objects[objnum];

	segmasks	result = get_seg_masks(newpos, objp->segnum, objp->size);

	if (result.facemask == 0) {
		//mprintf((0, "Object #%i moved from (%7.3f %7.3f %7.3f) to (%7.3f %7.3f %7.3f)\n", objnum, f2fl(objp->pos.x), f2fl(objp->pos.y), f2fl(objp->pos.z), f2fl(newpos->x), f2fl(newpos->y), f2fl(newpos->z)));
		objp->pos = *newpos;
	} else {
		if (verify_object_seg(&Objects[objnum], newpos)) {
			int		fate, count;
			int		viewer_segnum;
			object	temp_viewer_obj;
			fvi_query fq;
			fvi_info	hit_info;
			vms_vector	last_outside_pos;
			vms_vector	last_inside_pos;

			temp_viewer_obj = *Viewer;
			viewer_segnum = find_object_seg(&temp_viewer_obj);
			temp_viewer_obj.segnum = viewer_segnum;

			//	If the viewer is outside the mine, get him in the mine!
			if (viewer_segnum == -1) {
				//	While outside mine, move towards object
				count = 0;
				while (viewer_segnum == -1) {
					vms_vector	temp_vec;

					//mprintf((0, "[towards %7.3f %7.3f %7.3f]\n", f2fl(temp_viewer_obj.pos.x), f2fl(temp_viewer_obj.pos.y), f2fl(temp_viewer_obj.pos.z)));
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

					//mprintf((0, "[away %7.3f %7.3f %7.3f]\n", f2fl(temp_viewer_obj.pos.x), f2fl(temp_viewer_obj.pos.y), f2fl(temp_viewer_obj.pos.z)));
					last_inside_pos = temp_viewer_obj.pos;

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

				//mprintf((0, "Hit wall seg:side = %i:%i, point = (%7.3f %7.3f %7.3f)\n", hit_info.hit_seg, hit_info.hit_side, f2fl(hit_info.hit_pnt.x), f2fl(hit_info.hit_pnt.y), f2fl(hit_info.hit_pnt.z)));
				objp->pos = hit_info.hit_pnt;
				new_segnum = find_object_seg(objp);
				Assert(new_segnum != -1);
				obj_relink(objp-Objects, new_segnum);
				//mprintf((0, "Object moved from segment %i to %i\n", old_segnum, objp->segnum));
			} else {
				editor_status("Attempted to move object out of mine.  Object not moved.");
				//mprintf((0,"Attempted to move object out of mine.  Object not moved."));
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

	//mprintf((0, "Mouse click at %i %i, vector = %7.3f %7.3f %7.3f\n", xcrd, ycrd, f2fl(vec_through_screen.x), f2fl(vec_through_screen.y), f2fl(vec_through_screen.z)));

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

