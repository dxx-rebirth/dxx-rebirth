/* $Id: ipx_drv.h,v 1.3 2002-08-29 19:02:34 btb Exp $ */

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
#include <sys/types.h>

#define IPX_MANUAL_ADDRESS

#define MAX_PACKET_DATA		1500

typedef struct IPXAddressStruct {
	u_char Network[4] __attribute__((packed));
	u_char Node[6] __attribute__((packed));
	u_char Socket[2] __attribute__((packed));
} IPXAddress_t;

typedef struct IPXPacketStructure {
	u_short Checksum __attribute__((packed));
	u_short Length __attribute__((packed));
	u_char TransportControl __attribute__((packed));
	u_char PacketType __attribute__((packed));
	IPXAddress_t Destination __attribute__((packed));
	IPXAddress_t Source __attribute__((packed));
} IPXPacket_t;

typedef struct ipx_socket_struct {
#ifdef DOSEMU
	struct ipx_socket_struct *next;
	far_t listenList;
	int listenCount;
	far_t AESList;
	int AESCount;
	u_short PSP;
#endif
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
