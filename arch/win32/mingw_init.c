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
#include "pstypes.h"
#include "console.h"
#include "text.h"
#include "event.h"
#include "error.h"
#include "joy.h"
#include "args.h"

extern void arch_sdl_init();
extern void key_init();
extern int com_init();
extern void timer_init();

void arch_init_start()
{

}

void arch_init()
{
 // Initialise the library
	arch_sdl_init();
	if (!FindArg( "-nojoystick" ))  {
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_6);
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
    key_init();
}
