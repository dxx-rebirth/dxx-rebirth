/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * IPX-based driver interface
 *
 */

/*
 * This is the main interface to send packets via different protocols
 * It still uses IPX-style address information.
 * Should insure maximum compability to old versions/games and isn't worse than any other way to store addresses in an *universal method*.
 * Mapping actual addresses has to be done by the protocol code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include <WTypes.h>
#else
#include <sys/time.h>
#endif

#include "config.h"
#include "args.h"
#include "text.h"
#include "net_ipx.h"
#include "console.h"
#include "netdrv.h"
#include "checker.h"

ubyte broadcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
ubyte null_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

ubyte netdrv_installed=0;
u_int32_t ipx_network = 0;
ubyte MyAddress[10];
int ipx_packetnum = 0;			/* Sequence number */

/* User defined routing stuff */
typedef struct user_address {
	ubyte network[4];
	ubyte node[6];
	ubyte address[6];
} user_address;

socket_t socket_data;
#define MAX_USERS 64
int Ipx_num_users = 0;
user_address Ipx_users[MAX_USERS];
#define MAX_NETWORKS 64
int Ipx_num_networks = 0;
uint Ipx_networks[MAX_NETWORKS];

int netdrv_general_packet_ready(int fd)
{
	fd_set set;
	struct timeval tv;
	
	FD_ZERO(&set);
	FD_SET(fd, &set);
	tv.tv_sec = tv.tv_usec = 0;
	if (select(fd + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

struct net_driver *driver = NULL;

ubyte * netdrv_get_my_server_address()
{
	return (ubyte *)&ipx_network;
}

ubyte * netdrv_get_my_local_address()
{
	return (ubyte *)(MyAddress + 4);
}

void netdrv_close()
{
	if (netdrv_installed)
	{
#ifdef _WIN32
		WSACleanup();
#endif
		driver->close_socket(&socket_data);
	}

	netdrv_installed = 0;
}

//---------------------------------------------------------------
// Initializes all driver internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//		-1 if socket already open.
//		-2 if socket table full.
//		-3 if driver not installed.
//		-4 if couldn't allocate low dos memory
//		-5 if error with getting internetwork address
int netdrv_init( int socket_number )
{
	static int cleanup = 0;
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
#endif

	if (!driver)
		return -1;

#ifdef _WIN32
	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup( wVersionRequested, &wsaData))
	{
		return -1;
	}
#endif
	memset(MyAddress,0,10);

	if (GameArg.MplIpxNetwork)
	{
		unsigned long n = strtol(GameArg.MplIpxNetwork, NULL, 16);
		MyAddress[0] = n >> 24; MyAddress[1] = (n >> 16) & 255;
		MyAddress[2] = (n >> 8) & 255; MyAddress[3] = n & 255;
		con_printf(CON_DEBUG,"IPX: Using network %08x\n", (unsigned int)n);
	}

	if (driver->open_socket(&socket_data, socket_number))
	{
		return -3;
	}

	memcpy(&ipx_network, MyAddress, 4);
	Ipx_num_networks = 0;
	memcpy( &Ipx_networks[Ipx_num_networks++], &ipx_network, 4 );

	netdrv_installed = 1;

	if (!cleanup)
		atexit(netdrv_close);
	cleanup = 1;

	return 0;
}

int netdrv_set(int arg)
{
	int netdrv_err;

	netdrv_close();

	con_printf(CON_VERBOSE, "\n%s ", TXT_INITIALIZING_NETWORK);

	switch (arg)
	{
#ifndef __APPLE__
		case NETPROTO_IPX:
			driver = &netdrv_ipx;
			break;
#endif
#ifdef __LINUX__
		case NETPROTO_KALINIX:
			driver = &netdrv_kali;
			break;
#endif
		case NETPROTO_UDP:
			driver = &netdrv_udp;
			break;
		default:
			driver = NULL;
			break;
	}

	if ((netdrv_err=netdrv_init(IPX_DEFAULT_SOCKET))==0)
	{
		con_printf(CON_VERBOSE, "%s %d.\n", TXT_IPX_CHANNEL, IPX_DEFAULT_SOCKET );
		Network_active = 1;
	}
	else
	{
		switch (netdrv_err)
		{
			case 3:
				con_printf(CON_VERBOSE, "%s\n", TXT_NO_NETWORK);
				break;
			case -2:
				con_printf(CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET);
				break;
			case -4:
				con_printf(CON_VERBOSE, "%s\n", TXT_MEMORY_IPX );
				break;
			default:
				con_printf(CON_VERBOSE, "%s %d", TXT_ERROR_IPX, netdrv_err );
				break;
		}

		con_printf(CON_VERBOSE, "%s\n",TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}

	return netdrv_installed?0:-1;
}

int netdrv_get_packet_data( ubyte * data )
{
	struct recv_data rd;
	char *buf;
	int size;

	if (driver->usepacketnum)
		buf=alloca(MAX_IPX_DATA);
	else
		buf=(char *)data;

	memset(rd.src_network,1,4);

	while (driver->packet_ready(&socket_data))
	{
		if ((size = driver->receive_packet(&socket_data, buf, MAX_IPX_DATA, &rd)) > 4)
		{
			if (!memcmp(rd.src_network, MyAddress, 10))
			{
				continue;	/* don't get own pkts */
			}

			if (driver->usepacketnum)
			{
				memcpy(data, buf + 4, size - 4);
				return size-4;
			}
			else
			{
				return size;
			}
		}
	}
	return 0;
}

void netdrv_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address )
{
	IPXPacket_t ipx_header;

	if (!netdrv_installed)
		return;

	memcpy(ipx_header.Destination.Network, network, 4);
	memcpy(ipx_header.Destination.Node, immediate_address, 6);
	ipx_header.PacketType = 4; /* Packet Exchange */

	if (driver->usepacketnum)
	{
		ubyte buf[MAX_IPX_DATA];
		*(uint *)buf = ipx_packetnum++;

		memcpy(buf + 4, data, datasize);
		driver->send_packet(&socket_data, &ipx_header, buf, datasize + 4);
	}
	else
		driver->send_packet(&socket_data, &ipx_header, data, datasize);//we can save 4 bytes
}

void netdrv_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	memcpy( local_target, node, 6 );
}

void netdrv_send_broadcast_packet_data( ubyte * data, int datasize )	
{
	int i, j;
	ubyte local_address[6];

	if (!netdrv_installed)
		return;

	// Set to all networks besides mine
	for (i=0; i<Ipx_num_networks; i++ )
	{
		if ( memcmp( &Ipx_networks[i], &ipx_network, 4 ) )
		{
			netdrv_get_local_target( (ubyte *)&Ipx_networks[i], broadcast_addr, local_address );
			netdrv_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast_addr, local_address );
		}
		else
		{
			netdrv_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast_addr, broadcast_addr );
		}
	}

	// Send directly to all users not on my network or in the network list.
	for (i=0; i<Ipx_num_users; i++ )
	{
		if ( memcmp( Ipx_users[i].network, &ipx_network, 4 ) )
		{
			for (j=0; j<Ipx_num_networks; j++ )
			{
				if (!memcmp( Ipx_users[i].network, &Ipx_networks[j], 4 ))
					goto SkipUser;
			}

			netdrv_send_packet_data( data, datasize, Ipx_users[i].network, Ipx_users[i].node, Ipx_users[i].address );
SkipUser:
			j = 0;
		}
	}
}

// Sends a non-localized packet... needs 4 byte server, 6 byte address
void netdrv_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address )
{
	ubyte local_address[6];

	if (!netdrv_installed)
		return;

#ifdef WORDS_NEED_ALIGNMENT
	int zero = 0;
	if (memcmp(server, &zero, 4))
#else // WORDS_NEED_ALIGNMENT
	if ((*(uint *)server) != 0)
#endif // WORDS_NEED_ALIGNMENT
	{
		netdrv_get_local_target( server, address, local_address );
		netdrv_send_packet_data( data, datasize, server, address, local_address );
	}
	else
	{
		// Old method, no server info.
		netdrv_send_packet_data( data, datasize, server, address, address );
	}
}

int netdrv_change_default_socket( ushort socket_number )
{
	if ( !netdrv_installed )
		return -3;

	driver->close_socket(&socket_data);

	if (driver->open_socket(&socket_data, socket_number))
	{
		return -3;
	}

	return 0;
}

// Return type of net_driver
int netdrv_type(void)
{
	return driver->type;
}

