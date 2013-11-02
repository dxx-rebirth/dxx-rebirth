/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes for reading controls
 *
 */


#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "config.h"
#include "event.h"
#include "key.h"
#include "joy.h"
#include "mouse.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _control_info {
	float key_pitch_forward_down_time, key_pitch_backward_down_time, key_heading_left_down_time, key_heading_right_down_time, key_slide_left_down_time, key_slide_right_down_time, key_slide_up_down_time, key_slide_down_down_time, key_bank_left_down_time, key_bank_right_down_time; // to scale movement depending on how long the key is pressed
	fix pitch_time, vertical_thrust_time, heading_time, sideways_thrust_time, bank_time, forward_thrust_time;
	ubyte key_pitch_forward_state, key_pitch_backward_state, key_heading_left_state, key_heading_right_state, key_slide_left_state, key_slide_right_state, key_slide_up_state, key_slide_down_state, key_bank_left_state, key_bank_right_state; // to scale movement for keys only we need them to be seperate from joystick/mouse buttons
	ubyte btn_slide_left_state, btn_slide_right_state, btn_slide_up_state, btn_slide_down_state, btn_bank_left_state, btn_bank_right_state;
	ubyte slide_on_state, bank_on_state;
	ubyte accelerate_state, reverse_state, cruise_plus_state, cruise_minus_state, cruise_off_count;
	ubyte rear_view_state;
	ubyte fire_primary_state, fire_secondary_state, fire_flare_count, drop_bomb_count;
	ubyte automap_state;
	ubyte cycle_primary_count, cycle_secondary_count, select_weapon_count;
#if defined(DXX_BUILD_DESCENT_II)
	ubyte toggle_bomb_count;
	ubyte afterburner_state, headlight_count, energy_to_shield_state;
#endif
	fix joy_axis[JOY_MAX_AXES], raw_joy_axis[JOY_MAX_AXES], mouse_axis[3], raw_mouse_axis[3];
} control_info;

#define CONTROL_USING_JOYSTICK	1
#define CONTROL_USING_MOUSE		2
#define MOUSEFS_DELTA_RANGE 512
#if defined(DXX_BUILD_DESCENT_I)
#define NUM_DXX_REBIRTH_CONTROLS    30
#define MAX_DXX_REBIRTH_CONTROLS    30
#define MAX_CONTROLS 50
#elif defined(DXX_BUILD_DESCENT_II)
#define NUM_DXX_REBIRTH_CONTROLS    30
#define MAX_DXX_REBIRTH_CONTROLS    30
#define MAX_CONTROLS        60		// there are actually 48, so this leaves room for more
#endif

extern control_info Controls;
extern void kconfig_read_controls(d_event *event, int automap_flag);
extern void kconfig(int n, const char *title);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern const ubyte DefaultKeySettingsRebirth[MAX_DXX_REBIRTH_CONTROLS];
extern const ubyte DefaultKeySettings[3][MAX_CONTROLS];
#endif

extern void kc_set_controls();

//set the cruise speed to zero
extern void reset_cruise(void);

extern char *joybutton_text[JOY_MAX_BUTTONS];
extern char *joyaxis_text[JOY_MAX_AXES];
extern fix Cruise_speed;

#ifdef __cplusplus
}
#endif

#endif /* _KCONFIG_H */
