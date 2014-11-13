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

extern void free_temp_point(g3s_point *p);
extern g3s_point **clip_polygon(g3s_point **src,g3s_point **dest,int *nv,g3s_codes *cc);
extern void init_free_points(void);
void clip_line(g3s_point *&p0,g3s_point *&p1,ubyte codes_or);

#endif

#endif
