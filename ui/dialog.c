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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
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
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "mouse.h"
#include "timer.h"
#include "dxxerror.h"

#define D_X             (dlg->x)
#define D_Y             (dlg->y)
#define D_WIDTH         (dlg->width)
#define D_HEIGHT        (dlg->height)
#define D_GADGET        (dlg->gadget)
#define D_TEXT_X        (dlg->text_x)
#define D_TEXT_Y        (dlg->text_y)

#ifndef __MSDOS__
#define _disable()
#define _enable()
#endif

#define BORDER_WIDTH 8

static unsigned int FrameCount = 0;
unsigned int ui_event_counter = 0;
unsigned int ui_number_of_events = 0;
static UI_EVENT *   EventBuffer = NULL;
static int          Record = 0;
static int          RecordFlags = 0;

static unsigned char SavedState[256];

static int PlaybackSpeed = 1;

// 1=1x faster, 2=2x faster, etc
void ui_set_playback_speed( int speed )
{
	PlaybackSpeed = speed;
}

int ui_record_events( int NumberOfEvents, UI_EVENT * buffer, int Flags )
{
	if ( Record > 0 || buffer==NULL ) return 1;

	RecordFlags = Flags;
	EventBuffer = buffer;
	ui_event_counter = 0;
	FrameCount = 0;
	ui_number_of_events = NumberOfEvents;
	Record = 1;
	return 0;
}

int ui_play_events_realtime( int NumberOfEvents, UI_EVENT * buffer )
{	int i;
	if ( buffer == NULL ) return 1;

	EventBuffer = buffer;
	FrameCount = 0;
	ui_event_counter = 0;
	ui_number_of_events = NumberOfEvents;
	Record = 2;
	_disable();
	keyd_last_released= 0;
	keyd_last_pressed= 0;
	for (i=0; i<256; i++ )
		SavedState[i] = keyd_pressed[i];
	_enable();
	key_flush();
	return 0;
}

int ui_play_events_fast( int NumberOfEvents, UI_EVENT * buffer )
{
	int i;
	if ( buffer == NULL ) return 1;

	EventBuffer = buffer;
	FrameCount = 0;
	ui_event_counter = 0;
	ui_number_of_events = NumberOfEvents;
	Record = 3;
	_disable();
	keyd_last_released= 0;
	keyd_last_pressed= 0;

	for (i=0; i<256; i++ )
	{
		SavedState[i] = keyd_pressed[i];
	}
	_enable();
	key_flush();
	return 0;
}

// Returns:  0=Normal, 1=Recording, 2=Playback normal, 3=Playback fast
int ui_recorder_status()
{
	return Record;
}

void ui_dialog_draw(UI_DIALOG *dlg)
{
	int x, y, w, h;
	int req_w, req_h;

	x = D_X;
	y = D_Y;
	w = D_WIDTH;
	h = D_HEIGHT;

	req_w = w;
	req_h = h;
	
	if (dlg->flags & DF_BORDER)
	{
		req_w -= 2*BORDER_WIDTH;
		req_h -= 2*BORDER_WIDTH;
		
		gr_set_current_canvas( NULL );
		ui_draw_frame( x, y, x+w-1, y+h-1 );
	}
	
	ui_dialog_set_current_canvas(dlg);
	
	if (dlg->flags & DF_FILLED)
		ui_draw_box_out( 0, 0, req_w-1, req_h-1 );
	
	gr_set_fontcolor( CBLACK, CWHITE );
	
	D_TEXT_X = 0;
	D_TEXT_Y = 0;
}


// The dialog handler borrows heavily from the newmenu_handler
int ui_dialog_handler(window *wind, d_event *event, UI_DIALOG *dlg)
{
	int rval = 0;

	if (event->type == EVENT_WINDOW_CLOSED ||
		event->type == EVENT_WINDOW_ACTIVATED ||
		event->type == EVENT_WINDOW_DEACTIVATED)
		return 0;
	
	if (dlg->callback)
		if ((*dlg->callback)(dlg, event, dlg->userdata))
			return 1;		// event handled

	if (!window_exists(wind))
		return 1;

	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_MOVED:
			/*return*/ ui_dialog_do_gadgets(dlg, event);
			if (!window_exists(wind))
				return 1;

			rval = mouse_in_window(dlg->wind);
			break;
			
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
			rval = ui_dialog_do_gadgets(dlg, event);
			break;

		case EVENT_IDLE:
			timer_delay2(50);
			rval = ui_dialog_do_gadgets(dlg, event);
			break;
			
		case EVENT_WINDOW_DRAW:
		{
			d_event event2 = { EVENT_UI_DIALOG_DRAW };
			ui_dialog_draw(dlg);
			rval = ui_dialog_do_gadgets(dlg, event);
			window_send_event(wind, &event2);
			break;
		}

		case EVENT_WINDOW_CLOSE:
			ui_gadget_delete_all(dlg);
			selected_gadget = NULL;
			d_free( dlg );
			break;
			
		default:
			break;
	}
	
	return rval;
}

UI_DIALOG * ui_create_dialog( short x, short y, short w, short h, enum dialog_flags flags, int (*callback)(UI_DIALOG *, d_event *, void *), void *userdata )
{
	UI_DIALOG	*dlg;
	int sw, sh, req_w, req_h;

	dlg = (UI_DIALOG *) d_malloc(sizeof(UI_DIALOG));
	if (dlg==NULL) Error("Could not create dialog: Out of memory");

	sw = grd_curscreen->sc_w;
	sh = grd_curscreen->sc_h;

	//mouse_set_limits(0, 0, sw - 1, sh - 1);

	req_w = w;
	req_h = h;
	
	dlg->flags = flags;

	if (flags & DF_BORDER)
	{
		x -= BORDER_WIDTH;
		y -= BORDER_WIDTH;
		w += 2*BORDER_WIDTH;
		h += 2*BORDER_WIDTH;
	}

	if ( x < 0 ) x = 0;
	if ( (x+w-1) >= sw ) x = sw - w;
	if ( y < 0 ) y = 0;
	if ( (y+h-1) >= sh ) y = sh - h;

	D_X = x;
	D_Y = y;
	D_WIDTH = w;
	D_HEIGHT = h;
	D_GADGET = NULL;
	dlg->keyboard_focus_gadget = NULL;
	selected_gadget = NULL;

	dlg->callback = callback;
	dlg->userdata = userdata;
	dlg->wind = window_create(&grd_curscreen->sc_canvas,
						 x + ((flags & DF_BORDER) ? BORDER_WIDTH : 0),
						 y + ((flags & DF_BORDER) ? BORDER_WIDTH : 0),
						 req_w, req_h, (int (*)(window *, d_event *, void *)) ui_dialog_handler, dlg);
	
	if (!dlg->wind)
	{
		d_free(dlg);
		return NULL;
	}

	if (!(flags & DF_MODAL))
		window_set_modal(dlg->wind, 0);	// make this window modeless, allowing events to propogate through the window stack

	return dlg;

}

window *ui_dialog_get_window(UI_DIALOG *dlg)
{
	return dlg->wind;
}

void ui_dialog_set_current_canvas(UI_DIALOG *dlg)
{
	gr_set_current_canvas(window_get_canvas(dlg->wind));
}

void ui_close_dialog( UI_DIALOG * dlg )
{
	window_close(dlg->wind);
}

void restore_state()
{
	int i;
	_disable();
	for (i=0; i<256; i++ )
	{
		keyd_pressed[i] = SavedState[i];
	}
	_enable();
}


#if 0
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

void ui_dprintf( UI_DIALOG * dlg, char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);

	ui_dialog_set_current_canvas( dlg );

	D_TEXT_X = gr_string( D_TEXT_X, D_TEXT_Y, buffer );
}

void ui_dprintf_at( UI_DIALOG * dlg, short x, short y, char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsprintf(buffer,format,args);

	ui_dialog_set_current_canvas( dlg );

	gr_string( x, y, buffer );
}
