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


#include "u_mem.h"
#include "gr.h"

namespace dcx {

#if !DXX_USE_OGL
int gr_ucircle(grs_canvas &canvas, const fix xc1, const fix yc1, const fix r1, const color_palette_index color)
{
	int p,x, y, xc, yc, r;

	r = f2i(r1);
	xc = f2i(xc1);
	yc = f2i(yc1);
	p=3-(r*2);
	x=0;
	y=r;

	while(x<y)
	{
		// Draw the first octant
		gr_upixel(canvas.cv_bitmap, xc - y, yc - x, color);
		gr_upixel(canvas.cv_bitmap, xc + y, yc - x, color);
		gr_upixel(canvas.cv_bitmap, xc - y, yc + x, color);
		gr_upixel(canvas.cv_bitmap, xc + y, yc + x, color);

		if (p<0)
			p=p+(x<<2)+6;
		else {
			// Draw the second octant
			gr_upixel(canvas.cv_bitmap, xc - x, yc - y, color);
			gr_upixel(canvas.cv_bitmap, xc + x, yc - y, color);
			gr_upixel(canvas.cv_bitmap, xc - x, yc + y, color);
			gr_upixel(canvas.cv_bitmap, xc + x, yc + y, color);
			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y) {
		gr_upixel(canvas.cv_bitmap, xc - x, yc - y, color);
		gr_upixel(canvas.cv_bitmap, xc + x, yc - y, color);
		gr_upixel(canvas.cv_bitmap, xc - x, yc + y, color);
		gr_upixel(canvas.cv_bitmap, xc + x, yc + y, color);
	}
	return 0;
}

#endif //!OGL

}
