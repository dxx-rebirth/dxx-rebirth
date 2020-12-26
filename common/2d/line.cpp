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
 * Graphical routines for drawing lines.
 *
 */

#include <stdlib.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "maths.h"
#include "clip.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {

namespace {

/*
Symmetric Double Step Line Algorithm
by Brian Wyvill
from "Graphics Gems", Academic Press, 1990
*/

/* non-zero flag indicates the pixels needing EXCHG back. */
static void plot(grs_canvas &canvas, int x, int y, int flag, const color_palette_index color)
#define plot(x,y,f)	plot(canvas,x,y,f,color)
{
	if (flag)
		std::swap(x, y);
	gr_upixel(canvas.cv_bitmap, x, y, color);
}

static void gr_hline(grs_canvas &canvas, int x1, int x2, const int y, const color_palette_index color)
{
	using std::swap;
	if (x1 > x2)
		swap(x1,x2);
	for (int i=x1; i<=x2; i++ )
		gr_upixel(canvas.cv_bitmap, i, y, color);
}

static void gr_vline(grs_canvas &canvas, int y1, int y2, const int x, const color_palette_index color)
{
	using std::swap;
	if (y1 > y2) swap(y1,y2);
	for (int i=y1; i<=y2; i++ )
		gr_upixel(canvas.cv_bitmap, x, i, color);
}

static void gr_universal_uline(grs_canvas &canvas, int a1, int b1, int a2, int b2, const color_palette_index color)
{
	int dx, dy, incr1, incr2, D, x, y, xend, c, pixels_left;
	int x1, y1;
	int sign_x = 1, sign_y = 1, step, reverse;

	if (a1==a2) {
		gr_vline(canvas, b1, b2, a1, color);
		return;
	}

	if (b1==b2) {
		gr_hline(canvas, a1, a2, b1, color);
		return;
	}

	dx = a2 - a1;
	dy = b2 - b1;

	if (dx < 0) {
		sign_x = -1;
		dx *= -1;
	}
	if (dy < 0) {
		sign_y = -1;
		dy *= -1;
	}

	/* decide increment sign by the slope sign */
	if (sign_x == sign_y)
		step = 1;
	else
		step = -1;

	if (dy > dx) {          /* chooses axis of greatest movement (make * dx) */
		using std::swap;
		swap(a1, b1);
		swap(a2, b2);
		swap(dx, dy);
		reverse = 1;
	} else
		reverse = 0;
	/* note error check for dx==0 should be included here */
	if (a1 > a2) {          /* start from the smaller coordinate */
		x = a2;
		y = b2;
		x1 = a1;
		y1 = b1;
	} else {
		x = a1;
		y = b1;
		x1 = a2;
		y1 = b2;
	}


	/* Note dx=n implies 0 - n or (dx+1) pixels to be set */
	/* Go round loop dx/4 times then plot last 0,1,2 or 3 pixels */
	/* In fact (dx-1)/4 as 2 pixels are already plottted */
	xend = (dx - 1) / 4;
	pixels_left = (dx - 1) % 4;     /* number of pixels left over at the
	                                 * end */
	plot(x, y, reverse);
	plot(x1, y1, reverse);  /* plot first two points */
	incr2 = 4 * dy - 2 * dx;
	if (incr2 < 0) {        /* slope less than 1/2 */
		c = 2 * dy;
		incr1 = 2 * c;
		D = incr1 - dx;

		for (uint_fast32_t i = xend; i--;)
		{    /* plotting loop */
			++x;
			--x1;
			if (D < 0) {
					/* pattern 1 forwards */
				plot(x, y, reverse);
				plot(++x, y, reverse);
				/* pattern 1 backwards */
				plot(x1, y1, reverse);
				plot(--x1, y1, reverse);
				D += incr1;
			} else {
				if (D < c) {
					/* pattern 2 forwards */
					plot(x, y, reverse);
					plot(++x, y += step, reverse);
					/* pattern 2 backwards */
					plot(x1, y1, reverse);
					plot(--x1, y1 -= step, reverse);
				} else {
					/* pattern 3 forwards */
					plot(x, y += step, reverse);
					plot(++x, y, reverse);
					/* pattern 3 backwards */
					plot(x1, y1 -= step, reverse);
					plot(--x1, y1, reverse);
				}
				D += incr2;
			}
		}               /* end for */

		/* plot last pattern */
		if (pixels_left) {
			if (D < 0) {
				plot(++x, y, reverse);  /* pattern 1 */
				if (pixels_left > 1)
					plot(++x, y, reverse);
				if (pixels_left > 2)
					plot(--x1, y1, reverse);
			} else {
				if (D < c) {
					plot(++x, y, reverse);  /* pattern 2  */
					if (pixels_left > 1)
						plot(++x, y += step, reverse);
					if (pixels_left > 2)
						plot(--x1, y1, reverse);
				} else {
				  /* pattern 3 */
					plot(++x, y += step, reverse);
					if (pixels_left > 1)
						plot(++x, y, reverse);
					if (pixels_left > 2)
						plot(--x1, y1 -= step, reverse);
				}
			}
		}               /* end if pixels_left */
	}
	/* end slope < 1/2 */
	else {                  /* slope greater than 1/2 */
		c = 2 * (dy - dx);
		incr1 = 2 * c;
		D = incr1 + dx;
		for (uint_fast32_t i = xend; i--;)
		{
			++x;
			--x1;
			if (D > 0) {
			  /* pattern 4 forwards */
				plot(x, y += step, reverse);
				plot(++x, y += step, reverse);
			  /* pattern 4 backwards */
				plot(x1, y1 -= step, reverse);
				plot(--x1, y1 -= step, reverse);
				D += incr1;
			} else {
				if (D < c) {
				  /* pattern 2 forwards */
					plot(x, y, reverse);
					plot(++x, y += step, reverse);

				  /* pattern 2 backwards */
					plot(x1, y1, reverse);
					plot(--x1, y1 -= step, reverse);
				} else {
				  /* pattern 3 forwards */
					plot(x, y += step, reverse);
					plot(++x, y, reverse);
				  /* pattern 3 backwards */
					plot(x1, y1 -= step, reverse);
					plot(--x1, y1, reverse);
				}
				D += incr2;
			}
		}               /* end for */
		/* plot last pattern */
		if (pixels_left) {
			if (D > 0) {
				plot(++x, y += step, reverse);  /* pattern 4 */
				if (pixels_left > 1)
					plot(++x, y += step, reverse);
				if (pixels_left > 2)
					plot(--x1, y1 -= step, reverse);
			} else {
				if (D < c) {
					plot(++x, y, reverse);  /* pattern 2  */
					if (pixels_left > 1)
						plot(++x, y += step, reverse);
					if (pixels_left > 2)
						plot(--x1, y1, reverse);
				} else {
				  /* pattern 3 */
					plot(++x, y += step, reverse);
					if (pixels_left > 1)
						plot(++x, y, reverse);
					if (pixels_left > 2) {
						if (D > c) /* step 3 */
						   plot(--x1, y1 -= step, reverse);
						else /* step 2 */
							plot(--x1, y1, reverse);
					}
				}
			}
		}
	}
}

}

//unclipped version just calls clipping version for now
void gr_uline(grs_canvas &canvas, const fix _a1, const fix _b1, const fix _a2, const fix _b2, const color_palette_index color)
{
	int a1,b1,a2,b2;
	a1 = f2i(_a1); b1 = f2i(_b1); a2 = f2i(_a2); b2 = f2i(_b2);
	switch(canvas.cv_bitmap.get_type())
	{
#if DXX_USE_OGL
	case bm_mode::ogl:
		ogl_ulinec(canvas, a1, b1, a2, b2, color);
		return;
#endif
	case bm_mode::linear:
		gr_universal_uline(canvas, a1, b1, a2, b2, color);
		return;
	}
	return;
}

// Returns 0 if drawn with no clipping, 1 if drawn but clipped, and
// 2 if not drawn at all.

void gr_line(grs_canvas &canvas, fix a1, fix b1, fix a2, fix b2, const color_palette_index color)
{
	int x1, y1, x2, y2;
	x1 = i2f(MINX);
	y1 = i2f(MINY);
	x2 = i2f(canvas.cv_bitmap.bm_w - 1);
	y2 = i2f(canvas.cv_bitmap.bm_h - 1);

	CLIPLINE(a1,b1,a2,b2,x1,y1,x2,y2,return,, FIXSCALE );
	gr_uline(canvas, a1, b1, a2, b2, color);
}

}
