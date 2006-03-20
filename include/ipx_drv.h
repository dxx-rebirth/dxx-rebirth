/* 
 * IPX driver interface
 *
 * parts from:
 * ipx.h header file for IPX for the DOS emulator
 * 		Tim Bird, tbird@novell.com
 */
#ifndef _IPX_DRV_H
#define _IPX_DRV_H
#define IPX_MANUAL_ADDRESS

#include "types.h"

#ifdef __WINDOWS__
#include <winsock.h>
#else
#include <netinet/in.h> /* for htons & co. */
#endif

#define MAX_PACKET_DATA		1500

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

typedef struct IPXAddressStruct {
  ubyte Network[4] __pack__;
  ubyte Node[6] __pack__;
  ubyte Socket[2] __pack__;
} IPXAddress_t;

typedef struct IPXPacketStructure {
  ushort Checksum __pack__;
  ushort Length __pack__;
  ubyte TransportControl __pack__;
  ubyte PacketType __pack__;
  IPXAddress_t Destination __pack__;
  IPXAddress_t Source __pack__;
} IPXPacket_t;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

typedef struct ipx_socket_struct {
  ushort socket;
  int fd;
} ipx_socket_t;

struct ipx_recv_data {
	/* all network order */
	ubyte src_network[4];
	ubyte src_node[6];
	ushort src_socket;
	ushort dst_socket;
	int pkt_type;
};

struct ipx_driver {
//	int (*GetMyAddress)(void);
	int (*OpenSocket)(int port);
	void (*CloseSocket)(void);
	int (*SendPacket)(IPXPacket_t *IPXHeader,
	 ubyte *data, int dataLen);
	int (*ReceivePacket)(char *buffer, int bufsize, 
	 struct ipx_recv_data *rec);
	int (*PacketReady)(void);
	int (*CheckReadyToJoin)(unsigned char *server, unsigned char *node);
	int usepacketnum;//we can save 4 bytes
	void (*GetLocalTarget) ( ubyte * server, ubyte * node, ubyte * local_target );
	int (*GetPacketData) ( ubyte * data );
	void (*SendPacketData) ( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
};

int ipx_general_PacketReady(int fd);

extern unsigned char ipx_MyAddress[10];

extern ubyte broadcast_addr[];
extern ubyte null_addr[];
extern u_int32_t ipx_network;

struct ipx_driver * arch_ipx_set_driver(char *arg);

#ifdef SUPPORTS_NET_IP
extern struct ipx_driver ipx_ip;
#endif

#endif /* _IPX_DRV_H */
