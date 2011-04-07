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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/3d.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:06 $
 *
 * Header file for 3d library
 *
 * $Log: 3d.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:06  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:03  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.2  1995/09/14  14:08:58  allender
 * return value for g3_draw_sphere
 *
 * Revision 1.1  1995/05/05  08:48:41  allender
 * Initial revision
 *
 * 
 *
 */

#ifndef _3D_H
#define _3D_H

#include "fix.h"
#include "vecmat.h"	//the vector/matrix library
#include "gr.h"

struct object;

extern int g3d_interp_outline;		//if on, polygon models outlined in white
extern vms_vector Matrix_scale;     //how the matrix is currently scaled
extern short highest_texture_num;

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
typedef struct g3s_uvl {
	fix u,v,l;
} g3s_uvl;

//Structure for storing light color. Also uses l of g3s-uvl to add/compute mono (white) light
typedef struct g3s_lrgb {
	fix r,g,b;
} g3s_lrgb;

//Stucture to store clipping codes in a word
typedef struct g3s_codes {
	ubyte or,and;	//or is low byte, and is high byte
} g3s_codes;

//flags for point structure
#define PF_PROJECTED 	1	//has been projected, so sx,sy valid
#define PF_OVERFLOW		2	//can't project
#define PF_TEMP_POINT	4	//created during clip
#define PF_UVS				8	//has uv values set
#define PF_LS				16	//has lighting values set

//clipping codes flags

#define CC_OFF_LEFT	1
#define CC_OFF_RIGHT	2
#define CC_OFF_BOT	4
#define CC_OFF_TOP	8
#define CC_BEHIND		0x80

//Used to store rotated points for mines.  Has frame count to indictate
//if rotated, and flag to indicate if projected.
typedef struct g3s_point {
	vms_vector p3_vec;         //reference as vector...
	fix p3_u,p3_v,p3_l;
	fix p3_sx,p3_sy;		//screen x&y
	ubyte p3_codes;		//clipping codes
	ubyte p3_flags;		//projected?
	short p3_pad;			//keep structure longwork aligned
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
bool g3_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb,grs_bitmap *bm);

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
bool g3_check_and_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb, grs_bitmap *bm,vms_vector *norm,vms_vector *pnt);

//draws a line. takes two points.
bool g3_draw_line(g3s_point *p0,g3s_point *p1);

//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
bool g3_draw_rod_flat(g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width);

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
bool g3_draw_rod_tmap(grs_bitmap *bitmap,g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width,g3s_lrgb light);

//draws a bitmap with the specified 3d width & height 
//returns 1 if off screen, 0 if drew
bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(void (*tmap_drawer)(),void (*flat_drawer)(),int (*line_drawer)(fix, fix, fix, fix));

//Object functions:

//gives the interpreter an array of points to use
void g3_set_interp_points(g3s_point *pointlist);

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
bool g3_draw_polygon_model(void *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,g3s_lrgb light,fix *glow_values);

//init code for bitmap models
void g3_init_polygon_model(void *model_ptr);

//alternate interpreter for morphing object
bool g3_draw_morphing_model(void *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,g3s_lrgb light,vms_vector *new_points);

// check a polymodel for it's color and return it
int g3_poly_get_color(void *model_ptr);

// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(ubyte *data);

#endif

