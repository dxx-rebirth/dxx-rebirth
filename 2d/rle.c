/* $Id: rle.c,v 1.5 2002-08-15 05:42:33 btb Exp $ */
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
/*
 * Changed shorts to ints in parameters.
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: rle.c,v 1.5 2002-08-15 05:42:33 btb Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "mono.h"


#include "gr.h"
#include "grdef.h"
#include "error.h"
//#include "key.h"
#include "rle.h"
//#define RLE_CODE 		0xC0
//#define NOT_RLE_CODE	63

#define RLE_CODE 			0xE0
#define NOT_RLE_CODE		31

void rle_expand_texture_sub( grs_bitmap * bmp, grs_bitmap * rle_temp_bitmap_1 );

#ifndef NO_ASM
#ifdef __WATCOMC__
int gr_rle_decode_asm( ubyte * src, ubyte * dest );
#pragma aux gr_rle_decode_asm parm [esi] [edi] value [edi] modify exact [eax ebx ecx edx esi edi] = \
"  cld                  " \
"   xor ecx, ecx        " \
"   cld                 " \
"   jmp NextByte        " \
"                           " \
"Unique:                    " \
"   mov [edi],al        " \
"   inc edi         " \
"                           " \
"NextByte:              " \
"   mov al,[esi]        " \
"   inc esi         " \
"                           " \
"   mov ah, al      " \
"   and ah, 0xE0    " \
"  cmp  ah, 0xE0        " \
"   jne   Unique        " \
"                           " \
"   mov cl, al      " \
"   and cl, 31          " \
"   je      done            " \
"                           " \
"   mov al,[esi]        " \
"   inc esi         " \
"   mov ah, al      " \
"   shr ecx,1           " \
"   rep stosw           " \
"   jnc NextByte        " \
"   mov [edi],al        " \
"   inc edi         " \
"                           " \
"   jmp NextByte        " \
"                           " \
"done:                  ";

void rle_stosb(char *dest, int len, int color);
#pragma aux rle_stosb = "cld rep    stosb" parm [edi] [ecx] [eax] modify exact [edi ecx];
#elif defined __GNUC__
static inline int gr_rle_decode_asm( ubyte * src, ubyte * dest ) {
   register int __ret;
   int dummy;
   __asm__ __volatile__ (
"   cld;"
"   xorl %%ecx, %%ecx;"
"   jmp 1f;"
"0:;"
"   movb %%al,(%%edi);"
"   incl %%edi;"
"1:;"
"   movb (%%esi), %%al;"
"   incl %%esi;"
"   movb %%al, %%ah;"
"   andb $0xE0, %%ah;"
"   cmpb $0xE0, %%ah;"
"   jne 0b;"
"   movb %%al, %%cl;"
"   andb $31, %%cl;"
"   je 2f;"
"   movb (%%esi), %%al;"
"   incl %%esi;"
"   movb %%al, %%ah;"
"   shrl $1, %%ecx;"
"   rep; stosw;"
"   jnc 1b;"
"   movb %%al, (%%edi);"
"   incl %%edi;"
"   jmp 1b;"
"2:"
: "=D" (__ret), "=S" (dummy) : "1" (src), "D" (dest) : "%eax", "%ecx");
  return __ret;
}

static inline void rle_stosb(char *dest, int len, int color) {
	int dummy[2];
   __asm__ __volatile__ (
    "cld; rep; stosb"
    : "=D" (dummy[0]), "=c" (dummy[1])
	: "0" (dest), "1" (len), "a" (color) );
}
#elif defined _MSC_VER
__inline int gr_rle_decode_asm( ubyte * src, ubyte * dest )
{
	 int retval;
	__asm {
		mov esi,[src]
		mov edi,[dest]
        xor ecx, ecx
		cld
		jmp NextByte
Unique:
		mov [edi], al
		inc edi
NextByte:
		mov al,[esi]
		inc esi
		mov ah, al
		and ah,0xE0
		cmp ah,0xE0
		jne Unique

		mov cl, al
		and cl, 31
		je done

		mov al, [esi]
		inc esi
		mov ah, al
		shr ecx, 1
		rep stosw
		jnc NextByte
		mov [edi], al
		inc edi
		jmp NextByte
done:
		mov [retval],edi
	}
	return retval;
}

__inline void rle_stosb(char *dest, int len, int color)
{
  __asm {
	mov edi,[dest]
	mov ecx,[len]
	mov eax,[color]
	cld
	rep stosb
  }
}

#else
# undef NO_ASM
# define NO_ASM 1
/* Well, if inline assembler is not supported for this compiler, we don't
 **really** want ASM... */
#endif
#endif

#ifdef NO_ASM
void rle_stosb(ubyte *dest, int len, int color)
{
	int i;
	for (i=0; i<len; i++ )
		*dest++ = color;
}
#endif

void gr_rle_decode( ubyte * src, ubyte * dest )
{
#ifdef NO_ASM
	int i;
	ubyte data, count = 0;

	while(1) {
		data = *src++;
		if ( (data & RLE_CODE) != RLE_CODE ) {
			*dest++ = data;
		} else {
			count = data & NOT_RLE_CODE;
			if (count==0) return;
			data = *src++;
			for (i=0; i<count; i++ )	
				*dest++ = data;
		}
	}
#else
    gr_rle_decode_asm( src, dest );
#endif
}

// Given pointer to start of one scanline of rle data, uncompress it to
// dest, from source pixels x1 to x2.
void gr_rle_expand_scanline_masked( ubyte *dest, ubyte *src, int x1, int x2  )
{
	int i = 0;
        ubyte count=0;
        ubyte color=0;

	if ( x2 < x1 ) return;

	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE)==RLE_CODE )	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if ( x1+count > x2 )	{
		count = x2-x1+1;
		if ( color != 255 )	rle_stosb( dest, count, color );
		return;
	}

	if ( color != 255 )	rle_stosb( dest, count, color );
	dest += count;
	i += count;

	while( i <= x2 )		{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE) == (RLE_CODE) )	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
			if ( color != 255 )rle_stosb( dest, count, color );
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			if ( color != 255 )rle_stosb( dest, count, color );
			i += count;
			dest += count;
		}

	}	
}

void gr_rle_expand_scanline( ubyte *dest, ubyte *src, int x1, int x2  )
{
	int i = 0;
        ubyte count=0;
        ubyte color=0;

	if ( x2 < x1 ) return;

	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE)==RLE_CODE )	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if ( x1+count > x2 )	{
		count = x2-x1+1;
		rle_stosb( dest, count, color );
		return;
	}

	rle_stosb( dest, count, color );
	dest += count;
	i += count;

	while( i <= x2 )		{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE)==RLE_CODE )	{
			count = color & (~RLE_CODE);
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
			rle_stosb( dest, count, color );
			i += count;
			dest += count;
		} else {
			count = x2-i+1;
			rle_stosb( dest, count, color );
			i += count;
			dest += count;
		}
	}	
}


int gr_rle_encode( int org_size, ubyte *src, ubyte *dest )
{
	int i;
	ubyte c, oc;
	ubyte count;
	ubyte *dest_start;

	dest_start = dest;
	oc = *src++;
	count = 1;

	for (i=1; i<org_size; i++ )	{
		c = *src++;							
		if ( c!=oc )	{
			if ( count )	{
				if ( (count==1) && ((oc & RLE_CODE)!=RLE_CODE) )	{
					*dest++ = oc;
					Assert( oc != RLE_CODE );
				} else {
					count |= RLE_CODE;
					*dest++ = count;
					*dest++ = oc;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if ( count == NOT_RLE_CODE )	{
			count |= RLE_CODE;
			*dest++=count;
			*dest++=oc;
			count = 0;
		}
	}
	if (count)	{
		if ( (count==1) && ((oc & RLE_CODE)!=RLE_CODE) )	{
			*dest++ = oc;
			Assert( oc != RLE_CODE );
		} else {
			count |= RLE_CODE;
			*dest++ = count;
			*dest++ = oc;
		}
	}
	*dest++ = RLE_CODE;

	return dest-dest_start;
}


int gr_rle_getsize( int org_size, ubyte *src )
{
	int i;
	ubyte c, oc;
	ubyte count;
	int dest_size=0;

	oc = *src++;
	count = 1;

	for (i=1; i<org_size; i++ )	{
		c = *src++;							
		if ( c!=oc )	{
			if ( count )	{
				if ( (count==1) && ((oc & RLE_CODE)!=RLE_CODE) )	{
					dest_size++;
				} else {
					dest_size++;
					dest_size++;
				}
			}
			oc = c;
			count = 0;
		}
		count++;
		if ( count == NOT_RLE_CODE )	{
			dest_size++;
			dest_size++;
			count = 0;
		}
	}
	if (count)	{
		if ( (count==1) && ((oc & RLE_CODE)!=RLE_CODE) )	{
			dest_size++;
		} else {
			dest_size++;
			dest_size++;
		}
	}
	dest_size++;

	return dest_size;
}

int gr_bitmap_rle_compress( grs_bitmap * bmp )
{
	int y, d1, d;
	int doffset;
	ubyte *rle_data;

	rle_data=d_malloc( (bmp->bm_w+1)* bmp->bm_h );
	if (rle_data==NULL) return 0;
	doffset = 4 + bmp->bm_h;
	for (y=0; y<bmp->bm_h; y++ )	{
		d1= gr_rle_getsize( bmp->bm_w, &bmp->bm_data[bmp->bm_w*y] );
		if ( ((doffset+d1) > bmp->bm_w*bmp->bm_h) || (d1 > 255 ) )	{
			d_free(rle_data);
			return 0;
		}
		d = gr_rle_encode( bmp->bm_w, &bmp->bm_data[bmp->bm_w*y], &rle_data[doffset] );
		Assert( d==d1 );
		doffset	+= d;
		rle_data[y+4] = d;
	}
	//mprintf( 0, "Bitmap of size %dx%d, (%d bytes) went down to %d bytes\n", bmp->bm_w, bmp->bm_h, bmp->bm_h*bmp->bm_w, doffset );
	memcpy( 	rle_data, &doffset, 4 );
	memcpy( 	bmp->bm_data, rle_data, doffset );
	d_free(rle_data);
	bmp->bm_flags |= BM_FLAG_RLE;
	return 1;
}

#define MAX_CACHE_BITMAPS 32

typedef struct rle_cache_element {
	grs_bitmap * rle_bitmap;
	ubyte * rle_data;
	grs_bitmap * expanded_bitmap;			
	int last_used;
} rle_cache_element;

int rle_cache_initialized = 0;
int rle_counter = 0;
int rle_next = 0;
rle_cache_element rle_cache[MAX_CACHE_BITMAPS];

int rle_hits = 0;
int rle_misses = 0;

void rle_cache_close(void)
{
	if (rle_cache_initialized)	{
		int i;
		rle_cache_initialized = 0;
		for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
			gr_free_bitmap(rle_cache[i].expanded_bitmap);
		}
	}
}

void rle_cache_init()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].expanded_bitmap = gr_create_bitmap( 64, 64 );
		rle_cache[i].last_used = 0;
		Assert( rle_cache[i].expanded_bitmap != NULL );
	}	
	rle_cache_initialized = 1;
	atexit( rle_cache_close );
}

void rle_cache_flush()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].last_used = 0;
	}	
}



grs_bitmap * rle_expand_texture( grs_bitmap * bmp )
{
	int i;
	int lowest_count, lc;
	int least_recently_used;

	if (!rle_cache_initialized) rle_cache_init();

	Assert( !(bmp->bm_flags & BM_FLAG_PAGED_OUT) );

	lc = rle_counter;
	rle_counter++;
	if ( rle_counter < lc )	{
		for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
			rle_cache[i].rle_bitmap = NULL;
			rle_cache[i].last_used = 0;
		}
	}

//	if (((rle_counter % 100)==1) && (rle_hits+rle_misses > 0))
//		mprintf(( 0, "RLE-CACHE %d%%, H:%d, M:%d\n", (rle_misses*100)/(rle_hits+rle_misses), rle_hits, rle_misses ));

	lowest_count = rle_cache[rle_next].last_used;
	least_recently_used = rle_next;
	rle_next++;
	if ( rle_next >= MAX_CACHE_BITMAPS )
		rle_next = 0;
		
	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		if (rle_cache[i].rle_bitmap == bmp) 	{
			rle_hits++;
			rle_cache[i].last_used = rle_counter;
			return rle_cache[i].expanded_bitmap;
		}
		if ( rle_cache[i].last_used < lowest_count )	{
			lowest_count = rle_cache[i].last_used;
			least_recently_used = i;
		}
	}	
	rle_misses++;
	rle_expand_texture_sub( bmp, rle_cache[least_recently_used].expanded_bitmap );
	rle_cache[least_recently_used].rle_bitmap = bmp;
	rle_cache[least_recently_used].last_used = rle_counter;
	return rle_cache[least_recently_used].expanded_bitmap;
}

void rle_expand_texture_sub( grs_bitmap * bmp, grs_bitmap * rle_temp_bitmap_1 )
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i;
#ifndef NO_ASM
	unsigned char * dbits1;
#endif

#ifdef D1XD3D
	Assert (bmp->iMagic == BM_MAGIC_NUMBER);
#endif

	sbits = &bmp->bm_data[4 + 64];
	dbits = rle_temp_bitmap_1->bm_data;

	rle_temp_bitmap_1->bm_flags = bmp->bm_flags & (~BM_FLAG_RLE);

	for (i=0; i < 64; i++ )    {
#ifdef NO_ASM
                gr_rle_decode(sbits,dbits);
#else
		dbits1=(unsigned char *)gr_rle_decode_asm( sbits, dbits );
#endif
		sbits += (int)bmp->bm_data[4+i];
		dbits += 64;
#ifndef NO_ASM
		Assert( dbits == dbits1 );		// Get John, bogus rle data!
#endif
	}
#ifdef D1XD3D
	gr_set_bitmap_data (rle_temp_bitmap_1, rle_temp_bitmap_1->bm_data);
#endif
}


void gr_rle_expand_scanline_generic( grs_bitmap * dest, int dx, int dy, ubyte *src, 
	int x1, int x2, int masked  )
{
	int i = 0, j;
        int count=0;
        ubyte color=0;

	if ( x2 < x1 ) return;

	while ( i < x1 )	{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE) == RLE_CODE )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		i += count;
	}
	count = i - x1;
	i = x1;
	// we know have '*count' pixels of 'color'.
	
	if ( x1+count > x2 )	{
		count = x2-x1+1;
		if (!masked || color != 255)
		for ( j=0; j<count; j++ )
			gr_bm_pixel( dest, dx++, dy, color );
		return;
	}

	if (masked && color == 255)
		dx += count;
	else
	for ( j=0; j<count; j++ )
		gr_bm_pixel( dest, dx++, dy, color );
	i += count;

	while( i <= x2 )		{
		color = *src++;
		if ( color == RLE_CODE ) return;
		if ( (color & RLE_CODE) == RLE_CODE )	{
			count = color & NOT_RLE_CODE;
			color = *src++;
		} else {
			// unique
			count = 1;
		}
		// we know have '*count' pixels of 'color'.
		if ( i+count <= x2 )	{
			if (masked && color == 255)
				dx += count;
			else
			for ( j=0; j<count; j++ )
				gr_bm_pixel( dest, dx++, dy, color );
			i += count;
		} else {
			count = x2-i+1;
			if (masked && color == 255)
				dx += count;
			else
			for ( j=0; j<count; j++ )
				gr_bm_pixel( dest, dx++, dy, color );
			i += count;
		}
	}
}

void rle_swap_0_255(grs_bitmap *bmp)
{
	int i, j;

	if (!bmp->bm_data)
		return;

	if (bmp->bm_flags & BM_FLAG_RLE_BIG)
		j = 4 + 2 * bmp->bm_h;
	else
		j = 4 + bmp->bm_h;

	for (i = 0; i < bmp->bm_h; i++) {
		if ((bmp->bm_data[j] & RLE_CODE) != RLE_CODE) {
			if (bmp->bm_data[j] == 0)
				bmp->bm_data[j] = 255;
			else if (bmp->bm_data[j] == 255)
				bmp->bm_data[j] = 0;
			j++;
		} else {
			if ((bmp->bm_data[j] & NOT_RLE_CODE) == 0)
				continue;
			j++;
			if (bmp->bm_data[j] == 0)
				bmp->bm_data[j] = 255;
			else if (bmp->bm_data[j] == 255)
				bmp->bm_data[j] = 0;
			j++;
		}
	}
}
