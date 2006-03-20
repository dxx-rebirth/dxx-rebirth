#ifndef _COMLIB_H
#define _COMLIB_H
#include "dzcomm.h"
typedef comm_port PORT;

#define COM1   0
#define COM2   1
#define COM3   2
#define COM4   3

#define IRQ2   2
#define IRQ3   3
#define IRQ4   4
#define IRQ7   7
#define IRQ15  15

#define ASSUCCESS      1
#define ASBUFREMPTY    -1
#define TRIGGER_04     4

#define ON     1
#define OFF    0


PORT * PortOpenGreenleafFast(int port, int baud,char parity,int databits, int stopbits);
void SetDtr(PORT *port,int state);
void SetRts(PORT *port,int state);
void UseRtsCts(PORT *port,int state);
void WriteChar(PORT *port,char ch);
void ClearRXBuffer(PORT *port);
int ReadBufferTimed(PORT *port, char *buf, int length, int timeout);
int Change8259Priority(int a);
int FastSetPortHardware(int comport,int IRQ, int baseaddr);
int FastGetPortHardware(int comport,int *IRQ, int *baseaddr);
void FastSet16550TriggerLevel(int a);
void FastSet16550UseTXFifos(int a);
void FastSavePortParameters(int comport);
int PortClose(PORT *port);
void FastRestorePortParameters(int num);
int GetCd(PORT *port);
int ReadCharTimed(PORT *port, int blah);
int ReadChar(PORT *port);
void ClearLineStatus(PORT *port);
int HMInputLine(PORT *port, int a, char *buf, int b);
void HMWaitForOK(int a, char *b);
int HMSendString(PORT *port, char *msg);
int HMSendStringWait(PORT *port, char *msg, int wait);
void HMReset(PORT *port);
void HMDial(PORT *port, char *pPhoneNum);
void HMSendStringNoWait(PORT *port, char *pbuf,int a);
void HMAnswer(PORT *port);
void ClearTXBuffer(PORT *port);
void WriteBuffer(PORT *port, char *pbuff, int len);

int GetLineStatus(PORT *port);
void ClearLineStatus(PORT *port);

#endif
