// linux init.c - added Matt Mueller 9/6/98
#include <stdlib.h>
#include <stdio.h>
#include "text.h"
#include "event.h"
#include "error.h"
#include "joy.h"
#include "args.h"

extern int Inferno_verbose;
extern void arch_sdl_init();
extern void arch_svgalib_init();
extern void key_init();

void arch_init_start()
{

}

void arch_init()
{
 // Initialise the library
#ifdef __SDL__
	arch_sdl_init();
#endif
#ifdef __SVGALIB__
	arch_svgalib_init();
#endif
	if (!FindArg( "-nojoystick" ))  {
		if (Inferno_verbose) printf( "\n%s", TXT_VERBOSE_6);
		joy_init();
	}

    key_init();
}
