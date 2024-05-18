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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Hacked-in polygon objects
 *
 */


#include <span>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "robot.h"
#include "vecmat.h"
#include "cntrlcen.h"
#include "interp.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "physfs-serial.h"
#include "physfsx.h"
#ifndef DRIVE
#include "bm.h"
#include "textures.h"
#include "object.h"
#include "lighting.h"
#include "piggy.h"
#endif
#include "render.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#else
#include "texmap.h"
#endif
#include "bm.h"

#include "d_zip.h"
#include "partial_range.h"
#include <memory>

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

namespace {

#define	MODEL_BUF_SIZE	32768

static void _pof_cfseek(int len, int type, std::size_t &Pof_addr)
{
	switch (type) {
		case SEEK_SET:	Pof_addr = len;	break;
		case SEEK_CUR:	Pof_addr += len;	break;
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

#define pof_cfseek(_buf,_len,_type) _pof_cfseek((_len), (_type), Pof_addr)

static int32_t pof_read_int(const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	const auto s = bufp.subspan(Pof_addr, 4);
	Pof_addr += 4;
	return {GET_INTEL_INT<int32_t>(s.data())};
}

static std::size_t pof_cfread(void *const dst, const size_t elsize, const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	if (Pof_addr + elsize > bufp.size())
		return 0;

	memcpy(dst, &bufp[Pof_addr], elsize);

	Pof_addr += elsize;

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
	return elsize;
}

#define new_pof_read_int(i,f) pof_cfread(&(i), sizeof(i), (f), Pof_addr)

static int16_t pof_read_short(const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	const auto s = bufp.subspan(Pof_addr, 2);
	Pof_addr += 2;
	const auto r = GET_INTEL_SHORT(s.data());
	return r;
}

static void pof_skip_string(int max_char, const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	for (int i=0; i<max_char; i++) {
		if (bufp[Pof_addr++] == 0)
			break;
	}
}

static void pof_read_vec(vms_vector &vec, const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	vec.x = pof_read_int(bufp, Pof_addr);
	vec.y = pof_read_int(bufp, Pof_addr);
	vec.z = pof_read_int(bufp, Pof_addr);
	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

static void pof_read_ang(vms_angvec &ang, const std::span<const uint8_t> bufp, std::size_t &Pof_addr)
{
	ang.p = pof_read_short(bufp, Pof_addr);
	ang.b = pof_read_short(bufp, Pof_addr);
	ang.h = pof_read_short(bufp, Pof_addr);
	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this object
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.

//reads a binary file containing a 3d model
static void read_model_file(polymodel &pm, const char *const filename, robot_info *const r)
{
	short version;
	int len, next_chunk;

	auto &&[ifile, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Failed to open file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));

	std::size_t Pof_addr = 0;
	std::array<uint8_t, MODEL_BUF_SIZE> model_storage;
	const std::size_t Pof_file_end = PHYSFSX_readBytes(ifile, model_storage.data(), model_storage.size());
	const std::span model_buf{model_storage.data(), Pof_file_end};
	ifile.reset();
	const int model_id = pof_read_int(model_buf, Pof_addr);

	if (model_id != 0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf, Pof_addr);
	
	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	int pof_id;
	while (new_pof_read_int(pof_id, model_buf))
	{
		pof_id = INTEL_INT(pof_id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf, Pof_addr);
		next_chunk = Pof_addr + len;

		switch (pof_id)
		{
			case ID_OHDR: {		//Object header
				vms_vector pmmin,pmmax;

				pm.n_models = pof_read_int(model_buf, Pof_addr);
				pm.rad = pof_read_int(model_buf, Pof_addr);

				assert(pm.n_models <= MAX_SUBMODELS);

				pof_read_vec(pmmin, model_buf, Pof_addr);
				pof_read_vec(pmmax, model_buf, Pof_addr);

				break;
			}
			
			case ID_SOBJ: {		//Subobject header
				int n;

				n = pof_read_short(model_buf, Pof_addr);

				Assert(n < MAX_SUBMODELS);

				pm.submodel_parents[n] = pof_read_short(model_buf, Pof_addr);

				pof_read_vec(pm.submodel_norms[n], model_buf, Pof_addr);
				pof_read_vec(pm.submodel_pnts[n], model_buf, Pof_addr);
				pof_read_vec(pm.submodel_offsets[n], model_buf, Pof_addr);

				pm.submodel_rads[n] = pof_read_int(model_buf, Pof_addr);		//radius
				pm.submodel_ptrs[n] = pof_read_int(model_buf, Pof_addr);	//offset

				break;

			}
			
			#ifndef DRIVE
			case ID_GUNS: {		//List of guns on this object

				if (r) {
					vms_vector gun_dir;

					const uint8_t n_guns = pof_read_int(model_buf, Pof_addr);
					r->n_guns = std::min<uint8_t>(n_guns, MAX_GUNS);
					for (const auto i : xrange(n_guns))
					{
						(void)i;
						const uint_fast32_t gun_id = pof_read_short(model_buf, Pof_addr);
						/*
						 * D1 v1.0 boss02.pof has id=4 and r->n_guns==4.  This
						 * is wrong, but it will not cause memory corruption.
						 */
						const auto submodel = pof_read_short(model_buf, Pof_addr);
						vms_vector v;
						pof_read_vec(v, model_buf, Pof_addr);
						Assert(submodel != 0xff);
						if (gun_id < std::size(r->gun_submodels))
						{
							/* Cast to robot_gun_number is well defined because
							 * the if test ensures the index is in range for
							 * the array.
							 */
							const auto g = static_cast<robot_gun_number>(gun_id);
							r->gun_submodels[g] = submodel;
							r->gun_points[g] = v;
						}

						if (version >= 7)
							pof_read_vec(gun_dir, model_buf, Pof_addr);
					}
				}
				else
					pof_cfseek(model_buf,len,SEEK_CUR);

				break;
			}
			
			case ID_ANIM:		//Animation data
				if (r) {
					enumerated_array<std::array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES, robot_animation_state> anim_angs{};
					unsigned n_frames;

					n_frames = pof_read_short(model_buf, Pof_addr);

					Assert(n_frames == N_ANIM_STATES);

					for (int m = 0; m < pm.n_models; ++m)
						range_for (auto &f, partial_range(anim_angs, n_frames))
							pof_read_ang(f[m], model_buf, Pof_addr);

					robot_set_angles(*r, pm, anim_angs);
				}
				else
					pof_cfseek(model_buf,len,SEEK_CUR);

				break;
			#endif
			
			case ID_TXTR: {		//Texture filename list
				int n;

				n = pof_read_short(model_buf, Pof_addr);
				while (n--) {
					pof_skip_string(128, model_buf, Pof_addr);
				}
				break;
			}
			
			case ID_IDTA:		//Interpreter data
				pm.model_data_size = len;
				pm.model_data = std::make_unique<uint8_t[]>(pm.model_data_size);

				pof_cfread(pm.model_data.get(), len, model_buf, Pof_addr);

				break;

			default:
				pof_cfseek(model_buf,len,SEEK_CUR);
				break;

		}
		if ( version >= 8 )		// Version 8 needs 4-byte alignment!!!
			pof_cfseek(model_buf,next_chunk,SEEK_SET);
	}

	if constexpr (DXX_WORDS_NEED_ALIGNMENT)
		align_polygon_model_data(&pm);
	if constexpr (words_bigendian)
		swap_polygon_model_data(pm.model_data.get());
}
}

namespace dsx {

//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
void read_model_guns(const char *filename, reactor &r)
{
	auto &gun_points = r.gun_points;
	auto &gun_dirs = r.gun_dirs;
	short version;
	int len;
	int n_guns=0;

	auto &&[ifile, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Failed to open file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));

	std::size_t Pof_addr = 0;
	std::array<uint8_t, MODEL_BUF_SIZE> model_storage;
	const std::size_t Pof_file_end = PHYSFSX_readBytes(ifile, model_storage.data(), model_storage.size());
	const std::span model_buf{model_storage.data(), Pof_file_end};
	ifile.reset();

	const int model_id = pof_read_int(model_buf, Pof_addr);

	if (model_id != 0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf, Pof_addr);

	Assert(version >= 7);		//must be 7 or higher for this data

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	int pof_id;
	while (new_pof_read_int(pof_id,model_buf))
	{
		pof_id = INTEL_INT(pof_id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf, Pof_addr);

		if (pof_id == ID_GUNS)
		{		//List of guns on this object
			n_guns = pof_read_int(model_buf, Pof_addr);

			for (int i=0;i<n_guns;i++) {
				int sm;

				const int gun_id = pof_read_short(model_buf, Pof_addr);
				sm = pof_read_short(model_buf, Pof_addr);
				if (sm!=0)
					Error("Invalid gun submodel in file <%s>",filename);
				pof_read_vec(gun_points[gun_id], model_buf, Pof_addr);
				pof_read_vec(gun_dirs[gun_id], model_buf, Pof_addr);
			}

		}
		else
			pof_cfseek(model_buf,len,SEEK_CUR);

	}
	r.n_guns = n_guns;
}

}

namespace dcx {

//free up a model, getting rid of all its memory
#if defined(DXX_BUILD_DESCENT_I)
static
#endif
void free_model(polymodel &po)
{
	po.model_data.reset();
}

}

//draw a polygon model

namespace dsx {

void draw_polygon_model(const enumerated_array<polymodel, MAX_POLYGON_MODELS, polygon_model_index> &Polygon_models, grs_canvas &canvas, const tmap_drawer_type tmap_drawer_ptr, const vms_vector &pos, const vms_matrix &orient, const submodel_angles anim_angles, const polygon_model_index model_num, const unsigned flags, const g3s_lrgb light, const glow_values_t *const glow_values, const alternate_textures alt_textures)
{
	draw_polygon_model(canvas, tmap_drawer_ptr, pos, orient, anim_angles, Polygon_models[model_num], flags, light, glow_values, alt_textures);
}

static polygon_model_index build_polygon_model_index_from_polygon_simpler_model_index(const polygon_simpler_model_index i)
{
	return static_cast<polygon_model_index>(underlying_value(i) - 1);
}

void draw_polygon_model(grs_canvas &canvas, const tmap_drawer_type tmap_drawer_ptr, const vms_vector &pos, const vms_matrix &orient, const submodel_angles anim_angles, const polymodel &pm, unsigned flags, const g3s_lrgb light, const glow_values_t *const glow_values, const alternate_textures alt_textures)
{
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const polymodel *po = &pm;

	//check if should use simple model
	if (po->simpler_model != polygon_simpler_model_index::None)	//must have a simpler model
		if (flags==0)							//can't switch if this is debris
			//alt textures might not match, but in the one case we're using this
			//for on 11/14/94, they do match.  So we leave it in.
			{
				int cnt=1;
				const auto depth = g3_calc_point_depth(pos);		//gets 3d depth
				while (po->simpler_model != polygon_simpler_model_index::None && depth > cnt++ * Simple_model_threshhold_scale * po->rad)
					po = &Polygon_models[build_polygon_model_index_from_polygon_simpler_model_index(po->simpler_model)];
			}

	std::array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
	{
		const unsigned n_textures = po->n_textures;
		std::array<bitmap_index, MAX_POLYOBJ_TEXTURES> texture_list_index;
		auto &&tlir = partial_range(texture_list_index, n_textures);
		if (const std::span<const bitmap_index> a{alt_textures}; !a.empty())
		{
			for (auto &&[at, tli] : zip(a.first(n_textures), tlir))
				tli = at;
		}
		else
		{
			const unsigned first_texture = po->first_texture;
			for (auto &&[obp, tli] : zip(partial_range(ObjBitmapPtrs, first_texture, first_texture + n_textures), tlir))
				tli = ObjBitmaps[obp];
		}

	// Make sure the textures for this object are paged in...
		for (auto &&[tli, tl] : zip(tlir, partial_range(texture_list, n_textures)))
		{
			tl = &GameBitmaps[tli];
			PIGGY_PAGE_IN(tli);
		}
	}
	// Hmmm... cache got flushed in the middle of paging all these in,
	// so we need to reread them all in.
	// Make sure that they can all fit in memory.

	auto &&ctx = g3_start_instance_matrix(pos, orient);

	polygon_model_points robot_points;

	if (flags == 0)		//draw entire object
		g3_draw_polygon_model(texture_list.data(), robot_points, canvas, tmap_drawer_ptr, anim_angles, light, glow_values, po->model_data.get());

	else {
		for (int i=0;flags;flags>>=1,i++)
			if (flags & 1) {
				Assert(i < po->n_models);

				//if submodel, rotate around its center point, not pivot point
				auto &&subctx = g3_start_instance_matrix();
				g3_draw_polygon_model(texture_list.data(), robot_points, canvas, tmap_drawer_ptr, anim_angles, light, glow_values, &po->model_data[po->submodel_ptrs[i]]);
				g3_done_instance(subctx);
			}	
	}
	g3_done_instance(ctx);
}

void free_polygon_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState)
{
	for (auto &i : partial_range(LevelSharedPolygonModelState.Polygon_models, LevelSharedPolygonModelState.N_polygon_models))
		free_model(i);
#if defined(DXX_BUILD_DESCENT_II)
	LevelSharedPolygonModelState.Exit_models_loaded = false;
#endif
}

}

namespace dcx {
namespace {

static void assign_max(fix &a, const fix &b)
{
	a = std::max(a, b);
}

static void assign_min(fix &a, const fix &b)
{
	a = std::min(a, b);
}

template <fix vms_vector::*p>
static void update_bounds(vms_vector &minv, vms_vector &maxv, const vms_vector &vp)
{
	auto &mx = maxv.*p;
	assign_max(mx, vp.*p);
	auto &mn = minv.*p;
	assign_min(mn, vp.*p);
}

static void assign_minmax(vms_vector &minv, vms_vector &maxv, const vms_vector &v)
{
	update_bounds<&vms_vector::x>(minv, maxv, v);
	update_bounds<&vms_vector::y>(minv, maxv, v);
	update_bounds<&vms_vector::z>(minv, maxv, v);
}

static void polyobj_find_min_max(polymodel &pm)
{
	auto &big_mn = pm.mins;
	auto &big_mx = pm.maxs;
	for (int m = 0; m < pm.n_models; ++m)
	{
		auto &mn = pm.submodel_mins[m];
		auto &mx = pm.submodel_maxs[m];
		const auto &ofs = pm.submodel_offsets[m];

		auto data = reinterpret_cast<const uint16_t *>(&pm.model_data[pm.submodel_ptrs[m]]);
	
		const auto type = *data++;
	
		Assert(type == 7 || type == 1);
	
		const uint16_t nverts = *data++ - 1;
	
		if (type==7)
			data+=2;		//skip start & pad
	
		auto vp = reinterpret_cast<const vms_vector *>(data);
	
		mn = mx = *vp++;

		if (m==0)
			big_mn = big_mx = mn;
	
		range_for (auto &v, unchecked_partial_range(vp, nverts))
		{
			assign_minmax(mn, mx, v);
			assign_minmax(big_mn, big_mx, vm_vec_add(v, ofs));
		}
	}
}
}

void init_polygon_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState)
{
	LevelSharedPolygonModelState.N_polygon_models = 0;
}

}

namespace dsx {

//returns the number of this model
polygon_model_index load_polygon_model(const char *filename, int n_textures, int first_texture, robot_info *const r)
{
	Assert(n_textures < MAX_POLYOBJ_TEXTURES);

	Assert(strlen(filename) <= 12);
	const auto n_models = build_polygon_model_index_from_untrusted(LevelSharedPolygonModelState.N_polygon_models);
	if (n_models == polygon_model_index::None)
		throw std::runtime_error("too many polygon models");
	strcpy(LevelSharedPolygonModelState.Pof_names[n_models], filename);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	auto &model = Polygon_models[n_models];
	read_model_file(model, filename, r);
	polyobj_find_min_max(model);

	const auto highest_texture_num = g3_init_polygon_model(std::span{model.model_data.get(), model.model_data_size});

	if (highest_texture_num+1 != n_textures)
		Error("Model <%s> references %d textures but specifies %d.",filename,highest_texture_num+1,n_textures);

	model.n_textures = n_textures;
	model.first_texture = first_texture;
	model.simpler_model = polygon_simpler_model_index{};

	++LevelSharedPolygonModelState.N_polygon_models;
	return n_models;
}

}

//compare against this size when figuring how far to place eye for picture
#define BASE_MODEL_SIZE 0x28000

#define DEFAULT_VIEW_DIST 0x60000

//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.
void draw_model_picture(grs_canvas &canvas, const polymodel &mn, const vms_angvec &orient_angles)
{
	g3s_lrgb	lrgb = { f1_0, f1_0, f1_0 };

	gr_clear_canvas(canvas, BM_XRGB(0,0,0));
	g3_start_frame(canvas);
	vms_vector temp_pos{};
	g3_set_view_matrix(temp_pos,vmd_identity_matrix,0x9000);

	if (mn.rad != 0)
		temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST, mn.rad, BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;

	const auto &&temp_orient = vm_angles_2_matrix(orient_angles);
	draw_polygon_model(canvas, draw_tmap, temp_pos, temp_orient, nullptr, mn, 0, lrgb, nullptr, alternate_textures{});
	g3_end_frame();
}

namespace dcx {

DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE();
DEFINE_SERIAL_UDT_TO_MESSAGE(polymodel, p, (p.n_models, p.model_data_size, serial::pad<4>(), p.submodel_ptrs, p.submodel_offsets, p.submodel_norms, p.submodel_pnts, p.submodel_rads, p.submodel_parents, p.submodel_mins, p.submodel_maxs, p.mins, p.maxs, p.rad, p.n_textures, p.first_texture, p.simpler_model));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(polymodel, 12 + (10 * 4) + (10 * 3 * sizeof(vms_vector)) + (10 * sizeof(fix)) + 10 + (10 * 2 * sizeof(vms_vector)) + (2 * sizeof(vms_vector)) + 8);

/*
 * reads a polymodel structure from a PHYSFS_File
 */
void polymodel_read(polymodel &pm, const NamedPHYSFS_File fp)
{
	pm.model_data.reset();
	PHYSFSX_serialize_read(fp, pm);
}

}

#if 0
void polymodel_write(PHYSFS_File *fp, const polymodel &pm)
{
	PHYSFSX_serialize_write(fp, pm);
}
#endif

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
namespace dsx {
void polygon_model_data_read(polymodel *pm, PHYSFS_File *fp)
{
	auto model_data_size = pm->model_data_size;
	pm->model_data = std::make_unique<uint8_t[]>(model_data_size);
	PHYSFSX_readBytes(fp, pm->model_data, model_data_size);
	/* Aligning model data changes pm->model_data_size.  Reload it
	 * afterward.
	 */
	if constexpr (DXX_WORDS_NEED_ALIGNMENT)
	{
		align_polygon_model_data(pm);
		model_data_size = pm->model_data_size;
	}
	if constexpr (words_bigendian)
		swap_polygon_model_data(pm->model_data.get());
#if defined(DXX_BUILD_DESCENT_I)
	g3_validate_polygon_model(std::span{pm->model_data.get(), model_data_size});
#elif defined(DXX_BUILD_DESCENT_II)
	g3_init_polygon_model(std::span{pm->model_data.get(), model_data_size});
#endif
}

polygon_model_index build_polygon_model_index_from_untrusted(const unsigned i)
{
	if (i >= MAX_POLYGON_MODELS)
		return polygon_model_index::None;
	return static_cast<polygon_model_index>(i);
}

}
