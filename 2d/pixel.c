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

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#ifdef OGL
#include "ogl_init.h"
#endif


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
	}
}

void gr_pixel( int x, int y )
{
	if ((x<0) || (y<0) || (x>=GWIDTH) || (y>=GHEIGHT)) return;
	gr_upixel (x, y);
}

static inline void gr_bm_upixel( grs_bitmap * bm, int x, int y, unsigned char color )
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
	}
}

void gr_bm_pixel( grs_bitmap * bm, int x, int y, unsigned char color )
{
	if ((x<0) || (y<0) || (x>=bm->bm_w) || (y>=bm->bm_h)) return;
	gr_bm_upixel (bm, x, y, color);
}
