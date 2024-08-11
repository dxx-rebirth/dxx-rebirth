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
	/* Do not use `palette.size() - 1` here.  That only works in compilers
	 * which implement P2280R4, due to use of a reference parameter in a
	 * constant expression, even though the value referenced has no effect on
	 * the result of `size()`.
	 *
	 * <gcc-14 rejects it.
	 * >=gcc-14 accept it.
	 * <clang-16: untested
	 * clang-16: fails
	 * clang-17: fails
	 * >clang-17: untested
	 */
	for (auto &&[candidate_idx, candidate_rgb] : enumerate(std::span(palette).first<255>()))
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

void gr_read_palette_file_bytes(RAIIPHYSFS_File fp)
{
	if (const auto fsize{PHYSFS_fileLength(fp)}; unlikely(fsize != 9472))
	{
		/* This can only happen if an ill-formed data file is provided. */
		gr_palette = {};
		gr_fade_table = {};
		return;
	}
	PHYSFSX_readBytes(fp, gr_palette, sizeof(gr_palette[0]) * gr_palette.size());
	PHYSFSX_readBytes(fp, gr_fade_table, 256*34);
}

void gr_read_palette_file(RAIIPHYSFS_File fp)
{
	Num_computed_colors = 0;	//	Flush palette cache.
	gr_read_palette_file_bytes(std::move(fp));

	// This is the TRANSPARENCY COLOR
	for (auto &i : gr_fade_table)
		i[255] = 255;
}

}

}

namespace dsx {

namespace {

RAIIPHYSFS_File gr_open_palette_file(const char *const filename)
{
	if (auto [fp_requested_palette, physfserr_requested_palette]{PHYSFSX_openReadBuffered(filename)}; fp_requested_palette)
	{
		/* gcc interprets `return fp_requested_palette;` to move
		 * `fp_requested_palette` to the caller.
		 *
		 * clang interprets `return fp_requested_palette;` to copy
		 * `fp_requested_palette` to the caller, then errors out because the
		 * copy constructor is inaccessible.
		 *
		 * Use an explicit `std::move` to cause clang to use the move
		 * constructor.  From inspection, gcc generates the same code with an
		 * explicit `std::move` as with an implicit move, so this is not a
		 * pessimizing move.  However, for future proofing, only call
		 * `std::move` when using clang.
		 *
		 * clang version results:
		 * <clang-16: untested
		 * clang-16: fails
		 * clang-17: fails
		 * >clang-17: untested
		 */
		return
#ifdef __clang__
			std::move
#endif
			(fp_requested_palette);
	}
	else
	{
#if defined(DXX_BUILD_DESCENT_I)
		Error("Failed to open palette file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr_requested_palette));
#elif defined(DXX_BUILD_DESCENT_II)
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
		static char default_level_palette[]{DEFAULT_LEVEL_PALETTE};
		if (auto [fp_fallback_default_palette, physfserr_fallback_default_palette]{PHYSFSX_openReadBuffered_updateCase(default_level_palette)}; fp_fallback_default_palette)
		{
			return
#ifdef __clang__
				std::move
#endif
				(fp_fallback_default_palette);
		}
		else
			Error("Failed to open both palette file <%s> and default palette file <" DEFAULT_LEVEL_PALETTE ">: (\"%s\", \"%s\")", filename, PHYSFS_getErrorByCode(physfserr_requested_palette), PHYSFS_getErrorByCode(physfserr_fallback_default_palette));
#endif
	}
}

}

#if defined(DXX_BUILD_DESCENT_II)
void gr_copy_palette(palette_array_t &gr_palette, const palette_array_t &pal)
{
	gr_palette = pal;

	        Num_computed_colors = 0;
}
#endif

void gr_use_palette_table(const char * filename )
{
	gr_read_palette_file(gr_open_palette_file(filename));
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

color_palette_index gr_find_closest_color_current(const int r, const int g, const int b)
{
	return gr_find_closest_color_palette(r, g, b, gr_current_pal);
}

}
