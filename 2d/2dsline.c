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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <conf.h>
#include <string.h>

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"

#ifdef __DJGPP__
#include "modex.h"
#include "vesa.h"
#endif

int Gr_scanline_darkening_level = GR_FADE_LEVELS;

#ifndef NO_ASM
# ifdef __WATCOMC__
void gr_linear_darken( ubyte * dest, int darkening_level, int count, ubyte * fade_table );
#  pragma aux gr_linear_darken parm [edi] [eax] [ecx] [edx] modify exact [eax ebx ecx edx edi] = \
"					xor	ebx, ebx					"	\
"					mov	bh, al					"  \
"gld_loop:		mov	bl, [edi]				"	\
"					mov	al, [ebx+edx]			"	\
"					mov	[edi], al				"	\
"					inc	edi						"	\
"					dec	ecx						"	\
"                   jnz gld_loop                    "

# elif defined __GNUC__
static inline void gr_linear_darken( ubyte * dest, int darkening_level, int count, ubyte * fade_table ) {
   int dummy[4];
   __asm__ __volatile__ (
"               xorl %%ebx, %%ebx;"
"               movb %%al, %%bh;"
"0:             movb (%%edi), %%bl;"
"               movb (%%ebx, %%edx), %%al;"
"               movb %%al, (%%edi);"
"               incl %%edi;"
"               decl %%ecx;"
"               jnz 0b"
   : "=D" (dummy[0]), "=a" (dummy[1]), "=c" (dummy[2]), "=d" (dummy[3])
   : "0" (dest), "1" (darkening_level), "2" (count), "3" (fade_table)
   : "%ebx");
}
# elif defined _MSC_VER
__inline void gr_linear_darken( ubyte * dest, int darkening_level, int count, ubyte * fade_table )
{
  __asm {
    mov edi,[dest]
	mov eax,[darkening_level]
	mov ecx,[count]
	mov edx,[fade_table]
	xor ebx, ebx
	mov bh, al
gld_loop:
	mov bl,[edi]
	mov al,[ebx+edx]
	mov [edi],al
	inc edi
	dec ecx
	jnz gld_loop
  }
}
# else
// Unknown compiler. So we use C rather than inline assembler.
#  define USE_C_GR_LINEAR_DARKEN 1
# endif

#else // No Assembler. So we use C.
# define USE_C_GR_LINEAR_DARKEN 1
void gr_linear_stosd( ubyte * dest, unsigned char color, unsigned int nbytes) {
	memset(dest,color,nbytes);
}
#endif

#ifdef USE_C_GR_LINEAR_DARKEN
void gr_linear_darken(ubyte * dest, int darkening_level, int count, ubyte * fade_table) {
	register int i;

	for (i=0;i<count;i++)
		*dest=fade_table[(*dest++)+(darkening_level*256)];
}
#endif

void gr_uscanline( int x1, int x2, int y )
{
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS )	{
#ifdef __DJGPP__
		switch(TYPE)
		{
		case BM_LINEAR:
#endif
			gr_linear_stosd( DATA + ROWSIZE*y + x1, (unsigned char)COLOR, x2-x1+1);
#ifdef __DJGPP__
                        break;
		case BM_MODEX:
			gr_modex_uscanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		case BM_SVGA:
			gr_vesa_scanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		}
#endif
	} else {
#ifdef __DJGPP__
		switch(TYPE)
		{
		case BM_LINEAR:
#endif
			gr_linear_darken( DATA + ROWSIZE*y + x1, Gr_scanline_darkening_level, x2-x1+1, gr_fade_table);
#ifdef __DJGPP__
			break;
		case BM_MODEX:
			gr_modex_uscanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		case BM_SVGA:
			gr_vesa_scanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		}
#endif
	}
}

void gr_scanline( int x1, int x2, int y )
{
	if ((y<0)||(y>MAXY)) return;

	if (x2 < x1 ) x2 ^= x1 ^= x2;

	if (x1 > MAXX) return;
	if (x2 < MINX) return;

	if (x1 < MINX) x1 = MINX;
	if (x2 > MAXX) x2 = MAXX;

	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS )	{
#ifdef __DJGPP__
		switch(TYPE)
		{
		case BM_LINEAR:
#endif
			gr_linear_stosd( DATA + ROWSIZE*y + x1, (unsigned char)COLOR, x2-x1+1);
#ifdef __DJGPP__
			break;
		case BM_MODEX:
			gr_modex_uscanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		case BM_SVGA:
			gr_vesa_scanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		}
#endif
	} else {
#ifdef __DJGPP__
		switch(TYPE)
		{
		case BM_LINEAR:
#endif
			gr_linear_darken( DATA + ROWSIZE*y + x1, Gr_scanline_darkening_level, x2-x1+1, gr_fade_table);
#ifdef __DJGPP__
			break;
		case BM_MODEX:
			gr_modex_uscanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		case BM_SVGA:
			gr_vesa_scanline( x1+XOFFSET, x2+XOFFSET, y+YOFFSET, COLOR );
			break;
		}
#endif
	}
}
