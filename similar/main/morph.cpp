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
#include "polyobj.h"
#include "game.h"
#include "lighting.h"
#include "newdemo.h"
#include "piggy.h"
#include "bm.h"
#include "interp.h"
#include "render.h"
#include "object.h"

#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"

using std::max;

namespace dcx {

morph_data::morph_data(object_base &o) :
	obj(&o), Morph_sig(o.signature)
{
	DXX_POISON_VAR(submodel_active, 0xcc);
	DXX_POISON_VAR(morph_vecs, 0xcc);
	DXX_POISON_VAR(morph_deltas, 0xcc);
	DXX_POISON_VAR(morph_times, 0xcc);
	DXX_POISON_VAR(n_morphing_points, 0xcc);
	DXX_POISON_VAR(submodel_startpoints, 0xcc);
}

d_level_unique_morph_object_state::~d_level_unique_morph_object_state() = default;

//returns ptr to data for this object, or NULL if none
std::unique_ptr<morph_data> *find_morph_data(object_base &obj)
{
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	if (Newdemo_state == ND_STATE_PLAYBACK) {
		return nullptr;
	}

	range_for (auto &i, morph_objects)
		if (i && i->obj == &obj)
			return &i;
	return nullptr;
}

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

static fix update_bounding_box_extent(const vms_vector &vp, const vms_vector &box_size, fix vms_vector::*const p, const fix entry_extent)
{
	if (!(vp.*p))
		return entry_extent;
	const auto box_size_p = box_size.*p;
	const auto abs_vp_p = abs(vp.*p);
	if (f2i(box_size_p) >= abs_vp_p / 2)
		return entry_extent;
	const fix t = fixdiv(box_size_p, abs_vp_p);
	return std::min(entry_extent, t);
}

static fix compute_bounding_box_extents(const vms_vector &vp, const vms_vector &box_size)
{
	fix k = INT32_MAX;

	k = update_bounding_box_extent(vp, box_size, &vms_vector::x, k);
	k = update_bounding_box_extent(vp, box_size, &vms_vector::y, k);
	k = update_bounding_box_extent(vp, box_size, &vms_vector::z, k);

	return k;
}

static void init_points(const polymodel *const pm, const vms_vector *const box_size, const unsigned submodel_num, morph_data *const md)
{
	auto data = reinterpret_cast<const uint16_t *>(&pm->model_data[pm->submodel_ptrs[submodel_num]]);

	const uint16_t type = *data++;

	Assert(type == 7 || type == 1);

	const uint16_t nverts = *data++;

	md->n_morphing_points[submodel_num] = 0;

	const unsigned startpoint = (type == 7)
		? *exchange(data, data + 2)		//get start point number, skip pad
		: 0;				//start at zero

	const unsigned endpoint = startpoint + nverts;

	md->submodel_startpoints[submodel_num] = startpoint;

	auto &&zr = zip(
		unchecked_partial_range(reinterpret_cast<const vms_vector *>(data), nverts),
		partial_range(md->morph_vecs, startpoint, endpoint),
		partial_range(md->morph_deltas, startpoint, endpoint),
		partial_range(md->morph_times, startpoint, endpoint)
	);
	range_for (auto &&z, zr)
	{
		const auto vp = &std::get<0>(z);
		auto &morph_vec = std::get<1>(z);
		auto &morph_delta = std::get<2>(z);
		auto &morph_time = std::get<3>(z);
		fix k;

		if (box_size && (k = compute_bounding_box_extents(*vp, *box_size) != INT32_MAX))
			vm_vec_copy_scale(morph_vec, *vp, k);
		else
			morph_vec = {};

		const fix dist = vm_vec_normalized_dir_quick(morph_delta, *vp, morph_vec);
		morph_time = fixdiv(dist, morph_rate);

		if (morph_time != 0)
			md->n_morphing_points[submodel_num]++;

		vm_vec_scale(morph_delta, morph_rate);
	}
}

static void update_points(const polymodel *const pm, const unsigned submodel_num, morph_data *const md)
{
	auto data = reinterpret_cast<uint16_t *>(&pm->model_data[pm->submodel_ptrs[submodel_num]]);

	const uint16_t type = *data++;

	Assert(type == 7 || type == 1);

	const uint16_t nverts = *data++;

	const unsigned startpoint = (type == 7)
		? *exchange(data, data + 2)		//get start point number, skip pad
		: 0;				//start at zero

	range_for (auto &&e, enumerate(unchecked_partial_range(reinterpret_cast<const vms_vector *>(data), nverts), startpoint))
	{
		const auto vp = &e.value;
		const auto i = e.idx;
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
	}
}


//process the morphing object for one frame
void do_morph_frame(object &obj)
{
	const auto umd = find_morph_data(obj);

	if (!umd) {					//maybe loaded half-morphed from disk
		obj.flags |= OF_SHOULD_BE_DEAD;		//..so kill it
		return;
	}
	const auto md = umd->get();
	assert(md->obj == &obj);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const polymodel *const pm = &Polygon_models[obj.rtype.pobj_info.model_num];

	for (uint_fast32_t i = 0; i != pm->n_models; ++i)
		if (md->submodel_active[i] == morph_data::submodel_state::animating)
		{
			update_points(pm,i,md);
			if (md->n_morphing_points[i] == 0) {		//maybe start submodel
				md->submodel_active[i] = morph_data::submodel_state::visible;		//not animating, just visible
				md->n_submodels_active--;		//this one done animating
				for (uint_fast32_t t = 0; t != pm->n_models; ++t)
					if (pm->submodel_parents[t] == i) {		//start this one

						init_points(pm,nullptr,t,md);
						md->n_submodels_active++;
						md->submodel_active[t] = morph_data::submodel_state::animating;
					}
			}
		}

	if (!md->n_submodels_active) {			//done morphing!

		obj.control_type = md->morph_save_control_type;
		set_object_movement_type(obj, md->morph_save_movement_type);

		obj.render_type = RT_POLYOBJ;

		obj.mtype.phys_info = md->morph_save_phys_info;
		umd->reset();
	}
}

constexpr vms_vector morph_rotvel{0x4000,0x2000,0x1000};

void init_morphs()
{
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	morph_objects = {};
}


//make the object morph
void morph_start(const vmobjptr_t obj)
{
	polymodel *pm;
	vms_vector pmmin,pmmax;
	vms_vector box_size;

	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	const auto mob = morph_objects.begin();
	const auto moe = morph_objects.end();
	const auto mop = [](const std::unique_ptr<morph_data> &pmo) {
		if (!pmo)
			return true;
		auto &mo = *pmo.get();
		return mo.obj->type == OBJ_NONE || mo.obj->signature != mo.Morph_sig;
	};
	const auto moi = std::find_if(mob, moe, mop);

	if (moi == moe)		//no free slots
		return;

	*moi = make_unique<morph_data>(obj);
	morph_data *const md = moi->get();

	Assert(obj->render_type == RT_POLYOBJ);

	md->morph_save_control_type = obj->control_type;
	md->morph_save_movement_type = obj->movement_type;
	md->morph_save_phys_info = obj->mtype.phys_info;

	Assert(obj->control_type == CT_AI);		//morph objects are also AI objects

	obj->control_type = CT_MORPH;
	obj->render_type = RT_MORPH;
	obj->movement_type = MT_PHYSICS;		//RT_NONE;

	obj->mtype.phys_info.rotvel = morph_rotvel;

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	pm = &Polygon_models[obj->rtype.pobj_info.model_num];

	find_min_max(pm,0,pmmin,pmmax);

	box_size.x = max(-pmmin.x,pmmax.x) / 2;
	box_size.y = max(-pmmin.y,pmmax.y) / 2;
	box_size.z = max(-pmmin.z,pmmax.z) / 2;

	//clear all points
	md->morph_times = {};
	//clear all parts
	md->submodel_active = {};

	md->submodel_active[0] = morph_data::submodel_state::animating;		//1 means visible & animating

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
	range_for (const uint_fast32_t i, xrange(n_models))
		if (md->submodel_active[i] != morph_data::submodel_state::invisible && pm->submodel_parents[i] == submodel_num)
		{
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

namespace dsx {

void draw_morph_object(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;
	polymodel *po;

	const auto umd = find_morph_data(obj);
	if (!umd)
		throw std::runtime_error("missing morph data");
	const auto md = umd->get();

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	po=&Polygon_models[obj->rtype.pobj_info.model_num];

	const auto light = compute_object_light(LevelUniqueLightState, obj);

	g3_start_instance_matrix(obj->pos, obj->orient);
	polygon_model_points robot_points;
	draw_model(canvas, robot_points, po, 0, obj->rtype.pobj_info.anim_angles, light, md);

	g3_done_instance();

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_morph_frame(obj);
}

}
