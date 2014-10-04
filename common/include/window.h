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

enum class window_event_result
{
	// Window ignored event.  Bubble up.
	ignored,
	// Window handled event.
	handled,
	close,
};

void arch_init(void);

template <typename T>
class window_subfunction_t
{
public:
	typedef window_event_result (*type)(window *menu,const d_event &event, T *userdata);
};

class unused_window_userdata_t;
static unused_window_userdata_t *const unused_window_userdata = NULL;

struct embed_window_pointer_t
{
	window *wind;
};

struct ignore_window_pointer_t
{
};

window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction_t<void>::type event_callback, void *data);

static inline void set_embedded_window_pointer(embed_window_pointer_t *wp, window *w)
{
	wp->wind = w;
}

static inline void set_embedded_window_pointer(ignore_window_pointer_t *, window *) {}
static inline void set_embedded_window_pointer(unused_window_userdata_t *, window *) {}

template <typename T>
window *window_create(grs_canvas *src, int x, int y, int w, int h, typename window_subfunction_t<T>::type event_callback, T *data)
{
	auto win = window_create(src, x, y, w, h, (window_subfunction_t<void>::type)event_callback, static_cast<void *>(data));
	set_embedded_window_pointer(data, win);
	return win;
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
window_event_result window_send_event(window *wind,const d_event &event);
extern void window_set_modal(window *wind, int modal);
extern int window_is_modal(window *wind);

static inline window_event_result WINDOW_SEND_EVENT(window *w, const d_event &event, const char *file, unsigned line, const char *e)
{
	auto c = window_get_canvas(w);
	con_printf(CON_DEBUG, "%s:%u: sending event %s to window of dimensions %dx%d", file, line, e, c->cv_bitmap.bm_w, c->cv_bitmap.bm_h);
	return window_send_event(w, event);
}

#define WINDOW_SEND_EVENT(w, e)	(event.type = e, (WINDOW_SEND_EVENT)(w, event, __FILE__, __LINE__, #e))

#endif

#endif
