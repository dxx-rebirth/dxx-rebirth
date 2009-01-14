/* $Id: udp.h,v 1.2 2005/03/27 01:31:50 stpohle Exp $
 * UDP Network */

#ifndef _UDP_H
#define _UDP_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#ifdef _WIN32
	#include <winsock.h>
	#include <io.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/time.h>
#endif

// #define IPv6 // just here for debugging - set via SConstruct

#ifdef IPv6
	#define _sockaddr sockaddr_in6
	#define _af AF_INET6
	#define _pf PF_INET6
#else
	#define _sockaddr sockaddr_in
	#define _af AF_INET
	#define _pf PF_INET
#endif

#define DXXcfgid "D1Rc" // identification string for UDP/IP configuration packets
#define MAX_CONNECTIONS 32 // maximum connections that can be stored in UDPPeers
#define LEN_SERVERNAME 41
#define LEN_PORT 6
#define UDP_BASEPORT 31017

// CFGFlags
#define CFG_FIRSTCONTACT_REQ	1 // Request to get in contact with host for the first time to show the game
#define CFG_FIRSTCONTACT_ACK	2 // Ack so client proceeds and adds host addr to it's peer list
#define CFG_HANDSHAKE_INIT	3 // Request from Host to Handshake between two clients
#define CFG_HANDSHAKE_ACK	4 // Handshake OK answer
#define CFG_HANDSHAKE_PING	5 // Contact from connected client to a new one
#define CFG_HANDSHAKE_PONG	6 // Answer from new client - Handshake successful

typedef struct peer_list
{
	struct _sockaddr addr; // real address information about this peer
	int valid; // 1 = client connected / 2 = client ready for handshaking / 3 = client done with handshake and fully joined
	fix timestamp; // time of received packet - used for timeout
	char hs_list[MAX_CONNECTIONS]; // list to store all handshake results from clients assigned to this peer
	int hstimeout; // counts the number of tries the client tried to connect - if reached 10, client put to relay if allowed
	int relay; // relay packets by this clients over host
} __pack__ peer_list;

extern sequence_packet My_Seq;
extern netgame_info Active_games[MAX_ACTIVE_NETGAMES];
extern void network_init(void);
extern int network_do_join_game(netgame_info *jgame);
extern int get_and_show_netgame_info(ubyte *server, ubyte *node, ubyte *net_address);
extern int UDPReceivePacket(socket_t *unused, char *text, int len, struct recv_data *rd);
extern int UDPgeneral_PacketReady(socket_t *unused);

#endif
