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


#pragma off (unreferenced)
static char rcsid[] = "$Id: barbox.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#include <stdio.h>
#include <stdarg.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "key.h"


static UI_WINDOW * wnd = NULL;
static int bar_width, bar_height, bar_x, bar_y, bar_maxlength;

void ui_barbox_open( char * text, int length )
{
	grs_font * temp_font;
	short text_width, text_height, avg;
	int w,h, width, height, xc, yc, x, y;

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	width = w/3;

	bar_maxlength = length;

	if ( w < 640 ) 	{
		temp_font = grd_curscreen->sc_canvas.cv_font;
		grd_curscreen->sc_canvas.cv_font = ui_small_font;
	}

	gr_get_string_size(text, &text_width, &text_height, &avg );
	
	text_width += avg*6;
	text_width += 10;

	if (text_width > width )
		width = text_width;

	height = text_height;
	height += 30;

	xc = w/2;
	yc = h/2;

	x = xc - width/2;
	y = yc - height/2;

	if (x < 0 ) x = 0;
	if ( (x+width-1) >= w ) x = w - width;
	if (y < 0 ) y = 0;
	if ( (y+height-1) >= h ) y = h - height;

	wnd = ui_open_window( x, y, width, height, WIN_DIALOG );

	y = 5 + text_height/2 ;

	ui_string_centered( width/2, y, text );

	y = 10 + text_height;
	
	bar_width = width - 15;
	bar_height = 10;
	bar_x = (width/2) - (bar_width/2);
	bar_y = height - 15;

	ui_draw_line_in( bar_x-2, bar_y-2, bar_x+bar_width+1, bar_y+bar_height+1 );

	grd_curscreen->sc_canvas.cv_font = temp_font;
		
}

int ui_barbox_update( int position )
{
	int spos;

	spos =  (position * bar_width)/bar_maxlength;

	gr_set_current_canvas( wnd->canvas );

	ui_mouse_hide();
	gr_setcolor( BM_XRGB(0,0,10));
	gr_rect( bar_x, bar_y, bar_x+spos-1, bar_y+bar_height-1 );
	ui_mouse_show();
	ui_mouse_process();

}

void ui_barbox_close()
{
	if (wnd)
		ui_close_window(wnd);
}
