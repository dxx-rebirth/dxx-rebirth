/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/joy.c,v $
 * $Revision: 1.4 $
 * $Author: bradleyb $
 * $Date: 2002-03-23 09:13:00 $
 *
 * SDL joystick support
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2002/03/05 12:13:33  bradleyb
 * SDL joystick stuff mostly done
 *
 * Revision 1.2  2001/12/03 02:43:02  bradleyb
 * lots of makefile fixes, and sdl joystick stuff
 *
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.1  2001/10/10 03:01:29  bradleyb
 * Replacing win32 joystick (broken) with SDL joystick (stubs)
 *
 *
 */

#define JOYSTICK_DEBUG

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"
#include "event.h"

#define MAX_JOYSTICKS 16
#define MAX_AXES 32

#define MAX_AXES_PER_JOYSTICK 8
#define MAX_BUTTONS_PER_JOYSTICK 16

char joy_present = 0;
int num_joysticks = 0;

int joy_deadzone = 0;

struct joybutton {
	int state;
	fix time_went_down;
	int num_downs;
	int num_ups;
};

struct joyaxis {
	int		value;
	int		min_val;
	int		center_val;
	int		max_val;
};

static struct joyinfo {
	int n_axes;
	int n_buttons;
	struct joyaxis axes[MAX_AXES];
	struct joybutton buttons[MAX_BUTTONS];
} Joystick;

static struct {
	SDL_Joystick *handle;
	int n_axes;
	int n_buttons;
	int axis_map[MAX_AXES_PER_JOYSTICK];
	int button_map[MAX_BUTTONS_PER_JOYSTICK];
} SDL_Joysticks[MAX_JOYSTICKS];

void joy_button_handler(SDL_JoyButtonEvent *jbe)
{
	int button;

	button = SDL_Joysticks[jbe->which].button_map[jbe->button];

	Joystick.buttons[button].state = jbe->state;

	switch (jbe->type) {
	case SDL_JOYBUTTONDOWN:
		Joystick.buttons[button].time_went_down
			= timer_get_fixed_seconds();
		Joystick.buttons[button].num_downs++;
		break;
	case SDL_JOYBUTTONUP:
		Joystick.buttons[button].num_ups++;
		break;
	}
}

void joy_axis_handler(SDL_JoyAxisEvent *jae)
{
	int axis;

	axis = SDL_Joysticks[jae->which].axis_map[jae->axis];
	
	Joystick.axes[axis].value = jae->value;
}


/* ----------------------------------------------- */

int joy_init()
{
	int i,j,n;

	memset(&Joystick,0,sizeof(Joystick));

	n = SDL_NumJoysticks();

	con_printf(CON_VERBOSE, "sdl-joystick: found %d joysticks\n", n);
	for (i = 0; i < n; i++) {
		con_printf(CON_VERBOSE, "sdl-joystick %d: %s\n", i, SDL_JoystickName(i));
		SDL_Joysticks[num_joysticks].handle = SDL_JoystickOpen(i);
		if (SDL_Joysticks[num_joysticks].handle) {
			joy_present = 1;
			SDL_Joysticks[num_joysticks].n_axes
				= SDL_JoystickNumAxes(SDL_Joysticks[num_joysticks].handle);
			SDL_Joysticks[num_joysticks].n_buttons
				= SDL_JoystickNumButtons(SDL_Joysticks[num_joysticks].handle);
			con_printf(CON_VERBOSE, "sdl-joystick: %d axes\n", SDL_Joysticks[num_joysticks].n_axes);
			con_printf(CON_VERBOSE, "sdl-joystick: %d buttons\n", SDL_Joysticks[num_joysticks].n_buttons);
			for (j=0; j < SDL_Joysticks[num_joysticks].n_axes; j++)
				SDL_Joysticks[num_joysticks].axis_map[j] = Joystick.n_axes++;
			for (j=0; j < SDL_Joysticks[num_joysticks].n_buttons; j++)
				SDL_Joysticks[num_joysticks].button_map[j] = Joystick.n_buttons++;
			num_joysticks++;
		} else
			con_printf(CON_VERBOSE, "sdl-joystick: initialization failed!\n");
		con_printf(CON_VERBOSE, "sdl-joystick: %d axes (total)\n", Joystick.n_axes);
		con_printf(CON_VERBOSE, "sdl-joystick: %d buttons (total)\n", Joystick.n_buttons);
	}
		
	return joy_present;
}

void joy_close()
{
	while (num_joysticks)
		SDL_JoystickClose(SDL_Joysticks[--num_joysticks].handle);
}

void joy_get_pos(int *x, int *y)
{
	int axis[MAX_AXES];

	if (!num_joysticks) {
		*x=*y=0;
		return;
	}

	joystick_read_raw_axis (JOY_ALL_AXIS, axis);

	*x = joy_get_scaled_reading( axis[0], 0 );
	*y = joy_get_scaled_reading( axis[1], 1 );
}

int joy_get_btns()
{
#if 0 // This is never used?
	int i, buttons = 0;
	for (i=0; i++; i<buttons) {
		switch (Joystick.buttons[i].state) {
		case SDL_PRESSED:
			buttons |= 1<<i;
			break;
		case SDL_RELEASED:
			break;
		}
	}
	return buttons;
#else
	return 0;
#endif
}

int joy_get_button_down_cnt( int btn )
{
	int num_downs;

	if (!num_joysticks)
		return 0;

	event_poll();

	num_downs = Joystick.buttons[btn].num_downs;
	Joystick.buttons[btn].num_downs = 0;

	return num_downs;
}

fix joy_get_button_down_time(int btn)
{
	fix time;

	if (!num_joysticks)
		return 0;

	event_poll();

	switch (Joystick.buttons[btn].state) {
	case SDL_PRESSED:
		time = timer_get_fixed_seconds() - Joystick.buttons[btn].time_went_down;
		Joystick.buttons[btn].time_went_down = timer_get_fixed_seconds();
		break;
	case SDL_RELEASED:
		time = 0;
		break;
	}

	return time;
}

ubyte joystick_read_raw_axis( ubyte mask, int * axis )
{
	int i;
	
	if (!num_joysticks)
		return 0;

	event_poll();

	for (i = 0; i <= JOY_NUM_AXES; i++) {
		axis[i] = Joystick.axes[i].value;
	}

	return 0;
}

void joy_flush()
{
	int i;

	if (!num_joysticks)
		return;

	for (i = 0; i < Joystick.n_buttons; i++) {
		Joystick.buttons[i].time_went_down = 0;
		Joystick.buttons[i].num_downs = 0;
	}
	
}

int joy_get_button_state( int btn )
{
	if (!num_joysticks)
		return 0;

	if(btn >= Joystick.n_buttons)
		return 0;

	event_poll();

	return Joystick.buttons[btn].state;
}

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_NUM_AXES; i++) {
		axis_center[i] = Joystick.axes[i].center_val;
		axis_min[i] = Joystick.axes[i].min_val;
		axis_max[i] = Joystick.axes[i].max_val;
	}
}

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < JOY_NUM_AXES; i++) {
		Joystick.axes[i].center_val = axis_center[i];
		Joystick.axes[i].min_val = axis_min[i];
		Joystick.axes[i].max_val = axis_max[i];
	}
}

int joy_get_scaled_reading( int raw, int axis_num )
{
#if 1
	return raw/256;
#else
	int d, x;

	raw -= Joystick.axes[axis_num].center_val;
	
	if (raw < 0)
		d = Joystick.axes[axis_num].center_val - Joystick.axes[axis_num].min_val;
	else if (raw > 0)
		d = Joystick.axes[axis_num].max_val - Joystick.axes[axis_num].center_val;
	else
		d = 0;
	
	if (d)
		x = ((raw << 7) / d);
	else
		x = 0;
	
	if ( x < -128 )
		x = -128;
	if ( x > 127 )
		x = 127;
	
	d =  (joy_deadzone) * 6;
	if ((x > (-1*d)) && (x < d))
		x = 0;
	
	return x;
#endif
}

void joy_set_slow_reading( int flag )
{
}
