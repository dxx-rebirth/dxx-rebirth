/* $Id: ibitblt.h,v 1.3 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Prototypes for the ibitblt functions.
 *
 */

#ifndef _IBITBLT_H
#define _IBITBLT_H

// Finds location/size of the largest "hole" in bitmap mask_bmp
void gr_ibitblt_find_hole_size ( grs_bitmap * mask_bmp, int *minx, int *miny, int *maxx, int *maxy );

// Creates a code mask that will copy data from a bitmap that is sw by
// sh starting from location sx, sy with a rowsize of srowsize onto
// another bitmap but only copies into pixel locations that are
// defined as transparent in bitmap bmp.

#ifdef __MSDOS__
ubyte * gr_ibitblt_create_mask(grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize);
ubyte * gr_ibitblt_create_mask_svga(grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize);
ubyte * gr_ibitblt_create_mask_pa( grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize);
#else
void gr_ibitblt_create_mask_pa(grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize);
void gr_ibitblt_create_mask(grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize);
#endif

// Copy source bitmap onto destination bitmap, not copying pixels that
// are defined transparent by the mask

#ifdef __MSDOS__
void gr_ibitblt(grs_bitmap *source_bmp, grs_bitmap *dest_bmp, ubyte *mask);
#else
void gr_ibitblt(grs_bitmap *source_bmp, grs_bitmap *dest_bmp, ubyte pixel_double);
#endif

#endif
