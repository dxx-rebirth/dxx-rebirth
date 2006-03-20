// SDL keyboard input support

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "event.h"
#include "error.h"
#include "key.h"
#include "timer.h"



void key_handler(SDL_KeyboardEvent *event)
{
	generic_key_handler(event->keysym.sym,(event->state == SDL_PRESSED));
}
void arch_key_close(void)
{
}
void arch_key_init(void)
{
	keyd_fake_repeat=1;
}
void arch_key_flush(void)
{
}


void arch_key_poll(void)
{
	event_poll();
}



