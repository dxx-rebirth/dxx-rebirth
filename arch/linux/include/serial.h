// Name: Serial port support for Linux D1X
// Author: dph
// Date: Sun Oct 18, 1998

#ifndef _SERIAL_H
#define _SERIAL_H
void com_done(void);
int com_init(void);
int com_read(char *buffer, int len, int timeout_value);
int com_write(char *buffer, int len);
void com_port_hangup();
int com_getdcd();
void com_flushbuffers();
void com_setbaudrate(int rate);
int com_readline(int timeout, char *input_buffer,int len);

extern int commlib_initialised;
#endif
