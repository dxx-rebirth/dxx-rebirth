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

void ui_draw_scrollbar(UI_DIALOG &dlg, UI_GADGET_SCROLLBAR &scrollbar)
{
#if 0  //ndef OGL
	if (scrollbar.status==0)
		return;
#endif

	gr_set_current_canvas(scrollbar.canvas);
	auto &canvas = *grd_curcanv;

	const auto color = (dlg.keyboard_focus_gadget == &scrollbar)
		? CRED
		: CGREY;

	gr_rect(canvas, 0, 0, scrollbar.width - 1, scrollbar.fake_position - 1, color);
	gr_rect(canvas, 0, scrollbar.fake_position + scrollbar.fake_size, scrollbar.width - 1, scrollbar.height - 1, color);
	ui_draw_box_out(canvas, 0, scrollbar.fake_position, scrollbar.width - 1, scrollbar.fake_position + scrollbar.fake_size - 1);
}

}

std::unique_ptr<UI_GADGET_SCROLLBAR> ui_add_gadget_scrollbar(UI_DIALOG &dlg, short x, short y, short w, short h, int start, int stop, int position, int window_size)
{
	int tw;

	auto &up = "\x1e";
	auto &down = "\x1f";

	gr_get_string_size(*grd_curcanv->cv_font, up, &tw, nullptr, nullptr);

	w = tw + 10;

	if (stop < start ) stop = start;

	auto scrollbar = ui_gadget_add<UI_GADGET_SCROLLBAR>(dlg, x, y + w, x + w - 1, y + h - w - 1);

	scrollbar->up_button = ui_add_gadget_button(dlg, x, y, w, w, up, nullptr);
	scrollbar->up_button->parent = scrollbar.get();

	scrollbar->down_button = ui_add_gadget_button(dlg, x, y+h-w, w, w, down, nullptr);
	scrollbar->down_button->parent = scrollbar.get();

	scrollbar->horz = 0;
	scrollbar->width = scrollbar->x2-scrollbar->x1+1;
	scrollbar->height = scrollbar->y2-scrollbar->y1+1;
	scrollbar->start = start;
	scrollbar->stop = stop;
	scrollbar->position = position;
	scrollbar->window_size = window_size;
	scrollbar->fake_length = scrollbar->height;
	scrollbar->fake_position =  0;
	if (stop!=start)
		scrollbar->fake_size = (window_size * scrollbar->height)/(stop-start+1+window_size);
	else
		scrollbar->fake_size = scrollbar->height;

	if (scrollbar->fake_size < 7) scrollbar->fake_size = 7;
	scrollbar->dragging = 0;
	scrollbar->moved=0;
	scrollbar->last_scrolled = 0;
	return scrollbar;

}

window_event_result UI_GADGET_SCROLLBAR::event_handler(UI_DIALOG &dlg, const d_event &event)
{
	int x, y, z;
	window_event_result rval = window_event_result::ignored;
		
	if (event.type == EVENT_WINDOW_DRAW)
	{
		ui_draw_scrollbar(dlg, *this);
		return window_event_result::ignored;
	}

	const auto keyfocus = (dlg.keyboard_focus_gadget == this);

	if (start==stop)
	{
		position = 0;
		fake_position = 0;
		ui_draw_scrollbar(dlg, *this);
		return window_event_result::ignored;
	}

	const auto op = position;

	moved = 0;


	if (keyfocus && event.type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);

		if (key & KEY_UP)
		{
			up_button->position = 2;
			rval = window_event_result::handled;
		}
		else if (key & KEY_DOWN)
		{
			down_button->position = 2;
			rval = window_event_result::handled;
		}
	}
	else if (keyfocus && event.type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		if (key & KEY_UP)
		{
			up_button->position = 0;
			rval = window_event_result::handled;
		}
		else if (key & KEY_DOWN)
		{
			down_button->position = 0;
			rval = window_event_result::handled;
		}
	}
	
	if (up_button->position!=0)
	{
		if (timer_query() > last_scrolled + 1)
		{
			last_scrolled = timer_query();
			position--;
			if (position < start )
				position = start;
			fake_position = position-start;
			fake_position *= height-fake_size;
			fake_position /= (stop-start);
		}
	}

	if (down_button->position!=0)
	{
		if (timer_query() > last_scrolled + 1)
		{
			last_scrolled = timer_query();
			position++;
			if (position > stop )
				position = stop;
			fake_position = position-start;
			fake_position *= height-fake_size;
			fake_position /= (stop-start);
		}
	}

	const auto OnMe = ui_mouse_on_gadget(*this);

	//gr_ubox(0, fake_position, width-1, fake_position+fake_size-1 );

	if (B1_JUST_RELEASED)
		dragging = 0;

	//if (B1_PRESSED && OnMe )
	//    listbox->dragging = 1;


	mouse_get_pos(&x, &y, &z);

	const auto OnSlider = y >= fake_position + y1 &&
		y < fake_position + y1 + fake_size &&
		OnMe;

	if (B1_JUST_PRESSED && OnSlider )
	{
		dragging = 1;
		drag_x = x;
		drag_y = y;
		drag_starting = fake_position;
		rval = window_event_result::handled;
	}
	else if (B1_JUST_PRESSED && OnMe)
	{
		dragging = 2;	// outside the slider
		rval = window_event_result::handled;
	}

	if  ((dragging == 2) && OnMe && !OnSlider && (timer_query() > last_scrolled + 4))
	{
		last_scrolled = timer_query();

		if ( y < fake_position+y1 )
		{
			// Page Up
			position -= window_size;
			if (position < start )
				position = start;

		} else {
			// Page Down
			position += window_size;
			if (position > stop )
				position = stop;
		}
		fake_position = position-start;
		fake_position *= height-fake_size;
		fake_position /= (stop-start);
	}

	if (selected_gadget == this && dragging == 1)
	{
		//x = drag_x;
		fake_position = drag_starting + (y - drag_y );
		if (fake_position<0)
		{
			fake_position = 0;
			//y = fake_position + drag_y - drag_starting;
		}
		if (fake_position > (height-fake_size))
		{
			fake_position = (height-fake_size);
			//y = fake_position + drag_y - drag_starting;
		}

		//mouse_set_pos( x, y );

		position = fake_position;
		position *= (stop-start);
		position /= ( height-fake_size ) ;
		position += start;

		if (position > stop )
			position = stop;

		if (position < start )
			position = start;

		//fake_position = position-start;
		//fake_position *= height-fake_size;
		//fake_position /= (stop-start);

	}

	if (op != position )
		moved = 1;
	if (moved)
	{
		rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, *this);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}
	return rval;
}

}
