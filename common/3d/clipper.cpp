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
temporary_points_t::temporary_points_t() :
	free_point_num(0)
{
	auto p = &temp_points.front();
	range_for (auto &f, free_points)
		f = p++;
}

namespace {

static g3s_point &get_temp_point(temporary_points_t &t)
{
	if (t.free_point_num >= t.free_points.size())
		throw std::out_of_range("not enough free points");
	auto &p = *t.free_points[t.free_point_num++];
	p.p3_flags = PF_TEMP_POINT;
	return p;
}

}

void temporary_points_t::free_temp_point(g3s_point *p)
{
	if (!(p->p3_flags & PF_TEMP_POINT))
		throw std::invalid_argument("freeing non-temporary point");
	if (--free_point_num >= free_points.size())
		throw std::out_of_range("too many free points");
	free_points[free_point_num] = p;
	p->p3_flags &= ~PF_TEMP_POINT;
}

namespace {

//clips an edge against one plane. 
static g3s_point &clip_edge(int plane_flag,g3s_point *on_pnt,g3s_point *off_pnt, temporary_points_t &tp)
{
	fix psx_ratio;
	fix a,b,kn,kd;

	//compute clipping value k = (xs-zs) / (xs-xe-zs+ze)
	//use x or y as appropriate, and negate x/y value as appropriate

	if (plane_flag & (CC_OFF_RIGHT | CC_OFF_LEFT)) {
		a = on_pnt->p3_x;
		b = off_pnt->p3_x;
	}
	else {
		a = on_pnt->p3_y;
		b = off_pnt->p3_y;
	}

	if (plane_flag & (CC_OFF_LEFT | CC_OFF_BOT)) {
		a = -a;
		b = -b;
	}

	kn = a - on_pnt->p3_z;						//xs-zs
	kd = kn - b + off_pnt->p3_z;				//xs-zs-xe+ze

	auto &tmp = get_temp_point(tp);

	psx_ratio = fixdiv( kn, kd );
	tmp.p3_x = on_pnt->p3_x + fixmul( (off_pnt->p3_x-on_pnt->p3_x), psx_ratio);
	tmp.p3_y = on_pnt->p3_y + fixmul( (off_pnt->p3_y-on_pnt->p3_y), psx_ratio);

	if (plane_flag & (CC_OFF_TOP|CC_OFF_BOT))
		tmp.p3_z = tmp.p3_y;
	else
		tmp.p3_z = tmp.p3_x;

	if (plane_flag & (CC_OFF_LEFT|CC_OFF_BOT))
		tmp.p3_z = -tmp.p3_z;

	if (on_pnt->p3_flags & PF_UVS) {
// PSX_HACK!!!!
//		tmp.p3_u = on_pnt->p3_u + fixmuldiv(off_pnt->p3_u-on_pnt->p3_u,kn,kd);
//		tmp.p3_v = on_pnt->p3_v + fixmuldiv(off_pnt->p3_v-on_pnt->p3_v,kn,kd);
		tmp.p3_u = on_pnt->p3_u + fixmul((off_pnt->p3_u-on_pnt->p3_u), psx_ratio);
		tmp.p3_v = on_pnt->p3_v + fixmul((off_pnt->p3_v-on_pnt->p3_v), psx_ratio);

		tmp.p3_flags |= PF_UVS;
	}

	if (on_pnt->p3_flags & PF_LS) {
// PSX_HACK
//		tmp.p3_r = on_pnt->p3_r + fixmuldiv(off_pnt->p3_r-on_pnt->p3_r,kn,kd);
//		tmp.p3_g = on_pnt->p3_g + fixmuldiv(off_pnt->p3_g-on_pnt->p3_g,kn,kd);
//		tmp.p3_b = on_pnt->p3_b + fixmuldiv(off_pnt->p3_b-on_pnt->p3_b,kn,kd);
		tmp.p3_l = on_pnt->p3_l + fixmul((off_pnt->p3_l-on_pnt->p3_l), psx_ratio);
		tmp.p3_flags |= PF_LS;
	}
	g3_code_point(tmp);
	return tmp;
}

}

//clips a line to the viewing pyramid.
void clip_line(g3s_point *&p0,g3s_point *&p1,const uint_fast8_t codes_or, temporary_points_t &tp)
{
	//might have these left over
	p0->p3_flags &= ~(PF_UVS|PF_LS);
	p1->p3_flags &= ~(PF_UVS|PF_LS);

	for (int plane_flag=1;plane_flag<16;plane_flag<<=1)
		if (codes_or & plane_flag) {

			if (p0->p3_codes & plane_flag)
				std::swap(p0, p1);
			const auto old_p1 = std::exchange(p1, &clip_edge(plane_flag,p0,p1,tp));
			if (old_p1->p3_flags & PF_TEMP_POINT)
				tp.free_temp_point(old_p1);
		}
}

namespace {

static int clip_plane(int plane_flag,polygon_clip_points &src,polygon_clip_points &dest,int *nv,g3s_codes *cc, temporary_points_t &tp)
{
	//copy first two verts to end
	src[*nv] = src[0];
	src[*nv+1] = src[1];

	cc->uand = 0xff; cc->uor = 0;

	uint_fast32_t j = 0;
	for (int i=1;i<=*nv;i++) {

		if (src[i]->p3_codes & plane_flag) {				//cur point off?

			if (! (src[i-1]->p3_codes & plane_flag)) {	//prev not off?

				dest[j] = &clip_edge(plane_flag,src[i-1],src[i],tp);
				cc->uor  |= dest[j]->p3_codes;
				cc->uand &= dest[j]->p3_codes;
				++j;
			}

			if (! (src[i+1]->p3_codes & plane_flag)) {

				dest[j] = &clip_edge(plane_flag,src[i+1],src[i],tp);
				cc->uor  |= dest[j]->p3_codes;
				cc->uand &= dest[j]->p3_codes;
				++j;
			}

			//see if must free discarded point

			if (src[i]->p3_flags & PF_TEMP_POINT)
				tp.free_temp_point(src[i]);
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

const polygon_clip_points &clip_polygon(polygon_clip_points &rsrc,polygon_clip_points &rdest,int *nv,g3s_codes *cc, temporary_points_t &tp)
{
	polygon_clip_points *src = &rsrc, *dest = &rdest;
	for (int plane_flag=1;plane_flag<16;plane_flag<<=1)

		if (cc->uor & plane_flag) {

			*nv = clip_plane(plane_flag,*src,*dest,nv,cc,tp);

			if (cc->uand)		//clipped away
				return *dest;

			std::swap(src, dest);
		}

	return *src;		//we swapped after we copied
}
#endif

}
