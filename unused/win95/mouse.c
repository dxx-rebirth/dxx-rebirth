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


#pragma off (unreferenced)
static char rcsid[] = "$Id: mouse.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <i86.h>

#include "error.h"
#include "fix.h"
#include "mouse.h"
#include "timer.h"
#include "mono.h"

#define ME_CURSOR_MOVED (1<<0)
#define ME_LB_P                         (1<<1)
#define ME_LB_R                         (1<<2)
#define ME_RB_P                         (1<<3)
#define ME_RB_R                         (1<<4)
#define ME_MB_P                         (1<<5)
#define ME_MB_R                         (1<<6)
#define ME_OB_P                         (1<<7)
#define ME_OB_R                         (1<<8)
#define ME_X_C                  (1<<9)
#define ME_Y_C                  (1<<10)
#define ME_Z_C                  (1<<11)
#define ME_P_C                  (1<<12)
#define ME_B_C                  (1<<13)
#define ME_H_C                  (1<<14)
#define ME_O_C                  (1<<15)

#define MOUSE_MAX_BUTTONS       11

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
	fix             ctime;
	ubyte           cyberman;
	int             num_buttons;
	ubyte           pressed[MOUSE_MAX_BUTTONS];
	fix             time_went_down[MOUSE_MAX_BUTTONS];
	fix             time_held_down[MOUSE_MAX_BUTTONS];
	uint            num_downs[MOUSE_MAX_BUTTONS];
	uint            num_ups[MOUSE_MAX_BUTTONS];
	event_info *x_info;
	ushort  button_status;
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
static HWND _hMouseWnd = 0;
static Mouse_center = 0;

extern int timer_initialized;


void mouse_set_window(HWND wnd)
{
	_hMouseWnd = wnd;
}

int Save_x=320,Save_y=240;

#define MOUSE_CENTER_X	160
#define MOUSE_CENTER_Y	100

int mouse_set_mode(int i)
{
	int old;
	old = Mouse_center;
	if (i) Mouse_center = 1;
	else Mouse_center = 0;

	if (Mouse_center) {
		int x,y;
		mouse_get_pos( &Save_x, &Save_y);		//save current pos
		mouse_get_delta(&x,&y);						//flush old movement
	}
	else
		mouse_set_pos( Save_x, Save_y);			//restore pos

	return old;
}



//--------------------------------------------------------
// returns 0 if no mouse
//           else number of buttons

int mouse_init(int enable_cyberman)
{
	if (Mouse_installed)
		return 2;
				
	Mouse_installed = 1;

	atexit( mouse_close );

	mouse_flush();
	

	return 2;
}



void mouse_close()
{
	if (Mouse_installed)    {
		Mouse_installed = 0;
		_hMouseWnd = 0;
	}
}


void mouse_set_limits( int x1, int y1, int x2, int y2 )
{

}

void mouse_get_pos( int *x, int *y)
{
	POINT point;

	GetCursorPos(&point);

	//ScreenToClient(_hMouseWnd, &point);
	*x = (int)point.x;
	*y = (int)point.y;
}

void mouse_get_delta( int *dx, int *dy )
{
	POINT point;
 
   GetCursorPos(&point);	
	*dx = (point.x-MOUSE_CENTER_X)/2;
	*dy = (point.y-MOUSE_CENTER_Y)/2;

	//mprintf((0,"C=%d (%d,%d) (%d,%d) ",Mouse_center,point.x,point.y,*dx,*dy));

	if (Mouse_center) 
   	//SetCursorPos (320,240);
   	SetCursorPos (MOUSE_CENTER_X,MOUSE_CENTER_Y);
}

int mouse_get_btns()
{
	int i;
	uint flag=1;
	int status = 0;

	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )    {
		if (Mouse.pressed[i])
			status |= flag;
		flag <<= 1;
	}
	return status;
}

void mouse_set_pos( int x, int y)
{
	POINT point;
	point.x = x;
	point.y = y;
	
	ClientToScreen(_hMouseWnd, &point);
	SetCursorPos(point.x, point.y);
}

void mouse_flush()
{
	int i;
	fix CurTime;


	//Clear the mouse data
	CurTime =timer_get_fixed_secondsX();
	for (i=0; i<MOUSE_MAX_BUTTONS; i++ )    {
		Mouse.pressed[i] = 0;
		Mouse.time_went_down[i] = CurTime;
		Mouse.time_held_down[i] = 0;
		Mouse.num_downs[i]=0;
		Mouse.num_ups[i]=0;
	}
}


// Returns how many times this button has went down since last call.
int mouse_button_down_count(int button) 
{
	int count;


	count = Mouse.num_downs[button];
	Mouse.num_downs[button]=0;

	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)      
{
	int state;
   

	state = Mouse.pressed[button];

	return state;
}



// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)  
{
	fix time_down, time;


	if ( !Mouse.pressed[button] )   {
		time_down = Mouse.time_held_down[button];
		Mouse.time_held_down[button] = 0;
	} else  {
		time = timer_get_fixed_secondsX();
		time_down =  time - Mouse.time_went_down[button];
		Mouse.time_went_down[button] = time;
	}

	return time_down;
}

void mouse_get_cyberman_pos( int *x, int *y )
{
	*x = 0;
	*y = 0;
}


//	Mouse Callback from windows

void mouse_win_callback(UINT msg, UINT wParam, UINT lParam)
{
	if (!timer_initialized) return;

	Mouse.ctime = timer_get_fixed_secondsX();
	
	switch (msg)
	{
		case WM_LBUTTONDOWN:
			mprintf ((0,"Left down!\n"));

			if (!Mouse.pressed[MB_LEFT]) {
				Mouse.pressed[MB_LEFT] = 1;
				Mouse.time_went_down[MB_LEFT] = Mouse.ctime;
			}
			Mouse.num_downs[MB_LEFT]++;
			break;

		case WM_LBUTTONUP:
			if (Mouse.pressed[MB_LEFT]) {
				Mouse.pressed[MB_LEFT] = 0;
				Mouse.time_held_down[MB_LEFT] += Mouse.ctime-Mouse.time_went_down[MB_LEFT];
			}
			Mouse.num_ups[MB_LEFT]++;
			break;

		case WM_RBUTTONDOWN:
	
			mprintf ((0,"Right down!\n"));
			if (!Mouse.pressed[MB_RIGHT]) {
				Mouse.pressed[MB_RIGHT] = 1;
				Mouse.time_went_down[MB_RIGHT] = Mouse.ctime;
			}
			Mouse.num_downs[MB_RIGHT]++;
			break;

		case WM_RBUTTONUP:
			if (Mouse.pressed[MB_RIGHT])	{
				Mouse.pressed[MB_RIGHT] = 0;
				Mouse.time_held_down[MB_RIGHT] += Mouse.ctime-Mouse.time_went_down[MB_RIGHT];
			}
			Mouse.num_ups[MB_RIGHT]++;
			break;

		case WM_MBUTTONDOWN:
			if (!Mouse.pressed[MB_MIDDLE])	{
				Mouse.pressed[MB_MIDDLE] = 1;
				Mouse.time_went_down[MB_MIDDLE] = Mouse.ctime;
			}
			Mouse.num_downs[MB_MIDDLE]++;
			break;

		case WM_MBUTTONUP:
			if (Mouse.pressed[MB_MIDDLE])	{
				Mouse.pressed[MB_MIDDLE] = 0;
				Mouse.time_held_down[MB_MIDDLE] += Mouse.ctime-Mouse.time_went_down[MB_MIDDLE];
			}
			Mouse.num_ups[MB_MIDDLE]++;
			break;

	}
}



		
