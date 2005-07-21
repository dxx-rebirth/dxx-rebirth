/* $Id: ipx_mcast4.c,v 1.3 2005-07-21 09:34:22 chris Exp $ */

/*
 *
 * "ipx driver" for IPv4 multicasting
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <ctype.h>

#include "pstypes.h"
#include "ipx_mcast4.h"
#include "args.h"
#include "error.h"
#include "newmenu.h"
#include "../../main/multi.h"

//#define IPX_MCAST4_DEBUG

extern unsigned char ipx_MyAddress[10];

#define UDP_BASEPORT 28342
#define PORTSHIFT_TOLERANCE 0x100
#define MAX_PACKETSIZE 8192

/* OUR port. Can be changed by "@X[+=]..." argument (X is the shift value)
 */
static int baseport=UDP_BASEPORT;

static struct in_addr game_addr;    // The game's multicast address

#define MSGHDR "IPX_mcast4: "

#ifdef IPX_MCAST4_DEBUG
static void msg(const char *fmt, ...)
{
	va_list ap;

	fputs(MSGHDR, stdout);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	putchar('\n');
}
#else
#define msg(m...)
#endif

#define FAIL(m...) do{ nm_messagebox("Error", 1, "Ok", ##m); return -1; } while (0)

#ifdef IPX_MCAST4_DEBUG
/* Dump raw form of IP address/port by fancy output to user
 */
static void dumpraddr(unsigned char *a)
{
	short port;

	printf("[%u.%u.%u.%u]", a[0], a[1], a[2], a[3]);
	port=(signed short)ntohs(*(unsigned short *)(a+4));
	if (port) printf(":%+d",port);
}

/* Like dumpraddr() but for structure "sockaddr_in"
 */
static void dumpaddr(struct sockaddr_in *sin)
{
	unsigned short ports;
	unsigned char qhbuf[8];

	memcpy(qhbuf + 0, &sin->sin_addr, 4);
	ports = htons(((short)ntohs(sin->sin_port)) - UDP_BASEPORT);
	memcpy(qhbuf + 4, &ports, 2);
	dumpraddr(qhbuf);
}
#endif

// The multicast address for Descent 2 game announcements.
// TODO: Pick a better address for this
#define DESCENT2_ANNOUNCE_ADDR inet_addr("239.255.1.2")

/* Open the socket and subscribe to the multicast session */
static int ipx_mcast4_OpenSocket(ipx_socket_t *sk, int port)
{
	u_char loop;
	struct ip_mreq mreq;
	struct sockaddr_in sin;
	int ttl = 128;

	if((sk->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		sk->fd = -1;
		FAIL("socket() creation failed on port %d: %m", port);
	}

	// Bind to the port
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(baseport);
	if (bind(sk->fd, (struct sockaddr *)&sin, sizeof(sin)))
	{
		if (close(sk->fd))
			msg("close() failed during error recovery: %m");
		sk->fd = -1;
		FAIL("bind() to UDP port %d failed: %m", baseport);
	}

	// Set the TTL so the packets can get out of the local network.
	if(setsockopt(sk->fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
		FAIL("setsockopt() failed to set TTL to 128");

	// Disable multicast loopback
	loop = 0;
	if(setsockopt(sk->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
		FAIL("setsockopt() failed to disable multicast loopback: %m");

	// Subscribe to the game announcement address
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = DESCENT2_ANNOUNCE_ADDR;
	mreq.imr_interface.s_addr = INADDR_ANY;
	if(setsockopt(sk->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		FAIL("setsockopt() failed to subscribe to the game announcement multicast group");

	// We're not subscribed to a game address yet
	game_addr.s_addr = 0;

	sk->socket = port;
	return 0;
}

static void ipx_mcast4_CloseSocket(ipx_socket_t *sk)
{
	if(close(sk->fd) < 0)
		msg("Close failed");
	sk->fd = -1;
}

static int ipx_mcast4_SendPacket(ipx_socket_t *sk, IPXPacket_t *IPXHeader, u_char *data, int dataLen)
{
	struct sockaddr_in toaddr;
	int i;

	msg("SendPacket enter, dataLen=%d", dataLen);

	if(dataLen < 0 || dataLen > MAX_PACKETSIZE)
		return -1;

	toaddr.sin_family = AF_INET;
	memcpy(&toaddr.sin_addr, IPXHeader->Destination.Node + 0, 4);
	//toaddr.sin_port = htons(((short)ntohs(*(unsigned short *)(IPXHeader->Destination.Node + 4))) + UDP_BASEPORT);
	// For now, just use the same port for everything
	toaddr.sin_port = htons(UDP_BASEPORT);

	// If it's the broadcast address, then we want to send it to the
	// GAME ANNOUNCEMENT address.
	// Data to be sent to the GAME has the destination already set by
	// ipx_mcast4_SendGamePacket
	if(toaddr.sin_addr.s_addr == INADDR_BROADCAST)
		toaddr.sin_addr.s_addr = DESCENT2_ANNOUNCE_ADDR;

#ifdef IPX_MCAST4_DEBUG
	printf(MSGHDR "sendto((%d),Node=[4] %02X %02X,Socket=%02X %02X,s_port=%u,",
	       dataLen,
	       IPXHeader->Destination.Node[4], IPXHeader->Destination.Node[5],
	       IPXHeader->Destination.Socket[0], IPXHeader->Destination.Socket[1],
	       ntohs(toaddr.sin_port));
	dumpaddr(&toaddr);
	puts(").");
#endif

	i = sendto(sk->fd, data, dataLen, 0, (struct sockaddr *)&toaddr, sizeof(toaddr));
	return i;
}

static int ipx_mcast4_ReceivePacket(ipx_socket_t *sk, char *outbuf, int outbufsize, struct ipx_recv_data *rd)
{
	int size;
	struct sockaddr_in fromaddr;
	uint fromaddrsize = sizeof(fromaddr);

	if((size = recvfrom(sk->fd, outbuf, outbufsize, 0, (struct sockaddr*)&fromaddr, &fromaddrsize)) < 0)
		return -1;

#ifdef IPX_MCAST4_DEBUG
	printf(MSGHDR "Got packet from ");
	dumpaddr(&fromaddr);
	puts("");
#endif

	// We have the packet, now fill out the receive data.
	memset(rd, 0, sizeof(*rd));
	memcpy(rd->src_node, &fromaddr.sin_addr, 4);
	// TODO: Include the port like in ipx_udp.c
	rd->pkt_type = 0;

	return size;
}

/* Handle the netgame aux data
 * Byte 0 is the protocol version number.
 * Bytes 1-4 are the IPv4 multicast session to join, in network byte order.
 */
static int ipx_mcast4_HandleNetgameAuxData(ipx_socket_t *sk, const u_char buf[NETGAME_AUX_SIZE])
{
	// Extract the multicast session and subscribe to it.  We should
	// now be getting packets intended for the players of this game.

	// Note that we stay subscribed to the game announcement session,
	// so we can reply to game info requests
	struct ip_mreq mreq;
	int ttl = 128;

	// Check the protocol version
	if(buf[0] != IPX_MCAST4_VERSION)
	{
		FAIL("mcast4 protocol\nversion mismatch!\nGame version is %02x,\nour version is %02x", buf[0], IPX_MCAST4_VERSION);
	}

	// Get the multicast session
	memcpy(&game_addr, buf + 1, sizeof(game_addr));

#ifdef IPX_MCAST4_DEBUG
	{
		struct sockaddr_in tmpaddr;
		tmpaddr.sin_addr = game_addr;
		tmpaddr.sin_port = 0;

		printf("Handling netgame aux data: Subscribing to ");
		dumpaddr(&tmpaddr);
		puts("");
	}
#endif

	// Set the TTL so the packets can get out of the local network.
	if(setsockopt(sk->fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
		FAIL("setsockopt() failed to set TTL to 128");

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr = game_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;

	if(setsockopt(sk->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		FAIL("setsockopt() failed to subscribe to the game group");

	return 0;
}

/* Create the netgame aux data.
 * Byte 0 is the protcol version number.
 * Bytes 1-4 hold the IPv4 multicast session for the game.
 */
static void ipx_mcast4_InitNetgameAuxData(ipx_socket_t *sk, u_char buf[NETGAME_AUX_SIZE])
{
	Assert(game_addr.s_addr == 0);

	// The first byte is the version number
	buf[0] = IPX_MCAST4_VERSION;

	// Generate a random session
	game_addr = inet_makeaddr(239*256 + 255, d_rand() % 0xFFFF);
	memcpy(buf + 1, &game_addr, sizeof(game_addr));

	// Since we're obviously the hosting machine, subscribe to this address
	ipx_mcast4_HandleNetgameAuxData(sk, buf);
}

static void ipx_mcast4_HandleLeaveGame(ipx_socket_t *sk)
{
	// We left the game, so unsubscribe from its multicast session
	struct ip_mreq mreq;

	Assert(game_addr.s_addr != 0);

#ifdef IPX_MCAST4_DEBUG
	printf("Unsubscribing from game's multicast group: ");
	dumpraddr(&game_addr.s_addr);
	printf("\n");
#endif

	mreq.imr_multiaddr = game_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;
	if(setsockopt(sk->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		msg("setsockopt() failed unsubscribing from previous group!");
	game_addr.s_addr = 0;
}

// Send a packet to every member of the game.  We can just multicast it here.
static int ipx_mcast4_SendGamePacket(ipx_socket_t *sk, ubyte *data, int dataLen)
{
	struct sockaddr_in toaddr;
	int i;

	memset(&toaddr, 0, sizeof(toaddr));
	toaddr.sin_addr = game_addr;
	toaddr.sin_port = htons(UDP_BASEPORT);

	msg("ipx_mcast4_SendGamePacket");

	i = sendto(sk->fd, data, dataLen, 0, (struct sockaddr *)&toaddr, sizeof(toaddr));

	return i;
}

// Pull this in from ipx_udp.c since it's the same for us.
extern int ipx_udp_GetMyAddress();

struct ipx_driver ipx_mcast4 = {
	ipx_udp_GetMyAddress,
	ipx_mcast4_OpenSocket,
	ipx_mcast4_CloseSocket,
	ipx_mcast4_SendPacket,
	ipx_mcast4_ReceivePacket,
	ipx_general_PacketReady,
	ipx_mcast4_InitNetgameAuxData,
	ipx_mcast4_HandleNetgameAuxData,
	ipx_mcast4_HandleLeaveGame,
	ipx_mcast4_SendGamePacket
};
