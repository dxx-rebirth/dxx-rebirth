/* $Id: gamefont.c,v 1.5 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Fonts for the game.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: gamefont.c,v 1.5 2004-08-28 23:17:45 schaffner Exp $";
#endif

#include <stdlib.h>

#include "inferno.h"
#include "gr.h"
#include "gamefont.h"

// if 1, use high-res versions of fonts
int FontHires = 0;
int FontHiresAvailable = 0;

char * Gamefont_filenames[] = { "font1-1.fnt",      // Font 0
                                "font1-1h.fnt",     // Font 0 High-res
                                "font2-1.fnt",      // Font 1
                                "font2-1h.fnt",     // Font 1 High-res
                                "font2-2.fnt",      // Font 2
                                "font2-2h.fnt",     // Font 2 High-res
                                "font2-3.fnt",      // Font 3
                                "font2-3h.fnt",     // Font 3 High-res
                                "font3-1.fnt",      // Font 4
                                "font3-1h.fnt",     // Font 4 High-res
                              };

grs_font *Gamefonts[MAX_FONTS];

int Gamefont_installed = 0;

void gamefont_init()
{
	int i;

	if (Gamefont_installed)
		return;
	Gamefont_installed = 1;
	FontHiresAvailable = 1;

	for (i = 0; i < MAX_FONTS; i += 2)
		Gamefonts[i] = gr_init_font(Gamefont_filenames[i]);
	for (i = 1; i < MAX_FONTS; i += 2)
	{
		Gamefonts[i] = gr_init_font(Gamefont_filenames[i]);
		if (!Gamefonts[i])
			FontHiresAvailable = 0;
	}

	atexit(gamefont_close);
}


void gamefont_close()
{
	int i;

	if (!Gamefont_installed)
		return;
	Gamefont_installed = 0;

	for (i = 0; i < MAX_FONTS; i++)
	{
		gr_close_font(Gamefonts[i]);
		Gamefonts[i] = NULL;
	}
}
