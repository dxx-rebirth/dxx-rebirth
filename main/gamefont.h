/* $Id: gamefont.h,v 1.4 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Font declarations for the game,.
 *
 */


#ifndef _GAMEFONT_H
#define _GAMEFONT_H

#include "gr.h"

// When adding a new font, don't forget to change the filename in
// gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

// We are interleaving low & high resolution fonts, so to access a
// font you say fontnum+flag where flag is 0 for lowres, 1 for hires

#define GFONT_BIG_1     0
#define GFONT_MEDIUM_1  2
#define GFONT_MEDIUM_2  4
#define GFONT_MEDIUM_3  6
#define GFONT_SMALL     8

#define SMALL_FONT      (Gamefonts[GFONT_SMALL + FontHires])
#define MEDIUM1_FONT    (Gamefonts[GFONT_MEDIUM_1 + FontHires])
#define MEDIUM2_FONT    (Gamefonts[GFONT_MEDIUM_2 + FontHires])
#define MEDIUM3_FONT    (Gamefonts[GFONT_MEDIUM_3 + FontHires])
#define HUGE_FONT       (Gamefonts[GFONT_BIG_1 + FontHires])

#define GAME_FONT       SMALL_FONT

#define MAX_FONTS 10

extern grs_font *Gamefonts[MAX_FONTS];

void gamefont_init();
void gamefont_close();

extern int FontHires;
extern int FontHiresAvailable;

#endif /* _GAMEFONT_H */
