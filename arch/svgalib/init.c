/*
 * $Source: /cvs/cvsroot/d2x/arch/svgalib/init.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-24 09:25:05 $
 *
 * SVGALib initialization
 *
 * $Log: not supported by cvs2svn $
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
