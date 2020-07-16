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
#include <array>
#include <bitset>

namespace dcx {
void build_colormap_good(const palette_array_t &palette, std::array<color_palette_index, 256> &colormap);
void decode_data(color_palette_index *data, uint_fast32_t num_pixels, std::array<color_palette_index, 256> &colormap, std::bitset<256> &used);
}

#endif
