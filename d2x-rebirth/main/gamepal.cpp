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
 * Functions for loading palettes
 *
 */

#include <string.h>
#include <stdlib.h>

#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "palette.h"
#include "rle.h"
#include "inferno.h"
#include "game.h"
#include "gamepal.h"
#include "mission.h"
#include "newmenu.h"
#include "texmerge.h"
#include "piggy.h"
#include "strutil.h"
#include "gauges.h"

namespace dsx {

char last_palette_loaded[FILENAME_LEN]="";
std::array<char, FILENAME_LEN> last_palette_loaded_pig;

//special hack to tell that palette system about a pig that's been loaded elsewhere
void load_palette_for_pig(const std::span<const char> name)
{
	/* Never write to the last byte, so that the array is always null terminated. */
	std::memcpy(last_palette_loaded_pig.data(), name.data(), std::min(name.size(), last_palette_loaded_pig.size() - 1));
}

//load a palette by name. returns 1 if new palette loaded, else 0
//if used_for_level is set, load pig, etc.
//if no_change_screen is set, the current screen does not get remapped,
//and the hardware palette does not get changed
int load_palette(const std::span<const char> name, const load_palette_use used_for_level, const load_palette_change_screen change_screen)
{
	char pigname[FILENAME_LEN];
	palette_array_t old_pal;

	if (used_for_level != load_palette_use::background && strcmp(last_palette_loaded_pig.data(), name.data()) != 0)
	{
		const auto path = d_splitpath(name.data());
		snprintf(pigname, sizeof(pigname), "%.*s.pig", DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start);
		//if not editor, load pig first so small install message can come
		//up in old palette.  If editor version, we must load the pig after
		//the palette is loaded so we can remap new textures.
#if !DXX_USE_EDITOR
		piggy_new_pigfile(pigname);
		#endif
	}

	if (d_stricmp(last_palette_loaded, name.data()) != 0)
	{
		old_pal = gr_palette;

		std::memcpy(last_palette_loaded, name.data(), std::min(name.size(), sizeof(last_palette_loaded) - 1));
		gr_use_palette_table(name.data());

		if (change_screen == load_palette_change_screen::immediate)
		{
			if (Game_wind)
				gr_remap_bitmap_good(grd_curscreen->sc_canvas.cv_bitmap, old_pal, -1, -1);
			gr_palette_load(gr_palette);
		}

		newmenu_free_background(); // palette changed! free menu!
		gr_remap_color_fonts();

		Color_0_31_0 = -1;		//for gauges
	}


	if (used_for_level != load_palette_use::background && strcmp(last_palette_loaded_pig.data(), name.data()) != 0)
	{
		std::memcpy(last_palette_loaded_pig.data(), name.data(), std::min(name.size(), last_palette_loaded_pig.size() - 1));

#if DXX_USE_EDITOR
		piggy_new_pigfile(pigname);
		#endif

		texmerge_flush();
		rle_cache_flush();
	}

	return 1;
}

}
