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
#include "fwd-window.h"
namespace dcx {

enum class window_event_result : uint8_t
{
	// Window ignored event.  Bubble up.
	ignored,
	// Window handled event.
	handled,
	close,
};

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

struct window
{
private:
	grs_canvas w_canv;					// the window's canvas to draw to
	window_subfunction<void> w_callback;	// the event handler
	int w_visible;						// whether it's visible
	int w_modal;						// modal = accept all user input exclusively
	void *w_data;							// whatever the user wants (eg menu data for 'newmenu' menus)
	struct window *prev;				// the previous window in the doubly linked list
	struct window *next;				// the next window in the doubly linked list
	
public:
	// For creating the window, there are two ways - using the (older) window_create function
	// or using the constructor, passing an event handler that takes a subclass of window.
	explicit window(grs_canvas *src, int x, int y, int w, int h, window_subfunction<void> event_callback, void *data, const void *createdata);
	
	template <typename T>
	window(grs_canvas *src, int x, int y, int w, int h, window_subclass_subfunction<T> event_callback) :
	window(src, x, y, w, h, reinterpret_cast<window_subclass_subfunction<window>>(event_callback), nullptr, nullptr) {}
	
	// Declaring as friends to keep function syntax, for historical reasons (for now at least)
	// Intended to transition to the class method form
	friend window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction<void> event_callback, void *userdata, const void *createdata);
	
	friend int window_close(window *wind);
	friend int window_exists(window *wind);
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
		auto r = wind.w_callback(&wind, event, wind.w_data);
		if (r == window_event_result::close)
			window_close(&wind);
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
	
};

template <typename T1, typename T2 = const void>
static inline window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction<T1> event_callback, T1 *data, T2 *createdata = nullptr)
{
	auto win = new window(src, x, y, w, h, reinterpret_cast<window_subfunction<void>>(event_callback), static_cast<void *>(data), static_cast<const void *>(createdata));
	set_embedded_window_pointer(data, win);
	return win;
}

template <typename T1, typename T2 = const void>
static inline window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction<const T1> event_callback, const T1 *userdata, T2 *createdata = nullptr)
{
	return new window(src, x, y, w, h, reinterpret_cast<window_subfunction<void>>(event_callback), static_cast<void *>(const_cast<T1 *>(userdata)), static_cast<const void *>(createdata));
}

static inline window_event_result (WINDOW_SEND_EVENT)(window &w, const d_event &event, const char *file, unsigned line, const char *e)
{
	auto &c = window_get_canvas(w);
	con_printf(CON_DEBUG, "%s:%u: sending event %s to window of dimensions %dx%d", file, line, e, c.cv_bitmap.bm_w, c.cv_bitmap.bm_h);
	return window_send_event(w, event);
}

}
#endif
