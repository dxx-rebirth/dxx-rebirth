/* $Id: event.c,v 1.11 2003-06-02 05:56:37 btb Exp $ */
/*
 *
 * SDL Event related stuff
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#ifdef CONSOLE
#include "CON_console.h"
#endif

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
#ifndef USE_LINUX_JOY // stpohle - so we can choose at compile time..
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);
#endif

static int initialised=0;

void event_poll()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
#ifdef CONSOLE
		if(!CON_Events(&event))
			continue;
#endif
		switch(event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			key_handler((SDL_KeyboardEvent *)&event);
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			mouse_button_handler((SDL_MouseButtonEvent *)&event);
			break;
		case SDL_MOUSEMOTION:
			mouse_motion_handler((SDL_MouseMotionEvent *)&event);
			break;
#ifndef USE_LINUX_JOY       // stpohle - so we can choose at compile time..
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			joy_button_handler((SDL_JoyButtonEvent *)&event);
			break;
		case SDL_JOYAXISMOTION:
			joy_axis_handler((SDL_JoyAxisEvent *)&event);
			break;
		case SDL_JOYHATMOTION:
			joy_hat_handler((SDL_JoyHatEvent *)&event);
			break;
		case SDL_JOYBALLMOTION:
			break;
#endif
		case SDL_QUIT: {
			void quit_request();
			quit_request();
		} break;
		}
	}
}

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}
