/* $Id: ipx.c,v 1.7 2004-08-28 23:17:45 schaffner Exp $ */
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
 * Routines for IPX communications, copied from d1x
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef __GNUC__
#define _BORLAND_DOS_REGS 1
#define far
#endif

#include <i86.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <conio.h>
#include <assert.h>

#include "pstypes.h"
#include "timer.h"
#include "ipx.h"
#include "error.h"
#include "u_dpmi.h"
#include "key.h"
#include "ipx_drv.h"

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef struct local_address {
	ubyte address[6];
} __pack__ local_address;

typedef struct net_address {
	BYTE				network_id[4];			
	local_address	node_id;
	WORD				socket_id;
} __pack__ net_address;

typedef struct ipx_header {
	WORD			checksum;
	WORD			length;
	BYTE			transport_control;
	BYTE			packet_type;
	net_address	destination;
	net_address	source;
} __pack__ ipx_header;

typedef struct ecb_header {
	WORD			link[2];
	WORD			esr_address[2];
	BYTE			in_use;
	BYTE			completion_code;
	WORD			socket_id;
	BYTE			ipx_reserved[14];        
	WORD			connection_id;
	local_address immediate_address;
	WORD    		fragment_count;
	WORD			fragment_pointer[2];
	WORD			fragment_size;
} __pack__ ecb_header;

typedef struct packet_data {
	int			packetnum;
	byte			data[IPX_MAX_DATA_SIZE];
} __pack__ packet_data;

typedef struct ipx_packet {
	ecb_header	ecb;
	ipx_header	ipx;
	packet_data	pd;
} __pack__ ipx_packet;

static int ipx_packetnum = 0;

#define MAX_PACKETS 64

static packet_data packet_buffers[MAX_PACKETS];
static short packet_free_list[MAX_PACKETS];
static int num_packets = 0;
static int largest_packet_index = 0;
static short packet_size[MAX_PACKETS];

WORD ipx_socket=0;
static ubyte ipx_installed=0;
WORD ipx_vector_segment;
WORD ipx_vector_offset;
ubyte ipx_socket_life = 0; 	// 0=closed at prog termination, 0xff=closed when requested.
//DWORD ipx_network = 0;
//local_address ipx_my_node;
#define ipx_my_node (ipx_MyAddress+4)
WORD ipx_num_packets=32;		// 32 Ipx packets
ipx_packet * packets;
int neterrors = 0;
ushort ipx_packets_selector;

ecb_header * last_ecb=NULL;
int lastlen=0;

static void got_new_packet( ecb_header * ecb );
static void ipx_listen_for_packet(ecb_header * ecb );

static void free_packet( int id )
{
	packet_buffers[id].packetnum = -1;
	packet_free_list[ --num_packets ] = id;
	if (largest_packet_index==id)	
		while ((--largest_packet_index>0) && (packet_buffers[largest_packet_index].packetnum == -1 ));
}

static int ipx_dos_get_packet_data( ubyte * data )
{
	int i, n, best, best_id, size;

	for (i=1; i<ipx_num_packets; i++ )	{
		if ( !packets[i].ecb.in_use )	{
			got_new_packet( &packets[i].ecb );
			packets[i].ecb.in_use = 0;
			ipx_listen_for_packet(&packets[i].ecb);
		}			
	}

	best = -1;
	n = 0;
	best_id = -1;

	for (i=0; i<=largest_packet_index; i++ )	{
		if ( packet_buffers[i].packetnum > -1 ) {
			n++;
			if ( best == -1 || (packet_buffers[i].packetnum<best) )	{
				best = packet_buffers[i].packetnum;
				best_id = i;
			}
		}			
	}

	//mprintf( (0, "Best id = %d, pn = %d, last_ecb = %x, len=%x, ne = %d\n", best_id, best, last_ecb, lastlen, neterrors ));
	//mprintf( (1, "<%d> ", neterrors ));

	if ( best_id < 0 ) return 0;

	size = packet_size[best_id];
	memcpy( data, packet_buffers[best_id].data, size );
	free_packet(best_id);

	return size;
}

#ifndef __GNUC__
unsigned int swap_short( unsigned int short )
#pragma aux swap_short parm [eax] = "xchg al,ah";
#else
static inline unsigned int swap_short( unsigned int sshort ) {
	int __retval;
	asm("xchg %%ah,%%al" : "=a" (__retval) : "a" (sshort));
	return __retval;
}
#endif

static void got_new_packet( ecb_header * ecb )
{
	ipx_packet * p;
	int id;
	unsigned short datasize;

	datasize = 0;
	last_ecb = ecb;
	p = (ipx_packet *)ecb;

	if ( p->ecb.in_use ) { neterrors++; return; }
	if	( p->ecb.completion_code )	{ neterrors++; return; }

	//	Error( "Recieve error %d for completion code", p->ecb.completion_code );
	
	if ( memcmp( &p->ipx.source.node_id, ipx_my_node, 6 ) )	{
		datasize=swap_short(p->ipx.length);
		lastlen=datasize;
		datasize -= sizeof(ipx_header);
		// Find slot to put packet in...
		if ( datasize > 0 && datasize <= sizeof(packet_data) )	{
			if ( num_packets >= MAX_PACKETS ) {
                                //printf( 1, "IPX: Packet buffer overrun!!!\n" );
				neterrors++;
				return;
			}		
			id = packet_free_list[ num_packets++ ];
			if (id > largest_packet_index ) largest_packet_index = id;
			packet_size[id] = datasize-sizeof(int);
			packet_buffers[id].packetnum =  p->pd.packetnum;
			if ( packet_buffers[id].packetnum < 0 ) { neterrors++; return; }
			memcpy( packet_buffers[id].data, p->pd.data, packet_size[id] );
		} else {
			neterrors++; return;
		}
	} 
	// Repost the ecb
	p->ecb.in_use = 0;
	//ipx_listen_for_packet(&p->ecb);
}

/*ubyte * ipx_get_my_local_address()
{
	return ipx_my_node.address;
}

ubyte * ipx_get_my_server_address()
{
	return (ubyte *)&ipx_network;
}*/

static void ipx_listen_for_packet(ecb_header * ecb )	
{
	dpmi_real_regs rregs;
	ecb->in_use = 0x1d;
	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.ebx = 4;	// Listen For Packet function
	rregs.esi = DPMI_real_offset(ecb);
	rregs.es = DPMI_real_segment(ecb);
	dpmi_real_int386x( 0x7A, &rregs );
}

/*static void ipx_cancel_listen_for_packet(ecb_header * ecb )	
{
	dpmi_real_regs rregs;
	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.ebx = 6;	// IPX Cancel event
	rregs.esi = DPMI_real_offset(ecb);
	rregs.es = DPMI_real_segment(ecb);
	dpmi_real_int386x( 0x7A, &rregs );
}
*/

static void ipx_send_packet(ecb_header * ecb )	
{
	dpmi_real_regs rregs;
	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.ebx = 3;	// Send Packet function
	rregs.esi = DPMI_real_offset(ecb);
	rregs.es = DPMI_real_segment(ecb);
	dpmi_real_int386x( 0x7A, &rregs );
}

typedef struct {
	ubyte 	network[4];
	ubyte		node[6];
	ubyte		local_target[6];
} __pack__ net_xlat_info;

static void ipx_dos_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	net_xlat_info * info;
	dpmi_real_regs rregs;
		
	// Get dos memory for call...
	info = (net_xlat_info *)dpmi_get_temp_low_buffer( sizeof(net_xlat_info) );	
	assert( info != NULL );
	memcpy( info->network, server, 4 );
	memcpy( info->node, node, 6 );
	
	memset(&rregs,0,sizeof(dpmi_real_regs));

	rregs.ebx = 2;		// Get Local Target	
	rregs.es = DPMI_real_segment(info);
	rregs.esi = DPMI_real_offset(info->network);
	rregs.edi = DPMI_real_offset(info->local_target);

	dpmi_real_int386x( 0x7A, &rregs );

	// Save the local target...
	memcpy( local_target, info->local_target, 6 );
}

static void ipx_close()
{
	dpmi_real_regs rregs;
	if ( ipx_installed )	{
		// When using VLM's instead of NETX, the sockets don't
		// seem to automatically get closed, so we must explicitly
		// close them at program termination.
		ipx_installed = 0;
		memset(&rregs,0,sizeof(dpmi_real_regs));
		rregs.edx = ipx_socket;
		rregs.ebx = 1;	// Close socket
		dpmi_real_int386x( 0x7A, &rregs );
	}
}

static int ipx_init(int socket_number)
{
	int show_address=0;
	dpmi_real_regs rregs;
	ubyte *ipx_real_buffer;
	int i;

//	atexit(ipx_close);

	ipx_packetnum = 0;

	// init packet buffers.
	for (i=0; i<MAX_PACKETS; i++ )	{
		packet_buffers[i].packetnum = -1;
		packet_free_list[i] = i;
	}
	num_packets = 0;
	largest_packet_index = 0;

	// Get the IPX vector
	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.eax=0x00007a00;
	dpmi_real_int386x( 0x2f, &rregs );

	if ( (rregs.eax & 0xFF) != 0xFF )	{
		return IPX_NOT_INSTALLED;   
	}
	ipx_vector_offset = rregs.edi & 0xFFFF;
	ipx_vector_segment = rregs.es;
	//printf( "IPX entry point at %.4x:%.4x\n", ipx_vector_segment, ipx_vector_offset );

	// Open a socket for IPX

	memset(&rregs,0,sizeof(dpmi_real_regs));
	swab( (char *)&socket_number,(char *)&ipx_socket, 2 );
	rregs.edx = ipx_socket;
	rregs.eax = ipx_socket_life;
	rregs.ebx = 0;	// Open socket
	dpmi_real_int386x( 0x7A, &rregs );
	
	ipx_socket = rregs.edx & 0xFFFF;
	
	if ( rregs.eax & 0xFF )	{
		//mprintf( (1, "IPX error opening channel %d\n", socket_number-IPX_DEFAULT_SOCKET ));
		return IPX_SOCKET_TABLE_FULL;
	}
	
	ipx_installed = 1;

	// Find our internetwork address
	ipx_real_buffer = dpmi_get_temp_low_buffer( 1024 );	// 1k block
	if ( ipx_real_buffer == NULL )	{
		//printf( "Error allocation realmode memory\n" );
		return IPX_NO_LOW_DOS_MEM;
	}

	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.ebx = 9;		// Get internetwork address
	rregs.esi = DPMI_real_offset(ipx_real_buffer);
	rregs.es = DPMI_real_segment(ipx_real_buffer);
	dpmi_real_int386x( 0x7A, &rregs );

	if ( rregs.eax & 0xFF )	{
		//printf( "Error getting internetwork address!\n" );
		return IPX_SOCKET_TABLE_FULL;
	}

/*	memcpy( &ipx_network, ipx_real_buffer, 4 );
	memcpy( ipx_my_node, &ipx_real_buffer[4], 6 );*/
	memcpy(ipx_MyAddress,ipx_real_buffer,10);

	if ( show_address )	{
		printf( "My IPX addresss is " );
		printf( "%02X%02X%02X%02X/", ipx_real_buffer[0],ipx_real_buffer[1],ipx_real_buffer[2],ipx_real_buffer[3] );
		printf( "%02X%02X%02X%02X%02X%02X\n", ipx_real_buffer[4],ipx_real_buffer[5],ipx_real_buffer[6],ipx_real_buffer[7],ipx_real_buffer[8],ipx_real_buffer[9] );
		printf( "\n" );
	}

	packets = dpmi_real_malloc( sizeof(ipx_packet)*ipx_num_packets, &ipx_packets_selector );
	if ( packets == NULL )	{
		//printf( "Couldn't allocate real memory for %d packets\n", ipx_num_packets );
		return IPX_NO_LOW_DOS_MEM;
	}
#if 0 /* adb: not needed, fails with cwsdpmi */
	if (!dpmi_lock_region( packets, sizeof(ipx_packet)*ipx_num_packets ))	{
		//printf( "Couldn't lock real memory for %d packets\n", ipx_num_packets );
		return IPX_NO_LOW_DOS_MEM;
	}
#endif
	memset( packets, 0, sizeof(ipx_packet)*ipx_num_packets );

	for (i=1; i<ipx_num_packets; i++ )	{
		packets[i].ecb.in_use = 0x1d;
		//packets[i].ecb.in_use = 0;
		packets[i].ecb.socket_id = ipx_socket;
		packets[i].ecb.fragment_count = 1;
		packets[i].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[i].ipx);
		packets[i].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[i].ipx);
		packets[i].ecb.fragment_size = sizeof(ipx_packet)-sizeof(ecb_header);			//-sizeof(ecb_header);

		ipx_listen_for_packet(&packets[i].ecb);
	}

	packets[0].ecb.socket_id = ipx_socket;
	packets[0].ecb.fragment_count = 1;
	packets[0].ecb.fragment_pointer[0] = DPMI_real_offset(&packets[0].ipx);
	packets[0].ecb.fragment_pointer[1] = DPMI_real_segment(&packets[0].ipx);
	packets[0].ipx.packet_type = 4;		// IPX packet
	packets[0].ipx.destination.socket_id = ipx_socket;
//	memcpy( packets[0].ipx.destination.network_id, &ipx_network, 4 );
	memset( packets[0].ipx.destination.network_id, 0, 4 );

	return IPX_INIT_OK;
}

static void ipx_dos_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address )
{
	assert(ipx_installed);

	if ( datasize >= IPX_MAX_DATA_SIZE )	{
		printf( "Data too big\n" );
//added/replaced on 11/8/98 by Victor Rachels to stop crappage
                return;
//                exit(1);
//end this section replacement - VR
	}

	// Make sure no one is already sending something
	while( packets[0].ecb.in_use )
	{
	}
	
	if (packets[0].ecb.completion_code)	{
//                printf( "Send error %d for completion code\n", packets[0].ecb.completion_code );
//added/replaced on 11/8/98 by Victor Rachels to stop crappage
                return;
        //        exit(1);
//end this section replacement - VR

	}

	// Fill in destination address
	if ( memcmp( network, &ipx_network, 4 ) )
		memcpy( packets[0].ipx.destination.network_id, network, 4 );
	else
		memset( packets[0].ipx.destination.network_id, 0, 4 );
	memcpy( packets[0].ipx.destination.node_id.address, address, 6 );
	memcpy( packets[0].ecb.immediate_address.address, immediate_address, 6 );
	packets[0].pd.packetnum = ipx_packetnum++;

	// Fill in data to send
	packets[0].ecb.fragment_size = sizeof(ipx_header) + sizeof(int) + datasize;

	assert( datasize > 1 );
	assert( packets[0].ecb.fragment_size <= 576 );

	memcpy( packets[0].pd.data, data, datasize );

	// Send it
	ipx_send_packet( &packets[0].ecb );

}

/*static int ipx_change_default_socket( ushort socket_number )
{
	int i;
	WORD new_ipx_socket;
	dpmi_real_regs rregs;

	if ( !ipx_installed ) return -3;

	// Open a new socket	
	memset(&rregs,0,sizeof(dpmi_real_regs));
	swab( (char *)&socket_number,(char *)&new_ipx_socket, 2 );
	rregs.edx = new_ipx_socket;
	rregs.eax = ipx_socket_life;
	rregs.ebx = 0;	// Open socket
	dpmi_real_int386x( 0x7A, &rregs );
	
	new_ipx_socket = rregs.edx & 0xFFFF;
	
	if ( rregs.eax & 0xFF )	{
		//printf( (1, "IPX error opening channel %d\n", socket_number-IPX_DEFAULT_SOCKET ));
		return -2;
	}

	for (i=1; i<ipx_num_packets; i++ )	{
		ipx_cancel_listen_for_packet(&packets[i].ecb);
	}

	// Close existing socket...
	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.edx = ipx_socket;
	rregs.ebx = 1;	// Close socket
	dpmi_real_int386x( 0x7A, &rregs );

	ipx_socket = new_ipx_socket;

	// Repost all listen requests on the new socket...	
	for (i=1; i<ipx_num_packets; i++ )	{
		packets[i].ecb.in_use = 0;
		packets[i].ecb.socket_id = ipx_socket;
		ipx_listen_for_packet(&packets[i].ecb);
	}

	packets[0].ecb.socket_id = ipx_socket;
	packets[0].ipx.destination.socket_id = ipx_socket;

	ipx_packetnum = 0;
	// init packet buffers.
	for (i=0; i<MAX_PACKETS; i++ )	{
		packet_buffers[i].packetnum = -1;
		packet_free_list[i] = i;
	}
	num_packets = 0;
	largest_packet_index = 0;

	return 0;
}
*/

struct ipx_driver ipx_dos = {
//	NULL,
	ipx_init,
	ipx_close,
	NULL,
	NULL,
	NULL,
	NULL,
	1,
	ipx_dos_get_local_target,
	ipx_dos_get_packet_data,
	ipx_dos_send_packet_data
};

void arch_ipx_set_driver(int ipx_driver)
{
	driver = &ipx_dos;
	if (ipx_driver != IPX_DRIVER_IPX)
		Warning("Unknown network driver! Defaulting to real IPX");
}


//---typedef struct rip_entry {
//---	uint	 	network;
//---	ushort	nhops;
//---	ushort	nticks;
//---} rip_entry;
//---
//---typedef struct rip_packet {
//---	ushort		operation;		//1=request, 2=response
//---	rip_entry	rip[50];
//---} rip_packet;
//---
//---
//---void  ipx_find_all_servers()
//---{
//---	int i;
//---	rip_packet * rp;
//---	assert(ipx_installed);
//---
//---	ipx_change_default_socket( 0x0453 );
//---	//	ipx_change_default_socket( 0x5304 );
//---
//---	// Make sure no one is already sending something
//---	while( packets[0].ecb.in_use )
//---	{
//---	}
//---	
//---	if (packets[0].ecb.completion_code)	{
//---		printf( "AAAA:Send error %d for completion code\n", packets[0].ecb.completion_code );
//---		//exit(1);
//---	}
//---
//---	rp = (rip_packet *)&packets[0].pd;
//---
//---	// Fill in destination address
//---	{
//---		char mzero1[] = {0,0,0,1};
//---		char mzero[] = {0,0,0,0,0,1};
//---		char immediate[6];
//---		//memcpy( packets[0].ipx.destination.network_id, &ipx_network, 4 );
//---		//memcpy( packets[0].ipx.destination.node_id.address, ipx_my_node.address, 6 );
//---
//---	  	memcpy( packets[0].ipx.destination.network_id, mzero1, 4 );
//---		memcpy( packets[0].ipx.destination.node_id.address, mzero, 6 );
//---
//---		memcpy( packets[0].ipx.destination.socket_id, &ipx_socket, 2 );
//---		memcpy( packets[0].ipx.source.network_id, &ipx_network, 4 );
//---		memcpy( packets[0].ipx.source.node_id.address, ipx_my_node.address, 6 );
//---		memcpy( packets[0].ipx.source.socket_id, &ipx_socket, 2 );
//---		//memcpy( packets[0].ecb.immediate_address.address, ipx_my_node.address, 6 );
//---		//mzero1[3] = 1;
//---		//memcpy( packets[0].ipx.destination.network_id, mzero1, 4 );
//---		//mzero[5] = 1;
//---		//memcpy( packets[0].ipx.destination.node_id.address, mzero, 6 );
//---		//ipx_get_local_target( mzero1, mzero, immediate );
//---		//memcpy( packets[0].ecb.immediate_address.address, mzero, 6 );
//---		//memcpy( packets[0].ecb.immediate_address.address, immediate, 6 );
//---		//mzero[5] = 0;
//---	}
//---
//---	packets[0].ipx.packet_type = 1;		// RIP packet
//---
//---	// Fill in data to send
//---	packets[0].ecb.fragment_size = sizeof(ipx_header) + sizeof(rip_packet);
//---	assert( packets[0].ecb.fragment_size <= 576 );
//---
//---	rp->operation = 0;		// Request
//---	for (i=0;i<50; i++)	{
//---		rp->rip[i].network = 0xFFFFFFFF;
//---		rp->rip[i].nhops = 0;
//---		rp->rip[i].nticks = 0;
//---	}
//---
//---	// Send it
//---	ipx_send_packet( &packets[0].ecb );
//---
//---	for (i=0;i<50; i++)	{
//---		if ( rp->rip[i].network != 0xFFFFFFFF )
//---			printf( "Network = %8x, Hops=%d, Ticks=%d\n", rp->rip[i].network, rp->rip[i].nhops, rp->rip[i].nticks );
//---	}
//---}
//---
//---
