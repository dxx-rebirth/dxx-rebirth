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



#ifndef _COMM_H
#define _COMM_H


#define COMM_BUF_EMPTY			-1

#define COMM_STATUS_BREAK		1
#define COMM_STATUS_FRAME		2
#define COMM_STATUS_OVERRUN	4
#define COMM_STATUS_RXPARITY	8


#define COMM_DEVICE_MODEM1		1	// OBSOLETE!! use XTAPI
#define COMM_DEVICE_MODEM0		0	// OBSOLETE!! use XTAPI

#define COMM_DEVICE_COM1		-1
#define COMM_DEVICE_COM2		-2
#define COMM_DEVICE_COM3		-3
#define COMM_DEVICE_COM4		-4

typedef struct COMM_OBJ	{
	HANDLE handle;
	BOOL connect;
	HANDLE hThread;
	DWORD threadID;
	OVERLAPPED rov, wov;
	DCB dcb;
	unsigned count;
	char name[32];	 					// OBSOLETE!! use XTAPI
	char port;
	int type;
	int baud;
	char cmd[8][32];					// OBSOLETE!! use XTAPI
} COMM_OBJ;


int comm_open_connection(COMM_OBJ *obj);
int comm_close_connection(COMM_OBJ *obj);
int comm_read_char(COMM_OBJ *obj);
int comm_read_char_timed(COMM_OBJ *obj, int msecs);
int comm_write_char(COMM_OBJ *obj, int ch);

int comm_get_cd(COMM_OBJ *obj);
int comm_get_line_status(COMM_OBJ *obj);

void comm_set_dtr(COMM_OBJ *obj, int flag);
void comm_set_rtscts(COMM_OBJ *obj, int flag);
void comm_clear_line_status(COMM_OBJ *obj);

void comm_set_delay(COMM_OBJ *obj, int msecs);
int comm_wait(COMM_OBJ *obj, int msecs);


/* OBSOLETE.  Use XTAPI calls to do the same job much better */

#define COMM_PREFIX_CMD			0
#define COMM_RESET_CMD			1
#define COMM_DIALPREF_CMD		2
#define COMM_ANSWER_CMD			3
#define COMM_HANGUP_CMD			4


int comm_get_modem_info(int modem, COMM_OBJ *obj);
int comm_modem_input_line(COMM_OBJ *obj, int msecs, char *buffer, int chars);
int comm_modem_send_string(COMM_OBJ *obj, char *str);
int comm_modem_send_string_nowait(COMM_OBJ *obj, char *str, int term); 
int comm_modem_reset(COMM_OBJ *obj);
int comm_modem_dial(COMM_OBJ *obj, char *phonenum);
int comm_modem_answer(COMM_OBJ *obj);



void comm_dump_info(COMM_OBJ *obj);


#endif
