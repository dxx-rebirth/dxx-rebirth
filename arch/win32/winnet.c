/* $Id: winnet.c,v 1.4 2002-08-31 03:16:35 btb Exp $ */
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
 * Win32 net
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock.h>

#include "pstypes.h"
#include "config.h"
#include "args.h"

#include "ipx_drv.h"

extern struct ipx_driver ipx_win;

#define MAX_IPX_DATA 576

int ipx_fd;
ipx_socket_t ipx_socket_data;
ubyte ipx_installed=0;
ushort ipx_socket = 0;
uint ipx_network = 0;
ubyte ipx_MyAddress[10];
int ipx_packetnum = 0;			/* Sequence number */

/* User defined routing stuff */
typedef struct user_address {
	ubyte network[4];
	ubyte node[6];
	ubyte address[6];
} user_address;
#define MAX_USERS 64
int Ipx_num_users = 0;
user_address Ipx_users[MAX_USERS];

#define MAX_NETWORKS 64
int Ipx_num_networks = 0;
uint Ipx_networks[MAX_NETWORKS];

void ipx_close(void);

int ipx_general_PacketReady(ipx_socket_t *s) {
	fd_set set;
	struct timeval tv;
	
	FD_ZERO(&set);
	FD_SET(s->fd, &set);
	tv.tv_sec = tv.tv_usec = 0;
        if (select(FD_SETSIZE, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

struct ipx_driver *driver = &ipx_win;

ubyte * ipx_get_my_server_address()
{
	return (ubyte *)&ipx_network;
}

ubyte * ipx_get_my_local_address()
{
	return (ubyte *)(ipx_MyAddress + 4);
}

//---------------------------------------------------------------
// Initializes all IPX internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//				-1 if socket already open.
//				-2	if socket table full.
//				-3 if IPX not installed.
//				-4 if couldn't allocate low dos memory
//				-5 if error with getting internetwork address
int ipx_init( int socket_number, int show_address )
{
	int i;

        WORD wVersionRequested;
        WSADATA wsaData;

        wVersionRequested = MAKEWORD(2, 0);
        if (WSAStartup( wVersionRequested, &wsaData))
        {
          return -1;
        }

#if 0
        if ( LOBYTE( wsaData.wVersion ) != 2 ||
          HIBYTE( wsaData.wVersion ) != 0 ) {
           /* We couldn't find a usable WinSock DLL. */
           WSACleanup( );
           return -2;
        }
#endif

        printf("Using real IPX for network games\n");
        driver = &ipx_win;
	if ((i = FindArg("-ipxnetwork")) && Args[i + 1]) {
		unsigned long n = strtol(Args[i + 1], NULL, 16);
		ipx_MyAddress[0] = (unsigned char)n >> 24; ipx_MyAddress[1] = (unsigned char)(n >> 16) & 255;
		ipx_MyAddress[2] = (unsigned char)(n >> 8) & 255; ipx_MyAddress[3] = (unsigned char)n & 255;
                printf("IPX: Using network %08x\n", (int) n);
	}
	if (driver->OpenSocket(&ipx_socket_data, socket_number)) {
		return -3;
	}
	driver->GetMyAddress();
	memcpy(&ipx_network, ipx_MyAddress, 4);
	Ipx_num_networks = 0;
	memcpy( &Ipx_networks[Ipx_num_networks++], &ipx_network, 4 );
	ipx_installed = 1;
	atexit(ipx_close);
        printf("ipx succesfully installed\n");
	return 0;
}

void ipx_close()
{
	if (ipx_installed) {
                WSACleanup();
		driver->CloseSocket(&ipx_socket_data);
        }
	ipx_installed = 0;
}

int ipx_get_packet_data( ubyte * data )
{
	struct ipx_recv_data rd;
	char buf[MAX_IPX_DATA];
	uint best_id = 0;
	uint pkt_num;
	int size;
	int best_size = 0;
	
	// Like the original, only take latest packet, throw away rest
	while (driver->PacketReady(&ipx_socket_data)) {
		if ((size = 
		     driver->ReceivePacket(&ipx_socket_data, buf, 
		      sizeof(buf), &rd)) > 4) {
		     if (!memcmp(rd.src_network, ipx_MyAddress, 10)) 
		     	continue;	/* don't get own pkts */
		     pkt_num = *(uint *)buf;
		     if (pkt_num >= best_id) {
		     	memcpy(data, buf + 4, size - 4);
		     	best_id = pkt_num;
		     	best_size = size - 4;
		     }
		}
	}
	return best_size;
}

void ipx_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address )
{
	u_char buf[MAX_IPX_DATA];
	IPXPacket_t ipx_header;
	
	memcpy(ipx_header.Destination.Network, network, 4);
	memcpy(ipx_header.Destination.Node, immediate_address, 6);
	*(u_short *)ipx_header.Destination.Socket = htons(ipx_socket_data.socket);
	ipx_header.PacketType = 4; /* Packet Exchange */
	*(uint *)buf = ipx_packetnum++;
	memcpy(buf + 4, data, datasize);
	driver->SendPacket(&ipx_socket_data, &ipx_header, buf, datasize + 4);
}

void ipx_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	// let's hope Linux knows how to route it
	memcpy( local_target, node, 6 );
}

void ipx_send_broadcast_packet_data( ubyte * data, int datasize )	
{
	int i, j;
	ubyte broadcast[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	ubyte local_address[6];

	// Set to all networks besides mine
	for (i=0; i<Ipx_num_networks; i++ )	{
		if ( memcmp( &Ipx_networks[i], &ipx_network, 4 ) )	{
			ipx_get_local_target( (ubyte *)&Ipx_networks[i], broadcast, local_address );
			ipx_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast, local_address );
		} else {
			ipx_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast, broadcast );
		}
	}

	//OLDipx_send_packet_data( data, datasize, (ubyte *)&ipx_network, broadcast, broadcast );

	// Send directly to all users not on my network or in the network list.
	for (i=0; i<Ipx_num_users; i++ )	{
		if ( memcmp( Ipx_users[i].network, &ipx_network, 4 ) )	{
			for (j=0; j<Ipx_num_networks; j++ )		{
				if (!memcmp( Ipx_users[i].network, &Ipx_networks[j], 4 ))
					goto SkipUser;
			}
			ipx_send_packet_data( data, datasize, Ipx_users[i].network, Ipx_users[i].node, Ipx_users[i].address );
SkipUser:
			j = 0;
		}
	}
}

// Sends a non-localized packet... needs 4 byte server, 6 byte address
void ipx_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address )
{
	ubyte local_address[6];

	if ( (*(uint *)server) != 0 )	{
		ipx_get_local_target( server, address, local_address );
		ipx_send_packet_data( data, datasize, server, address, local_address );
	} else {
		// Old method, no server info.
		ipx_send_packet_data( data, datasize, server, address, address );
	}
}

int ipx_change_default_socket( ushort socket_number )
{
	if ( !ipx_installed ) return -3;

	driver->CloseSocket(&ipx_socket_data);
	if (driver->OpenSocket(&ipx_socket_data, socket_number)) {
		return -3;
	}
	return 0;
}

void ipx_read_user_file(char * filename)
{
	FILE * fp;
	user_address tmp;
	char temp_line[132], *p1;
	int n, ln=0, x;

	if (!filename) return;

	Ipx_num_users = 0;

	fp = fopen( filename, "rt" );
	if ( !fp ) return;

	printf( "Broadcast Users:\n" );

	while (fgets(temp_line, 132, fp)) {
		ln++;
		p1 = strchr(temp_line,'\n'); if (p1) *p1 = '\0';
		p1 = strchr(temp_line,';'); if (p1) *p1 = '\0';
#if 1 // adb: replaced sscanf(..., "%2x...", (char *)...) with better, but longer code
		if (strlen(temp_line) >= 21 && temp_line[8] == '/') {
			for (n = 0; n < 4; n++) {
				if (sscanf(temp_line + n * 2, "%2x", &x) != 1)
					break;
				tmp.network[n] = x;
			}
			if (n != 4)
				continue;
			for (n = 0; n < 6; n++) {
				if (sscanf(temp_line + 9 + n * 2, "%2x", &x) != 1)
					break;
				tmp.node[n] = x;
			}
			if (n != 6)
				continue;
		} else
			continue;
#else
		n = sscanf( temp_line, "%2x%2x%2x%2x/%2x%2x%2x%2x%2x%2x", &tmp.network[0], &tmp.network[1], &tmp.network[2], &tmp.network[3], &tmp.node[0], &tmp.node[1], &tmp.node[2],&tmp.node[3], &tmp.node[4], &tmp.node[5] );
		if ( n != 10 ) continue;
#endif
		if ( Ipx_num_users < MAX_USERS )	{
			ubyte * ipx_real_buffer = (ubyte *)&tmp;
			ipx_get_local_target( tmp.network, tmp.node, tmp.address );
			Ipx_users[Ipx_num_users++] = tmp;
			printf( "%02X%02X%02X%02X/", ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3] );
			printf( "%02X%02X%02X%02X%02X%02X\n", ipx_real_buffer[4],ipx_real_buffer[5],ipx_real_buffer[6],ipx_real_buffer[7],ipx_real_buffer[8],ipx_real_buffer[9] );
		} else {
			printf( "Too many addresses in %s! (Limit of %d)\n", filename, MAX_USERS );
			fclose(fp);
			return;
		}
	}
	fclose(fp);
}


void ipx_read_network_file(char * filename)
{
	FILE * fp;
	user_address tmp;
	char temp_line[132], *p1;
	int i, n, ln=0, x;

	if (!filename) return;

	fp = fopen( filename, "rt" );
	if ( !fp ) return;

	printf( "Using Networks:\n" );
	for (i=0; i<Ipx_num_networks; i++ )		{
		ubyte * n1 = (ubyte *)&Ipx_networks[i];
		printf("* %02x%02x%02x%02x\n", n1[0], n1[1], n1[2], n1[3] );
	}

	while (fgets(temp_line, 132, fp)) {
		ln++;
		p1 = strchr(temp_line,'\n'); if (p1) *p1 = '\0';
		p1 = strchr(temp_line,';'); if (p1) *p1 = '\0';
#if 1 // adb: replaced sscanf(..., "%2x...", (char *)...) with better, but longer code
		if (strlen(temp_line) >= 8) {
			for (n = 0; n < 4; n++) {
				if (sscanf(temp_line + n * 2, "%2x", &x) != 1)
					break;
				tmp.network[n] = x;
			}
			if (n != 4)
				continue;
		} else
			continue;
#else
		n = sscanf( temp_line, "%2x%2x%2x%2x", &tmp.network[0], &tmp.network[1], &tmp.network[2], &tmp.network[3] );
		if ( n != 4 ) continue;
#endif
		if ( Ipx_num_networks < MAX_NETWORKS  )	{
			int j;
			for (j=0; j<Ipx_num_networks; j++ )	
				if ( !memcmp( &Ipx_networks[j], tmp.network, 4 ) )
					break;
			if ( j >= Ipx_num_networks )	{
				memcpy( &Ipx_networks[Ipx_num_networks++], tmp.network, 4 );
				printf("  %02x%02x%02x%02x\n", tmp.network[0], tmp.network[1], tmp.network[2], tmp.network[3] );
			}
		} else {
			printf( "Too many networks in %s! (Limit of %d)\n", filename, MAX_NETWORKS );
			fclose(fp);
			return;
		}
	}
	fclose(fp);
}
