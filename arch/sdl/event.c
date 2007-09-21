/* $Id: event.c,v 1.1.1.1 2006/03/17 19:53:40 zicodxx Exp $ */
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

#include <SDL/SDL.h>

#ifdef GP2X
#include "gp2x.h"
extern void key_handler(SDL_JoyButtonEvent *event);
#else
extern void key_handler(SDL_KeyboardEvent *event);
#endif
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);

static int initialised=0;

#ifndef GP2X
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
			case SDL_JOYHATMOTION:
				joy_hat_handler((SDL_JoyHatEvent *)&event);
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				void quit_request();
				quit_request();
			} break;
		}
	}
}
#else
void event_poll()
{
	SDL_Event event;
	
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			switch(event.jbutton.button)
			{
				case GP2X_BUTTON_X:
				case GP2X_BUTTON_Y:
				case GP2X_BUTTON_A:
				case GP2X_BUTTON_B:
				case GP2X_BUTTON_UP:
				case GP2X_BUTTON_DOWN:
				case GP2X_BUTTON_LEFT:
				case GP2X_BUTTON_RIGHT:
				case GP2X_BUTTON_UPLEFT:
				case GP2X_BUTTON_UPRIGHT:
				case GP2X_BUTTON_DOWNLEFT:
				case GP2X_BUTTON_DOWNRIGHT:
				case GP2X_BUTTON_L:
				case GP2X_BUTTON_R:
				case GP2X_BUTTON_CLICK:
				case GP2X_BUTTON_START:
				case GP2X_BUTTON_VOLUP:
				case GP2X_BUTTON_VOLDOWN:
				case GP2X_BUTTON_SELECT:
			
				key_handler((SDL_JoyButtonEvent *)&event);
				break;
			}
			break;
			case SDL_QUIT: 
			{
				void quit_request();
				quit_request();
			}
			break;
		}
	}
}
#endif

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}
