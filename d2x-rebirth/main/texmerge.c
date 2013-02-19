/*
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

#include <stdio.h>

#include "gr.h"
#include "dxxerror.h"
#include "game.h"
#include "textures.h"
#include "rle.h"
#include "piggy.h"
#include "timer.h"

#ifdef OGL
#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200
#else
#define MAX_NUM_CACHE_BITMAPS 50
#endif

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	int 		orient;
	fix64		last_time_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE Cache[MAX_NUM_CACHE_BITMAPS];

static int num_cache_entries = 0;

static int cache_hits = 0;
static int cache_misses = 0;

void texmerge_close();
void merge_textures_super_xparent(int type, grs_bitmap *bottom_bmp, grs_bitmap *top_bmp,
											 ubyte *dest_data);
void merge_textures_new(int type, grs_bitmap *bottom_bmp, grs_bitmap *top_bmp,
								ubyte *dest_data);

//----------------------------------------------------------------------

int texmerge_init(int num_cached_textures)
{
	int i;

	if ( num_cached_textures <= MAX_NUM_CACHE_BITMAPS )
		num_cache_entries = num_cached_textures;
	else
		num_cache_entries = MAX_NUM_CACHE_BITMAPS;
	
	for (i=0; i<num_cache_entries; i++ )	{
		Cache[i].bitmap = NULL;
		Cache[i].last_time_used = -1;
		Cache[i].top_bmp = NULL;
		Cache[i].bottom_bmp = NULL;
		Cache[i].orient = -1;
	}

	return 1;
}

void texmerge_flush()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )	{
		Cache[i].last_time_used = -1;
		Cache[i].top_bmp = NULL;
		Cache[i].bottom_bmp = NULL;
		Cache[i].orient = -1;
	}
}


//-------------------------------------------------------------------------
void texmerge_close()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )	{
		if (Cache[i].bitmap != NULL)
			gr_free_bitmap( Cache[i].bitmap );
		Cache[i].bitmap = NULL;
	}
}

//--unused-- int info_printed = 0;

grs_bitmap * texmerge_get_cached_bitmap( int tmap_bottom, int tmap_top )
{
	grs_bitmap *bitmap_top, *bitmap_bottom;
	int i, orient;
	int lowest_time_used;
	int least_recently_used;

	bitmap_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];
	bitmap_bottom = &GameBitmaps[Textures[tmap_bottom].index];
	
	orient = ((tmap_top&0xC000)>>14) & 3;

	least_recently_used = 0;
	lowest_time_used = Cache[0].last_time_used;
	
	for (i=0; i<num_cache_entries; i++ )	{
		if ( (Cache[i].last_time_used > -1) && (Cache[i].top_bmp==bitmap_top) && (Cache[i].bottom_bmp==bitmap_bottom) && (Cache[i].orient==orient ))	{
			cache_hits++;
			Cache[i].last_time_used = timer_query();
			return Cache[i].bitmap;
		}	
		if ( Cache[i].last_time_used < lowest_time_used )	{
			lowest_time_used = Cache[i].last_time_used;
			least_recently_used = i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	// Make sure the bitmaps are paged in...
	piggy_page_flushed = 0;

	PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
	PIGGY_PAGE_IN(Textures[tmap_bottom]);
	if (piggy_page_flushed)	{
		// If cache got flushed, re-read 'em.
		piggy_page_flushed = 0;
		PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
		PIGGY_PAGE_IN(Textures[tmap_bottom]);
	}
	Assert( piggy_page_flushed == 0 );
	if (bitmap_bottom->bm_w != bitmap_bottom->bm_h || bitmap_top->bm_w != bitmap_top->bm_h)
		Error("Texture width != texture height!\n");
	if (bitmap_bottom->bm_w != bitmap_top->bm_w || bitmap_bottom->bm_h != bitmap_top->bm_h)
		Error("Top and Bottom textures have different size!\n");

	if (Cache[least_recently_used].bitmap != NULL)
		gr_free_bitmap(Cache[least_recently_used].bitmap);
	Cache[least_recently_used].bitmap = gr_create_bitmap(bitmap_bottom->bm_w,  bitmap_bottom->bm_h);
#ifdef OGL
	ogl_freebmtexture(Cache[least_recently_used].bitmap);
#endif

	if (bitmap_top->bm_flags & BM_FLAG_SUPER_TRANSPARENT)	{
		merge_textures_super_xparent( orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap->bm_data );
		Cache[least_recently_used].bitmap->bm_flags = BM_FLAG_TRANSPARENT;
		Cache[least_recently_used].bitmap->avg_color = bitmap_top->avg_color;
	} else	{
		merge_textures_new( orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap->bm_data );
		Cache[least_recently_used].bitmap->bm_flags = bitmap_bottom->bm_flags & (~BM_FLAG_RLE);
		Cache[least_recently_used].bitmap->avg_color = bitmap_bottom->avg_color;
	}

	Cache[least_recently_used].top_bmp = bitmap_top;
	Cache[least_recently_used].bottom_bmp = bitmap_bottom;
	Cache[least_recently_used].last_time_used = timer_query();
	Cache[least_recently_used].orient = orient;

	return Cache[least_recently_used].bitmap;
}

void merge_textures_new( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data )
{
	ubyte * top_data, *bottom_data, c = 0;
	int x, y, wh;

	if ( top_bmp->bm_flags & BM_FLAG_RLE )
		top_bmp = rle_expand_texture(top_bmp);

	if ( bottom_bmp->bm_flags & BM_FLAG_RLE )
		bottom_bmp = rle_expand_texture(bottom_bmp);

	top_data = top_bmp->bm_data;
	bottom_data = bottom_bmp->bm_data;
	wh = bottom_bmp->bm_w;

	switch( type )	{
		case 0:
			// Normal
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )	{
					c = top_data[ wh*y+x ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 1:
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*x+((wh-1)-y) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 2:
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-y)+((wh-1)-x) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
		case 3:
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-x)+y  ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					*dest_data++ = c;
				}
			break;
	}
}

void merge_textures_super_xparent( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data )
{
	ubyte * top_data, *bottom_data, c = 0;
	int x, y, wh;

	if ( top_bmp->bm_flags & BM_FLAG_RLE )
		top_bmp = rle_expand_texture(top_bmp);

	if ( bottom_bmp->bm_flags & BM_FLAG_RLE )
		bottom_bmp = rle_expand_texture(bottom_bmp);

	top_data = top_bmp->bm_data;
	bottom_data = bottom_bmp->bm_data;
	wh = bottom_bmp->bm_w;

	switch( type )
	{
		case 0:
			// Normal
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*y+x ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 1:
			// 
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*x+((wh-1)-y) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 2:
			// Normal
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-y)+((wh-1)-x) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 3:
			// Normal
			for (y=0; y<wh; y++ )
				for (x=0; x<wh; x++ )
				{
					c = top_data[ wh*((wh-1)-x)+y  ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ wh*y+x ];
					else if (c==254)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
	}
}
