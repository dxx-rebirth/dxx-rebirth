/* $Id: bitblt.c,v 1.15 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Routines for bitblt's.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "pa_enabl.h"                   //$$POLY_ACC
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "mono.h"
#include "byteswap.h"       // because of rle code that has short for row offsets
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *gr_bitblt_fade_table=NULL;

extern void gr_vesa_bitmap( grs_bitmap * source, grs_bitmap * dest, int x, int y );

void gr_linear_movsd( ubyte * source, ubyte * dest, unsigned int nbytes);
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_movsd parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
" cld "					\
" mov		ebx, ecx	"	\
" mov		eax, edi"	\
" and		eax, 011b"	\
" jz		d_aligned"	\
" mov		ecx, 4"		\
" sub		ecx, eax"	\
" sub		ebx, ecx"	\
" rep		movsb"		\
" d_aligned: "			\
" mov		ecx, ebx"	\
" shr		ecx, 2"		\
" rep 	movsd"		\
" mov		ecx, ebx"	\
" and 	ecx, 11b"	\
" rep 	movsb";

#elif !defined(NO_ASM) && defined(__GNUC__)

inline void gr_linear_movsd(ubyte *src, ubyte *dest, unsigned int num_pixels) {
	int dummy[3];
 __asm__ __volatile__ (
" cld;"
" movl      %%ecx, %%ebx;"
" movl      %%edi, %%eax;"
" andl      $3, %%eax;"
" jz        0f;"
" movl      $4, %%ecx;"
" subl      %%eax,%%ecx;"
" subl      %%ecx,%%ebx;"
" rep;      movsb;"
"0: ;"
" movl      %%ebx, %%ecx;"
" shrl      $2, %%ecx;"
" rep;      movsl;"
" movl      %%ebx, %%ecx;"
" andl      $3, %%ecx;"
" rep;      movsb"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 :	"%eax", "%ebx");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_movsd(ubyte *src, ubyte *dest, unsigned int num_pixels)
{
 __asm {
   mov esi, [src]
   mov edi, [dest]
   mov ecx, [num_pixels]
   cld
   mov ebx, ecx
   mov eax, edi
   and eax, 011b
   jz d_aligned
   mov ecx, 4
   sub ecx, eax
   sub ebx, ecx
   rep movsb
d_aligned:
   mov ecx, ebx
   shr ecx, 2
   rep movsd
   mov ecx, ebx
   and ecx, 11b
   rep movsb
 }
}

#else // NO_ASM or unknown compiler

#define THRESHOLD   8

#ifdef RELEASE
#define test_byteblit   0
#else
ubyte test_byteblit = 0;
#endif

void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels )
{
	int i;
	uint n, r;
	double *d, *s;
	ubyte *d1, *s1;

// check to see if we are starting on an even byte boundry
// if not, move appropriate number of bytes to even
// 8 byte boundry

	if ( (num_pixels < THRESHOLD) || (((int)src & 0x7) != ((int)dest & 0x7)) || test_byteblit ) {
		for (i = 0; i < num_pixels; i++)
			*dest++ = *src++;
		return;
	}

	i = 0;
	if ((r = (int)src & 0x7)) {
		for (i = 0; i < 8 - r; i++)
			*dest++ = *src++;
	}
	num_pixels -= i;

	n = num_pixels / 8;
	r = num_pixels % 8;
	s = (double *)src;
	d = (double *)dest;
	for (i = 0; i < n; i++)
		*d++ = *s++;
	s1 = (ubyte *)s;
	d1 = (ubyte *)d;
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
}

#endif	//#ifdef NO_ASM


static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsdm parm [esi] [edi] [ecx] modify exact [ecx esi edi eax] = \
"nextpixel:"                \
    "mov    al,[esi]"       \
    "inc    esi"            \
    "cmp    al, " TRANSPARENCY_COLOR_STR   \
    "je skip_it"            \
    "mov    [edi], al"      \
"skip_it:"                  \
    "inc    edi"            \
    "dec    ecx"            \
    "jne    nextpixel";

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels ) {
	int dummy[3];
 __asm__ __volatile__ (
"0: ;"
" movb  (%%esi), %%al;"
" incl  %%esi;"
" cmpb  $" TRANSPARENCY_COLOR_STR ", %%al;"
" je    1f;"
" movb  %%al,(%%edi);"
"1: ;"
" incl  %%edi;"
" decl  %%ecx;"
" jne   0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 : "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels )
{
 __asm {
  nextpixel:
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  mov al,  [esi]
  inc esi
  cmp al,  TRANSPARENCY_COLOR
  je skip_it
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

#else

static void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, unsigned int num_pixels )
{
	int i;
	for (i=0; i<num_pixels; i++ ) {
		if (*src != TRANSPARENCY_COLOR )
			*dest = *src;
		dest++;
		src++;
	}
}

#endif

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, ubyte fade_value );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] = \
"  xor eax, eax"                    \
"  mov ah, bl"                      \
"nextpixel:"                        \
    "mov    al,[esi]"               \
    "inc    esi"                    \
    "cmp    al, " TRANSPARENCY_COLOR_STR     \
    "je skip_it"                    \
    "mov  al, gr_fade_table[eax]"   \
    "mov    [edi], al"              \
"skip_it:"                          \
    "inc    edi"                    \
    "dec    ecx"                    \
    "jne    nextpixel";

#elif !defined(NO_ASM) && defined(__GNUC__)

/* #pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] */
static inline void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, ubyte fade_value ) {
	int dummy[4];
 __asm__ __volatile__ (
"  xorl   %%eax, %%eax;"
"  movb   %%bl, %%ah;"
"0:;"
"  movb   (%%esi), %%al;"
"  incl   %%esi;"
"  cmpb   $" TRANSPARENCY_COLOR_STR ", %%al;"
"  je 1f;"
#ifdef __ELF__
"  movb   gr_fade_table(%%eax), %%al;"
#else
"  movb   _gr_fade_table(%%eax), %%al;"
#endif
"  movb   %%al, (%%edi);"
"1:"
"  incl   %%edi;"
"  decl   %%ecx;"
"  jne    0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2]), "=b" (dummy[3])
 : "0" (src), "1" (dest), "2" (num_pixels), "3" (fade_value)
 : "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsdm_faded(void * src, void * dest, unsigned int num_pixels, ubyte fade_value )
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  movzx ebx, byte ptr [fade_value]
  xor eax, eax
  mov ah, bl
  nextpixel:
  mov al, [esi]
  inc esi
  cmp al, TRANSPARENCY_COLOR
  je skip_it
  mov al, gr_fade_table[eax]
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

#else

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, unsigned int num_pixels, ubyte fade_value )
{
	int i;
	ubyte source;
	ubyte *fade_base;

	fade_base = gr_fade_table + (fade_value * 256);

	for (i=num_pixels; i != 0; i-- )
	{
		source = *src;
		if (source != (ubyte)TRANSPARENCY_COLOR )
			*dest = *(fade_base + source);
		dest++;
		src++;
	}
}

#endif


void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels);

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
    "shr    ecx, 1"             \
    "jnc    nextpixel"          \
    "mov    al, [esi]"          \
    "mov    [edi], al"          \
    "inc    esi"                \
    "inc    edi"                \
    "cmp    ecx, 0"             \
    "je done"                   \
"nextpixel:"                    \
    "mov    al,[esi]"           \
    "mov    ah, al"             \
    "mov    [edi], ax"          \
    "inc    esi"                \
    "inc    edi"                \
    "inc    edi"                \
    "dec    ecx"                \
    "jne    nextpixel"          \
"done:"

#elif !defined(NO_ASM) && defined (__GNUC__)

inline void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels)
{
/* #pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] */
	int dummy[3];
 __asm__ __volatile__ (
    "shrl   $1, %%ecx;"
    "jnc    0f;"
    "movb   (%%esi), %%al;"
    "movb   %%al, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "cmpl   $0, %%ecx;"
    "je 1f;"
"0: ;"
    "movb   (%%esi), %%al;"
    "movb   %%al, %%ah;"
    "movw   %%ax, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "incl   %%edi;"
    "decl   %%ecx;"
    "jne    0b;"
"1:"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_dest_pixels)
 :      "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_dest_pixels)
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_dest_pixels]
  shr ecx, 1
  jnc nextpixel
  mov al, [esi]
  mov [edi], al
  inc esi
  inc edi
  cmp ecx, 0
  je done
nextpixel:
  mov al, [esi]
  mov ah, al
  mov [edi], ax
  inc esi
  inc edi
  inc edi
  dec ecx
  jne nextpixel
done:
 }
}

#else

void gr_linear_rep_movsd_2x(ubyte *src, ubyte *dest, unsigned int num_pixels)
{
	double  *d = (double *)dest;
	uint    *s = (uint *)src;
	uint    doubletemp[2];
	uint    temp, work;
	int     i;

	if (num_pixels & 0x3) {
		// not a multiple of 4?  do single pixel at a time
		for (i=0; i<num_pixels; i++) {
			*dest++ = *src;
			*dest++ = *src++;
		}
		return;
	}

	for (i = 0; i < num_pixels / 4; i++) {
		temp = work = *s++;

		temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
		temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
		doubletemp[0] = temp;

		work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
		work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
		doubletemp[1] = work;

		*d = *(double *) &(doubletemp[0]);
		d++;
	}
}

#endif


#ifdef __MSDOS__

static void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"            \
    "mov    al,[esi]"   \
    "add    esi, ebx"   \
    "mov    [edi], al"  \
    "add    edi, edx"   \
    "dec    ecx"        \
    "jne    nextpixel"

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize ) {
/*#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 __asm__ __volatile__ (
"0: ;"
    "movb   (%%esi), %%al;"
    "addl   %%ebx, %%esi;"
    "movb   %%al, (%%edi);"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : : "S" (src), "D" (dest), "c" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 : "%eax", "%ecx", "%esi", "%edi");
}

#else

static void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize )
{
	src = src;
	dest = dest;
	num_pixels = num_pixels;
	src_rowsize = src_rowsize;
	dest_rowsize = dest_rowsize;
	Int3();
}

#endif


static void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"            \
    "mov    al,[esi]"   \
    "add    esi, ebx"   \
    "cmp    al, " TRANSPARENCY_COLOR_STR   \
    "je skip_itx"       \
    "mov    [edi], al"  \
"skip_itx:"             \
    "add    edi, edx"   \
    "dec    ecx"        \
    "jne    nextpixel"

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize ) {
/* #pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 int dummy[3];
 __asm__ __volatile__ (
"0: ;"
    "movb    (%%esi), %%al;"
    "addl    %%ebx, %%esi;"
    "cmpb    $" TRANSPARENCY_COLOR_STR ", %%al;"
    "je 1f;"
    "movb   %%al, (%%edi);"
"1: ;"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 :      "%eax" );
}

#else

static void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize )
{
	src = src;
	dest = dest;
	num_pixels = num_pixels;
	src_rowsize = src_rowsize;
	dest_rowsize = dest_rowsize;
	Int3();
}

#endif

#endif /* __MSDOS__ */

void gr_ubitmap00( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);

	src = bm->bm_data;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x( src, dest, bm->bm_w );
		else
			gr_linear_movsd( src, dest, bm->bm_w );
		src += bm->bm_rowsize;
		dest+= (int)(dest_rowsize);
	}
}

void gr_ubitmap00m( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);

	src = bm->bm_data;

	if (gr_bitblt_fade_table==NULL) {
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm( src, dest, bm->bm_w );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	} else {
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm_faded( src, dest, bm->bm_w, gr_bitblt_fade_table[y1+y] );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	}
}

#if 0
"       jmp     aligned4        "   \
"       mov eax, edi            "   \
"       and eax, 11b            "   \
"       jz      aligned4        "   \
"       mov ebx, 4              "   \
"       sub ebx, eax            "   \
"       sub ecx, ebx            "   \
"alignstart:                    "   \
"       mov al, [esi]           "   \
"       add esi, 4              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne alignstart          "   \
"aligned4:                      "
#endif

#ifdef __MSDOS__

static void modex_copy_scanline( ubyte * src, ubyte * dest, int npixels );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx            "   \
"       and ebx, 11b            "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group        "   \
"next4pixels:                   "   \
"       mov al, [esi+8]         "   \
"       mov ah, [esi+12]        "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+4]         "   \
"       mov [edi], eax          "   \
"       add esi, 16             "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                      "   \
"       cmp ebx, 0              "   \
"       je      done2           "   \
"finishend:                     "   \
"       mov al, [esi]           "   \
"       add esi, 4              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                         ";

#elif !defined (NO_ASM) && defined(__GNUC__)

static inline void modex_copy_scanline( ubyte * src, ubyte * dest, int npixels ) {
/* #pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je   1f;"
"0: ;"
"       movb 8(%%esi), %%al;"
"       movb 12(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 4(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $16, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmpl $0, %%ebx;"
"       je      3f;"
"2: ;"
"       movb (%%esi), %%al;"
"       addl $4, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx" );
}

#else

static void modex_copy_scanline( ubyte * src, ubyte * dest, int npixels )
{
	src = src;
	dest = dest;
	npixels = npixels;
	Int3();
}

#endif

static void modex_copy_scanline_2x( ubyte * src, ubyte * dest, int npixels );

#if !defined(NO_ASM) && defined(__WATCOMC__)

#pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx            "   \
"       and ebx, 11b            "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group        "   \
"next4pixels:                   "   \
"       mov al, [esi+4]         "   \
"       mov ah, [esi+6]         "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+2]         "   \
"       mov [edi], eax          "   \
"       add esi, 8              "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                      "   \
"       cmp ebx, 0              "   \
"       je      done2           "   \
"finishend:                     "   \
"       mov al, [esi]           "   \
"       add esi, 2              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                         ";

#elif !defined(NO_ASM) && defined(__GNUC__)

static inline void modex_copy_scanline_2x( ubyte * src, ubyte * dest, int npixels ) {
/* #pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je 1f;"
"0: ;"
"       movb 4(%%esi), %%al;"
"       movb 6(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 2(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $8, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmp $0, %%ebx;"
"       je  3f;"
"2:"
"       movb (%%esi),%%al;"
"       addl $2, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx" );
}

#else

static void modex_copy_scanline_2x( ubyte * src, ubyte * dest, int npixels )
{
	src = src;
	dest = dest;
	npixels = npixels;
	Int3();
}

#endif


// From Linear to ModeX
void gr_bm_ubitblt01(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int sstep,dstep;
	int y,plane;
	int w1;

	if ( w < 4 ) return;

	sstep = src->bm_rowsize;
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	if (!gr_bitblt_double) {
		for (plane=0; plane<4; plane++ ) {
			gr_modex_setplane( (plane+dx)&3 );
			sbits = src->bm_data + (src->bm_rowsize * sy) + sx + plane;
			dbits = &gr_video_memory[(dest->bm_rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ( (w&3) > plane ) w1++;
			for (y=dy; y < dy+h; y++ ) {
				modex_copy_scanline( sbits, dbits, w1 );
				dbits += dstep;
				sbits += sstep;
			}
		}
	} else {
		for (plane=0; plane<4; plane++ ) {
			gr_modex_setplane( (plane+dx)&3 );
			sbits = src->bm_data + (src->bm_rowsize * sy) + sx + plane/2;
			dbits = &gr_video_memory[(dest->bm_rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ( (w&3) > plane ) w1++;
			for (y=dy; y < dy+h; y++ ) {
				modex_copy_scanline_2x( sbits, dbits, w1 );
				dbits += dstep;
				sbits += sstep;
			}
		}
	}
}


// From Linear to ModeX masked
void gr_bm_ubitblt01m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	//ubyte * dbits1;
	//ubyte * sbits1;

	ubyte * dbits;
	ubyte * sbits;

	int x;
	//int y;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   &gr_video_memory[(dest->bm_rowsize * dy) + dx/4];

	for (x=dx; x < dx+w; x++ ) {
		gr_modex_setplane( x&3 );

		//sbits1 = sbits;
		//dbits1 = dbits;
		//for (y=0; y < h; y++ )    {
		//	*dbits1 = *sbits1;
		//	sbits1 += src_bm_rowsize;
		//	dbits1 += dest_bm_rowsize;
		//}
		modex_copy_column_m(sbits, dbits, h, src->bm_rowsize, dest->bm_rowsize << gr_bitblt_dest_step_shift );

		sbits++;
		if ( (x&3)==3 )
			dbits++;
	}
}

#endif /* __MSDOS__ */


void gr_ubitmap012( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_data;

	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			gr_setcolor( *src++ );
			gr_upixel( x1, y1 );
		}
	}
}

void gr_ubitmap012m( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_data;

	for (y1=y; y1 < (y+bm->bm_h); y1++ ) {
		for (x1=x; x1 < (x+bm->bm_w); x1++ ) {
			if ( *src != TRANSPARENCY_COLOR ) {
				gr_setcolor( *src );
				gr_upixel( x1, y1 );
			}
			src++;
		}
	}
}

#if defined(POLY_ACC)
void gr_ubitmap05( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char *src;
	short *dst;
	int mod;

	pa_flush();
	src = bm->bm_data;
	dst = (short *)(DATA + y * ROWSIZE + x * PA_BPP);
	mod = ROWSIZE / 2 - bm->bm_w;

	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			*dst++ = pa_clut[*src++];
		}
		dst += mod;
	}
}

void gr_ubitmap05m( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char *src;
	short *dst;
	int mod;

	pa_flush();
	src = bm->bm_data;
	dst = (short *)(DATA + y * ROWSIZE + x * PA_BPP);
	mod = ROWSIZE / 2 - bm->bm_w;

	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			if ( *src != TRANSPARENCY_COLOR )   {
				*dst = pa_clut[*src];
			}
			src++;
			++dst;
		}
		dst += mod;
	}
}

void gr_bm_ubitblt05_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned short * dbits;
	unsigned char * sbits, scanline[640];
	int i, data_offset, j, nextrow;

	pa_flush();
	nextrow=dest->bm_rowsize/PA_BPP;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = (unsigned short *)(dest->bm_data + (dest->bm_rowsize * dy) + dx*PA_BPP);

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline( scanline, sbits, sx, sx+w-1 );
		for(j = 0; j != w; ++j)
			dbits[j] = pa_clut[scanline[j]];
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += nextrow;
	}
}

void gr_bm_ubitblt05m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned short * dbits;
	unsigned char * sbits, scanline[640];
	int i, data_offset, j, nextrow;

	pa_flush();
	nextrow=dest->bm_rowsize/PA_BPP;
	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = (unsigned short *)(dest->bm_data + (dest->bm_rowsize * dy) + dx*PA_BPP);

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline( scanline, sbits, sx, sx+w-1 );
		for(j = 0; j != w; ++j)
			if(scanline[j] != TRANSPARENCY_COLOR)
				dbits[j] = pa_clut[scanline[j]];
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += nextrow;
	}
}
#endif

void gr_ubitmapGENERIC(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		for (x1=0; x1 < bm->bm_w; x1++ )    {
			gr_setcolor( gr_gpixel(bm,x1,y1) );
			gr_upixel( x+x1, y+y1 );
		}
	}
}

void gr_ubitmapGENERICm(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;
	ubyte c;

	for (y1=0; y1 < bm->bm_h; y1++ ) {
		for (x1=0; x1 < bm->bm_w; x1++ ) {
			c = gr_gpixel(bm,x1,y1);
			if ( c != TRANSPARENCY_COLOR ) {
				gr_setcolor( c );
				gr_upixel( x+x1, y+y1 );
			}
		}
	}
}


#ifdef __MSDOS__
// From linear to SVGA
void gr_bm_ubitblt02(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;

	unsigned int offset, EndingOffset, VideoLocation;

	int sbpr, dbpr, y1, page, BytesToMove;

	sbpr = src->bm_rowsize;

	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;

	sbits = src->bm_data + ( sbpr*sy ) + sx;

	for (y1=0; y1 < h; y1++ )    {

		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;

		gr_vesa_setpage( page );

		EndingOffset = offset+w-1;

		if ( EndingOffset <= 0xFFFF )
		{
			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+0xA0000), w );
			else
				gr_linear_movsd( (void *)sbits, (void *)(offset+0xA0000), w );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;

			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+0xA0000), BytesToMove );
			else
				gr_linear_movsd( (void *)sbits, (void *)(offset+0xA0000), BytesToMove );

			page++;
			gr_vesa_setpage(page);

			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)(sbits+BytesToMove/2), (void *)0xA0000, EndingOffset - 0xFFFF );
			else
				gr_linear_movsd( (void *)(sbits+BytesToMove), (void *)0xA0000, EndingOffset - 0xFFFF );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}
#endif

#ifdef __MSDOS__

void gr_bm_ubitblt02m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;

	unsigned int offset, EndingOffset, VideoLocation;

	int sbpr, dbpr, y1, page, BytesToMove;

	sbpr = src->bm_rowsize;

	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;

	sbits = src->bm_data + ( sbpr*sy ) + sx;

	for (y1=0; y1 < h; y1++ )    {

		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;

		gr_vesa_setpage( page );

		EndingOffset = offset+w-1;

		if ( EndingOffset <= 0xFFFF )
		{
			gr_linear_rep_movsdm( (void *)sbits, (void *)(offset+0xA0000), w );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;

			gr_linear_rep_movsdm( (void *)sbits, (void *)(offset+0xA0000), BytesToMove );

			page++;
			gr_vesa_setpage(page);

			gr_linear_rep_movsdm( (void *)(sbits+BytesToMove), (void *)0xA0000, EndingOffset - 0xFFFF );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}


// From SVGA to linear
void gr_bm_ubitblt20(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;

	unsigned int offset, offset1, offset2;

	int sbpr, dbpr, y1, page;

	dbpr = dest->bm_rowsize;

	sbpr = src->bm_rowsize;

	for (y1=0; y1 < h; y1++ )    {

		offset2 =   (unsigned int)src->bm_data  + (sbpr * (y1+sy)) + sx;
		dbits   =   dest->bm_data + (dbpr * (y1+dy)) + dx;

		page = offset2 >> 16;
		offset = offset2 & 0xFFFF;
		offset1 = offset+w-1;
		gr_vesa_setpage( page );

		if ( offset1 > 0xFFFF )  {
			// Overlaps two pages
			while( offset <= 0xFFFF )
				*dbits++ = gr_video_memory[offset++];
			offset1 -= (0xFFFF+1);
			offset = 0;
			page++;
			gr_vesa_setpage(page);
		}
		while( offset <= offset1 )
			*dbits++ = gr_video_memory[offset++];

	}
}

#endif

//@extern int Interlacing_on;

// From Linear to Linear
void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;

	int i;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x( sbits, dbits, w );
		else
			gr_linear_movsd( sbits, dbits, w );
		sbits += src->bm_rowsize;
		dbits += dstep;
	}
}
// From Linear to Linear Masked
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int src_bm_rowsize_2, dest_bm_rowsize_2;

	int i;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (gr_bitblt_fade_table==NULL) {
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm( sbits, dbits, w );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	} else {
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm_faded( sbits, dbits, w, gr_bitblt_fade_table[dy+i] );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
}


extern void gr_lbitblt( grs_bitmap * source, grs_bitmap * dest, int height, int width );

#ifdef MACINTOSH

// width == number of destination pixels

void gr_linear_movsd_double(ubyte *src, ubyte *dest, int width)
{
	double  *d = (double *)dest;
	uint    *s = (uint *)src;
	uint    doubletemp[2];
	uint    temp, work;
	int     i, num_pixels;

	num_pixels = width / 2;
	if ( (num_pixels & 0x3) || (((int)src & 0x7) != ((int)dest & 0x7)) ) {
		// not a multiple of 4?  do single pixel at a time
		for (i=0; i<num_pixels; i++) {
			*dest++ = *src;
			*dest++ = *src++;
		}
		return;
	}

	for (i = 0; i < num_pixels / 4; i++) {
		temp = work = *s++;

		temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
		temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
		doubletemp[0] = temp;

		work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
		work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
		doubletemp[1] = work;

		*d = *(double *) &(doubletemp[0]);
		d++;
	}
}

//extern void BlitLargeAlign(ubyte *draw_buffer, int dstRowBytes, ubyte *dstPtr, int w, int h, int modulus);

asm void BlitLargeAlign(ubyte *rSrcPtr, int rDblDStrd, ubyte *rDst1Ptr, int rWidth, int rHeight, int rModulus)
{
    stw     r31,-4(SP)          // store non-volatile reg in red zone
    addi    r5,r5,-8            // subtract 8 from dst
    stw     r30,-8(SP)          // store non-volatile reg in red zone

    la      r30,-16(SP)         // calculate copy of local 8-byte variable
    sub     r9,r8,r6
                                // rSStrd = modulus - w
    add     r31,r5,r4           // dst2 = dstRowBytes + dst1
    sub     r4,r4,r6            // r4 = dstRowBytes - w
    addi    r7,r7,-1            // subtract 1 from height count
    srawi   r6,r6,2             // rWidth = w >> 2
    addi    r3,r3,-4            // subtract 4 from src
    addi    r6,r6,-1            // subtract 1 from width count
    add     r4,r4,r4            // rDblDStrd = 2 * r4

BlitLargeAlignY:                // y count is in r7
    lwzu     r10,4(r3)          // load a long into r10
    mr       r0,r10             // put a copy in r0
    mr       r11,r10
// these are simplified -- can't use 'em    inslwi   r0,r10,16,8
// these are simplified -- can't use 'em    insrwi   r11,r10,16,8
    rlwimi   r0,r10,24,8,31
    rlwimi   r11,r10,8,8,23
    rlwimi   r0,r10,16,24,31
    stw      r0,0(r30)
    rlwimi   r11,r10,16,0,7
    stw      r11,4(r30)
    mtctr       r6              // copy x count into the counter
    lfd      fp0,0(r30)

BlitLargeAlignX:
    lwzu     r10,4(r3)          // load a long into r10
    stfdu    fp0,8(r5)
    mr       r0,r10             // put a copy in r0
    mr       r11,r10
// simplefied   inslwi   r0,r10,16,8
// simplefied   insrwi   r11,r10,16,8
    rlwimi   r0,r10,24,8,31
    rlwimi   r11,r10,8,8,23
    rlwimi   r0,r10,16,24,31
    stw      r0,0(r30)
    rlwimi   r11,r10,16,0,7
    stw      r11,4(r30)
    stfdu    fp0,8(r31)
    lfd      fp0,0(r30)
    bdnz     BlitLargeAlignX    // loop over all x

    stfdu    fp0,8(r5)
    addic.   r7,r7,-1           // decrement the counter
    add      r3,r3,r9
                                // src += sstride
    add      r5,r5,r4
                                // dst1 += dstride
    stfdu    fp0,8(r31)
    add      r31,r31,r4
                                // dst2 += dstride
    bne      BlitLargeAlignY    // loop for all y

    lwz     r30,-8(SP)          // restore non-volatile regs
    lwz     r31,-4(SP)          // restore non-volatile regs
    blr                         // return to caller
}

void gr_bm_ubitblt_double(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap *src, grs_bitmap *dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int dstep, i;

	sbits = src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;
	Assert( !((int)dbits & 0x7) );  // assert to check double word alignment
	BlitLargeAlign(sbits, dstep, dbits, src->bm_w, src->bm_h, src->bm_rowsize);
}

// w and h are the doubled width and height

void gr_bm_ubitblt_double_slow(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap *src, grs_bitmap *dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int dstep, i;

	sbits = src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	for (i=0; i < h; i++ )    {

		gr_linear_movsd_double(sbits, dbits, w);
		dbits += dstep;
		if (i & 1)
			sbits += src->bm_rowsize;
	}
}

#endif /* MACINTOSH */


// Clipped bitmap ...

void gr_bitmap( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w ) { dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h ) { dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

}

//-NOT-used // From linear to SVGA
//-NOT-used void gr_bm_ubitblt02_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * sbits;
//-NOT-used
//-NOT-used 	unsigned int offset, EndingOffset, VideoLocation;
//-NOT-used
//-NOT-used 	int sbpr, dbpr, y1, page, BytesToMove;
//-NOT-used
//-NOT-used 	sbpr = src->bm_rowsize;
//-NOT-used
//-NOT-used 	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;
//-NOT-used
//-NOT-used 	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;
//-NOT-used
//-NOT-used 	sbits = src->bm_data + ( sbpr*sy ) + sx;
//-NOT-used
//-NOT-used 	for (y1=0; y1 < h; y1++ )    {
//-NOT-used
//-NOT-used 		page    = VideoLocation >> 16;
//-NOT-used 		offset  = VideoLocation & 0xFFFF;
//-NOT-used
//-NOT-used 		gr_vesa_setpage( page );
//-NOT-used
//-NOT-used 		EndingOffset = offset+w-1;
//-NOT-used
//-NOT-used 		if ( EndingOffset <= 0xFFFF )
//-NOT-used 		{
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+0xA0000), w );
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used 		else
//-NOT-used 		{
//-NOT-used 			BytesToMove = 0xFFFF-offset+1;
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+0xA0000), BytesToMove );
//-NOT-used
//-NOT-used 			page++;
//-NOT-used 			gr_vesa_setpage(page);
//-NOT-used
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)(sbits+BytesToMove/2), (void *)0xA0000, EndingOffset - 0xFFFF );
//-NOT-used
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used
//-NOT-used
//-NOT-used 	}
//-NOT-used }


//-NOT-used // From Linear to Linear
//-NOT-used void gr_bm_ubitblt00_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * dbits;
//-NOT-used 	unsigned char * sbits;
//-NOT-used 	//int src_bm_rowsize_2, dest_bm_rowsize_2;
//-NOT-used
//-NOT-used 	int i;
//-NOT-used
//-NOT-used 	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
//-NOT-used 	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;
//-NOT-used
//-NOT-used 	// No interlacing, copy the whole buffer.
//-NOT-used 	for (i=0; i < h; i++ )    {
//-NOT-used 		gr_linear_rep_movsd_2x( sbits, dbits, w );
//-NOT-used
//-NOT-used 		sbits += src->bm_rowsize;
//-NOT-used 		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
//-NOT-used 	}
//-NOT-used }

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];

	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline( dbits, sbits, sx, sx+w-1 );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline_masked( dbits, sbits, sx, sx+w-1 );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

// in rle.c

extern void gr_rle_expand_scanline_generic( grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2  );
extern void gr_rle_expand_scanline_generic_masked( grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2  );
extern void gr_rle_expand_scanline_svga_masked( grs_bitmap * dest, int dx, int dy, ubyte *src, int x1, int x2  );

void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//mprintf( 0, "SVGA RLE!\n" );

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_generic( dest, dx, dy+y1,  sbits, sx, sx+w-1  );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_data[4+y1+sy];
	}

}

void gr_bm_ubitblt0xm_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//mprintf( 0, "SVGA RLE!\n" );

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_generic_masked( dest, dx, dy+y1,  sbits, sx, sx+w-1  );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_data[4+y1+sy];
	}

}

#ifdef __MSDOS__
void gr_bm_ubitblt02m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	//mprintf( 0, "SVGA RLE!\n" );

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_svga_masked( dest, dx, dy+y1,  sbits, sx, sx+w-1  );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_data[4+y1+sy];
	}
}
#endif

void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;

	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_LINEAR ))
	{
		if ( src->bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00_rle( w, h, dx, dy, sx, sy, src, dest );
		else
			gr_bm_ubitblt00( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

#ifdef OGL
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_LINEAR ))
	{
		ogl_ubitblt_tolinear(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt_copy(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
#endif

#ifdef D1XD3D
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)dest->bm_data == BM_D3D_RENDER || (int)dest->bm_data == BM_D3D_DISPLAY);
		Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 0);
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_LINEAR ))
	{
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_DIRECTX ))
	{
		return;
	}
#endif

	if ( (src->bm_flags & BM_FLAG_RLE ) && (src->bm_type == BM_LINEAR) ) {
		gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest );
		return;
	}

#ifdef __MSDOS__
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_SVGA ))
	{
		gr_bm_ubitblt02( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

	if ( (src->bm_type == BM_SVGA) && (dest->bm_type == BM_LINEAR ))
	{
		gr_bm_ubitblt20( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_MODEX ))
	{
		gr_bm_ubitblt01( w, h, dx+XOFFSET, dy+YOFFSET, sx, sy, src, dest );
		return;
	}
#endif

#if defined(POLY_ACC)
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_LINEAR15 ))
	{
		ubyte *s = src->bm_data + sy * src->bm_rowsize + sx;
		ushort *t = (ushort *)(dest->bm_data + dy * dest->bm_rowsize + dx * PA_BPP);
		int x;
		pa_flush();
		for(;h--;)
		{
			for(x = 0; x < w; x++)
				t[x] = pa_clut[s[x]];
			s += src->bm_rowsize;
			t += dest->bm_rowsize / PA_BPP;
		}
		return;
	}

	if ( (src->bm_type == BM_LINEAR15) && (dest->bm_type == BM_LINEAR15 ))
	{
		pa_blit(dest, dx, dy, src, sx, sy, w, h);
		return;
	}
#endif

	for (y1=0; y1 < h; y1++ )    {
		for (x1=0; x1 < w; x1++ )    {
			gr_bm_pixel( dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1) );
		}
	}
}

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int dx1=dx, dx2=dx+dest->bm_w-1;
	int dy1=dy, dy2=dy+dest->bm_h-1;

	int sx1=sx, sx2=sx+src->bm_w-1;
	int sy1=sy, sy2=sy+src->bm_h-1;

	if ((dx1 >= dest->bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h ) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx1 += -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy1 += -dy1; dy1 = 0; }
	if ( dx2 >= dest->bm_w ) { dx2 = dest->bm_w-1; }
	if ( dy2 >= dest->bm_h ) { dy2 = dest->bm_h-1; }

	if ((sx1 >= src->bm_w ) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h ) || (sy2 < 0)) return;
	if ( sx1 < 0 ) { dx1 += -sx1; sx1 = 0; }
	if ( sy1 < 0 ) { dy1 += -sy1; sy1 = 0; }
	if ( sx2 >= src->bm_w ) { sx2 = src->bm_w-1; }
	if ( sy2 >= src->bm_h ) { sy2 = src->bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if ( dx2-dx1+1 < w )
		w = dx2-dx1+1;
	if ( dy2-dy1+1 < h )
		h = dy2-dy1+1;
	if ( sx2-sx1+1 < w )
		w = sx2-sx1+1;
	if ( sy2-sy1+1 < h )
		h = sy2-sy1+1;

	gr_bm_ubitblt(w,h, dx1, dy1, sx1, sy1, src, dest );
}

void gr_ubitmap( int x, int y, grs_bitmap *bm )
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	if (source==BM_LINEAR) {
		switch( dest )
		{
		case BM_LINEAR:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap00( x, y, bm );
			return;
#ifdef OGL
		case BM_OGL:
			ogl_ubitmapm(x,y,bm);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			Assert ((int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_RENDER || (int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bm, 0, 0, bm->bm_w, bm->bm_h, x, y, 0);
			return;
#endif
#ifdef __MSDOS__
		case BM_SVGA:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt0x_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_vesa_bitmap( bm, &grd_curcanv->cv_bitmap, x, y );
			return;
		case BM_MODEX:
			gr_bm_ubitblt01(bm->bm_w, bm->bm_h, x+XOFFSET, y+YOFFSET, 0, 0, bm, &grd_curcanv->cv_bitmap);
			return;
#endif
#if defined(POLY_ACC)
		case BM_LINEAR15:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt05_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap05( x, y, bm);
			return;

#endif
		default:
			gr_ubitmap012( x, y, bm );
			return;
		}
	} else  {
		gr_ubitmapGENERIC(x, y, bm);
	}
}


void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	Assert(x+bm->bm_w <= grd_curcanv->cv_w);
	Assert(y+bm->bm_h <= grd_curcanv->cv_h);

#ifdef _3DFX
	_3dfx_Blit( x, y, bm );
	if ( _3dfx_skip_ddraw )
		return;
#endif

	if (source==BM_LINEAR) {
		switch( dest )
		{
		case BM_LINEAR:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap00m( x, y, bm );
			return;
#ifdef OGL
		case BM_OGL:
			ogl_ubitmapm(x,y,bm);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			if (bm->bm_w < 35 && bm->bm_h < 35) {
				// ugly hack needed for reticle
				if ( bm->bm_flags & BM_FLAG_RLE )
					gr_bm_ubitblt0x_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap, 1 );
				else
					gr_ubitmapGENERICm(x, y, bm);
				return;
			}
			Assert ((int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_RENDER || (int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bm, 0, 0, bm->bm_w, bm->bm_h, x, y, 1);
			return;
#endif
#ifdef __MSDOS__
		case BM_SVGA:
			if (bm->bm_flags & BM_FLAG_RLE)
				gr_bm_ubitblt02m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
			//gr_bm_ubitblt0xm_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
			else
				gr_bm_ubitblt02m(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
			//gr_ubitmapGENERICm(x, y, bm);
			return;
		case BM_MODEX:
			gr_bm_ubitblt01m(bm->bm_w, bm->bm_h, x+XOFFSET, y+YOFFSET, 0, 0, bm, &grd_curcanv->cv_bitmap);
			return;
#endif
#if defined(POLY_ACC)
		case BM_LINEAR15:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt05m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap05m( x, y, bm );
			return;
#endif

		default:
			gr_ubitmap012m( x, y, bm );
			return;
		}
	} else {
		gr_ubitmapGENERICm(x, y, bm);
	}
}


void gr_bitmapm( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w ) { dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h ) { dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	if ( (bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_LINEAR ))
	{
		if ( bm->bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00m_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		else
			gr_bm_ubitblt00m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		return;
	}
#ifdef __MSDOS__
	else if ( (bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_SVGA ))
	{
		gr_bm_ubitblt02m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		return;
	}
#endif

	gr_bm_ubitbltm(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

}

void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;
	ubyte c;

#ifdef OGL
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_LINEAR ))
	{
		ogl_ubitblt_tolinear(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt_copy(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
#endif
#ifdef D1XD3D
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)dest->bm_data == BM_D3D_RENDER || (int)dest->bm_data == BM_D3D_DISPLAY);
		Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 1);
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)src->bm_data == BM_D3D_RENDER || (int)src->bm_data == BM_D3D_DISPLAY);
		//Win32_BlitDirectXToDirectX (w, h, dx, dy, sx, sy, src->bm_data, dest->bm_data, 0);
		return;
	}
#endif
#if defined(POLY_ACC)
	if(src->bm_type == BM_LINEAR && dest->bm_type == BM_LINEAR15)
	{
		ubyte *s;
		ushort *d;
		ushort u;
		int smod, dmod;

		pa_flush();
		s = (ubyte *)(src->bm_data + src->bm_rowsize * sy + sx);
		smod = src->bm_rowsize - w;
		d = (ushort *)(dest->bm_data + dest->bm_rowsize * dy + dx * PA_BPP);
		dmod = dest->bm_rowsize / PA_BPP - w;
		for (; h--;) {
			for (x1=w; x1--; ) {
				if ((u = *s) != TRANSPARENCY_COLOR)
					*d = pa_clut[u];
				++s;
				++d;
			}
			s += smod;
			d += dmod;
		}
	}

	if(src->bm_type == BM_LINEAR15)
	{
		Assert(src->bm_type == dest->bm_type);         // I don't support 15 to 8 yet.
		pa_blit_transparent(dest, dx, dy, src, sx, sy, w, h);
		return;
	}
#endif

	for (y1=0; y1 < h; y1++ )    {
		for (x1=0; x1 < w; x1++ )    {
			if ((c=gr_gpixel(src,sx+x1,sy+y1))!=TRANSPARENCY_COLOR)
				gr_bm_pixel( dest, dx+x1, dy+y1,c  );
		}
	}
}


// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void scale_line(unsigned char *in, unsigned char *out, int ilen, int olen)
{
	int a = olen/ilen, b = olen%ilen;
	int c = 0, i;
	unsigned char *end = out + olen;
	while(out<end) {
		i = a;
		c += b;
		if(c >= ilen) {
			c -= ilen;
			goto inside;
		}
		while(--i>=0) {
		inside:
			*out++ = *in;
		}
		in++;
	}
}

void gr_bitmap_scale_to(grs_bitmap *src, grs_bitmap *dst)
{
	unsigned char *s = src->bm_data;
	unsigned char *d = dst->bm_data;
	int h = src->bm_h;
	int a = dst->bm_h/h, b = dst->bm_h%h;
	int c = 0, i, y;

	for(y=0; y<h; y++) {
		i = a;
		c += b;
		if(c >= h) {
			c -= h;
			goto inside;
		}
		while(--i>=0) {
		inside:
			scale_line(s, d, src->bm_w, dst->bm_w);
			d += dst->bm_rowsize;
		}
		s += src->bm_rowsize;
	}
}

void show_fullscr(grs_bitmap *bm)
{
	grs_bitmap * const scr = &grd_curcanv->cv_bitmap;

#ifdef OGL
	if(bm->bm_type == BM_LINEAR && scr->bm_type == BM_OGL) {
		ogl_ubitblt_i(scr->bm_w,scr->bm_h,0,0,bm->bm_w,bm->bm_h,0,0,bm,scr);//use opengl to scale, faster and saves ram. -MPM
		return;
	}
#endif
	if(scr->bm_type != BM_LINEAR) {
		grs_bitmap *tmp = gr_create_bitmap(scr->bm_w, scr->bm_h);
		gr_bitmap_scale_to(bm, tmp);
		gr_bitmap(0, 0, tmp);
		gr_free_bitmap(tmp);
		return;
	}
	gr_bitmap_scale_to(bm, scr);
}
