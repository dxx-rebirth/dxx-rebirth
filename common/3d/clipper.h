/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Header for clipper.c
 * 
 */

#ifndef _CLIPPER_H
#define _CLIPPER_H

#include "pstypes.h"

#ifdef __cplusplus

struct g3s_codes;
struct g3s_point;

#include "compiler-array.h"
struct polygon_clip_points : array<g3s_point *, MAX_POINTS_IN_POLY> {};

extern void free_temp_point(g3s_point *p);
const polygon_clip_points &clip_polygon(polygon_clip_points &src,polygon_clip_points &dest,int *nv,g3s_codes *cc);
extern void init_free_points(void);
void clip_line(g3s_point *&p0,g3s_point *&p1,ubyte codes_or);

#endif

#endif
