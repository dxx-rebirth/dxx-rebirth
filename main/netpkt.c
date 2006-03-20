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
// Network packet conversions
// Based on macnet.c from MAC version of Descent 1

#ifdef NETWORK

#include <string.h>
#include "types.h"
#include "multi.h"
#include "network.h"
#include "object.h"
#include "powerup.h"
#include "error.h"
#include "ipx.h"
#include "netpkt.h"

void receive_netplayer_info(ubyte *data, netplayer_info *info, int d1x)
{
	int loc = 0;
	
	memcpy(info->callsign, &(data[loc]), CALLSIGN_LEN);	      loc += CALLSIGN_LEN;
	info->callsign[CALLSIGN_LEN] = 0;
	if (!d1x) loc++;
	memcpy(&(info->server), &(data[loc]), 4);				  loc += 4;
	memcpy(&(info->node), &(data[loc]), 6); 					  loc += 6;
	memcpy(&(info->socket), &(data[loc]), 2);				  loc += 2;
//MWA  don't think we need to swap this because we need it in high order	info->socket = swapshort(info->socket);
	info->connected = data[loc]; loc++;
	#ifndef SHAREWARE
 //edited 03/04/99 Matt Mueller - sub_protocol was being set wrong in non-d1x games.. I still think its screwed somewhere else too though
	if (d1x){     
		info->sub_protocol = data[loc]; loc++;
//		printf ("%i "__FUNCTION__ " name=%s sub_protocol=%i\n",Player_num,info->callsign,info->sub_protocol);
	}else
		info->sub_protocol = 0;
//end edit -MM
	#endif
}

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	short tmps;
	int loc;

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.type;			loc++;
	memcpy(&(out_buffer[loc]), seq.player.callsign, CALLSIGN_LEN+1);		loc += CALLSIGN_LEN+1;
	memcpy(&(out_buffer[loc]), seq.player.server, 4);	  loc += 4;
	memcpy(&(out_buffer[loc]), seq.player.node, 6); 	  loc += 6;
	tmps = swapshort(seq.player.socket);
	memcpy(&(out_buffer[loc]), &tmps, 2);		loc += 2;
	out_buffer[loc] = seq.player.connected;	loc++;
	out_buffer[loc] = MULTI_PROTO_D1X_MINOR; loc++;

	if (net_address != NULL)	
		ipx_send_packet_data( out_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		ipx_send_broadcast_packet_data( out_buffer, loc );
	else
		ipx_send_internetwork_packet_data( out_buffer, loc, server, node);
}

void receive_sequence_packet(ubyte *data, sequence_packet *seq)
{
	int loc = 0;
	
	seq->type = data[0];				loc++;
	receive_netplayer_info(&(data[loc]), &(seq->player), 0);
}


void send_netgame_packet(ubyte *server, ubyte *node)
{
	uint tmpi;
	ushort tmps;
	int i, j;
	int loc = 0;
	
#ifndef SHAREWARE
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
		send_d1x_netgame_packet(server, node);
		return;
	}
#endif
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
	out_buffer[loc] = Netgame.type; loc++;
	memcpy(&(out_buffer[loc]), Netgame.game_name, NETGAME_NAME_LEN+1); loc += (NETGAME_NAME_LEN+1);
	memcpy(&(out_buffer[loc]), Netgame.team_name, 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	out_buffer[loc] = Netgame.gamemode; loc++;
	out_buffer[loc] = Netgame.difficulty; loc++;
	out_buffer[loc] = Netgame.game_status; loc++;
	out_buffer[loc] = Netgame.numplayers; loc++;
	out_buffer[loc] = Netgame.max_numplayers; loc++;
	out_buffer[loc] = Netgame.game_flags; loc++;
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(out_buffer[loc]), Netgame.players[i].callsign, CALLSIGN_LEN+1);	loc += CALLSIGN_LEN+1;
		memcpy(&(out_buffer[loc]), Netgame.players[i].server, 4);				  loc += 4;
		memcpy(&(out_buffer[loc]), Netgame.players[i].node, 6); 					  loc += 6;
		memcpy(&(out_buffer[loc]), &(Netgame.players[i].socket), 2);				  loc += 2;
		memcpy(&(out_buffer[loc]), &(Netgame.players[i].connected), 1);				loc++;
	}
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			out_buffer[loc] = Netgame.locations[i]; loc++;
		}
	} else {
		for (i = 0; i < MAX_PLAYERS; i++) {
			tmpi = swapint(Netgame.locations[i]);
			memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 		// SWAP HERE!!!
		}
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			tmps = swapshort(Netgame.kills[i][j]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
		}
	}

	tmpi = swapint(Netgame.levelnum);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	out_buffer[loc] = Netgame.protocol_version; loc++;
	out_buffer[loc] = Netgame.team_vector; loc++;
	tmps = swapshort(Netgame.segments_checksum);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	tmps = swapshort(Netgame.team_kills[0]);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	tmps = swapshort(Netgame.team_kills[1]);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = swapshort(Netgame.killed[i]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = swapshort(Netgame.player_kills[i]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
	}

#ifndef SHAREWARE
	tmpi = swapint(Netgame.level_time);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = swapint(Netgame.control_invul_time);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = swapint(Netgame.monitor_vector);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	for (i = 0; i < 8; i++) {
		tmpi = swapint(Netgame.player_score[i]);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	}
	for (i = 0; i < 8; i++) {
		memcpy(&(out_buffer[loc]), &(Netgame.player_flags[i]), 1); loc++;
	}
	memcpy(&(out_buffer[loc]), Netgame.mission_name, 9); loc += 9;
	memcpy(&(out_buffer[loc]), Netgame.mission_title, MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
	    out_buffer[loc] = Netgame.packets_per_sec; loc++;
	    tmpi = swapint(Netgame.flags);
	    memcpy(&out_buffer[loc], &tmpi, 4); loc += 4;
        }
#endif

	if (server == NULL)
		ipx_send_broadcast_packet_data(out_buffer, loc);
	else
		ipx_send_internetwork_packet_data( out_buffer, loc, server, node );
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame, int d1x)
{
	int i, j;
	int loc = 0;
#ifndef SHAREWARE
	if (d1x) {
		receive_d1x_netgame_packet(data, netgame);
		return;
	}
#endif

	netgame->type = data[loc];	loc++;
	memcpy(netgame->game_name, &(data[loc]), NETGAME_NAME_LEN+1); loc += (NETGAME_NAME_LEN+1);
	memcpy(netgame->team_name, &(data[loc]), 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	netgame->gamemode = data[loc]; loc++;
	netgame->difficulty = data[loc]; loc++;
	netgame->game_status = data[loc]; loc++;
	netgame->numplayers = data[loc]; loc++;
	netgame->max_numplayers = data[loc]; loc++;
	netgame->game_flags = data[loc]; loc++;
	for (i = 0; i < MAX_PLAYERS; i++) {
		receive_netplayer_info(&(data[loc]), &(netgame->players[i]), 0);
		loc += NETPLAYER_ORIG_SIZE;
	}
#if 0
	if (d1x) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			netgame->locations[i] = data[loc]; loc++;
		}
	} else
#endif
	{
		for (i = 0; i < MAX_PLAYERS; i++) {
			memcpy(&(netgame->locations[i]), &(data[loc]), 4); loc += 4;			// SWAP HERE!!!
			netgame->locations[i] = swapint(netgame->locations[i]);
		}
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			memcpy(&(netgame->kills[i][j]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
			netgame->kills[i][j] = swapshort(netgame->kills[i][j]);
		}
	}

	memcpy(&(netgame->levelnum), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->levelnum = swapint(netgame->levelnum);
	netgame->protocol_version = data[loc]; loc++;
	netgame->team_vector = data[loc]; loc++;
	memcpy(&(netgame->segments_checksum), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->segments_checksum = swapshort(netgame->segments_checksum);
	memcpy(&(netgame->team_kills[0]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->team_kills[0] = swapshort(netgame->team_kills[0]);
	memcpy(&(netgame->team_kills[1]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->team_kills[1] = swapshort(netgame->team_kills[1]);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->killed[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
		netgame->killed[i] = swapshort(netgame->killed[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_kills[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
		netgame->player_kills[i] = swapshort(netgame->player_kills[i]);
	}

#ifndef SHAREWARE
	memcpy(&(netgame->level_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->level_time = swapint(netgame->level_time);
	memcpy(&(netgame->control_invul_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->control_invul_time = swapint(netgame->control_invul_time);
	memcpy(&(netgame->monitor_vector), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->monitor_vector = swapint(netgame->monitor_vector);
	for (i = 0; i < 8; i++) {
		memcpy(&(netgame->player_score[i]), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->player_score[i] = swapint(netgame->player_score[i]);
	}
	memcpy(netgame->player_flags, &(data[loc]), 8); loc += 8;
#if 0
	for (i = 0; i < 8; i++) {
		memcpy(&(netgame->player_flags[i]), &(data[loc]), 1); loc++;
	}
#endif

	memcpy(netgame->mission_name, &(data[loc]), 9); loc += 9;
	memcpy(netgame->mission_title, &(data[loc]), MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
#if 0
	if (d1x && netgame->protocol_version == MULTI_PROTO_D1X_VER) {
	    netgame->packets_per_sec = data[loc]; loc++;
	    memcpy(&netgame->flags, &data[loc], 4); loc += 4;
	    netgame->flags = swapint(netgame->flags);
	}
#endif
#endif
}

#ifndef SHAREWARE
void store_netplayer_info(ubyte *data, netplayer_info *player, int d1x)
{
	memcpy(data, player->callsign, CALLSIGN_LEN);	   data += CALLSIGN_LEN;
	if (!d1x) *(data++) = 0;
	memcpy(data, player->server, 4);		   data += 4;
	memcpy(data, player->node, 6);			   data += 6;
	memcpy(data, &player->socket, 2);		 data += 2;
	*data = player->connected;     data++;
	if (d1x) {
		*data = player->sub_protocol;  data++;
//	    printf ("%i "__FUNCTION__ " name=%s sub_protocol=%i\n",Player_num,player->callsign,player->sub_protocol);
	}
}

void send_d1x_netgame_packet(ubyte *server, ubyte *node)
{
	uint tmpi;
	ushort tmps;
	int i, j;
	int loc = 0;
	
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
	out_buffer[loc] = Netgame.type; loc++;
	out_buffer[loc] = MULTI_PROTO_D1X_VER; loc++;
	out_buffer[loc] = Netgame.subprotocol; loc++;
	out_buffer[loc] = Netgame.required_subprotocol; loc++;
	memcpy(&(out_buffer[loc]), Netgame.game_name, NETGAME_NAME_LEN); loc += NETGAME_NAME_LEN;
	memcpy(&(out_buffer[loc]), Netgame.mission_name, 8); loc += 8;
	memcpy(&(out_buffer[loc]), Netgame.mission_title, MISSION_NAME_LEN); loc += MISSION_NAME_LEN;
	out_buffer[loc] = Netgame.levelnum; loc++;
	out_buffer[loc] = Netgame.gamemode; loc++;
	out_buffer[loc] = Netgame.difficulty; loc++;
	out_buffer[loc] = Netgame.game_status; loc++;
	out_buffer[loc] = Netgame.game_flags; loc++;
	out_buffer[loc] = Netgame.max_numplayers; loc++;
	out_buffer[loc] = Netgame.team_vector; loc++;
	if (Netgame.type == PID_D1X_GAME_LITE) {
		int master = -1;
		j = 0;
		for (i = 0; i < Netgame.numplayers; i++)
			if (Players[i].connected) {
				if (master == -1)
					master = i;
				j++;
			}
		out_buffer[loc] = j; loc++; /* numconnected */
		if (master == -1)   /* should not happen, but... */
			master = Player_num; 
		store_netplayer_info(&(out_buffer[loc]), &Netgame.players[master], 1); loc += NETPLAYER_D1X_SIZE;
		//added 4/18/99 Matt Mueller - send .flags as well, so 'I' can show them
		tmpi = swapint(Netgame.flags);
		memcpy(&out_buffer[loc], &tmpi, 4); loc += 4;
		//end addition -MM
	} else {
		out_buffer[loc] = Netgame.numplayers; loc++;
		out_buffer[loc] = NETPLAYER_D1X_SIZE; loc++; // sizeof netplayer struct
		for (i = 0; i < MAX_PLAYERS; i++) {
			store_netplayer_info(&(out_buffer[loc]), &Netgame.players[i], 1);
			loc += NETPLAYER_D1X_SIZE;
		}
		memcpy(&(out_buffer[loc]), Netgame.team_name[0], CALLSIGN_LEN); loc += CALLSIGN_LEN;
		memcpy(&(out_buffer[loc]), Netgame.team_name[1], CALLSIGN_LEN); loc += CALLSIGN_LEN;
		for (i = 0; i < MAX_PLAYERS; i++) {
			out_buffer[loc] = Netgame.locations[i]; loc++;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			for (j = 0; j < MAX_PLAYERS; j++) {
				tmps = swapshort(Netgame.kills[i][j]);
				memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 	// SWAP HERE!!!
			}
		}
		tmps = swapshort(Netgame.segments_checksum);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		tmps = swapshort(Netgame.team_kills[0]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		tmps = swapshort(Netgame.team_kills[1]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		for (i = 0; i < MAX_PLAYERS; i++) {
			tmps = swapshort(Netgame.killed[i]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 		// SWAP HERE!!!
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			tmps = swapshort(Netgame.player_kills[i]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 		// SWAP HERE!!!
		}
		tmpi = swapint(Netgame.level_time);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		tmpi = swapint(Netgame.control_invul_time);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		tmpi = swapint(Netgame.monitor_vector);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		for (i = 0; i < 8; i++) {
			tmpi = swapint(Netgame.player_score[i]);
			memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		}
		memcpy(&(out_buffer[loc]), Netgame.player_flags, MAX_PLAYERS); loc += MAX_PLAYERS;
		out_buffer[loc] = Netgame.packets_per_sec; loc++;
		tmpi = swapint(Netgame.flags);
		memcpy(&out_buffer[loc], &tmpi, 4); loc += 4;
	}

	if (server == NULL)
		ipx_send_broadcast_packet_data(out_buffer, loc);
	else
		ipx_send_internetwork_packet_data(out_buffer, loc, server, node);
}

void receive_d1x_netgame_packet(ubyte *data, netgame_info *netgame) {
	int i, j;
	int loc = 0;
	
	netgame->type = data[loc];	loc++;
	if (netgame->type == PID_D1X_GAME_LITE) {
		memset(netgame, 0, sizeof(netgame_info));
		netgame->type = PID_D1X_GAME_LITE;
	}
	netgame->protocol_version = data[loc]; loc++;
	netgame->subprotocol = data[loc]; loc++;
	netgame->required_subprotocol = data[loc]; loc++;
	memcpy(netgame->game_name, &(data[loc]), NETGAME_NAME_LEN); loc += NETGAME_NAME_LEN;
	netgame->game_name[NETGAME_NAME_LEN] = 0;
	memcpy(netgame->mission_name, &(data[loc]), 8); loc += 8;
	netgame->mission_name[8] = 0;
	memcpy(netgame->mission_title, &(data[loc]), MISSION_NAME_LEN); loc += MISSION_NAME_LEN;
	netgame->mission_title[MISSION_NAME_LEN] = 0;
        netgame->levelnum = ((signed char *)data)[loc]; loc++;
	netgame->gamemode = data[loc]; loc++;
	netgame->difficulty = data[loc]; loc++;
	netgame->game_status = data[loc]; loc++;
	netgame->game_flags = data[loc]; loc++;
	netgame->max_numplayers = data[loc]; loc++;
	netgame->team_vector = data[loc]; loc++;
	if (netgame->type == PID_D1X_GAME_LITE) {
		j = netgame->numplayers = data[loc]; loc++;
		for (i = 0; i < j; i++)
			netgame->players[i].connected = 1;
		receive_netplayer_info(&(data[loc]), &(netgame->players[0]), 1);loc += NETPLAYER_D1X_SIZE;
		//added 4/18/99 Matt Mueller - send .flags as well, so 'I' can show them
		if (netgame->subprotocol>=1){
			memcpy(&netgame->flags, &data[loc], 4); loc += 4;
			netgame->flags = swapint(netgame->flags);
		}
		//end addition -MM
	} else {
		netgame->numplayers = data[loc]; loc++;
		j = data[loc]; loc++; // sizeof netplayer struct
		if (j > 29) { /* sanity: 304+29*8=536, just below IPX_MAX=542 */
		    netgame->protocol_version = 0; return;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			receive_netplayer_info(&(data[loc]), &(netgame->players[i]), 1);
			loc += j;
		}
		memcpy(netgame->team_name[0], &(data[loc]), CALLSIGN_LEN); loc += CALLSIGN_LEN;
		netgame->team_name[0][CALLSIGN_LEN] = 0;
		memcpy(netgame->team_name[1], &(data[loc]), CALLSIGN_LEN); loc += CALLSIGN_LEN;
		netgame->team_name[1][CALLSIGN_LEN] = 0;
                for (i = 0; i < MAX_PLAYERS; i++) {
			netgame->locations[i] = data[loc]; loc++;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			for (j = 0; j < MAX_PLAYERS; j++) {
				memcpy(&(netgame->kills[i][j]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
				netgame->kills[i][j] = swapshort(netgame->kills[i][j]);
			}
		}
		memcpy(&(netgame->segments_checksum), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->segments_checksum = swapshort(netgame->segments_checksum);
		memcpy(&(netgame->team_kills[0]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->team_kills[0] = swapshort(netgame->team_kills[0]);
		memcpy(&(netgame->team_kills[1]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->team_kills[1] = swapshort(netgame->team_kills[1]);
		for (i = 0; i < MAX_PLAYERS; i++) {
			memcpy(&(netgame->killed[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
			netgame->killed[i] = swapshort(netgame->killed[i]);
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			memcpy(&(netgame->player_kills[i]), &(data[loc]), 2); loc += 2; 		// SWAP HERE!!!
			netgame->player_kills[i] = swapshort(netgame->player_kills[i]);
		}
		memcpy(&(netgame->level_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->level_time = swapint(netgame->level_time);
		memcpy(&(netgame->control_invul_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->control_invul_time = swapint(netgame->control_invul_time);
		memcpy(&(netgame->monitor_vector), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->monitor_vector = swapint(netgame->monitor_vector);
		for (i = 0; i < 8; i++) {
			memcpy(&(netgame->player_score[i]), &(data[loc]), 4); loc += 4; 			// SWAP_HERE
			netgame->player_score[i] = swapint(netgame->player_score[i]);
		}
		memcpy(netgame->player_flags, &(data[loc]), 8); loc += 8;
		netgame->packets_per_sec = data[loc]; loc++;
		memcpy(&netgame->flags, &data[loc], 4); loc += 4;
		netgame->flags = swapint(netgame->flags);
	}
}
#endif

void send_frameinfo_packet(ubyte *server, ubyte *node, ubyte *address, int short_packet)
{
	int loc, tmpi;
	short tmps;
#ifndef SHAREWARE
	ushort tmpus;
#endif
	object *pl_obj = &Objects[Players[Player_num].objnum];
	
	loc = 0;
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
#ifdef SHAREWARE
	out_buffer[0] = PID_PDATA;		loc++;
	tmpi = swapint(MySyncPack.numpackets);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmps = swapshort(Players[Player_num].objnum);
	memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
	out_buffer[loc] = Player_num; loc++;
	tmps = swapshort(pl_obj->segnum);
	memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;

	tmpi = swapint((int)pl_obj->pos.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->pos.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->pos.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	
	tmpi = swapint((int)pl_obj->orient.rvec.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.rvec.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.rvec.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.uvec.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.uvec.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.uvec.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.fvec.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.fvec.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->orient.fvec.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	
	tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.thrust.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.thrust.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.thrust.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.mass);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.drag);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.brakes);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotthrust.x);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotthrust.y);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmpi = swapint((int)pl_obj->mtype.phys_info.rotthrust.z);
	memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
	tmps = swapshort((short)pl_obj->mtype.phys_info.turnroll);
	memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
	tmps = swapshort(pl_obj->mtype.phys_info.flags);
	memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
	
	out_buffer[loc] = pl_obj->render_type; loc++;
	out_buffer[loc] = Current_level_num; loc++;
	tmps = swapshort(MySyncPack.data_size);
	memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
	memcpy(&(out_buffer[loc]), MySyncPack.data, MySyncPack.data_size);
	loc += MySyncPack.data_size;
#else
	if (short_packet == 1) {
		loc = 0;
		out_buffer[loc] = PID_SHORTPDATA; loc++;
		out_buffer[loc] = Player_num; loc++;
		out_buffer[loc] = pl_obj->render_type; loc++;
                out_buffer[loc] = Current_level_num; loc++;
                create_shortpos((shortpos *)(out_buffer + loc), pl_obj);
                loc += 9+2*3+2+2*3; // go past shortpos structure
                *(ushort *)(out_buffer + loc) = MySyncPack.data_size; loc += 2;
                memcpy(out_buffer + loc, MySyncPack.data, MySyncPack.data_size);
                loc += MySyncPack.data_size;
        } else if (short_packet == 2) {
		loc = 0;
		out_buffer[loc] = PID_PDATA_SHORT2; loc++;
                out_buffer[loc] = MySyncPack.numpackets & 255; loc++;
                create_shortpos((shortpos *)(out_buffer + loc), pl_obj);
                loc += 9+2*3+2+2*3; // go past shortpos structure
                tmpus = MySyncPack.data_size | (Player_num << 12) | (pl_obj->render_type << 15);
                *(ushort *)(out_buffer + loc) = tmpus; loc += 2;
                out_buffer[loc] = Current_level_num; loc++;
                memcpy(out_buffer + loc, MySyncPack.data, MySyncPack.data_size);
                loc += MySyncPack.data_size;
	} else {
		out_buffer[0] = PID_PDATA;		loc++;	loc += 3;		// skip three for pad byte
		tmpi = swapint(MySyncPack.numpackets);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->pos.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->pos.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->pos.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->orient.rvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.rvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.rvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmps = swapshort(pl_obj->segnum);
		memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
		tmps = swapshort(MySyncPack.data_size);
		memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;

		out_buffer[loc] = Player_num; loc++;
		out_buffer[loc] = pl_obj->render_type; loc++;
		out_buffer[loc] = Current_level_num; loc++;
		memcpy(&(out_buffer[loc]), MySyncPack.data, MySyncPack.data_size);
		loc += MySyncPack.data_size;
	}
#endif
#if 0 // adb: not possible (always array passed)
	if (address == NULL)
                ipx_send_internetwork_packet_data( out_buffer, loc, server, node );
	else
#endif
		ipx_send_packet_data( out_buffer, loc, server, node, address);
}

void receive_frameinfo_packet(ubyte *data, frame_info *info, int short_packet)
{
	int loc;
	
#ifdef SHAREWARE
	loc = 0;

	info->type = data[0];							loc++;
	memcpy(&(info->numpackets), &(data[loc]), 4);	loc += 4;
	info->numpackets = swapint(info->numpackets);
	memcpy(&(info->objnum), &(data[loc]), 2);		loc += 2;
	info->objnum = swapshort(info->objnum);
	info->playernum = data[loc];					loc++;

	memcpy(&(info->obj_segnum), &(data[loc]), 2);	loc += 2;
	info->obj_segnum = swapshort(info->obj_segnum);

	memcpy(&(info->obj_pos.x), &(data[loc]), 4);	loc += 4;
	info->obj_pos.x = (fix)swapint((int)info->obj_pos.x);
	memcpy(&(info->obj_pos.y), &(data[loc]), 4);	loc += 4;
	info->obj_pos.y = (fix)swapint((int)info->obj_pos.y);
	memcpy(&(info->obj_pos.z), &(data[loc]), 4);	loc += 4;
	info->obj_pos.z = (fix)swapint((int)info->obj_pos.z);

	memcpy(&(info->obj_orient.rvec.x), &(data[loc]), 4);	loc += 4;
	info->obj_orient.rvec.x = (fix)swapint((int)info->obj_orient.rvec.x);
	memcpy(&(info->obj_orient.rvec.y), &(data[loc]), 4);	loc += 4;
	info->obj_orient.rvec.y = (fix)swapint((int)info->obj_orient.rvec.y);
	memcpy(&(info->obj_orient.rvec.z), &(data[loc]), 4);	loc += 4;
	info->obj_orient.rvec.z = (fix)swapint((int)info->obj_orient.rvec.z);
	memcpy(&(info->obj_orient.uvec.x), &(data[loc]), 4);	loc += 4;
	info->obj_orient.uvec.x = (fix)swapint((int)info->obj_orient.uvec.x);
	memcpy(&(info->obj_orient.uvec.y), &(data[loc]), 4);	loc += 4;
	info->obj_orient.uvec.y = (fix)swapint((int)info->obj_orient.uvec.y);
	memcpy(&(info->obj_orient.uvec.z), &(data[loc]), 4);	loc += 4;
	info->obj_orient.uvec.z = (fix)swapint((int)info->obj_orient.uvec.z);
	memcpy(&(info->obj_orient.fvec.x), &(data[loc]), 4);	loc += 4;
	info->obj_orient.fvec.x = (fix)swapint((int)info->obj_orient.fvec.x);
	memcpy(&(info->obj_orient.fvec.y), &(data[loc]), 4);	loc += 4;
	info->obj_orient.fvec.y = (fix)swapint((int)info->obj_orient.fvec.y);
	memcpy(&(info->obj_orient.fvec.z), &(data[loc]), 4);	loc += 4;
	info->obj_orient.fvec.z = (fix)swapint((int)info->obj_orient.fvec.z);
	
	memcpy(&(info->obj_phys_info.velocity.x), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.velocity.x = (fix)swapint((int)info->obj_phys_info.velocity.x);
	memcpy(&(info->obj_phys_info.velocity.y), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.velocity.y = (fix)swapint((int)info->obj_phys_info.velocity.y);
	memcpy(&(info->obj_phys_info.velocity.z), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.velocity.z = (fix)swapint((int)info->obj_phys_info.velocity.z);

	memcpy(&(info->obj_phys_info.thrust.x), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.thrust.x = (fix)swapint((int)info->obj_phys_info.thrust.x);
	memcpy(&(info->obj_phys_info.thrust.y), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.thrust.y = (fix)swapint((int)info->obj_phys_info.thrust.y);
	memcpy(&(info->obj_phys_info.thrust.z), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.thrust.z = (fix)swapint((int)info->obj_phys_info.thrust.z);

	memcpy(&(info->obj_phys_info.mass), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.mass = (fix)swapint((int)info->obj_phys_info.mass);
	memcpy(&(info->obj_phys_info.drag), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.drag = (fix)swapint((int)info->obj_phys_info.drag);
	memcpy(&(info->obj_phys_info.brakes), &(data[loc]), 4); loc += 4;
	info->obj_phys_info.brakes = (fix)swapint((int)info->obj_phys_info.brakes);

	memcpy(&(info->obj_phys_info.rotvel.x), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotvel.x = (fix)swapint((int)info->obj_phys_info.rotvel.x);
	memcpy(&(info->obj_phys_info.rotvel.y), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotvel.y = (fix)swapint((int)info->obj_phys_info.rotvel.y);
	memcpy(&(info->obj_phys_info.rotvel.z), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotvel.z = (fix)swapint((int)info->obj_phys_info.rotvel.z);

	memcpy(&(info->obj_phys_info.rotthrust.x), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotthrust.x = (fix)swapint((int)info->obj_phys_info.rotthrust.x);
	memcpy(&(info->obj_phys_info.rotthrust.y), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotthrust.y = (fix)swapint((int)info->obj_phys_info.rotthrust.y);
	memcpy(&(info->obj_phys_info.rotthrust.z), &(data[loc]), 4);	loc += 4;
	info->obj_phys_info.rotthrust.z = (fix)swapint((int)info->obj_phys_info.rotthrust.z);

	memcpy(&(info->obj_phys_info.turnroll), &(data[loc]), 2);	loc += 2;
	info->obj_phys_info.turnroll = (fixang)swapshort((short)info->obj_phys_info.turnroll);

	memcpy(&(info->obj_phys_info.flags), &(data[loc]), 2);	loc += 2;
	info->obj_phys_info.flags = (ushort)swapshort(info->obj_phys_info.flags);

	info->obj_render_type = data[loc];		loc++;
	info->level_num = data[loc];			loc++;	
	memcpy( &(info->data_size), &(data[loc]), 2);	loc += 2;
	info->data_size = swapshort(info->data_size);
	memcpy(info->data, &(data[loc]), info->data_size);
#else
	if (short_packet == 1) {
		loc = 0;
                info->type = data[loc]; loc++;
                info->playernum = data[loc]; loc++;
                info->obj_render_type = data[loc]; loc++;
                info->level_num = data[loc]; loc++;
                loc += 9+2*3+2+2*3; // skip shortpos structure
                info->data_size = *(ushort *)(data + loc); loc+=2;
                memcpy(info->data, &(data[loc]), info->data_size);
        } else if (short_packet == 2) {
		ushort tmpus;

                loc = 0;
                info->type = data[loc];         loc++;
                info->numpackets = data[loc];   loc++;
                loc += 9+2*3+2+2*3; // skip shortpos structure
                tmpus = *(ushort *)(data + loc); loc+=2;
                info->data_size = tmpus & 0xfff;
                info->playernum = (tmpus >> 12) & 7;
                info->obj_render_type = tmpus >> 15;
                info->numpackets |= Players[info->playernum].n_packets_got & (~255);
                if (info->numpackets - Players[info->playernum].n_packets_got > 224)
                        info->numpackets -= 256;
                else if (Players[info->playernum].n_packets_got - info->numpackets > 128)
                        info->numpackets += 256;
                info->level_num = data[loc];    loc++;
                memcpy(info->data, &(data[loc]), info->data_size);
	} else {
		loc = 0;
		info->type = data[loc]; 		loc++;	loc += 3;		// skip three for pad byte
		memcpy(&(info->numpackets), &(data[loc]), 4);	loc += 4;
		info->numpackets = swapint(info->numpackets);

		memcpy(&(info->obj_pos.x), &(data[loc]), 4);	loc += 4;
		info->obj_pos.x = swapint((int)info->obj_pos.x);
		memcpy(&(info->obj_pos.y), &(data[loc]), 4);	loc += 4;
		info->obj_pos.y = swapint((int)info->obj_pos.y);
		memcpy(&(info->obj_pos.z), &(data[loc]), 4);	loc += 4;
		info->obj_pos.z = swapint((int)info->obj_pos.z);
	
		memcpy(&(info->obj_orient.rvec.x), &(data[loc]), 4);	loc += 4;
		info->obj_orient.rvec.x = swapint((int)info->obj_orient.rvec.x);
		memcpy(&(info->obj_orient.rvec.y), &(data[loc]), 4);	loc += 4;
		info->obj_orient.rvec.y = swapint((int)info->obj_orient.rvec.y);
		memcpy(&(info->obj_orient.rvec.z), &(data[loc]), 4);	loc += 4;
		info->obj_orient.rvec.z = swapint((int)info->obj_orient.rvec.z);
		memcpy(&(info->obj_orient.uvec.x), &(data[loc]), 4);	loc += 4;
		info->obj_orient.uvec.x = swapint((int)info->obj_orient.uvec.x);
		memcpy(&(info->obj_orient.uvec.y), &(data[loc]), 4);	loc += 4;
		info->obj_orient.uvec.y = swapint((int)info->obj_orient.uvec.y);
		memcpy(&(info->obj_orient.uvec.z), &(data[loc]), 4);	loc += 4;
		info->obj_orient.uvec.z = swapint((int)info->obj_orient.uvec.z);
		memcpy(&(info->obj_orient.fvec.x), &(data[loc]), 4);	loc += 4;
		info->obj_orient.fvec.x = swapint((int)info->obj_orient.fvec.x);
		memcpy(&(info->obj_orient.fvec.y), &(data[loc]), 4);	loc += 4;
		info->obj_orient.fvec.y = swapint((int)info->obj_orient.fvec.y);
		memcpy(&(info->obj_orient.fvec.z), &(data[loc]), 4);	loc += 4;
		info->obj_orient.fvec.z = swapint((int)info->obj_orient.fvec.z);

		memcpy(&(info->phys_velocity.x), &(data[loc]), 4);	loc += 4;
		info->phys_velocity.x = swapint((int)info->phys_velocity.x);
		memcpy(&(info->phys_velocity.y), &(data[loc]), 4);	loc += 4;
		info->phys_velocity.y = swapint((int)info->phys_velocity.y);
		memcpy(&(info->phys_velocity.z), &(data[loc]), 4);	loc += 4;
		info->phys_velocity.z = swapint((int)info->phys_velocity.z);

		memcpy(&(info->phys_rotvel.x), &(data[loc]), 4);	loc += 4;
		info->phys_rotvel.x = swapint((int)info->phys_rotvel.x);
		memcpy(&(info->phys_rotvel.y), &(data[loc]), 4);	loc += 4;
		info->phys_rotvel.y = swapint((int)info->phys_rotvel.y);
		memcpy(&(info->phys_rotvel.z), &(data[loc]), 4);	loc += 4;
		info->phys_rotvel.z = swapint((int)info->phys_rotvel.z);
	
		memcpy(&(info->obj_segnum), &(data[loc]), 2);	loc += 2;
		info->obj_segnum = swapshort(info->obj_segnum);
		memcpy(&(info->data_size), &(data[loc]), 2);	loc += 2;
		info->data_size = swapshort(info->data_size);
	
		info->playernum = data[loc];		loc++;
		info->obj_render_type = data[loc];	loc++;
		info->level_num = data[loc];		loc++;
		memcpy(info->data, &(data[loc]), info->data_size);
	}
#endif
}

void send_endlevel_packet(endlevel_info *info, ubyte *server, ubyte *node, ubyte *address)
{
	int i, j;
	int loc = 0;
	ushort tmps;
	
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
	out_buffer[loc] = info->type;			loc++;
	out_buffer[loc] = info->player_num;		loc++;
	out_buffer[loc] = info->connected;		loc++;
	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			tmps = swapshort(info->kill_matrix[i][j]);
			memcpy(&(out_buffer[loc]), &tmps, 2);
			loc += 2;
		}
	}
	tmps = swapshort(info->kills);
	memcpy(&(out_buffer[loc]), &tmps, 2);  loc += 2;
	tmps = swapshort(info->killed);
	memcpy(&(out_buffer[loc]), &tmps, 2);  loc += 2;
	out_buffer[loc] = info->seconds_left; loc++;	

	ipx_send_packet_data( out_buffer, loc, server, node, address);
}

void receive_endlevel_packet(ubyte *data, endlevel_info *info)
{
	int i, j;
	int loc = 0;

	info->type = data[loc];						loc++;
	info->player_num = data[loc];				loc++;
	info->connected = data[loc];				loc++;

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			memcpy(&(info->kill_matrix[i][j]), &(data[loc]), 2);	loc += 2;
			info->kill_matrix[i][j] = swapshort(info->kill_matrix[i][j]);
		}
	}
	memcpy(&(info->kills), &(data[loc]), 2);  loc += 2;
	info->kills = swapshort(info->kills);
	memcpy(&(info->killed), &(data[loc]), 2);  loc += 2;
	info->killed = swapshort(info->killed);
	info->seconds_left = data[loc];
}

void swap_object(object *obj)
{
// swap the short and int entries for this object
	obj->signature 			= swapint(obj->signature);
	obj->next				= swapshort(obj->next);
	obj->prev				= swapshort(obj->prev);
	obj->segnum				= swapshort(obj->segnum);
	obj->attached_obj		= swapshort(obj->attached_obj);
	obj->pos.x 				= swapint(obj->pos.x);
	obj->pos.y 				= swapint(obj->pos.y);
	obj->pos.z 				= swapint(obj->pos.z);

	obj->orient.rvec.x 		= swapint(obj->orient.rvec.x);
	obj->orient.rvec.y 		= swapint(obj->orient.rvec.y);
	obj->orient.rvec.z 		= swapint(obj->orient.rvec.z);
	obj->orient.fvec.x 		= swapint(obj->orient.fvec.x);
	obj->orient.fvec.y 		= swapint(obj->orient.fvec.y);
	obj->orient.fvec.z 		= swapint(obj->orient.fvec.z);
	obj->orient.uvec.x 		= swapint(obj->orient.uvec.x);
	obj->orient.uvec.y 		= swapint(obj->orient.uvec.y);
	obj->orient.uvec.z 		= swapint(obj->orient.uvec.z);
	
	obj->size				= swapint(obj->size);
	obj->shields			= swapint(obj->shields);
	
	obj->last_pos.x 		= swapint(obj->last_pos.x);
	obj->last_pos.y 		= swapint(obj->last_pos.y);
	obj->last_pos.z 		= swapint(obj->last_pos.z);
	
	obj->lifeleft			= swapint(obj->lifeleft);
	
	switch (obj->movement_type) {
	
	case MT_PHYSICS:
	
		obj->mtype.phys_info.velocity.x = swapint(obj->mtype.phys_info.velocity.x);
		obj->mtype.phys_info.velocity.y = swapint(obj->mtype.phys_info.velocity.y);
		obj->mtype.phys_info.velocity.z = swapint(obj->mtype.phys_info.velocity.z);
	
		obj->mtype.phys_info.thrust.x 	= swapint(obj->mtype.phys_info.thrust.x);
		obj->mtype.phys_info.thrust.y 	= swapint(obj->mtype.phys_info.thrust.y);
		obj->mtype.phys_info.thrust.z 	= swapint(obj->mtype.phys_info.thrust.z);
	
		obj->mtype.phys_info.mass		= swapint(obj->mtype.phys_info.mass);
		obj->mtype.phys_info.drag		= swapint(obj->mtype.phys_info.drag);
		obj->mtype.phys_info.brakes		= swapint(obj->mtype.phys_info.brakes);
	
		obj->mtype.phys_info.rotvel.x	= swapint(obj->mtype.phys_info.rotvel.x);
		obj->mtype.phys_info.rotvel.y	= swapint(obj->mtype.phys_info.rotvel.y);
		obj->mtype.phys_info.rotvel.z 	= swapint(obj->mtype.phys_info.rotvel.z);
	
		obj->mtype.phys_info.rotthrust.x = swapint(obj->mtype.phys_info.rotthrust.x);
		obj->mtype.phys_info.rotthrust.y = swapint(obj->mtype.phys_info.rotthrust.y);
		obj->mtype.phys_info.rotthrust.z = swapint(obj->mtype.phys_info.rotthrust.z);
	
		obj->mtype.phys_info.turnroll	= swapint(obj->mtype.phys_info.turnroll);
		obj->mtype.phys_info.flags		= swapshort(obj->mtype.phys_info.flags);
	
		break;
	
	case MT_SPINNING:
		
		obj->mtype.spin_rate.x = swapint(obj->mtype.spin_rate.x);
		obj->mtype.spin_rate.y = swapint(obj->mtype.spin_rate.y);
		obj->mtype.spin_rate.z = swapint(obj->mtype.spin_rate.z);
		break;
	}
	
	switch (obj->control_type) {
	
	case CT_WEAPON:
		obj->ctype.laser_info.parent_type		= swapshort(obj->ctype.laser_info.parent_type);
		obj->ctype.laser_info.parent_num		= swapshort(obj->ctype.laser_info.parent_num);
		obj->ctype.laser_info.parent_signature	= swapshort(obj->ctype.laser_info.parent_signature);
		obj->ctype.laser_info.creation_time		= swapint(obj->ctype.laser_info.creation_time);
		obj->ctype.laser_info.last_hitobj		= swapint(obj->ctype.laser_info.last_hitobj);
		obj->ctype.laser_info.multiplier		= swapint(obj->ctype.laser_info.multiplier);
		break;
	
	case CT_EXPLOSION:
		obj->ctype.expl_info.spawn_time		= swapint(obj->ctype.expl_info.spawn_time);
		obj->ctype.expl_info.delete_time	= swapint(obj->ctype.expl_info.delete_time);
		obj->ctype.expl_info.delete_objnum	= swapshort(obj->ctype.expl_info.delete_objnum);
		obj->ctype.expl_info.attach_parent	= swapshort(obj->ctype.expl_info.attach_parent);
		obj->ctype.expl_info.prev_attach	= swapshort(obj->ctype.expl_info.prev_attach);
		obj->ctype.expl_info.next_attach	= swapshort(obj->ctype.expl_info.next_attach);
		break;
	
	case CT_AI:
		obj->ctype.ai_info.hide_segment			= swapshort(obj->ctype.ai_info.hide_segment);
		obj->ctype.ai_info.hide_index			= swapshort(obj->ctype.ai_info.hide_index);
		obj->ctype.ai_info.path_length			= swapshort(obj->ctype.ai_info.path_length);
		obj->ctype.ai_info.cur_path_index		= swapshort(obj->ctype.ai_info.cur_path_index);
		obj->ctype.ai_info.follow_path_start_seg	= swapshort(obj->ctype.ai_info.follow_path_start_seg);
		obj->ctype.ai_info.follow_path_end_seg		= swapshort(obj->ctype.ai_info.follow_path_end_seg);
		obj->ctype.ai_info.danger_laser_signature = swapint(obj->ctype.ai_info.danger_laser_signature);
		obj->ctype.ai_info.danger_laser_num		= swapshort(obj->ctype.ai_info.danger_laser_num);
		break;
	
	case CT_LIGHT:
		obj->ctype.light_info.intensity = swapint(obj->ctype.light_info.intensity);
		break;
	
	case CT_POWERUP:
		obj->ctype.powerup_info.count = swapint(obj->ctype.powerup_info.count);
		if (obj->id == POW_VULCAN_WEAPON)
			obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
		break;
	
	}
	
	switch (obj->render_type) {
	
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
	
		obj->rtype.pobj_info.model_num		= swapint(obj->rtype.pobj_info.model_num);
	
		for (i=0;i<MAX_SUBMODELS;i++) {
			obj->rtype.pobj_info.anim_angles[i].p = swapint(obj->rtype.pobj_info.anim_angles[i].p);
			obj->rtype.pobj_info.anim_angles[i].b = swapint(obj->rtype.pobj_info.anim_angles[i].b);
			obj->rtype.pobj_info.anim_angles[i].h = swapint(obj->rtype.pobj_info.anim_angles[i].h);
		}
	
		obj->rtype.pobj_info.subobj_flags	= swapint(obj->rtype.pobj_info.subobj_flags);
		obj->rtype.pobj_info.tmap_override	= swapint(obj->rtype.pobj_info.tmap_override);
		obj->rtype.pobj_info.alt_textures	= swapint(obj->rtype.pobj_info.alt_textures);
		break;
	}
	
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		obj->rtype.vclip_info.vclip_num	= swapint(obj->rtype.vclip_info.vclip_num);
		obj->rtype.vclip_info.frametime	= swapint(obj->rtype.vclip_info.frametime);
		break;
	
	case RT_LASER:
		break;
	
	}
//  END OF SWAPPING OBJECT STRUCTURE

}


#endif //ifdef NETWORK
