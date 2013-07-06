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


#include <stdlib.h>

#include "u_mem.h"
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "mouse.h"
#include "ui.h"


#define MENU_BORDER 2
#define MENU_VERT_SPACING 2

typedef struct menu
{
	UI_GADGET_BUTTON ** button_g;
	char ** button;
	int *choice;
	int num_buttons;
} menu;

static int menu_handler(UI_DIALOG *dlg, d_event *event, menu *m)
{
	int i;
	
	for (i=0; i<m->num_buttons; i++ )
	{
		if (GADGET_PRESSED(m->button_g[i]))
		{
			*(m->choice) = i+1;
			return 1;
		}
	}
	
	if ( (*(m->choice)==0) && B1_JUST_RELEASED )
	{
		*(m->choice) = -1;
		return 1;
	}
	
	return 0;
}

int MenuX( int x, int y, int NumButtons, char * text[] )
{
	UI_DIALOG * dlg;
	menu *m;
	int button_width, button_height, width, height;
	int i;
	int w, h;
	int choice;

	MALLOC(m, menu, 1);
	m->num_buttons = NumButtons;
	m->button_g = (UI_GADGET_BUTTON **) d_malloc(sizeof(UI_GADGET_BUTTON *)*NumButtons);
	MALLOC(m->button, char *, NumButtons);
	m->choice = &choice;

	button_width = button_height = 0;

	for (i=0; i<NumButtons; i++ )
	{
		m->button[i] = text[i];

		ui_get_button_size( m->button[i], &width, &height );

		if ( width > button_width ) button_width = width;
		if ( height > button_height ) button_height = height;
	}

	width = button_width + 2*(MENU_BORDER+3);

	height = (button_height*NumButtons) + (MENU_VERT_SPACING*(NumButtons-1)) ;
	height += (MENU_BORDER+3) * 2;

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	{
		int mx, my, mz;
		
		mouse_get_pos(&mx, &my, &mz);
		if ( x == -1 ) x = mx - width/2;
		if ( y == -1 ) y = my;
	}

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

	dlg = ui_create_dialog( x, y, width, height, DF_FILLED | DF_SAVE_BG | DF_MODAL, (int (*)(UI_DIALOG *, d_event *, void *))menu_handler, m );

	x = MENU_BORDER+3;
	y = MENU_BORDER+3;

	for (i=0; i<NumButtons; i++ )
	{
		m->button_g[i] = ui_add_gadget_button( dlg, x, y, button_width, button_height, m->button[i], NULL );
		y += button_height+MENU_VERT_SPACING;
	}

	choice = 0;

	while(choice==0)
		event_process();

	ui_close_dialog(dlg);
	d_free(m->button);
	d_free(m->button_g);
	d_free(m);

	return choice;
}

