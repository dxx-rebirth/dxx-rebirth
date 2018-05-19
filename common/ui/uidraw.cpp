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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"

namespace dcx {

void Hline(grs_canvas &canvas, const fix x1, const fix x2, const fix y, const uint8_t color)
{
	gr_uline(canvas, i2f(x1), i2f(y), i2f(x2), i2f(y), color);
}

void Vline(grs_canvas &canvas, const fix y1, const fix y2, const fix x, const uint8_t color)
{
	gr_uline(canvas, i2f(x), i2f(y1), i2f(x), i2f(y2), color);
}

void ui_string_centered(grs_canvas &canvas, const unsigned x, const unsigned y, const char *const s)
{
	int height, width;

	gr_get_string_size(*canvas.cv_font, s, &width, &height, nullptr);

	//baseline = height-grd_curcanv->cv_font->ft_baseline;

	gr_ustring(canvas, *canvas.cv_font, x - ((width - 1) / 2), y - ((height - 1) / 2), s);
}


void ui_draw_shad(grs_canvas &canvas, const unsigned x1, const unsigned y1, const unsigned x2, const unsigned y2, const unsigned c1, const unsigned c2)
{
	Hline(canvas, x1, x2 - 1, y1, c1);
	Vline(canvas, y1 + 1, y2, x1, c1);

	Hline(canvas, x1 + 1, x2, y2, c2);
	Vline(canvas, y1, y2 - 1, x2, c2);
}

void ui_draw_frame(grs_canvas &canvas, const unsigned x1, const unsigned y1, const unsigned x2, const unsigned y2)
{
	const auto cbright = CBRIGHT;
	const auto cgrey = CGREY;
	const auto cwhite = CWHITE;
	ui_draw_shad(canvas, x1 + 0, y1 + 0, x2 - 0, y2 - 0, cbright, cgrey);
	ui_draw_shad(canvas, x1 + 1, y1 + 1, x2 - 1, y2 - 1, cbright, cgrey);

	ui_draw_shad(canvas, x1 + 2, y1 + 2, x2 - 2, y2 - 2, cwhite, cwhite);
	ui_draw_shad(canvas, x1 + 3, y1 + 3, x2 - 3, y2 - 3, cwhite, cwhite);
	ui_draw_shad(canvas, x1 + 4, y1 + 4, x2 - 4, y2 - 4, cwhite, cwhite);
	ui_draw_shad(canvas, x1 + 5, y1 + 5, x2 - 5, y2 - 5, cwhite, cwhite);

	ui_draw_shad(canvas, x1 + 6, y1 + 6, x2 - 6, y2 - 6, cgrey, cbright);
	ui_draw_shad(canvas, x1 + 7, y1 + 7, x2 - 7, y2 - 7, cgrey, cbright);
}

void ui_draw_box_out(grs_canvas &canvas, const unsigned x1, const unsigned y1, const unsigned x2, const unsigned y2)
{
	const uint8_t color = CWHITE;
	const auto cbright = CBRIGHT;
	const auto cgrey = CGREY;
	gr_urect(canvas, x1 + 2, y1 + 2, x2 - 2, y2 - 2, color);

	ui_draw_shad(canvas, x1 + 0, y1 + 0, x2 - 0, y2 - 0, cbright, cgrey);
	ui_draw_shad(canvas, x1 + 1, y1 + 1, x2 - 1, y2 - 1, cbright, cgrey);
}

void ui_draw_box_in(grs_canvas &canvas, const unsigned x1, const unsigned y1, const unsigned x2, const unsigned y2)
{
	const auto cbright = CBRIGHT;
	const auto cgrey = CGREY;
	ui_draw_shad(canvas, x1, y1, x2, y2, cgrey, cbright);
	ui_draw_shad(canvas, x1 + 1, y1 + 1, x2 - 1, y2 - 1, cgrey, cbright);
}

}
