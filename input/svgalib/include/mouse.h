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
 * $Source: /cvs/cvsroot/d2x/input/svgalib/include/mouse.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-01-22 13:00:22 $
 *
 * Header for mouse functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 22:01:46  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.10  1995/02/02  10:22:29  john
 * Added cyberman init parameter.
 * 
 * Revision 1.9  1994/11/18  23:18:09  john
 * Changed some shorts to ints.
 * 
 * Revision 1.8  1994/09/13  12:33:49  john
 * Added functions to get down count and state.
 * 
 * Revision 1.7  1994/08/29  20:52:20  john
 * Added better cyberman support; also, joystick calibration
 * value return funcctiionn,
 * 
 * Revision 1.6  1994/08/24  17:54:35  john
 * *** empty log message ***
 * 
 * Revision 1.5  1994/08/24  17:51:43  john
 * Added transparent cyberman support
 * 
 * Revision 1.4  1993/07/27  09:32:22  john
 * *** empty log message ***
 * 
 * Revision 1.3  1993/07/26  10:46:44  john
 * added definition for mouse_set_pos
 * 
 * Revision 1.2  1993/07/22  13:07:59  john
 * added header for mousesetlimts
 * 
 * Revision 1.1  1993/07/10  13:10:40  matt
 * Initial revision
 * 
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "pstypes.h"
#include "fix.h"

#define MOUSE_MAX_BUTTONS       8

#define MB_LEFT			0
#define MB_RIGHT		1
#define MB_MIDDLE		2
#define MB_Z_UP			3
#define MB_Z_DOWN		4
#define MB_PITCH_BACKWARD	5
#define MB_PITCH_FORWARD        6
#define MB_BANK_LEFT		7
#define MB_BANK_RIGHT	        8
#define MB_HEAD_LEFT		9
#define MB_HEAD_RIGHT	        10

#define MOUSE_LBTN 1
#define MOUSE_RBTN 2
#define MOUSE_MBTN 4

#undef NOMOUSE
#ifndef NOMOUSE

//========================================================================
// Check for mouse driver, reset driver if installed. returns number of
// buttons if driver is present.

extern int mouse_set_limits( int x1, int y1, int x2, int y2 );
extern void mouse_flush();	// clears all mice events...

//========================================================================
extern void mouse_get_pos( int *x, int *y);
extern void mouse_get_delta( int *dx, int *dy );
extern int mouse_get_btns();
extern void mouse_set_pos( int x, int y);
extern void mouse_get_cyberman_pos( int *x, int *y );

// Returns how long this button has been down since last call.
extern fix mouse_button_down_time(int button);

// Returns how many times this button has went down since last call.
extern int mouse_button_down_count(int button);

// Returns 1 if this button is currently down
extern int mouse_button_state(int button);

#else
// 'Neutered' functions... :-)
#define mouse_init(a) -1
#define mouse_set_limits(a,b,c,d) -1
#define mouse_flush()
#define mouse_close()
#define mouse_get_pos(a,b)
#define mouse_get_delta(a,b)
#define mouse_get_btns() 0
#define mouse_set_pos(a,b)
#define mouse_get_cyberman_pos(a,b)
#define mouse_button_down_time(a) 0
#define mouse_button_down_count(a) 0
#define mouse_button_state(a) 0

#endif

#endif

