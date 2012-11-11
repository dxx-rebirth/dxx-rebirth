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

static void display_win32_alert(const char *message, int error)
{
	d_event	event;
	window	*wind;

	// Handle Descent's windows properly
	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);

	int fullscreen = (grd_curscreen && gr_check_fullscreen());
	if (fullscreen)
		gr_toggle_fullscreen();

	MessageBox(NULL, message, error?"Sorry, a critical error has occurred.":"Attention!", error?MB_OK|MB_ICONERROR:MB_OK|MB_ICONWARNING);

	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
	
	if (!error && fullscreen)
		gr_toggle_fullscreen();
}

void msgbox_warning(const char *message)
{
	display_win32_alert(message, 0);
}

void msgbox_error(const char *message)
{
	display_win32_alert(message, 1);
}
