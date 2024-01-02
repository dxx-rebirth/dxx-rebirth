/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL Event related stuff
 *
 *
 */

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "event.h"
#include "key.h"
#include "mouse.h"
#include "window.h"
#include "timer.h"
#include "cmd.h"
#include "config.h"
#include "inferno.h"

#include "joy.h"
#include "args.h"
#include "partial_range.h"
#include "backports-ranges.h"

namespace dcx {

#if SDL_MAJOR_VERSION == 2
extern SDL_Window *g_pRebirthSDLMainWindow;

static void windowevent_handler(const SDL_WindowEvent &windowevent)
{
	switch (windowevent.event)
	{
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			{
				const d_window_size_event e{windowevent.data1, windowevent.data2};
				event_send(e);
				break;
			}
	}
}
#endif

window_event_result event_poll()
{
	window_event_result result = window_event_result::ignored;

	event_send(d_event_begin_loop{});

	SDL_Event event;
    while (SDL_PollEvent(&event)) {
		switch(event.type) {
#if SDL_MAJOR_VERSION == 2
			case SDL_WINDOWEVENT:
				windowevent_handler(event.window);
				break;
#endif
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				result = key_handler(&event.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (!CGameArg.CtlNoMouse)
					result = mouse_button_handler(&event.button);
				break;
			case SDL_MOUSEMOTION:
				if (!CGameArg.CtlNoMouse)
					result = mouse_motion_handler(&event.motion);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				if (!CGameArg.CtlNoJoystick)
					result = joy_button_handler(&event.jbutton);
				break;
			case SDL_JOYAXISMOTION:
				if (!CGameArg.CtlNoJoystick)
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
					result = joy_axisbutton_handler(&event.jaxis);
#endif
					result = joy_axis_handler(&event.jaxis);
				break;
			case SDL_JOYHATMOTION:
				if (!CGameArg.CtlNoJoystick)
					result = joy_hat_handler(&event.jhat);
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				result = call_default_handler(d_event{event_type::quit});
				break;
			}
			default:
				break;
		}
	}
	event_send(d_event_end_loop{});

	if (result == window_event_result::deleted)
		return result;
	
	// Send the idle event when all events are processed
	const d_event ievent{event_type::idle};
	result = event_send(ievent);
	
#if DXX_USE_EDITOR
	event_reset_idle_seconds();
#endif
	mouse_cursor_autohide();
	return result;
}

window_event_result call_default_handler(const d_event &event)
{
	return standard_handler(event);
}

window_event_result event_send(const d_event &event)
{
	window *wind;
	window_event_result handled = window_event_result::ignored;

	for (wind = window_get_front(); wind && handled == window_event_result::ignored; wind = window_get_prev(*wind))
		if (wind->is_visible())
		{
			handled = wind->send_event(event);

			if (handled == window_event_result::deleted) // break away if necessary: window_send_event() could have closed wind by now
				break;
			if (wind->is_modal())
				break;
		}
	
	if (handled == window_event_result::ignored)
		return call_default_handler(event);

	return handled;
}

// Process the first event in queue, sending to the appropriate handler
// This is the new object-oriented system
// Uses the old system for now, but this may change
window_event_result event_process(void)
{
    const Uint32 frameStart = SDL_GetTicks();
	
	window *wind = window_get_front();
	window_event_result highest_result;

	timer_update();

	highest_result = event_poll();	// send input events first

	cmd_queue_process();

	// Doing this prevents problems when a draw event can create a newmenu,
	// such as some network menus when they report a problem
	// Also checking for window_event_result::deleted in case a window was created
	// with the same pointer value as the deleted one
	if ((highest_result == window_event_result::deleted) || (window_get_front() != wind))
		return highest_result;

	const d_event event{event_type::window_draw};	// then draw all visible windows
	for (wind = window_get_first(); wind != nullptr;)
	{
		if (wind->is_visible())
		{
			auto prev = window_get_prev(*wind);
			auto result = wind->send_event(event);
			highest_result = std::max(result, highest_result);
			if (result == window_event_result::deleted)
			{
				if (!prev)
				{
					wind = window_get_first();
					continue;
				}
				wind = prev;	// take the previous window and get the next one from that (if prev isn't nullptr)
			}
		}
		wind = window_get_next(*wind);
	}

	gr_flip(frameStart);

	return highest_result;
}

namespace {

template <bool activate_focus>
static void event_change_focus()
{
	const auto enable_grab = activate_focus && CGameCfg.Grabinput && likely(!CGameArg.DbgForbidConsoleGrab);
#if SDL_MAJOR_VERSION == 1
	SDL_WM_GrabInput(enable_grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
#elif SDL_MAJOR_VERSION == 2
	SDL_SetWindowGrab(g_pRebirthSDLMainWindow, enable_grab ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(enable_grab ? SDL_TRUE : SDL_FALSE);
#endif
	if (activate_focus)
		mouse_disable_cursor();
	else
		mouse_enable_cursor();
}

}

void event_enable_focus()
{
	event_change_focus<true>();
}

void event_disable_focus()
{
	event_change_focus<false>();
}

#if DXX_USE_EDITOR
static fix64 last_event = 0;

void event_reset_idle_seconds()
{
	last_event = timer_query();
}

fix event_get_idle_seconds()
{
	return (timer_query() - last_event)/F1_0;
}
#endif

}
