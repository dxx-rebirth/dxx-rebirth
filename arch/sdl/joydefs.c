/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/joydefs.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-24 09:25:05 $
 *
 * SDL joystick support
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/10/10 03:01:29  bradleyb
 * Replacing win32 joystick (broken) with SDL joystick (stubs)
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "joydefs.h"

int joydefs_calibrate_flag = 0;

void
joydefs_calibrate()
{
  return;
}

void
joydefs_config()
{
  return;
}
