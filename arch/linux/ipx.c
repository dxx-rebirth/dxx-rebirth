/* IPX driver using BSD style sockets */
/* Mostly taken from dosemu */
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netipx/ipx.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "net_ipx.h"
#include "console.h"

static int ipx_get_my_address( void )
{
	int sock;
	struct sockaddr_ipx ipxs;
	struct sockaddr_ipx ipxs2;
	socklen_t len;
	int i;
	
	sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
	if(sock==-1)
	{
		con_printf(CON_URGENT,"IPX: could not open socket in GetMyAddress\n");
		return(-1);
	}
	
	/* bind this socket to network 0 */
	ipxs.sipx_family=AF_IPX;
	memcpy(&ipxs.sipx_network, MyAddress, 4);
	ipxs.sipx_port=0;
	
	if(bind(sock,(struct sockaddr *)&ipxs,sizeof(ipxs))==-1)
	{
		con_printf(CON_URGENT,"IPX: could bind to network 0 in GetMyAddress\n");
		close( sock );
		return(-1);
	}
	
	len = sizeof(ipxs2);
	if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0)
	{
		con_printf(CON_URGENT,"IPX: could not get socket name in GetMyAddress\n");
		close( sock );
		return(-1);
	}
	
	memcpy(MyAddress, &ipxs2.sipx_network, 4);
	for (i = 0; i < 6; i++)
	{
		MyAddress[4+i] = ipxs2.sipx_node[i];
	}
	close( sock );
	return(0);
}

static int ipx_open_socket(socket_t *sk, int port)
{
	int sock;			/* sock here means Linux socket handle */
	int opt;
	struct sockaddr_ipx ipxs;
	socklen_t len;
	struct sockaddr_ipx ipxs2;

	/* do a socket call, then bind to this port */
	sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
	if (sock == -1) {
		con_printf(CON_URGENT,"IPX: could not open IPX socket.\n");
		return -1;
	}

	opt = 1;
	/* Permit broadcast output */
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) == -1)
	{
		con_printf(CON_URGENT,"IPX: could not set socket option for broadcast.\n");
		return -1;
	}

	ipxs.sipx_family = AF_IPX;
	ipxs.sipx_network = *((unsigned int *)&MyAddress[0]);
	/*  ipxs.sipx_network = htonl(MyNetwork); */
	bzero(ipxs.sipx_node, 6);	/* Please fill in my node name */
	ipxs.sipx_port = htons(port);
	
	/* now bind to this port */
	if (bind(sock, (struct sockaddr *) &ipxs, sizeof(ipxs)) == -1)
	{
		con_printf(CON_URGENT,"IPX: could not bind socket to address\n");
		close( sock );
		return -1;
	}

	if( port==0 )
	{
		len = sizeof(ipxs2);
		if (getsockname(sock,(struct sockaddr *)&ipxs2,&len) < 0)
		{
			con_printf(CON_URGENT,"IPX: could not get socket name in ipx_open_socket\n");
			close( sock );
			return -1;
		}
		else
		{
			port = htons(ipxs2.sipx_port);
			con_printf(CON_URGENT,"IPX: opened dynamic socket %04x\n", port);
		}
	}

	sk->fd = sock;
	sk->socket = port;

	ipx_get_my_address();

	return 0;
}

static void ipx_close_socket(socket_t *mysock)
{
	/* now close the file descriptor for the socket, and free it */
	con_printf(CON_URGENT,"IPX: closing file descriptor on socket %x\n", mysock->socket);
	close(mysock->fd);
}

static int ipx_send_packet(socket_t *mysock, IPXPacket_t *IPXHeader, u_char *data, int dataLen)
{
	struct sockaddr_ipx ipxs;
	
	ipxs.sipx_family = AF_IPX;
	/* get destination address from IPX packet header */
	memcpy(&ipxs.sipx_network, IPXHeader->Destination.Network, 4);
	/* if destination address is 0, then send to my net */
	if (ipxs.sipx_network == 0)
	{
		ipxs.sipx_network = *((unsigned int *)&MyAddress[0]);
	}

	memcpy(&ipxs.sipx_node, IPXHeader->Destination.Node, 6);
	ipxs.sipx_port=htons(mysock->socket);
	ipxs.sipx_type = IPXHeader->PacketType;

	return sendto(mysock->fd, data, dataLen, 0, (struct sockaddr *) &ipxs, sizeof(ipxs));
}

static int ipx_receive_packet(socket_t *s, char *buffer, int bufsize, struct recv_data *rd)
{
	socklen_t sz;
	int size;
	struct sockaddr_ipx ipxs;
 
	sz = sizeof(ipxs);
	if ((size = recvfrom(s->fd, buffer, bufsize, 0, (struct sockaddr *) &ipxs, &sz)) <= 0)
	     return size;
	memcpy(rd->src_network, &ipxs.sipx_network, 4);
	memcpy(rd->src_node, ipxs.sipx_node, 6);
	rd->src_socket = ipxs.sipx_port;
	rd->dst_socket = s->socket;
	rd->pkt_type = ipxs.sipx_type;

	return size;
}

static int ipx_general_packet_ready(socket_t *s)
{
	return ipxdrv_general_packet_ready(s->fd);
}

struct net_driver ipxdrv_ipx = {
	ipx_open_socket,
	ipx_close_socket,
	ipx_send_packet,
	ipx_receive_packet,
	ipx_general_packet_ready,
	1,
	NETPROTO_IPX
};
