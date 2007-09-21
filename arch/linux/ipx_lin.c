#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#ifdef HAVE_NETIPX_IPX_H
#include <netipx/ipx.h>
#else
# include <linux/ipx.h>
# ifndef IPX_TYPE
#  define IPX_TYPE 1
# endif
#endif

#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "ipx_ld.h"
#include "ipx_hlpr.h"
#include "ipx_lin.h"

extern unsigned char ipx_MyAddress[10];

int 
ipx_linux_GetMyAddress( void )
{
  int sock;
  struct sockaddr_ipx ipxs;
  struct sockaddr_ipx ipxs2;
  int len;
  int i;
  
  sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
  if(sock==-1)
  {
    n_printf("IPX: could not open socket in GetMyAddress\n");
    return(-1);
  }

  /* bind this socket to network 0 */  
  ipxs.sipx_family=AF_IPX;
#ifdef IPX_MANUAL_ADDRESS
  memcpy(&ipxs.sipx_network, ipx_MyAddress, 4);
#else
  ipxs.sipx_network=0;
#endif  
  ipxs.sipx_port=0;
  
  if(bind(sock,(struct sockaddr *)&ipxs,sizeof(ipxs))==-1)
  {
    n_printf("IPX: could bind to network 0 in GetMyAddress\n");
    close( sock );
    return(-1);
  }
  
  len = sizeof(ipxs2);
  if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
    n_printf("IPX: could not get socket name in GetMyAddress\n");
    close( sock );
    return(-1);
  }

  memcpy(ipx_MyAddress, &ipxs2.sipx_network, 4);
  for (i = 0; i < 6; i++) {
    ipx_MyAddress[4+i] = ipxs2.sipx_node[i];
  }
  close( sock );
  return(0);
}

int
ipx_linux_OpenSocket(ipx_socket_t *sk, int port)
{
  int sock;			/* sock here means Linux socket handle */
  int opt;
  struct sockaddr_ipx ipxs;
  int len;
  struct sockaddr_ipx ipxs2;

  /* DANG_FIXTHIS - kludge to support broken linux IPX stack */
  /* need to convert dynamic socket open into a real socket number */
/*  if (port == 0) {
    n_printf("IPX: using socket %x\n", nextDynamicSocket);
    port = nextDynamicSocket++;
  }
*/
  /* do a socket call, then bind to this port */
  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
  if (sock == -1) {
    n_printf("IPX: could not open IPX socket.\n");
    return -1;
  }

#ifdef DOSEMU
  opt = 1;
  /* turn on socket debugging */
  if (d.network) {
    enter_priv_on();
    if (setsockopt(sock, SOL_SOCKET, SO_DEBUG, &opt, sizeof(opt)) == -1) {
      leave_priv_setting();
      n_printf("IPX: could not set socket option for debugging.\n");
      return -1;
    }
    leave_priv_setting();
  }
#endif  
  opt = 1;
  /* Permit broadcast output */
  enter_priv_on();
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
		 &opt, sizeof(opt)) == -1) {
    leave_priv_setting();
    n_printf("IPX: could not set socket option for broadcast.\n");
    return -1;
  }
#ifdef DOSEMU
  /* allow setting the type field in the IPX header */
  opt = 1;
#if 0 /* this seems to be wrong: IPX_TYPE can only be set on level SOL_IPX */
  if (setsockopt(sock, SOL_SOCKET, IPX_TYPE, &opt, sizeof(opt)) == -1) {
#else
  /* the socket _is_ an IPX socket, hence it first passes ipx_setsockopt()
   * in file linux/net/ipx/af_ipx.c. This one handles SOL_IPX itself and
   * passes SOL_SOCKET-levels down to sock_setsockopt().
   * Hence I guess the below is correct (can somebody please verify this?)
   * -- Hans, June 14 1997
   */
  if (setsockopt(sock, SOL_IPX, IPX_TYPE, &opt, sizeof(opt)) == -1) {
#endif
    leave_priv_setting();
    n_printf("IPX: could not set socket option for type.\n");
    return -1;
  }
#endif  
  ipxs.sipx_family = AF_IPX;
  ipxs.sipx_network = *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sipx_network = htonl(MyNetwork); */
  bzero(ipxs.sipx_node, 6);	/* Please fill in my node name */
  ipxs.sipx_port = htons(port);

  /* now bind to this port */
  if (bind(sock, (struct sockaddr *) &ipxs, sizeof(ipxs)) == -1) {
    n_printf("IPX: could not bind socket to address\n");
    close( sock );
    leave_priv_setting();
    return -1;
  }
  
  if( port==0 ) {
    len = sizeof(ipxs2);
    if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
      n_printf("IPX: could not get socket name in IPXOpenSocket\n");
      close( sock );
      leave_priv_setting();
      return -1;
    } else {
      port = htons(ipxs2.sipx_port);
      n_printf("IPX: opened dynamic socket %04x\n", port);
    }
  }
  leave_priv_setting();
  sk->fd = sock;
  sk->socket = port;
  return 0;
}

void ipx_linux_CloseSocket(ipx_socket_t *mysock) {
  /* now close the file descriptor for the socket, and free it */
  n_printf("IPX: closing file descriptor on socket %x\n", mysock->socket);
  close(mysock->fd);
}

int ipx_linux_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
 u_char *data, int dataLen) {
  struct sockaddr_ipx ipxs;
 
  ipxs.sipx_family = AF_IPX;
  /* get destination address from IPX packet header */
  memcpy(&ipxs.sipx_network, IPXHeader->Destination.Network, 4);
  /* if destination address is 0, then send to my net */
  if (ipxs.sipx_network == 0) {
    ipxs.sipx_network = *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sipx_network = htonl(MyNetwork); */
  }
  memcpy(&ipxs.sipx_node, IPXHeader->Destination.Node, 6);
  memcpy(&ipxs.sipx_port, IPXHeader->Destination.Socket, 2);
  ipxs.sipx_type = IPXHeader->PacketType;
  /*	ipxs.sipx_port=htons(0x452); */
  return sendto(mysock->fd, data, dataLen, 0,
	     (struct sockaddr *) &ipxs, sizeof(ipxs));
}

int ipx_linux_ReceivePacket(ipx_socket_t *s, char *buffer, int bufsize, 
 struct ipx_recv_data *rd) {
	int sz, size;
	struct sockaddr_ipx ipxs;
 
	sz = sizeof(ipxs);
	if ((size = recvfrom(s->fd, buffer, bufsize, 0,
	     (struct sockaddr *) &ipxs, &sz)) <= 0)
	     return size;
	memcpy(rd->src_network, &ipxs.sipx_network, 4);
	memcpy(rd->src_node, ipxs.sipx_node, 6);
	rd->src_socket = ipxs.sipx_port;
	rd->dst_socket = s->socket;
	rd->pkt_type = ipxs.sipx_type;
  
	return size;
}
