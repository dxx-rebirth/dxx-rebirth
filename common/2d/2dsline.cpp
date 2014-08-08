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
 * Graphical routines for drawing solid scanlines.
 *
 */

#include <algorithm>
#include <string.h>
#include "gr.h"
#include "grdef.h"

static void gr_linear_darken(ubyte * dest, int darkening_level, int count, const gft_array1 &fade_table) {
	auto predicate = [&](ubyte c) { return fade_table[darkening_level][c]; };
	std::transform(dest, dest + count, dest, predicate);
}

static void gr_linear_stosd( ubyte * dest, unsigned char color, unsigned int nbytes) {
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
