/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL joystick support
 *
 */

#include <memory>
#include <vector>
#include <tuple>
#include <type_traits>
#include "joy.h"
#include "key.h"
#include "dxxerror.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "u_mem.h"
#include "kconfig.h"
#include "compiler-range_for.h"

#if DXX_MAX_JOYSTICKS
#include "compiler-cf_assert.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "partial_range.h"
#include <utility>

namespace dcx {

namespace {

int num_joysticks = 0;

std::vector<unsigned> joy_key_map;

/* This struct is a "virtual" joystick, which includes all the axes
 * and buttons of every joystick found.
 */
static struct joyinfo {
#if DXX_MAX_BUTTONS_PER_JOYSTICK
	std::array<uint8_t, JOY_MAX_BUTTONS> button_state;
#endif
} Joystick;

struct d_event_joystickbutton : d_event
{
	const unsigned button;
	constexpr d_event_joystickbutton(const event_type t, const unsigned b) :
		d_event{t}, button{b}
	{
	}
};

struct d_event_joystick_moved : d_event, d_event_joystick_axis_value
{
	using d_event::d_event;
};

class SDL_Joystick_deleter
{
public:
	void operator()(SDL_Joystick *j) const
	{
		SDL_JoystickClose(j);
	}
};

#ifndef DXX_USE_SIZE_SORTED_TUPLE
#define DXX_USE_SIZE_SORTED_TUPLE	(__cplusplus > 201103L)
#endif

#if DXX_USE_SIZE_SORTED_TUPLE
template <typename T, std::size_t... Is, std::size_t... Js>
auto d_split_tuple(T &&t, std::index_sequence<Is...>, std::index_sequence<Js...>) ->
	std::pair<
		std::tuple<typename std::tuple_element<Is, T>::type...>,
		std::tuple<typename std::tuple_element<sizeof...(Is) + Js, T>::type...>
	>;

template <typename>
class d_size_sorted;

/* Given an input tuple T, define a public member `type` with the same
 * members as T, but sorted such that sizeof(Ti) <= sizeof(Tj) for all i
 * <= j.
 */
template <typename... Ts>
class d_size_sorted<std::tuple<Ts...>>
{
	using split_tuple = decltype(d_split_tuple(
		std::declval<std::tuple<Ts...>>(),
		std::make_index_sequence<sizeof...(Ts) / 2>(),
		std::make_index_sequence<(1 + sizeof...(Ts)) / 2>()
	));
	using first_type = typename split_tuple::first_type;
	using second_type = typename split_tuple::second_type;
public:
	using type = typename std::conditional<(sizeof(first_type) < sizeof(second_type)),
		decltype(std::tuple_cat(std::declval<first_type>(), std::declval<second_type>())),
		decltype(std::tuple_cat(std::declval<second_type>(), std::declval<first_type>()))
	>::type;
};

template <typename T1>
class d_size_sorted<std::tuple<T1>>
{
public:
	using type = std::tuple<T1>;
};
#endif

template <typename>
class ignore_empty {};

template <typename T>
using maybe_empty_array = typename std::conditional<T().empty(), ignore_empty<T>, T>::type;

/* This struct is an array, with one entry for each physical joystick
 * found.
 */
class d_physical_joystick
{
#define for_each_tuple_item(HANDLE_VERB,VERB)	\
	HANDLE_VERB(handle)	\
	VERB(hat_map)	\
	VERB(button_map)	\
	VERB(axis_map)	\
	VERB(axis_value)	\
	VERB(axis_button_map)	\

#if DXX_USE_SIZE_SORTED_TUPLE
	template <typename... Ts>
		using tuple_type = typename d_size_sorted<std::tuple<Ts...>>::type;
#define define_handle_getter(N)	\
	define_getter(N, tuple_member_type_##N)
#define define_array_getter(N)	\
	define_getter(N, maybe_empty_array<tuple_member_type_##N>)
#else
	template <typename... Ts>
		using tuple_type = std::tuple<Ts...>;
	enum
	{
#define define_enum(V)	tuple_item_##V,
		for_each_tuple_item(define_enum, define_enum)
#undef define_enum
	};
	/* std::get<index> does not handle the maybe_empty_array case, so
	 * reuse the same getter for both handle and array.
	 */
#define define_handle_getter(N)	\
	define_getter(N, tuple_item_##N)
#define define_array_getter	define_handle_getter
#endif
#define define_getter(N,V)	\
	auto &N()	\
	{	\
		return std::get<V>(t);	\
	}
	using tuple_member_type_handle = std::unique_ptr<SDL_Joystick, SDL_Joystick_deleter>;
	//Note: Descent expects hats to be buttons, so these are indices into Joystick.buttons
	struct tuple_member_type_hat_map : std::array<unsigned, DXX_MAX_HATS_PER_JOYSTICK> {};
	struct tuple_member_type_button_map : std::array<unsigned, DXX_MAX_BUTTONS_PER_JOYSTICK> {};
	struct tuple_member_type_axis_map : std::array<unsigned, DXX_MAX_AXES_PER_JOYSTICK> {};
	struct tuple_member_type_axis_value : std::array<int, DXX_MAX_AXES_PER_JOYSTICK> {};
	struct tuple_member_type_axis_button_map : std::array<unsigned, DXX_MAX_AXES_PER_JOYSTICK> {};
	tuple_type<
		tuple_member_type_handle,
		maybe_empty_array<tuple_member_type_hat_map>,
		maybe_empty_array<tuple_member_type_button_map>,
		maybe_empty_array<tuple_member_type_axis_map>,
		maybe_empty_array<tuple_member_type_axis_value>,
		maybe_empty_array<tuple_member_type_axis_button_map>
	> t;
public:
	for_each_tuple_item(define_handle_getter, define_array_getter);
#undef define_handle_getter
#undef define_array_getter
#undef define_getter
#undef for_each_tuple_item
};

static std::array<d_physical_joystick, DXX_MAX_JOYSTICKS> SDL_Joysticks;

}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
window_event_result joy_button_handler(const SDL_JoyButtonEvent *const jbe)
{
	const unsigned button = SDL_Joysticks[jbe->which].button_map()[jbe->button];

	Joystick.button_state[button] = jbe->state;

	const d_event_joystickbutton event{
		(jbe->type == SDL_JOYBUTTONDOWN) ? event_type::joystick_button_down : event_type::joystick_button_up,
		button
	};
	con_printf(CON_DEBUG, "Sending event %s, button %d", (jbe->type == SDL_JOYBUTTONDOWN) ? "event_type::joystick_button_down" : "EVENT_JOYSTICK_JOYSTICK_UP", event.button);
	return event_send(event);
}
#endif

#if DXX_MAX_HATS_PER_JOYSTICK
window_event_result joy_hat_handler(const SDL_JoyHatEvent *const jhe)
{
	int hat = SDL_Joysticks[jhe->which].hat_map()[jhe->hat];
	window_event_result highest_result(window_event_result::ignored);
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
	range_for (const unsigned i, xrange(4u))
	{
		const auto current_button_state = !!(jhe_value & (1 << i));
		auto &saved_button_state = Joystick.button_state[hat + i];
		if (saved_button_state == current_button_state)
			// Same state as before
			continue;
		saved_button_state = current_button_state;
		const d_event_joystickbutton event{current_button_state ? event_type::joystick_button_down : event_type::joystick_button_up, hat + i};
		if (current_button_state) //last_state up, current state down
		{
			con_printf(CON_DEBUG, "Sending event event_type::joystick_button_down, button %d", event.button);
		}
		else	//last_state down, current state up
		{
			con_printf(CON_DEBUG, "Sending event event_type::joystick_button_up, button %d", event.button);
		}
		highest_result = std::max(event_send(event), highest_result);
	}

	return highest_result;
}
#endif

#if DXX_MAX_AXES_PER_JOYSTICK
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
namespace {

static window_event_result send_axis_button_event(unsigned button, event_type e)
{
	Joystick.button_state[button] = (e == event_type::joystick_button_up) ? 0 : 1;
	const d_event_joystickbutton event{ e, button };
	con_printf(CON_DEBUG, "Sending event %s, button %d", (e == event_type::joystick_button_up) ? "event_type::joystick_button_up" : "event_type::joystick_button_down", event.button);
	return event_send(event);
}

}

window_event_result joy_axisbutton_handler(const SDL_JoyAxisEvent *const jae)
{
	auto &js = SDL_Joysticks[jae->which];
	auto axis_value = js.axis_value()[jae->axis];
	auto button = js.axis_button_map()[jae->axis];
	window_event_result highest_result(window_event_result::ignored);

	// We have to hardcode a deadzone here. It's not mapped into the settings.
	// We could add another deadzone slider called "axis button deadzone".
	// I think it's safe to assume a 30% deadzone on analog button presses for now.
	const decltype(axis_value) deadzone = 38;
	auto prev_value = apply_deadzone(axis_value, deadzone);
	auto new_value = apply_deadzone(jae->value/256, deadzone);

	if (prev_value <= 0 && new_value >= 0) // positive pressed
	{
		if (prev_value < 0) // Do previous direction release first if the case
			highest_result = std::max(send_axis_button_event(button + 1, event_type::joystick_button_up), highest_result);
		if (new_value > 0)
			highest_result = std::max(send_axis_button_event(button, event_type::joystick_button_down), highest_result);
	}
	else if (prev_value >= 0 && new_value <= 0) // negative pressed
	{
		if (prev_value > 0) // Do previous direction release first if the case
			highest_result = std::max(send_axis_button_event(button, event_type::joystick_button_up), highest_result);
		if (new_value < 0)
			highest_result = std::max(send_axis_button_event(button + 1, event_type::joystick_button_down), highest_result);
	}

	return highest_result;
}
#endif

window_event_result joy_axis_handler(const SDL_JoyAxisEvent *const jae)
{
	auto &js = SDL_Joysticks[jae->which];
	const auto axis = js.axis_map()[jae->axis];
	auto &axis_value = js.axis_value()[jae->axis];
	// inaccurate stick is inaccurate. SDL might send SDL_JoyAxisEvent even if the value is the same as before.
	if (axis_value == jae->value/256)
		return window_event_result::ignored;

	d_event_joystick_moved event{event_type::joystick_moved};
	event.value = axis_value = jae->value/256;
	event.axis = axis;
	con_printf(CON_DEBUG, "Sending event event_type::joystick_moved, axis: %d, value: %d",event.axis, event.value);

	return event_send(event);
}
#endif


/* ----------------------------------------------- */

namespace {

static unsigned check_warn_joy_support_limit(const unsigned n, const char *const desc, const unsigned MAX)
{
	if (n <= MAX)
	{
		con_printf(CON_NORMAL, "sdl-joystick: %d %ss", n, desc);
		return n;
	}
	Warning("sdl-joystick: found %d %ss, only %d supported.", n, desc, MAX);
	return MAX;
}

}

void joy_init()
{
	if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		con_printf(CON_NORMAL, "sdl-joystick: initialisation failed: %s.",SDL_GetError());
		return;
	}

	Joystick = {};
#if DXX_MAX_AXES_PER_JOYSTICK
	joyaxis_text.clear();
#endif
	joybutton_text.clear();
	joy_key_map.clear();

	const auto n = check_warn_joy_support_limit(SDL_NumJoysticks(), "joystick", DXX_MAX_JOYSTICKS);
	cf_assert(n <= DXX_MAX_JOYSTICKS);
	unsigned joystick_n_buttons = 0, joystick_n_axes = 0;
	range_for (const unsigned i, xrange(n))
	{
		auto &joystick = SDL_Joysticks[num_joysticks];
		const auto handle = SDL_JoystickOpen(i);
		joystick.handle().reset(handle);
#if SDL_MAJOR_VERSION == 1
		con_printf(CON_NORMAL, "sdl-joystick %d: %s", i, SDL_JoystickName(i));
#else
		con_printf(CON_NORMAL, "sdl-joystick %d: %s", i, SDL_JoystickName(handle));
#endif
		if (handle)
		{
#if DXX_MAX_AXES_PER_JOYSTICK
			const auto n_axes = check_warn_joy_support_limit(SDL_JoystickNumAxes(handle), "axe", DXX_MAX_AXES_PER_JOYSTICK);

			joyaxis_text.resize(joyaxis_text.size() + n_axes);
			for (auto &&[idx, value] : enumerate(partial_range(joystick.axis_map(), n_axes), 1))
			{
				cf_assert(idx <= DXX_MAX_AXES_PER_JOYSTICK);
				auto &text = joyaxis_text[joystick_n_axes];
				value = joystick_n_axes++;
				snprintf(&text[0], sizeof(text), "J%d A%u", i + 1, idx);
			}
#else
            const auto n_axes = 0;
#endif

			const auto n_buttons = check_warn_joy_support_limit(SDL_JoystickNumButtons(handle), "button", DXX_MAX_BUTTONS_PER_JOYSTICK);
			const auto n_hats = check_warn_joy_support_limit(SDL_JoystickNumHats(handle), "hat", DXX_MAX_HATS_PER_JOYSTICK);

			const auto n_virtual_buttons = n_buttons + (4 * n_hats) + (2 * n_axes);
			joybutton_text.resize(joybutton_text.size() + n_virtual_buttons);
			joy_key_map.resize(joy_key_map.size() + n_virtual_buttons, 0);
#if DXX_MAX_BUTTONS_PER_JOYSTICK
			for (auto &&[idx, value] : enumerate(partial_range(joystick.button_map(), n_buttons), 1))
			{
				cf_assert(idx <= DXX_MAX_BUTTONS_PER_JOYSTICK);
				switch (idx)
				{
					case 1: joy_key_map[joystick_n_buttons] = KEY_ENTER; break;
					case 2: joy_key_map[joystick_n_buttons] = KEY_ESC; break;
					case 3: joy_key_map[joystick_n_buttons] = KEY_SPACEBAR; break;
					case 4: joy_key_map[joystick_n_buttons] = KEY_DELETE; break;
					default: break;
				}
				auto &text = joybutton_text[joystick_n_buttons];
				value = joystick_n_buttons++;
				snprintf(&text[0], sizeof(text), "J%u B%u", i + 1, idx);
			}
#endif
#if DXX_MAX_HATS_PER_JOYSTICK
			for (auto &&[idx, value] : enumerate(partial_range(joystick.hat_map(), n_hats), 1))
			{
				cf_assert(idx <= DXX_MAX_HATS_PER_JOYSTICK);
				value = joystick_n_buttons;
				//a hat counts as four buttons
				joy_key_map[joystick_n_buttons] = KEY_UP;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u H%u%c", i + 1, idx, 0202);
				joy_key_map[joystick_n_buttons] = KEY_RIGHT;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u H%u%c", i + 1, idx, 0177);
				joy_key_map[joystick_n_buttons] = KEY_DOWN;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u H%u%c", i + 1, idx, 0200);
				joy_key_map[joystick_n_buttons] = KEY_LEFT;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u H%u%c", i + 1, idx, 0201);
			}
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
			for (auto &&[idx, value] : enumerate(partial_range(joystick.axis_button_map(), n_axes), 1))
			{
				cf_assert(idx <= DXX_MAX_AXES_PER_JOYSTICK);
				value = joystick_n_buttons;
				//an axis count as 2 buttons. negative - and positive +
				joy_key_map[joystick_n_buttons] = (idx == 1) ? KEY_RIGHT : (idx == 2) ? KEY_DOWN : 0;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u -A%u", i + 1, idx);
				joy_key_map[joystick_n_buttons] = (idx == 1) ? KEY_LEFT : (idx == 2) ? KEY_UP : 0;
				snprintf(&joybutton_text[joystick_n_buttons++][0], sizeof(joybutton_text[0]), "J%u +A%u", i + 1, idx);
			}
#endif

			num_joysticks++;
		}
		else
			con_puts(CON_NORMAL, "sdl-joystick: initialization failed!");

		con_printf(CON_NORMAL, "sdl-joystick: %d axes (total)", joystick_n_axes);
		con_printf(CON_NORMAL, "sdl-joystick: %d buttons (total)", joystick_n_buttons);
	}
}

void joy_close()
{
	range_for (auto &j, SDL_Joysticks)
		j.handle().reset();
#if DXX_MAX_AXES_PER_JOYSTICK
	joyaxis_text.clear();
#endif
	joybutton_text.clear();
}

#if DXX_MAX_AXES_PER_JOYSTICK
const d_event_joystick_axis_value &event_joystick_get_axis(const d_event &event)
{
	auto &e = static_cast<const d_event_joystick_moved &>(event);
	assert(e.type == event_type::joystick_moved);
	return e;
}
#endif

void joy_flush()
{
	if (!num_joysticks)
		return;

	static_assert(SDL_RELEASED == uint8_t(), "SDL_RELEASED not 0.");
#if DXX_MAX_BUTTONS_PER_JOYSTICK
	Joystick.button_state = {};
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
	range_for (auto &j, SDL_Joysticks)
		j.axis_value() = {};
#endif
}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
int event_joystick_get_button(const d_event &event)
{
	auto &e = static_cast<const d_event_joystickbutton &>(event);
	assert(e.type == event_type::joystick_button_down || e.type == event_type::joystick_button_up);
	return e.button;
}
#endif

int apply_deadzone(int value, int deadzone)
{
	if (value > deadzone)
		return ((value - deadzone) * 128) / (128 - deadzone);
	else if (value < -deadzone)
		return ((value + deadzone) * 128) / (128 - deadzone);
	else
		return 0;
}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
bool joy_translate_menu_key(const d_event &event) {
	if (event.type != event_type::joystick_button_down)
		return false;
	auto &e = static_cast<const d_event_joystickbutton &>(event);
	assert(e.button < joy_key_map.size());
	auto key = joy_key_map[e.button];
	if (key)
	{
		event_keycommand_send(key);
		return true;
	}
	return false;
}
#endif

}
#endif
