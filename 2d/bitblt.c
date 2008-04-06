/* $Id: bitblt.c,v 1.1.1.1 2006/03/17 19:51:56 zicodxx Exp $ */
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

#include <string.h>

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "byteswap.h"       // because of rle code that has short for row offsets
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
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
	memcpy(dest,src,num_pixels);
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
	register ubyte c;
	while (num_pixels--) {
		if (num_pixels&1)
			*dest++=*src++;
		else {
		        unsigned short *sp=(unsigned short *)dest;
			c=*src++;
			*sp=((short)c<<8)|(short)c;
			dest+=2;
		}
	}
}

#endif

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
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		return;
	}
#endif

	if ( (src->bm_flags & BM_FLAG_RLE ) && (src->bm_type == BM_LINEAR) ) {
		gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest );
		return;
	}

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
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
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
	if(bm->bm_type == BM_LINEAR && scr->bm_type == BM_OGL && 
		bm->bm_w <= grd_curscreen->sc_w && bm->bm_h <= grd_curscreen->sc_h) // only scale with OGL if bitmap is not bigger than screen size
	{
		ogl_ubitmapm_cs(0,0,-1,-1,bm,-1,F1_0);//use opengl to scale, faster and saves ram. -MPM
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
