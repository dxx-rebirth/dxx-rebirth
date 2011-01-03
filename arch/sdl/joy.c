/*
 *
 * SDL joystick support
 *
 */

#include <string.h>   // for memset
#include <SDL/SDL.h>

#include "joy.h"
#include "error.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "text.h"
#include "u_mem.h"
#include "playsave.h"

extern char *joybutton_text[]; //from kconfig.c
extern char *joyaxis_text[]; //from kconfig.c

int num_joysticks = 0;
int joy_num_axes = 0;

struct joybutton {
	int   state;
	int   last_state;
	fix64 time_went_down;
	int   num_downs;
	int   num_ups;
};

struct joyaxis {
	int value;
	int min_val;
	int center_val;
	int max_val;
};

/* This struct is a "virtual" joystick, which includes all the axes
 * and buttons of every joystick found.
 */
static struct joyinfo {
	int n_axes;
	int n_buttons;
	struct joyaxis axes[JOY_MAX_AXES];
	struct joybutton buttons[JOY_MAX_BUTTONS];
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
		Joystick.buttons[button].time_went_down = timer_query();
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
				= timer_query();
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

void joy_init()
{
	int i,j,n;
	char temp[10];

	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		con_printf(CON_NORMAL, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
		return;
	}

	memset(&Joystick,0,sizeof(Joystick));
	memset(joyaxis_text, 0, JOY_MAX_AXES * sizeof(char *));
	memset(joybutton_text, 0, JOY_MAX_BUTTONS * sizeof(char *));

	n = SDL_NumJoysticks();

	con_printf(CON_NORMAL, "sdl-joystick: found %d joysticks\n", n);
	for (i = 0; i < n; i++) {
		con_printf(CON_NORMAL, "sdl-joystick %d: %s\n", i, SDL_JoystickName(i));
		SDL_Joysticks[num_joysticks].handle = SDL_JoystickOpen(i);
		if (SDL_Joysticks[num_joysticks].handle) {

			SDL_Joysticks[num_joysticks].n_axes
				= SDL_JoystickNumAxes(SDL_Joysticks[num_joysticks].handle);
			if(SDL_Joysticks[num_joysticks].n_axes > MAX_AXES_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d axes, only %d supported.\n", SDL_Joysticks[num_joysticks].n_axes, MAX_AXES_PER_JOYSTICK);
				Warning("sdl-joystick: found %d axes, only %d supported.\n", SDL_Joysticks[num_joysticks].n_axes, MAX_AXES_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_axes = MAX_AXES_PER_JOYSTICK;
			}

			SDL_Joysticks[num_joysticks].n_buttons
				= SDL_JoystickNumButtons(SDL_Joysticks[num_joysticks].handle);
			if(SDL_Joysticks[num_joysticks].n_buttons > MAX_BUTTONS_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d buttons, only %d supported.\n", SDL_Joysticks[num_joysticks].n_buttons, MAX_BUTTONS_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_buttons = MAX_BUTTONS_PER_JOYSTICK;
			}

			SDL_Joysticks[num_joysticks].n_hats
				= SDL_JoystickNumHats(SDL_Joysticks[num_joysticks].handle);
			if(SDL_Joysticks[num_joysticks].n_hats > MAX_HATS_PER_JOYSTICK)
			{
				Warning("sdl-joystick: found %d hats, only %d supported.\n", SDL_Joysticks[num_joysticks].n_hats, MAX_HATS_PER_JOYSTICK);
				SDL_Joysticks[num_joysticks].n_hats = MAX_HATS_PER_JOYSTICK;
			}

			con_printf(CON_NORMAL, "sdl-joystick: %d axes\n", SDL_Joysticks[num_joysticks].n_axes);
			con_printf(CON_NORMAL, "sdl-joystick: %d buttons\n", SDL_Joysticks[num_joysticks].n_buttons);
			con_printf(CON_NORMAL, "sdl-joystick: %d hats\n", SDL_Joysticks[num_joysticks].n_hats);

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
			con_printf(CON_NORMAL, "sdl-joystick: initialization failed!\n");

		con_printf(CON_NORMAL, "sdl-joystick: %d axes (total)\n", Joystick.n_axes);
		con_printf(CON_NORMAL, "sdl-joystick: %d buttons (total)\n", Joystick.n_buttons);
	}

	joy_num_axes = Joystick.n_axes;
}

void joy_close()
{
	SDL_JoystickClose(SDL_Joysticks[num_joysticks].handle);

	while (Joystick.n_axes--)
		d_free(joyaxis_text[Joystick.n_axes]);
	while (Joystick.n_buttons--)
		d_free(joybutton_text[Joystick.n_buttons]);
}

int joy_get_button_down_cnt( int btn )
{
	int num_downs;

	if (!num_joysticks  || btn < 0 || btn >= JOY_MAX_BUTTONS)
		return 0;

//	event_poll();

	num_downs = Joystick.buttons[btn].num_downs;
	Joystick.buttons[btn].num_downs = 0;

	return num_downs;
}

fix joy_get_button_down_time(int btn)
{
	fix time = F0_0;

	if (!num_joysticks  || btn < 0 || btn >= JOY_MAX_BUTTONS)
		return 0;

//	event_poll();

	switch (Joystick.buttons[btn].state) {
	case SDL_PRESSED:
		time = timer_query() - Joystick.buttons[btn].time_went_down;
		Joystick.buttons[btn].time_went_down = timer_query();
		break;
	case SDL_RELEASED:
		time = 0;
		break;
	}

	return time;
}

void joystick_read_raw_axis( int * axis )
{
	int i;
	
	if (!num_joysticks)
		return;

//	event_poll();

	for (i = 0; i < Joystick.n_axes; i++)
		axis[i] = Joystick.axes[i].value;
}

void joy_flush()
{
	int i;

	if (!num_joysticks)
		return;

	for (i = 0; i < Joystick.n_buttons; i++) {
		Joystick.buttons[i].time_went_down = 0;
		Joystick.buttons[i].num_downs = 0;
		Joystick.buttons[i].state = SDL_RELEASED;
	}
	
}

int joy_get_button_state( int btn )
{
	if (!num_joysticks)
		return 0;

	if(btn >= Joystick.n_buttons)
		return 0;

//	event_poll();

	return Joystick.buttons[btn].state;
}

int joy_get_scaled_reading( int raw )
{
	return raw/256;
}
