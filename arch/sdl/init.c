// SDL architecture support
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "text.h"
#include "event.h"
#include "error.h"
#include "args.h"
#include "key.h"
#include "joy.h"

extern void d_mouse_init();

void sdl_close()
{
	SDL_Quit();
}

void arch_sdl_init()
{
	if (SDL_Init(SDL_INIT_VIDEO)<0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();
	joy_init();
	
	if (!FindArg("-nomouse"))
		d_mouse_init();
	
	atexit(sdl_close);
}
