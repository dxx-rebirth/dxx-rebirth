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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/gpixel.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:02 $
 *
 * Graphical routines for getting a pixel's value.
 *
 * $Log: gpixel.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:02  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 21:57:23  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.5  1994/11/18  22:50:20  john
 * Changed shorts to ints in parameters.
 * 
 * Revision 1.4  1994/05/06  12:50:08  john
 * Added supertransparency; neatend things up; took out warnings.
 * 
 * Revision 1.3  1993/10/15  16:22:50  john
 * y
 * 
 * Revision 1.2  1993/09/29  16:15:00  john
 * optimized
 * 
 * Revision 1.1  1993/09/08  11:43:40  john
 * Initial revision
 * 
 *
 */
#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#ifdef __MSDOS__
#include "modex.h"
#include "vesa.h"
#endif

unsigned char gr_ugpixel( grs_bitmap * bitmap, int x, int y )
{
#ifdef __MSDOS__
	switch(bitmap->bm_type)
	{
	case BM_LINEAR:
#endif
		return bitmap->bm_data[ bitmap->bm_rowsize*y + x ];
#ifdef __MSDOS__
        case BM_MODEX:
		x += bitmap->bm_x;
		y += bitmap->bm_y;
		gr_modex_setplane( x & 3 );
		return gr_video_memory[(bitmap->bm_rowsize * y) + (x/4)];
	case BM_SVGA:
		{
		unsigned int offset;
		offset = (unsigned int)bitmap->bm_data + (unsigned int)bitmap->bm_rowsize * y + x;
		gr_vesa_setpage( offset >> 16 );
		return gr_video_memory[offset & 0xFFFF];
		}
	}
	return 0;
#endif
}

unsigned char gr_gpixel( grs_bitmap * bitmap, int x, int y )
{
	if ((x<0) || (y<0) || (x>=bitmap->bm_w) || (y>=bitmap->bm_h)) return 0;
#ifdef __MSDOS__
	switch(bitmap->bm_type)
	{
	case BM_LINEAR:
#endif
		return bitmap->bm_data[ bitmap->bm_rowsize*y + x ];
#ifdef __MSDOS__
        case BM_MODEX:
		x += bitmap->bm_x;
		y += bitmap->bm_y;
		gr_modex_setplane( x & 3 );
		return gr_video_memory[(bitmap->bm_rowsize * y) + (x/4)];
	case BM_SVGA:
		{
		unsigned int offset;
		offset = (unsigned int)bitmap->bm_data + (unsigned int)bitmap->bm_rowsize * y + x;
		gr_vesa_setpage( offset >> 16 );
		return gr_video_memory[offset & 0xFFFF];
		}
	}
	return 0;
#endif
}

