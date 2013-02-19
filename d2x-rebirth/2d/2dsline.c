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
 * Graphical routines for drawing solid scanlines.
 *
 */

#include <string.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"

void gr_linear_darken(ubyte * dest, int darkening_level, int count, ubyte * fade_table) {
	register int i;

	for (i=0;i<count;i++)
	{
		*dest = fade_table[(*dest)+(darkening_level*256)];
		dest++;
	}
}

void gr_linear_stosd( ubyte * dest, unsigned char color, unsigned int nbytes) {
	memset(dest,color,nbytes);
}

void gr_uscanline( int x1, int x2, int y )
{
	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF) {
		switch(TYPE)
		{
		case BM_LINEAR:
#ifdef OGL
		case BM_OGL:
#endif
			gr_linear_stosd( DATA + ROWSIZE*y + x1, (unsigned char)COLOR, x2-x1+1);
			break;
		}
	} else {
		switch(TYPE)
		{
		case BM_LINEAR:
#ifdef OGL
		case BM_OGL:
#endif
			gr_linear_darken( DATA + ROWSIZE*y + x1, grd_curcanv->cv_fade_level, x2-x1+1, gr_fade_table);
			break;
		}
	}
}

void gr_scanline( int x1, int x2, int y )
{
	if ((y<0)||(y>MAXY)) return;

	if (x2 < x1 ) x2 ^= x1 ^= x2;

	if (x1 > MAXX) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > MAXX) x2 = MAXX;

	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF) {
		switch(TYPE)
		{
		case BM_LINEAR:
#ifdef OGL
		case BM_OGL:
#endif
			gr_linear_stosd( DATA + ROWSIZE*y + x1, (unsigned char)COLOR, x2-x1+1);
			break;
		}
	} else {
		switch(TYPE)
		{
		case BM_LINEAR:
#ifdef OGL
		case BM_OGL:
#endif
			gr_linear_darken( DATA + ROWSIZE*y + x1, grd_curcanv->cv_fade_level, x2-x1+1, gr_fade_table);
			break;
		}
	}
}
