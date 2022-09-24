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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for gamepal.c
 *
 */

#pragma once
#include <cstddef>

namespace dcx {
template <std::size_t>
struct PHYSFSX_gets_line_t;
}

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
#include <span>
#include "inferno.h"

namespace dsx {
#define D2_DEFAULT_PALETTE "default.256"
#define MENU_PALETTE    "default.256"

extern char last_palette_loaded[FILENAME_LEN];
extern PHYSFSX_gets_line_t<FILENAME_LEN> Current_level_palette;
extern char last_palette_loaded_pig[FILENAME_LEN];

// load a palette by name. returns 1 if new palette loaded, else 0
// if used_for_level is set, load pig, etc.
// if no_change_screen is set, the current screen does not get
// remapped, and the hardware palette does not get changed
int load_palette(std::span<const char> name, int used_for_level, int no_change_screen);
}
#endif
