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

#include "gr.h"
#include "window.h"
#include "u_mem.h"
#include "console.h"
#include "dxxerror.h"
#include "event.h"

namespace dcx {

static window *FrontWindow = nullptr;
static window *FirstWindow = nullptr;

window::window(grs_canvas &src, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) :
	// Default to visible and modal
	prev(FrontWindow)
{
	gr_init_sub_canvas(w_canv, src, {x}, {y}, {w}, {h});

	if (FirstWindow == nullptr)
		FirstWindow = this;
}

void window::send_creation_events()
{
	const auto prev_front = window_get_front();
	if (FrontWindow)
		FrontWindow->next = this;
	FrontWindow = this;
	if (prev_front)
		prev_front->send_event(d_event{EVENT_WINDOW_DEACTIVATED});
	this->send_event(d_create_event{});
	this->send_event(d_event{EVENT_WINDOW_ACTIVATED});
}

window::~window()
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

mixin_trackable_window::~mixin_trackable_window()
{
	if (exists)
		*exists = false;
}

int window_close(window *wind)
{
	if (wind == window_get_front())
		wind->send_event(d_event{EVENT_WINDOW_DEACTIVATED});	// Deactivate first

	const auto result = wind->send_event(d_event{EVENT_WINDOW_CLOSE});
	if (result == window_event_result::handled)
	{
		// User 'handled' the event, cancelling close
		if (wind == window_get_front())
			wind->send_event(d_event{EVENT_WINDOW_ACTIVATED});
		return 0;
	}

	menu_destroy_hook(wind);

	if (result != window_event_result::deleted)	// don't attempt to re-delete
		delete wind;

	if (const auto prev = window_get_front())
		prev->send_event(d_event{EVENT_WINDOW_ACTIVATED});

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
			prev->send_event(d_event{EVENT_WINDOW_DEACTIVATED});
		wind.send_event(d_event{EVENT_WINDOW_ACTIVATED});
	}
}

window *window::set_visible(uint8_t visible)
{
	window *prev = window_get_front();
	w_visible = visible;
	auto wind = window_get_front();	// get the new front window
	if (wind == prev)
		return wind;
	
	if (prev)
		prev->send_event(d_event{EVENT_WINDOW_DEACTIVATED});

	if (wind)
		wind->send_event(d_event{EVENT_WINDOW_ACTIVATED});
	return wind;
}

window_event_result window::send_event(const d_event &event
#if DXX_HAVE_CXX_BUILTIN_FILE_LINE
									, const char *const file, const unsigned line
#endif
									)
{
#if DXX_HAVE_CXX_BUILTIN_FILE_LINE
	con_printf(CON_DEBUG, "%s:%u: sending event %i to window of dimensions %dx%d", file, line, event.type, w_canv.cv_bitmap.bm_w, w_canv.cv_bitmap.bm_h);
#endif
	const auto r = event_handler(event);
	if (r == window_event_result::close)
		if (window_close(this))
			return window_event_result::deleted;
	return r;
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
