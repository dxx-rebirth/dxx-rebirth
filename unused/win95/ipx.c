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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#pragma off (unreferenced)
static char rcsid[] = "$Id: ipx.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <wsipx.h>


#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <conio.h>


#include "pstypes.h"
#include "winapp.h"
#include "ipx.h"
#include "error.h"
#include "mono.h"



//	---------------------------------------------------------------------------

#define NUM_IPX_PACKETS	32

#define MAX_USERS		64
#define MAX_SUBNETS	64
#define MAX_PACKETS	64


typedef struct tIpxNetworkID {
	BYTE b[4];
} tIpxNetworkID;  

typedef struct tIpxAddrGeneric { 
	BYTE b[6]; 
} tIpxAddrGeneric;

typedef struct tIpxNetAddr {
	tIpxNetworkID net_id;
	tIpxAddrGeneric node_addr;
	WORD ipx_socket;
} tIpxNetAddr;

typedef struct tIpxUserAddr {
	tIpxNetworkID net_id;
	tIpxAddrGeneric node_addr;
	tIpxAddrGeneric addr;
} tIpxUserAddr;

typedef struct tIpxPacket {
	tIpxNetAddr saddr;
	tIpxNetAddr daddr;
	int in_use;
	int datalen;
	int packetnum;
	ubyte data[IPX_MAX_DATA_SIZE];
} tIpxPacket;


typedef struct tPacket {
	int free;
	int size;
	int packetnum;
	ubyte data[IPX_MAX_DATA_SIZE];
} tPacket;



//	---------------------------------------------------------------------------
//	Data
// ---------------------------------------------------------------------------

static ubyte 					ipx_installed = 0;
static int						neterrors = 0;

static int		 				NumIPXUsers = 0;
static tIpxUserAddr			IPXUsers[MAX_USERS];

static int						NumIPXSubnet = 0;
static tIpxNetworkID 		IPXSubnetID[MAX_SUBNETS];

static tIpxPacket				*IPXPackets = NULL;
static int		 				IPXPacketNum = 0;

static SOCKET					WinSocket = 0;
static WORD						IPXSocket = 0;
static tIpxNetworkID			ThisIPXSubnet;
static tIpxAddrGeneric		ThisIPXNode;

static int						PacketListTail, PacketListCur;
static tPacket					*PacketList;



//	---------------------------------------------------------------------------
//	Function Prototypes
//	---------------------------------------------------------------------------

uint wipx_init();
uint wipx_open_socket(int sock_num, SOCKET *sock_id, WORD *ipx_socket);
uint wipx_close_socket(SOCKET sock_id);
uint wipx_get_socket_addr(SOCKET sock_id, tIpxNetworkID *net_addr, tIpxAddrGeneric *node_addr);
uint wipx_set_socket_mode_bool(SOCKET sock_id, int opt, BOOL toggle);
uint wipx_socket_listen(SOCKET sock_id, tIpxPacket *packet);
uint wipx_socket_send(SOCKET sock_id, tIpxPacket *packet);
uint wipx_close(); 
uint wipx_logerror(uint ipx_result, char *text);

void got_new_packet( tIpxPacket *packet );
void free_packet( int id );
void ipx_close();

ubyte *ipx_get_my_local_address() {
	return (ubyte *)&ThisIPXNode;
}

ubyte *ipx_get_my_server_address() {
  return (ubyte *)&ThisIPXSubnet;
}

void ipx_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	memset(local_target, 0, 6);
}



//	---------------------------------------------------------------------------
//	Functions
//	---------------------------------------------------------------------------	

//---------------------------------------------------------------
// Initializes all IPX internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//				-1 if socket already open.
//				-2	if socket table full.
//				-3 if IPX not installed.
//				-4 if couldn't allocate memory
//				-5 if error with getting internetwork address

int ipx_init(int socket_number, int show_address)
{
	int i;
	uint ipx_result;

	atexit(ipx_close);

//	Establish Winsock IPX Link.
	ipx_result = wipx_init();
	if (ipx_result) return -3;

	ipx_result = wipx_open_socket(socket_number, &WinSocket, &IPXSocket);
	if (ipx_result) {
		if (ipx_result == WSAEPROTONOSUPPORT) return -3;
		if (ipx_result == WSAENOBUFS) return -2;
		return -1;
	}

//	Get this station's IPX address.
	ipx_result = wipx_get_socket_addr(WinSocket, &ThisIPXSubnet, &ThisIPXNode);
	if (ipx_result) return -5;
	NumIPXSubnet = 0;
	memcpy(&IPXSubnetID[NumIPXSubnet++], &ThisIPXSubnet, sizeof(tIpxNetworkID));	

//	Initialize packet buffers
	PacketList = (tPacket *)GlobalAllocPtr(GPTR, MAX_PACKETS*sizeof(tPacket));
	if (!PacketList) 
		return -4;	  							// Memory will be freed in ipx_close

	for (i = 0; i < MAX_PACKETS; i++) 
	{
		PacketList[i].packetnum = -1;
		PacketList[i].free = i;
	}

	IPXPacketNum = 0;
	PacketListCur = 0;
	PacketListTail = 0;

//	Setup IPX packets for packet retrieval and sending
	IPXPackets = (tIpxPacket *)GlobalAllocPtr(GPTR, sizeof(tIpxPacket)*NUM_IPX_PACKETS);
	if (!IPXPackets) 
		return -4;	  							// Memory will be freed in ipx_close

	for (i = 1; i < NUM_IPX_PACKETS; i++)
	{
		IPXPackets[i].in_use = 1;
		wipx_socket_listen(WinSocket, &IPXPackets[i]);
	}

	IPXPackets[0].daddr.ipx_socket = htons(IPXSocket);
	memset( &IPXPackets[0].daddr.net_id, 0, sizeof(tIpxNetworkID));

	ipx_installed = 1;

	return 0;
}	


void ipx_close()
{
	if (WinSocket) {
		wipx_close_socket(WinSocket);
		wipx_close();
	}
	if (IPXPackets) {
		GlobalFreePtr(IPXPackets);
		IPXPackets = NULL;
	}
	if (PacketList) {
		GlobalFreePtr(PacketList);
		PacketList = NULL;
	}
}


//	----------------------------------------------------------------------------
//	Listen/Retrieve Packet Functions
//	----------------------------------------------------------------------------

int ipx_get_packet_data( ubyte * data )
{
	int i, n, best, best_id, size;

	for (i=1; i < NUM_IPX_PACKETS; i++ )	
	{
		IPXPackets[i].in_use = 1;
		wipx_socket_listen(WinSocket, &IPXPackets[i]);
		if (!IPXPackets[i].in_use) 
			got_new_packet(&IPXPackets[i]); 			
	}

//	Take oldest packet from list and get data.
	best = -1;
	n = 0;
	best_id = -1;

	for (i=0; i <= PacketListTail; i++ )	
	{
		if ( PacketList[i].packetnum > -1 ) {
			n++;
			if ((best == -1) || (PacketList[i].packetnum < best) )	{
				best = PacketList[i].packetnum;
				best_id = i;
			}
		}			
	}

	if ( best_id < 0 ) return 0;

	size = PacketList[best_id].size;
	memcpy( data, PacketList[best_id].data, size );

	free_packet(best_id);							

	return size;
}


void got_new_packet( tIpxPacket *packet )
{
	int id;
	unsigned short datasize;

	datasize = 0;

	if (memcmp( &packet->saddr.node_addr, &ThisIPXNode, sizeof(tIpxAddrGeneric))) {
	// Find slot to put packet in...
		datasize = packet->datalen;	  

		if ( datasize > 0 && datasize <= sizeof(tPacket) )	{
			if ( PacketListCur >= MAX_PACKETS ) {
				neterrors++;
				return;
			}		
			id = PacketList[PacketListCur++].free;
			if (id > PacketListTail ) PacketListTail = id;
			PacketList[id].size = datasize - sizeof(int);
			PacketList[id].packetnum = packet->packetnum;
			if (PacketList[id].packetnum < 0) { neterrors++; return; } 
			memcpy( PacketList[id].data, packet->data, PacketList[id].size );
		} else {
			neterrors++; return;
		}
	} 
	// Repost the ecb
	packet->in_use = 0;
}


void free_packet( int id )
{
	PacketList[id].packetnum = -1;
	PacketList[ --PacketListCur].free = id;
	if (PacketListTail==id)	
		while ((--PacketListTail>0) && (PacketList[PacketListTail].packetnum == -1 ));
}



//	----------------------------------------------------------------------------
//	Send IPX Packet Functions
//	----------------------------------------------------------------------------

void ipx_send_packet_data( ubyte * data, 
				int datasize, 
				ubyte *network, 
				ubyte *address, 
				ubyte *immediate_address )
{
	Assert(ipx_installed);

	if ( datasize >= IPX_MAX_DATA_SIZE )	{
		Error("Illegal sized IPX packet being sent.");		
	}

// Make sure no one is already sending something
	IPXPackets[0].packetnum = IPXPacketNum;
	IPXPacketNum++;	
	memcpy( &IPXPackets[0].daddr.net_id, (tIpxNetworkID *)network, 4);
	memcpy( &IPXPackets[0].daddr.node_addr, (tIpxAddrGeneric *)address, sizeof(tIpxAddrGeneric) );

// Fill in data to send
	Assert(datasize > 1);
	IPXPackets[0].datalen = sizeof(int) + datasize;
	memcpy( IPXPackets[0].data, data, datasize );

// Send it
	IPXPackets[0].daddr.ipx_socket = htons(IPXSocket);
	wipx_socket_send( WinSocket, &IPXPackets[0]);
}


void ipx_send_broadcast_packet_data( ubyte * data, int datasize )	
{
	int i, j;
	ubyte broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	tIpxAddrGeneric local_address;

	wipx_set_socket_mode_bool(WinSocket, SO_BROADCAST, TRUE);

// Set to all networks besides mine
	for (i = 0; i < NumIPXSubnet; i++ )	
	{
		if (memcmp(&IPXSubnetID[i], &ThisIPXSubnet, 4)) {
			ipx_get_local_target( (ubyte *)&IPXSubnetID[i], 
						broadcast, 
						(ubyte*)&local_address );
			ipx_send_packet_data( data, datasize, 
						(ubyte *)&IPXSubnetID[i], 
						broadcast, 
						(ubyte *)&local_address );
		} else {
			ipx_send_packet_data( data, datasize, 
						(ubyte *)&IPXSubnetID[i], 
						broadcast, 
						broadcast );
		}
	}
							 
// Send directly to all users not on my network or in the network list.
	for (i = 0; i < NumIPXUsers; i++ )	
	{
		if ( memcmp( &IPXUsers[i].net_id, &ThisIPXSubnet,4 ) )	{
			for (j=0; j < NumIPXSubnet; j++ )		{
				if (!memcmp( &IPXUsers[i].net_id, &IPXSubnetID[j], 4 ))
					goto SkipUser;
			}
			ipx_send_packet_data( data, datasize, 
						(ubyte*)&IPXUsers[i].net_id, 
						(ubyte*)&IPXUsers[i].node_addr, 
						(ubyte*)&IPXUsers[i].addr );
SkipUser:
			j = 0;
		}
	}
}


//	Functions Sends a non-localized packet... needs 4 byte server, 
//	6 byte address

void ipx_send_internetwork_packet_data( ubyte * data, int datasize, 
						ubyte * server, 
						ubyte *address )
{
	tIpxAddrGeneric local_address;

	if ( (*(uint *)server) != 0 )	{
		ipx_get_local_target( server, address, (ubyte *)&local_address );
		ipx_send_packet_data( data, datasize, server, address, (ubyte *)&local_address );
	} else {
	// Old method, no server info.
		ipx_send_packet_data( data, datasize, server, address, address );
	}
}



//	---------------------------------------------------------------------------
// Read IPX User file stuff
//	---------------------------------------------------------------------------

int ipx_change_default_socket( ushort socket_number )
{
	int i;
	int result;
	SOCKET new_socket;
	WORD new_ipx_socket;

	if ( !ipx_installed ) return -3;

// Open a new socket	
	result = wipx_open_socket(socket_number, &new_socket, &new_ipx_socket);
	if (result) return -2;
	

// Close existing socket...
	wipx_close_socket(WinSocket);

	IPXSocket = new_ipx_socket;
	WinSocket = new_socket;

// Repost all listen requests on the new socket...	
	for (i=1; i<NUM_IPX_PACKETS; i++ )	
	{
		IPXPackets[i].in_use = 0;
		wipx_socket_listen(WinSocket, &IPXPackets[i]);
	}

	IPXPackets[0].daddr.ipx_socket = htons(IPXSocket);
	IPXPacketNum = 0;

// init packet buffers.
	for (i=0; i<MAX_PACKETS; i++ )	
	{
		PacketList[i].packetnum = -1;
		PacketList[i].free = i;
	}
	PacketListCur = PacketListTail = 0;

	return 0;
}


void ipx_read_user_file(char * filename)
{
	FILE * fp;
	tIpxUserAddr tmp;
	char temp_line[132], *p1;
	int n, ln=0;

	if (!filename) return;

	NumIPXUsers = 0;

	fp = fopen( filename, "rt" );
	if ( !fp ) return;

	printf( "Broadcast Users:\n" );

	while (fgets(temp_line, 132, fp)) {
		ln++;
		p1 = strchr(temp_line,'\n'); if (p1) *p1 = '\0';
		p1 = strchr(temp_line,';'); if (p1) *p1 = '\0';
		n = sscanf( temp_line, "%2x%2x%2x%2x/%2x%2x%2x%2x%2x%2x", &tmp.net_id.b[0], 
					&tmp.net_id.b[1], &tmp.net_id.b[2], &tmp.net_id.b[3],
					&tmp.node_addr.b[0], &tmp.node_addr.b[1], &tmp.node_addr.b[2],
					&tmp.node_addr.b[3], &tmp.node_addr.b[4], &tmp.node_addr.b[5] );
		if ( n != 10 ) continue;
		if ( NumIPXUsers < MAX_USERS )	{
			ipx_get_local_target( tmp.net_id.b, tmp.node_addr.b, tmp.addr.b );
			IPXUsers[NumIPXUsers++] = tmp;

//			printf( "%02X%02X%02X%02X/", ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3] );
//			printf( "%02X%02X%02X%02X%02X%02X\n", ipx_real_buffer[4],ipx_real_buffer[5],ipx_real_buffer[6],ipx_real_buffer[7],ipx_real_buffer[8],ipx_real_buffer[9] );
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
	tIpxUserAddr tmp;
	char temp_line[132], *p1;
	int i, n, ln=0;

	if (!filename) return;

	fp = fopen( filename, "rt" );
	if ( !fp ) return;

	printf( "Using Networks:\n" );
	for (i=0; i<NumIPXSubnet; i++ )		{
		ubyte * n1 = (ubyte *)IPXSubnetID[i].b;
		printf("* %02x%02x%02x%02x\n", n1[0], n1[1], n1[2], n1[3] );
	}

	while (fgets(temp_line, 132, fp)) {
		ln++;
		p1 = strchr(temp_line,'\n'); if (p1) *p1 = '\0';
		p1 = strchr(temp_line,';'); if (p1) *p1 = '\0';
		n = sscanf( temp_line, "%2x%2x%2x%2x", &tmp.net_id.b[0], &tmp.net_id.b[1], &tmp.net_id.b[2], &tmp.net_id.b[3] );
		if ( n != 4 ) continue;
		if ( NumIPXSubnet < MAX_SUBNETS  )	{
			int j;
			for (j=0; j<NumIPXSubnet; j++ )	
				if ( !memcmp( &IPXSubnetID[j], &tmp.net_id, 4 ) )
					break;
			if ( j >= NumIPXSubnet )	{
				memcpy( &IPXSubnetID[NumIPXSubnet++], &tmp.net_id, 4 );
//				printf("  %02x%02x%02x%02x\n", &tmp.net_id.b[0], &tmp.net_id.b[1], tmp.network[2], tmp.network[3] );
			}
		} else {
			printf( "Too many networks in %s! (Limit of %d)\n", filename, MAX_SUBNETS );
			fclose(fp);
			return;
		}
	}
	fclose(fp);

}



//	----------------------------------------------------------------------------
//	Winsock API layer
//	----------------------------------------------------------------------------

uint wipx_init()
{
	WSADATA version_info;
	WORD version_requested;
	int result;
	
	version_requested = MAKEWORD(2,0);
	
	result = WSAStartup(version_requested, &version_info);
	if (result) {
		wipx_logerror(result, "wpx_init");	
		return result;
	}
		
	if (LOBYTE(version_requested) < 1 && HIBYTE(version_requested) < 1) {
		logentry("Bad version of Winsock DLL %d.%d.\n", LOBYTE(version_requested), HIBYTE(version_requested));
		return 0xffffffff;
	}

	return 0;
}


uint wipx_open_socket(int sock_num, SOCKET *sock_id, WORD *ipx_socket)
{
	LINGER ling;
	SOCKET s;
	SOCKADDR_IPX ipx_addr;
	unsigned long ioctlval;

	s = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
	if (s == INVALID_SOCKET) {
		*sock_id = 0;
		*ipx_socket = 0;
		wipx_logerror(s, "wipx_open_socket::socket");
		return INVALID_SOCKET;
	}

//	Set hard close for sockets
	ling.l_onoff = 1;
	ling.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (PSTR)&ling, sizeof(ling));
	
	memset(&ipx_addr, 0, sizeof(SOCKADDR_IPX));
	ipx_addr.sa_socket = htons(sock_num);
	ipx_addr.sa_family = AF_IPX;
	if (bind(s, (SOCKADDR *)&ipx_addr, sizeof(SOCKADDR_IPX)) == SOCKET_ERROR) {
		closesocket(s);
		*ipx_socket = 0;
		*sock_id = 0;
		wipx_logerror(SOCKET_ERROR, "wipx_open_socket::bind");
		return SOCKET_ERROR;
	}
	
	ioctlval = 1;								// Set socket to non-blocking mode
	if (ioctlsocket(s, FIONBIO, &ioctlval) == SOCKET_ERROR) {
		closesocket(s);
		*ipx_socket = 0;
		*sock_id = 0;
		wipx_logerror(SOCKET_ERROR, "wipx_open_socket::ioctlsocket");
		return SOCKET_ERROR;
	}
	
	*sock_id = s;
	*ipx_socket = sock_num;

	return 0;
}


uint wipx_close_socket(SOCKET sock_id)
{
	if (closesocket(sock_id) == SOCKET_ERROR) {
		wipx_logerror(SOCKET_ERROR, "wipx_close_socket");
		return SOCKET_ERROR;
	}
	else return 0;
}


uint wipx_get_socket_addr(SOCKET sock_id, tIpxNetworkID *net_addr, 
				tIpxAddrGeneric *node_addr)
{
	SOCKADDR_IPX 	ipx_addr;
	int 				len;

//	Use getsockname to get info
	len = sizeof(SOCKADDR_IPX);
	if (getsockname(sock_id, (SOCKADDR *)&ipx_addr, &len) == SOCKET_ERROR) {
		wipx_logerror(SOCKET_ERROR, "getsockname");
		return SOCKET_ERROR;
	}
	else {
		memcpy(net_addr, ipx_addr.sa_netnum, 4);
		memcpy(node_addr, ipx_addr.sa_nodenum, 6);
		return 0;
	}
}


uint wipx_set_socket_mode_bool(SOCKET sock_id, int opt, BOOL toggle)
{
	if (setsockopt(sock_id, SOL_SOCKET, opt, (LPSTR)&toggle,
				 sizeof(toggle)) == SOCKET_ERROR) {
		wipx_logerror(SOCKET_ERROR, "wipx_set_socket_mode_bool");
		return SOCKET_ERROR;
	}
	return 0;
}


uint wipx_socket_listen(SOCKET sock_id, tIpxPacket *packet)
{
	unsigned long ioctlval = 0;
	int bytes, length = sizeof(SOCKADDR_IPX);
	SOCKADDR_IPX ipx_addr;
	
	if (ioctlsocket(sock_id, FIONREAD, &ioctlval) == SOCKET_ERROR)	{
		wipx_logerror(SOCKET_ERROR, "wipx_socket_listen::ioctlsocket");
		return SOCKET_ERROR;
	}
	else if (ioctlval) {
		ipx_addr.sa_socket = htons(IPXSocket);
		ipx_addr.sa_family = AF_IPX;
		bytes = recvfrom(sock_id, (char *)(&packet->packetnum),
					sizeof(int) + IPX_MAX_DATA_SIZE,
					0,
					&ipx_addr,
					&length);
		if (bytes == SOCKET_ERROR) {
			wipx_logerror(SOCKET_ERROR, "wipx_socket_listen::recvfrom");
			return SOCKET_ERROR;
		}
		packet->in_use = 0;
		packet->datalen = bytes;
		return 0;
	}
	else return 0;
}


uint wipx_socket_send(SOCKET sock_id, tIpxPacket *packet)
{
	SOCKADDR_IPX ipx_addr;
	int bytes = 0;

	ipx_addr.sa_socket = htons(IPXSocket);
	ipx_addr.sa_family = AF_IPX;
	memcpy(ipx_addr.sa_nodenum, &packet->daddr.node_addr, 6);
	memcpy(ipx_addr.sa_netnum, &packet->daddr.net_id, 4);
	
/*	logentry("Sending packet to %2X%2X%2X%2X:%2X%2X%2X%2X%2X%2X\n",
				packet->daddr.net_id.b[0],
				packet->daddr.net_id.b[1],
				packet->daddr.net_id.b[2],
				packet->daddr.net_id.b[3],
				packet->daddr.node_addr.b[0],
				packet->daddr.node_addr.b[1],
				packet->daddr.node_addr.b[2],
				packet->daddr.node_addr.b[3],
				packet->daddr.node_addr.b[4],
				packet->daddr.node_addr.b[5]);
*/

	bytes = sendto(sock_id, (char *)(&packet->packetnum),
				packet->datalen,
				0,
				&ipx_addr,
				sizeof(SOCKADDR_IPX));		
	packet->in_use = 1;
	if (bytes == SOCKET_ERROR) {
		wipx_logerror(SOCKET_ERROR, "wipx_socket_send::sendto");
		return SOCKET_ERROR;
	}
	else return 0;
}


uint wipx_close()
{
	return WSACleanup();
}

 
uint wipx_logerror(uint ipx_result, char *text)
{
	logentry("%s:: error %x.\n", text, ipx_result);
	return ipx_result;
}	

