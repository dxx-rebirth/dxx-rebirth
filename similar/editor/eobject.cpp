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

#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"
#include "object.h"
#include "robot.h"
#include "game.h"
#include "bm.h"
#include "3d.h"		//	For g3_point_to_vec
#include "fvi.h"
#include "vclip.h"

#include "powerup.h"
#include "hostage.h"
#include "player.h"
#include "gameseg.h"
#include "cntrlcen.h"

#include "d_construct.h"
#include "d_levelstate.h"

#define	OBJ_SCALE		(F1_0/2)
#define	OBJ_DEL_SIZE	(F1_0/2)

//returns the number of the first object in a segment, skipping the player
static objnum_t get_first_object(fvcobjptr &vcobjptr, const unique_segment &seg)
{
	const auto id{seg.objects};
	if (id == object_none)
		return object_none;
	auto &o = *vcobjptr(id);
	if (&o == ConsoleObject)
		return o.next;
	return id;
}

//returns the number of the next object in a segment, skipping the player
static objnum_t get_next_object(const unique_segment &seg, objnum_t id)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	if (id == object_none)
		return get_first_object(vcobjptr, seg);
	for (auto o = vmobjptr(id);;)
	{
		id = o->next;
		if (id == object_none)
			return get_first_object(vcobjptr, seg);
		o = vmobjptr(id);
		if (o != ConsoleObject)
			return id;
	}
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

namespace dsx {

//	------------------------------------------------------------------------------------
int place_object(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_polygon_model_state &LevelSharedPolygonModelState, const d_robot_info_array &Robot_info, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const vmsegptridx_t segp, const vms_vector &object_pos, short object_type, const uint8_t object_id)
{
	auto seg_matrix{med_extract_matrix_from_segment(segp)};

	imobjptridx_t objnum = object_none;
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	switch (object_type)
	{

		case OBJ_HOSTAGE:
		{
			objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_HOSTAGE, -1, 
					segp,object_pos,&seg_matrix,HOSTAGE_SIZE,
					object::control_type::None, object::movement_type::None, render_type::RT_HOSTAGE);

			if ( objnum == object_none)
				return 0;

			const vmobjptridx_t obj = objnum;

			// Fill in obj->id and other hostage info
			obj->id = 0;

			obj->control_source = object::control_type::powerup;
			
			obj->rtype.vclip_info.vclip_num = Hostage_vclip_num[object_id];
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
			obj->rtype.vclip_info.framenum = 0;
			break;
		}
		case OBJ_ROBOT:
		{
			segnum_t hide_segment;
			if (Markedsegp)
				hide_segment = Markedsegp;
			else
				hide_segment = segment_none;

			const robot_id rid{object_id};
			auto &ri = Robot_info[rid];
			objnum = robot_create(Robot_info, rid, segp, object_pos,
								&seg_matrix, Polygon_models[ri.model_num].rad,
								ri.attack_type ?
								//	robots which lunge forward to attack cannot have behavior type still.
								ai_behavior::AIB_NORMAL :
								ai_behavior::AIB_STILL,
								hide_segment);

			if ( objnum == object_none)
				return 0;

			const vmobjptridx_t obj = objnum;

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = ri.model_num;
			obj->rtype.pobj_info.subobj_flags = 0;

			//set Physics info
		
			obj->mtype.phys_info.mass = ri.mass;
			obj->mtype.phys_info.drag = ri.drag;

			obj->mtype.phys_info.flags |= (PF_LEVELLING);

			obj->shields = ri.strength;
			break;
		}
		case OBJ_POWERUP:
		{
			objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_POWERUP, object_id,
					segp, object_pos, &seg_matrix, Powerup_info[(powerup_type_t{object_id})].size,
					object::control_type::powerup, object::movement_type::None, render_type::RT_POWERUP);

			if ( objnum == object_none)
				return 0;

			const vmobjptridx_t obj = objnum;

			//set powerup-specific data

			obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
			obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].play_time/Vclip[obj->rtype.vclip_info.vclip_num].num_frames;
			obj->rtype.vclip_info.framenum = 0;

			if (get_powerup_id(obj) == powerup_type_t::POW_VULCAN_WEAPON)
				obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
			else
				obj->ctype.powerup_info.count = 1;
			break;
		}
		case OBJ_CNTRLCEN: 
		{
			const auto model_num =
#if DXX_BUILD_DESCENT == 1
			ObjId[object_type];
#elif DXX_BUILD_DESCENT == 2
			Reactors[object_id].model_num;
#endif
			objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_CNTRLCEN, object_id, segp, object_pos,
					&seg_matrix, Polygon_models[model_num].rad,
					object::control_type::cntrlcen, object::movement_type::None, render_type::RT_POLYOBJ);

			if ( objnum == object_none)
				return 0;

			const vmobjptridx_t obj = objnum;

			//Set polygon-object-specific data 
			obj->shields = 0;	// stored in Reactor_strength or calculated
			obj->rtype.pobj_info.model_num = model_num;
			obj->rtype.pobj_info.subobj_flags = 0;

			break;
		}
		case OBJ_PLAYER:	{
			objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_PLAYER, object_id, segp, object_pos,
				&seg_matrix, Polygon_models[Player_ship->model_num].rad,
				object::control_type::None, object::movement_type::physics, render_type::RT_POLYOBJ);

			if ( objnum == object_none)
				return 0;

			const vmobjptridx_t obj = objnum;

			//Set polygon-object-specific data 

			obj->rtype.pobj_info.model_num = Player_ship->model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//for (i=0;i<MAX_SUBMODELS;i++)
			//	vm_angvec_zero(&obj->rtype.pobj_info.anim_angles[i]);

			//set Physics info

			obj->mtype.phys_info.velocity = {};
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

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Count number of player objects, return value.
static int compute_num_players(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	int	count = 0;

	for (auto &obj : vcobjptr)
	{
		if (obj.type == OBJ_PLAYER)
			count++;
	}

	return count;

}

int ObjectMakeCoop(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	Assert(Cur_object_index != object_none);
	Assert(Cur_object_index < MAX_OBJECTS);
//	Assert(Objects[Cur_object_index.type == OBJ_PLAYER);

	const auto &&objp = vmobjptr(Cur_object_index);
	if (objp->type == OBJ_PLAYER)
	{
		objp->type = OBJ_COOP;
		editor_status("You just made a player object COOPERATIVE");
	} else
		editor_status("This is not a player object");

	return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Place current object at center of current segment.
int ObjectPlaceObject(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int	old_cur_object_index;
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
	auto &vcvertptr = Vertices.vcptr;
	const auto cur_object_loc{compute_segment_center(vcvertptr, Cursegp)};

	old_cur_object_index = Cur_object_index;
	const auto rval = place_object(LevelUniqueObjectState, LevelSharedPolygonModelState, LevelSharedRobotInfoState.Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, Cursegp, cur_object_loc, Cur_object_type, Cur_object_id);

	if (old_cur_object_index != Cur_object_index)
		vmobjptr(Cur_object_index)->rtype.pobj_info.tmap_override = -1;

	return rval;

}

//	------------------------------------------------------------------------------------------------------
//	Place current object at center of current segment.
int ObjectPlaceObjectTmap(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int	old_cur_object_index;
	//update_due_to_new_segment();
	auto &vcvertptr = Vertices.vcptr;
	const auto cur_object_loc{compute_segment_center(vcvertptr, Cursegp)};

	old_cur_object_index = Cur_object_index;
	const auto rval = place_object(LevelUniqueObjectState, LevelSharedPolygonModelState, LevelSharedRobotInfoState.Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, Cursegp, cur_object_loc, Cur_object_type, Cur_object_id);

	if (Cur_object_index != old_cur_object_index && Objects[Cur_object_index].render_type == render_type::RT_POLYOBJ)
		Objects[Cur_object_index].rtype.pobj_info.tmap_override = CurrentTexture;
	else
		editor_status("Unable to apply current texture map to this object.");
	
	return rval;
}

//	------------------------------------------------------------------------------------------------------
int ObjectSelectNextinSegment(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	//update_due_to_new_segment();

	//Assert(Cur_object_seg == Cursegp);

	const unique_segment &objuseg = Cursegp;
	if (Cur_object_index == object_none) {
		Cur_object_index = objuseg.objects;
	} else {
		if (Objects[Cur_object_index].segnum != Cursegp)
			Cur_object_index = objuseg.objects;
	}


	//Debug: make sure current object is in current segment
	objnum_t id;
	for (id = objuseg.objects; id != Cur_object_index && id != object_none; id = Objects[id].next)
	{
	}
	Assert(id == Cur_object_index);		//should have found object

	//	Select the next object, wrapping back to start if we are at the end of the linked list for this segment.
	if (id != object_none)
		Cur_object_index = get_next_object(objuseg, Cur_object_index);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;

}

//Moves to next object in the mine, skipping the player
int ObjectSelectNextInMine()
{	int i;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	for (i=0;i<MAX_OBJECTS;i++) {
		Cur_object_index++;
		if (Cur_object_index>= MAX_OBJECTS ) Cur_object_index= 0;

		const auto &&objp = vcobjptr(Cur_object_index);
		if (objp->type != OBJ_NONE && objp != ConsoleObject)
		{
			Cursegp = imsegptridx(objp->segnum);
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	for (i=0;i<MAX_OBJECTS;i++) {
		if (!(Cur_object_index --))
			Cur_object_index = MAX_OBJECTS-1;

		const auto &&objp = vcobjptr(Cur_object_index);
		if (objp->type != OBJ_NONE && objp != ConsoleObject)
		{
			Cursegp = imsegptridx(objp->segnum);
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;

	if (Cur_object_index != object_none) {
		auto delete_objnum = Cur_object_index;
		ObjectSelectNextinSegment();

		obj_delete(LevelUniqueObjectState, Segments, vmobjptridx(delete_objnum));

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
static int move_object_within_mine(fvmobjptr &vmobjptr, segment_array &Segments, fvcvertptr &vcvertptr, const vmobjptridx_t obj, const vms_vector &newpos)
{
	for (const auto &&segp : Segments.vmptridx)
	{
		if (get_seg_masks(vcvertptr, obj->pos, segp, 0).centermask == sidemask_t{})
		{
			fvi_info	hit_info;

			//	See if the radius pokes through any wall.
			const auto fate = find_vector_intersection(fvi_query{
				obj->pos,
				newpos,
				fvi_query::unused_ignore_obj_list,
				fvi_query::unused_LevelUniqueObjectState,
				fvi_query::unused_Robot_info,
				0,
				object_none,
			}, obj->segnum, obj->size, hit_info);
			if (fate != fvi_hit_type::Wall)
			{
				if (segp != obj->segnum)
					obj_relink(vmobjptr, Segments.vmptr, obj, segp);
				obj->pos = newpos;
				return 0;
			}
		}
	}
	return 1;
}

//	Return 0 if object is in expected segment, else return 1
static int verify_object_seg(fvmobjptr &vmobjptr, segment_array &Segments, fvcvertptr &vcvertptr, const vmobjptridx_t objp, const vms_vector &newpos)
{
	const auto &&result = get_seg_masks(vcvertptr, newpos, Segments.vcptr(objp->segnum), objp->size);
	if (result.facemask == 0)
		return 0;
	else
		return move_object_within_mine(vmobjptr, Segments, vcvertptr, objp, newpos);
}

namespace {

class extract_fvec_from_segment
{
public:
	static vms_vector get(fvcvertptr &vcvertptr, const shared_segment &segp)
	{
		return extract_forward_vector_from_segment(vcvertptr, segp);
	}
};

class extract_rvec_from_segment
{
public:
	static vms_vector get(fvcvertptr &vcvertptr, const shared_segment &segp)
	{
		return extract_right_vector_from_segment(vcvertptr, segp);
	}
};

class extract_uvec_from_segment
{
public:
	static vms_vector get(fvcvertptr &vcvertptr, const shared_segment &segp)
	{
		return extract_up_vector_from_segment(vcvertptr, segp);
	}
};

static int ObjectMoveFailed()
{
	editor_status("No current object, cannot move.");
	return 1;
}

static int ObjectMovePos(const vmobjptridx_t obj, vms_vector &&vec, int scale)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	vm_vec_normalize(vec);
	const auto &&newpos = vm_vec_build_add(obj->pos, vm_vec_scale(vec, scale));
	auto &vcvertptr = Vertices.vcptr;
	if (!verify_object_seg(vmobjptr, Segments, vcvertptr, obj, newpos))
		obj->pos = newpos;
	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

template <typename extract_type, int direction>
static int ObjectMove()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptridx = Objects.vmptridx;
	const auto i{Cur_object_index};
	if (i == object_none)
		return ObjectMoveFailed();
	const auto &&obj = vmobjptridx(i);
	auto &vcvertptr = Vertices.vcptr;
	return ObjectMovePos(obj, extract_type::get(vcvertptr, vcsegptr(obj->segnum)), direction * OBJ_SCALE);
}

}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveForward(void)
{
	return ObjectMove<extract_fvec_from_segment, 1>();
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveBack(void)
{
	return ObjectMove<extract_fvec_from_segment, -1>();
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveLeft(void)
{
	return ObjectMove<extract_rvec_from_segment, -1>();
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveRight(void)
{
	return ObjectMove<extract_rvec_from_segment, 1>();
}

//	------------------------------------------------------------------------------------------------------
int	ObjectSetDefault(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	//update_due_to_new_segment();

	if (Cur_object_index == object_none) {
		editor_status("No current object, cannot move.");
		return 1;
	}

	const auto &&objp = vmobjptr(Cur_object_index);
	auto &vcvertptr = Vertices.vcptr;
	objp->pos = compute_segment_center(vcvertptr, vcsegptr(objp->segnum));

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


//	------------------------------------------------------------------------------------------------------
int	ObjectMoveUp(void)
{
	return ObjectMove<extract_uvec_from_segment, 1>();
}

//	------------------------------------------------------------------------------------------------------
int	ObjectMoveDown(void)
{
	return ObjectMove<extract_uvec_from_segment, -1>();
}

//	------------------------------------------------------------------------------------------------------

static int rotate_object(const vmobjptridx_t obj, int p, int b, int h)
{
	vms_angvec ang;
//	vm_extract_angles_matrix( &ang,&obj->orient);

//	ang.p += p;
//	ang.b += b;
//	ang.h += h;

	ang.p = p;
	ang.b = b;
	ang.h = h;

	const auto &&rotmat{vm_angles_2_matrix(ang)};
	obj->orient = vm_matrix_x_matrix(obj->orient, rotmat);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

static void reset_object(const vmobjptridx_t obj)
{
	reconstruct_at(obj->orient, med_extract_matrix_from_segment, *vcsegptr(obj->segnum));
}

int ObjectResetObject()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	reset_object(vmobjptridx(Cur_object_index));

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}


int ObjectFlipObject()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const auto m = &vmobjptr(Cur_object_index)->orient;

	vm_vec_negate(m->uvec);
	vm_vec_negate(m->rvec);

	Update_flags |= UF_WORLD_CHANGED;

	return 1;
}

template <int p, int b, int h>
int ObjectChangeRotation()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	return rotate_object(vmobjptridx(Cur_object_index), p, b, h);
}

template int ObjectDecreaseBank();
template int ObjectIncreaseBank();
template int ObjectDecreasePitch();
template int ObjectIncreasePitch();
template int ObjectDecreaseHeading();
template int ObjectIncreaseHeading();

template int ObjectDecreaseBankBig();
template int ObjectIncreaseBankBig();
template int ObjectDecreasePitchBig();
template int ObjectIncreasePitchBig();
template int ObjectDecreaseHeadingBig();
template int ObjectIncreaseHeadingBig();

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

static void move_object_to_position(const vmobjptridx_t objp, const vms_vector &newpos)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vcvertptr = Vertices.vcptr;
	if (get_seg_masks(vcvertptr, newpos, vcsegptr(objp->segnum), objp->size).facemask == 0)
	{
		objp->pos = newpos;
	} else {
		if (verify_object_seg(vmobjptr, Segments, vcvertptr, objp, newpos)) {
			fvi_info	hit_info;

			auto temp_viewer_obj = *Viewer;
			auto viewer_segnum = find_object_seg(LevelSharedSegmentState, LevelUniqueSegmentState, *Viewer);
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

			const auto fate = find_vector_intersection(fvi_query{
				temp_viewer_obj.pos,
				newpos,
				fvi_query::unused_ignore_obj_list,
				fvi_query::unused_LevelUniqueObjectState,
				fvi_query::unused_Robot_info,
				0,
				object_none,
			}, temp_viewer_obj.segnum, temp_viewer_obj.size, hit_info);
			if (fate == fvi_hit_type::Wall)
			{
				objp->pos = hit_info.hit_pnt;
				const auto &&segp = find_object_seg(LevelSharedSegmentState, LevelUniqueSegmentState, objp);
				if (segp != segment_none)
					obj_relink(vmobjptr, vmsegptr, objp, segp);
			} else {
				editor_status("Attempted to move object out of mine.  Object not moved.");
			}
		}
	}

	Update_flags |= UF_WORLD_CHANGED;
}

static void move_object_to_vector(const vms_vector &vec_through_screen, fix delta_distance)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	const auto &&objp = vmobjptridx(Cur_object_index);
	const auto result{vm_vec_scale_add(Viewer->pos, vec_through_screen, vm_vec_dist(Viewer->pos, objp->pos) + delta_distance)};
	move_object_to_position(objp, result);
}

}

static void move_object_to_mouse_click_delta(fix delta_distance)
{
	short			xcrd,ycrd;

	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return;
	}

	xcrd = GameViewBox->b1_drag_x1;
	ycrd = GameViewBox->b1_drag_y1;

	const auto vec_through_screen{med_point_2_vec(&_canv_editor_game, xcrd, ycrd)};

	move_object_to_vector(vec_through_screen, delta_distance);

}

void move_object_to_mouse_click(void)
{
	move_object_to_mouse_click_delta(0);
}

namespace dsx {

int	ObjectMoveNearer(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(-4*F1_0);		//	Move four units closer to eye

	const auto &&result = vm_vec_normalized(vm_vec_build_sub(vcobjptr(Cur_object_index)->pos, Viewer->pos));
	move_object_to_vector(result, -4*F1_0);

	return 1;	
}

int	ObjectMoveFurther(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	if (Cur_object_index == object_none) {
		editor_status("Cur_object_index == -1, cannot move that peculiar object...aborting!");
		return 1;
	}

//	move_object_to_mouse_click_delta(+4*F1_0);		//	Move four units further from eye

	const auto &&result = vm_vec_normalized(vm_vec_build_sub(vcobjptr(Cur_object_index)->pos, Viewer->pos));
	move_object_to_vector(result, 4*F1_0);

	return 1;	
}

}
