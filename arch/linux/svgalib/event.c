// SDL Event related stuff

#include <stdio.h>
#include <stdlib.h>

#include <vgakeyboard.h>
#include <vgamouse.h>

void key_handler(int scancode, int press);
//added on 10/17/98 by Hans de Goede for mouse functionality
//extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
//extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
//end this section addition - Hans

void event_poll()
{
 keyboard_update();
 mouse_update();
}

/*int event_init()
{
 initialised = 1;
 return 0;
}*/
