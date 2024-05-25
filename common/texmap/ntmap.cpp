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
 * Start of conversion to new texture mapper.
 *
 */

#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "dxxerror.h"
#include "render.h"
#include "texmap.h"
#include "texmapl.h"
#include "rle.h"
#include "scanline.h"
#include "u_mem.h"
#include "d_zip.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include <utility>

namespace dcx {

#if DXX_USE_EDITOR
#define EDITOR_TMAP 1       //if in, include extra stuff
#endif

// Temporary texture map, interface from Matt's 3d system to Mike's texture mapper.

int     Lighting_on=1;                  // initialize to no lighting
unsigned	Current_seg_depth;		// HACK INTERFACE: how far away the current segment (& thus texture) is

// These variables are the interface to assembler.  They get set for each texture map, which is a real waste of time.
//	They should be set only when they change, which is generally when the window bounds change.  And, even still, it's
//	a pretty bad interface.
int	bytes_per_row=-1;
unsigned char *write_buffer;

fix fx_l, fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx, fx_dl_dx;
int fx_xleft, fx_xright, fx_y;
const color_palette_index *pixptr;
uint8_t Transparency_on = 0;
uint8_t tmap_flat_color;

int	Interpolation_method;	// 0 = choose best method
// -------------------------------------------------------------------------------------
template <std::size_t... N>
static inline constexpr const std::array<fix, 1 + sizeof...(N)> init_fix_recip_table(std::index_sequence<0, N...>)
{
	/* gcc 4.5 fails on bare initializer list */
	return std::array<fix, 1 + sizeof...(N)>{{F1_0, (F1_0 / N)...}};
}

constexpr std::array<fix, FIX_RECIP_TABLE_SIZE> fix_recip_table = init_fix_recip_table(std::make_index_sequence<FIX_RECIP_TABLE_SIZE>());

// -------------------------------------------------------------------------------------
//	Initialize interface variables to assembler.
//	These things used to be constants.  This routine is now (10/6/93) getting called for
//	every texture map.  It should get called whenever the window changes, or, preferably,
//	not at all.  I'm pretty sure these variables are only being used for range checking.
void init_interface_vars_to_assembler(void)
{
	grs_bitmap	*bp;
	bp = &grd_curcanv->cv_bitmap;

	Assert(bp!=NULL);
	Assert(bp->bm_data!=NULL);
	//	If bytes_per_row has changed, create new table of pointers.
	if (bytes_per_row != static_cast<int>(bp->bm_rowsize)) {
		bytes_per_row = static_cast<int>(bp->bm_rowsize);
	}

	write_buffer = bp->bm_mdata;

	Window_clip_left = 0;
	Window_clip_right = static_cast<int>(bp->bm_w)-1;
	Window_clip_top = 0;
	Window_clip_bot = static_cast<int>(bp->bm_h)-1;
}

static int Lighting_enabled;
// -------------------------------------------------------------------------------------
//                             VARIABLES

// -------------------------------------------------------------------------------------
//	Returns number preceding val modulo modulus.
//	prevmod(3,4) = 2
//	prevmod(0,4) = 3
int prevmod(int val,int modulus)
{
	if (val > 0)
		return val-1;
	else
		return modulus-1;
//	return (val + modulus - 1) % modulus;
}


//	Returns number succeeding val modulo modulus.
//	succmod(3,4) = 0
//	succmod(0,4) = 1
int succmod(int val,int modulus)
{
	if (val < modulus-1)
		return val+1;
	else
		return 0;

//	return (val + 1) % modulus;
}

// -------------------------------------------------------------------------------------
//	Select topmost vertex (minimum y coordinate) and bottommost (maximum y coordinate) in
//	texture map.  If either is part of a horizontal edge, then select leftmost vertex for
//	top, rightmost vertex for bottom.
//	Important: Vertex is selected with integer precision.  So, if there are vertices at
//	(0.0,0.7) and (0.5,0.3), the first vertex is selected, because they y coordinates are
//	considered the same, so the smaller x is favored.
//	Parameters:
//		nv		number of vertices
//		v3d	pointer to 3d vertices containing u,v,x2d,y2d coordinates
//	Results in:
//		*min_y_ind
//		*max_y_ind
// -------------------------------------------------------------------------------------
void compute_y_bounds(const g3ds_tmap &t, int &vlt, int &vlb, int &vrt, int &vrb,int &bottom_y_ind)
{
	int	min_y,max_y;
	int	min_y_ind;
	int	original_vrt;
	fix	min_x;

	// Scan all vertices, set min_y_ind to vertex with smallest y coordinate.
	min_y = f2i(t.verts[0].y2d);
	max_y = min_y;
	min_y_ind = 0;
	min_x = f2i(t.verts[0].x2d);
	bottom_y_ind = 0;

	for (int i=1; i<t.nv; i++) {
		if (f2i(t.verts[i].y2d) < min_y) {
			min_y = f2i(t.verts[i].y2d);
			min_y_ind = i;
			min_x = f2i(t.verts[i].x2d);
		} else if (f2i(t.verts[i].y2d) == min_y) {
			if (f2i(t.verts[i].x2d) < min_x) {
				min_y_ind = i;
				min_x = f2i(t.verts[i].x2d);
			}
		}
		if (f2i(t.verts[i].y2d) > max_y) {
			max_y = f2i(t.verts[i].y2d);
			bottom_y_ind = i;
		}
	}

//--removed mk, 11/27/94--	//	Check for a non-upright-hourglass polygon and fix, if necessary, by bashing a y coordinate.
//--removed mk, 11/27/94--	//	min_y_ind = index of minimum y coordinate, *bottom_y_ind = index of maximum y coordinate
//--removed mk, 11/27/94--{
//--removed mk, 11/27/94--	int	max_temp, min_temp;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	max_temp = *bottom_y_ind;
//--removed mk, 11/27/94--	if (*bottom_y_ind < min_y_ind)
//--removed mk, 11/27/94--		max_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=min_y_ind; i<max_temp; i++) {
//--removed mk, 11/27/94--		if (f2i(t->verts[i%t->nv].y2d) > f2i(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	min_temp = min_y_ind;
//--removed mk, 11/27/94--	if (min_y_ind < *bottom_y_ind)
//--removed mk, 11/27/94--		min_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=*bottom_y_ind; i<min_temp; i++) {
//--removed mk, 11/27/94--		if (f2i(t->verts[i%t->nv].y2d) < f2i(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--}

	// Set "vertex left top", etc. based on vertex with topmost y coordinate
	vlb = prevmod(vlt = min_y_ind,t.nv);
	vrb = succmod(vrt = vlt,t.nv);

	// If right edge is horizontal, then advance along polygon bound until it no longer is or until all
	// vertices have been examined.
	// (Left edge cannot be horizontal, because *vlt is set to leftmost point with highest y coordinate.)

	original_vrt = vrt;

	while (f2i(t.verts[vrt].y2d) == f2i(t.verts[vrb].y2d)) {
		if (succmod(vrt,t.nv) == original_vrt) {
			break;
		}
		vrt = succmod(vrt,t.nv);
		vrb = succmod(vrt,t.nv);
	}
}

// -------------------------------------------------------------------------------------
//	Returns dx/dy given two vertices.
//	If dy == 0, returns 0.0
// -------------------------------------------------------------------------------------
//--fix compute_dx_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex)
//--{
//--	int	dy;
//--
//--	// compute delta x with respect to y for any edge
//--	dy = f2i(t->verts[bottom_vertex].y2d - t->verts[top_vertex].y2d) + 1;
//--	if (dy)
//--		return (t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d) / dy;
//--	else
//--		return 0;
//--
//--}

//#if !DXX_USE_OGL
static fix compute_du_dy_lin(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t.verts[bottom_vertex].u - t.verts[top_vertex].u, recip_dy);
}


static fix compute_dv_dy_lin(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t.verts[bottom_vertex].v - t.verts[top_vertex].v, recip_dy);
}

static fix compute_dl_dy_lin(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t.verts[bottom_vertex].l - t.verts[top_vertex].l, recip_dy);
}

fix compute_dx_dy(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t.verts[bottom_vertex].x2d - t.verts[top_vertex].x2d, recip_dy);
}

static fix compute_du_dy(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t.verts[bottom_vertex].u,t.verts[bottom_vertex].z) - fixmul(t.verts[top_vertex].u,t.verts[top_vertex].z), recip_dy);
}

static fix compute_dv_dy(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t.verts[bottom_vertex].v,t.verts[bottom_vertex].z) - fixmul(t.verts[top_vertex].v,t.verts[top_vertex].z), recip_dy);
}

static fix compute_dz_dy(const g3ds_tmap &t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t.verts[bottom_vertex].z - t.verts[top_vertex].z, recip_dy);

}

// -------------------------------------------------------------------------------------
//	Texture map current scanline in perspective.
// -------------------------------------------------------------------------------------
static void ntmap_scanline_lighted(const grs_bitmap &srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright)
{
	fix	dx,recip_dx;

	fx_xright = f2i(xright);
	//edited 06/27/99 Matt Mueller - moved these tests up from within the switch so as not to do a bunch of needless calculations when we are just gonna return anyway.  Slight fps boost?
	if (fx_xright < Window_clip_left)
		return;
	fx_xleft = f2i(xleft);
	if (fx_xleft > Window_clip_right)
		return;
	//end edit -MM

	dx = fx_xright - fx_xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	// setup to call assembler scanline renderer
	recip_dx = fix_recip(dx);

	fx_u = uleft;
	fx_v = vleft;
	fx_z = zleft;
	
	fx_du_dx = fixmul(uright - uleft,recip_dx);
	fx_dv_dx = fixmul(vright - vleft,recip_dx);
	fx_dz_dx = fixmul(zright - zleft,recip_dx);
	fx_y = y;
	pixptr = srcb.bm_data;

	switch (Lighting_enabled) {
		case 0:
			//added 05/17/99 Matt Mueller - prevent writing before the buffer
            if ((fx_y == 0) && (fx_xleft < 0))
				fx_xleft = 0;
			//end addition -MM
			if (fx_xright > Window_clip_right)
				fx_xright = Window_clip_right;
			
			cur_tmap_scanline_per();
			break;
		case 1: {
			fix	mul_thing;

			if (lleft < 0) lleft = 0;
			if (lright < 0) lright = 0;
			if (lleft > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lleft = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);
			if (lright > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lright = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);

			fx_l = lleft;
			fx_dl_dx = fixmul(lright - lleft,recip_dx);

			//	This is a pretty ugly hack to prevent lighting overflows.
			mul_thing = dx * fx_dl_dx;
			if (lleft + mul_thing < 0)
				fx_dl_dx += 12;
			else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2))
				fx_dl_dx -= 12;

			//added 05/17/99 Matt Mueller - prevent writing before the buffer
            if ((fx_y == 0) && (fx_xleft < 0))
				fx_xleft = 0;
			//end addition -MM
			if (fx_xright > Window_clip_right)
				fx_xright = Window_clip_right;

			cur_tmap_scanline_per();
			break;
		}
		case 2:
#ifdef EDITOR_TMAP
			fx_xright = f2i(xright);
			fx_xleft = f2i(xleft);

			tmap_flat_color = 1;
			c_tmap_scanline_flat();
#else
			Int3();	//	Illegal, called an editor only routine!
#endif
			break;
	}

}

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
static void ntexture_map_lighted(const grs_bitmap &srcb, const g3ds_tmap &t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dz_dy_left,dz_dy_right;
	fix	dl_dy_left,dl_dy_right;
	fix	recip_dyl, recip_dyr;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,zleft,zright,lleft,lright;
	int	next_break_left, next_break_right;

        //remove stupid warnings in compile
        dl_dy_left = F1_0;
        dl_dy_right = F1_0;
        lleft = F1_0;
        lright = F1_0;

	auto &v3d = t.verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t,vlt,vlb,vrt,vrb,max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);
	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	// Set amount to change x coordinate for each advance to next scanline.
	dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
	recip_dyl = fix_recip(dy);

	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	du_dy_left = compute_du_dy(t,vlt,vlb, recip_dyl);
	dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dyl);
	dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dyl);

	dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
	recip_dyr = fix_recip(dy);

	du_dy_right = compute_du_dy(t,vrt,vrb, recip_dyr);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);
	dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dyr);
	dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dyr);

	if (Lighting_enabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	zleft = v3d[vlt].z;
	zright = v3d[vrt].z;

	uleft = fixmul(v3d[vlt].u,zleft);
	uright = fixmul(v3d[vrt].u,zright);
	vleft = fixmul(v3d[vlt].v,zleft);
	vright = fixmul(v3d[vrt].v,zright);

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (int y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t.nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
			recip_dy = fix_recip(dy);

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			zleft = v3d[vlt].z;
			uleft = fixmul(v3d[vlt].u,zleft);
			vleft = fixmul(v3d[vlt].v,zleft);
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dy);
			dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t.nv);
			}

			next_break_right = f2i(v3d[vrb].y2d);

			dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
			recip_dy = fix_recip(dy);

			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			zright = v3d[vrt].z;
			uright = fixmul(v3d[vrt].u,zright);
			vright = fixmul(v3d[vrt].v,zright);

			du_dy_right = compute_du_dy(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dy);
			dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) {
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

		zleft += dz_dy_left;
		zright += dz_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

	ntmap_scanline_lighted(srcb,boty,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
}


// -------------------------------------------------------------------------------------
//	Texture map current scanline using linear interpolation.
// -------------------------------------------------------------------------------------
static void ntmap_scanline_lighted_linear(const grs_bitmap &srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix lleft, fix lright)
{
	fix	dx,recip_dx,du_dx,dv_dx,dl_dx;

	dx = f2i(xright) - f2i(xleft);
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

		// setup to call assembler scanline renderer
	recip_dx = fix_recip(dx);

		du_dx = fixmul(uright - uleft,recip_dx);
		dv_dx = fixmul(vright - vleft,recip_dx);

		fx_u = uleft;
		fx_v = vleft;
		fx_du_dx = du_dx;
		fx_dv_dx = dv_dx;
		fx_y = y;
		fx_xright = f2i(xright);
		fx_xleft = f2i(xleft);
		pixptr = srcb.bm_data;

		switch (Lighting_enabled) {
			case 0:
				//added 07/11/99 adb - prevent writing before the buffer
				if (fx_xleft < 0)
					fx_xleft = 0;
				//end addition -adb
				
				c_tmap_scanline_lin_nolight();
				break;
			case 1:
				if (lleft < F1_0/2)
					lleft = F1_0/2;
				if (lright < F1_0/2)
					lright = F1_0/2;

				if (lleft > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lleft = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;
				if (lright > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lright = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;

				//added 07/11/99 adb - prevent writing before the buffer
				if (fx_xleft < 0)
					fx_xleft = 0;
				//end addition -adb

{
			fix mul_thing;

			fx_l = lleft;
			fx_dl_dx = fixmul(lright - lleft,recip_dx);

			//	This is a pretty ugly hack to prevent lighting overflows.
			mul_thing = dx * fx_dl_dx;
			if (lleft + mul_thing < 0)
				fx_dl_dx += 12;
			else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2))
				fx_dl_dx -= 12;
}

				fx_l = lleft;
				dl_dx = fixmul(lright - lleft,recip_dx);
				fx_dl_dx = dl_dx;
				c_tmap_scanline_lin();
				break;
			case 2:
#ifdef EDITOR_TMAP
				fx_xright = f2i(xright);
				fx_xleft = f2i(xleft);
				tmap_flat_color = 1;
				c_tmap_scanline_flat();
#else
				Int3();	//	Illegal, called an editor only routine!
#endif
				break;
		}
}

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
static void ntexture_map_lighted_linear(const grs_bitmap &srcb, const g3ds_tmap &t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dl_dy_left,dl_dy_right;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,lleft,lright;
	int	next_break_left, next_break_right;
	fix	recip_dyl, recip_dyr;

        //remove stupid warnings in compile
        dl_dy_left = F1_0;
        dl_dy_right = F1_0;
        lleft = F1_0;
        lright = F1_0;

	auto &v3d = t.verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t,vlt,vlb,vrt,vrb,max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);

	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
	recip_dyl = fix_recip(dy);

	dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
	recip_dyr = fix_recip(dy);

	// Set amount to change x coordinate for each advance to next scanline.
	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);

	du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dyl);
	du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dyr);

	dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dyl);
	dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dyr);

	if (Lighting_enabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	uleft = v3d[vlt].u;
	uright = v3d[vrt].u;
	vleft = v3d[vlt].v;
	vright = v3d[vrt].v;

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (int y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t.nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t.verts[vlb].y2d) - f2i(t.verts[vlt].y2d);
			recip_dy = fix_recip(dy);

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			uleft = v3d[vlt].u;
			vleft = v3d[vlt].v;
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t.nv);
			}

			dy = f2i(t.verts[vrb].y2d) - f2i(t.verts[vrt].y2d);
			recip_dy = fix_recip(dy);

			next_break_right = f2i(v3d[vrb].y2d);
			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			uright = v3d[vrt].u;
			vright = v3d[vrt].v;

			du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) {
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

	ntmap_scanline_lighted_linear(srcb,boty,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
}

// fix	DivNum = F1_0*12;

// -------------------------------------------------------------------------------------
// Interface from Matt's data structures to Mike's texture mapper.
// -------------------------------------------------------------------------------------
void draw_tmap(grs_canvas &canvas, const grs_bitmap &rbp, const std::span<const g3s_point *const> vertbuf)
{
	//	These variables are used in system which renders texture maps which lie on one scanline as a line.
	// fix	div_numerator;
	int	lighting_on_save = Lighting_on;

	assert(vertbuf.size() <= MAX_TMAP_VERTS);

	const grs_bitmap *bp = &rbp;
	//	If no transparency and seg depth is large, render as flat shaded.
	if ((Current_seg_depth > Max_linear_depth) && ((bp->get_flag_mask(3)) == 0)) {
		draw_tmap_flat(canvas, rbp, vertbuf);
		return;
	}

	bp = rle_expand_texture(*bp);		// Expand if rle'd

	Transparency_on = bp->get_flag_mask(BM_FLAG_TRANSPARENT);
	if (bp->get_flag_mask(BM_FLAG_NO_LIGHTING))
		Lighting_on = 0;


	// Setup texture map in Tmap1
	g3ds_tmap Tmap1;
	Tmap1.nv = vertbuf.size();						// Initialize number of vertices

// 	div_numerator = DivNum;	//f1_0*3;

	for (auto &&[vp, tvp] : zip(vertbuf, Tmap1.verts))
	{
		tvp.x2d = vp->p3_sx;
		tvp.y2d = vp->p3_sy;

		//	Check for overflow on fixdiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
		auto clipped_p3_z = std::max(256, vp->p3_vec.z);
		tvp.z = fixdiv(F1_0*12, clipped_p3_z);
		tvp.u = vp->p3_u << 6; //* bp->bm_w;
		tvp.v = vp->p3_v << 6; //* bp->bm_h;

		Assert(Lighting_on < 3);

		if (Lighting_on)
			tvp.l = vp->p3_l * NUM_LIGHTING_LEVELS;
	}


	Lighting_enabled = Lighting_on;

	// Now, call my texture mapper.
		switch (Interpolation_method) {	// 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
			case 0:								// choose best interpolation
				if (Current_seg_depth > Max_perspective_depth)
				{
				case 1:								// linear interpolation
					ntexture_map_lighted_linear(*bp, Tmap1);
				}
				else
				{
					[[fallthrough]];
				case 2:								// perspective every 8th pixel interpolation
				case 3:								// perspective every pixel interpolation
					ntexture_map_lighted(*bp, Tmap1);
				}
				break;
			default:
				Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}

	Lighting_on = lighting_on_save;

}

}
