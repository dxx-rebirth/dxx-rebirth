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

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include "u_mem.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "event.h"
#include "mouse.h"
#include "window.h"
#include "dxxerror.h"

#include "key.h"

namespace dcx {

UI_GADGET * selected_gadget;

namespace {

struct event_gadget : d_event
{
	UI_GADGET &gadget;
	constexpr event_gadget(const event_type t, UI_GADGET &g) :
		d_event(t), gadget(g)
	{
	}
};

template <UI_GADGET *UI_GADGET::*step>
UI_GADGET &ui_gadget_get_step(UI_GADGET &gadget)
{
	auto tmp = gadget.*step;
	while (tmp != &gadget && tmp->parent)
		tmp = tmp->*step;
	return *tmp;
}

UI_GADGET &ui_gadget_get_prev(UI_GADGET &gadget)
{
	return ui_gadget_get_step<&UI_GADGET::prev>(gadget);
}

}

void ui_gadget_add(UI_DIALOG &dlg, short x1, short y1, short x2, short y2, UI_GADGET &gadget)
{
	if (dlg.gadget == NULL)
	{
		dlg.gadget = &gadget;
		gadget.prev = &gadget;
		gadget.next = &gadget;
	} else {
		dlg.gadget->prev->next = &gadget;
		gadget.next = dlg.gadget;
		gadget.prev = dlg.gadget->prev;
		dlg.gadget->prev = &gadget;
	}

	gadget.when_tab = nullptr;
	gadget.when_btab = nullptr;
	gadget.when_up = nullptr;
	gadget.when_down = nullptr;
	gadget.when_left = nullptr;
	gadget.when_right = nullptr;
	gadget.status = 1;
	gadget.oldstatus = 0;
	if (x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
		gadget.canvas.reset();
	else
		gadget.canvas = gr_create_sub_canvas(dlg.w_canv, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	gadget.x1 = gadget.canvas->cv_bitmap.bm_x;
	gadget.y1 = gadget.canvas->cv_bitmap.bm_y;
	gadget.x2 = gadget.canvas->cv_bitmap.bm_x + x2 - x1 + 1;
	gadget.y2 = gadget.canvas->cv_bitmap.bm_y + y2 - y1 + 1;
	gadget.parent = nullptr;
	gadget.hotkey = -1;
}


#if 0
int is_under_another_window( UI_DIALOG * dlg, UI_GADGET * gadget )
{
	UI_DIALOG * temp;

	temp = dlg->next;

	while( temp != NULL )	{
		if (	( gadget->x1 > temp->x)						&&
				( gadget->x1 < (temp->x+temp->width) )	&&
				( gadget->y1 > temp->y)						&& 
				( gadget->y1 < (temp->y+temp->height) )
			)	
		{
				//gadget->status =1;
				return 1;
		}
		

		if (	( gadget->x2 > temp->x)						&&
				( gadget->x2 < (temp->x+temp->width) )	&&
				( gadget->y2 > temp->y)						&& 
				( gadget->y2 < (temp->y+temp->height) )
			)
		{
				//gadget->status =1;
				return 1;
		}
		

		temp = temp->next;
	}
	return 0;
}
#endif

int ui_mouse_on_gadget(UI_GADGET &gadget)
{
	int x, y, z;
	
	mouse_get_pos(&x, &y, &z);
	if (x >= gadget.x1 && x <= gadget.x2 - 1 && y >= gadget.y1 && y <= gadget.y2 - 1)
	{
#if 0	// check is no longer required - if it is under another window, that dialog's handler would have returned 1
		if (is_under_another_window(dlg, gadget))
			return 0;
#endif
		return 1;
	} else
		return 0;
}

window_event_result ui_gadget_send_event(UI_DIALOG &dlg, const event_type type, UI_GADGET &gadget)
{
	const event_gadget event{type, gadget};

	if (const auto parent = gadget.parent)
		return gadget.parent->event_handler(dlg, event);

	return window_send_event(dlg, event);
}

UI_GADGET &ui_event_get_gadget(const d_event &event)
{
	auto &e = static_cast<const event_gadget &>(event);
	Assert(e.type >= EVENT_UI_GADGET_PRESSED);	// Any UI event
	return e.gadget;
}

window_event_result ui_dialog_do_gadgets(UI_DIALOG &dlg, const d_event &event)
{
	int keypress = 0;
	UI_GADGET * tmp, * tmp1;

	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	tmp = dlg.gadget;

	if (tmp == NULL) return window_event_result::ignored;

	if (selected_gadget==NULL)
		selected_gadget = tmp;

	tmp1 = dlg.keyboard_focus_gadget;

	do
	{
		if (B1_JUST_PRESSED && ui_mouse_on_gadget(*tmp))
		{
			selected_gadget = tmp;
			if (tmp->parent!=NULL)
			{
				while (tmp->parent != NULL )
					tmp = tmp->parent;
			}
			dlg.keyboard_focus_gadget = tmp;
			break;
		}
		if ( tmp->hotkey == keypress )
		{
			dlg.keyboard_focus_gadget = tmp;
			break;
		}
		tmp = tmp->next;
	} while(tmp != dlg.gadget);

	if (dlg.keyboard_focus_gadget)
	{
		[&]{
		UI_GADGET *UI_GADGET::*when;
		switch (keypress )
		{
			case (KEY_TAB):
				when = &UI_GADGET::when_tab;
				break;
			case (KEY_TAB+KEY_SHIFTED):
				when = &UI_GADGET::when_btab;
				break;
			case (KEY_UP):
				when = &UI_GADGET::when_up;
	  			break;
			case (KEY_DOWN):
				when = &UI_GADGET::when_down;
				break;
			case (KEY_LEFT):
				when = &UI_GADGET::when_left;
				break;
			case (KEY_RIGHT):
				when = &UI_GADGET::when_right;
				break;
			default:
				return;
		}
		if (const auto p = dlg.keyboard_focus_gadget->*when)
			dlg.keyboard_focus_gadget = p;
		}();
	}

	window_event_result rval = window_event_result::ignored;
	if (dlg.keyboard_focus_gadget != tmp1)
	{
		if (dlg.keyboard_focus_gadget)
			dlg.keyboard_focus_gadget->status = 1;
		if (tmp1 != NULL )
			tmp1->status = 1;
		rval = window_event_result::handled;
		
		if (keypress)
			return rval;
	}

	tmp = dlg.gadget;
	do
	{
		// If it is under another dialog, that dialog's handler would have returned 1 for mouse events.
		// Key events are handled in a priority depending on the window ordering.
		//if (!is_under_another_window( dlg, tmp ))
			rval = tmp->event_handler(dlg, event);
		
		if (rval == window_event_result::deleted)
			break;

		tmp = tmp->next;
	} while(/*!rval &&*/ tmp != dlg.gadget);	// have to look for pesky scrollbars in case an arrow button or arrow key are held down
	
	return rval;
}

UI_GADGET &ui_gadget_get_next(UI_GADGET &gadget)
{
	return ui_gadget_get_step<&UI_GADGET::next>(gadget);
}

void ui_gadget_calc_keys(UI_DIALOG &dlg)
{
	auto tmp = dlg.gadget;
	if (tmp==NULL) return;

	do
	{
		tmp->when_tab = &ui_gadget_get_next(*tmp);
		tmp->when_btab = &ui_gadget_get_prev(*tmp);

		tmp = tmp->next;
	} while(tmp != dlg.gadget);
}

}
