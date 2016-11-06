/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Header for joystick functions
 *
 */

#pragma once

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "event.h"

#if DXX_MAX_JOYSTICKS
#include "pstypes.h"
#include "maths.h"
#include <SDL.h>

namespace dcx {

#define JOY_MAX_AXES				(DXX_MAX_AXES_PER_JOYSTICK * DXX_MAX_JOYSTICKS)
#define JOY_MAX_BUTTONS				(DXX_MAX_BUTTONS_PER_JOYSTICK * DXX_MAX_JOYSTICKS)

struct d_event_joystick_axis_value
{
	unsigned axis;
	int value;
};

extern void joy_init();
extern void joy_close();
const d_event_joystick_axis_value &event_joystick_get_axis(const d_event &event);
extern void joy_flush();
extern int event_joystick_get_button(const d_event &event);

}
#else
#define joy_init()
#define joy_flush()
#define joy_close()
#endif

namespace dcx {

#if DXX_MAX_BUTTONS_PER_JOYSTICK
extern window_event_result joy_button_handler(SDL_JoyButtonEvent *jbe);
#else
#define joy_button_handler(jbe) (static_cast<SDL_JoyButtonEvent *const &>(jbe), window_event_result::ignored)
#endif

#if DXX_MAX_HATS_PER_JOYSTICK
extern window_event_result joy_hat_handler(SDL_JoyHatEvent *jhe);
#else
#define joy_hat_handler(jbe) (static_cast<SDL_JoyHatEvent *const &>(jbe), window_event_result::ignored)
#endif

#if DXX_MAX_AXES_PER_JOYSTICK
extern window_event_result joy_axis_handler(SDL_JoyAxisEvent *jae);
#else
#define joy_axis_handler(jbe) (static_cast<SDL_JoyAxisEvent *const &>(jbe), window_event_result::ignored)
#endif

}
