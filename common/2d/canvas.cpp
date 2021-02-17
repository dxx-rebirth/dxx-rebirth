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

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"
#include "gr.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include <memory>

namespace dcx {

grs_canvas * grd_curcanv;    //active canvas
std::unique_ptr<grs_screen> grd_curscreen;  //active screen

grs_canvas_ptr gr_create_canvas(uint16_t w, uint16_t h)
{
	grs_canvas_ptr n = std::make_unique<grs_main_canvas>();
	unsigned char *pixdata;
	MALLOC(pixdata, unsigned char, MAX_BMP_SIZE(w, h));
	gr_init_canvas(*n.get(), pixdata, bm_mode::linear, w, h);
	return n;
}

grs_subcanvas_ptr gr_create_sub_canvas(grs_canvas &canv, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	auto n = std::make_unique<grs_subcanvas>();
	gr_init_sub_canvas(*n.get(), canv, x, y, w, h);
	return n;
}

void gr_init_canvas(grs_canvas &canv, unsigned char *const pixdata, const bm_mode pixtype, const uint16_t w, const uint16_t h)
{
	canv.cv_fade_level = GR_FADE_OFF;
	canv.cv_font = NULL;
	canv.cv_font_fg_color = 0;
	canv.cv_font_bg_color = 0;
	auto wreal = w;
	gr_init_bitmap(canv.cv_bitmap, pixtype, 0, 0, w, h, wreal, pixdata);
}

void gr_init_sub_canvas(grs_canvas &n, grs_canvas &src, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	n.cv_fade_level = src.cv_fade_level;
	n.cv_font = src.cv_font;
	n.cv_font_fg_color = src.cv_font_fg_color;
	n.cv_font_bg_color = src.cv_font_bg_color;

	gr_init_sub_bitmap (n.cv_bitmap, src.cv_bitmap, x, y, w, h);
}

grs_main_canvas::~grs_main_canvas()
{
	gr_free_bitmap_data(cv_bitmap);
}

#if DXX_DEBUG_CURRENT_CANVAS_ORIGIN > 0
/* `g_cc_file`, `g_cc_line` are write-only for the program and are meant
 * to be read back by a debugger.
 */
__attribute_used
static const char *g_cc_file[DXX_DEBUG_CURRENT_CANVAS_ORIGIN];
__attribute_used
static unsigned g_cc_line[DXX_DEBUG_CURRENT_CANVAS_ORIGIN];
static unsigned g_cc_which;
#endif

void (gr_set_default_canvas)(DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_DECL_VARS)
{
	(gr_set_current_canvas)(grd_curscreen->sc_canvas DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

void (gr_set_current_canvas)(grs_canvas &canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
#if DXX_DEBUG_CURRENT_CANVAS_ORIGIN > 0
	const auto which = g_cc_which;
	g_cc_which = (g_cc_which + 1) % DXX_DEBUG_CURRENT_CANVAS_ORIGIN;
	g_cc_file[which] = file;
	g_cc_line[which] = line;
#endif
	grd_curcanv = &canv;
}

void gr_set_current_canvas2(grs_canvas *canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	(gr_set_current_canvas_inline)(canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

void gr_clear_canvas(grs_canvas &canvas, const color_palette_index color)
{
	gr_rect(canvas, 0, 0, canvas.cv_bitmap.bm_w - 1, canvas.cv_bitmap.bm_h - 1, color);
}

void gr_settransblend(grs_canvas &canvas, const int fade_level, const gr_blend blend_func)
{
	canvas.cv_fade_level = fade_level;
#if DXX_USE_OGL
	ogl_set_blending(blend_func);
#endif
}

}
