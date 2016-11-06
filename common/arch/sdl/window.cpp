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

window::window(grs_canvas &src, const int x, const int y, const int w, const int h, const window_subfunction<void> event_callback, void *const data) :
	// Default to visible and modal
	w_callback(event_callback), w_visible(1), w_modal(1), w_data(data), prev(FrontWindow), next(nullptr), w_exists(nullptr)
{
	Assert(event_callback != nullptr);
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
		d_event event;
		WINDOW_SEND_EVENT(prev_front, EVENT_WINDOW_DEACTIVATED);
	}
	{
		d_create_event event;
		event.createdata = createdata;
		WINDOW_SEND_EVENT(this, EVENT_WINDOW_CREATED);
	}
	{
		d_event event;
		WINDOW_SEND_EVENT(this, EVENT_WINDOW_ACTIVATED);
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

int window_close(window *wind)
{
	window *prev;
	d_event event;
	window_event_result result;

	if (wind == window_get_front())
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);	// Deactivate first

	if ((result = WINDOW_SEND_EVENT(wind, EVENT_WINDOW_CLOSE)) == window_event_result::handled)
	{
		// User 'handled' the event, cancelling close
		if (wind == window_get_front())
		{
			WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
		}
		return 0;
	}

	if (result != window_event_result::deleted)	// don't attempt to re-delete
		delete wind;

	if ((prev = window_get_front()))
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_ACTIVATED);

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
	d_event event;
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
			WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);
		WINDOW_SEND_EVENT(&wind, EVENT_WINDOW_ACTIVATED);
	}
}

window *window_set_visible(window &w, int visible)
{
	window *prev = window_get_front();
	w.w_visible = visible;
	auto wind = window_get_front();	// get the new front window
	d_event event;
	if (wind == prev)
		return wind;
	
	if (prev)
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);

	if (wind)
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
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
