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

/*
 *
 * Header for keyboard functions
 *
 */

#ifndef _KEY_H
#define _KEY_H 

#include <SDL/SDL_keysym.h>
#include "pstypes.h"
#include "fix.h"
#include "event.h"

#define KEY_BUFFER_SIZE 16
#define KEY_REPEAT_DELAY 400
#define KEY_REPEAT_INTERVAL 50

//==========================================================================
// This installs the int9 vector and initializes the keyboard in buffered
// ASCII mode. key_close simply undoes that.
extern void key_init();
extern void key_close();

// Time in seconds when last key was pressed...
extern fix64 keyd_time_when_last_pressed;

// Stores Unicode values registered in one event_loop call
extern unsigned char unicode_frame_buffer[KEY_BUFFER_SIZE];

extern void key_flush();    // Clears the 256 char buffer
extern int event_key_get(d_event *event);	// Get the keycode from the EVENT_KEY_COMMAND event
extern int event_key_get_raw(d_event *event);	// same as above but without mod states
extern unsigned char key_ascii();

// Set to 1 if the key is currently down, else 0
extern volatile unsigned char keyd_pressed[256];
extern volatile unsigned char keyd_last_pressed;
extern volatile unsigned char keyd_last_released;

extern void key_toggle_repeat(int enable);

// for key_ismodlck
#define KEY_ISMOD	1
#define KEY_ISLCK	2

#define KEY_SHIFTED     0x100
#define KEY_ALTED       0x200
#define KEY_CTRLED      0x400
#define KEY_DEBUGGED	0x800
#define KEY_METAED		0x1000
#define KEY_COMMAND		KEY_METAED	// Mac meta key

#define KEY_0           0x0B
#define KEY_1           0x02
#define KEY_2           0x03
#define KEY_3           0x04
#define KEY_4           0x05
#define KEY_5           0x06
#define KEY_6           0x07
#define KEY_7           0x08
#define KEY_8           0x09
#define KEY_9           0x0A

#define KEY_A           0x1E
#define KEY_B           0x30
#define KEY_C           0x2E
#define KEY_D           0x20
#define KEY_E           0x12
#define KEY_F           0x21
#define KEY_G           0x22
#define KEY_H           0x23
#define KEY_I           0x17
#define KEY_J           0x24
#define KEY_K           0x25
#define KEY_L           0x26
#define KEY_M           0x32
#define KEY_N           0x31
#define KEY_O           0x18
#define KEY_P           0x19
#define KEY_Q           0x10
#define KEY_R           0x13
#define KEY_S           0x1F
#define KEY_T           0x14
#define KEY_U           0x16
#define KEY_V           0x2F
#define KEY_W           0x11
#define KEY_X           0x2D
#define KEY_Y           0x15
#define KEY_Z           0x2C

#define KEY_MINUS       0x0C
#define KEY_EQUAL       0x0D
//Note: this is what we normally think of as slash:
#define KEY_DIVIDE      0x35
//Note: this is BACKslash:
#define KEY_SLASH       0x2B
#define KEY_COMMA       0x33
#define KEY_PERIOD      0x34
#define KEY_SEMICOL     0x27

#define KEY_LBRACKET    0x1A
#define KEY_RBRACKET    0x1B

#define KEY_RAPOSTRO    0x28
#define KEY_LAPOSTRO    0x29

#define KEY_ESC         0x01
#define KEY_ENTER       0x1C
#define KEY_BACKSP      0x0E
#define KEY_TAB         0x0F
#define KEY_SPACEBAR    0x39

#define KEY_NUMLOCK     0x45
#define KEY_SCROLLOCK   0x46
#define KEY_CAPSLOCK    0x3A

#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36

#define KEY_LALT        0x38
#define KEY_RALT        0xB8

#define KEY_LCTRL       0x1D
#define KEY_RCTRL       0x9D
#define KEY_LMETA		0x9E
#define KEY_RMETA		0x9F

#define KEY_F1          0x3B
#define KEY_F2          0x3C
#define KEY_F3          0x3D
#define KEY_F4          0x3E
#define KEY_F5          0x3F
#define KEY_F6          0x40
#define KEY_F7          0x41
#define KEY_F8          0x42
#define KEY_F9          0x43
#define KEY_F10         0x44
#define KEY_F11         0x57
#define KEY_F12         0x58

#define KEY_PAD0        0x52
#define KEY_PAD1        0x4F
#define KEY_PAD2        0x50
#define KEY_PAD3        0x51
#define KEY_PAD4        0x4B
#define KEY_PAD5        0x4C
#define KEY_PAD6        0x4D
#define KEY_PAD7        0x47
#define KEY_PAD8        0x48
#define KEY_PAD9        0x49
#define KEY_PADMINUS    0x4A
#define KEY_PADPLUS     0x4E
#define KEY_PADPERIOD   0x53
#define KEY_PADDIVIDE   0xB5
#define KEY_PADMULTIPLY 0x37
#define KEY_PADENTER    0x9C

#define KEY_INSERT      0xD2
#define KEY_HOME        0xC7
#define KEY_PAGEUP      0xC9
#define KEY_DELETE      0xD3
#define KEY_END         0xCF
#define KEY_PAGEDOWN    0xD1
#define KEY_UP          0xC8
#define KEY_DOWN        0xD0
#define KEY_LEFT        0xCB
#define KEY_RIGHT       0xCD

#define KEY_PRINT_SCREEN	0xB7

#define KEY_PAUSE			0x61

typedef struct key_props {
	const char *key_text;
	unsigned char ascii_value;
	SDLKey sym;
} key_props;

extern const key_props key_properties[256];

#endif
