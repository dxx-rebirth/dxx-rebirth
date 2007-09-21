#ifndef _SERIAL_H
#define _SERIAL_H

extern void com_done(void);
extern int com_init(void);
extern int com_read(char *buffer, int len, int timeout_value);
extern int com_write(char *buffer, int len);
extern void com_port_hangup();
extern int com_getdcd();
extern void com_flushbuffers();
extern void com_setbaudrate(int rate);
extern int com_readline(int timeout_value, char *input_buffer,int len);

extern int commlib_initialised;
#endif
