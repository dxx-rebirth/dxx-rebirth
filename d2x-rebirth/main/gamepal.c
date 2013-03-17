/*
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

#include "fix.h"
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

char Current_level_palette[FILENAME_LEN];

extern int Color_0_31_0;

char last_palette_loaded[FILENAME_LEN]="";
char last_palette_loaded_pig[FILENAME_LEN]="";

//load a palette by name. returns 1 if new palette loaded, else 0
//if used_for_level is set, load pig, etc.
//if no_change_screen is set, the current screen does not get remapped,
//and the hardware palette does not get changed
int load_palette(char *name,int used_for_level,int no_change_screen)
{
	char pigname[FILENAME_LEN];
	ubyte old_pal[256*3];

	//special hack to tell that palette system about a pig that's been loaded elsewhere
	if (used_for_level == -2) {
		strncpy(last_palette_loaded_pig,name,sizeof(last_palette_loaded_pig));
		return 1;
	}

	if (name==NULL)
		name = last_palette_loaded_pig;

	if (used_for_level && d_stricmp(last_palette_loaded_pig,name) != 0) {

		d_splitpath(name,NULL,NULL,pigname,NULL);
		strcat(pigname,".pig");
		//if not editor, load pig first so small install message can come
		//up in old palette.  If editor version, we must load the pig after
		//the palette is loaded so we can remap new textures.
		#ifndef EDITOR
		piggy_new_pigfile(pigname);
		#endif
	}

	if (d_stricmp(last_palette_loaded,name) != 0) {

		memcpy(old_pal,gr_palette,sizeof(old_pal));

		strncpy(last_palette_loaded,name,sizeof(last_palette_loaded));

		gr_use_palette_table(name);

		if (Game_wind && !no_change_screen)
			gr_remap_bitmap_good( &grd_curscreen->sc_canvas.cv_bitmap, old_pal, -1, -1 );

		if (!no_change_screen)
			gr_palette_load(gr_palette);

		newmenu_free_background(); // palette changed! free menu!
		gr_remap_color_fonts();
		gr_remap_mono_fonts();

		Color_0_31_0 = -1;		//for gauges
	}


	if (used_for_level && d_stricmp(last_palette_loaded_pig,name) != 0) {

		strncpy(last_palette_loaded_pig,name,sizeof(last_palette_loaded_pig));

		#ifdef EDITOR
		piggy_new_pigfile(pigname);
		#endif

		texmerge_flush();
		rle_cache_flush();
	}

	return 1;
}

