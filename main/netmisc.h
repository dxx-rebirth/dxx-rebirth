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



#ifndef _NETMISC_H
#define _NETMISC_H

#include "multi.h"
#include "network.h"

//Returns a checksum of a block of memory.
extern ushort netmisc_calc_checksum( void * vptr, int len );

//Finds the difference between block1 and block2.  Fills in diff_buffer and returns the size of diff_buffer.
extern int netmisc_find_diff( void *block1, void *block2, int block_size, void *diff_buffer );

//Applies diff_buffer to block1 to create a new block1.  Returns the final size of block1.
extern int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size );

#ifdef WORDS_BIGENDIAN

// some mac only routines to deal with incorrectly aligned network structures

void receive_netplayer_info(ubyte *data, AllNetPlayers_info *info);

void receive_netplayers_packet(ubyte *data, AllNetPlayers_info *pinfo);
void send_netplayers_packet(ubyte *server, ubyte *node);
#define send_broadcast_netplayers_packet() \
	send_netplayers_packet(NULL, NULL)

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address);
#define send_internetwork_sequence_packet(seq, server, node) \
	send_sequence_packet(seq, server, node, NULL)
#define send_broadcast_sequence_packet(seq) \
	send_sequence_packet(seq, NULL, NULL, NULL)
void receive_sequence_packet(ubyte *data, sequence_packet *seq);

void send_netgame_packet(ubyte *server, ubyte *node, ubyte *net_address, int lite_flag);
#define send_full_netgame_packet(server, node, net_address) \
	send_netgame_packet(server, node, net_address, 0)
#define send_lite_netgame_packet(server, node, net_address) \
	send_netgame_packet(server, node, net_address, 1)
#define send_internetwork_full_netgame_packet(server, node) \
	send_netgame_packet(server, node, NULL, 0)
#define send_internetwork_lite_netgame_packet(server, node) \
	send_netgame_packet(server, node, NULL, 1)
#define send_broadcast_full_netgame_packet() \
	send_netgame_packet(NULL, NULL, NULL, 0)
#define send_broadcast_lite_netgame_packet() \
	send_netgame_packet(NULL, NULL, NULL, 1)
void receive_netgame_packet(ubyte *data, netgame_info *netgame, int lite_flag);
#define receive_full_netgame_packet(data, netgame) \
	receive_netgame_packet(data, netgame, 0)
#define receive_lite_netgame_packet(data, netgame) \
	receive_netgame_packet(data, netgame, 1)

void swap_object(object *obj);

#else

#define receive_netplayers_packet(data, pinfo) \
	memcpy(pinfo, data, sizeof(AllNetPlayers_info))
#define send_netplayers_packet(server, node) \
	ipx_send_internetwork_packet_data((ubyte *)&NetPlayers, sizeof(AllNetPlayers_info), server, node)
#define send_broadcast_netplayers_packet() \
	ipx_send_broadcast_packet_data((ubyte *)&NetPlayers, sizeof(AllNetPlayers_info))

#define send_sequence_packet(seq, server, node, net_address) \
	ipx_send_packet_data((ubyte *)&seq, sizeof(sequence_packet), server, node, net_address)
#define send_internetwork_sequence_packet(seq, server, node) \
	ipx_send_internetwork_packet_data((ubyte *)&seq, sizeof(sequence_packet), server, node)
#define send_broadcast_sequence_packet(seq) \
	ipx_send_broadcast_packet_data((ubyte *)&seq, sizeof(sequence_packet))

#define send_full_netgame_packet(server, node, net_address) \
	ipx_send_packet_data((ubyte *)&Netgame, sizeof(netgame_info), server, node, net_address)
#define send_lite_netgame_packet(server, node, net_address) \
	ipx_send_packet_data((ubyte *)&Netgame, sizeof(lite_info), server, node, net_address)
#define send_internetwork_full_netgame_packet(server, node) \
	ipx_send_internetwork_packet_data((ubyte *)&Netgame, sizeof(netgame_info), server, node)
#define send_internetwork_lite_netgame_packet(server, node) \
	ipx_send_internetwork_packet_data((ubyte *)&Netgame, sizeof(lite_info), server, node)
#define send_broadcast_full_netgame_packet() \
	ipx_send_broadcast_packet_data((ubyte *)&Netgame, sizeof(netgame_info))
#define send_broadcast_lite_netgame_packet() \
	ipx_send_broadcast_packet_data((ubyte *)&Netgame, sizeof(lite_info))
#define receive_full_netgame_packet(data, netgame) \
	memcpy((ubyte *)(netgame), data, sizeof(netgame_info))
#define receive_lite_netgame_packet(data, netgame) \
	memcpy((ubyte *)(netgame), data, sizeof(lite_info))

#define swap_object(obj)

#endif

#endif
