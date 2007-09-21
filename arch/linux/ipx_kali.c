/* IPX driver for KaliNix interface */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h> /* for htons & co. */
#include "ipx_drv.h"
#include "ukali.h"

static ipx_socket_t mysock;

static int open_sockets = 0;
static int dynamic_socket = 0x401;
//static int last_socket = 0;

int have_empty_address() {
	int i;
	for (i = 0; i < 10 && !ipx_MyAddress[i]; i++) ;
	return i == 10;
}

int ipx_kali_GetMyAddress(void) {

	kaliaddr_ipx mKaliAddr;

	if (!have_empty_address())
		return 0;

	if (KaliGetNodeNum(&mKaliAddr) < 0)
		return -1;

	memset(ipx_MyAddress, 0, 4);
	memcpy(ipx_MyAddress + 4, mKaliAddr.sa_nodenum, sizeof(mKaliAddr.sa_nodenum));

	return 0;
}

int ipx_kali_OpenSocket(int port) {
	printf("IPX_kali: OpenSocket on port(%d)\n", port);

	if (!open_sockets) {
		if (have_empty_address()) {
			if (ipx_kali_GetMyAddress() < 0) {
				printf("IPX_kali: Error communicating with KaliNix\n");
				return -1;
			}
		}
	}
	if (!port)
		port = dynamic_socket++;

	if ((mysock.fd = KaliOpenSocket(htons(port))) < 0) {
		printf("IPX_kali: OpenSocket Failed on port(%d)\n", port);
		mysock.fd = -1;
		return -1;
	}
	open_sockets++;
//	last_socket = port;
	mysock.socket = port;
	return 0;
}

void ipx_kali_CloseSocket(void) {
	if (!open_sockets) {
		printf("IPX_kali: close w/o open\n");
		return;
	}
	printf("IPX_kali: CloseSocket on port(%d)\n", mysock.socket);
	KaliCloseSocket(mysock.fd);
	if (--open_sockets) {
		printf("IPX_kali: (closesocket) %d sockets left\n", open_sockets);
		return;
	}
}

int ipx_kali_SendPacket(IPXPacket_t *IPXHeader,
 u_char *data, int dataLen) {
	kaliaddr_ipx toaddr;
	int i;
 
	memcpy(toaddr.sa_nodenum, IPXHeader->Destination.Node, sizeof(toaddr.sa_nodenum));
//	memcpy(&toaddr.sa_socket, IPXHeader->Destination.Socket, sizeof(toaddr.sa_socket));
	toaddr.sa_socket=htons(mysock.socket);

	if ((i = KaliSendPacket(mysock.fd, (char *)data, dataLen, &toaddr)) < 0)
		return -1;

	return i;
}

int ipx_kali_ReceivePacket(char *outbuf, int outbufsize, 
 struct ipx_recv_data *rd) {
	int size;
	kaliaddr_ipx fromaddr;

	if ((size = KaliReceivePacket(mysock.fd, outbuf, outbufsize, &fromaddr)) < 0)
		return -1;

	rd->dst_socket = mysock.socket;
	rd->src_socket = ntohs(fromaddr.sa_socket);
	memcpy(rd->src_node, fromaddr.sa_nodenum, sizeof(fromaddr.sa_nodenum));
	memset(rd->src_network, 0, 4);
	rd->pkt_type = 0;

	return size;
}

static int ipx_kali_general_PacketReady(void) {
	return ipx_general_PacketReady(mysock.fd);
}

struct ipx_driver ipx_kali = {
//	ipx_kali_GetMyAddress,
	ipx_kali_OpenSocket,
	ipx_kali_CloseSocket,
	ipx_kali_SendPacket,
	ipx_kali_ReceivePacket,
	ipx_kali_general_PacketReady,
	NULL,
	1,
	NULL,
	NULL,
	NULL
};
