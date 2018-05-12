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
#include "gr.h"

namespace dcx {

namespace {

struct flushable_mouseinfo
{
	int    delta_x, delta_y, delta_z;
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

d_event_mousebutton::d_event_mousebutton(const event_type etype, const unsigned b) :
	d_event{etype}, button(b)
{
}

void mouse_init(void)
{
	Mouse = {};
}

void mouse_close(void)
{
	SDL_ShowCursor(SDL_ENABLE);
}

static window_event_result maybe_send_z_move(const unsigned button)
{
	short dz;
	if (button == MBTN_Z_UP)
	{
		Mouse.delta_z += Z_SENSITIVITY;
		Mouse.z += Z_SENSITIVITY;
		dz = Z_SENSITIVITY;
	}
	else if (button == MBTN_Z_DOWN)
	{
		Mouse.delta_z -= Z_SENSITIVITY;
		Mouse.z -= Z_SENSITIVITY;
		dz = -1*Z_SENSITIVITY;
	}
	else
		return window_event_result::ignored;
	const d_event_mouse_moved event{EVENT_MOUSE_MOVED, 0, 0, dz};
	return event_send(event);
}

static window_event_result send_singleclick(const bool pressed, const unsigned button)
{
	const d_event_mousebutton event{pressed ? EVENT_MOUSE_BUTTON_DOWN : EVENT_MOUSE_BUTTON_UP, button};
	con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_BUTTON_%s, button %d, coords %d,%d,%d",
			   pressed ? "DOWN" : "UP", event.button, Mouse.x, Mouse.y, Mouse.z);
	return event_send(event);
}

static window_event_result maybe_send_doubleclick(const fix64 now, const unsigned button)
{
	auto &when = Mouse.time_lastpressed[button];
	const auto then = when;
	when = now;
	if (now > then + F1_0/5)
		return window_event_result::ignored;
	const d_event_mousebutton event{EVENT_MOUSE_DOUBLE_CLICKED, button};
	con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_DOUBLE_CLICKED, button %d, coords %d,%d", button, Mouse.x, Mouse.y);
	return event_send(event);
}

window_event_result mouse_button_handler(SDL_MouseButtonEvent *mbe)
{
	window_event_result highest_result(window_event_result::ignored);

	if (unlikely(CGameArg.CtlNoMouse))
		return window_event_result::ignored;
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
	const unsigned button_idx = mbe->button - 1; // -1 since SDL seems to start counting at 1
	if (unlikely(button_idx >= button_remap.size()))
		return window_event_result::ignored;

	const auto now = timer_query();
	const auto button = button_remap[button_idx];
	const auto mbe_state = mbe->state;
	Mouse.cursor_time = now;

	const auto pressed = mbe_state != SDL_RELEASED;
	if (pressed) {
		highest_result = maybe_send_z_move(button);
	}
	highest_result = std::max(send_singleclick(pressed, button), highest_result);
	//Double-click support
	if (pressed)
	{
		highest_result = std::max(maybe_send_doubleclick(now, button), highest_result);
	}

	return highest_result;
}

window_event_result mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
	Mouse.cursor_time = timer_query();
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
	
	// z handled in mouse_button_handler
	const d_event_mouse_moved event{EVENT_MOUSE_MOVED, mme->xrel, mme->yrel, 0};
	
	//con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_MOVED, relative motion %d,%d,%d",
	//		   event.dx, event.dy, event.dz);
	return event_send(event);
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
	*dz = Mouse.delta_z;
	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
	SDL_GetRelativeMouseState(dx, dy);
}

template <bool noactivate>
static void mouse_change_cursor()
{
	Mouse.cursor_enabled = (!noactivate && !CGameArg.CtlNoMouse && !CGameArg.CtlNoCursor);
	if (!Mouse.cursor_enabled)
		SDL_ShowCursor(SDL_DISABLE);
}

void mouse_enable_cursor()
{
	mouse_change_cursor<false>();
}

void mouse_disable_cursor()
{
	mouse_change_cursor<true>();
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

}
