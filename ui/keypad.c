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
#include <stdlib.h>
#include <stdio.h>
#ifndef __LINUX__
#include <conio.h>
#include <dos.h>
#include <direct.h>
#endif
#include <string.h>
#include <math.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "key.h"

#include "mono.h"

#include "ui.h"

#include "u_mem.h"
#include "func.h"
#include "error.h"

#define MAX_NUM_PADS 20

static UI_GADGET_BUTTON * Pad[17];
static UI_KEYPAD * KeyPad[MAX_NUM_PADS];
static int active_pad;

static int desc_x, desc_y;

static int HotKey[17];
static int HotKey1[17];

#define REMOVE_EOL(s)     (*(strstr( (s), "\n" ))='\0')

int ui_pad_get_current()
{
	return active_pad;
}

void ui_pad_init()
{
	int i;

	for (i=0; i< MAX_NUM_PADS; i++ )
		KeyPad[i] = NULL;

	active_pad = -1;
}

void ui_pad_close()
{
	int i, j;

	for (i=0; i< MAX_NUM_PADS; i++ )
		if (KeyPad[i])
		{
			for (j=0; j<17; j++ )
				free(KeyPad[i]->buttontext[j]);
			free( KeyPad[i] );
			KeyPad[i] = NULL;	
		}

}


void LineParse( int n, char * dest, char * source )
{
	int i = 0, j=0, cn = 0;

	// Go to the n'th line
	while (cn < n )
        if ((unsigned char)source[i++] == 179 )
			cn++;

	// Read up until the next comma
    while ( (unsigned char)source[i] != 179 )
	{
		dest[j] = source[i++];
		j++;		
	}

	// Null-terminate	
	dest[j++] = 0;
}

void ui_pad_activate( UI_WINDOW * wnd, int x, int y )
{
	int w,h,row,col, n;
	int bh, bw;

	bw = 56; bh = 30;

	gr_set_current_canvas( wnd->canvas );
	ui_draw_box_in( x, y, x+(bw*4)+10 + 200, y+(bh*5)+45 );

	x += 5;
	y += 20;

	desc_x = x+2;
	desc_y = y-17;
		
	n=0; row = 0; col = 0; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=1; row = 0; col = 1; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=2; row = 0; col = 2; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=3; row = 0; col = 3; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=4; row = 1; col = 0; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=5; row = 1; col = 1; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=6; row = 1; col = 2; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=7; row = 1; col = 3; w = 1; h = 2; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=8; row = 2; col = 0; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=9; row = 2; col = 1; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=10; row = 2; col = 2; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=11; row = 3; col = 0; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=12; row = 3; col = 1; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=13; row = 3; col = 2; w = 1; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=14; row = 3; col = 3; w = 1; h = 2; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=15; row = 4; col = 0; w = 2; h = 1;
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;
	n=16; row = 4; col = 2; w = 1; h = 1; 
	Pad[n] = ui_add_gadget_button( wnd, x+(bw*col), y+(bh*row), bw*w, bh*h, NULL, NULL );
	Pad[n]->canvas->cv_font = ui_small_font;

	HotKey[0] = KEY_CTRLED + KEY_NUMLOCK;
	HotKey[1] = KEY_CTRLED + KEY_PADDIVIDE;
	HotKey[2] = KEY_CTRLED + KEY_PADMULTIPLY;
	HotKey[3] = KEY_CTRLED + KEY_PADMINUS;
	HotKey[4] = KEY_CTRLED + KEY_PAD7;
	HotKey[5] = KEY_CTRLED + KEY_PAD8;
	HotKey[6] = KEY_CTRLED + KEY_PAD9;
	HotKey[7] = KEY_CTRLED + KEY_PADPLUS;
	HotKey[8] = KEY_CTRLED + KEY_PAD4;
	HotKey[9] = KEY_CTRLED + KEY_PAD5;
	HotKey[10] = KEY_CTRLED + KEY_PAD6;
	HotKey[11] = KEY_CTRLED + KEY_PAD1;
	HotKey[12] = KEY_CTRLED + KEY_PAD2;
	HotKey[13] = KEY_CTRLED + KEY_PAD3;
	HotKey[14] = KEY_CTRLED + KEY_PADENTER;
	HotKey[15] = KEY_CTRLED + KEY_PAD0;
	HotKey[16] = KEY_CTRLED + KEY_PADPERIOD;

	HotKey1[0] = KEY_SHIFTED + KEY_CTRLED + KEY_NUMLOCK;
	HotKey1[1] = KEY_SHIFTED + KEY_CTRLED + KEY_PADDIVIDE;
	HotKey1[2] = KEY_SHIFTED + KEY_CTRLED + KEY_PADMULTIPLY;
	HotKey1[3] = KEY_SHIFTED + KEY_CTRLED + KEY_PADMINUS;
	HotKey1[4] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD7;
	HotKey1[5] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD8;
	HotKey1[6] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD9;
	HotKey1[7] = KEY_SHIFTED + KEY_CTRLED + KEY_PADPLUS;
	HotKey1[8] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD4;
	HotKey1[9] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD5;
	HotKey1[10] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD6;
	HotKey1[11] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD1;
	HotKey1[12] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD2;
	HotKey1[13] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD3;
	HotKey1[14] = KEY_SHIFTED + KEY_CTRLED + KEY_PADENTER;
	HotKey1[15] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD0;
	HotKey1[16] = KEY_SHIFTED + KEY_CTRLED + KEY_PADPERIOD;

	active_pad = -1;

}


void ui_pad_deactivate()
{
	int i;

	for (i=0; i<17; i++ )
	{
		Pad[i]->text = NULL;
	}
}

static void ui_pad_set_active( int n )
{
	int np;
	char * name;
	int i, j;

	

	gr_set_current_canvas( NULL );
	gr_setcolor( CWHITE );
	gr_urect( desc_x, desc_y, desc_x+ 56*4-1, desc_y+15 );
	gr_set_fontcolor( CBLACK, CWHITE );
	gr_ustring( desc_x, desc_y, KeyPad[n]->description );
		
	for (i=0; i<17; i++ )
	{
		Pad[i]->text = KeyPad[n]->buttontext[i];
		Pad[i]->status = 1;
		Pad[i]->user_function = NULL;
		Pad[i]->dim_if_no_function = 1;
		Pad[i]->hotkey = -1;
				
		for (j=0; j< KeyPad[n]->numkeys; j++ )
		{
			if (HotKey[i] == KeyPad[n]->keycode[j] )
			{
				Pad[i]->hotkey =  HotKey[i];
				Pad[i]->user_function = func_nget( KeyPad[n]->function_number[j], &np, &name );
			}
			if (HotKey1[i] == KeyPad[n]->keycode[j] )
			{
				Pad[i]->hotkey1 =  HotKey1[i];
				Pad[i]->user_function1 = func_nget( KeyPad[n]->function_number[j], &np, &name );			}
		}
	}

	active_pad = n;
}

void ui_pad_goto(int n)
{
	if ( KeyPad[n] != NULL )
		ui_pad_set_active(n);
}

void ui_pad_goto_next()
{
	int i, si;

	i = active_pad + 1;
	si = i;
	
	while( KeyPad[i]==NULL )
	{
		i++;
		if (i >= MAX_NUM_PADS)
			i = 0;
		if (i == si )
			break;
	}
	ui_pad_set_active(i);
}

void ui_pad_goto_prev()
{
	int i, si;

	if (active_pad == -1 ) 
		active_pad = MAX_NUM_PADS;
	
	i = active_pad - 1;
	if (i<0) i= MAX_NUM_PADS - 1;
	si = i;
	
	while( KeyPad[i]==NULL )
	{
		i--;
		if (i < 0)
			i = MAX_NUM_PADS-1;
		if (i == active_pad )
			break;
	}
	ui_pad_set_active(i);
}

void ui_pad_read( int n, char * filename )
{
	char * ptr;
	char buffer[100];
	char text[100];
	FILE * infile;
	int linenumber = 0;
	int i;
	int keycode, functionnumber;

	infile = fopen( filename, "rt" );
	if (!infile) {
		Warning( "Couldn't find %s", filename );
		return;
	}
					  
	MALLOC( KeyPad[n], UI_KEYPAD, 1 );

			
	for (i=0; i < 17; i++ ) {
		MALLOC( KeyPad[n]->buttontext[i], char, 100 );
	}

	KeyPad[n]->numkeys = 0;

	for (i=0; i<100; i++ )
	{
		KeyPad[n]->keycode[i] = -1;
		KeyPad[n]->function_number[i] = 0;
	}

	while ( linenumber < 22)
	{
		fgets( buffer, 100, infile );
		REMOVE_EOL( buffer );

		switch( linenumber+1 )
		{
		case 1:
			strncpy( KeyPad[n]->description, buffer, 100 );
			break;
		//===================== ROW 0 ==============================
		case 3:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[0], "%s\n", text );
			LineParse( 2, text, buffer );
			sprintf( KeyPad[n]->buttontext[1], "%s\n", text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[2], "%s\n", text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[3], "%s\n", text );
			break;
		case 4:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[0], "%s%s\n", KeyPad[n]->buttontext[0],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[1], "%s%s\n", KeyPad[n]->buttontext[1],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[2], "%s%s\n", KeyPad[n]->buttontext[2],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[3], "%s%s\n", KeyPad[n]->buttontext[3],text );
			break;
		case 5:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[0], "%s%s", KeyPad[n]->buttontext[0],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[1], "%s%s", KeyPad[n]->buttontext[1],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[2], "%s%s", KeyPad[n]->buttontext[2],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[3], "%s%s", KeyPad[n]->buttontext[3],text );
			break;
		//===================== ROW 1 ==============================
		case 7:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[4], "%s\n", text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[5], "%s\n", text );
			LineParse( 3, text, buffer);	   
			sprintf( KeyPad[n]->buttontext[6], "%s\n", text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s\n", text );
			break;
		case 8:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[4], "%s%s\n", KeyPad[n]->buttontext[4],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[5], "%s%s\n", KeyPad[n]->buttontext[5],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[6], "%s%s\n", KeyPad[n]->buttontext[6],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s%s\n", KeyPad[n]->buttontext[7],text );
			break;
		case 9:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[4], "%s%s", KeyPad[n]->buttontext[4],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[5], "%s%s", KeyPad[n]->buttontext[5],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[6], "%s%s", KeyPad[n]->buttontext[6],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s%s\n", KeyPad[n]->buttontext[7],text );
			break;
		case 10:
			ptr = strrchr( buffer, 179 );
			*ptr = 0;
			ptr = strrchr( buffer, 180 );	ptr++;
			sprintf( KeyPad[n]->buttontext[7], "%s%s\n", KeyPad[n]->buttontext[7],ptr );
			break;
		//======================= ROW 2 ==============================
		case 11:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[8], "%s\n", text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[9], "%s\n", text );
			LineParse( 3, text, buffer);	   
			sprintf( KeyPad[n]->buttontext[10], "%s\n", text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s%s\n", KeyPad[n]->buttontext[7],text );
			break;
		case 12:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[8], "%s%s\n", KeyPad[n]->buttontext[8],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[9], "%s%s\n", KeyPad[n]->buttontext[9],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[10], "%s%s\n", KeyPad[n]->buttontext[10],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s%s\n", KeyPad[n]->buttontext[7],text );
			break;
		case 13:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[8], "%s%s", KeyPad[n]->buttontext[8],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[9], "%s%s", KeyPad[n]->buttontext[9],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[10], "%s%s", KeyPad[n]->buttontext[10],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[7], "%s%s", KeyPad[n]->buttontext[7],text );
			break;
		// ====================== ROW 3 =========================
		case 15:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[11], "%s\n", text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[12], "%s\n", text );
			LineParse( 3, text, buffer);	   
			sprintf( KeyPad[n]->buttontext[13], "%s\n", text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s\n", text );
			break;
		case 16:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[11], "%s%s\n", KeyPad[n]->buttontext[11],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[12], "%s%s\n", KeyPad[n]->buttontext[12],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[13], "%s%s\n", KeyPad[n]->buttontext[13],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s%s\n", KeyPad[n]->buttontext[14],text );
			break;
		case 17:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[11], "%s%s", KeyPad[n]->buttontext[11],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[12], "%s%s", KeyPad[n]->buttontext[12],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[13], "%s%s", KeyPad[n]->buttontext[13],text );
			LineParse( 4, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s%s\n", KeyPad[n]->buttontext[14],text );
			break;
		case 18:
			ptr = strrchr( buffer, 179 );
			*ptr = 0;
			ptr = strrchr( buffer, 180 ); ptr++;
			sprintf( KeyPad[n]->buttontext[14], "%s%s\n", KeyPad[n]->buttontext[14], ptr );
			break;
		//======================= ROW 4 =========================
		case 19:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[15], "%s\n", text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[16], "%s\n", text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s%s\n", KeyPad[n]->buttontext[14],text );
			break;
		case 20:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[15], "%s%s\n", KeyPad[n]->buttontext[15],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[16], "%s%s\n", KeyPad[n]->buttontext[16],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s%s\n", KeyPad[n]->buttontext[14],text );
			break;
		case 21:
			LineParse( 1, text, buffer );
			sprintf( KeyPad[n]->buttontext[15], "%s%s", KeyPad[n]->buttontext[15],text );
			LineParse( 2, text, buffer );	 
			sprintf( KeyPad[n]->buttontext[16], "%s%s", KeyPad[n]->buttontext[16],text );
			LineParse( 3, text, buffer );
			sprintf( KeyPad[n]->buttontext[14], "%s%s", KeyPad[n]->buttontext[14],text );
			break;
		}
										
		linenumber++;	
	}

	// Get the keycodes...

	while (fscanf( infile, " %s %s ", text, buffer )!=EOF)
	{	
		keycode = DecodeKeyText(text);
		functionnumber = func_get_index(buffer);
		if (functionnumber==-1)
		{
			Error( "Unknown function, %s, in %s\n", buffer, filename );
		} else if (keycode==-1)
		{
			Error( "Unknown keystroke, %s, in %s\n", text, filename );
			//MessageBox( -2, -2, 1, buffer, "Ok" );

		} else {
			KeyPad[n]->keycode[KeyPad[n]->numkeys] = keycode;
			KeyPad[n]->function_number[KeyPad[n]->numkeys] = functionnumber;
			KeyPad[n]->numkeys++;
		}
	}
	
	fclose(infile);

}














/*

	ui_pad_init(2);

	ui_pad_read( 0, "curve.pad" );
	ui_pad_read( 1, "segmove.pad" );
	
	
	//open window
		
		ui_pad_activate();

		
			ui_pad_goto( n );
			ui_pad_goto_next( n );
			ui_pad_goto_previous( n );
	

		ui_pad_deactivate();
		
	

	//close window

	ui_pad_close();

	exit();
*/

