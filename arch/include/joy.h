/*
 *
 * Header for joystick functions
 *
 */

#ifndef _JOY_H
#define _JOY_H

#include "pstypes.h"
#include "fix.h"

struct d_event;

#define MAX_JOYSTICKS				8
#define MAX_AXES_PER_JOYSTICK		128
#define MAX_BUTTONS_PER_JOYSTICK	128
#define MAX_HATS_PER_JOYSTICK		4
#define JOY_MAX_AXES				(MAX_AXES_PER_JOYSTICK * MAX_JOYSTICKS)
#define JOY_MAX_BUTTONS				(MAX_BUTTONS_PER_JOYSTICK * MAX_JOYSTICKS)

extern int joy_num_axes; // set to Joystick.n_axes. solve different?
extern void joy_init();
extern void joy_close();
extern void event_joystick_get_axis(struct d_event *event, int *axis, int *value);
extern void joy_flush();
extern int event_joystick_get_button(struct d_event *event);

#endif // _JOY_H
