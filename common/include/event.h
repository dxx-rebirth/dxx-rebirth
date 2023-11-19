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

enum class event_type : uint8_t
{
	idle = 0,
	quit,

#if DXX_MAX_BUTTONS_PER_JOYSTICK
	joystick_button_down,
	joystick_button_up,
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
	joystick_moved,
#endif

	mouse_button_down,
	mouse_button_up,
	mouse_double_clicked,
	mouse_moved,

	key_command,
	key_release,

	window_created,
#if SDL_MAJOR_VERSION == 2
	window_resize,
#endif
	window_activated,
	window_deactivated,
	window_draw,
	window_close,

	newmenu_draw,					// draw after the newmenu stuff is drawn (e.g. savegame previews)
	newmenu_changed,				// an item had its value/text changed
	newmenu_selected,				// user chose something - pressed enter/clicked on it

	loop_begin_loop,
	loop_end_loop,

	ui_dialog_draw,				// draw after the dialog stuff is drawn (e.g. spinning robots)
	ui_gadget_pressed,				// user 'pressed' a gadget
	ui_listbox_moved,
	ui_listbox_selected,
	ui_userbox_dragged
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
		d_event{event_type::window_created}
	{
	}
};

struct d_change_event : d_event
{
	const int citem;
	constexpr d_change_event(const int c) :
		d_event{event_type::newmenu_changed}, citem{c}
	{
	}
};

struct d_select_event : d_event
{
	int citem;
	d_select_event(const int c) :
		d_event{event_type::newmenu_selected}, citem{c}
	{
	}
};

#if SDL_MAJOR_VERSION == 2
struct d_window_size_event : d_event
{
	Sint32 width;
	Sint32 height;
	d_window_size_event(const Sint32 w, const Sint32 h) :
		d_event{event_type::window_resize}, width{w}, height{h}
	{
	}
};
#endif

struct d_event_begin_loop : d_event
{
	d_event_begin_loop() :
		d_event{event_type::loop_begin_loop}
	{
	}
};

struct d_event_end_loop : d_event
{
	d_event_end_loop() :
		d_event{event_type::loop_end_loop}
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
