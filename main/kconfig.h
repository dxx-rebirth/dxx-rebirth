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



#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "config.h"
#include "gamestat.h"

typedef struct _control_info {
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

//   vms_angvec heading;
//	char oem_message[64];
  
	ubyte	afterburner_state;
	ubyte cycle_primary_count;
	ubyte cycle_secondary_count;
	ubyte headlight_count;	
} control_info;

typedef struct ext_control_info {
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

// vms_angvec heading;	 // for version >=1.0 
//	char oem_message[64]; // for version >=1.0

// vms_vector ship_pos   // for version >=2.0
// vms_matrix ship_orient // for version >=2.0
    
// ubyte cycle_primary_count // for version >=3.0
// ubyte cycle_secondary_count // for version >=3.0 
// ubyte afterburner_state // for version >=3.0
// ubyte headlight_count // for version >=3.0

// everything below this line is for version >=4.0

// ubyte headlight_state

// int primary_weapon_flags 
// int secondary_weapon_flags
// ubyte Primary_weapon_selected
// ubyte Secondary_weapon_selected

// vms_vector force_vector
// vms_matrix force_matrix
// int joltinfo[3]
// int x_vibrate_info[2]
// int y_vibrate_info[2]

// int x_vibrate_clear
// int y_vibrate_clear

// ubyte game_status;

// ubyte keyboard[128]; // scan code array, not ascii
// ubyte current_guidebot_command;

// ubyte Reactor_blown
 
} ext_control_info;

typedef struct advanced_ext_control_info {
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

 ubyte keyboard[128]; // scan code array, not ascii

 ubyte Reactor_blown;
 
} advanced_ext_control_info;

extern ubyte ExtGameStatus;
extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char * title );

//added on 2/4/99 by Victor Rachels to add new keys menu
#define NUM_D2X_CONTROLS 20
#define MAX_D2X_CONTROLS 40

extern ubyte kconfig_d2x_settings[MAX_D2X_CONTROLS];
extern ubyte default_kconfig_d2x_settings[MAX_D2X_CONTROLS];
//end this section addition - VR

#define NUM_KEY_CONTROLS 57
#define NUM_OTHER_CONTROLS 31
#define MAX_CONTROLS 60         //there are actually 48, so this leaves room for more   

extern ubyte kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern ubyte default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern char *control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);

extern void kconfig_init_external_controls(int intno, int address);


#endif
