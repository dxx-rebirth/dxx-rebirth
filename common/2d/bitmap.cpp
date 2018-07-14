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
 * Graphical routines for manipulating grs_bitmaps.
 *
 */

#include <algorithm>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "bitmap.h"

#include "compiler-make_unique.h"

namespace dcx {

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
static grs_bitmap_ptr gr_create_bitmap_raw(uint16_t w, uint16_t h, unsigned char * raw_data);

void gr_set_bitmap_data(grs_bitmap &bm, const uint8_t *data)
{
#if DXX_USE_OGL
	ogl_freebmtexture(bm);
#endif
	bm.bm_data = data;
}

grs_bitmap_ptr gr_create_bitmap(uint16_t w, uint16_t h )
{
	unsigned char *d;
	MALLOC(d, unsigned char, MAX_BMP_SIZE(w, h));
	return gr_create_bitmap_raw (w, h, d);
}

grs_bitmap_ptr gr_create_bitmap_raw(uint16_t w, uint16_t h, unsigned char * raw_data )
{
	auto n = make_unique<grs_main_bitmap>();
	gr_init_bitmap(*n.get(), bm_mode::linear, 0, 0, w, h, w, raw_data);
	return n;
}

void gr_init_bitmap(grs_bitmap &bm, const bm_mode mode, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint16_t bytesperline, const uint8_t *const data) // TODO: virtualize
{
	bm.bm_x = x;
	bm.bm_y = y;
	bm.bm_w = w;
	bm.bm_h = h;
	bm.set_flags(0);
	bm.set_type(mode);
	bm.bm_rowsize = bytesperline;

	bm.bm_data = nullptr;
#if DXX_USE_OGL
	bm.bm_parent = nullptr;
	bm.gltexture = nullptr;
#endif
	gr_set_bitmap_data(bm, data);
}

void gr_init_bitmap_alloc(grs_main_bitmap &bm, const bm_mode mode, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint16_t bytesperline)
{
	unsigned char *d;
	MALLOC(d, unsigned char, MAX_BMP_SIZE(w, h));
	gr_init_bitmap(bm, mode, x, y, w, h, bytesperline, d);
}

grs_main_bitmap::grs_main_bitmap()
{
	static_cast<grs_bitmap &>(*this) = {};
}

grs_subbitmap_ptr gr_create_sub_bitmap(grs_bitmap &bm, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	auto n = make_unique<grs_subbitmap>();
	gr_init_sub_bitmap(*n.get(), bm, x, y, w, h);
	return n;
}

void gr_free_bitmap_data (grs_bitmap &bm) // TODO: virtulize
{
	if (auto &d = bm.bm_mdata)
		d_free(d);
#if DXX_USE_OGL
	ogl_freebmtexture(bm);
#endif
}

void gr_init_sub_bitmap (grs_bitmap &bm, grs_bitmap &bmParent, uint16_t x, uint16_t y, uint16_t w, uint16_t h )	// TODO: virtualize
{
	uint32_t subx = x + bmParent.bm_x;
	uint32_t suby = y + bmParent.bm_y;
	if (subx != (bm.bm_x = static_cast<uint16_t>(subx)) ||
		suby != (bm.bm_y = static_cast<uint16_t>(suby)))
		throw std::overflow_error("offset overflow");
	bm.bm_w = w;
	bm.bm_h = h;
	bm.set_flags(bmParent.get_flags());
	bm.set_type(bmParent.get_type());
	bm.bm_rowsize = bmParent.bm_rowsize;

#if DXX_USE_OGL
	bm.gltexture = bmParent.gltexture;
#endif
	bm.bm_parent = &bmParent;
	bm.bm_data = &bmParent.bm_data[static_cast<uint32_t>((y*bmParent.bm_rowsize)+x)];
}

void decode_data(ubyte *data, uint_fast32_t num_pixels, array<color_t, 256> &colormap, array<bool, 256> &used)
{
	const auto a = [&](uint8_t mapped) {
		return used[mapped] = true, colormap[mapped];
	};
	std::transform(data, data + num_pixels, data, a);
}

static void gr_set_super_transparent(grs_bitmap &bm, bool bOpaque)
{
	bm.set_flag_mask(!bOpaque, BM_FLAG_SUPER_TRANSPARENT);
}

void build_colormap_good(const palette_array_t &palette, array<color_t, 256> &colormap)
{
	const auto a = [](const rgb_t &p) {
		return gr_find_closest_color(p.r, p.g, p.b);
	};
	std::transform(palette.begin(), palette.end(), colormap.begin(), a);
}

void gr_remap_bitmap_good(grs_bitmap &bmp, palette_array_t &palette, uint_fast32_t transparent_color, uint_fast32_t super_transparent_color)
{
	array<uint8_t, 256> colormap;
	build_colormap_good(palette, colormap);

	if (super_transparent_color < colormap.size())
		colormap[super_transparent_color] = 254;

	if (transparent_color < colormap.size())
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	array<bool, 256> freq{};
	if (bmp.bm_w == bmp.bm_rowsize)
		decode_data(bmp.get_bitmap_data(), bmp.bm_w * bmp.bm_h, colormap, freq );
	else {
		auto p = bmp.get_bitmap_data();
		for (uint_fast32_t y = bmp.bm_h; y--; p += bmp.bm_rowsize)
			decode_data(p, bmp.bm_w, colormap, freq );
	}

	if (transparent_color < freq.size() && freq[transparent_color])
		gr_set_transparent(bmp, 1);

	if (super_transparent_color < freq.size() && freq[super_transparent_color])
		gr_set_super_transparent(bmp, 1);
}

}
