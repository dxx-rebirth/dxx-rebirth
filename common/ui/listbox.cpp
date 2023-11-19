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

namespace {

static void gr_draw_sunken_border( short x1, short y1, short x2, short y2 );

void ui_draw_listbox(UI_DIALOG &dlg, UI_GADGET_LISTBOX &listbox)
{
	int x, y, stop;
	int w, h;

	//if (listbox.current_item<0)
	//    listbox.current_item=0;
	//if (listbox.current_item>=listbox.num_items)
	//    listbox.current_item = listbox.num_items - 1;
	//if (listbox.first_item<0)
	//   listbox.first_item=0;
	//if (listbox.first_item>(listbox.num_items-listbox.num_items_displayed))
	//    listbox.first_item=(listbox.num_items-listbox.num_items_displayed);

#if 0  //ndef OGL
	if ((listbox.status!=1) && !listbox.moved)
		return;
#endif

	gr_set_current_canvas(listbox.canvas);
	auto &canvas = *grd_curcanv;
	
	w = listbox.width;
	h = listbox.height;

	gr_rect(canvas, 0, 0, w-1, h-1, CBLACK);
	
	gr_draw_sunken_border(-2, -2, w+listbox.scrollbar->width+4, h+1);
	
	stop = listbox.first_item+listbox.num_items_displayed;
	if (stop>listbox.num_items) stop = listbox.num_items;

	x = y = 0;

	for (int i = listbox.first_item; i < stop; ++i)
	{
		const auto color = (i == listbox.current_item)
			? CGREY
			: CBLACK;
		gr_rect(canvas, x, y, listbox.width - 1, y + h - 1, color);

		if (i != listbox.current_item)
		{
			if (listbox.current_item == -1 && dlg.keyboard_focus_gadget == &listbox && i == listbox.first_item)
				gr_set_fontcolor(canvas, CRED, -1);
			else
				gr_set_fontcolor(canvas, CWHITE, -1);
		}
		else
		{
			if (dlg.keyboard_focus_gadget == &listbox)
				gr_set_fontcolor(canvas, CRED, -1);
			else
				gr_set_fontcolor(canvas, CBLACK, -1);
		}
		const auto &&[w, h] = gr_get_string_size(*canvas.cv_font, listbox.list[i]);
		gr_string(canvas, *canvas.cv_font, x + 2, y, listbox.list[i], w, h);
		y += h;
	}

	if (stop < listbox.num_items_displayed - 1)
	{
		gr_rect(canvas, x, y, listbox.width - 1, listbox.height-1, CBLACK);
	}
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

}

std::unique_ptr<UI_GADGET_LISTBOX> ui_add_gadget_listbox(UI_DIALOG &dlg, short x, short y, short w, short h, short numitems, char **list)
{
	int i;
	const auto th = gr_get_string_size(*grd_curcanv->cv_font, "*").height;

	i = h / th;
	h = i * th;

	auto listbox = ui_gadget_add<UI_GADGET_LISTBOX>(dlg, x, y, x + w - 1, y + h - 1);
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
	listbox->scrollbar = ui_add_gadget_scrollbar(dlg, x + w + 3, y, 0, h, 0, numitems - i, 0, i);
	listbox->scrollbar->parent = listbox.get();
	return listbox;
}

window_event_result UI_GADGET_LISTBOX::event_handler(UI_DIALOG &dlg, const d_event &event)
{
	int kf;
	if (event.type == event_type::window_draw)
	{
		ui_draw_listbox(dlg, *this);
		return window_event_result::ignored;
	}
	const auto keypress = (event.type == event_type::key_command)
		? event_key_get(event)
		: 0u;
	selected_item = -1;

	moved = 0;

	if (num_items < 1 ) {
		current_item = -1;
		first_item = 0;
		old_current_item = current_item;
		old_first_item = first_item;

		if (dlg.keyboard_focus_gadget == this)
		{
			dlg.keyboard_focus_gadget = &ui_gadget_get_next(*this);
		}

		return window_event_result::ignored;
	}

	old_current_item = current_item;
	old_first_item = first_item;


	if (GADGET_PRESSED(scrollbar.get()))
	{
		moved = 1;

		first_item = scrollbar->position;

		if (current_item<first_item)
			current_item = first_item;

		if (current_item>(first_item+num_items_displayed-1))
			current_item = first_item + num_items_displayed-1;

	}

	if (B1_JUST_RELEASED)
		dragging = 0;

	window_event_result rval = window_event_result::ignored;
	if (B1_JUST_PRESSED && ui_mouse_on_gadget(*this))
	{
		dragging = 1;
		rval = window_event_result::handled;
	}

	if (dlg.keyboard_focus_gadget == this)
	{
		if (keypress==KEY_ENTER)   {
			selected_item = current_item;
			rval = window_event_result::handled;
		}

		kf = 0;

		switch(keypress)
		{
			case (KEY_UP):
				current_item--;
				kf = 1;
				break;
			case (KEY_DOWN):
				current_item++;
				kf = 1;
				break;
			case (KEY_HOME):
				current_item=0;
				kf = 1;
				break;
			case (KEY_END):
				current_item=num_items-1;
				kf = 1;
				break;
			case (KEY_PAGEUP):
				current_item -= num_items_displayed;
				kf = 1;
				break;
			case (KEY_PAGEDOWN):
				current_item += num_items_displayed;
				kf = 1;
				break;
		}

		if (kf==1)
		{
			moved = 1;
			rval = window_event_result::handled;

			if (current_item<0)
				current_item=0;

			if (current_item>=num_items)
				current_item = num_items-1;

			if (current_item<first_item)
				first_item = current_item;

			if (current_item>=(first_item+num_items_displayed))
				first_item = current_item-num_items_displayed+1;

			if (num_items <= num_items_displayed )
				first_item = 0;
			else
			{
				scrollbar->position = first_item;

				scrollbar->fake_position = scrollbar->position-scrollbar->start;
				scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
				scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);

				if (scrollbar->fake_position<0)
				{
					scrollbar->fake_position = 0;
				}
				if (scrollbar->fake_position > (scrollbar->height-scrollbar->fake_size))
				{
					scrollbar->fake_position = (scrollbar->height-scrollbar->fake_size);
				}
			}
		}
	}


	if (selected_gadget == this)
	{
		if (dragging)
		{
			int x, y, z;
			
			mouse_get_pos(&x, &y, &z);
			const auto mitem = (y < y1)
				? -1
				: (y - y1) / textheight;

			if ((mitem < 0) && (timer_query() > last_scrolled + 1))
			{
				current_item--;
				last_scrolled = timer_query();
				moved = 1;
			}

			if ((mitem >= num_items_displayed) &&
				 (timer_query() > last_scrolled + 1))
			{
				current_item++;
				last_scrolled = timer_query();
				moved = 1;
			}

			if ((mitem>=0) && (mitem<num_items_displayed))
			{
				current_item = mitem+first_item;
				moved=1;
			}

			if (current_item <0 )
				current_item = 0;

			if (current_item >= num_items )
				current_item = num_items-1;

			if (current_item<first_item)
				first_item = current_item;

			if (current_item>=(first_item+num_items_displayed))
				first_item = current_item-num_items_displayed+1;

			if (num_items <= num_items_displayed )
				first_item = 0;
			else
			{
				scrollbar->position = first_item;

				scrollbar->fake_position = scrollbar->position-scrollbar->start;
				scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
				scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);

				if (scrollbar->fake_position<0)
				{
					scrollbar->fake_position = 0;
				}
				if (scrollbar->fake_position > (scrollbar->height-scrollbar->fake_size))
				{
					scrollbar->fake_position = (scrollbar->height-scrollbar->fake_size);
				}
			}

		}

		if (B1_DOUBLE_CLICKED )
		{
			selected_item = current_item;
			rval = window_event_result::handled;
		}

	}
	if (moved || (selected_item > 0))
	{
		rval = ui_gadget_send_event(dlg, (selected_item > 0) ? event_type::ui_listbox_selected : event_type::ui_listbox_moved, *this);
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


}

}
