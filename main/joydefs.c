/* $Id: joydefs.c,v 1.3 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Joystick Settings Stuff
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: joydefs.c,v 1.3 2004-08-28 23:17:45 schaffner Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <dos.h>
#endif

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
#if 0
#include "victor.h"
#endif
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

int joydefs_calibrate_flag = 0;

#ifdef MACINTOSH
ubyte joydefs_calibrating = 0;		// stupid hack all because of silly mouse cursor emulation
#endif

void joy_delay()
{
	stop_time();
	timer_delay(.25);
	//delay(250);				// changed by allender because	1) more portable
							//								2) was totally broken on PC
	joy_flush();
	start_time();
}


int joycal_message( char * title, char * text )
{
	int i;
	newmenu_item	m[2];
	MAC(joydefs_calibrating = 1;)
	m[0].type = NM_TYPE_TEXT; m[0].text = text;
	m[1].type = NM_TYPE_MENU; m[1].text = TXT_OK;
	i = newmenu_do( title, NULL, 2, m, NULL );
	MAC(joydefs_calibrating = 0;)
	if ( i < 0 )
		return 1;
	return 0;
}

extern int WriteConfigFile();

void joydefs_calibrate(void)
{
	#ifndef MACINTOSH
	if ( (Config_control_type!=CONTROL_JOYSTICK) && (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) && (Config_control_type!=CONTROL_THRUSTMASTER_FCS) )
		return;
	#else
	if ( (Config_control_type == CONTROL_NONE) || (Config_control_type == CONTROL_MOUSE) )
		return;
	#endif

	palette_save();
	apply_modified_palette();
	reset_palette_add();

	gr_palette_load(gr_palette);
#if 0
	joydefs_calibrate2();
#endif

	reset_cockpit();

	palette_restore();

}

#if 0
#ifndef MACINTOSH
void joydefs_calibrate2()
{
	ubyte masks;
	int org_axis_min[4];
	int org_axis_center[4];
	int org_axis_max[4];

	int axis_min[4] = { 0, 0, 0, 0 };
	int axis_cen[4] = { 0, 0, 0, 0 };
	int axis_max[4] = { 0, 0, 0, 0 };

	int temp_values[4];
	char title[50];
	char text[50];
	int nsticks = 0;

	joydefs_calibrate_flag = 0;

	joy_get_cal_vals(org_axis_min, org_axis_center, org_axis_max);

	joy_set_cen();
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );

	if (!joy_present)	{
		nm_messagebox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}

	masks = joy_get_present_mask();

#ifndef WINDOWS
	if ( masks == JOY_ALL_AXIS )
		nsticks = 2;
	else
		nsticks = 1;
#else
	nsticks = 1;
#endif

	if (Config_control_type == CONTROL_THRUSTMASTER_FCS)
		nsticks = 1;		//ignore for now the Sidewinder Pro X2 axis

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
	}

	if (joycal_message( title, text )) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_min[0] = temp_values[0];
	axis_min[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_max[0] = temp_values[0];
	axis_max[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_CENTER);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_CENTER);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_cen[0] = temp_values[0];
	axis_cen[1] = temp_values[1];
	axis_cen[2] = temp_values[2];
	joy_delay();

	// The fcs uses axes 3 for hat, so don't calibrate it.
	if ( Config_control_type == CONTROL_THRUSTMASTER_FCS )	{
	#ifndef WINDOWS
		//set Y2 axis, which is hat
		axis_min[3] = 0;
		axis_cen[3] = temp_values[3]/2;
		axis_max[3] = temp_values[3];
		joy_delay();

		//if X2 exists, calibrate it (for Sidewinder Pro)
		if ( kconfig_is_axes_used(2) && (masks & JOY_2_X_AXIS) )	{
			sprintf( title, "Joystick X2 axis\nLEFT");
			sprintf( text, "Move joystick X2 axis\nall the way left");
			if (joycal_message( title, text )) {
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_min[2] = temp_values[2];
			axis_min[3] = temp_values[3];
			joy_delay();

			sprintf( title, "Joystick X2 axis\nRIGHT");
			sprintf( text, "Move joystick X2 axis\nall the way right");
			if (joycal_message( title, text ))	{
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_max[2] = temp_values[2];
			axis_max[3] = temp_values[3];
			joy_delay();
		}
	#endif
	} else {
		masks = joy_get_present_mask();

		if ( nsticks == 2 )	{
			if ( kconfig_is_axes_used(2) || kconfig_is_axes_used(3) )	{
				sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
				sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
				if (joycal_message( title, text )) {
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_min[2] = temp_values[2];
				axis_min[3] = temp_values[3];
				joy_delay();

				sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
				sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
				if (joycal_message( title, text ))	{
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_max[2] = temp_values[2];
				axis_max[3] = temp_values[3];
				joy_delay();

				sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_CENTER);
				sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
				if (joycal_message( title, text ))	{
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_cen[2] = temp_values[2];
				axis_cen[3] = temp_values[3];
				joy_delay();
			}
		}
	#ifndef WINDOWS
		else if ( (!(masks & JOY_2_X_AXIS)) && (masks & JOY_2_Y_AXIS) )	{
			if ( kconfig_is_axes_used(3) )	{
				// A throttle axis!!!!!
				sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_FORWARD);
				if (joycal_message( title, TXT_MOVE_THROTTLE_F))	{
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_min[3] = temp_values[3];
				joy_delay();

				sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_REVERSE);
				if (joycal_message( title, TXT_MOVE_THROTTLE_R)) {
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_max[3] = temp_values[3];
				joy_delay();

				sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_CENTER);
				if (joycal_message( title, TXT_MOVE_THROTTLE_C)) {
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_cen[3] = temp_values[3];
				joy_delay();
			}
		}
	#endif
	}
	joy_set_cal_vals(axis_min, axis_cen, axis_max);

	WriteConfigFile();
}
#else
void joydefs_calibrate2()
{
	ubyte masks;
	int org_axis_min[4];
	int org_axis_center[4];
	int org_axis_max[4];

	int axis_min[4] = { 0, 0, 0, 0 };
	int axis_cen[4] = { 0, 0, 0, 0 };
	int axis_max[4] = { 0, 0, 0, 0 };

	int temp_values[4];
	char title[50];
	char text[50];
	int i, nsticks = 0;

	joydefs_calibrate_flag = 0;

	if ( Config_control_type == CONTROL_THRUSTMASTER_FCS ) {
		axis_cen[0] = axis_cen[1] = axis_cen[2] = 0;
		axis_min[0] = axis_min[1] = axis_min[2] = -127;
		axis_max[0] = axis_max[1] = axis_max[2] = 127;
		axis_min[3] = 0;
		axis_max[3] = 255;
		axis_cen[3] = 128;
		joy_set_cal_vals(axis_min, axis_cen, axis_max);
		return;
	}

	if ( Config_control_type == CONTROL_FLIGHTSTICK_PRO ) 		// no calibration needed
		return;

	joy_get_cal_vals(org_axis_min, org_axis_center, org_axis_max);

	joy_set_cen();
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );

	if (!joy_present)	{
		nm_messagebox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}

	masks = joy_get_present_mask();

	if ( masks == JOY_ALL_AXIS )
		nsticks = 2;
	else
		nsticks = 1;

	if ( (Config_control_type == CONTROL_THRUSTMASTER_FCS) || (Config_control_type == CONTROL_FLIGHTSTICK_PRO) )
		nsticks = 1;		//ignore for now the Sidewinder Pro X2 axis

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
	}

	if (joycal_message( title, text )) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_min[0] = temp_values[0];
	axis_min[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_max[0] = temp_values[0];
	axis_max[1] = temp_values[1];
	joy_delay();

	if ( nsticks == 2 )	{
		sprintf( title, "%s #1\n%s", TXT_JOYSTICK, TXT_CENTER);
		sprintf( text, "%s #1 %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
	} else {
		sprintf( title, "%s\n%s", TXT_JOYSTICK, TXT_CENTER);
		sprintf( text, "%s %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
	}
	if (joycal_message( title, text)) {
		joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
		return;
	}
	joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
	axis_cen[0] = temp_values[0];
	axis_cen[1] = temp_values[1];
	axis_cen[2] = temp_values[2];
	joy_delay();

	masks = joy_get_present_mask();

	if ( nsticks == 2 )	{
		if ( kconfig_is_axes_used(2) || kconfig_is_axes_used(3) )	{
			sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
			sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
			if (joycal_message( title, text )) {
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_min[2] = temp_values[2];
			axis_min[3] = temp_values[3];
			joy_delay();

			sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
			sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
			if (joycal_message( title, text ))	{
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_max[2] = temp_values[2];
			axis_max[3] = temp_values[3];
			joy_delay();

			sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_CENTER);
			sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
			if (joycal_message( title, text ))	{
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_cen[2] = temp_values[2];
			axis_cen[3] = temp_values[3];
			joy_delay();
		}
	} else {
		if ( kconfig_is_axes_used(3) )	{
			// A throttle axis!!!!!
			sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_FORWARD);
			if (joycal_message( title, TXT_MOVE_THROTTLE_F))	{
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_min[3] = temp_values[3];
			joy_delay();

			sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_REVERSE);
			if (joycal_message( title, TXT_MOVE_THROTTLE_R)) {
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_max[3] = temp_values[3];
			joy_delay();

			sprintf( title, "%s\n%s", TXT_THROTTLE, TXT_CENTER);
			if (joycal_message( title, TXT_MOVE_THROTTLE_C)) {
				joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
				return;
			}
			joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
			axis_cen[3] = temp_values[3];
			joy_delay();
		}
	}
	joy_set_cal_vals(axis_min, axis_cen, axis_max);

	WriteConfigFile();
}
#endif
#endif // 0

//char *control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick", "Flightstick Pro", "Thrustmaster FCS", "Gravis Gamepad", "Mouse", "Cyberman" };

#ifndef MACINTOSH		// mac will have own verion of this function

#define CONTROL_MAX_TYPES_DOS	(CONTROL_MAX_TYPES-1)	//last item is windows only

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = Config_control_type;

	nitems = nitems;
	last_key = last_key;
	citem = citem;

	for (i=0; i<CONTROL_MAX_TYPES_DOS; i++ )
		if (items[i].value) Config_control_type = i;

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS ) )	{
		nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
	}

	if (oc_type != Config_control_type) {
		switch (Config_control_type) {
	//		case	CONTROL_NONE:
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
	//		case	CONTROL_GRAVIS_GAMEPAD:
	//		case	CONTROL_MOUSE:
	//		case	CONTROL_CYBERMAN:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
	}

}

#else		// ifndef MACINTOSH

#define MAX_MAC_CONTROL_TYPES	6

char *ch_warning = "Choosing this option will\noverride any settings for Descent II\nin the CH control panels.  This\noption provides direct programming\nonly.  See the readme for details.";
char *tm_warning = "Choosing this option might\ncause settings in the Thrustmaster\ncontrol panels to be overridden\ndepending on the direct inhibition\nsetting in that panel.\nSee the readme for details.";
char *ms_warning = "When using a Mousestick II,\nbe sure that there is not a stick\nset active for Descent II.\nHaving a stick set active might cause\nundesirable joystick and\nkeyboard behavior.  See\nthe readme for detals.";
char *joy_warning = "Please use your joystick's\ncontrol panel to customize the\nbuttons and axis for Descent II.\nSee the joystick's manual for\ninstructions.";

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = Config_control_type;
	char *warning_text = NULL;

	nitems = nitems;
	last_key = last_key;
	citem = citem;

	for (i=0; i<MAX_MAC_CONTROL_TYPES; i++ )
		if (items[i].value) Config_control_type = i;

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_FLIGHTSTICK_PRO ) )	{
		warning_text = ch_warning;
	}

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS ) )	{
		warning_text = tm_warning;
	}

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_GRAVIS_GAMEPAD ) )	{
		warning_text = ms_warning;
	}

	if (warning_text) {
		hide_cursor();
		nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, warning_text );
		show_cursor();
	}

	if (oc_type != Config_control_type) {
		switch (Config_control_type) {
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
			case	CONTROL_GRAVIS_GAMEPAD:		// this really means a firebird or mousestick
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
		joydefs_set_type( Config_control_type );
	}

}

#endif

extern ubyte kc_use_external_control;
extern ubyte kc_enable_external_control;
extern ubyte *kc_external_name;

#ifndef WINDOWS

#ifdef MACINTOSH

//NOTE: MAC VERSION
void joydefs_config()
{
	int i, old_masks, masks,nitems;
	newmenu_item m[14];
	int i1=11;
	char xtext[128];

	do {
		m[0].type = NM_TYPE_RADIO; m[0].text = CONTROL_TEXT(0); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = CONTROL_TEXT(1); m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO; m[2].text = CONTROL_TEXT(2); m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO; m[3].text = CONTROL_TEXT(3); m[3].value = 0; m[3].group = 0;
// change the text for the thrustmaster
		m[3].text = "Thrustmaster";
		m[4].type = NM_TYPE_RADIO; m[4].text = CONTROL_TEXT(4); m[4].value = 0; m[4].group = 0;
// change the text of the gravis gamepad to be the mac gravis sticks
		m[4].text = "Gravis Firebird/MouseStick II";
		m[5].type = NM_TYPE_RADIO; m[5].text = CONTROL_TEXT(5); m[5].value = 0; m[5].group = 0;

		m[6].type = NM_TYPE_MENU;   m[6].text=TXT_CUST_ABOVE;
		m[7].type = NM_TYPE_TEXT;   m[7].text="";
		m[8].type = NM_TYPE_SLIDER; m[8].text=TXT_JOYS_SENSITIVITY; m[8].value=Config_joystick_sensitivity; m[8].min_value =0; m[8].max_value = 8;
		m[9].type = NM_TYPE_TEXT;   m[9].text="";
		m[10].type = NM_TYPE_MENU;  m[10].text=TXT_CUST_KEYBOARD;
		nitems=11;

		m[Config_control_type].value = 1;

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );
		Config_joystick_sensitivity = m[8].value;

		switch(i1)	{
		case 6: {
				old_masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						old_masks |= (1<<i);
				}
				if ( Config_control_type==0 )
					// nothing...
					Config_control_type=0;
				else if ( Config_control_type == 1)  // this is just a joystick
					nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, joy_warning );
				else if ( Config_control_type<5 ) {
					char title[64];

					if (Config_control_type == 3)
						strcpy(title, "Thrustmaster");
					else if (Config_control_type == 4)
						strcpy(title, "Gravis Firebird/Mousestick II");
					else
						strcpy(title, CONTROL_TEXT(Config_control_type) );


					kconfig(1, title );
				} else
					kconfig(2, CONTROL_TEXT(Config_control_type) );

				masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						masks |= (1<<i);
				}

				switch (Config_control_type) {
				case	CONTROL_JOYSTICK:
				case	CONTROL_FLIGHTSTICK_PRO:
				case	CONTROL_THRUSTMASTER_FCS:
				case	CONTROL_GRAVIS_GAMEPAD:
					{
						for (i=0; i<4; i++ )	{
							if ( (masks&(1<<i)) && (!(old_masks&(1<<i))))
								joydefs_calibrate_flag = 1;
						}
					}
					break;
				}
			}
			break;
		case 10:
			kconfig(0, TXT_KEYBOARD);
			break;
		}

	} while(i1>-1);

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
//	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		if ( joydefs_calibrate_flag )
			joydefs_calibrate();
		break;
	}

}

// silly routine to tell the joystick handler which type of control
// we are using
void joydefs_set_type(ubyte type)
{
	ubyte joy_type;

	switch (type)
	{
		case	CONTROL_NONE:				joy_type = JOY_AS_NONE;					break;
		case	CONTROL_JOYSTICK:			joy_type = JOY_AS_MOUSE;				break;
		case	CONTROL_FLIGHTSTICK_PRO:	joy_type = JOY_AS_JOYMANAGER;			break;
		case	CONTROL_THRUSTMASTER_FCS:	joy_type = JOY_AS_THRUSTMASTER;			break;
		case	CONTROL_GRAVIS_GAMEPAD:		joy_type = JOY_AS_MOUSESTICK;			break;
		case	CONTROL_MOUSE:				joy_type = JOY_AS_MOUSE;				break;
	}
	joy_set_type(joy_type);
}

#else		// #ifdef MACINTOSH

//NOTE: UNIX/DOS VERSION
void joydefs_config()
{
	int i, old_masks, masks,nitems;
	newmenu_item m[14];
	int i1=11;
	char xtext[128];

	do {
		nitems=12;
		m[0].type = NM_TYPE_RADIO; m[0].text = CONTROL_TEXT(0); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = CONTROL_TEXT(1); m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO; m[2].text = CONTROL_TEXT(2); m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO; m[3].text = CONTROL_TEXT(3); m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_RADIO; m[4].text = CONTROL_TEXT(4); m[4].value = 0; m[4].group = 0;
		m[5].type = NM_TYPE_RADIO; m[5].text = CONTROL_TEXT(5); m[5].value = 0; m[5].group = 0;
		m[6].type = NM_TYPE_RADIO; m[6].text = CONTROL_TEXT(6); m[6].value = 0; m[6].group = 0;

		m[ 7].type = NM_TYPE_MENU;		m[ 7].text=TXT_CUST_ABOVE;
		m[ 8].type = NM_TYPE_TEXT;		m[ 8].text="";
		m[ 9].type = NM_TYPE_SLIDER;	m[ 9].text=TXT_JOYS_SENSITIVITY; m[9].value=Config_joystick_sensitivity; m[9].min_value =0; m[9].max_value = 8;
		m[10].type = NM_TYPE_TEXT;		m[10].text="";
		m[11].type = NM_TYPE_MENU;		m[11].text=TXT_CUST_KEYBOARD;

		m[Config_control_type].value = 1;

		if ( kc_use_external_control )	{
			sprintf( xtext, "Enable %s", kc_external_name );
			m[12].type = NM_TYPE_CHECK; m[12].text = xtext; m[12].value = kc_enable_external_control;
			nitems++;
		}

		m[nitems].type = NM_TYPE_MENU; m[nitems].text="CUSTOMIZE D2X KEYS"; nitems++;

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );
		Config_joystick_sensitivity = m[9].value;

		switch(i1)	{
		case 7: {
				old_masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						old_masks |= (1<<i);
				}
				if ( Config_control_type==0 )
					kconfig(0, TXT_KEYBOARD);
				else if ( Config_control_type<5 )
					kconfig(1, CONTROL_TEXT(Config_control_type) );
				else
					kconfig(2, CONTROL_TEXT(Config_control_type) );

				masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						masks |= (1<<i);
				}

				switch (Config_control_type) {
				case	CONTROL_JOYSTICK:
				case	CONTROL_FLIGHTSTICK_PRO:
				case	CONTROL_THRUSTMASTER_FCS:
					{
						for (i=0; i<4; i++ )	{
							if ( (masks&(1<<i)) && (!(old_masks&(1<<i))))
								joydefs_calibrate_flag = 1;
						}
					}
					break;
				}
			}
			break;
		case 11:
			kconfig(0, TXT_KEYBOARD);
			break;
		case 12:
			kconfig(4, "D2X KEYS");
			break;
		}

		if ( kc_use_external_control )	{
			kc_enable_external_control = m[12].value;
		}

	} while(i1>-1);

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
		if ( joydefs_calibrate_flag )
			joydefs_calibrate();
		break;
	}

}

#endif	// ifdef MACINTOSH

#else 	//ifndef WINDOWS

void joydef_menuset_win(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = Config_control_type;

	Int3();	//need to make this code work for windows

	nitems = nitems;
	last_key = last_key;
	citem = citem;

	for (i=0; i<CONTROL_MAX_TYPES; i++ )
		if (items[i].value) Config_control_type = i;

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS ) )	{
		nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
	}

	if (oc_type != Config_control_type) {
		switch (Config_control_type) {
	//		case	CONTROL_NONE:
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
	//		case	CONTROL_GRAVIS_GAMEPAD:
	//		case	CONTROL_MOUSE:
	//		case	CONTROL_CYBERMAN:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
	}

}

//NOTE: WINDOWS VERSION
void joydefs_config()
{
	int i, old_masks, masks,nitems;
	newmenu_item m[14];
	int i1=11;
	char xtext[128];

	Int3();		//this routine really needs to be cleaned up

	do {
		nitems=13;
		m[0].type = NM_TYPE_RADIO; m[0].text = CONTROL_TEXT(0); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = CONTROL_TEXT(1); m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO; m[2].text = CONTROL_TEXT(2); m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO; m[3].text = CONTROL_TEXT(3); m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_RADIO; m[4].text = CONTROL_TEXT(4); m[4].value = 0; m[4].group = 0;
		m[5].type = NM_TYPE_RADIO; m[5].text = CONTROL_TEXT(5); m[5].value = 0; m[5].group = 0;
		m[6].type = NM_TYPE_RADIO; m[6].text = CONTROL_TEXT(6); m[6].value = 0; m[6].group = 0;
		m[7].type = NM_TYPE_RADIO; m[7].text = CONTROL_TEXT(7); m[7].value = 0; m[7].group = 0;

		m[8].type = NM_TYPE_MENU; m[8].text=TXT_CUST_ABOVE;
		m[9].type = NM_TYPE_TEXT;   m[9].text="";
		m[10].type = NM_TYPE_SLIDER; m[10].text=TXT_JOYS_SENSITIVITY; m[10].value=Config_joystick_sensitivity; m[10].min_value =0; m[10].max_value = 8;
		m[11].type = NM_TYPE_TEXT;   m[11].text="";
		m[12].type = NM_TYPE_MENU; m[12].text=TXT_CUST_KEYBOARD;

		m[Config_control_type].value = 1;

		if ( kc_use_external_control )	{
			sprintf( xtext, "Enable %s", kc_external_name );
			m[13].type = NM_TYPE_CHECK; m[13].text = xtext; m[13].value = kc_enable_external_control;
			nitems++;
		}

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_win, i1 );
		Config_joystick_sensitivity = m[10].value;

		switch(i1)	{
		case 8: {
				old_masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						old_masks |= (1<<i);
				}
				if ( Config_control_type==0 )
					// nothing...
					Config_control_type=0;
				else if ( Config_control_type<5 )
					kconfig(1, CONTROL_TEXT(Config_control_type) );
				else
					kconfig(2, CONTROL_TEXT(Config_control_type) );

				masks = 0;
				for (i=0; i<4; i++ )		{
					if (kconfig_is_axes_used(i))
						masks |= (1<<i);
				}

				switch (Config_control_type) {
				case	CONTROL_JOYSTICK:
				case	CONTROL_FLIGHTSTICK_PRO:
				case	CONTROL_THRUSTMASTER_FCS:
					{
						for (i=0; i<4; i++ )	{
							if ( (masks&(1<<i)) && (!(old_masks&(1<<i))))
								joydefs_calibrate_flag = 1;
						}
					}
					break;
				}
			}
			break;
		case 12:
			kconfig(0, TXT_KEYBOARD);
			break;
		}

		if ( kc_use_external_control )	{
			kc_enable_external_control = m[13].value;
		}

	} while(i1>-1);

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
		if ( joydefs_calibrate_flag )
			joydefs_calibrate();
		break;
	}

}

#endif 	//ifndef WINDOWS
