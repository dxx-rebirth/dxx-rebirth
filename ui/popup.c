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
static char rcsid[] = "$Id: popup.c,v 1.1 2004-12-19 13:08:49 btb Exp $";
#pragma on (unreferenced)
#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "mouse.h"


#define MENU_BORDER 2
#define MENU_VERT_SPACING 2

extern void ui_mouse_flip_buttons();

int PopupMenu( int NumButtons, char * text[] )
{
	UI_WINDOW * wnd;
	UI_GADGET_BUTTON * ButtonG[10];

	short SavedMouseX, SavedMouseY;
	char * Button[10];

	int button_width, button_height, width, height;

	short i, x, y;
	short w, h;

	int choice;

	ui_mouse_flip_buttons();

	//ui_mouse_process();

	if ( B1_RELEASED )
	{
		ui_mouse_flip_buttons();
		return -1;
	}

	if ((NumButtons < 1) || (NumButtons>10))
	{
		ui_mouse_flip_buttons();
		return -1;
	}

	SavedMouseX = Mouse.x; SavedMouseY = Mouse.y;

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

	x = Mouse.x - width/2;
	y = Mouse.y - (MENU_BORDER+3) - button_height/2;

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	if (x < 0 ) {
		x = 0;
		Mouse.x = x + width / 2;
	}

	if ( (x+width-1) >= w ) {
		x = w - width;
		Mouse.x = x + width / 2;
	}

	if (y < 0 ) {
		y = 0;
		Mouse.y = y + (MENU_BORDER+3) + button_height/2;
	}

	if ( (y+height-1) >= h ) {
		y = h - height;
		Mouse.y = y + (MENU_BORDER+3) + button_height/2;
	}

	wnd = ui_open_window( x, y, width, height, WIN_DIALOG );

	mouse_set_pos( Mouse.x, Mouse.y );

	x = MENU_BORDER+3;
	y = MENU_BORDER+3;

	for (i=0; i<NumButtons; i++ )
	{
		ButtonG[i] = ui_add_gadget_button( wnd, x, y, button_width, button_height, Button[i], NULL );
		y += button_height+MENU_VERT_SPACING;
	}

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

		if ( (choice==0) && B1_JUST_RELEASED )  {
			choice = -1;
			break;
		}

	}

	ui_close_window(wnd);

	ui_mouse_flip_buttons();

	return choice;

}

