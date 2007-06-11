/*
 *
 * mingw_init.c - Basically same as linux init.c
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "console.h"
#include "text.h"
#include "event.h"
#include "error.h"
#include "joy.h"
#include "args.h"

extern void arch_sdl_init();
extern void key_init();

void arch_init_start()
{

}

void arch_init()
{
 // Initialise the library
	arch_sdl_init();
	
	if (!FindArg( "-nojoystick" ))  {
		if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_6);
		joy_init();
	}
	
    key_init();
}
