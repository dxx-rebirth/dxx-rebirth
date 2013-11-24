#ifndef _BITMAP_H
#define _BITMAP_H

#include "pstypes.h"

#ifdef __cplusplus

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );
void decode_data(ubyte *data, int num_pixels, ubyte * colormap, int * count );

#endif

#endif
