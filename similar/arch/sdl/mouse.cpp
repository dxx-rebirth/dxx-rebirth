/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL mouse driver
 *
 */

#include <string.h>
#include <SDL.h>

#include "maths.h"
#include "timer.h"
#include "event.h"
#include "window.h"
#include "mouse.h"
#include "playsave.h"
#include "dxxerror.h"
#include "args.h"

namespace {

struct flushable_mouseinfo
{
	array<uint8_t, MOUSE_MAX_BUTTONS> button_state;
	int    delta_x, delta_y, delta_z, old_delta_x, old_delta_y;
	int z;
};

struct mouseinfo : flushable_mouseinfo
{
	int    x,y;
	int    cursor_enabled;
	fix64  cursor_time;
	array<fix64, MOUSE_MAX_BUTTONS> time_lastpressed;
};

}

static mouseinfo Mouse;

struct d_event_mousebutton : d_event
{
	int button;
};

struct d_event_mouse_moved : d_event
{
	short		dx, dy, dz;
};

void mouse_init(void)
{
	Mouse = {};
}

void mouse_close(void)
{
	SDL_ShowCursor(SDL_ENABLE);
}

void mouse_button_handler(SDL_MouseButtonEvent *mbe)
{
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	static const array<int, 17> button_remap{{
		MBTN_LEFT,
		MBTN_MIDDLE,
		MBTN_RIGHT,
		MBTN_Z_UP,
		MBTN_Z_DOWN,
		MBTN_PITCH_BACKWARD,
		MBTN_PITCH_FORWARD,
		MBTN_BANK_LEFT,
		MBTN_BANK_RIGHT,
		MBTN_HEAD_LEFT,
		MBTN_HEAD_RIGHT,
		MBTN_11,
		MBTN_12,
		MBTN_13,
		MBTN_14,
		MBTN_15,
		MBTN_16
	}};

	int button = button_remap[mbe->button - 1]; // -1 since SDL seems to start counting at 1
	d_event_mousebutton event;

	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();

	if (mbe->state == SDL_PRESSED) {
		d_event_mouse_moved event2{};
		event2.type = EVENT_MOUSE_MOVED;

		Mouse.button_state[button] = 1;

		if (button == MBTN_Z_UP) {
			Mouse.delta_z += Z_SENSITIVITY;
			Mouse.z += Z_SENSITIVITY;
			event2.dz = Z_SENSITIVITY;
		} else if (button == MBTN_Z_DOWN) {
			Mouse.delta_z -= Z_SENSITIVITY;
			Mouse.z -= Z_SENSITIVITY;
			event2.dz = -1*Z_SENSITIVITY;
		}
		
		if (event2.dz)
		{
			//con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_MOVED, relative motion %d,%d,%d",
			//		   event2.dx, event2.dy, event2.dz);
			event_send(event2);
		}
	} else {
		Mouse.button_state[button] = 0;
	}
	
	event.type = (mbe->state == SDL_PRESSED) ? EVENT_MOUSE_BUTTON_DOWN : EVENT_MOUSE_BUTTON_UP;
	event.button = button;
	
	con_printf(CON_DEBUG, "Sending event %s, button %d, coords %d,%d,%d",
			   (mbe->state == SDL_PRESSED) ? "EVENT_MOUSE_BUTTON_DOWN" : "EVENT_MOUSE_BUTTON_UP", event.button, Mouse.x, Mouse.y, Mouse.z);
	event_send(event);
	
	//Double-click support
	if (Mouse.button_state[button])
	{
		if (timer_query() <= Mouse.time_lastpressed[button] + F1_0/5)
		{
			event.type = EVENT_MOUSE_DOUBLE_CLICKED;
			//event.button = button; // already set the button
			con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_DOUBLE_CLICKED, button %d, coords %d,%d",
					   event.button, Mouse.x, Mouse.y);
			event_send(event);
		}

		Mouse.time_lastpressed[button] = Mouse.cursor_time;
	}
}

void mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
	d_event_mouse_moved event;
	
	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
	
	event.type = EVENT_MOUSE_MOVED;
	event.dx = mme->xrel;
	event.dy = mme->yrel;
	event.dz = 0;		// handled in mouse_button_handler
	
	Mouse.old_delta_x = event.dx;
	Mouse.old_delta_y = event.dy;
	
	//con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_MOVED, relative motion %d,%d,%d",
	//		   event.dx, event.dy, event.dz);
	event_send(event);
}

void mouse_flush()	// clears all mice events...
{
//	event_poll();
	static_cast<flushable_mouseinfo &>(Mouse) = {};
	SDL_GetMouseState(&Mouse.x, &Mouse.y); // necessary because polling only gives us the delta.
}

//========================================================================
void mouse_get_pos( int *x, int *y, int *z )
{
	//event_poll();		// Have to assume this is called in event_process, because event_poll can cause a window to close (depending on what the user does)
	*x=Mouse.x;
	*y=Mouse.y;
	*z=Mouse.z;
}

window_event_result mouse_in_window(window *wind)
{
	auto &canv = window_get_canvas(*wind);
	return	(static_cast<unsigned>(Mouse.x) - canv.cv_bitmap.bm_x <= canv.cv_bitmap.bm_w) &&
			(static_cast<unsigned>(Mouse.y) - canv.cv_bitmap.bm_y <= canv.cv_bitmap.bm_h) ? window_event_result::handled : window_event_result::ignored;
}

void mouse_get_delta( int *dx, int *dy, int *dz )
{
	SDL_GetRelativeMouseState( &Mouse.delta_x, &Mouse.delta_y );
	*dx = Mouse.delta_x;
	*dy = Mouse.delta_y;
	*dz = Mouse.delta_z;

	Mouse.old_delta_x = *dx;
	Mouse.old_delta_y = *dy;

	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
}

void event_mouse_get_delta(const d_event &event, int *dx, int *dy, int *dz)
{
	auto &e = static_cast<const d_event_mouse_moved &>(event);
	Assert(e.type == EVENT_MOUSE_MOVED);
	*dx = e.dx;
	*dy = e.dy;
	*dz = e.dz;
}

int event_mouse_get_button(const d_event &event)
{
	auto &e = static_cast<const d_event_mousebutton &>(event);
	Assert(e.type == EVENT_MOUSE_BUTTON_DOWN || e.type == EVENT_MOUSE_BUTTON_UP);
	return e.button;
}

void mouse_toggle_cursor(int activate)
{
	Mouse.cursor_enabled = (activate && !GameArg.CtlNoMouse && !GameArg.CtlNoCursor);
	if (!Mouse.cursor_enabled)
		SDL_ShowCursor(SDL_DISABLE);
}

// If we want to display/hide cursor, do so if not already and also hide it automatically after some time.
void mouse_cursor_autohide()
{
	static fix64 hidden_time = 0;

	const auto is_showing = SDL_ShowCursor(SDL_QUERY);
	int result;
	if (Mouse.cursor_enabled)
	{
		const auto now = timer_query();
		const auto cursor_time = Mouse.cursor_time;
		const auto recent_cursor_time = cursor_time + (F1_0*2) >= now;
		if (is_showing)
		{
			if (recent_cursor_time)
				return;
			hidden_time = now;
			result = SDL_DISABLE;
		}
		else
		{
			if (!(recent_cursor_time && hidden_time + (F1_0/2) < now))
				return;
			result = SDL_ENABLE;
		}
	}
	else
	{
		if (!is_showing)
			return;
		result = SDL_DISABLE;
	}
	SDL_ShowCursor(result);
}
