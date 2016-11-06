/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
#include "config.h"
#include "inferno.h"

#include "joy.h"
#include "args.h"

namespace dcx {

window_event_result event_poll()
{
	SDL_Event event;
	int clean_uniframe=1;
	window *wind = window_get_front();
	window_event_result highest_result(window_event_result::ignored);
	
	// If the front window changes, exit this loop, otherwise unintended behavior can occur
	// like pressing 'Return' really fast at 'Difficulty Level' causing multiple games to be started
	while ((highest_result != window_event_result::deleted) && (wind == window_get_front()) && (event = {}, SDL_PollEvent(&event)))
	{
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (clean_uniframe)
					unicode_frame_buffer = {};
				clean_uniframe=0;
				highest_result = std::max(key_handler(&event.key), highest_result);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if (CGameArg.CtlNoMouse)
					break;
				highest_result = std::max(mouse_button_handler(&event.button), highest_result);
				break;
			case SDL_MOUSEMOTION:
				if (CGameArg.CtlNoMouse)
					break;
				highest_result = std::max(mouse_motion_handler(&event.motion), highest_result);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				if (CGameArg.CtlNoJoystick)
					break;
				highest_result = std::max(joy_button_handler(&event.jbutton), highest_result);
				break;
			case SDL_JOYAXISMOTION:
				if (CGameArg.CtlNoJoystick)
					break;
				highest_result = std::max(joy_axis_handler(&event.jaxis), highest_result);
				break;
			case SDL_JOYHATMOTION:
				if (CGameArg.CtlNoJoystick)
					break;
				highest_result = std::max(joy_hat_handler(&event.jhat), highest_result);
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				d_event qevent = { EVENT_QUIT };
				highest_result = std::max(call_default_handler(qevent), highest_result);
			} break;
		}
	}

	// Send the idle event if there were no other events (or they were ignored)
	if (highest_result == window_event_result::ignored)
	{
		d_event ievent;
		
		ievent.type = EVENT_IDLE;
		highest_result = std::max(event_send(ievent), highest_result);
	}
	else
		event_reset_idle_seconds();
	
	mouse_cursor_autohide();

	return highest_result;
}

void event_flush()
{
	SDL_Event event;
	
	while (SDL_PollEvent(&event));
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
		if (window_is_visible(wind))
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
	d_event event;
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
	
	event.type = EVENT_WINDOW_DRAW;	// then draw all visible windows
	wind = window_get_first();
	while (wind != NULL)
	{
		window *prev = window_get_prev(*wind);
		if (window_is_visible(wind))
		{
			auto result = window_send_event(*wind, event);
			if (result == window_event_result::deleted)
			{
				if (!prev) // well there isn't a previous window ...
					break; // ... just bail out - we've done everything for this frame we can.
				wind = window_get_next(*prev); // the current window seemed to be closed. so take the next one from the previous which should be able to point to the one after the current closed
			}
			else
				wind = window_get_next(*wind);

			highest_result = std::max(result, highest_result);
		}
	}

	gr_flip();

	return highest_result;
}

template <bool activate_focus>
static void event_change_focus()
{
	SDL_WM_GrabInput(activate_focus && CGameCfg.Grabinput && likely(!CGameArg.DbgForbidConsoleGrab) ? SDL_GRAB_ON : SDL_GRAB_OFF);
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

static fix64 last_event = 0;

void event_reset_idle_seconds()
{
	last_event = timer_query();
}

fix event_get_idle_seconds()
{
	return (timer_query() - last_event)/F1_0;
}

}
