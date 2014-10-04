/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
// Event header file

#ifndef _EVENT_H
#define _EVENT_H
#include "maths.h"

#ifdef __cplusplus

typedef enum event_type
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
	EVENT_WINDOW_CLOSED,

	EVENT_NEWMENU_DRAW,					// draw after the newmenu stuff is drawn (e.g. savegame previews)
	EVENT_NEWMENU_CHANGED,				// an item had its value/text changed
	EVENT_NEWMENU_SELECTED,				// user chose something - pressed enter/clicked on it
	
	EVENT_UI_DIALOG_DRAW,				// draw after the dialog stuff is drawn (e.g. spinning robots)
	EVENT_UI_GADGET_PRESSED,				// user 'pressed' a gadget
	EVENT_UI_LISTBOX_MOVED,
	EVENT_UI_LISTBOX_SELECTED,
	EVENT_UI_USERBOX_DRAGGED
} event_type;

// A vanilla event. Cast to the correct type of event according to 'type'.
struct d_event
{
	event_type type;
};

int event_init();

// Sends input events to event handlers
void event_poll();
void event_flush();

// Set and call the default event handler
void set_default_handler(int (*handler)(const d_event &event));
int call_default_handler(const d_event &event);

// Send an event to the front window as first priority, then to the windows behind if it's not modal (editor), then the default handler
void event_send(const d_event &event);

// Sends input, idle and draw events to event handlers
void event_process();

void event_toggle_focus(int activate_focus);

// See how long we were idle for
void event_reset_idle_seconds();
fix event_get_idle_seconds();

#endif

#endif
