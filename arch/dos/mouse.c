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
 * $Source: /cvs/cvsroot/d2x/arch/dos/mouse.c,v $
 * $Revision: 1.1.1.2 $
 * $Author: bradleyb $
 * $Date: 2001-01-19 03:33:52 $
 * 
 * Functions to access Mouse and Cyberman...
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 21:58:38  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.11  1995/02/10  18:52:17  john
 * Fixed bug with mouse not getting closed.
 * 
 * Revision 1.10  1995/02/02  11:10:33  john
 * Changed a bunch of mouse stuff around to maybe get
 * around PS/2 mouse hang.
 * 
 * Revision 1.9  1995/01/14  19:19:52  john
 * Fixed signed short error cmp with -1 that caused mouse
 * to break under Watcom 10.0
 * 
 * Revision 1.8  1994/12/27  12:38:23  john
 * Made mouse use temporary dos buffer instead of
 * 
 * allocating its own.
 * 
 * 
 * Revision 1.7  1994/12/05  23:54:53  john
 * Fixed bug with mouse_get_delta only returning positive numbers..
 * 
 * Revision 1.6  1994/11/18  23:18:18  john
 * Changed some shorts to ints.
 * 
 * Revision 1.5  1994/09/13  12:34:02  john
 * Added functions to get down count and state.
 * 
 * Revision 1.4  1994/08/29  20:52:19  john
 * Added better cyberman support; also, joystick calibration
 * value return funcctiionn,
 * 
 * Revision 1.3  1994/08/24  18:54:32  john
 * *** empty log message ***
 * 
 * Revision 1.2  1994/08/24  18:53:46  john
 * Made Cyberman read like normal mouse; added dpmi module; moved
 * mouse from assembly to c. Made mouse buttons return time_down.
 * 
 * Revision 1.1  1994/08/24  13:56:37  john
 * Initial revision
 * 
 * 
 */

#include <conf.h>

#ifdef __DJGPP__
#include <dpmi.h>
#define _BORLAND_DOS_REGS 1
#define near
_go32_dpmi_registers handler_regs;
#endif

#include <go32.h>

#include <stdlib.h>
#include <stdio.h>
//#include <conio.h>
#include <dos.h>
//#include <i86.h>
#include <string.h>

#include "error.h"
#include "fix.h"
#include "u_dpmi.h"
#include "mouse.h"
#include "timer.h"

#define ME_CURSOR_MOVED	(1<<0)
#define ME_LB_P 			(1<<1)
#define ME_LB_R 			(1<<2)
#define ME_RB_P 			(1<<3)
#define ME_RB_R 			(1<<4)
#define ME_MB_P 			(1<<5)
#define ME_MB_R 			(1<<6)
#define ME_OB_P 			(1<<7)
#define ME_OB_R 			(1<<8)
#define ME_X_C 			(1<<9)
#define ME_Y_C 			(1<<10)
#define ME_Z_C 			(1<<11)
#define ME_P_C 			(1<<12)
#define ME_B_C 			(1<<13)
#define ME_H_C 			(1<<14)
#define ME_O_C 			(1<<15)

#define MOUSE_MAX_BUTTONS	11

typedef struct event_info {
	short x;
	short y;
	short z;
	short pitch;
	short bank;
	short heading;
	ushort button_status;
	ushort device_dependant;
} event_info;

typedef struct mouse_info {
	fix		ctime;
	ubyte		cyberman;
	int		num_buttons;
	ubyte		pressed[MOUSE_MAX_BUTTONS];
	fix		time_went_down[MOUSE_MAX_BUTTONS];
	fix		time_held_down[MOUSE_MAX_BUTTONS];
	uint		num_downs[MOUSE_MAX_BUTTONS];
	uint		num_ups[MOUSE_MAX_BUTTONS];
	event_info *x_info;
	ushort	button_status;
} mouse_info;

typedef struct cyberman_info {
	ubyte device_type;
	ubyte major_version;
	ubyte minor_version;
	ubyte x_descriptor;
	ubyte y_descriptor;
	ubyte z_descriptor;
	ubyte pitch_descriptor;
	ubyte roll_descriptor;
	ubyte yaw_descriptor;
	ubyte reserved;
} cyberman_info;

static mouse_info Mouse;

static int Mouse_installed = 0;

#ifdef __DJGPP__
#define m_ax r->d.eax
#define mbx r->d.ebx
#define mcx r->d.ecx
#define mdx r->d.edx
#define msi r->d.esi
#define mdi r->d.edi
void mouse_handler (_go32_dpmi_registers *r)
{
#else
#pragma off (check_stack)
void _loadds far mouse_handler (int m_ax, int mbx, int mcx, int mdx, int msi, int mdi)
{
#pragma aux mouse_handler parm [EAX] [EBX] [ECX] [EDX] [ESI] [EDI]
#endif
	Mouse.ctime = timer_get_fixed_secondsX();

	if (m_ax & ME_LB_P)	{	// left button pressed
		if (!Mouse.pressed[MB_LEFT])	{
			Mouse.pressed[MB_LEFT] = 1;
			Mouse.time_went_down[MB_LEFT] = Mouse.ctime;
		}
		Mouse.num_downs[MB_LEFT]++;
	} else if (m_ax & ME_LB_R )	{  // left button released
		if (Mouse.pressed[MB_LEFT])	{
			Mouse.pressed[MB_LEFT] = 0;
			Mouse.time_held_down[MB_LEFT] += Mouse.ctime-Mouse.time_went_down[MB_LEFT];
		}
		Mouse.num_ups[MB_LEFT]++;
	}

	if (m_ax & ME_RB_P ) {	// right button pressed
		if (!Mouse.pressed[MB_RIGHT])	{
			Mouse.pressed[MB_RIGHT] = 1;
			Mouse.time_went_down[MB_RIGHT] = Mouse.ctime;
		}
		Mouse.num_downs[MB_RIGHT]++;
	} else if (m_ax & ME_RB_R )	{// right button released
		if (Mouse.pressed[MB_RIGHT])	{
			Mouse.pressed[MB_RIGHT] = 0;
			Mouse.time_held_down[MB_RIGHT] += Mouse.ctime-Mouse.time_went_down[MB_RIGHT];
		}
		Mouse.num_ups[MB_RIGHT]++;
	}

	if (m_ax & ME_MB_P )	{ // middle button pressed
		if (!Mouse.pressed[MB_MIDDLE])	{
			Mouse.pressed[MB_MIDDLE] = 1;
			Mouse.time_went_down[MB_MIDDLE] = Mouse.ctime;
		}
		Mouse.num_downs[MB_MIDDLE]++;
	} else if (m_ax & ME_MB_R )	{ // middle button released
		if (Mouse.pressed[MB_MIDDLE])	{
			Mouse.pressed[MB_MIDDLE] = 0;
			Mouse.time_held_down[MB_MIDDLE] += Mouse.ctime-Mouse.time_went_down[MB_MIDDLE];
		}
		Mouse.num_ups[MB_MIDDLE]++;
	}

	if (Mouse.cyberman && (m_ax & (ME_Z_C|ME_P_C|ME_B_C|ME_H_C)))	{
		Mouse.x_info = (event_info *)((msi & 0xFFFF) << 4);

		if (m_ax & ME_Z_C )	{ // z axis changed
			if (Mouse.pressed[MB_Z_UP])	{
			 	// z up released
				Mouse.pressed[MB_Z_UP] = 0;
				Mouse.time_held_down[MB_Z_UP] += Mouse.ctime-Mouse.time_went_down[MB_Z_UP];
				Mouse.num_ups[MB_Z_UP]++;
			}  else if ( Mouse.x_info->z>0 )	{
			 	// z up pressed
				Mouse.pressed[MB_Z_UP] = 1;
				Mouse.time_went_down[MB_Z_UP]=Mouse.ctime;
				Mouse.num_downs[MB_Z_UP]++;
			}
			if (Mouse.pressed[MB_Z_DOWN])	{
			 	// z down released
				Mouse.pressed[MB_Z_DOWN] = 0;
				Mouse.time_held_down[MB_Z_DOWN] += Mouse.ctime-Mouse.time_went_down[MB_Z_DOWN];
				Mouse.num_ups[MB_Z_DOWN]++;
			}  else if ( Mouse.x_info->z<0 )	{
			 	// z down pressed
				Mouse.pressed[MB_Z_DOWN] = 1;
				Mouse.time_went_down[MB_Z_DOWN]=Mouse.ctime;
				Mouse.num_downs[MB_Z_DOWN]++;
			}
		}
		if (m_ax & ME_P_C )	{ // pitch changed
			if (Mouse.pressed[MB_PITCH_BACKWARD])	{
			 	// pitch backward released
				Mouse.pressed[MB_PITCH_BACKWARD] = 0;
				Mouse.time_held_down[MB_PITCH_BACKWARD] += Mouse.ctime-Mouse.time_went_down[MB_PITCH_BACKWARD];
				Mouse.num_ups[MB_PITCH_BACKWARD]++;
			}  else if ( Mouse.x_info->pitch>0 )	{
			 	// pitch backward pressed
				Mouse.pressed[MB_PITCH_BACKWARD] = 1;
				Mouse.time_went_down[MB_PITCH_BACKWARD]=Mouse.ctime;
				Mouse.num_downs[MB_PITCH_BACKWARD]++;
			}
			if (Mouse.pressed[MB_PITCH_FORWARD])	{
			 	// pitch forward released
				Mouse.pressed[MB_PITCH_FORWARD] = 0;
				Mouse.time_held_down[MB_PITCH_FORWARD] += Mouse.ctime-Mouse.time_went_down[MB_PITCH_FORWARD];
				Mouse.num_ups[MB_PITCH_FORWARD]++;
			}  else if ( Mouse.x_info->pitch<0 )	{
			 	// pitch forward pressed
				Mouse.pressed[MB_PITCH_FORWARD] = 1;
				Mouse.time_went_down[MB_PITCH_FORWARD]=Mouse.ctime;
				Mouse.num_downs[MB_PITCH_FORWARD]++;
			}
		}

		if (m_ax & ME_B_C )	{ // bank changed
			if (Mouse.pressed[MB_BANK_LEFT])	{
			 	// bank left released
				Mouse.pressed[MB_BANK_LEFT] = 0;
				Mouse.time_held_down[MB_BANK_LEFT] += Mouse.ctime-Mouse.time_went_down[MB_BANK_LEFT];
				Mouse.num_ups[MB_BANK_LEFT]++;
			}  else if ( Mouse.x_info->bank>0 )	{
			 	// bank left pressed
				Mouse.pressed[MB_BANK_LEFT] = 1;
				Mouse.time_went_down[MB_BANK_LEFT]=Mouse.ctime;
				Mouse.num_downs[MB_BANK_LEFT]++;
			}
			if (Mouse.pressed[MB_BANK_RIGHT])	{
			 	// bank right released
				Mouse.pressed[MB_BANK_RIGHT] = 0;
				Mouse.time_held_down[MB_BANK_RIGHT] += Mouse.ctime-Mouse.time_went_down[MB_BANK_RIGHT];
				Mouse.num_ups[MB_BANK_RIGHT]++;
			}  else if ( Mouse.x_info->bank<0 )	{
			 	// bank right pressed
				Mouse.pressed[MB_BANK_RIGHT] = 1;
				Mouse.time_went_down[MB_BANK_RIGHT]=Mouse.ctime;
				Mouse.num_downs[MB_BANK_RIGHT]++;
			}
		}

		if (m_ax & ME_H_C )	{ // heading changed
			if (Mouse.pressed[MB_HEAD_LEFT])	{
			 	// head left released
				Mouse.pressed[MB_HEAD_LEFT] = 0;
				Mouse.time_held_down[MB_HEAD_LEFT] += Mouse.ctime-Mouse.time_went_down[MB_HEAD_LEFT];
				Mouse.num_ups[MB_HEAD_LEFT]++;
			}  else if ( Mouse.x_info->heading>0 )	{
			 	// head left pressed
				Mouse.pressed[MB_HEAD_LEFT] = 1;
				Mouse.time_went_down[MB_HEAD_LEFT]=Mouse.ctime;
				Mouse.num_downs[MB_HEAD_LEFT]++;
			}
			if (Mouse.pressed[MB_HEAD_RIGHT])	{
			 	// head right released
				Mouse.pressed[MB_HEAD_RIGHT] = 0;
				Mouse.time_held_down[MB_HEAD_RIGHT] += Mouse.ctime-Mouse.time_went_down[MB_HEAD_RIGHT];
				Mouse.num_ups[MB_HEAD_RIGHT]++;
			}  else if ( Mouse.x_info->heading<0 )	{
			 	// head right pressed
				Mouse.pressed[MB_HEAD_RIGHT] = 1;
				Mouse.time_went_down[MB_HEAD_RIGHT]=Mouse.ctime;
				Mouse.num_downs[MB_HEAD_RIGHT]++;
			}
		}
	}
	
}




void mouse_handler_end (void)  // dummy functions
{
}
#pragma on (check_stack)

//--------------------------------------------------------
// returns 0 if no mouse
//           else number of buttons
int mouse_init(int enable_cyberman)
{
	dpmi_real_regs rr;
	cyberman_info *ci;
#ifndef __DJGPP__
	struct SREGS sregs;
#endif
	union REGS inregs, outregs;
	ubyte *Mouse_dos_mem;

	if (Mouse_installed)
		return Mouse.num_buttons;

#ifdef __DJGPP__
       if (_farpeekl(_dos_ds, 0x33 * 4) == 0) {
#else
       if (_dos_getvect(0x33) == NULL) {
#endif
		// No mouse driver loaded
		return 0;
       }

	// Reset the mouse driver
	memset( &inregs, 0, sizeof(inregs) );
	inregs.w.ax = 0;
	int386(0x33, &inregs, &outregs);
	if (outregs.w.ax != 0xffff)
		return 0;

	Mouse.num_buttons = outregs.w.bx;
	Mouse.cyberman = 0;

	// Enable mouse driver
	memset( &inregs, 0, sizeof(inregs) );
	inregs.w.ax = 0x0020;
	int386(0x33, &inregs, &outregs);
	if (outregs.w.ax != 0xffff )
		return 0;

	if ( enable_cyberman )	{
		Mouse_dos_mem = dpmi_get_temp_low_buffer( 64 );
		if (Mouse_dos_mem==NULL)	{
			printf( "Unable to allocate DOS buffer in mouse.c\n" );
		} else {
			// Check for Cyberman...	
			memset( &rr, 0, sizeof(dpmi_real_regs) );
			rr.es = DPMI_real_segment(Mouse_dos_mem);
			rr.edx = DPMI_real_offset(Mouse_dos_mem);
			rr.eax = 0x53c1;
			dpmi_real_int386x( 0x33, &rr );
			if (rr.eax==1)	{
				// SWIFT functions supported
				ci	= (cyberman_info *)Mouse_dos_mem;
				if (ci->device_type==1)	{	// Cyberman	
					Mouse.cyberman = 1;
					//printf( "Cyberman mouse detected\n" );
					Mouse.num_buttons = 11;
				}
			}
		}
	}

	if (!dpmi_lock_region(&Mouse,sizeof(mouse_info)))	{
		printf( "Unable to lock mouse data region" );
		exit(1);
	}
	if (!dpmi_lock_region((void near *)mouse_handler,(char *)mouse_handler_end - (char near *)mouse_handler))	{
		printf( "Unable to lock mouse handler" );
		exit(1);
	}

	// Install mouse handler
#ifdef __DJGPP__
	{
	 dpmi_real_regs rregs;
	 _go32_dpmi_seginfo info;
	 memset(&rregs, 0, sizeof(rregs));
	 info.pm_offset = (unsigned int)&mouse_handler;
	 if (_go32_dpmi_allocate_real_mode_callback_retf(&info, &handler_regs)) {
		printf( "Unable allocate mouse handler callback" );
                exit(1);
         }
	 rregs.eax     = 0xC;
	 rregs.ecx     = ME_LB_P|ME_LB_R|ME_RB_P|ME_RB_R|ME_MB_P|ME_MB_R;      // watch all 3 button ups/downs
	 if (Mouse.cyberman)
		rregs.ecx     |= ME_Z_C| ME_P_C| ME_B_C| ME_H_C;      // if using a cyberman, also watch z, pitch, bank, heading.
         rregs.edx       = info.rm_offset;
	 rregs.es	 = info.rm_segment;
	 dpmi_real_int386x( 0x33, &rregs );
	}
#else
	memset( &inregs, 0, sizeof(inregs));
        memset( &sregs, 0, sizeof(sregs));
        inregs.w.ax     = 0xC;
        inregs.w.cx     = ME_LB_P|ME_LB_R|ME_RB_P|ME_RB_R|ME_MB_P|ME_MB_R;      // watch all 3 button ups/downs
        if (Mouse.cyberman)
                inregs.w.cx     |= ME_Z_C| ME_P_C| ME_B_C| ME_H_C;      // if using a cyberman, also watch z, pitch, bank, heading.
        inregs.x.edx    = FP_OFF(mouse_handler);
	sregs.es 		= FP_SEG(mouse_handler);
	int386x(0x33, &inregs, &outregs, &sregs);
#endif

	Mouse_installed = 1;

	atexit( mouse_close );

	mouse_flush();

	return Mouse.num_buttons;
}



void mouse_close()
{
	struct SREGS sregs;
	union REGS inregs, outregs;

	if (Mouse_installed)	{
		Mouse_installed = 0;
		// clear mouse handler by setting flags to 0.
		memset( &inregs, 0, sizeof(inregs));
		memset( &sregs, 0, sizeof(sregs));
		inregs.w.ax 	= 0xC;
		inregs.w.cx		= 0;		// disable event handler by setting to zero.
		inregs.x.edx 	= 0;	
		sregs.es       = 0;
		int386x(0x33, &inregs, &outregs, &sregs);
	}
}


void mouse_set_limits( int x1, int y1, int x2, int y2 )
{
	union REGS inregs, outregs;

	if (!Mouse_installed) return;

	memset( &inregs, 0, sizeof(inregs));
	inregs.w.ax = 0x7;	// Set Horizontal Limits for Pointer
	inregs.w.cx = x1;
	inregs.w.dx = x2;
	int386(0x33, &inregs, &outregs);

	memset( &inregs, 0, sizeof(inregs));
	inregs.w.ax = 0x8;	// Set Vertical Limits for Pointer
	inregs.w.cx = y1;
	inregs.w.dx = y2;
	int386(0x33, &inregs, &outregs);
}

void mouse_get_pos( int *x, int *y)
{
	union REGS inregs, outregs;

	if (!Mouse_installed) {
		*x = *y = 0;
		return;
	}
	memset( &inregs, 0, sizeof(inregs));
	inregs.w.ax = 0x3;	// Get Mouse Position and Button Status
	int386(0x33, &inregs, &outregs);
	*x = (short)outregs.w.cx; 
	*y = (short)outregs.w.dx; 
}

void mouse_get_delta( int *dx, int *dy )
{
	union REGS inregs, outregs;

	if (!Mouse_installed) {
		*dx = *dy = 0;
		return;
	}

	memset( &inregs, 0, sizeof(inregs));
	inregs.w.ax = 0xb;	// Read Mouse motion counters
	int386(0x33, &inregs, &outregs);
	*dx = (short)outregs.w.cx; 
	*dy = (short)outregs.w.dx; 
}

int mouse_get_btns()
{
	int i;
	uint flag=1;
	int status = 0;

	if (!Mouse_installed) 
		return 0;

	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )	{
		if (Mouse.pressed[i])
			status |= flag;
		flag <<= 1;
	}
	return status;
}

void mouse_set_pos( int x, int y)
{
	union REGS inregs, outregs;

	if (!Mouse_installed) 
		return;

	memset( &inregs, 0, sizeof(inregs));
	inregs.w.ax = 0x4;	// Set Mouse Pointer Position
	inregs.w.cx = x;
	inregs.w.dx = y;
	int386(0x33, &inregs, &outregs);

}

void mouse_flush()
{
	int i;
	fix CurTime;

	if (!Mouse_installed) 
		return;

	_disable();

	//Clear the mouse data
	CurTime =timer_get_fixed_secondsX();
	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )	{
		Mouse.pressed[i] = 0;
		Mouse.time_went_down[i] = CurTime;
		Mouse.time_held_down[i] = 0;
		Mouse.num_downs[i]=0;
		Mouse.num_ups[i]=0;
	}
	_enable();
}


// Returns how many times this button has went down since last call.
int mouse_button_down_count(int button)	
{
	int count;

	if (!Mouse_installed) 
		return 0;

	_disable();

	count = Mouse.num_downs[button];
	Mouse.num_downs[button]=0;

	_enable();

	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)	
{
	int state;

	if (!Mouse_installed) 
		return 0;

	_disable();

	state = Mouse.pressed[button];

	_enable();

	return state;
}



// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)	
{
	fix time_down, time;

	if (!Mouse_installed) 
		return 0;

	_disable();

	if ( !Mouse.pressed[button] )	{
		time_down = Mouse.time_held_down[button];
		Mouse.time_held_down[button] = 0;
	} else	{
		time = timer_get_fixed_secondsX();
		time_down =  time - Mouse.time_went_down[button];
		Mouse.time_went_down[button] = time;
	}

	_enable();

	return time_down;
}

void mouse_get_cyberman_pos( int *x, int *y )
{
	dpmi_real_regs rr;
	event_info * ei;
	ubyte *Mouse_dos_mem;

	if ( (!Mouse_installed) || (!Mouse.cyberman) ) {
		*x = *y = 0;
		return;
	}

	Mouse_dos_mem = dpmi_get_temp_low_buffer( 64 );

	if ( !Mouse_dos_mem )	{
		*x = *y = 0;
		return;
	}


	memset( &rr, 0, sizeof(dpmi_real_regs) );
	rr.es = DPMI_real_segment(Mouse_dos_mem);
	rr.edx = DPMI_real_offset(Mouse_dos_mem);
	rr.eax = 0x5301;
	dpmi_real_int386x( 0x33, &rr );

	ei = (event_info *)Mouse_dos_mem;

	*x = (((ei->x+8128)*256)/(8064+8128+1)) - 127;
	*y = (((ei->y+8128)*256)/(8064+8128+1)) - 127;
}

