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
 *
 * Protypes for rle functions.
 *
 */

#ifndef _RLE_H
#define _RLE_H

#include "pstypes.h"
#include "gr.h"

void gr_rle_decode( ubyte * src, ubyte * dest );
int gr_rle_encode( int org_size, ubyte *src, ubyte *dest );
int gr_rle_getsize( int org_size, ubyte *src );
ubyte * gr_rle_find_xth_pixel( ubyte *src, int x,int * count, ubyte color );
int gr_bitmap_rle_compress( grs_bitmap * bmp );
void gr_rle_expand_scanline_masked( ubyte *dest, ubyte *src, int x1, int x2 );
void gr_rle_expand_scanline( ubyte *dest, ubyte *src, int x1, int x2  );
grs_bitmap * rle_expand_texture( grs_bitmap * bmp );
void rle_cache_close();
void rle_cache_flush();
void rle_swap_0_255(grs_bitmap *bmp);
void rle_remap(grs_bitmap *bmp, ubyte *colormap);
void gr_rle_expand_scanline_generic( grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2 );

#endif
