
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

#include "key.h"

void main (void)
{
	int i,c, j;

	key_init();
	keyd_buffer_type = 1;

	printf( "\n\n\n\nThis tests the keyboard library.\n\n" );
	printf( "Press any key to start...\n" );
	printf( "You pressed: %c\n\n", key_getch() );
	printf( "Press F1 to turn off buffering.\n" );
	printf( "      F2 to turn on ASCII buffering.\n" );
	printf( "      F3 to turn on scan code buffering.\n" );
	printf( "      F4 to flush keyboard.\n" );
	printf( "      F5 to turn repeat off.\n");
	printf( "      F6 to turn repeat on.\n");
	printf( "      F7 to do an INT 3.\n" );
	printf( "      F10 to display some boxes.\n" );
	printf( "      The arrows to see fast multiple keystrokes.\n");
	printf( "      ESC to exit.\n\n" );


	while( !keyd_pressed[KEY_ESC]  ) {

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
			putch( 24 );

		if (keyd_pressed[KEY_DOWN])
			putch( 25 );

		if (keyd_pressed[KEY_LEFT])
			putch( 27 );

		if (keyd_pressed[KEY_RIGHT])
			putch( 26 );


		if (keyd_pressed[KEY_F10])
			putch( 254 );

		if (key_checkch())   {
			c = key_getch();
			if (keyd_buffer_type==1) {
				if (c==13)
				   printf("\n");
				else
				   putch( c );
			}
			else
			{
				printf( "[%2X]\n", c );

			}
			delay(80);      // So we can test buffer
		}

	}

	key_close();

}
