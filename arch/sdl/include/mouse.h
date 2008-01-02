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
 *
 * Header for mouse functions
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "pstypes.h"
#include "fix.h"

#define MOUSE_MAX_BUTTONS       8
#define Z_SENSITIVITY		100
#define MBTN_LEFT			0
#define MBTN_RIGHT		1
#define MBTN_MIDDLE		2
#define MBTN_Z_UP			3
#define MBTN_Z_DOWN		4
#define MBTN_PITCH_BACKWARD	5
#define MBTN_PITCH_FORWARD        6
#define MBTN_BANK_LEFT		7
#define MBTN_BANK_RIGHT	        8
#define MBTN_HEAD_LEFT		9
#define MBTN_HEAD_RIGHT	        10

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
extern void mouse_get_pos( int *x, int *y, int *dz);
// extern void mouse_get_delta( int *dx, int *dy );
extern void mouse_get_delta( int *dx, int *dy, int *dz );
extern int mouse_get_btns();
extern void mouse_set_pos( int x, int y);

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
#define mouse_get_pos(a,b,c)
#define mouse_get_delta(a,b,c)
#define mouse_get_btns() 0
#define mouse_set_pos(a,b)
#define mouse_button_down_time(a) 0
#define mouse_button_down_count(a) 0
#define mouse_button_state(a) 0

#endif

#endif

