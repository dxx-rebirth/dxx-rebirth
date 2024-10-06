/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#include "dxxerror.h"
#include "args.h"
#include "gr.h"

#include "d_underlying_value.h"

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
	enumerated_array<fix64, MOUSE_MAX_BUTTONS, mbtn> time_lastpressed;
};

static mouseinfo Mouse;

}

d_event_mousebutton::d_event_mousebutton(const event_type etype, const mbtn b) :
	d_event{etype}, button{b}
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

namespace {

static window_event_result maybe_send_z_move(const mbtn button)
{
	short dz;
	if (button == mbtn::z_up)
	{
		Mouse.delta_z += Z_SENSITIVITY;
		Mouse.z += Z_SENSITIVITY;
		dz = Z_SENSITIVITY;
	}
	else if (button == mbtn::z_down)
	{
		Mouse.delta_z -= Z_SENSITIVITY;
		Mouse.z -= Z_SENSITIVITY;
		dz = -1*Z_SENSITIVITY;
	}
	else
		return window_event_result::ignored;
	return event_send(d_event_mouse_moved{event_type::mouse_moved, 0, 0, dz});
}

static window_event_result send_singleclick(const bool pressed, const mbtn button)
{
	const d_event_mousebutton event{pressed ? event_type::mouse_button_down : event_type::mouse_button_up, button};
	con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_BUTTON_%s, button %d, coords %d,%d,%d",
			   pressed ? "DOWN" : "UP", underlying_value(event.button), Mouse.x, Mouse.y, Mouse.z);
	return event_send(event);
}

static window_event_result maybe_send_doubleclick(const fix64 now, const mbtn button)
{
	auto &when = Mouse.time_lastpressed[button];
	const auto then{when};
	when = now;
	if (now > then + F1_0/5)
		return window_event_result::ignored;
	const d_event_mousebutton event{event_type::mouse_double_clicked, button};
	con_printf(CON_DEBUG, "Sending event event_type::mouse_double_clicked, button %d, coords %d,%d", underlying_value(button), Mouse.x, Mouse.y);
	return event_send(event);
}

}

window_event_result mouse_button_handler(const SDL_MouseButtonEvent *const mbe)
{
	window_event_result highest_result(window_event_result::ignored);

	if (unlikely(CGameArg.CtlNoMouse))
		return window_event_result::ignored;
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	static constexpr std::array<mbtn, 17> button_remap{{
		mbtn::left,
		mbtn::middle,
		mbtn::right,
		mbtn::z_up,
		mbtn::z_down,
		mbtn::pitch_backward,
		mbtn::pitch_forward,
		mbtn::bank_left,
		mbtn::bank_right,
		mbtn::head_left,
		mbtn::head_right,
		mbtn::_11,
		mbtn::_12,
		mbtn::_13,
		mbtn::_14,
		mbtn::_15,
		mbtn::_16,
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

window_event_result mouse_motion_handler(const SDL_MouseMotionEvent *const mme)
{
	Mouse.cursor_time = timer_query();
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
	
	// z handled in mouse_button_handler
	const d_event_mouse_moved event{event_type::mouse_moved, mme->xrel, mme->yrel, 0};
	
	//con_printf(CON_DEBUG, "Sending event event_type::mouse_moved, relative motion %d,%d,%d",
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
std::tuple<int, int, int> mouse_get_pos()
{
	//event_poll();		// Have to assume this is called in event_process, because event_poll can cause a window to close (depending on what the user does)
	return {
		Mouse.x,
		Mouse.y,
		Mouse.z,
	};
}

window_event_result mouse_in_window(window *wind)
{
	auto &canv = wind->w_canv;
	return	(static_cast<unsigned>(Mouse.x) - canv.cv_bitmap.bm_x <= canv.cv_bitmap.bm_w) &&
			(static_cast<unsigned>(Mouse.y) - canv.cv_bitmap.bm_y <= canv.cv_bitmap.bm_h) ? window_event_result::handled : window_event_result::ignored;
}

#if 0
std::tuple<int, int, int> mouse_get_delta()
{
	const int dz{Mouse.delta_z};
	int dx{}, dy{};
	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
	SDL_GetRelativeMouseState(&dx, &dy);
	return {
		dx,
		dy,
		dz,
	};
}
#endif

namespace {

template <bool noactivate>
static void mouse_change_cursor()
{
	Mouse.cursor_enabled = (!noactivate && !CGameArg.CtlNoMouse && !CGameArg.CtlNoCursor);
	if (!Mouse.cursor_enabled)
		SDL_ShowCursor(SDL_DISABLE);
}

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
