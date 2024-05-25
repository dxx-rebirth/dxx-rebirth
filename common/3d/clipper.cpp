/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "3d.h"
#include "globvars.h"
#include "clipper.h"
#include "dxxerror.h"

#include "compiler-range_for.h"
#include <stdexcept>

namespace dcx {

#if !DXX_USE_OGL
temporary_points_t::temporary_points_t() = default;

namespace {

static g3s_point &get_temp_point(temporary_points_t &t)
{
	auto &p{t.free_points_used
		? *t.free_points.at(--t.free_points_used)
		: t.temp_points.at(t.temporary_points_used++)
	};
	p.p3_flags = projection_flag::temp_point;
	return p;
}

}

void temporary_points_t::free_temp_point(g3s_point &p)
{
	if (!(p.p3_flags & projection_flag::temp_point))
		throw std::invalid_argument("freeing non-temporary point");
	p.p3_flags &= ~projection_flag::temp_point;
	free_points.at(free_points_used++) = &p;
}

namespace {

//clips an edge against one plane. 
static g3s_point &clip_edge(const clipping_code plane_flag, g3s_point *on_pnt, g3s_point *off_pnt, temporary_points_t &tp)
{
	fix psx_ratio;
	fix a,b,kn,kd;

	//compute clipping value k = (xs-zs) / (xs-xe-zs+ze)
	//use x or y as appropriate, and negate x/y value as appropriate

	if ((plane_flag & (clipping_code::off_right | clipping_code::off_left)) != clipping_code::None)
	{
		a = on_pnt->p3_vec.x;
		b = off_pnt->p3_vec.x;
	}
	else {
		a = on_pnt->p3_vec.y;
		b = off_pnt->p3_vec.y;
	}

	if ((plane_flag & (clipping_code::off_left | clipping_code::off_bot)) != clipping_code::None)
	{
		a = -a;
		b = -b;
	}

	kn = a - on_pnt->p3_vec.z;						//xs-zs
	kd = kn - b + off_pnt->p3_vec.z;				//xs-zs-xe+ze

	auto &tmp = get_temp_point(tp);

	psx_ratio = fixdiv( kn, kd );
	tmp.p3_vec.x = on_pnt->p3_vec.x + fixmul( (off_pnt->p3_vec.x-on_pnt->p3_vec.x), psx_ratio);
	tmp.p3_vec.y = on_pnt->p3_vec.y + fixmul( (off_pnt->p3_vec.y-on_pnt->p3_vec.y), psx_ratio);

	if ((plane_flag & (clipping_code::off_top | clipping_code::off_bot)) != clipping_code::None)
		tmp.p3_vec.z = tmp.p3_vec.y;
	else
		tmp.p3_vec.z = tmp.p3_vec.x;

	if ((plane_flag & (clipping_code::off_left | clipping_code::off_bot)) != clipping_code::None)
		tmp.p3_vec.z = -tmp.p3_vec.z;

	if (on_pnt->p3_flags & projection_flag::uvs) {
// PSX_HACK!!!!
//		tmp.p3_u = on_pnt->p3_u + fixmuldiv(off_pnt->p3_u-on_pnt->p3_u,kn,kd);
//		tmp.p3_v = on_pnt->p3_v + fixmuldiv(off_pnt->p3_v-on_pnt->p3_v,kn,kd);
		tmp.p3_u = on_pnt->p3_u + fixmul((off_pnt->p3_u-on_pnt->p3_u), psx_ratio);
		tmp.p3_v = on_pnt->p3_v + fixmul((off_pnt->p3_v-on_pnt->p3_v), psx_ratio);

		tmp.p3_flags |= projection_flag::uvs;
	}

	if (on_pnt->p3_flags & projection_flag::ls) {
// PSX_HACK
//		tmp.p3_r = on_pnt->p3_r + fixmuldiv(off_pnt->p3_r-on_pnt->p3_r,kn,kd);
//		tmp.p3_g = on_pnt->p3_g + fixmuldiv(off_pnt->p3_g-on_pnt->p3_g,kn,kd);
//		tmp.p3_b = on_pnt->p3_b + fixmuldiv(off_pnt->p3_b-on_pnt->p3_b,kn,kd);
		tmp.p3_l = on_pnt->p3_l + fixmul((off_pnt->p3_l-on_pnt->p3_l), psx_ratio);
		tmp.p3_flags |= projection_flag::ls;
	}
	g3_code_point(tmp);
	return tmp;
}

}

//clips a line to the viewing pyramid.
void clip_line(g3s_point *&p0, g3s_point *&p1, const clipping_code codes_or, temporary_points_t &tp)
{
	//might have these left over
	p0->p3_flags &= ~(projection_flag::uvs|projection_flag::ls);
	p1->p3_flags &= ~(projection_flag::uvs|projection_flag::ls);

	for (uint8_t plane_step = 1; plane_step < 16; plane_step <<= 1)
	{
		const clipping_code plane_flag{plane_step};
		if ((codes_or & plane_flag) != clipping_code::None)
		{
			if ((p0->p3_codes & plane_flag) != clipping_code::None)
				std::swap(p0, p1);
			auto &old_p1 = *std::exchange(p1, &clip_edge(plane_flag, p0, p1, tp));
			if (old_p1.p3_flags & projection_flag::temp_point)
				tp.free_temp_point(old_p1);
		}
	}
}

namespace {

static int clip_plane(const clipping_code plane_flag, polygon_clip_points &src, polygon_clip_points &dest, std::size_t *const nv, g3s_codes *const cc, temporary_points_t &tp)
{
	//copy first two verts to end
	src[*nv] = src[0];
	src[*nv+1] = src[1];

	*cc = {};

	uint_fast32_t j = 0;
	for (int i=1;i<=*nv;i++) {

		if ((src[i]->p3_codes & plane_flag) != clipping_code::None)
		{				//cur point off?

			if ((src[i-1]->p3_codes & plane_flag) == clipping_code::None)
			{	//prev not off?
				dest[j] = &clip_edge(plane_flag,src[i-1],src[i],tp);
				cc->uor  |= dest[j]->p3_codes;
				cc->uand &= dest[j]->p3_codes;
				++j;
			}

			if ((src[i+1]->p3_codes & plane_flag) == clipping_code::None)
			{
				dest[j] = &clip_edge(plane_flag,src[i+1],src[i],tp);
				cc->uor  |= dest[j]->p3_codes;
				cc->uand &= dest[j]->p3_codes;
				++j;
			}

			//see if must free discarded point

			if (src[i]->p3_flags & projection_flag::temp_point)
				tp.free_temp_point(*src[i]);
		}
		else {			//cur not off, copy to dest buffer

			dest[j++] = src[i];

			cc->uor  |= src[i]->p3_codes;
			cc->uand &= src[i]->p3_codes;
		}
	}
	return j;
}

}

const polygon_clip_points &clip_polygon(polygon_clip_points &rsrc, polygon_clip_points &rdest, std::size_t *nv, g3s_codes *const cc, temporary_points_t &tp)
{
	polygon_clip_points *src = &rsrc, *dest = &rdest;
	for (uint8_t plane_step = 1; plane_step < 16; plane_step <<= 1)
	{
		const clipping_code plane_flag{plane_step};
		if ((cc->uor & plane_flag) != clipping_code::None)
		{
			*nv = clip_plane(plane_flag,*src,*dest,nv,cc,tp);
			if (cc->uand != clipping_code::None)		//clipped away
				return *dest;

			std::swap(src, dest);
		}
	}

	return *src;		//we swapped after we copied
}
#endif

}
