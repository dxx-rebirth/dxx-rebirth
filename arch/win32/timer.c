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

// File nearly completely rewritten by dph-man

#include <windows.h>
#include <mmsystem.h> // DPH: We use timeGetTime from here...

#include "types.h"
#include "maths.h"
#include "timer.h"

static int Installed = 0;

static unsigned long old_tv;

fix timer_get_fixed_seconds()
{
        fix x;

	//Ye good olde unix:
        //Ye bad olde Windows DPH:-)
/* DPH: Using timeGetTime will fail approximately 47 days after Windows was
   started as the timer wraps around to 0. Ever had Windows not crash for 47
   consecutive days? Thought not. */

        unsigned long tv_now=timeGetTime()-old_tv;
        x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
	return x;
}

void delay(int d_time)
{
	fix t, total;
	
	total = (F1_0 * d_time) / 1000;
	t = timer_get_fixed_seconds();
	while (timer_get_fixed_seconds() - t < total) ;
}

void timer_close()
{
 Installed = 0;
}

void timer_init()
{

	if (Installed)
		return;
	Installed = 1;		

/* DPH: Using timeGetTime will fail approximately 47 days after Windows was
   started as the timer wraps around to 0. Ever had Windows not crash for 47
   consecutive days? Thought not. */
 old_tv=timeGetTime();

}

// NOTE: This C file has been "neutered" by dph-man. If someone wants to work
// on this, feel free. I don't use joystick :-)
