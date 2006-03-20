#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "fix.h"
#include "timer.h"
#include "mono.h"
#include "commlib.h"

#define F_1_1000 (F1_0/1000) // 1/1000

struct portparam { int base,irq; };
struct portparam portparams[4] = {{0x3f8, 4}, {0x2f8, 3}, {0x3e8, 4}, {0x2e8, 3}};

/* return Data Carrier Detect status */
int comm_get_dcd(comm_port *port) {
    return (inportb(port->MSR) & MSR_DCD) != 0;
}

void comm_printf(PORT *port, char *msg, ...) {
    char buf[1024];
    va_list vp;

    va_start(vp, msg);
    vsprintf(buf, msg, vp);
    va_end(vp);
mprintf((0, "comm_printf: writing \"%s\"\n", buf));
    WriteBuffer(port, buf, strlen(buf));
}

PORT * PortOpenGreenleafFast(int port, int baud, char parity, int databits, int stopbits)
{
    PORT *com;
    static int did_init = 0;

    if (!did_init) {
       dzcomm_init();
       did_init = 1;
    }

    if (!(com = comm_port_init(port))) {
       mprintf((1,"comm_port_init: %s\n", szDZCommErr));
       return NULL;
    }
    com->nBaud = baud;
    com->nPort = portparams[port].base;
    com->nIRQ = portparams[port].irq;
    switch (parity) {
       case 'N': com->nParity = NO_PARITY; break;
       case 'E': com->nParity = EVEN_PARITY; break;
       case 'O': com->nParity = ODD_PARITY; break;
    }
    switch (databits) {
       case 5: com->nData = BITS_5; break;
       case 6: com->nData = BITS_6; break;
       case 7: com->nData = BITS_7; break;
       case 8: com->nData = BITS_8; break;
    }
    switch (stopbits) {
       case 1: com->nStop = STOP_1; break;
       case 2: com->nStop = STOP_2; break;
    }
    com->control_type = NO_CONTROL;
    if (!comm_port_install_handler(com)) {
       mprintf((1,"comm_port_install: %s\n", szDZCommErr));
       comm_port_delete(com);
       return NULL;
    }
   return com;
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
    #if 0 /* disabled for now */
    if (state && port->control_type != RTS_CTS) {
       port->cts=CTS_ON;
       port->rts=RTS_ON;
       if (!queue_empty(port->OutBuf))
           outportb(port->MCR,inportb(port->MCR)|0x02); //setting RTS
       port->control_type = RTS_CTS;
    } else if (!state && port->control_type == RTS_CTS) {
       port->control_type = NO_CONTROL;
       if (!queue_empty(port->OutBuf))
           interrupt_on(port, THREINT);
    }
    #endif
    return;
}

void WriteChar(PORT *port, char ch)
{
    //mprintf((0, "(%d,%d)", port->OutBuf->head, port->OutBuf->tail));
    //mprintf((0, ">%02x ", (unsigned char)ch));
    comm_port_out(port, ch);
    return;
}

void ClearRXBuffer(PORT *port)
{
    queue_reset(port->InBuf);
}

int ReadBufferTimed(PORT *port, char *buf, int length, int timeout)
{
    fix stop = timer_get_fixed_seconds() + timeout * F_1_1000;
    int c;

    if (!length)
       return 1; // ok
    while (stop >= timer_get_fixed_seconds()) {
       if ((c = comm_port_test(port)) >= 0) {
           mprintf((0, "<%02x ", c));
           *(buf++) = c;
           if (!(--length))
               return 1; // ok
       }

    }
    return 0; // timeout
}

int Change8259Priority(int irq)
{
    return ASSUCCESS;
}

int FastSetPortHardware(int port, int IRQ, int baseaddr)
{
    portparams[port].irq = IRQ;
    portparams[port].base = baseaddr;
    return 0;
}

int FastGetPortHardware(int port, int *IRQ, int *baseaddr)
{
    *IRQ = portparams[port].irq;
    *baseaddr = portparams[port].base;
    return 0;
}

void FastSet16550TriggerLevel(int level)
{
    return;
}
void FastSet16550UseTXFifos(int on)
{
    return;
}

void FastSavePortParameters(int port)
{
}

int PortClose(PORT *port)
{
    comm_port_delete(port);
    return 0;
}

void FastRestorePortParameters(int port)
{
}

int GetCd(PORT *port)
{
    return comm_get_dcd(port);
}

int ReadCharTimed(PORT *port, int timeout)
{
    fix stop = timer_get_fixed_seconds() + timeout * F_1_1000;
    int c = -1;

    while (stop >= timer_get_fixed_seconds() && (c = comm_port_test(port)) < 0)
       ;
    if (c >= 0)
       mprintf((0, "<%02x ", c));
    return c;
}

int ReadChar(PORT *port)
{
    int c = comm_port_test(port);
    //if (c >= 0)
    //   mprintf((0, "<%02x ", c));
    return c;
}

void ClearLineStatus(PORT *port)
{
}

void WriteBuffer(PORT *port, char *pbuff, int len)
{
    mprintf((0, ">x%d ", len));
    while (len--)
       comm_port_out(port, *(pbuff++));
}


int HMInputLine(PORT *port, int timeout, char *buf, int bufsize)
{
    fix stop = timer_get_fixed_seconds() + timeout * F_1_1000;
    int c;
    char *p = buf;
    int bufleft = bufsize;

mprintf((1, "port->InBuf->head=%d, port->InBuf->tail=%d\n", port->InBuf->head,
  port->InBuf->tail));
    while (stop >= timer_get_fixed_seconds()) {
       if ((c = comm_port_test(port)) >= 0) {
mprintf((0,"got char %d (%c)", c, c >= 32 ? c : '.'));
           if (c == '\n' && p > buf) {
                *p = 0;
                mprintf((0,"got line \"%s\"", buf));
               if (p - buf >= 2 && !strncmp(buf, "AT", 2)) { /* assume AT... response is echo */
                   p = buf;
                   bufleft = bufsize;
               } else {
                   *p = 0;
                   return 1; // ok
               }
           }
           if (c != '\r' && c != '\n') {
               if (!bufleft)
                   return -1; // buffer overflow
               *(p++) = c;
               bufleft--;
           }
       }
    }
mprintf((0,"hminputline: timeout\n"));
    return 0; // timeout
}

void HMWaitForOK(int timeout, char *buf)
{
    /* what is this supposed to do without a port handle? */
}

void HMSendStringNoWait(PORT *port, char *pbuf, int a)
{
    comm_printf(port, "%s\r", pbuf);
}

int HMSendString(PORT *port, char *msg)
{
//    char buf[80];

    HMSendStringNoWait(port, msg, -2);
//    HMInputLine(port, 5000, buf, sizeof(buf) - 1); /* 5 sec timeout */
//    if (!strncmp(buf, "OK", 2))
//       return 0; // Ok
//    return -1; // Error
    if(HMInputLine(port, 5000, msg, sizeof(msg) - 1) >= 0)
     return 0;
    return -1;
}
//added on 11/5/98 by Victor Rachels to wait less
int HMSendStringWait(PORT *port, char *msg, int wait)
{
  HMSendStringNoWait(port, msg, -2);
   if(HMInputLine(port, wait, msg, sizeof(msg) - 1) >= 0)
    return 0;
  return -1;
}
//end this section addition - VR

void HMReset(PORT *port)
{ 
}

void HMDial(PORT *port, char *pPhoneNum)
{
    comm_printf(port, "ATD%s\r", pPhoneNum);
}

void HMAnswer(PORT *port)
{
    comm_printf(port, "ATA\r");
}

void ClearTXBuffer(PORT *port)
{
    queue_reset(port->OutBuf);
}

int GetLineStatus(PORT *port)
{
    return 0;
}
