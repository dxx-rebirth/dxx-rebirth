//added 06/09/99 Matt Mueller - fix nonetwork compile
#ifdef NETWORK
//end addition -MM
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "serial.h"
//added 02/06/99 Matt Mueller - allow selection of different devices
#include "args.h"
//end addition
//added 05/17/99 Matt Mueller - needed to redefine FD_* so that no asm is used
#include "checker.h"
//end addition

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/modem"
#define FALSE 0
#define TRUE 1

//edited 03/05/99 Matt Mueller - allow reading from different device
static int fd,rfd;
//end edit -MM
static struct termios oldtio, newtio;

void com_done(void)
{
 if (!commlib_initialised) return;
 commlib_initialised=0;
 tcsetattr(fd, TCSANOW, &oldtio);
 close(fd);
}

int com_init(void)
{
//edited 02/06/99 Matt Mueller - allow selection of different devices
 char *modem=MODEMDEVICE;
 int t;
 if ((t = FindArg( "-serialdevice" ))) {
	 modem=Args[t+1];
 }
 //edited 03/05/99 Matt Mueller - allow reading from different device
 if ((t = FindArg( "-serialread" ))) {
     char *readpipe=NULL;
     readpipe=Args[t+1];
     rfd=open(readpipe, O_RDONLY | O_NOCTTY | O_NONBLOCK);
     if (rfd < 0) { perror(readpipe); return -1; }
 }else
     rfd=-1;
	     
 if (commlib_initialised) return -1;

 fd=open(modem, (rfd<0?O_RDWR:O_WRONLY) | O_NOCTTY);
 if (fd < 0) { perror(modem); return -1; }
 if (rfd<0)
    rfd=fd;
 //end edit -MM
//end edit -MM
	    
 tcgetattr(fd, &oldtio);
 bzero(&newtio, sizeof(newtio));
 
 newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
 newtio.c_iflag = IGNPAR;
 newtio.c_oflag = 0;

 /* set input mode (non-canonical, no echo,...) */
 newtio.c_lflag = 0;
 
 newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
 newtio.c_cc[VMIN]     = 0;   /* don't block on reads */

 tcflush(fd, TCIFLUSH);
 tcsetattr(fd, TCSANOW, &newtio);
    
 atexit(com_done);
 commlib_initialised=1;
 return 0;
}

//edited 03/05/99 Matt Mueller - allow reading from different device
int com_read(char *buffer, int len, int timeout_value)
{
 struct timeval timeout;
 fd_set set;
 int i;

 if (!commlib_initialised) return -1;
 if (timeout_value==0) return read(rfd,buffer,len);

 /* Initialise the file descriptor set */
 FD_ZERO(&set);
 FD_SET (rfd, &set);

 /* Initialise the timeout timer value */
 timeout.tv_sec=timeout_value / 1000;
//edited 02/06/99 Matt Mueller - microseconds, not milliseconds
 timeout.tv_usec=(timeout_value % 1000) * 1000;
//end edit -MM

 i=select(FD_SETSIZE, &set, NULL, NULL, &timeout);
 if (i>0) {
  i=read(rfd,buffer,len);
  return i;
 }
 return i;
}

int com_write(char *buffer, int len)
{
 if (!commlib_initialised) return -1;
 return write(fd,buffer,len);
}
//end edit -MM

/* Hangup by dropping speed down to 0 and raising it again */
//edited 02/06/99 Matt Mueller - added the standard ath stuff, since the baud=0 didn't work for my modem
void com_port_hangup()
{
 struct termios save, temp;

 if (!commlib_initialised) return;

 com_write("+++",3);
 tcgetattr(fd, &save);
 tcgetattr(fd, &temp);
 cfsetispeed(&temp, B0);
 cfsetospeed(&temp, B0);
 tcsetattr(fd, TCSANOW, &temp);
 sleep(1);
 tcsetattr(fd, TCSANOW, &save);
 com_write("ath0\r\n",5);
 com_flushbuffers();//this should (hopefully?) clear out the OK and such.
//end edit -MM
}

/* Get the status of the DCD (Data Carrier Detect) line */
int com_getdcd()
{
 int msr;
 if (!commlib_initialised) return -1;
 ioctl(fd, TIOCMGET, &msr);
 return ((msr&TIOCM_CAR)?1:0);
}

void com_flushbuffers()
{
 if (!commlib_initialised) return;
 ioctl(fd, TCFLSH, 2);
}

void com_setbaudrate(int rate)
{
 int speed;
 struct termios temp;

 if (!commlib_initialised) return;
 switch(rate) {
   case 9600: speed=B9600; break;
   case 19200: speed=B19200; break;
   case 38400: speed=B38400; break;
   default: speed=B19200; break;
 }
 tcgetattr(fd, &temp);
 cfsetispeed(&temp, speed);
 cfsetospeed(&temp, speed);
 tcsetattr(fd, TCSANOW, &temp);
}

int com_readline(int timeout_value, char *input_buffer,int len)
{
 char c;
 int j;
 int i=0;
 struct timeval timeout;
 fd_set set;

 if (timeout_value>0) {
 /* Initialise the file descriptor set */
 FD_ZERO(&set);
//edited 03/05/99 Matt Mueller - allow reading from different device
 FD_SET (rfd, &set);
//end edit -MM

 /* Initialise the timeout timer value */
 timeout.tv_sec=timeout_value / 1000;
//edited 02/06/99 Matt Mueller - microseconds, not milliseconds
 timeout.tv_usec=(timeout_value % 1000) * 1000;
//end edit -MM
	 
 j=select(FD_SETSIZE, &set, NULL, NULL, &timeout);
 if (j==0) return 0;
 }
 
 do {
//  j=com_read(&c,1,0);
  j=com_read(&c,1,timeout_value);
  if (isprint(c) && (j>0)) {
   input_buffer[i++]=c;
   if (i>=len) return i;
  }
 } while (c!='\n');
 input_buffer[i]=0;
 return i;
}

//added 06/09/99 Matt Mueller - fix nonetwork compile
#endif
//end addition -MM
