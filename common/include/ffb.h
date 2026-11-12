/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Force feedback, restoring the I-FORCE ("tactile") support of the
 * original Descent II Windows release.  The game calls the same
 * functions the original tactile.c / I-FORCE API provided; they are
 * played on an SDL2 haptic device (joystick or wheel) or as gamepad
 * rumble.  With SDL 1.2 or without joystick support, every call is a
 * no-op.
 *
 */

#pragma once

#include "dxxsconf.h"
#include <SDL_version.h>
#include "fwd-vecmat.h"

#if DXX_MAX_JOYSTICKS && SDL_MAJOR_VERSION == 2
#define DXX_USE_FFB	1
#include <SDL_joystick.h>
#else
#define DXX_USE_FFB	0
#endif

namespace dcx {

namespace tactile {

#if DXX_USE_FFB

/* Called once per game frame.  enabled, strength_percent and
 * centering_percent (centering spring, 0 = off) come from the
 * player config; fire_held is the primary fire control (drives the
 * button reflex jolt).
 */
void frame(bool enabled, unsigned strength_percent, unsigned centering_percent, bool fire_held);
void close();
/* Controller hotplug: retry opening a device / drop a removed one. */
void device_added();
void device_removed(SDL_JoystickID instance_id);
/* Short test jolt, used by the options menu. */
void test_jolt();

/* tactile.c */
void Tactile_apply_force(const vms_vector &force_vec, const vms_matrix &orient);
void Tactile_do_collide(const vms_vector &force_vec, const vms_matrix &orient);
void Tactile_Xvibrate(unsigned mag, unsigned freq);
void Tactile_Xvibrate_clear();

/* I-FORCE API.  Magnitudes 0-100, directions in degrees (the side the
 * force comes from, 0 = front), durations in ms.
 */
void Jolt(unsigned magnitude, int direction, unsigned duration);
void ButtonReflexJolt(unsigned magnitude, int direction, unsigned duration, unsigned repeat);
void ButtonReflexClear();
void Buffeting(unsigned magnitude);
void EnableForces();
void DisableForces();
void ClearForces();

/* weapon.c: arm the fire jolt for the selected primary weapon. */
void tactile_set_button_jolt(unsigned primary_weapon);

#else

static inline void frame(bool, unsigned, unsigned, bool) {}
static inline void close() {}
static inline void test_jolt() {}
static inline void Tactile_apply_force(const vms_vector &, const vms_matrix &) {}
static inline void Tactile_do_collide(const vms_vector &, const vms_matrix &) {}
static inline void Tactile_Xvibrate(unsigned, unsigned) {}
static inline void Tactile_Xvibrate_clear() {}
static inline void Jolt(unsigned, int, unsigned) {}
static inline void ButtonReflexJolt(unsigned, int, unsigned, unsigned) {}
static inline void ButtonReflexClear() {}
static inline void Buffeting(unsigned) {}
static inline void EnableForces() {}
static inline void DisableForces() {}
static inline void ClearForces() {}
static inline void tactile_set_button_jolt(unsigned) {}

#endif

}

}
