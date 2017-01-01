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

/*
 *
 * Routines for bitblt's.
 *
 */

#include <algorithm>
#include <utility>
#include <string.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "dxxerror.h"
#include "byteutil.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-array.h"
#include "compiler-exchange.h"

namespace dcx {

static int gr_bitblt_dest_step_shift = 0;

static void gr_bm_ubitblt00_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest);
#if !DXX_USE_OGL
static void gr_bm_ubitblt00m_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest);
static void gr_bm_ubitblt0x_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest);
#endif

#define gr_linear_movsd(S,D,L)	memcpy(D,S,L)

#if !DXX_USE_OGL
static void gr_linear_rep_movsdm(uint8_t *const dest, const uint8_t *const src, const uint_fast32_t num_pixels)
{
	auto predicate = [&](uint8_t s, uint8_t d) {
		return s == 255 ? d : s;
	};
	std::transform(src, src + num_pixels, dest, dest, predicate);
}
#endif

template <typename F>
static void gr_for_each_bitmap_line(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm, F f)
{
	const size_t src_width = bm.bm_w;
	const uintptr_t src_rowsize = bm.bm_rowsize;
	const uintptr_t dest_rowsize = canvas.cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	auto dest = &(canvas.cv_bitmap.get_bitmap_data()[ dest_rowsize*y+x ]);
	auto src = bm.get_bitmap_data();
	for (uint_fast32_t y1 = bm.bm_h; y1 --;)
	{
		f(dest, src, src_width);
		src += src_rowsize;
		dest+= dest_rowsize;
	}
}

static void gr_ubitmap00(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	gr_for_each_bitmap_line(canvas, x, y, bm, memcpy);
}

#if !DXX_USE_OGL
static void gr_ubitmap00m(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	gr_for_each_bitmap_line(canvas, x, y, bm, gr_linear_rep_movsdm);
}
#endif

template <typename F>
static inline void gr_for_each_bitmap_byte(grs_canvas &canvas, const uint_fast32_t bx, const uint_fast32_t by, const grs_bitmap &bm, F f)
{
	auto src = bm.bm_data;
	const auto ey = by + bm.bm_h;
	const auto ex = bx + bm.bm_w;
	for (auto iy = by; iy != ey; ++iy)
		for (auto ix = bx; ix != ex; ++ix)
			f(canvas, src++, ix, iy);
}

static void gr_ubitmap012(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	const auto a = [](grs_canvas &cv, const uint8_t *const src, const uint_fast32_t px, const uint_fast32_t py) {
		const auto color = *src;
		gr_upixel(cv, px, py, color);
	};
	gr_for_each_bitmap_byte(canvas, x, y, bm, a);
}

#if !DXX_USE_OGL
static void gr_ubitmap012m(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	const auto a = [](grs_canvas &cv, const uint8_t *const src, const uint_fast32_t px, const uint_fast32_t py) {
		const uint8_t c = *src;
		if (c != 255)
		{
			gr_upixel(cv, px, py, c);
		}
	};
	gr_for_each_bitmap_byte(canvas, x, y, bm, a);
}
#endif

static void gr_ubitmapGENERIC(grs_canvas &canvas, unsigned x, unsigned y, const grs_bitmap &bm)
{
	const uint_fast32_t bm_h = bm.bm_h;
	const uint_fast32_t bm_w = bm.bm_w;
	for (uint_fast32_t y1 = 0; y1 != bm_h; ++y1)
	{
		for (uint_fast32_t x1 = 0; x1 != bm_w; ++x1)
		{
			const auto color = gr_gpixel(bm, x1, y1);
			gr_upixel(canvas, x + x1, y + y1, color);
		}
	}
}

#if !DXX_USE_OGL
static void gr_ubitmapGENERICm(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	const uint_fast32_t bm_h = bm.bm_h;
	const uint_fast32_t bm_w = bm.bm_w;
	for (uint_fast32_t y1 = 0; y1 != bm_h; ++y1)
	{
		for (uint_fast32_t x1 = 0; x1 != bm_w; ++x1)
		{
			const auto c = gr_gpixel(bm,x1,y1);
			if ( c != 255 )	{
				gr_upixel(canvas, x + x1, y + y1, c);
			}
		}
	}
}
#endif

void gr_ubitmap(grs_canvas &canvas, grs_bitmap &bm)
{
	const unsigned x = 0;
	const unsigned y = 0;

	const auto source = bm.get_type();
	const auto dest = canvas.cv_bitmap.get_type();

	if (source==bm_mode::linear) {
		switch( dest )
		{
		case bm_mode::linear:
			if ( bm.bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00_rle(bm.bm_w, bm.bm_h, x, y, 0, 0, bm, canvas.cv_bitmap);
			else
				gr_ubitmap00(canvas, x, y, bm);
			return;
#if DXX_USE_OGL
		case bm_mode::ogl:
			ogl_ubitmapm_cs(canvas, x, y, -1, -1, bm, ogl_colors::white, F1_0);
			return;
#endif
		default:
			gr_ubitmap012(canvas, x, y, bm);
			return;
		}
	} else  {
		gr_ubitmapGENERIC(canvas, x, y, bm);
	}
}

#if !DXX_USE_OGL
void gr_ubitmapm(grs_canvas &canvas, const unsigned x, const unsigned y, grs_bitmap &bm)
{
	const auto source = bm.get_type();
	if (source==bm_mode::linear) {
		switch(canvas.cv_bitmap.get_type())
		{
		case bm_mode::linear:
			if ( bm.bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00m_rle(bm.bm_w, bm.bm_h, x, y, 0, 0, bm, canvas.cv_bitmap);
			else
				gr_ubitmap00m(canvas, x, y, bm);
			return;
		default:
			gr_ubitmap012m(canvas, x, y, bm);
			return;
		}
	} else  {
		gr_ubitmapGENERICm(canvas, x, y, bm);
	}
}

// From Linear to Linear
static void gr_bm_ubitblt00(unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest)
{
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	auto sbits = &src.get_bitmap_data()[(src.bm_rowsize * sy) + sx];
	auto dbits = &dest.get_bitmap_data()[(dest.bm_rowsize * dy) + dx];
	auto dstep = dest.bm_rowsize << gr_bitblt_dest_step_shift;
	// No interlacing, copy the whole buffer.
	for (uint_fast32_t i = h; i --;)
	{
		gr_linear_movsd( sbits, dbits, w );
		//memcpy(dbits, sbits, w);
		sbits += src.bm_rowsize;
		dbits += dstep;
	    }
}

// From Linear to Linear Masked
static void gr_bm_ubitblt00m(const unsigned w, const uint_fast32_t h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest)
{
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	auto sbits = &src.get_bitmap_data()[(src.bm_rowsize * sy) + sx];
	auto dbits = &dest.get_bitmap_data()[(dest.bm_rowsize * dy) + dx];

	// No interlacing, copy the whole buffer.

	{
		for (auto i = h; i; --i)
		{
			gr_linear_rep_movsdm(dbits, sbits, w);
			sbits += src.bm_rowsize;
			dbits += dest.bm_rowsize;
		}
	}
}

void gr_bm_ubitblt(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest)
{
	if (src.get_type() == bm_mode::linear && dest.get_type() == bm_mode::linear)
	{
		if ( src.bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00_rle( w, h, dx, dy, sx, sy, src, dest );
		else
			gr_bm_ubitblt00( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

#if DXX_USE_OGL
	if (src.get_type() == bm_mode::linear && dest.get_type() == bm_mode::ogl)
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if (src.get_type() == bm_mode::ogl && dest.get_type() == bm_mode::linear)
	{
		return;
	}
	if (src.get_type() == bm_mode::ogl && dest.get_type() == bm_mode::ogl)
	{
		return;
	}
#endif

	if ((src.bm_flags & BM_FLAG_RLE) && src.get_type() == bm_mode::linear)
	{
		gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest);
	 	return;
	}

	for (uint_fast32_t y1 = 0; y1 != h; ++y1)
		for (uint_fast32_t x1 = 0; x1 != w; ++x1)
			gr_bm_pixel(dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1) );
}
#endif

// Clipped bitmap ...
void gr_bitmap(grs_canvas &canvas, const unsigned x, const unsigned y, grs_bitmap &bm)
{
	int dx1=x, dx2=x+bm.bm_w-1;
	int dy1=y, dy2=y+bm.bm_h-1;

	if (dx1 >= canvas.cv_bitmap.bm_w || dx2 < 0)
		return;
	if (dy1 >= canvas.cv_bitmap.bm_h || dy2 < 0)
		return;
	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
#if DXX_USE_OGL
	ogl_ubitmapm_cs(canvas, x, y, 0, 0, bm, ogl_colors::white, F1_0);
#else
	int sx = 0, sy = 0;
	if ( dx1 < 0 )
	{
		sx = -dx1;
		dx1 = 0;
	}
	if ( dy1 < 0 )
	{
		sy = -dy1;
		dy1 = 0;
	}
	if (dx2 >= canvas.cv_bitmap.bm_w)
		dx2 = canvas.cv_bitmap.bm_w - 1;
	if (dy2 >= canvas.cv_bitmap.bm_h)
		dy2 = canvas.cv_bitmap.bm_h - 1;

	gr_bm_ubitblt(dx2 - dx1 + 1,dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, canvas.cv_bitmap);
#endif
}

#if !DXX_USE_OGL
void gr_bitmapm(grs_canvas &canvas, const unsigned x, const unsigned y, const grs_bitmap &bm)
{
	int dx1=x, dx2=x+bm.bm_w-1;
	int dy1=y, dy2=y+bm.bm_h-1;
	int sx=0, sy=0;

	if (dx1 >= canvas.cv_bitmap.bm_w || dx2 < 0)
		return;
	if (dy1 >= canvas.cv_bitmap.bm_h || dy2 < 0)
		return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if (dx2 >= canvas.cv_bitmap.bm_w)
		dx2 = canvas.cv_bitmap.bm_w - 1;
	if (dy2 >= canvas.cv_bitmap.bm_h)
		dy2 = canvas.cv_bitmap.bm_h - 1;

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	if (bm.get_type() == bm_mode::linear && canvas.cv_bitmap.get_type() == bm_mode::linear)
	{
		if ( bm.bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00m_rle(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, canvas.cv_bitmap );
		else
			gr_bm_ubitblt00m(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, canvas.cv_bitmap );
		return;
	}
	gr_bm_ubitbltm(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, canvas.cv_bitmap);
}

void gr_bm_ubitbltm(unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest)
{
	ubyte c;

#if DXX_USE_OGL
	if (src.get_type() == bm_mode::linear && dest.get_type() == bm_mode::ogl)
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if (src.get_type() == bm_mode::ogl && dest.get_type() == bm_mode::linear)
	{
		return;
	}
	if (src.get_type() == bm_mode::ogl && dest.get_type() == bm_mode::ogl)
	{
		return;
	}
#endif
	for (uint_fast32_t y1 = 0; y1 != h; ++y1)
		for (uint_fast32_t x1 = 0; x1 != w; ++x1)
			if ((c=gr_gpixel(src,sx+x1,sy+y1))!=255)
				gr_bm_pixel(dest, dx+x1, dy+y1,c  );
}
#endif

static void gr_bm_ubitblt00_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest)
{
	int data_offset;

	data_offset = 1;
	if (src.bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;
	auto sbits = &src.get_bitmap_data()[4 + (src.bm_h*data_offset)];
	for (uint_fast32_t i = 0; i != sy; ++i)
		sbits += src.bm_data[4+(i*data_offset)];
	auto dbits = &dest.get_bitmap_data()[(dest.bm_rowsize * dy) + dx];
	// No interlacing, copy the whole buffer.
	for (uint_fast32_t i = 0; i != h; ++i)
	{
		gr_rle_expand_scanline( dbits, sbits, sx, sx+w-1 );
		if ( src.bm_flags & BM_FLAG_RLE_BIG )
			sbits += GET_INTEL_SHORT(&src.bm_data[4 + ((i + sy) * data_offset)]);
		else
			sbits += static_cast<int>(src.bm_data[4+i+sy]);
		dbits += dest.bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

#if !DXX_USE_OGL
static void gr_bm_ubitblt00m_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest)
{
	int data_offset;

	data_offset = 1;
	if (src.bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;
	auto sbits = &src.get_bitmap_data()[4 + (src.bm_h*data_offset)];
	for (uint_fast32_t i = 0; i != sy; ++i)
		sbits += src.bm_data[4+(i*data_offset)];
	auto dbits = &dest.get_bitmap_data()[(dest.bm_rowsize * dy) + dx];
	// No interlacing, copy the whole buffer.
	for (uint_fast32_t i = 0; i != h; ++i)
	{
		gr_rle_expand_scanline_masked( dbits, sbits, sx, sx+w-1 );
		if ( src.bm_flags & BM_FLAG_RLE_BIG )
			sbits += GET_INTEL_SHORT(&src.bm_data[4 + ((i + sy) * data_offset)]);
		else
			sbits += static_cast<int>(src.bm_data[4+i+sy]);
		dbits += dest.bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

// in rle.c

static void gr_bm_ubitblt0x_rle(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest)
{
	int data_offset;
	data_offset = 1;
	if (src.bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;
	auto sbits = &src.bm_data[4 + (src.bm_h*data_offset)];
	for (uint_fast32_t i = 0; i != sy; ++i)
		sbits += src.bm_data[4 + (i * data_offset)];

	for (uint_fast32_t y1 = 0; y1 != h; ++y1)
	{
		gr_rle_expand_scanline_generic(dest, dx, dy+y1, sbits, sx, sx+w-1);
		if ( src.bm_flags & BM_FLAG_RLE_BIG )
			sbits += GET_INTEL_SHORT(&src.bm_data[4 + ((y1 + sy) * data_offset)]);
		else
			sbits += static_cast<int>(src.bm_data[4+y1+sy]);
	}
}
#endif

// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

static void scale_line(const uint8_t *in, uint8_t *out, const uint_fast32_t ilen, const uint_fast32_t olen)
{
	uint_fast32_t a = olen / ilen, b = olen % ilen, c = 0;
	for (uint8_t *const end = out + olen; out != end;)
	{
		uint_fast32_t i = a;
		c += b;
		if(c >= ilen) {
			c -= ilen;
			++i;
		}
		auto e = out + i;
		std::fill(exchange(out, e), e, *in++);
	}
}

static void gr_bitmap_scale_to(const grs_bitmap &src, grs_bitmap &dst)
{
	auto s = src.get_bitmap_data();
	auto d = dst.get_bitmap_data();
	int h = src.bm_h;
	int a = dst.bm_h/h, b = dst.bm_h%h;
	int c = 0, i;

	for (uint_fast32_t y = src.bm_h; y; --y) {
		i = a;
		c += b;
		if(c >= h) {
			c -= h;
			goto inside;
		}
		while(--i>=0) {
inside:
			scale_line(s, d, src.bm_w, dst.bm_w);
			d += dst.bm_rowsize;
		}
		s += src.bm_rowsize;
	}
}

void show_fullscr(grs_bitmap &bm)
{
	auto &scr = grd_curcanv->cv_bitmap;
#if DXX_USE_OGL
	if (bm.get_type() == bm_mode::linear && scr.get_type() == bm_mode::ogl &&
		bm.bm_w <= grd_curscreen->get_screen_width() && bm.bm_h <= grd_curscreen->get_screen_height()) // only scale with OGL if bitmap is not bigger than screen size
	{
		ogl_ubitmapm_cs(*grd_curcanv, 0, 0, -1, -1, bm, ogl_colors::white, F1_0);//use opengl to scale, faster and saves ram. -MPM
		return;
	}
#endif
	if (scr.get_type() != bm_mode::linear)
	{
		grs_bitmap_ptr p = gr_create_bitmap(scr.bm_w, scr.bm_h);
		auto &tmp = *p.get();
		gr_bitmap_scale_to(bm, tmp);
		gr_bitmap(*grd_curcanv, 0, 0, tmp);
		return;
	}
	gr_bitmap_scale_to(bm, scr);
}

// Find transparent area in bitmap
void gr_bitblt_find_transparent_area(const grs_bitmap &bm, unsigned &minx, unsigned &miny, unsigned &maxx, unsigned &maxy)
{
	using std::advance;
	using std::min;
	using std::max;

	if (!(bm.bm_flags&BM_FLAG_TRANSPARENT))
		return;

	minx = bm.bm_w - 1;
	maxx = 0;
	miny = bm.bm_h - 1;
	maxy = 0;

	unsigned i = 0, count = 0;
	auto check = [&](unsigned x, unsigned y, ubyte c) {
		if (c == TRANSPARENCY_COLOR) {				// don't look for transparancy color here.
			count++;
			minx = min(x, minx);
			miny = min(y, miny);
			maxx = max(x, maxx);
			maxy = max(y, maxy);
		}
	};
	// decode the bitmap
	const uint_fast32_t bm_h = bm.bm_h;
	const uint_fast32_t bm_w = bm.bm_w;
	if (bm.bm_flags & BM_FLAG_RLE){
		unsigned data_offset;

		data_offset = 1;
		if (bm.bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;
		auto sbits = &bm.get_bitmap_data()[4 + (bm.bm_h * data_offset)];
		for (uint_fast32_t y = 0; y != bm_h; ++y)
		{
			array<ubyte, 4096> buf;
			gr_rle_decode({sbits, begin(buf)}, rle_end(bm, buf));
			advance(sbits, bm.bm_data[4+i] | (data_offset == 2 ? static_cast<unsigned>(bm.bm_data[5+i]) << 8 : 0));
			i += data_offset;
			for (uint_fast32_t x = 0; x != bm_w; ++x)
				check(x, y, buf[x]);
		}
	}
	else
	{
		for (uint_fast32_t y = 0; y != bm_h; ++y)
			for (uint_fast32_t x = 0; x != bm_w; ++x)
				check(x, y, bm.bm_data[i++]);
	}
	Assert (count);
}

}
