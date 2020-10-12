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

void ui_draw_scrollbar( UI_DIALOG *dlg, UI_GADGET_SCROLLBAR * scrollbar )
{
#if 0  //ndef OGL
	if (scrollbar->status==0)
		return;
#endif

	scrollbar->status = 0;
	gr_set_current_canvas( scrollbar->canvas );
	auto &canvas = *grd_curcanv;

	const auto color = (dlg->keyboard_focus_gadget == scrollbar)
		? CRED
		: CGREY;

	gr_rect(canvas, 0, 0, scrollbar->width - 1, scrollbar->fake_position - 1, color);
	gr_rect(canvas, 0, scrollbar->fake_position + scrollbar->fake_size, scrollbar->width - 1, scrollbar->height - 1, color);
	ui_draw_box_out(canvas, 0, scrollbar->fake_position, scrollbar->width - 1, scrollbar->fake_position + scrollbar->fake_size - 1);
}

std::unique_ptr<UI_GADGET_SCROLLBAR> ui_add_gadget_scrollbar(UI_DIALOG * dlg, short x, short y, short w, short h, int start, int stop, int position, int window_size)
{
	int tw;

	auto &up = "\x1e";
	auto &down = "\x1f";

	gr_get_string_size(*grd_curcanv->cv_font, up, &tw, nullptr, nullptr);

	w = tw + 10;

	if (stop < start ) stop = start;

	auto scrollbar = ui_gadget_add<UI_GADGET_SCROLLBAR>(*dlg, x, y + w, x + w - 1, y + h - w - 1);

	scrollbar->up_button = ui_add_gadget_button(*dlg, x, y, w, w, up, nullptr);
	scrollbar->up_button->parent = scrollbar.get();

	scrollbar->down_button = ui_add_gadget_button(*dlg, x, y+h-w, w, w, down, nullptr);
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

window_event_result ui_scrollbar_do( UI_DIALOG *dlg, UI_GADGET_SCROLLBAR * scrollbar,const d_event &event )
{
	int OnMe, OnSlider, keyfocus;
	int oldpos, op;
	int x, y, z;
	window_event_result rval = window_event_result::ignored;
		
	if (event.type == EVENT_WINDOW_DRAW)
	{
		ui_draw_scrollbar( dlg, scrollbar );
		return window_event_result::ignored;
	}

	keyfocus = 0;

	if (dlg->keyboard_focus_gadget==scrollbar)
		keyfocus = 1;

	if (scrollbar->start==scrollbar->stop)
	{
		scrollbar->position = 0;
		scrollbar->fake_position = 0;
		ui_draw_scrollbar( dlg, scrollbar );
		return window_event_result::ignored;
	}

	op = scrollbar->position;

	oldpos = scrollbar->fake_position;

	scrollbar->moved = 0;


	if (keyfocus && event.type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);

		if (key & KEY_UP)
		{
			scrollbar->up_button->position = 2;
			rval = window_event_result::handled;
		}
		else if (key & KEY_DOWN)
		{
			scrollbar->down_button->position = 2;
			rval = window_event_result::handled;
		}
	}
	else if (keyfocus && event.type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		if (key & KEY_UP)
		{
			scrollbar->up_button->position = 0;
			rval = window_event_result::handled;
		}
		else if (key & KEY_DOWN)
		{
			scrollbar->down_button->position = 0;
			rval = window_event_result::handled;
		}
	}
	
	if (scrollbar->up_button->position!=0)
	{
		if (timer_query() > scrollbar->last_scrolled + 1)
		{
			scrollbar->last_scrolled = timer_query();
			scrollbar->position--;
			if (scrollbar->position < scrollbar->start )
				scrollbar->position = scrollbar->start;
			scrollbar->fake_position = scrollbar->position-scrollbar->start;
			scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
			scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);
		}
	}

	if (scrollbar->down_button->position!=0)
	{
		if (timer_query() > scrollbar->last_scrolled + 1)
		{
			scrollbar->last_scrolled = timer_query();
			scrollbar->position++;
			if (scrollbar->position > scrollbar->stop )
				scrollbar->position = scrollbar->stop;
			scrollbar->fake_position = scrollbar->position-scrollbar->start;
			scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
			scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);
		}
	}

	OnMe = ui_mouse_on_gadget( scrollbar );

	//gr_ubox(0, scrollbar->fake_position, scrollbar->width-1, scrollbar->fake_position+scrollbar->fake_size-1 );

	if (B1_JUST_RELEASED)
		scrollbar->dragging = 0;

	//if (B1_PRESSED && OnMe )
	//    listbox->dragging = 1;


	mouse_get_pos(&x, &y, &z);

	OnSlider = 0;
	if ((y >= scrollbar->fake_position+scrollbar->y1) && \
		(y < scrollbar->fake_position+scrollbar->y1+scrollbar->fake_size) && OnMe )
		OnSlider = 1;

	if (B1_JUST_PRESSED && OnSlider )
	{
		scrollbar->dragging = 1;
		scrollbar->drag_x = x;
		scrollbar->drag_y = y;
		scrollbar->drag_starting = scrollbar->fake_position;
		rval = window_event_result::handled;
	}
	else if (B1_JUST_PRESSED && OnMe)
	{
		scrollbar->dragging = 2;	// outside the slider
		rval = window_event_result::handled;
	}

	if  ((scrollbar->dragging == 2) && OnMe && !OnSlider && (timer_query() > scrollbar->last_scrolled + 4))
	{
		scrollbar->last_scrolled = timer_query();

		if ( y < scrollbar->fake_position+scrollbar->y1 )
		{
			// Page Up
			scrollbar->position -= scrollbar->window_size;
			if (scrollbar->position < scrollbar->start )
				scrollbar->position = scrollbar->start;

		} else {
			// Page Down
			scrollbar->position += scrollbar->window_size;
			if (scrollbar->position > scrollbar->stop )
				scrollbar->position = scrollbar->stop;
		}
		scrollbar->fake_position = scrollbar->position-scrollbar->start;
		scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
		scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);
	}

	if ((selected_gadget==scrollbar) && (scrollbar->dragging == 1))
	{
		//x = scrollbar->drag_x;
		scrollbar->fake_position = scrollbar->drag_starting + (y - scrollbar->drag_y );
		if (scrollbar->fake_position<0)
		{
			scrollbar->fake_position = 0;
			//y = scrollbar->fake_position + scrollbar->drag_y - scrollbar->drag_starting;
		}
		if (scrollbar->fake_position > (scrollbar->height-scrollbar->fake_size))
		{
			scrollbar->fake_position = (scrollbar->height-scrollbar->fake_size);
			//y = scrollbar->fake_position + scrollbar->drag_y - scrollbar->drag_starting;
		}

		//mouse_set_pos( x, y );

		scrollbar->position = scrollbar->fake_position;
		scrollbar->position *= (scrollbar->stop-scrollbar->start);
		scrollbar->position /= ( scrollbar->height-scrollbar->fake_size ) ;
		scrollbar->position += scrollbar->start;

		if (scrollbar->position > scrollbar->stop )
			scrollbar->position = scrollbar->stop;

		if (scrollbar->position < scrollbar->start )
			scrollbar->position = scrollbar->start;

		//scrollbar->fake_position = scrollbar->position-scrollbar->start;
		//scrollbar->fake_position *= scrollbar->height-scrollbar->fake_size;
		//scrollbar->fake_position /= (scrollbar->stop-scrollbar->start);

	}

	if (op != scrollbar->position )
		scrollbar->moved = 1;
	if (scrollbar->moved)
	{
		rval = ui_gadget_send_event(*dlg, EVENT_UI_GADGET_PRESSED, *scrollbar);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}

	if (oldpos != scrollbar->fake_position)
		scrollbar->status = 1;

	return rval;
}

}
