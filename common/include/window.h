/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
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

#ifdef __cplusplus

struct window;

void arch_init(void);

template <typename T>
class window_subfunction_t
{
public:
	typedef int (*type)(window *menu, d_event *event, T *userdata);
};

class unused_window_userdata_t;
static unused_window_userdata_t *const unused_window_userdata = NULL;

window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction_t<void>::type event_callback, void *data);

template <typename T>
window *window_create(grs_canvas *src, int x, int y, int w, int h, typename window_subfunction_t<T>::type event_callback, T *data)
{
	return window_create(src, x, y, w, h, (window_subfunction_t<void>::type)event_callback, (void *)(data));
}

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
	con_printf(CON_DEBUG, "Sending event %s to window of dimensions %dx%d", #e, window_get_canvas(w)->cv_bitmap.bm_w, window_get_canvas(w)->cv_bitmap.bm_h);	\
	event.type = e;	\
	window_send_event(w, &event);	\
} while (0)

#endif

#endif
