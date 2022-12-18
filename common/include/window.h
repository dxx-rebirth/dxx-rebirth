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

#include "dxxsconf.h"
#include "gr.h"
#include "console.h"

#include <assert.h>

#include "fwd-window.h"
#include "event.h"

namespace dcx {

class window
{
public:
	grs_subcanvas w_canv;					// the window's canvas to draw to
private:
	class window *prev;				// the previous window in the doubly linked list
	class window *next = nullptr;				// the next window in the doubly linked list
	uint8_t w_visible = 1;						// whether it's visible
	uint8_t w_modal = 1;						// modal = accept all user input exclusively
	bool *w_exists = nullptr;					// optional pointer to a tracking variable
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
#if !DXX_USE_OGL
	friend void window_update_canvases();
#endif

	uint8_t is_visible() const
	{
		return w_visible;
	}
	window *set_visible(uint8_t visible);

	void set_modal(uint8_t modal)
	{
		w_modal = modal;
	}

	uint8_t is_modal() const
	{
		return w_modal;
	}

	window_event_result send_event(const d_event &event
#ifdef DXX_HAVE_CXX_BUILTIN_FILE_LINE
								, const char *file = __builtin_FILE(), unsigned line = __builtin_LINE()
#endif
								);
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

void menu_destroy_hook(window *w);

template <typename T1, typename... ConstructionArgs>
T1 *window_create(ConstructionArgs &&... args)
{
	auto r = std::make_unique<T1>(std::forward<ConstructionArgs>(args)...);
	r->send_creation_events();
	return r.release();
}

}
