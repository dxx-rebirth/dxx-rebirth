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
 * Font declarations for the game.
 *
 */

#pragma once

#ifdef __cplusplus
#include "gr.h"

// When adding a new font, don't forget to change the filename in
// gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

// We are interleaving low & high resolution fonts, so to access a
// font you say fontnum+flag where flag is 0 for lowres, 1 for hires

#if defined(DXX_BUILD_DESCENT_I)
#define GFONT_BIG_1     MacPig	// the Mac data doesn't have this in hires, in the automap the scaled/hires one won't fit
#elif defined(DXX_BUILD_DESCENT_II)
#define GFONT_BIG_1     0
#endif
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
#define LINE_SPACING    ((float)(FNTScaleY*(grd_curcanv->cv_font->ft_h+(GAME_FONT->ft_h/5))))

extern array<grs_font_ptr, MAX_FONTS> Gamefonts;

class font_scale_float
{
	const float scale;
public:
	font_scale_float(float s) :
		scale(s)
	{
	}
	float operator()(const int &i) const
	{
		return scale * i;
	}
};

static inline font_scale_float FSPACX()
{
	return FNTScaleX * (GAME_FONT->ft_w / 7);
}

static inline float FSPACX(const int &x)
{
	return FSPACX()(x);
}

static inline font_scale_float FSPACY()
{
	return FNTScaleY * (GAME_FONT->ft_h / 5);
}

static inline float FSPACY(const int &y)
{
	return FSPACY()(y);
}

void gamefont_init();
void gamefont_close();
void gamefont_choose_game_font(int scrx,int scry);

#endif
