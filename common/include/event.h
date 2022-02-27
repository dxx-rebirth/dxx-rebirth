/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
// Event header file

#pragma once

#include <SDL_version.h>
#include "fwd-event.h"
#include "maths.h"

#if SDL_MAJOR_VERSION == 2
#include <SDL_video.h>
#endif

#ifdef __cplusplus
namespace dcx {

enum event_type : unsigned
{
	EVENT_IDLE = 0,
	EVENT_QUIT,

#if DXX_MAX_BUTTONS_PER_JOYSTICK
	EVENT_JOYSTICK_BUTTON_DOWN,
	EVENT_JOYSTICK_BUTTON_UP,
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
	EVENT_JOYSTICK_MOVED,
#endif

	EVENT_MOUSE_BUTTON_DOWN,
	EVENT_MOUSE_BUTTON_UP,
	EVENT_MOUSE_DOUBLE_CLICKED,
	EVENT_MOUSE_MOVED,

	EVENT_KEY_COMMAND,
	EVENT_KEY_RELEASE,

	EVENT_WINDOW_CREATED,
#if SDL_MAJOR_VERSION == 2
	EVENT_WINDOW_RESIZE,
#endif
	EVENT_WINDOW_ACTIVATED,
	EVENT_WINDOW_DEACTIVATED,
	EVENT_WINDOW_DRAW,
	EVENT_WINDOW_CLOSE,

	EVENT_NEWMENU_DRAW,					// draw after the newmenu stuff is drawn (e.g. savegame previews)
	EVENT_NEWMENU_CHANGED,				// an item had its value/text changed
	EVENT_NEWMENU_SELECTED,				// user chose something - pressed enter/clicked on it

	EVENT_LOOP_BEGIN_LOOP,
	EVENT_LOOP_END_LOOP,

	EVENT_UI_DIALOG_DRAW,				// draw after the dialog stuff is drawn (e.g. spinning robots)
	EVENT_UI_GADGET_PRESSED,				// user 'pressed' a gadget
	EVENT_UI_LISTBOX_MOVED,
	EVENT_UI_LISTBOX_SELECTED,
	EVENT_UI_USERBOX_DRAGGED
};

enum class window_event_result : uint8_t
{
	// Window ignored event.  Bubble up.
	ignored,
	// Window handled event.
	handled,
	close,
	// Window handler already deleted window (most likely because it was subclassed), don't attempt to re-delete
	deleted,
};

// A vanilla event. Cast to the correct type of event according to 'type'.
struct d_event
{
	const event_type type;
	constexpr d_event(const event_type t) :
		type(t)
	{
	}
};

struct d_create_event : d_event
{
	constexpr d_create_event() :
		d_event{EVENT_WINDOW_CREATED}
	{
	}
};

struct d_change_event : d_event
{
	const int citem;
	constexpr d_change_event(const int c) :
		d_event{EVENT_NEWMENU_CHANGED}, citem(c)
	{
	}
};

struct d_select_event : d_event
{
	int citem;
	d_select_event(const int c) :
		d_event{EVENT_NEWMENU_SELECTED}, citem(c)
	{
	}
};

#if SDL_MAJOR_VERSION == 2
struct d_window_size_event : d_event
{
	Sint32 width;
	Sint32 height;
	d_window_size_event(const Sint32 w, const Sint32 h) :
		d_event{EVENT_WINDOW_RESIZE}, width(w), height(h)
	{
	}
};
#endif

struct d_event_begin_loop : d_event
{
	d_event_begin_loop() :
		d_event{EVENT_LOOP_BEGIN_LOOP}
	{
	}
};

struct d_event_end_loop : d_event
{
	d_event_end_loop() :
		d_event{EVENT_LOOP_END_LOOP}
	{
	}
};

#if DXX_USE_EDITOR
fix event_get_idle_seconds();
#endif

// Process all events until the front window is deleted
// Won't work if there's the possibility of another window on top
// without its own event loop
static inline void event_process_all(void)
{
	while (event_process() != window_event_result::deleted) {}
}

}
#endif
