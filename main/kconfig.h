/* $Id: kconfig.h,v 1.1.1.1 2006/03/17 19:57:17 zicodxx Exp $ */
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
#include "gamestat.h"

typedef struct _control_info {
	fix pitch_time;
	fix vertical_thrust_time;
	fix heading_time;
	fix sideways_thrust_time;
	fix bank_time;
	fix forward_thrust_time;

	ubyte rear_view_down_count;
	ubyte rear_view_down_state;

	ubyte fire_primary_down_count;
	ubyte fire_primary_state;
	ubyte fire_secondary_state;
	ubyte fire_secondary_down_count;
	ubyte fire_flare_down_count;

	ubyte drop_bomb_down_count;

	ubyte automap_down_count;
	ubyte automap_state;

	ubyte afterburner_state;
	ubyte cycle_primary_count;
	ubyte cycle_secondary_count;
	ubyte headlight_count;
} control_info;

typedef struct ext_control_info {
	fix pitch_time;
	fix vertical_thrust_time;
	fix heading_time;
	fix sideways_thrust_time;
	fix bank_time;
	fix forward_thrust_time;

	ubyte rear_view_down_count;
	ubyte rear_view_down_state;

	ubyte fire_primary_down_count;
	ubyte fire_primary_state;
	ubyte fire_secondary_state;
	ubyte fire_secondary_down_count;
	ubyte fire_flare_down_count;

	ubyte drop_bomb_down_count;

	ubyte automap_down_count;
	ubyte automap_state;

} ext_control_info;

typedef struct advanced_ext_control_info {
	fix pitch_time;
	fix vertical_thrust_time;
	fix heading_time;
	fix sideways_thrust_time;
	fix bank_time;
	fix forward_thrust_time;

	ubyte rear_view_down_count;
	ubyte rear_view_down_state;

	ubyte fire_primary_down_count;
	ubyte fire_primary_state;
	ubyte fire_secondary_state;
	ubyte fire_secondary_down_count;
	ubyte fire_flare_down_count;

	ubyte drop_bomb_down_count;

	ubyte automap_down_count;
	ubyte automap_state;

	// everything below this line is for version >=1.0

	vms_angvec heading;
	char oem_message[64];

	// everything below this line is for version >=2.0

	vms_vector ship_pos;
	vms_matrix ship_orient;

	// everything below this line is for version >=3.0

	ubyte cycle_primary_count;
	ubyte cycle_secondary_count;
	ubyte afterburner_state;
	ubyte headlight_count;

	// everything below this line is for version >=4.0

	int primary_weapon_flags;
	int secondary_weapon_flags;
	ubyte current_primary_weapon;
	ubyte current_secondary_weapon;

	vms_vector force_vector;
	vms_matrix force_matrix;
	int joltinfo[3];
	int x_vibrate_info[2];
	int y_vibrate_info[2];

	int x_vibrate_clear;
	int y_vibrate_clear;

	ubyte game_status;

	ubyte headlight_state;
	ubyte current_guidebot_command;

	ubyte keyboard[128];    // scan code array, not ascii

	ubyte Reactor_blown;

} advanced_ext_control_info;

#define NUM_D2X_CONTROLS    20
#define MAX_D2X_CONTROLS    40

#define NUM_KEY_CONTROLS    57
#define NUM_JOYSTICK_CONTROLS  56
#define NUM_MOUSE_CONTROLS  30
#define MAX_CONTROLS        60		// there are actually 48, so this leaves room for more

extern control_info Controls;
extern void controls_read_all(int automap_flag);
extern void kconfig(int n, char *title);

extern ubyte DefaultKeySettingsD2X[MAX_D2X_CONTROLS];

extern ubyte DefaultKeySettings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern char *control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);

extern int isJoyRotationKey(int test_key);       //tells if "test_key" is setup for rotation on the joy
extern int isMouseRotationKey(int test_key);     //tells if "test_key" is setup for rotation on the mouse
extern int isKeyboardRotationKey(int test_key);  //tells if "test_key" is setup for rotation on the keyboard

#endif /* _KCONFIG_H */
