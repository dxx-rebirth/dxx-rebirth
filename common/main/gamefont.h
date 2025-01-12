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

#include "dsx-ns.h"
#include "gr.h"
#include "fwd-d_array.h"

// When adding a new font, don't forget to change the filename in
// gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

// We are interleaving low & high resolution fonts, so to access a
// font you say fontnum+flag where flag is 0 for lowres, 1 for hires

namespace dcx {

enum class gamefont_index : uint8_t
{
	big,
	medium1,
	medium2,
	medium3,
	small,
};

#define GAME_FONT       (Gamefonts[gamefont_index::small].font)
#define MEDIUM1_FONT    (Gamefonts[gamefont_index::medium1].font)
#define MEDIUM2_FONT    (Gamefonts[gamefont_index::medium2].font)
#define MEDIUM3_FONT    (Gamefonts[gamefont_index::medium3].font)

#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 1
// The Mac data doesn't have this in hires, in the automap the scaled/hires one won't fit
#define HUGE_FONT       (Gamefonts[MacPig ? gamefont_index::medium1 : gamefont_index::big].font)
#elif DXX_BUILD_DESCENT == 2
#define HUGE_FONT       (Gamefonts[gamefont_index::big].font)
#endif
#endif

constexpr std::integral_constant<unsigned, 5> MAX_FONTS{};

struct loaded_game_font
{
	grs_font_ptr font;
};

// add (scaled) spacing to given font coordinate

extern enumerated_array<loaded_game_font, MAX_FONTS, gamefont_index> Gamefonts;

}

class base_font_scale_proportion
{
protected:
	float f;
public:
	base_font_scale_proportion() = default;
	explicit constexpr base_font_scale_proportion(const float v) :
		f{v}
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
	constexpr bool operator==(const base_font_scale_proportion &rhs) const = default;
};

template <char tag>
class font_scale_proportion : public base_font_scale_proportion
{
public:
	using base_font_scale_proportion::base_font_scale_proportion;
	bool operator==(const font_scale_proportion &rhs) const = default;	/* no `constexpr` due to gcc bug #98712 */
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
		f{v}
	{
	}
	base_font_scaled_float(int) = delete;
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
	using base_font_scaled_float::base_font_scaled_float;
};

template <char tag>
class font_scale_float
{
	const float scale;
public:
	using scaled = font_scaled_float<tag>;
	explicit constexpr font_scale_float(const float s) :
		scale{s}
	{
	}
	font_scale_float(int) = delete;
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
	return font_x_scale_float(FNTScaleX * (GAME_FONT->ft_w / 7));
}

static inline auto FSPACX(const int &x)
{
	return FSPACX()(x);
}

static inline font_y_scale_float FSPACY()
{
	return font_y_scale_float(FNTScaleY * (GAME_FONT->ft_h / 5));
}

static inline auto FSPACY(const int &y)
{
	return FSPACY()(y);
}

#ifdef DXX_BUILD_DESCENT
namespace dsx {
void gamefont_init();
}
#endif
void gamefont_close();
void gamefont_choose_game_font(int scrx,int scry);
