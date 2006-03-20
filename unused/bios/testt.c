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

#include <stdio.h>

#include "timer.h"
#include "key.h"


void main (void)
{
	unsigned int MyTimer, t,t1, t2, ot, myc;
	int i,s, b;

	key_init();

	//timer_init( 4661, NULL );
	timer_init( 0, NULL );

	s = TICKER;
	MyTimer = timer_get_microseconds();

	while(!keyd_pressed[KEY_ESC])
	{
		delay(1);
		ot =t;
		t = timer_get_milliseconds();
		t1 = (TICKER - s)*10000/182;
		//printf( "%d\t%d\t%u\t%d\t%u\t%u\t%u\n", (TICKER-s)*10000/182, myc, t, (int)t-(int)ot, key_down_time(KEY_G), key_down_count(KEY_G), key_up_count(KEY_G) );
		printf( "%u\t%u\t%d\n", t1, t, (int)t - (int)t1 );
	}

	t1 = timer_get_microseconds();
	delay(100);
	t2 = timer_get_microseconds();

	printf( "This number should be about 100000:  %d\n", t2-t1 );

	t1 = timer_get_microseconds();
	t2 = timer_get_microseconds();

	printf( "Overhead for 'timer_get_microseconds' call:  %d ÊSeconds\n", t2-t1 );


	timer_close();

}
