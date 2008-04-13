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

/*
 *
 * prototype definitions for descent.cfg reading/writing
 *
 */


#ifndef _CONFIG_H
#define _CONFIG_H

#include "player.h"
#include "mission.h"

typedef struct Cfg
{
	ubyte DigiVolume;
	ubyte MidiVolume;
	ubyte RedbookVolume;
	int ReverseStereo;
	int GammaLevel;
	char LastPlayer[CALLSIGN_LEN+1];
	char LastMission[MISSION_NAME_LEN+1];
	int ResolutionX;
	int ResolutionY;
	int AspectX;
	int AspectY;
	int WindowMode;
	int TexFilt;
} __attribute__ ((packed)) Cfg;

extern struct Cfg GameCfg;

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);

//values for Config_control_type
#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_MOUSE 5
#define CONTROL_MAX_TYPES 8
#define CONTROL_JOYMOUSE 9

// old stuff - kept for compability reasons
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_CYBERMAN 6
#define CONTROL_WINJOYSTICK 7

#define D2X_KEYS 1

#endif
