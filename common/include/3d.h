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
#include <span>

#if DXX_USE_OGL
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
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

//clipping codes flags

enum class clipping_code : uint8_t
{
	None = 0,
	off_left = 1,
	off_right = 2,
	off_bot = 4,
	off_top = 8,
	behind = 0x80,
};

static constexpr clipping_code operator&(const clipping_code a, const clipping_code b)
{
	return static_cast<clipping_code>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

static constexpr clipping_code &operator&=(clipping_code &a, const clipping_code b)
{
	return a = (a & b);
}

static constexpr clipping_code operator|(const clipping_code a, const clipping_code b)
{
	return static_cast<clipping_code>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

static constexpr clipping_code &operator|=(clipping_code &a, const clipping_code b)
{
	return a = (a | b);
}

static constexpr clipping_code operator~(const clipping_code a)
{
	return static_cast<clipping_code>(~static_cast<uint8_t>(a));
}

//Stucture to store clipping codes in a word
struct g3s_codes {
	//or is low byte, and is high byte
	clipping_code uor = clipping_code{0};
	clipping_code uand = clipping_code{0xff};
};

enum class projection_flag : uint8_t
{
	//flags for point structure
	projected = 1,		//has been projected, so sx,sy valid
	overflow = 2,		//can't project
#if !DXX_USE_OGL
	temp_point = 4,	//created during clip
	uvs = 8,			//has uv values set
	ls = 16,			//has lighting values set
#endif
};

static constexpr uint8_t operator&(const projection_flag a, const projection_flag b)
{
	return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

static constexpr projection_flag &operator&=(projection_flag &a, const projection_flag b)
{
	return a = static_cast<projection_flag>(a & b);
}

static constexpr projection_flag operator|(const projection_flag a, const projection_flag b)
{
	return static_cast<projection_flag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

static constexpr projection_flag &operator|=(projection_flag &a, const projection_flag b)
{
	return a = (a | b);
}

static constexpr projection_flag operator~(const projection_flag a)
{
	return static_cast<projection_flag>(~static_cast<uint8_t>(a));
}

namespace dcx {

struct g3_instance_context
{
	const vms_matrix matrix;
	const vms_vector position;
};

struct g3_rotated_point
{
	vms_vector p3_vec;
protected:
	g3_rotated_point() = default;
};

/*
 * This stores a point that has been rotated about the Viewer's orientation
 * matrix.  p3_flags tracks whether the point has been projected into screen
 * coordinates (`projected`) and, if so, whether the resulting coordinates are
 * outside the screen's viewable area (`overflow`).
 */
struct g3s_point : g3_rotated_point
{
#if !DXX_USE_OGL
	fix p3_u, p3_v, p3_l; //u,v,l coords
#endif
	fix p3_sx, p3_sy;    //screen x&y
	clipping_code p3_codes;     //clipping codes
	projection_flag p3_flags;     //projected?
	g3s_point() = default;
};

/* Store the frame count at which this point was last updated.  If the stored
 * count matches the current frame generation, then the other members are valid
 * for this frame and do not need to be recomputed.  If the stored count does
 * not match the current frame generation, then the data is stale, may be
 * invalid, and should be recomputed.
 *
 * In the unlikely case that the viewer has not moved or rotated at all since
 * the frame in which this was computed, stale data might match what will be
 * recomputed.  However, this case is sufficiently uncommon that it is not
 * worth detecting it solely to avoid recomputing the rotation.
 */
struct g3s_reusable_point : g3s_point
{
	uint16_t p3_last_generation;
	g3s_reusable_point() = default;
};

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
[[nodiscard]]
g3_instance_context g3_start_instance_matrix();
[[nodiscard]]
g3_instance_context g3_start_instance_matrix(const vms_vector &pos, const vms_matrix &orient);

//instance at specified point with specified orientation
[[nodiscard]]
g3_instance_context g3_start_instance_angles(const vms_vector &pos, const vms_angvec &angles);

//pops the old context
void g3_done_instance(const g3_instance_context &);

//Misc utility functions:

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(const vms_vector &v,const vms_vector &norm);

}

//Point definition and rotation functions:

namespace dcx {

//rotates a point. returns codes.  does not check if already rotated
clipping_code g3_rotate_point(g3s_point &dest,const vms_vector &src);

[[nodiscard]]
static inline g3s_point g3_rotate_point(const vms_vector &src)
{
	g3s_point dest;
	return g3_rotate_point(dest, src), dest;
}

//projects a point
void g3_project_point(g3s_point &point);

//calculate the depth of a point - returns the z coord of the rotated point
[[nodiscard]]
fix g3_calc_point_depth(const vms_vector &pnt);

#if DXX_USE_EDITOR
//from a 2d point, compute the vector through that point
[[nodiscard]]
vms_vector g3_point_2_vec(short sx, short sy);
#endif

//code a point.  fills in the p3_codes field of the point, and returns the codes
clipping_code g3_code_point(g3s_point &point);

//delta rotation functions
[[nodiscard]]
vms_vector g3_rotate_delta_vec(const vms_vector &src);

void g3_add_delta_vec(g3s_point &dest,const g3s_point &src,const vms_vector &deltav);

//Drawing functions:

//draw a flat-shaded face.
void _g3_draw_poly(grs_canvas &, std::span<cg3s_point *const> pointlist, uint8_t color);
template <std::size_t N>
static inline void g3_draw_poly(grs_canvas &canvas, const uint_fast32_t nv, const std::array<cg3s_point *, N> &pointlist, const uint8_t color)
{
	_g3_draw_poly(canvas, std::span(pointlist).first(nv), color);
}

constexpr std::integral_constant<std::size_t, 64> MAX_POINTS_PER_POLY{};

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void g3_draw_sphere(grs_canvas &, cg3s_point &pnt, fix rad, uint8_t color);

#if !DXX_USE_OGL
static inline void g3_draw_sphere(grs_canvas &canvas, g3s_point &&pnt, fix rad, uint8_t color)
{
	g3_draw_sphere(canvas, pnt, rad, color);
}
#endif

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

//draws a line. takes two points.
#if !DXX_USE_OGL
struct temporary_points_t;
#endif

//draws a bitmap with the specified 3d width & height
//returns 1 if off screen, 0 if drew
void g3_draw_bitmap(grs_canvas &, const vms_vector &pos, fix width, fix height, grs_bitmap &bm);

#if DXX_USE_OGL
struct g3_draw_line_colors
{
	const std::array<GLfloat, 8> color_array;
	g3_draw_line_colors(color_palette_index color);
};
#endif

class g3_draw_line_context
#if DXX_USE_OGL
	: public g3_draw_line_colors
#endif
{
public:
	grs_canvas &canvas;
	const color_palette_index color;
	g3_draw_line_context(grs_canvas &canvas, color_palette_index color) :
#if DXX_USE_OGL
		g3_draw_line_colors{color},
#endif
		canvas{canvas}, color{color}
	{
	}
	g3_draw_line_context(const g3_draw_line_context &) = delete;
	g3_draw_line_context &operator=(const g3_draw_line_context &) = delete;
};

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
#if DXX_USE_OGL
enum class tmap_drawer_type : bool
{
	polygon,
	flat,
	draw_tmap = polygon,
	draw_tmap_flat = flat,
};

using tmap_drawer_type::draw_tmap_flat;
using tmap_drawer_type::draw_tmap;
#else
void g3_draw_line(const g3_draw_line_context &, cg3s_point &p0, cg3s_point &p1, temporary_points_t &);
constexpr std::integral_constant<std::size_t, 100> MAX_POINTS_IN_POLY{};

using tmap_drawer_type = void (*)(grs_canvas &, const grs_bitmap &bm, std::span<const g3s_point *const> vertlist);

//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(grs_canvas &, uint_fast32_t nverts, const std::array<fix, MAX_POINTS_IN_POLY * 2> &vert, uint8_t color);
#endif

//draw a bitmap object that is always facing you
void g3_draw_rod_tmap(grs_canvas &, grs_bitmap &bitmap, const g3s_point &bot_point, fix bot_width, const g3s_point &top_point, fix top_width, g3s_lrgb light, tmap_drawer_type tmap_drawer_ptr);

//draw a texture-mapped face.
void _g3_draw_tmap(grs_canvas &canvas, std::span<cg3s_point *const> pointlist, const g3s_uvl *uvl_list, const g3s_lrgb *light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr);

template <std::size_t N>
static inline void g3_draw_tmap(grs_canvas &canvas, unsigned nv, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	static_assert(N <= MAX_POINTS_PER_POLY, "too many points in tmap");
#ifdef DXX_CONSTANT_TRUE
	if (DXX_CONSTANT_TRUE(nv > N))
		DXX_ALWAYS_ERROR_FUNCTION("reading beyond array");
#endif
	if (nv > N)
		return;
	_g3_draw_tmap(canvas, std::span(pointlist).first(nv), uvl_list.data(), light_rgb.data(), bm, tmap_drawer_ptr);
}

template <std::size_t N>
requires(N <= MAX_POINTS_PER_POLY)
static inline void g3_draw_tmap(grs_canvas &canvas, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	_g3_draw_tmap(canvas, pointlist, uvl_list.data(), light_rgb.data(), bm, tmap_drawer_ptr);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(grs_canvas &canvas, unsigned nv, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	if (do_facing_check(pointlist))
		g3_draw_tmap(canvas, nv, pointlist, uvl_list, light_rgb, bm, tmap_drawer_ptr);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(grs_canvas &canvas, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	g3_check_and_draw_tmap(canvas, N, pointlist, uvl_list, light_rgb, bm, tmap_drawer_ptr);
}

void g3_draw_line(const g3_draw_line_context &context, cg3s_point &p0, cg3s_point &p1);

}
