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

/*
 *
 * Graphical routines for setting the palette
 *
 */

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "physfsx.h"
#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "dxxerror.h"
#include "maths.h"
#include "palette.h"

#include "d_enumerate.h"
#include "d_underlying_value.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include <array>

namespace dcx {

namespace {

unsigned get_squared_color_delta(const int r, const int g, const int b, const rgb_t &rgb)
{
	const auto dr = r - rgb.r;
	const auto dg = g - rgb.g;
	const auto db = b - rgb.b;
	return (dr * dr) + (dg * dg) + (db * db);
}

color_palette_index gr_find_closest_color_palette(const int r, const int g, const int b, const palette_array_t &palette)
{
	// Anything is better than this.
	unsigned best_value = UINT_MAX;
	color_palette_index best_index{};
	// only go to 255, 'cause we dont want to check the transparent color.
	for (auto &&[candidate_idx, candidate_rgb] : enumerate(unchecked_partial_range(palette, palette.size() - 1)))
	{
		const auto candidate_value = get_squared_color_delta(r, g, b, candidate_rgb);
		if (best_value > candidate_value)
		{
			best_index = candidate_idx;
			if (candidate_value == 0)
				/* Perfect match */
				break;
			best_value = candidate_value;
		}
	}
	return best_index;
}

static unsigned Num_computed_colors;

uint8_t gr_palette_gamma_param;

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

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
void gr_copy_palette(palette_array_t &gr_palette, const palette_array_t &pal)
{
	gr_palette = pal;

	        Num_computed_colors = 0;
}
#endif

void gr_use_palette_table(const char * filename )
{
	int fsize;

	auto &&[fp, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!fp)
	{
#if defined(DXX_BUILD_DESCENT_I)
		Error("Failed to open palette file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));
#elif defined(DXX_BUILD_DESCENT_II)
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
		auto &&[fp2, physfserr2] = PHYSFSX_openReadBuffered(DEFAULT_LEVEL_PALETTE);
		if (!fp)
			Error("Failed to open both palette file <%s> and default palette file <" DEFAULT_LEVEL_PALETTE ">: (\"%s\", \"%s\")", filename, PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
		fp = std::move(fp2);
#endif
	}

	fsize	= PHYSFS_fileLength( fp );
	Assert( fsize == 9472 );
	(void)fsize;
	PHYSFSX_readBytes(fp, gr_palette, sizeof(gr_palette[0]) * std::size(gr_palette));
	PHYSFSX_readBytes(fp, gr_fade_table, 256*34);
	fp.reset();

	// This is the TRANSPARENCY COLOR
	range_for (auto &i, gr_fade_table)
		i[255] = 255;
#if defined(DXX_BUILD_DESCENT_II)
	Num_computed_colors = 0;	//	Flush palette cache.
#endif
}

}

namespace dcx {

void reset_computed_colors()
{
	Num_computed_colors = 0;
}

color_palette_index gr_find_closest_color(const int r, const int g, const int b)
{
	struct color_record {
		uint8_t r, g, b;
		color_palette_index color_num;
	};
	static std::array<color_record, 32> Computed_colors;
	const auto num_computed_colors{Num_computed_colors};
	//	If we've already computed this color, return it!
	for (auto &&[i, c] : enumerate(std::span(Computed_colors).first(num_computed_colors)))
	{
		if (r == c.r && g == c.g && b == c.b)
		{
			const auto color_num{c.color_num};
			if (i)
				/* Move the color toward the beginning of the range, so that
				 * recently popular colors require fewer steps to find on a
				 * subsequent call.
				 */
				std::swap(Computed_colors[i - 1], c);
			return color_num;
		}
	}
//	Add a computed color to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace the last one.  Rely on the bubble-up logic
//	above to move popular entries away from the end.
	auto &cc = (num_computed_colors < Computed_colors.size())
		? Computed_colors[Num_computed_colors++]
		: Computed_colors.back();
	cc.r = r;
	cc.g = g;
	cc.b = b;
	return cc.color_num = gr_find_closest_color_palette(r, g, b, gr_palette);
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

color_palette_index gr_find_closest_color_current(const int r, const int g, const int b)
{
	return gr_find_closest_color_palette(r, g, b, gr_current_pal);
}

}
