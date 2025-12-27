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
 * Routines to cache merged textures.
 *
 */


#include "gr.h"
#include "dxxerror.h"
#include "fmtcheck.h"
#include "textures.h"
#include "rle.h"
#include "timer.h"
#include "piggy.h"
#include "segment.h"
#include "texmerge.h"
#include "piggy.h"

#include "compiler-range_for.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "partial_range.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {

namespace {

struct TEXTURE_CACHE {
	enum class cache_key : uint32_t
	{
	};
	static constexpr cache_key build_cache_key(const bitmap_index bottom, const bitmap_index top, const texture2_rotation_low orient)
	{
		/* All valid values for `bottom` and `top` are below
		 * `MAX_BITMAP_FILES`, so each of them can be packed into a field
		 * `std::bit_width(MAX_BITMAP_FILES)` bits wide.  If an invalid value
		 * were passed for `bottom` or `top`, then that value could influence
		 * bits outside its slice of the cache key.  However, if an invalid
		 * value were passed, the caller would suffer undefined behavior when
		 * it indexed outside GameBitmaps, and likely crash soon after.
		 * Therefore, there is no need to check for invalid values or clamp the
		 * inputs to valid values.
		 *
		 * `std::bit_width(d1x::MAX_BITMAP_FILES)` = 11
		 * `std::bit_width(d2x::MAX_BITMAP_FILES)` = 12
		 * Use 12u in both games, since 11u does not save enough space to allow
		 * use of a smaller integer for the key.
		 */
		constexpr unsigned bitmap_shift_width{12u};
		static_assert(MAX_BITMAP_FILES < (1u << bitmap_shift_width));
		return cache_key{
			uint32_t{underlying_value(bottom)} |
			(uint32_t{underlying_value(top)} << bitmap_shift_width) |
			(uint32_t{underlying_value(orient)} << (2 * bitmap_shift_width)) |
			/* There are 4 valid values for `texture2_rotation_low`, so
			 * allocate 2 bits for it.
			 *
			 * Enable a fixed `1` here, so that `build_cache_key(0, 0, 0)` is
			 * distinct from `cache_key{}`.  Construction will have set all
			 * cache keys to `cache_key{}`.  The program should not try to
			 * composite `bitmap_index{0}` with itself, but if it did and this
			 * line were absent, then the cache system would consider a
			 * value-initialized record to match, and would return it.  Avoid
			 * that by ensuring that no combination of inputs to
			 * `build_cache_key` can generate a result that compares equal to
			 * `cache_key{}`.
			 */
			(uint32_t{1} << (2 + (2 * bitmap_shift_width)))
		};
	}
	cache_key key{};
	grs_bitmap_ptr bitmap;
	fix64		last_time_used{};
};

/* Helper classes merge_texture_0 through merge_texture_3 correspond to
 * the four values of `orient` used by texmerge_get_cached_bitmap.
 */
struct merge_texture_0
{
	static size_t get_top_data_index(const unsigned wh, const unsigned y, const unsigned x)
	{
		return wh * y + x;
	}
};

struct merge_texture_1
{
	static size_t get_top_data_index(const unsigned wh, const unsigned y, const unsigned x)
	{
		return wh * x + ((wh - 1) - y);
	}
};

struct merge_texture_2
{
	static size_t get_top_data_index(const unsigned wh, const unsigned y, const unsigned x)
	{
		return wh * ((wh - 1) - y) + ((wh - 1) - x);
	}
};

struct merge_texture_3
{
	static size_t get_top_data_index(const unsigned wh, const unsigned y, const unsigned x)
	{
		return wh * ((wh - 1) - x) + y;
	}
};

/* For supertransparent colors, remap 254.
 * For regular transparent colors, do nothing.
 *
 * In both cases, the caller remaps TRANSPARENCY_COLOR to the bottom
 * bitmap.
 */
struct merge_transform_super_xparent
{
	static uint8_t transform_color(uint8_t c)
	{
		return c == 254 ? TRANSPARENCY_COLOR : c;
	}
};

struct merge_transform_new
{
	static uint8_t transform_color(uint8_t c)
	{
		return c;
	}
};

/* Run the transform for one texture merge case.  Different values of
 * `orient` in texmerge_get_cached_bitmap lead to different types for
 * `get_index`.
 */
template <typename texture_transform, typename get_index>
static void merge_textures_case(const unsigned wh, const uint8_t *const top_data, const uint8_t *const bottom_data, uint8_t *dest_data)
{
	const auto &&whr = xrange(wh);
	for (const auto y : whr)
		for (const auto x : whr)
		{
			const auto c = top_data[get_index::get_top_data_index(wh, y, x)];
			/* All merged textures support TRANSPARENCY_COLOR, so handle
			 * it here.  Supertransparency is delegated down to
			 * `texture_transform`, since not all textures want
			 * supertransparency.
			 */
			*dest_data++ = (c == TRANSPARENCY_COLOR)
				? bottom_data[wh * y + x]
				: texture_transform::transform_color(c);
		}
}

/* Dispatch a texture transformation based on the value of `orient`.
 * The loops are duplicated in each case so that `orient` is not reread
 * for each byte processed.
 */
template <typename texture_transform>
static void merge_textures(const unsigned wh, const uint8_t *const top_data, const uint8_t *const bottom_data, uint8_t *const dest_data, const texture2_rotation_low orient)
{
	switch (orient)
	{
		default:
			/* The default label should be unreachable.  Define it equal to
			 * `Normal` so that the compiler is free to let that path run the
			 * normal case instead of needing a jump to bypass all cases.
			 */
		case texture2_rotation_low::Normal:
			merge_textures_case<texture_transform, merge_texture_0>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_low::_1:
			merge_textures_case<texture_transform, merge_texture_1>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_low::_2:
			merge_textures_case<texture_transform, merge_texture_2>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_low::_3:
			merge_textures_case<texture_transform, merge_texture_3>(wh, top_data, bottom_data, dest_data);
			break;
	}
}

static std::array<TEXTURE_CACHE, /* MAX_NUM_CACHE_BITMAPS = */ 10> Cache;

static int cache_hits = 0;
static int cache_misses = 0;

}

//----------------------------------------------------------------------

void texmerge_flush()
{
	range_for (auto &i, Cache)
	{
		i.last_time_used = {};
		i.key = {};
	}
}


//-------------------------------------------------------------------------
void texmerge_close()
{
	range_for (auto &i, Cache)
	{
		i.bitmap.reset();
	}
}

}

namespace dsx {

grs_bitmap &texmerge_get_cached_bitmap(GameBitmaps_array &GameBitmaps, const Textures_array &Textures, const texture1_value tmap_bottom, const texture2_value tmap_top)
{
	const auto texture_top{Textures[get_texture_index(tmap_top)]};
	const auto texture_bottom{Textures[get_texture_index(tmap_bottom)]};
	
	const auto orient{get_texture_rotation_low(tmap_top)};

	auto least_recently_used = &Cache.front();
	auto lowest_time_used{least_recently_used->last_time_used};
	const auto cache_lookup_key{TEXTURE_CACHE::build_cache_key(texture_bottom, texture_top, orient)};
	range_for (auto &i, Cache)
	{
		if (i.key == cache_lookup_key)	{
			cache_hits++;
			i.last_time_used = timer_query();
			return *i.bitmap.get();
		}	
		if ( i.last_time_used < lowest_time_used )	{
			lowest_time_used = {i.last_time_used};
			least_recently_used = &i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	// Make sure the bitmaps are paged in...

	const auto &bitmap_top{GameBitmaps[texture_top]};
	const auto &bitmap_bottom{GameBitmaps[texture_bottom]};
	PIGGY_PAGE_IN(texture_top);
	PIGGY_PAGE_IN(texture_bottom);
	if (bitmap_bottom.bm_w != bitmap_bottom.bm_h || bitmap_top.bm_w != bitmap_top.bm_h)
		Error("Texture width != texture height!\nbottom tmap = %u; bottom bitmap = %u; bottom width = %u; bottom height = %u\ntop tmap = %hu; top bitmap = %u; top width=%u; top height=%u", underlying_value(tmap_bottom), underlying_value(texture_bottom), bitmap_bottom.bm_w, bitmap_bottom.bm_h, underlying_value(tmap_top), underlying_value(texture_top), bitmap_top.bm_w, bitmap_top.bm_h);
	if (bitmap_bottom.bm_w != bitmap_top.bm_w || bitmap_bottom.bm_h != bitmap_top.bm_h)
		Error("Top and Bottom textures have different size!\nbottom tmap = %u; bottom bitmap = %u; bottom width = %u; bottom height = %u\ntop tmap = %hu; top bitmap = %u; top width=%u; top height=%u", underlying_value(tmap_bottom), underlying_value(texture_bottom), bitmap_bottom.bm_w, bitmap_bottom.bm_h, underlying_value(tmap_top), underlying_value(texture_top), bitmap_top.bm_w, bitmap_top.bm_h);

	auto merged_bitmap{gr_create_bitmap(bitmap_bottom.bm_w, bitmap_bottom.bm_h)};
	auto &mb{*merged_bitmap.get()};
#if DXX_USE_OGL
	ogl_freebmtexture(mb);
#endif

	auto &expanded_top_bmp{*rle_expand_texture(bitmap_top)};
	auto &expanded_bottom_bmp{*rle_expand_texture(bitmap_bottom)};
	if (bitmap_top.get_flag_mask(BM_FLAG_SUPER_TRANSPARENT))
	{
		merge_textures<merge_transform_super_xparent>(expanded_bottom_bmp.bm_w, expanded_top_bmp.bm_data, expanded_bottom_bmp.bm_data, mb.get_bitmap_data(), orient);
		gr_set_bitmap_flags(mb, BM_FLAG_TRANSPARENT);
#if !DXX_USE_OGL
		mb.avg_color = bitmap_top.avg_color;
#endif
	} else	{
		merge_textures<merge_transform_new>(expanded_bottom_bmp.bm_w, expanded_top_bmp.bm_data, expanded_bottom_bmp.bm_data, mb.get_bitmap_data(), orient);
		mb.set_flags(bitmap_bottom.get_flag_mask(~BM_FLAG_RLE));
#if !DXX_USE_OGL
		mb.avg_color = bitmap_bottom.avg_color;
#endif
	}

	least_recently_used->key = cache_lookup_key;
	least_recently_used->bitmap = std::move(merged_bitmap);
	least_recently_used->last_time_used = timer_query();
	return mb;
}

tmapinfo_flags get_side_combined_tmapinfo_flags(const d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, const unique_side &uside)
{
	const auto texture1_index{get_texture_index(uside.tmap_num)};
	const auto tmap1_flags{TmapInfo[texture1_index].flags};
	if (const auto tmap_num2{uside.tmap_num2}; tmap_num2 != texture2_value::None)
	{
		const auto texture2_index{get_texture_index(tmap_num2)};
		/* No other call site needs to combine two sets of tmapinfo_flags, so there
		 * is no overloaded operator to handle this.  Use the casts to allow it
		 * here.
		 */
		return static_cast<tmapinfo_flags>(underlying_value(tmap1_flags) | underlying_value(TmapInfo[texture2_index].flags));
	}
	return tmap1_flags;
}

}
