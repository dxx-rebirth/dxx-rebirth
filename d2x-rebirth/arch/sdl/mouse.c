/*
 *
 * SDL mouse driver
 *
 */

#include <string.h>
#include <SDL/SDL.h>

#include "fix.h"
#include "timer.h"
#include "event.h"
#include "window.h"
#include "mouse.h"
#include "playsave.h"
#include "args.h"

static struct mouseinfo {
	ubyte  button_state[MOUSE_MAX_BUTTONS];
	fix64  time_lastpressed[MOUSE_MAX_BUTTONS];
	int    delta_x, delta_y, delta_z, old_delta_x, old_delta_y;
	int    x,y,z;
	int    cursor_enabled;
	fix64  cursor_time;
} Mouse;

typedef struct d_event_mousebutton
{
	event_type type;
	int button;
} d_event_mousebutton;

typedef struct d_event_mouse_moved
{
	event_type	type;	// EVENT_MOUSE_MOVED
	short		dx, dy, dz;
} d_event_mouse_moved;

void mouse_init(void)
{
	memset(&Mouse,0,sizeof(Mouse));
}

void mouse_close(void)
{
	SDL_ShowCursor(SDL_ENABLE);
}

void mouse_button_handler(SDL_MouseButtonEvent *mbe)
{
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	int button_remap[17] = {
		MBTN_LEFT,
		MBTN_MIDDLE,
		MBTN_RIGHT,
		MBTN_Z_UP,
		MBTN_Z_DOWN,
		MBTN_PITCH_BACKWARD,
		MBTN_PITCH_FORWARD,
		MBTN_BANK_LEFT,
		MBTN_BANK_RIGHT,
		MBTN_HEAD_LEFT,
		MBTN_HEAD_RIGHT,
		MBTN_11,
		MBTN_12,
		MBTN_13,
		MBTN_14,
		MBTN_15,
		MBTN_16
	};

	int button = button_remap[mbe->button - 1]; // -1 since SDL seems to start counting at 1
	d_event_mousebutton event;

	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();

	if (mbe->state == SDL_PRESSED) {
		d_event_mouse_moved event2 = { EVENT_MOUSE_MOVED, 0, 0, 0 };

		Mouse.button_state[button] = 1;

		if (button == MBTN_Z_UP) {
			Mouse.delta_z += Z_SENSITIVITY;
			Mouse.z += Z_SENSITIVITY;
			event2.dz = Z_SENSITIVITY;
		} else if (button == MBTN_Z_DOWN) {
			Mouse.delta_z -= Z_SENSITIVITY;
			Mouse.z -= Z_SENSITIVITY;
			event2.dz = -1*Z_SENSITIVITY;
		}
		
		if (event2.dz)
		{
			//con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_MOVED, relative motion %d,%d,%d\n",
			//		   event2.dx, event2.dy, event2.dz);
			event_send((d_event *)&event2);
		}
	} else {
		Mouse.button_state[button] = 0;
	}
	
	event.type = (mbe->state == SDL_PRESSED) ? EVENT_MOUSE_BUTTON_DOWN : EVENT_MOUSE_BUTTON_UP;
	event.button = button;
	
	con_printf(CON_DEBUG, "Sending event %s, button %d, coords %d,%d,%d\n",
			   (mbe->state == SDL_PRESSED) ? "EVENT_MOUSE_BUTTON_DOWN" : "EVENT_MOUSE_BUTTON_UP", event.button, Mouse.x, Mouse.y, Mouse.z);
	event_send((d_event *)&event);
	
	//Double-click support
	if (Mouse.button_state[button])
	{
		if (timer_query() <= Mouse.time_lastpressed[button] + F1_0/5)
		{
			event.type = EVENT_MOUSE_DOUBLE_CLICKED;
			//event.button = button; // already set the button
			con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_DOUBLE_CLICKED, button %d, coords %d,%d\n",
					   event.button, Mouse.x, Mouse.y);
			event_send((d_event *)&event);
		}

		Mouse.time_lastpressed[button] = Mouse.cursor_time;
	}
}

void mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
	d_event_mouse_moved event;
	
	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
	
	event.type = EVENT_MOUSE_MOVED;
	event.dx = mme->xrel;
	event.dy = mme->yrel;
	event.dz = 0;		// handled in mouse_button_handler
	
	Mouse.old_delta_x = event.dx;
	Mouse.old_delta_y = event.dy;
	
	//con_printf(CON_DEBUG, "Sending event EVENT_MOUSE_MOVED, relative motion %d,%d,%d\n",
	//		   event.dx, event.dy, event.dz);
	event_send((d_event *)&event);
}

void mouse_flush()	// clears all mice events...
{
	int i;

//	event_poll();

	for (i=0; i<MOUSE_MAX_BUTTONS; i++)
		Mouse.button_state[i]=0;
	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
	Mouse.old_delta_x = 0;
	Mouse.old_delta_y = 0;
	Mouse.x = 0;
	Mouse.y = 0;
	Mouse.z = 0;
	SDL_GetMouseState(&Mouse.x, &Mouse.y); // necessary because polling only gives us the delta.
}

//========================================================================
void mouse_get_pos( int *x, int *y, int *z )
{
	//event_poll();		// Have to assume this is called in event_process, because event_poll can cause a window to close (depending on what the user does)
	*x=Mouse.x;
	*y=Mouse.y;
	*z=Mouse.z;
}

int mouse_in_window(window *wind)
{
	grs_canvas *canv;
	
	canv = window_get_canvas(wind);
	return	(Mouse.x >= canv->cv_bitmap.bm_x) &&
			(Mouse.x <= canv->cv_bitmap.bm_x + canv->cv_bitmap.bm_w) && 
			(Mouse.y >= canv->cv_bitmap.bm_y) && 
			(Mouse.y <= canv->cv_bitmap.bm_y + canv->cv_bitmap.bm_h);
}

void mouse_get_delta( int *dx, int *dy, int *dz )
{
	SDL_GetRelativeMouseState( &Mouse.delta_x, &Mouse.delta_y );
	*dx = Mouse.delta_x;
	*dy = Mouse.delta_y;
	*dz = Mouse.delta_z;

	Mouse.old_delta_x = *dx;
	Mouse.old_delta_y = *dy;

	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
}

void event_mouse_get_delta(d_event *event, int *dx, int *dy, int *dz)
{
	Assert(event->type == EVENT_MOUSE_MOVED);

	*dx = ((d_event_mouse_moved *)event)->dx;
	*dy = ((d_event_mouse_moved *)event)->dy;
	*dz = ((d_event_mouse_moved *)event)->dz;
}

int event_mouse_get_button(d_event *event)
{
	Assert((event->type == EVENT_MOUSE_BUTTON_DOWN) || (event->type == EVENT_MOUSE_BUTTON_UP));
	return ((d_event_mousebutton *)event)->button;
}

int mouse_get_btns()
{
	int i;
	uint flag=1;
	int status = 0;

//	event_poll();

	for (i=0; i<MOUSE_MAX_BUTTONS; i++ ) {
		if (Mouse.button_state[i])
			status |= flag;
		flag <<= 1;
	}

	return status;
}

void mouse_toggle_cursor(int activate)
{
	Mouse.cursor_enabled = (activate && !GameArg.CtlNoMouse && !GameArg.CtlNoCursor);
	if (!Mouse.cursor_enabled)
		SDL_ShowCursor(SDL_DISABLE);
}

// If we want to display/hide cursor, do so if not already and also hide it automatically after some time.
void mouse_cursor_autohide()
{
	int show = SDL_ShowCursor(SDL_QUERY);
	static fix64 hidden_time = 0;

	if (Mouse.cursor_enabled)
	{
		if ( (Mouse.cursor_time + (F1_0*2)) >= timer_query() && hidden_time + (F1_0/2) < timer_query() && !show)
			SDL_ShowCursor(SDL_ENABLE);
		else if ( (Mouse.cursor_time + (F1_0*2)) < timer_query() && show)
		{
			SDL_ShowCursor(SDL_DISABLE);
			hidden_time = timer_query();
		}
	}
	else
	{
		if (show)
			SDL_ShowCursor(SDL_DISABLE);
	}
}
