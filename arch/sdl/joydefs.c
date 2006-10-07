/* $Id: joydefs.c,v 1.1.1.1 2006/03/17 19:53:43 zicodxx Exp $ */
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
extern int joy_deadzone;

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

	for (i=0; i<4; i++ )
		if (items[i].value) Config_control_type = i;

        if (Config_control_type == 2) Config_control_type = CONTROL_MOUSE;
        if (Config_control_type == 3) Config_control_type = CONTROL_JOYMOUSE;

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
			case	CONTROL_JOYMOUSE:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
	}
}

void joydefs_config()
{
	newmenu_item m[13];
	int i, i1 = 5, j;
#ifdef D2X_KEYS
	int nitems = 12;
#else
	int nitems = 11;
#endif

	m[0].type = NM_TYPE_RADIO;  m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0;
	m[1].type = NM_TYPE_RADIO;  m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0;
	m[2].type = NM_TYPE_RADIO;  m[2].text = "MOUSE";    m[2].value = 0; m[2].group = 0;
	m[3].type = NM_TYPE_RADIO;  m[3].text = "JOYSTICK & MOUSE"; m[3].value = 0; m[3].group = 0;
	m[4].type = NM_TYPE_TEXT;   m[4].text = "";
	m[5].type = NM_TYPE_MENU;   m[5].text = TXT_CUST_ABOVE;
	m[6].type = NM_TYPE_MENU;   m[6].text = TXT_CUST_KEYBOARD;
	m[7].type = NM_TYPE_TEXT;   m[7].text = "";

	m[8].type = NM_TYPE_SLIDER; m[8].text="Mouse sensitivity"; m[8].value=Config_mouse_sensitivity; m[8].min_value = 0; m[8].max_value = 16;
	m[9].type = NM_TYPE_SLIDER; m[9].text="Joystick sensitivity"; m[9].value=Config_joystick_sensitivity; m[9].min_value = 0; m[9].max_value = 16;
	m[10].type = NM_TYPE_SLIDER; m[10].text="Joystick Deadzone"; m[10].value=joy_deadzone; m[10].min_value=0; m[10].max_value = 16;

#ifdef D2X_KEYS
	m[11].type = NM_TYPE_MENU;   m[11].text = "CUSTOMIZE D2X KEYS";
#endif

	do {

		i = Config_control_type;
		if (i == CONTROL_MOUSE) i = 2;
		if (i==CONTROL_JOYMOUSE) i = 3;
		m[i].value = 1;

		i1 = newmenu_do1(NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1);

		Config_mouse_sensitivity = m[8].value;
		Config_joystick_sensitivity = m[9].value;
		joy_deadzone = m[10].value;

		for (j = 0; j <= 3; j++)
			if (m[j].value)
				Config_control_type = j;
		i = Config_control_type;
		if (Config_control_type == 2)
			Config_control_type = CONTROL_MOUSE;
		if (Config_control_type == 3)
			Config_control_type = CONTROL_JOYMOUSE;

		switch (i1) {
		case 5:
			kconfig(i, m[i].text);
			break;
		case 6:
			kconfig(0, "KEYBOARD");
			break;
#ifdef D2X_KEYS
		case 11:
			kconfig(4, "D2X KEYS");
			break;
#endif
		}

	} while (i1>-1);

}
