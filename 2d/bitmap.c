/* $Id: bitmap.c,v 1.5 2002-10-10 19:11:23 btb Exp $ */
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
 *
 * Graphical routines for manipulating grs_bitmaps.
 *
 * Old Log:
 * Revision 1.11  1995/08/23  18:46:06  allender
 * fixed compiler warning
 *
 * Revision 1.10  1995/08/14  14:25:45  allender
 * changed transparency color to 0
 *
 * Revision 1.9  1995/07/05  16:04:51  allender
 * transparency/game kitchen changes
 *
 * Revision 1.8  1995/06/15  09:50:48  allender
 * new malloc to align bitmap on 8 byte bountry
 *
 * Revision 1.7  1995/05/12  11:52:19  allender
 * changed memory stuff again
 *
 * Revision 1.6  1995/05/11  12:48:34  allender
 * nge transparency color
 *
 * Revision 1.5  1995/05/04  19:59:21  allender
 * use NewPtr instead of malloc
 *
 * Revision 1.4  1995/04/27  07:33:04  allender
 * rearrange functions
 *
 * Revision 1.3  1995/04/19  14:37:17  allender
 * removed dead asm code
 *
 * Revision 1.2  1995/04/18  12:04:51  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/03/09  08:48:06  allender
 * Initial revision
 *
 *
 * -------  PC version RCS information
 * Revision 1.17  1994/11/18  22:50:25  john
 * Changed shorts to ints in parameters.
 *
 * Revision 1.16  1994/11/10  15:59:46  john
 * Fixed bugs with canvas's being created with bogus bm_flags.
 *
 * Revision 1.15  1994/10/26  23:55:53  john
 * Took out roller; Took out inverse table.
 *
 * Revision 1.14  1994/09/19  14:40:21  john
 * Changed dpmi stuff.
 *
 * Revision 1.13  1994/09/19  11:44:04  john
 * Changed call to allocate selector to the dpmi module.
 *
 * Revision 1.12  1994/06/09  13:14:57  john
 * Made selectors zero our
 * out, I meant.
 *
 * Revision 1.11  1994/05/06  12:50:07  john
 * Added supertransparency; neatend things up; took out warnings.
 *
 * Revision 1.10  1994/04/08  16:59:39  john
 * Add fading poly's; Made palette fade 32 instead of 16.
 *
 * Revision 1.9  1994/03/16  17:21:09  john
 * Added slow palette searching options.
 *
 * Revision 1.8  1994/03/14  17:59:35  john
 * Added function to check bitmap's transparency.
 *
 * Revision 1.7  1994/03/14  17:16:21  john
 * fixed bug with counting freq of pixels.
 *
 * Revision 1.6  1994/03/14  16:55:47  john
 * Changed grs_bitmap structure to include bm_flags.
 *
 * Revision 1.5  1994/02/18  15:32:22  john
 * *** empty log message ***
 *
 * Revision 1.4  1993/10/15  16:22:49  john
 * *** empty log message ***
 *
 * Revision 1.3  1993/09/08  17:37:11  john
 * Checking for errors with Yuan...
 *
 * Revision 1.2  1993/09/08  14:46:27  john
 * looking for possible bugs...
 *
 * Revision 1.1  1993/09/08  11:43:05  john
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "error.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

void gr_set_bitmap_data (grs_bitmap *bm, unsigned char *data)
{
#ifdef OGL
//	if (bm->bm_data!=data)
		ogl_freebmtexture(bm);
#endif
	bm->bm_data = data;
#ifdef D1XD3D
	Assert (bm->iMagic == BM_MAGIC_NUMBER);
	Win32_SetTextureBits (bm, data, bm->bm_flags & BM_FLAG_RLE);
#endif
}

grs_bitmap *gr_create_bitmap(int w, int h )
{
	return gr_create_bitmap_raw (w, h, d_malloc(w * h));
}

grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data )
{
    grs_bitmap *new;

    new = (grs_bitmap *)d_malloc( sizeof(grs_bitmap) );
	gr_init_bitmap (new, 0, 0, 0, w, h, w, raw_data);

    return new;
}


#if defined(POLY_ACC)
//
//  Creates a bitmap of the requested size and type.
//    w, and h are in pixels.
//    type is a BM_... and is used to set the rowsize.
//    if data is NULL, memory is allocated, otherwise data is used for bm_data.
//
//  This function is used only by the polygon accelerator code to handle the mixture of 15bit and
//  8bit bitmaps.
//
grs_bitmap *gr_create_bitmap2(int w, int h, int type, void *data )
{
	grs_bitmap *new;

	new = (grs_bitmap *)malloc( sizeof(grs_bitmap) );
	new->bm_x = 0;
	new->bm_y = 0;
	new->bm_w = w;
	new->bm_h = h;
	new->bm_flags = 0;
    new->bm_type = type;
    switch(type)
    {
        case BM_LINEAR:     new->bm_rowsize = w;            break;
        case BM_LINEAR15:   new->bm_rowsize = w*PA_BPP;     break;
        default: Int3();    // unsupported type.
    }
    if(data)
        new->bm_data = data;
    else
        new->bm_data = malloc(new->bm_rowsize * new->bm_h);
	new->bm_handle = 0;

	return new;
}
#endif

void gr_init_bitmap( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline, unsigned char * data ) // TODO: virtualize
{
#ifdef D1XD3D
	Assert (bm->iMagic != BM_MAGIC_NUMBER || bm->pvSurface == NULL);
#endif

	bm->bm_x = x;
	bm->bm_y = y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = 0;
	bm->bm_type = mode;
	bm->bm_rowsize = bytesperline;

	bm->bm_data = NULL;
#ifdef D1XD3D
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = NULL;
#endif

#ifdef D1XD3D
	Win32_CreateTexture (bm);
#endif
#ifdef OGL
	bm->bm_parent=NULL;bm->gltexture=NULL;
#endif

//	if (data != 0)
		gr_set_bitmap_data (bm, data);
/*
	else
		gr_set_bitmap_data (bm, d_malloc (w * h));
*/

#ifdef BITMAP_SELECTOR
	bm->bm_selector = 0;
#endif
}

void gr_init_bitmap_alloc( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline)
{
	gr_init_bitmap(bm, mode, x, y, w, h, bytesperline, 0);
	gr_set_bitmap_data(bm, d_malloc(w * h));
}

void gr_init_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
	bm->bm_data = NULL;
#ifdef D1XD3D
	Assert (bm->iMagic != BM_MAGIC_NUMBER);
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = NULL;
#endif
#ifdef OGL
//	ogl_freebmtexture(bm);//not what we want here.
	bm->bm_parent=NULL;bm->gltexture=NULL;
#endif
}

grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm, int x, int y, int w, int h )
{
    grs_bitmap *new;

    new = (grs_bitmap *)d_malloc( sizeof(grs_bitmap) );
	gr_init_sub_bitmap (new, bm, x, y, w, h);

	return new;
}

void gr_free_bitmap(grs_bitmap *bm )
{
	gr_free_bitmap_data (bm);
	if (bm!=NULL)
		d_free(bm);
}

void gr_free_sub_bitmap(grs_bitmap *bm )
{
	if (bm!=NULL)
	{
#ifdef D1XD3D
		bm->iMagic = 0;
#endif
		d_free(bm);
	}
}


void gr_free_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
#ifdef D1XD3D
	Assert (bm->iMagic == BM_MAGIC_NUMBER);

	Win32_FreeTexture (bm);
	bm->iMagic = 0;
	if (bm->bm_data == BM_D3D_RENDER)
		bm->bm_data = NULL;
#endif
#ifdef OGL
	ogl_freebmtexture(bm);
#endif
	if (bm->bm_data != NULL)
		d_free (bm->bm_data);
	bm->bm_data = NULL;
}

void gr_init_sub_bitmap (grs_bitmap *bm, grs_bitmap *bmParent, int x, int y, int w, int h )	// TODO: virtualize
{
	bm->bm_x = x + bmParent->bm_x;
	bm->bm_y = y + bmParent->bm_y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = bmParent->bm_flags;
	bm->bm_type = bmParent->bm_type;
	bm->bm_rowsize = bmParent->bm_rowsize;

#ifdef OGL
	bm->gltexture=bmParent->gltexture;
	bm->bm_parent=bmParent;
#endif
#ifdef D1XD3D
	Assert (bmParent->iMagic == BM_MAGIC_NUMBER);
	bm->iMagic = BM_MAGIC_NUMBER;
	bm->pvSurface = bmParent->pvSurface;
	if (bm->bm_type == BM_DIRECTX)
	{
		bm->bm_data = bmParent->bm_data;
	}
	else
#endif
	{
		bm->bm_data = bmParent->bm_data+(unsigned int)((y*bmParent->bm_rowsize)+x);
	}

}

void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count );

#if !defined(NO_ASM) && defined(__WATCOMC__)

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

#elif !defined(NO_ASM) && defined(__GNUC__)

inline void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count ) {
	int dummy[4];
   __asm__ __volatile__ (
    "xorl   %%eax,%%eax;"
"0:;"
    "movb   (%%esi), %%al;"
    "incl   (%%ebx, %%eax, 4);"
    "movb   (%%edi, %%eax), %%al;"
    "movb   %%al, (%%esi);"
    "incl   %%esi;"
    "decl   %%ecx;"
    "jne    0b"
    : "=S" (dummy[0]), "=c" (dummy[1]), "=D" (dummy[2]), "=b" (dummy[3])
	: "0" (data), "1" (num_pixels), "2" (colormap), "3" (count)
	: "%eax");
}

#elif !defined(NO_ASM) && defined(_MSC_VER)

__inline void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count )
{
  __asm {
	mov esi,[data]
	mov ecx,[num_pixels]
	mov edi,[colormap]
	mov ebx,[count]
again_ddn:
	xor eax,eax
	mov al,[esi]
	inc dword ptr [ebx+eax*4]
	mov al,[edi+eax]
	mov [esi],al
	inc esi
	dec ecx
	jne again_ddn
  }
}

#else // NO_ASM or unknown compiler

void decode_data_asm(ubyte *data, int num_pixels, ubyte *colormap, int *count)
{
	int i;
	ubyte mapped;

	for (i = 0; i < num_pixels; i++) {
		count[*data]++;
		mapped = *data;
		*data = colormap[mapped];
		data++;
	}
}

#endif

void gr_set_bitmap_flags (grs_bitmap *pbm, int flags)
{
#ifdef D1XD3D
	Assert (pbm->iMagic == BM_MAGIC_NUMBER);

	if (pbm->pvSurface)
	{
		if ((flags & BM_FLAG_TRANSPARENT) != (pbm->bm_flags & BM_FLAG_TRANSPARENT))
		{
			Win32_SetTransparent (pbm->pvSurface, flags & BM_FLAG_TRANSPARENT);
		}
	}
#endif
	pbm->bm_flags = flags;
}

void gr_set_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_TRANSPARENT);
	}
}

void gr_set_super_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_SUPER_TRANSPARENT);
	}
}

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq )
{
	int i, r, g, b;

	for (i=0; i<256; i++ )	{
		r = *palette++;
		g = *palette++;
		b = *palette++;
 		*colormap++ = gr_find_closest_color( r, g, b );
		*freq++ = 0;
	}
}

void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];

	if (bmp->bm_type != BM_LINEAR)
		return;	 //can't do it

	// This should be build_colormap_asm, but we're not using invert table, so...
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 0);
}

void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	if (bmp->bm_w == bmp->bm_rowsize)
		decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );
	else {
		int y;
		ubyte *p = bmp->bm_data;
		for (y=0;y<bmp->bm_h;y++,p+=bmp->bm_rowsize)
			decode_data_asm(p, bmp->bm_w, colormap, freq );
	}

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 1);
}

#ifdef BITMAP_SELECTOR
int gr_bitmap_assign_selector( grs_bitmap * bmp )
{
	if (!dpmi_allocate_selector( bmp->bm_data, bmp->bm_w*bmp->bm_h, &bmp->bm_selector )) {
		bmp->bm_selector = 0;
		return 1;
	}
	return 0;
}
#endif

void gr_bitmap_check_transparency( grs_bitmap * bmp )
{
	int x, y;
	ubyte * data;

	data = bmp->bm_data;

	for (y=0; y<bmp->bm_h; y++ )	{
		for (x=0; x<bmp->bm_w; x++ )	{
			if (*data++ == TRANSPARENCY_COLOR )	{
				gr_set_transparent (bmp, 1);
				return;
			}
		}
		data += bmp->bm_rowsize - bmp->bm_w;
	}

	bmp->bm_flags = 0;

}
