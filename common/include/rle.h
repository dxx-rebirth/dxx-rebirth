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
 * Protypes for rle functions.
 *
 */

#ifndef _RLE_H
#define _RLE_H

#include "pstypes.h"
#include "gr.h"

#ifdef __cplusplus
#include <cstdint>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-begin.h"

struct rle_position_t
{
	const uint8_t *src;
	uint8_t *dst;
};

static inline const uint8_t *end(const grs_bitmap &b)
{
	return &b.bm_data[b.bm_h * b.bm_w];
}

static inline uint8_t *end(grs_bitmap &b)
{
	return &b.get_bitmap_data()[b.bm_h * b.bm_w];
}

template <typename T1, typename T2>
static inline rle_position_t rle_begin(const T1 &src, T2 &dst)
{
	return {begin(src), begin(dst)};
}

template <typename T1, typename T2>
static inline rle_position_t rle_end(const T1 &src, T2 &dst)
{
	return {end(src), end(dst)};
}

namespace dcx {
rle_position_t gr_rle_decode(rle_position_t b, const rle_position_t e);
int gr_bitmap_rle_compress(grs_bitmap &bmp);
void gr_rle_expand_scanline_masked(uint8_t *dest, const uint8_t *src, int x1, int x2);
void gr_rle_expand_scanline(uint8_t *dest, const uint8_t *src, int x1, int x2);
grs_bitmap *_rle_expand_texture(const grs_bitmap &bmp);
static inline const grs_bitmap *rle_expand_texture(const grs_bitmap &bmp) __attribute_warn_unused_result;
static inline const grs_bitmap *rle_expand_texture(const grs_bitmap &bmp)
{
	if (bmp.bm_flags & BM_FLAG_RLE)
		return _rle_expand_texture(bmp);
	return &bmp;
}
void rle_cache_close();
void rle_cache_flush();
void rle_swap_0_255(grs_bitmap &bmp);
void rle_remap(grs_bitmap &bmp, array<color_t, 256> &colormap);
#if !DXX_USE_OGL
#define gr_rle_expand_scanline_generic(C,D,DX,DY,S,X1,X2) gr_rle_expand_scanline_generic(D,DX,DY,S,X1,X2)
#endif
void gr_rle_expand_scanline_generic(grs_canvas &, grs_bitmap &dest, int dx, int dy, const ubyte *src, int x1, int x2 );
}

#endif

#endif
