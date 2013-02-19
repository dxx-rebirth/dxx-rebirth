/*
 *
 * Graphical routines for drawing a disk.
 *
 */

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"

#ifndef OGL
int gr_disk(fix xc1,fix yc1,fix r1)
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
		gr_scanline( xc-y, xc+y, yc-x );
		gr_scanline( xc-y, xc+y, yc+x );

		if (p<0) 
			p=p+(x<<2)+6;
		else	{
			// Draw the second octant
			gr_scanline( xc-x, xc+x, yc-y );
			gr_scanline( xc-x, xc+x, yc+y );
			p=p+((x-y)<<2)+10;
			y--;
		}
		x++;
	}
	if(x==y)	{
		gr_scanline( xc-x, xc+x, yc-y );
		gr_scanline( xc-x, xc+x, yc+y );
	}
	return 0;
}

#endif
