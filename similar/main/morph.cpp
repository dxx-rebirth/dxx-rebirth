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
 * Morphing code
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "texmap.h"
#include "dxxerror.h"
#include "inferno.h"
#include "morph.h"
#include "polyobj.h"
#include "game.h"
#include "lighting.h"
#include "newdemo.h"
#include "piggy.h"
#include "bm.h"
#include "interp.h"

#include "compiler-range_for.h"
#include "partial_range.h"

using std::max;

array<morph_data, MAX_MORPH_OBJECTS> morph_objects;

//returns ptr to data for this object, or NULL if none
morph_data *find_morph_data(object &obj)
{
	if (Newdemo_state == ND_STATE_PLAYBACK) {
		morph_objects[0].obj = &obj;
		return &morph_objects[0];
	}

	range_for (auto &i, morph_objects)
		if (i.obj == &obj)
			return &i;
	return nullptr;
}

static void assign_max(fix &a, const fix &b)
{
	a = std::max(a, b);
}

static void assign_min(fix &a, const fix &b)
{
	a = std::min(a, b);
}

template <fix vms_vector::*p>
static void update_bounds(vms_vector &minv, vms_vector &maxv, const vms_vector *vp)
{
	auto &mx = maxv.*p;
	assign_max(mx, vp->*p);
	auto &mn = minv.*p;
	assign_min(mn, vp->*p);
}

//takes pm, fills in min & max
static void find_min_max(polymodel *pm,int submodel_num,vms_vector &minv,vms_vector &maxv)
{
	ushort nverts;
	uint16_t type;

	auto data = reinterpret_cast<uint16_t *>(&pm->model_data[pm->submodel_ptrs[submodel_num]]);

	type = *data++;

	Assert(type == 7 || type == 1);

	nverts = *data++;

	if (type==7)
		data+=2;		//skip start & pad

	auto vp = reinterpret_cast<const vms_vector *>(data);

	minv = maxv = *vp++;
	nverts--;

	while (nverts--) {
		update_bounds<&vms_vector::x>(minv, maxv, vp);
		update_bounds<&vms_vector::y>(minv, maxv, vp);
		update_bounds<&vms_vector::z>(minv, maxv, vp);
		vp++;
	}

}

#define MORPH_RATE (f1_0*3)

constexpr fix morph_rate = MORPH_RATE;

static void init_points(polymodel *pm,const vms_vector *box_size,int submodel_num,morph_data *md)
{
	ushort nverts;
	uint16_t type;
	int i;

	auto data = reinterpret_cast<uint16_t *>(&pm->model_data[pm->submodel_ptrs[submodel_num]]);

	type = *data++;

	Assert(type == 7 || type == 1);

	nverts = *data++;

	md->n_morphing_points[submodel_num] = 0;

	if (type==7) {
		i = *data++;		//get start point number
		data++;				//skip pad
	}
	else
		i = 0;				//start at zero

	Assert(i+nverts < MAX_VECS);

	md->submodel_startpoints[submodel_num] = i;

	auto vp = reinterpret_cast<const vms_vector *>(data);

	while (nverts--) {
		fix k,dist;

		if (box_size) {
			fix t;

			k = INT32_MAX;

			if (vp->x && f2i(box_size->x)<abs(vp->x)/2 && (t = fixdiv(box_size->x,abs(vp->x))) < k) k=t;
			if (vp->y && f2i(box_size->y)<abs(vp->y)/2 && (t = fixdiv(box_size->y,abs(vp->y))) < k) k=t;
			if (vp->z && f2i(box_size->z)<abs(vp->z)/2 && (t = fixdiv(box_size->z,abs(vp->z))) < k) k=t;

			if (k == INT32_MAX)
				k = 0;

		}
		else
			k=0;

		vm_vec_copy_scale(md->morph_vecs[i],*vp,k);

		dist = vm_vec_normalized_dir_quick(md->morph_deltas[i],*vp,md->morph_vecs[i]);

		md->morph_times[i] = fixdiv(dist,morph_rate);

		if (md->morph_times[i] != 0)
			md->n_morphing_points[submodel_num]++;

		vm_vec_scale(md->morph_deltas[i],morph_rate);

		vp++; i++;

	}
}

static void update_points(polymodel *pm,int submodel_num,morph_data *md)
{
	ushort nverts;
	uint16_t type;
	int i;

	auto data = reinterpret_cast<uint16_t *>(&pm->model_data[pm->submodel_ptrs[submodel_num]]);

	type = *data++;

	Assert(type == 7 || type == 1);

	nverts = *data++;

	if (type==7) {
		i = *data++;		//get start point number
		data++;				//skip pad
	}
	else
		i = 0;				//start at zero

	auto vp = reinterpret_cast<const vms_vector *>(data);

	while (nverts--) {

		if (md->morph_times[i])		//not done yet
		{
			if ((md->morph_times[i] -= FrameTime) <= 0) {
				md->morph_vecs[i] = *vp;
				md->morph_times[i] = 0;
				md->n_morphing_points[submodel_num]--;
			}
			else
				vm_vec_scale_add2(md->morph_vecs[i],md->morph_deltas[i],FrameTime);
		}
		vp++; i++;
	}
}


//process the morphing object for one frame
void do_morph_frame(object &obj)
{
	polymodel *pm;
	morph_data *md;

	md = find_morph_data(obj);

	if (md == NULL) {					//maybe loaded half-morphed from disk
		obj.flags |= OF_SHOULD_BE_DEAD;		//..so kill it
		return;
	}
	assert(md->obj == &obj);

	pm = &Polygon_models[obj.rtype.pobj_info.model_num];

	for (uint_fast32_t i = 0; i != pm->n_models; ++i)
		if (md->submodel_active[i]==1) {
			update_points(pm,i,md);
			if (md->n_morphing_points[i] == 0) {		//maybe start submodel
				md->submodel_active[i] = 2;		//not animating, just visible
				md->n_submodels_active--;		//this one done animating
				for (uint_fast32_t t = 0; t != pm->n_models; ++t)
					if (pm->submodel_parents[t] == i) {		//start this one

						init_points(pm,nullptr,t,md);
						md->n_submodels_active++;
						md->submodel_active[t] = 1;

					}
			}
		}

	if (!md->n_submodels_active) {			//done morphing!

		obj.control_type = md->morph_save_control_type;
		set_object_movement_type(obj, md->morph_save_movement_type);

		obj.render_type = RT_POLYOBJ;

		obj.mtype.phys_info = md->morph_save_phys_info;
		md->obj = NULL;
	}
}

constexpr vms_vector morph_rotvel{0x4000,0x2000,0x1000};

void init_morphs()
{
	range_for (auto &i, morph_objects)
		i.obj = nullptr;
}


//make the object morph
void morph_start(const vmobjptr_t obj)
{
	polymodel *pm;
	vms_vector pmmin,pmmax;
	vms_vector box_size;

	const auto mob = morph_objects.begin();
	const auto moe = morph_objects.end();
	const auto mop = [](const morph_data &mo) {
		return mo.obj == nullptr || mo.obj->type == OBJ_NONE || mo.obj->signature != mo.Morph_sig;
	};
	const auto moi = std::find_if(mob, moe, mop);

	if (moi == moe)		//no free slots
		return;

	morph_data *const md = &*moi;

	Assert(obj->render_type == RT_POLYOBJ);

	md->obj = obj;
	md->Morph_sig = obj->signature;

	md->morph_save_control_type = obj->control_type;
	md->morph_save_movement_type = obj->movement_type;
	md->morph_save_phys_info = obj->mtype.phys_info;

	Assert(obj->control_type == CT_AI);		//morph objects are also AI objects

	obj->control_type = CT_MORPH;
	obj->render_type = RT_MORPH;
	obj->movement_type = MT_PHYSICS;		//RT_NONE;

	obj->mtype.phys_info.rotvel = morph_rotvel;

	pm = &Polygon_models[obj->rtype.pobj_info.model_num];

	find_min_max(pm,0,pmmin,pmmax);

	box_size.x = max(-pmmin.x,pmmax.x) / 2;
	box_size.y = max(-pmmin.y,pmmax.y) / 2;
	box_size.z = max(-pmmin.z,pmmax.z) / 2;

	//clear all points
	md->morph_times = {};
	//clear all parts
	md->submodel_active = {};

	md->submodel_active[0] = 1;		//1 means visible & animating

	md->n_submodels_active = 1;

	//now, project points onto surface of box

	init_points(pm,&box_size,0,md);

}

static void draw_model(grs_canvas &canvas, polygon_model_points &robot_points, polymodel *const pm, const unsigned submodel_num, const submodel_angles anim_angles, g3s_lrgb light, morph_data *const md)
{
	array<unsigned, MAX_SUBMODELS> sort_list;
	unsigned sort_n;


	//first, sort the submodels

	sort_list[0] = submodel_num;
	sort_n = 1;

	const uint_fast32_t n_models = pm->n_models;
	for (uint_fast32_t i = 0; i != n_models; ++i)
		if (md->submodel_active[i] && pm->submodel_parents[i]==submodel_num) {
			const auto facing = g3_check_normal_facing(pm->submodel_pnts[i],pm->submodel_norms[i]);
			if (!facing)
				sort_list[sort_n] = i;
			else {		//put at start
				const auto b = sort_list.begin();
				const auto e = std::next(b, sort_n);
				std::move_backward(b, e, std::next(e));
				sort_list[0] = i;
			}
			++sort_n;
		}

	//now draw everything

	range_for (const auto mn, partial_const_range(sort_list, sort_n))
	{
		if (mn == submodel_num) {
			array<bitmap_index, MAX_POLYOBJ_TEXTURES> texture_list_index;
			for (unsigned i = 0; i < pm->n_textures; ++i)
			{
				const auto ptr = ObjBitmapPtrs[pm->first_texture + i];
				const auto &bmp = ObjBitmaps[ptr];
				texture_list_index[i] = bmp;
				texture_list[i] = &GameBitmaps[bmp.index];
			}

			// Make sure the textures for this object are paged in...
			range_for (auto &j, partial_const_range(texture_list_index, pm->n_textures))
				PIGGY_PAGE_IN(j);
			// Hmmm... cache got flushed in the middle of paging all these in,
			// so we need to reread them all in.
			// Make sure that they can all fit in memory.
			g3_draw_morphing_model(canvas, &pm->model_data[pm->submodel_ptrs[submodel_num]], &texture_list[0], anim_angles, light, &md->morph_vecs[md->submodel_startpoints[submodel_num]], robot_points);
		}
		else {
			const auto &&orient = vm_angles_2_matrix(anim_angles[mn]);
			g3_start_instance_matrix(pm->submodel_offsets[mn], orient);
			draw_model(canvas, robot_points,pm,mn,anim_angles,light,md);
			g3_done_instance();
		}
	}

}

void draw_morph_object(grs_canvas &canvas, const vmobjptridx_t obj)
{
//	int save_light;
	polymodel *po;
	g3s_lrgb light;
	morph_data *md;

	md = find_morph_data(obj);
	Assert(md != NULL);

	po=&Polygon_models[obj->rtype.pobj_info.model_num];

	light = compute_object_light(obj);

	g3_start_instance_matrix(obj->pos, obj->orient);
	polygon_model_points robot_points;
	draw_model(canvas, robot_points, po, 0, obj->rtype.pobj_info.anim_angles, light, md);

	g3_done_instance();

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_morph_frame(md);
}
