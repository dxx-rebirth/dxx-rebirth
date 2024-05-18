/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * Header for clipper.cpp
 */

#pragma once
#include "dxxsconf.h"
#include <cstdint>

#if !DXX_USE_OGL
#include "dsx-ns.h"
#include "3d.h"
#include "globvars.h"
#include <array>

namespace dcx {

struct polygon_clip_points : std::array<g3s_point *, MAX_POINTS_IN_POLY> {};
struct temporary_points_t
{
	using point_storage = std::array<g3s_point, MAX_POINTS_IN_POLY>;
	/* Initialize to 0 since no `free_points` members are in use.
	 * `free_points_used` will increase and decrease as pointers are placed in
	 * `free_points` and consumed from `free_points`.
	 */
	unsigned free_points_used{};
	/* Initialize to 0 since no `temp_points` members are in use.
	 * `temporary_points_used` will increase when a new element of
	 * `temp_points` is activated.  `temporary_points_used` never decreases,
	 * since deactivated elements in `temp_points` are pushed onto
	 * `free_points`.  The implementation could have been written to decrease
	 * `temporary_points_used` for the special case that the freed point is the
	 * last one in use, but this adds complexity for no benefit.
	 */
	unsigned temporary_points_used{};
	/* Elements in [0, `free_points_used`) are pointers to elements of
	 * `temp_points` that were once used, but are now free and available for
	 * reuse.
	 *
	 * Elements in [`free_points_used`, free_points.end()) are undefined.
	 *
	 * No two elements in `free_points` will point to the same element in
	 * `temp_points`.  Every valid pointer in `free_points` must point to an
	 * element in `temp_points` from the same containing `temporary_points_t`.
	 *
	 * When a temporary point is needed, the last element of `free_points` will
	 * be consumed to obtain a pointer to an available temporary point.  If
	 * `free_points` is empty, `temporary_points_used` will be read to pick the
	 * next unused `temp_points` member, and that member will be used as an
	 * available temporary point.
	 *
	 * When a temporary point is deactivated, its address will be written to
	 * `free_points`.  Only temporary points allocated from `temp_points` are
	 * tracked, so `free_points` cannot overflow since the worst case is that
	 * every element in `free_points` points to an element in `temp_points`.
	 */
	std::array<point_storage::pointer, MAX_POINTS_IN_POLY> free_points
#ifndef NDEBUG
		/* For debug builds, value-initialize the array with `nullptr` to make
		 * bugs obvious.  For release builds, leave it uninitialized for
		 * performance.
		 */
	{}
#endif
		;
	point_storage temp_points;
	temporary_points_t();
	void free_temp_point(g3s_point &cp);
};

const polygon_clip_points &clip_polygon(polygon_clip_points &src, polygon_clip_points &dest, std::size_t *nv, g3s_codes *cc, temporary_points_t &);
void clip_line(g3s_point *&p0, g3s_point *&p1, clipping_code codes_or, temporary_points_t &);

}

#endif
