// Event header file

#ifndef _EVENT_H
#define _EVENT_H

int event_init();
void event_poll();

#ifdef GII_XWIN
#include <ggi/input/xwin.h>
void init_gii_xwin(Display *disp,Window win);
#endif

#endif
