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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvs/cvsroot/d2x/main/editor/texpage.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Routines for displaying texture pages
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:04:32  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:51  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.14  1995/01/14  19:18:02  john
 * First version of object paging.
 * 
 * Revision 1.13  1994/11/27  23:17:15  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.12  1994/11/23  15:49:00  mike
 * fix bug in tmapnum == 0 always getting reassigned.
 * 
 * Revision 1.11  1994/11/23  12:17:34  mike
 * changing texture maps in all mines.
 * 
 * Revision 1.10  1994/11/19  00:04:42  john
 * Changed some shorts to ints.
 * 
 * Revision 1.9  1994/11/16  17:59:36  john
 * Fixed bug with writing to canvas before initing it.
 * 
 * Revision 1.8  1994/11/16  13:15:21  matt
 * Fixed grab bug, and cleaned up code
 * 
 * Revision 1.7  1994/08/05  12:26:37  matt
 * Fixed overplot problem with texture names
 * 
 * Revision 1.6  1994/04/11  12:01:58  yuan
 * Fixed resetting to first texture on page annoyance.
 * 
 * Revision 1.5  1994/04/01  11:15:53  yuan
 * Added objects to objpage. Added buttons for easier tmap scrolling.
 * Objects are selected fully from objpage and add object menu or pad.
 * 
 * Revision 1.4  1994/03/15  16:33:37  yuan
 * Fixed bm loader (might have some changes in walls and switches)
 * 
 * Revision 1.3  1993/12/16  17:25:46  john
 * Moved texture and object selection to texpage and objpage
 * 
 * Revision 1.2  1993/12/16  15:57:39  john
 * moved texture selection stuff to texpage.c
 * 
 * Revision 1.1  1993/12/16  15:06:56  john
 * Initial revision
 * 
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef RCS
static char rcsid[] = "$Id: texpage.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include "inferno.h"
#include "gameseg.h"
#include "screens.h"			// For GAME_SCREEN?????
#include "editor.h"			// For TMAP_CURBOX??????
#include "gr.h"				// For canves, font stuff
#include "ui.h"				// For UI_GADGET stuff
#include "textures.h"		// For NumTextures
#include "error.h"
#include "key.h"
#include "mono.h"
#include "gamesave.h"

#include "texpage.h"
#include "piggy.h"

#define TMAPS_PER_PAGE 12

static UI_GADGET_USERBOX * TmapBox[TMAPS_PER_PAGE];
static UI_GADGET_USERBOX * TmapCurrent;

int CurrentTmap = 0;		// Used globally
int CurrentTexture = 0;		// Used globally

int TextureLights;
int TextureEffects;
int TextureMetals;

static int TexturePage = 0;

static grs_canvas * TmapnameCanvas;
static char tmap_filename[13];

static void texpage_print_name( char name[13] ) 
{
	 int w,h,aw;
	int i;

	for (i=strlen(name);i<12;i++)
		name[i]=' ';
	name[i]=0;
	
    gr_set_current_canvas( TmapnameCanvas );
    gr_get_string_size( name, &w, &h, &aw );
    gr_string( 0, 0, name );			  
}

static void texpage_display_name( char *format, ... ) 
{
	va_list ap;

	va_start(ap, format);
   vsprintf(tmap_filename, format, ap);
	va_end(ap);

   texpage_print_name(tmap_filename);
}

//Redraw the list of textures, based on TexturePage
void texpage_redraw()
{
	int i;

	for (i=0;  i<TMAPS_PER_PAGE; i++ )
		{
		gr_set_current_canvas(TmapBox[i]->canvas);
		if (i+TexturePage*TMAPS_PER_PAGE < Num_tmaps )	{
			PIGGY_PAGE_IN( Textures[TmapList[i+TexturePage*TMAPS_PER_PAGE]]);
			gr_ubitmap(0,0, &GameBitmaps[Textures[TmapList[i+TexturePage*TMAPS_PER_PAGE]].index]);
		} else 
			gr_clear_canvas( CGREY );
		}
}

//shows the current texture, updating the window and printing the name, base
//on CurrentTexture
void texpage_show_current()
{
	gr_set_current_canvas(TmapCurrent->canvas);
	PIGGY_PAGE_IN(Textures[CurrentTexture]);
	gr_ubitmap(0,0, &GameBitmaps[Textures[CurrentTexture].index]);
	texpage_display_name( TmapInfo[CurrentTexture].filename );
}

int texpage_goto_first()
{
	TexturePage=0;
	texpage_redraw();
	return 1;
}

int texpage_goto_metals()
{

	TexturePage=TextureMetals/TMAPS_PER_PAGE;
	texpage_redraw();
	return 1;
}


// Goto lights (paste ons)
int texpage_goto_lights()
{
	TexturePage=TextureLights/TMAPS_PER_PAGE;
	texpage_redraw();
	return 1;
}

int texpage_goto_effects()
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
	if ((TexturePage+1)*TMAPS_PER_PAGE < Num_tmaps ) {
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
	int i;

	if ( (n<0) || ( n>= Num_tmaps) ) return 0;

	CurrentTexture = n;

	for (i=0;i<Num_tmaps;i++)
		if (TmapList[i] == n) {
			CurrentTmap = i;
			break;
		}
	Assert(i!=Num_tmaps);
	
	TexturePage = CurrentTmap / TMAPS_PER_PAGE;
	
	if (TexturePage*TMAPS_PER_PAGE < Num_tmaps )
		texpage_redraw();

	texpage_show_current();
	
	return 1;
}


// INIT TEXTURE STUFF

void texpage_init( UI_WINDOW * win )
{
	int i;

	ui_add_gadget_button( win, TMAPCURBOX_X + 00, TMAPCURBOX_Y - 24, 30, 20, "<<", texpage_goto_prev );
	ui_add_gadget_button( win, TMAPCURBOX_X + 32, TMAPCURBOX_Y - 24, 30, 20, ">>", texpage_goto_next );

	ui_add_gadget_button( win, TMAPCURBOX_X + 00, TMAPCURBOX_Y - 48, 15, 20, "T", texpage_goto_first );
	ui_add_gadget_button( win, TMAPCURBOX_X + 17, TMAPCURBOX_Y - 48, 15, 20, "M", texpage_goto_metals );
	ui_add_gadget_button( win, TMAPCURBOX_X + 34, TMAPCURBOX_Y - 48, 15, 20, "L", texpage_goto_lights );
	ui_add_gadget_button( win, TMAPCURBOX_X + 51, TMAPCURBOX_Y - 48, 15, 20, "E", texpage_goto_effects );
	

	for (i=0;i<TMAPS_PER_PAGE;i++)
		TmapBox[i] = ui_add_gadget_userbox( win, TMAPBOX_X + (i/3)*(2+TMAPBOX_W), TMAPBOX_Y + (i%3)*(2+TMAPBOX_H), TMAPBOX_W, TMAPBOX_H);

	TmapCurrent = ui_add_gadget_userbox( win, TMAPCURBOX_X, TMAPCURBOX_Y, 64, 64 );

	TmapnameCanvas = gr_create_sub_canvas(&grd_curscreen->sc_canvas, TMAPCURBOX_X , TMAPCURBOX_Y + TMAPBOX_H + 10, 100, 20);
	gr_set_current_canvas( TmapnameCanvas );
	gr_set_curfont( ui_small_font ); 
   gr_set_fontcolor( CBLACK, CWHITE );

	texpage_redraw();

// Don't reset the current tmap every time we go back to the editor.
//	CurrentTmap = TexturePage*TMAPS_PER_PAGE;
//	CurrentTexture = TmapList[CurrentTmap];
	texpage_show_current();

}

void texpage_close()
{
	gr_free_sub_canvas(TmapnameCanvas);
}


// DO TEXTURE STUFF

#define	MAX_REPLACEMENTS	32

typedef struct replacement {
	int	new, old;
} replacement;

replacement Replacement_list[MAX_REPLACEMENTS];
int	Num_replacements=0;

void texpage_do()
{
	int i;

	for (i=0; i<TMAPS_PER_PAGE; i++ ) {
		if (TmapBox[i]->b1_clicked && (i+TexturePage*TMAPS_PER_PAGE < Num_tmaps)) {
			CurrentTmap = i+TexturePage*TMAPS_PER_PAGE;
			CurrentTexture = TmapList[CurrentTmap];
			texpage_show_current();

			if (keyd_pressed[KEY_LSHIFT]) {
				mprintf((0, "Will replace CurrentTexture (%i) with...(select by pressing Ctrl)\n", CurrentTexture));
				Replacement_list[Num_replacements].old = CurrentTexture;
			}

			if (keyd_pressed[KEY_LCTRL]) {
				mprintf((0, "...Replacement texture for %i is %i\n", Replacement_list[Num_replacements].old, CurrentTexture));
				Replacement_list[Num_replacements].new = CurrentTexture;
				Num_replacements++;
			}
		}
	}
}

void init_replacements(void)
{
	Num_replacements = 0;
}

void do_replacements(void)
{
	int	replnum, segnum, sidenum;

	med_compress_mine();

	for (replnum=0; replnum<Num_replacements; replnum++) {
		int	old_tmap_num, new_tmap_num;

		old_tmap_num = Replacement_list[replnum].old;
		new_tmap_num = Replacement_list[replnum].new;
		Assert(old_tmap_num >= 0);
		Assert(new_tmap_num >= 0);

		for (segnum=0; segnum <= Highest_segment_index; segnum++) {
			segment	*segp=&Segments[segnum];
			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
				side	*sidep=&segp->sides[sidenum];
				if (sidep->tmap_num == old_tmap_num) {
					sidep->tmap_num = new_tmap_num;
					// mprintf((0, "Replacing tmap_num on segment:side = %i:%i\n", segnum, sidenum));
				}
				if ((sidep->tmap_num2 != 0) && ((sidep->tmap_num2 & 0x3fff) == old_tmap_num)) {
					if (new_tmap_num == 0) {
						Int3();	//	Error.  You have tried to replace a tmap_num2 with 
									//	the 0th tmap_num2 which is ILLEGAL!
					} else {
						sidep->tmap_num2 = new_tmap_num | (sidep->tmap_num2 & 0xc000);
						// mprintf((0, "Replacing tmap_num2 on segment:side = %i:%i\n", segnum, sidenum));
					}
				}
			}
		}
	}

}

void do_replacements_all(void)
{
	int	i;

	for (i=0; i<NUM_SHAREWARE_LEVELS; i++) {
		load_level(Shareware_level_names[i]);
		do_replacements();
		save_level(Shareware_level_names[i]);
	}

	for (i=0; i<NUM_REGISTERED_LEVELS; i++) {
		load_level(Registered_level_names[i]);
		do_replacements();
		save_level(Registered_level_names[i]);
	}

}

