/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#include <algorithm>
#include <cstdint>
#include "d_underlying_value.h"
#include "fwd-gr.h"

namespace dcx {

namespace {

uint8_t gr_palette_gamma_param;

}

palette_array_t gr_palette;
palette_array_t gr_current_pal;
gft_array1 gr_fade_table;

uint8_t gr_palette_gamma;

void copy_bound_palette(palette_array_t &d, const palette_array_t &s)
{
	std::ranges::transform(s, d.begin(), [](const rgb_t c) {
		constexpr uint8_t bound{63};
		return rgb_t{
			.r = std::min(c.r, bound),
			.g = std::min(c.g, bound),
			.b = std::min(c.b, bound)
		};
	});
}

void gr_palette_set_gamma(uint8_t gamma)
{
        if ( gamma > 16 ) gamma = 16;      //was 8

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		gr_palette_load( gr_palette );
	}
}

uint8_t gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}

color_palette_index gr_find_closest_color_15bpp(const packed_color_r5g5b5 rgb)
{
	const auto urgb = underlying_value(rgb);
	return gr_find_closest_color(
		((urgb >> 9) & 62),
		((urgb >> 4) & 62),
		((urgb << 1) & 62)
		);
}

}
