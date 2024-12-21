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
#include <ranges>
#include <span>
#include "gr.h"
#include "grdef.h"

namespace dcx {

#if DXX_USE_OGL
namespace {
#endif
void gr_uscanline(grs_canvas &canvas, const unsigned x1, const unsigned x2, const unsigned y, const uint8_t color)
{
	switch(canvas.cv_bitmap.get_type())
		{
		case bm_mode::linear:
#if DXX_USE_OGL
		case bm_mode::ogl:
#endif
			{
				const auto count = x2 - x1 + 1;
				const std::span<uint8_t> data{&canvas.cv_bitmap.get_bitmap_data()[canvas.cv_bitmap.bm_rowsize * y + x1], count};
				if (const auto r{gr_fade_table.valid_index(canvas.cv_fade_level)})
					std::ranges::transform(data, data.begin(), [&t = gr_fade_table[*r]](const uint8_t c) { return t[c]; });
				else
					std::ranges::fill(data, color);
			}
			break;
		case bm_mode::ilbm:
		case bm_mode::rgb15:
			break;
		}
}
#if DXX_USE_OGL
}
#endif

void gr_scanline(grs_canvas &canvas, int x1, int x2, const unsigned y, const uint8_t color)
{
	const auto maxy = (canvas.cv_bitmap.bm_h - 1);
	if (y >= maxy)
		return;

	if (x2 < x1)
		std::swap(x1, x2);

	const auto maxx = (canvas.cv_bitmap.bm_w - 1);
	if (x1 > maxx) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > maxx) x2 = maxx;
	gr_uscanline(canvas, x1, x2, y, color);
}

}
