/* $Id: bitmap.h,v 1.4 2002-09-05 07:55:20 btb Exp $ */
#ifndef _BITMAP_H
#define _BITMAP_H

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );
void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count );

#endif
