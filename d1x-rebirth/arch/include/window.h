/*
 *  A 'window' is simply a canvas that can receive events.
 *  It can be anything from a simple message box to the
 *	game screen when playing.
 *
 *  See event.c for event handling code.
 *
 *	-kreator 2009-05-06
 */

#ifndef DESCENT_WINDOW_H
#define DESCENT_WINDOW_H

#include "event.h"
#include "gr.h"
#include "console.h"

typedef struct window window;

extern window *window_create(grs_canvas *src, int x, int y, int w, int h, int (*event_callback)(window *wind, d_event *event, void *data), void *data);
extern int window_close(window *wind);
extern int window_exists(window *wind);
extern window *window_get_front(void);
extern window *window_get_first(void);
extern window *window_get_next(window *wind);
extern window *window_get_prev(window *wind);
extern void window_select(window *wind);
extern void window_set_visible(window *wind, int visible);
extern int window_is_visible(window *wind);
extern grs_canvas *window_get_canvas(window *wind);
extern void window_update_canvases(void);
extern int window_send_event(window *wind, d_event *event);
extern void window_set_modal(window *wind, int modal);
extern int window_is_modal(window *wind);

#define WINDOW_SEND_EVENT(w, e)	\
do {	\
	con_printf(CON_DEBUG, "Sending event %s to window of dimensions %dx%d\n", #e, window_get_canvas(w)->cv_bitmap.bm_w, window_get_canvas(w)->cv_bitmap.bm_h);	\
	event.type = e;	\
	window_send_event(w, &event);	\
} while (0)

#endif
