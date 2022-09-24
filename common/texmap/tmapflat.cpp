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
 * Flat shader derived from texture mapper (a little slow)
 *
 */

#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"

#include "compiler-range_for.h"
#include "partial_range.h"

#if !DXX_USE_OGL
#include "3d.h"
#include "dxxerror.h"
#include "d_zip.h"

namespace dcx {

namespace {

static void gr_upoly_tmap_ylr(grs_canvas &, uint_fast32_t nverts, const int *vert, uint8_t color);

// -------------------------------------------------------------------------------------
//	Texture map current scanline.
//	Uses globals Du_dx and Dv_dx to incrementally compute u,v coordinates
// -------------------------------------------------------------------------------------
static void tmap_scanline_flat(grs_canvas &canvas, int y, fix xleft, fix xright)
{
	if (xright < xleft)
		return;

	// setup to call assembler scanline renderer

	fx_y = y;
	fx_xleft = xleft/F1_0;		// (xleft >> 16) != xleft/F1_0 for negative numbers, f2i caused random crashes
	fx_xright = xright/F1_0;

	if (canvas.cv_fade_level >= GR_FADE_OFF)
		c_tmap_scanline_flat();
	else	{
		c_tmap_scanline_shaded(canvas.cv_fade_level);
	}	
}

//--unused-- void tmap_scanline_shaded(int y, fix xleft, fix xright)
//--unused-- {
//--unused-- 	fix	dx;
//--unused-- 
//--unused-- 	dx = xright - xleft;
//--unused-- 
//--unused-- 	// setup to call assembler scanline renderer
//--unused-- 
//--unused-- 	fx_y = y << 16;
//--unused-- 	fx_xleft = xleft;
//--unused-- 	fx_xright = xright;
//--unused-- 
//--unused-- 	asm_tmap_scanline_shaded();
//--unused-- }

// -------------------------------------------------------------------------------------
//	Render a texture map.
// Linear in outer loop, linear in inner loop.
// -------------------------------------------------------------------------------------
static void texture_map_flat(grs_canvas &canvas, const g3ds_tmap &t, int color)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,dy;
	fix	dx_dy_left,dx_dy_right;
	int	max_y_vertex;
	fix	xleft,xright;
	fix	recip_dy;
	auto &v3d = t.verts;

	tmap_flat_color = color;

	// Determine top and bottom y coords.
	compute_y_bounds(t,vlt,vlb,vrt,vrb,max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);

	// Set amount to change x coordinate for each advance to next scanline.
	dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
	recip_dy = fix_recip(dy);

	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

	dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
	recip_dy = fix_recip(dy);

	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	// scan all rows in texture map from top through first break.
	// @mk: Should we render the scanline for y==boty?  This violates Matt's spec.

	for (int y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == f2i(v3d[vlb].y2d)) {
			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t.nv);
			}
			dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
			recip_dy = fix_recip(dy);

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == f2i(v3d[vrb].y2d)) {
			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t.nv);
			}

			dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
			recip_dy = fix_recip(dy);

			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;

		}
		tmap_scanline_flat(canvas, y, xleft, xright);

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}
	tmap_scanline_flat(canvas, boty, xleft, xright);
}

}

//	-----------------------------------------------------------------------------------------
//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(grs_canvas &canvas, uint_fast32_t nverts, const std::array<fix, MAX_POINTS_IN_POLY * 2> &vert, const uint8_t color)
{
	gr_upoly_tmap_ylr(canvas, nverts, vert.data(), color);
}

namespace {

struct pnt2d {
	fix x,y;
};

}

//this takes the same partms as draw_tmap, but draws a flat-shaded polygon
void draw_tmap_flat(grs_canvas &canvas, const grs_bitmap &bp, const std::span<const g3s_point *const> vertbuf)
{
	union {
		std::array<pnt2d, MAX_TMAP_VERTS> points;
		std::array<int, MAX_TMAP_VERTS * (sizeof(pnt2d) / sizeof(int))> ipoints;
	};
	static_assert(sizeof(points) == sizeof(ipoints), "array size mismatch");
	fix	average_light;
	assert(vertbuf.size() < MAX_TMAP_VERTS);
	average_light = 0;
	for (const auto vb : vertbuf)
		average_light += vb->p3_l;

	if (vertbuf.size() == 4)
		average_light = f2i(average_light * NUM_LIGHTING_LEVELS/4);
	else
		average_light = f2i(average_light * NUM_LIGHTING_LEVELS / vertbuf.size());

	if (average_light < 0)
		average_light = 0;
	else if (average_light > NUM_LIGHTING_LEVELS-1)
		average_light = NUM_LIGHTING_LEVELS-1;

	const auto color = gr_fade_table[static_cast<gr_fade_level>(average_light)][bp.avg_color];

	for (auto &&[vb, pt] : zip(vertbuf, points))
	{
		pt.x = vb->p3_sx;
		pt.y = vb->p3_sy;
	}
	gr_upoly_tmap_ylr(canvas, vertbuf.size(), ipoints.data(), color);
}

namespace {

//	-----------------------------------------------------------------------------------------
//This is like gr_upoly_tmap() but instead of drawing, it calls the specified
//function with ylr values
static void gr_upoly_tmap_ylr(grs_canvas &canvas, uint_fast32_t nverts, const int *vert, const uint8_t color)
{
	g3ds_tmap	my_tmap;
	my_tmap.nv = nverts;

	range_for (auto &i, partial_range(my_tmap.verts, nverts))
	{
		i.x2d = *vert++;
		i.y2d = *vert++;
	}
	texture_map_flat(canvas, my_tmap, color);
}

}

}
#endif //!OGL
