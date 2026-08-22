/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL2 GameController support
 *
 */

#include "dxxsconf.h"
#include "joy.h"

#if DXX_MAX_JOYSTICKS && SDL_MAJOR_VERSION == 2

#include <memory>
#include <vector>
#include <array>
#include "key.h"
#include "dxxerror.h"
#include "timer.h"
#include "console.h"
#include "event.h"
#include "u_mem.h"
#include "kconfig.h"
#include "physfsx.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "partial_range.h"
#include "physfsrwops.h"

/* Allow the build system to pick whether to search the SDL base directory. */
#ifndef DXX_ENABLE_GAMECONTROLLER_SEARCH_SDL_BASE_DIRECTORY
#ifdef __linux__
/* By default, disable on Linux, since it is normal for Linux
 * to install the executable in a directory separate from data files.
 */
#define DXX_ENABLE_GAMECONTROLLER_SEARCH_SDL_BASE_DIRECTORY	0
#else
/* By default, enable on other platforms, since it is common for Windows
 * systems to bundle everything in one directory.  Mac OS X may work the same,
 * and the extra search is harmless if it is not needed.
 */
#define DXX_ENABLE_GAMECONTROLLER_SEARCH_SDL_BASE_DIRECTORY	1
#endif
#endif

namespace dcx {

int num_controllers{};
joybutton_text_t gcbutton_text;

namespace {

struct SDL_GameController_deleter
{
	static void operator()(SDL_GameController *gc)
	{
		SDL_GameControllerClose(gc);
	}
};

struct d_gamecontroller
{
	std::unique_ptr<SDL_GameController, SDL_GameController_deleter> handle;
	SDL_JoystickID instance_id{-1};
};

static std::array<d_gamecontroller, DXX_MAX_JOYSTICKS> GameControllers;

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

/* SDL_GameController has a fixed layout:
 * 15 buttons: A, B, X, Y, Back, Guide, Start, LeftStick, RightStick,
 *             LeftShoulder, RightShoulder, DPadUp, DPadDown, DPadLeft, DPadRight
 * 6 axes:     LeftX, LeftY, RightX, RightY, TriggerLeft, TriggerRight
 *
 * We map these to a virtual joystick with:
 *   Buttons 0-14: the 15 GameController buttons
 *   Buttons 15-26: axis-as-button pairs (6 axes * 2 directions)
 *   Axes 0-5: the 6 GameController axes
 */

constexpr unsigned GC_NUM_BUTTONS = SDL_CONTROLLER_BUTTON_MAX;
constexpr unsigned GC_NUM_AXES = SDL_CONTROLLER_AXIS_MAX;
constexpr unsigned GC_NUM_VIRTUAL_BUTTONS = GC_NUM_BUTTONS + (2 * GC_NUM_AXES);
constexpr unsigned GC_AXIS_BUTTON_START = GC_NUM_BUTTONS;

static const char *gc_button_name(int button)
{
	switch (button)
	{
		case SDL_CONTROLLER_BUTTON_A: return "A";
		case SDL_CONTROLLER_BUTTON_B: return "B";
		case SDL_CONTROLLER_BUTTON_X: return "X";
		case SDL_CONTROLLER_BUTTON_Y: return "Y";
		case SDL_CONTROLLER_BUTTON_BACK: return "Back";
		case SDL_CONTROLLER_BUTTON_GUIDE: return "Guide";
		case SDL_CONTROLLER_BUTTON_START: return "Start";
		case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
		case SDL_CONTROLLER_BUTTON_DPAD_UP: return "Up";
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "Down";
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "Left";
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "Right";
		default: return "?";
	}
}

constexpr std::array<char, 3> gc_axis_name(const SDL_GameControllerAxis axis)
{
	switch (axis)
	{
		case SDL_CONTROLLER_AXIS_LEFTX:
			return {"LX"};
		case SDL_CONTROLLER_AXIS_LEFTY:
			return {"LY"};
		case SDL_CONTROLLER_AXIS_RIGHTX:
			return {"RX"};
		case SDL_CONTROLLER_AXIS_RIGHTY:
			return {"RY"};
		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			return {"LT"};
		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			return {"RT"};
		default:
			[[unlikely]];
			return {"?\0"};
	}
}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
constexpr auto gc_key_map{[]() {
	std::array<unsigned, 1 + (GC_AXIS_BUTTON_START + (SDL_CONTROLLER_AXIS_LEFTY * 2) + 1)> gc_key_map{};
	// Standard menu key mappings using GameController button names
	gc_key_map[SDL_CONTROLLER_BUTTON_A] = KEY_ENTER;
	gc_key_map[SDL_CONTROLLER_BUTTON_B] = KEY_ESC;
	gc_key_map[SDL_CONTROLLER_BUTTON_X] = KEY_SPACEBAR;
	gc_key_map[SDL_CONTROLLER_BUTTON_Y] = KEY_DELETE;
	gc_key_map[SDL_CONTROLLER_BUTTON_DPAD_UP] = KEY_UP;
	gc_key_map[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = KEY_DOWN;
	gc_key_map[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = KEY_LEFT;
	gc_key_map[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = KEY_RIGHT;
	gc_key_map[SDL_CONTROLLER_BUTTON_START] = KEY_PAUSE;
	gc_key_map[SDL_CONTROLLER_BUTTON_BACK] = KEY_ESC;
	gc_key_map[SDL_CONTROLLER_BUTTON_LEFTSTICK] = KEY_F4 + KEY_SHIFTED; // Guidebot menu (D2)
	// Left stick axis-buttons map to arrows for menu navigation
	gc_key_map[GC_AXIS_BUTTON_START + (SDL_CONTROLLER_AXIS_LEFTX * 2)] = KEY_RIGHT;      // +LX = right
	gc_key_map[GC_AXIS_BUTTON_START + (SDL_CONTROLLER_AXIS_LEFTX * 2) + 1] = KEY_LEFT;   // -LX = left
	gc_key_map[GC_AXIS_BUTTON_START + (SDL_CONTROLLER_AXIS_LEFTY * 2)] = KEY_DOWN;       // +LY = down
	gc_key_map[GC_AXIS_BUTTON_START + (SDL_CONTROLLER_AXIS_LEFTY * 2) + 1] = KEY_UP;     // -LY = up
	return gc_key_map;
}()};
#endif

static std::array<int, GC_NUM_AXES> gc_axis_values{};

static void gc_load_controller_db()
{
	// Try to load gamecontrollerdb.txt from PhysFS search path
	if (auto &&[rwops, physfserr]{PHYSFSRWOPS_openRead("gamecontrollerdb.txt")}; rwops)
	{
		if (const auto n{SDL_GameControllerAddMappingsFromRW(rwops.get(), 0)}; n >= 0)
		{
			con_printf(CON_NORMAL, "gamecontroller: loaded %d mappings from PhysFS gamecontrollerdb.txt", n);
			return;
		}
		con_printf(CON_NORMAL, "gamecontroller: PhysFS found gamecontrollerdb.txt, but no mappings could be loaded: %s", SDL_GetError());
	}
	else if (physfserr != PHYSFS_ERR_NOT_FOUND)
		con_printf(CON_NORMAL, "gamecontroller: failed to use PhysFS to open gamecontrollerdb.txt: %s", PHYSFS_getErrorByCode(physfserr));
#ifdef DXX_GAMECONTROLLER_DB_DIRECTORY
	/* Allow the build system to specify one additional search directory.  If
	 * set, `DXX_GAMECONTROLLER_DB_DIRECTORY` must be a string literal suitable
	 * for use with `SDL_RWFromFile`.  It must be a path in the platform native
	 * form and, if relative, is parsed relative to the current working
	 * directory of the game.
	 */
	{
		static constexpr char path[]{DXX_GAMECONTROLLER_DB_DIRECTORY};
		if (const auto n{SDL_GameControllerAddMappingsFromFile(path)}; n >= 0)
		{
			con_printf(CON_NORMAL, "gamecontroller: loaded %d mappings from %s", n, path);
			return;
		}
		con_printf(CON_NORMAL, "gamecontroller: found " DXX_GAMECONTROLLER_DB_DIRECTORY ", but no mappings could be loaded: %s", SDL_GetError());
	}
#endif
#if DXX_ENABLE_GAMECONTROLLER_SEARCH_SDL_BASE_DIRECTORY
	// Also try from the executable directory
	if (const auto base = SDL_GetBasePath())
	{
		std::array<char, PATH_MAX> path;
		snprintf(path.data(), path.size(), "%sgamecontrollerdb.txt", base);
		SDL_free(base);
		const auto n = SDL_GameControllerAddMappingsFromFile(path.data());
		if (n >= 0)
		{
			con_printf(CON_NORMAL, "gamecontroller: loaded %d mappings from %s", n, path.data());
			return;
		}
		con_printf(CON_NORMAL, "gamecontroller: SDL base directory contained gamecontrollerdb.txt, but no mappings could be loaded: %s", SDL_GetError());
	}
#endif
}

static void gc_open_controller(int device_index)
{
	// Check if already open (SDL2 may fire ADDED for already-connected devices)
	const auto instance_id = SDL_JoystickGetDeviceInstanceID(device_index);
	for (int i = 0; i < num_controllers; i++)
	{
		if (GameControllers[i].instance_id == instance_id)
			return;
	}

	if (num_controllers >= DXX_MAX_JOYSTICKS)
	{
		con_printf(CON_NORMAL, "gamecontroller: ignoring device %d, max %d controllers supported", device_index, DXX_MAX_JOYSTICKS);
		return;
	}

	auto &gc = GameControllers[num_controllers];
	gc.handle.reset(SDL_GameControllerOpen(device_index));
	if (!gc.handle)
	{
		con_printf(CON_NORMAL, "gamecontroller: failed to open device %d: %s", device_index, SDL_GetError());
		return;
	}

	auto *joy = SDL_GameControllerGetJoystick(gc.handle.get());
	gc.instance_id = SDL_JoystickInstanceID(joy);
	const char *name = SDL_GameControllerName(gc.handle.get());
	con_printf(CON_NORMAL, "gamecontroller %d: %s (instance %d)", num_controllers, name ? name : "Unknown", gc.instance_id);
	num_controllers++;
}

static void gc_close_controller(SDL_JoystickID instance_id)
{
	for (int i = 0; i < num_controllers; i++)
	{
		if (GameControllers[i].instance_id == instance_id)
		{
			con_printf(CON_NORMAL, "gamecontroller: removed controller %d (instance %d)", i, instance_id);
			GameControllers[i].handle.reset();
			GameControllers[i].instance_id = -1;
			// Shift remaining controllers down
			for (int j = i; j < num_controllers - 1; j++)
				GameControllers[j] = std::move(GameControllers[j + 1]);
			num_controllers--;
			return;
		}
	}
}

} // end anonymous namespace

#if DXX_MAX_AXES_PER_JOYSTICK
constexpr gamecontroller_axis_text_array gamecontroller_axis_text{
	[]() {
		gamecontroller_axis_text_array r;
		for (const auto i : {
			SDL_CONTROLLER_AXIS_LEFTX,
			SDL_CONTROLLER_AXIS_LEFTY,
			SDL_CONTROLLER_AXIS_RIGHTX,
			SDL_CONTROLLER_AXIS_RIGHTY,
			SDL_CONTROLLER_AXIS_TRIGGERLEFT,
			SDL_CONTROLLER_AXIS_TRIGGERRIGHT
			})
			r[i] = gc_axis_name(i);
		return r;
	}()
};
#endif

void gamecontroller_init()
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
		con_printf(CON_NORMAL, "gamecontroller: initialization failed: %s.", SDL_GetError());
		return;
	}

	gc_axis_values = {};
	gcbutton_text.clear();
	num_controllers = 0;

	// Load external controller database
	gc_load_controller_db();

	// Enable controller events
	SDL_GameControllerEventState(SDL_ENABLE);

	// Set up fixed button/axis text and key mappings
	gcbutton_text.resize(GC_NUM_VIRTUAL_BUTTONS);

	// Button text and key mappings
	for (unsigned i = 0; i < GC_NUM_BUTTONS; i++)
	{
		auto &text = gcbutton_text[i];
		snprintf(text.data(), text.size(), "%s", gc_button_name(i));
	}

	// Axis-as-button text and key mappings
	for (unsigned i = 0; i < GC_NUM_AXES; i++)
	{
		const auto base = GC_AXIS_BUTTON_START + (i * 2);
		auto &text_pos = gcbutton_text[base];
		auto &text_neg = gcbutton_text[base + 1];
		auto &&name{gc_axis_name(static_cast<SDL_GameControllerAxis>(i))};
		snprintf(text_pos.data(), text_pos.size(), "+%s", name.data());
		snprintf(text_neg.data(), text_neg.size(), "-%s", name.data());
	}

	const auto n_js = SDL_NumJoysticks();
	con_printf(CON_NORMAL, "gamecontroller: %d joystick(s) detected", n_js);
	for (int i = 0; i < n_js; i++)
	{
		if (SDL_IsGameController(i))
			gc_open_controller(i);
	}
}

void gamecontroller_close()
{
	for (auto &gc : GameControllers)
		gc.handle.reset();
	num_controllers = 0;
	gcbutton_text.clear();
}

void gamecontroller_flush()
{
	if (!num_controllers)
		return;
	gc_axis_values = {};
}

// --- GameController event handlers ---

window_event_result gc_button_handler(const SDL_ControllerButtonEvent *const cbe)
{
	const unsigned button = cbe->button;
	if (button >= GC_NUM_BUTTONS)
		return window_event_result::ignored;

	const d_event_joystickbutton event{
		(cbe->type == SDL_CONTROLLERBUTTONDOWN) ? event_type::joystick_button_down : event_type::joystick_button_up,
		button
	};
	con_printf(CON_DEBUG, "gamecontroller: button %s (%u) %s", gc_button_name(button), button,
		(cbe->type == SDL_CONTROLLERBUTTONDOWN) ? "down" : "up");
	return event_send(event);
}

namespace {

static window_event_result gc_send_axis_button_event(unsigned button, event_type e)
{
	const d_event_joystickbutton event{e, button};
	return event_send(event);
}

}

window_event_result gc_axisbutton_handler(const SDL_ControllerAxisEvent *const cae)
{
	const auto axis = cae->axis;
	if (axis >= GC_NUM_AXES)
		return window_event_result::ignored;

	const auto button = GC_AXIS_BUTTON_START + (axis * 2);
	const auto old_value = gc_axis_values[axis];
	const auto new_raw = cae->value / 256;  // Scale to -128..127

	const int deadzone = 38;  // ~30% deadzone
	auto prev_value = apply_deadzone(old_value, deadzone);
	auto new_value = apply_deadzone(new_raw, deadzone);

	window_event_result highest_result{window_event_result::ignored};

	if (prev_value <= 0 && new_value >= 0)
	{
		if (prev_value < 0)
			highest_result = std::max(gc_send_axis_button_event(button + 1, event_type::joystick_button_up), highest_result);
		if (new_value > 0)
			highest_result = std::max(gc_send_axis_button_event(button, event_type::joystick_button_down), highest_result);
	}
	else if (prev_value >= 0 && new_value <= 0)
	{
		if (prev_value > 0)
			highest_result = std::max(gc_send_axis_button_event(button, event_type::joystick_button_up), highest_result);
		if (new_value < 0)
			highest_result = std::max(gc_send_axis_button_event(button + 1, event_type::joystick_button_down), highest_result);
	}

	return highest_result;
}

window_event_result gc_axis_handler(const SDL_ControllerAxisEvent *const cae)
{
	const auto axis = cae->axis;
	if (axis >= GC_NUM_AXES)
		return window_event_result::ignored;

	const auto new_value = cae->value / 256;
	auto &old_value = gc_axis_values[axis];
	if (old_value == new_value)
		return window_event_result::ignored;

	d_event_joystick_moved event{event_type::joystick_moved};
	event.axis = axis;
	event.value = old_value = new_value;

	return event_send(event);
}

window_event_result gc_device_added(const SDL_ControllerDeviceEvent *const cde)
{
	con_printf(CON_NORMAL, "gamecontroller: device added (index %d)", cde->which);
	gc_open_controller(cde->which);
	return window_event_result::handled;
}

window_event_result gc_device_removed(const SDL_ControllerDeviceEvent *const cde)
{
	con_printf(CON_NORMAL, "gamecontroller: device removed (instance %d)", cde->which);
	gc_close_controller(cde->which);
	return window_event_result::handled;
}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
bool gamecontroller_translate_menu_key(const unsigned button)
{
	if (button >= gc_key_map.size())
		return false;
	auto key = gc_key_map[button];
	if (key)
	{
		event_keycommand_send(key);
		return true;
	}
	return false;
}
#endif

}

#endif // DXX_MAX_JOYSTICKS && SDL_MAJOR_VERSION == 2
