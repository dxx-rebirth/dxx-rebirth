#ifndef _GP2X_H_
#define _GP2X_H_

#include <SDL/SDL.h>


extern SDL_Joystick *gp2xJoystick;
extern int gp2xJoyButtons;

#define GP2X_BUTTON_UP              (0)
#define GP2X_BUTTON_DOWN            (4)
#define GP2X_BUTTON_LEFT            (2)
#define GP2X_BUTTON_RIGHT           (6)
#define GP2X_BUTTON_UPLEFT          (1)
#define GP2X_BUTTON_UPRIGHT         (7)
#define GP2X_BUTTON_DOWNLEFT        (3)
#define GP2X_BUTTON_DOWNRIGHT       (5)

#define GP2X_BUTTON_A               (12)
#define GP2X_BUTTON_B               (13)
#define GP2X_BUTTON_X               (14)
#define GP2X_BUTTON_Y               (15)
#define GP2X_BUTTON_L               (10)
#define GP2X_BUTTON_R               (11)
#define GP2X_BUTTON_CLICK           (18)
#define GP2X_BUTTON_START           (8)
#define GP2X_BUTTON_SELECT          (9)
#define GP2X_BUTTON_VOLUP           (16)
#define GP2X_BUTTON_VOLDOWN         (17)


/* map gp2x joy buttons by redefining sdl keys.
   reference to key_properties in arch/sdl/key.c.
   these buttons should be configured in a player file.
   mapped assuning X/Y and L/R buttons are swapped. */

#undef  SDLK_ESCAPE
#undef  SDLK_LALT
#undef  SDLK_MINUS
#undef  SDLK_EQUALS
#undef  SDLK_a
#undef  SDLK_KP_MULTIPLY
#undef  SDLK_l
#undef  SDLK_r
#undef  SDLK_x
#undef  SDLK_y
#undef  SDLK_UP
#undef  SDLK_DOWN
#undef  SDLK_LEFT
#undef  SDLK_RIGHT
#undef  SDLK_KP1
#undef  SDLK_KP3
#undef  SDLK_KP7
#undef  SDLK_KP9

#define SDLK_ESCAPE       GP2X_BUTTON_START
#define SDLK_LALT         GP2X_BUTTON_SELECT
#define SDLK_MINUS        GP2X_BUTTON_VOLDOWN
#define SDLK_EQUALS       GP2X_BUTTON_VOLUP
#define SDLK_a            GP2X_BUTTON_A
#define SDLK_KP_ENTER     GP2X_BUTTON_B
#define SDLK_l            GP2X_BUTTON_R
#define SDLK_r            GP2X_BUTTON_L
#define SDLK_x            GP2X_BUTTON_Y
#define SDLK_y            GP2X_BUTTON_X
#define SDLK_UP           GP2X_BUTTON_UP
#define SDLK_DOWN         GP2X_BUTTON_DOWN
#define SDLK_LEFT         GP2X_BUTTON_LEFT
#define SDLK_RIGHT        GP2X_BUTTON_RIGHT
#define SDLK_KP1          GP2X_BUTTON_DOWNLEFT
#define SDLK_KP3          GP2X_BUTTON_DOWNRIGHT
#define SDLK_KP7          GP2X_BUTTON_UPLEFT
#define SDLK_KP9          GP2X_BUTTON_UPRIGHT

#endif
