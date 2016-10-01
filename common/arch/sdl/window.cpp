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

static window *FrontWindow = NULL;
static window *FirstWindow = NULL;

window::window(grs_canvas *src, int x, int y, int w, int h, window_alloc_type type, window_subfunction<void> event_callback, void *data, const void *createdata)
{
	window *prev_front = window_get_front();
	d_create_event event;
	window *wind = this;
	Assert(src != NULL);
	Assert(event_callback != NULL);
	gr_init_sub_canvas(wind->w_canv, *src, x, y, w, h);
	wind->w_callback = event_callback;
	wind->w_visible = 1;	// default to visible
	wind->w_modal =	1;		// default to modal
	wind->w_alloc = type;
	wind->w_data = data;

	if (FirstWindow == NULL)
		FirstWindow = wind;
	wind->prev = FrontWindow;
	if (FrontWindow)
		FrontWindow->next = wind;
	wind->next = NULL;
	FrontWindow = wind;
	if (prev_front)
		WINDOW_SEND_EVENT(prev_front, EVENT_WINDOW_DEACTIVATED);

	event.createdata = createdata;
	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_CREATED);
	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
}

void window::unlink()
{
	if (this == FrontWindow)
		FrontWindow = this->prev;
	if (this == FirstWindow)
		FirstWindow = this->next;
	if (this->next)
		this->next->prev = this->prev;
	if (this->prev)
		this->prev->next = this->next;
}

window::~window()
{
	if (w_alloc == window_alloc_type::subclass)
		unlink();
}

int window_close(window *wind)
{
	window *prev;
	d_event event;
	window_event_result (*w_callback)(window *wind,const d_event &event, void *data) = wind->w_callback;
	window_alloc_type type = wind->w_alloc;

	if (wind == window_get_front())
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);	// Deactivate first

	if (WINDOW_SEND_EVENT(wind, EVENT_WINDOW_CLOSE) == window_event_result::handled)
	{
		// User 'handled' the event, cancelling close
		if (wind == window_get_front())
		{
			WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
		}
		return 0;
	}

	if (type == window_alloc_type::separate)
		wind->unlink();		// Remove from the window list

	if ((prev = window_get_front()))
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_ACTIVATED);

	// Callback needs to recognise this is a NULL pointer! (And 'wind' will be a dangling pointer for window_alloc_type::subclass)
	event.type = EVENT_WINDOW_CLOSED;
	w_callback(wind, event, NULL);
	
	if (type == window_alloc_type::separate)
		delete wind;

	return 1;
}

int window_exists(window *wind)
{
	window *w;

	for (w = FirstWindow; w && w != wind; w = w->next) {}
	
	return w && w == wind;
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
	
	if (window_is_visible(&wind))
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
