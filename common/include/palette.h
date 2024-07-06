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
 * Protoypes for palette functions
 *
 */

#pragma once

#include "pstypes.h"
#include <cstddef>
#include <cstdint>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include <array>

struct rgb_t {
	ubyte r,g,b;
	[[nodiscard]]
	constexpr bool operator==(const rgb_t &) const = default;
};

typedef uint8_t color_t;

namespace dcx {

struct palette_array_t : public std::array<rgb_t, 256> {};

#ifdef DXX_BUILD_DESCENT_II
#define DEFAULT_LEVEL_PALETTE "groupa.256" //don't confuse with D2_DEFAULT_PALETTE
#endif

void copy_bound_palette(palette_array_t &d, const palette_array_t &s);
void gr_palette_set_gamma(uint8_t gamma);
uint8_t gr_palette_get_gamma();
void gr_palette_load( palette_array_t &pal );
color_t gr_find_closest_color_current( int r, int g, int b );
#if !DXX_USE_OGL
void gr_palette_read(palette_array_t &palette);
#endif
void reset_computed_colors();
extern ubyte gr_palette_gamma;
extern palette_array_t gr_current_pal;
extern palette_array_t gr_palette;

using color_palette_index = uint8_t;

static constexpr const rgb_t &PAL2T(const color_palette_index c)
{
	return gr_palette[static_cast<std::size_t>(c)];
}

}
