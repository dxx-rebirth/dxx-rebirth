//JOYC.C for D1_3Dfx and D1OpenGL
//D1_3Dfx is a Win32 executable using Glide and DirectX 3
//D1OpenGL is a Win32 executable using OpenGL and DirectX 3
//This joystick code should run on DirectInput 2 and higher
//We are not using DirectX 5 for now, since we want to stay compatible to the HH-loved Windows NT :).
//So ForceFeedback is not supported

//Notes:
// 1) Windows DirectX supports up to 7 axes, D1/DOS only 4, so we needed to increase the number to 7 at several points
//    Also JOY_ALL_AXIS must be (1+2+4+8+16+32+64) in JOY.H file
// 2) Windows DirectX supports up to 32 buttons. So far we however only support 4. Adding support for more should be however easily possible.
// 3) _enable and _disable are not needed
// 4) joy_bogus_reading is not needed, so all calls are just removed
// 5) The joystick query goes over the DirectInputs function
//    MMRESULT joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji);
//    All functions reading over BIOS, including the ASM code are removed



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
Modifications by Heiko Herrmann
Later modified by Owen Evans to work with D1X (3/7/99)
*/


#define _WIN32_OS		//HH

#include <windows.h>	//HH
#include <mmsystem.h>	//HH
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "joy.h"
#include "timer.h"
#include "args.h"

#define my_hwnd g_hWnd // D1X compatibility
extern HWND g_hWnd;

char joy_installed = 0;
char joy_present = 0;

typedef struct Button_info {
	ubyte		ignore;
	ubyte		state;
	ubyte		last_state;
	int		timedown;
	ubyte		downcount;
	ubyte		upcount;
} Button_info;

typedef struct Joy_info {
	int			joyid;
	ubyte			present_mask;
	ubyte			slow_read;
	int			max_timer;
	int			read_count;
	ubyte			last_value;
	Button_info	buttons[MAX_BUTTONS];
        int                     axis_min[JOY_NUM_AXES];    //changed 
        int                     axis_center[JOY_NUM_AXES]; //changed --orulz
        int                     axis_max[JOY_NUM_AXES];    //changed 
} Joy_info;

Joy_info joystick;


int joy_deadzone = 0;

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

        for (i=0; i<JOY_NUM_AXES; i++)             {       //changed - orulz
		axis_min[i] = joystick.axis_min[i];
		axis_center[i] = joystick.axis_center[i];
		axis_max[i] = joystick.axis_max[i];
	}
}

void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;

        for (i=0; i<JOY_NUM_AXES; i++)             {       //changed - orulz
		joystick.axis_min[i] = axis_min[i];
		joystick.axis_center[i] = axis_center[i];
		joystick.axis_max[i] = axis_max[i];
	}
}

ubyte joy_get_present_mask()	{
	return joystick.present_mask;
}

void joy_set_timer_rate(int max_value )	{
	joystick.max_timer = max_value;
}

int joy_get_timer_rate()	{
	return joystick.max_timer;
}

void joy_flush()	{
	int i;

	if (!joy_installed) return;

	for (i=0; i<MAX_BUTTONS; i++ )	{
		joystick.buttons[i].ignore = 0;
		joystick.buttons[i].state = 0;	
		joystick.buttons[i].timedown = 0;	
		joystick.buttons[i].downcount = 0;	
		joystick.buttons[i].upcount = 0;	
	}

}


//Repalces joy_handler
//Since Windows calls us directly, we have to get our time difference since last time ourselves
//while in DOS we got it with the paramter ticks_this_time
LRESULT joy_handler32(HWND hWnd, UINT joymsg, UINT wParam, LPARAM lParam)
{
	DWORD time_diff, time_now;
	static DWORD time_last = 0xffffffff;
	
	Button_info *button;
	int i;
	int state=0;
	DWORD value=0;

	if (!joy_installed) return 1;
	if (time_last == 0xffffffff) {
		time_last = timer_get_fixed_seconds();
		return 0;
	}

	if (joymsg==MM_JOY1MOVE)
	{
		JOYINFOEX	joy;

		memset(&joy, 0, sizeof(joy));
		joy.dwSize = sizeof(joy);
		joy.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNPOV;
		if (joyGetPosEx(joystick.joyid, &joy)!=JOYERR_NOERROR) {
			return 1;
		}
		value = joy.dwButtons;
	}

	time_now = timer_get_fixed_seconds();
	if (time_now < time_last) {
		time_last = abs(time_now-time_last);
	}
	time_diff = time_now - time_last;

	for (i = 0; i < MAX_BUTTONS; i++)
	{
		button = &joystick.buttons[i];
		
		if (!button->ignore) {
			if ( i < (MAX_BUTTONS-4) )
				state = (value >> i) & 1;
			else
				state = 0;

			if ( button->last_state == state )	{
				if (state) {
					button->timedown += time_diff;	//ticks_this_time;
				}
			} else {
				if (state)	{
					button->downcount += state;
					button->state = 1;
				} else {	
					button->upcount += button->state;
					button->state = 0;
				}
				button->last_state = state;
			}
		}
	}

	time_last = time_now;
	return 0;
}		


//Begin section modified 3/7/99 - Owen Evans

ubyte joy_read_raw_buttons()
{
	JOYINFOEX joy;
        int i;

        if (!joy_present)
		return 0; 
	
	memset(&joy, 0, sizeof(joy));
	joy.dwSize = sizeof(joy);
        joy.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNPOVCTS | JOY_USEDEADZONE;
	
        if (joyGetPosEx(joystick.joyid, &joy)!=JOYERR_NOERROR)
		return 0;

        for (i = 0; i < MAX_BUTTONS; i++) {
                joystick.buttons[i].last_state = joystick.buttons[i].state;
                joystick.buttons[i].state = (joy.dwButtons >> i) & 0x1;
                if (!joystick.buttons[i].last_state && joystick.buttons[i].state) {
                        joystick.buttons[i].timedown = timer_get_fixed_seconds();
                        joystick.buttons[i].downcount++;
                }
        }

        /* Hat stuff */

        if (joy.dwPOV != JOY_POVCENTERED)
         {
           joystick.buttons[19].state = (joy.dwPOV < JOY_POVRIGHT || joy.dwPOV > JOY_POVLEFT);
           joystick.buttons[15].state = (joy.dwPOV < JOY_POVBACKWARD && joy.dwPOV > JOY_POVFORWARD);
           joystick.buttons[11].state = (joy.dwPOV < JOY_POVLEFT && joy.dwPOV > JOY_POVRIGHT);
           joystick.buttons[7].state = (joy.dwPOV > JOY_POVBACKWARD);
         }

        return (ubyte)joy.dwButtons;
}

//end changed section - OE


ubyte joystick_read_raw_axis( ubyte mask, int * axis )
{
	JOYINFOEX	joy;
	ubyte read_masks = 0;

        axis[0] = 0; axis[1] = 0; //orulz: 1=x 2=y 3=r 4=z 5=u 6=v
        axis[2] = 0; axis[3] = 0;
	axis[4] = 0; axis[5] = 0; //HH: Added support for axes R and U
	
	if (!joy_installed) {
		return 0;
	}
	memset(&joy, 0, sizeof(joy));
	joy.dwSize = sizeof(joy);
	joy.dwFlags = JOY_RETURNALL | JOY_USEDEADZONE;
	if (joyGetPosEx(joystick.joyid, &joy)!=JOYERR_NOERROR) {
		return 0;
	}
			
	mask &= joystick.present_mask;			// Don't read non-present channels
	if ( mask==0 )	{
		return 0;		// Don't read if no stick connected.
	}

	if (mask & JOY_1_X_AXIS) { axis[0] = joy.dwXpos; read_masks |= JOY_1_X_AXIS; }
	if (mask & JOY_1_Y_AXIS) { axis[1] = joy.dwYpos; read_masks |= JOY_1_Y_AXIS; }
//orulz:
//        if (mask & JOY_1_Z_AXIS) { axis[2] = joy.dwZpos; read_masks |= JOY_1_Z_AXIS; }
//        if (mask & JOY_1_POV)    { axis[3] = joy.dwPOV;  read_masks |= JOY_1_POV;    }
//        if (mask & JOY_1_R_AXIS) { axis[4] = joy.dwRpos; read_masks |= JOY_1_R_AXIS; }
//        if (mask & JOY_1_U_AXIS) { axis[5] = joy.dwUpos; read_masks |= JOY_1_U_AXIS; }
//        if (mask & JOY_1_V_AXIS) { axis[6] = joy.dwVpos; read_masks |= JOY_1_V_AXIS; }
        if (mask & JOY_1_R_AXIS) { axis[2] = joy.dwRpos; read_masks |= JOY_1_R_AXIS; }
        if (mask & JOY_1_Z_AXIS) { axis[3] = joy.dwZpos; read_masks |= JOY_1_Z_AXIS; }
        if (mask & JOY_1_U_AXIS) { axis[4] = joy.dwZpos; read_masks |= JOY_1_U_AXIS; }
        if (mask & JOY_1_V_AXIS) { axis[5] = joy.dwUpos; read_masks |= JOY_1_V_AXIS; }

	return read_masks;
}

int hh_average(int val1, int val2)
{
	return abs(val1-val2)/2;
}

int joy_init(int joyid) //HH: added joyid parameter
{
	int i;
        int temp_axis[JOY_NUM_AXES];       //changed - orulz
	JOYCAPS pjc;

	if (FindArg( "-nojoystick" ))
		return 0;

	atexit(joy_close);	//HH: we are a bit lazy :). Errors are ignored, so we are even double-lazy :)

	joy_flush();
	memset(&joystick, 0, sizeof(joystick));

	for (i=0; i<MAX_BUTTONS; i++)
		joystick.buttons[i].last_state = 0;

	if ( !joy_installed )   {
                joy_present = 0;
                joy_installed = 1;
                joystick.max_timer = 65536;
                joystick.read_count = 0;
                joystick.last_value = 0;
	}


	joystick.present_mask = JOY_ALL_AXIS;		// Assume they're all present
        joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS, temp_axis );

	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;

	
	//HH: Main Win32 joystick initialization, incl. reading cal. stuff

	if (joyGetDevCaps(joyid, &pjc, sizeof(pjc))!=JOYERR_NOERROR) {
		return 0;
	}

	if (joySetThreshold(joyid, pjc.wXmax/256)!=JOYERR_NOERROR) {
		return 0;
	}
	
	if (joySetCapture(my_hwnd, joyid, JOY_POLL_RATE, FALSE)!=JOYERR_NOERROR) {
		return 0;
	}

        joystick.max_timer      = pjc.wPeriodMax;
	joystick.axis_min[0]	= pjc.wXmin;
	joystick.axis_min[1]	= pjc.wYmin;
//orulz:
//        joystick.axis_min[2]    = pjc.wZmin;
//        //HH: joystick.axis_min[3]  = pov-stuff
//        joystick.axis_min[4]    = pjc.wRmin;
//        joystick.axis_min[5]    = pjc.wUmin;
//        joystick.axis_min[6]    = pjc.wVmin;
        joystick.axis_min[2]    = pjc.wRmin;
        joystick.axis_min[3]    = pjc.wZmin;
        joystick.axis_min[4]    = pjc.wUmin;
        joystick.axis_min[5]    = pjc.wVmin;

	joystick.axis_max[0]	= pjc.wXmax;
	joystick.axis_max[1]	= pjc.wYmax;
//orulz:
//        joystick.axis_max[2]    = pjc.wZmax;
//        //HH: joystick.axis_max[3]  = pov-stuff
//        joystick.axis_max[4]    = pjc.wRmax;
//        joystick.axis_max[5]    = pjc.wUmax;
//        joystick.axis_max[6]    = pjc.wVmax;
        joystick.axis_max[2]    = pjc.wRmax;
        joystick.axis_max[3]    = pjc.wZmax;
        joystick.axis_max[4]    = pjc.wUmax;
        joystick.axis_max[5]    = pjc.wVmax;

        joystick.axis_center[0]	= hh_average(pjc.wXmax,pjc.wXmin);
	joystick.axis_center[1]	= hh_average(pjc.wYmax,pjc.wYmin);
//orulz:
//        joystick.axis_center[2] = hh_average(pjc.wZmax,pjc.wZmin);
//        joystick.axis_center[3] = JOY_POVCENTERED;
//        joystick.axis_center[4] = hh_average(pjc.wRmax,pjc.wRmin);
//        joystick.axis_center[5] = hh_average(pjc.wUmax,pjc.wUmin);
//        joystick.axis_center[6] = hh_average(pjc.wVmax,pjc.wVmin);
        joystick.axis_center[2] = hh_average(pjc.wRmax,pjc.wRmin);
        joystick.axis_center[3] = hh_average(pjc.wZmax,pjc.wZmin);
        joystick.axis_center[4] = hh_average(pjc.wUmax,pjc.wUmin);
        joystick.axis_center[5] = hh_average(pjc.wVmax,pjc.wVmin);

	joystick.present_mask = JOY_1_X_AXIS | JOY_1_Y_AXIS;
	if (pjc.wCaps & JOYCAPS_HASZ)	joystick.present_mask |= JOY_1_Z_AXIS;
//        if (pjc.wCaps & JOYCAPS_HASPOV) joystick.present_mask |= JOY_1_POV;
	if (pjc.wCaps & JOYCAPS_HASR)	joystick.present_mask |= JOY_1_R_AXIS;
	if (pjc.wCaps & JOYCAPS_HASU)	joystick.present_mask |= JOY_1_U_AXIS;
	if (pjc.wCaps & JOYCAPS_HASV)	joystick.present_mask |= JOY_1_V_AXIS;

	return joy_present;
}


void joy_close()
{
	if (!joy_installed) return;
	joyReleaseCapture(joystick.joyid); //HH: added to release joystick from the application. We ignore errors here
	joy_installed = 0;
}


void joy_set_ul()	
{
	joystick.present_mask = JOY_ALL_AXIS;		// Assume they're all present
        joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS, joystick.axis_min );

	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}

void joy_set_lr()	
{
	joystick.present_mask = JOY_ALL_AXIS;		// Assume they're all present
        joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS, joystick.axis_max );

	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}

void joy_set_cen() 
{
	joystick.present_mask = JOY_ALL_AXIS;		// Assume they're all present
        joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS, joystick.axis_center );

	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}

void joy_set_cen_fake(int channel)	
{ }


int joy_get_scaled_reading( int raw, int axn )	
{
 int x, d;

  // Make sure it's calibrated properly.
   if ( joystick.axis_center[axn] - joystick.axis_min[axn] < 128 )
    return 0; //HH: had to increase to 128
   if ( joystick.axis_max[axn] - joystick.axis_center[axn] < 128 )
    return 0; //HH: had to increase to 128
    
   if (!(joystick.present_mask & (1<<axn))) return 0;//fixes joy config bug where it'll always set an axis you don't even have. - 2000/01/14 Matt Mueller

  raw -= joystick.axis_center[axn];

   if ( raw < 0 )
    d = joystick.axis_center[axn]-joystick.axis_min[axn];
   else
    d = joystick.axis_max[axn]-joystick.axis_center[axn];


   if ( d )
    x = (raw << 7) / d;
   else 
    x = 0;


   if ( x < -128 ) x = -128;
   if ( x > 127 ) x = 127;

//added on 4/13/99 by Victor Rachels to add deadzone control
  d =  (joy_deadzone) * 6;
   if ((x > (-1*d)) && (x < d))
    x = 0;
//end this section addition -VR

	return x;
}

void joy_get_pos( int *x, int *y )	
{
	ubyte flags;
        int axis[JOY_NUM_AXES];

	if ((!joy_installed)||(!joy_present)) { *x=*y=0; return; }

	flags=joystick_read_raw_axis( JOY_1_X_AXIS+JOY_1_Y_AXIS, axis );

	if ( flags & JOY_1_X_AXIS )
		*x = joy_get_scaled_reading( axis[0], 0 );
	else
		*x = 0;

	if ( flags & JOY_1_Y_AXIS )
		*y = joy_get_scaled_reading( axis[1], 1 );
	else
		*y = 0;
}

ubyte joy_read_stick( ubyte masks, int *axis )	
{
	ubyte flags;
        int raw_axis[JOY_NUM_AXES];

	if ((!joy_installed)||(!joy_present)) { 
		axis[0] = 0; axis[1] = 0;
		axis[2] = 0; axis[3] = 0;
                axis[4] = 0; axis[5] = 0; 
		return 0;  
	}

	flags=joystick_read_raw_axis( masks, raw_axis );

	if ( flags & JOY_1_X_AXIS )
		axis[0] = joy_get_scaled_reading( raw_axis[0], 0 );
	else
		axis[0] = 0;

	if ( flags & JOY_1_Y_AXIS )
		axis[1] = joy_get_scaled_reading( raw_axis[1], 1 );
	else
		axis[1] = 0;

        if ( flags & JOY_1_R_AXIS )
                axis[2] = joy_get_scaled_reading( raw_axis[2], 2 );
	else
                axis[2] = 0;

        if ( flags & JOY_1_Z_AXIS )
                axis[3] = joy_get_scaled_reading( raw_axis[3], 3 );
	else
		axis[3] = 0;

	if ( flags & JOY_1_U_AXIS )
                axis[4] = joy_get_scaled_reading( raw_axis[4], 4);
	else
                axis[4] = 0;

	if ( flags & JOY_1_V_AXIS )
                axis[5] = joy_get_scaled_reading( raw_axis[5], 5 );
	else
                axis[5] = 0;


	return flags;
}

int joy_get_btns()	
{
	if ((!joy_installed)||(!joy_present)) return 0;

	return joy_read_raw_buttons();
}


//Begin section modified 3/7/99 - Owen Evans

void joy_get_btn_down_cnt( int *btn0, int *btn1 )
{
	if ((!joy_installed)||(!joy_present)) { *btn0=*btn1=0; return; }

        joy_get_btns();

	*btn0 = joystick.buttons[0].downcount;
	joystick.buttons[0].downcount = 0;
	*btn1 = joystick.buttons[1].downcount;
	joystick.buttons[1].downcount = 0;
}

int joy_get_button_state( int btn )	
{    
	if ((!joy_installed)||(!joy_present)) return 0;
	if ( btn >= MAX_BUTTONS ) return 0;

        joy_get_btns();

        return joystick.buttons[btn].state;
}

int joy_get_button_up_cnt( int btn ) 
{
	int count;

	if ((!joy_installed)||(!joy_present)) return 0;

	if ( btn >= MAX_BUTTONS ) return 0;

	count = joystick.buttons[btn].upcount;
	joystick.buttons[btn].upcount = 0;

	return count;
}

int joy_get_button_down_cnt( int btn ) 
{
	int count;

	if ((!joy_installed)||(!joy_present)) return 0;
	if ( btn >= MAX_BUTTONS ) return 0;

        joy_get_btns();

	count = joystick.buttons[btn].downcount;
	joystick.buttons[btn].downcount = 0;

	return count;
}

fix joy_get_button_down_time( int btn ) 
{
        fix count;

        if ((!joy_installed)||(!joy_present)) return 0;
	if ( btn >= MAX_BUTTONS ) return 0;

        joy_get_btns();

        if (joystick.buttons[btn].state) {
                count = timer_get_fixed_seconds() - joystick.buttons[btn].timedown;
                joystick.buttons[btn].timedown = 0;
        } else count = 0;
        
        return count;
}

//end changed section - OE


void joy_get_btn_up_cnt( int *btn0, int *btn1 ) 
{
	if ((!joy_installed)||(!joy_present)) { *btn0=*btn1=0; return; }

	*btn0 = joystick.buttons[0].upcount;
	joystick.buttons[0].upcount = 0;
	*btn1 = joystick.buttons[1].upcount;
	joystick.buttons[1].upcount = 0;
}

void joy_set_btn_values( int btn, int state, fix timedown, int downcount, int upcount )
{
	joystick.buttons[btn].ignore = 1;
	joystick.buttons[btn].state = state;
	joystick.buttons[btn].timedown = fixmuldiv( timedown, 1193180, 65536 );
	joystick.buttons[btn].downcount = downcount;
	joystick.buttons[btn].upcount = upcount;
}

void joy_poll()
{
}

