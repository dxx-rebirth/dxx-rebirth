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
 * Font declarations for the game.
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
#define GFONT_MEDIUM_1  1
#define GFONT_MEDIUM_2  2
#define GFONT_MEDIUM_3  3
#define GFONT_SMALL     4

#define GAME_FONT       (Gamefonts[GFONT_SMALL])
#define MEDIUM1_FONT    (Gamefonts[GFONT_MEDIUM_1])
#define MEDIUM2_FONT    (Gamefonts[GFONT_MEDIUM_2])
#define MEDIUM3_FONT    (Gamefonts[GFONT_MEDIUM_3])
#define HUGE_FONT       (Gamefonts[GFONT_BIG_1])

#define MAX_FONTS 5

extern float FNTScaleX, FNTScaleY;

// add (scaled) spacing to given font coordinate
#define FSPACX(x)	((float)((x)*(FNTScaleX*(GAME_FONT->ft_w/7))))
#define FSPACY(y)	((float)((y)*(FNTScaleY*(GAME_FONT->ft_h/5))))
#define LINE_SPACING    ((float)(FNTScaleY*(grd_curcanv->cv_font->ft_h+(GAME_FONT->ft_h/5))))

extern grs_font *Gamefonts[MAX_FONTS];

void gamefont_init();
void gamefont_close();
void gamefont_choose_game_font(int scrx,int scry);

#endif /* _GAMEFONT_H */
