/*
 * $Source: /cvsroot/dxx-rebirth/d2x-rebirth/arch/svgalib/init.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:54:31 $
 *
 * SVGALib initialization
 *
 * $Log: init.c,v $
 * Revision 1.1.1.1  2006/03/17 19:54:31  zicodxx
 * initial import
 *
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.4  2001/01/29 14:03:57  bradleyb
 * Fixed build, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "args.h"

extern void d_mouse_init();

void arch_svgalib_init()
{
 if (!FindArg("-nomouse"))
	d_mouse_init();
}
