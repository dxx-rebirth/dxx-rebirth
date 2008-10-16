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

#include "pstypes.h"

#ifndef _KCONFIG_H
#define _KCONFIG_H

typedef struct control_info {
	fix	pitch_time;
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
	ubyte	rear_view_down_count;	
	ubyte	rear_view_down_state;	
	ubyte	fire_primary_down_count;
	ubyte	fire_primary_state;
	ubyte	fire_secondary_state;
	ubyte	fire_secondary_down_count;
	ubyte	fire_flare_down_count;
	ubyte	drop_bomb_down_count;	
	ubyte	automap_down_count;
	ubyte	automap_state;
        ubyte   cycle_primary_state;
        ubyte   cycle_primary_down_count;
        ubyte   cycle_secondary_state;
        ubyte   cycle_secondary_down_count;
} control_info;

#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_MOUSE 5
#define CONTROL_MAX_TYPES 7
#define CONTROL_JOYMOUSE 8

// old stuff - kept for compability reasons
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_CYBERMAN 6

#define NUM_D1X_CONTROLS 24
#define MAX_D1X_CONTROLS 40
#define NUM_KEY_CONTROLS 46
#define NUM_JOYSTICK_CONTROLS 44
#define NUM_MOUSE_CONTROLS 29
#define MAX_CONTROLS 62
#define MAX_NOND1X_CONTROLS 50

void kconfig_sense_init();
extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char * title );
extern ubyte DefaultKeySettingsD1X[MAX_D1X_CONTROLS];
extern ubyte DefaultKeySettings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern char *control_text[CONTROL_MAX_TYPES];
extern void kc_set_controls();
extern void reset_cruise(void);
extern int kconfig_is_axes_used(int axis);
extern int isJoyRotationKey(int test_key);       //tells if "test_key" is setup for rotation on the joy
extern int isMouseRotationKey(int test_key);     //tells if "test_key" is setup for rotation on the mouse
extern int isKeyboardRotationKey(int test_key);  //tells if "test_key" is setup for rotation on the keyboard
#endif
