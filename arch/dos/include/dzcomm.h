/*
 * DZcomm : serial communication add-on for Allegro.
 * Copyright (c) 1997 Dim Zegebart, Moscow Russia.
 * zager@post.comstar.ru
 * file : dzcomm.h
 * version : 0.6
 */

//#x# are marks for automatic documentator and may be ignored by human reader.
//#d# function description
//#v# returned values
//#c# any comments, notes, etc.
//#n# exclude line from documentation
//#sb# begin structure. Type structure name (if any) after this tag.
//#ds# in structure description means what field maintained by system.
//#se# end structure
//#p# parameters list
//#dv# variable description

#ifdef __cplusplus
extern "C" {
#endif

#define not_commented(_s_) ((_s_)[-1]!='/'&&(_s_)[-2]!='/')

//-------------------------- SOME DECODERS --------------------------

#define scan_(_key_)  ({((_key_)>>8)&0xff;}) //#d#extract scan part
#define ascii_(_key_) ({(_key_)&0xff;})//#d#extract ascii part
#define key_shifts_(_key_) ({((_key_)>>16)&0xff;})//#d#extract shifts keys status

#define data_(_comm_data_) ({(_comm_data_)&0xff;}) //#d#extract data unsigned char
#define q_reg_(_comm_data_) ({((_comm_data_)>>16)&0xff;}) //#d#extract status registers unsigned char

//#d#Determine if Ctrl-... pressed
//#v# 1 - if yes. For example if(ctrl_,'C') {exit(1);}
//0 - if not
#define ctrl_(_key_,c)\
({((key_shifts&KB_CTRL_FLAG)&&((ascii_(_key_)+'A'-1)==(c)));})

//----------------------------- FIFO QUEUE ------------------------
//#sb# First In First Out queue
typedef struct
{ unsigned int size;  //size of queue. Use queue_resize(new_size) to change queue size
  unsigned int dsize; //size of data element (default 4 unsigned chars)
  unsigned int initial_size;
  unsigned int resize_counter;
  unsigned int fill_level;
  int (*empty_handler)();
  void *queue;     //pointer to queue. Points to int[], where actual data stored.
  unsigned int head,tail;  //number of head and tail elements
                   //head points to the first element in queue
                   //tail points to the first element next to last
                   //element in queue. (For example : we have ten elements
                   //in queue, then head==0, tail==10
} fifo_queue;
//#se#
fifo_queue* queue_new(unsigned int size); //#d#Allocate storage space for new fifo_queue.
fifo_queue* queue_new_(unsigned int size,unsigned int dsize);
//#v# NULL if failed
// pointer to newly created fifo_queue structure
void queue_delete(fifo_queue *q); //#d#Delete queue description and free occuped space.
inline void queue_reset(fifo_queue *q);//#d#Set head and tail to zero (data not lost).
int queue_resize(fifo_queue *q,unsigned int new_size); //#d#Increase(decrease) storage space
//for fifo queue.
//#v# 0 if failed
//1 if success
inline int queue_put(fifo_queue *q,int c); //#d#Put value into given queue.
inline int queue_get(fifo_queue *q); //#d#Get values from queue.
inline int queue_put_(fifo_queue *q,void *data);
inline int queue_get_(fifo_queue *q,void *data);

//#v# Byte from queue's head
//#c# Use queue_empty befor calling queue_get. queue_get don't test
// queue's head and tail, just return queue[head].
inline int queue_empty(fifo_queue *q);//#d#Test queue for emptyness.
//#v# 1 if empty (head==tail)
// 0 if not empty (head!=tail)
//#c# Call this function before any queue_get calling

//---------------------------- COMM_PORT --------------------------

typedef enum {_com1,_com2,_com3,_com4,_com5,_com6,_com7,_com8} comm;
typedef enum {BITS_8=0x03,BITS_7=0x02,BITS_6=0x01,BITS_5=0x00} data_bits;
typedef enum {STOP_1=0x00,STOP_2=0x01} stop_bits;
typedef enum {EVEN_PARITY=0x03,ODD_PARITY=0x01,NO_PARITY=0x00} parity_bits;
typedef enum {_110=110,_150=150,_300=300,_600=600,
              _1200=1200,_2400=2400,_4800=4800,
              _9600=9600,_19200=19200,_38400=38400,
              _57600=57600,_115200=115200} baud_bits;
//typedef enum {DTR_ON=0xFD,RTS_ON=0xFE,DTR_OFF=~0x1,RTS_OFF=~0x2} hand;
typedef enum {CTS_ON=0x01,CTS_OFF=0x0,RTS_ON=0x1,RTS_OFF=0x0} hand;
typedef enum {XON_RCVD=1,XON_SENT=1,XOFF_RCVD=0,XOFF_SENT=0} xon_xoff_status;
typedef enum {NO_CONTROL=1,XON_XOFF=2,RTS_CTS=3} flow_control_type;
typedef enum {XON_ASCII=0x11,XOFF_ASCII=0x13} control_char;

extern char szDZCommErr[50];

typedef enum {DATA_BYTE=1,MSR_BYTE=2,LSR_BYTE=4} comm_status_byte;

//#sb#Comm port.
typedef struct tag_comm_port
{ //general parametrs
//You may alter this six  fields after comm_port_new() where defaults values are set.
  char szName[255]; //#d#name of comm port. For example : MODEM
  comm  nComm; //#d#comm port number
  unsigned short nPort; //#d#comm port address
  unsigned char  nIRQ; //#d#comm IRQ
  unsigned short nIRQVector; //#d#number of software interrupt vector.
  flow_control_type control_type; //default XON_XOFF

  unsigned char xon;
  unsigned char xoff;
  unsigned char xonxoff_send;
  unsigned char xonxoff_rcvd;
  unsigned char rts;
  unsigned char cts;

 //next two fields are for unsigned char counting
  unsigned int in_cnt; //counter for input data
  unsigned int out_cnt; //counter for output data

  unsigned char  interrupt_enable_mask; //#ds#
  int (*comm_handler)(); //#d#pointer to short function see below
  int (*msr_handler)(); //#d#pointer to modem status register handler.
  int (*lsr_handler)(); //#d#pointer to line status register handler.
// template to comm's interrupt handler
//comm_port *your_comm,*your_comm1; //must be declared globaly
//int foo_comm_wrapper(void)
//{ dz_comm_interrupt_handler(ptr_your_comm);
//  return(0);
//}

  enum {YES,NO} installed; //#ds#

  //communication parametrs
  baud_bits   nBaud;   //#d#baud rate
  data_bits   nData;   //#d#data length
  stop_bits   nStop;   //#d#stop bits
  parity_bits nParity; //#d#parity

  //input and output queue
  fifo_queue *InBuf; //#ds#pointer to read buffer
  fifo_queue *OutBuf; //#ds#pointer to write buffer

  unsigned short THR;  //#ds# Transmitter Holding Register */
  unsigned short RDR;  //#ds# Reciever Data Register */
  unsigned short BRDL; //#ds#Baud Rate Divisor, Low unsigned short */
  unsigned short BRDH; //#ds# Baud Rate Divisor, High Byte */
  unsigned short IER;  //#ds# Interupt Enable Register */
  unsigned short IIR;  //#ds# Interupt Identification Register */
  unsigned short FCR;  //#ds# FIFO Control Register */
  unsigned short LCR;  //#ds# Line Control Register */
  unsigned short MCR;  //#ds# Modem Control Register */
  unsigned short LSR;  //#ds# Line Status Register */
  unsigned short MSR;  //#ds# Modem Status Register */
  unsigned short SCR;  //#ds# SCR Register */
  unsigned short ISR_8259; //#ds#interrupt service register
  unsigned short IMR_8259; //#ds#interrupt mask register

  int fifo; // 1 if 16550 FIFO enabled

  void *next_port;
  void *last_port;
} comm_port;
#define cport_(_p_) ({(comm_port*)(_p_);})
//#se#
extern char szDZCommErr[50];
comm_port *comm_port_init(comm com);
int comm_port_install_handler(comm_port *port);
inline void comm_port_out(comm_port *port,unsigned char c);
inline int comm_port_test(comm_port *port);
inline int dz_comm_port_interrupt_handler(comm_port *port);
void comm_port_delete(comm_port *port);
void comm_port_string_send(comm_port *port,char *s);
void comm_port_command_send(comm_port *port,char *s);
void comm_port_break_send(comm_port *port);
void comm_port_hand(comm_port *port,int m);
int comm_port_load_settings(comm_port *port,char *ini_name);
inline int comm_port_send_xoff(comm_port *port);
inline int comm_port_send_xon(comm_port *port);
int comm_port_reinsatall(comm_port *port);
void modem_hangup(comm_port *port);
void dzcomm_init(void);
void print_bin_s(int c,char *s);

extern comm_port *com1;
extern comm_port *com2;
extern comm_port *com3;
extern comm_port *com4;
extern comm_port *com5;
extern comm_port *com6;
extern comm_port *com7;
extern comm_port *com8;

#define LSR_DATAREADY  1
#define LSR_OVERRUN    2
#define LSR_PARITYERR  4
#define LSR_FRAMINGERR 8
#define LSR_BREAK      16
#define LSR_THRE       32  /* transmitter holding register empty */
#define LSR_TSRE       64  /* transmitter shift register empty */
#define LSR_FIFOERR    128 /* 16550 PE/FE/Break in FIFO queue, 0 for 8250 & 16450 */

#define MSR_DCTS       1
#define MSR_DRTS       2
#define MSR_DRI        4
#define MSR_DDCD       8
#define MSR_CTS        16
#define MSR_RTS        32
#define MSR_RI        64
#define MSR_DCD        128


#ifdef __cplusplus
} //end extern "C"
#endif



