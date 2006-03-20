#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "polyobj.h"
#include "vecmat.h"
#include "3d.h"
#include "error.h"

byte model_tree[MAX_SUBMODELS][MAX_SUBMODELS+1];
g3s_instance_context context_array[MAX_SUBMODELS];


void new_draw_polygon_model(polymodel *po,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,fix light,fix *glow_values)
{
	int i;
	g3s_instance_context saved_context;

	//first, build a tree of the parts.  This is ugly.  The tree should be stored
	//with the model

	for (i=0;i<po->n_models;i++)
		model_tree[i][0] = -1;

	Assert(po->submodel_parents[0] == 0xff);

	for (i=1;i<po->n_models;i++) {
		int p,j;
		p = po->submodel_parents[i];
		for (j=0;model_tree[p][j]!=-1;j++)
			;
		model_tree[p][j]=i;
		model_tree[p][j+1]=-1;
	}

	//now, go through model recursively, saving instance context for each submodel

	instance_model_part(po,0,anim_angles);

	//now, do whatever sorting on the part of the model you want

		//we don't do any sorting, now

	//lastly, render our parts

	g3_get_instance_context(&saved_context);	//save before we start mucking with it

	for (i=0;i<po->n_models;i++) {

		g3_set_instance_context(&context_array[i]);

		g3_draw_polygon_model(&po->model_data[po->submodel_ptrs[i]],model_bitmaps,anim_angles,light,glow_values);
	}

	g3_set_instance_context(&saved_context);	//restore now that we're done

}


instance_model_part(polymodel *po,int part_number,vms_angvec *anim_angles)
{
	int i;

	g3_start_instance_angles(&po->submodel_offsets[part_number],anim_angles?&anim_angles[part_number]:NULL);

	g3_get_instance_context(&context_array[part_number]);

	for (i=0;model_tree[part_number][i]!=-1;i++)
		instance_model_part(po,model_tree[part_number][i],anim_angles);

	g3_done_instance();

}
