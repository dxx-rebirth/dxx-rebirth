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


#ifdef RCS
static char rcsid[] = "$Id: gamefont.c,v 1.1.1.1 2001-01-19 03:30:04 bradleyb Exp $";
#endif

#include <conf.h>
#include <stdlib.h>

#include "inferno.h"
#include "gr.h"
#include "gamefont.h"

//if 1, use high-res versions of fonts
int FontHires=0;

char * Gamefont_filenames[] = {	"font1-1.fnt",			// Font 0
										 	"font1-1h.fnt",		// Font 0 High-res
											"font2-1.fnt",			// Font 1
											"font2-1h.fnt",		// Font 1 High-res
											"font2-2.fnt",			// Font 2
											"font2-2h.fnt",		// Font 2 High-res
											"font2-3.fnt",			// Font 3
											"font2-3h.fnt",		// Font 3 High-res
											"font3-1.fnt",			// Font 4
											"font3-1h.fnt",		// Font 4 High-res
										};

grs_font *Gamefonts[MAX_FONTS];

int Gamefont_installed=0;

void gamefont_init()
{
	int i;

	if (Gamefont_installed) return;
	Gamefont_installed = 1;

	for (i=0; i<MAX_FONTS; i++ )
		Gamefonts[i] = gr_init_font(Gamefont_filenames[i]);

	atexit( gamefont_close );
}


void gamefont_close()
{
	int i;

	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (i=0; i<MAX_FONTS; i++ )	{
		gr_close_font( Gamefonts[i] );
		Gamefonts[i] = NULL;
	}

}
