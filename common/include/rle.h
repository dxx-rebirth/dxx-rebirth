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

#pragma once

#include "pstypes.h"
#include "gr.h"

#include <cstdint>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-poison.h"
#include <iterator>

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
static inline rle_position_t rle_end(const T1 &src, T2 &dst)
{
	return {end(src), end(dst)};
}

namespace dcx {
uint8_t *gr_rle_decode(const uint8_t *sb, uint8_t *db, rle_position_t e);
int gr_bitmap_rle_compress(grs_bitmap &bmp);
#if !DXX_USE_OGL
void gr_rle_expand_scanline_masked(uint8_t *dest, const uint8_t *src, int x1, int x2);
#endif
void gr_rle_expand_scanline(uint8_t *dest, const uint8_t *src, int x1, int x2);
grs_bitmap *_rle_expand_texture(const grs_bitmap &bmp);
static inline const grs_bitmap *rle_expand_texture(const grs_bitmap &bmp) __attribute_warn_unused_result;
static inline const grs_bitmap *rle_expand_texture(const grs_bitmap &bmp)
{
	if (bmp.get_flag_mask(BM_FLAG_RLE))
		return _rle_expand_texture(bmp);
	return &bmp;
}
void rle_cache_close();
void rle_cache_flush();
void rle_swap_0_255(grs_bitmap &bmp);
void rle_remap(grs_bitmap &bmp, std::array<color_t, 256> &colormap);
#if !DXX_USE_OGL
#define gr_rle_expand_scanline_generic(C,D,DX,DY,S,X1,X2) gr_rle_expand_scanline_generic(D,DX,DY,S,X1,X2)
void gr_rle_expand_scanline_generic(grs_canvas &, grs_bitmap &dest, int dx, int dy, const ubyte *src, int x1, int x2 );
#endif

class bm_rle_expand_range
{
	uint8_t *iter_dbits;
	uint8_t *const end_dbits;
public:
	bm_rle_expand_range(uint8_t *const i, uint8_t *const e) :
		iter_dbits(i), end_dbits(e)
	{
	}
	template <std::size_t N>
		bm_rle_expand_range(std::array<uint8_t, N> &a) :
			iter_dbits(a.data()), end_dbits(std::next(iter_dbits, N))
	{
	}
	uint8_t *get_begin_dbits() const
	{
		return iter_dbits;
	}
	uint8_t *get_end_dbits() const
	{
		return end_dbits;
	}
	void consume_dbits(const unsigned w)
	{
		iter_dbits += w;
	}
};

class bm_rle_src_stride
{
	/* Width of an individual element of the table of row lengths.  This
	 * is sizeof(uint8_t) if the bitmap is not RLE_BIG, or is
	 * sizeof(uint16_t) otherwise.
	 */
	const unsigned src_bit_stride_size;
	/* Bitmask used for filtering the table load.  To minimize
	 * branching, the code always loads two bytes from the table of row
	 * lengths.  If the bitmap is not RLE_BIG, then `src_bit_load_mask`
	 * will be 0xff and will be used to mask out the high byte from the
	 * load.  Otherwise, `src_bit_load_mask` will be 0xffff and the mask
	 * operation will leave the loaded value unchanged.
	 */
	const unsigned src_bit_load_mask;
protected:
	/* Pointer to the table of row lengths.  The table is uint8_t[] if
	 * the bitmap is not RLE_BIG, or is uint16_t[] otherwise.  The table
	 * is not required to be aligned.
	 */
	const uint8_t *ptr_src_bit_lengths;
	/* Pointer to the table of RLE-encoded bitmap elements.
	 */
	const uint8_t *src_bits;
public:
	bm_rle_src_stride(const grs_bitmap &src, const unsigned rle_big) :
		/* Jump threading should collapse the separate ?: uses */
		src_bit_stride_size(rle_big ? sizeof(uint16_t) : sizeof(uint8_t)),
		src_bit_load_mask(rle_big ? 0xffff : 0xff),
		ptr_src_bit_lengths(&src.bm_data[4]),
		src_bits(&ptr_src_bit_lengths[rle_big ? src.bm_h * 2 : src.bm_h])
	{
	}
	void advance_src_bits();
};

class bm_rle_expand : bm_rle_src_stride
{
	/* Pointer to the first byte that is not part of the table of row
	 * lengths.  When `ptr_src_bit_lengths` == `end_src_bit_lengths`, no
	 * further rows are available.
	 */
	const uint8_t *const end_src_bit_lengths;
	/* Pointer to the first byte that is not part of the bitmap data.
	 * When `end_src_bm` == `src_bits`, no further bitmap elements are
	 * available.
	 */
	const uint8_t *const end_src_bm;
public:
	enum step_result
	{
		/* A row decoded successfully.  Act on the decoded buffer as
		 * necessary, then call this class again.
		 *
		 * This result is returned even if the returned row is the last
		 * row.  The first call after the last row will return
		 * src_exhausted, which is the trigger to stop calling the
		 * class.
		 */
		again,
		/* The source was exhausted before any work was done.  No data
		 * is available.  No further calls should be made with this
		 * source.
		 */
		src_exhausted,
		/* The destination was exhausted before decoding completed.  The
		 * source is left at the beginning of the row which failed to
		 * decode.  The caller may try again with a larger destination
		 * buffer or may abandon the attempt.  Further calls with the
		 * same source and destination will continue to return
		 * `dst_exhausted`.
		 */
		dst_exhausted,
	};
	bm_rle_expand(const grs_bitmap &src) :
		bm_rle_src_stride(src, src.get_flag_mask(BM_FLAG_RLE_BIG)),
		end_src_bit_lengths(src_bits),
		end_src_bm(end(src))
	{
	}
	template <typename T>
		/* Decode one row of the bitmap, then return control to the
		 * caller.  If the return value is `again`, then the caller
		 * should perform any per-row processing, then call step()
		 * again.  If the return value is not `again`, then the
		 * destination buffer is undefined and the caller should not
		 * access it or call step().
		 *
		 * `const T &` to ensure that t is only modified by the caller
		 * and that the caller does not accidentally provide an
		 * implementation of `get_begin_dbits` that moves the
		 * destination pointer.
		 */
		step_result step(const T &t)
		{
			/* Poison the memory first, so that it is undefined even if
			 * the source is exhausted.
			 */
			const auto b = t.get_begin_dbits();
			const auto e = t.get_end_dbits();
			DXX_MAKE_MEM_UNDEFINED(b, e);
			/* Check for source exhaustion, so that empty bitmaps are
			 * not read at all.  This allows callers to treat
			 * src_exhausted as a definitive end-of-record with no data
			 * available.
			 */
			if (ptr_src_bit_lengths == end_src_bit_lengths)
				return src_exhausted;
			return step_internal(b, e);
		}
	template <typename T>
		/* Decode until source or destination is exhausted.  The
		 * destination position is updated automatically as each row is
		 * decoded.  There is no opportunity for callers to perform
		 * per-row processing.  Callers should call step() directly if
		 * per-row processing is required.
		 *
		 * `T &&` since some callers may not care about the state of `t`
		 * afterward; `T &&` lets them pass an anonymous temporary.
		 */
		bool loop(const unsigned bm_w, T &&t)
		{
			for (;;)
			{
				switch (step(t))
				{
					case again:
						/* Step succeeded.  Notify `t` to update its
						 * dbits position, then loop around.
						 */
						t.consume_dbits(bm_w);
						break;
					case src_exhausted:
						/* Success: source buffer exhausted and no error
						 * conditions detected.
						 */
						return true;
					case dst_exhausted:
						/* Failure: destination buffer too small to hold
						 * expanded source.
						 */
						return false;
				}
			}
		}
private:
	step_result step_internal(uint8_t *begin_dbits, uint8_t *end_dbits);
};

}
