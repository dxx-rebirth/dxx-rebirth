/* $Id: */

/*
 *
 * IPX driver interface
 *
 * parts from:
 * ipx.h header file for IPX for the DOS emulator
 * 		Tim Bird, tbird@novell.com
 *
 */

#ifndef _IPX_DRV_H
#define _IPX_DRV_H

#define IPX_MANUAL_ADDRESS

#include <winsock.h>

typedef unsigned char ubyte;
typedef unsigned short ushort;
typedef unsigned int uint;

#ifdef __GNUC__
# define __pack__ __attribute__((packed))
#else
# define __pack__
#endif

#define MAX_PACKET_DATA 1500

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

typedef struct IPXAddressStruct {
	u_char Network[4] __pack__;
	u_char Node[6] __pack__;
	u_char Socket[2] __pack__;
} IPXAddress_t;

typedef struct IPXPacketStructure {
	u_short Checksum __pack__;
	u_short Length __pack__;
	u_char TransportControl __pack__;
	u_char PacketType __pack__;
	IPXAddress_t Destination __pack__;
	IPXAddress_t Source __pack__;
} IPXPacket_t;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

typedef struct ipx_socket_struct {
	u_short socket;
	int fd;
} ipx_socket_t;

struct ipx_recv_data {
	/* all network order */
	u_char src_network[4];
	u_char src_node[6];
	u_short src_socket;
	u_short dst_socket;
	int pkt_type;
};

struct ipx_driver {
	int (*GetMyAddress)(void);
	int (*OpenSocket)(ipx_socket_t *sk, int port);
	void (*CloseSocket)(ipx_socket_t *mysock);
	int (*SendPacket)(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
	                  u_char *data, int dataLen);
	int (*ReceivePacket)(ipx_socket_t *s, char *buffer, int bufsize,
	                     struct ipx_recv_data *rec);
	int (*PacketReady)(ipx_socket_t *s);
};

int ipx_general_PacketReady(ipx_socket_t *s);

extern unsigned char ipx_MyAddress[10];

#endif /* _IPX_DRV_H */
