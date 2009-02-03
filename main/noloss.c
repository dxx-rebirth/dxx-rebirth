/*
 * Packet-loss-prevention code for DXX-Rebirth 
 * This code will save the data field of a netgame packet to a queue.
 * Saving is only done for urgent packets.
 * Each PDATA packet will be ACK'd by the receiver.
 * If all receivers submitted their ACK packets to initial sender, the packet is removed from the queue.
 * If a player has not ACK'd a PDATA packet within a second, it will resend to him. 
 * Timeout, disconnects, or changes in player count are handled as well.
 */

#include "noloss.h"

struct pdata_noloss_store noloss_queue[NOLOSS_QUEUE_SIZE];
extern frame_info MySyncPack;
extern int MaxXDataSize;
extern int N_players;

// Adds a packet to our queue
// Should be called when an IMPORTANT frameinfo packet is created
void noloss_add_packet_to_queue(int urgent, int pkt_num, char *data, ushort data_size)
{
	int i;
	
	// Only proceed if this is a version-checked game
	if (Netgame.protocol_version != MULTI_PROTO_D2X_VER)
		return;

	// Only add urgent packets
	if (!urgent)
		return;
	
	for (i = 0; i < NOLOSS_QUEUE_SIZE; i++)
	{
		if (noloss_queue[i].used)
			continue;

		noloss_queue[i].used = 1;
		noloss_queue[i].pkt_initial_timestamp = GameTime;
		noloss_queue[i].pkt_timestamp = GameTime;
		noloss_queue[i].pkt_num = pkt_num;
		memset( &noloss_queue[i].player_ack, 0, sizeof(ubyte)*MAX_PLAYERS );
		noloss_queue[i].N_players = N_players;
		memcpy( &noloss_queue[i].data[0], data, data_size );
		noloss_queue[i].data_size = data_size;
		
		return;
	}
	con_printf(CON_DEBUG, "Noloss queue is full!\n");
}

// Send the packet stored in queue list at index to given receiver_pnum
// Called from inside noloss_process_queue()
void noloss_send_queued_packet(int queue_index)
{
	short_frame_info ShortSyncPack;
	int objnum = Players[Player_num].objnum;
	
	// Update Timestamp
	noloss_queue[queue_index].pkt_timestamp = GameTime;
	// Copy the multibuf data to MySyncPack
	memcpy( &MySyncPack.data[0],&noloss_queue[queue_index].data[0], noloss_queue[queue_index].data_size );
	MySyncPack.data_size = noloss_queue[queue_index].data_size;
	
	// The following code HEAVILY borrows from network_do_frame()
	// Create a frameinfo packet
	if (Netgame.ShortPackets)
	{
#ifdef WORDS_BIGENDIAN
		ubyte send_data[MAX_DATA_SIZE];
#endif
		int i;

		memset(&ShortSyncPack,0,sizeof(short_frame_info));
		create_shortpos(&ShortSyncPack.thepos, Objects+objnum, 0);
		ShortSyncPack.type                      = PID_PDATA;
		ShortSyncPack.playernum                 = Player_num;
		ShortSyncPack.obj_render_type           = Objects[objnum].render_type;
		ShortSyncPack.level_num                 = Current_level_num;
		ShortSyncPack.data_size                 = MySyncPack.data_size;
		memcpy (&ShortSyncPack.data[0],&MySyncPack.data[0],MySyncPack.data_size);

		MySyncPack.numpackets = INTEL_INT(noloss_queue[queue_index].pkt_num);
		ShortSyncPack.numpackets = MySyncPack.numpackets;
#ifndef WORDS_BIGENDIAN
		for(i=0; i<N_players; i++)
		{
			// Check if a player is not connected anymore so we won't send a packet and set positive
			if (!noloss_queue[queue_index].player_ack[i] && !Players[i].connected)
				noloss_queue[queue_index].player_ack[i] = 1;

			if(Players[i].connected && (i != Player_num))
				netdrv_send_packet_data((ubyte*)&ShortSyncPack, sizeof(short_frame_info) - MaxXDataSize + MySyncPack.data_size, NetPlayers.players[i].network.ipx.server, NetPlayers.players[i].network.ipx.node,Players[i].net_address);
		}
#else
		squish_short_frame_info(ShortSyncPack, send_data);

		for(i=0; i<N_players; i++) {
			// Check if a player is not connected anymore so we won't send a packet and set positive
			if (!noloss_queue[queue_index].player_ack[i] && !Players[i].connected)
				noloss_queue[queue_index].player_ack[i] = 1;

			if(!noloss_queue[queue_index].player_ack[i] && Players[i].connected && (i != Player_num))
				netdrv_send_packet_data((ubyte*)send_data, IPX_SHORT_INFO_SIZE-MaxXDataSize+MySyncPack.data_size, NetPlayers.players[i].network.ipx.server, NetPlayers.players[i].network.ipx.node,Players[i].net_address);
		}
#endif

	}
	else  // If long packets
	{
		int i;
		
		MySyncPack.numpackets					= noloss_queue[queue_index].pkt_num;
		MySyncPack.type                         = PID_PDATA;
		MySyncPack.playernum                    = Player_num;
		MySyncPack.obj_render_type              = Objects[objnum].render_type;
		MySyncPack.level_num                    = Current_level_num;
		MySyncPack.obj_segnum                   = Objects[objnum].segnum;
		MySyncPack.obj_pos                      = Objects[objnum].pos;
		MySyncPack.obj_orient                   = Objects[objnum].orient;
		MySyncPack.phys_velocity                = Objects[objnum].mtype.phys_info.velocity;
		MySyncPack.phys_rotvel                  = Objects[objnum].mtype.phys_info.rotvel;
		
		for(i=0; i<N_players; i++)
		{
			// Check if a player is not connected anymore so we won't send a packet and set positive
			if (!noloss_queue[queue_index].player_ack[i] && !Players[i].connected)
				noloss_queue[queue_index].player_ack[i] = 1;

			if(!noloss_queue[queue_index].player_ack[i] && Players[i].connected && (i != Player_num))
			{
				send_frameinfo_packet(&MySyncPack, NetPlayers.players[i].network.ipx.server, NetPlayers.players[i].network.ipx.node,Players[i].net_address);
			}
		}
	}
	
	MySyncPack.data_size = 0;               // Start data over at 0 length.
}

// We have received a PDATA packet. Send ACK response to sender!
// ACK packet needs to contain: packet num, sender player num, receiver player num
// Call in network_process_packet() at case PID_PDATA
void noloss_send_ack(int pkt_num, ubyte receiver_pnum)
{
	noloss_ack ack;
	
	memset(&ack,0,sizeof(noloss_ack));
	
	ack.type = PID_PDATA_ACK;
	ack.sender_pnum = Player_num;
	ack.receiver_pnum = receiver_pnum;
	ack.pkt_num = pkt_num;

	netdrv_send_packet_data( (ubyte *)&ack, sizeof(noloss_ack), NetPlayers.players[receiver_pnum].network.ipx.server, NetPlayers.players[receiver_pnum].network.ipx.node,Players[receiver_pnum].net_address );
}

// We got an ACK by a player. Set this player slot to positive!
void noloss_got_ack(ubyte *data)
{
	int i;
	noloss_ack *gotack = (noloss_ack *)data;
	
	if (gotack->receiver_pnum != Player_num)
		return;
	
	for (i = 0; i < NOLOSS_QUEUE_SIZE; i++)
	{
		if (gotack->pkt_num == noloss_queue[i].pkt_num)
		{
			noloss_queue[i].player_ack[gotack->sender_pnum] = 1;
			break;
		}
	}
}

// Init/Free the queue. Call at start and end of a game or level.
void noloss_init_queue(void)
{
	memset(&noloss_queue,0,sizeof(pdata_noloss_store)*NOLOSS_QUEUE_SIZE);
}

// The main queue-process function.
// 1) Check if we can remove a packet from queue (all players positive or packet too old)
// 2) Check if there are packets in queue which we need to re-send to player(s) (if packet is older than one second)
void noloss_process_queue(void)
{
	int i, count = 0;
	
	for (i = 0; i < NOLOSS_QUEUE_SIZE; i++)
	{
		int j, resend = 0;
		
		if (!noloss_queue[i].used)
			continue;
		
		// If N_players differs (because of disconnet), we resend to all to make sure we we get the right ACK packets for the right players
		if (N_players != noloss_queue[i].N_players)
		{
			memset( noloss_queue[i].player_ack, 0, sizeof(ubyte)*8 );
			noloss_queue[i].N_players = N_players;
			resend = 1;
		}
		else
		{
			// Check if at least one connected player has not ACK'd the packet
			for (j = 0; j < N_players; j++)
			{
				if (!noloss_queue[i].player_ack[j] && Players[j].connected && j != Player_num)
				{
					resend = 1;
				}
			}
		}

		// Check if we can remove a packet...
		if (!resend || 
			((noloss_queue[i].pkt_initial_timestamp + (F1_0*15) <= GameTime) || (GameTime < noloss_queue[i].pkt_initial_timestamp)))
		{
			memset(&noloss_queue[i],0,sizeof(pdata_noloss_store));
		}
		// ... otherwise resend if a second has passed
		else if ((noloss_queue[i].pkt_timestamp + F1_0 <= GameTime) || (GameTime < noloss_queue[i].pkt_timestamp))
		{
			con_printf(CON_DEBUG, "Re-Sending queued packet %i\n",i);
			noloss_send_queued_packet(i);
			count++;
		}
		
		// Only send 5 packets from the queue by each time the queue process is called
		if (count >= 5)
			break;
	}
}
