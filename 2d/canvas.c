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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/canvas.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:38:57 $
 *
 * Graphical routines for manipulating grs_canvas's.
 *
 * $Log: canvas.c,v $
 * Revision 1.1.1.1  2006/03/17 19:38:57  zicodxx
 * initial import
 *
 * Revision 1.2  1999/08/05 22:53:40  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.1.1.1  1999/06/14 21:57:17  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.14  1995/03/06  09:18:45  john
 * Made modex page flipping wait for retrace be default.
 * 
 * Revision 1.13  1995/03/01  15:37:40  john
 * Better ModeX support.
 * 
 * Revision 1.12  1994/11/28  17:08:29  john
 * Took out some unused functions in linear.asm, moved
 * gr_linear_movsd from linear.asm to bitblt.c, made sure that
 * the code in ibiblt.c sets the direction flags before rep movsing.
 * 
 * Revision 1.11  1994/11/18  22:50:24  john
 * Changed shorts to ints in parameters.
 * 
 * Revision 1.10  1994/11/10  15:59:33  john
 * Fixed bugs with canvas's being created with bogus bm_flags.
 * 
 * Revision 1.9  1994/06/24  17:26:34  john
 * Made rowsizes bigger than actual screen work with SVGA.
 * 
 * Revision 1.8  1994/05/06  12:50:41  john
 * Added supertransparency; neatend things up; took out warnings.
 * 
 * Revision 1.7  1993/12/08  16:41:26  john
 * fixed color = -1 bug
 * 
 * Revision 1.6  1993/10/15  16:22:25  john
 * *** empty log message ***
 * 
 * Revision 1.5  1993/09/29  16:14:07  john
 * added globol variables describing current canvas
 * 
 * Revision 1.4  1993/09/14  16:03:40  matt
 * Added new function, gr_clear_canvas()
 * 
 * Revision 1.3  1993/09/14  13:51:38  matt
 * in gr_init_sub_canvas(), copy bm_rowsize from source canvas
 * 
 * Revision 1.2  1993/09/08  17:37:34  john
 * Checking for potential errors
 * 
 * Revision 1.1  1993/09/08  11:43:18  john
 * Initial revision
 * 
 *
 */

#include <stdlib.h>
//#include <malloc.h>
#include <stdio.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#ifdef __MSDOS__
#include "modex.h"
#include "vesa.h"
#endif

grs_canvas * grd_curcanv;    //active canvas
grs_screen * grd_curscreen;  //active screen

grs_canvas *gr_create_canvas(int w, int h)
{
	grs_canvas *new;
	
	new = (grs_canvas *)malloc( sizeof(grs_canvas) );
	gr_init_bitmap_alloc (&new->cv_bitmap, BM_LINEAR, 0, 0, w, h, w);

	new->cv_color = 0;
	new->cv_drawmode = 0;
	new->cv_font = NULL;
	new->cv_font_fg_color = 0;
	new->cv_font_bg_color = 0;
	return new;
}

grs_canvas *gr_create_sub_canvas(grs_canvas *canv, int x, int y, int w, int h)
{
    grs_canvas *new;

	new = (grs_canvas *)malloc( sizeof(grs_canvas) );
	gr_init_sub_bitmap (&new->cv_bitmap, &canv->cv_bitmap, x, y, w, h);

	new->cv_color = canv->cv_color;
	new->cv_drawmode = canv->cv_drawmode;
	new->cv_font = canv->cv_font;
	new->cv_font_fg_color = canv->cv_font_fg_color;
	new->cv_font_bg_color = canv->cv_font_bg_color;
	return new;
}

void gr_init_canvas(grs_canvas *canv, unsigned char * pixdata, int pixtype, int w, int h)
{
	int wreal;
    canv->cv_color = 0;
    canv->cv_drawmode = 0;
    canv->cv_font = NULL;
	canv->cv_font_fg_color = 0;
	canv->cv_font_bg_color = 0;


#ifndef __MSDOS__
	wreal = w;
#else
	wreal = (pixtype == BM_MODEX) ? w / 4 : w;
#endif
	gr_init_bitmap (&canv->cv_bitmap, pixtype, 0, 0, w, h, wreal, pixdata);
}

void gr_init_sub_canvas(grs_canvas *new, grs_canvas *src, int x, int y, int w, int h)
{
	new->cv_color = src->cv_color;
	new->cv_drawmode = src->cv_drawmode;
	new->cv_font = src->cv_font;
	new->cv_font_fg_color = src->cv_font_fg_color;
	new->cv_font_bg_color = src->cv_font_bg_color;

	gr_init_sub_bitmap (&new->cv_bitmap, &src->cv_bitmap, x, y, w, h);
}

void gr_free_canvas(grs_canvas *canv)
{
	gr_free_bitmap_data(&canv->cv_bitmap);
    free(canv);
}

void gr_free_sub_canvas(grs_canvas *canv)
{
    free(canv);
}

int gr_wait_for_retrace = 1;

void gr_show_canvas( grs_canvas *canv )
{
#ifdef __MSDOS__
	if (canv->cv_bitmap.bm_type == BM_MODEX )
		gr_modex_setstart( canv->cv_bitmap.bm_x, canv->cv_bitmap.bm_y, gr_wait_for_retrace );

	else if (canv->cv_bitmap.bm_type == BM_SVGA )
		gr_vesa_setstart( canv->cv_bitmap.bm_x, canv->cv_bitmap.bm_y );
#endif
		//	else if (canv->cv_bitmap.bm_type == BM_LINEAR )
		// Int3();		// Get JOHN!
		//gr_linear_movsd( canv->cv_bitmap.bm_data, (void *)gr_video_memory, 320*200);
}

void gr_set_current_canvas( grs_canvas *canv )
{
	if (canv==NULL)
		grd_curcanv = &(grd_curscreen->sc_canvas);
	else
		grd_curcanv = canv;
#ifndef NO_ASM
	if ( (grd_curcanv->cv_color >= 0) && (grd_curcanv->cv_color <= 255) )	{
		gr_var_color = grd_curcanv->cv_color;
	} else
		gr_var_color  = 0;
	gr_var_bitmap = grd_curcanv->cv_bitmap.bm_data;
	gr_var_bwidth = grd_curcanv->cv_bitmap.bm_rowsize;
#endif
}

void gr_clear_canvas(int color)
{
	gr_setcolor(color);
	gr_rect(0,0,GWIDTH-1,GHEIGHT-1);
}

void gr_setcolor(int color)
{
	grd_curcanv->cv_color=color;
#ifndef NO_ASM
	gr_var_color = color;
#endif
}

