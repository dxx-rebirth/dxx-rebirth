/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/event.c,v $
 * $Revision: 1.6 $
 * $Author: bradleyb $
 * $Date: 2002-07-16 22:37:14 $
 *
 * SDL Event related stuff
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2002/02/16 02:08:30  bradleyb
 * allow older sdl versions
 *
 * Revision 1.4  2001/12/03 02:43:02  bradleyb
 * lots of makefile fixes, and sdl joystick stuff
 *
 * Revision 1.3  2001/11/14 10:43:10  bradleyb
 * remove cruft, fix formatting, begin joystick stuff
 *
 * Revision 1.2  2001/10/31 07:41:54  bradleyb
 * Sync with d1x
 *
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.2  2001/01/29 14:03:57  bradleyb
 * Fixed build, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);

static int initialised=0;

void event_poll()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
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
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			joy_button_handler((SDL_JoyButtonEvent *)&event);
			break;
		case SDL_JOYAXISMOTION:
			joy_axis_handler((SDL_JoyAxisEvent *)&event);
			break;
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION:
			break;
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
