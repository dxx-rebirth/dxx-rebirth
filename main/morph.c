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
 * Morphing code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
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

morph_data morph_objects[MAX_MORPH_OBJECTS];

//returns ptr to data for this object, or NULL if none
morph_data *find_morph_data(object *obj)
{
	int i;

	if (Newdemo_state == ND_STATE_PLAYBACK) {
		morph_objects[0].obj = obj;
		return &morph_objects[0];
	}

	for (i=0;i<MAX_MORPH_OBJECTS;i++)
		if (morph_objects[i].obj == obj)
			return &morph_objects[i];

	return NULL;
}


//takes pm, fills in min & max
void find_min_max(polymodel *pm,int submodel_num,vms_vector *minv,vms_vector *maxv)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data,type;

	data = (ushort *) &pm->model_data[pm->submodel_ptrs[submodel_num]];

	type = *data++;

	Assert(type == 7 || type == 1);

	nverts = *data++;

	if (type==7)
		data+=2;		//skip start & pad

	vp = (vms_vector *) data;

	*minv = *maxv = *vp++; nverts--;

	while (nverts--) {
		if (vp->x > maxv->x) maxv->x = vp->x;
		if (vp->y > maxv->y) maxv->y = vp->y;
		if (vp->z > maxv->z) maxv->z = vp->z;

		if (vp->x < minv->x) minv->x = vp->x;
		if (vp->y < minv->y) minv->y = vp->y;
		if (vp->z < minv->z) minv->z = vp->z;

		vp++;
	}

}

#define MORPH_RATE (f1_0*3)

fix morph_rate = MORPH_RATE;

void init_points(polymodel *pm,vms_vector *box_size,int submodel_num,morph_data *md)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data,type;
	int i;

	data = (ushort *) &pm->model_data[pm->submodel_ptrs[submodel_num]];

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

	vp = (vms_vector *) data;

	while (nverts--) {
		fix k,dist;

		if (box_size) {
			fix t;

			k = 0x7fffffff;

			if (vp->x && f2i(box_size->x)<abs(vp->x)/2 && (t = fixdiv(box_size->x,abs(vp->x))) < k) k=t;
			if (vp->y && f2i(box_size->y)<abs(vp->y)/2 && (t = fixdiv(box_size->y,abs(vp->y))) < k) k=t;
			if (vp->z && f2i(box_size->z)<abs(vp->z)/2 && (t = fixdiv(box_size->z,abs(vp->z))) < k) k=t;

			if (k==0x7fffffff) k=0;

		}
		else
			k=0;

		vm_vec_copy_scale(&md->morph_vecs[i],vp,k);

		dist = vm_vec_normalized_dir_quick(&md->morph_deltas[i],vp,&md->morph_vecs[i]);

		md->morph_times[i] = fixdiv(dist,morph_rate);

		if (md->morph_times[i] != 0)
			md->n_morphing_points[submodel_num]++;

		vm_vec_scale(&md->morph_deltas[i],morph_rate);

		vp++; i++;

	}

}

void update_points(polymodel *pm,int submodel_num,morph_data *md)
{
	ushort nverts;
	vms_vector *vp;
	ushort *data,type;
	int i;

	data = (ushort *) &pm->model_data[pm->submodel_ptrs[submodel_num]];

	type = *data++;

	Assert(type == 7 || type == 1);

	nverts = *data++;

	if (type==7) {
		i = *data++;		//get start point number
		data++;				//skip pad
	}
	else
		i = 0;				//start at zero

	vp = (vms_vector *) data;

	while (nverts--) {

		if (md->morph_times[i])		//not done yet
		{

			if ((md->morph_times[i] -= FrameTime) <= 0) {
				md->morph_vecs[i] = *vp;
				md->morph_times[i] = 0;
				md->n_morphing_points[submodel_num]--;
			}
			else
				vm_vec_scale_add2(&md->morph_vecs[i],&md->morph_deltas[i],FrameTime);
		}

		vp++; i++;
	}
}


//process the morphing object for one frame
void do_morph_frame(object *obj)
{
	int i;
	polymodel *pm;
	morph_data *md;

	md = find_morph_data(obj);

	if (md == NULL) {					//maybe loaded half-morphed from disk
		obj->flags |= OF_SHOULD_BE_DEAD;		//..so kill it
		return;
	}

	pm = &Polygon_models[md->obj->rtype.pobj_info.model_num];

	for (i=0;i<pm->n_models;i++)
		if (md->submodel_active[i]==1) {

			update_points(pm,i,md);

			if (md->n_morphing_points[i] == 0) {		//maybe start submodel
				int t;

				md->submodel_active[i] = 2;		//not animating, just visible

				md->n_submodels_active--;		//this one done animating

				for (t=0;t<pm->n_models;t++)
					if (pm->submodel_parents[t] == i) {		//start this one

						init_points(pm,NULL,t,md);
						md->n_submodels_active++;
						md->submodel_active[t] = 1;

					}
			}

		}

	if (!md->n_submodels_active) {			//done morphing!

		md->obj->control_type = md->morph_save_control_type;
		md->obj->movement_type = md->morph_save_movement_type;

		md->obj->render_type = RT_POLYOBJ;

		md->obj->mtype.phys_info = md->morph_save_phys_info;

		md->obj = NULL;
	}

}

vms_vector morph_rotvel = {0x4000,0x2000,0x1000};

void init_morphs()
{
	int i;

	for (i=0;i<MAX_MORPH_OBJECTS;i++)
		morph_objects[i].obj = NULL;
}


//make the object morph
void morph_start(object *obj)
{
	polymodel *pm;
	vms_vector pmmin,pmmax;
	vms_vector box_size;
	int i;
	morph_data *md;

	for (i=0;i<MAX_MORPH_OBJECTS;i++)
		if (morph_objects[i].obj == NULL || morph_objects[i].obj->type==OBJ_NONE  || morph_objects[i].obj->signature!=morph_objects[i].Morph_sig)
			break;

	if (i==MAX_MORPH_OBJECTS)		//no free slots
		return;

	md = &morph_objects[i];

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

	find_min_max(pm,0,&pmmin,&pmmax);

	box_size.x = max(-pmmin.x,pmmax.x) / 2;
	box_size.y = max(-pmmin.y,pmmax.y) / 2;
	box_size.z = max(-pmmin.z,pmmax.z) / 2;

	for (i=0;i<MAX_VECS;i++)		//clear all points
		md->morph_times[i] = 0;

	for (i=1;i<MAX_SUBMODELS;i++)		//clear all parts
		md->submodel_active[i] = 0;

	md->submodel_active[0] = 1;		//1 means visible & animating

	md->n_submodels_active = 1;

	//now, project points onto surface of box

	init_points(pm,&box_size,0,md);

}

void draw_model(polymodel *pm,int submodel_num,vms_angvec *anim_angles,g3s_lrgb light,morph_data *md)
{
	int i,mn;
	int facing;
	int sort_list[MAX_SUBMODELS],sort_n;


	//first, sort the submodels

	sort_list[0] = submodel_num;
	sort_n = 1;

	for (i=0;i<pm->n_models;i++)

		if (md->submodel_active[i] && pm->submodel_parents[i]==submodel_num) {

			facing = g3_check_normal_facing(&pm->submodel_pnts[i],&pm->submodel_norms[i]);

			if (!facing)

				sort_list[sort_n++] = i;

			else {		//put at start
				int t;

				for (t=sort_n;t>0;t--)
					sort_list[t] = sort_list[t-1];

				sort_list[0] = i;

				sort_n++;


			}

		}
	

	//now draw everything

	for (i=0;i<sort_n;i++) {

		mn = sort_list[i];

		if (mn == submodel_num) {
 			int i;

			for (i=0;i<pm->n_textures;i++)		{
				texture_list_index[i] = ObjBitmaps[ObjBitmapPtrs[pm->first_texture+i]];
				texture_list[i] = &GameBitmaps[ObjBitmaps[ObjBitmapPtrs[pm->first_texture+i]].index];
			}

			// Make sure the textures for this object are paged in...
			piggy_page_flushed = 0;
			for (i=0;i<pm->n_textures;i++)	
				PIGGY_PAGE_IN( texture_list_index[i] );
			// Hmmm... cache got flushed in the middle of paging all these in,
			// so we need to reread them all in.
			if (piggy_page_flushed)	{
				piggy_page_flushed = 0;
				for (i=0;i<pm->n_textures;i++)	
					PIGGY_PAGE_IN( texture_list_index[i] );
			}
			// Make sure that they can all fit in memory.
			Assert( piggy_page_flushed == 0 );

			g3_draw_morphing_model(&pm->model_data[pm->submodel_ptrs[submodel_num]],texture_list,anim_angles,light,&md->morph_vecs[md->submodel_startpoints[submodel_num]]);

		}
		else {

			vms_matrix orient;

			vm_angles_2_matrix(&orient,&anim_angles[mn]);

			g3_start_instance_matrix(&pm->submodel_offsets[mn],&orient);

			draw_model(pm,mn,anim_angles,light,md);

			g3_done_instance();

		}
	}

}

void draw_morph_object(object *obj)
{
//	int save_light;
	polymodel *po;
	g3s_lrgb light;
	morph_data *md;

	md = find_morph_data(obj);
	Assert(md != NULL);

	Assert(obj->rtype.pobj_info.model_num < N_polygon_models);

	po=&Polygon_models[obj->rtype.pobj_info.model_num];

	light = compute_object_light(obj,NULL);

	g3_start_instance_matrix(&obj->pos,&obj->orient);
	g3_set_interp_points(robot_points);

	draw_model(po,0,obj->rtype.pobj_info.anim_angles,light,md);

	g3_done_instance();

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_morph_frame(md);
}
