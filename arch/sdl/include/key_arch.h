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
 * $Source: /cvs/cvsroot/d2x/arch/sdl/include/key_arch.h,v $
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
 * Revision 1.1.1.1  1999/06/14 22:02:01  donut
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

#ifndef _SDL_KEY_H
#define _SDL_KEY_H

#include <SDL/SDL.h>

#define ARCH_KEY_0           SDLK_0
#define ARCH_KEY_1           SDLK_1
#define ARCH_KEY_2           SDLK_2
#define ARCH_KEY_3           SDLK_3
#define ARCH_KEY_4           SDLK_4
#define ARCH_KEY_5           SDLK_5
#define ARCH_KEY_6           SDLK_6
#define ARCH_KEY_7           SDLK_7
#define ARCH_KEY_8           SDLK_8
#define ARCH_KEY_9           SDLK_9

#define ARCH_KEY_A           SDLK_a
#define ARCH_KEY_B           SDLK_b
#define ARCH_KEY_C           SDLK_c
#define ARCH_KEY_D           SDLK_d
#define ARCH_KEY_E           SDLK_e
#define ARCH_KEY_F           SDLK_f
#define ARCH_KEY_G           SDLK_g
#define ARCH_KEY_H           SDLK_h
#define ARCH_KEY_I           SDLK_i
#define ARCH_KEY_J           SDLK_j
#define ARCH_KEY_K           SDLK_k
#define ARCH_KEY_L           SDLK_l
#define ARCH_KEY_M           SDLK_m
#define ARCH_KEY_N           SDLK_n
#define ARCH_KEY_O           SDLK_o
#define ARCH_KEY_P           SDLK_p
#define ARCH_KEY_Q           SDLK_q
#define ARCH_KEY_R           SDLK_r
#define ARCH_KEY_S           SDLK_s
#define ARCH_KEY_T           SDLK_t
#define ARCH_KEY_U           SDLK_u
#define ARCH_KEY_V           SDLK_v
#define ARCH_KEY_W           SDLK_w
#define ARCH_KEY_X           SDLK_x
#define ARCH_KEY_Y           SDLK_y
#define ARCH_KEY_Z           SDLK_z

#define ARCH_KEY_MINUS       SDLK_MINUS
#define ARCH_KEY_EQUAL       SDLK_EQUALS
#define ARCH_KEY_DIVIDE      SDLK_SLASH
#define ARCH_KEY_SLASH       SDLK_BACKSLASH
#define ARCH_KEY_COMMA       SDLK_COMMA
#define ARCH_KEY_PERIOD      SDLK_PERIOD
#define ARCH_KEY_SEMICOL     SDLK_SEMICOLON

#define ARCH_KEY_LBRACKET    SDLK_LEFTBRACKET
#define ARCH_KEY_RBRACKET    SDLK_RIGHTBRACKET

#define ARCH_KEY_RAPOSTRO    SDLK_QUOTE
#define ARCH_KEY_LAPOSTRO    SDLK_BACKQUOTE

#define ARCH_KEY_ESC         SDLK_ESCAPE
#define ARCH_KEY_ENTER       SDLK_RETURN
#define ARCH_KEY_BACKSP      SDLK_BACKSPACE
#define ARCH_KEY_TAB         SDLK_TAB
#define ARCH_KEY_SPACEBAR    SDLK_SPACE

#define ARCH_KEY_NUMLOCK     SDLK_NUMLOCK
#define ARCH_KEY_SCROLLOCK   SDLK_SCROLLOCK
#define ARCH_KEY_CAPSLOCK    SDLK_CAPSLOCK

#define ARCH_KEY_LSHIFT      SDLK_LSHIFT
#define ARCH_KEY_RSHIFT      SDLK_RSHIFT

#define ARCH_KEY_LALT        SDLK_LALT
#define ARCH_KEY_RALT        SDLK_RALT

#define ARCH_KEY_LCTRL       SDLK_LCTRL
#define ARCH_KEY_RCTRL       SDLK_RCTRL

#define ARCH_KEY_F1          SDLK_F1
#define ARCH_KEY_F2          SDLK_F2
#define ARCH_KEY_F3          SDLK_F3
#define ARCH_KEY_F4          SDLK_F4
#define ARCH_KEY_F5          SDLK_F5
#define ARCH_KEY_F6          SDLK_F6
#define ARCH_KEY_F7          SDLK_F7
#define ARCH_KEY_F8          SDLK_F8
#define ARCH_KEY_F9          SDLK_F9
#define ARCH_KEY_F10         SDLK_F10
#define ARCH_KEY_F11         SDLK_F11
#define ARCH_KEY_F12         SDLK_F12

#define ARCH_KEY_PAD0        SDLK_KP0
#define ARCH_KEY_PAD1        SDLK_KP1
#define ARCH_KEY_PAD2        SDLK_KP2
#define ARCH_KEY_PAD3        SDLK_KP3
#define ARCH_KEY_PAD4        SDLK_KP4
#define ARCH_KEY_PAD5        SDLK_KP5
#define ARCH_KEY_PAD6        SDLK_KP6
#define ARCH_KEY_PAD7        SDLK_KP7
#define ARCH_KEY_PAD8        SDLK_KP8
#define ARCH_KEY_PAD9        SDLK_KP9
#define ARCH_KEY_PADMINUS    SDLK_KP_MINUS
#define ARCH_KEY_PADPLUS     SDLK_KP_PLUS
#define ARCH_KEY_PADPERIOD   SDLK_KP_PERIOD
#define ARCH_KEY_PADDIVIDE   SDLK_KP_DIVIDE
#define ARCH_KEY_PADMULTIPLY SDLK_KP_MULTIPLY
#define ARCH_KEY_PADENTER    SDLK_KP_ENTER

#define ARCH_KEY_INSERT      SDLK_INSERT
#define ARCH_KEY_HOME        SDLK_HOME
#define ARCH_KEY_PAGEUP      SDLK_PAGEUP
#define ARCH_KEY_DELETE      SDLK_DELETE
#define ARCH_KEY_END         SDLK_END
#define ARCH_KEY_PAGEDOWN    SDLK_PAGEDOWN
#define ARCH_KEY_UP          SDLK_UP
#define ARCH_KEY_DOWN        SDLK_DOWN
#define ARCH_KEY_LEFT        SDLK_LEFT
#define ARCH_KEY_RIGHT       SDLK_RIGHT

#define ARCH_KEY_PRINT_SCREEN	SDLK_PRINT
#define ARCH_KEY_PAUSE		SDLK_PAUSE

#endif
