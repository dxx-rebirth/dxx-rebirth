#ifndef _NETPKTOR_H
#define _NETPKTOR_H

#include "byteswap.h"
#include "netdrv.h"
#include "network.h"

//Returns a checksum of a block of memory.
extern ushort netmisc_calc_checksum( void * vptr, int len );

ubyte out_buffer[MAX_DATA_SIZE];		// used for tmp netgame packets as well as sending object data
extern frame_info 	MySyncPack;
void send_d1x_netgame_packet(ubyte *server, ubyte *node);
void receive_d1x_netgame_packet(ubyte *data, netgame_info *netgame);
//end change

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address);
void receive_sequence_packet(ubyte *data, sequence_packet *seq);
void send_netgame_packet(ubyte *server, ubyte *node);
void receive_netgame_packet(ubyte *data, netgame_info *netgame, int d1x);
void swap_object(object *obj);

#ifdef WORDS_BIGENDIAN
void send_frameinfo_packet(frame_info *info, ubyte *server, ubyte *node, ubyte *net_address);
void receive_frameinfo_packet(ubyte *data, frame_info *info);
#else // !WORDS_BIGENDIAN
#define send_frameinfo_packet(info, server, node, net_address) \
	NetDrvSendPacketData((ubyte *)info, sizeof(frame_info) - NET_XDATA_SIZE + (info)->data_size, server, node, net_address)
#define receive_frameinfo_packet(data, info) \
	do { memcpy((ubyte *)(info), data, sizeof(frame_info) - NET_XDATA_SIZE); \
		memcpy((info)->data, &data[sizeof(frame_info) - NET_XDATA_SIZE], (info)->data_size); } while(0)
#endif // WORDS_BIGENDIAN

#endif
