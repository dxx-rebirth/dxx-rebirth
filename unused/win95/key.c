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


#pragma off (unreferenced)
static char rcsid[] = "$Id: key.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

//#define WATCOM_10

#include "mono.h"
#include "error.h"
#include "key.h"
#include "timer.h"

#define KEY_BUFFER_SIZE 16

//-------- Variable accessed by outside functions ---------
unsigned char				keyd_print_screen=0;

unsigned char 				keyd_buffer_type;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
unsigned char 				keyd_repeat;
unsigned char 				keyd_editor_mode;
volatile unsigned char 	keyd_last_pressed;
volatile unsigned char 	keyd_last_released;
volatile unsigned char	keyd_pressed[256];
volatile int				keyd_time_when_last_pressed;

typedef struct keyboard	{
	unsigned short		keybuffer[KEY_BUFFER_SIZE];
	fix					time_pressed[KEY_BUFFER_SIZE];
	fix					TimeKeyWentDown[256];
	fix					TimeKeyHeldDown[256];
	unsigned int		NumDowns[256];
	unsigned int		NumUps[256];
	unsigned int 		keyhead, keytail;
	unsigned char 		E0Flag;
	unsigned char 		E1Flag;
} keyboard;

static volatile keyboard key_data;

static unsigned char Installed=0;

unsigned char ascii_table[128] = 
{ 255, 255, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',255,255,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 255, 255,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`',
  255, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 255,'*',
  255, ' ', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,255,255,
  255, 255, 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255 };

unsigned char shifted_ascii_table[128] = 
{ 255, 255, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',255,255,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 255, 255,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 
  255, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 255,255,
  255, ' ', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,255,255,
  255, 255, 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255 };


//	Initialization and Cleanup Routines

void key_init()
{
	keyd_time_when_last_pressed = timer_get_fixed_seconds();
	keyd_buffer_type = 1;
	keyd_repeat = 1;
	key_data.E0Flag = 0;
	key_data.E1Flag = 0;

	// Clear the keyboard array
	key_flush();
	if (!Installed) Installed = 1;
}


void key_close()
{
	if (!Installed) return;
	key_clear_bios_buffer_all();
}


extern char key_to_ascii(int keycode )
{
	int shifted;

	shifted = keycode & KEY_SHIFTED;
	keycode &= 0xFF;

	if ( keycode>=127 )
		return 255;

	if (shifted)
		return shifted_ascii_table[keycode];
	else
		return ascii_table[keycode];
}

void key_clear_bios_buffer_all()
{

}

void key_clear_bios_buffer()
{

}

void key_flush()
{
	int i;
	fix CurTime;

	// Clear the BIOS buffer
	key_clear_bios_buffer();

	key_data.keyhead = key_data.keytail = 0;

	//Clear the keyboard buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )	{
		key_data.keybuffer[i] = 0;
		key_data.time_pressed[i] = 0;
	}
	
	//Clear the keyboard array

	CurTime =timer_get_fixed_secondsX();

	for (i=0; i<256; i++ )	{
		keyd_pressed[i] = 0;
		key_data.TimeKeyWentDown[i] = CurTime;
		key_data.TimeKeyHeldDown[i] = 0;
		key_data.NumDowns[i]=0;
		key_data.NumUps[i]=0;
	}
	keyd_print_screen = 0;
}

int add_one( int n )
{
	n++;
	if ( n >= KEY_BUFFER_SIZE ) n=0;
	return n;
}

// Returns 1 if character waiting... 0 otherwise
int key_checkch()
{
	int is_one_waiting = 0;

	key_clear_bios_buffer();

	if (key_data.keytail!=key_data.keyhead)
		is_one_waiting = 1;

	return is_one_waiting;
}

int key_inkey()
{
	int key = 0;

	key_clear_bios_buffer();

	if (key_data.keytail!=key_data.keyhead)	{
		key = key_data.keybuffer[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}

	return key;
}

int key_inkey_time(fix * time)
{
	int key = 0;

	key_clear_bios_buffer();

	if (key_data.keytail!=key_data.keyhead)	{
		key = key_data.keybuffer[key_data.keyhead];
		*time = key_data.time_pressed[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}

	return key;
}


void key_putkey (unsigned short keycode)
 {
  // this function simulates a key stroke entered
  char temp;
  

  temp = key_data.keytail+1;
  if ( temp >= KEY_BUFFER_SIZE ) temp=0;
  if (temp!=key_data.keyhead)	
    {
  		key_data.keybuffer[key_data.keytail] = keycode;
  		key_data.keytail = temp;
    }
 }  


int key_peekkey()
{
	int key = 0;

	key_clear_bios_buffer();

	if (key_data.keytail!=key_data.keyhead)	{
		key = key_data.keybuffer[key_data.keyhead];
	}

	return key;
}

// If not installed, uses BIOS and returns getch();
//	Else returns pending key (or waits for one if none waiting).
int key_getch()
{
	int dummy=0;
	
	if (!Installed)
		return getch();

	while (!key_checkch())
		dummy++;
	return key_inkey();
}

unsigned int key_get_shift_status()
{
	unsigned int shift_status = 0;


	key_clear_bios_buffer();

	if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT] )
		shift_status |= KEY_SHIFTED;

	if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT] )
		shift_status |= KEY_ALTED;

	if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL] )
		shift_status |= KEY_CTRLED;

#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE])
		shift_status |=KEY_DEBUGGED;
#endif


	return shift_status;
}

// Returns the number of seconds this key has been down since last call.
fix key_down_time(int scancode)	{
	fix time_down, time;

	if ((scancode<0)|| (scancode>255)) return 0;

#ifndef NDEBUG
	if (keyd_editor_mode && key_get_shift_status() )
		return 0;  
#endif

	if ( !keyd_pressed[scancode] )	{
		time_down = key_data.TimeKeyHeldDown[scancode];
		key_data.TimeKeyHeldDown[scancode] = 0;
	} else	{
		time = timer_get_fixed_secondsX();
		time_down =  time - key_data.TimeKeyWentDown[scancode];
		key_data.TimeKeyWentDown[scancode] = time;
	}

	return time_down;
}

// Returns number of times key has went from up to down since last call.
unsigned int key_down_count(int scancode)	{
	int n;

	if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.NumDowns[scancode];
	key_data.NumDowns[scancode] = 0;

	return n;
}


// Returns number of times key has went from down to up since last call.
unsigned int key_up_count(int scancode)	{
	int n;

	if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.NumUps[scancode];
	key_data.NumUps[scancode] = 0;

	return n;
}

// Use intrinsic forms so that we stay in the locked interrup code.

void Int5();
#pragma aux Int5 = "int 5";



// Send keyboard message to Descent keyboard system.

void send_key_msg(UINT msg, WPARAM vkeycode, LPARAM keypack)
{
	unsigned char scancode, temp;
	unsigned short keycode;   
	
//	Extract 8-bit OEM scancode
	scancode = (char)((keypack >> 16) & 0xff);

	if (vkeycode == VK_SNAPSHOT) {
		scancode = KEY_PRINT_SCREEN;		
		keyd_print_screen = 1;
	}
	else if (keypack & 0x01000000) scancode |= 0x80;
	if (vkeycode == VK_PAUSE) scancode = KEY_PAUSE;

	switch (msg)
	{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:					// Keyboard down
		// Handle Pauses the easy way.
			keyd_last_pressed = scancode;
			keyd_time_when_last_pressed = timer_get_fixed_secondsX();
			
			if (!keyd_pressed[scancode]) {
			// First time key is down.
				key_data.TimeKeyWentDown[scancode] = timer_get_fixed_secondsX();
				keyd_pressed[scancode] = 1;
				key_data.NumDowns[scancode]++;

			#ifndef NDEBUG
				if ((keyd_pressed[KEY_LSHIFT]) && (scancode == KEY_BACKSP)) {
					keyd_pressed[KEY_LSHIFT] = 0;
				//	Int5();
				}
			#endif
			}
			else if (!keyd_repeat) {
			// don't buffer repeating key if repeat mode is off	
				scancode = 0xaa;
			}
			if (scancode != 0xaa) {
				keycode = (unsigned short)scancode;
				
				if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) 
					keycode |= KEY_SHIFTED;

				if (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT]) 
					keycode |= KEY_ALTED;			

				if (keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
					keycode |= KEY_CTRLED;
			
			#ifndef NDEBUG
				if (keyd_pressed[KEY_DELETE])
					keycode |= KEY_DEBUGGED;
			#endif

				temp = key_data.keytail+1;
				if (temp >= KEY_BUFFER_SIZE) temp = 0;

				if (temp!= key_data.keyhead) {
					key_data.keybuffer[key_data.keytail] = keycode;
					key_data.time_pressed[key_data.keytail] = keyd_time_when_last_pressed;
					key_data.keytail = temp;
				}
			}
			break;
			
		case WM_SYSKEYUP:
		case WM_KEYUP:						// handle key ups
			keyd_last_released = scancode;
			keyd_pressed[scancode] = 0;
			key_data.NumUps[scancode]++;
			temp = 0;
			temp |= keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT];
			temp |= keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT];
			temp |= keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL];
		
		#ifndef NDEBUG
			temp |= keyd_pressed[KEY_DELETE];
			if (!(keyd_editor_mode && temp))
		#endif 
				// NOTICE LINK TO ABOVE IF!!!!
				key_data.TimeKeyHeldDown[scancode] += timer_get_fixed_secondsX() - 
															key_data.TimeKeyWentDown[scancode];
			break;
	}	// switch (msg)

}
