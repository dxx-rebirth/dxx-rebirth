// UDP/IP driver
// Taken inspiration from bomberclone - Thanks Steffen!

#define UDP_LEN_HOSTNAME 128
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

#include "console.h"
#include "error.h"
#include "netdrv.h"
#include "net_ipx.h"
#include "timer.h"
#include "netdrv_udp.h"
#include "key.h"
#include "text.h"

int UDP_sock = -1;
unsigned int myid=0; // My personal ID which I will get from host and will be used for IPX-Node
struct peer_list UDP_Peers[MAX_CONNECTIONS]; // The Peer list.
void udp_close_socket(socket_t *unused);

// Receive Configuration: Exchanging Peers, doing Handshakes, etc.
void udp_receive_cfg(char *text, struct _sockaddr *sAddr)
{
	switch (text[4])
	{
		case CFG_FIRSTCONTACT_REQ:
		{
			int i, clientid=0;
			ubyte outbuf[6];

			// Check if sAddr is not used already (existing client or if client got this packet)
			for (i = 1; i < MAX_CONNECTIONS; i++)
			{
				if (!memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)))
				{
					clientid=i;
				}
			}

			// If we haven't found a place already...
			if (!clientid)
			{
				for (i = 1; i < MAX_CONNECTIONS; i++) // ... find a free place in list...
				{
					if (!UDP_Peers[i].valid) // ... Found it!
					{
						clientid=i;
						break;
					}
				}
			}

			if (!clientid)
				return;

			UDP_Peers[clientid].valid=1;
			UDP_Peers[clientid].timestamp=timer_get_fixed_seconds();
			memset(UDP_Peers[clientid].hs_list,0,MAX_CONNECTIONS);
			UDP_Peers[clientid].hstimeout=0;
			UDP_Peers[clientid].relay=0;
			memcpy(&UDP_Peers[clientid].addr,sAddr,sizeof(struct _sockaddr)); // Copy address of new client to Peer list
			memcpy(outbuf+0,DXXcfgid,4); // PacketCFG ID
			outbuf[4]=CFG_FIRSTCONTACT_ACK; // CFG Type
			outbuf[5]=clientid; // personal ID for the client
			sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) sAddr, sizeof(struct _sockaddr)); // Send!
			return;
		}

		case CFG_FIRSTCONTACT_ACK:
		{
			My_Seq.player.network.ipx.node[0] = MyAddress[4] = myid = text[5]; // got my ID
			memcpy(&UDP_Peers[0].addr,sAddr,sizeof(struct _sockaddr)); // Add sender -> host!
			UDP_Peers[0].valid=1;
			UDP_Peers[0].timestamp=timer_get_fixed_seconds();
			return;
		}

		// got signal from host to connect with a new client
		case CFG_HANDSHAKE_INIT:
		{
			int i;
			struct _sockaddr tmpAddr;
			ubyte outbuf[6];

			memcpy(outbuf+0,DXXcfgid,4); // PacketCFG ID
			outbuf[4]=CFG_HANDSHAKE_PING; // CFG Type
			outbuf[5]=myid; // my ID that will be assigned to the new client
			memcpy(&tmpAddr,text+5,sizeof(struct _sockaddr));
			for (i=0; i<3; i++)
				sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) &tmpAddr, sizeof(struct _sockaddr)); // send!
			return;
		}

		// Got a Handshake Ping from a connected client -> Add it to peer list and Pong back
		case CFG_HANDSHAKE_PING:
		{
			int i;
			ubyte outbuf[6];

			memcpy(&UDP_Peers[(int)text[5]].addr,sAddr,sizeof(struct _sockaddr)); // Copy address of new client to Peer list
			UDP_Peers[(int)text[5]].valid=1;
			UDP_Peers[(int)text[5]].timestamp=timer_get_fixed_seconds();
			memcpy(outbuf+0,DXXcfgid,4); // PacketCFG ID
			outbuf[4]=CFG_HANDSHAKE_PONG; // CFG Type
			outbuf[5]=myid; // my ID
			for (i=0; i<3; i++)
				sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) sAddr, sizeof(struct _sockaddr)); // send!
			return;
		}

		// Got response from new client -> Send this to host and add new client to peer list
		case CFG_HANDSHAKE_PONG:
		{
			int i;
			ubyte outbuf[7];

			memcpy(&UDP_Peers[(int)text[5]].addr,sAddr,sizeof(struct _sockaddr)); // Copy address of new client to Peer list
			UDP_Peers[(int)text[5]].valid=1;
			UDP_Peers[(int)text[5]].timestamp=timer_get_fixed_seconds();
			memcpy(outbuf+0,DXXcfgid,4); // PacketCFG ID
			outbuf[4]=CFG_HANDSHAKE_ACK; // CFG Type
			outbuf[5]=myid; // my ID
			outbuf[6]=text[5]; // ID of the added client
			for (i=0; i<3; i++)
				sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) &UDP_Peers[0].addr, sizeof(struct _sockaddr)); // send!
			return;
		}

		// Got a message from a client about a new client -> Add hs_list info!
		case CFG_HANDSHAKE_ACK:
		{
			UDP_Peers[(int)text[6]].hs_list[(int)text[5]]=1;
			return;
		}
	}
	return;
}

// Handshaking between clients
// Here the HOST will motivate existing clients to connect to a new one
// If all went good, client can join, if not host will relay this client if GameArg.MplIpRelay or being dumped
int udp_handshake_frame(struct _sockaddr *sAddr, char *inbuf)
{
	int i,checkid=-1;

	if (Network_send_objects)
		return 0;//if we are currently letting someone else join, we don't know if this person can join ok.

	// Find the player we want to Handshake
	for (i=1; i<MAX_CONNECTIONS; i++)
	{
		if (!memcmp(sAddr,(struct sockaddr*)&UDP_Peers[i].addr,sizeof(struct _sockaddr)))
		{
			checkid=i;
			UDP_Peers[checkid].valid=2;
			break;
		}
	}

	if (checkid<0)
		return 0;

	if (UDP_Peers[checkid].relay)
		return 1;

	// send Handshake init to all valid players except the new one (checkid)
	for (i=1; i<MAX_CONNECTIONS; i++)
	{
		if (UDP_Peers[i].valid == 3 && i != checkid  && !UDP_Peers[i].relay && memcmp(sAddr,(struct sockaddr*)&UDP_Peers[i].addr,sizeof(struct _sockaddr)))
		{
			char outbuf[6+sizeof(struct _sockaddr)];

			// send request to connected clients to handshake with new client
			memcpy(outbuf+0,DXXcfgid,4);
			outbuf[4]=CFG_HANDSHAKE_INIT;
			memcpy(outbuf+5,sAddr,sizeof(struct _sockaddr));
			sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) &UDP_Peers[i].addr, sizeof(struct _sockaddr));
		}
	}

	// check if that client is already fully joined
	if (UDP_Peers[checkid].valid == 3)
		return 1;

	// Now check if Handshake was successful on requesting player - if not, return 0
	for (i=1; i<MAX_CONNECTIONS; i++)
	{
		if (UDP_Peers[i].valid == 3 && memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)))
		{
			if (UDP_Peers[checkid].hs_list[i] != 1 && !UDP_Peers[i].relay)
			{
 				if (UDP_Peers[checkid].hstimeout > 10)
 				{
					if (GameArg.MplIpRelay)
					{
						con_printf(CON_NORMAL,"UDP: Relaying Client #%i over Host\n",checkid);
						UDP_Peers[checkid].relay=1;
						memset(UDP_Peers[checkid].hs_list,1,MAX_CONNECTIONS);
						UDP_Peers[checkid].valid=3;
						return 1;
					}
 				}
 				UDP_Peers[checkid].hstimeout++;
				return 0;
			}
		}
	}

	// Set all vals to true since this could be the first client in our list that had no need to Handshake.
	// However in that case this should be true for the next client joning
	memset(UDP_Peers[checkid].hs_list,1,MAX_CONNECTIONS);
	UDP_Peers[checkid].valid=3;

	return 1;
}

// Check if we got data from sAddr within the last 10 seconds (NETWORK_TIMEOUT). If yes, update timestamp of peer, otherwise remove it.
void udp_check_disconnect(struct _sockaddr *sAddr, char *text)
{
	int i;

	// Check all connections for Disconnect or Timeout.
	for (i = 0; i < MAX_CONNECTIONS; i++)
	{
		// Find the player we got the packet from
		if (!memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)))
		{
			// Update timestamp
			UDP_Peers[i].timestamp=timer_get_fixed_seconds();
		}
		else if (UDP_Peers[i].valid && UDP_Peers[i].timestamp + NETWORK_TIMEOUT <= timer_get_fixed_seconds())
		{
			// Timeout!
			UDP_Peers[i].valid=0;
			UDP_Peers[i].timestamp=0;
			memset(UDP_Peers[i].hs_list,0,MAX_CONNECTIONS);
			UDP_Peers[i].hstimeout=0;
			UDP_Peers[i].relay=0;
		}
	}
}

// Relay packets over Host
void udp_packet_relay(char *text, int len, struct _sockaddr *sAddr)
{
	int i, relayid=0;
	
	// Only host will relay
	if (myid)
		return;

	// Never relay PING packets
	if (text[0] == PID_PING_SEND || text[0] == PID_PING_RETURN)
		return;
	
	// We got an PID_PDATA_ACK packet - check if it is for me (host). If not, transfer it to designated client if sender or receiver is relay client.
	if (text[0] == PID_PDATA_ACK)
	{
		if ((int)text[2] != Player_num && (UDP_Peers[NetPlayers.players[(int)text[1]].network.ipx.node[0]].relay || UDP_Peers[NetPlayers.players[(int)text[2]].network.ipx.node[0]].relay))
		{
			sendto (UDP_sock, text, len, 0, (struct sockaddr *) &UDP_Peers[NetPlayers.players[(int)text[2]].network.ipx.node[0]].addr, sizeof(struct _sockaddr));
		}
		return;
	}

	// Check if sender is a relay client and store ID if so
	for (i = 1; i < MAX_CONNECTIONS; i++)
	{
		if (!memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)) && UDP_Peers[i].relay)
		{
			relayid=i;
		}
	}

	if (relayid>0) // Relay packets from relay client to all others
	{
		for (i = 1; i < MAX_CONNECTIONS; i++)
		{
			if (memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)) && i != relayid && UDP_Peers[i].valid>1)
			{
				sendto (UDP_sock, text, len, 0, (struct sockaddr *) &UDP_Peers[i].addr, sizeof(struct _sockaddr));
			}
		}
	}
	else // Relay packets from normal client to relay clients
	{
		for (i = 1; i < MAX_CONNECTIONS; i++)
		{
			if (memcmp(sAddr,(struct _sockaddr *)&UDP_Peers[i].addr,sizeof(struct _sockaddr)) && UDP_Peers[i].relay)
			{
				sendto (UDP_sock, text, len, 0, (struct sockaddr *) &UDP_Peers[i].addr, sizeof(struct _sockaddr));
			}
		}
	}
}

// Resolve address
int udp_dns_filladdr(char *host, int hostlen, int port, int portlen, struct _sockaddr *sAddr)
{
	struct hostent *he;

#ifdef IPv6
	int socktype=AF_INET6;

	he = gethostbyname2 (host,socktype);

	if (!he)
	{
		socktype=AF_INET;
		he = gethostbyname2 (host,socktype);
	}

	if (!he)
	{
		con_printf(CON_URGENT,"udp_dns_filladdr (gethostbyname) failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not resolve address");
		return -1;
	}

	((struct _sockaddr *) sAddr)->sin6_family = socktype; // host byte order
	((struct _sockaddr *) sAddr)->sin6_port = htons (port); // short, network byte order
	((struct _sockaddr *) sAddr)->sin6_flowinfo = 0;
	((struct _sockaddr *) sAddr)->sin6_addr = *((struct in6_addr *) he->h_addr);
	((struct _sockaddr *) sAddr)->sin6_scope_id = 0;
#else
	if ((he = gethostbyname (host)) == NULL) // get the host info
	{
		con_printf(CON_URGENT,"udp_dns_filladdr (gethostbyname) failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not resolve address");
		return -1;
	}

	((struct _sockaddr *) sAddr)->sin_family = _af; // host byte order
	((struct _sockaddr *) sAddr)->sin_port = htons (port); // short, network byte order
	((struct _sockaddr *) sAddr)->sin_addr = *((struct in_addr *) he->h_addr);
	memset (&(((struct _sockaddr *) sAddr)->sin_zero), '\0', 8); // zero the rest of the struct
#endif

	return 0;
}

// Connect to a game host - we want to play!
int UDPConnectManual(char *textaddr)
{
	struct _sockaddr HostAddr;
	fix start_time = 0;
	ubyte node[6];
	char outbuf[12], inbuf[5];
	ubyte null_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	// Resolve address
	if (udp_dns_filladdr(textaddr, LEN_SERVERNAME, UDP_BASEPORT, LEN_PORT, &HostAddr) < 0)
	{
		return -1;
	}

	network_init();
	N_players = 0;
	memset(node,0,6); // Host is always on ID 0
	memset(inbuf,0,5);
	memset(outbuf,0,12);
	start_time = timer_get_fixed_seconds();

	while (!(!memcmp(inbuf+0,DXXcfgid,4) && inbuf[4] == CFG_FIRSTCONTACT_ACK)) // Yay! got ACK by this host!
	{
		// Cancel this with ESC
		if (key_inkey()==KEY_ESC)
			return 0;

		// Timeout after 10 seconds
		if (timer_get_fixed_seconds() >= start_time + (F1_0*10) || timer_get_fixed_seconds() < start_time)
		{
			nm_messagebox(TXT_ERROR,1,TXT_OK,"No response by host");
			return -1;
		}

		// Send request to get added by host...
		memcpy(outbuf+0,DXXcfgid,4);
		outbuf[4]=CFG_FIRSTCONTACT_REQ;
		sendto (UDP_sock, outbuf, sizeof(outbuf), 0, (struct sockaddr *) &HostAddr, sizeof(struct _sockaddr));
		timer_delay(500);
		// ... and wait for answer
		udp_receive_packet(NULL,inbuf,6,NULL);
	}

	
	if (get_and_show_netgame_info(null_addr,node,NULL)) //  show netgame info and keep connection alive!
		return network_do_join_game(0); // join the game actually
	else
		return 0;
}

// Open socket
int udp_open_socket(socket_t *unused, int port)
{
	int i;

#ifdef _WIN32
	struct _sockaddr sAddr;   // my address information
	int reuse_on = -1;

	// close stale socket
	if( UDP_sock != -1 )
		udp_close_socket(NULL);

	memset( &sAddr, '\0', sizeof( sAddr ) );
	
	if ((UDP_sock = socket (_af, SOCK_DGRAM, 0)) == -1) {
		con_printf(CON_URGENT,"udp_open_socket: socket creation failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not create socket");
		return -1;
	}

	// this is pretty annoying in win32. Not doing that will lead to 
	// "Could not bind name to socket" errors. It may be suitable for other
	// socket implementations, too
	(void)setsockopt( UDP_sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_on, sizeof( reuse_on ));

#ifdef IPv6
	sAddr.sin6_family = _pf; // host byte order
	sAddr.sin6_port = htons (GameArg.MplIpBasePort+UDP_BASEPORT);; // short, network byte order
	sAddr.sin6_flowinfo = 0;
	sAddr.sin6_addr = in6addr_any; // automatically fill with my IP
	sAddr.sin6_scope_id = 0;
#else
	sAddr.sin_family = _pf; // host byte order
	sAddr.sin_port = htons (GameArg.MplIpBasePort+UDP_BASEPORT); // short, network byte order
	sAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
#endif
	
	memset (&(sAddr.sin_zero), '\0', 8); // zero the rest of the struct
	
	if (bind (UDP_sock, (struct sockaddr *) &sAddr, sizeof (struct sockaddr)) == -1) 
	{      
		con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not bind name to socket");
		udp_close_socket(NULL);
		return -1;
	}
#else
	struct addrinfo hints,*res,*sres;
	int err,ai_family_;
	char cport[LEN_PORT];
	
	// close stale socket
	if( UDP_sock != -1 )
		udp_close_socket(NULL);
	
	memset (&hints, '\0', sizeof (struct addrinfo));
	memset(cport,'\0',sizeof(char)*LEN_PORT);
	
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = _pf;
	hints.ai_socktype = SOCK_DGRAM;
	
	ai_family_ = 0;

	sprintf(cport,"%i",UDP_BASEPORT+GameArg.MplIpBasePort);

	if ((err = getaddrinfo (NULL, cport, &hints, &res)) == 0)
	{
		sres = res;
		while ((ai_family_ == 0) && (sres))
		{
			if (sres->ai_family == _pf || _pf == PF_UNSPEC)
				ai_family_ = sres->ai_family;
			else
				sres = sres->ai_next;
		}
	
		if (sres == NULL)
			sres = res;
	
		ai_family_ = sres->ai_family;
		if (ai_family_ != _pf && _pf != PF_UNSPEC)
		{
			// ai_family is not identic
			freeaddrinfo (res);
			return -1;
		}
	
		if ((UDP_sock = socket (sres->ai_family, SOCK_DGRAM, 0)) < 0)
		{
			con_printf(CON_URGENT,"udp_open_socket: socket creation failed\n");
			nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not create socket");
			freeaddrinfo (res);
			return -1;
		}
	
		if ((err = bind (UDP_sock, sres->ai_addr, sres->ai_addrlen)) < 0)
		{
			con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed\n");
			nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not bind name to socket");
			udp_close_socket(NULL);
			freeaddrinfo (res);
			return -1;
		}
	
		freeaddrinfo (res);
	}
	else {
		UDP_sock = -1;
		con_printf(CON_URGENT,"udp_open_socket (getaddrinfo):%s\n", gai_strerror (err));
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not get address information:\n%s",gai_strerror (err));
	}
#endif

	// Prepare UDP_Peers
	for (i=0; i<MAX_CONNECTIONS;i++)
	{
		memset(&UDP_Peers[i].addr,0,sizeof(struct _sockaddr));
		UDP_Peers[i].valid=0;
		UDP_Peers[i].timestamp=0;
		memset(UDP_Peers[i].hs_list,0,MAX_CONNECTIONS);
		UDP_Peers[i].hstimeout=0;
		UDP_Peers[i].relay=0;
	}
	myid=0;

	return 0;
};


// Closes an existing udp socket
void udp_close_socket(socket_t *unused)
{
	int i;

	if (UDP_sock != -1)
	{
#ifdef _WIN32
		closesocket(UDP_sock);
#else
		close (UDP_sock);
#endif
	}
	UDP_sock = -1;

	// Prepare UDP_Peers
	for (i=0; i<MAX_CONNECTIONS;i++)
	{
		memset(&UDP_Peers[i].addr,0,sizeof(struct _sockaddr));
		UDP_Peers[i].valid=0;
		UDP_Peers[i].timestamp=0;
		memset(UDP_Peers[i].hs_list,0,MAX_CONNECTIONS);
		UDP_Peers[i].hstimeout=0;
		UDP_Peers[i].relay=0;
	}
	myid=0;
}

// Send text to someone
// This function get's IPXHeader as address. The first byte in this header represents the UDP_Peers ID, so sAddr can be assigned.
static int udp_send_packet(socket_t *unused, IPXPacket_t *IPXHeader, ubyte *text, int len)
{
	// check if Header is in a sane range for UDP_Peers
	if (IPXHeader->Destination.Node[0] >= MAX_CONNECTIONS)
		return 0;

	if (!UDP_Peers[IPXHeader->Destination.Node[0]].valid)
	{
		// PID_PDATA_ACK needs to be delivered to designated peer. If there's a relay player, Host cannot relay, as PID_PDATA_ACK is sent specifically.
		// So in this case, if we encounter invalid ID while PID_PDATA_ACK, wrap this packet to Host so it can be relayed.
		if (text[0] == PID_PDATA_ACK)
			return sendto (UDP_sock, text, len, 0, (struct sockaddr *) &UDP_Peers[0].addr, sizeof(struct _sockaddr));
		else
			return 0;
	}

	return sendto (UDP_sock, text, len, 0, (struct sockaddr *) &UDP_Peers[IPXHeader->Destination.Node[0]].addr, sizeof(struct _sockaddr));
}

// Gets some text
// Returns 0 if nothing on there
// rd can safely be ignored here
int udp_receive_packet(socket_t *unused, char *text, int len, struct recv_data *rd)
{
	unsigned int clen = sizeof (struct _sockaddr), msglen = 0;
	struct _sockaddr sAddr;

	if (UDP_sock == -1)
		return -1;

	if (udp_general_packet_ready(NULL))
	{
		msglen = recvfrom (UDP_sock, text, len, 0, (struct sockaddr *) &sAddr, &clen);

		if (msglen < 0)
			return 0;

		if ((msglen >= 0) && (msglen < len))
			text[msglen] = 0;

		// Wrap UDP CFG packets here!
		if (!memcmp(text+0,DXXcfgid,4))
		{
			udp_receive_cfg(text,&sAddr);
			return 0;
		}

		// Check for Disconnect!
		udp_check_disconnect(&sAddr, text);

		// Seems someone wants to enter the game actually.
		// If I am host, init handshakes! if not sccessful, return 0 and never process this player's request signal - cool thing, eh?
		if (text[0] == PID_REQUEST && myid == 0)
			if (!udp_handshake_frame(&sAddr,text))
				return 0;
				
		udp_packet_relay(text, msglen, &sAddr);
	}

	return msglen;
}

int udp_general_packet_ready(socket_t *unused)
{
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(UDP_sock, &set);
	tv.tv_sec = tv.tv_usec = 0;
	if (select(UDP_sock + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

struct net_driver netdrv_udp =
{
	udp_open_socket,
	udp_close_socket,
	udp_send_packet,
	udp_receive_packet,
	udp_general_packet_ready,
	0, //save 4 bytes.  udp/ip is completely inaccessable by the other methods, so we don't need to worry about compatibility.
	NETPROTO_UDP
};
