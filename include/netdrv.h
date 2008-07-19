/*
 *
 * IPX-based driver interface
 *
 */

#ifndef _NET_DRV_H
#define _NET_DRV_H

#include "pstypes.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h> /* for htons & co. */
#endif

#define MAX_PACKET_DATA		1500
#define MAX_DATA_SIZE		542
#define MAX_IPX_DATA		576

#define IPX_DEFAULT_SOCKET 0x5100

#define NETPROTO_IPX		1
#define NETPROTO_KALINIX	2
#define NETPROTO_UDP		3

typedef struct IPXAddressStruct {
	ubyte Network[4];
	ubyte Node[6];
	ubyte Socket[2];
} IPXAddress_t;

typedef struct IPXPacketStructure {
	ushort Checksum;
	ushort Length;
	ubyte TransportControl;
	ubyte PacketType;
	IPXAddress_t Destination;
	IPXAddress_t Source;
} IPXPacket_t;

typedef struct socket_struct {
  ushort socket;
  int fd;
} socket_t;

struct recv_data {
	/* all network order */
	ubyte src_network[4];
	ubyte src_node[6];
	ushort src_socket;
	ushort dst_socket;
	int pkt_type;
};

struct net_driver {
	int (*OpenSocket)(socket_t *sk, int port);
	void (*CloseSocket)(socket_t *mysock);
	int (*SendPacket)(socket_t *mysock, IPXPacket_t *IPXHeader, ubyte *data, int dataLen);
	int (*ReceivePacket)(socket_t *s, char *buffer, int bufsize, struct recv_data *rec);
	int (*PacketReady)(socket_t *s);
	int usepacketnum;//we can save 4 bytes
	int type; // type of driver (NETPROTO_*). Can be used to make driver-specific rules in other parts of the multiplayer code.
};

extern int NetDrvGeneralPacketReady(int fd);
extern void NetDrvGetLocalTarget( ubyte * server, ubyte * node, ubyte * local_target );
extern int NetDrvSet(int arg);
extern int NetDrvChangeDefaultSocket( ushort socket_number );
extern ubyte * NetDrvGetMyLocalAddress();
extern ubyte * NetDrvGetMyServerAddress();
extern int NetDrvGetPacketData( ubyte * data );
extern void NetDrvSendBroadcastPacketData( ubyte * data, int datasize );
extern void NetDrvSendPacketData( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
extern void NetDrvSendInternetworkPacketData( ubyte * data, int datasize, ubyte * server, ubyte *address );
extern int NetDrvType(void);

#ifndef __APPLE__
extern struct net_driver netdrv_ipx;
#endif
#ifdef __LINUX__
extern struct net_driver netdrv_kali;
#endif
extern struct net_driver netdrv_udp;

extern unsigned char MyAddress[10];
extern ubyte broadcast_addr[];
extern ubyte null_addr[];
extern u_int32_t ipx_network;

#endif /* _NET_DRV_H */
