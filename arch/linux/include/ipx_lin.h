/*
 * $Source: /cvs/cvsroot/d2x/arch/linux/include/ipx_lin.h,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 07:29:37 $
 *
 * FIXME: add description
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifndef IPX_LINUX_H_
#define IPX_LINUX_H_
#include <sys/types.h>
#include "ipx_hlpr.h"
int ipx_linux_GetMyAddress(void);
int ipx_linux_OpenSocket(ipx_socket_t *sk, int port);
void ipx_linux_CloseSocket(ipx_socket_t *mysock);
int ipx_linux_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
 u_char *data, int dataLen);
int ipx_linux_ReceivePacket(ipx_socket_t *s, char *buffer, int bufsize,
 struct ipx_recv_data *rd);

#endif
