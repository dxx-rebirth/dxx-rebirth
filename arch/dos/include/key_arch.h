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
 * $Source: /cvs/cvsroot/d2x/arch/dos/include/key_arch.h,v $
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
 * Revision 1.1.1.1  1999/06/14 22:00:12  donut
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

#ifndef _DOS_KEY_H
#define _DOS_KEY_H

#define ARCH_KEY_0           0x0B
#define ARCH_KEY_1           0x02
#define ARCH_KEY_2           0x03
#define ARCH_KEY_3           0x04
#define ARCH_KEY_4           0x05
#define ARCH_KEY_5           0x06
#define ARCH_KEY_6           0x07
#define ARCH_KEY_7           0x08
#define ARCH_KEY_8           0x09
#define ARCH_KEY_9           0x0A

#define ARCH_KEY_A           0x1E
#define ARCH_KEY_B           0x30
#define ARCH_KEY_C           0x2E
#define ARCH_KEY_D           0x20
#define ARCH_KEY_E           0x12
#define ARCH_KEY_F           0x21
#define ARCH_KEY_G           0x22
#define ARCH_KEY_H           0x23
#define ARCH_KEY_I           0x17
#define ARCH_KEY_J           0x24
#define ARCH_KEY_K           0x25
#define ARCH_KEY_L           0x26
#define ARCH_KEY_M           0x32
#define ARCH_KEY_N           0x31
#define ARCH_KEY_O           0x18
#define ARCH_KEY_P           0x19
#define ARCH_KEY_Q           0x10
#define ARCH_KEY_R           0x13
#define ARCH_KEY_S           0x1F
#define ARCH_KEY_T           0x14
#define ARCH_KEY_U           0x16
#define ARCH_KEY_V           0x2F
#define ARCH_KEY_W           0x11
#define ARCH_KEY_X           0x2D
#define ARCH_KEY_Y           0x15
#define ARCH_KEY_Z           0x2C

#define ARCH_KEY_MINUS       0x0C
#define ARCH_KEY_EQUAL       0x0D
//this is what we normally think of as slash:
#define ARCH_KEY_DIVIDE      0x35
//this is BACKslash:
#define ARCH_KEY_SLASH       0x2B
#define ARCH_KEY_COMMA       0x33
#define ARCH_KEY_PERIOD      0x34
#define ARCH_KEY_SEMICOL     0x27

#define ARCH_KEY_LBRACKET    0x1A
#define ARCH_KEY_RBRACKET    0x1B

#define ARCH_KEY_RAPOSTRO    0x28
#define ARCH_KEY_LAPOSTRO    0x29

#define ARCH_KEY_ESC         0x01
#define ARCH_KEY_ENTER       0x1C
#define ARCH_KEY_BACKSP      0x0E
#define ARCH_KEY_TAB         0x0F
#define ARCH_KEY_SPACEBAR    0x39

#define ARCH_KEY_NUMLOCK     0x45
#define ARCH_KEY_SCROLLOCK   0x46
#define ARCH_KEY_CAPSLOCK    0x3A

#define ARCH_KEY_LSHIFT      0x2A
#define ARCH_KEY_RSHIFT      0x36

#define ARCH_KEY_LALT        0x38
#define ARCH_KEY_RALT        0xB8

#define ARCH_KEY_LCTRL       0x1D
#define ARCH_KEY_RCTRL       0x9D

#define ARCH_KEY_F1          0x3B
#define ARCH_KEY_F2          0x3C
#define ARCH_KEY_F3          0x3D
#define ARCH_KEY_F4          0x3E
#define ARCH_KEY_F5          0x3F
#define ARCH_KEY_F6          0x40
#define ARCH_KEY_F7          0x41
#define ARCH_KEY_F8          0x42
#define ARCH_KEY_F9          0x43
#define ARCH_KEY_F10         0x44
#define ARCH_KEY_F11         0x57
#define ARCH_KEY_F12         0x58

#define ARCH_KEY_PAD0        0x52
#define ARCH_KEY_PAD1        0x4F
#define ARCH_KEY_PAD2        0x50
#define ARCH_KEY_PAD3        0x51
#define ARCH_KEY_PAD4        0x4B
#define ARCH_KEY_PAD5        0x4C
#define ARCH_KEY_PAD6        0x4D
#define ARCH_KEY_PAD7        0x47
#define ARCH_KEY_PAD8        0x48
#define ARCH_KEY_PAD9        0x49
#define ARCH_KEY_PADMINUS    0x4A
#define ARCH_KEY_PADPLUS     0x4E
#define ARCH_KEY_PADPERIOD   0x53
#define ARCH_KEY_PADDIVIDE   0xB5
#define ARCH_KEY_PADMULTIPLY 0x37
#define ARCH_KEY_PADENTER    0x9C

#define ARCH_KEY_INSERT      0xD2
#define ARCH_KEY_HOME        0xC7
#define ARCH_KEY_PAGEUP      0xC9
#define ARCH_KEY_DELETE      0xD3
#define ARCH_KEY_END         0xCF
#define ARCH_KEY_PAGEDOWN    0xD1
#define ARCH_KEY_UP          0xC8
#define ARCH_KEY_DOWN        0xD0
#define ARCH_KEY_LEFT        0xCB
#define ARCH_KEY_RIGHT       0xCD

#define ARCH_KEY_PRINT_SCREEN	0xB7
#define ARCH_KEY_PAUSE			0x61

#endif
