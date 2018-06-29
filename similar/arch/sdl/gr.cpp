/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL video functions.
 *
 */

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <SDL.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "game.h"
#include "console.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "vers_id.h"
#include "gamefont.h"
#include "args.h"
#include "config.h"
#include "palette.h"

#include "compiler-make_unique.h"

using std::min;

namespace dcx {

static int sdl_video_flags = SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
static SDL_Surface *screen, *canvas;
static int gr_installed;

void gr_flip()
{
	SDL_Rect src, dest;

	dest.x = src.x = dest.y = src.y = 0;
	dest.w = src.w = canvas->w;
	dest.h = src.h = canvas->h;

	SDL_BlitSurface(canvas, &src, screen, &dest);
	SDL_Flip(screen);
}

// returns possible (fullscreen) resolutions if any.
uint_fast32_t gr_list_modes(array<screen_mode, 50> &gsmodes)
{
	SDL_Rect** modes;
	int modesnum = 0;
	int sdl_check_flags = sdl_video_flags;

	sdl_check_flags |= SDL_FULLSCREEN; // always use Fullscreen as lead.

	modes = SDL_ListModes(NULL, sdl_check_flags);

	if (modes == reinterpret_cast<SDL_Rect **>(0)) // check if we get any modes - if not, return 0
		return 0;


	if (modes == reinterpret_cast<SDL_Rect**>(-1))
	{
		return 0; // can obviously use any resolution... strange!
	}
	else
	{
		for (int i = 0; modes[i]; ++i)
		{
			if (modes[i]->w > 0xFFF0 || modes[i]->h > 0xFFF0 // resolutions saved in 32bits. so skip bigger ones (unrealistic in 2010) (kreatordxx - made 0xFFF0 to kill warning)
				|| modes[i]->w < 320 || modes[i]->h < 200) // also skip everything smaller than 320x200
				continue;
			gsmodes[modesnum].width = modes[i]->w;
			gsmodes[modesnum].height = modes[i]->h;
			modesnum++;
			if (modesnum >= gsmodes.size()) // that really seems to be enough big boy.
				break;
		}
		return modesnum;
	}
}

}

namespace dsx {

int gr_set_mode(screen_mode mode)
{
	screen=NULL;

	SDL_WM_SetCaption(DESCENT_VERSION, DXX_SDL_WINDOW_CAPTION);
	SDL_WM_SetIcon( SDL_LoadBMP( DXX_SDL_WINDOW_ICON_BITMAP ), NULL );

	const auto sdl_video_flags = ::sdl_video_flags;
	const auto DbgBpp = CGameArg.DbgBpp;
	if(SDL_VideoModeOK(SM_W(mode), SM_H(mode), DbgBpp, sdl_video_flags))
	{
	}
	else
	{
		con_printf(CON_URGENT,"Cannot set %hux%hu. Fallback to 640x480", SM_W(mode), SM_H(mode));
		mode.width = 640;
		mode.height = 480;
		Game_screen_mode = mode;
	}
	const unsigned w = SM_W(mode), h = SM_H(mode);
	screen = SDL_SetVideoMode(w, h, DbgBpp, sdl_video_flags);

	if (screen == NULL)
	{
		Error("Could not set %dx%dx%d video mode\n", w, h, DbgBpp);
		exit(1);
	}

	canvas = SDL_CreateRGBSurface(sdl_video_flags, w, h, 8, 0, 0, 0, 0);
	if (canvas == NULL)
	{
		Error("Could not create canvas surface\n");
		exit(1);
	}

	*grd_curscreen = {};
	grd_curscreen->set_screen_width_height(w, h);
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->get_screen_width() * GameCfg.AspectX, grd_curscreen->get_screen_height() * GameCfg.AspectY);
	gr_init_canvas(grd_curscreen->sc_canvas, reinterpret_cast<unsigned char *>(canvas->pixels), bm_mode::linear, w, h);
	window_update_canvases();
	gr_set_default_canvas();

	SDL_ShowCursor(0);
	gamefont_choose_game_font(w,h);
	gr_palette_load(gr_palette);
	gr_remap_color_fonts();

	return 0;
}

}

namespace dcx {

int gr_check_fullscreen(void)
{
	return !!(sdl_video_flags & SDL_FULLSCREEN);
}

void gr_toggle_fullscreen()
{
	sdl_video_flags ^= SDL_FULLSCREEN;
	const int WindowMode = !(sdl_video_flags & SDL_FULLSCREEN);
	CGameCfg.WindowMode = WindowMode;
	gr_remap_color_fonts();
	SDL_WM_ToggleFullScreen(screen);
}

}

namespace dsx {

int gr_init()
{
	// Only do this function once!
	if (gr_installed==1)
		return -1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Error("SDL library video initialisation failed: %s.",SDL_GetError());
	}

	grd_curscreen = make_unique<grs_screen>();
	*grd_curscreen = {};

	if (!CGameCfg.WindowMode && !CGameArg.SysWindow)
		sdl_video_flags|=SDL_FULLSCREEN;

	if (CGameArg.SysNoBorders)
		sdl_video_flags|=SDL_NOFRAME;

	if (CGameArg.DbgSdlHWSurface)
		sdl_video_flags|=SDL_HWSURFACE;

	if (CGameArg.DbgSdlASyncBlit)
		sdl_video_flags|=SDL_ASYNCBLIT;

	// Set the mode.
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	gr_installed = 1;

	return 0;
}

void gr_close()
{
	if (gr_installed==1)
	{
		gr_installed = 0;
		grd_curscreen.reset();
		SDL_ShowCursor(1);
		SDL_FreeSurface(canvas);
	}
}

}

namespace dcx {

// Palette functions follow.
static int last_r=0, last_g=0, last_b=0;

void gr_palette_step_up( int r, int g, int b )
{
	palette_array_t &p = gr_palette;
	SDL_Palette *palette;

	if ( (r==last_r) && (g==last_g) && (b==last_b) )
		return;

	last_r = r;
	last_g = g;
	last_b = b;

	palette = canvas->format->palette;

	if (palette == NULL)
		return; // Display is not palettised

	array<SDL_Color, 256> colors{};
	for (int i=0; i<256; i++)
	{
		const auto ir = static_cast<int>(p[i].r) + r + gr_palette_gamma;
		colors[i].r = std::min(std::max(ir, 0), 63) * 4;
		const auto ig = static_cast<int>(p[i].g) + g + gr_palette_gamma;
		colors[i].g = std::min(std::max(ig, 0), 63) * 4;
		const auto ib = static_cast<int>(p[i].b) + b + gr_palette_gamma;
		colors[i].b = std::min(std::max(ib, 0), 63) * 4;
	}
	SDL_SetColors(canvas, colors.data(), 0, colors.size());
}

void gr_palette_load( palette_array_t &pal )
{
	SDL_Palette *palette;
	array<uint8_t, 64> gamma;

	if (pal != gr_current_pal)
		SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 0, 0, 0));

	copy_bound_palette(gr_current_pal, pal);

	if (canvas == NULL)
		return;

	palette = canvas->format->palette;

	if (palette == NULL)
		return; // Display is not palettised

	for (int i=0;i<64;i++)
		gamma[i] = static_cast<int>((pow((static_cast<double>(14)/static_cast<double>(32)), 1.0)*i) + 0.5);

	array<SDL_Color, 256> colors{};
	for (int i = 0, j = 0; j < 256; j++)
	{
		int c;
		c = gr_find_closest_color(gamma[gr_palette[j].r],gamma[gr_palette[j].g],gamma[gr_palette[j].b]);
		gr_fade_table[14][j] = c;
		colors[j].r = (min(gr_current_pal[i].r + gr_palette_gamma, 63)) * 4;
		colors[j].g = (min(gr_current_pal[i].g + gr_palette_gamma, 63)) * 4;
		colors[j].b = (min(gr_current_pal[i].b + gr_palette_gamma, 63)) * 4;
		i++;
	}

	SDL_SetColors(canvas, colors.data(), 0, colors.size());
	init_computed_colors();
	gr_remap_color_fonts();
}

void gr_palette_read(palette_array_t &pal)
{
	SDL_Palette *palette;
	unsigned i;

	palette = canvas->format->palette;

	if (palette == NULL)
		return; // Display is not palettised

	for (i = 0; i < 256; i++)
	{
		pal[i].r = palette->colors[i].r / 4;
		pal[i].g = palette->colors[i].g / 4;
		pal[i].b = palette->colors[i].b / 4;
	}
}

}
