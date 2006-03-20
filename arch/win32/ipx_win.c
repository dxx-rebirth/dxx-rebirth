/* IPX driver using BSD style sockets */
/* Mostly taken from dosemu */
#include <string.h>
#include <stdlib.h>
#include <winsock.h>
#include <wsipx.h>
//#include <errno.h>

#include "ipx_drv.h"

#include "mono.h"

static ipx_socket_t mysock;

//#define n_printf(format, args...) mprintf((1, format, ## args))

static int ipx_win_GetMyAddress( void )
{
#if 0
  int sock;
  struct sockaddr_ipx ipxs;
  struct sockaddr_ipx ipxs2;
  int len;
  int i;
  
//  sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
  sock=socket(AF_IPX,SOCK_DGRAM,0);

  if(sock==-1)
  {
    mprintf((1,"IPX: could not open socket in GetMyAddress\n"));
    return(-1);
  }

  /* bind this socket to network 0 */  
  ipxs.sa_family=AF_IPX;
#ifdef IPX_MANUAL_ADDRESS
  memcpy(ipxs.sa_netnum, ipx_MyAddress, 4);
#else
  memset(ipxs.sa_netnum, 0, 4);
#endif  
  ipxs.sa_socket=0;
  
  if(bind(sock,(struct sockaddr *)&ipxs,sizeof(ipxs))==-1)
  {
    mprintf((1,"IPX: could bind to network 0 in GetMyAddress\n"));
    closesocket( sock );
    return(-1);
  }
  
  len = sizeof(ipxs2);
  if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
    mprintf((1,"IPX: could not get socket name in GetMyAddress\n"));
    closesocket( sock );
    return(-1);
  }

  memcpy(ipx_MyAddress, ipxs2.sa_netnum, 4);
  for (i = 0; i < 6; i++) {
    ipx_MyAddress[4+i] = ipxs2.sa_nodenum[i];
  }
/*  printf("My address is 0x%.4X:%.2X.%.2X.%.2X.%.2X.%.2X.%.2X\n", *((int *)ipxs2.sa_netnum), ipxs2.sa_nodenum[0],
  ipxs2.sa_nodenum[1], ipxs2.sa_nodenum[2], ipxs2.sa_nodenum[3], ipxs2.sa_nodenum[4], ipxs2.sa_nodenum[5]);*/

    closesocket( sock );

#endif
  return(0);
}

static int ipx_win_OpenSocket(int port)
{
  int sock;			/* sock here means Linux socket handle */
  int opt;
  struct sockaddr_ipx ipxs;
  int len;
  struct sockaddr_ipx ipxs2;

        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = MAKEWORD(2, 0);
        if (WSAStartup( wVersionRequested, &wsaData))
        {
          return -1;
        }
#if 0
        if ( LOBYTE( wsaData.wVersion ) != 2 ||
          HIBYTE( wsaData.wVersion ) != 0 ) {
           /* We couldn't find a usable WinSock DLL. */
           WSACleanup( );
           return -2;
        }
#endif


  /* DANG_FIXTHIS - kludge to support broken linux IPX stack */
  /* need to convert dynamic socket open into a real socket number */
/*  if (port == 0) {
    mprintf((1,"IPX: using socket %x\n", nextDynamicSocket));
    port = nextDynamicSocket++;
  }
*/
  /* do a socket call, then bind to this port */
//  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
//  sock = socket(AF_IPX, SOCK_DGRAM, 0);
  sock = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);//why NSPROTO_IPX?  I looked in the quake source and thats what they used. :) -MPM  (on w2k 0 and PF_IPX don't work)
  if (sock == -1) {
    mprintf((1,"IPX: could not open IPX socket.\n"));
    return -1;
  }


  opt = 1;
  /* Permit broadcast output */
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
		 (const char *)&opt, sizeof(opt)) == -1) {
    mprintf((1,"IPX: could not set socket option for broadcast.\n"));
    return -1;
  }
  ipxs.sa_family = AF_IPX;
  memcpy(ipxs.sa_netnum, ipx_MyAddress, 4);
//  *((unsigned int *)&ipxs.sa_netnum[0]) = *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sa_netnum = htonl(MyNetwork); */
  memset(ipxs.sa_nodenum, 0, 6);
//  bzero(ipxs.sa_nodenum, 6);	/* Please fill in my node name */
  ipxs.sa_socket = htons((short)port);

  /* now bind to this port */
  if (bind(sock, (struct sockaddr *) &ipxs, sizeof(ipxs)) == -1) {
    mprintf((1,"IPX: could not bind socket to address\n"));
    closesocket( sock );
    return -1;
  }
  
//  if( port==0 ) {
//    len = sizeof(ipxs2);
//    if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
//       //n_printf("IPX: could not get socket name in IPXOpenSocket\n");
//     closesocket( sock );
//      return -1;
//    } else {
//      port = htons(ipxs2.sa_socket);
//      //n_printf("IPX: opened dynamic socket %04x\n", port);
//    }
  len = sizeof(ipxs2);
  if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0) {
    mprintf((1,"IPX: could not get socket name in IPXOpenSocket\n"));
    closesocket( sock );
    return -1;
  } 
  if (port == 0) {
    port = htons(ipxs2.sa_socket);
    mprintf((1,"IPX: opened dynamic socket %04x\n", port));
  }

  memcpy(ipx_MyAddress, ipxs2.sa_netnum, 4);
  memcpy(ipx_MyAddress + 4, ipxs2.sa_nodenum, 6);

  mysock.fd = sock;
  mysock.socket = port;

  ipx_win_GetMyAddress();

  return 0;
}

static void ipx_win_CloseSocket(void) {
  /* now close the file descriptor for the socket, and free it */
  mprintf((1,"IPX: closing file descriptor on socket %x\n", mysock.socket));
  closesocket(mysock.fd);
  WSACleanup();
}

static int ipx_win_SendPacket(IPXPacket_t *IPXHeader,
 ubyte *data, int dataLen) {
  struct sockaddr_ipx ipxs;
 
  ipxs.sa_family = AF_IPX;
  /* get destination address from IPX packet header */
  memcpy(&ipxs.sa_netnum, IPXHeader->Destination.Network, 4);
  /* if destination address is 0, then send to my net */
  if ((*(unsigned int *)&ipxs.sa_netnum) == 0) {
    (*(unsigned int *)&ipxs.sa_netnum)= *((unsigned int *)&ipx_MyAddress[0]);
/*  ipxs.sa_netnum = htonl(MyNetwork); */
  }
  memcpy(&ipxs.sa_nodenum, IPXHeader->Destination.Node, 6);
//  memcpy(&ipxs.sa_socket, IPXHeader->Destination.Socket, 2);
  ipxs.sa_socket=htons(mysock.socket);
//  ipxs.sa_type = IPXHeader->PacketType;
  /*	ipxs.sipx_port=htons(0x452); */
  return sendto(mysock.fd, data, dataLen, 0,
	     (struct sockaddr *) &ipxs, sizeof(ipxs));
}

static int ipx_win_ReceivePacket(char *buffer, int bufsize, 
 struct ipx_recv_data *rd) {
	int sz, size;
	struct sockaddr_ipx ipxs;
 
	sz = sizeof(ipxs);
	if ((size = recvfrom(mysock.fd, buffer, bufsize, 0,
	     (struct sockaddr *) &ipxs, &sz)) <= 0)
	     return size;
        memcpy(rd->src_network, ipxs.sa_netnum, 4);
	memcpy(rd->src_node, ipxs.sa_nodenum, 6);
	rd->src_socket = ipxs.sa_socket;
	rd->dst_socket = mysock.socket;
//	rd->pkt_type = ipxs.sipx_type;
  
	return size;
}

static int ipx_win_general_PacketReady(void) {
	return ipx_general_PacketReady(mysock.fd);
}

struct ipx_driver ipx_win = {
//	ipx_win_GetMyAddress,
	ipx_win_OpenSocket,
	ipx_win_CloseSocket,
	ipx_win_SendPacket,
	ipx_win_ReceivePacket,
	ipx_win_general_PacketReady,
	NULL,
	1,
	NULL,
	NULL,
	NULL
};
