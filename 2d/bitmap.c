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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/bitmap.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:38:51 $
 *
 * Graphical routines for manipulating grs_bitmaps.
 *
 * $Log: bitmap.c,v $
 * Revision 1.1.1.1  2006/03/17 19:38:51  zicodxx
 * initial import
 *
 * Revision 1.7  1999/10/08 08:50:54  donut
 * fixed ogl sub bitmap support
 *
 * Revision 1.6  1999/10/07 20:48:17  donut
 * ogl fix for sub bitmaps and segv
 *
 * Revision 1.5  1999/10/07 02:27:14  donut
 * OGL includes to remove warnings
 *
 * Revision 1.4  1999/09/22 02:02:31  donut
 * ogl: gr_set_bitmap_data frees the texture rather than just resetting the gltexture flag
 *
 * Revision 1.3  1999/09/21 04:05:54  donut
 * mostly complete OGL implementation (still needs bitmap handling (reticle), and door/fan textures are corrupt)
 *
 * Revision 1.2  1999/08/05 22:53:40  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.1.1.1  1999/06/14 21:57:15  donut
 * Import of d1x 1.37 source.
 *
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
 *
 */

#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "bitmap.h"
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
#endif

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );

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
		gr_set_bitmap_data (bm, malloc (w * h));
*/

#ifdef BITMAP_SELECTOR
	bm->bm_selector = 0;
#endif
}

void gr_init_bitmap_alloc( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline)
{
	gr_init_bitmap(bm, mode, x, y, w, h, bytesperline, 0);
	gr_set_bitmap_data(bm, malloc(MAX_BMP_SIZE(w, h)));
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

void gr_free_bitmap(grs_bitmap *bm )
{
	gr_free_bitmap_data (bm);
	if (bm!=NULL)
		free(bm);
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
		free (bm->bm_data);
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

void gr_free_sub_bitmap(grs_bitmap *bm )
{
	if (bm!=NULL)
	{
#ifdef D1XD3D
		bm->iMagic = 0;
#endif
		free(bm);
	}
}


grs_bitmap *gr_create_bitmap(int w, int h )
{
	return gr_create_bitmap_raw (w, h, malloc(MAX_BMP_SIZE(w, h)));
}

grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data )
{
    grs_bitmap *new;

    new = (grs_bitmap *)malloc( sizeof(grs_bitmap) );
	gr_init_bitmap (new, 0, 0, 0, w, h, w, raw_data);

    return new;
}


grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm, int x, int y, int w, int h )
{
    grs_bitmap *new;

    new = (grs_bitmap *)malloc( sizeof(grs_bitmap) );
	gr_init_sub_bitmap (new, bm, x, y, w, h);

	return new;
}

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

void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];

	// This should be build_colormap_asm, but we're not using invert table, so...
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = 255;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 0);
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


void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];
   
	build_colormap_good( palette, colormap, freq );

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = 255;

	decode_data_asm(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );

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
			if (*data++ == 255 )	{
				gr_set_transparent (bmp, 1);
				return;
			}
		}
		data += bmp->bm_rowsize - bmp->bm_w;
	}

	bmp->bm_flags = 0;

}
