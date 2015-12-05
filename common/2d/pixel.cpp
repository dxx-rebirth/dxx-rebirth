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
 * Graphical routines for setting a pixel.
 *
 */

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#ifdef OGL
#include "ogl_init.h"
#endif

inline namespace dcx {

void gr_upixel(unsigned x, unsigned y)
{
	switch (TYPE)
	{
#ifdef OGL
	case BM_OGL:
		ogl_upixelc(x,y,COLOR);
		return;
#endif
	case BM_LINEAR:
		DATA[ ROWSIZE*y+x ] = COLOR;
		return;
	}
}

void gr_pixel(unsigned x, unsigned y)
{
	if (unlikely(x >= GWIDTH || y >= GHEIGHT))
		return;
	gr_upixel (x, y);
}

static inline void gr_bm_upixel(grs_bitmap &bm, uint_fast32_t x, uint_fast32_t y, uint8_t color )
{
	switch (bm.get_type())
	{
#ifdef OGL
	case BM_OGL:
		ogl_upixelc(bm.bm_x+x,bm.bm_y+y,color);
		return;
#endif
	case BM_LINEAR:
		bm.get_bitmap_data()[bm.bm_rowsize*y+x] = color;
		return;
	}
}

void gr_bm_pixel(grs_bitmap &bm, uint_fast32_t x, uint_fast32_t y, uint8_t color )
{
	if (unlikely(x >= bm.bm_w || y >= bm.bm_h))
		return;
	gr_bm_upixel (bm, x, y, color);
}

}
