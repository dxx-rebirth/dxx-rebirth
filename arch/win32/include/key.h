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
 * $Source: /cvs/cvsroot/d2x/arch/win32/include/key.h,v $
 * $Revision: 1.2 $
 * $Author: schaffner $
 * $Date: 2004-08-28 23:17:45 $
 *
 * Header for keyboard functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 22:01:23  donut
 * Import of d1x 1.37 source.
 *
 */

#ifndef _KEY_H
#define _KEY_H 

#include "fix.h"
#include "types.h"
#include <ctype.h> // For 'toupper'

//==========================================================================
// This installs the int9 vector and initializes the keyboard in buffered
// ASCII mode. key_close simply undoes that.
extern void key_init();
extern void key_close();

//==========================================================================
// These are configuration parameters to setup how the buffer works.
// set keyd_buffer_type to 0 for no key buffering.
// set it to 1 and it will buffer scancodes.
extern unsigned char keyd_buffer_type;
extern unsigned char keyd_repeat;	 // 1=allow repeating, 0=dont allow repeat

// keyd_editor_mode... 0=game mode, 1=editor mode.
// Editor mode makes key_down_time always return 0 if modifiers are down.
extern unsigned char keyd_editor_mode;

// Time in seconds when last key was pressed...
extern volatile int keyd_time_when_last_pressed;

//==========================================================================
// These are the "buffered" keypress routines.  Use them by setting the
// "keyd_buffer_type" variable.

extern void key_flush();    // Clears the 256 char buffer
extern int key_checkch();   // Returns 1 if a char is waiting
extern int key_getch();     // Gets key if one waiting other waits for one.
extern int key_inkey();     // Gets key if one, other returns 0.
extern int key_inkey_time(fix *time);     // Same as inkey, but returns the time the key was pressed down.
extern int key_peekkey();   // Same as inkey, but doesn't remove key from buffer.

extern unsigned char key_to_ascii(int keycode );
extern char *key_name(int keycode); // Convert keycode to the name of the key

extern void key_debug();    // Does an INT3

//==========================================================================
// These are the unbuffered routines. Index by the keyboard scancode.

// Set to 1 if the key is currently down, else 0
extern volatile unsigned char keyd_pressed[];
extern volatile unsigned char keyd_last_pressed;
extern volatile unsigned char keyd_last_released;

// Returns the seconds this key has been down since last call.
extern fix key_down_time(int scancode);

// Returns number of times key has went from up to down since last call.
extern unsigned int key_down_count(int scancode);

// Returns number of times key has went from down to up since last call.
extern unsigned int key_up_count(int scancode);

// Clears the times & counts used by the above functions
// Took out... use key_flush();
//void key_clear_times();
//void key_clear_counts();

extern char * key_text[256];

#define KEY_SHIFTED     0x100
#define KEY_ALTED       0x200
#define KEY_CTRLED      0x400
#define KEY_DEBUGGED	0x800

#define KEY_0           11
#define KEY_1           2
#define KEY_2           3
#define KEY_3           4
#define KEY_4           5
#define KEY_5           6
#define KEY_6           7
#define KEY_7           8
#define KEY_8           9
#define KEY_9           10

#define KEY_A           30
#define KEY_B           48
#define KEY_C           46
#define KEY_D           32
#define KEY_E           18
#define KEY_F           33
#define KEY_G           34
#define KEY_H           35
#define KEY_I           23
#define KEY_J           36
#define KEY_K           37
#define KEY_L           38
#define KEY_M           50
#define KEY_N           49
#define KEY_O           24
#define KEY_P           25
#define KEY_Q           16
#define KEY_R           19
#define KEY_S           31
#define KEY_T           20
#define KEY_U           22
#define KEY_V           47
#define KEY_W           17
#define KEY_X           45
#define KEY_Y           21
#define KEY_Z           44

#define KEY_MINUS       12
#define KEY_EQUAL       13
#define KEY_DIVIDE      43
#define KEY_SLASH       28
#define KEY_COMMA       51
#define KEY_PERIOD      52
#define KEY_SEMICOL     39

#define KEY_LBRACKET    26
#define KEY_RBRACKET    27

#define KEY_RAPOSTRO    40
#define KEY_LAPOSTRO    41

#define KEY_ESC         1
#define KEY_ENTER       28
#define KEY_BACKSP      14
#define KEY_TAB         15
#define KEY_SPACEBAR    57

#define KEY_NUMLOCK     69
#define KEY_SCROLLOCK   70
#define KEY_CAPSLOCK    58

#define KEY_LSHIFT      42
#define KEY_RSHIFT      54

#define KEY_LALT        56
#define KEY_RALT        194

#define KEY_LCTRL       29
#define KEY_RCTRL       157

#define KEY_F1          59
#define KEY_F2          60
#define KEY_F3          61
#define KEY_F4          62
#define KEY_F5          63
#define KEY_F6          64
#define KEY_F7          65
#define KEY_F8          66
#define KEY_F9          67
#define KEY_F10         68
#define KEY_F11         87
#define KEY_F12         88

#define KEY_PAD0        82
#define KEY_PAD1        79
#define KEY_PAD2        80
#define KEY_PAD3        81
#define KEY_PAD4        75
#define KEY_PAD5        76
#define KEY_PAD6        77
#define KEY_PAD7        71
#define KEY_PAD8        72
#define KEY_PAD9        73
#define KEY_PADMINUS    74
#define KEY_PADPLUS     78
#define KEY_PADPERIOD   83
#define KEY_PADDIVIDE   81
#define KEY_PADMULTIPLY 55
#define KEY_PADENTER    156

#define KEY_INSERT      210
#define KEY_HOME        209
#define KEY_PAGEUP      221
#define KEY_DELETE      211
#define KEY_END         217
#define KEY_PAGEDOWN    219
#define KEY_UP          200
#define KEY_DOWN        208
#define KEY_LEFT        203
#define KEY_RIGHT       205

#define KEY_PRINT_SCREEN	165
#define KEY_PAUSE		143

#endif
