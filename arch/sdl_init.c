// SDL architecture support
#include <conf.h>
#ifdef __ENV_LINUX__
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "text.h"
#include "event.h"
#include "error.h"
#include "args.h"
#include "digi.h"

extern void d_mouse_init();

void sdl_close()
{
	SDL_Quit();
}

void arch_sdl_init()
{
 // Initialise the library
//edited on 01/03/99 by Matt Mueller - if we use SDL_INIT_EVERYTHING, cdrom is initialized even if -nocdaudio is used
#ifdef SDL_INPUT
 if (!args_find("-nomouse"))
   d_mouse_init();
#endif
 if (!args_find("-nosound"))
   digi_init();
 atexit(sdl_close);
}

#endif // __ENV_LINUX__
