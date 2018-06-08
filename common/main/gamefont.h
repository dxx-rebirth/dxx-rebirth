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

constexpr std::integral_constant<unsigned, 5> MAX_FONTS{};

// add (scaled) spacing to given font coordinate

extern array<grs_font_ptr, MAX_FONTS> Gamefonts;

class base_font_scale_proportion
{
protected:
	float f;
public:
	base_font_scale_proportion() = default;
	explicit constexpr base_font_scale_proportion(const float v) :
		f(v)
	{
	}
	explicit operator float() const
	{
		return f;
	}
	float operator*(const float v) const
	{
		return f * v;
	}
	void reset(const float v)
	{
		f = v;
	}
};

template <char tag>
class font_scale_proportion : public base_font_scale_proportion
{
public:
	DXX_INHERIT_CONSTRUCTORS(font_scale_proportion, base_font_scale_proportion);
	bool operator!=(const font_scale_proportion &rhs) const
	{
		return f != rhs.f;
	}
};

using font_x_scale_proportion = font_scale_proportion<'x'>;
using font_y_scale_proportion = font_scale_proportion<'y'>;

extern font_x_scale_proportion FNTScaleX;
extern font_y_scale_proportion FNTScaleY;

static inline float LINE_SPACING(const grs_font &active_font, const grs_font &game_font)
{
	return FNTScaleY * (active_font.ft_h + (game_font.ft_h / 5));
}

/* All the logic is in the base class */
class base_font_scaled_float
{
	const float f;
public:
	explicit constexpr base_font_scaled_float(const float v) :
		f(v)
	{
	}
	operator float() const
	{
		return f;
	}
};

/* Use an otherwise unnecessary tag to prevent mixing x-scale with
 * y-scale.
 */
template <char tag>
class font_scaled_float : public base_font_scaled_float
{
public:
	DXX_INHERIT_CONSTRUCTORS(font_scaled_float, base_font_scaled_float);
};

template <char tag>
class font_scale_float
{
	const float scale;
public:
	using scaled = font_scaled_float<tag>;
	constexpr font_scale_float(const float s) :
		scale(s)
	{
	}
	auto operator()(const int &i) const
	{
		return scaled(scale * i);
	}
};

using font_x_scale_float = font_scale_float<'x'>;
using font_y_scale_float = font_scale_float<'y'>;
using font_x_scaled_float = font_x_scale_float::scaled;
using font_y_scaled_float = font_y_scale_float::scaled;

static inline font_x_scale_float FSPACX()
{
	return FNTScaleX * (GAME_FONT->ft_w / 7);
}

static inline auto FSPACX(const int &x)
{
	return FSPACX()(x);
}

static inline font_y_scale_float FSPACY()
{
	return FNTScaleY * (GAME_FONT->ft_h / 5);
}

static inline auto FSPACY(const int &y)
{
	return FSPACY()(y);
}

#ifdef dsx
namespace dsx {
void gamefont_init();
}
#endif
void gamefont_close();
void gamefont_choose_game_font(int scrx,int scry);

#endif
