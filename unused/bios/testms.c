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

#include "mouse.h"
#include "key.h"

short i_dx, i_dy, i_b, i_f, i_happend;

void John( mouse_event * evt )
{
	i_dx = evt->dx;
	i_dy = evt->dy;
	i_b = evt->buttons;
	i_f = evt->flags;
	i_happend = 1;
}

void main (void)
{
	short x, y, dx, dy, i, b;

	printf( "This tests the mouse interface. ESC exits.\n" );

	if (!mouse_init())  {
		printf( "No mouse installed.\n" );
		exit(1);
	}

	key_init();

	mouse_set_handler( ME_LEFT_DOWN | ME_MOVEMENT | ME_RIGHT_DOWN, John );

	while( !keyd_pressed[KEY_ESC])
	{

		if (i_happend)
		{
			i_happend = 0;
			printf( "INT: POS:(%d,%d)\tBTN:%d\tFLG:%d\n", i_dx, i_dy, i_b, i_f );
		}
	}

	mouse_clear_handler();

	delay(500);

	while( !keyd_pressed[KEY_ESC])
	{

		if (i_happend)
		{
			printf( "ERROR: INT SHouLD NOT HAVe HAPND !!\n" );
			break;
		}
		mouse_get_pos( &x, &y );
		mouse_get_delta( &dx, &dy );
		b = mouse_get_btns();
		printf( "POS:(%d,%d)\tDELTA:(%d,%d)\tBUTTONS:%d\n", x, y, dx, dy, b );
	}


	mouse_close();
	key_close();
}
