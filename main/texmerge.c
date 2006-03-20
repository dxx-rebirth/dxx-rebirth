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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/texmerge.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:59 $
 * 
 * Routines to cache merged textures.
 * 
 * $Log: texmerge.c,v $
 * Revision 1.1.1.1  2006/03/17 19:41:59  zicodxx
 * initial import
 *
 * Revision 1.3  1999/10/07 21:08:36  donut
 * OGL alternate texmerge
 *
 * Revision 1.2  1999/09/21 04:05:55  donut
 * mostly complete OGL implementation (still needs bitmap handling (reticle), and door/fan textures are corrupt)
 *
 * Revision 1.1.1.1  1999/06/14 22:11:46  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:08  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.28  1995/01/14  19:16:56  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.27  1994/12/14  18:21:58  yuan
 * *** empty log message ***
 * 
 * Revision 1.26  1994/12/13  09:50:08  john
 * Added Asserts to stop if wall looks like door.
 * 
 * Revision 1.25  1994/12/07  00:35:24  mike
 * change how flat shading average color is computed for paste-ons.
 * 
 * Revision 1.24  1994/11/19  15:20:29  mike
 * rip out unused code and data
 * 
 * Revision 1.23  1994/11/12  16:38:51  mike
 * deal with avg_color in texture merging.
 * 
 * Revision 1.22  1994/11/09  19:55:39  john
 * Added full rle support with texture rle caching.
 * 
 * Revision 1.21  1994/10/20  15:21:16  john
 * Took out the texmerge caching.
 * 
 * Revision 1.20  1994/10/10  19:00:57  john
 * Made caching info print every 1000 frames.
 * 
 * Revision 1.19  1994/10/10  18:41:21  john
 * Printed out texture caching info.
 * 
 * Revision 1.18  1994/08/11  18:59:02  mike
 * Use new assembler version of merge functions.
 * 
 * Revision 1.17  1994/06/09  12:13:14  john
 * Changed selectors so that all bitmaps have a selector of
 * 0, but inside the texture mapper they get a selector set.
 * 
 * Revision 1.16  1994/05/14  17:15:15  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.15  1994/05/09  17:21:09  john
 * Took out mprintf with cache hits/misses.
 * 
 * Revision 1.14  1994/05/05  12:55:07  john
 * Made SuperTransparency work.
 * 
 * Revision 1.13  1994/05/04  11:15:37  john
 * Added Super Transparency
 * 
 * Revision 1.12  1994/04/28  23:36:04  john
 * Took out a debugging mprintf.
 * 
 * Revision 1.11  1994/04/22  17:44:48  john
 * Made top 2 bits of paste-ons pick the
 * orientation of the bitmap.
 * 
 * Revision 1.10  1994/03/31  12:05:51  matt
 * Cleaned up includes
 * 
 * Revision 1.9  1994/03/15  16:31:52  yuan
 * Cleaned up bm-loading code.
 * (And structures)
 * 
 * Revision 1.8  1994/01/24  13:15:19  john
 * Made caching work with pointers, not texture numbers,
 * that way, the animated textures cache.
 * 
 * Revision 1.7  1994/01/21  16:38:10  john
 * Took out debug info.
 * 
 * Revision 1.6  1994/01/21  16:28:43  john
 * added warning to print cache hit/miss.
 * 
 * Revision 1.5  1994/01/21  16:22:30  john
 * Put in caching/
 * 
 * Revision 1.4  1994/01/21  15:34:49  john
 * *** empty log message ***
 * 
 * Revision 1.3  1994/01/21  15:33:08  john
 * *** empty log message ***
 * 
 * Revision 1.2  1994/01/21  15:15:35  john
 * Created new module texmerge, that merges textures together and 
 * caches the results.
 * 
 * Revision 1.1  1994/01/21  14:55:29  john
 * Initial revision
 * 
 * 
 */

#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: texmerge.c,v 1.1.1.1 2006/03/17 19:41:59 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdlib.h>

#include "gr.h"
#include "error.h"
#include "game.h"
#include "textures.h"
#include "mono.h"
#include "rle.h"
#include "piggy.h"

//resending textures into video ram is very slow, so cache more (worst case, the ogl driver will swap out some itself, probably doing a better job) -MM
#ifdef OGL
#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200
#else
#define MAX_NUM_CACHE_BITMAPS 50
#endif

#ifndef D1XD3D
#define PCCODE
#endif

void merge_textures_new( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data );
void merge_textures_super_xparent( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data );

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	int 		orient;
	int		last_frame_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE Cache[MAX_NUM_CACHE_BITMAPS];

static int num_cache_entries = 0;

static int cache_hits = 0;
static int cache_misses = 0;

void texmerge_close();

//----------------------------------------------------------------------

int texmerge_init(int num_cached_textures)
{
	int i;

	if ( num_cached_textures <= MAX_NUM_CACHE_BITMAPS )
		num_cache_entries = num_cached_textures;
	else
		num_cache_entries = MAX_NUM_CACHE_BITMAPS;
	
	for (i=0; i<num_cache_entries; i++ )	{
			// Make temp tmap for use when combining
		Cache[i].bitmap = gr_create_bitmap( 64, 64 );

		//if (get_selector( Cache[i].bitmap->bm_data, 64*64,  &Cache[i].bitmap->bm_selector))
		//	Error( "ERROR ALLOCATING CACHE BITMAP'S SELECTORS!!!!" );

		Cache[i].last_frame_used = -1;
		Cache[i].top_bmp = NULL;
		Cache[i].bottom_bmp = NULL;
		Cache[i].orient = -1;
	}
	atexit( texmerge_close );

	return 1;
}

void texmerge_flush()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )	{
		Cache[i].last_frame_used = -1;
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
		gr_free_bitmap( Cache[i].bitmap );
		Cache[i].bitmap = NULL;
	}
}

//--unused-- int info_printed = 0;

grs_bitmap * texmerge_get_cached_bitmap( int tmap_bottom, int tmap_top )
{
	grs_bitmap *bitmap_top, *bitmap_bottom;
	int i, orient;
	int lowest_frame_count;
	int least_recently_used;

//	if ( ((FrameCount % 1000)==0) && ((cache_hits+cache_misses)>0) && (!info_printed) )	{
//		mprintf( 0, "Texmap caching:  %d hits, %d misses. (Missed=%d%%)\n", cache_hits, cache_misses, (cache_misses*100)/(cache_hits+cache_misses)  );
//		info_printed = 1;
//	} else {
//		info_printed = 0;
//	}

	bitmap_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];
	bitmap_bottom = &GameBitmaps[Textures[tmap_bottom].index];
	
	orient = ((tmap_top&0xC000)>>14) & 3;

	least_recently_used = 0;
	lowest_frame_count = Cache[0].last_frame_used;
	
	for (i=0; i<num_cache_entries; i++ )	{
		if ( (Cache[i].last_frame_used > -1) && (Cache[i].top_bmp==bitmap_top) && (Cache[i].bottom_bmp==bitmap_bottom) && (Cache[i].orient==orient ))	{
			cache_hits++;
			Cache[i].last_frame_used = FrameCount;
			return Cache[i].bitmap;
		}	
		if ( Cache[i].last_frame_used < lowest_frame_count )	{
			lowest_frame_count = Cache[i].last_frame_used;
			least_recently_used = i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	// Make sure the bitmaps are paged in...
#ifdef PIGGY_USE_PAGING
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
#endif

#ifdef OGL
	ogl_freebmtexture(Cache[least_recently_used].bitmap);
#endif
	if (bitmap_top->bm_flags & BM_FLAG_SUPER_TRANSPARENT)	{
		merge_textures_super_xparent( orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap->bm_data );
		gr_set_bitmap_flags (Cache[least_recently_used].bitmap, BM_FLAG_TRANSPARENT);
		Cache[least_recently_used].bitmap->avg_color = bitmap_top->avg_color;
	} else	{
		merge_textures_new( orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap->bm_data );
		Cache[least_recently_used].bitmap->bm_flags = bitmap_bottom->bm_flags & (~BM_FLAG_RLE);
		Cache[least_recently_used].bitmap->avg_color = bitmap_bottom->avg_color;
	}

#ifdef D1XD3D
	gr_set_bitmap_data (Cache[least_recently_used].bitmap, Cache[least_recently_used].bitmap->bm_data);
#endif
		
	Cache[least_recently_used].top_bmp = bitmap_top;
	Cache[least_recently_used].bottom_bmp = bitmap_bottom;
	Cache[least_recently_used].last_frame_used = FrameCount;
	Cache[least_recently_used].orient = orient;

	return Cache[least_recently_used].bitmap;
}

#ifndef NMONO
grs_bitmap * usedp[500];
int usedc[500];
int usedn,usedna,usedg,usedgc;
void addused(grs_bitmap *bm){
	int u;
	for (u=0;u<usedn;u++)
		if (usedp[u]==bm){
			usedc[u]++;
			return;
		}
	usedp[usedn]=bm;
	usedc[usedn]=1;
	usedn++;
	if (!(bm->bm_flags&BM_FLAG_SUPER_TRANSPARENT))
		usedna++;
#ifdef OGL
	if (bm->gltexture>0)
		usedg++;
#endif
}
void merge_textures_stats(void){
	int i,num=0,numu=0,strans=0;
	usedn=usedna=usedg=usedgc=0;
	for (i=0; i<num_cache_entries; i++ ){
		if (Cache[i].last_frame_used<0){
			numu++;
			continue;
		}
		num++;
		if (Cache[i].top_bmp->bm_flags&BM_FLAG_SUPER_TRANSPARENT)
			strans++;
		addused(Cache[i].top_bmp);
		addused(Cache[i].bottom_bmp);
#ifdef OGL
		if (Cache[i].bitmap->gltexture>0)
			usedgc++;
#endif
	}
	mprintf((0,"texmerge: %i(%ig, %igc, %i left), strans:%i, uniq:%i(%i)\n",num,usedg,usedgc,numu,strans,usedn,usedna));
}
#endif //!NMONO

void merge_textures_new( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data )
{
#ifndef PCCODE
  ubyte c;
  int x,y;
#endif
	ubyte * top_data, *bottom_data;

	if ( top_bmp->bm_flags & BM_FLAG_RLE )
		top_bmp = rle_expand_texture(top_bmp);

	if ( bottom_bmp->bm_flags & BM_FLAG_RLE )
		bottom_bmp = rle_expand_texture(bottom_bmp);

//	Assert( bottom_bmp != top_bmp );

	top_data = top_bmp->bm_data;
	bottom_data = bottom_bmp->bm_data;

//	Assert( bottom_data != top_data );

	// mprintf( 0, "Type=%d\n", type );

	switch( type )	{
		case 0:
			// Normal
			
#ifdef PCCODE
			gr_merge_textures( bottom_data, top_data, dest_data );
#else
			for (y=0;y<64;y++) for (x=0;x<64;x++) {
				c=top_data[64*y+x];
				if (c==TRANSPARENCY_COLOR)
					c=bottom_data[64*y+x];
				*dest_data++=c;
            }
#endif
			break;
		case 1:
#ifdef PCCODE
			gr_merge_textures_1( bottom_data, top_data, dest_data );
#else
		  for (y=0; y<64; y++ )
			  for (x=0; x<64; x++ )   {
				  c = top_data[ 64*x+(63-y) ];
				  if (c==255)
					  c = bottom_data[ 64*y+x ];
				  *dest_data++ = c;
			  }
#endif
			break;
		case 2:

#ifdef PCCODE
			gr_merge_textures_2( bottom_data, top_data, dest_data );
#else
			for (y=0; y<64; y++ )
			  for (x=0; x<64; x++ )   {
				  c = top_data[ 64*(63-y)+(63-x) ];
				  if (c==255)
					  c = bottom_data[ 64*y+x ];
				  *dest_data++ = c;
			  }
#endif
			break;
		case 3:
#ifdef PCCODE
			gr_merge_textures_3( bottom_data, top_data, dest_data );
#else
		  for (y=0; y<64; y++ )
			  for (x=0; x<64; x++ )   {
				  c = top_data[ 64*(63-x)+y  ];
				  if (c==255)
					  c = bottom_data[ 64*y+x ];
				  *dest_data++ = c;
			  }
#endif
			break;
	}
}

void merge_textures_super_xparent( int type, grs_bitmap * bottom_bmp, grs_bitmap * top_bmp, ubyte * dest_data )
{
	ubyte c;
	int x,y;

	ubyte * top_data, *bottom_data;

	if ( top_bmp->bm_flags & BM_FLAG_RLE )
		top_bmp = rle_expand_texture(top_bmp);

	if ( bottom_bmp->bm_flags & BM_FLAG_RLE )
		bottom_bmp = rle_expand_texture(bottom_bmp);

//	Assert( bottom_bmp != top_bmp );

	top_data = top_bmp->bm_data;
	bottom_data = bottom_bmp->bm_data;

//	Assert( bottom_data != top_data );

	//mprintf( 0, "SuperX remapping type=%d\n", type );
	//Int3();
	 
	switch( type )	{
		case 0:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*y+x ];		
					if (c==255)
						c = bottom_data[ 64*y+x ];
					else if (c==254)
						c = 255;
					*dest_data++ = c;
				}
			break;
		case 1:
			// 
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*x+(63-y) ];		
					if (c==255)
						c = bottom_data[ 64*y+x ];
					else if (c==254)
						c = 255;
					*dest_data++ = c;
				}
			break;
		case 2:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*(63-y)+(63-x) ];
					if (c==255)
						c = bottom_data[ 64*y+x ];
					else if (c==254)
						c = 255;
					*dest_data++ = c;
				}
			break;
		case 3:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*(63-x)+y  ];
					if (c==255)
						c = bottom_data[ 64*y+x ];
					else if (c==254)
						c = 255;
					*dest_data++ = c;
				}
			break;
	}
}
