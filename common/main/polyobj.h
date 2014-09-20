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
 * Header for polyobj.c, the polygon object code
 *
 */


#ifndef _POLYOBJ_H
#define _POLYOBJ_H

#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#include "robot.h"
#include "piggy.h"

#ifdef __cplusplus
#include "pack.h"

struct glow_values_t;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_POLYGON_MODELS 85
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_POLYGON_MODELS 200
#endif

// array of names of currently-loaded models
extern char Pof_names[MAX_POLYGON_MODELS][13];

//for each model, a model number for dying & dead variants, or -1 if none
extern int Dying_modelnums[MAX_POLYGON_MODELS];
extern int Dead_modelnums[MAX_POLYGON_MODELS];
#endif
#define MAX_SUBMODELS 10

//used to describe a polygon model
struct polymodel : prohibit_void_ptr<polymodel>
{
	int     n_models;
	int     model_data_size;
	std::unique_ptr<ubyte[]>   model_data;
	array<int, MAX_SUBMODELS> submodel_ptrs;
	array<vms_vector, MAX_SUBMODELS> submodel_offsets;
	array<vms_vector, MAX_SUBMODELS> submodel_norms;   // norm for sep plane
	array<vms_vector, MAX_SUBMODELS> submodel_pnts;    // point on sep plane
	array<fix, MAX_SUBMODELS> submodel_rads;       // radius for each submodel
	array<ubyte, MAX_SUBMODELS> submodel_parents;    // what is parent for each submodel
	array<vms_vector, MAX_SUBMODELS> submodel_mins;
	array<vms_vector, MAX_SUBMODELS> submodel_maxs;
	vms_vector mins,maxs;                       // min,max for whole model
	fix     rad;
	ubyte   n_textures;
	ushort  first_texture;
	ubyte   simpler_model;                      // alternate model with less detail (0 if none, model_num+1 else)
	//vms_vector min,max;
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
// array of pointers to polygon objects
extern array<polymodel, MAX_POLYGON_MODELS> Polygon_models;
#endif

// how many polygon objects there are
extern unsigned N_polygon_models;

void free_polygon_models();
void init_polygon_models();

int load_polygon_model(const char *filename,int n_textures,int first_texture,robot_info *r);

// draw a polygon model
void draw_polygon_model(vms_vector *pos,vms_matrix *orient,vms_angvec *anim_angles,int model_num,int flags,g3s_lrgb lrgb,glow_values_t *glow_values,bitmap_index alt_textures[]);

// fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(const char *filename,vms_vector *gun_points, vms_vector *gun_dirs, int *gun_submodels);

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void draw_model_picture(uint_fast32_t mn,vms_angvec *orient_angles);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_POLYOBJ_TEXTURES 50
#elif defined(DXX_BUILD_DESCENT_II)
// free up a model, getting rid of all its memory
void free_model(polymodel *po);

#define MAX_POLYOBJ_TEXTURES 100
static const unsigned N_D2_POLYGON_MODELS = 166;
#endif
extern array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
extern bitmap_index texture_list_index[MAX_POLYOBJ_TEXTURES];
#endif
#define MAX_POLYGON_VECS 1000
extern g3s_point robot_points[MAX_POLYGON_VECS];

/*
 * reads a polymodel structure from a PHYSFS_file
 */
extern void polymodel_read(polymodel *pm, PHYSFS_file *fp);
void polymodel_write(PHYSFS_file *fp, const polymodel &pm);

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void polygon_model_data_read(polymodel *pm, PHYSFS_file *fp);
void robot_set_angles(robot_info *r,polymodel *pm,vms_angvec angs[N_ANIM_STATES][MAX_SUBMODELS]);

#endif

#endif /* _POLYOBJ_H */
