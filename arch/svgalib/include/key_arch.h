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
 * $Source: /cvs/cvsroot/d2x/arch/svgalib/include/key_arch.h,v $
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
 * Revision 1.1.1.1  1999/06/14 22:01:46  donut
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

#ifndef _SVGALIB_KEY_H
#define _SVGALIB_KEY_H

#include <vgakeyboard.h>

#define ARCH_KEY_0           11
#define ARCH_KEY_1           2
#define ARCH_KEY_2           3
#define ARCH_KEY_3           4
#define ARCH_KEY_4           5
#define ARCH_KEY_5           6
#define ARCH_KEY_6           7
#define ARCH_KEY_7           8
#define ARCH_KEY_8           9
#define ARCH_KEY_9           10

#define ARCH_KEY_A           30
#define ARCH_KEY_B           48
#define ARCH_KEY_C           46
#define ARCH_KEY_D           32
#define ARCH_KEY_E           18
#define ARCH_KEY_F           33
#define ARCH_KEY_G           34
#define ARCH_KEY_H           35
#define ARCH_KEY_I           23
#define ARCH_KEY_J           36
#define ARCH_KEY_K           37
#define ARCH_KEY_L           38
#define ARCH_KEY_M           50
#define ARCH_KEY_N           49
#define ARCH_KEY_O           24
#define ARCH_KEY_P           25
#define ARCH_KEY_Q           16
#define ARCH_KEY_R           19
#define ARCH_KEY_S           31
#define ARCH_KEY_T           20
#define ARCH_KEY_U           22
#define ARCH_KEY_V           47
#define ARCH_KEY_W           17
#define ARCH_KEY_X           45
#define ARCH_KEY_Y           21
#define ARCH_KEY_Z           44

#define ARCH_KEY_MINUS       12
#define ARCH_KEY_EQUAL       13
#define ARCH_KEY_DIVIDE      43
#define ARCH_KEY_SLASH       28
#define ARCH_KEY_COMMA       51
#define ARCH_KEY_PERIOD      52
#define ARCH_KEY_SEMICOL     39

#define ARCH_KEY_LBRACKET    26
#define ARCH_KEY_RBRACKET    27

#define ARCH_KEY_RAPOSTRO    40
#define ARCH_KEY_LAPOSTRO    41

#define ARCH_KEY_ESC         1
#define ARCH_KEY_ENTER       28
#define ARCH_KEY_BACKSP      14
#define ARCH_KEY_TAB         15
#define ARCH_KEY_SPACEBAR    57

#define ARCH_KEY_NUMLOCK     69
#define ARCH_KEY_SCROLLOCK   70
#define ARCH_KEY_CAPSLOCK    58

#define ARCH_KEY_LSHIFT      42
#define ARCH_KEY_RSHIFT      54

#define ARCH_KEY_LALT        56
#define ARCH_KEY_RALT        100

#define ARCH_KEY_LCTRL       29
#define ARCH_KEY_RCTRL       97

#define ARCH_KEY_F1          59
#define ARCH_KEY_F2          60
#define ARCH_KEY_F3          61
#define ARCH_KEY_F4          62
#define ARCH_KEY_F5          63
#define ARCH_KEY_F6          64
#define ARCH_KEY_F7          65
#define ARCH_KEY_F8          66
#define ARCH_KEY_F9          67
#define ARCH_KEY_F10         68
#define ARCH_KEY_F11         87
#define ARCH_KEY_F12         88

#define ARCH_KEY_PAD0        82
#define ARCH_KEY_PAD1        79
#define ARCH_KEY_PAD2        80
#define ARCH_KEY_PAD3        81
#define ARCH_KEY_PAD4        75
#define ARCH_KEY_PAD5        76
#define ARCH_KEY_PAD6        77
#define ARCH_KEY_PAD7        71
#define ARCH_KEY_PAD8        72
#define ARCH_KEY_PAD9        73
#define ARCH_KEY_PADMINUS    74
#define ARCH_KEY_PADPLUS     78
#define ARCH_KEY_PADPERIOD   83
#define ARCH_KEY_PADDIVIDE   98
#define ARCH_KEY_PADMULTIPLY 55
#define ARCH_KEY_PADENTER    96

#define ARCH_KEY_INSERT      110
#define ARCH_KEY_HOME        102
#define ARCH_KEY_PAGEUP      104
#define ARCH_KEY_DELETE      111
#define ARCH_KEY_END         107
#define ARCH_KEY_PAGEDOWN    109
#define ARCH_KEY_UP          103
#define ARCH_KEY_DOWN        108
#define ARCH_KEY_LEFT        105
#define ARCH_KEY_RIGHT       106

#define ARCH_KEY_PRINT_SCREEN	99
#define ARCH_KEY_PAUSE		119

#endif
