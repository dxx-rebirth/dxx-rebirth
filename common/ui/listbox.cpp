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

#include "maths.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"
#include "timer.h"

namespace dcx {

static void gr_draw_sunken_border( short x1, short y1, short x2, short y2 );

void ui_draw_listbox( UI_DIALOG *dlg, UI_GADGET_LISTBOX * listbox )
{
	int i, x, y, stop;
	int w, h;

	//if (listbox->current_item<0)
	//    listbox->current_item=0;
	//if (listbox->current_item>=listbox->num_items)
	//    listbox->current_item = listbox->num_items-1;
	//if (listbox->first_item<0)
	//   listbox->first_item=0;
	//if (listbox->first_item>(listbox->num_items-listbox->num_items_displayed))
	//    listbox->first_item=(listbox->num_items-listbox->num_items_displayed);

#if 0  //ndef OGL
	if ((listbox->status!=1) && !listbox->moved )
		return;
#endif

	gr_set_current_canvas( listbox->canvas );
	auto &canvas = *grd_curcanv;
	
	w = listbox->width;
	h = listbox->height;

	gr_rect(canvas, 0, 0, w-1, h-1, CBLACK);
	
	gr_draw_sunken_border( -2, -2, w+listbox->scrollbar->width+4, h+1);
	
	stop = listbox->first_item+listbox->num_items_displayed;
	if (stop>listbox->num_items) stop = listbox->num_items;

	listbox->status = 0;

	x = y = 0;

	for (i= listbox->first_item; i< stop; i++ )
	{
		uint8_t color = (i == listbox->current_item)
			? CGREY
			: CBLACK;
		gr_rect(canvas, x, y, listbox->width - 1, y + h - 1, color);

		if (i !=listbox->current_item)
		{
			if ((listbox->current_item == -1) && (dlg->keyboard_focus_gadget == listbox) && (i == listbox->first_item)  )
				gr_set_fontcolor(canvas, CRED, -1);
			else
				gr_set_fontcolor(canvas, CWHITE, -1);
		}
		else
		{
			if (dlg->keyboard_focus_gadget == listbox)
				gr_set_fontcolor(canvas, CRED, -1);
			else
				gr_set_fontcolor(canvas, CBLACK, -1);
		}
		gr_get_string_size(*canvas.cv_font, listbox->list[i], &w, &h, nullptr);
		gr_string(canvas, *canvas.cv_font, x + 2, y, listbox->list[i], w, h);
		y += h;
	}

	if (stop < listbox->num_items_displayed-1 )
	{
		gr_rect(canvas, x, y, listbox->width-1, listbox->height-1, CBLACK);
	}

	//gr_ubox( -1, -1, listbox->width, listbox->height);
}


static void gr_draw_sunken_border( short x1, short y1, short x2, short y2 )
{
	const uint8_t cgrey = CGREY;
	const uint8_t cbright = CBRIGHT;
	Hline(*grd_curcanv, x1 - 1, x2 + 1, y1 - 1, cgrey);
	Vline(*grd_curcanv, y1 - 1, y2 + 1, x1 - 1, cgrey);

	Hline(*grd_curcanv, x1 - 1, x2 + 1, y2 + 1, cbright);
	Vline(*grd_curcanv, y1, y2 + 1, x2 + 1, cbright);
}


std::unique_ptr<UI_GADGET_LISTBOX> ui_add_gadget_listbox(UI_DIALOG *dlg, short x, short y, short w, short h, short numitems, char **list)
{
	int th, i;
	gr_get_string_size(*grd_curcanv->cv_font, "*", nullptr, &th, nullptr);

	i = h / th;
	h = i * th;

	std::unique_ptr<UI_GADGET_LISTBOX> listbox{ui_gadget_add<UI_GADGET_LISTBOX>( dlg, x, y, x+w-1, y+h-1 )};
	listbox->list = list;
	listbox->width = w;
	listbox->height = h;
	listbox->num_items = numitems;
	listbox->num_items_displayed = i;
	listbox->first_item = 0;
	listbox->current_item = -1;
	listbox->last_scrolled = 0;
	listbox->textheight = th;
	listbox->dragging = 0;
	listbox->selected_item = -1;
	listbox->moved = 1;
	listbox->scrollbar = ui_add_gadget_scrollbar( dlg, x+w+3, y, 0, h, 0, numitems-i, 0, i );
	listbox->scrollbar->parent = listbox.get();
	return listbox;
}

window_event_result ui_listbox_do( UI_DIALOG *dlg, UI_GADGET_LISTBOX * listbox,const d_event &event )
{
	int mitem, oldfakepos, kf;
	int keypress = 0;
	if (event.type == EVENT_WINDOW_DRAW)
	{
		ui_draw_listbox( dlg, listbox );
		return window_event_result::ignored;
	}
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	listbox->selected_item = -1;

	listbox->moved = 0;

	if (listbox->num_items < 1 ) {
		listbox->current_item = -1;
		listbox->first_item = 0;
		listbox->old_current_item = listbox->current_item;
		listbox->old_first_item = listbox->first_item;
		//ui_draw_listbox( dlg, listbox );

		if (dlg->keyboard_focus_gadget == listbox)
		{
			dlg->keyboard_focus_gadget = ui_gadget_get_next(listbox);
		}

		return window_event_result::ignored;
	}

	listbox->old_current_item = listbox->current_item;
	listbox->old_first_item = listbox->first_item;


	if (GADGET_PRESSED(listbox->scrollbar.get()))
	{
		listbox->moved = 1;

		listbox->first_item = listbox->scrollbar->position;

		if (listbox->current_item<listbox->first_item)
			listbox->current_item = listbox->first_item;

		if (listbox->current_item>(listbox->first_item+listbox->num_items_displayed-1))
			listbox->current_item = listbox->first_item + listbox->num_items_displayed-1;

	}

	if (B1_JUST_RELEASED)
		listbox->dragging = 0;

	window_event_result rval = window_event_result::ignored;
	if (B1_JUST_PRESSED && ui_mouse_on_gadget( listbox ))
	{
		listbox->dragging = 1;
		rval = window_event_result::handled;
	}

	if ( dlg->keyboard_focus_gadget==listbox )
	{
		if (keypress==KEY_ENTER)   {
			listbox->selected_item = listbox->current_item;
			rval = window_event_result::handled;
		}

		kf = 0;

		switch(keypress)
		{
			case (KEY_UP):
				listbox->current_item--;
				kf = 1;
				break;
			case (KEY_DOWN):
				listbox->current_item++;
				kf = 1;
				break;
			case (KEY_HOME):
				listbox->current_item=0;
				kf = 1;
				break;
			case (KEY_END):
				listbox->current_item=listbox->num_items-1;
				kf = 1;
				break;
			case (KEY_PAGEUP):
				listbox->current_item -= listbox->num_items_displayed;
				kf = 1;
				break;
			case (KEY_PAGEDOWN):
				listbox->current_item += listbox->num_items_displayed;
				kf = 1;
				break;
		}

		if (kf==1)
		{
			listbox->moved = 1;
			rval = window_event_result::handled;

			if (listbox->current_item<0)
				listbox->current_item=0;

			if (listbox->current_item>=listbox->num_items)
				listbox->current_item = listbox->num_items-1;

			if (listbox->current_item<listbox->first_item)
				listbox->first_item = listbox->current_item;

			if (listbox->current_item>=(listbox->first_item+listbox->num_items_displayed))
				listbox->first_item = listbox->current_item-listbox->num_items_displayed+1;

			if (listbox->num_items <= listbox->num_items_displayed )
				listbox->first_item = 0;
			else
			{
				oldfakepos = listbox->scrollbar->position;
				listbox->scrollbar->position = listbox->first_item;

				listbox->scrollbar->fake_position = listbox->scrollbar->position-listbox->scrollbar->start;
				listbox->scrollbar->fake_position *= listbox->scrollbar->height-listbox->scrollbar->fake_size;
				listbox->scrollbar->fake_position /= (listbox->scrollbar->stop-listbox->scrollbar->start);

				if (listbox->scrollbar->fake_position<0)
				{
					listbox->scrollbar->fake_position = 0;
				}
				if (listbox->scrollbar->fake_position > (listbox->scrollbar->height-listbox->scrollbar->fake_size))
				{
					listbox->scrollbar->fake_position = (listbox->scrollbar->height-listbox->scrollbar->fake_size);
				}

				if (oldfakepos != listbox->scrollbar->position )
					listbox->scrollbar->status = 1;
			}
		}
	}


	if (selected_gadget==listbox)
	{
		if (listbox->dragging)
		{
			int x, y, z;
			
			mouse_get_pos(&x, &y, &z);
			if (y < listbox->y1)
				mitem = -1;
			else
				mitem = (y - listbox->y1)/listbox->textheight;

			if ((mitem < 0) && (timer_query() > listbox->last_scrolled + 1))
			{
				listbox->current_item--;
				listbox->last_scrolled = timer_query();
				listbox->moved = 1;
			}

			if ((mitem >= listbox->num_items_displayed) &&
				 (timer_query() > listbox->last_scrolled + 1))
			{
				listbox->current_item++;
				listbox->last_scrolled = timer_query();
				listbox->moved = 1;
			}

			if ((mitem>=0) && (mitem<listbox->num_items_displayed))
			{
				listbox->current_item = mitem+listbox->first_item;
				listbox->moved=1;
			}

			if (listbox->current_item <0 )
				listbox->current_item = 0;

			if (listbox->current_item >= listbox->num_items )
				listbox->current_item = listbox->num_items-1;

			if (listbox->current_item<listbox->first_item)
				listbox->first_item = listbox->current_item;

			if (listbox->current_item>=(listbox->first_item+listbox->num_items_displayed))
				listbox->first_item = listbox->current_item-listbox->num_items_displayed+1;

			if (listbox->num_items <= listbox->num_items_displayed )
				listbox->first_item = 0;
			else
			{
				oldfakepos = listbox->scrollbar->position;
				listbox->scrollbar->position = listbox->first_item;

				listbox->scrollbar->fake_position = listbox->scrollbar->position-listbox->scrollbar->start;
				listbox->scrollbar->fake_position *= listbox->scrollbar->height-listbox->scrollbar->fake_size;
				listbox->scrollbar->fake_position /= (listbox->scrollbar->stop-listbox->scrollbar->start);

				if (listbox->scrollbar->fake_position<0)
				{
					listbox->scrollbar->fake_position = 0;
				}
				if (listbox->scrollbar->fake_position > (listbox->scrollbar->height-listbox->scrollbar->fake_size))
				{
					listbox->scrollbar->fake_position = (listbox->scrollbar->height-listbox->scrollbar->fake_size);
				}

				if (oldfakepos != listbox->scrollbar->position )
					listbox->scrollbar->status = 1;
			}

		}

		if (B1_DOUBLE_CLICKED )
		{
			listbox->selected_item = listbox->current_item;
			rval = window_event_result::handled;
		}

	}
	
	if (listbox->moved || (listbox->selected_item > 0))
	{
		rval = ui_gadget_send_event(dlg, (listbox->selected_item > 0) ? EVENT_UI_LISTBOX_SELECTED : EVENT_UI_LISTBOX_MOVED, listbox);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}

	return rval;
}

void ui_listbox_change(UI_DIALOG *, UI_GADGET_LISTBOX *listbox, uint_fast32_t numitems, const char *const *list)
{
	int stop, start;
	UI_GADGET_SCROLLBAR * scrollbar;

	listbox->list = list;
	listbox->num_items = numitems;
	listbox->first_item = 0;
	listbox->current_item = -1;
	listbox->last_scrolled = timer_query();
	listbox->dragging = 0;
	listbox->selected_item = -1;
	listbox->status = 1;
	listbox->first_item = 0;
	listbox->current_item = listbox->old_current_item = 0;
	listbox->moved = 0;

	scrollbar = listbox->scrollbar.get();

	start=0;
	stop= numitems - listbox->num_items_displayed;

	if (stop < start) stop = start;

	scrollbar->horz = 0;
	scrollbar->start = start;
	scrollbar->stop = stop;
	scrollbar->fake_length = scrollbar->height;
	scrollbar->fake_position =  0;
	if (stop!=start)
		scrollbar->fake_size = (listbox->num_items_displayed * scrollbar->height)/(stop-start+1+listbox->num_items_displayed);
	else
		scrollbar->fake_size = scrollbar->height;

	if (scrollbar->fake_size < 7) scrollbar->fake_size = 7;
	scrollbar->dragging = 0;
	scrollbar->moved=0;
	scrollbar->status=1;


}

}
