/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/joy.c,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-12-03 02:43:02 $
 *
 * SDL joystick support
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.1  2001/10/10 03:01:29  bradleyb
 * Replacing win32 joystick (broken) with SDL joystick (stubs)
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"

#define MAX_JOYSTICKS 16
#define MAX_AXES 32

#define MAX_AXES_PER_JOYSTICK 8
#define MAX_BUTTONS_PER_JOYSTICK 16

char joy_present = 0;
int num_joysticks = 0;

struct joybutton {
	int state;
	fix time_went_down;
	fix time_held_down;
	int num_downs;
	int num_ups;
};

static struct joyinfo {
	int n_axes;
	int n_buttons;
	int axes[MAX_AXES];
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
		Joystick.buttons[button].time_held_down
			+= timer_get_fixed_seconds()
			- Joystick.buttons[button].time_went_down;
		Joystick.buttons[button].num_ups++;
		break;
	}
}

void joy_axis_handler(SDL_JoyAxisEvent *jae)
{
	int axis;

	axis = SDL_Joysticks[jae->which].axis_map[jae->axis];
	
	Joystick.axes[axis] = jae->value;
}


/* ----------------------------------------------- */

int joy_init()
{
	int i,j,n;

	memset(&Joystick,0,sizeof(Joystick));

	n = SDL_NumJoysticks();

	con_printf(CON_VERBOSE, "Joystick: found %d joysticks\n", n);
	for (i = 0; i < n; i++) {
		con_printf(CON_VERBOSE, "Joystick %d: %s\n", i, SDL_JoystickName(i));
		SDL_Joysticks[num_joysticks].handle = SDL_JoystickOpen(i);
		if (SDL_Joysticks[num_joysticks].handle) {
			joy_present = 1;
			SDL_Joysticks[num_joysticks].n_axes
				= SDL_JoystickNumAxes(SDL_Joysticks[num_joysticks].handle);
			SDL_Joysticks[num_joysticks].n_buttons
				= SDL_JoystickNumButtons(SDL_Joysticks[num_joysticks].handle);
			con_printf(CON_VERBOSE, "Joystick: %d axes\n", SDL_Joysticks[num_joysticks].n_axes);
			con_printf(CON_VERBOSE, "Joystick: %d buttons\n", SDL_Joysticks[num_joysticks].n_buttons);
			for (j=0; j < SDL_Joysticks[num_joysticks].n_axes; j++)
				SDL_Joysticks[num_joysticks].axis_map[j] = Joystick.n_axes++;
			for (j=0; j < SDL_Joysticks[num_joysticks].n_buttons; j++)
				SDL_Joysticks[num_joysticks].button_map[j] = Joystick.n_buttons++;
			num_joysticks++;
		} else
			con_printf(CON_VERBOSE, "Joystick: initialization failed!\n");
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
	*x = Joystick.axes[0] << 8;
	*y = Joystick.axes[1] << 8;
}

int joy_get_btns()
{
	return 0;
}

int joy_get_button_down_cnt( int btn )
{
	return 0;
}

fix joy_get_button_down_time(int btn)
{
	fix time;

	time = Joystick.buttons[btn].time_held_down;
	Joystick.buttons[btn].time_held_down = 0;

	return time;
}

ubyte joystick_read_raw_axis( ubyte mask, int * axis )
{
	return 0;
}

void joy_flush()
{
}

int joy_get_button_state( int btn )
{
	return 0;
}

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
}

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
}

int joy_get_scaled_reading( int raw, int axn )
{
	return 0;
}

void joy_set_slow_reading( int flag )
{
}
