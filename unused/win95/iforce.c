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
static char rcsid[] = "$Id: iforce.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#include "joy.h"
#include "tactile.h"
#include "mono.h"
#include "winapp.h"
#include "iforce.h"

static JoystickRecord 	iForceCaps;
static int					iForceInit=FALSE;


#define WIN_TACTILE_ON

//	----------------------------------------------------------------------------

#ifdef WIN_TACTILE_ON

extern Bool _cdecl InitStick(JoystickRecord*);

int IForce_Init(int port)
{
	Bool result;

	if (iForceInit) return 1;

	if (port < 1 || port > 4) return 0;

	SetJoystickPort(port);	
	result = InitStick(&iForceCaps);
	if (!result) {
		logentry( "IFORCE: Initialization failed!.\n");
		return 0;
	}
	
	if (!EnableForces()) {
		logentry("IFORCE: Unable to enable forces.\n");
		return 0;
	}

	ClearForces();

	iForceInit = TRUE;

	logentry("IFORCE: Initialization complete.\n");

	return 1;
}


int IForce_Close()
{
	int i = 0;

	if (!iForceInit) return 1;

	while (i < 5) {
		CloseStick();
		i++;
	}
	return 1;
}	


int IForce_GetCaps(IForce_Caps *caps)
{
	if (!iForceInit) return 0;

	if (iForceCaps.PositionAxes >= 2) {
		caps->axes_mask = JOY_1_X_AXIS;
		caps->axes_mask &= JOY_1_Y_AXIS;
	}
	if (iForceCaps.PositionAxes == 4) {
		caps->axes_mask &= JOY_1_POV;
		caps->has_pov = 1;
	}
	return 1;
}		


void IForce_ReadRawValues(int *axis)
{
	JOYINFOEX	joy;

	if (!iForceInit) return;
	
	memset(&joy, 0, sizeof(joy));
	joy.dwSize = sizeof(joy);
	joy.dwFlags = JOY_RETURNALL | JOY_USEDEADZONE;
	joyGetPosEx(JOYSTICKID1, &joy);

	axis[0] = joy.dwXpos;
	axis[1] = joy.dwYpos;
	axis[2] = joy.dwZpos;
	axis[4] = joy.dwRpos;
	axis[5] = joy.dwUpos;
	axis[6] = joy.dwVpos;
	axis[3] = joy.dwPOV;
}

#endif
