/*
 * ipclient.h - udp/ip client code
 * added 2000/02/07 Matt Mueller
 */
#ifndef __IPCLIENT_H_
#define __IPCLIENT_H_

#include <stdio.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C"{
#endif
#include "types.h"
#include "ipx.h"
#include "ipx_drv.h"
void ip_sendtoall(char *buf,int len);
int ip_connect_manual(char *textaddr);//make it extern C so that it can be called from .c files.
//void ip_portshift(ubyte*qhbuf,const char *cs);

int arch_ip_get_my_addr(ushort myport);
int arch_ip_open_socket(int port);
void arch_ip_close_socket(void);
int arch_ip_recvfrom(char *outbuf,int outbufsize,struct ipx_recv_data *rd);
int arch_ip_PacketReady(void);
int arch_ip_queryhost(ip_addr *addr,char *buf,ushort baseport);

int ipx_ip_GetMyAddress(void);

extern int myport;

extern int baseport;


#ifdef __cplusplus
}
#endif


#define MSGHDR "IPX_ip: "

static inline void msg(const char *fmt,...)
{
	va_list ap;
	fputs(MSGHDR,stdout);
	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
	putchar('\n');
}

#define FAIL(m...) do { msg(#m); return -1; } while (0)

static inline void chk(void *p){
	if (p) return;
	msg("FATAL: Virtual memory exhausted!");
	exit(EXIT_FAILURE);
}


#endif
