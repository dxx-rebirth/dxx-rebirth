/*
 *
 * Header for joystick functions
 *
 */

#ifndef _JOY_H
#define _JOY_H

#include "pstypes.h"
#include "fix.h"

#define MAX_JOYSTICKS				8
#define MAX_AXES_PER_JOYSTICK		128
#define MAX_BUTTONS_PER_JOYSTICK	128
#define MAX_HATS_PER_JOYSTICK		4
#define JOY_MAX_AXES				(MAX_AXES_PER_JOYSTICK * MAX_JOYSTICKS)
#define JOY_MAX_BUTTONS				(MAX_BUTTONS_PER_JOYSTICK * MAX_JOYSTICKS)

extern int joy_num_axes; // set to Joystick.n_axes. solve different?
extern void joy_init();
extern void joy_close();
extern fix joy_get_button_down_time(int btn);
extern int joy_get_button_down_cnt(int btn);
extern void joystick_read_raw_axis(int *axis);
extern void joy_flush();
extern int joy_get_button_state(int btn);
extern int joy_get_scaled_reading(int raw);

#endif // _JOY_H
