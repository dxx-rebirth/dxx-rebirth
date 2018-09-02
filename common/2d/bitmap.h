/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "pstypes.h"

#ifdef __cplusplus
#include "palette.h"
#include "compiler-array.h"

namespace dcx {
void build_colormap_good(const palette_array_t &palette, array<color_t, 256> &colormap);
void decode_data(ubyte *data, uint_fast32_t num_pixels, array<color_t, 256> &colormap, array<bool, 256> &used);
}

#endif
