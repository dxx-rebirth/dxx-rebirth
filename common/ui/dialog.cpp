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

/*
 *
 * Windowing functions and controller.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "window.h"
#include "u_mem.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "mouse.h"
#include "timer.h"
#include "dxxerror.h"
#include <memory>

namespace dcx {

static void ui_dialog_draw(UI_DIALOG *dlg)
{
	int w, h;
	int req_w, req_h;

	w = dlg->d_width;
	h = dlg->d_height;

	req_w = w;
	req_h = h;

	ui_dialog_set_current_canvas(*dlg);
	
	if (dlg->d_flags & DF_FILLED)
		ui_draw_box_out(*grd_curcanv, 0, 0, req_w-1, req_h-1);
	
	gr_set_fontcolor(*grd_curcanv, CBLACK, CWHITE);
}

// The dialog handler borrows heavily from the newmenu_handler
window_event_result UI_DIALOG::event_handler(const d_event &event)
{
	if (event.type == EVENT_WINDOW_ACTIVATED ||
		event.type == EVENT_WINDOW_DEACTIVATED)
		return window_event_result::ignored;
	auto rval = callback_handler(event);
	if (rval != window_event_result::ignored)
		return rval;		// event handled

	switch (event.type)
	{
		case EVENT_IDLE:
			timer_delay2(50);
			DXX_BOOST_FALLTHROUGH;
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
			return ui_dialog_do_gadgets(*this, event);
		case EVENT_WINDOW_DRAW:
		{
			ui_dialog_draw(this);
			rval = ui_dialog_do_gadgets(*this, event);
			if (rval != window_event_result::close)
			{
				d_event event2 = { EVENT_UI_DIALOG_DRAW };
				window_send_event(*this, event2);
			}
			return rval;
		}

		case EVENT_WINDOW_CLOSE:
			if (rval != window_event_result::deleted)	// check if handler already deleted dialog (e.g. if UI_DIALOG was subclassed)
				delete this;
			return window_event_result::deleted;	// free the window in any case (until UI_DIALOG is subclass of window)
		default:
			return window_event_result::ignored;
	}
}

static short adjust_starting_coordinate(short value, const int limit)
{
	if (value < 0)
		value = 0;
	if (value - 1 >= limit)
		value = limit;
	return value;
}

UI_DIALOG::UI_DIALOG(short x, short y, const short w, const short h, const enum dialog_flags flags) :
	window(grd_curscreen->sc_canvas, adjust_starting_coordinate(x, grd_curscreen->get_screen_width() - w), adjust_starting_coordinate(y, grd_curscreen->get_screen_height() - h), w, h),
	d_width(w), d_height(h), d_flags(flags)
{
	selected_gadget = NULL;

	if (!(flags & DF_MODAL))
		set_modal(0);	// make this window modeless, allowing events to propogate through the window stack
}

void ui_dialog_set_current_canvas(UI_DIALOG &dlg)
{
	gr_set_current_canvas(dlg.w_canv);
}

UI_DIALOG::~UI_DIALOG()
{
	selected_gadget = NULL;
}

void ui_close_dialog(UI_DIALOG &dlg)
{
	window_close(&dlg);
}

#if 0
void restore_state()
{
	_disable();
	range_for (const int i, xrange(256u))
	{
		keyd_pressed[i] = SavedState[i];
	}
	_enable();
}


void ui_mega_process()
{
	int mx, my, mz;
	unsigned char k;
	
	event_process();

	switch( Record )
	{
	case 0:
		mouse_get_delta( &mx, &my, &mz );
		Mouse.new_dx = mx;
		Mouse.new_dy = my;
		Mouse.new_buttons = mouse_get_btns();
		last_keypress = key_inkey();

		if ( Mouse.new_buttons || last_keypress || Mouse.new_dx || Mouse.new_dy )	{
			last_event = timer_query();
		}

		break;
	case 1:

		if (ui_event_counter==0 )
		{
			EventBuffer[ui_event_counter].frame = 0;
			EventBuffer[ui_event_counter].type = 7;
			EventBuffer[ui_event_counter].data = ui_number_of_events;
			ui_event_counter++;
		}


		if (ui_event_counter==1 && (RecordFlags & UI_RECORD_MOUSE) )
		{
			Mouse.new_buttons = 0;
			EventBuffer[ui_event_counter].frame = FrameCount;
			EventBuffer[ui_event_counter].type = 6;
			EventBuffer[ui_event_counter].data = ((Mouse.y & 0xFFFF) << 16) | (Mouse.x & 0xFFFF);
			ui_event_counter++;
		}

		mouse_get_delta( &mx, &my, &mz );
		MouseDX = mx;
		MouseDY = my;
		MouseButtons = mouse_get_btns();

		Mouse.new_dx = MouseDX;
		Mouse.new_dy = MouseDY;

		if ((MouseDX != 0 || MouseDY != 0)  && (RecordFlags & UI_RECORD_MOUSE)  )
		{
			if (ui_event_counter < ui_number_of_events-1 )
			{
				EventBuffer[ui_event_counter].frame = FrameCount;
				EventBuffer[ui_event_counter].type = 1;
				EventBuffer[ui_event_counter].data = ((MouseDY & 0xFFFF) << 16) | (MouseDX & 0xFFFF);
				ui_event_counter++;
			} else {
				Record = 0;
			}

		}

		if ( (MouseButtons != Mouse.new_buttons) && (RecordFlags & UI_RECORD_MOUSE) )
		{
			Mouse.new_buttons = MouseButtons;

			if (ui_event_counter < ui_number_of_events-1 )
			{
				EventBuffer[ui_event_counter].frame = FrameCount;
				EventBuffer[ui_event_counter].type = 2;
				EventBuffer[ui_event_counter].data = MouseButtons;
				ui_event_counter++;
			} else {
				Record = 0;
			}

		}


		if ( keyd_last_pressed && (RecordFlags & UI_RECORD_KEYS) )
		{
			_disable();
			k = keyd_last_pressed;
			keyd_last_pressed= 0;
			_enable();

			if (ui_event_counter < ui_number_of_events-1 )
			{
				EventBuffer[ui_event_counter].frame = FrameCount;
				EventBuffer[ui_event_counter].type = 3;
				EventBuffer[ui_event_counter].data = k;
				ui_event_counter++;
			} else {
				Record = 0;
			}

		}

		if ( keyd_last_released && (RecordFlags & UI_RECORD_KEYS) )
		{
			_disable();
			k = keyd_last_released;
			keyd_last_released= 0;
			_enable();

			if (ui_event_counter < ui_number_of_events-1 )
			{
				EventBuffer[ui_event_counter].frame = FrameCount;
				EventBuffer[ui_event_counter].type = 4;
				EventBuffer[ui_event_counter].data = k;
				ui_event_counter++;
			} else {
				Record = 0;
			}
		}

		last_keypress = key_inkey();

		if (last_keypress == KEY_F12 )
		{
			ui_number_of_events = ui_event_counter;
			last_keypress = 0;
			Record = 0;
			break;
		}

		if ((last_keypress != 0) && (RecordFlags & UI_RECORD_KEYS) )
		{
			if (ui_event_counter < ui_number_of_events-1 )
			{
				EventBuffer[ui_event_counter].frame = FrameCount;
				EventBuffer[ui_event_counter].type = 5;
				EventBuffer[ui_event_counter].data = last_keypress;
				ui_event_counter++;
			} else {
				Record = 0;
			}
		}

		FrameCount++;

		break;
	case 2:
	case 3:
		Mouse.new_dx = 0;
		Mouse.new_dy = 0;
		Mouse.new_buttons = 0;
		last_keypress = 0;

		if ( keyd_last_pressed ) {
			_disable();			
			k = keyd_last_pressed;
			keyd_last_pressed = 0;
			_disable();
			SavedState[k] = 1;
		}

		if ( keyd_last_released ) 
		{
			_disable();			
			k = keyd_last_released;
			keyd_last_released = 0;
			_disable();
			SavedState[k] = 0;
		}
		
		if (key_inkey() == KEY_F12 )
		{
			restore_state();
			Record = 0;
			break;
		}

		if (EventBuffer==NULL) {
			restore_state();
			Record = 0;
			break;
		}

		while( (ui_event_counter < ui_number_of_events) && (EventBuffer[ui_event_counter].frame <= FrameCount) )
		{
			switch ( EventBuffer[ui_event_counter].type )
			{
			case 1:     // Mouse moved
				Mouse.new_dx = EventBuffer[ui_event_counter].data & 0xFFFF;
				Mouse.new_dy = (EventBuffer[ui_event_counter].data >> 16) & 0xFFFF;
				break;
			case 2:     // Mouse buttons changed
				Mouse.new_buttons = EventBuffer[ui_event_counter].data;
				break;
			case 3:     // Key moved down
				keyd_pressed[ EventBuffer[ui_event_counter].data ] = 1;
				break;
			case 4:     // Key moved up
				keyd_pressed[ EventBuffer[ui_event_counter].data ] = 0;
				break;
			case 5:     // Key pressed
				last_keypress = EventBuffer[ui_event_counter].data;
				break;
			case 6:     // Initial Mouse X position
				Mouse.x = EventBuffer[ui_event_counter].data & 0xFFFF;
				Mouse.y = (EventBuffer[ui_event_counter].data >> 16) & 0xFFFF;
				break;
			case 7:
				break;
			}
			ui_event_counter++;
			if (ui_event_counter >= ui_number_of_events )
			{
				Record = 0;
				restore_state();
				//( 0, "Done playing %d events.\n", ui_number_of_events );
			}
		}

		switch (Record)
		{
		case 2:
			{
				int next_frame;
			
				if ( ui_event_counter < ui_number_of_events )
				{
					next_frame = EventBuffer[ui_event_counter].frame;
					
					if ( (FrameCount+PlaybackSpeed) < next_frame )
						FrameCount = next_frame - PlaybackSpeed;
					else
						FrameCount++;
				} else {
				 	FrameCount++;
				}	
			}
			break;

		case 3:			
 			if ( ui_event_counter < ui_number_of_events ) 
				FrameCount = EventBuffer[ui_event_counter].frame;
			else 		
				FrameCount++;
			break;
		default:		
			FrameCount++;
		}
	}

	ui_mouse_process();

}
#endif // 0

void (ui_dprintf_at)( UI_DIALOG * dlg, short x, short y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsnprintf(buffer,sizeof(buffer),format,args);
	va_end(args);

	ui_dputs_at(dlg, x, y, buffer);
}

void ui_dputs_at( UI_DIALOG * dlg, short x, short y, const char * buffer )
{
	ui_dialog_set_current_canvas(*dlg);
	gr_string(*grd_curcanv, *grd_curcanv->cv_font, x, y, buffer);
}

}
