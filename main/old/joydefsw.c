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
static char rcsid[] = "$Id: joydefsw.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include "desw.h"
#include <mmsystem.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>

#include "mono.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "victor.h"
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "gamefont.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"
#include "screens.h"
#include "config.h"

//	Extra control stuff

// #include "cyberimp.h"


//##int joydefs_calibrate_flag = 0;
int joydefs_max_joysticks = 0;
int joydefs_num_joysticks = 0;
int Joystick_calibrating = 0;


extern int WriteConfigFile();	
extern void kconfig_set_fcs_button(int btn, int button);


WinJoystickDesc WinJoyDesc;

//	Windows 95:
//		Calibration and Menus because of automatic joystick detection
//		and 6 axis' (XYZ-RUV)

extern ubyte default_kconfig_settings[][MAX_CONTROLS];


void win95_autodetect_joystick()
{
	JOYCAPS jc;

//	Set default windows joystick settings.
//	AUTODETECT !!!
	joyGetDevCaps(JOYSTICKID1, &jc, sizeof(jc));

//	Do buttons first
	default_kconfig_settings[CONTROL_WINJOYSTICK][1] = 0xff;
	default_kconfig_settings[CONTROL_WINJOYSTICK][4] = 0xff;
	default_kconfig_settings[CONTROL_WINJOYSTICK][26] = 0xff;

	default_kconfig_settings[CONTROL_WINJOYSTICK][0] = 0x0;
	if (jc.wMaxButtons > 1) 
		default_kconfig_settings[CONTROL_WINJOYSTICK][1] = 0x1;
	if (jc.wMaxButtons > 2) 
		default_kconfig_settings[CONTROL_WINJOYSTICK][4] = 0x2;
	if (jc.wMaxButtons > 3) 
		default_kconfig_settings[CONTROL_WINJOYSTICK][26] = 0x3;

//	Do Hat!
	default_kconfig_settings[CONTROL_WINJOYSTICK][6] = 0xff;
	default_kconfig_settings[CONTROL_WINJOYSTICK][7] = 0xff;
	default_kconfig_settings[CONTROL_WINJOYSTICK][8] = 0xff;
	default_kconfig_settings[CONTROL_WINJOYSTICK][9] = 0xff;

	if (jc.wCaps & JOYCAPS_HASPOV) {
		default_kconfig_settings[CONTROL_WINJOYSTICK][6] = 0x10;
		default_kconfig_settings[CONTROL_WINJOYSTICK][7] = 0x12;
		default_kconfig_settings[CONTROL_WINJOYSTICK][8] = 0x13;
		default_kconfig_settings[CONTROL_WINJOYSTICK][9] = 0x11;
	} 

//	Do Z-Axis (Throttle)

//	Do rudder? (R-AXIS) 
		
}


int win95_controls_init()
{
	//int i = 0;
	int spjoy = 0;

	if (FindArg("-SpecialDevice")) spjoy = 1;

	joydefs_max_joysticks = joy_init(0,spjoy);
	if (!joy95_init_stick(1, spjoy)) return 0;

//@@	for (i = 2; i <= joydefs_max_joysticks; i++)
//@@		if (!joy95_init_stick(i,0)) {
//@@			joydefs_max_joysticks = i-1;
//@@			break;
//@@		}

	joy_handler_win(0,0,0,0);
	win95_autodetect_joystick();

	return 1;
}


void joy_delay()
{
	stop_time();
	WinDelay(250);			
	joy_flush();
	start_time();
}

void dump_axis_vals(int *t_vals)
{
	mprintf_at((0, 2, 32, "X:%6d\n", t_vals[0]));
	mprintf_at((0, 3, 32, "Y:%6d\n", t_vals[1]));
	mprintf_at((0, 4, 32, "Z:%6d\n", t_vals[2]));
	mprintf_at((0, 5, 32, "R:%6d\n", t_vals[4]));
	mprintf_at((0, 6, 32, "U:%6d\n", t_vals[5]));
	mprintf_at((0, 7, 32, "V:%6d\n", t_vals[6]));
}


//	MAIN CALIBRATION ROUTINES
//	----------------------------------------------------------------------------

void joydefs_calibrate(void)
{
	DWORD dwval;
	BOOL flag;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char name[80];
	char oldname[80];
	int j;
	int old_mode= -1;
	
	Joystick_calibrating = 1;

	if (GRMODEINFO(modex)) {
		old_mode = Screen_mode;
		set_screen_mode(SCREEN_MENU);
	}
	palette_save();
	apply_modified_palette();
	reset_palette_add();

	gr_palette_load(gr_palette);

//	Call calibration Applet
	joy95_get_name(JOYSTICKID1, oldname, 79);

	logentry("Closing joystick for control panel init.\n");
	joy_close();							// Close down joystick polling

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	flag = CreateProcess(
			NULL,
			"rundll32.exe shell32.dll,Control_RunDLL joy.cpl",
			NULL, NULL,
			FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
			&si, &pi);			
	if (!flag) {
		nm_messagebox( NULL, 1, TXT_OK, "UNABLE TO INITIATE\nJOYSTICK CONTROL PANEL");
	}
	else {
		dwval = WaitForInputIdle(pi.hProcess, INFINITE);
		mprintf((1, "JOY.CPL initiated?\n"));
		WinDelayIdle();							// Clear out messages.

		while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0)
		{
			if (multi_menu_poll()==-1)	{
				TerminateProcess(pi.hProcess,0);
				Sleep(100); 
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				break;
			}
		}

		mprintf((1, "JOY.CPL process finished?\n"));
		ShowWindow(_hAppWnd, SW_MAXIMIZE);
		WinDelayIdle();
		SetForegroundWindow(_hAppWnd);
		WinDelayIdle();
		_RedrawScreen = FALSE;
	}

	if (win95_controls_init())					// Reinitializing joystick polling.
		logentry("Reinitialized joystick with new settings.\n");
	else 
		logentry("Failed to reinitialize joystick.\n");

//	Set kconfig settings to updated vals.
	joy95_get_name(JOYSTICKID1, name, 79);

	if (Config_control_type == CONTROL_WINJOYSTICK) {
		if (strcmp(name, oldname)) {
			for (j=0; j<MAX_CONTROLS; j++ )
				kconfig_settings[CONTROL_WINJOYSTICK][j] = default_kconfig_settings[CONTROL_WINJOYSTICK][j];
			kc_set_controls();
		}
	}

//@@	joydefs_calibrate2();
	
	reset_cockpit();

	palette_restore();

	if (old_mode != -1)	{
		set_screen_mode(old_mode);
	}
	
	Joystick_calibrating = 0;
}


//@@//	We should make our own windowed calibrator
//@@void joydefs_calibrate2()
//@@{
//@@	ubyte masks;
//@@	char calmsg[6][32];
//@@
//@@	int org_axis_min[7];
//@@	int org_axis_center[7];
//@@	int org_axis_max[7];
//@@
//@@	int axis_min[7] = { 0, 0, 0, 0, 0, 0, 0 };
//@@	int axis_cen[7] = { 0, 0, 0, 0, 0, 0, 0 };
//@@	int axis_max[7] = { 0, 0, 0, 0, 0, 0, 0 };
//@@
//@@	int t_vals[7];
//@@
//@@	char title[50];
//@@	char text[50];
//@@	int res;
//@@	int nsticks = 0;
//@@
//@@//	Get Current Calibration stuff
//@@	joydefs_calibrate_flag = 0;
//@@
//@@	joy_get_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@
//@@	joy_set_cen();
//@@
//@@	masks = joystick_read_raw_axis( JOY_ALL_AXIS+JOY_EXT_AXIS, t_vals );
//@@
//@@	if (!joy_present)	{
//@@		nm_messagebox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
//@@		return;
//@@	}
//@@	
//@@	masks = joy_get_present_mask();
//@@
//@@	nsticks = 1;
//@@
//@@	res = joy_calibrate_xy(&axis_min[0], &axis_cen[0],	&axis_max[0],
//@@		  							&axis_min[1], &axis_cen[1], &axis_max[1]);
//@@	
//@@	if (!res) {
//@@		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@		return;
//@@	}
//@@
//@@	strcpy(calmsg[0], "MOVE JOYSTICK UNTIL BAR REACHES");
//@@	strcpy(calmsg[1], "TOP AND PRESS BUTTON");
//@@	strcpy(calmsg[2], "MOVE JOYSTICK UNTIL BAR REACHES");
//@@	strcpy(calmsg[3], "BOTTOM AND PRESS BUTTON");
//@@	strcpy(calmsg[4], "RELEASE JOYSTICK");
//@@	strcpy(calmsg[5], "AND PRESS BUTTON");
//@@
//@@	mprintf((0, "Masks: %x\n", masks));
//@@
//@@	if (masks&JOY_1_Z_AXIS) {
//@@		res = joy_calibrate_bar(2, "CALIBRATE Z-AXIS", calmsg,
//@@							&axis_min[2], &axis_cen[2], &axis_max[2]);
//@@		if (!res) {
//@@			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@			return;
//@@		}
//@@  	}
//@@
//@@	if (masks&JOY_1_R_AXIS) {
//@@		res = joy_calibrate_bar(4, "CALIBRATE RUDDER", calmsg,
//@@							&axis_min[4], &axis_cen[4], &axis_max[4]);
//@@		if (!res) {
//@@			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@			return;
//@@		}
//@@  	}
//@@
//@@	if (masks&JOY_1_U_AXIS) {
//@@		res = joy_calibrate_bar(5, "CALIBRATE U-AXIS", calmsg,
//@@							&axis_min[5], &axis_cen[5], &axis_max[5]);
//@@		if (!res) {
//@@			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@			return;
//@@		}
//@@  	}
//@@
//@@	if (masks&JOY_1_V_AXIS) {
//@@		res = joy_calibrate_bar(6, "CALIBRATE V-AXIS", calmsg,
//@@							&axis_min[6], &axis_cen[6], &axis_max[6]);
//@@		if (!res) {
//@@			joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
//@@			return;
//@@		}
//@@  	}
//@@
//@@
//@@	joy_set_cal_vals(axis_min, axis_cen, axis_max);
//@@
//@@	WriteConfigFile();	
//@@}


//	Joydefs button detection
//		This function will do Windows Joystick Button stuff
//		may emulate Thrustmaster FCS, Flightstick Pro, etc.
		
int joydefsw_do_button()
{
	int axis[7];	
	int code=255;
	int i;

	joystick_read_raw_axis(JOY_ALL_AXIS, axis);	

	switch (Config_control_type) 
	{
		case CONTROL_WINJOYSTICK:
			code = joydefsw_do_winjoybutton(axis);
			break;
	}

//	Get standard buttons
	for (i=0; i<16; i++ )	{
		if ( joy_get_button_state(i) )
			code = i;
	}

	return code;			  
}


int joydefsw_do_winjoybutton(int *axis)
{
//	Do POV detection first.  We should use the same
// procedure as the DOS version so that we can emulate the
// Thrustmaster selections (to keep Player files the same between
//	versions.  (19,15,11,7,0)
	int button;

	if (axis[3] == JOY_POVBACKWARD) 
		button = 17;
	else if (axis[3] == JOY_POVFORWARD)
		button = 19;
	else if (axis[3] == JOY_POVLEFT)
		button = 16;
	else if (axis[3] == JOY_POVRIGHT)
		button = 18;
	else if (axis[3] == JOY_POVCENTERED) 
		button = 0;

	kconfig_set_fcs_button(19,button);
	kconfig_set_fcs_button(18,button);
	kconfig_set_fcs_button(17,button);
	kconfig_set_fcs_button(16, button);

	if (button == 0) button = 255;

	return button;
} 



//	Joystick Menu 
//		We want to keep the config files the same for the Windows/DOS
//		versions.  But we also autodetect joystick stuff.
//		Find some way to keep this easier than the DOS version without screwing 
//		up configuration files.

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	//int i;
	int oc_type = Config_control_type;

	nitems = nitems;
	last_key = last_key;
	citem = citem;		

	if (items[0].value) Config_control_type = CONTROL_NONE;
	else if (items[1].value) Config_control_type = CONTROL_MOUSE;
	else if (items[2].value) Config_control_type = CONTROL_WINJOYSTICK;

//@@	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS ) )	{
//@@			nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
//@@		}

	if (oc_type != Config_control_type) {
		//##switch (Config_control_type) {
		//##	case 	CONTROL_WINJOYSTICK:
		//##		joydefs_calibrate_flag = 1;
		//##}
		kc_set_controls();
	}

}

extern ubyte kc_use_external_control;
extern ubyte kc_enable_external_control;
extern ubyte *kc_external_name;

void joydefs_config()
{
	int i, old_masks, masks,nitems;
	newmenu_item m[13];
	int i1=11;
	char xtext[92];
	char joytext[128];
	int no_joystick_option = 0;

	//##joydefs_calibrate_flag = 0;

	joy95_get_name(JOYSTICKID1, xtext, 127);
	if (!strcmpi(xtext, "NO JOYSTICK DETECTED") && (Config_control_type == CONTROL_WINJOYSTICK))	{
		Config_control_type = CONTROL_NONE;
		no_joystick_option = 1;
	}
	else no_joystick_option = 0;
	nm_wrap_text(joytext, xtext, 20);
	
	do {
		nitems=10;
		m[0].type = NM_TYPE_RADIO; m[0].text = CONTROL_TEXT(0); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = CONTROL_TEXT(5); m[1].value = 0; m[1].group = 0;
	
		if (no_joystick_option) {
			m[2].type = NM_TYPE_TEXT; m[2].text = joytext; m[2].value = 0; m[2].group = 0;
		} else {
			m[2].type = NM_TYPE_RADIO; m[2].text = joytext; m[2].value = 0; m[2].group = 0;
		}

		m[3].type = NM_TYPE_TEXT;  m[3].text="";
		m[4].type = NM_TYPE_MENU; m[4].text=TXT_CUST_ABOVE;
		m[5].type = NM_TYPE_TEXT;  m[5].text="";
		m[6].type = NM_TYPE_SLIDER; m[6].text=TXT_JOYS_SENSITIVITY; m[6].value=Config_joystick_sensitivity; m[6].min_value =0; m[6].max_value = 8;
		m[7].type = NM_TYPE_TEXT;   m[7].text="";
		m[8].type = NM_TYPE_TEXT;   m[8].text="";
		m[9].type = NM_TYPE_MENU; m[9].text=TXT_CUST_KEYBOARD;

		if (Config_control_type == CONTROL_WINJOYSTICK) m[2].value = 1;
		else if (Config_control_type == CONTROL_MOUSE) m[1].value = 1;
		else if (Config_control_type == CONTROL_NONE) m[0].value = 1;
		else m[0].value = 1;   

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );
		Config_joystick_sensitivity = m[6].value;

		switch(i1)	{
		case 4: {
				old_masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						old_masks |= (1<<i);
				}
				if ( Config_control_type==0 )
					Config_control_type=0;
				else if ( Config_control_type < CONTROL_MOUSE ) 
					kconfig(1, CONTROL_TEXT(Config_control_type) ); 
				else if ( Config_control_type == CONTROL_WINJOYSTICK) 
					kconfig(3, CONTROL_TEXT(Config_control_type) );
				else 
					kconfig(2, CONTROL_TEXT(Config_control_type) ); 

				masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						masks |= (1<<i);
				}

				//##switch (Config_control_type) {
				//##case 	CONTROL_WINJOYSTICK:
				//##	{
				//##		for (i=0; i<4; i++ )	{
				//##			if ( (masks&(1<<i)) && (!(old_masks&(1<<i))))
				//##				joydefs_calibrate_flag = 1;
				//##		}
				//##	}
				//##	break;
				//##}
			}
			break;
		case 9: 
			kconfig(0, TXT_KEYBOARD); 
			break;
		} 

	} while(i1 > -1);

//	No calibration under new system where control panel does this.
//	switch (Config_control_type) {
//		case 	CONTROL_WINJOYSTICK:
//			if ( joydefs_calibrate_flag )
//				joydefs_calibrate();
//			break;
//	}
}


//	Extended joystick menu
//	----------------------------------------------------------------------------
void joydefsw_win_joyselect(char *title)
{
	FILE *fp;
	char filename[64];
	char *token;
	ubyte axis;

	if (newmenu_filelist("Windows 95 Controls", "*.joy", filename)) {
		fp = fopen(filename, "rb");
		if (!fp) {
			nm_messagebox(TXT_ERROR, 1, TXT_OK, "Unable to find control file");
			strcpy(title, "Windows 95 Custom");
			return;
		}
	
	// Get Axis support.
		if (!fread(&axis, sizeof(ubyte), 1, fp)) {
			nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal or corrupt control file");
			strcpy(title, "Windows 95 Custom");
			fclose(fp);
			return;
		}


		if (fread(kconfig_settings[CONTROL_WINJOYSTICK], 
					sizeof(kconfig_settings[CONTROL_WINJOYSTICK]), 
				1, fp) != 1) {
			nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal or corrupt control file");
			strcpy(title, "Windows 95 Custom");
			fclose(fp);
			return;
		}			
		fclose(fp);					

		token = strtok(filename, ".");
		strcpy(title, token);
	}
	else strcpy(title, "Windows 95 Custom");
}

