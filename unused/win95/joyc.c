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


#pragma off (unreferenced)
static char rcsid[] = "$Id: joyc.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "winapp.h"
#include "types.h"
#include "mono.h"
#include "joy.h"
#include "tactile.h"
#include "winregs.h"

#include "args.h"

//	Controls
//#include "win\cyberimp.h"
//#include "spw_inp.h"

#define WIN_TACTILE_ON

#define JOY_READ_BUTTONS 
#define MAX_BUTTONS 20
#define JOY_POLL_RATE	75	// 100 ms.


//	Data Structures ============================================================

typedef struct Button_info {
	ubyte		ignore;
	ubyte		state;
	ubyte		last_state;
	unsigned	timedown;
	ubyte		downcount;
	ubyte		upcount;
} Button_info;

typedef struct Joy_info {
	int			joyid;
	ubyte			present_mask;
	int			max_timer;
	int			read_count;
	ubyte			last_value;
	Button_info	buttons[MAX_BUTTONS];
	int			axis_min[7];
	int			axis_center[7];
	int			axis_max[7];
	int			has_pov;
	int			threshold;
	int			raw_x1, raw_y1, raw_z1, raw_r1, raw_u1, raw_v1, pov;
} Joy_info;

LRESULT joy_handler_win(HWND hWnd, UINT joymsg, UINT wParam, LPARAM lParam);


//	Globals ====================================================================

Joy_info joystick;
int SpecialDevice = 0;

char SBall_installed = 0;
char joy_installed = 0;
char joy_present = 0;


//	Extra Controls structures
//SPW_Control SBall_Control;						// Space Ball Interface

static JOYCAPS	WinJoyCaps;
static BOOL		CHStickHack=0;


//	Functions ==================================================================

/*	joy_init
		Reset joystick information and request joystick notification from
		Windows...  Calibration shouldn't be a problem since Windows 95 takes
		care of that.  Also attaches a window to the joystick.
*/
int joy_init(int joy, int spjoy)
{
	int i;
	//int num_devs;

	atexit(joy_close);

//	Reset Joystick information.
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

	if (spjoy) {
	// Special joystick interface initialize.
		int port;

		SpecialDevice = 0;
		joy_present = 0;

	//	if (FindArg("-CyberImpact")) 
	//		SpecialDevice = TACTILE_CYBERNET;			

		if (i = FindArg("-Iforce")) {
			SpecialDevice = TACTILE_IMMERSION;
			if (atoi(Args[i+1]) != 0) port = atoi(Args[i+1]);
		}
			
		if (SpecialDevice) 
			mprintf((0, "Using Special Joystick!\n"));
		
		switch(SpecialDevice)
		{
		#ifdef WIN_TACTILE_ON
			case TACTILE_CYBERNET:
				TactileStick = TACTILE_CYBERNET;
				if (!Tactile_open(0)) joy_present = 0;
				else joy_present = 1;
				break;

			case TACTILE_IMMERSION:
				TactileStick = TACTILE_IMMERSION;
				if (!Tactile_open(port)) joy_present = 0;
				else joy_present = 1;
				break;
		#endif
		}
		if (!joy_present) SpecialDevice = 0;
	}	
	else 
		joy_present = (char)joyGetNumDevs();
		

	return joy_present;
}


//	joy_close
//		closes to joystick by releasing it from the application window

void joy_close()
{
	if (joy_installed) {
	
		switch (SpecialDevice)
		{
			case TACTILE_CYBERNET:
				SpecialDevice = 0;
				break;
		
			case TACTILE_IMMERSION:
				SpecialDevice = 0;	
				break;
		}
		joyReleaseCapture(joystick.joyid);
		joy_installed = 0;
	}
}
		

//	joy_get/set_cal_vals
//		functions that access all of the joystick axis'.  
//		An array of at least four elements must be passed to each function

void joy_get_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
   int i;

   for (i=0; i < 7; i++)
	{
      axis_min[i] = joystick.axis_min[i];
      axis_center[i] = joystick.axis_center[i];
      axis_max[i] = joystick.axis_max[i];
   }
}


void joy_set_cal_vals(int *axis_min, int *axis_center, int *axis_max)
{
	int i;
	
   for (i=0; i < 7; i++)     
	{
      joystick.axis_min[i] = axis_min[i];
      joystick.axis_center[i] = axis_center[i];
      joystick.axis_max[i] = axis_max[i];
   }
}


//	joy_get_present_mask
//		returns the joystick axis' available to be used and accessed

ubyte joy_get_present_mask()	
{
	return joystick.present_mask;
}



//	joy_flush
//		resets joystick button parameters

void joy_flush()	
{
   int i;

   for (i=0; i<MAX_BUTTONS; i++ )   {
      joystick.buttons[i].ignore = 0;
      joystick.buttons[i].state = 0;
      joystick.buttons[i].timedown = 0;
      joystick.buttons[i].downcount = 0;
      joystick.buttons[i].upcount = 0;
   }
}


//	joy_read_raw_buttons
//		returns the status of the buttons at that moment.

ubyte joy_read_raw_buttons()	
{
	JOYINFOEX	joy;
	ubyte			value=0;

	if (!joy_present) { return 0; }
	
	memset(&joy, 0, sizeof(joy));
	joy.dwSize = sizeof(joy);
	joy.dwFlags = JOY_RETURNBUTTONS | JOY_USEDEADZONE;
	joyGetPosEx(joystick.joyid, &joy);

//	For now we support just four buttons
	value = value | ((ubyte)joy.dwButtons & JOY_BUTTON1);
	value = value | ((ubyte)joy.dwButtons & JOY_BUTTON2);
	value = value | ((ubyte)joy.dwButtons & JOY_BUTTON3);
	value = value | ((ubyte)joy.dwButtons & JOY_BUTTON4);
	
	return value;
}


ubyte joystick_read_raw_axis( ubyte mask, int * axis )
{
	JOYINFOEX	joy;
	ubyte read_masks = 0;
	int i;

	if (!joy_installed) return 0;
	if (!joy_present) return 0;

	for (i = 0; i < 7; i++)
		axis[i] = 0;

	axis[3] = JOY_POVCENTERED;

	switch (SpecialDevice)
	{
		case TACTILE_CYBERNET:
//@@			CyberImpact_ReadRawValues(mask, axis);
//@@			joystick.raw_x1 = axis[0];
//@@			joystick.raw_y1 = axis[1];
//@@			joystick.raw_z1 = axis[2];
//@@			joystick.raw_r1 = axis[4];
//@@			joystick.raw_u1 = axis[5];
//@@			joystick.raw_v1 = axis[6];
//@@			joystick.pov = axis[3];
//@@			break;

		case TACTILE_IMMERSION:
		#ifdef WIN_TACTILE_ON
			IForce_ReadRawValues(axis);
		#endif
			joystick.raw_x1 = axis[0];
			joystick.raw_y1 = axis[1];
			joystick.raw_z1 = axis[2];
			joystick.raw_r1 = axis[4];
			joystick.raw_u1 = axis[5];
			joystick.raw_v1 = axis[6];
			joystick.pov = axis[3];
			break;

		default:
			memset(&joy, 0, sizeof(joy));
			joy.dwSize = sizeof(joy);
			joy.dwFlags = JOY_RETURNALL | JOY_USEDEADZONE;
			joyGetPosEx(joystick.joyid, &joy);
			joystick.raw_x1 = joy.dwXpos;
			joystick.raw_y1 = joy.dwYpos;
			joystick.raw_z1 = joy.dwZpos;
			joystick.raw_r1 = joy.dwRpos;
			joystick.raw_u1 = joy.dwUpos;
			joystick.raw_v1 = joy.dwVpos;
			joystick.pov = joy.dwPOV;
	}

	mask &= joystick.present_mask;
	if (mask == 0) return 0;				// Don't read if no stick present

	if (mask & JOY_1_X_AXIS) {
		axis[0] = joystick.raw_x1;
		read_masks |= JOY_1_X_AXIS;
	}
	if (mask & JOY_1_Y_AXIS) {
		axis[1] = joystick.raw_y1;
		read_masks |= JOY_1_Y_AXIS;
	}
	if (mask & JOY_1_Z_AXIS) {
		axis[2] = joystick.raw_z1;
		read_masks |= JOY_1_Z_AXIS;
	}
	if (mask & JOY_1_R_AXIS) {
		axis[4] = joystick.raw_r1;
		read_masks |= JOY_1_R_AXIS;
	}
	if (mask & JOY_1_U_AXIS) {
		axis[5] = joystick.raw_u1;
		read_masks |= JOY_1_U_AXIS;
	}
	if (mask & JOY_1_V_AXIS) {
		axis[6] = joystick.raw_v1;
		read_masks |= JOY_1_V_AXIS;
	}
	if (mask & JOY_1_POV) {
		axis[3] = joystick.pov;
		read_masks |= JOY_1_POV;
	}

	return read_masks;
}


void joy_set_ul()	
{
	joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS+JOY_EXT_AXIS, joystick.axis_min );
	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}


void joy_set_lr()	
{
	joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS+JOY_EXT_AXIS, joystick.axis_max );
	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}


void joy_set_cen() 
{
	switch (SpecialDevice)
	{
		case TACTILE_CYBERNET:
		//@@	CyberImpact_CalibCenter();
			break;
	}

	joystick.present_mask = joystick_read_raw_axis( JOY_ALL_AXIS+JOY_EXT_AXIS, joystick.axis_center );
	if ( joystick.present_mask & 3 )
		joy_present = 1;
	else
		joy_present = 0;
}


void joy_set_cen_fake(int channel)	
{
//	Do we need this???
}


int joy_get_scaled_reading( int raw, int axn )	
{
	int x, d;

	// Make sure it's calibrated properly.
	if ( joystick.axis_center[axn] - joystick.axis_min[axn] < 128 ) return 0;
	if ( joystick.axis_max[axn] - joystick.axis_center[axn] < 128 ) return 0;

	raw -= joystick.axis_center[axn];

	if ( raw < 0 )	{
		d = joystick.axis_center[axn]-joystick.axis_min[axn];
	} else {
		d = joystick.axis_max[axn]-joystick.axis_center[axn];
	}

	if ( d )
		x = (raw << 7) / d;
	else 
		x = 0;

	if ( x < -128 ) x = -128;
	if ( x > 127 ) x = 127;

	return x;

//@@	int x;
//@@
//@@	if ((joystick.axis_max[axn] - joystick.axis_min[axn]) == 0) return 0;
//@@	if (axn == 3) return raw;
//@@	
//@@	x = (256 * raw)/(joystick.axis_max[axn]-joystick.axis_min[axn]);
//@@	x=x- 128;
//@@
//@@	return x;
}	


void joy_get_pos( int *x, int *y )	
{
	ubyte flags;
	int axis[4];


	if (!joy_installed || !joy_present) { *x = *y = 0; return; }

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
	int raw_axis[7];	


	if ((!joy_installed)||(!joy_present)) { 
		axis[0] = 0; axis[1] = 0;
		axis[2] = 0; axis[3] = 0;
		axis[4] = 0; axis[5] = 0;
		axis[6] = 0; 
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

	if ( flags & JOY_1_Z_AXIS )
		axis[2] = joy_get_scaled_reading( raw_axis[2], 2 );
	else
		axis[2] = 0;

	if ( flags & JOY_1_R_AXIS )
		axis[4] = joy_get_scaled_reading( raw_axis[4], 4 );
	else
		axis[4] = 0;

	if ( flags & JOY_1_U_AXIS )
		axis[5] = joy_get_scaled_reading( raw_axis[5], 5 );
	else
		axis[5] = 0;

	if ( flags & JOY_1_V_AXIS )
		axis[6] = joy_get_scaled_reading( raw_axis[6], 6 );
	else
		axis[6] = 0;

	if ( flags & JOY_1_POV )
		axis[3] = joy_get_scaled_reading( raw_axis[3], 3 );
	else
		axis[3] = 0;

	return flags;
}


int joy_get_btns()	
{
	if ((!joy_installed)||(!joy_present)) return 0;
	return joy_read_raw_buttons();
}


void joy_get_btn_down_cnt( int *btn0, int *btn1 ) 
{
	if ((!joy_installed)||(!joy_present)) { *btn0=*btn1=0; return; }
		
	*btn0 = joystick.buttons[0].downcount;
	joystick.buttons[0].downcount = 0;
	*btn1 = joystick.buttons[1].downcount;
	joystick.buttons[1].downcount = 0;
}


int joy_get_button_state( int btn )	
{
	int count;

	if ((!joy_installed)||(!joy_present)) return 0;

	if ( btn >= MAX_BUTTONS ) return 0;

	count = joystick.buttons[btn].state;
	
	return  count;
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

	count = joystick.buttons[btn].downcount;
	joystick.buttons[btn].downcount = 0;

	return count;
}

	
fix joy_get_button_down_time( int btn ) 
{
	fix count;

	if ((!joy_installed)||(!joy_present)) return 0;
	if ( btn >= MAX_BUTTONS ) return 0;

	count = joystick.buttons[btn].timedown;
	joystick.buttons[btn].timedown = 0;

	return count;		//fixdiv(count, 1000);
}


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
	joystick.buttons[btn].timedown = timedown;	//fixmul(timedown,1000);	//convert to miliseconds
	joystick.buttons[btn].downcount = downcount;
	joystick.buttons[btn].upcount = upcount;
}


//	Windows 95 Custom joystick configuration.
//		These are 'low level' routines to get information about a joystick
//		in Windows 95.
//		Note: spjoy is for 'unique' joysticks.

int joy95_init_stick(int joy, int spjoy)
{
	UINT 			joyid;
	MMRESULT 	mmresult;
	char 			joyname[256];

	if (!joy_installed) 		  
		return 0;

	switch (SpecialDevice)
	{
		case TACTILE_CYBERNET:
		//@@	joystick.present_mask = JOY_1_X_AXIS | JOY_1_Y_AXIS | JOY_1_Z_AXIS | JOY_1_POV;
		//@@	joystick.has_pov = 1;
			break;

		case TACTILE_IMMERSION:
			mprintf((0, "Joystick name= I-Force compatible device.\n"));
			break;
	}

//	if (spjoy) return 1;

	joystick.joyid = 0;

	if (joy == 1) joystick.joyid = joyid = JOYSTICKID1;
	else 
		return 0;

	mmresult = joyGetDevCaps(joyid, &WinJoyCaps, sizeof(WinJoyCaps));
	if (mmresult != JOYERR_NOERROR) {
		mprintf((1, "Attempt to get Joystick %d caps failed.\n", joy));
 		return 0;
	}
   mprintf ((0,"Joystick name=%s\n",WinJoyCaps.szPname));

//	Tell our Window App. about this joystick.
	joySetThreshold(joyid, WinJoyCaps.wXmax/256);
	mmresult = joySetCapture(GetLibraryWindow(), 
						joyid, 
						JOY_POLL_RATE, 
						FALSE);

	if (mmresult != JOYERR_NOERROR) {
 		mprintf((1, "Unable to capture joystick %d. Error=%d\n", joy,mmresult));
		return 0;
	}

//	Get raw axis' min and max.
	joystick.threshold 			= WinJoyCaps.wXmax/256;
	joystick.max_timer 			= WinJoyCaps.wPeriodMax;

	joystick.threshold 			= WinJoyCaps.wXmax/256;
	joystick.max_timer 			= WinJoyCaps.wPeriodMax;
	joystick.axis_min[0]			= WinJoyCaps.wXmin;
	joystick.axis_min[1]			= WinJoyCaps.wYmin;
	joystick.axis_min[2]			= WinJoyCaps.wZmin;
	joystick.axis_min[4]			= WinJoyCaps.wRmin;
	joystick.axis_min[5]			= WinJoyCaps.wUmin;
	joystick.axis_min[6]			= WinJoyCaps.wVmin;
	joystick.axis_max[0]			= WinJoyCaps.wXmax;
	joystick.axis_max[1]			= WinJoyCaps.wYmax;
	joystick.axis_max[2]			= WinJoyCaps.wZmax;
	joystick.axis_max[4]			= WinJoyCaps.wRmax;
	joystick.axis_max[5]			= WinJoyCaps.wUmax;
	joystick.axis_max[6]			= WinJoyCaps.wVmax;
	joystick.axis_center[0]			= (WinJoyCaps.wXmax-WinJoyCaps.wXmin)/2;
	joystick.axis_center[1]			= (WinJoyCaps.wYmax-WinJoyCaps.wYmin)/2;
	joystick.axis_center[2]			= (WinJoyCaps.wZmax-WinJoyCaps.wZmin)/2;
	joystick.axis_center[4]			= (WinJoyCaps.wRmax-WinJoyCaps.wRmin)/2;
	joystick.axis_center[5]			= (WinJoyCaps.wUmax-WinJoyCaps.wUmin)/2;
	joystick.axis_center[6]			= (WinJoyCaps.wVmax-WinJoyCaps.wVmin)/2;

	joystick.present_mask = JOY_1_X_AXIS | JOY_1_Y_AXIS;
	
	if (WinJoyCaps.wCaps & JOYCAPS_HASPOV) {
		joystick.has_pov = 1;
		joystick.present_mask |= JOY_1_POV;
	}
	else joystick.has_pov = 0;

	if (WinJoyCaps.wCaps & JOYCAPS_HASZ) 
		joystick.present_mask |= JOY_1_Z_AXIS;
	if (WinJoyCaps.wCaps & JOYCAPS_HASR) 
		joystick.present_mask |= JOY_1_R_AXIS;
	if (WinJoyCaps.wCaps & JOYCAPS_HASU)
		joystick.present_mask |= JOY_1_U_AXIS;
	if (WinJoyCaps.wCaps & JOYCAPS_HASV) 
		joystick.present_mask |= JOY_1_V_AXIS;

	joy95_get_name(JOYSTICKID1, joyname, 255);
	if (!strcmpi(joyname, "CH Flightstick Pro") || FindArg("-ordinaljoy")) {
		CHStickHack = 1;
	}
	else CHStickHack = 0;

			
	return 1;
}

void joy95_get_name(int joyid, char *name, int namesize) 
{
	JOYCAPS jc;
	MMRESULT res;
	//HKEY hkey;
	//long lres;
	char regpath[256];
	registry_handle *rhandle;
	char reglabel[32];

	name[0] = 0;

	switch(SpecialDevice) 
	{
		case TACTILE_IMMERSION:
			strcpy(name, "I-Force Force-Feedback");
			return;
	}

	res = joyGetDevCaps(joyid, &jc, sizeof(jc));
	if (res != JOYERR_NOERROR) {
		strcpy(name, "NO JOYSTICK DETECTED");
		return;
	}
	
// we have the reg key!
	registry_setpath(HKEY_LOCAL_MACHINE);
	strcpy(regpath, "System\\CurrentControlSet\\control\\MediaResources\\joystick\\");
	strcat(regpath, jc.szRegKey);
	strcat(regpath, "\\CurrentJoystickSettings");
	sprintf(reglabel, "Joystick%dOEMName", joyid+1);

	if ((rhandle = registry_open(regpath))) {
		if (!registry_getstring(rhandle, reglabel, name, namesize)) {
			registry_close(rhandle);
			strcpy(name, "JOYSTICK");
			return;
		}
		registry_close(rhandle);
	
	// have reg entry for full name
		strcpy(regpath, "System\\CurrentControlSet\\control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\");
		strcat(regpath, name);
	
		if ((rhandle = registry_open(regpath))) {
			if (!registry_getstring(rhandle, "OEMName", name, namesize)) {
				registry_close(rhandle);
				strcpy(name, "JOYSTICK");
				return;
			}
			registry_close(rhandle);
		}
		else strcpy(name, "JOYSTICK"); 
	}
	else strcpy(name, "JOYSTICK");
}
		  

	
/*	joy_handler_win
		This function takes the place of the joy_handler function for dos.  
		The main window procedure will call this function whenever a joystick message
		is transmitted.
*/
LRESULT joy_handler_win(HWND hWnd, UINT joymsg, UINT wParam, LPARAM lParam)
{
	DWORD interval_time;
	DWORD new_time;
	static DWORD old_time = 0xffffffff;
	
	Button_info *button;
	int state=0;
	DWORD value=0;
	int i;

//	take care of first time initialization
	if (!joy_installed) return 1;
	if (old_time == 0xffffffff) {
		old_time = timer_get_fixed_seconds();
		return 0;
	}

//	read and translate buttons 0-3
	switch (joymsg)
	{
		JOYINFOEX	joy;

		case MM_JOY1MOVE:
		//	For now we support just four buttons
			memset(&joy, 0, sizeof(joy));
			joy.dwSize = sizeof(joy);
			joy.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNPOV;
			joyGetPosEx(joystick.joyid, &joy);

			if (CHStickHack) {
				if (joy.dwPOV == JOY_POVCENTERED && joy.dwButtons > 0) {		  
					value = (1 << (joy.dwButtons-1));
				}
				else value = 0;
				mprintf_at((0, 2, 32, "RAW:%08x OUR:%08x\n", joy.dwButtons, value));
			}	
			else {
				value = joy.dwButtons;
			}
			break;
	}

	new_time = timer_get_fixed_seconds();
	if (new_time < old_time) old_time = abs(new_time - old_time);
	interval_time = new_time - old_time;

	for (i = 0; i < MAX_BUTTONS; i++)
	{
	//	Check status of each button and translate information to the button
	// structure.
		button = &joystick.buttons[i];
		
		if (!button->ignore) {
			if ( i < (MAX_BUTTONS-4) ) state = (value >> i) & 1;
			else state = 0;

			if ( button->last_state == state )	{
				if (state) {
					button->timedown += interval_time;	//interval_ms;
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

	old_time = new_time;
	return 0;
}		


void joy_stop_poll()
{
	joyReleaseCapture(joystick.joyid);
}


void joy_start_poll()
{
	joySetCapture(GetLibraryWindow(), 
						joystick.joyid, 
						JOY_POLL_RATE, 
						FALSE);
}


//	OBSOLETE!!!!

//	joy_get/set_timer_rate
//		these functions are currently implemented for compatability with 
//		DOS Descent2, but may be unecessary for now.

void joy_set_timer_rate(int max_value )	
{
//	No real effect, since Joystick runs on its own timer in Windows, for now.
}


int joy_get_timer_rate()	
{
	return joystick.max_timer;
}

void joy_handler(int ticks_this_time)	
{
//	replaced by joy_handler_win for Windows
}

void joy_set_slow_reading(int flag)
{
//	No need to do this.  In Windows, there are only slow readings.
}

void joy_poll()
{

}



