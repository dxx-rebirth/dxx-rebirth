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

#include <algorithm>
#include "u_mem.h"


#include "gr.h"
#include "grdef.h"

namespace dcx {

namespace {

static void gr_ubox0(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const color_palette_index color)
{
	int d;
	const auto rowsize = canvas.cv_bitmap.bm_rowsize;
	const auto ptr1 = &canvas.cv_bitmap.get_bitmap_data()[rowsize * top + left];
	auto ptr2 = ptr1;
	d = right - left;

	std::fill_n(ptr1 + 1, (right - left) - 1, color);
	for (uint_fast32_t i = bot - top + 1; i--;)
	{
		ptr2[0] = color;
		ptr2[d] = color;
		ptr2 += rowsize;
	}
	std::fill_n(ptr2 + 1, (right - left) - 1, color);
}

static void gr_ubox12(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const color_palette_index color)
{
	gr_uline(canvas, i2f(left), i2f(top), i2f(right), i2f(top), color);
	gr_uline(canvas, i2f(right), i2f(top), i2f(right), i2f(bot), color);
	gr_uline(canvas, i2f(left), i2f(top), i2f(left), i2f(bot), color);
	gr_uline(canvas, i2f(left), i2f(bot), i2f(right), i2f(bot), color);
}

#if DXX_USE_EDITOR
static void gr_box0(grs_canvas &canvas, const uint_fast32_t left, const uint_fast32_t top, uint_fast32_t right, uint_fast32_t bot, const color_palette_index color)
{
	const auto maxy = canvas.cv_bitmap.bm_h - 1;
	if (top > maxy)
		return;
	const auto maxx = canvas.cv_bitmap.bm_w - 1;
	if (left > maxx)
		return;
	if (bot > maxy)
		bot = maxy;
	if (right > maxx)
		right = maxx;
	gr_ubox0(canvas, left, top, right, bot, color);
}

static void gr_box12(grs_canvas &canvas, const uint_fast32_t left, const uint_fast32_t top, uint_fast32_t right, uint_fast32_t bot, const color_palette_index color)
{
	const auto maxy = canvas.cv_bitmap.bm_h - 1;
	if (top > maxy)
		return;
	const auto maxx = canvas.cv_bitmap.bm_w - 1;
	if (left > maxx)
		return;
	if (bot > maxy)
		bot = maxy;
	if (right > maxx)
		right = maxx;
	gr_ubox12(canvas, left, top, right, bot, color);
}
#endif

}

void gr_ubox(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const color_palette_index color)
{
	if (canvas.cv_bitmap.get_type() == bm_mode::linear)
		gr_ubox0(canvas, left, top, right, bot, color);
    else
		gr_ubox12(canvas, left, top, right, bot, color);
}

#if DXX_USE_EDITOR
void gr_box(grs_canvas &canvas, const uint_fast32_t left, const uint_fast32_t top, const uint_fast32_t right, const uint_fast32_t bot, const color_palette_index color)
{
	if (canvas.cv_bitmap.get_type() == bm_mode::linear)
		gr_box0(canvas, left, top, right, bot, color);
	else
		gr_box12(canvas, left, top, right, bot, color);
}
#endif

}
