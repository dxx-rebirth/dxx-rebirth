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

#ifdef RCS
static char rcsid[] = "$Id: message.c,v 1.2 2004-12-19 14:10:33 btb Exp $";
#endif
#include <stdio.h>
#include <stdarg.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "key.h"

// ts = total span
// w = width of each item
// n = number of items
// i = item number (0 based)
#define EVEN_DIVIDE(ts,w,n,i) ((((ts)-((w)*(n)))*((i)+1))/((n)+1))+((w)*(i))

#define BUTTON_HORZ_SPACING 20
#define TEXT_EXTRA_HEIGHT 5

int MessageBoxN( short xc, short yc, int NumButtons, char * text, char * Button[] )
{
	grs_font * temp_font;
	UI_WINDOW * wnd;
	UI_GADGET_BUTTON * ButtonG[10];

	int i, width, height, avg, x, y;
	int button_width, button_height, text_height, text_width;
	int w, h;

	int choice;

	if ((NumButtons < 1) || (NumButtons>10)) return -1;

	button_width = button_height = 0;

	gr_set_current_canvas( &grd_curscreen->sc_canvas );
	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;
	temp_font = grd_curscreen->sc_canvas.cv_font;

	if ( w < 640 ) 	{
		grd_curscreen->sc_canvas.cv_font = ui_small_font;
	}

	for (i=0; i<NumButtons; i++ )
	{
		ui_get_button_size( Button[i], &width, &height );

		if ( width > button_width ) button_width = width;
		if ( height > button_height ) button_height = height;
	}

	gr_get_string_size(text, &text_width, &text_height, &avg );

	width = button_width*NumButtons;
	width += BUTTON_HORZ_SPACING*(NumButtons+1);
	width ++;

	text_width += avg*6;
	text_width += 10;

	if (text_width > width )
		width = text_width;

	height = text_height;
	height += button_height;
	height += 4*TEXT_EXTRA_HEIGHT;
	height += 2;  // For line in middle

	if ( xc == -1 )
		xc = Mouse.x;

	if ( yc == -1 )
		yc = Mouse.y - button_height/2;

	if ( xc == -2 )
		xc = w/2;

	if ( yc == -2 )
		yc = h/2;

	x = xc - width/2;
	y = yc - height/2;

	if (x < 0 ) {
		x = 0;
	}

	if ( (x+width-1) >= w ) {
		x = w - width;
	}

	if (y < 0 ) {
		y = 0;
	}

	if ( (y+height-1) >= h ) {
		y = h - height;
	}

	wnd = ui_open_window( x, y, width, height, WIN_DIALOG );

	//ui_draw_line_in( MESSAGEBOX_BORDER, MESSAGEBOX_BORDER, width-MESSAGEBOX_BORDER, height-MESSAGEBOX_BORDER );

	y = TEXT_EXTRA_HEIGHT + text_height/2 - 1;

	ui_string_centered( width/2, y, text );

	y = 2*TEXT_EXTRA_HEIGHT + text_height;

	gr_setcolor( CGREY );
	Hline(1, width-2, y+1 );

	gr_setcolor( CBRIGHT );
	Hline(2, width-2, y+2 );

	y = height - TEXT_EXTRA_HEIGHT - button_height;

	for (i=0; i<NumButtons; i++ )
	{

		x = EVEN_DIVIDE(width,button_width,NumButtons,i);

		ButtonG[i] = ui_add_gadget_button( wnd, x, y, button_width, button_height, Button[i], NULL );
	}

	ui_gadget_calc_keys(wnd);

	//key_flush();

	wnd->keyboard_focus_gadget = (UI_GADGET *)ButtonG[0];

	choice = 0;

	while(choice==0)
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		for (i=0; i<NumButtons; i++ )
		{
			if (ButtonG[i]->pressed)   {
				choice = i+1;
				break;
			}
		}

	}

	ui_close_window(wnd);
	
	grd_curscreen->sc_canvas.cv_font = temp_font;

	return choice;

}


int MessageBox( short xc, short yc, int NumButtons, char * text, ... )
{
	va_list marker;
	char * Button[10];

	short i;

	if ((NumButtons < 1) || (NumButtons>10)) return -1;

	va_start( marker, text );
	for (i=0; i<NumButtons; i++ )
	{
		Button[i] = va_arg( marker, char * );

	}
	va_end( marker );


	return MessageBoxN( xc, yc, NumButtons, text, Button );
	
}
