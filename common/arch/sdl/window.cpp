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

#include "gr.h"
#include "window.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "event.h"

namespace dcx {

static window *FrontWindow = nullptr;
static window *FirstWindow = nullptr;

window::window(grs_canvas &src, const int x, const int y, const int w, const int h) :
	// Default to visible and modal
	w_visible(1), w_modal(1), prev(FrontWindow), next(nullptr), w_exists(nullptr)
{
	gr_init_sub_canvas(w_canv, src, x, y, w, h);

	if (FirstWindow == nullptr)
		FirstWindow = this;
}

void window::send_creation_events(const void *const createdata)
{
	const auto prev_front = window_get_front();
	if (FrontWindow)
		FrontWindow->next = this;
	FrontWindow = this;
	if (prev_front)
	{
		const d_event event{EVENT_WINDOW_DEACTIVATED};
		WINDOW_SEND_EVENT(prev_front);
	}
	{
		const d_create_event event{EVENT_WINDOW_CREATED, createdata};
		WINDOW_SEND_EVENT(this);
	}
	{
		const d_event event{EVENT_WINDOW_ACTIVATED};
		WINDOW_SEND_EVENT(this);
	}
}

window::~window()
{
	if (w_exists)
		*w_exists = false;
	if (this == FrontWindow)
		FrontWindow = this->prev;
	if (this == FirstWindow)
		FirstWindow = this->next;
	if (this->next)
		this->next->prev = this->prev;
	if (this->prev)
		this->prev->next = this->next;
}

callback_window::callback_window(grs_canvas &src, const int x, const int y, const int w, const int h, const window_subfunction<void> event_callback, void *const data) :
	window(src, x, y, w, h),
	w_callback((assert(event_callback != nullptr), event_callback)), w_data(data)
{
}

window_event_result callback_window::event_handler(const d_event &event)
{
	return w_callback(this, event, w_data);
}

int window_close(window *wind)
{
	window *prev;
	window_event_result result;

	if (wind == window_get_front())
	{
		const d_event event{EVENT_WINDOW_DEACTIVATED};
		WINDOW_SEND_EVENT(wind);	// Deactivate first
	}

	{
		const d_event event{EVENT_WINDOW_CLOSE};
		result = WINDOW_SEND_EVENT(wind);
	}
	if (result == window_event_result::handled)
	{
		// User 'handled' the event, cancelling close
		if (wind == window_get_front())
		{
			const d_event event{EVENT_WINDOW_ACTIVATED};
			WINDOW_SEND_EVENT(wind);
		}
		return 0;
	}

	menu_destroy_hook(wind);

	if (result != window_event_result::deleted)	// don't attempt to re-delete
		delete wind;

	if ((prev = window_get_front()))
	{
		const d_event event{EVENT_WINDOW_ACTIVATED};
		WINDOW_SEND_EVENT(prev);
	}

	return 1;
}

// Get the top window that's visible
window *window_get_front(void)
{
	window *wind;

	for (wind = FrontWindow; (wind != NULL) && !wind->w_visible; wind = wind->prev) {}

	return wind;
}

window *window_get_first(void)
{
	return FirstWindow;
}

// Make wind the front window
void window_select(window &wind)
{
	window *prev = window_get_front();
	if (&wind == FrontWindow)
		return;
	if (&wind == FirstWindow && wind.next)
		FirstWindow = FirstWindow->next;

	if (wind.next)
		wind.next->prev = wind.prev;
	if (wind.prev)
		wind.prev->next = wind.next;
	wind.prev = FrontWindow;
	FrontWindow->next = &wind;
	wind.next = nullptr;
	FrontWindow = &wind;
	
	if (wind.is_visible())
	{
		if (prev)
		{
			const d_event event{EVENT_WINDOW_DEACTIVATED};
			WINDOW_SEND_EVENT(prev);
		}
		const d_event event{EVENT_WINDOW_ACTIVATED};
		WINDOW_SEND_EVENT(&wind);
	}
}

window *window_set_visible(window &w, int visible)
{
	window *prev = window_get_front();
	w.w_visible = visible;
	auto wind = window_get_front();	// get the new front window
	if (wind == prev)
		return wind;
	
	if (prev)
	{
		const d_event event{EVENT_WINDOW_DEACTIVATED};
		WINDOW_SEND_EVENT(prev);
	}

	if (wind)
	{
		const d_event event{EVENT_WINDOW_ACTIVATED};
		WINDOW_SEND_EVENT(wind);
	}
	return wind;
}

#if !DXX_USE_OGL
void window_update_canvases()
{
	window *wind;
	
	for (wind = FirstWindow; wind != NULL; wind = wind->next)
		gr_init_sub_bitmap (wind->w_canv.cv_bitmap,
							*wind->w_canv.cv_bitmap.bm_parent,
							wind->w_canv.cv_bitmap.bm_x,
							wind->w_canv.cv_bitmap.bm_y,
							wind->w_canv.cv_bitmap.bm_w,
							wind->w_canv.cv_bitmap.bm_h);
}
#endif

}
