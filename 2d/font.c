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

/* $Source: /cvs/cvsroot/d2x/2d/font.c,v $
 * $Revision: 1.10 $
 * $Author: bradleyb $
 * $Date: 2001-11-14 09:34:32 $
 *
 * Graphical routines for drawing fonts.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.9  2001/11/08 10:37:25  bradleyb
 * free OGL font data when rereading
 *
 * Revision 1.8  2001/11/04 03:58:25  bradleyb
 * re-init ogl fonts after remapping colors.
 *
 * Revision 1.7  2001/11/02 10:46:23  bradleyb
 * fixed gr_remap_font, minor stuff
 *
 * Revision 1.6  2001/11/02 02:03:35  bradleyb
 * Enable OpenGL fonts\!
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "error.h"

#include "cfile.h"
#include "mono.h"
#include "byteswap.h"
#include "bitmap.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#include "makesig.h"

#define MAX_OPEN_FONTS	50
#define FILENAME_LEN		13

typedef struct openfont {
	char filename[FILENAME_LEN];
	grs_font *ptr;
} openfont;

//list of open fonts, for use (for now) for palette remapping
openfont open_font[MAX_OPEN_FONTS];

#define FONT        grd_curcanv->cv_font
#define FG_COLOR    grd_curcanv->cv_font_fg_color
#define BG_COLOR    grd_curcanv->cv_font_bg_color
#define FWIDTH       FONT->ft_w
#define FHEIGHT      FONT->ft_h
#define FBASELINE    FONT->ft_baseline
#define FFLAGS       FONT->ft_flags
#define FMINCHAR     FONT->ft_minchar
#define FMAXCHAR     FONT->ft_maxchar
#define FDATA        FONT->ft_data
#define FCHARS       FONT->ft_chars
#define FWIDTHS      FONT->ft_widths

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

int gr_internal_string_clipped(int x, int y, char *s );
int gr_internal_string_clipped_m(int x, int y, char *s );

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
#define INFONT(_c) ((_c >= 0) && (_c <= FMAXCHAR-FMINCHAR))

//takes the character BEFORE being offset into current font
void get_char_width(ubyte c,ubyte c2,int *width,int *spacing)
{
	int letter;

	letter = c-FMINCHAR;

	if (!INFONT(letter)) {				//not in font, draw as space
		*width=0;
		if (FFLAGS & FT_PROPORTIONAL)
			*spacing = FWIDTH/2;
		else
			*spacing = FWIDTH;
		return;
	}

	if (FFLAGS & FT_PROPORTIONAL)
		*width = FWIDTHS[letter];
	else
		*width = FWIDTH;

	*spacing = *width;

	if (FFLAGS & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2;

			letter2 = c2-FMINCHAR;

			if (INFONT(letter2)) {

				p = find_kern_entry(FONT,(ubyte)letter,letter2);

				if (p)
					*spacing = p[2];
			}
		}
	}
}

int get_centered_x(char *s)
{
	int w,w2,s2;

	for (w=0;*s!=0 && *s!='\n';s++) {
		if (*s<=0x06) {
			if (*s<=0x03)
				s++;
			continue;//skip color codes.
		}
		get_char_width(s[0],s[1],&w2,&s2);
		w += s2;
	}

	return ((grd_curcanv->cv_bitmap.bm_w - w) / 2);
}

//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color type thing would be better, but that would be way too much trouble for little gain
int gr_message_color_level=1;
#define CHECK_EMBEDDED_COLORS() if ((*text_ptr >= 0x01) && (*text_ptr <= 0x03)) { \
		text_ptr++; \
		if (*text_ptr){ \
			if (gr_message_color_level >= *(text_ptr-1)) \
				FG_COLOR = *text_ptr - 1; \
			text_ptr++; \
		} \
	} \
	else if ((*text_ptr >= 0x04) && (*text_ptr <= 0x06)){ \
		if (gr_message_color_level >= *text_ptr - 3) \
			FG_COLOR=orig_color; \
		text_ptr++; \
	}

int gr_internal_string0(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
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

		for (r=0; r<FHEIGHT; r++)
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
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++ )
						DATA[VideoOffset++] = (unsigned char) FG_COLOR;
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
							DATA[VideoOffset++] = (unsigned char) FG_COLOR;
						else
							DATA[VideoOffset++] = (unsigned char) BG_COLOR;
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

int gr_internal_string0m(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	int orig_color=FG_COLOR;//to allow easy reseting to default string color with colored strings -MPM

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

		for (r=0; r<FHEIGHT; r++)
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
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter) || *text_ptr<=0x06) {	//not in font, draw as space
					CHECK_EMBEDDED_COLORS() else{
						VideoOffset += spacing;
						text_ptr++;
					}
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++ )
						DATA[VideoOffset++] = (unsigned int) FG_COLOR;
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
							DATA[VideoOffset++] = (unsigned int) FG_COLOR;
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

#ifdef __MSDOS__
int gr_internal_string2(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int page_switched, skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	VideoOffset1 = (size_t)DATA + y * ROWSIZE + x;

	bits = 0;

	gr_vesa_setpage(VideoOffset1 >> 16);

	VideoOffset1 &= 0xFFFF;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
			gr_vesa_setpage(VideoOffset1 >> 16);
			VideoOffset1 &= 0xFFFF;
		}

		for (r=0; r<FHEIGHT; r++)
		{
			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			page_switched = 0;

			while (*text_ptr)
			{
				if (*text_ptr == '\n' )
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				Assert(width==spacing);		//no kerning support here

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
				{
					if ( VideoOffset+width > 0xFFFF )
					{
						for (i=0; i< width; i++ )
						{
							gr_video_memory[VideoOffset++] = FG_COLOR;

							if (VideoOffset > 0xFFFF )
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}
						}
					}
					else
					{
						for (i=0; i< width; i++ )
							gr_video_memory[VideoOffset++] = FG_COLOR;
					}
				}
				else
				{
					// fp -- dword
					// VideoOffset
					// width

					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					if ( VideoOffset+width > 0xFFFF )
					{
						for (i=0; i< width; i++ )
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								gr_video_memory[VideoOffset++] = BG_COLOR;

							BitMask >>= 1;

							if (VideoOffset > 0xFFFF )
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}

						}
					} else {

						if (width == 8 )
						{
							bits = *fp++;

							if (bits & 0x80) gr_video_memory[VideoOffset+0] = FG_COLOR;
							else gr_video_memory[VideoOffset+0] = BG_COLOR;

							if (bits & 0x40) gr_video_memory[VideoOffset+1] = FG_COLOR;
							else gr_video_memory[VideoOffset+1] = BG_COLOR;

							if (bits & 0x20) gr_video_memory[VideoOffset+2] = FG_COLOR;
							else gr_video_memory[VideoOffset+2] = BG_COLOR;

							if (bits & 0x10) gr_video_memory[VideoOffset+3] = FG_COLOR;
							else gr_video_memory[VideoOffset+3] = BG_COLOR;

							if (bits & 0x08) gr_video_memory[VideoOffset+4] = FG_COLOR;
							else gr_video_memory[VideoOffset+4] = BG_COLOR;

							if (bits & 0x04) gr_video_memory[VideoOffset+5] = FG_COLOR;
							else gr_video_memory[VideoOffset+5] = BG_COLOR;

							if (bits & 0x02) gr_video_memory[VideoOffset+6] = FG_COLOR;
							else gr_video_memory[VideoOffset+6] = BG_COLOR;

							if (bits & 0x01) gr_video_memory[VideoOffset+7] = FG_COLOR;
							else gr_video_memory[VideoOffset+7] = BG_COLOR;

							VideoOffset += 8;
						} else {
							for (i=0; i< width/2 ; i++ )
							{
								if (BitMask==0) {
									bits = *fp++;
									BitMask = 0x80;
								}

								if (bits & BitMask)
									gr_video_memory[VideoOffset++] = FG_COLOR;
								else
									gr_video_memory[VideoOffset++] = BG_COLOR;
								BitMask >>= 1;


								// Unroll twice

								if (BitMask==0) {
									bits = *fp++;
									BitMask = 0x80;
								}

								if (bits & BitMask)
									gr_video_memory[VideoOffset++] = FG_COLOR;
								else
									gr_video_memory[VideoOffset++] = BG_COLOR;
								BitMask >>= 1;
							}
						}
					}
				}
				text_ptr++;
			}

			y ++;
			VideoOffset1 += ROWSIZE;

			if (VideoOffset1 > 0xFFFF ) {
				VideoOffset1 -= 0xFFFF + 1;
				if (!page_switched)
					gr_vesa_incpage();
			}
		}

		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

int gr_internal_string2m(int x, int y, char *s )
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int page_switched, skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

	VideoOffset1 = (size_t)DATA + y * ROWSIZE + x;

	gr_vesa_setpage(VideoOffset1 >> 16);

	VideoOffset1 &= 0xFFFF;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
			VideoOffset1 = y * ROWSIZE + xx;
			gr_vesa_setpage(VideoOffset1 >> 16);
			VideoOffset1 &= 0xFFFF;
		}

		for (r=0; r<FHEIGHT; r++)
		{
			text_ptr = text_ptr1;

			VideoOffset = VideoOffset1;

			page_switched = 0;

			while (*text_ptr)
			{
				if (*text_ptr == '\n' )
				{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					VideoOffset += width;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
				{
					if ( VideoOffset+width > 0xFFFF )
					{
						for (i=0; i< width; i++ )
						{
							gr_video_memory[VideoOffset++] = FG_COLOR;

							if (VideoOffset > 0xFFFF )
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}
						}
					}
					else
					{
						for (i=0; i< width; i++ )
							gr_video_memory[VideoOffset++] = FG_COLOR;
					}
				}
				else
				{
					fp += BITS_TO_BYTES(width)*r;

					BitMask = 0;

					if ( VideoOffset+width > 0xFFFF )
					{
						for (i=0; i< width; i++ )
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								VideoOffset++;

							BitMask >>= 1;

							if (VideoOffset > 0xFFFF )
							{
								VideoOffset -= 0xFFFF + 1;
								page_switched = 1;
								gr_vesa_incpage();
							}

						}
					} else {
						for (i=0; i< width; i++ )
						{
							if (BitMask==0) {
								bits = *fp++;
								BitMask = 0x80;
							}

							if (bits & BitMask)
								gr_video_memory[VideoOffset++] = FG_COLOR;
							else
								VideoOffset++;;
							BitMask >>= 1;
						}
					}
				}
				text_ptr++;

				VideoOffset += spacing-width;
			}

			y ++;
			VideoOffset1 += ROWSIZE;

			if (VideoOffset1 > 0xFFFF ) {
				VideoOffset1 -= 0xFFFF + 1;
				if (!page_switched)
					gr_vesa_incpage();
			}
		}

		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}

#endif // __MSDOS__

#if defined(POLY_ACC)
int gr_internal_string5(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

    pa_flush();
    VideoOffset1 = y * ROWSIZE + x * PA_BPP;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
            VideoOffset1 = y * ROWSIZE + xx * PA_BPP;
		}

		for (r=0; r<FHEIGHT; r++)
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
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
                    VideoOffset += spacing * PA_BPP;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++ )
                    {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
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
                        {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
                        else
                        {    *(short *)(DATA + VideoOffset) = pa_clut[BG_COLOR]; VideoOffset += PA_BPP; }
                        BitMask >>= 1;
					}
				}

                VideoOffset += PA_BPP * (spacing-width);       //for kerning

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

int gr_internal_string5m(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int	skip_lines = 0;

	unsigned int VideoOffset, VideoOffset1;

    pa_flush();
    VideoOffset1 = y * ROWSIZE + x * PA_BPP;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		if (x==0x8000) {			//centered
			int xx = get_centered_x(text_ptr1);
            VideoOffset1 = y * ROWSIZE + xx * PA_BPP;
		}

		for (r=0; r<FHEIGHT; r++)
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
					FG_COLOR = *(text_ptr+1);
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
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
                    VideoOffset += spacing * PA_BPP;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)
					for (i=0; i< width; i++ )
                    {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
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
                        {    *(short *)(DATA + VideoOffset) = pa_clut[FG_COLOR]; VideoOffset += PA_BPP; }
                        else
                            VideoOffset += PA_BPP;
						BitMask >>= 1;
					}
				}
				text_ptr++;

                VideoOffset += PA_BPP * (spacing-width);
			}

			VideoOffset1 += ROWSIZE; y++;
		}
		y += skip_lines;
		VideoOffset1 += ROWSIZE * skip_lines;
		skip_lines = 0;
	}
	return 0;
}
#endif

#ifndef OGL
//a bitmap for the character
grs_bitmap char_bm = {
				0,0,0,0,						//x,y,w,h
				BM_LINEAR,					//type
				BM_FLAG_TRANSPARENT,		//flags
				0,								//rowsize
				NULL,							//data
#ifdef BITMAP_SELECTOR
				0,				//selector
#endif
				0,     //avg_color
				0      //unused
};

int gr_internal_color_string(int x, int y, char *s )
{
	unsigned char * fp;
	ubyte * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx,yy;

	char_bm.bm_h = FHEIGHT;		//set height for chars of this font

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
				yy += FHEIGHT;
				break;
			}

			letter = *text_ptr-FMINCHAR;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			if (!INFONT(letter)) {	//not in font, draw as space
				xx += spacing;
				text_ptr++;
				continue;
			}

			if (FFLAGS & FT_PROPORTIONAL)
				fp = FCHARS[letter];
			else
				fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

			gr_init_bitmap (&char_bm, BM_LINEAR, 0, 0, width, FHEIGHT, width, fp);
			gr_bitmapm(xx,yy,&char_bm);

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

#else //OGL
#include "../main/inferno.h"
#include "ogl_init.h"
#include "args.h"
//font handling routines for OpenGL - Added 9/25/99 Matthew Mueller - they are here instead of in arch/ogl because they use all these defines

int pow2ize(int x);//from ogl.c

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
				mprintf((0,"failed to fit (%ix%i, %ic)\n",w,h,nc));
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
		mprintf((0,"fit: %ix%i  %i tries\n",w,h,tries));

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
	mprintf((0,"using %ix%i\n",*rw,*rh));
	
}

void ogl_init_font(grs_font * font){
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int i,w,h,tw,th,x,y,curx=0,cury=0;
	char *fp;
	//	char data[32*32*4];
	char *data;
	int gap=0;//having a gap just wastes ram, since we don't filter text textures at all.
	//	char s[2];
	ogl_font_choose_size(font,gap,&tw,&th);
	data=d_malloc(tw*th);
	gr_init_bitmap(&font->ft_parent_bitmap,BM_LINEAR,0,0,tw,th,tw,data);

	font->ft_parent_bitmap.gltexture=ogl_get_free_texture();

	font->ft_bitmaps=(grs_bitmap*)d_malloc( nchars * sizeof(grs_bitmap));
	mprintf((0,"ogl_init_font %s, %s, nchars=%i, (%ix%i tex)\n",(font->ft_flags & FT_PROPORTIONAL)?"proportional":"fixedwidth",(font->ft_flags & FT_COLOR)?"color":"mono",nchars,tw,th));
	//	s[1]=0;
	h=font->ft_h;
	//	sleep(5);

	for(i=0;i<nchars;i++){
		//		s[0]=font->ft_minchar+i;
		//		gr_get_string_size(s,&w,&h,&aw);
		if (font->ft_flags & FT_PROPORTIONAL)
			w=font->ft_widths[i];
		else
			w=font->ft_w;
//		mprintf((0,"char %i(%ix%i): ",i,w,h));
		if (w<1 || w>256){
			mprintf((0,"grr\n"));continue;
		}
		if (curx+w+gap>tw){
			cury+=h+gap;
			curx=0;
		}
		if (cury+h>th)
			Error("font doesn't really fit (%i/%i)?\n",i,nchars);
		if (font->ft_flags & FT_COLOR) {
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * w*h;
			for (y=0;y<h;y++)
				for (x=0;x<w;x++){
					font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=fp[x+y*w];
				}

			//			gr_init_bitmap(&font->ft_bitmaps[i],BM_LINEAR,0,0,w,h,w,font->);
		}else{
			int BitMask,bits=0,white=gr_find_closest_color(63,63,63);
			//			if (w*h>sizeof(data))
			//				Error("ogl_init_font: toobig\n");
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
	if (!(font->ft_flags & FT_COLOR)) {
		//use GL_INTENSITY instead of GL_RGB
		if (ogl_intensity4_ok){
			font->ft_parent_bitmap.gltexture->internalformat=GL_INTENSITY4;
			font->ft_parent_bitmap.gltexture->format=GL_LUMINANCE;
		}else if (ogl_luminance4_alpha4_ok){
			font->ft_parent_bitmap.gltexture->internalformat=GL_LUMINANCE4_ALPHA4;
			font->ft_parent_bitmap.gltexture->format=GL_LUMINANCE_ALPHA;
		}else if (ogl_rgba2_ok){
			font->ft_parent_bitmap.gltexture->internalformat=GL_RGBA2;
			font->ft_parent_bitmap.gltexture->format=GL_RGBA;
		}else{
			font->ft_parent_bitmap.gltexture->internalformat=ogl_rgba_format;
			font->ft_parent_bitmap.gltexture->format=GL_RGBA;
		}
	}
	ogl_loadbmtexture_m(&font->ft_parent_bitmap,0);
}

int ogl_internal_string(int x, int y, char *s )
{
	ubyte * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx,yy;
	int orig_color=FG_COLOR;//to allow easy reseting to default string color with colored strings -MPM

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
			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += FHEIGHT;
				break;
			}

			letter = *text_ptr-FMINCHAR;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			if (!INFONT(letter) || *text_ptr<=0x06) {	//not in font, draw as space
				CHECK_EMBEDDED_COLORS() else{
					xx += spacing;
					text_ptr++;
				}
				continue;
			}
			
//			ogl_ubitblt(FONT->ft_bitmaps[letter].bm_w,FONT->ft_bitmaps[letter].bm_h,xx,yy,0,0,&FONT->ft_bitmaps[letter],NULL);
//			if (*text_ptr>='0' && *text_ptr<='9'){
			if (FFLAGS&FT_COLOR)
				gr_ubitmapm(xx,yy,&FONT->ft_bitmaps[letter]);
			else{
				if (grd_curcanv->cv_bitmap.bm_type==BM_OGL)
					ogl_ubitmapm_c(xx,yy,&FONT->ft_bitmaps[letter],FG_COLOR);
				else
					Error("ogl_internal_string: non-color string to non-ogl dest\n");
//					gr_ubitmapm(xx,yy,&FONT->ft_bitmaps[letter]);//ignores color..
			}
			//}

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

int gr_internal_color_string(int x, int y, char *s ){
	return ogl_internal_string(x,y,s);
}
#endif //OGL

int gr_string(int x, int y, char *s )
{
	int w, h, aw;
	int clipped=0;

	Assert(FONT != NULL);

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
		mprintf( (1, "Text '%s' at (%d,%d) is off screen!\n", s, x, y ));
		return 0;
	}

	if ( clipped & 1 )	{
		// Partially clipped...
		//mprintf( (0, "Text '%s' at (%d,%d) is getting clipped!\n", s, x, y ));
	}

	// Partially clipped...
#ifdef OGL
	if (TYPE==BM_OGL)
		return ogl_internal_string(x,y,s);
#endif

	if (FFLAGS & FT_COLOR)
		return gr_internal_color_string( x, y, s);

	if ( BG_COLOR == -1)
		return gr_internal_string_clipped_m( x, y, s );

	return gr_internal_string_clipped( x, y, s );
}

int gr_ustring(int x, int y, char *s )
{
#ifdef OGL
	if (TYPE==BM_OGL)
		return ogl_internal_string(x,y,s);
#endif
	
	if (FFLAGS & FT_COLOR) {

		return gr_internal_color_string(x,y,s);

	}
	else
		switch( TYPE )
		{
		case BM_LINEAR:
			if ( BG_COLOR == -1)
				return gr_internal_string0m(x,y,s);
			else
				return gr_internal_string0(x,y,s);
#ifdef __MSDOS__
		case BM_SVGA:
			if ( BG_COLOR == -1)
				return gr_internal_string2m(x,y,s);
			else
				return gr_internal_string2(x,y,s);
#endif // __MSDOS__
#if defined(POLY_ACC)
        case BM_LINEAR15:
			if ( BG_COLOR == -1)
				return gr_internal_string5m(x,y,s);
			else
				return gr_internal_string5(x,y,s);
#endif
		}
	return 0;
}


void gr_get_string_size(char *s, int *string_width, int *string_height, int *average_width )
{
	int i = 0, longest_width = 0;
	int width,spacing;

	*string_height = FHEIGHT;
	*string_width = 0;
	*average_width = FWIDTH;

	if (s != NULL )
	{
		*string_width = 0;
		while (*s)
		{
//			if (*s == CC_UNDERLINE)
//				s++;
			while (*s == '\n')
			{
				s++;
				*string_height += FHEIGHT;
				*string_width = 0;
			}

			if (*s == 0) break;

			//	1 = next byte specifies color, so skip the 1 and the color value
			if (*s == CC_COLOR)
				s += 2;
			else if (*s == CC_LSPACING) {
				*string_height += *(s+1)-'0';
				s += 2;
			} else {
				get_char_width(s[0],s[1],&width,&spacing);

				*string_width += spacing;

				if (*string_width > longest_width)
					longest_width = *string_width;

				i++;
				s++;
			}
		}
	}
	*string_width = longest_width;
}


int gr_uprintf( int x, int y, char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);
	return gr_ustring( x, y, buffer );
}

int gr_printf( int x, int y, char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);
	return gr_string( x, y, buffer );
}

void gr_close_font( grs_font * font )
{
	if (font)
	{
		int fontnum;

		//find font in list
		for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=font;fontnum++);
		Assert(fontnum<MAX_OPEN_FONTS);	//did we find slot?

		open_font[fontnum].ptr = NULL;

		if ( font->ft_chars )
			d_free( font->ft_chars );
		d_free( font->oldfont );
#ifdef OGL
		if (font->ft_bitmaps)
			d_free( font->ft_bitmaps );
		gr_free_bitmap_data(&font->ft_parent_bitmap);
//		ogl_freebmtexture(&font->ft_parent_bitmap);
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
			gr_remap_font(font,open_font[fontnum].filename);
	}
}

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );
void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count );
#ifdef __WATCOMC__
#pragma aux decode_data_asm parm [esi] [ecx] [edi] [ebx] modify exact [esi edi eax ebx ecx] = \
"again_ddn:"							\
	"xor	eax,eax"				\
	"mov	al,[esi]"			\
	"inc	dword ptr [ebx+eax*4]"		\
	"mov	al,[edi+eax]"		\
	"mov	[esi],al"			\
	"inc	esi"					\
	"dec	ecx"					\
	"jne	again_ddn"

#endif

grs_font * gr_init_font( char * fontname )
{
	static int first_time=1;
	old_grs_font *font;
	grs_font *newfont;
	int i,fontnum;
	unsigned char * ptr;
	int nchars;
	CFILE *fontfile;
	u_int32_t file_id;
	int32_t datasize;	//size up to (but not including) palette

	if (first_time) {
		int i;
		for (i=0;i<MAX_OPEN_FONTS;i++)
			open_font[i].ptr = NULL;
		first_time=0;
	}

	//find free font slot
	for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=NULL;fontnum++);
	Assert(fontnum<MAX_OPEN_FONTS);	//did we find one?

	strncpy(open_font[fontnum].filename,fontname,FILENAME_LEN);

	fontfile = cfopen(fontname, "rb");

	if (!fontfile)
		Error( "Can't open font file %s", fontname );

	cfread(&file_id,sizeof(file_id),1,fontfile);
	file_id=swapint(file_id);
	cfread(&datasize,sizeof(datasize),1,fontfile);
	datasize=swapint(datasize);

	if (file_id != MAKE_SIG('N','F','S','P'))
		Error( "File %s is not a font file", fontname );

	font = (old_grs_font *) d_malloc(datasize);
	newfont = (grs_font *) d_malloc(sizeof(grs_font));
	newfont->oldfont=font;

	open_font[fontnum].ptr = newfont;

	cfread(font,1,datasize,fontfile);

	newfont->ft_flags=swapint(font->ft_flags);
	newfont->ft_w=swapshort(font->ft_w);
	newfont->ft_h=swapshort(font->ft_h);
	newfont->ft_baseline=swapshort(font->ft_baseline);
	newfont->ft_maxchar=font->ft_maxchar;
	newfont->ft_minchar=font->ft_minchar;
	newfont->ft_bytewidth=swapshort(font->ft_bytewidth);

	nchars = newfont->ft_maxchar-newfont->ft_minchar+1;

	if (newfont->ft_flags & FT_PROPORTIONAL) {

		newfont->ft_widths = (short *) (swapint(font->ft_widths) + ((ubyte *) font));
		newfont->ft_data = (swapint(font->ft_data)) + ((ubyte *) font);
		newfont->ft_chars = (unsigned char **)d_malloc( nchars * sizeof(unsigned char *));

		ptr = newfont->ft_data;

		for (i=0; i< nchars; i++ ) {
			newfont->ft_chars[i] = ptr;
			if (newfont->ft_flags & FT_COLOR)
				ptr += newfont->ft_widths[i] * newfont->ft_h;
			else
				ptr += BITS_TO_BYTES(newfont->ft_widths[i]) * newfont->ft_h;
		}

	} else  {

		newfont->ft_data   = ((unsigned char *) font) + sizeof(*font);
		newfont->ft_chars  = NULL;
		newfont->ft_widths = NULL;

		ptr = newfont->ft_data + (nchars * newfont->ft_w * newfont->ft_h);
	}

	if (newfont->ft_flags & FT_KERNED)
		newfont->ft_kerndata = swapint(font->ft_kerndata) + ((ubyte *) font);

	if (newfont->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		cfread(palette,3,256,fontfile);		//read the palette

#ifdef MACINTOSH			// swap the first and last palette entries (black and white)
		{
			int i;
			ubyte c;

			for (i = 0; i < 3; i++) {
				c = palette[i];
				palette[i] = palette[765+i];
				palette[765+i] = c;
			}

//  we also need to swap the data entries as well.  black is white and white is black

			for (i = 0; i < ptr-newfont->ft_data; i++) {
				if (newfont->ft_data[i] == 0)
					newfont->ft_data[i] = 255;
				else if (newfont->ft_data[i] == 255)
					newfont->ft_data[i] = 0;
			}

		}
#endif

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;		// chaged from colormap[255] = 255 to this for macintosh

		decode_data_asm(newfont->ft_data, ptr-newfont->ft_data, colormap, freq );

	}

	cfclose(fontfile);

//	memcpy(newfont,font,(ubyte*)&newfont->oldfont-(ubyte*)newfont);//fill in newfont data from oldfont struct
//	mprintf((0,"%i %i %i\n",sizeof(grs_font),sizeof(old_grs_font),(ubyte*)&newfont->oldfont-(ubyte*)newfont));

	//set curcanv vars

	FONT        = newfont;
	FG_COLOR    = 0;
	BG_COLOR    = 0;

	{
		int x,y,aw;
		char tests[]="abcdefghij1234.A";
		gr_get_string_size(tests,&x,&y,&aw);
//		newfont->ft_aw=x/(float)strlen(tests);
	}

#ifdef OGL
	ogl_init_font(newfont);
#endif

	return newfont;

}

//remap a font by re-reading its data & palette
void gr_remap_font( grs_font *font, char * fontname )
{
	int i;
	int nchars;
	CFILE *fontfile;
	u_int32_t file_id;
	int32_t datasize;        //size up to (but not including) palette
	int data_len;

	if (! (font->ft_flags & FT_COLOR))
		return;

	fontfile = cfopen(fontname, "rb");

	if (!fontfile)
		Error( "Can't open font file %s", fontname );

	file_id = cfile_read_int(fontfile);
	datasize = cfile_read_int(fontfile);

	if (file_id != MAKE_SIG('N','F','S','P'))
		Error( "File %s is not a font file", fontname );

	nchars = font->ft_maxchar-font->ft_minchar+1;

	//compute data length
	data_len = 0;
	if (font->ft_flags & FT_PROPORTIONAL) {

		for (i=0; i< nchars; i++ ) {
			if (font->ft_flags & FT_COLOR)
				data_len += font->ft_widths[i] * font->ft_h;
			else
				data_len += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else
		data_len = nchars * font->ft_w * font->ft_h;

	cfread(font->oldfont, 1, datasize, fontfile);   //read raw data

	if (font->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		cfseek(fontfile,-sizeof(palette),SEEK_END);

		cfread(palette,3,256,fontfile);		//read the palette

#ifdef MACINTOSH			// swap the first and last palette entries (black and white)
		{
			int i;
			ubyte c;

			for (i = 0; i < 3; i++) {
				c = palette[i];
				palette[i] = palette[765+i];
				palette[765+i] = c;
			}

//  we also need to swap the data entries as well.  black is white and white is black

			for (i = 0; i < data_len; i++) {
				if (font->ft_data[i] == 0)
					font->ft_data[i] = 255;
				else if (font->ft_data[i] == 255)
					font->ft_data[i] = 0;
			}

		}
#endif

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;	// changed from		colormap[255] = 255;

		decode_data_asm(font->ft_data, data_len, colormap, freq );

	}

	cfclose(fontfile);

#ifdef OGL
	if (font->ft_bitmaps)
		d_free( font->ft_bitmaps );
	gr_free_bitmap_data(&font->ft_parent_bitmap);
//	ogl_freebmtexture(&font->ft_parent_bitmap);

	ogl_init_font(font);
#endif
}


void gr_set_fontcolor( int fg, int bg )
{
	FG_COLOR    = fg;
	BG_COLOR    = bg;
}

void gr_set_curfont( grs_font * new )
{
	FONT = new;
}


int gr_internal_string_clipped(int x, int y, char *s )
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

        bits=0;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
			text_ptr = text_ptr1;
			x = last_x;

			while (*text_ptr)	{
				if (*text_ptr == '\n' )	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++ )	{
						gr_setcolor(FG_COLOR);
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
							gr_setcolor(FG_COLOR);
						else
							gr_setcolor(BG_COLOR);
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

int gr_internal_string_clipped_m(int x, int y, char *s )
{
	unsigned char * fp;
	char * text_ptr, * next_row, * text_ptr1;
	int r, BitMask, i, bits, width, spacing, letter, underline;
	int x1 = x, last_x;

        bits=0;

	next_row = s;

	while (next_row != NULL )
	{
		text_ptr1 = next_row;
		next_row = NULL;

		x = x1;
		if (x==0x8000)			//centered
			x = get_centered_x(text_ptr1);

		last_x = x;

		for (r=0; r<FHEIGHT; r++)	{
			x = last_x;

			text_ptr = text_ptr1;

			while (*text_ptr)	{
				if (*text_ptr == '\n' )	{
					next_row = &text_ptr[1];
					break;
				}

				if (*text_ptr == CC_COLOR) {
					FG_COLOR = *(text_ptr+1);
					text_ptr += 2;
					continue;
				}

				if (*text_ptr == CC_LSPACING) {
					Int3();	//	Warning: skip lines not supported for clipped strings.
					text_ptr += 2;
					continue;
				}

				underline = 0;
				if (*text_ptr == CC_UNDERLINE )	{
					if ((r==FBASELINE+2) || (r==FBASELINE+3))
						underline = 1;
					text_ptr++;
				}

				get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

				letter = *text_ptr-FMINCHAR;

				if (!INFONT(letter)) {	//not in font, draw as space
					x += spacing;
					text_ptr++;
					continue;
				}

				if (FFLAGS & FT_PROPORTIONAL)
					fp = FCHARS[letter];
				else
					fp = FDATA + letter * BITS_TO_BYTES(width)*FHEIGHT;

				if (underline)	{
					for (i=0; i< width; i++ )	{
						gr_setcolor(FG_COLOR);
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
							gr_setcolor(FG_COLOR);
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
