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
 * $Source: /cvs/cvsroot/d2x/arch/dos/key_arch.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 07:18:00 $
 * 
 * Functions for keyboard handler.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/02/07 10:09:52  donut
 * dos pragma ifdefs
 *
 * Revision 1.1  2000/01/17 05:58:38  donut
 * switched from multiply reimplemented/reduntant/buggy key.c for each arch to a single main/key.c that calls the much smaller arch-specific parts.  Also adds working emulated key repeat support.
 *
 * Revision 1.1.1.1  1999/06/14 21:58:32  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.35  1995/01/25  20:13:30  john
 * Took out not passing keys to debugger if w10.
 * 
 * Revision 1.34  1995/01/14  19:19:31  john
 * Made so when you press Shift+Baskspace, it release keys autmatically.
 * 
 * Revision 1.33  1994/12/13  09:21:48  john
 * Took out keyd_editor_mode, and KEY_DEBUGGED stuff for NDEBUG versions.
 * 
 * Revision 1.32  1994/11/12  13:52:01  john
 * Fixed bug with code that cleared bios buffer.
 * 
 * Revision 1.31  1994/10/24  15:16:16  john
 * Added code to detect KEY_PAUSE.
 * 
 * Revision 1.30  1994/10/24  13:57:53  john
 * Hacked in support for pause key onto code 0x61.
 * 
 * Revision 1.29  1994/10/21  15:18:13  john
 * *** empty log message ***
 * 
 * Revision 1.28  1994/10/21  15:17:24  john
 * Made LSHIFT+BACKSPACE do what PrtScr used to.
 * 
 * Revision 1.27  1994/09/22  16:09:18  john
 * Fixed some virtual memory lockdown problems with timer and
 * joystick.
 * 
 * Revision 1.26  1994/09/15  21:32:47  john
 * Added bounds checking for down_count scancode
 * parameter.
 * 
 * Revision 1.25  1994/08/31  12:22:20  john
 * Added KEY_DEBUGGED
 * 
 * Revision 1.24  1994/08/24  18:53:48  john
 * Made Cyberman read like normal mouse; added dpmi module; moved
 * mouse from assembly to c. Made mouse buttons return time_down.
 * 
 * Revision 1.23  1994/08/18  15:17:51  john
 * *** empty log message ***
 * 
 * Revision 1.22  1994/08/18  15:16:38  john
 * fixed some bugs with clear_key_times and then
 * removed it because i fixed key_flush to do the
 * same.
 * 
 * Revision 1.21  1994/08/17  19:01:25  john
 * Attempted to fix a bug with a key being held down
 * key_flush called, then the key released having too 
 * long of a time.
 * 
 * Revision 1.20  1994/08/08  10:43:48  john
 * Recorded when a key was pressed for key_inkey_time.
 * 
 * Revision 1.19  1994/06/22  15:00:03  john
 * Made keyboard close automatically on exit.
 * 
 * Revision 1.18  1994/06/21  09:16:29  john
 * *** empty log message ***
 * 
 * Revision 1.17  1994/06/21  09:08:23  john
 * *** empty log message ***
 * 
 * Revision 1.16  1994/06/21  09:05:01  john
 * *** empty log message ***
 * 
 * Revision 1.15  1994/06/21  09:04:24  john
 * Made PrtScreen do an int5
 * 
 * Revision 1.14  1994/06/17  17:17:06  john
 * Added keyd_time_last_key_was_pressed or something like that.
 * 
 * Revision 1.13  1994/05/14  13:55:16  matt
 * Added #define to control key passing to bios
 * 
 * Revision 1.12  1994/05/05  18:09:39  john
 * Took out BIOS to prevent stuck keys.
 * 
 * Revision 1.11  1994/05/03  17:39:12  john
 * *** empty log message ***
 * 
 * Revision 1.10  1994/04/29  12:14:20  john
 * Locked all memory used during interrupts so that program
 * won't hang when using virtual memory.
 * 
 * Revision 1.9  1994/04/28  23:49:41  john
 * Made key_flush flush more keys and also did something else but i forget what.
 * 
 * Revision 1.8  1994/04/22  12:52:12  john
 * *** empty log message ***
 * 
 * Revision 1.7  1994/04/01  10:44:59  mike
 * Change key_getch() to call getch() if our interrupt hasn't been installed.
 * 
 * Revision 1.6  1994/03/09  10:45:48  john
 * Neatend code a bit.
 * 
 * Revision 1.5  1994/02/17  17:24:16  john
 * Neatened up a bit.
 * 
 * Revision 1.4  1994/02/17  16:30:29  john
 * Put in code to pass keys when in debugger.
 * 
 * Revision 1.3  1994/02/17  15:57:59  john
 * Made handler not chain to BIOS handler.
 * 
 * Revision 1.2  1994/02/17  15:56:06  john
 * Initial version.
 * 
 * Revision 1.1  1994/02/17  15:54:07  john
 * Initial revision
 * 
 * 
 */

//#define PASS_KEYS_TO_BIOS	1			//if set, bios gets keys

#ifdef RCS
static char rcsid[] = "$Id: key_arch.c,v 1.1 2002-02-15 07:18:00 bradleyb Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>

//#define WATCOM_10
#ifdef __DJGPP__
#include <dpmi.h>
#define _far
#define __far
#define __interrupt
#define near
_go32_dpmi_seginfo kbd_hand_info;
#endif
#include "error.h"
#include "key.h"
#include "timer.h"
#include "u_dpmi.h"

#define LOCAL_KEY_BUFFER_SIZE 32


typedef struct keyboard	{
	unsigned short		keybuffer[LOCAL_KEY_BUFFER_SIZE];
	unsigned short		statebuffer[LOCAL_KEY_BUFFER_SIZE];
	unsigned int 		keyhead, keytail;
	unsigned char 		E0Flag;
	unsigned char 		E1Flag;
	int 					in_key_handler;
#ifdef __DJGPP__
	_go32_dpmi_seginfo prev_int_9;
#else
	void (__interrupt __far *prev_int_9)();
#endif
} keyboard;

static volatile keyboard key_data;

static unsigned char Installed=0;

void arch_key_poll(void){
	_disable();
	while (key_data.keytail!=key_data.keyhead){
		generic_key_handler(key_data.keybuffer[key_data.keyhead],key_data.statebuffer[key_data.keyhead]);
		key_data.keyhead++;
		if ( key_data.keyhead >= LOCAL_KEY_BUFFER_SIZE ) key_data.keyhead=0;
	}
	_enable();
}

void key_clear_bios_buffer_all()
{
#ifdef __WATCOMC__
	// Clear keyboard buffer...
	*(ushort *)0x41a=*(ushort *)0x41c;
	// Clear the status bits...
	*(ubyte *)0x417 = 0;
	*(ubyte *)0x418 = 0;
#else
	_farpokew(_dos_ds,0x41a, _farpeekw(_dos_ds, 0x41c));
	_farpokeb(_dos_ds,0x417, 0);
	_farpokeb(_dos_ds,0x418, 0);
#endif
}

void key_clear_bios_buffer()
{
#ifdef __WATCOMC__
	// Clear keyboard buffer...
	*(ushort *)0x41a=*(ushort *)0x41c;
#else
	_farpokew(_dos_ds,0x41a, _farpeekw(_dos_ds, 0x41c));
#endif
}

void arch_key_flush()
{
	int i;

	_disable();

	// Clear the BIOS buffer
	key_clear_bios_buffer();

	key_data.keyhead = key_data.keytail = 0;

	//Clear the keyboard buffer
	for (i=0; i<LOCAL_KEY_BUFFER_SIZE; i++ )	{
		key_data.keybuffer[i] = 0;
		key_data.statebuffer[i] = 0;
	}

	//Clear the keyboard array

	_enable();
}


// Use intrinsic forms so that we stay in the locked interrup code.

#ifdef __WATCOMC__
void Int5();
#pragma aux Int5 = "int 5";
#else
#ifdef __GNUC__
#define Int5() asm volatile("int $5")
#endif
#endif

#ifndef __GNUC__
#pragma off (check_stack)
#endif
void __interrupt __far key_handler()
{
	unsigned char scancode, breakbit, temp;
	unsigned short keycode;

#ifndef WATCOM_10
#ifndef NDEBUG
#ifdef __WATCOMC__ /* must have _chain_intr */
	ubyte * MONO = (ubyte *)(0x0b0000+24*80*2);
	if (  ((MONO[0]=='D') && (MONO[2]=='B') && (MONO[4]=='G') && (MONO[6]=='>')) ||
			((MONO[14]=='<') && (MONO[16]=='i') && (MONO[18]=='>') && (MONO[20]==' ') && (MONO[22]=='-')) ||
			((MONO[0]==200 ) && (MONO[2]==27) && (MONO[4]==17) )
		)
 		_chain_intr( key_data.prev_int_9 );
#endif
#endif
#endif

	// Read in scancode
	scancode = inp( 0x60 );

	switch( scancode )	{
	case 0xE0:
		key_data.E0Flag = 0x80;
		break;
	default:
		// Parse scancode and break bit
		if (key_data.E1Flag > 0 )	{		// Special code for Pause, which is E1 1D 45 E1 9D C5
			key_data.E1Flag--;
			if ( scancode == 0x1D )	{
				scancode	= KEY_PAUSE;
				breakbit	= 0;
			} else if ( scancode == 0x9d ) {
				scancode	= KEY_PAUSE;
				breakbit	= 1;
			} else {
				break;		// skip this keycode
			}
		} else if ( scancode==0xE1 )	{
			key_data.E1Flag = 2;
			break;
		} else {
			breakbit	= scancode & 0x80;		// Get make/break bit
			scancode &= 0x7f;						// Strip make/break bit off of scancode
			scancode |= key_data.E0Flag;					// Add in extended key code
		}
		key_data.E0Flag = 0;								// Clear extended key code
		temp = key_data.keytail+1;
		if ( temp >= LOCAL_KEY_BUFFER_SIZE ) temp=0;

		if (temp!=key_data.keyhead)	{
			keycode=scancode;

			key_data.keybuffer[key_data.keytail] = keycode;
			key_data.statebuffer[key_data.keytail] = !breakbit;
			key_data.keytail = temp;
		}
	}

#ifndef NDEBUG
#ifdef PASS_KEYS_TO_BIOS
	_chain_intr( key_data.prev_int_9 );
#endif
#endif

	temp = inp(0x61);		// Get current port 61h state
	temp |= 0x80;			// Turn on bit 7 to signal clear keybrd
	outp( 0x61, temp );	// Send to port
	temp &= 0x7f;			// Turn off bit 7 to signal break
	outp( 0x61, temp );	// Send to port
	outp( 0x20, 0x20 );	// Reset interrupt controller
}

#ifndef __GNUC__
#pragma on (check_stack)
#endif

void key_handler_end()	{		// Dummy function to help calculate size of keyboard handler function
}

void arch_key_init()
{
	// Initialize queue

	key_data.in_key_handler = 0;
	key_data.E0Flag = 0;
	key_data.E1Flag = 0;

	if (Installed) return;
	Installed = 1;

	//--------------- lock everything for the virtal memory ----------------------------------
	if (!dpmi_lock_region ((void near *)key_handler, (char *)key_handler_end - (char near *)key_handler))	{
		printf( "Error locking keyboard handler!\n" );
		exit(1);
	}
	if (!dpmi_lock_region ((void *)&key_data, sizeof(keyboard)))	{
		printf( "Error locking keyboard handler's data1!\n" );
		exit(1);
	}

#ifndef __DJGPP__
	key_data.prev_int_9 = (void *)_dos_getvect( 9 );
    _dos_setvect( 9, key_handler );
#else
	_go32_dpmi_get_protected_mode_interrupt_vector(9,
	 (_go32_dpmi_seginfo *)&key_data.prev_int_9);
	kbd_hand_info.pm_offset = (int)key_handler;
	kbd_hand_info.pm_selector = _my_cs();
	_go32_dpmi_allocate_iret_wrapper(&kbd_hand_info);
	_go32_dpmi_set_protected_mode_interrupt_vector(9, &kbd_hand_info);
#endif
}

void arch_key_close()
{
	if (!Installed) return;
	Installed = 0;
	
#ifndef __DJGPP__
	_dos_setvect( 9, key_data.prev_int_9 );
#else
	_go32_dpmi_set_protected_mode_interrupt_vector(9,
	 (_go32_dpmi_seginfo *)&key_data.prev_int_9);
#endif

	_disable();
	key_clear_bios_buffer_all();
	_enable();

}
