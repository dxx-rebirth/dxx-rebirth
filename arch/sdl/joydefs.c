/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/joydefs.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-11-14 03:56:53 $
 *
 * SDL joystick support
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/11/14 03:29:39  bradleyb
 * copied joydefs_config from linux/joydefs.c - controls menu now works
 *
 * Revision 1.1  2001/10/24 09:25:05  bradleyb
 * Moved input stuff to arch subdirs, as in d1x.
 *
 * Revision 1.1  2001/10/10 03:01:29  bradleyb
 * Replacing win32 joystick (broken) with SDL joystick (stubs)
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

int joydefs_calibrate_flag = 0;

void joydefs_calibrate()
{
}

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
}

void joydefs_config()
{
        newmenu_item m[13];
        int i, i1=5, j, nitems=7;

	m[0].type = NM_TYPE_RADIO; m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0;
	m[1].type = NM_TYPE_RADIO; m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0;
	m[2].type = NM_TYPE_RADIO; m[2].text = "MOUSE"; m[2].value = 0; m[2].group = 0;
	m[3].type = NM_TYPE_TEXT; m[3].text="";
	m[4].type = NM_TYPE_MENU; m[4].text="CUSTOMIZE ABOVE";
	m[5].type = NM_TYPE_MENU; m[5].text="CUSTOMIZE KEYBOARD";
	m[6].type = NM_TYPE_MENU; m[6].text="CUSTOMIZE D1X KEYS";

	do {

		i = Config_control_type;
		if(i==CONTROL_MOUSE) i = 2;
		m[i].value=1;

		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );

		for (j = 0; j <= 2; j++)
			if (m[j].value)
				Config_control_type = j;
		i = Config_control_type;
		if (Config_control_type == 2)
			Config_control_type = CONTROL_MOUSE;

		switch(i1)	{
		case 4: 
			kconfig (i, m[i].text);
			break;
		case 5: 
			kconfig(0, "KEYBOARD"); 
			break;
		case 6:
			kconfig(3, "D1X KEYS");
			break;
		} 

	} while(i1>-1);

}
