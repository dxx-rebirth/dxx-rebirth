/* $Id: number.c,v 1.4 2005-01-24 22:19:10 schaffner Exp $ */
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
static char rcsid[] = "$Id: number.c,v 1.4 2005-01-24 22:19:10 schaffner Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"

#define TEXT_EXTRA_HEIGHT 5

double ui_input_number( short xc, short yc, char * text, double OrgNumber )
{
	UI_WINDOW * wnd;
	UI_GADGET_INPUTBOX * InputBox;

	short i, width, height, avg, x, y;
	short box_width, box_height, text_height, text_width;
	short w, h;
	char string[100];

	sprintf( string, "%f", OrgNumber );

	box_width = box_height = 0;

	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	box_width = 8*20;
	box_height = 20;

	gr_get_string_size(text, &text_width, &text_height, &avg );

	width = box_width + 50;

	text_width += avg*6;
	text_width += 10;

	if (text_width > width )
		width = text_width;

	height = text_height;
	height += box_height;
	height += 4*5;

	// Center X and Y
	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	if ( xc == -1 ) xc = Mouse.x;
	if ( yc == -1 ) yc = Mouse.y - box_height/2;
	if ( xc == -2 ) xc = w/2;
	if ( yc == -2 ) yc = h/2;

	x = xc - width/2;
	y = yc - height/2;

	// Make sure that we're onscreen
	if (x < 0 ) x = 0;
	if ( (x+width-1) >= w ) x = w - width;
	if (y < 0 ) y = 0;
	if ( (y+height-1) >= h ) y = h - height;


	wnd = ui_open_window( x, y, width, height, WIN_DIALOG );

	y = TEXT_EXTRA_HEIGHT + text_height/2 - 1;

	ui_string_centered( width/2, y, text );

	y = 2*TEXT_EXTRA_HEIGHT + text_height;

	y = height - TEXT_EXTRA_HEIGHT - box_height-10;

	InputBox = ui_add_gadget_inputbox( wnd, 10, y, 20, 20, string );

	ui_gadget_calc_keys(wnd);

	//key_flush();

	wnd->keyboard_focus_gadget = (UI_GADGET *)InputBox;

	while(1)
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		if (InputBox->pressed) break;
	}

	ui_close_window(wnd);

	OrgNumber = atof(inputbox->text);

	return OrgNumber;

}


