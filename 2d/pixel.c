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
 * Graphical routines for setting a pixel.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "vesa.h"
#include "modex.h"
#ifdef OGL
#include "ogl_init.h"
#endif


#ifndef D1XD3D
void gr_upixel( int x, int y )
{
	switch (TYPE)
	{
#ifdef OGL
		case BM_OGL:
			ogl_upixelc(x,y,COLOR);
			return;
#endif
	case BM_LINEAR:
		DATA[ ROWSIZE*y+x ] = COLOR;
		return;
#ifdef __ENV_DJGPP__
        case BM_MODEX:
		gr_modex_setplane( (x+XOFFSET) & 3 );
		gr_video_memory[(ROWSIZE * (y+YOFFSET)) + ((x+XOFFSET)>>2)] = COLOR;
		return;
	case BM_SVGA:
		gr_vesa_pixel( COLOR, (unsigned int)DATA + (unsigned int)ROWSIZE * y + x);
		return;
#endif
        }
}
#endif

void gr_pixel( int x, int y )
{
	if ((x<0) || (y<0) || (x>=GWIDTH) || (y>=GHEIGHT)) return;
	gr_upixel (x, y);
}

#ifndef D1XD3D
inline void gr_bm_upixel( grs_bitmap * bm, int x, int y, unsigned char color )
{
	switch (bm->bm_type)
	{
#ifdef OGL
		case BM_OGL:
			ogl_upixelc(bm->bm_x+x,bm->bm_y+y,color);
			return;
#endif
	case BM_LINEAR:
		bm->bm_data[ bm->bm_rowsize*y+x ] = color;
		return;
#ifdef __ENV_DJGPP__
        case BM_MODEX:
		x += bm->bm_x;
		y += bm->bm_y;
		gr_modex_setplane( x & 3 );
		gr_video_memory[(bm->bm_rowsize * y) + (x/4)] = color;
		return;
	case BM_SVGA:
		gr_vesa_pixel(color,(unsigned int)bm->bm_data + (unsigned int)bm->bm_rowsize * y + x);
		return;
#endif
        }
}
#endif

void gr_bm_pixel( grs_bitmap * bm, int x, int y, unsigned char color )
{
	if ((x<0) || (y<0) || (x>=bm->bm_w) || (y>=bm->bm_h)) return;
	gr_bm_upixel (bm, x, y, color);
}


