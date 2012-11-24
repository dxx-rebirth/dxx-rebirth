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

struct window
{
	grs_canvas w_canv;					// the window's canvas to draw to
	int (*w_callback)(window *wind, d_event *event, void *data);	// the event handler
	int w_visible;						// whether it's visible
	int w_modal;						// modal = accept all user input exclusively
	void *data;							// whatever the user wants (eg menu data for 'newmenu' menus)
	struct window *prev;				// the previous window in the doubly linked list
	struct window *next;				// the next window in the doubly linked list
};

static window *FrontWindow = NULL;
static window *FirstWindow = NULL;

window *window_create(grs_canvas *src, int x, int y, int w, int h, int (*event_callback)(window *wind, d_event *event, void *data), void *data)
{
	window *wind;
	window *prev = window_get_front();
	d_event event;
	
	MALLOC(wind, window, 1);
	if (wind == NULL)
		return NULL;

	Assert(src != NULL);
	Assert(event_callback != NULL);
	gr_init_sub_canvas(&wind->w_canv, src, x, y, w, h);
	wind->w_callback = event_callback;
	wind->w_visible = 1;	// default to visible
	wind->w_modal =	1;		// default to modal
	wind->data = data;

	if (FirstWindow == NULL)
		FirstWindow = wind;
	wind->prev = FrontWindow;
	if (FrontWindow)
		FrontWindow->next = wind;
	wind->next = NULL;
	FrontWindow = wind;
	if (prev)
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);

	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);

	return wind;
}

int window_close(window *wind)
{
	window *prev;
	d_event event;
	int (*w_callback)(window *wind, d_event *event, void *data) = wind->w_callback;

	if (wind == window_get_front())
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);	// Deactivate first

	event.type = EVENT_WINDOW_CLOSE;
	con_printf(CON_DEBUG,	"Sending event EVENT_WINDOW_CLOSE to window of dimensions %dx%d\n",
			   (wind)->w_canv.cv_bitmap.bm_w, (wind)->w_canv.cv_bitmap.bm_h);
	if (window_send_event(wind, &event))
	{
		// User 'handled' the event, cancelling close
		if (wind == window_get_front())
		{
			WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
		}
		return 0;
	}

	if (wind == FrontWindow)
		FrontWindow = wind->prev;
	if (wind == FirstWindow)
		FirstWindow = wind->next;
	if (wind->next)
		wind->next->prev = wind->prev;
	if (wind->prev)
		wind->prev->next = wind->next;

	if ((prev = window_get_front()))
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_ACTIVATED);

	d_free(wind);
	
	event.type = EVENT_WINDOW_CLOSED;
	w_callback(wind, &event, NULL);	// callback needs to recognise this is a NULL pointer!
	
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

window *window_get_next(window *wind)
{
	return wind->next;
}

window *window_get_prev(window *wind)
{
	return wind->prev;
}

// Make wind the front window
void window_select(window *wind)
{
	window *prev = window_get_front();
	d_event event;

	Assert (wind != NULL);

	if (wind == FrontWindow)
		return;
	if ((wind == FirstWindow) && FirstWindow->next)
		FirstWindow = FirstWindow->next;

	if (wind->next)
		wind->next->prev = wind->prev;
	if (wind->prev)
		wind->prev->next = wind->next;
	wind->prev = FrontWindow;
	FrontWindow->next = wind;
	wind->next = NULL;
	FrontWindow = wind;
	
	if (window_is_visible(wind))
	{
		if (prev)
			WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
	}
}

void window_set_visible(window *wind, int visible)
{
	window *prev = window_get_front();
	d_event event;

	wind->w_visible = visible;
	wind = window_get_front();	// get the new front window
	if (wind == prev)
		return;
	
	if (prev)
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);

	if (wind)
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
}

int window_is_visible(window *wind)
{
	return wind->w_visible;
}

grs_canvas *window_get_canvas(window *wind)
{
	return &wind->w_canv;
}

extern void window_update_canvases(void)
{
	window *wind;
	
	for (wind = FirstWindow; wind != NULL; wind = wind->next)
		gr_init_sub_bitmap (&wind->w_canv.cv_bitmap,
							wind->w_canv.cv_bitmap.bm_parent,
							wind->w_canv.cv_bitmap.bm_x,
							wind->w_canv.cv_bitmap.bm_y,
							wind->w_canv.cv_bitmap.bm_w,
							wind->w_canv.cv_bitmap.bm_h);
}

int window_send_event(window *wind, d_event *event)
{
	return wind->w_callback(wind, event, wind->data);
}

void window_set_modal(window *wind, int modal)
{
	wind->w_modal = modal;
}

int window_is_modal(window *wind)
{
	return wind->w_modal;
}
