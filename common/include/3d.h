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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for 3d library
 * except for functions implemented in interp.c
 *
 */

#pragma once

#include <cstdint>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "maths.h"
#include "vecmat.h" //the vector/matrix library
#include "fwd-gr.h"
#include <array>

struct grs_bitmap;

#if DXX_USE_EDITOR
namespace dcx {
extern int g3d_interp_outline;      //if on, polygon models outlined in white
}
#endif

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
struct g3s_uvl {
	fix u,v,l;
};

//Structure for storing light color. Also uses l of g3s-uvl to add/compute mono (white) light
struct g3s_lrgb {
	fix r,g,b;
};

//Stucture to store clipping codes in a word
struct g3s_codes {
	//or is low byte, and is high byte
	uint8_t uor = 0, uand = 0xff;
};

//flags for point structure
constexpr std::integral_constant<uint8_t, 1> PF_PROJECTED{};		//has been projected, so sx,sy valid
constexpr std::integral_constant<uint8_t, 2> PF_OVERFLOW{};		//can't project
#if !DXX_USE_OGL
constexpr std::integral_constant<uint8_t, 4> PF_TEMP_POINT{};	//created during clip
constexpr std::integral_constant<uint8_t, 8> PF_UVS{};			//has uv values set
constexpr std::integral_constant<uint8_t, 16> PF_LS{};			//has lighting values set
#endif

//clipping codes flags

constexpr std::integral_constant<uint8_t, 1> CC_OFF_LEFT{};
constexpr std::integral_constant<uint8_t, 2> CC_OFF_RIGHT{};
constexpr std::integral_constant<uint8_t, 4> CC_OFF_BOT{};
constexpr std::integral_constant<uint8_t, 8> CC_OFF_TOP{};
constexpr std::integral_constant<uint8_t, 0x80> CC_BEHIND{};

//Used to store rotated points for mines.  Has frame count to indictate
//if rotated, and flag to indicate if projected.
struct g3s_point {
	vms_vector p3_vec;  //x,y,z of rotated point
#if !DXX_USE_OGL
	fix p3_u,p3_v,p3_l; //u,v,l coords
#endif
	fix p3_sx,p3_sy;    //screen x&y
	ubyte p3_codes;     //clipping codes
	ubyte p3_flags;     //projected?
	uint16_t p3_last_generation;
};

//macros to reference x,y,z elements of a 3d point
#define p3_x p3_vec.x
#define p3_y p3_vec.y
#define p3_z p3_vec.z

#ifdef __cplusplus
//Functions in library

//Frame setup functions:

namespace dcx {

#if DXX_USE_OGL
typedef const g3s_point cg3s_point;
#else
typedef g3s_point cg3s_point;
#endif

//start the frame
void g3_start_frame(grs_canvas &);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(const vms_vector &view_pos,const vms_matrix &view_matrix,fix zoom);

//end the frame
#if DXX_USE_OGL
#define g3_end_frame() ogl_end_frame()
#else
#define g3_end_frame()
#endif

//Instancing

//instance at specified point with specified orientation
void g3_start_instance_matrix();
void g3_start_instance_matrix(const vms_vector &pos, const vms_matrix &orient);

//instance at specified point with specified orientation
void g3_start_instance_angles(const vms_vector &pos, const vms_angvec &angles);

//pops the old context
void g3_done_instance();

//Misc utility functions:

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(const vms_vector &v,const vms_vector &norm);

}

//Point definition and rotation functions:

//specify the arrays refered to by the 'pointlist' parms in the following
//functions.  I'm not sure if we will keep this function, but I need
//it now.
//void g3_set_points(g3s_point *points,vms_vector *vecs);

//returns codes_and & codes_or of a list of points numbers
g3s_codes g3_check_codes(int nv,g3s_point **pointlist);

namespace dcx {

//rotates a point. returns codes.  does not check if already rotated
ubyte g3_rotate_point(g3s_point &dest,const vms_vector &src);
static inline g3s_point g3_rotate_point(const vms_vector &src) __attribute_warn_unused_result;
static inline g3s_point g3_rotate_point(const vms_vector &src)
{
	g3s_point dest;
	return g3_rotate_point(dest, src), dest;
}

//projects a point
void g3_project_point(g3s_point &point);

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(const vms_vector &pnt);

//from a 2d point, compute the vector through that point
void g3_point_2_vec(vms_vector &v,short sx,short sy);

//code a point.  fills in the p3_codes field of the point, and returns the codes
ubyte g3_code_point(g3s_point &point);

//delta rotation functions
void g3_rotate_delta_vec(vms_vector &dest,const vms_vector &src);

ubyte g3_add_delta_vec(g3s_point &dest,const g3s_point &src,const vms_vector &deltav);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
void _g3_draw_poly(grs_canvas &, uint_fast32_t nv, cg3s_point *const *pointlist, uint8_t color);
template <std::size_t N>
static inline void g3_draw_poly(grs_canvas &canvas, const uint_fast32_t nv, const std::array<cg3s_point *, N> &pointlist, const uint8_t color)
{
	_g3_draw_poly(canvas, nv, &pointlist[0], color);
}

constexpr std::integral_constant<std::size_t, 64> MAX_POINTS_PER_POLY{};

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
void _g3_draw_tmap(grs_canvas &canvas, unsigned nv, cg3s_point *const *pointlist, const g3s_uvl *uvl_list, const g3s_lrgb *light_rgb, grs_bitmap &bm);

template <std::size_t N>
static inline void g3_draw_tmap(grs_canvas &canvas, unsigned nv, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	static_assert(N <= MAX_POINTS_PER_POLY, "too many points in tmap");
#ifdef DXX_CONSTANT_TRUE
	if (DXX_CONSTANT_TRUE(nv > N))
		DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_tmap_overread, "reading beyond array");
#endif
	if (nv > N)
		return;
	_g3_draw_tmap(canvas, nv, &pointlist[0], &uvl_list[0], &light_rgb[0], bm);
}

template <std::size_t N>
static inline void g3_draw_tmap(grs_canvas &canvas, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	g3_draw_tmap(canvas, N, pointlist, uvl_list, light_rgb, bm);
}

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void g3_draw_sphere(grs_canvas &, cg3s_point &pnt, fix rad, uint8_t color);

//@@//return ligting value for a point
//@@fix g3_compute_lighting_value(g3s_point *rotated_point,fix normval);

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
bool do_facing_check(const std::array<cg3s_point *, 3> &vertlist);

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
static inline void g3_check_and_draw_poly(grs_canvas &canvas, const std::array<cg3s_point *, 3> &pointlist, const uint8_t color)
{
	if (do_facing_check(pointlist))
		g3_draw_poly(canvas, pointlist.size(), pointlist, color);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(grs_canvas &canvas, unsigned nv, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	if (do_facing_check(pointlist))
		g3_draw_tmap(canvas, nv, pointlist, uvl_list, light_rgb, bm);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(grs_canvas &canvas, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	g3_check_and_draw_tmap(canvas, N, pointlist, uvl_list, light_rgb, bm);
}

//draws a line. takes two points.
#if !DXX_USE_OGL
struct temporary_points_t;
#endif

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
void g3_draw_rod_tmap(grs_canvas &, grs_bitmap &bitmap, const g3s_point &bot_point, fix bot_width, const g3s_point &top_point, fix top_width, g3s_lrgb light);

//draws a bitmap with the specified 3d width & height
//returns 1 if off screen, 0 if drew
void g3_draw_bitmap(grs_canvas &, const vms_vector &pos, fix width, fix height, grs_bitmap &bm);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
#if DXX_USE_OGL
enum class tmap_drawer_constant : uint_fast8_t
{
	polygon,
	flat,
};

#define draw_tmap tmap_drawer_constant::polygon
#define draw_tmap_flat tmap_drawer_constant::flat

class tmap_drawer_type
{
	tmap_drawer_constant type;
public:
	constexpr tmap_drawer_type(tmap_drawer_constant t) : type(t)
	{
	}
	bool operator==(tmap_drawer_constant t) const
		{
			return type == t;
		}
	bool operator!=(tmap_drawer_constant t) const
		{
			return type != t;
		}
};
#define g3_draw_line(C,P0,P1,c)	g3_draw_line(P0,P1,c)
#else
void g3_draw_line(grs_canvas &, cg3s_point &p0, cg3s_point &p1, uint8_t color, temporary_points_t &);
constexpr std::integral_constant<std::size_t, 100> MAX_POINTS_IN_POLY{};

using tmap_drawer_type = void (*)(grs_canvas &, const grs_bitmap &bm, uint_fast32_t nv, const g3s_point *const *vertlist);

//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(grs_canvas &, uint_fast32_t nverts, const std::array<fix, MAX_POINTS_IN_POLY * 2> &vert, uint8_t color);
#endif
void g3_draw_line(grs_canvas &canvas, cg3s_point &p0, cg3s_point &p1, uint8_t color);

void g3_set_special_render(tmap_drawer_type tmap_drawer);

extern tmap_drawer_type tmap_drawer_ptr;

}

#endif
