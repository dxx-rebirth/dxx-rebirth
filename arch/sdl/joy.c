/* $Id: joy.c,v 1.16 2005-04-04 08:56:34 btb Exp $ */
/*
 *
 * SDL joystick support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>   // for memset
#include <SDL.h>

#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "text.h"
#include "u_mem.h"

#define MAX_JOYSTICKS 16

#define MAX_AXES_PER_JOYSTICK 8
#define MAX_BUTTONS_PER_JOYSTICK 16
#define MAX_HATS_PER_JOYSTICK 4

extern char *joybutton_text[]; //from kconfig.c
extern char *joyaxis_text[]; //from kconfig.c

char joy_present = 0;
int num_joysticks = 0;

int joy_deadzone = 0;

int joy_num_axes = 0;

struct joybutton {
	int state;
	int last_state;
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

/* This struct is a "virtual" joystick, which includes all the axes
 * and buttons of every joystick found.
 */
static struct joyinfo {
	int n_axes;
	int n_buttons;
	struct joyaxis axes[JOY_MAX_AXES];
	struct joybutton buttons[MAX_BUTTONS];
} Joystick;

/* This struct is an array, with one entry for each physical joystick
 * found.
 */
static struct {
	SDL_Joystick *handle;
	int n_axes;
	int n_buttons;
	int n_hats;
	int hat_map[MAX_HATS_PER_JOYSTICK];  //Note: Descent expects hats to be buttons, so these are indices into Joystick.buttons
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

void joy_hat_handler(SDL_JoyHatEvent *jhe)
{
	int hat = SDL_Joysticks[jhe->which].hat_map[jhe->hat];
	int hbi;

	//Save last state of the hat-button
	Joystick.buttons[hat  ].last_state = Joystick.buttons[hat  ].state;
	Joystick.buttons[hat+1].last_state = Joystick.buttons[hat+1].state;
	Joystick.buttons[hat+2].last_state = Joystick.buttons[hat+2].state;
	Joystick.buttons[hat+3].last_state = Joystick.buttons[hat+3].state;

	//get current state of the hat-button
	Joystick.buttons[hat  ].state = ((jhe->value & SDL_HAT_UP)>0);
	Joystick.buttons[hat+1].state = ((jhe->value & SDL_HAT_RIGHT)>0);
	Joystick.buttons[hat+2].state = ((jhe->value & SDL_HAT_DOWN)>0);
	Joystick.buttons[hat+3].state = ((jhe->value & SDL_HAT_LEFT)>0);

	//determine if a hat-button up or down event based on state and last_state
	for(hbi=0;hbi<4;hbi++)
	{
		if(	!Joystick.buttons[hat+hbi].last_state && Joystick.buttons[hat+hbi].state) //last_state up, current state down
		{
			Joystick.buttons[hat+hbi].time_went_down
				= timer_get_fixed_seconds();
			Joystick.buttons[hat+hbi].num_downs++;
		}
		else if(Joystick.buttons[hat+hbi].last_state && !Joystick.buttons[hat+hbi].state)  //last_state down, current state up
		{
			Joystick.buttons[hat+hbi].num_ups++;
		}
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
	char temp[10];

	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		con_printf(CON_VERBOSE, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
		return 0;
	}

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
			if(SDL_Joysticks[num_joysticks].n_axes > MAX_AXES_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d axes, only %d supported.  Game may be unstable.\n", SDL_Joysticks[num_joysticks].n_axes, MAX_AXES_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_axes = MAX_AXES_PER_JOYSTICK;
			}

			SDL_Joysticks[num_joysticks].n_buttons
				= SDL_JoystickNumButtons(SDL_Joysticks[num_joysticks].handle);
			if(SDL_Joysticks[num_joysticks].n_buttons > MAX_BUTTONS_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d buttons, only %d supported.  Game may be unstable.\n", SDL_Joysticks[num_joysticks].n_buttons, MAX_BUTTONS_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_buttons = MAX_BUTTONS_PER_JOYSTICK;
			}

			SDL_Joysticks[num_joysticks].n_hats
				= SDL_JoystickNumHats(SDL_Joysticks[num_joysticks].handle);
			if(SDL_Joysticks[num_joysticks].n_hats > MAX_HATS_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d hats, only %d supported.  Game may be unstable.\n", SDL_Joysticks[num_joysticks].n_hats, MAX_HATS_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_hats = MAX_HATS_PER_JOYSTICK;
			}

			con_printf(CON_VERBOSE, "sdl-joystick: %d axes\n", SDL_Joysticks[num_joysticks].n_axes);
			con_printf(CON_VERBOSE, "sdl-joystick: %d buttons\n", SDL_Joysticks[num_joysticks].n_buttons);
			con_printf(CON_VERBOSE, "sdl-joystick: %d hats\n", SDL_Joysticks[num_joysticks].n_hats);

			for (j=0; j < SDL_Joysticks[num_joysticks].n_axes; j++)
			{
				sprintf(temp, "J%d A%d", i + 1, j + 1);
				joyaxis_text[Joystick.n_axes] = d_strdup(temp);
				SDL_Joysticks[num_joysticks].axis_map[j] = Joystick.n_axes++;
			}
			for (j=0; j < SDL_Joysticks[num_joysticks].n_buttons; j++)
			{
				sprintf(temp, "J%d B%d", i + 1, j + 1);
				joybutton_text[Joystick.n_buttons] = d_strdup(temp);
				SDL_Joysticks[num_joysticks].button_map[j] = Joystick.n_buttons++;
			}
			for (j=0; j < SDL_Joysticks[num_joysticks].n_hats; j++)
			{
				SDL_Joysticks[num_joysticks].hat_map[j] = Joystick.n_buttons;
				//a hat counts as four buttons
				sprintf(temp, "J%d H%d%c", i + 1, j + 1, 0202);
				joybutton_text[Joystick.n_buttons++] = d_strdup(temp);
				sprintf(temp, "J%d H%d%c", i + 1, j + 1, 0177);
				joybutton_text[Joystick.n_buttons++] = d_strdup(temp);
				sprintf(temp, "J%d H%d%c", i + 1, j + 1, 0200);
				joybutton_text[Joystick.n_buttons++] = d_strdup(temp);
				sprintf(temp, "J%d H%d%c", i + 1, j + 1, 0201);
				joybutton_text[Joystick.n_buttons++] = d_strdup(temp);
			}

			num_joysticks++;
		}
		else
			con_printf(CON_VERBOSE, "sdl-joystick: initialization failed!\n");

		con_printf(CON_VERBOSE, "sdl-joystick: %d axes (total)\n", Joystick.n_axes);
		con_printf(CON_VERBOSE, "sdl-joystick: %d buttons (total)\n", Joystick.n_buttons);
	}

	joy_num_axes = Joystick.n_axes;
	atexit(joy_close);

	return joy_present;
}

void joy_close()
{
	while (num_joysticks)
		SDL_JoystickClose(SDL_Joysticks[--num_joysticks].handle);
	while (Joystick.n_axes--)
		d_free(joyaxis_text[Joystick.n_axes]);
	while (Joystick.n_buttons--)
		d_free(joybutton_text[Joystick.n_buttons]);
}

void joy_get_pos(int *x, int *y)
{
	int axis[JOY_MAX_AXES];

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
	fix time = F0_0;

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
	ubyte channel_masks = 0;
	
	if (!num_joysticks)
		return 0;

	event_poll();

	for (i = 0; i < Joystick.n_axes; i++)
	{
		if ((axis[i] = Joystick.axes[i].value))
			channel_masks |= 1 << i;
	}

	return channel_masks;
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

	for (i = 0; i < Joystick.n_axes; i++)
	{
		axis_center[i] = Joystick.axes[i].center_val;
		axis_min[i] = Joystick.axes[i].min_val;
		axis_max[i] = Joystick.axes[i].max_val;
	}
}

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

	for (i = 0; i < Joystick.n_axes; i++)
	{
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
