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
#include "mouse.h"
#include "playsave.h"

struct mousebutton {
	ubyte pressed;
	fix64 time_went_down;
	fix64 time_held_down;
	uint  num_downs;
	uint  num_ups;
};

static struct mouseinfo {
	struct mousebutton buttons[MOUSE_MAX_BUTTONS];
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
	window *wind;

	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();

	if (mbe->state == SDL_PRESSED) {
		Mouse.buttons[button].pressed = 1;
		Mouse.buttons[button].time_went_down = timer_query();
		Mouse.buttons[button].num_downs++;

		if (button == MBTN_Z_UP) {
			Mouse.delta_z += Z_SENSITIVITY;
			Mouse.z += Z_SENSITIVITY;
		} else if (button == MBTN_Z_DOWN) {
			Mouse.delta_z -= Z_SENSITIVITY;
			Mouse.z -= Z_SENSITIVITY;
		}
	} else {
		Mouse.buttons[button].pressed = 0;
		Mouse.buttons[button].time_held_down += timer_query() - Mouse.buttons[button].time_went_down;
		Mouse.buttons[button].num_ups++;
	}
	
	event.type = (mbe->state == SDL_PRESSED) ? EVENT_MOUSE_BUTTON_DOWN : EVENT_MOUSE_BUTTON_UP;
	event.button = button;
	
	if ((wind = window_get_front()))
	{
		con_printf(CON_DEBUG, "Sending event %s, button %d, coords %d,%d,%d\n",
				   (mbe->state == SDL_PRESSED) ? "EVENT_MOUSE_BUTTON_DOWN" : "EVENT_MOUSE_BUTTON_UP", event.button, Mouse.x, Mouse.y, Mouse.z);
		if (!window_send_event(wind, (d_event *)&event))
			call_default_handler((d_event *)&event);
	}
	else
		call_default_handler((d_event *)&event);
}

void mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
	if (GameArg.CtlNoMouse)
		return;

	Mouse.cursor_time = timer_query();
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
}

void mouse_flush()	// clears all mice events...
{
	int i;

//	event_poll();

	for (i=0; i<MOUSE_MAX_BUTTONS; i++) {
		Mouse.buttons[i].pressed=0;
		Mouse.buttons[i].time_went_down=timer_query();
		Mouse.buttons[i].time_held_down=0;
		Mouse.buttons[i].num_ups=0;
		Mouse.buttons[i].num_downs=0;
	}
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

void mouse_get_delta( int *dx, int *dy, int *dz )
{
	SDL_GetRelativeMouseState( &Mouse.delta_x, &Mouse.delta_y );
	*dx = Mouse.delta_x;
	*dy = Mouse.delta_y;
	*dz = Mouse.delta_z;

	// filter delta?
	if (PlayerCfg.MouseFilter)
	{
		Mouse.delta_x = (*dx + Mouse.old_delta_x) * 0.5;
		Mouse.delta_y = (*dy + Mouse.old_delta_y) * 0.5;
	}

	Mouse.old_delta_x = *dx;
	Mouse.old_delta_y = *dy;

	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
}

int mouse_get_button(d_event *event)
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
		if (Mouse.buttons[i].pressed)
			status |= flag;
		flag <<= 1;
	}

	return status;
}

// Returns how long this button has been down since last call.
fix64 mouse_button_down_time(int button)
{
	fix64 time_down, time;

//	event_poll();

	if (!Mouse.buttons[button].pressed) {
		time_down = Mouse.buttons[button].time_held_down;
		Mouse.buttons[button].time_held_down = 0;
	} else {
		time = timer_query();
		time_down = time - Mouse.buttons[button].time_held_down;
		Mouse.buttons[button].time_held_down = time;
	}
	return time_down;
}

// Returns how many times this button has went down since last call
int mouse_button_down_count(int button)
{
	int count;

//	event_poll();

	count = Mouse.buttons[button].num_downs;
	Mouse.buttons[button].num_downs = 0;

	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)
{
//	event_poll();
	return Mouse.buttons[button].pressed;
}

void mouse_toggle_cursor(int activate)
{
	Mouse.cursor_enabled = (activate && !GameArg.CtlNoMouse);
	if (!Mouse.cursor_enabled)
		SDL_ShowCursor(SDL_DISABLE);
}

// If we want to display/hide cursor, do so if not already and also hide it automatically after some time.
void mouse_cursor_autohide()
{
	int show = SDL_ShowCursor(SDL_QUERY);

	if (Mouse.cursor_enabled)
	{
		if ( (Mouse.cursor_time + (F1_0*2)) >= timer_query() && !show)
			SDL_ShowCursor(SDL_ENABLE);
		else if ( (Mouse.cursor_time + (F1_0*2)) < timer_query() && show)
			SDL_ShowCursor(SDL_DISABLE);
	}
	else
	{
		if (show)
			SDL_ShowCursor(SDL_DISABLE);
	}
}
