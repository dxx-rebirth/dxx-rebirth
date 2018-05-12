/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
// Event header file

#pragma once

#include "fwd-event.h"
#include "maths.h"

#ifdef __cplusplus
namespace dcx {

enum event_type : unsigned
{
	EVENT_IDLE = 0,
	EVENT_QUIT,

	EVENT_JOYSTICK_BUTTON_DOWN,
	EVENT_JOYSTICK_BUTTON_UP,
	EVENT_JOYSTICK_MOVED,

	EVENT_MOUSE_BUTTON_DOWN,
	EVENT_MOUSE_BUTTON_UP,
	EVENT_MOUSE_DOUBLE_CLICKED,
	EVENT_MOUSE_MOVED,

	EVENT_KEY_COMMAND,
	EVENT_KEY_RELEASE,

	EVENT_WINDOW_CREATED,
	EVENT_WINDOW_ACTIVATED,
	EVENT_WINDOW_DEACTIVATED,
	EVENT_WINDOW_DRAW,
	EVENT_WINDOW_CLOSE,

	EVENT_NEWMENU_DRAW,					// draw after the newmenu stuff is drawn (e.g. savegame previews)
	EVENT_NEWMENU_CHANGED,				// an item had its value/text changed
	EVENT_NEWMENU_SELECTED,				// user chose something - pressed enter/clicked on it
	
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
	const void *const createdata;
	constexpr d_create_event(const event_type t, const void *const c) :
		d_event(t), createdata(c)
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
