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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#ifdef __DJGPP__
#include "modex.h"
#include "vesa.h"
#endif

grs_canvas * grd_curcanv;    //active canvas
grs_screen * grd_curscreen;  //active screen

grs_canvas *gr_create_canvas(int w, int h)
{
	grs_canvas *new;
	
	new = (grs_canvas *)d_malloc( sizeof(grs_canvas) );
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

	new = (grs_canvas *)d_malloc( sizeof(grs_canvas) );
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


#ifndef __DJGPP__
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
    d_free(canv);
}

void gr_free_sub_canvas(grs_canvas *canv)
{
    d_free(canv);
}

int gr_wait_for_retrace = 1;

void gr_show_canvas( grs_canvas *canv )
{
#ifdef __DJGPP__
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

