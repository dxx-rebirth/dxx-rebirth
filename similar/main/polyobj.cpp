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
#include "interp.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "args.h"
#include "byteutil.h"
#include "physfs-serial.h"
#include "physfsx.h"
#ifndef DRIVE
#include "texmap.h"
#include "bm.h"
#include "textures.h"
#include "object.h"
#include "lighting.h"
#include "piggy.h"
#endif
#include "render.h"
#ifdef OGL
#include "ogl_init.h"
#endif

#include "compiler-make_unique.h"
#include "partial_range.h"

unsigned N_polygon_models = 0;
array<polymodel, MAX_POLYGON_MODELS> Polygon_models;	// = {&bot11,&bot17,&robot_s2,&robot_b2,&bot11,&bot17,&robot_s2,&robot_b2};

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

	i = *((int *) &bufp[Pof_addr]);
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

	s = *((short *) &bufp[Pof_addr]);
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

static array<array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES> anim_angs;

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
#ifdef WORDS_NEED_ALIGNMENT
static const uint8_t *old_dest(const chunk &o) // return where chunk is (in unaligned struct)
{
	return GET_INTEL_SHORT(&o.old_base[o.offset]) + o.old_base;
}
static uint8_t *new_dest(const chunk &o) // return where chunk is (in aligned struct)
{
	return GET_INTEL_SHORT(&o.old_base[o.offset]) + o.new_base + o.correction;
}
/*
 * find chunk with smallest address
 */
static int get_first_chunks_index(chunk *chunk_list, int no_chunks)
{
	int first_index = 0;
	Assert(no_chunks >= 1);
	for (int i = 1; i < no_chunks; i++)
		if (old_dest(chunk_list[i]) < old_dest(chunk_list[first_index]))
			first_index = i;
	return first_index;
}
#define SHIFT_SPACE 500 // increase if insufficent

static void align_polygon_model_data(polymodel *pm)
{
	int chunk_len;
	int total_correction = 0;
	chunk cur_ch;
	chunk ch_list[MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = pm->model_data_size + SHIFT_SPACE;
	RAIIdmem<uint8_t[]> tmp;
	MALLOC(tmp, uint8_t[], tmp_size); // where we build the aligned version of pm->model_data

	Assert(tmp != NULL);
	//start with first chunk (is always aligned!)
	const uint8_t *cur_old = pm->model_data.get();
	auto cur_new = tmp.get();
	chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
	memcpy(cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int first_index = get_first_chunks_index(ch_list, no_chunks);
		cur_ch = ch_list[first_index];
		// remove first chunk from array:
		no_chunks--;
		for (int i = first_index; i < no_chunks; i++)
			ch_list[i] = ch_list[i + 1];
		// if (new) address unaligned:
		const uintptr_t u = reinterpret_cast<uintptr_t>(new_dest(cur_ch));
		if (u % 4L != 0) {
			// calculate how much to move to be aligned
			short to_shift = 4 - u % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (int i = 0; i < no_chunks; i++)
				ch_list[i].correction += to_shift;
			total_correction += to_shift;
			Assert(reinterpret_cast<uintptr_t>(new_dest(cur_ch)) % 4L == 0);
			Assert(total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		*((short *)(cur_ch.new_base + cur_ch.offset))
		  = INTEL_SHORT(static_cast<short>(cur_ch.correction + GET_INTEL_SHORT(cur_ch.old_base + cur_ch.offset)));
		//write (correctly aligned) chunk:
		cur_old = old_dest(cur_ch);
		cur_new = new_dest(cur_ch);
		chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
		memcpy(cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pm, too
		for (int i = 0; i < MAX_SUBMODELS; i++)
			if (&pm->model_data[pm->submodel_ptrs[i]] >= cur_old
			    && &pm->model_data[pm->submodel_ptrs[i]] < cur_old + chunk_len)
				pm->submodel_ptrs[i] += (cur_new - tmp.get()) - (cur_old - pm->model_data.get());
 	}
	pm->model_data_size += total_correction;
	pm->model_data = make_unique<ubyte[]>(pm->model_data_size);
	Assert(pm->model_data != NULL);
	memcpy(pm->model_data.get(), tmp.get(), pm->model_data_size);
}
#endif //def WORDS_NEED_ALIGNMENT

//reads a binary file containing a 3d model
static polymodel *read_model_file(polymodel *pm,const char *filename,robot_info *r)
{
	short version;
	int id,len, next_chunk;
	ubyte	model_buf[MODEL_BUF_SIZE];

	auto ifile = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	ifile.reset();
	id = pof_read_int(model_buf);

	if (id!=0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf);
	
	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	while (new_pof_read_int(id,model_buf) == 1) {
		id = INTEL_INT(id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);
		next_chunk = Pof_addr + len;

		switch (id) {

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
						uint_fast32_t id;

						id = pof_read_short(model_buf);
						/*
						 * D1 v1.0 boss02.pof has id=4 and r->n_guns==4.
						 * Relax the assert to check only for memory
						 * corruption.
						 */
						Assert(id < sizeof(r->gun_submodels) / sizeof(r->gun_submodels[0]));
						r->gun_submodels[id] = pof_read_short(model_buf);
						Assert(r->gun_submodels[id] != 0xff);
						pof_read_vecs(&r->gun_points[id],1,model_buf);

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
				pm->model_data = make_unique<ubyte[]>(pm->model_data_size);

				pof_cfread(pm->model_data.get(),1,len,model_buf);

				break;

			default:
				pof_cfseek(model_buf,len,SEEK_CUR);
				break;

		}
		if ( version >= 8 )		// Version 8 needs 4-byte alignment!!!
			pof_cfseek(model_buf,next_chunk,SEEK_SET);
	}

#ifdef WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
	if (words_bigendian)
	swap_polygon_model_data(pm->model_data.get());
	return pm;
}

//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(const char *filename,array<vms_vector, MAX_CONTROLCEN_GUNS> &gun_points, array<vms_vector, MAX_CONTROLCEN_GUNS> &gun_dirs);
int read_model_guns(const char *filename,array<vms_vector, MAX_CONTROLCEN_GUNS> &gun_points, array<vms_vector, MAX_CONTROLCEN_GUNS> &gun_dirs)
{
	short version;
	int id,len;
	int n_guns=0;
	ubyte	model_buf[MODEL_BUF_SIZE];

	auto ifile = PHYSFSX_openReadBuffered(filename);
	if (!ifile)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	ifile.reset();

	id = pof_read_int(model_buf);

	if (id!=0x4f505350) /* 'OPSP' */
		Error("Bad ID in model file <%s>",filename);

	version = pof_read_short(model_buf);

	Assert(version >= 7);		//must be 7 or higher for this data

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		Error("Bad version (%d) in model file <%s>",version,filename);

	while (new_pof_read_int(id,model_buf) == 1) {
		id = INTEL_INT(id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);

		if (id == ID_GUNS) {		//List of guns on this object
			n_guns = pof_read_int(model_buf);

			for (int i=0;i<n_guns;i++) {
				int id,sm;

				id = pof_read_short(model_buf);
				sm = pof_read_short(model_buf);
				if (sm!=0)
					Error("Invalid gun submodel in file <%s>",filename);
				pof_read_vecs(&gun_points[id],1,model_buf);

				pof_read_vecs(&gun_dirs[id],1,model_buf);
			}

		}
		else
			pof_cfseek(model_buf,len,SEEK_CUR);

	}

	return n_guns;
}

//free up a model, getting rid of all its memory
#if defined(DXX_BUILD_DESCENT_I)
static
#endif
void free_model(polymodel *po)
{
	po->model_data.reset();
}

array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
array<bitmap_index, MAX_POLYOBJ_TEXTURES> texture_list_index;

//draw a polygon model

void draw_polygon_model(const vms_vector &pos,const vms_matrix *orient,const submodel_angles anim_angles,int model_num,int flags,g3s_lrgb light,glow_values_t *glow_values, alternate_textures alt_textures)
{
	polymodel *po;
	if (model_num < 0)
		return;

	Assert(model_num < N_polygon_models);

	po=&Polygon_models[model_num];

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

	if (alt_textures)
   {
		for (int i=0;i<po->n_textures;i++) {
			texture_list_index[i] = alt_textures[i];
			texture_list[i] = &GameBitmaps[alt_textures[i].index];
		}
   }
	else
   {
		for (int i=0;i<po->n_textures;i++) {
			texture_list_index[i] = ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]];
			texture_list[i] = &GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index];
		}
   }

	// Make sure the textures for this object are paged in...
	piggy_page_flushed = 0;
	for (int i=0;i<po->n_textures;i++) 
		PIGGY_PAGE_IN( texture_list_index[i] );
	// Hmmm... cache got flushed in the middle of paging all these in,
	// so we need to reread them all in.
	if (piggy_page_flushed)	{
		piggy_page_flushed = 0;
		for (int i=0;i<po->n_textures;i++) 
			PIGGY_PAGE_IN( texture_list_index[i] );
	}
	// Make sure that they can all fit in memory.
	Assert( piggy_page_flushed == 0 );

	g3_start_instance_matrix(pos,orient);

	polygon_model_points robot_points;

	if (flags == 0)		//draw entire object

		g3_draw_polygon_model(po->model_data.get(),&texture_list[0],anim_angles,light,glow_values, robot_points);

	else {
		for (int i=0;flags;flags>>=1,i++)
			if (flags & 1) {
				Assert(i < po->n_models);

				//if submodel, rotate around its center point, not pivot point
	
				const auto ofs = vm_vec_negated(vm_vec_avg(po->submodel_mins[i],po->submodel_maxs[i]));
				g3_start_instance_matrix(ofs,NULL);
	
				g3_draw_polygon_model(&po->model_data[po->submodel_ptrs[i]],&texture_list[0],anim_angles,light,glow_values, robot_points);
	
				g3_done_instance();
			}	
	}

	g3_done_instance();
}

void free_polygon_models()
{
	range_for (auto &i, partial_range(Polygon_models, N_polygon_models))
		free_model(&i);
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
	ushort nverts;
	ushort *data,type;
	auto &big_mn = pm->mins;
	auto &big_mx = pm->maxs;
	for (int m=0;m<pm->n_models;m++) {
		auto &mn = pm->submodel_mins[m];
		auto &mx = pm->submodel_maxs[m];
		const auto &ofs = pm->submodel_offsets[m];

		data = (ushort *)&pm->model_data[pm->submodel_ptrs[m]];
	
		type = *data++;
	
		Assert(type == 7 || type == 1);
	
		nverts = *data++;
	
		if (type==7)
			data+=2;		//skip start & pad
	
		auto vp = reinterpret_cast<const vms_vector *>(data);
	
		mn = mx = *vp++;
		nverts--;

		if (m==0)
			big_mn = big_mx = mn;
	
		while (nverts--) {
			assign_minmax(mn, mx, *vp);
			assign_minmax(big_mn, big_mx, vm_vec_add(*vp, ofs));
			vp++;
		}
	}
}

char Pof_names[MAX_POLYGON_MODELS][FILENAME_LEN];

//returns the number of this model
int load_polygon_model(const char *filename,int n_textures,int first_texture,robot_info *r)
{
	Assert(N_polygon_models < MAX_POLYGON_MODELS);
	Assert(n_textures < MAX_POLYOBJ_TEXTURES);

	Assert(strlen(filename) <= 12);
	const auto n_models = N_polygon_models;
	strcpy(Pof_names[n_models], filename);

	auto &model = Polygon_models[n_models];
	read_model_file(&model, filename, r);

	polyobj_find_min_max(&model);

	const auto highest_texture_num = g3_init_polygon_model(model.model_data.get());

	if (highest_texture_num+1 != n_textures)
		Error("Model <%s> references %d textures but specifies %d.",filename,highest_texture_num+1,n_textures);

	model.n_textures = n_textures;
	model.first_texture = first_texture;
	model.simpler_model = 0;

//	Assert(polygon_models[N_polygon_models]!=NULL);

	N_polygon_models++;

	return N_polygon_models-1;

}


void init_polygon_models()
{
	N_polygon_models = 0;
}

//compare against this size when figuring how far to place eye for picture
#define BASE_MODEL_SIZE 0x28000

#define DEFAULT_VIEW_DIST 0x60000

//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.
void draw_model_picture(uint_fast32_t mn,vms_angvec *orient_angles)
{
	vms_vector	temp_pos=ZERO_VECTOR;
	g3s_lrgb	lrgb = { f1_0, f1_0, f1_0 };

	Assert(mn<N_polygon_models);

	gr_clear_canvas( BM_XRGB(0,0,0) );
	g3_start_frame();
	g3_set_view_matrix(temp_pos,vmd_identity_matrix,0x9000);

	if (Polygon_models[mn].rad != 0)
		temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST,Polygon_models[mn].rad,BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;

	const auto &&temp_orient = vm_angles_2_matrix(*orient_angles);
	draw_polygon_model(temp_pos,&temp_orient,NULL,mn,0,lrgb,NULL,NULL);
	g3_end_frame();
}

DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE();
DEFINE_SERIAL_UDT_TO_MESSAGE(polymodel, p, (p.n_models, p.model_data_size, serial::pad<4>(), p.submodel_ptrs, p.submodel_offsets, p.submodel_norms, p.submodel_pnts, p.submodel_rads, p.submodel_parents, p.submodel_mins, p.submodel_maxs, p.mins, p.maxs, p.rad, p.n_textures, p.first_texture, p.simpler_model));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(polymodel, 12 + (10 * 4) + (10 * 3 * sizeof(vms_vector)) + (10 * sizeof(fix)) + 10 + (10 * 2 * sizeof(vms_vector)) + (2 * sizeof(vms_vector)) + 8);

/*
 * reads a polymodel structure from a PHYSFS_file
 */
void polymodel_read(polymodel *pm, PHYSFS_file *fp)
{
	pm->model_data.reset();
	PHYSFSX_serialize_read(fp, *pm);
}

void polymodel_write(PHYSFS_file *fp, const polymodel &pm)
{
	PHYSFSX_serialize_write(fp, pm);
}

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void polygon_model_data_read(polymodel *pm, PHYSFS_file *fp)
{
	pm->model_data = make_unique<ubyte[]>(pm->model_data_size);
	PHYSFS_read(fp, pm->model_data, sizeof(ubyte), pm->model_data_size);
#ifdef WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
	if (words_bigendian)
	swap_polygon_model_data(pm->model_data.get());
#if defined(DXX_BUILD_DESCENT_II)
	g3_init_polygon_model(pm->model_data.get());
#endif
}
