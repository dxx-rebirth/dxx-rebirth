/* $Id: polyobj.h,v 1.7 2003-10-10 09:36:35 btb Exp $ */
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
 * Header for polyobj.c, the polygon object code
 *
 * Old Log:
 * Revision 1.2  1995/09/14  14:10:30  allender
 * two functions should be void
 *
 * Revision 1.1  1995/05/16  16:01:27  allender
 * Initial revision
 *
 * Revision 2.1  1995/02/27  18:21:54  john
 * Added extern for robot_points.
 *
 * Revision 2.0  1995/02/27  11:29:58  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.28  1995/01/12  12:10:16  adam
 * *** empty log message ***
 *
 * Revision 1.27  1994/11/11  19:28:58  matt
 * Added code to show low detail polygon models
 *
 * Revision 1.26  1994/11/10  14:03:05  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.25  1994/11/02  16:18:24  matt
 * Moved draw_model_picture() out of editor
 *
 * Revision 1.24  1994/10/18  10:55:57  adam
 * bumped MAX_POLYGON_OBJECTS up
 *
 * Revision 1.23  1994/10/17  21:35:01  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.22  1994/09/09  14:23:42  matt
 * Added glow code to polygon models for engine glow
 *
 * Revision 1.21  1994/08/26  18:03:43  matt
 * Added code to remap polygon model numbers by matching filenames
 *
 * Revision 1.20  1994/08/26  15:36:00  matt
 * Made eclips usable on more than one object at a time
 *
 * Revision 1.19  1994/07/22  20:44:23  matt
 * Killed unused fields in polygon model structure
 *
 * Revision 1.18  1994/06/16  17:52:11  matt
 * Made submodels rotate around their centers, not their pivot points
 *
 * Revision 1.17  1994/06/14  12:22:05  matt
 * Integrated with drive code, using #defines to switch versions
 *
 * Revision 1.16  1994/06/09  16:25:01  matt
 * Fixed confusion with two constants, MAX_SUBOBJS & MAX_SUBMODELS, which
 * were used for the same things, but had different values.
 *
 * Revision 1.15  1994/06/08  10:56:38  matt
 * Improved debris: now get submodel size from new POF files; debris now has
 * limited life; debris can now be blown up.
 *
 * Revision 1.14  1994/06/07  16:51:57  matt
 * Made object lighting work correctly; changed name of Ambient_light to
 * Dynamic_light; cleaned up polygobj object rendering a little.
 *
 * Revision 1.13  1994/05/26  21:08:59  matt
 * Moved robot stuff out of polygon model and into robot_info struct
 * Made new file, robot.c, to deal with robots
 *
 * Revision 1.12  1994/05/18  19:35:05  matt
 * Added fields for the rest of the subobj data
 *
 * Revision 1.11  1994/05/16  16:17:13  john
 * Bunch of stuff on my Inferno Task list May16-23
 *
 * Revision 1.10  1994/05/13  11:08:31  matt
 * Added support for multiple gun positions on polygon models
 *
 * Revision 1.9  1994/04/29  09:18:04  matt
 * Added support for multiple-piece explosions
 *
 * Revision 1.8  1994/04/28  18:44:18  matt
 * Took out code for old-style (non-interpreted) objects.
 *
 * Revision 1.7  1994/03/25  16:54:38  matt
 * draw_polygon_object() now takes pointer to animation data
 *
 * Revision 1.6  1994/03/15  17:44:33  matt
 * Changed a bunch of names
 *
 * Revision 1.5  1994/03/07  20:02:29  matt
 * Added pointer to normals in polyobj struct
 * Added prototype for draw_polygon_object()
 *
 * Revision 1.4  1994/03/01  17:16:19  matt
 * Lots of changes to support loadable binary ".pof" robot files
 *
 * Revision 1.3  1994/01/31  15:51:20  matt
 * Added ptr for rgb table for robot colors
 *
 * Revision 1.2  1994/01/28  13:52:01  matt
 * Added flesh to this previously skeletal file.
 *
 * Revision 1.1  1994/01/28  13:47:42  matt
 * Initial revision
 *
 *
 */


#ifndef _POLYOBJ_H
#define _POLYOBJ_H

#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#ifndef DRIVE
#include "robot.h"
#endif
#include "piggy.h"

#define MAX_POLYGON_MODELS 200

//used to describe a polygon model
typedef struct polymodel {
	int     n_models;
	int     model_data_size;
	ubyte   *model_data;
	int     submodel_ptrs[MAX_SUBMODELS];
	vms_vector submodel_offsets[MAX_SUBMODELS];
	vms_vector submodel_norms[MAX_SUBMODELS];   // norm for sep plane
	vms_vector submodel_pnts[MAX_SUBMODELS];    // point on sep plane
	fix     submodel_rads[MAX_SUBMODELS];       // radius for each submodel
	ubyte   submodel_parents[MAX_SUBMODELS];    // what is parent for each submodel
	vms_vector submodel_mins[MAX_SUBMODELS];
	vms_vector submodel_maxs[MAX_SUBMODELS];
	vms_vector mins,maxs;                       // min,max for whole model
	fix     rad;
	ubyte   n_textures;
	ushort  first_texture;
	ubyte   simpler_model;                      // alternate model with less detail (0 if none, model_num+1 else)
	//vms_vector min,max;
} __pack__ polymodel;

// array of pointers to polygon objects
extern polymodel Polygon_models[];

// switch to simpler model when the object has depth
// greater than this value times its radius.
extern int Simple_model_threshhold_scale;

// how many polygon objects there are
extern int N_polygon_models;


// array of names of currently-loaded models
extern char Pof_names[MAX_POLYGON_MODELS][13];

void init_polygon_models();

#ifndef DRIVE
int load_polygon_model(char *filename,int n_textures,int first_texture,robot_info *r);
#else
int load_polygon_model(char *filename,int n_textures,grs_bitmap ***textures);
#endif

// draw a polygon model
void draw_polygon_model(vms_vector *pos,vms_matrix *orient,vms_angvec *anim_angles,int model_num,int flags,fix light,fix *glow_values,bitmap_index alt_textures[]);

// fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(char *filename,vms_vector *gun_points, vms_vector *gun_dirs, int *gun_submodels);

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void draw_model_picture(int mn,vms_angvec *orient_angles);

// free up a model, getting rid of all its memory
void free_model(polymodel *po);

#define MAX_POLYOBJ_TEXTURES 100
extern grs_bitmap *texture_list[MAX_POLYOBJ_TEXTURES];
extern bitmap_index texture_list_index[MAX_POLYOBJ_TEXTURES];
extern g3s_point robot_points[];

#ifdef FAST_FILE_IO
#define polymodel_read(pm, fp) cfread(pm, sizeof(polymodel), 1, fp)
#define polymodel_read_n(pm, n, fp) cfread(pm, sizeof(polymodel), n, fp)
#else
/*
 * reads a polymodel structure from a CFILE
 */
extern void polymodel_read(polymodel *pm, CFILE *fp);

/*
 * reads n polymodel structs from a CFILE
 */
extern int polymodel_read_n(polymodel *pm, int n, CFILE *fp);
#endif

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void polygon_model_data_read(polymodel *pm, CFILE *fp);

#endif /* _POLYOBJ_H */
