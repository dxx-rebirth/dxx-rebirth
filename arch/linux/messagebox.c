/*
 *  messagebox.c
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#include "window.h"
#include "event.h"
#include "messagebox.h"

void display_linux_alert(char *message, int error)
{
	d_event	event = { EVENT_WINDOW_DEACTIVATED };
	window	*wind;

	// Handle Descent's windows properly
	if ((wind = window_get_front()))
		window_send_event(window_get_front(), &event);
	event.type = EVENT_WINDOW_ACTIVATED;

	// TODO: insert messagebox code...

	if (wind)
		window_send_event(window_get_front(), &event);
}

void msgbox_warning(char *message)
{
	display_linux_alert(message, 0);
}

void msgbox_error(char *message)
{
	display_linux_alert(message, 1);
}
