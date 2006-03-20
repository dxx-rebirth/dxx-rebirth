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

#include "key.h"
#include "joy.h"
#include "mouse.h"
#include "mono.h"

void main (void)
{
	int i,c, j, j1;

	short jx, jy, jb;

	key_init();
	keyd_buffer_type = 1;

	if (! minit()) {printf("No mono card\n"); exit(5);}

	mopen( 1,1,1,37,10,  "Keyboard" );
	mopen( 2,1,41,37,10, "Joystick");
	mopen( 3,13,1,37,10, "Mouse" );
	mopen( 4,13,41,37,10,"Instructions" );

	mprintf( 4, "F1 - turn off buffering.\n" );
	mprintf( 4, "F2 - turn on ASCII buffering.\n" );
	mprintf( 4, "F3 - turn on scan code buffering.\n" );
	mprintf( 4, "F4 - flush keyboard.\n" );
	mprintf( 4, "F5 - turn repeat off.\n");
	mprintf( 4, "F6 - turn repeat on.\n");
	mprintf( 4, "F7 - do an INT 3.\n" );
	mprintf( 4, "F10 - to display some boxes.\n" );
	//mprintf( 4, "Arrows - example unbuffered.\n");
	//mprintf( 4, "ESC - exit program.\n" );

	if ( (j1=joy_init())==0 )   {
		mprintf( 2, "Not installed.\n" );
	}


	while( !keyd_pressed[KEY_ESC]  ) {

		if (j1) {
			joy_get_pos( &jx, &jy );
			jb = joy_get_btns();
			mprintf( 2,"(%d,%d)\tB1:%d\tB2:%d\n", jx, jy, jb&1, jb&2 );
		}

		//ms_read();
		//mprintf( 3, "(%d,%d)\tB1:%d\tB2:%d\tB3:%d\n", (int)msd_deltax, (int)msd_deltay, 1&msd_button_status, 2&msd_button_status, 4&msd_button_status);

		if (keyd_pressed[KEY_C])
			mclose(1);

		if (keyd_pressed[KEY_O])
			mopen( 1,1,1,37,10,  "Keyboard" );


		if (keyd_pressed[KEY_F1])
			keyd_buffer_type = 0;

		if (keyd_pressed[KEY_F2])
			keyd_buffer_type = 1;

		if (keyd_pressed[KEY_F3])
			keyd_buffer_type = 2;

		if (keyd_pressed[KEY_F4])
			key_flush();

		if (keyd_pressed[KEY_F5])
			keyd_repeat = 0;

		if (keyd_pressed[KEY_F6])
			keyd_repeat = 1;

		if (keyd_pressed[KEY_F7] )
			key_debug();

		if (keyd_pressed[KEY_UP])
			mputc( 1, 24 );

		if (keyd_pressed[KEY_DOWN])
			mputc( 1, 25 );

		if (keyd_pressed[KEY_LEFT])
			mputc( 1, 27 );

		if (keyd_pressed[KEY_RIGHT])
			mputc( 1, 26 );

		if (keyd_pressed[KEY_F10])
			mputc( 1, 254 );

		if (key_checkch())   {
			c = key_getch();
			if (keyd_buffer_type==1) {
				mputc( 1, c );
			}
			else
			{
				mprintf( 1, "[%2X]\n", c );

			}
		}
	}

	key_close();

	mclose(0);

}
