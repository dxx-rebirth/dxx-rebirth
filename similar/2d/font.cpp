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
 * Graphical routines for drawing fonts.
 *
 */

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef macintosh
#include <fcntl.h>
#endif

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#include "common/2d/bitmap.h"
#include "makesig.h"
#include "physfsx.h"
#include "gamefont.h"
#include "byteutil.h"
#include "console.h"
#include "config.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-array.h"
#include "compiler-range_for.h"
#include "compiler-make_unique.h"
#include "partial_range.h"

static font_x_scale_float FONTSCALE_X()
{
	return FNTScaleX.operator float();
}

static auto FONTSCALE_Y(const int &y)
{
	return font_scaled_float<'y'>(FNTScaleY * y);
}

#define MAX_OPEN_FONTS	50

namespace {

constexpr std::integral_constant<uint8_t, 255> kerndata_terminator{};

}

namespace dcx {

//list of open fonts, for use (for now) for palette remapping
static array<grs_font *, MAX_OPEN_FONTS> open_font;

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

static int gr_internal_string_clipped(grs_canvas &, const grs_font &cv_font, int x, int y, const char *s);
static int gr_internal_string_clipped_m(grs_canvas &, const grs_font &cv_font, int x, int y, const char *s);

static const uint8_t *find_kern_entry(const grs_font &font, const uint8_t first, const uint8_t second)
{
	auto p = font.ft_kerndata;

	while (*p != kerndata_terminator)
		if (p[0]==first && p[1]==second)
			return p;
		else p+=3;
	return NULL;
}

//takes the character AFTER being offset into font

namespace {

class font_character_extent
{
	const unsigned r;
public:
	font_character_extent(const grs_font &cv_font) :
		r(cv_font.ft_maxchar - cv_font.ft_minchar)
	{
	}
	bool operator()(const unsigned c) const
	{
		return c <= r;
	}
};

template <typename T>
struct get_char_width_result
{
	T width, spacing;
};

/* Floating form never uses width.  This specialization allows the
 * compiler to recognize width as dead, shortening
 * get_char_width<float>.
 */
template <>
struct get_char_width_result<float>
{
	float spacing;
	get_char_width_result(float, float s) :
		spacing(s)
	{
	}
};

}

//takes the character BEFORE being offset into current font
template <typename T>
static get_char_width_result<T> get_char_width(const grs_font &cv_font, const uint8_t c, const uint8_t c2)
{
	const unsigned letter = c - cv_font.ft_minchar;
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;

	const auto &&fontscale_x = FONTSCALE_X();
	const auto &&INFONT = font_character_extent(cv_font);
	if (!INFONT(letter)) {				//not in font, draw as space
		return {0, static_cast<T>(proportional ? fontscale_x(cv_font.ft_w) / 2 : cv_font.ft_w)};
	}
	const T width = proportional ? fontscale_x(cv_font.ft_widths[letter]).operator float() : cv_font.ft_w;
	if (ft_flags & FT_KERNED) 
	{
		if (!(c2==0 || c2=='\n')) {
			const unsigned letter2 = c2 - cv_font.ft_minchar;

			if (INFONT(letter2)) {
				const auto p = find_kern_entry(cv_font, letter, letter2);
				if (p)
					return {width, static_cast<T>(fontscale_x(p[2]))};
			}
		}
	}
	return {width, width};
}

static int get_centered_x(const grs_canvas &canvas, const grs_font &cv_font, const char *s)
{
	float w;
	for (w = 0.; const char c = *s; ++s)
	{
		if (c == '\n')
			break;
		if (c <= 0x06)
		{
			if (c <= 0x03)
				s++;
			continue;//skip color codes.
		}
		w += get_char_width<float>(cv_font, c, s[1]).spacing;
	}

	return (canvas.cv_bitmap.bm_w - w) / 2;
}

//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color type thing would be better, but that would be way too much trouble for little gain
constexpr std::integral_constant<int, 1> gr_message_color_level{};
#define CHECK_EMBEDDED_COLORS() if ((*text_ptr >= 0x01) && (*text_ptr <= 0x02)) { \
		text_ptr++; \
		if (*text_ptr){ \
			if (gr_message_color_level >= *(text_ptr-1)) \
				canvas.cv_font_fg_color = static_cast<uint8_t>(*text_ptr); \
			text_ptr++; \
		} \
	} \
	else if (*text_ptr == 0x03) \
	{ \
		underline = 1; \
		text_ptr++; \
	} \
	else if ((*text_ptr >= 0x04) && (*text_ptr <= 0x06)){ \
		if (gr_message_color_level >= *text_ptr - 3) \
			canvas.cv_font_fg_color= static_cast<uint8_t>(orig_color); \
		text_ptr++; \
	}

template <bool masked_draws_background>
static int gr_internal_string0_template(grs_canvas &canvas, const grs_font &cv_font, const int x, int y, const char *const s)
{
	const auto &&INFONT = font_character_extent(cv_font);
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;
	const auto cv_font_bg_color = canvas.cv_font_bg_color;
	int	skip_lines = 0;

	unsigned int VideoOffset1;

	//to allow easy reseting to default string color with colored strings -MPM
	const auto orig_color = canvas.cv_font_fg_color;
	VideoOffset1 = y * canvas.cv_bitmap.bm_rowsize + x;
	auto next_row = s;
	while (next_row != NULL )
	{
		const auto text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(canvas, cv_font, text_ptr1);
			VideoOffset1 = y * canvas.cv_bitmap.bm_rowsize + xx;
		}

		for (int r=0; r < cv_font.ft_h; ++r)
		{
			auto text_ptr = text_ptr1;

			unsigned VideoOffset = VideoOffset1;

			for (; const uint8_t c0 = *text_ptr; ++text_ptr)
			{
				if (c0 == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}

				if (c0 == CC_COLOR)
				{
					canvas.cv_font_fg_color = static_cast<uint8_t>(*++text_ptr);
					continue;
				}

				if (c0 == CC_LSPACING)
				{
					skip_lines = *++text_ptr - '0';
					continue;
				}

				auto underline = unlikely(c0 == CC_UNDERLINE)
					? ++text_ptr, r == cv_font.ft_baseline + 2 || r == cv_font.ft_baseline + 3
					: 0;

				const uint8_t c = *text_ptr;
				const auto &result = get_char_width<int>(cv_font, c, text_ptr[1]);
				const auto &width = result.width;
				const auto &spacing = result.spacing;

				const unsigned letter = c - cv_font.ft_minchar;

				if (masked_draws_background)
				{
					if (!INFONT(letter)) {	//not in font, draw as space
						VideoOffset += spacing;
						text_ptr++;
						continue;
					}
				}
				else
				{
					if (!INFONT(letter) || c <= 0x06)	//not in font, draw as space
					{
						CHECK_EMBEDDED_COLORS() else{
							VideoOffset += spacing;
							text_ptr++;
						}
						continue;
					}
				}

				if (width)
				{
					auto data = &canvas.cv_bitmap.get_bitmap_data()[VideoOffset];
					const auto cv_font_fg_color = canvas.cv_font_fg_color;
				if (underline)
				{
					std::fill_n(data, width, cv_font_fg_color);
				}
				else
				{
					auto fp = proportional ? cv_font.ft_chars[letter] : &cv_font.ft_data[letter * BITS_TO_BYTES(width) * cv_font.ft_h];
					fp += BITS_TO_BYTES(width)*r;

					/* Setting bits=0 is a dead store, but is necessary to
					 * prevent -Og -Wuninitialized from issuing a bogus
					 * warning.  -Og does not see that bits_remaining=0
					 * guarantees that bits will be initialized from *fp before
					 * it is read.
					 */
					uint8_t bits_remaining = 0, bits = 0;
					for (uint_fast32_t i = width; i--; ++data, --bits_remaining)
					{
						if (!bits_remaining)
						{
							bits = *fp++;
							bits_remaining = 8;
						}

						const auto bit_enabled = (bits & 0x80);
						bits <<= 1;
						if (!masked_draws_background)
						{
							if (!bit_enabled)
								continue;
						}
						*data = bit_enabled ? cv_font_fg_color : cv_font_bg_color;
					}
				}
				}
				VideoOffset += spacing;
			}
			VideoOffset1 += canvas.cv_bitmap.bm_rowsize;
			y++;
		}
		y += skip_lines;
		VideoOffset1 += canvas.cv_bitmap.bm_rowsize * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

static int gr_internal_string0(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	return gr_internal_string0_template<true>(canvas, cv_font, x, y, s);
}

static int gr_internal_string0m(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	return gr_internal_string0_template<false>(canvas, cv_font, x, y, s);
}

#if !DXX_USE_OGL
static int gr_internal_color_string(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
//a bitmap for the character
	grs_bitmap char_bm = {};
	char_bm.set_type(bm_mode::linear);
	char_bm.set_flags(BM_FLAG_TRANSPARENT);
	const char *text_ptr, *next_row, *text_ptr1;
	int letter;
	int xx,yy;

	const auto &&INFONT = font_character_extent(cv_font);
	char_bm.bm_h = cv_font.ft_h;		//set height for chars of this font

	next_row = s;

	yy = y;

	const auto &&fspacy1 = FSPACY(1);
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = get_centered_x(canvas, cv_font, text_ptr);

		while (*text_ptr)
		{
			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += cv_font.ft_h + fspacy1;
				break;
			}

			letter = static_cast<uint8_t>(*text_ptr) - cv_font.ft_minchar;

			const auto &result = get_char_width<int>(cv_font, text_ptr[0], text_ptr[1]);
			const auto &width = result.width;
			const auto &spacing = result.spacing;

			if (!INFONT(letter)) {	//not in font, draw as space
				xx += spacing;
				text_ptr++;
				continue;
			}

			const auto fp = (cv_font.ft_flags & FT_PROPORTIONAL)
				? cv_font.ft_chars[letter]
				: &cv_font.ft_data[letter * BITS_TO_BYTES(width) * cv_font.ft_h];

			gr_init_bitmap(char_bm, bm_mode::linear, 0, 0, width, cv_font.ft_h, width, fp);
			gr_bitmapm(canvas, xx, yy, char_bm);

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

#else //OGL

static int get_font_total_width(const grs_font &font)
{
	if (font.ft_flags & FT_PROPORTIONAL)
	{
		int w=0;
		range_for (const auto v, unchecked_partial_range(font.ft_widths, static_cast<unsigned>(font.ft_maxchar - font.ft_minchar) + 1))
		{
			if (v < 0)
				throw std::underflow_error("negative width");
			w += v;
		}
		return w;
	}else{
		return font.ft_w*(font.ft_maxchar-font.ft_minchar+1);
	}
}

static void ogl_font_choose_size(grs_font * font,int gap,int *rw,int *rh){
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int r,x,y,nc=0,smallest=999999,smallr=-1,tries;
	int smallprop=10000;
	int w;
	for (int h=32;h<=256;h*=2){
//		h=pow2ize(font->ft_h*rows+gap*(rows-1));
		if (font->ft_h>h)continue;
		r=(h/(font->ft_h+gap));
		w=pow2ize((get_font_total_width(*font)+(nchars-r)*gap)/r);
		tries=0;
		do {
			if (tries)
				w=pow2ize(w+1);
			if(tries>3){
				break;
			}
			nc=0;
			y=0;
			while(y+font->ft_h<=h){
				x=0;
				while (x<w){
					if (nc==nchars)
						break;
					if (font->ft_flags & FT_PROPORTIONAL){
						if (x+font->ft_widths[nc]+gap>w)break;
						x+=font->ft_widths[nc++]+gap;
					}else{
						if (x+font->ft_w+gap>w)break;
						x+=font->ft_w+gap;
						nc++;
					}
				}
				if (nc==nchars)
					break;
				y+=font->ft_h+gap;
			}
			
			tries++;
		}while(nc!=nchars);
		if (nc!=nchars)
			continue;

		if (w*h==smallest){//this gives squarer sizes priority (ie, 128x128 would be better than 512*32)
			if (w>=h){
				if (w/h<smallprop){
					smallprop=w/h;
					smallest++;//hack
				}
			}else{
				if (h/w<smallprop){
					smallprop=h/w;
					smallest++;//hack
				}
			}
		}
		if (w*h<smallest){
			smallr=1;
			smallest=w*h;
			*rw=w;
			*rh=h;
		}
	}
	if (smallr<=0)
		Error("Could not fit font?\n");
}

static void ogl_init_font(grs_font * font)
{
	int oglflags = OGL_FLAG_ALPHA;
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int w,h,tw,th,curx=0,cury=0;
	uint8_t *data;
	int gap=1; // x/y offset between the chars so we can filter

	th = tw = 0xffff;

	ogl_font_choose_size(font,gap,&tw,&th);
	MALLOC(data, ubyte, tw*th);
	memset(data, TRANSPARENCY_COLOR, tw * th); // map the whole data with transparency so we won't have borders if using gap
	gr_init_bitmap(font->ft_parent_bitmap,bm_mode::linear,0,0,tw,th,tw,data);
	gr_set_transparent(font->ft_parent_bitmap, 1);

	if (!(font->ft_flags & FT_COLOR))
		oglflags |= OGL_FLAG_NOCOLOR;
	ogl_init_texture(*(font->ft_parent_bitmap.gltexture = ogl_get_free_texture()), tw, th, oglflags); // have to init the gltexture here so the subbitmaps will find it.

	font->ft_bitmaps = make_unique<grs_bitmap[]>(nchars);
	h=font->ft_h;

	for(int i=0;i<nchars;i++)
	{
		if (font->ft_flags & FT_PROPORTIONAL)
			w=font->ft_widths[i];
		else
			w=font->ft_w;

		if (w<1 || w>256)
			continue;

		if (curx+w+gap>tw)
		{
			cury+=h+gap;
			curx=0;
		}

		if (cury+h>th)
			Error("font doesn't really fit (%i/%i)?\n",i,nchars);

		if (font->ft_flags & FT_COLOR)
		{
			const auto fp = (font->ft_flags & FT_PROPORTIONAL)
				? font->ft_chars[i]
				: font->ft_data + i * w*h;
			for (int y=0;y<h;y++)
			{
				for (int x=0;x<w;x++)
				{
					font->ft_parent_bitmap.get_bitmap_data()[curx+x+(cury+y)*tw] = fp[x+y*w];
					// Let's call this a HACK:
					// If we filter the fonts, the sliders will be messed up as the border pixels will have an
					// alpha value while filtering. So the slider bitmaps will not look "connected".
					// To prevent this, duplicate the first/last pixel-row with a 1-pixel offset.
					if (gap && i >= 99 && i <= 102)
					{
						// See which bitmaps need left/right shifts:
						// 99  = SLIDER_LEFT - shift RIGHT
						// 100 = SLIDER_RIGHT - shift LEFT
						// 101 = SLIDER_MIDDLE - shift LEFT+RIGHT
						// 102 = SLIDER_MARKER - shift RIGHT

						// shift left border
						if (x==0 && i != 99 && i != 102)
							font->ft_parent_bitmap.get_bitmap_data()[(curx+x+(cury+y)*tw)-1] = fp[x+y*w];

						// shift right border
						if (x==w-1 && i != 100)
							font->ft_parent_bitmap.get_bitmap_data()[(curx+x+(cury+y)*tw)+1] = fp[x+y*w];
					}
				}
			}
		}
		else
		{
			int BitMask,bits=0,white=gr_find_closest_color(63,63,63);
			auto fp = (font->ft_flags & FT_PROPORTIONAL)
				? font->ft_chars[i]
				: font->ft_data + i * BITS_TO_BYTES(w)*h;
			for (int y=0;y<h;y++){
				BitMask=0;
				for (int x=0; x< w; x++ )
				{
					if (BitMask==0) {
						bits = *fp++;
						BitMask = 0x80;
					}

					if (bits & BitMask)
						font->ft_parent_bitmap.get_bitmap_data()[curx+x+(cury+y)*tw] = white;
					else
						font->ft_parent_bitmap.get_bitmap_data()[curx+x+(cury+y)*tw] = 255;
					BitMask >>= 1;
				}
			}
		}
		gr_init_sub_bitmap(font->ft_bitmaps[i],font->ft_parent_bitmap,curx,cury,w,h);
		curx+=w+gap;
	}
	ogl_loadbmtexture_f(font->ft_parent_bitmap, CGameCfg.TexFilt, 0, 0);
}

static int ogl_internal_string(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	const char * text_ptr, * next_row, * text_ptr1;
	int letter;
	int xx,yy;
	int orig_color = canvas.cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM
	int underline;

	next_row = s;

	yy = y;

	if (grd_curscreen->sc_canvas.cv_bitmap.get_type() != bm_mode::ogl)
		Error("carp.\n");
	const auto &&fspacy1 = FSPACY(1);
	const auto &&INFONT = font_character_extent(cv_font);
	const auto &&fontscale_x = FONTSCALE_X();
	const auto &&FONTSCALE_Y_ft_h = FONTSCALE_Y(cv_font.ft_h);
	ogl_colors colors;
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = get_centered_x(canvas, cv_font, text_ptr);

		while (*text_ptr)
		{

			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += FONTSCALE_Y_ft_h + fspacy1;
				break;
			}

			letter = static_cast<uint8_t>(*text_ptr) - cv_font.ft_minchar;

			const auto &result = get_char_width<int>(cv_font, text_ptr[0], text_ptr[1]);
			const auto &spacing = result.spacing;

			underline = 0;
			if (!INFONT(letter) || static_cast<uint8_t>(*text_ptr) <= 0x06) //not in font, draw as space
			{
				CHECK_EMBEDDED_COLORS() else{
					xx += spacing;
					text_ptr++;
				}
				
				if (underline)
				{
					const uint8_t color = canvas.cv_font_fg_color;
					gr_rect(canvas, xx, yy + cv_font.ft_baseline + 2, xx + cv_font.ft_w, yy + cv_font.ft_baseline + 3, color);
				}

				continue;
			}
			
			const auto ft_w = (cv_font.ft_flags & FT_PROPORTIONAL)
				? cv_font.ft_widths[letter]
				: cv_font.ft_w;

			ogl_ubitmapm_cs(canvas, xx, yy, fontscale_x(ft_w), FONTSCALE_Y_ft_h, cv_font.ft_bitmaps[letter], (cv_font.ft_flags & FT_COLOR) ? colors.white : (canvas.cv_bitmap.get_type() == bm_mode::ogl) ? colors.init(canvas.cv_font_fg_color) : throw std::runtime_error("non-color string to non-ogl dest"), F1_0);

			xx += spacing;

			text_ptr++;
		}
	}
	return 0;
}

#define gr_internal_color_string ogl_internal_string
#endif //OGL

void gr_string(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	int w, h;
	gr_get_string_size(cv_font, s, &w, &h, nullptr);
	gr_string(canvas, cv_font, x, y, s, w, h);
}

static void gr_ustring_mono(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	switch (canvas.cv_bitmap.get_type())
	{
		case bm_mode::linear:
			if (canvas.cv_font_bg_color == -1)
				gr_internal_string0m(canvas, cv_font, x, y, s);
			else
				gr_internal_string0(canvas, cv_font, x, y, s);
	}
}

void gr_string(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s, const int w, const int h)
{
	if (y + h < 0)
		return;
	const auto bm_h = canvas.cv_bitmap.bm_h;
	if (y > bm_h)
		return;
	const auto bm_w = canvas.cv_bitmap.bm_w;
	int xw = w;
	if ( x == 0x8000 )	{
		// for x, since this will be centered, only look at
		// width.
	} else {
		if (x > bm_w)
			return;
		xw += x;
		if (xw < 0)
			return;
	}
	if (
#if DXX_USE_OGL
		canvas.cv_bitmap.get_type() == bm_mode::ogl ||
#endif
		cv_font.ft_flags & FT_COLOR)
	{
		gr_internal_color_string(canvas, cv_font, x, y, s);
		return;
	}
	// Partially clipped...
	if (!(y < 0 ||
		x < 0 ||
		xw > bm_w ||
		y + h > bm_h))
	{
		gr_ustring_mono(canvas, cv_font, x, y, s);
		return;
	}

	if (canvas.cv_font_bg_color == -1)
	{
		gr_internal_string_clipped_m(canvas, cv_font, x, y, s);
		return;
	}
	gr_internal_string_clipped(canvas, cv_font, x, y, s);
}

void gr_ustring(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
#if DXX_USE_OGL
	if (canvas.cv_bitmap.get_type() == bm_mode::ogl)
	{
		ogl_internal_string(canvas, cv_font, x, y, s);
		return;
	}
#endif
	
	if (canvas.cv_font->ft_flags & FT_COLOR) {
		gr_internal_color_string(canvas, cv_font, x, y, s);
	}
	else
		gr_ustring_mono(canvas, cv_font, x, y, s);
}

void gr_get_string_size(const grs_font &cv_font, const char *s, int *const string_width, int *const string_height, int *const average_width)
{
	gr_get_string_size(cv_font, s, string_width, string_height, average_width, UINT_MAX);
}

void gr_get_string_size(const grs_font &cv_font, const char *s, int *const string_width, int *const string_height, int *const average_width, const unsigned max_chars_per_line)
{
	float longest_width=0.0,string_width_f=0.0;
	unsigned lines = 0;
	if (average_width)
		*average_width = cv_font.ft_w;
	if (!string_width && !string_height)
		return;
	if (s)
	{
		unsigned remaining_chars_this_line = max_chars_per_line;
		while (*s)
		{
			if (*s == '\n')
			{
				if (longest_width < string_width_f)
					longest_width = string_width_f;
				string_width_f = 0;
				const auto os = s;
				while (*++s == '\n')
				{
				}
				lines += s - os;
				if (!*s)
					break;
				remaining_chars_this_line = max_chars_per_line;
			}

			const auto &result = get_char_width<float>(cv_font, s[0], s[1]);
			const auto &spacing = result.spacing;
			string_width_f += spacing;
			s++;
			if (!--remaining_chars_this_line)
				break;
		}
	}
	if (string_width)
		*string_width = std::max(longest_width, string_width_f);
	if (string_height)
	{
		const auto fontscale_y = FONTSCALE_Y(cv_font.ft_h);
		*string_height = static_cast<int>(fontscale_y + (lines * (fontscale_y + FSPACY(1))));
	}
}

std::pair<const char *, unsigned> gr_get_string_wrap(const grs_font &cv_font, const char *s, unsigned limit)
{
	assert(s);
	float string_width_f=0.0;
	const float limit_f = limit;
	for (uint8_t c; (c = *s); ++s)
	{
		const auto &&spacing = get_char_width<float>(cv_font, c, s[1]).spacing;
		const float next_string_width = string_width_f + spacing;
		if (next_string_width >= limit_f)
			break;
		string_width_f = next_string_width;
	}
	return {s, static_cast<unsigned>(string_width_f)};
}

template <void (&F)(grs_canvas &, const grs_font &, int, int, const char *)>
void (gr_printt)(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const format, ...)
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	F(canvas, cv_font, x, y, buffer);
}

template void gr_printt<gr_string>(grs_canvas &, const grs_font &, int, int, const char *, ...);
template void gr_printt<gr_ustring>(grs_canvas &, const grs_font &, int, int, const char *, ...);

void gr_close_font(std::unique_ptr<grs_font> font)
{
	if (font)
	{
#if DXX_USE_OGL
		gr_free_bitmap_data(font->ft_parent_bitmap);
#endif
		//find font in list
		if (!(font->ft_flags & FT_COLOR))
			return;
		auto e = end(open_font);
		auto i = std::find(begin(open_font), e, font.get());
		if (i == e)
			throw std::logic_error("closing non-open font");
		*i = nullptr;
	}
}

//remap a font, re-reading its data & palette
static void gr_remap_font(grs_font *font, const char *fontname);

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts()
{
	range_for (auto font, open_font)
	{
		if (font)
			gr_remap_font(font, &font->ft_filename[0]);
	}
}

/*
 * reads a grs_font structure from a PHYSFS_File
 */
static void grs_font_read(grs_font *gf, PHYSFS_File *fp)
{
	gf->ft_w = PHYSFSX_readShort(fp);
	gf->ft_h = PHYSFSX_readShort(fp);
	gf->ft_flags = PHYSFSX_readShort(fp);
	gf->ft_baseline = PHYSFSX_readShort(fp);
	gf->ft_minchar = PHYSFSX_readByte(fp);
	gf->ft_maxchar = PHYSFSX_readByte(fp);
	PHYSFSX_readShort(fp);
	gf->ft_data = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(PHYSFSX_readInt(fp)) - GRS_FONT_SIZE);
	PHYSFSX_readInt(fp);
	gf->ft_widths = reinterpret_cast<int16_t *>(static_cast<uintptr_t>(PHYSFSX_readInt(fp)) - GRS_FONT_SIZE);
	gf->ft_kerndata = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(PHYSFSX_readInt(fp)) - GRS_FONT_SIZE);
}

static std::unique_ptr<grs_font> gr_internal_init_font(const char *fontname)
{
	const uint8_t *ptr;
	uint8_t *ft_data;
	struct {
		array<char, 4> magic;
		unsigned datasize;	//size up to (but not including) palette
	} file_header;

	auto fontfile = PHYSFSX_openReadBuffered(fontname);

	if (!fontfile) {
		con_printf(CON_VERBOSE, "Can't open font file %s", fontname);
		return {};
	}

	static_assert(sizeof(file_header) == 8, "file header size error");
	if (PHYSFS_read(fontfile, &file_header, sizeof(file_header), 1) != 1 ||
		memcmp(file_header.magic.data(), "PSFN", 4) ||
		(file_header.datasize = INTEL_INT(file_header.datasize)) < GRS_FONT_SIZE)
	{
		con_printf(CON_NORMAL, "Invalid header in font file %s", fontname);
		return {};
	}

	file_header.datasize -= GRS_FONT_SIZE; // subtract the size of the header.
	const auto &datasize = file_header.datasize;

	auto font = make_unique<grs_font>();
	grs_font_read(font.get(), fontfile);

	const unsigned nchars = font->ft_maxchar - font->ft_minchar + 1;
	std::size_t ft_chars_storage = (font->ft_flags & FT_PROPORTIONAL)
		? sizeof(uint8_t *) * nchars
		: 0;

	auto ft_allocdata = make_unique<uint8_t[]>(datasize + ft_chars_storage);
	const auto font_data = &ft_allocdata[ft_chars_storage];
	if (PHYSFS_read(fontfile, font_data, 1, datasize) != datasize)
	{
		con_printf(CON_URGENT, "Insufficient data in font file %s", fontname);
		return {};
	}

	if (font->ft_flags & FT_PROPORTIONAL) {
		const auto offset_widths = reinterpret_cast<uintptr_t>(font->ft_widths);
		auto w = reinterpret_cast<short *>(&font_data[offset_widths]);
		if (offset_widths >= datasize || offset_widths + (nchars * sizeof(*w)) >= datasize)
		{
			con_printf(CON_URGENT, "Missing widths in font file %s", fontname);
			return {};
		}
		font->ft_widths = w;
		const auto offset_data = reinterpret_cast<uintptr_t>(font->ft_data);
		if (offset_data >= datasize)
		{
			con_printf(CON_URGENT, "Missing data in font file %s", fontname);
			return {};
		}
		font->ft_data = ptr = ft_data = &font_data[offset_data];
		const auto ft_chars = reinterpret_cast<const uint8_t **>(ft_allocdata.get());
		font->ft_chars = reinterpret_cast<const uint8_t *const *>(ft_chars);

		const unsigned is_color = font->ft_flags & FT_COLOR;
		const unsigned ft_h = font->ft_h;
		std::generate_n(ft_chars, nchars, [is_color, ft_h, &w, &ptr]{
			const unsigned s = INTEL_SHORT(*w);
			if (words_bigendian)
				*w = static_cast<uint16_t>(s);
			++w;
			const auto r = ptr;
			ptr += ft_h * (is_color ? s : BITS_TO_BYTES(s));
			return r;
		});
	} else  {

		font->ft_data   = ft_data = font_data;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
	{
		const auto offset_kerndata = reinterpret_cast<uintptr_t>(font->ft_kerndata);
		if (datasize <= offset_kerndata)
		{
			con_printf(CON_URGENT, "Missing kerndata in font file %s", fontname);
			return {};
		}
		const auto begin_kerndata = reinterpret_cast<const unsigned char *>(&font_data[offset_kerndata]);
		const auto end_font_data = &font_data[datasize - ((datasize - offset_kerndata + 2) % 3)];
		for (auto cur_kerndata = begin_kerndata;; cur_kerndata += 3)
		{
			if (cur_kerndata == end_font_data)
			{
				con_printf(CON_URGENT, "Unterminated kerndata in font file %s", fontname);
				return {};
			}
			if (*cur_kerndata == kerndata_terminator)
				break;
		}
		font->ft_kerndata = begin_kerndata;
	}
	else
		font->ft_kerndata = nullptr;

	if (font->ft_flags & FT_COLOR) {		//remap palette
		palette_array_t palette;
		array<uint8_t, 256> colormap;
		/* `freq` exists so that decode_data can write to it, but it is
		 * otherwise unused.  `decode_data` is not guaranteed to write
		 * to every element, so `freq` must not be read without first
		 * adding an initialization step.
		 */
		array<bool, 256> freq;

		PHYSFS_read(fontfile,&palette[0],sizeof(palette[0]),palette.size());		//read the palette

		build_colormap_good(palette, colormap);

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(ft_data, ptr - font->ft_data, colormap, freq );
	}
	fontfile.reset();
	//set curcanv vars

	auto &ft_filename = font->ft_filename;
	font->ft_allocdata = move(ft_allocdata);
	strncpy(&ft_filename[0], fontname, ft_filename.size());
	return font;
}

grs_font_ptr gr_init_font(grs_canvas &canvas, const char *fontname)
{
	auto font = gr_internal_init_font(fontname);
	if (!font)
		return {};

	canvas.cv_font        = font.get();
	canvas.cv_font_fg_color    = 0;
	canvas.cv_font_bg_color    = 0;
	if (font->ft_flags & FT_COLOR)
	{
		auto e = end(open_font);
		auto i = std::find(begin(open_font), e, static_cast<grs_font *>(nullptr));
		if (i == e)
			throw std::logic_error("too many open fonts");
		*i = font.get();
	}
#if DXX_USE_OGL
	ogl_init_font(font.get());
#endif
	return grs_font_ptr(font.release());
}

//remap a font by re-reading its data & palette
void gr_remap_font(grs_font *font, const char *fontname)
{
	auto n = gr_internal_init_font(fontname);
	if (!n)
		return;
	if (!(n->ft_flags & FT_COLOR))
		return;
#if DXX_USE_OGL
	gr_free_bitmap_data(font->ft_parent_bitmap);
#endif
	*font = std::move(*n.get());
#if DXX_USE_OGL
	ogl_init_font(font);
#endif
}

void gr_set_curfont(grs_canvas &canvas, const grs_font *n)
{
	canvas.cv_font = n;
}

void gr_set_fontcolor(grs_canvas &canvas, const int fg_color, const int bg_color)
{
	canvas.cv_font_fg_color = fg_color;
	canvas.cv_font_bg_color = bg_color;
}

template <bool masked_draws_background>
static int gr_internal_string_clipped_template(grs_canvas &canvas, const grs_font &cv_font, int x, int y, const char *const s)
{
	const char * text_ptr, * next_row, * text_ptr1;
	int letter;
	int x1 = x, last_x;

	next_row = s;

	const auto &&INFONT = font_character_extent(cv_font);
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;
	const auto cv_font_bg_color = canvas.cv_font_bg_color;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(canvas, cv_font, text_ptr1);

		last_x = x;

		for (int r=0; r < cv_font.ft_h; r++) {
			text_ptr = text_ptr1;
			x = last_x;

			for (; const uint8_t c0 = *text_ptr; ++text_ptr)
			{
				if (c0 == '\n')
				{
					next_row = &text_ptr[1];
					break;
				}
				if (c0 == CC_COLOR)
				{
					canvas.cv_font_fg_color = static_cast<uint8_t>(*++text_ptr);
					continue;
				}

				if (c0 == CC_LSPACING)
				{
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 1;
					continue;
				}

				const auto underline = unlikely(c0 == CC_UNDERLINE)
					? ++text_ptr, r == cv_font.ft_baseline + 2 || r == cv_font.ft_baseline + 3
					: 0;
				const uint8_t c = *text_ptr;
				const auto &result = get_char_width<int>(cv_font, c, text_ptr[1]);
				const auto &width = result.width;
				const auto &spacing = result.spacing;

				letter = c - cv_font.ft_minchar;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					continue;
				}
				const auto cv_font_fg_color = canvas.cv_font_fg_color;
				auto color = cv_font_fg_color;
				if (width)
				{
				if (underline)	{
					for (uint_fast32_t i = width; i--;)
					{
						gr_pixel(canvas.cv_bitmap, x++, y, color);
					}
				} else {
					auto fp = proportional ? cv_font.ft_chars[letter] : cv_font.ft_data + letter * BITS_TO_BYTES(width) * cv_font.ft_h;
					fp += BITS_TO_BYTES(width)*r;

					/* Setting bits=0 is a dead store, but is necessary to
					 * prevent -Og -Wuninitialized from issuing a bogus
					 * warning.  -Og does not see that bits_remaining=0
					 * guarantees that bits will be initialized from *fp before
					 * it is read.
					 */
					uint8_t bits_remaining = 0, bits = 0;

					for (uint_fast32_t i = width; i--; ++x, --bits_remaining)
					{
						if (!bits_remaining)
						{
							bits = *fp++;
							bits_remaining = 8;
						}
						const auto bit_enabled = (bits & 0x80);
						bits <<= 1;
						if (masked_draws_background)
							color = bit_enabled ? cv_font_fg_color : cv_font_bg_color;
						else
						{
							if (!bit_enabled)
								continue;
						}
						gr_pixel(canvas.cv_bitmap, x, y, color);
					}
				}
				}
				x += spacing-width;		//for kerning
			}
			y++;
		}
	}
	return 0;
}

static int gr_internal_string_clipped_m(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	return gr_internal_string_clipped_template<true>(canvas, cv_font, x, y, s);
}

static int gr_internal_string_clipped(grs_canvas &canvas, const grs_font &cv_font, const int x, const int y, const char *const s)
{
	return gr_internal_string_clipped_template<false>(canvas, cv_font, x, y, s);
}

}
