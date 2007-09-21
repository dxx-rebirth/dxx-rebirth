#ifndef IPX_LINUX_H_
#define IPX_LINUX_H_
#include <sys/types.h>
#include "ipx_helper.h"
int ipx_linux_GetMyAddress(void);
int ipx_linux_OpenSocket(ipx_socket_t *sk, int port);
void ipx_linux_CloseSocket(ipx_socket_t *mysock);
int ipx_linux_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
 u_char *data, int dataLen);
int ipx_linux_ReceivePacket(ipx_socket_t *s, char *buffer, int bufsize,
 struct ipx_recv_data *rd);

#endif
