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
#include <conio.h>

#include "joy.h"
#include "key.h"
#include "timer.h"

void wait()
{   short i=0,j=0;

	while( !i && !key_inkey() )
		joy_get_btn_up_cnt(&i, &j);

}

void main (void)
{
	unsigned int t1, t2;
	int i, start, stop, frames;
	short bd1, bd2, bu1, bu2, b, x, y, bt1, bt2;

	key_init();

	if (!joy_init())   {
		printf( "No joystick detected.\n" );
		key_close();
		exit(1);
	}

	timer_init( 0, NULL );

	printf( "Displaying joystick button transitions and time...any key leaves.\n" );

	i = 0;
	while( !key_inkey() )
	{
		i++;
		if (i>500000)    {
			i = 0;
			joy_get_btn_down_cnt( &bd1, &bd2 );
			joy_get_btn_up_cnt( &bu1, &bu2 );
			joy_get_btn_time( &bt1, &bt2 );
			printf( "%d  %d   %d   %d      T1:%d   T2:%d\n",bd1, bu1, bd2, bu2, bt1, bt2 );
		}
	}

	printf( "\nPress c to do a deluxe-full-fledged calibration.\n" );
	printf( "or any other key to just use the cheap method.\n" );

	if (key_getch() == 'c')    {
		printf( "Move stick to center and press any key.\n" );
		wait();
		joy_set_cen();

		printf( "Move stick to Upper Left corner and press any key.\n" );
		wait();
		joy_set_ul();

		printf( "Move stick to Lower Right corner and press any key.\n" );
		wait();
		joy_set_lr();
	}


	while( !keyd_pressed[KEY_ESC])
	{
		frames++;

		joy_get_pos( &x, &y );
		b = joy_get_btns();

		printf( "%d, %d   (%d %d)\n", x, y, b & 1, b & 2);

	}


	printf( "Testing joystick reading speed...\n" );
	t1 = timer_get_microseconds();
	joy_get_pos( &x, &y );
	t2 = timer_get_microseconds();

	printf( "~ %u æsec per reading using Timer Timing\n", t2 - t1 );

	joy_close();
	key_close();
	timer_close();

	frames = 1000;
	start = TICKER;
	for (i=0; i<frames; i++ )
		joy_get_pos( &x, &y );

	stop = TICKER;

	printf( "~ %d æsec per reading using BIOS ticker as a stopwatch.\n", USECS_PER_READING( start, stop, frames ) );


}
