/*
 * $Source: /cvs/cvsroot/d2x/arch/include/event.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-01-28 16:10:57 $
 *
 * Event header file
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifndef _EVENT_H
#define _EVENT_H

#ifdef GII_XWIN
#include <ggi/input/xwin.h>
#endif

int event_init();
void event_poll();

#ifdef GII_XWIN
void init_gii_xwin(Display *disp,Window win);
#endif

#endif
