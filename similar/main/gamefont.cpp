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

constexpr enumerated_array<std::array<char, 16>, MAX_FONTS, gamefont_index> Gamefont_filenames_l{{{
	{{"font1-1.fnt"}}, // Font 0
	{{"font2-1.fnt"}}, // Font 1
	{{"font2-2.fnt"}}, // Font 2
	{{"font2-3.fnt"}}, // Font 3
	{{"font3-1.fnt"}}  // Font 4
}}};

constexpr enumerated_array<std::array<char, 16>, MAX_FONTS, gamefont_index> Gamefont_filenames_h{{{
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

static void gamefont_loadfont(grs_canvas &canvas, const gamefont_index gf, const loaded_game_font::font_index fi)
{
	if (auto &fc{Gamefonts[gf].fontconf[fi]}; PHYSFS_exists(fc.name.data()))
	{
		gamefont_unloadfont(Gamefonts[gf]);
		Gamefonts[gf].font = gr_init_font(canvas, fc.name);
	}else {
		if (auto &g{Gamefonts[gf].font}; !g)
		{
			Gamefonts[gf].cur = loaded_game_font::font_index::None;
			g = gr_init_font(canvas, Gamefont_filenames_l[gf]);
		}
		return;
	}
	Gamefonts[gf].cur=fi;
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
		gamefont_loadfont(*grd_curcanv, gf, m);
	}
}

namespace {

static void addfontconf(const gamefont_index gf, const uint16_t expected_screen_resolution_x, const uint16_t expected_screen_resolution_y, const std::array<char, 16> &fn)
{
	if (!PHYSFS_exists(fn.data()))
		return;

	auto &fc = Gamefonts[gf];
	for (const auto &&[i, f] : enumerate(partial_range(fc.fontconf, fc.total_fonts_loaded)))
		if (f.expected_screen_resolution_x == expected_screen_resolution_x && f.expected_screen_resolution_y == expected_screen_resolution_y)
		{
			if (i == fc.cur)
				gamefont_unloadfont(Gamefonts[gf]);
			f.name = fn;
			if (i == fc.cur)
				gamefont_loadfont(*grd_curcanv, gf, i);
			return;
		}
	auto &f = fc.fontconf[(loaded_game_font::font_index{fc.total_fonts_loaded})];
	++ fc.total_fonts_loaded;
	f.expected_screen_resolution_x = {expected_screen_resolution_x};
	f.expected_screen_resolution_y = {expected_screen_resolution_y};
	f.name = fn;
}

}

namespace dsx {
void gamefont_init()
{
	if (Gamefont_installed)
		return;

	Gamefont_installed = 1;

	for (auto &&[i, gf] : enumerate(Gamefonts))
	{
		gf.font = nullptr;

		if (!CGameArg.GfxSkipHiresFNT)
			addfontconf(i,640,480,Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts if available
#if DXX_BUILD_DESCENT == 1
		if (MacHog && (i != gamefont_index::big))
			addfontconf(i,640,480,Gamefont_filenames_l[i]); // Mac fonts are hires (except for the "big" one)
		else
#endif
			addfontconf(i,320,200,Gamefont_filenames_l[i]);
	}

	gamefont_choose_game_font(grd_curscreen->sc_canvas.cv_bitmap.bm_w,grd_curscreen->sc_canvas.cv_bitmap.bm_h);
}
}

void gamefont_close()
{
	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (auto &&[idx, gf] : enumerate(Gamefonts))
	{
		(void)gf;
		gamefont_unloadfont(Gamefonts[idx]);
	}
}
