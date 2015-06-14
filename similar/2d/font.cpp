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
#include "byteutil.h"
#include "common/2d/bitmap.h"
#include "makesig.h"
#include "physfsx.h"
#include "gamefont.h"
#include "console.h"
#include "config.h"
#include "inferno.h"
#ifdef OGL
#include "ogl_init.h"
#endif

#include "compiler-array.h"
#include "compiler-range_for.h"
#include "compiler-make_unique.h"
#include "partial_range.h"

#define FONTSCALE_X(x) ((float)(x)*(FNTScaleX))
#define FONTSCALE_Y(x) ((float)(x)*(FNTScaleY))

#define MAX_OPEN_FONTS	50

struct openfont
{
	array<char, FILENAME_LEN> filename;
	// Unowned
	grs_font *ptr;
	std::unique_ptr<uint8_t[]> dataptr;
};

//list of open fonts, for use (for now) for palette remapping
static array<openfont, MAX_OPEN_FONTS> open_font;

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

static int gr_internal_string_clipped(int x, int y, const char *s );
static int gr_internal_string_clipped_m(int x, int y, const char *s );

static const uint8_t *find_kern_entry(const grs_font &font, const uint8_t first, const uint8_t second)
{
	auto p = font.ft_kerndata;

	while (*p!=255)
		if (p[0]==first && p[1]==second)
			return p;
		else p+=3;
	return NULL;
}

//takes the character AFTER being offset into font
static inline bool INFONT(const unsigned c)
{
	return c <= grd_curcanv->cv_font->ft_maxchar - grd_curcanv->cv_font->ft_minchar;
}

namespace {

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
static get_char_width_result<T> get_char_width(const uint8_t c, const uint8_t c2)
{
	const auto &cv_font = *grd_curcanv->cv_font;
	const unsigned letter = c - cv_font.ft_minchar;
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;

	if (!INFONT(letter)) {				//not in font, draw as space
		return {0, static_cast<T>(proportional ? FONTSCALE_X(cv_font.ft_w) / 2 : cv_font.ft_w)};
	}
	const T width = proportional ? FONTSCALE_X(cv_font.ft_widths[letter]) : cv_font.ft_w;
	if (ft_flags & FT_KERNED) 
	{
		if (!(c2==0 || c2=='\n')) {
			const unsigned letter2 = c2 - cv_font.ft_minchar;

			if (INFONT(letter2)) {
				const auto p = find_kern_entry(cv_font, letter, letter2);
				if (p)
					return {width, static_cast<T>(FONTSCALE_X(p[2]))};
			}
		}
	}
	return {width, width};
}

static int get_centered_x(const char *s)
{
	float w;
	for (w=0;*s!=0 && *s!='\n';s++) {
		if (*s<=0x06) {
			if (*s<=0x03)
				s++;
			continue;//skip color codes.
		}
		w += get_char_width<float>(s[0],s[1]).spacing;
	}

	return ((grd_curcanv->cv_bitmap.bm_w - w) / 2);
}

//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color type thing would be better, but that would be way too much trouble for little gain
const int gr_message_color_level=1;
#define CHECK_EMBEDDED_COLORS() if ((*text_ptr >= 0x01) && (*text_ptr <= 0x02)) { \
		text_ptr++; \
		if (*text_ptr){ \
			if (gr_message_color_level >= *(text_ptr-1)) \
				grd_curcanv->cv_font_fg_color = (unsigned char)*text_ptr; \
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
			grd_curcanv->cv_font_fg_color=(unsigned char)orig_color; \
		text_ptr++; \
	}

template <bool masked_draws_background>
static int gr_internal_string0_template(int x, int y, const char *s)
{
	const auto &cv_font = *grd_curcanv->cv_font;
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;
	const auto cv_font_bg_color = grd_curcanv->cv_font_bg_color;
	int	skip_lines = 0;

	unsigned int VideoOffset1;

	//to allow easy reseting to default string color with colored strings -MPM
	const auto orig_color = grd_curcanv->cv_font_fg_color;
	VideoOffset1 = y * ROWSIZE + x;
	auto next_row = s;
	while (next_row != NULL )
	{
		const auto text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
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
					grd_curcanv->cv_font_fg_color = static_cast<uint8_t>(*++text_ptr);
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
				const auto &result = get_char_width<int>(c, text_ptr[1]);
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

				if (underline)
				{
					std::fill_n(&DATA[VideoOffset], width, grd_curcanv->cv_font_fg_color);
					VideoOffset += width;
				}
				else
				{
					auto fp = proportional ? cv_font.ft_chars[letter] : &cv_font.ft_data[letter * BITS_TO_BYTES(width) * cv_font.ft_h];
					fp += BITS_TO_BYTES(width)*r;

					uint8_t BitMask = 0, bits;

					const auto cv_font_fg_color = grd_curcanv->cv_font_fg_color;
					for (uint_fast32_t i = width; i--; ++VideoOffset, BitMask >>= 1)
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}

						const auto bit_enabled = (bits & BitMask);
						if (!masked_draws_background)
						{
							if (!bit_enabled)
								continue;
						}
						DATA[VideoOffset] = bit_enabled ? cv_font_fg_color : cv_font_bg_color;
					}
				}
				VideoOffset += spacing-width;
			}
			VideoOffset1 += ROWSIZE;
			y++;
		}
		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

static int gr_internal_string0(int x, int y, const char *s )
{
	return gr_internal_string0_template<true>(x, y, s);
}

static int gr_internal_string0m(int x, int y, const char *s )
{
	return gr_internal_string0_template<false>(x, y, s);
}

#ifndef OGL

static int gr_internal_color_string(int x, int y, const char *s )
{
//a bitmap for the character
	grs_bitmap char_bm = {};
	char_bm.bm_type = BM_LINEAR;
	char_bm.bm_flags = BM_FLAG_TRANSPARENT;
	unsigned char * fp;
	const char *text_ptr, *next_row, *text_ptr1;
	int letter;
	int xx,yy;

	char_bm.bm_h = grd_curcanv->cv_font->ft_h;		//set height for chars of this font

	next_row = s;

	yy = y;

	const auto &&fspacy = FSPACY();
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = get_centered_x(text_ptr);

		while (*text_ptr)
		{
			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += grd_curcanv->cv_font->ft_h + fspacy(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			const auto &result = get_char_width<int>(text_ptr[0], text_ptr[1]);
			const auto &width = result.width;
			const auto &spacing = result.spacing;

			if (!INFONT(letter)) {	//not in font, draw as space
				xx += spacing;
				text_ptr++;
				continue;
			}

			if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
				fp = grd_curcanv->cv_font->ft_chars[letter];
			else
				fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

			gr_init_bitmap(char_bm, BM_LINEAR, 0, 0, width, grd_curcanv->cv_font->ft_h, width, fp);
			gr_bitmapm(xx,yy,char_bm);

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
		Error("couldn't fit font?\n");
}

static void ogl_init_font(grs_font * font)
{
	int oglflags = OGL_FLAG_ALPHA;
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int w,h,tw,th,curx=0,cury=0;
	unsigned char *fp;
	ubyte *data;
	int gap=1; // x/y offset between the chars so we can filter

	th = tw = 0xffff;

	ogl_font_choose_size(font,gap,&tw,&th);
	MALLOC(data, ubyte, tw*th);
	memset(data, TRANSPARENCY_COLOR, tw * th); // map the whole data with transparency so we won't have borders if using gap
	gr_init_bitmap(font->ft_parent_bitmap,BM_LINEAR,0,0,tw,th,tw,data);
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
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * w*h;
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
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * BITS_TO_BYTES(w)*h;
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
	ogl_loadbmtexture_f(font->ft_parent_bitmap, GameCfg.TexFilt);
}

static int ogl_internal_string(int x, int y, const char *s )
{
	const char * text_ptr, * next_row, * text_ptr1;
	int letter;
	int xx,yy;
	int orig_color=grd_curcanv->cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM
	int underline;

	next_row = s;

	yy = y;

	if (grd_curscreen->sc_canvas.cv_bitmap.bm_type != BM_OGL)
		Error("carp.\n");
	const auto &&fspacy = FSPACY();
	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = get_centered_x(text_ptr);

		while (*text_ptr)
		{
			int ft_w;

			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += FONTSCALE_Y(grd_curcanv->cv_font->ft_h) + fspacy(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			const auto &result = get_char_width<int>(text_ptr[0], text_ptr[1]);
			const auto &spacing = result.spacing;

			underline = 0;
			if (!INFONT(letter) || (unsigned char)*text_ptr <= 0x06) //not in font, draw as space
			{
				CHECK_EMBEDDED_COLORS() else{
					xx += spacing;
					text_ptr++;
				}
				
				if (underline)
				{
					ubyte save_c = (unsigned char) COLOR;
					
					gr_setcolor(grd_curcanv->cv_font_fg_color);
					gr_rect(xx, yy + grd_curcanv->cv_font->ft_baseline + 2, xx + grd_curcanv->cv_font->ft_w, yy + grd_curcanv->cv_font->ft_baseline + 3);
					gr_setcolor(save_c);
				}

				continue;
			}
			
			if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
				ft_w = grd_curcanv->cv_font->ft_widths[letter];
			else
				ft_w = grd_curcanv->cv_font->ft_w;

			if (grd_curcanv->cv_font->ft_flags&FT_COLOR)
				ogl_ubitmapm_cs(xx,yy,FONTSCALE_X(ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),grd_curcanv->cv_font->ft_bitmaps[letter],-1,F1_0);
			else{
				if (grd_curcanv->cv_bitmap.bm_type==BM_OGL)
					ogl_ubitmapm_cs(xx,yy,ft_w*(FONTSCALE_X(grd_curcanv->cv_font->ft_w)/grd_curcanv->cv_font->ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),grd_curcanv->cv_font->ft_bitmaps[letter],grd_curcanv->cv_font_fg_color,F1_0);
				else
					Error("ogl_internal_string: non-color string to non-ogl dest\n");
			}

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

static int gr_internal_color_string(int x, int y, const char *s ){
	return ogl_internal_string(x,y,s);
}
#endif //OGL

void gr_string(int x, int y, const char *s )
{
	int w, h, aw;
	int clipped=0;

	Assert(grd_curcanv->cv_font != NULL);

	if ( x == 0x8000 )	{
		if ( y<0 ) clipped |= 1;
		gr_get_string_size(s, &w, &h, &aw );
		// for x, since this will be centered, only look at
		// width.
		if ( w > grd_curcanv->cv_bitmap.bm_w ) clipped |= 1;
		if ( (y+h) > grd_curcanv->cv_bitmap.bm_h ) clipped |= 1;

		if ( (y+h) < 0 ) clipped |= 2;
		if ( y > grd_curcanv->cv_bitmap.bm_h ) clipped |= 2;

	} else {
		if ( (x<0) || (y<0) ) clipped |= 1;
		gr_get_string_size(s, &w, &h, &aw );
		if ( (x+w) > grd_curcanv->cv_bitmap.bm_w ) clipped |= 1;
		if ( (y+h) > grd_curcanv->cv_bitmap.bm_h ) clipped |= 1;
		if ( (x+w) < 0 ) clipped |= 2;
		if ( (y+h) < 0 ) clipped |= 2;
		if ( x > grd_curcanv->cv_bitmap.bm_w ) clipped |= 2;
		if ( y > grd_curcanv->cv_bitmap.bm_h ) clipped |= 2;
	}

	if ( !clipped )
	{
		gr_ustring(x, y, s );
		return;
	}

	if ( clipped & 2 )	{
		// Completely clipped...
		return;
	}

	// Partially clipped...
#ifdef OGL
	if (TYPE==BM_OGL)
	{
		ogl_internal_string(x,y,s);
		return;
	}
#endif

	if (grd_curcanv->cv_font->ft_flags & FT_COLOR)
	{
		gr_internal_color_string( x, y, s);
		return;
	}

	if ( grd_curcanv->cv_font_bg_color == -1)
	{
		gr_internal_string_clipped_m( x, y, s );
		return;
	}

	gr_internal_string_clipped( x, y, s );
}

void gr_ustring(int x, int y, const char *s )
{
#ifdef OGL
	if (TYPE==BM_OGL)
	{
		ogl_internal_string(x,y,s);
		return;
	}
#endif
	
	if (grd_curcanv->cv_font->ft_flags & FT_COLOR) {

		gr_internal_color_string(x,y,s);

	}
	else
		switch( TYPE )
		{
		case BM_LINEAR:
			if ( grd_curcanv->cv_font_bg_color == -1)
				gr_internal_string0m(x,y,s);
			else
				gr_internal_string0(x,y,s);
		}
	return;
}


void gr_get_string_size(const char *s, int *string_width, int *string_height, int *average_width )
{
	float longest_width=0.0,string_width_f=0.0;
	unsigned lines = 0;
	const auto &cv_font = *grd_curcanv->cv_font;
	*average_width = cv_font.ft_w;
	if (s != NULL )
	{
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
			}

			const auto &result = get_char_width<float>(s[0], s[1]);
			const auto &spacing = result.spacing;
			string_width_f += spacing;
			s++;
		}
	}
	*string_width = std::max(longest_width, string_width_f);
	const auto fontscale_y = FONTSCALE_Y(cv_font.ft_h);
	const float string_height_f = fontscale_y + (lines * (fontscale_y + FSPACY(1)));
	*string_height = string_height_f;
}


void (gr_uprintf)( int x, int y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);
	va_end(args);
	gr_ustring( x, y, buffer );
}

void (gr_printf)( int x, int y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);
	va_end(args);
	gr_string( x, y, buffer );
}

void gr_close_font(std::unique_ptr<grs_font> font)
{
	if (font)
	{
		//find font in list
		auto e = end(open_font);
		auto i = std::find_if(begin(open_font), e, [&font](const openfont &o) { return o.ptr == font.get(); });
		if (i == e)
			throw std::logic_error("closing non-open font");
#ifdef OGL
		gr_free_bitmap_data(font->ft_parent_bitmap);
#endif
		auto &f = *i;
		f.dataptr.reset();
		f.ptr = nullptr;
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//remap a font, re-reading its data & palette
static void gr_remap_font( grs_font *font, const char * fontname, uint8_t *font_data );

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts()
{
	range_for (auto &i, open_font)
	{
		auto font = i.ptr;
		if (font && (font->ft_flags & FT_COLOR))
			gr_remap_font(font, &i.filename[0], i.dataptr.get());
	}
}

void gr_remap_mono_fonts()
{
	con_printf (CON_DEBUG, "gr_remap_mono_fonts ()");
	range_for (auto &i, open_font)
	{
		auto font = i.ptr;
		if (font && !(font->ft_flags & FT_COLOR))
			gr_remap_font(font, &i.filename[0], i.dataptr.get());
	}
}
#endif

/*
 * reads a grs_font structure from a PHYSFS_file
 */
static void grs_font_read(grs_font *gf, PHYSFS_file *fp)
{
	gf->ft_w = PHYSFSX_readShort(fp);
	gf->ft_h = PHYSFSX_readShort(fp);
	gf->ft_flags = PHYSFSX_readShort(fp);
	gf->ft_baseline = PHYSFSX_readShort(fp);
	gf->ft_minchar = PHYSFSX_readByte(fp);
	gf->ft_maxchar = PHYSFSX_readByte(fp);
	gf->ft_bytewidth = PHYSFSX_readShort(fp);
	gf->ft_data = (ubyte *)((size_t)PHYSFSX_readInt(fp) - GRS_FONT_SIZE);
	PHYSFSX_readInt(fp);
	gf->ft_widths = (short *)((size_t)PHYSFSX_readInt(fp) - GRS_FONT_SIZE);
	gf->ft_kerndata = (ubyte *)((size_t)PHYSFSX_readInt(fp) - GRS_FONT_SIZE);
}

grs_font_ptr gr_init_font( const char * fontname )
{
	unsigned char * ptr;
	int nchars;
	char file_id[4];
	int datasize;	//size up to (but not including) palette

	//find free font slot
	auto e = end(open_font);
	auto i = std::find_if(begin(open_font), e, [](const openfont &o) { return o.ptr == NULL; });
	if (i == e)
		throw std::logic_error("too many open fonts");
	auto &f = *i;

	strncpy(&f.filename[0], fontname, FILENAME_LEN);

	auto fontfile = PHYSFSX_openReadBuffered(fontname);

	if (!fontfile) {
		con_printf(CON_VERBOSE, "Can't open font file %s", fontname);
		return {};
	}

	PHYSFS_read(fontfile, file_id, 4, 1);
	if (memcmp( file_id, "PSFN", 4 )) {
		con_printf(CON_NORMAL, "File %s is not a font file", fontname);
		return {};
	}

	datasize = PHYSFSX_readInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	auto font = make_unique<grs_font>();
	grs_font_read(font.get(), fontfile);

	auto font_data = make_unique<uint8_t[]>(datasize);
	PHYSFS_read(fontfile, font_data, 1, datasize);

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = make_unique<uint8_t *[]>(nchars);

		ptr = font->ft_data;

		for (int i=0; i < nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = font_data.get();
		font->ft_chars.reset();
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		palette_array_t palette;
		array<uint8_t, 256> colormap;
		array<unsigned, 256> freq;

		PHYSFS_read(fontfile,&palette[0],sizeof(palette[0]),palette.size());		//read the palette

		build_colormap_good( palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );
	}
	fontfile.reset();
	//set curcanv vars

	grd_curcanv->cv_font        = font.get();
	grd_curcanv->cv_font_fg_color    = 0;
	grd_curcanv->cv_font_bg_color    = 0;

#ifdef OGL
	ogl_init_font(font.get());
#endif

	f.ptr = font.get();
	f.dataptr = move(font_data);
	return grs_font_ptr(font.release());
}

#if defined(DXX_BUILD_DESCENT_II)
//remap a font by re-reading its data & palette
void gr_remap_font( grs_font *font, const char * fontname, uint8_t *font_data )
{
	int nchars;
	char file_id[4];
	int datasize;        //size up to (but not including) palette
	unsigned char *ptr;

	if (! (font->ft_flags & FT_COLOR))
		return;

	auto fontfile = PHYSFSX_openReadBuffered(fontname);

	if (!fontfile)
		Error( "Can't open font file %s", fontname );

	PHYSFS_read(fontfile, file_id, 4, 1);
	if ( !strncmp( file_id, "NFSP", 4 ) )
		Error( "File %s is not a font file", fontname );

	datasize = PHYSFSX_readInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	font->ft_chars.reset();
	grs_font_read(font, fontfile); // have to reread in case mission hogfile overrides font.

	PHYSFS_read(fontfile, font_data, 1, datasize);  //read raw data

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = make_unique<uint8_t *[]>(nchars);

		ptr = font->ft_data;

		for (int i=0; i< nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = (unsigned char *) font_data;
		font->ft_chars.reset();
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		palette_array_t palette;
		array<uint8_t, 256> colormap;
		array<unsigned, 256> freq;

		PHYSFS_read(fontfile,&palette[0],sizeof(palette[0]),palette.size());		//read the palette
		build_colormap_good( palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );

	}
#ifdef OGL
	gr_free_bitmap_data(font->ft_parent_bitmap);
	ogl_init_font(font);
#endif
}
#endif

void gr_set_curfont(const grs_font *n)
{
	grd_curcanv->cv_font = n;
}

void gr_set_fontcolor( int fg_color, int bg_color )
{
	grd_curcanv->cv_font_fg_color    = fg_color;
	grd_curcanv->cv_font_bg_color    = bg_color;
}

template <bool masked_draws_background>
static int gr_internal_string_clipped_template(int x, int y, const char *s)
{
	const char * text_ptr, * next_row, * text_ptr1;
	int letter;
	int x1 = x, last_x;

	next_row = s;

	const auto &cv_font = *grd_curcanv->cv_font;
	const auto ft_flags = cv_font.ft_flags;
	const auto proportional = ft_flags & FT_PROPORTIONAL;
	const auto cv_font_bg_color = grd_curcanv->cv_font_bg_color;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(text_ptr1);

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
					grd_curcanv->cv_font_fg_color = static_cast<uint8_t>(*++text_ptr);
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
				const auto &result = get_char_width<int>(c, text_ptr[1]);
				const auto &width = result.width;
				const auto &spacing = result.spacing;

				letter = c - cv_font.ft_minchar;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					continue;
				}
				const auto cv_font_fg_color = grd_curcanv->cv_font_fg_color;
				gr_setcolor(cv_font_fg_color);
				if (underline)	{
					for (uint_fast32_t i = width; i--;)
					{
						gr_pixel( x++, y );
					}
				} else {
					auto fp = proportional ? cv_font.ft_chars[letter] : cv_font.ft_data + letter * BITS_TO_BYTES(width) * cv_font.ft_h;
					fp += BITS_TO_BYTES(width)*r;

					uint8_t BitMask = 0, bits;

					for (uint_fast32_t i = width; i--; ++x, BitMask >>= 1)
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						const auto bit_enabled = (bits & BitMask);
						if (masked_draws_background)
							gr_setcolor(bit_enabled ? cv_font_fg_color : cv_font_bg_color);
						else
						{
							if (!bit_enabled)
								continue;
						}
						gr_pixel(x, y);
					}
				}
				x += spacing-width;		//for kerning
			}
			y++;
		}
	}
	return 0;
}

static int gr_internal_string_clipped_m(int x, int y, const char *s )
{
	return gr_internal_string_clipped_template<true>(x, y, s);
}

static int gr_internal_string_clipped(int x, int y, const char *s )
{
	return gr_internal_string_clipped_template<false>(x, y, s);
}
