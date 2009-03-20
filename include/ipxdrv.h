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
	int (*open_socket)(socket_t *sk, int port);
	void (*close_socket)(socket_t *mysock);
	int (*send_packet)(socket_t *mysock, IPXPacket_t *IPXHeader, ubyte *data, int dataLen);
	int (*receive_packet)(socket_t *s, char *buffer, int bufsize, struct recv_data *rec);
	int (*packet_ready)(socket_t *s);
	int usepacketnum;//we can save 4 bytes
	int type; // type of driver (NETPROTO_*). Can be used to make driver-specific rules in other parts of the multiplayer code.
};

extern int ipxdrv_general_packet_ready(int fd);
extern void ipxdrv_get_local_target( ubyte * server, ubyte * node, ubyte * local_target );
extern int ipxdrv_set(int arg);
extern int ipxdrv_change_default_socket( ushort socket_number );
extern ubyte * ipxdrv_get_my_local_address();
extern ubyte * ipxdrv_get_my_server_address();
extern int ipxdrv_get_packet_data( ubyte * data );
extern void ipxdrv_send_broadcast_packet_data( ubyte * data, int datasize );
extern void ipxdrv_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
extern void ipxdrv_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address );
extern int ipxdrv_type(void);

#ifndef __APPLE__
extern struct net_driver ipxdrv_ipx;
#endif
#ifdef __LINUX__
extern struct net_driver ipxdrv_kali;
#endif

extern unsigned char MyAddress[10];
extern ubyte broadcast_addr[];
extern ubyte null_addr[];
extern u_int32_t ipx_network;

#endif /* _NET_DRV_H */
