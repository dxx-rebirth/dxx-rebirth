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



#ifndef _TACTILE_H
#define _TACTILE_H


#include "vecmat.h"

//#define TACTILE

#ifndef _BOOL_
typedef enum { False = 0, True = 1 } Bool;
#define _BOOL_
#endif


extern int TactileStick;
extern int Tactile_open(int);
extern void Tactile_apply_force (vms_vector *,vms_matrix *);
extern void Tactile_jolt (int,int,int);
extern void Tactile_Xvibrate (int,int);
extern void Tactile_Xvibrate_clear ();

/* DLL functions -moved from i-force.h */

#define 	MAX_POSITION_CHANNELS	4	/* Max num pos feedback chans */

typedef struct
{
	int	major;		/* major release number */
	int	minor;		/* minor release number */
	int	subminor;	/* subminor release number */
} FFversion;			/* firmware release version */

typedef struct
{
	int	month;	/* month of release */
	int	day;		/* day of release */
	int	year;		/* year of release */
} FFdate;

/*	Record containing all Joystick data
 *		Declare one of these structs in your app if you want
 *		Joystick data like serial number, model number, firmware
 *		version, etc reported back by the InitStick() command.
 *   Examples: (assuming 'StickRec' is declared as a Joystick Record)
 *      StickRec.SerialNumber - serial number
 *      StickRec.ForceAxes - number of force feedback axes
 */
typedef struct JoystickRecord
{
	int	ForceAxes;			/* number of force feedback axes */
	int	CommandAvailable;	/* command available */
	int	PositionAxes;		/* number of position feedback axes */
	int	Position[MAX_POSITION_CHANNELS];		/* 16-bit positions */

	FFversion	Version;			/* firmware release version */
	FFdate	Date;					/* firmware release date */
	unsigned long int SerialNumber;
	unsigned int	ModelNumber;

} JoystickRecord;

extern Bool	_cdecl InitStick(JoystickRecord *StickRec);
extern Bool	_cdecl GetStickStatus(char *StickStatus);
extern Bool	_cdecl Echo(void);
extern Bool _cdecl 	IForceAuthenticate(void);
extern int  _cdecl  GetStickError(void);
extern Bool _cdecl 	EnableForces(void);
extern Bool _cdecl 	DisableForces(void);
extern Bool _cdecl 	ClearForces(void);
extern Bool _cdecl 	CloseStick(void);
extern void	_cdecl SetJoystickPort(int Port);
extern void _cdecl 	SetJoystickAddressIRQ(unsigned int addr, int irq);

/* Force Effect Functions */
extern Bool _cdecl 	Jolt(unsigned int Magnitude,int Direction,unsigned int Duration);
extern Bool _cdecl 	ButtonReflexJolt(unsigned int ButtonAssign, unsigned int Magnitude,
		int direction, unsigned int Duration, unsigned int RepeatRate);
extern Bool _cdecl 	ButtonReflexClear(unsigned int ButtonAssign);
extern Bool _cdecl 	XVibration(unsigned int Lmagnitude, unsigned int Rmagnitude,
		unsigned int Frequency);
extern Bool _cdecl 	XVibrationClear(void);
extern Bool _cdecl 	YVibration(unsigned int Umagnitude, unsigned int Dmagnitude,
		unsigned int Frequency);
extern Bool _cdecl 	YVibrationClear(void);
extern Bool _cdecl 	Buffeting(unsigned int Magnitude);
extern Bool _cdecl 	BuffetingClear(void);
extern Bool _cdecl 	VectorForce(unsigned int Magnitude, int Direction);
extern Bool _cdecl 	VectorForceClear(void);
extern Bool _cdecl 	XYVectorForce(int XMagnitude,int YMagnitude);


#define TACTILE_IMMERSION 1
#define TACTILE_CYBERNET  2


#endif
