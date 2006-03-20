// SDL Event related stuff

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

extern void key_handler(SDL_KeyboardEvent *event);
//added on 10/17/98 by Hans de Goede for mouse functionality
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
//end this section addition - Hans

static int initialised=0;

void event_poll()
{
 SDL_Event event;
 while (SDL_PollEvent(&event))
 {
// if( (event.type == SDL_KEYEVENT) {
//added/changed on 10/17/98 by Hans de Goede for mouse functionality
//-killed-   if( (event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) ) {
   switch(event.type)
   {
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
//-killed-     return;
//end this section addition/change - Hans
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
