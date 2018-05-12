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

#pragma once

#include "gr.h"
#include "console.h"

#ifdef __cplusplus
#include <assert.h>

#include "fwd-window.h"
#include "event.h"
namespace dcx {

constexpr const unused_window_userdata_t *unused_window_userdata = nullptr;

struct embed_window_pointer_t
{
	window *wind;
};

struct ignore_window_pointer_t
{
};

static inline void set_embedded_window_pointer(embed_window_pointer_t *wp, window *w)
{
	wp->wind = w;
}

static inline void set_embedded_window_pointer(ignore_window_pointer_t *, window *) {}

class window
{
	grs_canvas w_canv;					// the window's canvas to draw to
	int w_visible;						// whether it's visible
	int w_modal;						// modal = accept all user input exclusively
	class window *prev;				// the previous window in the doubly linked list
	class window *next;				// the next window in the doubly linked list
	bool *w_exists;					// optional pointer to a tracking variable
	
public:
	// For creating the window, there are two ways - using the (older) window_create function
	// or using the constructor, passing an event handler that takes a subclass of window.
	explicit window(grs_canvas &src, int x, int y, int w, int h);

	virtual ~window();

	virtual window_event_result event_handler(const d_event &) = 0;

	void send_creation_events(const void *createdata);
	// Declaring as friends to keep function syntax, for historical reasons (for now at least)
	// Intended to transition to the class method form
	friend window *window_create(grs_canvas &src, int x, int y, int w, int h, window_subfunction<void> event_callback, void *userdata, const void *createdata);
	
	friend int window_close(window *wind);
	friend window *window_get_front();
	friend window *window_get_first();
	friend void window_select(window &wind);
	friend window *window_set_visible(window &wind, int visible);
#if !DXX_USE_OGL
	friend void window_update_canvases();
#endif
	
	friend grs_canvas &window_get_canvas(window &wind)
	{
		return wind.w_canv;
	}
	
	friend window *window_set_visible(window *wind, int visible)
	{
		return window_set_visible(*wind, visible);
	}

	int is_visible()
	{
		return w_visible;
	}

	friend int window_is_visible(window *wind)
	{
		return wind->is_visible();
	}

	void set_modal(int modal)
	{
		w_modal = modal;
	}

	friend void window_set_modal(window *wind, int modal)
	{
		wind->set_modal(modal);
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

class callback_window : public window
{
	window_subfunction<void> w_callback;	// the event handler
	void *w_data;						// whatever the user wants (eg menu data for 'newmenu' menus)
public:
	callback_window(grs_canvas &src, int x, int y, int w, int h, window_subfunction<void> event_callback, void *data);
	virtual window_event_result event_handler(const d_event &) override;
};

template <typename T1, typename T2 = const void>
static inline callback_window *window_create(grs_canvas &src, int x, int y, int w, int h, window_subfunction<T1> event_callback, T1 *data, T2 *createdata = nullptr)
{
	const auto win = new callback_window(src, x, y, w, h, reinterpret_cast<window_subfunction<void>>(event_callback), static_cast<void *>(data));
	set_embedded_window_pointer(data, win);
	win->send_creation_events(createdata);
	return win;
}

template <typename T1, typename T2 = const void>
static inline callback_window *window_create(grs_canvas &src, int x, int y, int w, int h, window_subfunction<const T1> event_callback, const T1 *userdata, T2 *createdata = nullptr)
{
	const auto win = new callback_window(src, x, y, w, h, reinterpret_cast<window_subfunction<void>>(event_callback), static_cast<void *>(const_cast<T1 *>(userdata)));
	win->send_creation_events(createdata);
	return win;
}

static inline window_event_result (WINDOW_SEND_EVENT)(window &w, const d_event &event, const char *const file, const unsigned line)
{
	auto &c = window_get_canvas(w);
	con_printf(CON_DEBUG, "%s:%u: sending event %i to window of dimensions %dx%d", file, line, event.type, c.cv_bitmap.bm_w, c.cv_bitmap.bm_h);
	return window_send_event(w, event);
}

void menu_destroy_hook(window *w);

}
#endif
