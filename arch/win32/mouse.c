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
* $Source: /cvs/cvsroot/d2x/arch/win32/mouse.c,v $
* $Revision: 1.2 $
* $Author: btb $
* $Date: 2004-05-19 02:18:19 $
* 
* Functions to access Mouse and Cyberman...
* 
* $Log: not supported by cvs2svn $
* Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
* Import of d2x-0.0.8
*
* Revision 1.5  1999/10/15 05:27:48  donut
* include to fix undef'd err
*
* Revision 1.4  1999/10/14 03:08:10  donut
* changed exit to mprintf on unknown mouse event
*
* Revision 1.3  1999/10/09 05:03:57  donut
* fixed win32 exit on mouse move
*
* Revision 1.2  1999/09/05 04:19:19  sekmu
* made mouse exclusive for windows
*
* Revision 1.1.1.1  1999/06/14 22:00:37  donut
* Import of d1x 1.37 source.
*
* Revision 1.8  1996/02/21  13:57:36  allender
* cursor device manager stuff added here so as not to
* rely on InterfaceLib anymore
*
* Revision 1.7  1995/10/17  15:42:21  allender
* new mouse function to determine single button press
*
* Revision 1.6  1995/10/03  11:27:31  allender
* fixed up hotspot problems with the mouse on multiple monitors
*
* Revision 1.5  1995/07/13  11:27:08  allender
* trap button checks at MAX_MOUSE_BUTTONS
*
* Revision 1.4  1995/06/25  21:56:53  allender
* added events include
*
* Revision 1.3  1995/05/11  17:06:38  allender
* fixed up mouse routines
*
* Revision 1.2  1995/05/11  13:05:53  allender
* of mouse handler code
*
* Revision 1.1  1995/05/05  09:54:45  allender
* Initial revision
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

#define WIN32_LEAN_AND_MEAN
#include <dinput.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "fix.h"
#include "mouse.h"
#include "mono.h"
#include "timer.h"

// These are to kludge up a bit my slightly broken GCC directx port.
#ifndef E_FAIL
#define E_FAIL (HRESULT)0x80004005L
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(a) ((HRESULT)(a) >= 0)
#endif
#ifndef S_OK
#define S_OK 0
#define S_FALSE 1
#endif
#ifndef SEVERITY_SUCCESS
#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1
#endif
#ifndef FACILITY_WIN32
#define FACILITY_WIN32                   7
#endif
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif

#define ME_CURSOR_MOVED	(1<<0)
#define ME_LB_P 		(1<<1)
#define ME_LB_R 		(1<<2)
#define ME_RB_P 		(1<<3)
#define ME_RB_R 		(1<<4)
#define ME_MB_P 		(1<<5)
#define ME_MB_R 		(1<<6)
#define ME_OB_P 		(1<<7)
#define ME_OB_R 		(1<<8)
#define ME_X_C 			(1<<9)
#define ME_Y_C 			(1<<10)
#define ME_Z_C 			(1<<11)
#define ME_P_C 			(1<<12)
#define ME_B_C 			(1<<13)
#define ME_H_C 			(1<<14)
#define ME_O_C 			(1<<15)

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
	ubyte	cyberman;
	int		num_buttons;
	ubyte	pressed[MOUSE_MAX_BUTTONS];
	fix		time_went_down[MOUSE_MAX_BUTTONS];
	fix		time_held_down[MOUSE_MAX_BUTTONS];
	uint	num_downs[MOUSE_MAX_BUTTONS];
	uint	num_ups[MOUSE_MAX_BUTTONS];
	//	ubyte	went_down; /* Not in PC version, not needed with 'num_downs' etc */
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

int WMMouse_Handler_Ready = 0;
int mouse_wparam, mouse_lparam, mouse_msg;


//GGI data:
//extern  ggi_visual_t		visual;
//extern  ggi_directbuffer_t		dbuf;	// GGI direct acces to screen memory
//extern  ggi_pixellinearbuffer	*plb;

//Mouse globals
static double mouse_x, mouse_y;
static double mouse_saved_x, mouse_saved_y; //used when hiding/unhiding to reset the real (displayed) postion
double mouse_accel=1.0;

void DrawMouse(void);
void EraseMouse(void);
void MoveMouse(/*int button,*/ int x, int y);

#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define SCR_WIDTH 640
#define SCR_HEIGHT 480

LPDIRECTINPUT g_lpdi;
LPDIRECTINPUTDEVICE g_lpdidMouse;
extern HWND g_hWnd;


HRESULT ReadMouse (DIDEVICEOBJECTDATA *pdidod)
{
	DWORD cElements = 1;
	HRESULT hr;

	if (g_lpdidMouse == NULL)
		return E_FAIL;

	hr = IDirectInputDevice_GetDeviceData (
		g_lpdidMouse,
		sizeof (*pdidod),
		pdidod,
		&cElements,
		0);

	if (hr == DIERR_INPUTLOST)
	{
		hr = IDirectInputDevice_Acquire (g_lpdidMouse);
		if (SUCCEEDED (hr))
		{
			hr = IDirectInputDevice_GetDeviceData (
				g_lpdidMouse,
				sizeof (*pdidod),
				pdidod,
				&cElements,
				0);
		}
	}

	if (SUCCEEDED (hr) && cElements != 1)
		hr = E_FAIL;

	return hr;
}


void UpdateMouseState (DIDEVICEOBJECTDATA *pdidod)
{
//        fix timeNow = timer_get_fixed_seconds ();

	ULONG iEvt = pdidod->dwOfs;
	switch (iEvt)
	{
		case DIMOFS_BUTTON0:
		case DIMOFS_BUTTON1:
		case DIMOFS_BUTTON2:
		case DIMOFS_BUTTON3:
		{
			BOOL bPressed = pdidod->dwData & 0x80;
			ULONG iButton = (iEvt - DIMOFS_BUTTON0) + MB_LEFT;
			if (bPressed)
			{
				if (!Mouse.pressed [iButton])
				{
					Mouse.pressed [iButton] = 1;
					Mouse.time_went_down [iButton] = Mouse.ctime;
					Mouse.num_downs [iButton]++;
					//			Mouse.went_down = 1;
				}
				Mouse.num_downs [iButton] ++;
			}
			else
			{
				if (Mouse.pressed [iButton])
				{
					Mouse.pressed [iButton] = 0;
					Mouse.time_held_down [iButton] += Mouse.ctime - Mouse.time_went_down [iButton];
					Mouse.num_ups [iButton]++;
					//			Mouse.went_down = 0;
				}
			}
			break;
		}
		case DIMOFS_X:
			mouse_x += (double) ((LONG) pdidod->dwData);
			break;

		case DIMOFS_Y:
			mouse_y += (double) ((LONG) pdidod->dwData);
			break;
		case DIMOFS_Z:
			break;//hm, handle this?
		default:
			mprintf((0,"unknown mouse event %i\n",iEvt));
//			exit (iEvt);//not happy.
			break;
	}
}

void mouse_handler()
{
	DIDEVICEOBJECTDATA didod;

	Mouse.ctime = timer_get_fixed_seconds();

	while (SUCCEEDED (ReadMouse (&didod)))
	{
		UpdateMouseState (&didod);
	}
}

void mouse_flush()
{
	int i;
	fix CurTime;
	
	if (!Mouse_installed)
		return;

        mouse_handler();
	//	_disable();
	CurTime = timer_get_fixed_seconds();
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) {
		Mouse.pressed[i] = 0;
		Mouse.time_went_down[i] = CurTime;
		Mouse.time_held_down[i] = 0;
		Mouse.num_downs[i] = 0;
		Mouse.num_ups[i] = 0;
	}
	//	Mouse.went_down = 0; /* mac only */
	//	_enable();

	{
		DWORD cElements = INFINITE;
//                HRESULT hr =
		IDirectInputDevice_GetDeviceData (
			g_lpdidMouse,
			sizeof (DIDEVICEOBJECTDATA),
			NULL,
			&cElements,
			0);
	}
}


void mouse_close(void)
{
	// 	if (Mouse_installed)   // DPH: Unnecessary...
	WMMouse_Handler_Ready=Mouse_installed = 0;

	if (g_lpdidMouse != NULL)
	{
		IDirectInputDevice_Unacquire (g_lpdidMouse);
		IDirectInputDevice_Release (g_lpdidMouse);
		g_lpdidMouse = NULL;
	}
	if (g_lpdi != NULL)
	{
		IDirectInput_Release (g_lpdi);
		g_lpdi = NULL;
	}
}



int mouse_init(int unused)
{
	if (Mouse_installed)
		return Mouse.num_buttons;
	
	{
		HRESULT hr;

		if (SUCCEEDED (hr = DirectInputCreate (GetModuleHandle (NULL), DIRECTINPUT_VERSION, &g_lpdi, NULL)))
		{
                        if (SUCCEEDED (hr = IDirectInput_CreateDevice (g_lpdi,(void *) &GUID_SysMouse, &g_lpdidMouse, NULL)))
			{
				DIPROPDWORD dipdw;
				dipdw.diph.dwSize = sizeof (DIPROPDWORD);
				dipdw.diph.dwHeaderSize = sizeof (DIPROPHEADER);
				dipdw.diph.dwObj = 0;
				dipdw.diph.dwHow = DIPH_DEVICE;
				dipdw.dwData = 40;

				if (SUCCEEDED (hr = IDirectInputDevice_SetDataFormat (g_lpdidMouse, &c_dfDIMouse)) &&
                                        //changed on 9/4/99 by Victor Rachels NONEX -> Exclusive
                                        SUCCEEDED (hr = IDirectInputDevice_SetCooperativeLevel (g_lpdidMouse, g_hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) &&
                                        //end this section edit -VR
					SUCCEEDED (hr = IDirectInputDevice_SetProperty (g_lpdidMouse, DIPROP_BUFFERSIZE, &dipdw.diph)) &&
					SUCCEEDED (hr = IDirectInputDevice_Acquire (g_lpdidMouse)))
				{
				}
				else
				{
					IDirectInputDevice_Release (g_lpdidMouse);
					g_lpdidMouse = NULL;
					return 0;
				}
			}
		}
	}
	Mouse.num_buttons = 3;
	
	WMMouse_Handler_Ready=Mouse_installed = 1;
	atexit(mouse_close);
	mouse_flush();
	//	mouse_set_center();
	
	return Mouse.num_buttons;
}

//WHS: added this
void mouse_center() {
	mouse_x=mouse_saved_x=WIN_WIDTH/2;
	mouse_y=mouse_saved_y=WIN_HEIGHT/2;
}

void mouse_get_pos( int *x, int *y)
{
	mouse_handler(); //temp
	
	*x=(int) mouse_x;
	*y=(int) mouse_y;
}

void mouse_get_delta( int *dx, int *dy )
{
	if (!Mouse_installed) {
		*dx = *dy = 0;
		return;
	}
	mouse_handler(); //temp
	
	*dx = (int) mouse_x - WIN_WIDTH/2;
	*dy = (int) mouse_y - WIN_HEIGHT/2;
	
	//Now reset the mouse position to the center of the screen
	mouse_x = (double) WIN_WIDTH/2;
	mouse_y = (double) WIN_HEIGHT/2;
}

void mouse_get_delta_no_reset( int *dx, int *dy )
{
	if (!Mouse_installed) {
		*dx = *dy = 0;
		return;
	}
	mouse_handler(); //temp
	
	*dx = (int) mouse_x - WIN_WIDTH/2;
	*dy = (int) mouse_y - WIN_HEIGHT/2;
}

int mouse_get_btns()
{
	int  i;
	uint flag=1;
	int  status = 0;
	
	if (!Mouse_installed)
		return 0;

	mouse_handler(); //temp
	
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) {
		if (Mouse.pressed[i])
			status |= flag;
		flag <<= 1;
	}
	return status;
}

/* This should be replaced with mouse_button_down_count(int button)	*/
int mouse_went_down(int button)
{
	int count;
	
	if (!Mouse_installed)
		return 0;

	mouse_handler(); //temp
	
	if ((button < 0) || (button >= MOUSE_MAX_BUTTONS))
		return 0;
	
	//	_disable();		
	count = Mouse.num_downs[button];
	Mouse.num_downs[button] = 0;
	
	// 	_enable();
	return count;
}

// Returns how many times this button has went down since last call.
int mouse_button_down_count(int button)	
{
	int count;
	
	if (!Mouse_installed)
		return 0;
	
	mouse_handler(); //temp
	
	if ((button < 0) || (button >= MOUSE_MAX_BUTTONS))
		return 0;
	
	//	_disable();
	count = Mouse.num_downs[button];
	Mouse.num_downs[button] = 0;
	//	_enable();
	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)
{
	int state;
	
	if (!Mouse_installed)
		return 0;
	
	mouse_handler(); //temp
	
	if ((button < 0) || (button >= MOUSE_MAX_BUTTONS))
		return 0;
	
	//	_disable();
	state = Mouse.pressed[button];
	//	_enable();
	return state;
}

// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)	
{
	fix time_down, time;
	
	if (!Mouse_installed)
		return 0;

	mouse_handler(); //temp
	
	if ((button < 0) || (button >= MOUSE_MAX_BUTTONS))
		return 0;
	
	//	_disable();
	if (!Mouse.pressed[button]) {
		time_down = Mouse.time_held_down[button];
		Mouse.time_held_down[button] = 0;
	} else {
		time = timer_get_fixed_seconds();
		time_down = time - Mouse.time_held_down[button];
		Mouse.time_held_down[button] = 0;
	}
	//	_enable();
	
	return time_down;
}

void mouse_get_cyberman_pos( int *x, int *y )
{
}



//WHS: I made this :)
void hide_cursor()
{
	ShowCursor(FALSE);
}

void show_cursor()
{
	ShowCursor(TRUE);
}


/* New mouse pointer stuff for GGI/DGA */
//#include "cursor.h" /* cursor and mask */

typedef struct Sprite {
	ushort width;
	ushort height;
	byte *pixels;
	byte *mask;
} Sprite;

//Sprite mouse_sprite = { cursor_width, cursor_height, cursor_bits, cursor_mask_bits};

//byte *behind_mouse;
//byte behind_mouse[cursor_width*cursor_height];


void DrawMouse(void)
{
}


void EraseMouse(void)
{
}

//Should add a mode, relative, absolute
#define MOVE_REL 0
#define MOVE_ABS 1
//void MoveMouse(int mode, int x, int y) {

void MoveMouse(int x, int y)
{
}
