/*
 * $Source: /cvs/cvsroot/d2x/input/svgalib_init.c,v $
 * $Revision: 1.4 $
 * $Author: bradleyb $
 * $Date: 2001-01-29 14:03:57 $
 *
 * SVGALib initialization
 *
 * $Log: not supported by cvs2svn $
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
