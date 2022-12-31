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

struct color_record {
	uint8_t r, g, b;
	color_palette_index color_num;
};

uint8_t gr_palette_gamma_param;

}

palette_array_t gr_palette;
palette_array_t gr_current_pal;
gft_array1 gr_fade_table;

ubyte gr_palette_gamma = 0;

void copy_bound_palette(palette_array_t &d, const palette_array_t &s)
{
	auto a = [](rgb_t c) {
		constexpr uint8_t bound{63};
		c.r = std::min(c.r, bound);
		c.g = std::min(c.g, bound);
		c.b = std::min(c.b, bound);
		return c;
	};
	std::transform(s.begin(), s.end(), d.begin(), a);
}

}

namespace dcx {

namespace {

static void diminish_entry(rgb_t &c)
{
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2; 
}

}

void diminish_palette(palette_array_t &palette)
{
	range_for (rgb_t &c, palette)
		diminish_entry(c);
}

void gr_palette_set_gamma( int gamma )
{
	if ( gamma < 0 ) gamma = 0;
        if ( gamma > 16 ) gamma = 16;      //was 8

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		gr_palette_load( gr_palette );
	}
}

int gr_palette_get_gamma()
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
	PHYSFS_read( fp, &gr_palette[0], sizeof(gr_palette[0]), gr_palette.size() );
	PHYSFS_read( fp, gr_fade_table, 256*34, 1 );
	fp.reset();

	// This is the TRANSPARENCY COLOR
	range_for (auto &i, gr_fade_table)
		i[255] = 255;
#if defined(DXX_BUILD_DESCENT_II)
	Num_computed_colors = 0;	//	Flush palette cache.
// swap colors 0 and 255 of the palette along with fade table entries

#ifdef SWAP_0_255
	for (i = 0; i < 3; i++) {
		ubyte c;
		c = gr_palette[i];
		gr_palette[i] = gr_palette[765+i];
		gr_palette[765+i] = c;
	}

	for (i = 0; i < GR_FADE_LEVELS * 256; i++) {
		if (gr_fade_table[i] == 0)
			gr_fade_table[i] = 255;
	}
	for (i=0; i<GR_FADE_LEVELS; i++)
		gr_fade_table[i*256] = TRANSPARENCY_COLOR;
#endif
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
	static std::array<color_record, 32> Computed_colors;
	const auto num_computed_colors = Num_computed_colors;
	//	If we've already computed this color, return it!
	for (unsigned i = 0; i < num_computed_colors; ++i)
	{
		auto &c = Computed_colors[i];
		if (r == c.r && g == c.g && b == c.b)
		{
			const auto color_num = c.color_num;
			if (i)
			{
						std::swap(Computed_colors[i-1], c);
					}
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

color_palette_index gr_find_closest_color_15bpp( int rgb )
{
	return gr_find_closest_color( ((rgb>>10)&31)*2, ((rgb>>5)&31)*2, (rgb&31)*2 );
}

color_palette_index gr_find_closest_color_current(const int r, const int g, const int b)
{
	return gr_find_closest_color_palette(r, g, b, gr_current_pal);
}

}
