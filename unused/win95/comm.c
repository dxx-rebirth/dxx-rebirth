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
static char rcsid[] = "$Id: comm.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define WIN32_LEAN_AND_MEAN
#define WIN95
#define _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>

#include "mono.h"
#include "winapp.h"
#include "comm.h"
#include "error.h"

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

#define MODEM_REGISTRY_PATH "System\\CurrentControlSet\\Services\\Class\\Modem"


static long comm_delay_val = 0;

DWORD FAR PASCAL comm_thread_proc(LPSTR lpdata);
int FAR PASCAL comm_idle_function(COMM_OBJ *obj);
void comm_dump_info(COMM_OBJ *obj);


//	---------------------------------------------------------------------
 
int comm_get_modem_info(int modem, COMM_OBJ *obj)
{
	HKEY hKey, hSubKey;
	char path[256];
	char buf[32];
 	DWORD len, err, type;

	memset(obj, 0, sizeof(COMM_OBJ));
	obj->connect = FALSE;

	if (modem < 0) {
	//	Direct Link
		obj->type = 0;
		obj->port = (char)(modem * -1);
		return 1;
 	}
	else obj->type = 1;
	
	sprintf(buf, "\\%04d", modem);

	strcpy(path, MODEM_REGISTRY_PATH);
	strcat(path, buf);
	
	mprintf((0, "Looking for modem in HKEY_LOCAL_MACHINE\\%s\n", path));

	err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey);

	if (err != ERROR_SUCCESS) return 0;

//	We have a key into the modem 000x
//	Retreive all pertinant info.
	len = sizeof(buf);
	err = RegQueryValueEx(hKey, "Model", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}

	strcpy(obj->name, buf);

	len = sizeof(buf);
	err = RegQueryValueEx(hKey, "PortSubClass", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}
	mprintf((0, "COM:%d\n", buf[0]));	
 
	obj->port = buf[0];

	len = sizeof(buf);
	err = RegQueryValueEx(hKey, "Reset", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}

	strcpy(obj->cmd[COMM_RESET_CMD], buf);

//	Retreive some settings stuff
	err =  RegOpenKeyEx(hKey, "Answer", 0, KEY_READ, &hSubKey);
	if (err !=ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}
	len = sizeof(buf);
	err = RegQueryValueEx(hSubKey, "1", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		RegCloseKey(hKey);
		return 0;
	}
	strcpy(obj->cmd[COMM_ANSWER_CMD], buf);
	RegCloseKey(hSubKey);

	err =  RegOpenKeyEx(hKey, "Hangup", 0, KEY_READ, &hSubKey);
	if (err !=ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}
	len = sizeof(buf);
	err = RegQueryValueEx(hSubKey, "1", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		RegCloseKey(hKey);
		return 0;
	}
	strcpy(obj->cmd[COMM_HANGUP_CMD], buf);
	RegCloseKey(hSubKey);

	err =  RegOpenKeyEx(hKey, "Settings", 0, KEY_READ, &hSubKey);
	if (err !=ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}
	len = sizeof(buf);
	err = RegQueryValueEx(hSubKey, "DialPrefix", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		RegCloseKey(hKey);
		return 0;
	}
	strcpy(obj->cmd[COMM_DIALPREF_CMD], buf);

	len = sizeof(buf);
	err = RegQueryValueEx(hSubKey, "Tone", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		RegCloseKey(hKey);
		return 0;
	}
	strcat(obj->cmd[COMM_DIALPREF_CMD], buf);

	len = sizeof(buf);
	err = RegQueryValueEx(hSubKey, "Prefix", 0, &type, buf, &len);
	if (err != ERROR_SUCCESS) {
		RegCloseKey(hSubKey);
		RegCloseKey(hKey);
		return 0;
	}
	strcpy(obj->cmd[COMM_PREFIX_CMD], buf);

	RegCloseKey(hSubKey);
	RegCloseKey(hKey);

	comm_dump_info(obj);

	return 1;
}	
 

//	int comm_open_connection(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int comm_open_connection(COMM_OBJ *obj)
{
	char filename[16];
	COMMTIMEOUTS ctimeouts;

	sprintf(filename, "COM%d", obj->port);
  											
	obj->handle = CreateFile(filename, 
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (obj->handle == INVALID_HANDLE_VALUE)	
		return 0;
	
//	 Modem COMport is open.
	SetCommMask(obj->handle, EV_RXCHAR);
	SetupComm(obj->handle, 4096, 4096);
	PurgeComm( obj->handle, 
					PURGE_TXABORT | 
					PURGE_RXABORT |
              	PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

//	Timeout after 10 sec.
	ctimeouts.ReadIntervalTimeout = 0xffffffff;
	ctimeouts.ReadTotalTimeoutMultiplier = 0;
	ctimeouts.ReadTotalTimeoutConstant = 10000;
	ctimeouts.WriteTotalTimeoutMultiplier = 0;
	ctimeouts.WriteTotalTimeoutConstant = 10000;
	SetCommTimeouts(obj->handle, &ctimeouts);

	memset(&obj->rov, 0, sizeof(OVERLAPPED));
	memset(&obj->wov, 0, sizeof(OVERLAPPED));

	obj->rov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (obj->rov.hEvent == NULL) {
		mprintf((0, "COMM: Unable to create read event.\n"));
		CloseHandle(obj->handle);
		return 0;
	}
	obj->wov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (obj->wov.hEvent == NULL) {
		mprintf((0, "COMM: Unable to create write event.\n"));
		CloseHandle(obj->rov.hEvent);
		CloseHandle(obj->handle); 
		return 0;
	}

//	Setup parameters for Connection
//	38400 8N1
	obj->dcb.DCBlength = sizeof(DCB);
	GetCommState(obj->handle, &obj->dcb);

	if (obj->baud) 
		obj->dcb.BaudRate = obj->baud;
	else obj->baud = obj->dcb.BaudRate;

	mprintf((0, "COM%d (%d) is opened.\n", obj->port,  obj->baud));

	obj->dcb.fBinary			= 1;
	obj->dcb.Parity 			= NOPARITY;
	obj->dcb.fNull 			= 0;
   obj->dcb.XonChar 			= ASCII_XON;
   obj->dcb.XoffChar 		= ASCII_XOFF;
   obj->dcb.XonLim 			= 1024;
   obj->dcb.XoffLim 			= 1024;
   obj->dcb.EofChar 			= 0;
   obj->dcb.EvtChar 			= 0;
	obj->dcb.fDtrControl		= DTR_CONTROL_ENABLE;// dtr=on
	obj->dcb.fRtsControl		= RTS_CONTROL_ENABLE;

	obj->dcb.ByteSize 		= 8;
	obj->dcb.StopBits 		= ONESTOPBIT;
	obj->dcb.fParity			= FALSE;

	obj->dcb.fOutxDsrFlow	= FALSE;
	obj->dcb.fOutxCtsFlow	= FALSE;					// rts/cts off

// obj->dcb.fInX = obj->dcb.fOutX = 1;				// Software flow control XON/XOFF
  	
	if (SetCommState(obj->handle, &obj->dcb) == TRUE) 
	{
		obj->hThread = NULL;
		obj->threadID = (DWORD)(-1);

	//	Send DTR
		EscapeCommFunction(obj->handle, SETDTR);
		obj->connect = TRUE;
	}
	else {
		mprintf((1, "COMM: Unable to set CommState: (%x)\n", GetLastError()));

		obj->hThread = NULL;
		obj->threadID = (DWORD)(-1);
		obj->connect = FALSE;
//		CloseHandle(obj->wov.hEvent);
//		CloseHandle(obj->rov.hEvent);
		CloseHandle(obj->handle);

		return 0;
	}

	return 1;
}


//	int comm_close_connection(COMM_OBJ *obj);
//	----------------------------------------------------------------------------	
int comm_close_connection(COMM_OBJ *obj)
{
	CloseHandle(obj->wov.hEvent);
	CloseHandle(obj->rov.hEvent);

	SetCommMask(obj->handle, 0);
	EscapeCommFunction(obj->handle, CLRDTR);
	PurgeComm( obj->handle, 
					PURGE_TXABORT | 
					PURGE_RXABORT |
              	PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
	CloseHandle(obj->handle);

	obj->connect = FALSE;
	obj->handle = NULL;
	mprintf((0, "COM%d closed.\n", obj->port));
	return 1;
}		


//	int comm_read_char(COMM_OBJ *obj);
//	----------------------------------------------------------------------------

#define ROTBUF_SIZE 16

static char rotbuf[ROTBUF_SIZE];
static int rotbuf_cur=0, rotbuf_tail=0;

int comm_read_char(COMM_OBJ *obj)
{
	DWORD errflags, result;
	COMSTAT comstat;
	DWORD length, length_to_read;

	if (!obj->connect) return COMM_BUF_EMPTY;

//	Do read buffer stuff
	if (rotbuf_cur < rotbuf_tail) {
		return (int)rotbuf[rotbuf_cur++];
	}

//	Now if we have a condition of cur = tail, then the buffer is empty
//	and we have to refill it.

	ClearCommError(obj->handle, &errflags, &comstat);

	if (comstat.cbInQue > 0) {
		if (comstat.cbInQue < ROTBUF_SIZE) 
			length_to_read = comstat.cbInQue;
		else
			length_to_read = ROTBUF_SIZE;

		result = ReadFile(obj->handle, rotbuf, length_to_read, &length, &obj->rov);

		if (result == 0) {
			if ((errflags = GetLastError()) != ERROR_IO_PENDING) {
				mprintf((1, "CommReadChar err: %X\n", errflags));
				ClearCommError(obj->handle, &errflags, &comstat);
			}
			else {
				while (!GetOverlappedResult(obj->handle, &obj->rov, &length, TRUE))
				{
					errflags = GetLastError();
					if (errflags == ERROR_IO_INCOMPLETE) {
						continue;
					}
					else {
						mprintf((1, "GetOverlappedResult error: %d.\n", errflags));
						rotbuf_cur = rotbuf_tail;
						return COMM_BUF_EMPTY;
					}
				}
			}
		}
		rotbuf_cur = 0;

		Assert(length <= ROTBUF_SIZE);
		rotbuf_tail = length;
		return (int)rotbuf[rotbuf_cur++];
	}
	else return COMM_BUF_EMPTY;
}


//	int comm_read_char_timed(COMM_OBJ *obj, int msecs);
//	----------------------------------------------------------------------------
int comm_read_char_timed(COMM_OBJ *obj, int msecs)
{
	DWORD err;
	COMSTAT cstat;
	int status;
	long timeout = timeGetTime() + (long)msecs;

	ClearCommError(obj->handle, &err, &cstat);
	while (cstat.cbInQue == 0)
	{
		status = comm_idle_function(obj);
		if (!status) return -1; 
		ClearCommError(obj->handle, &err, &cstat);
		if (timeout <= timeGetTime()) return -1;
	}
	return comm_read_char(obj);
}
	

//	int comm_write_char(COMM_OBJ *obj, int ch);
//	----------------------------------------------------------------------------
int comm_write_char(COMM_OBJ *obj, int ch)
{
	DWORD errflags;
	DWORD length;
	COMSTAT comstat;
	char c;

	if (!obj->connect) return 0;

	c = (char)ch;

//	mprintf((0, "%c", c));

	if (!WriteFile(obj->handle, &c, 1, &length, &obj->wov)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			while (!GetOverlappedResult(obj->handle, &obj->wov, &length, TRUE))
			{	
				errflags = GetLastError();
				if (errflags == ERROR_IO_INCOMPLETE)
					continue;
				else {
					ClearCommError(obj->handle, &errflags, &comstat);
				 	mprintf((1, "Comm: Write error!\n"));
					return 0;
				}
			}
		}
		else {
			ClearCommError(obj->handle, &errflags, &comstat);
			mprintf((1, "Comm: Write error!\n"));
			return 0;
		}
	}

//mprintf((0, "[%c]", c));

	return 1;
}
			

//	int comm_get_cd(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int comm_get_cd(COMM_OBJ *obj)
{
	DWORD status;

	if (!obj->connect) return 0;

	GetCommModemStatus(obj->handle, &status);
	if (status & MS_RLSD_ON) return 1;
	else return 0;
}


//	int comm_get_line_status(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int comm_get_line_status(COMM_OBJ *obj)
{
	COMSTAT comstat;
	DWORD err;
	int status=0;

	if (!obj->connect) return 0;

	ClearCommError(obj->handle, &err, &comstat);
	
	if (err & CE_BREAK) status |= COMM_STATUS_BREAK;
	if (err & CE_FRAME) status |= COMM_STATUS_FRAME;
	if (err & CE_OVERRUN) status |= COMM_STATUS_OVERRUN;
	if (err & CE_RXPARITY) status |= COMM_STATUS_RXPARITY;

	return status;
}


//	void comm_clear_line_status(COMM_OBJ *obj)
//	----------------------------------------------------------------------------
void comm_clear_line_status(COMM_OBJ *obj)
{
	
}


//	void comm_set_dtr(COMM_OBJ *obj, int flag);
//	----------------------------------------------------------------------------
void comm_set_dtr(COMM_OBJ *obj, int flag)
{
	if (!obj->connect) return;

	GetCommState(obj->handle, &obj->dcb);
	
	if (flag == 1) obj->dcb.fDtrControl = DTR_CONTROL_ENABLE;
	else obj->dcb.fDtrControl = DTR_CONTROL_DISABLE;
	
	SetCommState(obj->handle, &obj->dcb);
}
		

//	void comm_set_rtscts(COMM_OBJ *obj, int flag);
//	----------------------------------------------------------------------------
void comm_set_rtscts(COMM_OBJ *obj, int flag)
{
	
	if (!obj->connect) return;

	GetCommState(obj->handle, &obj->dcb);

	if (flag) {
		obj->dcb.fOutxCtsFlow	= TRUE;				// rts/cts off
		obj->dcb.fRtsControl	= RTS_CONTROL_HANDSHAKE;
	}
	else {
		obj->dcb.fOutxCtsFlow	= FALSE;				// rts/cts off
		obj->dcb.fRtsControl	= RTS_CONTROL_ENABLE;
	}

	SetCommState(obj->handle, &obj->dcb);
}


//	int comm_modem_input_line(COMM_OBJ *obj, int msecs, char *buffer, int chars);
//	----------------------------------------------------------------------------
int comm_modem_input_line(COMM_OBJ *obj, int msecs, char *buffer, int chars)
{		
	long timeout;
	int ch;
	int retval=1;
	int i;

	i = 0; 

	timeout = timeGetTime() + msecs;

	if (chars <= 0) return 0;

	while (1)
	{
		ch = comm_read_char(obj);
		if (ch >=0) { 
			if (ch == '\r') {
			//	mprintf((0, "\n"));
				break;
			}
			if (ch == '\n') continue;
			buffer[i++] = (char)ch;
			chars--;
			if (chars == 0) break;
		}
		else if (ch == COMM_BUF_EMPTY) {
			retval = 0;
			break;
		}
		else {
			retval = ch;
			break;
		}
		if (timeout <= timeGetTime()) break;
	}

	buffer[i] = '\0';
	if (retval < 0) return retval;

	retval = timeout - timeGetTime();
	if (retval < 0) retval = 0;
	return retval;
}


//	void comm_set_delay(COMM_OBJ *obj, int msecs)
//	----------------------------------------------------------------------------
void comm_set_delay(COMM_OBJ *obj, int msecs)
{
	comm_delay_val = (long)msecs;
}


//	void comm_wait(COMM_OBJ *obj, int msecs)
//	----------------------------------------------------------------------------
int comm_wait(COMM_OBJ *obj, int msecs)
{
	long delaytime;

	delaytime = timeGetTime() + (long)msecs;

	while (timeGetTime() < delaytime) {
		if (!comm_idle_function(obj)) {
			mprintf((1, "Comm_wait: THIS IS NOT GOOD!!\n")); 
			return 0;
		}
	}	
	return 1;
}


//	int comm_modem_send_string(COMM_OBJ *obj, char *str);
//	----------------------------------------------------------------------------
int comm_modem_send_string(COMM_OBJ *obj, char *str)
{
	int retval;
	long curtime;
	char buf[64];

	mprintf((0, "Commodem: send %s\n", str));
	retval = comm_modem_send_string_nowait(obj, str, -2);
	if (!retval) return 0;

	if (comm_delay_val > 0) {
		if (!comm_wait(obj, comm_delay_val)) return 0;
	}

//	Checking for OK response
	curtime = comm_delay_val;
	while(1) 
	{
		curtime = comm_modem_input_line(obj, curtime, buf, 64);
		if (curtime == 0) {
			mprintf((1, "CommSendString: Didn't get an OK!\n"));
			return 0;
		}
		if (strcmp("OK", buf) == 0) {
			return comm_wait(obj, 500);
		}
	}			

}


//	int comm_modem_send_string_nowait(COMM_OBJ *obj, char *str, int term); 
//	----------------------------------------------------------------------------
int comm_modem_send_string_nowait(COMM_OBJ *obj, char *str, int term)
{
	int i;

	if (term < -2 || term > 255) return 0;

//	mprintf((0, "<%s", str));
	for (i= 0; i < lstrlen(str); i++)
		comm_write_char(obj, str[i]);	

//@@	if (!WriteFile(obj->handle, &str, lstrlen(str), &length, &obj->wov)) {
//@@		if (GetLastError() == ERROR_IO_PENDING) {
//@@			mprintf((0,"Comm: SendStringNoWait IO pending...\n"));
//@@			while (!GetOverlappedResult(obj->handle, &obj->wov, &length, TRUE))
//@@			{	
//@@				errflags = GetLastError();
//@@				if (errflags == ERROR_IO_INCOMPLETE)
//@@					continue;
//@@				else {
//@@					ClearCommError(obj->handle, &errflags, &comstat);
//@@				 	mprintf((1, "Comm: SendStringNoWait error!\n"));
//@@					return 0;
//@@				}
//@@			}
//@@		}
//@@		else {
//@@			ClearCommError(obj->handle, &errflags, &comstat);
//@@			mprintf((1, "Comm: SendStringNoWait error!\n"));
//@@			return 0;
//@@		}
//@@	}

	if (term >=0) 
		comm_write_char(obj, term);
	else if (term == -2) {
		comm_write_char(obj, '\r');
		comm_write_char(obj, '\n');
	}

	return 1;
} 


//	int comm_modem_reset(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int comm_modem_reset(COMM_OBJ *obj)
{
	return comm_modem_send_string(obj, obj->cmd[COMM_RESET_CMD]);
}


//	int comm_modem_dial(COMM_OBJ *obj, char *phonenum);
//	----------------------------------------------------------------------------
int comm_modem_dial(COMM_OBJ *obj, char *phonenum)
{
	char str[40];
	
	strcpy(str, obj->cmd[COMM_PREFIX_CMD]);
	strcat(str, obj->cmd[COMM_DIALPREF_CMD]);
	strcat(str, phonenum);
	return comm_modem_send_string_nowait(obj, str, '\r');
}


//	int comm_modem_answer(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int comm_modem_answer(COMM_OBJ *obj)
{
	return comm_modem_send_string_nowait(obj, obj->cmd[COMM_ANSWER_CMD], '\r');
}



//@@	DWORD FAR PASCAL comm_thread_proc (LPSTR lpData);
//@@	-------------------------------------------------------------------------
//@@DWORD FAR PASCAL comm_thread_proc (LPSTR lpData)
//@@{
//@@	DWORD event_mask;
//@@	OVERLAPPED os;
//@@	COMM_OBJ *obj = (COMM_OBJ *)lpData;
//@@
//@@	memset(&os, 0, sizeof(OVERLAPPED));
//@@	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//@@	if (os.hEvent == NULL) {
//@@		mprintf((1, "CommThread: unable to create event!\n"));
//@@		return FALSE;
//@@	}
//@@
//@@	if (!SetCommMask(obj->handle, EV_RXCHAR)) return FALSE;
//@@
//@@	while (obj->connected) 
//@@	{
//@@		event_mask = 0;
//@@		WaitCommEvent(obj->handle, &event_mask, NULL);
//@@		if ((event_mask & EV_RXCHAR) == EV_RXCHAR) {
//@@		}
//@@	}
//@@}


//	int FAR PASCAL comm_idle_function(COMM_OBJ *obj);
//	----------------------------------------------------------------------------
int FAR PASCAL comm_idle_function(COMM_OBJ *obj)
{
	MSG msg;
	
	while (PeekMessage(&msg, 0,0,0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 1;
}


//	Dump Info
//	----------------------------------------------------------------------------
void comm_dump_info(COMM_OBJ *obj)
{
	int baud;

	switch (obj->dcb.BaudRate)
	{
		case CBR_9600:	baud = 9600; break;
		case CBR_14400: baud = 14400; break;
		case CBR_19200: baud = 19200; break;
		case CBR_38400: baud = 38400; break;
		case CBR_57600: baud = 57600; break;
		default:
			baud = -1;
	}
	mprintf((0, "CommObj %x:\n", obj->handle));
	mprintf((0, "\tConnect: %d\n\tDevice: %s\n", obj->connect, obj->name));
	mprintf((0,	"\tPort: COM%d\n", obj->port));
	mprintf((0, "\tBaud:	%d\n\tDTR:[%d]  RTS/CTS:[%d|%d]\n", baud, obj->dcb.fDtrControl,obj->dcb.fRtsControl,obj->dcb.fOutxCtsFlow));
	mprintf((0, "\tXON/XOFF [In|Out]:[%d|%d]  XON/OFF thresh:[%d|%d]\n", obj->dcb.fInX, obj->dcb.fOutX, obj->dcb.XonLim, obj->dcb.XoffLim));
}

	
	



	
