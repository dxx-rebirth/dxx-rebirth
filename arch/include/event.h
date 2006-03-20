/*
 * $Source: /cvsroot/dxx-rebirth/d2x-rebirth/arch/include/event.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:54:32 $
 *
 * Event header file
 *
 * $Log: event.h,v $
 * Revision 1.1.1.1  2006/03/17 19:54:32  zicodxx
 * initial import
 *
 * Revision 1.1  2001/01/28 16:10:57  bradleyb
 * unified input headers.
 *
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
