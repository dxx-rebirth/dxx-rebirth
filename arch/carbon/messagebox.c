/*
 *  messagebox.c
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#else
#include <Carbon.h>
#endif

#include "window.h"
#include "event.h"
#include "messagebox.h"

void display_mac_alert(char *message, int error)
{
	window	*wind;
	d_event	event;
	int		fullscreen;
	bool	osX = FALSE;
	uint 	response;
	int16_t itemHit;

	// Handle Descent's windows properly
	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);

	if ((fullscreen = gr_check_fullscreen()))
		gr_toggle_fullscreen();
	
	osX = ( Gestalt(gestaltSystemVersion, (long *) &response) == noErr)
		&& (response >= 0x01000 );

    ShowCursor();

	if (osX)
	{
#ifdef TARGET_API_MAC_CARBON
		DialogRef	alert;
		CFStringRef	error_text = CFSTR("Sorry, a critical error has occurred.");
		CFStringRef	text = NULL;

		text = CFStringCreateWithCString(CFAllocatorGetDefault(), message, kCFStringEncodingMacRoman);
		if (!text)
		{
			if (wind) WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
			return;
		}

		if (CreateStandardAlert(error ? kAlertStopAlert : kAlertNoteAlert, error ? error_text : text, error ? text : NULL, 0, &alert) != noErr)
		{
			CFRelease(text);
			if (wind) WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
			return;
		}
		
		RunStandardAlert(alert, 0, &itemHit);
		CFRelease(text);
#endif
	}
	else
	{
		Str255 	error_text = "\pSorry, a critical error has occurred.";
		Str255 	text;

#if !defined(MAC_OS_X_VERSION_MAX_ALLOWED) || (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4)	// kill warning
		CopyCStringToPascal(message, text);
#endif
		StandardAlert(error ? kAlertStopAlert : kAlertNoteAlert, error ? error_text : text, error ? text : NULL, 0, &itemHit);
	}

	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
	
	if (!error && fullscreen)
		gr_toggle_fullscreen();
}

void msgbox_warning(char *message)
{
	display_mac_alert(message, 0);
}

void msgbox_error(char *message)
{
	display_mac_alert(message, 1);
}
