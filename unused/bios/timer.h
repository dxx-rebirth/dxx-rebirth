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


#ifndef _TIMER_H
#define _TIMER_H

#include "pstypes.h"
#include "fix.h"

//==========================================================================
// This installs the timer services and interrupts at the rate specified by
// count_val.  If 'function' isn't 0, the function pointed to by function will
// be called 'freq' times per second.  Should be > 19 and anything around
// 2-3000 is gonna start slowing down the system.  Count_val should be
// 1,193,180 divided by your target frequency. Use 0 for the normal 18.2 Hz
// interrupt rate.

#define TIMER_FREQUENCY 1193180

extern void timer_init();
extern void timer_close();
extern void timer_set_rate(int count_val);
extern void timer_set_function( void _far * function );

//==========================================================================
// These functions return the time since the timer was initialized in
// some various units. The total length of reading time varies for each
// one.  They will roll around after they read 2^32.
// There are milliseconds, milliseconds times 10, milliseconds times 100,
// and microseconds.  They time out after 1000 hrs, 100 hrs, 10 hrs, and
// 1 hr, respectively.

extern fix timer_get_fixed_seconds();	// Rolls about every 9 hours...
extern fix timer_get_fixed_secondsX(); // Assume interrupts already disabled
extern fix timer_get_approx_seconds();		// Returns time since program started... accurate to 1/120th of a second

//NOT_USED extern unsigned int timer_get_microseconds();
//NOT_USED extern unsigned int timer_get_milliseconds100();
//NOT_USED extern unsigned int timer_get_milliseconds10();
//NOT_USED extern unsigned int timer_get_milliseconds();
//NOT_USED extern unsigned int timer_get_millisecondsX();	// Assume interrupts disabled

//==========================================================================
// Use to access the BIOS ticker... ie...   i = TICKER
void timer_delay(fix seconds);

//#define USECS_PER_READING( start, stop, frames ) (((stop-start)*54945)/frames)

#endif

