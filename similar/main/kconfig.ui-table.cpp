/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"

#define DXX_KCONFIG_UI_UDLR3(I)	DXX_KCONFIG_UI_UDLR_ ## I
#define DXX_KCONFIG_UI_UDLR2(I)	DXX_KCONFIG_UI_UDLR3(I)
#define DXX_KCONFIG_UI_UDLR()	DXX_KCONFIG_UI_UDLR2(__LINE__)

//	  x,  y, xi, w2,  u,  d,   l, r, type, state_bit, state_ptr
constexpr kc_item kc_keyboard[] = {
	{ 15, 49, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_pitch_forward} },
	{ 15, 49,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_pitch_forward} },
	{ 15, 57, 86, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_pitch_backward} },
	{ 15, 57,115, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_pitch_backward} },
	{ 15, 65, 86, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_heading_left} },
	{ 15, 65,115, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_heading_left} },
	{ 15, 73, 86, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_heading_right} },
	{ 15, 73,115, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_heading_right} },
	{ 15, 85, 86, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::slide_on} },
	{ 15, 85,115, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::slide_on} },
	{ 15, 93, 86, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_left} },
	{ 15, 93,115, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_left} },
	{ 15,101, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_right} },
	{ 15,101,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_right} },
	{ 15,109, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_up} },
	{ 15,109,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_up} },
	{ 15,117, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_slide_down} },
	{ 15,117,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_slide_down} },
	{ 15,129, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::bank_on} },
	{ 15,129,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::bank_on} },
	{ 15,137, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_left} },
	{ 15,137,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_left} },
	{ 15,145, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::key_bank_right} },
	{ 15,145,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::key_bank_right} },
	{158, 49,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::fire_primary} },
	{158, 49,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::fire_primary} },
	{158, 57,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::fire_secondary} },
	{158, 57,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::fire_secondary} },
	{158, 65,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::fire_flare} },
	{158, 65,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::fire_flare} },
	{158,105,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::accelerate} },
	{158,105,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::accelerate} },
	{158,113,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::reverse} },
	{158,113,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::reverse} },
	{158, 73,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::drop_bomb} },
	{158, 73,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::drop_bomb} },
	{158, 85,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::rear_view} },
	{158, 85,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::rear_view} },
#if defined(DXX_BUILD_DESCENT_I)
	{158,125,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_plus} },
	{158,125,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_plus} },
	{158,133,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_minus} },
	{158,133,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_minus} },
	{158,141,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
	{158,141,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,133,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_plus} },
	{158,133,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_plus} },
	{158,141,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::cruise_minus} },
	{158,141,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::cruise_minus} },
	{158,149,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
	{158,149,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cruise_off} },
#endif
	{158, 93,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::automap} },
	{158, 93,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::automap} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 15,157, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,157,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,165, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 15,165,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{158,121,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::afterburner} },
	{158,121,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::afterburner} },
	{ 15,161, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,161,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 15,169, 86, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 15,169,115, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::cycle_secondary} },
	{158,163,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::headlight} },
	{158,163,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::headlight} },
	{158,171,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT1, {&control_info::state_controls_t::energy_to_shield} },
	{158,171,270, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, STATE_BIT2, {&control_info::state_controls_t::energy_to_shield} },
	{158,179,241, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::toggle_bomb} },
#endif
};

#if DXX_MAX_JOYSTICKS
constexpr kc_item kc_joystick[] = {
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
#if defined(DXX_BUILD_DESCENT_I)
	{ 22, 46,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::reverse} },
	{ 22, 62,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22, 46,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::reverse} },
	{ 22, 62,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
#endif
	{174, 46,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::slide_on} },
	{174, 54,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::bank_on} },
	{174, 94,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::btn_bank_right} },
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
	{ 22,154, 73, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{ 22,154,121,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{ 22,162, 73, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{ 22,162,121,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{164,154,222, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{164,154,270,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{164,162,222, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{164,162,270,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{164,170,222, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{164,170,270,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{164,178,222, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_AXIS, 0, {NULL} },
	{164,178,270,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
#endif
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
#if defined(DXX_BUILD_DESCENT_I)
	{ 22, 94,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,104, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::automap} },
	{ 22,102,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::automap} },
	{ 22, 46,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::reverse} },
	{ 22, 62,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22, 94,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::afterburner} },
	{174,110,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
	{ 22,110,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::headlight} },
	{ 22, 46,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_primary} },
	{ 22, 54,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::fire_secondary} },
	{ 22, 78,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::accelerate} },
	{ 22, 86,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::reverse} },
	{ 22, 62,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
#endif
	{174, 46,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::slide_on} },
	{174, 54,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_left} },
	{174, 62,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_right} },
	{174, 70,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_up} },
	{174, 78,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_slide_down} },
	{174, 86,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::bank_on} },
	{174, 94,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_left} },
	{174,102,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::btn_bank_right} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 22, 94,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,133, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{174,110,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 22, 94,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::rear_view} },
	{ 22, 70,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
	{ 22,102,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::afterburner} },
#endif
	{174,110,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{174,118,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#if defined(DXX_BUILD_DESCENT_II)
	{ 22,110,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::headlight} },
	{ 22,126,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::automap} },
	{ 22,126,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::automap} },
	{ 22,118,102, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT3, {&control_info::state_controls_t::energy_to_shield} },
	{ 22,118,132, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, STATE_BIT4, {&control_info::state_controls_t::energy_to_shield} },
	{174,126,248, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::toggle_bomb} },
	{174,126,278, 26, DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::toggle_bomb} },
#endif
#endif
};
#endif

constexpr kc_item kc_mouse[] = {
	{ 25, 46,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::fire_primary} },
	{ 25, 54,110, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::fire_secondary} },
	{ 25, 78,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::accelerate} },
	{ 25, 86,110, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::reverse} },
	{ 25, 62,110, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::fire_flare} },
	{180, 46,239, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::slide_on} },
	{180, 54,239, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_left} },
	{180, 62,239, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_right} },
	{180, 70,239, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_up} },
	{180, 78,239, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_slide_down} },
	{180, 86,239, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::bank_on} },
	{180, 94,239, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_bank_left} },
	{180,102,239, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::btn_bank_right} },
	{ 25,154, 83, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,154,131,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{ 25,162, 83, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,162,131,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{ 25,170, 83, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,170,131,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{ 25,178, 83, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{ 25,178,131,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{180,154,238, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{180,154,286,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{180,162,238, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_AXIS, 0, {NULL} },
	{180,162,286,  8, DXX_KCONFIG_UI_UDLR(), BT_INVERT, 0, {NULL} },
	{ 25, 94,110, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::rear_view} },
	{ 25, 70,110, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::drop_bomb} },
#if defined(DXX_BUILD_DESCENT_I)
	{ 25,102,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 25,110,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#elif defined(DXX_BUILD_DESCENT_II)
	{ 25,102,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, STATE_BIT5, {&control_info::state_controls_t::afterburner} },
	{ 25,110,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_primary} },
	{ 25,118,110, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::cycle_secondary} },
#endif
};

constexpr kc_item kc_rebirth[] = {
	{ 15, 69,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 69,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 69,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,157, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26),  DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 77,273, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,157, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26),  DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 85,273, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,157, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26),  DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15, 93,273, 26,  DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,157, 26,  DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,101,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,109,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,117,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,125,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,133,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,157, 26, DXX_KCONFIG_UI_UDLR(), BT_KEY, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,215, DXX_KCONFIG_ITEM_JOY_WIDTH(26), DXX_KCONFIG_UI_UDLR(), BT_JOY_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
	{ 15,141,273, 26, DXX_KCONFIG_UI_UDLR(), BT_MOUSE_BUTTON, 0, {&control_info::state_controls_t::select_weapon} },
};

#undef DXX_KCONFIG_UI_UDLR
#undef DXX_KCONFIG_UI_UDLR2
#undef DXX_KCONFIG_UI_UDLR3
