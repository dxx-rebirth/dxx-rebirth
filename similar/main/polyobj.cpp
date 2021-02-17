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
#endif
#include "bm.h"

#include "d_zip.h"
#include "partial_range.h"
#include <memory>

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

static unsigned Pof_file_end;
static unsigned Pof_addr;

#define	MODEL_BUF_SIZE	32768

static void _pof_cfseek(int len,int type)
{
	switch (type) {
		case SEEK_SET:	Pof_addr = len;	break;
		case SEEK_CUR:	Pof_addr += len;	break;
		case SEEK_END:
			Assert(len <= 0);	//	seeking from end, better be moving back.
			Pof_addr = Pof_file_end + len;
			break;
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

#define pof_cfseek(_buf,_len,_type) _pof_cfseek((_len),(_type))

static int pof_read_int(ubyte *bufp)
{
	int i;

	i = *(reinterpret_cast<int *>(&bufp[Pof_addr]));
	Pof_addr += 4;
	return INTEL_INT(i);

//	if (PHYSFS_read(f,&i,sizeof(i),1) != 1)
//		Error("Unexpected end-of-file while reading object");
//
//	return i;
}

static size_t pof_cfread(void *dst, size_t elsize, size_t nelem, ubyte *bufp)
{
	if (Pof_addr + nelem*elsize > Pof_file_end)
		return 0;

	memcpy(dst, &bufp[Pof_addr], elsize*nelem);

	Pof_addr += elsize*nelem;

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();

	return nelem;
}

// #define new_read_int(i,f) PHYSFS_read((f),&(i),sizeof(i),1)
#define new_pof_read_int(i,f) pof_cfread(&(i),sizeof(i),1,(f))

static short pof_read_short(ubyte *bufp)
{
	short s;

	s = *(reinterpret_cast<int16_t *>(&bufp[Pof_addr]));
	Pof_addr += 2;
	return INTEL_SHORT(s);
//	if (PHYSFS_read(f,&s,sizeof(s),1) != 1)
//		Error("Unexpected end-of-file while reading object");
//
//	return s;
}

static void pof_read_string(char *buf,int max_char, ubyte *bufp)
{
	for (int i=0; i<max_char; i++) {
		if ((*buf++ = bufp[Pof_addr++]) == 0)
			break;
	}

//	while (max_char-- && (*buf=PHYSFSX_fgetc(f)) != 0) buf++;

}

static void pof_read_vecs(vms_vector *vecs,int n,ubyte *bufp)
{
//	PHYSFS_read(f,vecs,sizeof(vms_vector),n);
	for (int i = 0; i < n; i++)
	{
		vecs[i].x = pof_read_int(bufp);
		vecs[i].y = pof_read_int(bufp);
		vecs[i].z = pof_read_int(bufp);
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

static void pof_read_angs(vms_angvec *angs,int n,ubyte *bufp)
{
	for (int i = 0; i < n; i++)
	{
		angs[i].p = pof_read_short(bufp);
		angs[i].b = pof_read_short(bufp);
		angs[i].h = pof_read_short(bufp);
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this object
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

static std::array<std::array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES> anim_angs;

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.

//reads a binary file containing a 3d model
static polymodel *read_model_file(polymodel *pm,const char *filename,robot_info *r)
{
	short version;
	int len, next_chunk;
	ubyte	model_buf[MODEL_BUF_SIZE];

	auto ifile = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	ifile.reset();
	const int model_id = pof_read_int(model_buf);

	if (model_id != 0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf);
	
	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	int pof_id;
	while (new_pof_read_int(pof_id, model_buf) == 1)
	{
		pof_id = INTEL_INT(pof_id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);
		next_chunk = Pof_addr + len;

		switch (pof_id)
		{
			case ID_OHDR: {		//Object header
				vms_vector pmmin,pmmax;

				pm->n_models = pof_read_int(model_buf);
				pm->rad = pof_read_int(model_buf);

				Assert(pm->n_models <= MAX_SUBMODELS);

				pof_read_vecs(&pmmin,1,model_buf);
				pof_read_vecs(&pmmax,1,model_buf);

				break;
			}
			
			case ID_SOBJ: {		//Subobject header
				int n;

				n = pof_read_short(model_buf);

				Assert(n < MAX_SUBMODELS);

				pm->submodel_parents[n] = pof_read_short(model_buf);

				pof_read_vecs(&pm->submodel_norms[n],1,model_buf);
				pof_read_vecs(&pm->submodel_pnts[n],1,model_buf);
				pof_read_vecs(&pm->submodel_offsets[n],1,model_buf);

				pm->submodel_rads[n] = pof_read_int(model_buf);		//radius

				pm->submodel_ptrs[n] = pof_read_int(model_buf);	//offset

				break;

			}
			
			#ifndef DRIVE
			case ID_GUNS: {		//List of guns on this object

				if (r) {
					vms_vector gun_dir;

					r->n_guns = pof_read_int(model_buf);

					Assert(r->n_guns <= MAX_GUNS);

					for (int i=0;i<r->n_guns;i++) {
						const uint_fast32_t gun_id = pof_read_short(model_buf);
						/*
						 * D1 v1.0 boss02.pof has id=4 and r->n_guns==4.
						 * Relax the assert to check only for memory
						 * corruption.
						 */
						assert(gun_id < std::size(r->gun_submodels));
						auto &submodel = r->gun_submodels[gun_id];
						submodel = pof_read_short(model_buf);
						Assert(submodel != 0xff);
						pof_read_vecs(&r->gun_points[gun_id], 1, model_buf);

						if (version >= 7)
							pof_read_vecs(&gun_dir,1,model_buf);
					}
				}
				else
					pof_cfseek(model_buf,len,SEEK_CUR);

				break;
			}
			
			case ID_ANIM:		//Animation data
				if (r) {
					unsigned n_frames;

					n_frames = pof_read_short(model_buf);

					Assert(n_frames == N_ANIM_STATES);

					for (int m=0;m<pm->n_models;m++)
						range_for (auto &f, partial_range(anim_angs, n_frames))
							pof_read_angs(&f[m], 1, model_buf);


					robot_set_angles(r,pm,anim_angs);
				
				}
				else
					pof_cfseek(model_buf,len,SEEK_CUR);

				break;
			#endif
			
			case ID_TXTR: {		//Texture filename list
				int n;
				char name_buf[128];

				n = pof_read_short(model_buf);
				while (n--) {
					pof_read_string(name_buf,128,model_buf);
				}

				break;
			}
			
			case ID_IDTA:		//Interpreter data
				pm->model_data_size = len;
				pm->model_data = std::make_unique<uint8_t[]>(pm->model_data_size);

				pof_cfread(pm->model_data.get(),1,len,model_buf);

				break;

			default:
				pof_cfseek(model_buf,len,SEEK_CUR);
				break;

		}
		if ( version >= 8 )		// Version 8 needs 4-byte alignment!!!
			pof_cfseek(model_buf,next_chunk,SEEK_SET);
	}

#if DXX_WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
	if constexpr (words_bigendian)
		swap_polygon_model_data(pm->model_data.get());
	return pm;
}

//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
void read_model_guns(const char *filename, reactor &r)
{
	auto &gun_points = r.gun_points;
	auto &gun_dirs = r.gun_dirs;
	short version;
	int len;
	int n_guns=0;
	ubyte	model_buf[MODEL_BUF_SIZE];

	auto ifile = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	ifile.reset();

	const int model_id = pof_read_int(model_buf);

	if (model_id != 0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf);

	Assert(version >= 7);		//must be 7 or higher for this data

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	int pof_id;
	while (new_pof_read_int(pof_id,model_buf) == 1)
	{
		pof_id = INTEL_INT(pof_id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);

		if (pof_id == ID_GUNS)
		{		//List of guns on this object
			n_guns = pof_read_int(model_buf);

			for (int i=0;i<n_guns;i++) {
				int sm;

				const int gun_id = pof_read_short(model_buf);
				sm = pof_read_short(model_buf);
				if (sm!=0)
					Error("Invalid gun submodel in file <%s>",filename);
				pof_read_vecs(&gun_points[gun_id], 1, model_buf);
				pof_read_vecs(&gun_dirs[gun_id], 1, model_buf);
			}

		}
		else
			pof_cfseek(model_buf,len,SEEK_CUR);

	}
	r.n_guns = n_guns;
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

void draw_polygon_model(grs_canvas &canvas, const vms_vector &pos, const vms_matrix &orient, const submodel_angles anim_angles, const unsigned model_num, unsigned flags, const g3s_lrgb light, const glow_values_t *const glow_values, alternate_textures alt_textures)
{
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const polymodel *po = &Polygon_models[model_num];

	//check if should use simple model
	if (po->simpler_model )					//must have a simpler model
		if (flags==0)							//can't switch if this is debris
			//alt textures might not match, but in the one case we're using this
			//for on 11/14/94, they do match.  So we leave it in.
			{
				int cnt=1;
				const auto depth = g3_calc_point_depth(pos);		//gets 3d depth
				while (po->simpler_model && depth > cnt++ * Simple_model_threshhold_scale * po->rad)
					po = &Polygon_models[po->simpler_model-1];
			}

	std::array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
	{
		const unsigned n_textures = po->n_textures;
		std::array<bitmap_index, MAX_POLYOBJ_TEXTURES> texture_list_index;
		auto &&tlir = partial_range(texture_list_index, n_textures);
		if (alt_textures)
		{
			for (auto &&[at, tli] : zip(unchecked_partial_range(static_cast<const bitmap_index *>(alt_textures), n_textures), tlir))
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
			tl = &GameBitmaps[tli.index];
			PIGGY_PAGE_IN(tli);
		}
	}
	// Hmmm... cache got flushed in the middle of paging all these in,
	// so we need to reread them all in.
	// Make sure that they can all fit in memory.

	g3_start_instance_matrix(pos, orient);

	polygon_model_points robot_points;

	if (flags == 0)		//draw entire object

		g3_draw_polygon_model(&texture_list[0], robot_points, canvas, anim_angles, light, glow_values, po->model_data.get());

	else {
		for (int i=0;flags;flags>>=1,i++)
			if (flags & 1) {
				Assert(i < po->n_models);

				//if submodel, rotate around its center point, not pivot point
	
				g3_start_instance_matrix();
	
				g3_draw_polygon_model(&texture_list[0], robot_points, canvas, anim_angles, light, glow_values, &po->model_data[po->submodel_ptrs[i]]);
	
				g3_done_instance();
			}	
	}

	g3_done_instance();
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

static void polyobj_find_min_max(polymodel *pm)
{
	auto &big_mn = pm->mins;
	auto &big_mx = pm->maxs;
	for (int m=0;m<pm->n_models;m++) {
		auto &mn = pm->submodel_mins[m];
		auto &mx = pm->submodel_maxs[m];
		const auto &ofs = pm->submodel_offsets[m];

		auto data = reinterpret_cast<const uint16_t *>(&pm->model_data[pm->submodel_ptrs[m]]);
	
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

void init_polygon_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState)
{
	LevelSharedPolygonModelState.N_polygon_models = 0;
}

}

namespace dsx {

//returns the number of this model
int load_polygon_model(const char *filename,int n_textures,int first_texture,robot_info *r)
{
	Assert(n_textures < MAX_POLYOBJ_TEXTURES);

	Assert(strlen(filename) <= 12);
	const auto n_models = LevelSharedPolygonModelState.N_polygon_models;
	strcpy(LevelSharedPolygonModelState.Pof_names[n_models], filename);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	auto &model = Polygon_models[n_models];
	read_model_file(&model, filename, r);

	polyobj_find_min_max(&model);

	const auto highest_texture_num = g3_init_polygon_model(model.model_data.get(), model.model_data_size);

	if (highest_texture_num+1 != n_textures)
		Error("Model <%s> references %d textures but specifies %d.",filename,highest_texture_num+1,n_textures);

	model.n_textures = n_textures;
	model.first_texture = first_texture;
	model.simpler_model = 0;

	return LevelSharedPolygonModelState.N_polygon_models++;
}

}

//compare against this size when figuring how far to place eye for picture
#define BASE_MODEL_SIZE 0x28000

#define DEFAULT_VIEW_DIST 0x60000

//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.
void draw_model_picture(grs_canvas &canvas, const uint_fast32_t mn, const vms_angvec &orient_angles)
{
	g3s_lrgb	lrgb = { f1_0, f1_0, f1_0 };

	gr_clear_canvas(canvas, BM_XRGB(0,0,0));
	g3_start_frame(canvas);
	vms_vector temp_pos{};
	g3_set_view_matrix(temp_pos,vmd_identity_matrix,0x9000);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	if (Polygon_models[mn].rad != 0)
		temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST,Polygon_models[mn].rad,BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;

	const auto &&temp_orient = vm_angles_2_matrix(orient_angles);
	draw_polygon_model(canvas, temp_pos, temp_orient, nullptr, mn, 0, lrgb, nullptr, nullptr);
	g3_end_frame();
}

namespace dcx {

DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE();
DEFINE_SERIAL_UDT_TO_MESSAGE(polymodel, p, (p.n_models, p.model_data_size, serial::pad<4>(), p.submodel_ptrs, p.submodel_offsets, p.submodel_norms, p.submodel_pnts, p.submodel_rads, p.submodel_parents, p.submodel_mins, p.submodel_maxs, p.mins, p.maxs, p.rad, p.n_textures, p.first_texture, p.simpler_model));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(polymodel, 12 + (10 * 4) + (10 * 3 * sizeof(vms_vector)) + (10 * sizeof(fix)) + 10 + (10 * 2 * sizeof(vms_vector)) + (2 * sizeof(vms_vector)) + 8);

/*
 * reads a polymodel structure from a PHYSFS_File
 */
void polymodel_read(polymodel *pm, PHYSFS_File *fp)
{
	pm->model_data.reset();
	PHYSFSX_serialize_read(fp, *pm);
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
	PHYSFS_read(fp, pm->model_data, sizeof(uint8_t), model_data_size);
#if DXX_WORDS_NEED_ALIGNMENT
	/* Aligning model data changes pm->model_data_size.  Reload it
	 * afterward.
	 */
	align_polygon_model_data(pm);
	model_data_size = pm->model_data_size;
#endif
	if constexpr (words_bigendian)
		swap_polygon_model_data(pm->model_data.get());
#if defined(DXX_BUILD_DESCENT_I)
	g3_validate_polygon_model(pm->model_data.get(), model_data_size);
#elif defined(DXX_BUILD_DESCENT_II)
	g3_init_polygon_model(pm->model_data.get(), model_data_size);
#endif
}
}
