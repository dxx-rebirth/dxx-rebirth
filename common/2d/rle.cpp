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
 * Routines to do run length encoding/decoding
 * on bitmaps.
 *
 */

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "dxxerror.h"
#include "rle.h"
#include "byteutil.h"

#include "compiler-range_for.h"
#include "d_range.h"

namespace dcx {

constexpr uint8_t RLE_CODE = 0xe0;
constexpr uint8_t NOT_RLE_CODE = 0x1f;
static_assert((RLE_CODE | NOT_RLE_CODE) == 0xff, "RLE mask error");
static inline int IS_RLE_CODE(const uint8_t &x)
{
	return (x & RLE_CODE) == RLE_CODE;
}
#define rle_stosb(_dest, _len, _color)	memset(_dest,_color,_len)

uint8_t *gr_rle_decode(const color_palette_index *sb, color_palette_index *db, const rle_position_t e)
{
	using std::advance;
	using std::distance;
	for (; sb != e.src;)
	{
		auto p = sb;
		color_palette_index c;
		for (; c = *p, !IS_RLE_CODE(c);)
			if (++p == e.src)
				return db;
		const size_t count = (c & NOT_RLE_CODE);
		const size_t cn = std::min<size_t>(distance(sb, p), distance(db, e.dst));
		memcpy(db, sb, cn);
		advance(db, cn);
		if (!count)
			break;
		advance(sb, cn);
		if (sb == e.src || db == e.dst || count > static_cast<size_t>(distance(db, e.dst)))
			break;
		if (++ sb == e.src)
			break;
		std::fill_n(db, count, *sb++);
		advance(db, count);
	}
	return db;
}

#if !DXX_USE_OGL
// Given pointer to start of one scanline of rle data, uncompress it to
// dest, from source pixels x1 to x2.
void gr_rle_expand_scanline_masked(uint8_t *dest, const uint8_t *src, int x1, int x2)
{
	int i = 0;
	ubyte count;
	ubyte color=0;

	if ( x2 < x1 ) return;

	count = 0;
	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if ( x1+count > x2 )	{
		count = x2-x1+1;
		if ( color != TRANSPARENCY_COLOR )	rle_stosb( dest, count, color );
		return;
	}

	if ( color != TRANSPARENCY_COLOR )	rle_stosb( dest, count, color );
	dest += count;
	i += count;

	while( i <= x2 )
	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
			if ( color != 255 )rle_stosb( dest, count, color );
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			if ( color != 255 )rle_stosb( dest, count, color );
			i += count;
			dest += count;
		}
	}
}
#endif

void gr_rle_expand_scanline(uint8_t *dest, const uint8_t *src, int x1, int x2)
{
	int i = 0;
	ubyte count;
	ubyte color=0;

	if ( x2 < x1 ) return;

	count = 0;
	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if ( x1+count > x2 )	{
		count = x2-x1+1;
		rle_stosb( dest, count, color );
		return;
	}

	rle_stosb( dest, count, color );
	dest += count;
	i += count;

	while( i <= x2 )		{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
			rle_stosb( dest, count, color );
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			rle_stosb( dest, count, color );
			i += count;
			dest += count;
		}
	}
}

namespace {

static std::ptrdiff_t gr_rle_encode( int org_size, const uint8_t *src, ubyte *dest )
{
	ubyte c, oc;
	ubyte count;
	uint8_t *dest_start;

	dest_start = dest;
	oc = *src++;
	count = 1;

	if (org_size > 0)
	for (uint_fast32_t i = org_size; --i;)
	{
		c = *src++;
		if ( c!=oc )	{
			if ( count )	{
				if ( (count==1) && (! IS_RLE_CODE(oc)) )	{
					*dest++ = oc;
					Assert( oc != RLE_CODE );
				} else {
					count |= RLE_CODE;
					*dest++ = count;
					*dest++ = oc;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if ( count == NOT_RLE_CODE )	{
			count |= RLE_CODE;
			*dest++=count;
			*dest++=oc;
			count = 0;
		}
	}
	if (count)	{
		if ( (count==1) && (! IS_RLE_CODE(oc)) )	{
			*dest++ = oc;
			Assert( oc != RLE_CODE );
		} else {
			count |= RLE_CODE;
			*dest++ = count;
			*dest++ = oc;
		}
	}
	*dest++ = RLE_CODE;

	return dest-dest_start;
}

static unsigned gr_rle_getsize(int org_size, const uint8_t *src)
{
	ubyte c, oc;
	ubyte count;
	int dest_size=0;

	oc = *src++;
	count = 1;

	if (org_size > 0)
	for (uint_fast32_t i = org_size; --i;)
	{
		c = *src++;
		if ( c!=oc )	{
			if ( count )	{
				if ( (count==1) && (! IS_RLE_CODE(oc)) )	{
					dest_size++;
				} else {
					dest_size++;
					dest_size++;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if ( count == NOT_RLE_CODE )	{
			dest_size++;
			dest_size++;
			count = 0;
		}
	}
	if (count)	{
		if ( (count==1) && (! IS_RLE_CODE(oc)) )	{
			dest_size++;
		} else {
			dest_size++;
			dest_size++;
		}
	}
	dest_size++;

	return dest_size;
}

}

int gr_bitmap_rle_compress(grs_bitmap &bmp)
{
	int doffset;
	int large_rle = 0;

	// first must check to see if this is large bitmap.

	const uint_fast32_t bm_h = bmp.bm_h;
	const uint_fast32_t bm_w = bmp.bm_w;
	range_for (const uint_fast32_t y, xrange(bm_h))
	{
		auto d1 = gr_rle_getsize(bm_w, &bmp.get_bitmap_data()[bm_w * y]);
		if (d1 > 255) {
			large_rle = BM_FLAG_RLE_BIG;
			break;
		}
	}

	RAIIdmem<uint8_t[]> rle_data;
	MALLOC(rle_data, uint8_t[], MAX_BMP_SIZE(bm_w, bm_h));
	if (!rle_data) return 0;
	if (!large_rle)
		doffset = 4 + bm_h;
	else
		doffset = 4 + (2 * bm_h);		// each row of rle'd bitmap has short instead of byte offset now

	range_for (const uint_fast32_t y, xrange(bm_h))
	{
		auto d1 = gr_rle_getsize(bm_w, &bmp.get_bitmap_data()[bm_w * y]);
		if ( ((doffset+d1) > bmp.bm_w*bmp.bm_h) || (d1 > (large_rle?32767:255) ) ) {
			return 0;
		}
		const auto d = gr_rle_encode( bmp.bm_w, &bmp.get_bitmap_data()[bmp.bm_w*y], &rle_data[doffset] );
		Assert( d==d1 );
		doffset	+= d;
		if (large_rle)
			PUT_INTEL_SHORT(&rle_data[(y*2)+4], static_cast<short>(d));
		else
			rle_data[y+4] = d;
	}
	memcpy(bmp.get_bitmap_data(), &doffset, 4);
	memcpy(&bmp.get_bitmap_data()[4], &rle_data.get()[4], doffset - 4);
	bmp.add_flags(BM_FLAG_RLE | large_rle);
	return 1;
}

namespace {

struct rle_cache_element
{
	const grs_bitmap *rle_bitmap;
	grs_bitmap_ptr expanded_bitmap;
	int last_used;
};

static int rle_cache_initialized;
static int rle_counter;
static int rle_next;
static std::array<rle_cache_element, 32> rle_cache;

static void rle_cache_init()
{
	rle_cache = {};
	rle_cache_initialized = 1;
}

}

void rle_cache_close(void)
{
	if (rle_cache_initialized)	{
		rle_cache_initialized = 0;
		range_for (auto &i, rle_cache)
			i.expanded_bitmap.reset();
	}
}

void rle_cache_flush()
{
	range_for (auto &i, rle_cache)
	{
		i.rle_bitmap = NULL;
		i.last_used = 0;
	}
}

namespace {

static void rle_expand_texture_sub(const grs_bitmap &bmp, grs_bitmap &rle_temp_bitmap_1)
{
	auto sbits = &bmp.get_bitmap_data()[4 + bmp.bm_h];
	auto dbits = rle_temp_bitmap_1.get_bitmap_data();

	rle_temp_bitmap_1.set_flags(bmp.get_flags() & (~BM_FLAG_RLE));

	for (int i=0; i < bmp.bm_h; i++ ) {
		gr_rle_decode(sbits, dbits, rle_end(bmp, rle_temp_bitmap_1));
		sbits += static_cast<int>(bmp.bm_data[4+i]);
		dbits += bmp.bm_w;
	}
}

}

grs_bitmap *_rle_expand_texture(const grs_bitmap &bmp)
{
	int lowest_count, lc;

	if (!rle_cache_initialized) rle_cache_init();

	Assert(!(bmp.get_flag_mask(BM_FLAG_PAGED_OUT)));

	lc = rle_counter;
	rle_counter++;

	if (rle_counter < 0)
		rle_counter = 0;
	
	if ( rle_counter < lc )	{
		rle_cache_flush();
	}

	lowest_count = rle_cache[rle_next].last_used;
	auto least_recently_used = &rle_cache[rle_next];
	rle_next++;
	if (rle_next >= rle_cache.size())
		rle_next = 0;

	range_for (auto &i, rle_cache)
	{
		if (i.rle_bitmap == &bmp)
		{
			i.last_used = rle_counter;
			return i.expanded_bitmap.get();
		}
		if (i.last_used < lowest_count)
		{
			lowest_count = (least_recently_used = &i)->last_used;
		}
	}

	least_recently_used->expanded_bitmap = gr_create_bitmap(bmp.bm_w, bmp.bm_h);
	rle_expand_texture_sub(bmp, *least_recently_used->expanded_bitmap.get());
	least_recently_used->rle_bitmap = &bmp;
	least_recently_used->last_used = rle_counter;
	return least_recently_used->expanded_bitmap.get();
}

#if !DXX_USE_OGL
void gr_rle_expand_scanline_generic(grs_canvas &canvas, grs_bitmap &dest, int dx, const int dy, const uint8_t *src, const int x1, const int x2)
{
	int i = 0;
	int count;
	ubyte color=0;

	if ( x2 < x1 ) return;

	count = 0;
	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.

	if ( x1+count > x2 )	{
		count = x2-x1+1;
		for ( int j=0; j<count; j++ )
			gr_bm_pixel(canvas, dest, dx++, dy, color);
		return;
	}

	for ( int j=0; j<count; j++ )
		gr_bm_pixel(canvas, dest, dx++, dy, color);
	i += count;

	while( i <= x2 )		{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( IS_RLE_CODE(color) )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
		} else {
			count = x2-i+1;
		}
		for (unsigned j = 0; j < count; ++j)
			gr_bm_pixel(canvas, dest, dx++, dy, color);
		i += count;
	}
}
#endif

/*
 * swaps entries 0 and 255 in an RLE bitmap without uncompressing it
 */
void rle_swap_0_255(grs_bitmap &bmp)
{
	int len, rle_big;
	unsigned char *start;
	unsigned short line_size;

	rle_big = bmp.get_flag_mask(BM_FLAG_RLE_BIG);

	RAIIdmem<uint8_t[]> temp;
	MALLOC(temp, uint8_t[], MAX_BMP_SIZE(bmp.bm_w, bmp.bm_h));

	const std::size_t pointer_offset = rle_big ? 4 + 2 * bmp.bm_h : 4 + bmp.bm_h;
	auto ptr = &bmp.bm_data[pointer_offset];
	auto ptr2 = &temp[pointer_offset];
	for (int i = 0; i < bmp.bm_h; i++) {
		start = ptr2;
		if (rle_big)
			line_size = GET_INTEL_SHORT(&bmp.bm_data[4 + 2 * i]);
		else
			line_size = bmp.bm_data[4 + i];
		for (int j = 0; j < line_size; j++) {
			if ( ! IS_RLE_CODE(ptr[j]) ) {
				if (ptr[j] == 0) {
					*ptr2++ = RLE_CODE | 1;
					*ptr2++ = 255;
				} else
					*ptr2++ = ptr[j];
			} else {
				*ptr2++ = ptr[j];
				if ((ptr[j] & NOT_RLE_CODE) == 0)
					break;
				j++;
				if (ptr[j] == 0)
					*ptr2++ = 255;
				else if (ptr[j] == 255)
					*ptr2++ = 0;
				else
					*ptr2++ = ptr[j];
			}
		}
		if (rle_big)                // set line size
			PUT_INTEL_SHORT(&temp[4 + 2 * i], static_cast<uint16_t>(ptr2 - start));
		else
			temp[4 + i] = ptr2 - start;
		ptr += line_size;           // go to next line
	}
	len = ptr2 - temp.get();
	memcpy(bmp.get_bitmap_data(), &len, 4);
	memcpy(&bmp.get_bitmap_data()[4], &temp.get()[4], len - 4);
}

/*
 * remaps all entries using colormap in an RLE bitmap without uncompressing it
 */
void rle_remap(grs_bitmap &bmp, std::array<color_palette_index, 256> &colormap)
{
	int len, rle_big;
	unsigned short line_size;

	rle_big = bmp.get_flag_mask(BM_FLAG_RLE_BIG);

	RAIIdmem<color_palette_index[]> temp;
	MALLOC(temp, color_palette_index[], MAX_BMP_SIZE(bmp.bm_w, bmp.bm_h) + 30000);

	const std::size_t pointer_offset = rle_big ? 4 + 2 * bmp.bm_h : 4 + bmp.bm_h;
	auto ptr = &bmp.get_bitmap_data()[pointer_offset];
	auto ptr2 = &temp[pointer_offset];
	for (int i = 0; i < bmp.bm_h; i++) {
		auto start = ptr2;
		if (rle_big)
			line_size = GET_INTEL_SHORT(&bmp.get_bitmap_data()[4 + 2 * i]);
		else
			line_size = bmp.get_bitmap_data()[4 + i];
		for (int j = 0; j < line_size; j++) {
			const uint8_t v = ptr[j];
			if (!IS_RLE_CODE(v))
			{
				if (IS_RLE_CODE(colormap[v])) 
					*ptr2++ = color_palette_index{RLE_CODE | 1}; // add "escape sequence"
				*ptr2++ = colormap[v]; // translate
			} else {
				*ptr2++ = ptr[j]; // just copy current rle code
				if ((ptr[j] & NOT_RLE_CODE) == 0)
					break;
				j++;
				*ptr2++ = colormap[ptr[j]]; // translate
			}
		}
		if (rle_big)                // set line size
			PUT_INTEL_SHORT(&temp[4 + 2 * i], static_cast<uint16_t>(ptr2 - start));
		else
			temp[4 + i] = ptr2 - start;
		ptr += line_size;           // go to next line
	}
	len = ptr2 - temp.get();
	memcpy(bmp.get_bitmap_data(), &len, 4);
	memcpy(&bmp.get_bitmap_data()[4], &temp.get()[4], len - 4);
}

void bm_rle_src_stride::advance_src_bits()
{
	/* Both bytes are always legal to read since the bitmap data
	 * is placed after the length table.  Reading both, then
	 * conditionally masking out the high bits (dependent on
	 * BM_FLAG_RLE_BIG) encourages the compiler to implement
	 * this line without using branches.
	 */
	const uintptr_t u = (ptr_src_bit_lengths[0] | (static_cast<uintptr_t>(ptr_src_bit_lengths[1]) << 8)) & src_bit_load_mask;
	ptr_src_bit_lengths += src_bit_stride_size;
	src_bits += u;
}

bm_rle_expand::step_result bm_rle_expand::step_internal(uint8_t *const begin_dbits, uint8_t *const end_dbits)
{
	const auto rd = gr_rle_decode(src_bits, begin_dbits, {end_src_bm, end_dbits});
	/* If the destination buffer is exhausted, return without
	 * modifying the source state.  This lets the caller retry
	 * with a larger buffer, if desired.
	 */
	if (unlikely(begin_dbits == rd))
		return dst_exhausted;
	advance_src_bits();
	return again;
}

}
