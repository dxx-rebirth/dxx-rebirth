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

/*
 *
 * Header for netmisc.c
 *
 */


#ifndef _NETMISC_H
#define _NETMISC_H

#include "multi.h"
#include "network.h"

// Returns a checksum of a block of memory.
extern ushort netmisc_calc_checksum(void *vptr, int len);

// Finds the difference between block1 and block2.  Fills in
// diff_buffer and returns the size of diff_buffer.
extern int netmisc_find_diff(void *block1, void *block2, int block_size, void *diff_buffer);

// Applies diff_buffer to block1 to create a new block1.  Returns the
// final size of block1.
extern int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size);

#ifdef WORDS_BIGENDIAN

// some mac only routines to deal with incorrectly aligned network structures

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
#define send_internetwork_lite_netgame_packet(server, node, extended) \
	send_netgame_packet(server, node, NULL, !extended)
#define send_broadcast_full_netgame_packet() \
	send_netgame_packet(NULL, NULL, NULL, 0)
#define send_broadcast_lite_netgame_packet() \
	send_netgame_packet(NULL, NULL, NULL, 1)
void receive_netgame_packet(ubyte *data, netgame_info *netgame, int lite_flag);
#define receive_full_netgame_packet(data, netgame) \
	receive_netgame_packet(data, netgame, 0)
#define receive_lite_netgame_packet(data, netgame) \
	receive_netgame_packet(data, netgame, 1)

void send_frameinfo_packet(frame_info *info, ubyte *server, ubyte *node, ubyte *net_address);
void receive_frameinfo_packet(ubyte *data, frame_info *info);

void swap_object(object *obj);

#else

#define receive_netplayers_packet(data, pinfo) \
	memcpy(pinfo, data, sizeof(AllNetPlayers_info))
#define send_netplayers_packet(server, node) \
	NetDrvSendInternetworkPacketData((ubyte *)&NetPlayers, sizeof(AllNetPlayers_info), server, node)
#define send_broadcast_netplayers_packet() \
	NetDrvSendBroadcastPacketData((ubyte *)&NetPlayers, sizeof(AllNetPlayers_info))

#define send_sequence_packet(seq, server, node, net_address) \
	NetDrvSendPacketData((ubyte *)&seq, sizeof(sequence_packet), server, node, net_address)
#define send_internetwork_sequence_packet(seq, server, node) \
	NetDrvSendInternetworkPacketData((ubyte *)&seq, sizeof(sequence_packet), server, node)
#define send_broadcast_sequence_packet(seq) \
	NetDrvSendBroadcastPacketData((ubyte *)&seq, sizeof(sequence_packet))

#define send_full_netgame_packet(server, node, net_address) \
	NetDrvSendPacketData((ubyte *)&Netgame, sizeof(netgame_info), server, node, net_address)
#define send_lite_netgame_packet(server, node, net_address) \
	NetDrvSendPacketData((ubyte *)&Netgame, sizeof(lite_info), server, node, net_address)
#define send_internetwork_full_netgame_packet(server, node) \
	NetDrvSendInternetworkPacketData((ubyte *)&Netgame, sizeof(netgame_info), server, node)
#define send_internetwork_lite_netgame_packet(server, node, extended) \
	NetDrvSendInternetworkPacketData((ubyte *)&Netgame, (extended?sizeof(netgame_info):sizeof(lite_info)), server, node)
#define send_broadcast_full_netgame_packet() \
	NetDrvSendBroadcastPacketData((ubyte *)&Netgame, sizeof(netgame_info))
#define send_broadcast_lite_netgame_packet() \
	NetDrvSendBroadcastPacketData((ubyte *)&Netgame, sizeof(lite_info))
#define receive_full_netgame_packet(data, netgame) \
	memcpy((ubyte *)(netgame), data, sizeof(netgame_info))
#define receive_lite_netgame_packet(data, netgame) \
	memcpy((ubyte *)(netgame), data, sizeof(lite_info))

#define send_frameinfo_packet(info, server, node, net_address) \
	NetDrvSendPacketData((ubyte *)info, sizeof(frame_info) - MaxXDataSize + (info)->data_size, server, node, net_address)
#define receive_frameinfo_packet(data, info) \
	do { memcpy((ubyte *)(info), data, sizeof(frame_info) - MaxXDataSize); \
		memcpy((info)->data, &data[sizeof(frame_info) - MaxXDataSize], (info)->data_size); } while(0)

#define swap_object(obj)

#endif

#endif /* _NETMISC_H */
