/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL joystick support
 *
 */

#include "joy.h"
#include "dxxerror.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "u_mem.h"
#include "playsave.h"
#include "kconfig.h"

namespace {

int num_joysticks = 0;

/* This struct is a "virtual" joystick, which includes all the axes
 * and buttons of every joystick found.
 */
static struct joyinfo {
	int n_axes;
	int n_buttons;
	array<uint8_t, JOY_MAX_BUTTONS> button_state;
} Joystick;

struct d_event_joystickbutton : d_event
{
	int button;
};

struct d_event_joystick_moved : d_event, d_event_joystick_axis_value
{
};

/* This struct is an array, with one entry for each physical joystick
 * found.
 */
struct d_physical_joystick {
	SDL_Joystick *handle;
	int n_axes;
	int n_buttons;
	int n_hats;
	int hat_map[MAX_HATS_PER_JOYSTICK];  //Note: Descent expects hats to be buttons, so these are indices into Joystick.buttons
	array<unsigned, MAX_AXES_PER_JOYSTICK> axis_map;
	array<int, MAX_AXES_PER_JOYSTICK> axis_value;
	int button_map[MAX_BUTTONS_PER_JOYSTICK];
};

}

static array<d_physical_joystick, MAX_JOYSTICKS> SDL_Joysticks;

void joy_button_handler(SDL_JoyButtonEvent *jbe)
{
	int button;
	d_event_joystickbutton event;

	button = SDL_Joysticks[jbe->which].button_map[jbe->button];

	Joystick.button_state[button] = jbe->state;

	event.type = (jbe->type == SDL_JOYBUTTONDOWN) ? EVENT_JOYSTICK_BUTTON_DOWN : EVENT_JOYSTICK_BUTTON_UP;
	event.button = button;
	con_printf(CON_DEBUG, "Sending event %s, button %d", (jbe->type == SDL_JOYBUTTONDOWN) ? "EVENT_JOYSTICK_BUTTON_DOWN" : "EVENT_JOYSTICK_JOYSTICK_UP", event.button);
	event_send(event);
}

void joy_hat_handler(SDL_JoyHatEvent *jhe)
{
	int hat = SDL_Joysticks[jhe->which].hat_map[jhe->hat];
	//Save last state of the hat-button

	//get current state of the hat-button
	const auto jhe_value = jhe->value;
	/* Every value must have exactly one bit set, and the union must
	 * cover the lower four bits.  If any of these assertions fail, the
	 * loop will not work right.
	 */
#define assert_hat_one_bit(M)	\
	static_assert(!((SDL_HAT_##M) & ((SDL_HAT_##M) - 1)), "unexpected " #M " mask");
	assert_hat_one_bit(UP);
	assert_hat_one_bit(RIGHT);
	assert_hat_one_bit(DOWN);
	assert_hat_one_bit(LEFT);
#undef assert_hat_one_bit
	static_assert((SDL_HAT_UP | SDL_HAT_RIGHT | SDL_HAT_DOWN | SDL_HAT_LEFT) == 0xf, "unexpected hat mask");

	//determine if a hat-button up or down event based on state and last_state
	for (uint_fast32_t i = 0; i != 4; ++i)
	{
		const auto current_button_state = !!(jhe_value & (1 << i));
		auto &saved_button_state = Joystick.button_state[hat + i];
		if (saved_button_state == current_button_state)
			// Same state as before
			continue;
		saved_button_state = current_button_state;
		d_event_joystickbutton event;
		event.button = hat + i;
		if (current_button_state) //last_state up, current state down
		{
			event.type = EVENT_JOYSTICK_BUTTON_DOWN;
			con_printf(CON_DEBUG, "Sending event EVENT_JOYSTICK_BUTTON_DOWN, button %d", event.button);
		}
		else	//last_state down, current state up
		{
			event.type = EVENT_JOYSTICK_BUTTON_UP;
			con_printf(CON_DEBUG, "Sending event EVENT_JOYSTICK_BUTTON_UP, button %d", event.button);
		}
		event_send(event);
	}
}

int joy_axis_handler(SDL_JoyAxisEvent *jae)
{
	d_event_joystick_moved event;
	auto &js = SDL_Joysticks[jae->which];
	const auto axis = js.axis_map[jae->axis];
	auto &axis_value = js.axis_value[jae->axis];
	// inaccurate stick is inaccurate. SDL might send SDL_JoyAxisEvent even if the value is the same as before.
	if (axis_value == jae->value/256)
		return 0;

	event.value = axis_value = jae->value/256;
	event.type = EVENT_JOYSTICK_MOVED;
	event.axis = axis;
	con_printf(CON_DEBUG, "Sending event EVENT_JOYSTICK_MOVED, axis: %d, value: %d",event.axis, event.value);
	event_send(event);

	return 1;
}


/* ----------------------------------------------- */

template <unsigned MAX, typename T>
static T check_warn_joy_support_limit(const T n, const char *const desc)
{
	if (n <= MAX)
	{
		con_printf(CON_NORMAL, "sdl-joystick: %d %ss", n, desc);
		return n;
	}
	Warning("sdl-joystick: found %d %ss, only %d supported.\n", n, desc, MAX);
	return MAX;
}

void joy_init()
{
	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		con_printf(CON_NORMAL, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
		return;
	}

	Joystick = {};
	joyaxis_text.clear();
	joybutton_text.clear();

	const auto n = check_warn_joy_support_limit<MAX_JOYSTICKS>(SDL_NumJoysticks(), "joystick");
	for (int i = 0; i < n; i++) {
		auto &joystick = SDL_Joysticks[num_joysticks];
		const auto handle = joystick.handle = SDL_JoystickOpen(i);
#if SDL_MAJOR_VERSION == 1
		con_printf(CON_NORMAL, "sdl-joystick %d: %s", i, SDL_JoystickName(i));
#else
		con_printf(CON_NORMAL, "sdl-joystick %d: %s", i, SDL_JoystickName(handle));
#endif
		if (handle)
		{
			const auto n_axes = joystick.n_axes = check_warn_joy_support_limit<MAX_AXES_PER_JOYSTICK>(SDL_JoystickNumAxes(handle), "axe");
			const auto n_buttons = joystick.n_buttons = check_warn_joy_support_limit<MAX_BUTTONS_PER_JOYSTICK>(SDL_JoystickNumButtons(handle), "button");
			const auto n_hats = joystick.n_hats = check_warn_joy_support_limit<MAX_HATS_PER_JOYSTICK>(SDL_JoystickNumHats(handle), "hat");

			joyaxis_text.resize(joyaxis_text.size() + n_axes);
			for (int j=0; j < n_axes; j++)
			{
				snprintf(&joyaxis_text[Joystick.n_axes][0], sizeof(joyaxis_text[Joystick.n_axes]), "J%d A%d", i + 1, j + 1);
				joystick.axis_map[j] = Joystick.n_axes++;
			}
			joybutton_text.resize(joybutton_text.size() + n_buttons + (4 * n_hats));
			for (int j=0; j < n_buttons; j++)
			{
				snprintf(&joybutton_text[Joystick.n_buttons][0], sizeof(joybutton_text[Joystick.n_buttons]), "J%d B%d", i + 1, j + 1);
				joystick.button_map[j] = Joystick.n_buttons++;
			}
			for (int j=0; j < n_hats; j++)
			{
				joystick.hat_map[j] = Joystick.n_buttons;
				//a hat counts as four buttons
				snprintf(&joybutton_text[Joystick.n_buttons++][0], sizeof(joybutton_text[0]), "J%d H%d%c", i + 1, j + 1, 0202);
				snprintf(&joybutton_text[Joystick.n_buttons++][0], sizeof(joybutton_text[0]), "J%d H%d%c", i + 1, j + 1, 0177);
				snprintf(&joybutton_text[Joystick.n_buttons++][0], sizeof(joybutton_text[0]), "J%d H%d%c", i + 1, j + 1, 0200);
				snprintf(&joybutton_text[Joystick.n_buttons++][0], sizeof(joybutton_text[0]), "J%d H%d%c", i + 1, j + 1, 0201);
			}

			num_joysticks++;
		}
		else
			con_printf(CON_NORMAL, "sdl-joystick: initialization failed!");

		con_printf(CON_NORMAL, "sdl-joystick: %d axes (total)", Joystick.n_axes);
		con_printf(CON_NORMAL, "sdl-joystick: %d buttons (total)", Joystick.n_buttons);
	}
}

void joy_close()
{
	SDL_JoystickClose(SDL_Joysticks[num_joysticks].handle);

	joyaxis_text.clear();
	joybutton_text.clear();
}

const d_event_joystick_axis_value &event_joystick_get_axis(const d_event &event)
{
	auto &e = static_cast<const d_event_joystick_moved &>(event);
	Assert(e.type == EVENT_JOYSTICK_MOVED);
	return e;
}

void joy_flush()
{
	if (!num_joysticks)
		return;

	static_assert(SDL_RELEASED == uint8_t(), "SDL_RELEASED not 0.");
	Joystick.button_state = {};
}

int event_joystick_get_button(const d_event &event)
{
	auto &e = static_cast<const d_event_joystickbutton &>(event);
	Assert(e.type == EVENT_JOYSTICK_BUTTON_DOWN || e.type == EVENT_JOYSTICK_BUTTON_UP);
	return e.button;
}
