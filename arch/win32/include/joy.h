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

#ifndef _JOY_H
#define _JOY_H

#include "types.h"
#include "fix.h"

#define JOY_1_BUTTON_A	1
#define JOY_1_BUTTON_B	2
#define JOY_2_BUTTON_A	4
#define JOY_2_BUTTON_B	8
#define JOY_ALL_BUTTONS	(1+2+4+8)

#define JOY_1_X_AXIS		1
#define JOY_1_Y_AXIS		2
#define JOY_1_R_AXIS            4
#define JOY_1_Z_AXIS            8
#define JOY_1_U_AXIS            16
#define JOY_1_V_AXIS            32
#define JOY_ALL_AXIS            (1+2+4+8+16+32)

//#if 0
//#define JOY_2_X_AXIS            4
//#define JOY_2_Y_AXIS            8
//#define JOY_ALL_AXIS            (1+2+4+8)
//#endif

#define JOY_POLL_RATE		0 // use minimum FIXME: is this what we want?

#define JOY_NUM_AXES 6

#define MAX_BUTTONS 20

//==========================================================================
// This initializes the joy and does a "quick" calibration which
// assumes the stick is centered and sets the minimum value to 0 and
// the maximum value to 2 times the centered reading. Returns 0 if no
// joystick was detected, 1 if everything is ok.
// joy_init() is called.

extern int joy_init();
extern void joy_close();

extern char joy_installed;
extern char joy_present;

//==========================================================================
// The following 3 routines can be used to zero in on better joy
// calibration factors. To use them, ask the user to hold the stick
// in either the upper left, lower right, or center and then have them
// press a key or button and then call the appropriate one of these
// routines, and it will read the stick and update the calibration factors.
// Usually, assuming that the stick was centered when joy_init was
// called, you really only need to call joy_set_lr, since the upper
// left position is usually always 0,0 on most joys.  But, the safest
// bet is to do all three, or let the user choose which ones to set.

extern void joy_set_ul();
extern void joy_set_lr();
extern void joy_set_cen();


//==========================================================================
// This reads the joystick. X and Y will be between -128 and 127.
// Takes about 1 millisecond in the worst case when the stick
// is in the lower right hand corner. Always returns 0,0 if no stick
// is present.

extern void joy_get_pos( int *x, int *y );

//==========================================================================
// This just reads the buttons and returns their status.  When bit 0
// is 1, button 1 is pressed, when bit 1 is 1, button 2 is pressed.
extern int joy_get_btns();

//==========================================================================
// This returns the number of times a button went either down or up since
// the last call to this function.
extern int joy_get_button_up_cnt( int btn );
extern int joy_get_button_down_cnt( int btn );

//==========================================================================
// This returns how long (in approximate milliseconds) that each of the
// buttons has been held down since the last call to this function.
// It is the total time... say you pressed it down for 3 ticks, released
// it, and held it down for 6 more ticks. The time returned would be 9.
extern fix joy_get_button_down_time( int btn );

extern ubyte joy_read_raw_buttons();
extern ubyte joystick_read_raw_axis( ubyte mask, int * axis );
extern void joy_flush();
extern ubyte joy_get_present_mask();
extern void joy_set_timer_rate(int max_value );
extern int joy_get_timer_rate();

extern int joy_get_button_state( int btn );
extern void joy_set_cen_fake(int channel);
extern ubyte joy_read_stick( ubyte masks, int *axis );
extern void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max);
extern void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max);
extern void joy_set_btn_values( int btn, int state, fix timedown, int downcount, int upcount );
extern int joy_get_scaled_reading( int raw, int axn );
extern void joy_set_slow_reading( int flag );


extern int JOY_PORT;
extern int joy_deadzone;
#endif

