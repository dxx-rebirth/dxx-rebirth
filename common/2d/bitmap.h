/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _BITMAP_H
#define _BITMAP_H

#include "pstypes.h"

#ifdef __cplusplus
#include "palette.h"

void build_colormap_good( palette_array_t &palette, ubyte * colormap, int * freq );
void decode_data(ubyte *data, int num_pixels, ubyte * colormap, int * count );

#endif

#endif
