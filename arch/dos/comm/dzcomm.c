/*
 * DZcomm : serial communication add-on for Allegro.
 * version : 0.6
 * Copyright (c) 1997 Dim Zegebart, Moscow Russia.
 * zager@post.comstar.ru
 * file : dzcomm.c
 *
 */

#include <stdio.h>
#include <sys/movedata.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#include "dzcomm.h"
#include "allegro.h"

typedef unsigned int uint;
typedef unsigned short int usint;
typedef unsigned char byte;

#define com_(N) comm_port *com##N

com_(1);
com_(2);
com_(3);
com_(4);
com_(5);
com_(6);
com_(7);
com_(8);

#define com_wrapper_(N) static int com##N##_wrapper(void) \
                       { DISABLE(); /* adb -- why this? */ \
                          dz_comm_port_interrupt_handler(com##N##); \
                          return(0); \
                        } \
                        END_OF_FUNCTION(com##N##_wrapper);

com_wrapper_(1);
com_wrapper_(2);
com_wrapper_(3);
com_wrapper_(4);
com_wrapper_(5);
com_wrapper_(6);
com_wrapper_(7);
com_wrapper_(8);

static comm_port *irq_bot_com_port[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int (*com_wrapper[8])();
static comm_port **com_port[8];

char szDZCommErr[50];
inline void dz_make_comm_err(char *szErr) {strcpy(szDZCommErr,szErr);}
//comm port functions
static inline void comm_port_rdr(comm_port *port);
static inline void comm_port_trx(comm_port *port);
static inline void interrupt_on(comm_port *port,byte _interrupt_);
static inline void interrupt_off(comm_port *port,byte _interrupt_);
extern void lock_queue_functions(void);
void comm_port_fifo_init(comm_port *port);
inline void comm_port_rtmout(comm_port *port);
void comm_port_uninstall(comm_port *port);

//const byte THREOFF=0xfd;
const byte THREINT=0x02;
const byte RDAINT=0x04;

//-------------------------- PRINT BIN --------------------------

void print_bin_s(int c,char *s)
{ int i,j;
  int mask=0x01;
  sprintf (s,"%c : ",c);
  for (i=0;i<4;i++)
   { for(j=0;j<8;j++)
      if (c&(mask<<(j+i*8))) strcat(s,"1");
      else strcat(s,"0");
      strcat(s,".");
   }
}

//------------------------- INTERRUPT ON -----------------------------

static inline void interrupt_on(comm_port *port,byte _interrupt_)
{ byte i;
  i=inportb(port->IER);
  if (!(i&_interrupt_)) outportb(port->IER,i|_interrupt_);
}
END_OF_FUNCTION(interrupt_on);

//------------------------- INTERRUPT OFF -----------------------------

static inline void interrupt_off(comm_port *port,byte _interrupt_)
{ byte i;
  i=inportb(port->IER);
  if (i&_interrupt_) outportb(port->IER,i&~_interrupt_);
}
END_OF_FUNCTION(interrupt_off);

//------------------------- COMM PORT INIT --------------------------

comm_port *comm_port_init(comm com)
{ comm_port *port=(comm_port*)malloc(sizeof(comm_port));

  szDZCommErr[0]=0;

  if (port==NULL)
   { dz_make_comm_err("Out of memory !");
     return(NULL);
   }

  if ((port->InBuf=queue_new_(4096,1))==NULL)
   { dz_make_comm_err("Out of memory !");
     return(NULL);
   }
  if ((port->OutBuf=queue_new_(4096,1))==NULL)
   { dz_make_comm_err("Out of memory !");
     queue_delete(port->InBuf);
     return(NULL);
   }

  port->nPort=0;
  //getting comport base address
  //0x400+com*2 - memory address where stored default comm base address
  dosmemget(0x400+com*2,2,&port->nPort);
  if (port->nPort==0) //probably you have notebook or some brand named PC
   { if (com==_com1||com==_com3||com==_com5||com==_com7) port->nPort=0x3f8; //default I/O address for com1 or com3
     else port->nPort=0x2f8; //com2 or com4
   }

//set default values (user may change it later)

  if (com==_com1||com==_com3||com==_com5||com==_com7) port->nIRQ=4;
  else port->nIRQ=3; //com2 or com4

  strcpy(port->szName,"COM ");
  port->szName[3]=com+49;
  port->installed=NO;
  port->nComm=com;
  port->nBaud=_2400;
  port->nData=BITS_8;
  port->nStop=STOP_1;
  port->nParity=NO_PARITY;
  port->comm_handler=com_wrapper[com];
  port->control_type=XON_XOFF;
  port->msr_handler=NULL; //#d#pointer to modem status register handler.
  port->lsr_handler=NULL; //#d#pointer to line status register handler.

  _go32_dpmi_lock_data(port,sizeof(comm_port));
  *(com_port[com])=port;

  return(port);
}

//-------------------- COMM_PORT_FIFO_INIT ----------------------

void comm_port_fifo_init(comm_port *port)
{
//setting up FIFO if possible
  int c=inportb(port->SCR);
  outportb(port->SCR,0x55);
  port->fifo = 0;
  if(inportb(port->SCR)==0x55)  // greater then 8250
   { outportb(port->SCR,c);
     outportb(port->FCR,0x87);//try to enable FIFO with 8 byte trigger level
     if ((c=inportb(port->IIR)&0xc0)!=0xc0) //If not 16550A
       outportb(port->FCR,c&0xfe); //Disable FIFO
       else port->fifo = 1;
//     else printf("\nFIFO OK\n");
   }

}

//----------------------- COMM PORT INSTALL HANDLER ------------------
//modifyed by Neil Townsend to support IRQ sharing
int comm_port_install_handler(comm_port *port)
{
  if (port==NULL)
   { dz_make_comm_err("Set up comm parametrs first ! Call comm_port_new() first.");
     return(0);
   }
  if (port->installed==YES)
   { dz_make_comm_err("Comm already installed !");
     return(0);
   }
  if (port->nIRQ>15)
   { dz_make_comm_err("IRQ number out of range ! Must be 0 ... 15 .");
     return(0);
   }
  if (port->comm_handler==NULL)
   { dz_make_comm_err("Specify comm's handler (see 'comm_handler' member's description) !");
     return(0);
   }
  if (port->nBaud==0)
   { dz_make_comm_err("Invalid baud rate !");
     return(0);
   }

  switch(port->control_type)
   { case XON_XOFF :
       port->xon=0;
       port->xoff=0;
       port->xonxoff_rcvd=XON_RCVD;
       port->xonxoff_send=XON_SENT;
       break;
     case RTS_CTS :
       port->cts=CTS_ON;
       port->rts=RTS_ON; //fixed by SET (was RTS_OFF)
       break;
     default :
       break;
   }

  port->THR=port->nPort;
  port->RDR=port->nPort;
  port->BRDL=port->nPort;
  port->BRDH=1+port->nPort;
  port->IER=1+port->nPort;
  port->IIR=2+port->nPort;
  port->FCR=2+port->nPort;
  port->LCR=3+port->nPort;
  port->MCR=4+port->nPort;
  port->LSR=5+port->nPort;
  port->MSR=6+port->nPort;
  port->SCR=7+port->nPort;
  { byte i;
    for(i=0;i<8;i++) outportb(port->RDR+i,0);
  }

  comm_port_fifo_init(port);

  { unsigned char temp;
    do
     { temp=inportb(port->RDR);
       temp=inportb(port->LSR);
       temp=inportb(port->MSR);
       temp=inportb(port->IIR);
     }
    while(!(temp & 1));
  }


//set communication parametrs
//MOVED forwards so done before interupts enabled
  { int divisor=115200/port->nBaud;
    byte divlow,divhigh;
    byte comm_param=port->nData|(port->nStop<<2)|(port->nParity<<3);
    // Set Port Toggle to BRDL/BRDH registers
    outportb(port->LCR,comm_param|0x80);
    divlow=divisor&0x000000ff;
    divhigh=(divisor>>8)&0x000000ff;
    outportb(port->BRDL,divlow); // Set Baud Rate
    outportb(port->BRDH,divhigh);
    outportb(port->LCR,comm_param&0x7F); // Set LCR and Port Toggle
  }
  
//set interrupt parametrs
  { const byte IMR_8259_0=0x21;
    const byte IMR_8259_1=0xa1;
    const byte ISR_8259_0=0x20;
    const byte ISR_8259_1=0xa0;
    const byte IERALL = 0x0f; //mask for all communications interrupts
    const byte MCRALL = 0x0f; //DTR,RTS,OUT1=OUT2=1 is ON
    //calculating mask for 8259 controller's IMR
    //and number of interrupt hadler for given irq level
    if (port->nIRQ<=7) //if 0<=irq<=7 first IMR address used
     { port->interrupt_enable_mask=~(0x01<<port->nIRQ);
       port->nIRQVector=port->nIRQ+8;
       port->IMR_8259=IMR_8259_0;
       port->ISR_8259=ISR_8259_0;
     }
    else
     { port->interrupt_enable_mask=~(0x01<<(port->nIRQ%8));
       port->nIRQVector=0x70+(port->nIRQ-8);
       port->IMR_8259=IMR_8259_1;
       port->ISR_8259=ISR_8259_1;
     }

    port->next_port = NULL;
    port->last_port = irq_bot_com_port[port->nIRQ];
    if (irq_bot_com_port[port->nIRQ] != NULL) irq_bot_com_port[port->nIRQ]->next_port = port;
    irq_bot_com_port[port->nIRQ] = port;
    DISABLE();
    if (port->last_port == NULL)
     { // DZ orig lines in here
       _install_irq(port->nIRQVector,port->comm_handler);
       //enable interrupt port->nIRQ level
       outportb(port->IMR_8259,inportb(port->IMR_8259)&
                port->interrupt_enable_mask);
     }
       
    // DZ again:
    outportb(port->MCR,MCRALL); //setup modem
    outportb(port->IER,IERALL); //enable all communication's interrupts
    // to here
    ENABLE();
  }

  port->installed=YES;
  return(1);
}

//------------------ DZ COMM PORT INTERRUPT HANDLER ---------------
// always return zero
inline int dz_comm_port_interrupt_handler(comm_port *port)
{ int int_id;
  int c;
//  putch('1');

  while (1) //loop while !end of all interrupts
   { int_id=inportb(port->IIR); //get interrupt id
        //next loop added by Neil Townsend
     while ((int_id&0x01)==1) //no interrupts (left) to handle on this port
      { if (port->next_port==NULL)
         { outportb(0x20,0x20);
           if (port->nIRQ>7) //Thanks for SET for this fix
             outportb(0xA0,0x20); //now IRQ8-IRQ15 handled properly.

           return(0);
         }
        port=(comm_port*)port->next_port;
        int_id=inportb(port->IIR);
      }

      switch (int_id)
        {
          case 0xcc: //Character Timeout Indication N.Lohmann 6.3.98
                     // only possible if FiFo's are enabled.
            comm_port_rtmout(port);
            break;
          case 0xc6: //FiFo LSINT
          case 0x06: //LSINT:
            c=inportb(port->LSR);
            if (port->lsr_handler!=NULL) port->lsr_handler(c);
            break;
          case 0xc4: //FiFo RDAINT
          case 0x04: //RDAINT
            comm_port_rdr(port);
            break;
          case 0xc2: //FiFo THREINT
          case 0x02: //THREINT :
            comm_port_trx(port);
            break;
          case 0xc0: //FiFo MSINT
          case 0x00: //MSINT :
            c=inportb(port->MSR);
            if (port->msr_handler!=NULL) port->msr_handler(c);

            if (port->control_type==RTS_CTS)
             { if (port->cts==CTS_OFF && c&0x10)
                { port->cts=CTS_ON;
                  if (port->rts==RTS_ON)
                    interrupt_on(port,THREINT);
                }
               else
                 if (port->cts==CTS_ON && !c&0x10)
                  { port->cts=CTS_OFF;
                    interrupt_off(port,THREINT);
                  }
              }
            break;
         }//end case
   } //end while 1

  return(0);
}
END_OF_FUNCTION(dz_comm_port_interrupt_handler);

//----------------------- COMM PORT RTMOUT  ------------------------
//Nils Lohmann 8.3.98
//Time Out Indication, service routine  triggered after 4 Char-times
//if there is at least one Byte in FiFo and no read-Cykle happened.

inline void comm_port_rtmout(comm_port *port)
{
  comm_port_rdr(port);
}
END_OF_FUNCTION(comm_port_rtmout);

//----------------------- COMM PORT DELETE ------------------------

void comm_port_delete(comm_port *port)
{
  if (port->installed==NO) return;
 
  comm_port_uninstall(port);

  free(port);
}

//------------------------- COMM PORT OUT -----------------------------

inline void comm_port_out(comm_port *port,byte c)
{

  queue_put_(port->OutBuf,&c);
  switch (port->control_type)
   { case NO_CONTROL :
      interrupt_on(port,THREINT);
      break;
     case XON_XOFF : // XON/XOFF
      if (port->xonxoff_rcvd!=XOFF_RCVD)
        { interrupt_on(port,THREINT);
        }
     case RTS_CTS : // RTS_CTR
      if (port->cts==CTS_ON) //modem is 'clear to send'
       { if (port->rts==RTS_ON) //Fixed by SET (was RTS_OFF)
          { interrupt_on(port,THREINT);
          }
      }
      break;
     default:
      break;
   }
}

//------------------------- COMM PORT TEST -----------------------------

inline int comm_port_test(comm_port *port)
{ byte c;

  switch (port->control_type)
   { case NO_CONTROL :
      break;
     case XON_XOFF : // XON/XOFF
      if (((port->InBuf)->tail<(port->InBuf)->fill_level)&&(port->xonxoff_send!=XON_SENT))
        { port->xon=1;
          interrupt_on(port,THREINT);
        }
      break;
     case RTS_CTS :
      if (((port->InBuf)->tail<(port->InBuf)->fill_level)&&(port->rts==RTS_OFF))
        { outportb(port->MCR,inportb(port->MCR)|0x02); //setting RTS
          port->rts=RTS_ON;
          interrupt_on(port,THREINT);
       }
      break;
     default:
      break;
   }
  if (queue_empty(port->InBuf))
   { return(-1);
   }
  queue_get_(port->InBuf, &c);
  return(c);
}

//----------------------- COMM PORT STRING SEND -------------------------

void comm_port_string_send(comm_port *port,char *s)
{
  int i;

  if (s==NULL) return;
  for (i=0;s[i]!=0;i++) comm_port_out(port,s[i]);
}

//---------------------- COMM PORT COMMAND SEND -------------------------

void comm_port_command_send(comm_port *port,char *s)
{
  int i;

  if (s==NULL) return;
  for (i=0;s[i]!=0;i++)comm_port_out(port,s[i]);
  comm_port_out(port,'\r');
}

//------------------- MODEM HANG UP -----------------

void modem_hangup(comm_port *port)
{
  comm_port_out(port,2);
  delay(3000);
  comm_port_string_send(port,"+++");
  delay(3000);
  comm_port_string_send(port,"ATH0\r");
}

//---------------------- COMM PORT HAND ---------------------------

void comm_port_hand(comm_port *port,int m)
{ int c;
  c=inportb(port->MCR);
  outportb(port->MCR,c&m);
}

inline int comm_port_send_xoff(comm_port *port)
{
  if (port->xonxoff_send!=XOFF_SENT)
   { port->xoff=1;
     interrupt_on(port,THREINT);
   }
  return(1);
}

inline int comm_port_send_xon(comm_port *port)
{
  if (port->xonxoff_send!=XON_SENT)
   { port->xon=1;
     interrupt_on(port,THREINT);
   }
  return(1);
}

//-------------------- COMM PORT LOAD SETTINGS ---------------------

#ifdef __DEVICE__
int comm_port_load_settings(device *dev,char *ini_name)
#else
int comm_port_load_settings(comm_port *port,char *ini_name)
#endif
{
  #ifdef __DEVICE__
  comm_port *port=dev->device_user_data;
  #endif
  FILE *ini;
  int l,i;
  char *buf,*buf_end,*s,v[64];
  comm com=port->nComm;
  char com_name[7]={'[','c','o','m',com+49,']',0};
  char com_end[11]={'[','c','o','m',com+49,' ','e','n','d',']',0};

  if ((ini=fopen(ini_name,"rt"))==NULL) return(0);
  l=filelength(fileno(ini));
  if ((buf=(char*)alloca(l))==NULL)
    { fclose(ini);
      return(0);
    }

  l=fread(buf,1,l,ini);
  fclose(ini);
  buf[l]=0;
  strlwr(buf);

  s=strstr(buf,com_name);
  buf_end=strstr(buf,com_end);

  if (s==NULL||buf_end==NULL) return(-1);
  *buf_end=0;

  //get baud rate
  if ((s=strstr(buf,"baud"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
      { s++;i=0;
        while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
         if (!isspace(*s)) v[i++]=*s++;

        v[i]=0;
        if ((i=atoi(v))!=0) port->nBaud=i;
      }
   }

  //get irq number
  if ((s=strstr(buf,"irq"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
      { s++;i=0;
        while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
         if (!isspace(*s)) v[i++]=*s++;

        v[i]=0;
        if ((i=atoi(v))!=0) port->nIRQ=i;
      }
   }

  //get comm address
  if ((s=strstr(buf,"address"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
      { s++;i=0;
        while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
         if (!isspace(*s)) v[i++]=*s++;

        v[i]=0;
        if ((i=strtol(v,NULL,0))!=0) port->nPort=strtol(v,NULL,0);
      }
   }

  //get data bits
  if ((s=strstr(buf,"data"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
       { s++;i=0;
         while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
          if (!isspace(*s)) v[i++]=*s++;

         v[i]=0;
         if ((i=atoi(v))!=0)
          switch (i)
           { case 8 :
              port->nData=BITS_8;
              break;
             case 7 :
              port->nData=BITS_7;
              break;
             case 6 :
              port->nData=BITS_6;
              break;
             case 5 :
              port->nData=BITS_5;
              break;
           }
       }
   }

  //get parity bits
  if ((s=strstr(buf,"parity"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
       { s++;i=0;
         while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
          if (!isspace(*s)) v[i++]=*s++;
         v[i]=0;

         if (strcmp(v,"even")==0) port->nParity=EVEN_PARITY;
         else
        if (strcmp(v,"odd")==0) port->nParity=ODD_PARITY;
         else
         if (strcmp(v,"no")==0||strcmp(v,"none")==0) port->nParity=NO_PARITY;
       }
   }

  //get control type
  if ((s=strstr(buf,"control"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
       { s++;i=0;
         while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
          if (!isspace(*s)) v[i++]=*s++;
         v[i]=0;

         if (strcmp(v,"xon_xoff")==0||strcmp(v,"xon/xoff")==0) port->control_type=XON_XOFF;
         else
          if (strcmp(v,"no")==0||strcmp(v,"none")==0) port->control_type=NO_CONTROL;
          else
          if (strcmp(v,"rts_cts")==0||strcmp(v,"rts/cts")==0) port->control_type=RTS_CTS;
      }
   }

  if ((s=strstr(buf,"stop"))!=NULL) //get stop bits
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
       { s++;i=0;
         while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<9))
          if (!isspace(*s)) v[i++]=*s++;

         v[i]=0;
         if ((i=atoi(v))!=0)
          switch (i)
           { case 1 :
              port->nStop=STOP_1;
              break;
             case 2 :
              port->nStop=STOP_2;
              break;
           }
       }
   }

  if ((s=strstr(buf,"name"))!=NULL)
   { if (not_commented(s)&&(s=strchr(s,'='))!=NULL)
      { s++;i=0;
        while ((*s!=';')&&(*s!='\n')&&(*s!=0)&&(i<64))
         v[i++]=*s++;

        v[i]=0;
        strcpy(port->szName,v);
      }
   }

  return(1);
}


//----------------------- COMM PORT RDR ------------------------

static inline void comm_port_rdr(comm_port *port)
{ byte c;
  int n;

  do {
    c = inportb(port->RDR);
    switch (port->control_type)
     { case NO_CONTROL :
       queue_put_(port->InBuf,&c);
       port->in_cnt++;
       break;
       case XON_XOFF : // XON/XOFF
       if (c==XON_ASCII)
        { port->xonxoff_rcvd=XON_RCVD;
          interrupt_on(port,THREINT);
          return;
        }
       if (c==XOFF_ASCII)
        { port->xonxoff_rcvd=XOFF_RCVD;
          interrupt_off(port,THREINT);
          return;
        }
       n=queue_put_(port->InBuf,&c);
       port->in_cnt++;
       if (n) //buffer is near full
        { if (port->xonxoff_send!=XOFF_SENT)
           { port->xoff=1;
             interrupt_on(port,THREINT);
           }
        }
       break;
       case RTS_CTS:
      n=queue_put_(port->InBuf,&c);
       port->in_cnt++;
       if (n) //buffer is near full
        { outportb(port->MCR,inportb(port->MCR)&(~0x02)); //dropping RTS
          port->rts=RTS_OFF;
        }
       break;
       default :
       break;
     }
     c =inportb(port->LSR);
  } while (c & 1);
  // these lines are a cure for the well-known problem of TX interrupt
  // lock-ups when receiving and transmitting at the same time
  if (c & 0x40)
    comm_port_trx(port);
}
static END_OF_FUNCTION(comm_port_rdr)

//----------------------- COMM PORT TRX ------------------------

static inline void comm_port_trx(comm_port *port)
{ byte c;
  int i, count = port->fifo ? 16 : 1;

  for (i = count; i--; ) {
    switch (port->control_type)
     { case NO_CONTROL :
       if (queue_empty(port->OutBuf))//queue empty, nothig to send
        { interrupt_off(port,THREINT);
          return;
        }
       queue_get_(port->OutBuf, &c);
       port->out_cnt++;
       outportb(port->THR,c);
       break;
       case XON_XOFF : // XON/XOFF
       if (port->xoff==1)
        { outportb(port->THR,XOFF_ASCII);
          port->xoff=0;
          port->xonxoff_send=XOFF_SENT;
        }
       else if (port->xon==1)
        { outportb(port->THR,XON_ASCII);
          port->xon=0;
          port->xonxoff_send=XON_SENT;
          return;
        }
       if (queue_empty(port->OutBuf))//queue empty, nothig to send
        { interrupt_off(port,THREINT);
          return;
        }
       queue_get_(port->OutBuf, &c);
       outportb(port->THR,c);
       port->out_cnt++;
       break;
       case RTS_CTS:
       if (port->cts==CTS_OFF)
        { return;
        }
       if (port->rts==RTS_OFF)
        { port->rts=RTS_ON;
          outportb(port->MCR,inportb(port->MCR)|0x02); //setting RTS
        }
       if (queue_empty(port->OutBuf))//queue empty, nothig to send
        { outportb(port->MCR,inportb(port->MCR)&(~0x02)); //dropping RTS
          port->rts=RTS_OFF;
          interrupt_off(port,THREINT);
          return;
        }
       queue_get_(port->OutBuf, &c);
       outportb(port->THR,c);
       port->out_cnt++;
       break;
       default :
       break;
     }
  }
}
static END_OF_FUNCTION(comm_port_trx);

//------------------------- COMM PORT INIT -----------------------------

void dzcomm_init(void)
{
  LOCK_FUNCTION(dz_comm_port_interrupt_handler);
  LOCK_FUNCTION(comm_port_rdr);
  LOCK_FUNCTION(comm_port_trx);
  LOCK_FUNCTION(interrupt_on);
  LOCK_FUNCTION(interrupt_off);
  LOCK_FUNCTION(com1_wrapper);
  LOCK_FUNCTION(com2_wrapper);
  LOCK_FUNCTION(com3_wrapper);
  LOCK_FUNCTION(com4_wrapper);
  LOCK_FUNCTION(com5_wrapper);
  LOCK_FUNCTION(com6_wrapper);
  LOCK_FUNCTION(com7_wrapper);
  LOCK_FUNCTION(com8_wrapper);
  LOCK_VARIABLE(com1);
  LOCK_VARIABLE(com2);
  LOCK_VARIABLE(com3);
  LOCK_VARIABLE(com4);
  LOCK_VARIABLE(com5);
  LOCK_VARIABLE(com6);
  LOCK_VARIABLE(com7);
  LOCK_VARIABLE(com8);
  com1=NULL;
  com2=NULL;
  com3=NULL;
  com4=NULL;
  com5=NULL;
  com6=NULL;
  com7=NULL;
  com8=NULL;
  com_wrapper[0]=com1_wrapper;
  com_wrapper[1]=com2_wrapper;
  com_wrapper[2]=com3_wrapper;
  com_wrapper[3]=com4_wrapper;
  com_wrapper[4]=com5_wrapper;
  com_wrapper[5]=com6_wrapper;
  com_wrapper[6]=com7_wrapper;
  com_wrapper[7]=com8_wrapper;
  com_port[0]=&com1;
  com_port[1]=&com2;
  com_port[2]=&com3;
  com_port[3]=&com4;
  com_port[4]=&com5;
  com_port[5]=&com6;
  com_port[6]=&com7;
  com_port[7]=&com8;

}

//----------------------- COMM PORT UNINSTALL ---------------------
// New routine to allow uninstalling without deleting
// and to simplyfy _delete and _reinstall
// by Neil Townsend
void comm_port_uninstall(comm_port *port)
{ comm_port *i_port;

  if (port->installed==NO) return;
  i_port = irq_bot_com_port[port->nIRQ];

  DISABLE();
  if (i_port == port)
   { irq_bot_com_port[port->nIRQ] = (comm_port *) port->last_port;
     if (irq_bot_com_port[port->nIRQ]) irq_bot_com_port[port->nIRQ]->next_port = NULL;
     i_port = NULL;
   }
  else i_port = (comm_port *) i_port->last_port;
  while (i_port)
   { if (i_port == port)
      { if (i_port->last_port) ((comm_port *)i_port->last_port)->next_port = i_port->next_port;
        if (i_port->next_port) ((comm_port *)i_port->next_port)->last_port = i_port->last_port;
        i_port = NULL;
      }
     if (i_port) i_port = i_port->last_port;
   }

  if (irq_bot_com_port[port->nIRQ] == NULL) _remove_irq(port->nIRQVector);

  { const byte IEROFF=0x00;
    const byte MCROFF=0x00;
    outportb(port->IER,IEROFF);
    outportb(port->MCR,MCROFF);
    outportb(port->IMR_8259,inportb(port->IMR_8259)&~(port->interrupt_enable_mask));
  }
  ENABLE();
  port->installed = NO;

}


//----------------------- COMM PORT REINSTALL ---------------------------

int comm_port_reinstall(comm_port *port)
{
  if (port->installed==NO) return(0);

  comm_port_uninstall(port);
  return (comm_port_install_handler(port));
}
