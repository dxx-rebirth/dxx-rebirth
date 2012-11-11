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
#include "polyobj.h"
#include "vecmat.h"
#include "interp.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "args.h"
#include "byteswap.h"
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

polymodel Polygon_models[MAX_POLYGON_MODELS];	// = {&bot11,&bot17,&robot_s2,&robot_b2,&bot11,&bot17,&robot_s2,&robot_b2};

int N_polygon_models = 0;

g3s_point robot_points[MAX_POLYGON_VECS];

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

int	Pof_file_end;
int	Pof_addr;

#define	MODEL_BUF_SIZE	32768

void _pof_cfseek(int len,int type)
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

int pof_read_int(ubyte *bufp)
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

size_t pof_cfread(void *dst, size_t elsize, size_t nelem, ubyte *bufp)
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

short pof_read_short(ubyte *bufp)
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

void pof_read_string(char *buf,int max_char, ubyte *bufp)
{
	int	i;

	for (i=0; i<max_char; i++) {
		if ((*buf++ = bufp[Pof_addr++]) == 0)
			break;
	}

//	while (max_char-- && (*buf=PHYSFSX_fgetc(f)) != 0) buf++;

}

void pof_read_vecs(vms_vector *vecs,int n,ubyte *bufp)
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

void pof_read_angs(vms_angvec *angs,int n,ubyte *bufp)
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

#ifdef DRIVE
#define robot_info void
#else
vms_angvec anim_angs[N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(robot_info *r,polymodel *pm,vms_angvec angs[N_ANIM_STATES][MAX_SUBMODELS]);
#endif

#ifdef WORDS_NEED_ALIGNMENT
ubyte * old_dest(chunk o) // return where chunk is (in unaligned struct)
{
	return o.old_base + INTEL_SHORT(*((short *)(o.old_base + o.offset)));
}
ubyte * new_dest(chunk o) // return where chunk is (in aligned struct)
{
	return o.new_base + INTEL_SHORT(*((short *)(o.old_base + o.offset))) + o.correction;
}
/*
 * find chunk with smallest address
 */
int get_first_chunks_index(chunk *chunk_list, int no_chunks)
{
	int i, first_index = 0;
	Assert(no_chunks >= 1);
	for (i = 1; i < no_chunks; i++)
		if (old_dest(chunk_list[i]) < old_dest(chunk_list[first_index]))
			first_index = i;
	return first_index;
}
#define SHIFT_SPACE 500 // increase if insufficent

void align_polygon_model_data(polymodel *pm)
{
	int i, chunk_len;
	int total_correction = 0;
	ubyte *cur_old, *cur_new;
	chunk cur_ch;
	chunk ch_list[MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = pm->model_data_size + SHIFT_SPACE;
	ubyte *tmp = d_malloc(tmp_size); // where we build the aligned version of pm->model_data

	Assert(tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = pm->model_data;
	cur_new = tmp;
	chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
	memcpy(cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int first_index = get_first_chunks_index(ch_list, no_chunks);
		cur_ch = ch_list[first_index];
		// remove first chunk from array:
		no_chunks--;
		for (i = first_index; i < no_chunks; i++)
			ch_list[i] = ch_list[i + 1];
		// if (new) address unaligned:
		if ((u_int32_t)new_dest(cur_ch) % 4L != 0) {
			// calculate how much to move to be aligned
			short to_shift = 4 - (u_int32_t)new_dest(cur_ch) % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (i = 0; i < no_chunks; i++)
				ch_list[i].correction += to_shift;
			total_correction += to_shift;
			Assert((u_int32_t)new_dest(cur_ch) % 4L == 0);
			Assert(total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		*((short *)(cur_ch.new_base + cur_ch.offset))
		  = INTEL_SHORT(cur_ch.correction
				+ INTEL_SHORT(*((short *)(cur_ch.old_base + cur_ch.offset))));
		//write (correctly aligned) chunk:
		cur_old = old_dest(cur_ch);
		cur_new = new_dest(cur_ch);
		chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
		memcpy(cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pm, too
		for (i = 0; i < MAX_SUBMODELS; i++)
			if (pm->model_data + pm->submodel_ptrs[i] >= cur_old
			    && pm->model_data + pm->submodel_ptrs[i] < cur_old + chunk_len)
				pm->submodel_ptrs[i] += (cur_new - tmp) - (cur_old - pm->model_data);
 	}
	d_free(pm->model_data);
	pm->model_data_size += total_correction;
	pm->model_data = d_malloc(pm->model_data_size);
	Assert(pm->model_data != NULL);
	memcpy(pm->model_data, tmp, pm->model_data_size);
	d_free(tmp);
}
#endif //def WORDS_NEED_ALIGNMENT

//reads a binary file containing a 3d model
polymodel *read_model_file(polymodel *pm,char *filename,robot_info *r)
{
	PHYSFS_file *ifile;
	short version;
	int id,len, next_chunk;
	int anim_flag = 0;
	ubyte	model_buf[MODEL_BUF_SIZE];

	if ((ifile=PHYSFSX_openReadBuffered(filename))==NULL)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	PHYSFS_close(ifile);

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

				anim_flag++;

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
					int i;
					vms_vector gun_dir;
					ubyte gun_used[MAX_GUNS];

					r->n_guns = pof_read_int(model_buf);

					if ( r->n_guns )
						anim_flag++;

					Assert(r->n_guns <= MAX_GUNS);

					for (i=0;i<r->n_guns;i++)
						gun_used[i] = 0;

					for (i=0;i<r->n_guns;i++) {
						int id;

						id = pof_read_short(model_buf);
						Assert(id < r->n_guns);
						Assert(gun_used[id] == 0);
						(void)gun_used[id];
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
				anim_flag++;

				if (r) {
					int n_frames,f,m;

					n_frames = pof_read_short(model_buf);

					Assert(n_frames == N_ANIM_STATES);

					for (m=0;m<pm->n_models;m++)
						for (f=0;f<n_frames;f++)
							pof_read_angs(&anim_angs[f][m], 1, model_buf);


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
				pm->model_data = d_malloc(len);
				pm->model_data_size = len;

				pof_cfread(pm->model_data,1,len,model_buf);

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
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(pm->model_data);
#endif
	return pm;
}

//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(char *filename,vms_vector *gun_points, vms_vector *gun_dirs, int *gun_submodels)
{
	PHYSFS_file *ifile;
	short version;
	int id,len;
	int n_guns=0;
	ubyte	model_buf[MODEL_BUF_SIZE];

	if ((ifile=PHYSFSX_openReadBuffered(filename))==NULL)
		Error("Can't open file <%s>",filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	PHYSFS_close(ifile);

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

			int i;

			n_guns = pof_read_int(model_buf);

			for (i=0;i<n_guns;i++) {
				int id,sm;

				id = pof_read_short(model_buf);
				sm = pof_read_short(model_buf);
				if (gun_submodels)
					gun_submodels[id] = sm;
				else if (sm!=0)
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
void free_model(polymodel *po)
{
	d_free(po->model_data);
}

grs_bitmap *texture_list[MAX_POLYOBJ_TEXTURES];
bitmap_index texture_list_index[MAX_POLYOBJ_TEXTURES];

//draw a polygon model

void draw_polygon_model(vms_vector *pos,vms_matrix *orient,vms_angvec *anim_angles,int model_num,int flags,g3s_lrgb light,fix *glow_values,bitmap_index alt_textures[])
{
	polymodel *po;
	int i;

	if (model_num < 0)
		return;

	Assert(model_num < N_polygon_models);

	po=&Polygon_models[model_num];

	//check if should use simple model
	if (po->simpler_model )					//must have a simpler model
		if (flags==0)							//can't switch if this is debris
			//!!if (!alt_textures) {				//alternate textures might not match
			//alt textures might not match, but in the one case we're using this
			//for on 11/14/94, they do match.  So we leave it in.
			{
				int cnt=1;
				fix depth;
	
				depth = g3_calc_point_depth(pos);		//gets 3d depth

				while (po->simpler_model && depth > cnt++ * Simple_model_threshhold_scale * po->rad)
					po = &Polygon_models[po->simpler_model-1];
			}

	if (alt_textures)
   {
		for (i=0;i<po->n_textures;i++)	{
			texture_list_index[i] = alt_textures[i];
			texture_list[i] = &GameBitmaps[alt_textures[i].index];
		}
   }
	else
   {
		for (i=0;i<po->n_textures;i++)	{
			texture_list_index[i] = ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]];
			texture_list[i] = &GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index];
		}
   }

	// Make sure the textures for this object are paged in...
	piggy_page_flushed = 0;
	for (i=0;i<po->n_textures;i++)	
		PIGGY_PAGE_IN( texture_list_index[i] );
	// Hmmm... cache got flushed in the middle of paging all these in,
	// so we need to reread them all in.
	if (piggy_page_flushed)	{
		piggy_page_flushed = 0;
		for (i=0;i<po->n_textures;i++)	
			PIGGY_PAGE_IN( texture_list_index[i] );
	}
	// Make sure that they can all fit in memory.
	Assert( piggy_page_flushed == 0 );

	g3_start_instance_matrix(pos,orient);

	g3_set_interp_points(robot_points);

	if (flags == 0)		//draw entire object

		g3_draw_polygon_model(po->model_data,texture_list,anim_angles,light,glow_values);

	else {
		int i;
	
		for (i=0;flags;flags>>=1,i++)
			if (flags & 1) {
				vms_vector ofs;

				Assert(i < po->n_models);

				//if submodel, rotate around its center point, not pivot point
	
				vm_vec_avg(&ofs,&po->submodel_mins[i],&po->submodel_maxs[i]);
				vm_vec_negate(&ofs);
				g3_start_instance_matrix(&ofs,NULL);
	
				g3_draw_polygon_model(&po->model_data[po->submodel_ptrs[i]],texture_list,anim_angles,light,glow_values);
	
				g3_done_instance();
			}	
	}

	g3_done_instance();
}

void free_polygon_models()
{
	int i;

	for (i=0;i<N_polygon_models;i++) {
		free_model(&Polygon_models[i]);
	}

}

void polyobj_find_min_max(polymodel *pm)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data,type;
	int m;
	vms_vector *big_mn,*big_mx;
	
	big_mn = &pm->mins;
	big_mx = &pm->maxs;

	for (m=0;m<pm->n_models;m++) {
		vms_vector *mn,*mx,*ofs;

		mn = &pm->submodel_mins[m];
		mx = &pm->submodel_maxs[m];
		ofs= &pm->submodel_offsets[m];

		data = (ushort *)&pm->model_data[pm->submodel_ptrs[m]];
	
		type = *data++;
	
		Assert(type == 7 || type == 1);
	
		nverts = *data++;
	
		if (type==7)
			data+=2;		//skip start & pad
	
		vp = (vms_vector *) data;
	
		*mn = *mx = *vp++; nverts--;

		if (m==0)
			*big_mn = *big_mx = *mn;
	
		while (nverts--) {
			if (vp->x > mx->x) mx->x = vp->x;
			if (vp->y > mx->y) mx->y = vp->y;
			if (vp->z > mx->z) mx->z = vp->z;
	
			if (vp->x < mn->x) mn->x = vp->x;
			if (vp->y < mn->y) mn->y = vp->y;
			if (vp->z < mn->z) mn->z = vp->z;
	
			if (vp->x+ofs->x > big_mx->x) big_mx->x = vp->x+ofs->x;
			if (vp->y+ofs->y > big_mx->y) big_mx->y = vp->y+ofs->y;
			if (vp->z+ofs->z > big_mx->z) big_mx->z = vp->z+ofs->z;
	
			if (vp->x+ofs->x < big_mn->x) big_mn->x = vp->x+ofs->x;
			if (vp->y+ofs->y < big_mn->y) big_mn->y = vp->y+ofs->y;
			if (vp->z+ofs->z < big_mn->z) big_mn->z = vp->z+ofs->z;
	
			vp++;
		}
	}
}

extern short highest_texture_num;	//from the 3d

char Pof_names[MAX_POLYGON_MODELS][FILENAME_LEN];

//returns the number of this model
#ifndef DRIVE
int load_polygon_model(char *filename,int n_textures,int first_texture,robot_info *r)
#else
int load_polygon_model(char *filename,int n_textures,grs_bitmap ***textures)
#endif
{
	#ifdef DRIVE
	#define r NULL
	#endif

	Assert(N_polygon_models < MAX_POLYGON_MODELS);
	Assert(n_textures < MAX_POLYOBJ_TEXTURES);

	Assert(strlen(filename) <= 12);
	strcpy(Pof_names[N_polygon_models],filename);

	read_model_file(&Polygon_models[N_polygon_models],filename,r);

	polyobj_find_min_max(&Polygon_models[N_polygon_models]);

	g3_init_polygon_model(Polygon_models[N_polygon_models].model_data);

	if (highest_texture_num+1 != n_textures)
		Error("Model <%s> references %d textures but specifies %d.",filename,highest_texture_num+1,n_textures);

	Polygon_models[N_polygon_models].n_textures = n_textures;
	Polygon_models[N_polygon_models].first_texture = first_texture;
	Polygon_models[N_polygon_models].simpler_model = 0;

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
void draw_model_picture(int mn,vms_angvec *orient_angles)
{
	vms_vector	temp_pos=ZERO_VECTOR;
	vms_matrix	temp_orient = IDENTITY_MATRIX;
	g3s_lrgb	lrgb = { f1_0, f1_0, f1_0 };

	Assert(mn>=0 && mn<N_polygon_models);

	gr_clear_canvas( BM_XRGB(0,0,0) );
	g3_start_frame();
	g3_set_view_matrix(&temp_pos,&temp_orient,0x9000);

	if (Polygon_models[mn].rad != 0)
		temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST,Polygon_models[mn].rad,BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;

	vm_angles_2_matrix(&temp_orient, orient_angles);
	draw_polygon_model(&temp_pos,&temp_orient,NULL,mn,0,lrgb,NULL,NULL);
	g3_end_frame();
}

/*
 * reads a polymodel structure from a PHYSFS_file
 */
extern void polymodel_read(polymodel *pm, PHYSFS_file *fp)
{
	int i;

	pm->n_models = PHYSFSX_readInt(fp);
	pm->model_data_size = PHYSFSX_readInt(fp);
	pm->model_data = (ubyte *)(size_t)PHYSFSX_readInt(fp); // garbage, read it anyway just for consistency
	for (i = 0; i < MAX_SUBMODELS; i++)
		pm->submodel_ptrs[i] = PHYSFSX_readInt(fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		PHYSFSX_readVector(&(pm->submodel_offsets[i]), fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		PHYSFSX_readVector(&(pm->submodel_norms[i]), fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		PHYSFSX_readVector(&(pm->submodel_pnts[i]), fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		pm->submodel_rads[i] = PHYSFSX_readFix(fp);
	PHYSFS_read(fp, pm->submodel_parents, MAX_SUBMODELS, 1);
	for (i = 0; i < MAX_SUBMODELS; i++)
		PHYSFSX_readVector(&(pm->submodel_mins[i]), fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		PHYSFSX_readVector(&(pm->submodel_maxs[i]), fp);
	PHYSFSX_readVector(&(pm->mins), fp);
	PHYSFSX_readVector(&(pm->maxs), fp);
	pm->rad = PHYSFSX_readFix(fp);
	pm->n_textures = PHYSFSX_readByte(fp);
	pm->first_texture = PHYSFSX_readShort(fp);
	pm->simpler_model = PHYSFSX_readByte(fp);
}

/*
 * reads n polymodel structs from a PHYSFS_file
 */
extern int polymodel_read_n(polymodel *pm, int n, PHYSFS_file *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		pm[i].n_models = PHYSFSX_readInt(fp);
		pm[i].model_data_size = PHYSFSX_readInt(fp);
		pm->model_data = (ubyte *)(size_t)PHYSFSX_readInt(fp); // garbage, read it anyway just for consistency
		for (j = 0; j < MAX_SUBMODELS; j++)
			pm[i].submodel_ptrs[j] = PHYSFSX_readInt(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_offsets[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_norms[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_pnts[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			pm[i].submodel_rads[j] = PHYSFSX_readFix(fp);
		PHYSFS_read(fp, pm[i].submodel_parents, MAX_SUBMODELS, 1);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_mins[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_maxs[j]), fp);
		PHYSFSX_readVector(&(pm[i].mins), fp);
		PHYSFSX_readVector(&(pm[i].maxs), fp);
		pm[i].rad = PHYSFSX_readFix(fp);
		pm[i].n_textures = PHYSFSX_readByte(fp);
		pm[i].first_texture = PHYSFSX_readShort(fp);
		pm[i].simpler_model = PHYSFSX_readByte(fp);
	}
	return i;
}

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void polygon_model_data_read(polymodel *pm, PHYSFS_file *fp)
{
	pm->model_data = d_malloc(pm->model_data_size);
	Assert(pm->model_data != NULL);
	PHYSFS_read(fp, pm->model_data, sizeof(ubyte), pm->model_data_size);
#ifdef WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(pm->model_data);
#endif
	//verify(pm->model_data);
	g3_init_polygon_model(pm->model_data);
}
