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


#include <stdio.h>
#include <stdarg.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"
#include "u_mem.h"

// ts = total span
// w = width of each item
// n = number of items
// i = item number (0 based)
#define EVEN_DIVIDE(ts,w,n,i) ((((ts)-((w)*(n)))*((i)+1))/((n)+1))+((w)*(i))

#define BUTTON_HORZ_SPACING 20
#define TEXT_EXTRA_HEIGHT 5

typedef struct messagebox
{
	char				**button;
	UI_GADGET_BUTTON	*button_g[10];
	char				*text;
	int					*choice;
	int					num_buttons;
	int					width;
	int					text_y;
	int					line_y;
} messagebox;

static int messagebox_handler(UI_DIALOG *dlg, d_event *event, messagebox *m)
{
	int i;
	
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		grs_font * temp_font;

		gr_set_current_canvas( &grd_curscreen->sc_canvas );
		temp_font = grd_curscreen->sc_canvas.cv_font;
		
		if ( grd_curscreen->sc_w < 640 ) 	{
			grd_curscreen->sc_canvas.cv_font = ui_small_font;
		}
		
		ui_dialog_set_current_canvas(dlg);
		ui_string_centered( m->width/2, m->text_y, m->text );
		
		gr_setcolor( CGREY );
		Hline(1, m->width-2, m->line_y+1 );
		
		gr_setcolor( CBRIGHT );
		Hline(2, m->width-2, m->line_y+2 );

		grd_curscreen->sc_canvas.cv_font = temp_font;
		
		return 1;
	}

	for (i=0; i<m->num_buttons; i++ )
	{
		if (GADGET_PRESSED(m->button_g[i]))
		{
			*(m->choice) = i+1;
			return 1;
		}
	}
	
	return 0;
}

int ui_messagebox_n( short xc, short yc, int NumButtons, char * text, char * Button[] )
{
	UI_DIALOG * dlg;
	messagebox *m;

	int i, width, height, avg, x, y;
	int button_width, button_height, text_height, text_width;
	int w, h;

	int choice;

	if ((NumButtons < 1) || (NumButtons>10)) return -1;

	MALLOC(m, messagebox, 1);
	m->button = Button;
	m->text = text;
	m->choice = &choice;
	m->num_buttons = NumButtons;

	button_width = button_height = 0;

	gr_set_current_canvas( &grd_curscreen->sc_canvas );
	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

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

	{
		int mx, my, mz;
		
		mouse_get_pos(&mx, &my, &mz);

		if ( xc == -1 )
			xc = mx;

		if ( yc == -1 )
			yc = my - button_height/2;
	}

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

	dlg = ui_create_dialog( x, y, width, height, DF_DIALOG | DF_MODAL, (int (*)(UI_DIALOG *, d_event *, void *))messagebox_handler, m );

	//ui_draw_line_in( MESSAGEBOX_BORDER, MESSAGEBOX_BORDER, width-MESSAGEBOX_BORDER, height-MESSAGEBOX_BORDER );

	y = TEXT_EXTRA_HEIGHT + text_height/2 - 1;

	m->width = width;
	m->text_y = y;

	y = 2*TEXT_EXTRA_HEIGHT + text_height;
	
	m->line_y = y;

	y = height - TEXT_EXTRA_HEIGHT - button_height;

	for (i=0; i<NumButtons; i++ )
	{

		x = EVEN_DIVIDE(width,button_width,NumButtons,i);

		m->button_g[i] = ui_add_gadget_button( dlg, x, y, button_width, button_height, Button[i], NULL );
	}

	ui_gadget_calc_keys(dlg);

	//key_flush();

	dlg->keyboard_focus_gadget = (UI_GADGET *)m->button_g[0];

	choice = 0;

	while(choice==0)
		event_process();

	ui_close_dialog(dlg);

	return choice;
}


int ui_messagebox( short xc, short yc, int NumButtons, char * text, ... )
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


	return ui_messagebox_n( xc, yc, NumButtons, text, Button );
	
}
