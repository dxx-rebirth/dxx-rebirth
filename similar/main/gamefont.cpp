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
 * Fonts for the game.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "gr.h"
#include "dxxerror.h"
#include <string.h>
#include "args.h"
#include "physfsx.h"
#include "gamefont.h"
#include "mission.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "config.h"

#include "d_enumerate.h"
#include "partial_range.h"

namespace dcx {

enumerated_array<loaded_game_font, MAX_FONTS, gamefont_index> Gamefonts;

namespace {

	/* Low-resolution fonts need 11 characters for the name, plus one for the
	 * null.  High-resolution fonts need 12 characters for the name, plus one
	 * for the null.  Round up to 16 for alignment and easy indexing math.
	 */
using font_filename = std::array<char, 16>;

/* Both Descent 1 and Descent 2 ship fonts with these filenames.
 * For Descent 1 PC and for Descent 2, these fonts are low-resolution files.
 * For Descent 1 Mac, `font1-1.fnt` is low-resolution, and the rest are high
 * resolution.
 */
constexpr enumerated_array<font_filename, MAX_FONTS, gamefont_index> Gamefont_filenames_l{{{
	{{"font1-1.fnt"}}, // Font 0
	{{"font2-1.fnt"}}, // Font 1
	{{"font2-2.fnt"}}, // Font 2
	{{"font2-3.fnt"}}, // Font 3
	{{"font3-1.fnt"}}  // Font 4
}}};

/* Only Descent 2 ships fonts with these filenames.  However, D1X-Rebirth has
 * the ability to read high resolution fonts, and should prefer them when the
 * user's window dimensions are 640x480 or higher.  Therefore, these filenames
 * are defined in both games, and are probed in both games, even though
 * D1X-Rebirth will only find these files if the user has provided them outside
 * the game's original data files.  D2X-Rebirth should always have access to
 * these files.
 *
 * In both games, the user can choose not to load these files, via
 * `CGameArg.GfxSkipHiresFNT`.
 */
constexpr enumerated_array<font_filename, MAX_FONTS, gamefont_index> Gamefont_filenames_h{{{
	{{"font1-1h.fnt"}}, // Font 0
	{{"font2-1h.fnt"}}, // Font 1
	{{"font2-2h.fnt"}}, // Font 2
	{{"font2-3h.fnt"}}, // Font 3
	{{"font3-1h.fnt"}}  // Font 4
}}};

static int Gamefont_installed;

}

}

font_x_scale_proportion FNTScaleX(1);
font_y_scale_proportion FNTScaleY(1);

namespace {

static void gamefont_unloadfont(loaded_game_font &g)
{
	if (g.font)
	{
		g.cur = loaded_game_font::font_index::None;
		g.font.reset();
	}
}

static void gamefont_loadfont(grs_canvas &canvas, loaded_game_font &g, const loaded_game_font::font_index fi)
{
	auto &fc{g.fontconf[fi]};
	g.font = gr_init_font(canvas, fc.name);
	g.cur=fi;
}

}

void gamefont_choose_game_font(int scrx,int scry){
	if (!Gamefont_installed) return;

	int close=-1;
	auto m{loaded_game_font::font_index::None};
	for (const auto &&[gf, fc] : enumerate(Gamefonts))
	{
		for (const auto &&[i, f] : enumerate(partial_range(fc.fontconf, fc.total_fonts_loaded)))
			if ((scrx >= f.expected_screen_resolution_x && close < f.expected_screen_resolution_x) && (scry >= f.expected_screen_resolution_y && close < f.expected_screen_resolution_y))
			{
				close = f.expected_screen_resolution_x;
				m=i;
			}
		if (m == loaded_game_font::font_index::None)
			Error("no gamefont found for %ix%i\n",scrx,scry);

#if DXX_USE_OGL
	if (!CGameArg.OglFixedFont)
	{
		// if there's no texture filtering, scale by int
		auto &f = fc.fontconf[m];
		if (CGameCfg.TexFilt == opengl_texture_filter::classic)
		{
			FNTScaleX.reset(scrx / f.expected_screen_resolution_x);
			FNTScaleY.reset(scry / f.expected_screen_resolution_y);
		}
		else
		{
			FNTScaleX.reset(static_cast<float>(scrx) / f.expected_screen_resolution_x);
			FNTScaleY.reset(static_cast<float>(scry) / f.expected_screen_resolution_y);
		}

		// keep proportions
#if DXX_BUILD_DESCENT == 1
#define DXX_FONT_SCALE_MULTIPLIER	1
#elif DXX_BUILD_DESCENT == 2
#define DXX_FONT_SCALE_MULTIPLIER	100
#endif
		if (FNTScaleY*DXX_FONT_SCALE_MULTIPLIER < FNTScaleX*DXX_FONT_SCALE_MULTIPLIER)
			FNTScaleX.reset(FNTScaleY.operator float());
		else if (FNTScaleX*DXX_FONT_SCALE_MULTIPLIER < FNTScaleY*DXX_FONT_SCALE_MULTIPLIER)
			FNTScaleY.reset(FNTScaleX.operator float());
	}
#endif
		gamefont_loadfont(*grd_curcanv, fc, m);
	}
}

namespace dsx {

namespace {

/* For Descent 1, one caller needs to detect whether the load succeeded, so
 * return a `bool` reporting success versus failure.
 * For Descent 2, no caller needs to detect whether the load succeeded, so
 * return `void`.
 */
#if DXX_BUILD_DESCENT == 1
#define DXX_addfontconf_return_type bool
#define DXX_addfontconf_return_value
#elif DXX_BUILD_DESCENT == 2
#define DXX_addfontconf_return_type void
#define DXX_addfontconf_return_value (void)
#endif
static DXX_addfontconf_return_type addfontconf(loaded_game_font &fc, const uint16_t expected_screen_resolution_x, const uint16_t expected_screen_resolution_y, const font_filename &fn)
#undef DXX_addfontconf_return_type
{
	if (!PHYSFS_exists(fn.data()))
		/* File not found - load failed */
		return DXX_addfontconf_return_value(false);

	auto &f = fc.fontconf[(loaded_game_font::font_index{fc.total_fonts_loaded})];
	++ fc.total_fonts_loaded;
	f.expected_screen_resolution_x = {expected_screen_resolution_x};
	f.expected_screen_resolution_y = {expected_screen_resolution_y};
	f.name = fn;
	/* File found - presume the load will succeed when the file is eventually
	 * opened and read.
	 */
	return DXX_addfontconf_return_value(true);
#undef DXX_addfontconf_return_value
}

}

void gamefont_init()
{
	if (Gamefont_installed)
		return;

	Gamefont_installed = 1;

	for (auto &&[i, gf] : enumerate(Gamefonts))
	{
		gf.font = nullptr;
		constexpr uint16_t high_resolution_x{640};
		constexpr uint16_t high_resolution_y{480};
		constexpr uint16_t low_resolution_x{320};
		constexpr uint16_t low_resolution_y{200};

#if DXX_BUILD_DESCENT == 1
		if (MacHog && (i != gamefont_index::big))
		{
			/* Mac fonts are high-resolution (except for the "big" one), even
			 * when stored in the low-resolution filename.  Try to add the Mac
			 * font first.  If successful, continue, since adding the standard
			 * high-resolution font would be redundant, and there is no
			 * low-resolution font available.
			 *
			 * On failure, fall back to trying to add a high-resolution font if
			 * the user has provided one.  These are not included in the normal
			 * game data, but users could supply them via an add-on.
			 */
			if (addfontconf(gf, high_resolution_x, high_resolution_y, Gamefont_filenames_l[i]))
				continue;
			if (!CGameArg.GfxSkipHiresFNT)
				addfontconf(gf, high_resolution_x, high_resolution_y, Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts if available
			/* Both filenames have now been tried, so continue.
			 */
			continue;
		}
#endif
		/* If the above special case does not apply, add the Descent 2
		 * high-resolution font first, then add the low-resolution font.
		 */
		if (!CGameArg.GfxSkipHiresFNT)
			addfontconf(gf, high_resolution_x, high_resolution_y, Gamefont_filenames_h[i]);
		addfontconf(gf, low_resolution_x, low_resolution_y, Gamefont_filenames_l[i]);
	}

	gamefont_choose_game_font(grd_curscreen->sc_canvas.cv_bitmap.bm_w,grd_curscreen->sc_canvas.cv_bitmap.bm_h);
}
}

void gamefont_close()
{
	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (auto &gf : Gamefonts)
		gamefont_unloadfont(gf);
}
