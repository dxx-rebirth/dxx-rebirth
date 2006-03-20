// SDL architecture support
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "text.h"
#include "event.h"
#include "error.h"
#include "args.h"

extern void d_mouse_init();

void sdl_close()
{
	SDL_Quit();
}

void arch_sdl_init()
{
 // Initialise the library
//edited on 01/03/99 by Matt Mueller - if we use SDL_INIT_EVERYTHING, cdrom is initialized even if -nocdaudio is used
 if (SDL_Init(
#if SDL_VIDEO || SDL_GL
	SDL_INIT_VIDEO
#else
	0
#endif
	)<0) {
//end edit -MM
    Error("SDL library initialisation failed: %s.",SDL_GetError());
 }
#ifdef SDL_INPUT
 if (!FindArg("-nomouse"))
  d_mouse_init();
#endif
 atexit(sdl_close);
}
