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
static char rcsid[] = "$Id: xtapi.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <tapi.h>

#include "pstypes.h"
#include "mono.h"
#include "error.h"
#include "winapp.h"
#include "xtapi.h"


/*	The TAPI layer will interface between Descent 2 and Win32 TAPI.
	
	The application will go about using XTAPI when wanting to dial out
	to another player (modem) or when listening for a ring in.

	First an application will initialize the library.
	Then one needs to find the required device id by enumerating
	the devices and then selecting a device id.
	
	Then we open this device by calling xtapi_lock.  The application
	can only use this device between lock and unlock.  Also all functions
	with the device extension will act upon this locked device.
	When done with a device call xtapi_unlock.

	To dial-out.  Call xtapi_device_dialout.  

*/

#define TAPI_VERSION 0x00010004



/*
 *	Data
*/

struct tagTapiObj {
	HLINEAPP hObj;
	HWND hWnd;
	DWORD num_devs;
} TapiObj = { NULL, NULL, 0 };

struct tagTapiDev {
	BOOL valid;
	DWORD id;
	DWORD apiver;
	DWORD type;
	HLINE hLine;
	HCALL hCall;
	DWORD call_state;
	BOOL connected;
	BOOL lineReplyReceived;
 	BOOL callStateReceived;
	DWORD asyncRequestedID;
 	LONG lineReplyResult;

} TapiDev = { FALSE, 0, 0, 0, NULL, NULL, 0, FALSE, FALSE, FALSE, 0, 0 };	



/* 
 * Local function prototypes 
*/

void CALLBACK			tapi_callback(DWORD hDevice, DWORD dwMsg, DWORD dwCallbackInst, 
												DWORD dw1, 
												DWORD dw2, 
					    						DWORD dw3);

LINEDEVCAPS*			tapi_getdevcaps(uint dev);
LINEADDRESSSTATUS*	tapi_line_getaddrstatus(HLINE hLine, DWORD dwID);
LINEADDRESSCAPS*		tapi_line_getaddrcaps(uint dev, DWORD addrID);
LINECALLSTATUS*		tapi_line_getcallstatus(HCALL hCall);
LINECALLINFO*			tapi_line_getcallinfo(HCALL hCall);

int 						xtapi_err(LONG val);
LONG						xtapi_device_wait_for_reply(LONG reqID); 



/*
 * Functions
*/

/*	init
		initializes the TAPI interface

	returns:
		XTAPI_NODEVICES if no devices found
		XTAPI_BUSY if we are calling this function again for some reason
					  (non-reentrant)
		XTAPI_APPCONFLICT TAPI is being used somewhere else, and we can't gain
								  control
		XTAPI_INCOMPATIBLE_VERSION this interface is old or not supported
										   under current Win32.
*/

int xtapi_init(char *appname, int *numdevs)
{
	LONG retval;
	BOOL reinit;

	if (TapiObj.hObj) return XTAPI_SUCCESS;
	
	TapiObj.hWnd = GetLibraryWindow();

	reinit = TRUE;								// Allow for reinitialization

	while (1)
	{
		retval = lineInitialize(&TapiObj.hObj, 
						GetWindowInstance(TapiObj.hWnd),
						tapi_callback, 
						appname, 
						&TapiObj.num_devs);
		
		if (!retval) {
			break;
		}
		else if (retval == LINEERR_REINIT) {
			if (reinit)  {
				timer_delay(0x50000);		// delay 5 seconds 
				reinit = FALSE;
			}
			else {
				return XTAPI_APPCONFLICT;	// another app is using tapi resources we need
			}
		}
		else if (retval == LINEERR_NODEVICE) {
			return XTAPI_NODEVICES;			// should tell user to install a modem
		}
		else return XTAPI_GENERAL_ERR;
	}

	*numdevs = (int)TapiObj.num_devs;
	
	TapiDev.valid = FALSE;

	return XTAPI_SUCCESS;
}


/* shutdown
		performs a shutdown of the TAPI system
*/

int xtapi_shutdown()
{
	LONG retval;
	
	if (!TapiObj.hObj) return 0;

//	Close any device that may be open at this point!

//	shutdown TAPI
	retval = lineShutdown(TapiObj.hObj);
	if (retval!=0)	return xtapi_err(retval);

	TapiObj.hObj = NULL;
	TapiObj.num_devs = 0;
	TapiObj.hWnd = NULL;	

	return 0;
}


/* enumdevices
		enumerates the devices usable by TAPI.  good for
		letting the user select the device and TAPI to take over
*/

int xtapi_enumdevices(TapiDevice *dev, int num)
{
	LONG retval;
	int i;

	Assert(TapiObj.hObj != NULL);
	Assert(num <= TapiObj.num_devs);
					 
	for (i = 0; i < num; i++) 
	{
		LINEEXTENSIONID extid;
		LINEDEVCAPS *caps;
	
		caps = tapi_getdevcaps(i);
		if (caps == NULL) return XTAPI_OUT_OF_MEMORY;

		if ((caps->dwBearerModes & LINEBEARERMODE_VOICE) &&
			 (caps->dwMediaModes & LINEMEDIAMODE_DATAMODEM)) {
			dev[i].type = XTAPI_MODEM_DEVICE;
		}
		else dev[i].type = 0;

		retval = lineNegotiateAPIVersion(TapiObj.hObj, 
								i,	TAPI_VERSION, TAPI_VERSION,
								(DWORD *)&dev[i].apiver, &extid);

		if (retval == LINEERR_INCOMPATIBLEAPIVERSION)
			return XTAPI_INCOMPATIBLE_VERSION;
		else if (retval != 0)
			return xtapi_err(retval);

		dev[i].id = i;
		dev[i].min_baud = (uint)caps->dwMaxRate;
		dev[i].max_baud = (uint)caps->dwMaxRate;

		free(caps);			
	}
		
	return XTAPI_SUCCESS;
}


/*	lock
		this function sets the specified device as the current device to be
		used by the xtapi_device system
*/

int xtapi_lock(TapiDevice *dev)
{
	if (TapiDev.valid) return XTAPI_BUSY;
	
	TapiDev.id = (DWORD)dev->id;
	TapiDev.apiver = (DWORD)dev->apiver;
	TapiDev.type = (DWORD)dev->type;
	TapiDev.hLine = NULL;
	TapiDev.hCall = NULL;
	TapiDev.connected = FALSE;
	TapiDev.call_state = 0;
	TapiDev.lineReplyReceived = FALSE;
	TapiDev.callStateReceived = FALSE;
	TapiDev.lineReplyResult = 0;
	TapiDev.valid = TRUE;	
	
	return XTAPI_SUCCESS;
}


/*	unlock
		this functions just releases the device.  device functions won't work
		anymore until another lock
*/

int xtapi_unlock(TapiDevice *dev)
{
	if (!TapiDev.valid) return XTAPI_NOTLOCKED;

	if (TapiDev.hCall || TapiDev.hLine) 
		xtapi_device_hangup();
	
	TapiDev.id = 0;
	TapiDev.apiver = 0;
	TapiDev.hLine = NULL;
	TapiDev.hCall = NULL;
	TapiDev.call_state = 0;
	TapiDev.valid = FALSE;

	return XTAPI_SUCCESS;
}


/* device_dial 
		this function will dialout a given number through the current
		device.
*/

int xtapi_device_dialout(char *phonenum)
{
	LINEDEVCAPS *ldevcaps=NULL;
	LINECALLPARAMS *lcallparams=NULL;
	LONG retval;
	int xtapi_ret = XTAPI_SUCCESS;

	if (TapiDev.hLine) return XTAPI_BUSY;

//	check if we can dial out
	if (TapiDev.type != XTAPI_MODEM_DEVICE) {
		xtapi_ret = XTAPI_NOT_SUPPORTED;
		goto dialout_exit;
	}
	ldevcaps = tapi_getdevcaps(TapiDev.id);
	if (!ldevcaps) return XTAPI_OUT_OF_MEMORY;
	if (!(ldevcaps->dwLineFeatures & LINEFEATURE_MAKECALL)) {
		xtapi_ret = XTAPI_NOT_SUPPORTED;
		goto dialout_exit;
	}

//	open the line!
	retval = lineOpen(TapiObj.hObj, TapiDev.id, 
								&TapiDev.hLine,
								TapiDev.apiver, 0, 0, 
								LINECALLPRIVILEGE_NONE,
								LINEMEDIAMODE_DATAMODEM,
								0);
	if (retval != 0) {
		xtapi_ret = xtapi_err(retval);
		goto dialout_exit;
	}

	retval = lineSetStatusMessages(TapiDev.hLine, 
						LINEDEVSTATE_OTHER |
						LINEDEVSTATE_RINGING | LINEDEVSTATE_CONNECTED | 
						LINEDEVSTATE_DISCONNECTED | LINEDEVSTATE_OUTOFSERVICE |
						LINEDEVSTATE_MAINTENANCE |	LINEDEVSTATE_REINIT,
						LINEADDRESSSTATE_INUSEZERO |
						LINEADDRESSSTATE_INUSEONE |
						LINEADDRESSSTATE_INUSEMANY);

	if (retval != 0) {
		xtapi_ret = xtapi_err(retval);
		goto dialout_exit;
	}

//	Setup calling parameters
	lcallparams = (LINECALLPARAMS *)malloc(sizeof(LINECALLPARAMS)+strlen(phonenum)+1);
	if (!lcallparams) {
		xtapi_ret = XTAPI_OUT_OF_MEMORY;
		goto dialout_exit;
	}
	memset(lcallparams, 0, sizeof(LINECALLPARAMS)+strlen(phonenum)+1);							
	lcallparams->dwTotalSize = sizeof(LINECALLPARAMS)+strlen(phonenum)+1;
	lcallparams->dwBearerMode = LINEBEARERMODE_VOICE;
	lcallparams->dwMediaMode = LINEMEDIAMODE_DATAMODEM;
	lcallparams->dwCallParamFlags = LINECALLPARAMFLAGS_IDLE;
	lcallparams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
	lcallparams->dwAddressID = 0;
	lcallparams->dwDisplayableAddressOffset = sizeof(LINECALLPARAMS);
	lcallparams->dwDisplayableAddressSize = strlen(phonenum)+1;
	strcpy((LPSTR)lcallparams + sizeof(LINECALLPARAMS), phonenum);
	
//	Dial it!
	mprintf((0, "XTAPI: dialing %s.\n", phonenum));
	retval = xtapi_device_wait_for_reply(
		lineMakeCall(TapiDev.hLine, &TapiDev.hCall, NULL, 0, lcallparams)
	);
	if (retval < 0) {
		xtapi_ret = xtapi_err(retval);
		goto dialout_exit;
	}

	retval = xtapi_device_wait_for_reply(
		lineDial(TapiDev.hCall, phonenum, 0)
	);
	if (retval < 0) {
		xtapi_ret = xtapi_err(retval);
		goto dialout_exit;
	}
	
dialout_exit:
	if (lcallparams) free(lcallparams);
	if (ldevcaps) free(ldevcaps);
	
	return xtapi_ret;
}


/* device_dialin
		sets up modem to wait for a call.  All this function does
		is initialize the line.
*/

int xtapi_device_dialin()
{
	LONG retval;
	int xtapi_ret = XTAPI_SUCCESS;

	if (TapiDev.hLine) return XTAPI_BUSY;

//	check if we can dial out
	if (TapiDev.type != XTAPI_MODEM_DEVICE) {
		xtapi_ret = XTAPI_NOT_SUPPORTED;
		goto dialin_exit;
	}

//	open the line!
	retval = lineOpen(TapiObj.hObj, TapiDev.id, 
								&TapiDev.hLine,
								TapiDev.apiver, 0, 0, 
								LINECALLPRIVILEGE_OWNER,
								LINEMEDIAMODE_DATAMODEM,
								0);
	if (retval != 0) {
		xtapi_ret = xtapi_err(retval);
		goto dialin_exit;
	}

dialin_exit:
	return xtapi_ret;
}


/* device_answer
		the line should be open, and all this function does is grab the call
*/
		
int xtapi_device_answer()
{
	LONG retval;
	int xtapi_ret = XTAPI_SUCCESS;

	if (!TapiDev.hCall) return XTAPI_NOTOPEN;

	retval = xtapi_device_wait_for_reply(
			lineAnswer(TapiDev.hCall, NULL, 0)
	);
	
	if (retval < 0) {
		xtapi_ret = xtapi_err(retval);
	}

	return xtapi_ret;
}


/* device_poll_callstate
		we can be informed of the current state of a call made after the
		reply from lineMakeCall
*/

int xtapi_device_poll_callstate(uint *state)
{
//	 perform translation from TAPI to XTAPI!

	if (TapiDev.callStateReceived) {
		switch (TapiDev.call_state) 
		{
			case LINECALLSTATE_IDLE: *state = XTAPI_LINE_IDLE; break;
			case LINECALLSTATE_DIALTONE: *state = XTAPI_LINE_DIALTONE; break;
			case LINECALLSTATE_DIALING: *state = XTAPI_LINE_DIALING; break;
			case LINECALLSTATE_RINGBACK: *state = XTAPI_LINE_RINGBACK; break;
			case LINECALLSTATE_BUSY: *state = XTAPI_LINE_BUSY; break;
			case LINECALLSTATE_SPECIALINFO: *state = XTAPI_LINE_FEEDBACK; break;
			case LINECALLSTATE_CONNECTED: *state = XTAPI_LINE_CONNECTED; break;
			case LINECALLSTATE_DISCONNECTED: *state = XTAPI_LINE_DISCONNECTED; break;
			case LINECALLSTATE_PROCEEDING: *state = XTAPI_LINE_PROCEEDING; break;			
			case LINECALLSTATE_OFFERING: *state = XTAPI_LINE_RINGING; break;
			default:
				mprintf((0, "call_state: %x\n", TapiDev.call_state));
				*state = XTAPI_LINE_UNDEFINED;
		}
		TapiDev.callStateReceived = FALSE;
		TapiDev.call_state = 0;
	}
	else *state = 0;

	return XTAPI_SUCCESS;
}


/* device_create_comm_object
		once we are connected, we can create a COMM_OBJ to actually send
		and receive data through the modem.
*/

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13


int xtapi_device_create_comm_object(COMM_OBJ *commobj)
{
	VARSTRING *varstr = NULL;
	DWORD varstrsize;
	HANDLE hCommHandle=NULL;
	LINECALLINFO *lcinfo = NULL;
	int retval;
	int errval = XTAPI_SUCCESS;

	Assert(TapiDev.connected);

	varstrsize = sizeof(VARSTRING) + 1024;
	
	while (1)
	{
		varstr = (VARSTRING *)realloc(varstr, varstrsize);
		if (!varstr) {
			errval = XTAPI_OUT_OF_MEMORY;
			goto device_create_comm_exit;
		}

		varstr->dwTotalSize = varstrsize;

		retval = lineGetID(0,0,TapiDev.hCall, LINECALLSELECT_CALL, varstr,
						"comm/datamodem");
		errval = xtapi_err(retval);
		
		if (varstr->dwNeededSize > varstr->dwTotalSize) {
			varstrsize = varstr->dwNeededSize;
		}
		else break;
	}

	if (errval != XTAPI_SUCCESS) return errval;

	hCommHandle = *((LPHANDLE)((LPBYTE)varstr+varstr->dwStringOffset));

	lcinfo = tapi_line_getcallinfo(TapiDev.hCall);
	if (!lcinfo) {
		errval = XTAPI_OUT_OF_MEMORY;
		goto device_create_comm_exit;
	}


//	Create the COMM compatible COMM_OBJ
//	Most COMM settings will be set by TAPI, so this is less intensive than the 
//	COMM open connection
	{
		COMMTIMEOUTS ctimeouts;

		memset(commobj, 0, sizeof(COMM_OBJ));
	
		if (GetFileType(hCommHandle) != FILE_TYPE_CHAR) {
			errval = XTAPI_GENERAL_ERR;
			goto device_create_comm_exit;
		}
	
		GetCommState(hCommHandle, &commobj->dcb);
		GetCommTimeouts(hCommHandle, &ctimeouts);

		commobj->handle = hCommHandle;
		commobj->baud = lcinfo->dwRate;


//		commobj->dcb.BaudRate 		= commobj->baud;
//		commobj->dcb.fBinary			= 1;
//		commobj->dcb.fNull 			= 0;
//		commobj->dcb.ByteSize 		= 8;
//		commobj->dcb.StopBits 		= ONESTOPBIT;
//		commobj->dcb.fParity			= FALSE;
//		commobj->dcb.Parity 			= NOPARITY;
//	   commobj->dcb.XonChar 		= ASCII_XON;
//	   commobj->dcb.XoffChar 		= ASCII_XOFF;
//	   commobj->dcb.XonLim 			= 1024;
//   	commobj->dcb.XoffLim 		= 1024;
//	   commobj->dcb.EofChar 		= 0;
//	   commobj->dcb.EvtChar 		= 0;
//		commobj->dcb.fOutxDsrFlow	= FALSE;
//		commobj->dcb.fOutxCtsFlow	= FALSE;					// rts/cts off

//		commobj->dcb.fDtrControl	= DTR_CONTROL_ENABLE;// dtr=on
//		commobj->dcb.fRtsControl	= RTS_CONTROL_ENABLE;

      ctimeouts.ReadIntervalTimeout         = 250;
      ctimeouts.ReadTotalTimeoutMultiplier  = 0;
      ctimeouts.ReadTotalTimeoutConstant    = 0;
      ctimeouts.WriteTotalTimeoutMultiplier = 0;
      ctimeouts.WriteTotalTimeoutConstant   = 0;

      commobj->dcb.fAbortOnError = FALSE;

      SetCommTimeouts(hCommHandle, &ctimeouts);
      SetCommState(hCommHandle, &commobj->dcb);
	}

	memset(&commobj->rov, 0, sizeof(OVERLAPPED));
	memset(&commobj->wov, 0, sizeof(OVERLAPPED));
	commobj->rov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (commobj->rov.hEvent == NULL) {
		errval = XTAPI_GENERAL_ERR;
		goto device_create_comm_exit;
	}
	commobj->wov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (commobj->wov.hEvent == NULL) {
		CloseHandle(commobj->rov.hEvent);
		return 0;
	}

	commobj->hThread = NULL;
	commobj->threadID = (DWORD)(-1);
	commobj->connect = TRUE;

device_create_comm_exit:
	if (varstr) free(varstr);
	if (lcinfo) free(lcinfo);

	return errval;
}


/* device_hangup
		frees the call and line 

	damn well better assume that the COMM_OBJ allocated (if at all) is 
	closed via the comm_library.
*/

int xtapi_device_hangup()
{
	LONG retval;

	Assert(TapiDev.valid);					// Device should be locked!

//	if	(!TapiDev.hCall) return XTAPI_SUCCESS;
//	if (!TapiDev.hLine) return XTAPI_SUCCESS;

//	drop any call in progress
	if (TapiDev.hCall) {
		LINECALLSTATUS *lcallstat = NULL;
		MSG msg;
		DWORD call_state;		

		lcallstat = tapi_line_getcallstatus(TapiDev.hCall);
		if (!lcallstat) {
			return XTAPI_OUT_OF_MEMORY;
		}

		mprintf((0, "XTAPI: Got linestatus.\n"));
	
		if (!(lcallstat->dwCallState & LINECALLSTATE_IDLE))  {
		// line not IDLE so drop it!
			retval = xtapi_device_wait_for_reply(
				lineDrop(TapiDev.hCall, NULL, 0)
			);

			if (retval != XTAPI_SUCCESS) {
				mprintf((1, "XTAPI: error when lineDrop.\n"));
			}

			mprintf((0, "XTAPI: dropped line.\n"));
									
		// wait for IDLE
			mprintf((0, "XTAPI: Waiting for idle.\n"));
			while (1)
			{
				xtapi_device_poll_callstate((uint*)&call_state);
				if (call_state == XTAPI_LINE_IDLE) break;
	  			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
   	      	TranslateMessage(&msg);
      	     	DispatchMessage(&msg);
        		}
			}
		}
		retval = lineDeallocateCall(TapiDev.hCall);

		mprintf((0, "XTAPI: deallocated call.\n"));

		if (retval != 0) {
			free(lcallstat);
			return XTAPI_GENERAL_ERR;
		}
		TapiDev.hCall = NULL;		
		
		if (lcallstat) free(lcallstat);
	}

//	Free up line.
	if (TapiDev.hLine) {
		retval = lineClose(TapiDev.hLine);
		if (retval != 0) {
			return XTAPI_GENERAL_ERR;
		}
		TapiDev.hLine = NULL;		
	}

	mprintf((0, "XTAPI: Closed line.\n"));

	TapiDev.connected = FALSE;
	
	return XTAPI_SUCCESS;
}

		
/*	device_wait_for_reply
		once a function is called, we wait until we have been given a reply.
*/
LONG xtapi_device_wait_for_reply(LONG reqID)
{
	if (reqID > 0) {						// A valid ID.  so we shall wait and see
		MSG msg;
		
		TapiDev.lineReplyReceived = FALSE;
		TapiDev.asyncRequestedID = (DWORD)reqID;
 		TapiDev.lineReplyResult = LINEERR_OPERATIONFAILED;

		while (!TapiDev.lineReplyReceived) 
		{
  			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
         	TranslateMessage(&msg);
           	DispatchMessage(&msg);
        	}
			
		// Insert code to take care of shutting down while wait.
		}

		return (LONG)TapiDev.lineReplyResult;
	}

	return reqID;
}		


/*	err
		translate tapi to xtapi errors
*/

int xtapi_err(LONG err)
{
	mprintf((1, "TAPI err: %x\n", (DWORD)err));
	switch (err)
	{
		case 0:
			return XTAPI_SUCCESS;

		case LINEERR_ALLOCATED:
			return XTAPI_APPCONFLICT;
	
		case LINEERR_NOMEM:
			return XTAPI_OUT_OF_MEMORY;
		
		case LINEERR_INCOMPATIBLEAPIVERSION:
			return XTAPI_INCOMPATIBLE_VERSION;
		
		case LINEERR_NODEVICE:
			return XTAPI_NODEVICES;

		case LINEERR_OPERATIONFAILED:
			return XTAPI_FAIL;
	
		case LINEERR_OPERATIONUNAVAIL:
			return XTAPI_NOT_SUPPORTED;

		case LINEERR_RESOURCEUNAVAIL:
			return XTAPI_OUT_OF_RESOURCES;

		case LINEERR_REINIT:
			return XTAPI_BUSY;
	}
	return XTAPI_GENERAL_ERR;
}


/* callback
		this function is used by TAPI to inform us of when asynchronous
		requests are valid and of any changes to the current call_state
*/

void CALLBACK tapi_callback(DWORD hDevice, DWORD dwMsg, DWORD dwCallbackInst, 
												DWORD dw1, 
												DWORD dw2, 
					    						DWORD dw3)
{
	switch(dwMsg)
	{
		case LINE_REPLY:
			if (dw2 != 0) 
				mprintf((1, "XTAPI: LINE_REPLY err: %x.\n", dw2));
			else 
				mprintf((1, "XTAPI: LINE_REPLY received.\n"));
			if (TapiDev.asyncRequestedID == dw1) {
				TapiDev.lineReplyReceived = TRUE;
				TapiDev.lineReplyResult = (LONG)dw2;
			}
			break;
		
		case LINE_CALLSTATE:
			TapiDev.callStateReceived = TRUE;
			TapiDev.call_state = dw1;

			mprintf((0, "Call_State  = %x\n", dw1));			
			switch(TapiDev.call_state)
			{
				case LINECALLSTATE_CONNECTED:
					if (TapiDev.connected) break;
					TapiDev.connected = TRUE;
					break;
	
				case LINECALLSTATE_OFFERING:
					if (TapiDev.connected) {
						mprintf((1, "TAPI: offering after connected!\n"));
						break;
					}
					if (dw3 != LINECALLPRIVILEGE_OWNER) {
						mprintf((1, "TAPI: need owner privilege to accept call!.\n"));
						break;
					}
					TapiDev.hCall = (HCALL)hDevice;
					break;
			}
			break;
	}
}
				


/* 
 * TAPI functions
*/

/* tapi_getdevcaps
		gets device caps for specified device and returns a valid structure
*/

LINEDEVCAPS *tapi_getdevcaps(uint dev)
{
	LINEDEVCAPS *pcaps=NULL;
	LONG retval, size;
	DWORD apiver;

	size = sizeof(LINEDEVCAPS) + 256;
	
	while (1)
	{
		LINEEXTENSIONID extid;

		retval = lineNegotiateAPIVersion(TapiObj.hObj, 
								dev,	TAPI_VERSION, TAPI_VERSION,
								&apiver, &extid);

		if (retval != 0)
			return NULL;

		pcaps = (LINEDEVCAPS *)realloc(pcaps, size);
		if (!pcaps) return NULL;
		 
		memset(pcaps, 0, size);
		pcaps->dwTotalSize = size;

		retval = lineGetDevCaps(TapiObj.hObj, dev,
						apiver, 0, 
						pcaps);

		if (retval!=0) {
			free(pcaps);
			return NULL;
		}
		
		if (pcaps->dwNeededSize > pcaps->dwTotalSize) {
			size = pcaps->dwNeededSize;
			continue;
		}
		else break;
	}

	return pcaps;
}

/*	tapi_line_getaddrstatus
		retrieves the current status of a given address on the line.

	returns:
		NULL if fail.	
*/

LINEADDRESSSTATUS *tapi_line_getaddrstatus(HLINE hLine, DWORD dwID)
{
	LINEADDRESSSTATUS *ad=NULL;
	LONG retval;
	int size;

	size = sizeof(LINEADDRESSSTATUS) + 256;

	while (1)
	{
		ad = (LINEADDRESSSTATUS *)realloc(ad, size);
		if (!ad) return NULL;
		 
		memset(ad, 0, size);
		ad->dwTotalSize = size;

		retval = lineGetAddressStatus(hLine, dwID, ad);

		if (retval!=0) {
			free(ad);
			return NULL;
		}
		
		if (ad->dwNeededSize > ad->dwTotalSize) {
			size = ad->dwNeededSize;
			continue;
		}
		else break;
	}

	return ad;
}
	
		
/*	tapi_line_getaddrcaps
		retrieves the current caps of a given address on the line.

	returns:
		NULL if fail.	
*/

LINEADDRESSCAPS *tapi_line_getaddrcaps(uint dev, DWORD addrID)
{
	LINEADDRESSCAPS *ad=NULL;
	LONG retval;
	int size;

	size = sizeof(LINEADDRESSCAPS) + 256;

	while (1)
	{
		DWORD apiver;
		LINEEXTENSIONID extid;

		retval = lineNegotiateAPIVersion(TapiObj.hObj, 
								dev,	TAPI_VERSION, TAPI_VERSION,
								&apiver, &extid);

		if (retval != 0)
			return NULL;

		ad = (LINEADDRESSCAPS *)realloc(ad, size);
		if (!ad) return NULL;
		 
		memset(ad, 0, size);
		ad->dwTotalSize = size;

		retval = lineGetAddressCaps(TapiObj.hObj, dev, 
									addrID, apiver, 0,
									ad);

		if (retval!=0) {
			free(ad);
			return NULL;
		}
		
		if (ad->dwNeededSize > ad->dwTotalSize) {
			size = ad->dwNeededSize;
			continue;
		}
		else break;
	}

	return ad;
}


/*	tapi_line_getcallstatus
		retrieves the current status of a given call on the line.

	returns:
		NULL if fail.	
*/

LINECALLSTATUS *tapi_line_getcallstatus(HCALL hCall)
{
	LINECALLSTATUS *ad=NULL;
	LONG retval;
	int size;

	size = sizeof(LINECALLSTATUS) + 256;

	while (1)
	{
		ad = (LINECALLSTATUS *)realloc(ad, size);
		if (!ad) return NULL;
		 
		memset(ad, 0, size);
		ad->dwTotalSize = size;

		retval = lineGetCallStatus(hCall,ad); 

		if (retval!=0) {
			free(ad);
			return NULL;
		}
		
		if (ad->dwNeededSize > ad->dwTotalSize) {
			size = ad->dwNeededSize;
			continue;
		}
		else break;
	}

	return ad;
}


/*	tapi_line_getcallinfo
		retrieves the current status of a given call on the line.

	returns:
		NULL if fail.	
*/

LINECALLINFO *tapi_line_getcallinfo(HCALL hCall)
{
	LINECALLINFO *ad=NULL;
	LONG retval;
	int size;

	size = sizeof(LINECALLINFO) + 256;

	while (1)
	{
		ad = (LINECALLINFO *)realloc(ad, size);
		if (!ad) return NULL;
		 
		memset(ad, 0, size);
		ad->dwTotalSize = size;

		retval = lineGetCallInfo(hCall,ad); 

		if (retval!=0) {
			free(ad);
			return NULL;
		}
		
		if (ad->dwNeededSize > ad->dwTotalSize) {
			size = ad->dwNeededSize;
			continue;
		}
		else break;
	}

	return ad;
}
