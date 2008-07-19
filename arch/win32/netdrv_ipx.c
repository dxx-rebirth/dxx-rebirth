/* IPX driver using BSD style sockets */
/* Mostly taken from dosemu */
#include <string.h>
#include <stdlib.h>
#include <winsock.h>
#include <wsipx.h>
#include "netdrv.h"
#include "console.h"

static int IPXGetMyAddress( void )
{
	return(0);
}

static int IPXOpenSocket(socket_t *sk, int port)
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

	sock = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);

	if (sock == -1) {
		con_printf(CON_URGENT,"IPX: could not open IPX socket.\n");
		return -1;
	}

	opt = 1;
	/* Permit broadcast output */
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,(const char *)&opt, sizeof(opt)) == -1)
	{
		con_printf(CON_URGENT,"IPX: could not set socket option for broadcast.\n");
		return -1;
	}

	ipxs.sa_family = AF_IPX;
	memcpy(ipxs.sa_netnum, MyAddress, 4);
	memset(ipxs.sa_nodenum, 0, 6);
	//  bzero(ipxs.sa_nodenum, 6);	/* Please fill in my node name */
	ipxs.sa_socket = htons((short)port);

	/* now bind to this port */
	if (bind(sock, (struct sockaddr *) &ipxs, sizeof(ipxs)) == -1)
	{
		con_printf(CON_URGENT,"IPX: could not bind socket to address\n");
		closesocket( sock );
		return -1;
	}

	len = sizeof(ipxs2);
	if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0)
	{
		con_printf(CON_URGENT,"IPX: could not get socket name in IPXOpenSocket\n");
		closesocket( sock );
		return -1;
	}

	if (port == 0)
	{
		port = htons(ipxs2.sa_socket);
		con_printf(CON_URGENT,"IPX: opened dynamic socket %04x\n", port);
	}

	memcpy(MyAddress, ipxs2.sa_netnum, 4);
	memcpy(MyAddress + 4, ipxs2.sa_nodenum, 6);

	sk->fd = sock;
	sk->socket = port;

	IPXGetMyAddress();
	
	return 0;
}

static void IPXCloseSocket(socket_t *mysock)
{
	/* now close the file descriptor for the socket, and free it */
	con_printf(CON_URGENT,"IPX: closing file descriptor on socket %x\n", mysock->socket);
	closesocket(mysock->fd);
	WSACleanup();
}

static int IPXSendPacket(socket_t *mysock, IPXPacket_t *IPXHeader, ubyte *data, int dataLen)
{
	struct sockaddr_ipx ipxs;
	
	ipxs.sa_family = AF_IPX;
	/* get destination address from IPX packet header */
	memcpy(&ipxs.sa_netnum, IPXHeader->Destination.Network, 4);
	/* if destination address is 0, then send to my net */
	if ((*(unsigned int *)&ipxs.sa_netnum) == 0)
	{
		(*(unsigned int *)&ipxs.sa_netnum)= *((unsigned int *)&MyAddress[0]);
	}
	memcpy(&ipxs.sa_nodenum, IPXHeader->Destination.Node, 6);
	ipxs.sa_socket=htons(mysock->socket);
	
	return sendto(mysock->fd, data, dataLen, 0, (struct sockaddr *) &ipxs, sizeof(ipxs));
}

static int IPXReceivePacket(socket_t *s, char *buffer, int bufsize, struct recv_data *rd)
{
	int sz, size;
	struct sockaddr_ipx ipxs;

	sz = sizeof(ipxs);
	if ((size = recvfrom(s->fd, buffer, bufsize, 0, (struct sockaddr *) &ipxs, &sz)) <= 0)
		return size;
        memcpy(rd->src_network, ipxs.sa_netnum, 4);
	memcpy(rd->src_node, ipxs.sa_nodenum, 6);
	rd->src_socket = ipxs.sa_socket;
	rd->dst_socket = s->socket;

	return size;
}

static int IPXgeneral_PacketReady(socket_t *s)
{
	return NetDrvGeneralPacketReady(s->fd);
}

struct net_driver netdrv_ipx = {
	IPXOpenSocket,
	IPXCloseSocket,
	IPXSendPacket,
	IPXReceivePacket,
	IPXgeneral_PacketReady,
	1,
	NETPROTO_IPX
};
