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

#include <conf.h>

#ifdef __ENV_DJGPP__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
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
#include "render.h"
#include "palette.h"
#include "newmenu.h"
#include "args.h"
#include "text.h"
#include "kconfig.h"
#include "digi.h"
#include "playsave.h"

int joydefs_calibrate_flag = 0;

int Joy_is_Sidewinder = 0;

void joy_delay()
{
#ifdef __MSDOS__
	int t1 = TICKER + 19/4;			// Wait 1/4 second...
	stop_time();
	while( TICKER < t1 );
	joy_flush();
	start_time();
#endif
}


int joycal_message( char * title, char * text )
{
	int i;
	newmenu_item	m[2];
	m[0].type = NM_TYPE_TEXT; m[0].text = text;
	m[1].type = NM_TYPE_MENU; m[1].text = TXT_OK;
	i = newmenu_do( title, NULL, 2, m, NULL );
	if ( i < 0 ) 
		return 1;
	return 0;
}

extern int WriteConfigFile();	

void joydefs_calibrate()
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

	if ( (Config_control_type!=CONTROL_JOYSTICK) && (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) && (Config_control_type!=CONTROL_THRUSTMASTER_FCS) && (Config_control_type!=CONTROL_GRAVIS_GAMEPAD) )	
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

        if ( nsticks == 2 && !Joy_is_Sidewinder)     {
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

        if ( nsticks == 2 && !Joy_is_Sidewinder)     {
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

        if ( nsticks == 2 && !Joy_is_Sidewinder)     {
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
	joy_delay();

	// The fcs uses axes 3 for hat, so don't calibrate it.
        if (Config_control_type == CONTROL_THRUSTMASTER_FCS)  {
		axis_min[3] = 0;
		axis_cen[3] = temp_values[3]/2;
		axis_max[3] = temp_values[3];
		joy_delay();
	}

        if (Joy_is_Sidewinder || Config_control_type != CONTROL_THRUSTMASTER_FCS) {
            //    masks = joy_get_present_mask();

                if ( nsticks == 2 )     {
			if ( kconfig_is_axes_used(2) || kconfig_is_axes_used(3) )	{
                                if(Joy_is_Sidewinder)
                                 {
                                  sprintf( title, "%s\nTWIST-LEFT", TXT_JOYSTICK);
                                  sprintf( text, "Twist Joystick to \nthe left side");
                                 }
                                else
                                 {
                                  sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_UPPER_LEFT);
                                  sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_UL);
                                 }
				if (joycal_message( title, text )) {
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_min[2] = temp_values[2];
				axis_min[3] = temp_values[3];
				joy_delay();

                                if(Joy_is_Sidewinder)
                                 {
                                  sprintf( title, "%s\nTWIST-RIGHT", TXT_JOYSTICK);
                                  sprintf( text, "Twist Joystick to \nthe right side");
                                 }
                                else
                                 {
                                  sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_LOWER_RIGHT);
                                  sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_LR);
                                 }
				if (joycal_message( title, text ))	{
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max); 
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_max[2] = temp_values[2];
				axis_max[3] = temp_values[3];
				joy_delay();

                                
                                if(Joy_is_Sidewinder)
                                 {
                                  sprintf( title, "%s\nCENTER", TXT_JOYSTICK);
                                  sprintf( text, "%s %s",TXT_MOVE_JOYSTICK, TXT_TO_C);
                                 }
                                else
                                 {
                                   sprintf( title, "%s #2\n%s", TXT_JOYSTICK, TXT_CENTER);
                                   sprintf( text, "%s #2 %s", TXT_MOVE_JOYSTICK, TXT_TO_C);
                                 }

				if (joycal_message( title, text ))	{
					joy_set_cal_vals(org_axis_min, org_axis_center, org_axis_max);
					return;
				}
				joystick_read_raw_axis( JOY_ALL_AXIS, temp_values );
				axis_cen[2] = temp_values[2];
				axis_cen[3] = temp_values[3];	
				joy_delay();
			}
		} else if ( (!(masks & JOY_2_X_AXIS)) && (masks & JOY_2_Y_AXIS) )	{
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
	}
	joy_set_cal_vals(axis_min, axis_cen, axis_max);


//added 9/1/98 by Victor Rachels to make sidewinder calibratable
/*         if(Joy_is_Sidewinder)
          Config_control_type=tempstick; */
//end this section addition - Victor Rachels


	WriteConfigFile();
}


//char *control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick", "Flightstick Pro", "Thrustmaster FCS", "Gravis Gamepad", "Mouse", "Cyberman" };

void joydef_menuset_1(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int i;
	int oc_type = Config_control_type;

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
			case	CONTROL_GRAVIS_GAMEPAD:
	//		case	CONTROL_MOUSE:
	//		case	CONTROL_CYBERMAN:
				joydefs_calibrate_flag = 1;
		}
		kc_set_controls();
	}

}

extern ubyte kc_use_external_control;
extern ubyte kc_enable_external_control;
extern ubyte *kc_external_name;

void joydefs_config()
{
	char xtext[128];
	int i, old_masks, masks;
        newmenu_item m[13];
	int i1=9;
	int nitems;

	do {
                nitems = 12;
		m[0].type = NM_TYPE_RADIO; m[0].text = CONTROL_TEXT(0); m[0].value = 0; m[0].group = 0;
		m[1].type = NM_TYPE_RADIO; m[1].text = CONTROL_TEXT(1); m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO; m[2].text = CONTROL_TEXT(2); m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO; m[3].text = CONTROL_TEXT(3); m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_RADIO; m[4].text = CONTROL_TEXT(4); m[4].value = 0; m[4].group = 0;
		m[5].type = NM_TYPE_RADIO; m[5].text = CONTROL_TEXT(5); m[5].value = 0; m[5].group = 0;
		m[6].type = NM_TYPE_RADIO; m[6].text = CONTROL_TEXT(6); m[6].value = 0; m[6].group = 0;
		m[7].type = NM_TYPE_MENU; m[7].text=TXT_CUST_ABOVE;
		m[8].type = NM_TYPE_TEXT; m[8].text="";
		m[9].type = NM_TYPE_MENU; m[9].text=TXT_CUST_KEYBOARD;
//added on 2/5/99 by Victor Rachels to add d1x key menu
                m[10].type = NM_TYPE_MENU; m[10].text="CUSTOMIZE D1X KEYS";
//end this section addition - VR
                m[11].type = NM_TYPE_CHECK; m[11].text= "Joy is Sidewinder"; m[11].value=Joy_is_Sidewinder;
	
		if ( kc_use_external_control )	{
			sprintf( xtext, "Enable %s", kc_external_name );
                        m[12].type = NM_TYPE_CHECK; m[12].text = xtext; m[12].value = kc_enable_external_control;
			nitems = nitems + 1;
		}
		
		m[Config_control_type].value = 1;
	 
		i1 = newmenu_do1( NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1 );

//added on 9/1/98 by Victor Rachels to add Sidewinder calibration
                Joy_is_Sidewinder = m[11].value;
//end this section addition

		switch(i1)	{
		case 7: {
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
		case 9: 
			kconfig(0, TXT_KEYBOARD); 
			break;
//added on 2/5/99 by Victor Rachels to add d1x key menu
                case 10:
                        kconfig(3,"D1X Keys");
                        break;
//end this section addition - VR
		}

		if ( kc_use_external_control )	{
                        kc_enable_external_control = m[12].value;
		}

	} while(i1>-1);

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		if ( joydefs_calibrate_flag )
			joydefs_calibrate();
		break;
	}

}


#endif // __ENV_DJGPP__
