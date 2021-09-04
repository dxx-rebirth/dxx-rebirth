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

namespace dcx {

namespace {

struct event_poll_state
{
	uint8_t clean_uniframe = 1;
	const window *const front_window = window_get_front();
	window_event_result highest_result = window_event_result::ignored;
	void process_event_batch(partial_range_t<const SDL_Event *>);
};

}

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

static void event_notify_begin_loop()
{
	const d_event_begin_loop event;
	event_send(event);
}

window_event_result event_poll()
{
	event_poll_state state;
	event_notify_begin_loop();

	for (;;)
	{
	// If the front window changes, exit this loop, otherwise unintended behavior can occur
	// like pressing 'Return' really fast at 'Difficulty Level' causing multiple games to be started
		if (state.front_window != window_get_front())
			break;
		std::array<SDL_Event, 128> events;

		SDL_PumpEvents();
#if SDL_MAJOR_VERSION == 1
		const auto peep = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, SDL_ALLEVENTS);
#elif SDL_MAJOR_VERSION == 2
		const auto peep = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
#endif
		if (peep <= 0)
			break;
		state.process_event_batch(unchecked_partial_range(events, static_cast<unsigned>(peep)));
		if (state.highest_result == window_event_result::deleted)
			break;
	}
	// Send the idle event if there were no other events (or they were ignored)
	if (state.highest_result == window_event_result::ignored)
	{
		const d_event ievent{EVENT_IDLE};
		state.highest_result = std::max(event_send(ievent), state.highest_result);
	}
	else
	{
#if DXX_USE_EDITOR
		event_reset_idle_seconds();
#endif
	}
	mouse_cursor_autohide();
	return state.highest_result;
}

void event_poll_state::process_event_batch(partial_range_t<const SDL_Event *> events)
{
	for (auto &&event : events)
	{
		window_event_result result;
		switch(event.type) {
#if SDL_MAJOR_VERSION == 2
			case SDL_WINDOWEVENT:
				windowevent_handler(event.window);
				continue;
#endif
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (clean_uniframe)
				{
					clean_uniframe=0;
					unicode_frame_buffer = {};
				}
				result = key_handler(&event.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (CGameArg.CtlNoMouse)
					continue;
				result = mouse_button_handler(&event.button);
				break;
			case SDL_MOUSEMOTION:
				if (CGameArg.CtlNoMouse)
					continue;
				result = mouse_motion_handler(&event.motion);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				if (CGameArg.CtlNoJoystick)
					continue;
				result = joy_button_handler(&event.jbutton);
				break;
			case SDL_JOYAXISMOTION:
				if (CGameArg.CtlNoJoystick)
					continue;
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
				highest_result = std::max(joy_axisbutton_handler(&event.jaxis), highest_result);
#endif
				result = joy_axis_handler(&event.jaxis);
				break;
			case SDL_JOYHATMOTION:
				if (CGameArg.CtlNoJoystick)
					continue;
				result = joy_hat_handler(&event.jhat);
				break;
			case SDL_JOYBALLMOTION:
				continue;
			case SDL_QUIT: {
				d_event qevent = { EVENT_QUIT };
				result = call_default_handler(qevent);
				break;
			}
			default:
				continue;
		}
		highest_result = std::max(result, highest_result);
	}
}

void event_flush()
{
	std::array<SDL_Event, 128> events;
	for (;;)
	{
		SDL_PumpEvents();
#if SDL_MAJOR_VERSION == 1
		const auto peep = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, SDL_ALLEVENTS);
#elif SDL_MAJOR_VERSION == 2
		const auto peep = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
#endif
		if (peep != events.size())
			break;
	}
}

int event_init()
{
	// We should now be active and responding to events.
	return 0;
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
		if (window_is_visible(*wind))
		{
			handled = window_send_event(*wind, event);

			if (handled == window_event_result::deleted) // break away if necessary: window_send_event() could have closed wind by now
				break;
			if (window_is_modal(*wind))
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

	const d_event event{EVENT_WINDOW_DRAW};	// then draw all visible windows
	for (wind = window_get_first(); wind != nullptr;)
	{
		if (window_is_visible(*wind))
		{
			auto prev = window_get_prev(*wind);
			auto result = window_send_event(*wind, event);
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

	gr_flip();

	return highest_result;
}

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
