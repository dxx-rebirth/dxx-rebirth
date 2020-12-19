/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "mouse.h"
#include "ui.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include <memory>

namespace dcx {

#define MENU_BORDER 2
#define MENU_VERT_SPACING 2

namespace {

struct menu : UI_DIALOG
{
	explicit menu(short x, short y, short w, short h, enum dialog_flags flags, int &choice, int NumButtons) :
		UI_DIALOG(x, y, w, h, flags),
		button_g(std::make_unique<std::unique_ptr<UI_GADGET_BUTTON>[]>(NumButtons)),
		choice(&choice),
		num_buttons(NumButtons)
	{
	}
	const std::unique_ptr<std::unique_ptr<UI_GADGET_BUTTON>[]> button_g;
	int *const choice;
	const int num_buttons;
	virtual window_event_result callback_handler(const d_event &) override;
};

window_event_result menu::callback_handler(const d_event &event)
{
	for (int i=0; i < num_buttons; i++ )
	{
		if (GADGET_PRESSED(button_g[i].get()))
		{
			*(choice) = i+1;
			return window_event_result::handled;
		}
	}
	
	if ((*(choice)==0) && B1_JUST_RELEASED)
	{
		*(choice) = -1;
		return window_event_result::handled;
	}
	
	return window_event_result::ignored;
}

}

int MenuX( int x, int y, int NumButtons, const char *const text[] )
{
	int button_width, button_height;
	int w, h;
	int choice;

	button_width = button_height = 0;

	for (int i=0; i<NumButtons; i++ )
	{
		int width, height;
		ui_get_button_size(*grd_curcanv->cv_font, text[i], width, height);

		if ( width > button_width ) button_width = width;
		if ( height > button_height ) button_height = height;
	}

	unsigned width, height;
	width = button_width + 2*(MENU_BORDER+3);

	height = (button_height*NumButtons) + (MENU_VERT_SPACING*(NumButtons-1)) ;
	height += (MENU_BORDER+3) * 2;

	w = grd_curscreen->get_screen_width();
	h = grd_curscreen->get_screen_height();

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

	auto dlg = window_create<menu>(x, y, width, height, static_cast<dialog_flags>(DF_FILLED | DF_SAVE_BG | DF_MODAL), choice, NumButtons);

	x = MENU_BORDER+3;
	y = MENU_BORDER+3;

	for (int i=0; i<NumButtons; i++ )
	{
		dlg->button_g[i] = ui_add_gadget_button(*dlg, x, y, button_width, button_height, text[i], nullptr);
		y += button_height+MENU_VERT_SPACING;
	}

	choice = 0;

	while(choice==0)
		event_process();

	ui_close_dialog(*dlg);
	return choice;
}

}
