/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Header for clipper.c
 * 
 */

#pragma once

#ifdef __cplusplus
#include "dxxsconf.h"
#include <cstdint>

struct g3s_codes;
struct g3s_point;

#if !DXX_USE_OGL
#include "dsx-ns.h"
#include "3d.h"
#include "globvars.h"
#include <array>

namespace dcx {

struct polygon_clip_points : std::array<g3s_point *, MAX_POINTS_IN_POLY> {};
struct temporary_points_t
{
	uint_fast32_t free_point_num;
	std::array<g3s_point, MAX_POINTS_IN_POLY> temp_points;
	std::array<g3s_point *, MAX_POINTS_IN_POLY> free_points;
	temporary_points_t();
	void free_temp_point(g3s_point &cp);
};

const polygon_clip_points &clip_polygon(polygon_clip_points &src,polygon_clip_points &dest,int *nv,g3s_codes *cc,temporary_points_t &);
void clip_line(g3s_point *&p0,g3s_point *&p1,uint_fast8_t codes_or,temporary_points_t &);

}
#endif

#endif
