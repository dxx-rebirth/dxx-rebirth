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
 * $Source: /cvs/cvsroot/d2x/arch/win32/include/key_arch.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 07:18:00 $
 *
 * Header for keyboard functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/12/07 05:38:31  donut
 * Fix mismatched arg types in the win32 code, work around byte being defined by windows headers as well as our own, and allow overriding of make vars.
 *
 * Revision 1.1  2000/01/17 05:58:38  donut
 * switched from multiply reimplemented/reduntant/buggy key.c for each arch to a single main/key.c that calls the much smaller arch-specific parts.  Also adds working emulated key repeat support.
 *
 * Revision 1.1.1.1  1999/06/14 22:01:23  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.19  1994/10/24  13:58:12  john
 * Hacked in support for pause key onto code 0x61.
 * 
 * Revision 1.18  1994/10/21  15:17:10  john
 * Added KEY_PRINT_SCREEN
 * 
 * Revision 1.17  1994/08/31  12:22:13  john
 * Added KEY_DEBUGGED
 * 
 * Revision 1.16  1994/08/24  18:53:50  john
 * Made Cyberman read like normal mouse; added dpmi module; moved
 * mouse from assembly to c. Made mouse buttons return time_down.
 * 
 * Revision 1.15  1994/08/18  14:56:16  john
 * *** empty log message ***
 * 
 * Revision 1.14  1994/08/08  10:43:24  john
 * Recorded when a key was pressed for key_inkey_time.
 * 
 * Revision 1.13  1994/06/17  17:17:28  john
 * Added keyd_time_last_key_was_pressed or something like that.
 * 
 * Revision 1.12  1994/04/29  12:14:19  john
 * Locked all memory used during interrupts so that program
 * won't hang when using virtual memory.
 * 
 * Revision 1.11  1994/02/17  15:57:14  john
 * Changed key libary to C.
 * 
 * Revision 1.10  1994/01/31  08:34:09  john
 * Fixed reversed lshift/rshift keys.
 * 
 * Revision 1.9  1994/01/18  10:58:17  john
 * *** empty log message ***
 * 
 * Revision 1.8  1993/10/16  19:24:43  matt
 * Added new function key_clear_times() & key_clear_counts()
 * 
 * Revision 1.7  1993/10/15  10:17:09  john
 * added keyd_last_key_pressed and released for use with recorder.
 * 
 * Revision 1.6  1993/10/06  16:20:37  john
 * fixed down arrow bug
 * 
 * Revision 1.5  1993/10/04  13:26:42  john
 * changed the #defines for scan codes.
 * 
 * Revision 1.4  1993/09/28  11:35:20  john
 * added key_peekkey
 * 
 * Revision 1.3  1993/09/20  18:36:43  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/07/10  13:10:39  matt
 * Initial revision
 * 
 *
 */

#ifndef _WIN32_KEY_H
#define _WIN32_KEY_H

//changed key defines to use DIK_* - 2000/01/14 Matt Mueller.  -heh, a bunch were wrong.
#define byte w32_byte
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dinput.h> // For DIK_*
#undef byte

#define ARCH_KEY_0           DIK_0
#define ARCH_KEY_1           DIK_1
#define ARCH_KEY_2           DIK_2
#define ARCH_KEY_3           DIK_3
#define ARCH_KEY_4           DIK_4
#define ARCH_KEY_5           DIK_5
#define ARCH_KEY_6           DIK_6
#define ARCH_KEY_7           DIK_7
#define ARCH_KEY_8           DIK_8
#define ARCH_KEY_9           DIK_9

#define ARCH_KEY_A           DIK_A
#define ARCH_KEY_B           DIK_B
#define ARCH_KEY_C           DIK_C
#define ARCH_KEY_D           DIK_D
#define ARCH_KEY_E           DIK_E
#define ARCH_KEY_F           DIK_F
#define ARCH_KEY_G           DIK_G
#define ARCH_KEY_H           DIK_H
#define ARCH_KEY_I           DIK_I
#define ARCH_KEY_J           DIK_J
#define ARCH_KEY_K           DIK_K
#define ARCH_KEY_L           DIK_L
#define ARCH_KEY_M           DIK_M
#define ARCH_KEY_N           DIK_N
#define ARCH_KEY_O           DIK_O
#define ARCH_KEY_P           DIK_P
#define ARCH_KEY_Q           DIK_Q
#define ARCH_KEY_R           DIK_R
#define ARCH_KEY_S           DIK_S
#define ARCH_KEY_T           DIK_T
#define ARCH_KEY_U           DIK_U
#define ARCH_KEY_V           DIK_V
#define ARCH_KEY_W           DIK_W
#define ARCH_KEY_X           DIK_X
#define ARCH_KEY_Y           DIK_Y
#define ARCH_KEY_Z           DIK_Z

#define ARCH_KEY_MINUS       DIK_MINUS
#define ARCH_KEY_EQUAL       DIK_EQUALS
#define ARCH_KEY_DIVIDE      DIK_SLASH
#define ARCH_KEY_SLASH       DIK_BACKSLASH
#define ARCH_KEY_COMMA       DIK_COMMA
#define ARCH_KEY_PERIOD      DIK_PERIOD
#define ARCH_KEY_SEMICOL     DIK_SEMICOLON

#define ARCH_KEY_LBRACKET    DIK_LBRACKET
#define ARCH_KEY_RBRACKET    DIK_RBRACKET

#define ARCH_KEY_RAPOSTRO    DIK_APOSTROPHE
#define ARCH_KEY_LAPOSTRO    DIK_GRAVE

#define ARCH_KEY_ESC         DIK_ESCAPE
#define ARCH_KEY_ENTER       DIK_RETURN
#define ARCH_KEY_BACKSP      DIK_BACK
#define ARCH_KEY_TAB         DIK_TAB
#define ARCH_KEY_SPACEBAR    DIK_SPACE

#define ARCH_KEY_NUMLOCK     DIK_NUMLOCK
#define ARCH_KEY_SCROLLOCK   DIK_SCROLL
#define ARCH_KEY_CAPSLOCK    DIK_CAPSLOCK

#define ARCH_KEY_LSHIFT      DIK_LSHIFT
#define ARCH_KEY_RSHIFT      DIK_RSHIFT

#define ARCH_KEY_LALT        DIK_LALT
#define ARCH_KEY_RALT        DIK_RALT

#define ARCH_KEY_LCTRL       DIK_LCONTROL
#define ARCH_KEY_RCTRL       DIK_RCONTROL

#define ARCH_KEY_F1          DIK_F1
#define ARCH_KEY_F2          DIK_F2
#define ARCH_KEY_F3          DIK_F3
#define ARCH_KEY_F4          DIK_F4
#define ARCH_KEY_F5          DIK_F5
#define ARCH_KEY_F6          DIK_F6
#define ARCH_KEY_F7          DIK_F7
#define ARCH_KEY_F8          DIK_F8
#define ARCH_KEY_F9          DIK_F9
#define ARCH_KEY_F10         DIK_F10
#define ARCH_KEY_F11         DIK_F11
#define ARCH_KEY_F12         DIK_F12

#define ARCH_KEY_PAD0        DIK_NUMPAD0
#define ARCH_KEY_PAD1        DIK_NUMPAD1
#define ARCH_KEY_PAD2        DIK_NUMPAD2
#define ARCH_KEY_PAD3        DIK_NUMPAD3
#define ARCH_KEY_PAD4        DIK_NUMPAD4
#define ARCH_KEY_PAD5        DIK_NUMPAD5
#define ARCH_KEY_PAD6        DIK_NUMPAD6
#define ARCH_KEY_PAD7        DIK_NUMPAD7
#define ARCH_KEY_PAD8        DIK_NUMPAD8
#define ARCH_KEY_PAD9        DIK_NUMPAD9
#define ARCH_KEY_PADMINUS    DIK_NUMPADMINUS
#define ARCH_KEY_PADPLUS     DIK_NUMPADPLUS
#define ARCH_KEY_PADPERIOD   DIK_NUMPADPERIOD
#define ARCH_KEY_PADDIVIDE   DIK_NUMPADSLASH
#define ARCH_KEY_PADMULTIPLY DIK_NUMPADSTAR
#define ARCH_KEY_PADENTER    DIK_NUMPADENTER

#define ARCH_KEY_INSERT      DIK_INSERT
#define ARCH_KEY_HOME        DIK_HOME
#define ARCH_KEY_PAGEUP      DIK_PGUP
#define ARCH_KEY_DELETE      DIK_DELETE
#define ARCH_KEY_END         DIK_END
#define ARCH_KEY_PAGEDOWN    DIK_PGDN
#define ARCH_KEY_UP          DIK_UP
#define ARCH_KEY_DOWN        DIK_DOWN
#define ARCH_KEY_LEFT        DIK_LEFT
#define ARCH_KEY_RIGHT       DIK_RIGHT

#define ARCH_KEY_PRINT_SCREEN	DIK_SYSRQ
//there doesn't seem to be a DIK_PAUSE.
#define ARCH_KEY_PAUSE		197

#endif
