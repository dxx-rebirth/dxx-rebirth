/* $Id: bitmap.h,v 1.1.1.1 2006/03/17 19:51:57 zicodxx Exp $ */
#ifndef _BITMAP_H
#define _BITMAP_H

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );
void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count );

#endif
