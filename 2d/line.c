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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/line.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:00 $
 *
 * Graphical routines for drawing lines.
 *
 * $Log: line.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:00  zicodxx
 * initial import
 *
 * Revision 1.3  1999/10/07 02:27:14  donut
 * OGL includes to remove warnings
 *
 * Revision 1.2  1999/09/30 23:02:27  donut
 * opengl direct support for ingame and normal menus, fonts as textures, and automap support
 *
 * Revision 1.1.1.1  1999/06/14 21:57:25  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.10  1994/11/18  22:50:02  john
 * Changed shorts to ints in parameters.
 * 
 * Revision 1.9  1994/07/13  12:03:04  john
 * Added assembly modex line-drawer.
 * 
 * Revision 1.8  1993/12/06  18:18:03  john
 * took out aaline.
 * 
 * Revision 1.7  1993/12/03  12:11:17  john
 * ,
 * 
 * Revision 1.6  1993/11/18  09:40:22  john
 * Added laser-line
 * 
 * Revision 1.5  1993/10/15  16:23:36  john
 * y
 * 
 * Revision 1.4  1993/09/29  16:13:58  john
 * optimized
 * 
 * Revision 1.3  1993/09/26  18:44:12  matt
 * Added gr_uline(), which just calls gr_line(), and made both take
 * fixes, and shift down themselves.
 * 
 * Revision 1.2  1993/09/11  19:50:15  matt
 * In gr_vline() & gr_hline(), check for start > end, and EXCHG if so
 * 
 * Revision 1.1  1993/09/08  11:43:54  john
 * Initial revision
 * 
 *
 */

#include <stdlib.h>

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "fix.h"

#include "clip.h"

#ifdef __MSDOS__
#include "modex.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif


/*
Symmetric Double Step Line Algorithm
by Brian Wyvill
from "Graphics Gems", Academic Press, 1990
*/

/* non-zero flag indicates the pixels needing EXCHG back. */
void plot(int x,int y,int flag)
{   if (flag)
		gr_upixel(y, x);
	else
		gr_upixel(x, y);
}

int gr_hline(int x1, int x2, int y)
{   int i;

	if (x1 > x2) EXCHG(x1,x2);
	for (i=x1; i<=x2; i++ )
		gr_upixel( i, y );
	return 0;
}

int gr_vline(int y1, int y2, int x)
{   int i;
	if (y1 > y2) EXCHG(y1,y2);
	for (i=y1; i<=y2; i++ )
		gr_upixel( x, i );
	return 0;
}

void gr_universal_uline(int a1, int b1, int a2, int b2)
{
	int dx, dy, incr1, incr2, D, x, y, xend, c, pixels_left;
	int x1, y1;
	int sign_x = 1, sign_y = 1, step, reverse, i;

	if (a1==a2) {
		gr_vline(b1,b2,a1);
		return;
	}

	if (b1==b2) {
		gr_hline(a1,a2,b1);
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
		EXCHG(a1, b1);
		EXCHG(a2, b2);
		EXCHG(dx, dy);
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

		for (i = 0; i < xend; i++) {    /* plotting loop */
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
		for (i = 0; i < xend; i++) {
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


//unclipped version just calls clipping version for now
int gr_uline(fix _a1, fix _b1, fix _a2, fix _b2)
{
	int a1,b1,a2,b2;
	a1 = f2i(_a1); b1 = f2i(_b1); a2 = f2i(_a2); b2 = f2i(_b2);
	switch(TYPE)
	{
#ifdef OGL
		case BM_OGL:
			ogl_ulinec(a1,b1,a2,b2,COLOR);
			return 0;
#endif
	case BM_LINEAR:
               #ifdef NO_ASM
                gr_universal_uline( a1,b1,a2,b2);
               #else
		gr_linear_line( a1, b1, a2, b2 );
               #endif
		return 0;
#ifdef __MSDOS__
        case BM_MODEX:
		modex_line_x1 = a1+XOFFSET;		
		modex_line_y1 = b1+YOFFSET;		
		modex_line_x2 = a2+XOFFSET;		
		modex_line_y2 = b2+YOFFSET;		
		modex_line_Color = grd_curcanv->cv_color;
		gr_modex_line();
		return 0;
        default:
		gr_universal_uline( a1, b1, a2, b2 );
		return 0;
#endif
	}
	return 2;
}

// Returns 0 if drawn with no clipping, 1 if drawn but clipped, and
// 2 if not drawn at all.

int gr_line(fix a1, fix b1, fix a2, fix b2)
{
	int x1, y1, x2, y2;
	int clipped=0;

	x1 = i2f(MINX);
	y1 = i2f(MINY);
	x2 = i2f(MAXX);
	y2 = i2f(MAXY);

	CLIPLINE(a1,b1,a2,b2,x1,y1,x2,y2,return 2,clipped=1, FIXSCALE );

	gr_uline( a1, b1, a2, b2 );

	return clipped;

}


