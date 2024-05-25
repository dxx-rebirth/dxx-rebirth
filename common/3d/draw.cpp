/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Drawing routines
 * 
 */


#include "dxxerror.h"

#include "3d.h"
#include "globvars.h"
#include "texmap.h"
#if !DXX_USE_OGL
#include "clipper.h"
#include "gr.h"
#endif
#include "d_enumerate.h"
#include "d_zip.h"
#include "partial_range.h"

namespace dcx {

#if DXX_USE_OGL
namespace {

const std::array<GLfloat, 8> build_color_array_from_color_palette_index(const color_palette_index color)
{
	auto &&rgb{PAL2T(color)};
	const GLfloat color_r{rgb.r / 63.0f};
	const GLfloat color_g{rgb.g / 63.0f};
	const GLfloat color_b{rgb.b / 63.0f};
	return {{
		color_r, color_g, color_b, 1.0,
		color_r, color_g, color_b, 1.0,
	}};
}

}

g3_draw_line_colors::g3_draw_line_colors(const color_palette_index color) :
	color_array{build_color_array_from_color_palette_index(color)}
{
}
#else
namespace {

//deal with a clipped line
static void must_clip_line(const g3_draw_line_context &context, g3s_point *p0, g3s_point *p1, const clipping_code codes_or, temporary_points_t &tp)
{
	if ((p0->p3_flags&projection_flag::temp_point) || (p1->p3_flags&projection_flag::temp_point))
		;		//line has already been clipped, so give up
	else {
		clip_line(p0,p1,codes_or,tp);
		g3_draw_line(context, *p0, *p1, tp);
	}

	//free temp points

	if (p0->p3_flags & projection_flag::temp_point)
		tp.free_temp_point(*p0);

	if (p1->p3_flags & projection_flag::temp_point)
		tp.free_temp_point(*p1);
}

}

//draws a line. takes two points.  returns true if drew
void g3_draw_line(const g3_draw_line_context &context, g3s_point &p0, g3s_point &p1)
{
	temporary_points_t tp;
	g3_draw_line(context, p0, p1, tp);
}

void g3_draw_line(const g3_draw_line_context &context, g3s_point &p0, g3s_point &p1, temporary_points_t &tp)
{
	if ((p0.p3_codes & p1.p3_codes) != clipping_code::None)
		return;

	const clipping_code codes_or = p0.p3_codes | p1.p3_codes;
	if (
		(codes_or & clipping_code::behind) != clipping_code::None ||
		(static_cast<void>((p0.p3_flags & projection_flag::projected) || (g3_project_point(p0), 0)), p0.p3_flags & projection_flag::overflow) ||
		(static_cast<void>((p1.p3_flags & projection_flag::projected) || (g3_project_point(p1), 0)), p1.p3_flags & projection_flag::overflow)
	)
		return must_clip_line(context, &p0,&p1, codes_or, tp);
	gr_line(context.canvas, p0.p3_sx, p0.p3_sy, p1.p3_sx, p1.p3_sy, context.color);
}
#endif

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(const vms_vector &v,const vms_vector &norm)
{
	return (vm_vec_dot(vm_vec_sub(View_position,v),norm) > 0);
}

bool do_facing_check(const std::array<cg3s_point *, 3> &vertlist)
{
	//normal not specified, so must compute
		//get three points (rotated) and compute normal
	const auto tempv{vm_vec_perp(vertlist[0]->p3_vec, vertlist[1]->p3_vec, vertlist[2]->p3_vec)};
		return (vm_vec_dot(tempv,vertlist[1]->p3_vec) < 0);
}

#if !DXX_USE_OGL
namespace {

static void must_clip_tmap_face(grs_canvas &, std::size_t nv, g3s_codes cc, grs_bitmap &bm, polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1, tmap_drawer_type tmap_drawer_ptr);

//deal with face that must be clipped
static void must_clip_flat_face(grs_canvas &canvas, std::size_t nv, g3s_codes cc, polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1, const uint8_t color)
{
	temporary_points_t tp;
	auto &bufptr{clip_polygon(Vbuf0, Vbuf1, &nv, &cc, tp)};

	if (nv > 0 && (cc.uor & clipping_code::behind) == clipping_code::None && cc.uand == clipping_code::None)
	{
		std::array<fix, MAX_POINTS_IN_POLY*2> Vertex_list;
		for (int i=0;i<nv;i++) {
			g3s_point *p = bufptr[i];
	
			if (!(p->p3_flags&projection_flag::projected))
				g3_project_point(*p);
	
			if (p->p3_flags&projection_flag::overflow) {
				goto free_points;
			}

			Vertex_list[i*2]   = p->p3_sx;
			Vertex_list[i*2+1] = p->p3_sy;
		}
		gr_upoly_tmap(canvas, nv, Vertex_list, color);
	}
	//free temp points
free_points:
	;
}

}

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
void _g3_draw_poly(grs_canvas &canvas, const std::span<cg3s_point *const> pointlist, const uint8_t color)
{
	g3s_codes cc;

	polygon_clip_points Vbuf0, Vbuf1;
	auto &bufptr{Vbuf0};

	for (const auto &&[pl, bp] : zip(pointlist, bufptr))
	{
		bp = pl;
		cc.uand &= bp->p3_codes;
		cc.uor  |= bp->p3_codes;
	}

	if (cc.uand != clipping_code::None)
		return;	//all points off screen

	if (cc.uor != clipping_code::None)
	{
		must_clip_flat_face(canvas, pointlist.size(), cc, Vbuf0, Vbuf1, color);
		return;
	}

	//now make list of 2d coords (& check for overflow)
	std::array<fix, MAX_POINTS_IN_POLY*2> Vertex_list;
	for (const auto &&[i, pl, p] : enumerate(zip(pointlist, bufptr)))
	{
		if (!(p->p3_flags&projection_flag::projected))
			g3_project_point(*p);

		if (p->p3_flags&projection_flag::overflow)
		{
			must_clip_flat_face(canvas, pointlist.size(), cc, Vbuf0, Vbuf1, color);
			return;
		}

		Vertex_list[i*2]   = p->p3_sx;
		Vertex_list[i*2+1] = p->p3_sy;
	}
	gr_upoly_tmap(canvas, pointlist.size(), Vertex_list, color);
	//say it drew
}

//draw a texture-mapped face.
void _g3_draw_tmap(grs_canvas &canvas, const std::span<cg3s_point *const> pointlist, const g3s_uvl *const uvl_list, const g3s_lrgb *const light_rgb, grs_bitmap &bm, const tmap_drawer_type tmap_drawer_ptr)
{
	g3s_codes cc;

	polygon_clip_points Vbuf0, Vbuf1;
	auto &bufptr = Vbuf0;

	for (auto &&[i, pl, bp] : enumerate(zip(pointlist, bufptr)))
	{
		const auto p = bp = pl;

		cc.uand &= p->p3_codes;
		cc.uor  |= p->p3_codes;

		if constexpr (!DXX_USE_OGL)
		{
		p->p3_u = uvl_list[i].u;
		p->p3_v = uvl_list[i].v;
		p->p3_l = (light_rgb[i].r+light_rgb[i].g+light_rgb[i].b)/3;
		}

		p->p3_flags |= projection_flag::uvs | projection_flag::ls;

	}

	if (cc.uand != clipping_code::None)
		return;	//all points off screen

	if (cc.uor != clipping_code::None)
	{
		must_clip_tmap_face(canvas, pointlist.size(), cc, bm, Vbuf0, Vbuf1, tmap_drawer_ptr);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (auto &&p : std::span(bufptr).first(pointlist.size()))
	{
		if (!(p->p3_flags&projection_flag::projected))
			g3_project_point(*p);

		if (p->p3_flags&projection_flag::overflow) {
			Int3();		//should not overflow after clip
			return;
		}
	}
	(*tmap_drawer_ptr)(canvas, bm, std::span(bufptr).first(pointlist.size()));
}

namespace {

static void must_clip_tmap_face(grs_canvas &canvas, std::size_t nv, g3s_codes cc, grs_bitmap &bm, polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1, const tmap_drawer_type tmap_drawer_ptr)
{
	temporary_points_t tp;
	auto &bufptr{clip_polygon(Vbuf0, Vbuf1, &nv, &cc, tp)};
	if (nv && (cc.uor & clipping_code::behind) == clipping_code::None && cc.uand == clipping_code::None)
	{
		for (int i=0;i<nv;i++) {
			auto *const p{bufptr[i]};

			if (!(p->p3_flags&projection_flag::projected))
				g3_project_point(*p);
			if (p->p3_flags&projection_flag::overflow) {
				Int3();		//should not overflow after clip
				goto free_points;
			}
		}

		(*tmap_drawer_ptr)(canvas, bm, std::span(bufptr).first(nv));
	}

free_points:
	;

//	Assert(free_point_num==0);
}

}

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void g3_draw_sphere(grs_canvas &canvas, cg3s_point &pnt, const fix rad, const uint8_t color)
{
	if ((pnt.p3_codes & clipping_code::behind) == clipping_code::None)
	{
		if (! (pnt.p3_flags & projection_flag::projected))
			g3_project_point(pnt);

		if (! (pnt.p3_flags & projection_flag::overflow)) {
			const auto r2{fixmul(rad, Matrix_scale.x)};
#ifndef __powerc
			const auto ot{checkmuldiv(r2, Canv_w2, pnt.p3_vec.z)};
			if (!ot)
				return;
			const auto t{*ot};
#else
			if (pnt.p3_vec.z == 0)
				return;
			const fix t = fl2f(((f2fl(r2) * fCanv_w2) / f2fl(pnt.p3_vec.z)));
#endif
			gr_disk(canvas, pnt.p3_sx, pnt.p3_sy, t, color);
		}
	}
}
#endif

}
