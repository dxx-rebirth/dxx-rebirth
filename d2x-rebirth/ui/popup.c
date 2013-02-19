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


#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "ui.h"
#include "u_mem.h"
#include "mouse.h"


#define MENU_BORDER 2
#define MENU_VERT_SPACING 2

typedef struct popup
{
	UI_GADGET_BUTTON *button_g[10];
	int *choice;
	int num_buttons;
} popup;

static int popup_handler(UI_DIALOG *dlg, d_event *event, popup *p)
{
	short i;
	
	for (i=0; i<p->num_buttons; i++ )
	{
		if (GADGET_PRESSED(p->button_g[i]))
		{
			*(p->choice) = i+1;
			return 1;
		}
	}
	
	if ( (*(p->choice)==0) && B1_JUST_RELEASED )
	{
		*(p->choice) = -1;
		return 1;
	}
	
	return 0;
}

// Use: Left button (button 0) goes down, then up, then this is called
// as opposed to straight after the button goes down, holding down
// until an option is chosen.
// This is like the 'modern' behaviour of popup menus and also happens
// to be easier to implement because of the button code.
// Recommended for use in conjunction with a button gadget,
// i.e. when that button is pressed, call PopupMenu,
// update the button's text then the value in question.
int PopupMenu( int NumButtons, char * text[] )
{
	UI_DIALOG * dlg;
	popup	*p;

	char * Button[10];

	int button_width, button_height, width, height;

	short i, x, y;
	short w, h;

	int choice;

	MALLOC(p, popup, 1);
	
	p->num_buttons = NumButtons;
	p->choice = &choice;

	if ((NumButtons < 1) || (NumButtons>10))
	{
		d_free(p);
		return -1;
	}

	button_width = button_height = 0;

	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	for (i=0; i<NumButtons; i++ )
	{
		Button[i] = text[i];

		ui_get_button_size( Button[i], &width, &height );

		if ( width > button_width ) button_width = width;
		if ( height > button_height ) button_height = height;
	}

	width = button_width + 2*(MENU_BORDER+3);

	height = (button_height*NumButtons) + (MENU_VERT_SPACING*(NumButtons-1)) ;
	height += (MENU_BORDER+3) * 2;

	{
		int mx, my, mz;
		
		mouse_get_pos(&mx, &my, &mz);
		x = mx - width/2;
		y = my - (MENU_BORDER+3) - button_height/2;
	}

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	if (x < 0 ) {
		x = 0;
		//Mouse.x = x + width / 2;
	}

	if ( (x+width-1) >= w ) {
		x = w - width;
		//Mouse.x = x + width / 2;
	}

	if (y < 0 ) {
		y = 0;
		//Mouse.y = y + (MENU_BORDER+3) + button_height/2;
	}

	if ( (y+height-1) >= h ) {
		y = h - height;
		//Mouse.y = y + (MENU_BORDER+3) + button_height/2;
	}

	dlg = ui_create_dialog( x, y, width, height, DF_DIALOG | DF_MODAL, (int (*)(UI_DIALOG *, d_event *, void *))popup_handler, p );

	//mouse_set_pos(Mouse.x, Mouse.y);

	x = MENU_BORDER+3;
	y = MENU_BORDER+3;

	for (i=0; i<NumButtons; i++ )
	{
		p->button_g[i] = ui_add_gadget_button( dlg, x, y, button_width, button_height, Button[i], NULL );
		y += button_height+MENU_VERT_SPACING;
	}

	choice = 0;

	while(choice==0)
		event_process();

	ui_close_dialog(dlg);
	d_free(p);

	return choice;
}

