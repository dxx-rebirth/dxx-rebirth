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
extern int com_init();
extern void timer_init();

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
	//added 06/09/99 Matt Mueller - fix nonetwork compile
#ifdef NETWORK
	//end addition -MM
//added on 10/19/98 by Victor Rachels to add serial support (from DPH)
    if(!(FindArg("-noserial")))
     com_init();
//end this section addition - Victor 
	//added 06/09/99 Matt Mueller - fix nonetwork compile
#endif
	//end addition -MM
     timer_init();
    key_init();
}
