/*
 *  messagebox.c
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#include <windows.h>
#include "window.h"
#include "event.h"
#include "messagebox.h"

void display_win32_alert(char *message, int error)
{
	d_event	event;
	window	*wind;

	// Handle Descent's windows properly
	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);

	if (gr_check_fullscreen())
		gr_toggle_fullscreen();

	MessageBox(NULL, message, error?"Sorry, a critical error has occurred.":"Attention!", error?MB_OK|MB_ICONERROR:MB_OK|MB_ICONWARNING);

	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
}

void msgbox_warning(char *message)
{
	display_win32_alert(message, 0);
}

void msgbox_error(char *message)
{
	display_win32_alert(message, 1);
}
