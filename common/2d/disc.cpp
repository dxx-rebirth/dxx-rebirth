/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Graphical routines for drawing a disk.
 *
 */

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"

namespace dcx {

#if !DXX_USE_OGL
int gr_disk(fix xc1,fix yc1,fix r1, const uint8_t color)
{
	int p,x, y, xc, yc, r;

	r = f2i(r1);
	xc = f2i(xc1);
	yc = f2i(yc1);
	p=3-(r*2);
	x=0;
	y=r;

	// Big clip
	if ( (xc+r) < 0 ) return 1;
	if ( (xc-r) > GWIDTH ) return 1;
	if ( (yc+r) < 0 ) return 1;
	if ( (yc-r) > GHEIGHT ) return 1;

	while(x<y)
	{
		// Draw the first octant
		gr_scanline(xc-y, xc+y, yc-x, color);
		gr_scanline(xc-y, xc+y, yc+x, color);

		if (p<0) 
			p=p+(x<<2)+6;
		else	{
			// Draw the second octant
			gr_scanline(xc-x, xc+x, yc-y, color);
			gr_scanline(xc-x, xc+x, yc+y, color);
			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y)	{
		gr_scanline(xc-x, xc+x, yc-y, color);
		gr_scanline(xc-x, xc+x, yc+y, color);
	}
	return 0;
}

#endif

}
