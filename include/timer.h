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
 * $Source: /cvs/cvsroot/d2x/include/timer.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: bradleyb $
 * $Date: 2001-01-19 03:30:16 $
 *
 * Header for timer functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:21  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.8  1994/12/10  12:27:23  john
 * Added timer_get_approx_seconds.
 * 
 * Revision 1.7  1994/12/10  12:10:25  john
 * Added types.h.
 * 
 * 
 * 
 * 
 * Revision 1.6  1994/12/10  12:07:06  john
 * Added tick counter variable.
 * 
 * Revision 1.5  1994/11/15  12:04:15  john
 * Cleaned up timer code a bit... took out unused functions
 * like timer_get_milliseconds, etc.
 * 
 * Revision 1.4  1994/04/28  23:50:08  john
 * Changed calling for init_timer.  Made the function that the
 * timer calls be a far function. All of this was done to make
 * our timer system compatible with the HMI sound stuff.
 * 
 * Revision 1.3  1994/02/17  15:57:12  john
 * Changed key libary to C.
 * 
 * Revision 1.2  1994/01/18  10:58:34  john
 * Added timer_get_fixed_seconds
 * 
 * Revision 1.1  1993/07/10  13:10:41  matt
 * Initial revision
 * 
 *
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
#if !defined (__MSDOS__) || defined(__GNUC__)
#define _far
#define __far
#define __interrupt
#endif

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

extern fix timer_get_fixed_seconds();   // Rolls about every 9 hours...
#ifdef __MSDOS__
extern fix timer_get_fixed_secondsX(); // Assume interrupts already disabled
extern fix timer_get_approx_seconds();		// Returns time since program started... accurate to 1/120th of a second
extern void timer_set_joyhandler( void (*joy_handler)() );
#else
#define timer_get_fixed_secondsX timer_get_fixed_seconds
#define timer_get_approx_seconds timer_get_fixed_seconds
#endif

//NOT_USED extern unsigned int timer_get_microseconds();
//NOT_USED extern unsigned int timer_get_milliseconds100();
//NOT_USED extern unsigned int timer_get_milliseconds10();
//NOT_USED extern unsigned int timer_get_milliseconds();
//NOT_USED extern unsigned int timer_get_millisecondsX();	// Assume interrupts disabled

//==========================================================================
// Use to access the BIOS ticker... ie...   i = TICKER
#ifdef __LINUX__
#define TICKER (timer_get_fixed_seconds())
#endif

#ifdef __MSDOS__

#ifndef __GNUC__
#define TICKER (*(volatile int *)0x46C)
#else
#include <go32.h>
#include <sys/farptr.h>
#define TICKER _farpeekl(_dos_ds, 0x46c)
#endif
#define USECS_PER_READING( start, stop, frames ) (((stop-start)*54945)/frames)
#endif

#endif
