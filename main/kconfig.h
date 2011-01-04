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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes for reading controls
 *
 */


#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "config.h"
#include "key.h"
#include "joy.h"
#include "mouse.h"

typedef struct _control_info {
	fix joy_axis[JOY_MAX_AXES], raw_joy_axis[JOY_MAX_AXES], mouse_axis[3], raw_mouse_axis[3];
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
	ubyte cycle_primary_count;
	ubyte cycle_secondary_count;
} control_info;

#define CONTROL_USING_JOYSTICK	1
#define CONTROL_USING_MOUSE		2
#define MOUSEFS_DELTA_RANGE 512
#define NUM_D1X_CONTROLS    30
#define MAX_D1X_CONTROLS    30
#define NUM_KEY_CONTROLS 50
#define NUM_JOYSTICK_CONTROLS 48
#define NUM_MOUSE_CONTROLS 29
#define MAX_CONTROLS 50

extern control_info Controls;
extern void controls_read_all(int automap_flag);
extern void kconfig(int n, char *title);

extern ubyte DefaultKeySettingsD1X[MAX_D1X_CONTROLS];
extern ubyte DefaultKeySettings[3][MAX_CONTROLS];

extern void kc_set_controls();

//set the cruise speed to zero
extern void reset_cruise(void);

#endif /* _KCONFIG_H */
