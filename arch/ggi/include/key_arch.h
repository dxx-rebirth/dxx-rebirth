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
 * $Source: /cvs/cvsroot/d2x/arch/ggi/include/key_arch.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 07:18:00 $
 *
 * Header for keyboard functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2000/01/17 05:58:38  donut
 * switched from multiply reimplemented/reduntant/buggy key.c for each arch to a single main/key.c that calls the much smaller arch-specific parts.  Also adds working emulated key repeat support.
 *
 * Revision 1.2  1999/07/10 02:59:06  donut
 * more from orulz
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

#ifndef _GII_KEY_H
#define _GII_KEY_H

#include <ggi/gii.h>

#define ARCH_KEY_0		GIIUC_0
#define ARCH_KEY_1		GIIUC_1
#define ARCH_KEY_2		GIIUC_2
#define ARCH_KEY_3		GIIUC_3
#define ARCH_KEY_4		GIIUC_4
#define ARCH_KEY_5		GIIUC_5
#define ARCH_KEY_6		GIIUC_6
#define ARCH_KEY_7		GIIUC_7
#define ARCH_KEY_8		GIIUC_8
#define ARCH_KEY_9		GIIUC_9

#define ARCH_KEY_A		GIIUC_A
#define ARCH_KEY_B		GIIUC_B
#define ARCH_KEY_C		GIIUC_C
#define ARCH_KEY_D		GIIUC_D
#define ARCH_KEY_E		GIIUC_E
#define ARCH_KEY_F		GIIUC_F
#define ARCH_KEY_G		GIIUC_G
#define ARCH_KEY_H		GIIUC_H
#define ARCH_KEY_I		GIIUC_I
#define ARCH_KEY_J		GIIUC_J
#define ARCH_KEY_K		GIIUC_K
#define ARCH_KEY_L		GIIUC_L
#define ARCH_KEY_M		GIIUC_M
#define ARCH_KEY_N		GIIUC_N
#define ARCH_KEY_O		GIIUC_O
#define ARCH_KEY_P		GIIUC_P
#define ARCH_KEY_Q		GIIUC_Q
#define ARCH_KEY_R		GIIUC_R
#define ARCH_KEY_S		GIIUC_S
#define ARCH_KEY_T		GIIUC_T
#define ARCH_KEY_U		GIIUC_U
#define ARCH_KEY_V		GIIUC_V
#define ARCH_KEY_W		GIIUC_W
#define ARCH_KEY_X		GIIUC_X
#define ARCH_KEY_Y		GIIUC_Y
#define ARCH_KEY_Z		GIIUC_Z

#define ARCH_KEY_MINUS		GIIUC_Minus
#define ARCH_KEY_EQUAL		GIIUC_Equal
#define ARCH_KEY_DIVIDE		GIIUC_Slash
#define ARCH_KEY_SLASH		GIIUC_BackSlash
#define ARCH_KEY_COMMA		GIIUC_Comma
#define ARCH_KEY_PERIOD		GIIUC_Period
#define ARCH_KEY_SEMICOL		GIIUC_Semicolon

#define ARCH_KEY_LBRACKET		GIIUC_BracketLeft
#define ARCH_KEY_RBRACKET		GIIUC_BracketRight

#define ARCH_KEY_RAPOSTRO		GIIUC_Apostrophe
#define ARCH_KEY_LAPOSTRO		GIIUC_Grave

#define ARCH_KEY_ESC		GIIUC_Escape
#define ARCH_KEY_ENTER		GIIK_Enter
#define ARCH_KEY_BACKSP		GIIUC_BackSpace
#define ARCH_KEY_TAB		GIIUC_Tab
#define ARCH_KEY_SPACEBAR		GIIUC_Space

#define ARCH_KEY_NUMLOCK		GIIK_NumLock
#define ARCH_KEY_SCROLLOCK		GIIK_ScrollLock
#define ARCH_KEY_CAPSLOCK		GIIK_CapsLock

#define ARCH_KEY_LSHIFT		GIIK_ShiftL
#define ARCH_KEY_RSHIFT		GIIK_ShiftR

#define ARCH_KEY_LALT		GIIK_AltL
#define ARCH_KEY_RALT		GIIK_AltR

#define ARCH_KEY_LCTRL		GIIK_CtrlL
#define ARCH_KEY_RCTRL		GIIK_CtrlR

#define ARCH_KEY_F1		GIIK_F1
#define ARCH_KEY_F2		GIIK_F2
#define ARCH_KEY_F3		GIIK_F3
#define ARCH_KEY_F4		GIIK_F4
#define ARCH_KEY_F5		GIIK_F5
#define ARCH_KEY_F6		GIIK_F6
#define ARCH_KEY_F7		GIIK_F7
#define ARCH_KEY_F8		GIIK_F8
#define ARCH_KEY_F9		GIIK_F9
#define ARCH_KEY_F10		GIIK_F10
#define ARCH_KEY_F11		GIIK_F11
#define ARCH_KEY_F12		GIIK_F12

#define ARCH_KEY_PAD0		GIIK_P0
#define ARCH_KEY_PAD1		GIIK_P1
#define ARCH_KEY_PAD2		GIIK_P2
#define ARCH_KEY_PAD3		GIIK_P3
#define ARCH_KEY_PAD4		GIIK_P4
#define ARCH_KEY_PAD5		GIIK_P5
#define ARCH_KEY_PAD6		GIIK_P6
#define ARCH_KEY_PAD7		GIIK_P7
#define ARCH_KEY_PAD8		GIIK_P8
#define ARCH_KEY_PAD9		GIIK_P9
#define ARCH_KEY_PADMINUS		GIIK_PMinus
#define ARCH_KEY_PADPLUS		GIIK_PPlus
#define ARCH_KEY_PADPERIOD		GIIK_PDecimal
#define ARCH_KEY_PADDIVIDE		GIIK_PSlash
#define ARCH_KEY_PADMULTIPLY		GIIK_PAsterisk
#define ARCH_KEY_PADENTER		GIIK_PEnter

#define ARCH_KEY_INSERT		GIIK_Insert
#define ARCH_KEY_HOME		GIIK_Home
#define ARCH_KEY_PAGEUP		GIIK_PageUp
#define ARCH_KEY_DELETE		GIIK_Delete
#define ARCH_KEY_END		GIIK_End
#define ARCH_KEY_PAGEDOWN		GIIK_PageDown
#define ARCH_KEY_UP		GIIK_Up
#define ARCH_KEY_DOWN		GIIK_Down
#define ARCH_KEY_LEFT		GIIK_Left
#define ARCH_KEY_RIGHT		GIIK_Right

#define ARCH_KEY_PRINT_SCREEN		GIIK_PrintScreen
#define ARCH_KEY_PAUSE		GIIK_Pause

#endif
