/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Force feedback through SDL2 haptics / rumble.
 *
 * The game side keeps the calls of the original Descent II tactile.c
 * and the I-FORCE 1.0 API it used (Jolt, ButtonReflexJolt, XVibration,
 * Buffeting, Enable/Disable/ClearForces).  Here they are mapped to:
 *   - a force-feedback joystick or wheel (SDL_HAPTIC_CONSTANT): jolts are
 *     timed constant forces, vibration and buffeting share one sine
 *     effect.  A 1-axis device (wheel) drops the forward/back part of a
 *     jolt and plays it as a short shake instead.  The driver's
 *     autocenter is turned off; an optional centering spring (player
 *     setting, off by default like the original) replaces it.
 *   - a gamepad: jolts and vibration as rumble.  Buffeting is not played
 *     on rumble.
 *
 * Units follow the I-FORCE API: magnitudes 0-100, +x = right,
 * +y = forward, directions in degrees naming the side the force comes
 * from (0 = front).
 *
 */

#include "dxxsconf.h"
#include "ffb.h"

#if DXX_USE_FFB

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include "vecmat.h"
#include "console.h"

namespace dcx {

namespace tactile {

namespace {

constexpr unsigned buffet_hz{15};
constexpr unsigned shake_hz{25};
constexpr unsigned max_duration_ms{2500};
/* Tactile_Xvibrate is called while scraping a volatile wall; stop the
 * vibration when the calls stop.
 */
constexpr uint32_t xvibrate_hold_ms{150};
constexpr uint32_t xvibrate_timeout_ms{300};
/* SDL_JoystickRumble needs a duration; sustained rumble is refreshed
 * before it runs out.
 */
constexpr uint32_t rumble_length_ms{1000};
constexpr uint32_t rumble_refresh_ms{500};

/* Fire jolt per primary weapon, from the original weapon.c. */
constexpr unsigned tactile_fire_duration[]{120, 80, 150, 250, 150, 200, 100, 180, 280, 100};
constexpr unsigned tactile_fire_repeat[]{260, 90, 160, 160, 160, 210, 110, 191, 291, 111};
constexpr unsigned tactile_fire_magnitude{50};

enum class device_mode : uint8_t
{
	none,
	force,
	rumble,
};

struct reflex_state
{
	bool active;
	bool prev_pressed;
	int x, y;
	unsigned duration, repeat;
	uint32_t next;
};

struct ffb_state
{
	SDL_Joystick *joystick;
	SDL_Haptic *haptic;
	SDL_JoystickID instance_id{-1};
	device_mode mode;
	bool open_attempted;
	unsigned features;
	int axes;

	int pulse_id{-1}, shake_id{-1}, periodic_id{-1}, spring_id{-1};

	bool config_enabled;
	float gain{1.0f};
	bool forces_enabled{true};	// EnableForces / DisableForces

	unsigned xvib_mag, xvib_freq;
	uint32_t xvib_time;
	unsigned buffet;
	float applied_periodic{-1.0f};
	float centering;		// 0-1, centering spring setting
	float applied_centering{-1.0f};

	float rumble_sustained;
	float rumble_jolt;
	uint32_t rumble_jolt_until;
	float applied_rumble{-1.0f};
	uint32_t rumble_refresh_at;

	reflex_state reflex;
};

ffb_state S;

bool active()
{
	return S.mode != device_mode::none && S.config_enabled && S.forces_enabled;
}

int16_t to_level(const float unit)
{
	return static_cast<int16_t>(std::clamp(unit, 0.0f, 1.0f) * 32767.0f);
}

bool tick_reached(const uint32_t now, const uint32_t when)
{
	return static_cast<int32_t>(now - when) >= 0;
}

/* Same result as CalcTrig() in the I-FORCE DLL: the force pushes away
 * from direction.
 */
void dir_to_xy(unsigned magnitude, const int direction, int &x, int &y)
{
	const double rad = direction * 3.14159265358979 / 180.0;
	magnitude = std::min(magnitude, 100u);
	x = static_cast<int>(-static_cast<double>(magnitude) * std::sin(rad));
	y = static_cast<int>(-static_cast<double>(magnitude) * std::cos(rad));
}

/* --- Force-feedback effects (ported from sonik-br's DOSBox Staging joystick_ffb.cpp) --- */

void destroy_effect(int &id)
{
	if (id >= 0 && S.haptic)
		SDL_HapticDestroyEffect(S.haptic, id);
	id = -1;
}

void run_effect(int &id, SDL_HapticEffect &effect, const bool retrigger)
{
	if (id < 0)
	{
		id = SDL_HapticNewEffect(S.haptic, &effect);
		if (id < 0)
		{
			con_printf(CON_VERBOSE, "tactile: SDL_HapticNewEffect failed: %s", SDL_GetError());
			return;
		}
		if (SDL_HapticRunEffect(S.haptic, id, 1) != 0)
			con_printf(CON_VERBOSE, "tactile: SDL_HapticRunEffect failed: %s", SDL_GetError());
	}
	else
	{
		if (SDL_HapticUpdateEffect(S.haptic, id, &effect) != 0)
			con_printf(CON_VERBOSE, "tactile: SDL_HapticUpdateEffect failed: %s", SDL_GetError());
		if (retrigger)
			SDL_HapticRunEffect(S.haptic, id, 1);
	}
}

/* x = +right, y = +forward, -1..1.  SDL cartesian directions name the
 * side the force comes from (+x = right, +y = toward the user), so a
 * push to the right comes from -x and a push forward from +y.
 */
void set_direction(SDL_HapticDirection &d, const float x, const float y)
{
	d.type = SDL_HAPTIC_CARTESIAN;
	d.dir[0] = static_cast<int32_t>(-x * 10000.0f);
	d.dir[1] = static_cast<int32_t>(y * 10000.0f);
	if (!d.dir[0] && !d.dir[1])
		d.dir[1] = 1;
}

void play_constant_pulse(const float x, const float y, const uint32_t duration_ms)
{
	if (!(S.features & SDL_HAPTIC_CONSTANT))
		return;
	SDL_HapticEffect e{};
	e.type = SDL_HAPTIC_CONSTANT;
	set_direction(e.constant.direction, x, y);
	e.constant.length = duration_ms;
	e.constant.level = to_level(std::min(std::hypot(x, y), 1.0f) * S.gain);
	run_effect(S.pulse_id, e, true);
}

void play_shake(const float strength, const uint32_t duration_ms)
{
	if (!(S.features & SDL_HAPTIC_SINE))
		return;
	SDL_HapticEffect e{};
	e.type = SDL_HAPTIC_SINE;
	set_direction(e.periodic.direction, 1.0f, 0.0f);
	e.periodic.length = duration_ms;
	e.periodic.period = 1000 / shake_hz;
	e.periodic.magnitude = to_level(strength * S.gain);
	run_effect(S.shake_id, e, true);
}

void set_periodic(const float strength, const unsigned frequency_hz)
{
	if (!(S.features & SDL_HAPTIC_SINE))
		return;
	if (strength <= 0.0f)
	{
		destroy_effect(S.periodic_id);
		return;
	}
	SDL_HapticEffect e{};
	e.type = SDL_HAPTIC_SINE;
	set_direction(e.periodic.direction, 1.0f, 0.0f);
	e.periodic.length = SDL_HAPTIC_INFINITY;
	e.periodic.period = static_cast<uint16_t>(frequency_hz ? std::clamp(1000u / frequency_hz, 1u, 1000u) : 1000u / buffet_hz);
	e.periodic.magnitude = to_level(strength * S.gain);
	run_effect(S.periodic_id, e, false);
}

/* Centering spring on every force axis (wheel or stick).  Its strength
 * is its own setting, not scaled by the effect strength.
 */
void set_centering(const bool on)
{
	if (!(S.features & SDL_HAPTIC_SPRING))
		return;
	const float level{on ? S.centering : 0.0f};
	if (level == S.applied_centering)
		return;
	S.applied_centering = level;
	if (level <= 0.0f)
	{
		destroy_effect(S.spring_id);
		return;
	}
	SDL_HapticEffect e{};
	e.type = SDL_HAPTIC_SPRING;
	e.condition.direction.type = SDL_HAPTIC_CARTESIAN;
	e.condition.direction.dir[0] = 1;
	e.condition.length = SDL_HAPTIC_INFINITY;
	for (unsigned axis = 0; axis < 3; ++axis)
	{
		e.condition.left_coeff[axis] = e.condition.right_coeff[axis] = to_level(level);
		e.condition.left_sat[axis] = e.condition.right_sat[axis] = 0xffff;
	}
	run_effect(S.spring_id, e, false);
}

/* --- Rumble --- */

void apply_rumble(const uint32_t now)
{
	float level = S.rumble_sustained;
	/* A jolt stronger than the sustained level plays for its own
	 * duration, so it also ends when no frames are running (menus).
	 */
	uint32_t length = rumble_length_ms;
	if (S.rumble_jolt > 0.0f)
	{
		if (tick_reached(now, S.rumble_jolt_until))
			S.rumble_jolt = 0.0f;
		else if (S.rumble_jolt > level)
		{
			level = S.rumble_jolt;
			length = S.rumble_jolt_until - now;
		}
	}
	level = std::clamp(level * S.gain, 0.0f, 1.0f);
	if (level == S.applied_rumble && (level == 0.0f || !tick_reached(now, S.rumble_refresh_at)))
		return;
	S.applied_rumble = level;
	S.rumble_refresh_at = now + std::min(length, rumble_refresh_ms);
	if (S.joystick && !S.haptic)
	{
		const auto motor = static_cast<uint16_t>(level * 0xffff);
		SDL_JoystickRumble(S.joystick, motor, motor, level > 0.0f ? length : 0);
	}
	else if (S.haptic)
	{
		if (level > 0.0f)
			SDL_HapticRumblePlay(S.haptic, level, length);
		else
			SDL_HapticRumbleStop(S.haptic);
	}
}

/* --- Continuous effects, re-applied on any change --- */

void apply_continuous()
{
	const uint32_t now = SDL_GetTicks();
	const bool on = active();
	if (S.mode == device_mode::force)
	{
		/* Vibration and buffeting share one sine effect. */
		const unsigned level = on ? std::max(S.xvib_mag, S.buffet) : 0;
		const float strength = level / 100.0f;
		if (strength != S.applied_periodic)
		{
			S.applied_periodic = strength;
			set_periodic(strength, S.xvib_mag ? S.xvib_freq : buffet_hz);
		}
		/* Stands in for the driver autocenter, so it stays on while
		 * paused and in menus (not tied to Enable/DisableForces).
		 */
		set_centering(S.config_enabled);
	}
	else if (S.mode == device_mode::rumble)
	{
		S.rumble_sustained = on ? S.xvib_mag / 100.0f : 0.0f;
		if (!on)
			S.rumble_jolt = 0.0f;
		apply_rumble(now);
	}
}

void stop_transients()
{
	if (S.mode == device_mode::force)
	{
		if (S.pulse_id >= 0)
			SDL_HapticStopEffect(S.haptic, S.pulse_id);
		if (S.shake_id >= 0)
			SDL_HapticStopEffect(S.haptic, S.shake_id);
	}
	S.rumble_jolt = 0.0f;
}

/* --- Device selection --- */

bool try_open_force(const int index)
{
	const auto j = SDL_JoystickOpen(index);
	if (!j)
		return false;
	if (SDL_JoystickIsHaptic(j) == SDL_TRUE)
	{
		if (const auto h = SDL_HapticOpenFromJoystick(j))
		{
			const unsigned caps = SDL_HapticQuery(h);
			if (caps & SDL_HAPTIC_CONSTANT)
			{
				S.joystick = j;
				S.haptic = h;
				S.features = caps;
				S.axes = SDL_HapticNumAxes(h);
				S.mode = device_mode::force;
				if (caps & SDL_HAPTIC_AUTOCENTER)
					SDL_HapticSetAutocenter(h, 0);
				if (caps & SDL_HAPTIC_GAIN)
					SDL_HapticSetGain(h, 100);
				con_printf(CON_NORMAL, "tactile: force feedback on \"%s\" (%d axes%s%s)", SDL_JoystickName(j), S.axes,
					(caps & SDL_HAPTIC_SINE) ? ", sine" : "", (caps & SDL_HAPTIC_SPRING) ? ", spring" : "");
				return true;
			}
			SDL_HapticClose(h);
		}
	}
	SDL_JoystickClose(j);
	return false;
}

bool try_open_rumble(const int index)
{
	const auto j = SDL_JoystickOpen(index);
	if (!j)
		return false;
	if (SDL_JoystickRumble(j, 0, 0, 0) == 0)
	{
		S.joystick = j;
		S.mode = device_mode::rumble;
		con_printf(CON_NORMAL, "tactile: rumble on \"%s\"", SDL_JoystickName(j));
		return true;
	}
	if (SDL_JoystickIsHaptic(j) == SDL_TRUE)
	{
		if (const auto h = SDL_HapticOpenFromJoystick(j))
		{
			if (SDL_HapticRumbleSupported(h) == SDL_TRUE && SDL_HapticRumbleInit(h) == 0)
			{
				S.joystick = j;
				S.haptic = h;
				S.mode = device_mode::rumble;
				con_printf(CON_NORMAL, "tactile: haptic rumble on \"%s\"", SDL_JoystickName(j));
				return true;
			}
			SDL_HapticClose(h);
		}
	}
	SDL_JoystickClose(j);
	return false;
}

/* Rebirth has already opened the joysticks and game controllers;
 * SDL_JoystickOpen hands back the same device with one more reference.
 */
void open_device()
{
	if (S.open_attempted)
		return;
	S.open_attempted = true;
	if (!SDL_WasInit(SDL_INIT_HAPTIC) && SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0)
		con_printf(CON_NORMAL, "tactile: haptic initialisation failed: %s", SDL_GetError());
	const auto try_all = [n = SDL_NumJoysticks()](bool (*const try_open)(int)) {
		for (int i = 0; i < n; ++i)
			if (try_open(i))
				return true;
		return false;
	};
	if (!try_all(try_open_force) && !try_all(try_open_rumble))
	{
		con_puts(CON_NORMAL, "tactile: no force feedback or rumble device found");
		return;
	}
	S.instance_id = SDL_JoystickInstanceID(S.joystick);
	S.applied_periodic = -1.0f;
	S.applied_rumble = -1.0f;
	S.applied_centering = -1.0f;
	apply_continuous();
}

void close_device()
{
	if (S.haptic)
	{
		destroy_effect(S.pulse_id);
		destroy_effect(S.shake_id);
		destroy_effect(S.periodic_id);
		destroy_effect(S.spring_id);
		if (S.mode == device_mode::rumble)
			SDL_HapticRumbleStop(S.haptic);
		SDL_HapticClose(S.haptic);
	}
	else if (S.joystick)
		SDL_JoystickRumble(S.joystick, 0, 0, 0);
	if (S.joystick)
		SDL_JoystickClose(S.joystick);
	S.haptic = nullptr;
	S.joystick = nullptr;
	S.instance_id = -1;
	S.mode = device_mode::none;
	S.features = 0;
	S.axes = 0;
	S.open_attempted = false;
}

void play_jolt(const int x, const int y, const unsigned duration_ms)
{
	if (!active())
		return;
	const float fx = std::clamp(x, -100, 100) / 100.0f;
	const float fy = std::clamp(y, -100, 100) / 100.0f;
	if (S.mode == device_mode::force)
	{
		if (S.axes >= 2)
			play_constant_pulse(fx, fy, duration_ms);
		else
		{
			/* Wheel: no forward/back axis, so that part becomes a shake. */
			if (x)
				play_constant_pulse(fx, 0.0f, duration_ms);
			if (y)
				play_shake(std::fabs(fy), duration_ms);
		}
	}
	else
	{
		/* Rumble has no direction; strength is the length of the jolt. */
		const uint32_t now = SDL_GetTicks();
		S.rumble_jolt = std::min(std::hypot(fx, fy), 1.0f);
		S.rumble_jolt_until = now + std::max(duration_ms, 1u);
		S.applied_rumble = -1.0f;	// restart even at the same level
		apply_rumble(now);
	}
}

void poll_reflex(const bool pressed, const uint32_t now)
{
	auto &r = S.reflex;
	if (!r.active)
	{
		r.prev_pressed = false;
		return;
	}
	if (pressed && (!r.prev_pressed || tick_reached(now, r.next)))
	{
		play_jolt(r.x, r.y, r.duration);
		r.next = now + r.repeat;
	}
	r.prev_pressed = pressed;
}

}

void frame(const bool enabled, const unsigned strength_percent, const unsigned centering_percent, const bool fire_held)
{
	const float gain = std::min(strength_percent, 100u) / 100.0f;
	const float centering = std::min(centering_percent, 100u) / 100.0f;
	const bool config_changed = enabled != S.config_enabled || gain != S.gain || centering != S.centering;
	S.config_enabled = enabled;
	S.gain = gain;
	S.centering = centering;
	if (!enabled)
	{
		if (config_changed && S.mode != device_mode::none)
		{
			stop_transients();
			apply_continuous();
		}
		return;
	}
	open_device();
	if (S.mode == device_mode::none)
		return;
	const uint32_t now = SDL_GetTicks();
	if (S.xvib_mag && tick_reached(now, S.xvib_time + xvibrate_timeout_ms))
	{
		S.xvib_mag = 0;
		apply_continuous();
	}
	else if (config_changed)
	{
		S.applied_periodic = -1.0f;
		apply_continuous();
	}
	poll_reflex(fire_held && S.forces_enabled, now);
	if (S.mode == device_mode::rumble)
		apply_rumble(now);
}

void close()
{
	close_device();
}

void device_added()
{
	if (S.mode == device_mode::none)
		S.open_attempted = false;
}

void device_removed(const SDL_JoystickID instance_id)
{
	if (S.mode != device_mode::none && instance_id == S.instance_id)
	{
		close_device();
		/* Another device may still be usable. */
		S.open_attempted = false;
	}
}

void test_jolt()
{
	open_device();
	const bool saved = S.forces_enabled;
	S.forces_enabled = true;
	const bool saved_enabled = S.config_enabled;
	S.config_enabled = true;
	play_jolt(60, 0, 200);
	S.config_enabled = saved_enabled;
	S.forces_enabled = saved;
}

/* --- tactile.c --- */

#define MAX_FORCE (i2f(10))

void Tactile_apply_force(const vms_vector &force_vec, const vms_matrix &orient)
{
	/* As in the original: strength from the force's length, direction
	 * from its heading in ship coordinates, a jolt of 7 ms per percent.
	 */
	const auto feedvec = vm_vec_build_rotated(force_vec, orient);
	const auto feedang = vm_extract_angles_vector(feedvec);
	const fix feedmag{static_cast<fix>(vm_vec_mag_quick(force_vec))};
	const int feedforce = std::clamp(f2i(fixmuldiv(feedmag, i2f(100), MAX_FORCE)), 0, 100);
	int realangle = f2i(fixmul(static_cast<uint16_t>(feedang.h), i2f(360))) - 180;
	if (realangle < 0)
		realangle += 360;
	Jolt(feedforce, realangle, feedforce * 7);
}

void Tactile_do_collide(const vms_vector &force_vec, const vms_matrix &orient)
{
	/* Empty in the original release.  Here a wall hit is a jolt against
	 * the direction of travel (force_vec is the negated velocity),
	 * full strength at 60 units/s.
	 */
	const auto feedvec = vm_vec_build_rotated(force_vec, orient);
	const auto feedang = vm_extract_angles_vector(feedvec);
	const fix feedmag{static_cast<fix>(vm_vec_mag_quick(force_vec))};
	const int feedforce = std::clamp(f2i(fixmuldiv(feedmag, i2f(100), i2f(60))), 0, 100);
	if (feedforce < 5)
		return;
	int realangle = f2i(fixmul(static_cast<uint16_t>(feedang.h), i2f(360))) - 180;
	if (realangle < 0)
		realangle += 360;
	Jolt(feedforce, realangle, 60 + feedforce * 2);
}

void Tactile_Xvibrate(const unsigned mag, const unsigned freq)
{
	const bool changed = S.xvib_mag != std::min(mag, 100u) || S.xvib_freq != freq;
	S.xvib_mag = std::min(mag, 100u);
	S.xvib_freq = std::min(freq, 50u);
	S.xvib_time = SDL_GetTicks();
	if (changed && S.mode != device_mode::none)
		apply_continuous();
}

void Tactile_Xvibrate_clear()
{
	/* Called every frame the player is not touching a volatile wall;
	 * only stop once the vibration has not been renewed for a moment.
	 */
	if (!S.xvib_mag || !tick_reached(SDL_GetTicks(), S.xvib_time + xvibrate_hold_ms))
		return;
	S.xvib_mag = 0;
	if (S.mode != device_mode::none)
		apply_continuous();
}

/* --- I-FORCE API --- */

void Jolt(const unsigned magnitude, const int direction, const unsigned duration)
{
	int x, y;
	dir_to_xy(magnitude, direction, x, y);
	play_jolt(x, y, std::min(duration, max_duration_ms));
}

void ButtonReflexJolt(const unsigned magnitude, const int direction, unsigned duration, unsigned repeat)
{
	auto &r = S.reflex;
	duration = std::min(duration, max_duration_ms);
	repeat = std::clamp(repeat, duration, max_duration_ms);
	dir_to_xy(magnitude, direction, r.x, r.y);
	r.duration = duration;
	r.repeat = repeat;
	r.active = true;
}

void ButtonReflexClear()
{
	S.reflex = {};
}

void Buffeting(const unsigned magnitude)
{
	S.buffet = std::min(magnitude, 100u);
	if (S.mode != device_mode::none)
		apply_continuous();
}

void EnableForces()
{
	S.forces_enabled = true;
	if (S.mode != device_mode::none)
		apply_continuous();
}

void DisableForces()
{
	S.forces_enabled = false;
	if (S.mode != device_mode::none)
	{
		stop_transients();
		apply_continuous();
	}
}

void ClearForces()
{
	S.xvib_mag = 0;
	S.buffet = 0;
	S.reflex = {};
	if (S.mode != device_mode::none)
	{
		stop_transients();
		apply_continuous();
	}
}

void tactile_set_button_jolt(const unsigned primary_weapon)
{
	if (primary_weapon >= std::size(tactile_fire_duration))
		return;
	/* Direction 0: recoil pulls the stick back. */
	ButtonReflexJolt(tactile_fire_magnitude, 0, tactile_fire_duration[primary_weapon], tactile_fire_repeat[primary_weapon]);
}

}

}

#endif
