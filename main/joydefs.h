/* $Id: joydefs.h,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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
 * Variables for joystick settings.
 *
 * Old Log:
 * Revision 2.0  1995/02/27  11:32:12  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.12  1995/01/25  14:37:52  john
 * Made joystick only prompt for calibration once...
 *
 * Revision 1.11  1994/10/13  11:35:43  john
 * Made Thrustmaster FCS Hat work.  Put a background behind the
 * keyboard configure.  Took out turn_sensitivity.  Changed sound/config
 * menu to new menu. Made F6 be calibrate joystick.
 *
 * Revision 1.10  1994/09/10  15:46:49  john
 * First version of new keyboard configuration.
 *
 * Revision 1.9  1994/09/06  14:51:29  john
 * Added sensitivity adjustment, fixed bug with joystick button not
 * staying down.
 *
 * Revision 1.8  1994/08/31  12:56:29  john
 * *** empty log message ***
 *
 * Revision 1.7  1994/08/30  16:37:07  john
 * Added menu options to set controls.
 *
 * Revision 1.6  1994/08/29  21:18:33  john
 * First version of new keyboard/oystick remapping stuff.
 *
 * Revision 1.5  1994/08/24  19:00:30  john
 * Changed key_down_time to return fixed seconds instead of
 * milliseconds.
 *
 * Revision 1.4  1994/08/17  16:50:04  john
 * Added damaging fireballs, missiles.
 *
 * Revision 1.3  1994/07/01  10:55:18  john
 * Added analog joystick throttle
 *
 * Revision 1.2  1994/06/30  20:04:46  john
 * Added -joydef support.
 *
 * Revision 1.1  1994/06/30  18:08:12  john
 * Initial revision
 *
 *
 */


#ifndef _JOYDEFS_H
#define _JOYDEFS_H

void joydefs_calibrate();
void joydefs_config();

extern int joydefs_calibrate_flag;

#ifdef MACINTOSH
extern void joydefs_set_type(ubyte type);
#endif

#endif /* _JOYDEFS_H */
