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

#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"

using std::max;

namespace dcx {

namespace {

constexpr vms_vector morph_rotvel{0x4000,0x2000,0x1000};

class invalid_morph_model_type : public std::runtime_error
{
	__attribute_cold
	static std::string prepare_message(const unsigned type)
	{
		char buf[32 + sizeof("4294967295")];
		const auto len = std::snprintf(buf, sizeof buf, "invalid morph model type: %u", type);
		return std::string(buf, len);
	}
public:
	invalid_morph_model_type(const unsigned type) :
		runtime_error(prepare_message(type))
	{
	}
};

class invalid_morph_model_vertex_count : public std::runtime_error
{
	__attribute_cold
	static std::string prepare_message(const unsigned count, const morph_data::polymodel_idx idx, const unsigned submodel_num)
	{
		char buf[68 + 3 * sizeof("4294967295")];
		const unsigned uidx = idx.idx;
		const auto len = std::snprintf(buf, sizeof buf, "too many vertices in morph model: found %u in model %u, submodel %u", count, uidx, submodel_num);
		return std::string(buf, len);
	}
public:
	invalid_morph_model_vertex_count(const unsigned count, const morph_data::polymodel_idx idx, const unsigned submodel_num) :
		runtime_error(prepare_message(count, idx, submodel_num))
	{
	}
};

struct submodel_data
{
	const uint16_t *body;
	const unsigned type;
	const unsigned nverts;
	const unsigned startpoint;
};

submodel_data parse_model_data_header(const polymodel &pm, const unsigned submodel_num)
{
	auto data = reinterpret_cast<const uint16_t *>(&pm.model_data[pm.submodel_ptrs[submodel_num]]);
	const auto ptype = data++;

	const uint16_t type = *ptype;
	const auto pnverts = data++;

	const uint16_t startpoint = (type == 7)
		? *std::exchange(data, data + 2)		//get start point number, skip pad
		: (type == 1)
		? 0				//start at zero
		: throw invalid_morph_model_type(type);
	const uint16_t nverts = *pnverts;
	return {data, type, nverts, startpoint};
}

std::size_t count_submodel_points(const polymodel &pm, const morph_data::polymodel_idx model_idx, const unsigned submodel_num)
{
	/* Return the minimum array size that will not cause this submodel
	 * to index past the end of the array.
	 */
	const auto &&sd = parse_model_data_header(pm, submodel_num);
	const std::size_t count = sd.startpoint + sd.nverts;
	if (count > morph_data::MAX_VECS)
		throw invalid_morph_model_vertex_count(count, model_idx, submodel_num);
	return count;
}

std::size_t count_model_points(const polymodel &pm, const morph_data::polymodel_idx model_idx)
{
	/* Return the minimum array size that will not cause any used
	 * submodel of this model to index past the end of the array.
	 *
	 * Unused submodels are not considered.  A submodel is used if:
	 * - its index is not above pm.n_models
	 * - its parent index is valid
	 * - its parent index is a used submodel
	 *
	 * Submodel 0 is always used.
	 */
	auto count = count_submodel_points(pm, model_idx, 0);
	unsigned visited_submodels = 1;
	const unsigned mask_all_enabled_models = (1 << pm.n_models) - 1;
	const auto &&submodel_parents = enumerate(partial_range(pm.submodel_parents, pm.n_models));
	for (;;)
	{
		if (visited_submodels == mask_all_enabled_models)
			/* Every submodel has been checked, so the next pass through
			 * the loop will ignore every element.  Break out early to
			 * avoid the extra iteration.
			 */
			break;
		const auto previous_visited_submodels = visited_submodels;
		range_for (auto &&e, submodel_parents)
		{
			const unsigned mask_this_submodel = 1 << e.idx;
			if (mask_this_submodel & visited_submodels)
				/* Already tested on a prior iteration */
				continue;
			if (e.value >= pm.submodel_parents.size())
				/* Ignore submodels with out-of-range parents.  This
				 * avoids undefined behavior in the shift, since some
				 * submodels have a parent of 0xff.
				 */
				continue;
			const unsigned mask_parent_submodel = 1 << e.value;
			if (mask_parent_submodel & visited_submodels)
			{
				visited_submodels |= mask_this_submodel;
				/* Parent is in use, so this submodel is also in use. */
				const auto subcount = count_submodel_points(pm, model_idx, e.idx);
				count = std::max(count, subcount);
			}
		}
		if (previous_visited_submodels == visited_submodels)
			/* No changes on the most recent pass, so no changes will
			 * occur on any subsequent pass.  Break out.
			 */
			break;
	}
	return count;
}

}

void *morph_data::operator new(std::size_t, const max_vectors max_vecs)
{
	return ::operator new(sizeof(morph_data) + (max_vecs.count * (sizeof(fix) + sizeof(vms_vector) + sizeof(vms_vector))));
}

morph_data::ptr morph_data::create(object_base &o, const polymodel &pm, const polymodel_idx model_idx)
{
	const max_vectors m{count_model_points(pm, model_idx)};
	/* This is an unusual form of `new` overload.  Although arguments to
	 * `new` are typically considered a use of `placement new`, this is
	 * not used to place the object.  Instead, it is used to pass extra
	 * information to `operator new` so that the count of allocated
	 * bytes can be adjusted.
	 */
	return ptr(new(m) morph_data(o, m));
}

morph_data::morph_data(object_base &o, const max_vectors m) :
	obj(&o), Morph_sig(o.signature), max_vecs(m)
{
	DXX_POISON_VAR(submodel_active, 0xcc);
	const auto morph_times = get_morph_times();
	DXX_POISON_MEMORY(morph_times.begin(), morph_times.end(), 0xcc);
	const auto morph_vecs = get_morph_times();
	DXX_POISON_MEMORY(morph_vecs.begin(), morph_vecs.end(), 0xcc);
	const auto morph_deltas = get_morph_times();
	DXX_POISON_MEMORY(morph_deltas.begin(), morph_deltas.end(), 0xcc);
	DXX_POISON_VAR(n_morphing_points, 0xcc);
	DXX_POISON_VAR(submodel_startpoints, 0xcc);
}

span<fix> morph_data::get_morph_times()
{
	return {reinterpret_cast<fix *>(this + 1), max_vecs.count};
}

span<vms_vector> morph_data::get_morph_vecs()
{
	return {reinterpret_cast<vms_vector *>(get_morph_times().end()), max_vecs.count};
}

span<vms_vector> morph_data::get_morph_deltas()
{
	return {get_morph_vecs().end(), max_vecs.count};
}

d_level_unique_morph_object_state::~d_level_unique_morph_object_state() = default;

//returns ptr to data for this object, or NULL if none
morph_data::ptr *find_morph_data(d_level_unique_morph_object_state &LevelUniqueMorphObjectState, object_base &obj)
{
	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	if (Newdemo_state == ND_STATE_PLAYBACK) {
		return nullptr;
	}

	range_for (auto &i, morph_objects)
		if (i && i->obj == &obj)
			return &i;
	return nullptr;
}

namespace {

static void assign_max(fix &a, const fix &b)
{
	a = std::max(a, b);
}

static void assign_min(fix &a, const fix &b)
{
	a = std::min(a, b);
}

static void update_bounds(vms_vector &minv, vms_vector &maxv, const vms_vector &v, fix vms_vector::*const p)
{
	auto &mx = maxv.*p;
	assign_max(mx, v.*p);
	auto &mn = minv.*p;
	assign_min(mn, v.*p);
}

//takes pm, fills in min & max
static void find_min_max(const polymodel &pm, const unsigned submodel_num, vms_vector &minv, vms_vector &maxv)
{
	const auto &&sd = parse_model_data_header(pm, submodel_num);
	const unsigned nverts = sd.nverts;
	if (!nverts)
	{
		minv = maxv = {};
		return;
	}
	const auto vp = reinterpret_cast<const vms_vector *>(sd.body);

	minv = maxv = *vp;

	range_for (auto &v, unchecked_partial_range(vp + 1, nverts - 1))
	{
		update_bounds(minv, maxv, v, &vms_vector::x);
		update_bounds(minv, maxv, v, &vms_vector::y);
		update_bounds(minv, maxv, v, &vms_vector::z);
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

}

}

namespace {

static void init_points(const polymodel &pm, const vms_vector *const box_size, const unsigned submodel_num, morph_data *const md)
{
	const auto &&sd = parse_model_data_header(pm, submodel_num);
	const unsigned startpoint = sd.startpoint;
	const unsigned endpoint = sd.startpoint + sd.nverts;

	md->submodel_active[submodel_num] = morph_data::submodel_state::animating;
	md->n_morphing_points[submodel_num] = 0;
	md->submodel_startpoints[submodel_num] = startpoint;

	const auto morph_times = md->get_morph_times();
	const auto morph_vecs = md->get_morph_vecs();
	const auto morph_deltas = md->get_morph_deltas();
	auto &&zr = zip(
		unchecked_partial_range(reinterpret_cast<const vms_vector *>(sd.body), sd.nverts),
		partial_range(morph_vecs, startpoint, endpoint),
		partial_range(morph_deltas, startpoint, endpoint),
		partial_range(morph_times, startpoint, endpoint)
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

static void update_points(const polymodel &pm, const unsigned submodel_num, morph_data *const md)
{
	const auto &&sd = parse_model_data_header(pm, submodel_num);
	const unsigned startpoint = sd.startpoint;
	const unsigned endpoint = startpoint + sd.nverts;

	const auto morph_times = md->get_morph_times();
	const auto morph_vecs = md->get_morph_vecs();
	const auto morph_deltas = md->get_morph_deltas();
	auto &&zr = zip(
		unchecked_partial_range(reinterpret_cast<const vms_vector *>(sd.body), sd.nverts),
		partial_range(morph_vecs, startpoint, endpoint),
		partial_range(morph_deltas, startpoint, endpoint),
		partial_range(morph_times, startpoint, endpoint)
	);
	range_for (auto &&z, zr)
	{
		const auto vp = &std::get<0>(z);
		auto &morph_vec = std::get<1>(z);
		auto &morph_delta = std::get<2>(z);
		auto &morph_time = std::get<3>(z);
		if (morph_time)		//not done yet
		{
			if ((morph_time -= FrameTime) <= 0) {
				morph_vec = *vp;
				morph_time = 0;
				md->n_morphing_points[submodel_num]--;
			}
			else
				vm_vec_scale_add2(morph_vec, morph_delta, FrameTime);
		}
	}
}

}

namespace dsx {
//process the morphing object for one frame
void do_morph_frame(object &obj)
{
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	const auto umd = find_morph_data(LevelUniqueMorphObjectState, obj);

	if (!umd) {					//maybe loaded half-morphed from disk
		obj.flags |= OF_SHOULD_BE_DEAD;		//..so kill it
		return;
	}
	const auto md = umd->get();
	assert(md->obj == &obj);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const polymodel &pm = Polygon_models[obj.rtype.pobj_info.model_num];

	const auto n_models = pm.n_models;
	range_for (const auto &&zi, zip(xrange(n_models), md->submodel_active, md->n_morphing_points))
	{
		const unsigned i = std::get<0>(zi);
		auto &submodel_active = std::get<1>(zi);
		if (submodel_active == morph_data::submodel_state::animating)
		{
			update_points(pm,i,md);
			const auto &n_morphing_points = std::get<2>(zi);
			if (n_morphing_points == 0) {		//maybe start submodel
				submodel_active = morph_data::submodel_state::visible;		//not animating, just visible
				md->n_submodels_active--;		//this one done animating
				range_for (const auto &&zt, zip(xrange(n_models), pm.submodel_parents))
				{
					auto &submodel_parents = std::get<1>(zt);
					if (submodel_parents == i)
					{		//start this one
						const auto t = std::get<0>(zt);
						init_points(pm,nullptr,t,md);
						md->n_submodels_active++;
					}
				}
			}
		}
	}

	if (!md->n_submodels_active) {			//done morphing!

		obj.control_source = md->morph_save_control_type;
		obj.movement_source = md->morph_save_movement_type;

		obj.render_type = RT_POLYOBJ;

		obj.mtype.phys_info = md->morph_save_phys_info;
		umd->reset();
	}
}

void init_morphs()
{
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	morph_objects = {};
}

//make the object morph
void morph_start(d_level_unique_morph_object_state &LevelUniqueMorphObjectState, d_level_shared_polygon_model_state &LevelSharedPolygonModelState, object_base &obj)
{
	vms_vector pmmin,pmmax;
	vms_vector box_size;

	auto &morph_objects = LevelUniqueMorphObjectState.morph_objects;
	const auto mob = morph_objects.begin();
	const auto moe = morph_objects.end();
	const auto mop = [](const morph_data::ptr &pmo) {
		if (!pmo)
			return true;
		auto &mo = *pmo.get();
		return mo.obj->type == OBJ_NONE || mo.obj->signature != mo.Morph_sig;
	};
	const auto moi = std::find_if(mob, moe, mop);

	if (moi == moe)		//no free slots
		return;

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const morph_data::polymodel_idx pmi(obj.rtype.pobj_info.model_num);
	auto &pm = Polygon_models[pmi.idx];

	*moi = morph_data::create(obj, pm, pmi);
	morph_data *const md = moi->get();

	assert(obj.render_type == RT_POLYOBJ);

	md->morph_save_control_type = obj.control_source;
	md->morph_save_movement_type = obj.movement_source;
	md->morph_save_phys_info = obj.mtype.phys_info;

	assert(obj.control_source == object::control_type::ai);		//morph objects are also AI objects

	obj.control_source = object::control_type::morph;
	obj.render_type = RT_MORPH;
	obj.movement_source = object::movement_type::physics;		//RT_NONE;

	obj.mtype.phys_info.rotvel = morph_rotvel;

	find_min_max(pm,0,pmmin,pmmax);

	box_size.x = max(-pmmin.x,pmmax.x) / 2;
	box_size.y = max(-pmmin.y,pmmax.y) / 2;
	box_size.z = max(-pmmin.z,pmmax.z) / 2;

	//clear all points
	const auto morph_times = md->get_morph_times();
	std::fill(morph_times.begin(), morph_times.end(), fix());
	//clear all parts
	md->submodel_active = {};

	md->n_submodels_active = 1;

	//now, project points onto surface of box

	init_points(pm,&box_size,0,md);
}
}

namespace {

static void draw_model(grs_canvas &canvas, polygon_model_points &robot_points, polymodel *const pm, const unsigned submodel_num, const submodel_angles anim_angles, g3s_lrgb light, morph_data *const md)
{
	std::array<unsigned, MAX_SUBMODELS> sort_list;
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
			std::array<bitmap_index, MAX_POLYOBJ_TEXTURES> texture_list_index;
			std::array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
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
			const auto morph_vecs = md->get_morph_vecs();
			g3_draw_morphing_model(canvas, &pm->model_data[pm->submodel_ptrs[submodel_num]], &texture_list[0], anim_angles, light, &morph_vecs[md->submodel_startpoints[submodel_num]], robot_points);
		}
		else {
			const auto &&orient = vm_angles_2_matrix(anim_angles[mn]);
			g3_start_instance_matrix(pm->submodel_offsets[mn], orient);
			draw_model(canvas, robot_points,pm,mn,anim_angles,light,md);
			g3_done_instance();
		}
	}

}

}

namespace dsx {

void draw_morph_object(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;
	polymodel *po;

	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	const auto umd = find_morph_data(LevelUniqueMorphObjectState, obj);
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
