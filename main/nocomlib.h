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
//This include is just to allow compiling. It doesn't mean it will work. Values in here are only dummy values


//I just put in whatever felt good, consider this file nonesense.
typedef struct  {
	int status;
	int count;
	
} PORT;

#define COM1	0
#define COM2	1
#define COM3	2
#define COM4	3

#define IRQ2	2
#define IRQ3	3
#define IRQ4	4
#define IRQ7	7
#define IRQ15	15

#define ASSUCCESS	1
#define ASBUFREMPTY	0	
#define TRIGGER_04	4

#define ON	1
#define OFF 0

/*
PORT *PortOpenGreenleafFast(int port, int bps, char parity, int data, int stop);
*/
#define PortOpenGreenleafFast(port, bps, parity, data, stop) NULL
#define SetDtr(port, value) ((void)0)
#define SetRts(port, value) ((void)0)
#define UseRtsCts(port, value) ((void)0)
#define WriteChar(port, c) ((void)0)
#define ClearRXBuffer(port) ((void)0)
#define ReadBufferTimed(port, buf, size, timeout) ((void)0)
#define Change8259Priority(irq) 0
#define FastSetPortHardware(port,irq,base) 0
#define FastSet16550TriggerLevel(a) ((void)0)
#define FastSet16550UseTXFifos(a) ((void)0)
#define FastSavePortParameters(port) ((void)0)
#define FastGetPortHardware(port,irq,base) ((void)0)
#define PortClose(port) 0
#define FastRestorePortParameters(num)
#define GetCd(port) 0
#define ReadCharTimed(port, timeout) 0
#define ReadChar(port) 0
#define ClearLineStatus(port) ((void)0)
#define HMInputLine(port, a, buf, b) 0
#define HMWaitForOK(a, b) ((void)0)
#define HMSendString(port, msg) ({do ; while(0); 0;})
#define HMReset(port) ((void)0)
#define HMDial(port, pPhoneNum) ((void)0)
#define HMSendStringNoWait(port, pbuf,a) ({do ; while(0); 0;})
#define HMAnswer(port) ((void)0)
#define ClearTXBuffer(port) ((void)0)
#define WriteBuffer(port, pbuff, len) ((void)0)
#define GetLineStatus(port) 0
#define ClearLineStatus(port) ((void)0)
