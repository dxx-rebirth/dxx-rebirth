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


#include <conf.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "palette.h"
#include "rle.h"

#include "inferno.h"
#include "gamepal.h"
#include "mission.h"
#include "newmenu.h"
#include "texmerge.h"
#include "piggy.h"
#include "strutil.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

extern void g3_remap_interp_colors();

char Current_level_palette[FILENAME_LEN];

extern int Color_0_31_0, HUD_color;

//give a filename a new extension
void change_filename_ext( char *dest, char *src, char *ext )
{
	char *p;

	strcpy (dest, src);

	if (ext[0] == '.')
		ext++;

	p = strchr(dest,'.');
	if (!p) {
		p = dest + strlen(dest);
		*p = '.';
	}

	strcpy(p+1,ext);
}

//background for boxed messages
typedef struct bkg {
	short x, y, w, h;			// The location of the menu.
	grs_bitmap * bmp;			// The background under the menu.
} bkg;

extern bkg bg;

void load_background_bitmap(void);

char last_palette_loaded[FILENAME_LEN]="";
char last_palette_loaded_pig[FILENAME_LEN]="";

ubyte last_palette_for_color_fonts[768];

void remap_fonts_and_menus(int do_fadetable_hack)
{
	nm_remap_background();
	gr_remap_color_fonts();

	if (do_fadetable_hack) {
		int i;
		float g = 1.0;
		double intensity;
		ubyte gamma[64];

		intensity = (double)(14)/(double)(32);
		for (i=0;i<64;i++)
			gamma[i] = (int)((pow(intensity, 1.0/g)*i) + 0.5);
		for (i=0;i<256;i++) {
			int c;
			c = gr_find_closest_color(gamma[gr_palette[i*3]],gamma[gr_palette[i*3+1]],gamma[gr_palette[i*3+2]]);
			gr_fade_table[14*256+i] = c;
		}
	}

	memcpy(last_palette_for_color_fonts,gr_palette,sizeof(last_palette_for_color_fonts));
}

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

	if (used_for_level && stricmp(last_palette_loaded_pig,name) != 0) {

		_splitpath(name,NULL,NULL,pigname,NULL);
		strcat(pigname,".PIG");

		//if not editor, load pig first so small install message can come
		//up in old palette.  If editor version, we must load the pig after
		//the palette is loaded so we can remap new textures.
		#ifndef EDITOR
		piggy_new_pigfile(pigname);
		#endif
	}

	if (stricmp(last_palette_loaded,name) != 0) {

		memcpy(old_pal,gr_palette,sizeof(old_pal));

		strncpy(last_palette_loaded,name,sizeof(last_palette_loaded));

		gr_use_palette_table(name);

		if (Function_mode == FMODE_GAME && !no_change_screen)
			gr_remap_bitmap_good( &grd_curscreen->sc_canvas.cv_bitmap, old_pal, -1, -1 );

#if defined(POLY_ACC)
        if (bg.bmp && bg.bmp->bm_type == BM_LINEAR)
#else
        if (bg.bmp)
#endif
            gr_remap_bitmap_good( bg.bmp, old_pal, -1, -1 );

		if (!gr_palette_faded_out && !no_change_screen)
			gr_palette_load(gr_palette);

		remap_fonts_and_menus(0);

		Color_0_31_0 = -1;		//for gauges
		HUD_color = -1;

		load_background_bitmap();

		g3_remap_interp_colors();
	}


	if (used_for_level && stricmp(last_palette_loaded_pig,name) != 0) {

		strncpy(last_palette_loaded_pig,name,sizeof(last_palette_loaded_pig));

		#ifdef EDITOR
		piggy_new_pigfile(pigname);
		#endif

		texmerge_flush();
		rle_cache_flush();
	}

	return 1;
}

