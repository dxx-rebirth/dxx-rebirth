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
 * $Source: /cvs/cvsroot/d2x/arch/win32/include/mouse.h,v $
 * $Revision: 1.2 $
 * $Author: schaffner $
 * $Date: 2004-08-28 23:17:45 $
 *
 * Header for mouse functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.2  1999/10/09 05:04:34  donut
 * added mouse_init and mouse_close declarations
 *
 * Revision 1.1.1.1  1999/06/14 22:01:24  donut
 * Import of d1x 1.37 source.
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"
#include "fix.h"

#define MOUSE_MAX_BUTTONS       11

#define MB_LEFT			0

#ifdef MB_RIGHT
#undef MB_RIGHT
#endif
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

extern int mouse_init(int unused);
extern void mouse_close(void);


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

