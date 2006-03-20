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



#ifndef _XTAPI_H
#define _XTAPI_H

#include "comm.h"

// Error Codes ----------------------------------------------------------------

#define XTAPI_SUCCESS					0
#define XTAPI_GENERAL_ERR				-1
#define XTAPI_BUSY						-2
#define XTAPI_NODEVICES					-3
#define XTAPI_INCOMPATIBLE_VERSION	-4
#define XTAPI_OUT_OF_MEMORY			-5
#define XTAPI_OUT_OF_RESOURCES		-6
#define XTAPI_NOT_SUPPORTED			-7
#define XTAPI_APPCONFLICT				-8
#define XTAPI_NOTLOCKED					-9
#define XTAPI_NOTOPEN					-10
#define XTAPI_FAIL						-127


//	Device Types ---------------------------------------------------------------

#define XTAPI_MODEM_DEVICE				1


//	XTAPI messages -------------------------------------------------------------

#define XTAPI_LINE_IDLE					1
#define XTAPI_LINE_DIALTONE			2
#define XTAPI_LINE_DIALING				3
#define XTAPI_LINE_RINGBACK			4
#define XTAPI_LINE_BUSY					5
#define XTAPI_LINE_FEEDBACK			6	// failed to connect.  phone company stuff
#define XTAPI_LINE_CONNECTED			7
#define XTAPI_LINE_DISCONNECTED		8
#define XTAPI_LINE_PROCEEDING			9
#define XTAPI_LINE_RINGING				10
#define XTAPI_LINE_UNDEFINED			127


// Data structures ------------------------------------------------------------

typedef struct tagTapiDevice {
	uint id;
	uint type;
	uint apiver;
	uint min_baud;
	uint max_baud;
} TapiDevice;


//	Functions ------------------------------------------------------------------

int xtapi_init(char *appname, int *numdevs);
int xtapi_shutdown();
int xtapi_enumdevices(TapiDevice *dev, int num);
int xtapi_lock(TapiDevice *dev);
int xtapi_unlock(TapiDevice *dev);

int xtapi_device_dialout(char *phonenum);
int xtapi_device_dialin();
int xtapi_device_answer();
int xtapi_device_poll_callstate(uint *state);
int xtapi_device_create_comm_object(COMM_OBJ *commobj);
int xtapi_device_hangup();

#endif
