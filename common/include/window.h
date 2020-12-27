/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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

#pragma once

#include "gr.h"
#include "console.h"

#include <assert.h>

#include "fwd-window.h"
#include "event.h"

namespace dcx {

constexpr const unused_window_userdata_t *unused_window_userdata = nullptr;

class window
{
public:
	grs_canvas w_canv;					// the window's canvas to draw to
private:
	int w_visible;						// whether it's visible
	int w_modal;						// modal = accept all user input exclusively
	class window *prev;				// the previous window in the doubly linked list
	class window *next;				// the next window in the doubly linked list
	bool *w_exists;					// optional pointer to a tracking variable
public:
	explicit window(grs_canvas &src, int x, int y, int w, int h);
	window(const window &) = delete;
	window &operator=(const window &) = delete;
	virtual ~window();

	virtual window_event_result event_handler(const d_event &) = 0;

	void send_creation_events();
	friend int window_close(window *wind);
	friend window *window_get_front();
	friend window *window_get_first();
	friend void window_select(window &wind);
	friend window *window_set_visible(window &wind, int visible);
#if !DXX_USE_OGL
	friend void window_update_canvases();
#endif
	friend window *window_set_visible(window *wind, int visible)
	{
		return window_set_visible(*wind, visible);
	}

	int is_visible()
	{
		return w_visible;
	}

	friend int window_is_visible(window &wind)
	{
		return wind.is_visible();
	}

	void set_modal(int modal)
	{
		w_modal = modal;
	}

	friend int window_is_modal(window &wind)
	{
		return wind.w_modal;
	}

	friend window_event_result window_send_event(window &wind, const d_event &event)
	{
		auto r = wind.event_handler(event);
		if (r == window_event_result::close)
			if (window_close(&wind))
				return window_event_result::deleted;

		return r;
	}

	friend window *window_get_next(window &wind)
	{
		return wind.next;
	}
	
	friend window *window_get_prev(window &wind)
	{
		return wind.prev;
	}

	void track(bool *exists)
	{
		assert(w_exists == nullptr);
		w_exists = exists;
	}
};

static inline window_event_result (WINDOW_SEND_EVENT)(window &w, const d_event &event, const char *const file, const unsigned line)
{
	auto &c = w.w_canv;
	con_printf(CON_DEBUG, "%s:%u: sending event %i to window of dimensions %dx%d", file, line, event.type, c.cv_bitmap.bm_w, c.cv_bitmap.bm_h);
	return window_send_event(w, event);
}

void menu_destroy_hook(window *w);

template <typename T1, typename... ConstructionArgs>
T1 *window_create(ConstructionArgs &&... args)
{
	auto r = std::make_unique<T1>(std::forward<ConstructionArgs>(args)...);
	r->send_creation_events();
	return r.release();
}

}
