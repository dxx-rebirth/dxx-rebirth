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
 * Graphical routines for drawing fonts.
 *
 */

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
#include "byteswap.h"
#include "bitmap.h"
#include "makesig.h"
#include "gamefont.h"
#include "console.h"
#include "config.h"
#include "inferno.h"
#ifdef OGL
#include "ogl_init.h"
#endif

#define FONTSCALE_X(x) ((float)(x)*(FNTScaleX))
#define FONTSCALE_Y(x) ((float)(x)*(FNTScaleY))

#define MAX_OPEN_FONTS	50

typedef struct openfont {
	char filename[FILENAME_LEN];
	grs_font *ptr;
	char *dataptr;
} openfont;

//list of open fonts, for use (for now) for palette remapping
openfont open_font[MAX_OPEN_FONTS];

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

int gr_internal_string_clipped(int x, int y, const char *s );
int gr_internal_string_clipped_m(int x, int y, const char *s );

ubyte *find_kern_entry(grs_font *font,ubyte first,ubyte second)
{
	ubyte *p=font->ft_kerndata;

	while (*p!=255)
		if (p[0]==first && p[1]==second)
			return p;
		else p+=3;

	return NULL;

}

//takes the character AFTER being offset into font
#define INFONT(_c) ((_c >= 0) && (_c <= grd_curcanv->cv_font->ft_maxchar-grd_curcanv->cv_font->ft_minchar))

//takes the character BEFORE being offset into current font
void get_char_width(ubyte c,ubyte c2,int *width,int *spacing)
{
	int letter;

	letter = c-grd_curcanv->cv_font->ft_minchar;

	if (!INFONT(letter)) {				//not in font, draw as space
		*width=0;
		if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
			*spacing = FONTSCALE_X(grd_curcanv->cv_font->ft_w)/2;
		else
			*spacing = grd_curcanv->cv_font->ft_w;
		return;
	}

	if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
		*width = FONTSCALE_X(grd_curcanv->cv_font->ft_widths[letter]);
	else
		*width = grd_curcanv->cv_font->ft_w;

	*spacing = *width;

	if (grd_curcanv->cv_font->ft_flags & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2 = c2-grd_curcanv->cv_font->ft_minchar;

			if (INFONT(letter2)) {

				p = find_kern_entry(grd_curcanv->cv_font,(ubyte)letter,letter2);

				if (p)
					*spacing = FONTSCALE_X(p[2]);
			}
		}
	}
}

// Same as above but works with floats, which is better for string-size measurement while being bad for string composition of course
void get_char_width_f(ubyte c,ubyte c2,float *width,float *spacing)
{
	int letter;

	letter = c-grd_curcanv->cv_font->ft_minchar;

	if (!INFONT(letter)) {				//not in font, draw as space
		*width=0;
		if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
			*spacing = FONTSCALE_X(grd_curcanv->cv_font->ft_w)/2;
		else
			*spacing = grd_curcanv->cv_font->ft_w;
		return;
	}

	if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
		*width = FONTSCALE_X(grd_curcanv->cv_font->ft_widths[letter]);
	else
		*width = grd_curcanv->cv_font->ft_w;

	*spacing = *width;

	if (grd_curcanv->cv_font->ft_flags & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2 = c2-grd_curcanv->cv_font->ft_minchar;

			if (INFONT(letter2)) {

				p = find_kern_entry(grd_curcanv->cv_font,(ubyte)letter,letter2);

				if (p)
					*spacing = FONTSCALE_X(p[2]);
			}
		}
	}
}

int get_centered_x(const char *s)
{
	float w,w2,s2;

	for (w=0;*s!=0 && *s!='\n';s++) {
		if (*s<=0x06) {
			if (*s<=0x03)
				s++;
			continue;//skip color codes.
		}
		get_char_width_f(s[0],s[1],&w2,&s2);
		w += s2;
	}

	return ((grd_curcanv->cv_bitmap.bm_w - w) / 2);
}

//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color type thing would be better, but that would be way too much trouble for little gain
int gr_message_color_level=1;
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

int gr_internal_string0(int x, int y, const char *s )
{
	unsigned char * fp;
	const char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

        bits=0;

	VideoOffset1 = y * ROWSIZE + x;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
		}

		for (r=0; r<grd_curcanv->cv_font->ft_h; r++)
		{

			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			while (*text_ptr)
			{
				if (*text_ptr == '\n' )
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					grd_curcanv->cv_font_fg_color = (unsigned char)*(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )
				{
					if ((r==grd_curcanv->cv_font->ft_baseline+2) || (r==grd_curcanv->cv_font->ft_baseline+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += spacing;
					text_ptr++;
					continue;
				}

				if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
					fp = grd_curcanv->cv_font->ft_chars[letter];
				else
					fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

				if (underline)
					for (i=0; i< width; i++ )
						DATA[VideoOffset++] = (unsigned char) grd_curcanv->cv_font_fg_color;
				else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++ )
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}

						if (bits & BitMask)
							DATA[VideoOffset++] = (unsigned char) grd_curcanv->cv_font_fg_color;
						else
							DATA[VideoOffset++] = (unsigned char) grd_curcanv->cv_font_bg_color;
						BitMask >>= 1;
					}
				}

				VideoOffset += spacing-width;		//for kerning

				text_ptr++;
			}
			VideoOffset1 += ROWSIZE; y++;
		}
		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

int gr_internal_string0m(int x, int y, const char *s )
{
	unsigned char * fp;
	const char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	int orig_color=grd_curcanv->cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM

        bits = 0;

	VideoOffset1 = y * ROWSIZE + x;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
		}

		for (r=0; r<grd_curcanv->cv_font->ft_h; r++)
		{

			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			while (*text_ptr)
			{
				if (*text_ptr == '\n' )
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					grd_curcanv->cv_font_fg_color = (unsigned char)*(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					skip_lines = *(text_ptr+1) - '0';
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )
				{
					if ((r==grd_curcanv->cv_font->ft_baseline+2) || (r==grd_curcanv->cv_font->ft_baseline+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = (unsigned char)*text_ptr-grd_curcanv->cv_font->ft_minchar;

				if (!INFONT(letter) || (unsigned char) *text_ptr <= 0x06)	//not in font, draw as space
				{
					CHECK_EMBEDDED_COLORS() else{
						VideoOffset += spacing;
						text_ptr++;
					}
					continue;
				}

				if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
					fp = grd_curcanv->cv_font->ft_chars[letter];
				else
					fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

				if (underline)
					for (i=0; i< width; i++ )
						DATA[VideoOffset++] = (unsigned char) grd_curcanv->cv_font_fg_color;
				else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++ )
					{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}

						if (bits & BitMask)
							DATA[VideoOffset++] = (unsigned char) grd_curcanv->cv_font_fg_color;
						else
							VideoOffset++;
						BitMask >>= 1;
					}
				}
				text_ptr++;

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

#ifndef OGL
//a bitmap for the character
grs_bitmap char_bm = {
				0,0,0,0,		//x,y,w,h
				BM_LINEAR,		//type
				BM_FLAG_TRANSPARENT,	//flags
				0,			//rowsize
				NULL,			//data
				0,			//avg_color
				0			//unused
};

int gr_internal_color_string(int x, int y, const char *s )
{
	unsigned char * fp;
	const char *text_ptr, *next_row, *text_ptr1;
	int width, spacing,letter;
	int xx,yy;

	char_bm.bm_h = grd_curcanv->cv_font->ft_h;		//set height for chars of this font

	next_row = s;

	yy = y;

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
				yy += grd_curcanv->cv_font->ft_h+FSPACY(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			if (!INFONT(letter)) {	//not in font, draw as space
				xx += spacing;
				text_ptr++;
				continue;
			}

			if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
				fp = grd_curcanv->cv_font->ft_chars[letter];
			else
				fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

			gr_init_bitmap (&char_bm, BM_LINEAR, 0, 0, width, grd_curcanv->cv_font->ft_h, width, fp);
			gr_bitmapm(xx,yy,&char_bm);

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

#else //OGL

int get_font_total_width(grs_font * font){
	if (font->ft_flags & FT_PROPORTIONAL){
		int i,w=0,c=font->ft_minchar;
		for (i=0;c<=font->ft_maxchar;i++,c++){
			if (font->ft_widths[i]<0)
				Error("heh?\n");
			w+=font->ft_widths[i];
		}
		return w;
	}else{
		return font->ft_w*(font->ft_maxchar-font->ft_minchar+1);
	}
}
void ogl_font_choose_size(grs_font * font,int gap,int *rw,int *rh){
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int r,x,y,nc=0,smallest=999999,smallr=-1,tries;
	int smallprop=10000;
	int h,w;
	for (h=32;h<=256;h*=2){
//		h=pow2ize(font->ft_h*rows+gap*(rows-1));
		if (font->ft_h>h)continue;
		r=(h/(font->ft_h+gap));
		w=pow2ize((get_font_total_width(font)+(nchars-r)*gap)/r);
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

void ogl_init_font(grs_font * font)
{
	int oglflags = OGL_FLAG_ALPHA;
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int i,w,h,tw,th,x,y,curx=0,cury=0;
	unsigned char *fp;
	ubyte *data;
	int gap=1; // x/y offset between the chars so we can filter

	ogl_font_choose_size(font,gap,&tw,&th);
	MALLOC(data, ubyte, tw*th);
	memset(data, TRANSPARENCY_COLOR, tw * th); // map the whole data with transparency so we won't have borders if using gap
	gr_init_bitmap(&font->ft_parent_bitmap,BM_LINEAR,0,0,tw,th,tw,data);
	gr_set_transparent(&font->ft_parent_bitmap, 1);

	if (!(font->ft_flags & FT_COLOR))
		oglflags |= OGL_FLAG_NOCOLOR;
	ogl_init_texture(font->ft_parent_bitmap.gltexture = ogl_get_free_texture(), tw, th, oglflags); // have to init the gltexture here so the subbitmaps will find it.

	font->ft_bitmaps=(grs_bitmap*)d_malloc( nchars * sizeof(grs_bitmap));
	h=font->ft_h;

	for(i=0;i<nchars;i++)
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
			for (y=0;y<h;y++)
			{
				for (x=0;x<w;x++)
				{
					font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=fp[x+y*w];
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
							font->ft_parent_bitmap.bm_data[(curx+x+(cury+y)*tw)-1]=fp[x+y*w];

						// shift right border
						if (x==w-1 && i != 100)
							font->ft_parent_bitmap.bm_data[(curx+x+(cury+y)*tw)+1]=fp[x+y*w];
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
			for (y=0;y<h;y++){
				BitMask=0;
				for (x=0; x< w; x++ )
				{
					if (BitMask==0) {
						bits = *fp++;
						BitMask = 0x80;
					}

					if (bits & BitMask)
						font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=white;
					else
						font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=255;
					BitMask >>= 1;
				}
			}
		}
		gr_init_sub_bitmap(&font->ft_bitmaps[i],&font->ft_parent_bitmap,curx,cury,w,h);
		curx+=w+gap;
	}
	ogl_loadbmtexture_f(&font->ft_parent_bitmap, GameCfg.TexFilt);
}

int ogl_internal_string(int x, int y, const char *s )
{
	const char * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx,yy;
	int orig_color=grd_curcanv->cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM
	int underline;

	next_row = s;

	yy = y;

	if (grd_curscreen->sc_canvas.cv_bitmap.bm_type != BM_OGL)
		Error("carp.\n");
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
				yy += FONTSCALE_Y(grd_curcanv->cv_font->ft_h)+FSPACY(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

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
				ogl_ubitmapm_cs(xx,yy,FONTSCALE_X(ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),&grd_curcanv->cv_font->ft_bitmaps[letter],-1,F1_0);
			else{
				if (grd_curcanv->cv_bitmap.bm_type==BM_OGL)
					ogl_ubitmapm_cs(xx,yy,ft_w*(FONTSCALE_X(grd_curcanv->cv_font->ft_w)/grd_curcanv->cv_font->ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),&grd_curcanv->cv_font->ft_bitmaps[letter],grd_curcanv->cv_font_fg_color,F1_0);
				else
					Error("ogl_internal_string: non-color string to non-ogl dest\n");
			}

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

int gr_internal_color_string(int x, int y, const char *s ){
	return ogl_internal_string(x,y,s);
}
#endif //OGL

int gr_string(int x, int y, const char *s )
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
		return gr_ustring(x, y, s );

	if ( clipped & 2 )	{
		// Completely clipped...
		return 0;
	}

	// Partially clipped...
#ifdef OGL
	if (TYPE==BM_OGL)
		return ogl_internal_string(x,y,s);
#endif

	if (grd_curcanv->cv_font->ft_flags & FT_COLOR)
		return gr_internal_color_string( x, y, s);

	if ( grd_curcanv->cv_font_bg_color == -1)
		return gr_internal_string_clipped_m( x, y, s );

	return gr_internal_string_clipped( x, y, s );
}

int gr_ustring(int x, int y, const char *s )
{
#ifdef OGL
	if (TYPE==BM_OGL)
		return ogl_internal_string(x,y,s);
#endif
	
	if (grd_curcanv->cv_font->ft_flags & FT_COLOR) {

		return gr_internal_color_string(x,y,s);

	}
	else
		switch( TYPE )
		{
		case BM_LINEAR:
			if ( grd_curcanv->cv_font_bg_color == -1)
				return gr_internal_string0m(x,y,s);
			else
				return gr_internal_string0(x,y,s);
		}
	return 0;
}


void gr_get_string_size(const char *s, int *string_width, int *string_height, int *average_width )
{
	int i = 0;
	float width=0.0,spacing=0.0,longest_width=0.0,string_width_f=0.0,string_height_f=0.0;

	string_height_f = FONTSCALE_Y(grd_curcanv->cv_font->ft_h);
	string_width_f = 0;
	*average_width = grd_curcanv->cv_font->ft_w;

	if (s != NULL )
	{
		string_width_f = 0;
		while (*s)
		{
//			if (*s == '&')
//				s++;
			while (*s == '\n')
			{
				s++;
				string_height_f += FONTSCALE_Y(grd_curcanv->cv_font->ft_h)+FSPACY(1);
				string_width_f = 0;
			}

			if (*s == 0) break;

			get_char_width_f(s[0],s[1],&width,&spacing);

			string_width_f += spacing;

			if (string_width_f > longest_width)
				longest_width = string_width_f;

			i++;
			s++;
		}
	}
	string_width_f = longest_width;
	*string_width = string_width_f;
	*string_height = string_height_f;
}


int gr_uprintf( int x, int y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsnprintf(buffer,sizeof(buffer),format,args);
	return gr_ustring( x, y, buffer );
}

int gr_printf( int x, int y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsnprintf(buffer,sizeof(buffer),format,args);
	return gr_string( x, y, buffer );
}

void gr_close_font( grs_font * font )
{
	if (font)
	{
		int fontnum;
		char * font_data;

		//find font in list
		for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=font;fontnum++);
		Assert(fontnum<MAX_OPEN_FONTS);	//did we find slot?

		font_data = open_font[fontnum].dataptr;
		d_free( font_data );

		open_font[fontnum].ptr = NULL;
		open_font[fontnum].dataptr = NULL;

		if ( font->ft_chars )
			d_free( font->ft_chars );
#ifdef OGL
		if (font->ft_bitmaps)
			d_free( font->ft_bitmaps );
		gr_free_bitmap_data(&font->ft_parent_bitmap);
#endif
		d_free( font );
	}
}

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts()
{
	int fontnum;

	for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
		grs_font *font;

		font = open_font[fontnum].ptr;

		if (font && (font->ft_flags & FT_COLOR))
			gr_remap_font(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

void gr_remap_mono_fonts()
{
	int fontnum;
	con_printf (CON_DEBUG, "gr_remap_mono_fonts ()\n");
	for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
		grs_font *font;
		font = open_font[fontnum].ptr;
		if (font && !(font->ft_flags & FT_COLOR))
			gr_remap_font(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

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
	gf->ft_chars = (ubyte **)(size_t)PHYSFSX_readInt(fp);
	gf->ft_widths = (short *)((size_t)PHYSFSX_readInt(fp) - GRS_FONT_SIZE);
	gf->ft_kerndata = (ubyte *)((size_t)PHYSFSX_readInt(fp) - GRS_FONT_SIZE);
}

grs_font * gr_init_font( const char * fontname )
{
	static int first_time=1;
	grs_font *font;
	char *font_data;
	int i,fontnum;
	unsigned char * ptr;
	int nchars;
	PHYSFS_file *fontfile;
	char file_id[4];
	int datasize;	//size up to (but not including) palette

	if (first_time) {
		int i;
		for (i=0;i<MAX_OPEN_FONTS;i++)
		{
			open_font[i].ptr = NULL;
			open_font[i].dataptr = NULL;
		}
		first_time=0;
	}

	//find free font slot
	for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=NULL;fontnum++);
	Assert(fontnum<MAX_OPEN_FONTS);	//did we find one?

	strncpy(open_font[fontnum].filename,fontname,FILENAME_LEN);

	fontfile = PHYSFSX_openReadBuffered(fontname);

	if (!fontfile) {
		con_printf(CON_VERBOSE, "Can't open font file %s\n", fontname);
		return NULL;
	}

	PHYSFS_read(fontfile, file_id, 4, 1);
	if (memcmp( file_id, "PSFN", 4 )) {
		con_printf(CON_NORMAL, "File %s is not a font file\n", fontname);
		return NULL;
	}

	datasize = PHYSFSX_readInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	MALLOC(font, grs_font, 1);
	grs_font_read(font, fontfile);

	MALLOC(font_data, char, datasize);
	PHYSFS_read(fontfile, font_data, 1, datasize);

	open_font[fontnum].ptr = font;
	open_font[fontnum].dataptr = font_data;

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = (unsigned char **)d_malloc( nchars * sizeof(unsigned char *));

		ptr = font->ft_data;

		for (i=0; i< nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = (unsigned char *) font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		PHYSFS_read(fontfile,palette,3,256);		//read the palette

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );
	}

	PHYSFS_close(fontfile);

	//set curcanv vars

	grd_curcanv->cv_font        = font;
	grd_curcanv->cv_font_fg_color    = 0;
	grd_curcanv->cv_font_bg_color    = 0;

#ifdef OGL
	ogl_init_font(font);
#endif

	return font;
}

//remap a font by re-reading its data & palette
void gr_remap_font( grs_font *font, char * fontname, char *font_data )
{
	int i;
	int nchars;
	PHYSFS_file *fontfile;
	char file_id[4];
	int datasize;        //size up to (but not including) palette
	unsigned char *ptr;

	if (! (font->ft_flags & FT_COLOR))
		return;

	fontfile = PHYSFSX_openReadBuffered(fontname);

	if (!fontfile)
		Error( "Can't open font file %s", fontname );

	PHYSFS_read(fontfile, file_id, 4, 1);
	if ( !strncmp( file_id, "NFSP", 4 ) )
		Error( "File %s is not a font file", fontname );

	datasize = PHYSFSX_readInt(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	d_free(font->ft_chars);
	grs_font_read(font, fontfile); // have to reread in case mission hogfile overrides font.

	PHYSFS_read(fontfile, font_data, 1, datasize);  //read raw data

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = (unsigned char **)d_malloc( nchars * sizeof(unsigned char *));

		ptr = font->ft_data;

		for (i=0; i< nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = (unsigned char *) font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		PHYSFS_read(fontfile,palette,3,256);		//read the palette

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );

	}

	PHYSFS_close(fontfile);

#ifdef OGL
	if (font->ft_bitmaps)
		d_free( font->ft_bitmaps );
	gr_free_bitmap_data(&font->ft_parent_bitmap);

	ogl_init_font(font);
#endif
}

void gr_set_curfont( grs_font * n)
{
	grd_curcanv->cv_font = n;
}

void gr_set_fontcolor( int fg_color, int bg_color )
{
	grd_curcanv->cv_font_fg_color    = fg_color;
	grd_curcanv->cv_font_bg_color    = bg_color;
}

int gr_internal_string_clipped(int x, int y, const char *s )
{
	unsigned char * fp;
	const char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

        bits = 0;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(text_ptr1);

		last_x = x;

		for (r=0; r<grd_curcanv->cv_font->ft_h; r++)	{
			text_ptr = text_ptr1;
			x = last_x;

			while (*text_ptr)	{
				if (*text_ptr == '\n' )	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					grd_curcanv->cv_font_fg_color = (unsigned char)*(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )
				{
					if ((r==grd_curcanv->cv_font->ft_baseline+2) || (r==grd_curcanv->cv_font->ft_baseline+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = (unsigned char)*text_ptr-grd_curcanv->cv_font->ft_minchar;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
					fp = grd_curcanv->cv_font->ft_chars[letter];
				else
					fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

				if (underline)	{
					for (i=0; i< width; i++ )	{
						gr_setcolor(grd_curcanv->cv_font_fg_color);
						gr_pixel( x++, y );
					}
				} else {
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++ )	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)
							gr_setcolor(grd_curcanv->cv_font_fg_color);
						else
							gr_setcolor(grd_curcanv->cv_font_bg_color);
						gr_pixel( x++, y );
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				text_ptr++;
			}
			y++;
		}
	}
	return 0;
}

int gr_internal_string_clipped_m(int x, int y, const char *s )
{
	unsigned char * fp;
	const char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

        bits = 0;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(text_ptr1);

		last_x = x;

		for (r=0; r<grd_curcanv->cv_font->ft_h; r++)	{
			x = last_x;

			text_ptr = text_ptr1;

			while (*text_ptr)	{
				if (*text_ptr == '\n' )	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					grd_curcanv->cv_font_fg_color = (unsigned char)*(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )
				{
					if ((r==grd_curcanv->cv_font->ft_baseline+2) || (r==grd_curcanv->cv_font->ft_baseline+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = (unsigned char)*text_ptr-grd_curcanv->cv_font->ft_minchar;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
					fp = grd_curcanv->cv_font->ft_chars[letter];
				else
					fp = grd_curcanv->cv_font->ft_data + letter * BITS_TO_BYTES(width)*grd_curcanv->cv_font->ft_h;

				if (underline)	{
					for (i=0; i< width; i++ )	{
						gr_setcolor(grd_curcanv->cv_font_fg_color);
						gr_pixel( x++, y );
					}
				} else {
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					for (i=0; i< width; i++ )	{
						if (BitMask==0) {
							bits = *fp++;
							BitMask = 0x80;
						}
						if (bits & BitMask)	{
							gr_setcolor(grd_curcanv->cv_font_fg_color);
							gr_pixel( x++, y );
						} else {
							x++;
						}
						BitMask >>= 1;
					}
				}

				x += spacing-width;		//for kerning

				text_ptr++;
			}
			y++;
		}
	}
	return 0;
}
