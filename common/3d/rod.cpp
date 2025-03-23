/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Rod routines
 * 
 */


#include "3d.h"
#include "globvars.h"
#include "maths.h"
#if !DXX_USE_OGL
#include "gr.h"
#endif

namespace dcx {

#if !DXX_USE_OGL
g3_projected_point::g3_projected_point(const vms_vector &relative_position, g3_rotated_point::from_relative_position) :
	p3_codes{build_g3_clipping_code_from_viewer_relative_position(relative_position)},
	p3_flags{}
{
}

g3s_point::g3s_point(const vms_vector &relative_position, from_relative_position rel) :
	g3_rotated_point{relative_position, rel},
	g3_projected_point{relative_position, rel}
{
}
#endif

namespace {

struct rod_corners_result
{
#if DXX_USE_OGL
	using g3_rod_corner_point = g3_rotated_point;
#else
	using g3_rod_corner_point = g3s_point;
#endif
	std::array<g3_rod_corner_point, 4> points;
	clipping_code cc;
	static clipping_code build_clipping_code(const std::array<g3_rod_corner_point, 4> &points);
	rod_corners_result(const g3_rotated_point &top_point, const vms_vector &scaled_rod_top_width, const g3_rotated_point &bot_point, const vms_vector &scaled_rod_bot_width) :
		points{{
			g3_rod_corner_point{vm_vec_add(top_point.p3_vec, scaled_rod_top_width), g3_rotated_point::from_relative_position{}},
			g3_rod_corner_point{vm_vec_sub(top_point.p3_vec, scaled_rod_top_width), g3_rotated_point::from_relative_position{}},
			g3_rod_corner_point{vm_vec_sub(bot_point.p3_vec, scaled_rod_bot_width), g3_rotated_point::from_relative_position{}},
			g3_rod_corner_point{vm_vec_add(bot_point.p3_vec, scaled_rod_bot_width), g3_rotated_point::from_relative_position{}}
		}},
		cc{build_clipping_code(points)}
	{
	}
};

clipping_code rod_corners_result::build_clipping_code(const std::array<g3_rod_corner_point, 4> &points)
{
	clipping_code codes_and{0xff};
	for (auto &i : points)
		codes_and &= (
#if DXX_USE_OGL
			/* In the OpenGL build, the per-point code is not computed above
			 * because `g3_rotated_point` does not need it and has nowhere to
			 * store it.  Compute it now, and use it immediately.
			 */
			build_g3_clipping_code_from_viewer_relative_position(i.p3_vec)
#else
			/* In the SDL-only build, the per-point clipping_code is computed
			 * as part of initializing the individual `g3s_point` instances.
			 * Read it here and use it to update the composite clipping code.
			 */
			i.p3_codes
#endif
		);
	return codes_and;
}

//compute the corners of a rod.  fills in vertbuf.
static rod_corners_result calc_rod_corners(const g3_rotated_point &bot_point, const fix bot_width, const g3_rotated_point &top_point, const fix top_width)
{
	//compute vector from one point to other, do cross product with vector
	//from eye to get perpendiclar

	auto delta_vec{vm_vec_sub(bot_point.p3_vec, top_point.p3_vec)};

	//unscale for aspect

	delta_vec.x = fixdiv(delta_vec.x,Matrix_scale.x);
	delta_vec.y = fixdiv(delta_vec.y,Matrix_scale.y);

	//calc perp vector

	//do lots of normalizing to prevent overflowing.  When this code works,
	//it should be optimized

	vm_vec_normalize(delta_vec);

	const auto top = vm_vec_normalized(top_point.p3_vec);

	auto rod_norm{vm_vec_cross(delta_vec, top)};

	vm_vec_normalize(rod_norm);

	//scale for aspect

	rod_norm.x = fixmul(rod_norm.x,Matrix_scale.x);
	rod_norm.y = fixmul(rod_norm.y,Matrix_scale.y);
	rod_norm.z = {};
	return rod_corners_result{top_point, vm_vec_copy_scale(rod_norm, top_width), bot_point, vm_vec_copy_scale(rod_norm, bot_width)};
}

}

//draw a bitmap object that is always facing you
void g3_draw_rod_tmap(grs_canvas &canvas, grs_bitmap &bitmap, const g3_rotated_point &bot_point, fix bot_width, const g3_rotated_point &top_point, fix top_width, g3s_lrgb light, const tmap_drawer_type tmap_drawer_ptr)
{
	rod_corners_result rod{calc_rod_corners(bot_point, bot_width, top_point, top_width)};
	if (rod.cc != clipping_code::None)
		return;

	const fix average_light = static_cast<unsigned>(light.r+light.g+light.b)/3;
	const std::array<g3s_uvl, 4> uvl_list{{
		{ 0x0200, 0x0200, average_light },
		{ 0xfe00, 0x0200, average_light },
		{ 0xfe00, 0xfe00, average_light },
		{ 0x0200, 0xfe00, average_light }
	}};
	const std::array<g3s_lrgb, 4> lrgb_list{{
		light,
		light,
		light,
		light,
	}};

	g3_draw_tmap(canvas, std::array<g3_draw_tmap_point *, 4>{{
		&rod.points[0],
		&rod.points[1],
		&rod.points[2],
		&rod.points[3],
	}}, uvl_list, lrgb_list, bitmap, tmap_drawer_ptr);
}

#if !DXX_USE_OGL
//draws a bitmap with the specified 3d width & height 
void g3_draw_bitmap(grs_canvas &canvas, const vms_vector &pos, fix width, fix height, grs_bitmap &bm)
{
	g3s_point pnt;
	fix w,h;
	if ((g3_rotate_point(pnt, pos) & clipping_code::behind) != clipping_code::None)
		return;
	g3_project_point(pnt);
	if (pnt.p3_flags & projection_flag::overflow)
		return;
#ifndef __powerc
	const auto pz = pnt.p3_vec.z;
	if (const auto ox = checkmuldiv(width, Canv_w2, pz))
		w = fixmul(*ox, Matrix_scale.x);
	else
		return;

	if (const auto oy = checkmuldiv(height, Canv_h2, pz))
		h = fixmul(*oy, Matrix_scale.y);
	else
		return;
#else
	if (pnt.p3_vec.z == 0)
		return;
	double fz = f2fl(pnt.p3_vec.z);
	w = fixmul(fl2f(((f2fl(width)*fCanv_w2) / fz)), Matrix_scale.x);
	h = fixmul(fl2f(((f2fl(height)*fCanv_h2) / fz)), Matrix_scale.y);
#endif
	const fix blob0y = pnt.p3_sy - h, blob1x = pnt.p3_sx + w;
	const std::array<grs_point, 3> blob_vertices{{
		{pnt.p3_sx - w, blob0y},
		{blob1x, blob0y},
		{blob1x, pnt.p3_sy + h},
	}};
	scale_bitmap(bm, blob_vertices, 0, canvas.cv_bitmap);
}
#endif

}
