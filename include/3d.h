/* $Id: 3d.h,v 1.8 2002-10-28 21:13:17 btb Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for 3d library
 *
 * Old Log:
 * Revision 1.2  1995/09/14  14:08:58  allender
 * return value for g3_draw_sphere
 *
 * Revision 1.1  1995/05/05  08:48:41  allender
 * Initial revision
 *
 * Revision 1.34  1994/11/11  19:22:14  matt
 * Added new function, g3_calc_point_depth()
 *
 * Revision 1.33  1994/09/09  14:23:58  matt
 * Added support for glowing textures, to add engine glow to Descent.
 *
 * Revision 1.32  1994/09/01  10:42:27  matt
 * Blob routine, renamed g3_draw_bitmap(), now takes seperate 3d width & height.
 *
 * Revision 1.31  1994/07/29  18:16:14  matt
 * Added instance by angles, and corrected parms for g3_init()
 *
 * Revision 1.30  1994/07/25  00:00:00  matt
 * Made 3d no longer deal with point numbers, but only with pointers.
 *
 * Revision 1.29  1994/07/22  17:57:27  matt
 * Changed the name of the rod functions, and took out some debugging code
 *
 * Revision 1.28  1994/06/07  16:49:12  matt
 * Made interpreter take lighting value as parm, rather than in global var
 *
 * Revision 1.27  1994/05/31  18:35:28  matt
 * Added light value to g3_draw_facing_bitmap()
 *
 * Revision 1.26  1994/05/30  22:48:04  matt
 * Added support for morph effect
 *
 * Revision 1.25  1994/05/30  11:34:57  matt
 * Added g3_set_special_render() to allow a user to specify functions to
 * call for 2d draws.
 *
 * Revision 1.24  1994/05/19  21:46:31  matt
 * Moved texture lighting out of 3d and into the game
 *
 * Revision 1.23  1994/05/14  15:26:48  matt
 * Added extern for polyobj outline flag
 *
 * Revision 1.22  1994/04/19  18:26:33  matt
 * Added g3_draw_sphere() function.
 *
 * Revision 1.21  1994/03/25  18:22:28  matt
 * g3_draw_polygon_model() now takes ptr to list of angles
 *
 * Revision 1.20  1994/03/15  21:23:23  matt
 * Added interpreter functions
 *
 * Revision 1.19  1994/02/15  17:37:34  matt
 * New function, g3_draw_blob()
 *
 * Revision 1.18  1994/02/09  11:47:47  matt
 * Added rod & delta point functions
 *
 * Revision 1.17  1994/01/26  12:38:11  matt
 * Added function g3_compute_lighting_value()
 *
 * Revision 1.16  1994/01/25  18:00:02  yuan
 * Fixed variable beam_brightness...
 *
 * Revision 1.15  1994/01/24  14:08:34  matt
 * Added instancing functions
 *
 * Revision 1.14  1994/01/22  18:21:48  matt
 * New lighting stuff now done in 3d; g3_draw_tmap() takes lighting parm
 *
 * Revision 1.13  1994/01/20  17:21:24  matt
 * New function g3_compute_sky_polygon()
 *
 * Revision 1.12  1994/01/14  17:20:25  matt
 * Added prototype for new function g3_draw_horizon()
 *
 * Revision 1.10  1993/12/20  20:21:52  matt
 * Added g3_point_2_vec()
 *
 * Revision 1.9  1993/12/07  23:05:47  matt
 * Fixed mistyped function name.
 *
 * Revision 1.8  1993/12/05  23:47:03  matt
 * Added function g3_draw_line_ptrs()
 *
 * Revision 1.7  1993/12/05  23:13:22  matt
 * Added prototypes for g3_rotate_point() and g3_project_point()
 *
 * Revision 1.6  1993/12/05  23:03:28  matt
 * Changed uvl structs to g3s_uvl
 *
 * Revision 1.5  1993/11/22  10:51:09  matt
 * Moved uvl structure here from segment.h, made texture map functions use it
 *
 * Revision 1.4  1993/11/21  20:08:31  matt
 * Added function g3_draw_object()
 *
 * Revision 1.3  1993/11/04  18:49:19  matt
 * Added system to only rotate points once per frame
 *
 * Revision 1.2  1993/11/04  08:16:06  mike
 * Add light field (p3_l) to g3s_point.
 *
 * Revision 1.1  1993/10/29  22:20:56  matt
 * Initial revision
 *
 */

#ifndef _3D_H
#define _3D_H

#include "fix.h"
#include "vecmat.h" //the vector/matrix library
#include "gr.h"

extern int g3d_interp_outline;      //if on, polygon models outlined in white

extern vms_vector Matrix_scale;     //how the matrix is currently scaled

extern short highest_texture_num;

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
typedef struct g3s_uvl {
	fix u,v,l;
} g3s_uvl;

//Stucture to store clipping codes in a word
typedef struct g3s_codes {
	ubyte or,and;   //or is low byte, and is high byte
} g3s_codes;

//flags for point structure
#define PF_PROJECTED    1   //has been projected, so sx,sy valid
#define PF_OVERFLOW     2   //can't project
#define PF_TEMP_POINT   4   //created during clip
#define PF_UVS          8   //has uv values set
#define PF_LS           16  //has lighting values set

//clipping codes flags

#define CC_OFF_LEFT     1
#define CC_OFF_RIGHT    2
#define CC_OFF_BOT      4
#define CC_OFF_TOP      8
#define CC_BEHIND       0x80

//Used to store rotated points for mines.  Has frame count to indictate
//if rotated, and flag to indicate if projected.
typedef struct g3s_point {
	vms_vector p3_vec;  //x,y,z of rotated point
#ifdef D1XD3D
	vms_vector p3_orig;
#endif
	fix p3_u,p3_v,p3_l; //u,v,l coords
	fix p3_sx,p3_sy;    //screen x&y
	ubyte p3_codes;     //clipping codes
	ubyte p3_flags;     //projected?
	short p3_pad;       //keep structure longword aligned
} g3s_point;

//macros to reference x,y,z elements of a 3d point
#define p3_x p3_vec.x
#define p3_y p3_vec.y
#define p3_z p3_vec.z

//An object, such as a robot
typedef struct g3s_object {
	vms_vector o3_pos;       //location of this object
	vms_angvec o3_orient;    //orientation of this object
	int o3_nverts;           //number of points in the object
	int o3_nfaces;           //number of faces in the object

	//this will be filled in later

} g3s_object;

//Functions in library

//3d system startup and shutdown:

//initialize the 3d system
void g3_init(void);

//close down the 3d system
void g3_close(void);


//Frame setup functions:

//start the frame
void g3_start_frame(void);

//set view from x,y,z & p,b,h, zoom.  Must call one of g3_set_view_*() 
void g3_set_view_angles(vms_vector *view_pos,vms_angvec *view_orient,fix zoom);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(vms_vector *view_pos,vms_matrix *view_matrix,fix zoom);

//end the frame
void g3_end_frame(void);

//draw a horizon
void g3_draw_horizon(int sky_color,int ground_color);

//get vectors that are edge of horizon
int g3_compute_sky_polygon(fix *points_2d,vms_vector *vecs);

//Instancing

//instance at specified point with specified orientation
void g3_start_instance_matrix(vms_vector *pos,vms_matrix *orient);

//instance at specified point with specified orientation
void g3_start_instance_angles(vms_vector *pos,vms_angvec *angles);

//pops the old context
void g3_done_instance();

//Misc utility functions:

//get current field of view.  Fills in angle for x & y
void g3_get_FOV(fixang *fov_x,fixang *fov_y);

//get zoom.  For a given window size, return the zoom which will achieve
//the given FOV along the given axis.
fix g3_get_zoom(char axis,fixang fov,short window_width,short window_height);

//returns the normalized, unscaled view vectors
void g3_get_view_vectors(vms_vector *forward,vms_vector *up,vms_vector *right);

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(vms_vector *v,vms_vector *norm);

//Point definition and rotation functions:

//specify the arrays refered to by the 'pointlist' parms in the following
//functions.  I'm not sure if we will keep this function, but I need
//it now.
//void g3_set_points(g3s_point *points,vms_vector *vecs);

//returns codes_and & codes_or of a list of points numbers
g3s_codes g3_check_codes(int nv,g3s_point **pointlist);

//rotates a point. returns codes.  does not check if already rotated
ubyte g3_rotate_point(g3s_point *dest,vms_vector *src);

//projects a point
void g3_project_point(g3s_point *point);

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(vms_vector *pnt);

//from a 2d point, compute the vector through that point
void g3_point_2_vec(vms_vector *v,short sx,short sy);

//code a point.  fills in the p3_codes field of the point, and returns the codes
ubyte g3_code_point(g3s_point *point);

//delta rotation functions
vms_vector *g3_rotate_delta_x(vms_vector *dest,fix dx);
vms_vector *g3_rotate_delta_y(vms_vector *dest,fix dy);
vms_vector *g3_rotate_delta_z(vms_vector *dest,fix dz);
vms_vector *g3_rotate_delta_vec(vms_vector *dest,vms_vector *src);
ubyte g3_add_delta_vec(g3s_point *dest,g3s_point *src,vms_vector *deltav);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
bool g3_draw_poly(int nv,g3s_point **pointlist);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
bool g3_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bm);

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int g3_draw_sphere(g3s_point *pnt,fix rad);

//@@//return ligting value for a point
//@@fix g3_compute_lighting_value(g3s_point *rotated_point,fix normval);


//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
bool g3_check_and_draw_poly(int nv,g3s_point **pointlist,vms_vector *norm,vms_vector *pnt);
bool g3_check_and_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bm,vms_vector *norm,vms_vector *pnt);

//draws a line. takes two points.
bool g3_draw_line(g3s_point *p0,g3s_point *p1);

//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
bool g3_draw_rod_flat(g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width);

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
bool g3_draw_rod_tmap(grs_bitmap *bitmap,g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width,fix light);

//draws a bitmap with the specified 3d width & height
//returns 1 if off screen, 0 if drew
bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm, int orientation);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(void (*tmap_drawer)(),void (*flat_drawer)(),int (*line_drawer)(fix, fix, fix, fix));

//Object functions:

//gives the interpreter an array of points to use
void g3_set_interp_points(g3s_point *pointlist);

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
bool g3_draw_polygon_model(void *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,fix light,fix *glow_values);

//init code for bitmap models
void g3_init_polygon_model(void *model_ptr);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void *model_ptr);

//alternate interpreter for morphing object
bool g3_draw_morphing_model(void *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,fix light,vms_vector *new_points);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

// routine to convert little to big endian in polygon model data
#ifdef WORDS_BIGENDIAN
void swap_polygon_model_data(ubyte *data);
#else
#define swap_polygon_model_data(data)
#endif

#endif

