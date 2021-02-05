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
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {

void gr_upixel(grs_bitmap &cv_bitmap, const unsigned x, const unsigned y, const color_palette_index color)
{
	switch (cv_bitmap.get_type())
	{
#if DXX_USE_OGL
	case bm_mode::ogl:
		ogl_upixelc(cv_bitmap, x, y, color);
		return;
#endif
	case bm_mode::linear:
		cv_bitmap.get_bitmap_data()[cv_bitmap.bm_rowsize * y + x] = color;
		return;
	}
}

void gr_pixel(grs_bitmap &cv_bitmap, const unsigned x, const unsigned y, const color_palette_index color)
{
	if (unlikely(x >= cv_bitmap.bm_w || y >= cv_bitmap.bm_h))
		return;
	gr_upixel(cv_bitmap, x, y, color);
}

#if !DXX_USE_OGL
#define gr_bm_upixel(C,B,X,Y,C2) gr_bm_upixel(B,X,Y,C2)
#endif
static inline void gr_bm_upixel(grs_canvas &canvas, grs_bitmap &bm, const uint_fast32_t x, const uint_fast32_t y, const color_palette_index color)
{
	switch (bm.get_type())
	{
#if DXX_USE_OGL
	case bm_mode::ogl:
		ogl_upixelc(canvas.cv_bitmap, bm.bm_x + x, bm.bm_y + y, color);
		return;
#endif
	case bm_mode::linear:
		bm.get_bitmap_data()[bm.bm_rowsize*y+x] = color;
		return;
	}
}

void gr_bm_pixel(grs_canvas &canvas, grs_bitmap &bm, const uint_fast32_t x, const uint_fast32_t y, const color_palette_index color)
{
	if (unlikely(x >= bm.bm_w || y >= bm.bm_h))
		return;
	gr_bm_upixel(canvas, bm, x, y, color);
}

}
