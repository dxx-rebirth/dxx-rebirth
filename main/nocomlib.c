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

//These are do nothing functions to allow the code to compile without the comm functions -KRB

#include "nocomlib.h"
#if 0
PORT * PortOpenGreenleafFast(int port, int baud,char parity,int databits, int stopbits)
{
	return 0;

}

void SetDtr(PORT *port,int state)
{
	return;
}

void SetRts(PORT *port,int state)
{
	return;
}

void UseRtsCts(PORT *port,int state)
{
	return;
}

void WriteChar(PORT *port,char ch)
{
	return;
}

void ClearRXBuffer(PORT *port)
{
	return;
}

void ReadBufferTimed(PORT *port, char *buf,int a, int b)
{
	return;
}

int Change8259Priority(int a)
{
	return 0;
}

int FastSetPortHardware(PORT *port,int IRQ, int baseaddr)
{
	return 0;
}
void FastSet16550TriggerLevel(int a)
{
	return;
}
void FastSet16550UseTXFifos(int a)
{
	return;
}


FastSavePortParameters(PORT *port)
{
	return;
}
int PortClose(PORT *port)
{
	return 0;
}
void FastRestorePortParameters(int num)
{
	return;
}
int GetCd(PORT *port)
{
	return 0;
}
int ReadCharTimed(PORT *port, int blah)
{
	return 0;
}
int ReadChar(PORT *port)
{
	return 0;
}

void ClearLineStatus(PORT *port)
{
	return;
}
int HMInputLine(PORT *port, int a, char *buf, int b)
{
	return 0;
}
void HMWaitForOK(int a, int b)
{
	return;
}
HMSendString(PORT *port, char *msg)
{
	return;
}
void HMReset(PORT *port)
{	
	return;
}
void HMDial(PORT *port, char *pPhoneNum)
{
	return;
}
void HMSendStringNoWait(PORT *port, char *pbuf,int a)
{
	return;
}
void HMAnswer(PORT *port)
{
	return;
}
void ClearTXBuffer(PORT *port)
{
	return;
}
void WriteBuffer(PORT *port, char *pbuff, int len)
{
	return;
}

#endif


