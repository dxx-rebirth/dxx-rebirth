/*
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

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"

//#include "tmapext.h"

#ifndef OGL

void (*scanline_func)(int,fix,fix);

// -------------------------------------------------------------------------------------
//	Texture map current scanline.
//	Uses globals Du_dx and Dv_dx to incrementally compute u,v coordinates
// -------------------------------------------------------------------------------------
void tmap_scanline_flat(int y, fix xleft, fix xright)
{
	if (xright < xleft)
		return;

	// setup to call assembler scanline renderer

	fx_y = y;
	fx_xleft = xleft/F1_0;		// (xleft >> 16) != xleft/F1_0 for negative numbers, f2i caused random crashes
	fx_xright = xright/F1_0;

	if ( grd_curcanv->cv_fade_level >= GR_FADE_OFF )
		cur_tmap_scanline_flat();
	else	{
		tmap_flat_shade_value = grd_curcanv->cv_fade_level;
		cur_tmap_scanline_shaded();
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
void texture_map_flat(g3ds_tmap *t, int color)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,y, dy;
	fix	dx_dy_left,dx_dy_right;
	int	max_y_vertex;
	fix	xleft,xright;
	fix	recip_dy;
	g3ds_vertex *v3d;

	v3d = t->verts;

	tmap_flat_color = color;

	// Determine top and bottom y coords.
	compute_y_bounds(t,&vlt,&vlb,&vrt,&vrb,&max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);

	// Set amount to change x coordinate for each advance to next scanline.
	dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dy = fix_recip[dy];
	else
		recip_dy = F1_0/dy;

	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

	dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dy = fix_recip[dy];
	else
		recip_dy = F1_0/dy;

	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	// scan all rows in texture map from top through first break.
	// @mk: Should we render the scanline for y==boty?  This violates Matt's spec.

	for (y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == f2i(v3d[vlb].y2d)) {
			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t->nv);
			}
			dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == f2i(v3d[vrb].y2d)) {
			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t->nv);
			}

			dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;

		}

		//tmap_scanline_flat(y, xleft, xright);
		(*scanline_func)(y, xleft, xright);

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}
	//tmap_scanline_flat(y, xleft, xright);
	(*scanline_func)(y, xleft, xright);
}


//	-----------------------------------------------------------------------------------------
//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(int nverts, int *vert )
{
	gr_upoly_tmap_ylr(nverts, vert, tmap_scanline_flat);
}

#include "3d.h"
#include "dxxerror.h"

typedef struct pnt2d {
	fix x,y;
} pnt2d;

#ifdef __WATCOMC__
#pragma off (unreferenced)		//bp not referenced
#endif

//this takes the same partms as draw_tmap, but draws a flat-shaded polygon
void draw_tmap_flat(grs_bitmap *bp,int nverts,g3s_point **vertbuf)
{
	pnt2d	points[MAX_TMAP_VERTS];
	int	i;
	fix	average_light;
	int	color;

	Assert(nverts < MAX_TMAP_VERTS);

	average_light = vertbuf[0]->p3_l;
	for (i=1; i<nverts; i++)
		average_light += vertbuf[i]->p3_l;

	if (nverts == 4)
		average_light = f2i(average_light * NUM_LIGHTING_LEVELS/4);
	else
		average_light = f2i(average_light * NUM_LIGHTING_LEVELS/nverts);

	if (average_light < 0)
		average_light = 0;
	else if (average_light > NUM_LIGHTING_LEVELS-1)
		average_light = NUM_LIGHTING_LEVELS-1;

	color = gr_fade_table[average_light*256 + bp->avg_color];
	gr_setcolor(color);

	for (i=0;i<nverts;i++) {
		points[i].x = vertbuf[i]->p3_sx;
		points[i].y = vertbuf[i]->p3_sy;
	}

	gr_upoly_tmap(nverts,(int *) points);

}
#ifdef __WATCOMC__
#pragma on (unreferenced)
#endif

//	-----------------------------------------------------------------------------------------
//This is like gr_upoly_tmap() but instead of drawing, it calls the specified
//function with ylr values
void gr_upoly_tmap_ylr(int nverts, int *vert, void (*ylr_func)(int,fix,fix) )
{
	g3ds_tmap	my_tmap;
	int			i;

	//--now called from g3_start_frame-- init_interface_vars_to_assembler();

	my_tmap.nv = nverts;

	for (i=0; i<nverts; i++) {
		my_tmap.verts[i].x2d = *vert++;
		my_tmap.verts[i].y2d = *vert++;
	}

	scanline_func = ylr_func;

	texture_map_flat(&my_tmap, COLOR);
}

#endif //!OGL
