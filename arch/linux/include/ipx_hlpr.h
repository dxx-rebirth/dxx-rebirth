/*
 * $Source: /cvs/cvsroot/d2x/arch/linux/include/ipx_hlpr.h,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 07:29:37 $
 *
 * FIXME: add description
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifndef IPXHELPER_H_
#define IPXHELPER_H_
#include <sys/types.h>
#include "ipx_ld.h"

#define IPX_MANUAL_ADDRESS

struct ipx_recv_data {
	/* all network order */
	u_char src_network[4];
	u_char src_node[6];
	u_short src_socket;
	u_short dst_socket;
	int pkt_type;
};

struct ipx_helper {
	int (*GetMyAddress)(void);
	int (*OpenSocket)(ipx_socket_t *sk, int port);
	void (*CloseSocket)(ipx_socket_t *mysock);
	int (*SendPacket)(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
	 u_char *data, int dataLen);
	int (*ReceivePacket)(ipx_socket_t *s, char *buffer, int bufsize, 
	 struct ipx_recv_data *rec);
	int (*PacketReady)(ipx_socket_t *s);
};

#endif /* IPXHELPER_H_ */
