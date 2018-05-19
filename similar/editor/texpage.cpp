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
 * Routines for displaying texture pages
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "inferno.h"
#include "gameseg.h"
#include "screens.h"			// For GAME_SCREEN?????
#include "editor.h"			// For TMAP_CURBOX??????
#include "gr.h"				// For canves, font stuff
#include "ui.h"				// For UI_GADGET stuff
#include "textures.h"		// For NumTextures
#include "dxxerror.h"
#include "key.h"
#include "event.h"
#include "gamesave.h"
#include "mission.h"

#include "texpage.h"
#include "piggy.h"

#include "compiler-range_for.h"

constexpr std::integral_constant<unsigned, 12> TMAPS_PER_PAGE{};

static array<std::unique_ptr<UI_GADGET_USERBOX>, TMAPS_PER_PAGE> TmapBox;
static std::unique_ptr<UI_GADGET_USERBOX> TmapCurrent;

int CurrentTexture = 0;		// Used globally

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_TEXTURE_INITIALIZER(D1, D2)	D1
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_TEXTURE_INITIALIZER(D1, D2)	D2
#endif

int TextureLights = DXX_TEXTURE_INITIALIZER(263, 275);
int TextureEffects = DXX_TEXTURE_INITIALIZER(327, 308);
int TextureMetals = DXX_TEXTURE_INITIALIZER(156, 202);

namespace {

class texture_dialog
{
public:
	std::unique_ptr<UI_GADGET_BUTTON> prev_texture, next_texture, first_texture, metal_texture, light_texture, effects_texture;
};

}

static texture_dialog texpage_dialog;

static int TexturePage = 0;

static grs_subcanvas_ptr TmapnameCanvas;
static void texpage_print_name(d_fname name)
{
	name.back() = 0;
	
    gr_set_current_canvas( TmapnameCanvas );
    gr_string(*grd_curcanv, *grd_curcanv->cv_font, 0, 0, name);			  
}

//Redraw the list of textures, based on TexturePage
static void texpage_redraw()
{
	for (int i = 0;  i < TMAPS_PER_PAGE; i++)
	{
		gr_set_current_canvas(TmapBox[i]->canvas);
		if (i + TexturePage*TMAPS_PER_PAGE < NumTextures)
		{
			PIGGY_PAGE_IN(Textures[i + TexturePage*TMAPS_PER_PAGE]);
			gr_ubitmap(*grd_curcanv, GameBitmaps[Textures[i + TexturePage*TMAPS_PER_PAGE].index]);
		} else 
			gr_clear_canvas(*grd_curcanv, CGREY);
	}
}

//shows the current texture, updating the window and printing the name, base
//on CurrentTexture
static void texpage_show_current()
{
	gr_set_current_canvas(TmapCurrent->canvas);
	PIGGY_PAGE_IN(Textures[CurrentTexture]);
	gr_ubitmap(*grd_curcanv, GameBitmaps[Textures[CurrentTexture].index]);
	texpage_print_name( TmapInfo[CurrentTexture].filename );
}

int texpage_goto_first()
{
	TexturePage=0;
	texpage_redraw();
	return 1;
}

static int texpage_goto_metals()
{

	TexturePage=TextureMetals/TMAPS_PER_PAGE;
	texpage_redraw();
	return 1;
}


// Goto lights (paste ons)
static int texpage_goto_lights()
{
	TexturePage=TextureLights/TMAPS_PER_PAGE;
	texpage_redraw();
	return 1;
}

static int texpage_goto_effects()
{
	TexturePage=TextureEffects/TMAPS_PER_PAGE;
	texpage_redraw();
	return 1;
}

static int texpage_goto_prev()
{
	if (TexturePage > 0) {
		TexturePage--;
		texpage_redraw();
	}
	return 1;
}

static int texpage_goto_next()
{
	if ((TexturePage + 1)*TMAPS_PER_PAGE < NumTextures)
	{
		TexturePage++;
		texpage_redraw();
	}
	return 1;
}

//NOTE:  this code takes the texture map number, not this index in the
//list of available textures.  There are different if there are holes in
//the list
int texpage_grab_current(int n)
{
	if ((n < 0) || (n >= NumTextures)) return 0;

	CurrentTexture = n;

	TexturePage = CurrentTexture / TMAPS_PER_PAGE;
	
	if (TexturePage*TMAPS_PER_PAGE < NumTextures)
		texpage_redraw();

	texpage_show_current();
	
	return 1;
}


// INIT TEXTURE STUFF

void texpage_init( UI_DIALOG * dlg )
{
	auto &t = texpage_dialog;
	t.prev_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 00, TMAPCURBOX_Y - 24, 30, 20, "<<", texpage_goto_prev );
	t.next_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 32, TMAPCURBOX_Y - 24, 30, 20, ">>", texpage_goto_next );

	t.first_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 00, TMAPCURBOX_Y - 48, 15, 20, "T", texpage_goto_first );
	t.metal_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 17, TMAPCURBOX_Y - 48, 15, 20, "M", texpage_goto_metals );
	t.light_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 34, TMAPCURBOX_Y - 48, 15, 20, "L", texpage_goto_lights );
	t.effects_texture = ui_add_gadget_button( dlg, TMAPCURBOX_X + 51, TMAPCURBOX_Y - 48, 15, 20, "E", texpage_goto_effects );

	for (int i=0;i<TMAPS_PER_PAGE;i++)
		TmapBox[i] = ui_add_gadget_userbox( dlg, TMAPBOX_X + (i/3)*(2+TMAPBOX_W), TMAPBOX_Y + (i%3)*(2+TMAPBOX_H), TMAPBOX_W, TMAPBOX_H);

	TmapCurrent = ui_add_gadget_userbox( dlg, TMAPCURBOX_X, TMAPCURBOX_Y, 64, 64 );

	TmapnameCanvas = gr_create_sub_canvas(grd_curscreen->sc_canvas, TMAPCURBOX_X , TMAPCURBOX_Y + TMAPBOX_H + 10, 100, 20);
}

void texpage_close()
{
	TmapnameCanvas.reset();
}


// DO TEXTURE STUFF

#define	MAX_REPLACEMENTS	32

struct replacement
{
	int	n, old;
};

int	Num_replacements=0;
static array<replacement, MAX_REPLACEMENTS> Replacement_list;

int texpage_do(const d_event &event)
{
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		gr_set_current_canvas( TmapnameCanvas );
		gr_set_curfont(*grd_curcanv, ui_small_font.get());
		gr_set_fontcolor(*grd_curcanv, CBLACK, CWHITE);
		
		texpage_redraw();
		
		// Don't reset the current tmap every time we go back to the editor.
		//	CurrentTexture = TexturePage*TMAPS_PER_PAGE;
		texpage_show_current();
		
		return 1;
	}

	for (int i=0; i<TMAPS_PER_PAGE; i++ ) {
		if (GADGET_PRESSED(TmapBox[i].get()) && (i + TexturePage*TMAPS_PER_PAGE < NumTextures))
		{
			CurrentTexture = i + TexturePage*TMAPS_PER_PAGE;
			texpage_show_current();

			if (keyd_pressed[KEY_LSHIFT]) {
				Replacement_list[Num_replacements].old = CurrentTexture;
			}

			if (keyd_pressed[KEY_LCTRL]) {
				Replacement_list[Num_replacements].n = CurrentTexture;
				Num_replacements++;
			}
			
			return 1;
		}
	}
	
	return 0;
}

void init_replacements(void)
{
	Num_replacements = 0;
}

void do_replacements(void)
{
	med_compress_mine();

	for (int replnum=0; replnum<Num_replacements; replnum++) {
		int	old_tmap_num, new_tmap_num;

		old_tmap_num = Replacement_list[replnum].old;
		new_tmap_num = Replacement_list[replnum].n;
		Assert(old_tmap_num >= 0);
		Assert(new_tmap_num >= 0);

		range_for (const auto &&segp, vmsegptr)
		{
			for (int sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
				const auto sidep = &segp->sides[sidenum];
				if (sidep->tmap_num == old_tmap_num) {
					sidep->tmap_num = new_tmap_num;
				}
				if ((sidep->tmap_num2 != 0) && ((sidep->tmap_num2 & 0x3fff) == old_tmap_num)) {
					if (new_tmap_num == 0) {
						Int3();	//	Error.  You have tried to replace a tmap_num2 with 
									//	the 0th tmap_num2 which is ILLEGAL!
					} else {
						sidep->tmap_num2 = new_tmap_num | (sidep->tmap_num2 & 0xc000);
					}
				}
			}
		}
	}

}

void do_replacements_all(void)
{
	for (int i = 0; i < Last_level; i++)
	{
		load_level(Level_names[i]);
		do_replacements();
		save_level(Level_names[i]);
	}

	for (int i = 0; i < -Last_secret_level; i++)
	{
		load_level(Secret_level_names[i]);
		do_replacements();
		save_level(Secret_level_names[i]);
	}

}

