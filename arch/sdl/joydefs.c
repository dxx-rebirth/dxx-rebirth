/* $Id: joydefs.c,v 1.6 2003-03-14 05:11:29 btb Exp $ */
/*
 *
 * SDL joystick support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "joydefs.h"
#include "pstypes.h"
#include "newmenu.h"
#include "config.h"
#include "text.h"
#include "kconfig.h"

extern int num_joysticks;

int joydefs_calibrate_flag = 0;

void joydefs_calibrate()
{
	joydefs_calibrate_flag = 0;

	if (!num_joysticks) {
		nm_messagebox( NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}

	//Actual calibration if necessary

}

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = Config_control_type;

	nitems = nitems;
	last_key = last_key;
	citem = citem;		

	for (i=0; i<3; i++ )
		if (items[i].value) Config_control_type = i;

        if (Config_control_type == 2) Config_control_type = CONTROL_MOUSE;

	if ( (oc_type != Config_control_type) && (Config_control_type == CONTROL_THRUSTMASTER_FCS ) ) {
		nm_messagebox( TXT_IMPORTANT_NOTE, 1, TXT_OK, TXT_FCS );
	}

	if (oc_type != Config_control_type) {
		switch (Config_control_type) {
	//		case	CONTROL_NONE:
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
			case	CONTROL_GRAVIS_GAMEPAD:
	//		case	CONTROL_MOUSE:
	//		case	CONTROL_CYBERMAN:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
	}
}

void joydefs_config()
{
	newmenu_item m[13];
	int i, i1 = 5, j, nitems = 10;

	m[0].type = NM_TYPE_RADIO; m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0;
	m[1].type = NM_TYPE_RADIO; m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0;
	m[2].type = NM_TYPE_RADIO; m[2].text = "MOUSE"; m[2].value = 0; m[2].group = 0;
	m[3].type = NM_TYPE_TEXT; m[3].text="";
	m[4].type = NM_TYPE_MENU;   m[4].text = TXT_CUST_ABOVE;
	m[5].type = NM_TYPE_TEXT;   m[5].text = "";
	m[6].type = NM_TYPE_SLIDER;	m[6].text = TXT_JOYS_SENSITIVITY; m[6].value = Config_joystick_sensitivity; m[6].min_value =0; m[6].max_value = 16;
	m[7].type = NM_TYPE_TEXT;   m[7].text = "";
	m[8].type = NM_TYPE_MENU;   m[8].text = TXT_CUST_KEYBOARD;
	m[9].type = NM_TYPE_MENU;   m[9].text = "CUSTOMIZE D2X KEYS";

	do {

		i = Config_control_type;
		if(i==CONTROL_MOUSE) i = 2;
		m[i].value=1;

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );

		Config_joystick_sensitivity = m[6].value;

		for (j = 0; j <= 2; j++)
			if (m[j].value)
				Config_control_type = j;
		i = Config_control_type;
		if (Config_control_type == 2)
			Config_control_type = CONTROL_MOUSE;

		switch (i1) {
		case 4:
			kconfig (i, m[i].text);
			break;
		case 8:
			kconfig(0, "KEYBOARD");
			break;
		case 9:
			kconfig(4, "D2X KEYS");
			break;
		}

	} while (i1>-1);

}
