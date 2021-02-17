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
#include "partial_range.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#define MAX_NUM_CACHE_BITMAPS 10

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

namespace {

struct TEXTURE_CACHE {
	grs_bitmap_ptr bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	texture2_rotation_high orient;
	fix64		last_time_used;
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

}

/* Run the transform for one texture merge case.  Different values of
 * `orient` in texmerge_get_cached_bitmap lead to different types for
 * `get_index`.
 */
template <typename texture_transform, typename get_index>
static void merge_textures_case(const unsigned wh, const uint8_t *const top_data, const uint8_t *const bottom_data, uint8_t *dest_data)
{
	for (unsigned y = 0; y < wh; ++y)
		for (unsigned x = 0; x < wh; ++x)
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
static void merge_textures(const texture2_rotation_high orient, const grs_bitmap &expanded_bottom_bmp, const grs_bitmap &expanded_top_bmp, uint8_t *const dest_data)
{
	const auto &top_data = expanded_top_bmp.bm_data;
	const auto &bottom_data = expanded_bottom_bmp.bm_data;
	const auto wh = expanded_bottom_bmp.bm_w;
	switch (orient)
	{
		case texture2_rotation_high::Normal:
			merge_textures_case<texture_transform, merge_texture_0>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_high::_1:
			merge_textures_case<texture_transform, merge_texture_1>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_high::_2:
			merge_textures_case<texture_transform, merge_texture_2>(wh, top_data, bottom_data, dest_data);
			break;
		case texture2_rotation_high::_3:
			merge_textures_case<texture_transform, merge_texture_3>(wh, top_data, bottom_data, dest_data);
			break;
	}
}

static std::array<TEXTURE_CACHE, MAX_NUM_CACHE_BITMAPS> Cache;

static int cache_hits = 0;
static int cache_misses = 0;

//----------------------------------------------------------------------

int texmerge_init()
{
	range_for (auto &i, Cache)
	{
		i.bitmap = NULL;
		i.last_time_used = -1;
		i.top_bmp = NULL;
		i.bottom_bmp = NULL;
	}

	return 1;
}

void texmerge_flush()
{
	range_for (auto &i, Cache)
	{
		i.last_time_used = -1;
		i.top_bmp = NULL;
		i.bottom_bmp = NULL;
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

//--unused-- int info_printed = 0;

grs_bitmap &texmerge_get_cached_bitmap(const texture1_value tmap_bottom, const texture2_value tmap_top)
{
	grs_bitmap *bitmap_top, *bitmap_bottom;
	int lowest_time_used;

	auto &texture_top = Textures[get_texture_index(tmap_top)];
	bitmap_top = &GameBitmaps[texture_top.index];
	auto &texture_bottom = Textures[get_texture_index(tmap_bottom)];
	bitmap_bottom = &GameBitmaps[texture_bottom.index];
	
	const auto orient = get_texture_rotation_high(tmap_top);

	lowest_time_used = Cache[0].last_time_used;
	auto least_recently_used = &Cache.front();
	range_for (auto &i, Cache)
	{
		if ( (i.last_time_used > -1) && (i.top_bmp==bitmap_top) && (i.bottom_bmp==bitmap_bottom) && (i.orient==orient ))	{
			cache_hits++;
			i.last_time_used = timer_query();
			return *i.bitmap.get();
		}	
		if ( i.last_time_used < lowest_time_used )	{
			lowest_time_used = i.last_time_used;
			least_recently_used = &i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	// Make sure the bitmaps are paged in...

	PIGGY_PAGE_IN(texture_top);
	PIGGY_PAGE_IN(texture_bottom);
	if (bitmap_bottom->bm_w != bitmap_bottom->bm_h || bitmap_top->bm_w != bitmap_top->bm_h)
		Error("Texture width != texture height!\nbottom tmap = %u; bottom bitmap = %u; bottom width = %u; bottom height = %u\ntop tmap = %hu; top bitmap = %u; top width=%u; top height=%u", static_cast<uint16_t>(tmap_bottom), texture_bottom.index, bitmap_bottom->bm_w, bitmap_bottom->bm_h, static_cast<uint16_t>(tmap_top), texture_top.index, bitmap_top->bm_w, bitmap_top->bm_h);
	if (bitmap_bottom->bm_w != bitmap_top->bm_w || bitmap_bottom->bm_h != bitmap_top->bm_h)
		Error("Top and Bottom textures have different size!\nbottom tmap = %u; bottom bitmap = %u; bottom width = %u; bottom height = %u\ntop tmap = %hu; top bitmap = %u; top width=%u; top height=%u", static_cast<uint16_t>(tmap_bottom), texture_bottom.index, bitmap_bottom->bm_w, bitmap_bottom->bm_h, static_cast<uint16_t>(tmap_top), texture_top.index, bitmap_top->bm_w, bitmap_top->bm_h);

	least_recently_used->bitmap = gr_create_bitmap(bitmap_bottom->bm_w,  bitmap_bottom->bm_h);
#if DXX_USE_OGL
	ogl_freebmtexture(*least_recently_used->bitmap.get());
#endif

	auto &expanded_top_bmp = *rle_expand_texture(*bitmap_top);
	auto &expanded_bottom_bmp = *rle_expand_texture(*bitmap_bottom);
	if (bitmap_top->get_flag_mask(BM_FLAG_SUPER_TRANSPARENT))
	{
		merge_textures<merge_transform_super_xparent>(orient, expanded_bottom_bmp, expanded_top_bmp, least_recently_used->bitmap->get_bitmap_data());
		gr_set_bitmap_flags(*least_recently_used->bitmap.get(), BM_FLAG_TRANSPARENT);
#if !DXX_USE_OGL
		least_recently_used->bitmap->avg_color = bitmap_top->avg_color;
#endif
	} else	{
		merge_textures<merge_transform_new>(orient, expanded_bottom_bmp, expanded_top_bmp, least_recently_used->bitmap->get_bitmap_data());
		least_recently_used->bitmap->set_flags(bitmap_bottom->get_flag_mask(~BM_FLAG_RLE));
#if !DXX_USE_OGL
		least_recently_used->bitmap->avg_color = bitmap_bottom->avg_color;
#endif
	}

	least_recently_used->top_bmp = bitmap_top;
	least_recently_used->bottom_bmp = bitmap_bottom;
	least_recently_used->last_time_used = timer_query();
	least_recently_used->orient = orient;
	return *least_recently_used->bitmap.get();
}
