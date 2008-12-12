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

#include "inferno.h"
#include "pstypes.h"
#include "netpkt.h"
#include "netdrv.h"

//FIXME: Certain combinations of players having and not having the editor will cause the segment checksum code to fail.
#ifdef WORDS_BIGENDIAN

#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"

// routine to calculate the checksum of the segments.  We add these specialized routines
// since the current way is byte order dependent.

void mac_do_checksum_calc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

ushort mac_calc_segment_checksum()
{
	int i, j, k;
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	for (i = 0; i < Highest_segment_index + 1; i++) {
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			mac_do_checksum_calc((unsigned char *)&(Segments[i].sides[j].type), 1, &sum1, &sum2);
			mac_do_checksum_calc(&(Segments[i].sides[j].pad), 1, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].wall_num);
			mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num);
			mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			s = INTEL_SHORT(Segments[i].sides[j].tmap_num2);
			mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
			for (k = 0; k < 4; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].u));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].v));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].uvls[k].l));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
			for (k = 0; k < 2; k++) {
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].x));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].y));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
				t = INTEL_INT(((int)Segments[i].sides[j].normals[k].z));
				mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
			}
		}
		for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].children[j]);
			mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
			s = INTEL_SHORT(Segments[i].verts[j]);
			mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(Segments[i].objects);
		mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		mac_do_checksum_calc((unsigned char *)&(Segments[i].special), 1, &sum1, &sum2);
		mac_do_checksum_calc((unsigned char *)&(Segments[i].matcen_num), 1, &sum1, &sum2);
		s = INTEL_SHORT(Segments[i].value);
		mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
		t = INTEL_INT(((int)Segments[i].static_light));
		mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
#ifndef EDITOR
		s = INTEL_SHORT(Segments[i].pad);	// necessary? If this isn't set to 0 it won't work Intel-Intel anyway.
		mac_do_checksum_calc((ubyte *)&s, 2, &sum1, &sum2);
#endif
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

// this routine totally and completely relies on the fact that the network
//  checksum must be calculated on the segments!!!!!

ushort netmisc_calc_checksum(void * vptr, int len)
{
	vptr = vptr;
	len = len;
	return mac_calc_segment_checksum();
}
#endif

// following are routine for macintosh only that will swap the elements of
// structures send through the networking code.  The structures and
// this code must be kept in total sync

#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "object.h"
#include "powerup.h"
#include "error.h"

ubyte out_buffer[MAX_DATA_SIZE];    // used for tmp netgame packets as well as sending object data

void put_netplayer_info(ubyte *data, netplayer_info *player, int d1x)
{
	memcpy(data, player->callsign, CALLSIGN_LEN); data += CALLSIGN_LEN;
	if (!d1x) *(data++) = 0;
	memcpy(data, player->server, 4);		      data += 4;
	memcpy(data, player->node, 6);			      data += 6;
	PUT_INTEL_SHORT(data, player->socket);		  data += 2;
	*data = player->connected;                    data++;
	if (d1x) {
		*data = player->sub_protocol;             data++;
	}
}

void get_netplayer_info(ubyte *data, netplayer_info *info, int d1x)
{
	int loc = 0;

	memcpy(info->callsign, &(data[loc]), CALLSIGN_LEN);	      loc += CALLSIGN_LEN;
	info->callsign[CALLSIGN_LEN] = 0;
	if (!d1x) loc++;
	memcpy(&(info->server), &(data[loc]), 4);				  loc += 4;
	memcpy(&(info->node), &(data[loc]), 6); 					  loc += 6;
	memcpy(&(info->socket), &(data[loc]), 2);				  loc += 2;
//MWA  don't think we need to swap this because we need it in high order	info->socket = INTEL_SHORT(info->socket);
	info->connected = data[loc]; loc++;
 //edited 03/04/99 Matt Mueller - sub_protocol was being set wrong in non-d1x games.. I still think its screwed somewhere else too though
	if (d1x){     
		info->sub_protocol = data[loc]; loc++;
	}else
		info->sub_protocol = 0;
//end edit -MM
}

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	int loc;

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.type;                                       loc++;
	put_netplayer_info(&(out_buffer[loc]), &seq.player, 0);         loc += sizeof(netplayer_info);
	out_buffer[loc] = MULTI_PROTO_D1X_MINOR;						loc++;

	if (net_address != NULL)	
		NetDrvSendPacketData( out_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		NetDrvSendBroadcastPacketData( out_buffer, loc );
	else
		NetDrvSendInternetworkPacketData( out_buffer, loc, server, node);
}

void receive_sequence_packet(ubyte *data, sequence_packet *seq)
{
	int loc = 0;

	seq->type = data[0];                        loc++;
	get_netplayer_info(&(data[loc]), &(seq->player), 0);
}

void send_netgame_packet(ubyte *server, ubyte *node)
{
	int i, j;
	int loc = 0;
	
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
		send_d1x_netgame_packet(server, node);
		return;
	}
	memset(out_buffer, 0, MAX_DATA_SIZE);
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
		put_netplayer_info(&(out_buffer[loc]), &Netgame.players[i], 0); loc += sizeof(netplayer_info);
	}
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			out_buffer[loc] = Netgame.locations[i]; loc++;
		}
	} else {
		for (i = 0; i < MAX_PLAYERS; i++) {
			PUT_INTEL_INT(out_buffer + loc, Netgame.locations[i]);  loc += 4; 		// SWAP HERE!!!
		}
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			PUT_INTEL_SHORT(out_buffer + loc, Netgame.kills[i][j]); loc += 2;   // SWAP HERE!!!
		}
	}

	PUT_INTEL_INT(out_buffer + loc, Netgame.levelnum);              loc += 4;				// SWAP_HERE
	out_buffer[loc] = Netgame.protocol_version; loc++;
	out_buffer[loc] = Netgame.team_vector; loc++;
	PUT_INTEL_SHORT(out_buffer + loc, Netgame.segments_checksum);   loc += 2;   // SWAP_HERE
	PUT_INTEL_SHORT(out_buffer + loc, Netgame.team_kills[0]);       loc += 2;   // SWAP_HERE
	PUT_INTEL_SHORT(out_buffer + loc, Netgame.team_kills[1]);       loc += 2;   // SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		PUT_INTEL_SHORT(out_buffer + loc, Netgame.killed[i]);       loc += 2;   // SWAP HERE!!!
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		PUT_INTEL_SHORT(out_buffer + loc, Netgame.player_kills[i]); loc += 2;   // SWAP HERE!!!
	}

	PUT_INTEL_INT(out_buffer + loc, Netgame.level_time);            loc += 4;   // SWAP_HERE
	PUT_INTEL_INT(out_buffer + loc, Netgame.control_invul_time);    loc += 4;   // SWAP_HERE
	PUT_INTEL_INT(out_buffer + loc, Netgame.monitor_vector);        loc += 4;   // SWAP_HERE
	for (i = 0; i < 8; i++) {
		PUT_INTEL_INT(out_buffer + loc, Netgame.player_score[i]);   loc += 4;				// SWAP_HERE
	}
	for (i = 0; i < 8; i++) {
		out_buffer[loc] = Netgame.player_flags[i];                  loc++;
	}
	memcpy(&(out_buffer[loc]), Netgame.mission_name, 9);            loc += 9;
	memcpy(&(out_buffer[loc]), Netgame.mission_title, MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
	    out_buffer[loc] = Netgame.packets_per_sec;                  loc++;
	    PUT_INTEL_INT(out_buffer + loc, Netgame.flags);             loc += 4;
        }

	if (server == NULL)
		NetDrvSendBroadcastPacketData(out_buffer, loc);
	else
		NetDrvSendInternetworkPacketData( out_buffer, loc, server, node );
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame, int d1x)
{
	int i, j;
	int loc = 0;
	if (d1x) {
		receive_d1x_netgame_packet(data, netgame);
		return;
	}

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
		get_netplayer_info(&(data[loc]), &(netgame->players[i]), 0);
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
			netgame->locations[i] = INTEL_INT(netgame->locations[i]);
		}
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			memcpy(&(netgame->kills[i][j]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
			netgame->kills[i][j] = INTEL_SHORT(netgame->kills[i][j]);
		}
	}

	memcpy(&(netgame->levelnum), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->levelnum = INTEL_INT(netgame->levelnum);
	netgame->protocol_version = data[loc]; loc++;
	netgame->team_vector = data[loc]; loc++;
	memcpy(&(netgame->segments_checksum), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->segments_checksum = INTEL_SHORT(netgame->segments_checksum);
	memcpy(&(netgame->team_kills[0]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->team_kills[0] = INTEL_SHORT(netgame->team_kills[0]);
	memcpy(&(netgame->team_kills[1]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
	netgame->team_kills[1] = INTEL_SHORT(netgame->team_kills[1]);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->killed[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
		netgame->killed[i] = INTEL_SHORT(netgame->killed[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_kills[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
		netgame->player_kills[i] = INTEL_SHORT(netgame->player_kills[i]);
	}

	memcpy(&(netgame->level_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->level_time = INTEL_INT(netgame->level_time);
	memcpy(&(netgame->control_invul_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->control_invul_time = INTEL_INT(netgame->control_invul_time);
	memcpy(&(netgame->monitor_vector), &(data[loc]), 4); loc += 4;				// SWAP_HERE
	netgame->monitor_vector = INTEL_INT(netgame->monitor_vector);
	for (i = 0; i < 8; i++) {
		memcpy(&(netgame->player_score[i]), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->player_score[i] = INTEL_INT(netgame->player_score[i]);
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
	    netgame->flags = INTEL_INT(netgame->flags);
	}
#endif
}

void send_d1x_netgame_packet(ubyte *server, ubyte *node)
{
	uint tmpi;
	ushort tmps;
	int i, j;
	int loc = 0;
	
	memset(out_buffer, 0, MAX_DATA_SIZE);
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
		put_netplayer_info(&(out_buffer[loc]), &Netgame.players[master], 1); loc += NETPLAYER_D1X_SIZE;
		//added 4/18/99 Matt Mueller - send .flags as well, so 'I' can show them
		tmpi = INTEL_INT(Netgame.flags);
		memcpy(&out_buffer[loc], &tmpi, 4); loc += 4;
		//end addition -MM
	} else {
		out_buffer[loc] = Netgame.numplayers; loc++;
		out_buffer[loc] = NETPLAYER_D1X_SIZE; loc++; // sizeof netplayer struct
		for (i = 0; i < MAX_PLAYERS; i++) {
			put_netplayer_info(&(out_buffer[loc]), &Netgame.players[i], 1);
			loc += NETPLAYER_D1X_SIZE;
		}
		memcpy(&(out_buffer[loc]), Netgame.team_name[0], CALLSIGN_LEN); loc += CALLSIGN_LEN;
		memcpy(&(out_buffer[loc]), Netgame.team_name[1], CALLSIGN_LEN); loc += CALLSIGN_LEN;
		for (i = 0; i < MAX_PLAYERS; i++) {
			out_buffer[loc] = Netgame.locations[i]; loc++;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			for (j = 0; j < MAX_PLAYERS; j++) {
				tmps = INTEL_SHORT(Netgame.kills[i][j]);
				memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 	// SWAP HERE!!!
			}
		}
		tmps = INTEL_SHORT(Netgame.segments_checksum);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		tmps = INTEL_SHORT(Netgame.team_kills[0]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		tmps = INTEL_SHORT(Netgame.team_kills[1]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 			// SWAP_HERE
		for (i = 0; i < MAX_PLAYERS; i++) {
			tmps = INTEL_SHORT(Netgame.killed[i]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 		// SWAP HERE!!!
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			tmps = INTEL_SHORT(Netgame.player_kills[i]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2; 		// SWAP HERE!!!
		}
		tmpi = INTEL_INT(Netgame.level_time);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		tmpi = INTEL_INT(Netgame.control_invul_time);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		tmpi = INTEL_INT(Netgame.monitor_vector);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		for (i = 0; i < 8; i++) {
			tmpi = INTEL_INT(Netgame.player_score[i]);
			memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4; 			// SWAP_HERE
		}
		memcpy(&(out_buffer[loc]), Netgame.player_flags, MAX_PLAYERS); loc += MAX_PLAYERS;
		out_buffer[loc] = Netgame.packets_per_sec; loc++;
		tmpi = INTEL_INT(Netgame.flags);
		memcpy(&out_buffer[loc], &tmpi, 4); loc += 4;
	}

	if (server == NULL)
		NetDrvSendBroadcastPacketData(out_buffer, loc);
	else
		NetDrvSendInternetworkPacketData(out_buffer, loc, server, node);
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
		get_netplayer_info(&(data[loc]), &(netgame->players[0]), 1);loc += NETPLAYER_D1X_SIZE;
		//added 4/18/99 Matt Mueller - send .flags as well, so 'I' can show them
		if (netgame->subprotocol>=1){
			memcpy(&netgame->flags, &data[loc], 4); loc += 4;
			netgame->flags = INTEL_INT(netgame->flags);
		}
		//end addition -MM
	} else {
		netgame->numplayers = data[loc]; loc++;
		j = data[loc]; loc++; // sizeof netplayer struct
		if (j > 29) { /* sanity: 304+29*8=536, just below IPX_MAX=542 */
		    netgame->protocol_version = 0; return;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			get_netplayer_info(&(data[loc]), &(netgame->players[i]), 1);
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
				netgame->kills[i][j] = INTEL_SHORT(netgame->kills[i][j]);
			}
		}
		memcpy(&(netgame->segments_checksum), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->segments_checksum = INTEL_SHORT(netgame->segments_checksum);
		memcpy(&(netgame->team_kills[0]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->team_kills[0] = INTEL_SHORT(netgame->team_kills[0]);
		memcpy(&(netgame->team_kills[1]), &(data[loc]), 2); loc += 2;				// SWAP_HERE
		netgame->team_kills[1] = INTEL_SHORT(netgame->team_kills[1]);
		for (i = 0; i < MAX_PLAYERS; i++) {
			memcpy(&(netgame->killed[i]), &(data[loc]), 2); loc += 2;			// SWAP HERE!!!
			netgame->killed[i] = INTEL_SHORT(netgame->killed[i]);
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			memcpy(&(netgame->player_kills[i]), &(data[loc]), 2); loc += 2; 		// SWAP HERE!!!
			netgame->player_kills[i] = INTEL_SHORT(netgame->player_kills[i]);
		}
		memcpy(&(netgame->level_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->level_time = INTEL_INT(netgame->level_time);
		memcpy(&(netgame->control_invul_time), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->control_invul_time = INTEL_INT(netgame->control_invul_time);
		memcpy(&(netgame->monitor_vector), &(data[loc]), 4); loc += 4;				// SWAP_HERE
		netgame->monitor_vector = INTEL_INT(netgame->monitor_vector);
		for (i = 0; i < 8; i++) {
			memcpy(&(netgame->player_score[i]), &(data[loc]), 4); loc += 4; 			// SWAP_HERE
			netgame->player_score[i] = INTEL_INT(netgame->player_score[i]);
		}
		memcpy(netgame->player_flags, &(data[loc]), 8); loc += 8;
		netgame->packets_per_sec = data[loc]; loc++;
		memcpy(&netgame->flags, &data[loc], 4); loc += 4;
		netgame->flags = INTEL_INT(netgame->flags);
	}
}

#ifdef WORDS_BIGENDIAN
void send_frameinfo_packet(frame_info *info, ubyte *server, ubyte *node, ubyte *net_address)
{
	int loc = 0;

	out_buffer[loc] = info->type;									loc++;
	/*3 bytes of padding */											loc += 3;

	PUT_INTEL_INT(&out_buffer[loc], info->numpackets);				loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_pos.x);			loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_pos.y);			loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_pos.z);			loc += 4;
	
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.rvec.x);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.rvec.y);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.rvec.z);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.uvec.x);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.uvec.y);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.uvec.z);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.fvec.x);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.fvec.y);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->obj_orient.fvec.z);	loc += 4;
	
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_velocity.x);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_velocity.y);	loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_velocity.z);	loc += 4;
	
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_rotvel.x);		loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_rotvel.y);		loc += 4;
	PUT_INTEL_INT(&out_buffer[loc], (int)info->phys_rotvel.z);		loc += 4;
	
	PUT_INTEL_SHORT(&out_buffer[loc], info->obj_segnum);			loc += 2;
	PUT_INTEL_SHORT(&out_buffer[loc], info->data_size);				loc += 2;
	out_buffer[loc] = info->playernum;								loc++;
	out_buffer[loc] = info->obj_render_type;						loc++;
	out_buffer[loc] = info->level_num;								loc++;
	memcpy(&out_buffer[loc], info->data, info->data_size);			loc += info->data_size;

	// Always array passed
	NetDrvSendPacketData(out_buffer, loc, server, node, net_address);
}

void receive_frameinfo_packet(ubyte *data, frame_info *info)
{
	int loc = 0;
	
	info->type = data[loc];									loc++;
	/*3 bytes of padding */									loc += 3;
	
	info->numpackets = GET_INTEL_INT(&data[loc]);			loc += 4;
	info->obj_pos.x = GET_INTEL_INT(&data[loc]);			loc += 4;	
	info->obj_pos.y = GET_INTEL_INT(&data[loc]);			loc += 4;	
	info->obj_pos.z = GET_INTEL_INT(&data[loc]);			loc += 4;	
	
	info->obj_orient.rvec.x = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.rvec.y = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.rvec.z = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.uvec.x = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.uvec.y = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.uvec.z = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.fvec.x = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.fvec.y = GET_INTEL_INT(&data[loc]);	loc += 4;			
	info->obj_orient.fvec.z = GET_INTEL_INT(&data[loc]);	loc += 4;			
	
	info->phys_velocity.x = GET_INTEL_INT(&data[loc]);		loc += 4;		
	info->phys_velocity.y = GET_INTEL_INT(&data[loc]);		loc += 4;				
	info->phys_velocity.z = GET_INTEL_INT(&data[loc]);		loc += 4;				
	
	info->phys_rotvel.x = GET_INTEL_INT(&data[loc]);		loc += 4;				
	info->phys_rotvel.y = GET_INTEL_INT(&data[loc]);		loc += 4;				
	info->phys_rotvel.z = GET_INTEL_INT(&data[loc]);		loc += 4;				
	
	info->obj_segnum = GET_INTEL_SHORT(&data[loc]);			loc += 2;		
	info->data_size = GET_INTEL_SHORT(&data[loc]);			loc += 2;		
	info->playernum = data[loc];							loc++;
	info->obj_render_type = data[loc];						loc++;
	info->level_num = data[loc];							loc++;
	memcpy(info->data, &data[loc], info->data_size);		loc += info->data_size;
}
#endif

void swap_object(object *obj)
{
// swap the short and int entries for this object
	obj->signature 			= INTEL_INT(obj->signature);
	obj->next				= INTEL_SHORT(obj->next);
	obj->prev				= INTEL_SHORT(obj->prev);
	obj->segnum				= INTEL_SHORT(obj->segnum);
	obj->attached_obj		= INTEL_SHORT(obj->attached_obj);
	obj->pos.x 				= INTEL_INT(obj->pos.x);
	obj->pos.y 				= INTEL_INT(obj->pos.y);
	obj->pos.z 				= INTEL_INT(obj->pos.z);

	obj->orient.rvec.x 		= INTEL_INT(obj->orient.rvec.x);
	obj->orient.rvec.y 		= INTEL_INT(obj->orient.rvec.y);
	obj->orient.rvec.z 		= INTEL_INT(obj->orient.rvec.z);
	obj->orient.fvec.x 		= INTEL_INT(obj->orient.fvec.x);
	obj->orient.fvec.y 		= INTEL_INT(obj->orient.fvec.y);
	obj->orient.fvec.z 		= INTEL_INT(obj->orient.fvec.z);
	obj->orient.uvec.x 		= INTEL_INT(obj->orient.uvec.x);
	obj->orient.uvec.y 		= INTEL_INT(obj->orient.uvec.y);
	obj->orient.uvec.z 		= INTEL_INT(obj->orient.uvec.z);
	
	obj->size				= INTEL_INT(obj->size);
	obj->shields			= INTEL_INT(obj->shields);
	
	obj->last_pos.x 		= INTEL_INT(obj->last_pos.x);
	obj->last_pos.y 		= INTEL_INT(obj->last_pos.y);
	obj->last_pos.z 		= INTEL_INT(obj->last_pos.z);
	
	obj->lifeleft			= INTEL_INT(obj->lifeleft);
	
	switch (obj->movement_type) {
	
	case MT_PHYSICS:
	
		obj->mtype.phys_info.velocity.x = INTEL_INT(obj->mtype.phys_info.velocity.x);
		obj->mtype.phys_info.velocity.y = INTEL_INT(obj->mtype.phys_info.velocity.y);
		obj->mtype.phys_info.velocity.z = INTEL_INT(obj->mtype.phys_info.velocity.z);
	
		obj->mtype.phys_info.thrust.x 	= INTEL_INT(obj->mtype.phys_info.thrust.x);
		obj->mtype.phys_info.thrust.y 	= INTEL_INT(obj->mtype.phys_info.thrust.y);
		obj->mtype.phys_info.thrust.z 	= INTEL_INT(obj->mtype.phys_info.thrust.z);
	
		obj->mtype.phys_info.mass		= INTEL_INT(obj->mtype.phys_info.mass);
		obj->mtype.phys_info.drag		= INTEL_INT(obj->mtype.phys_info.drag);
		obj->mtype.phys_info.brakes		= INTEL_INT(obj->mtype.phys_info.brakes);
	
		obj->mtype.phys_info.rotvel.x	= INTEL_INT(obj->mtype.phys_info.rotvel.x);
		obj->mtype.phys_info.rotvel.y	= INTEL_INT(obj->mtype.phys_info.rotvel.y);
		obj->mtype.phys_info.rotvel.z 	= INTEL_INT(obj->mtype.phys_info.rotvel.z);
	
		obj->mtype.phys_info.rotthrust.x = INTEL_INT(obj->mtype.phys_info.rotthrust.x);
		obj->mtype.phys_info.rotthrust.y = INTEL_INT(obj->mtype.phys_info.rotthrust.y);
		obj->mtype.phys_info.rotthrust.z = INTEL_INT(obj->mtype.phys_info.rotthrust.z);
	
		obj->mtype.phys_info.turnroll	= INTEL_INT(obj->mtype.phys_info.turnroll);
		obj->mtype.phys_info.flags		= INTEL_SHORT(obj->mtype.phys_info.flags);
	
		break;
	
	case MT_SPINNING:
		
		obj->mtype.spin_rate.x = INTEL_INT(obj->mtype.spin_rate.x);
		obj->mtype.spin_rate.y = INTEL_INT(obj->mtype.spin_rate.y);
		obj->mtype.spin_rate.z = INTEL_INT(obj->mtype.spin_rate.z);
		break;
	}
	
	switch (obj->control_type) {
	
	case CT_WEAPON:
		obj->ctype.laser_info.parent_type		= INTEL_SHORT(obj->ctype.laser_info.parent_type);
		obj->ctype.laser_info.parent_num		= INTEL_SHORT(obj->ctype.laser_info.parent_num);
		obj->ctype.laser_info.parent_signature	= INTEL_SHORT(obj->ctype.laser_info.parent_signature);
		obj->ctype.laser_info.creation_time		= INTEL_INT(obj->ctype.laser_info.creation_time);
		obj->ctype.laser_info.last_hitobj		= INTEL_INT(obj->ctype.laser_info.last_hitobj);
		obj->ctype.laser_info.multiplier		= INTEL_INT(obj->ctype.laser_info.multiplier);
		break;
	
	case CT_EXPLOSION:
		obj->ctype.expl_info.spawn_time		= INTEL_INT(obj->ctype.expl_info.spawn_time);
		obj->ctype.expl_info.delete_time	= INTEL_INT(obj->ctype.expl_info.delete_time);
		obj->ctype.expl_info.delete_objnum	= INTEL_SHORT(obj->ctype.expl_info.delete_objnum);
		obj->ctype.expl_info.attach_parent	= INTEL_SHORT(obj->ctype.expl_info.attach_parent);
		obj->ctype.expl_info.prev_attach	= INTEL_SHORT(obj->ctype.expl_info.prev_attach);
		obj->ctype.expl_info.next_attach	= INTEL_SHORT(obj->ctype.expl_info.next_attach);
		break;
	
	case CT_AI:
		obj->ctype.ai_info.hide_segment			= INTEL_SHORT(obj->ctype.ai_info.hide_segment);
		obj->ctype.ai_info.hide_index			= INTEL_SHORT(obj->ctype.ai_info.hide_index);
		obj->ctype.ai_info.path_length			= INTEL_SHORT(obj->ctype.ai_info.path_length);
		obj->ctype.ai_info.cur_path_index		= INTEL_SHORT(obj->ctype.ai_info.cur_path_index);
		obj->ctype.ai_info.follow_path_start_seg	= INTEL_SHORT(obj->ctype.ai_info.follow_path_start_seg);
		obj->ctype.ai_info.follow_path_end_seg		= INTEL_SHORT(obj->ctype.ai_info.follow_path_end_seg);
		obj->ctype.ai_info.danger_laser_signature = INTEL_INT(obj->ctype.ai_info.danger_laser_signature);
		obj->ctype.ai_info.danger_laser_num		= INTEL_SHORT(obj->ctype.ai_info.danger_laser_num);
		break;
	
	case CT_LIGHT:
		obj->ctype.light_info.intensity = INTEL_INT(obj->ctype.light_info.intensity);
		break;
	
	case CT_POWERUP:
		obj->ctype.powerup_info.count = INTEL_INT(obj->ctype.powerup_info.count);
		if (obj->id == POW_VULCAN_WEAPON)
			obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
		break;
	
	}
	
	switch (obj->render_type) {
	
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
	
		obj->rtype.pobj_info.model_num		= INTEL_INT(obj->rtype.pobj_info.model_num);
	
		for (i=0;i<MAX_SUBMODELS;i++) {
			obj->rtype.pobj_info.anim_angles[i].p = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].p);
			obj->rtype.pobj_info.anim_angles[i].b = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].b);
			obj->rtype.pobj_info.anim_angles[i].h = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].h);
		}
	
		obj->rtype.pobj_info.subobj_flags	= INTEL_INT(obj->rtype.pobj_info.subobj_flags);
		obj->rtype.pobj_info.tmap_override	= INTEL_INT(obj->rtype.pobj_info.tmap_override);
		obj->rtype.pobj_info.alt_textures	= INTEL_INT(obj->rtype.pobj_info.alt_textures);
		break;
	}
	
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		obj->rtype.vclip_info.vclip_num	= INTEL_INT(obj->rtype.vclip_info.vclip_num);
		obj->rtype.vclip_info.frametime	= INTEL_INT(obj->rtype.vclip_info.frametime);
		break;
	
	case RT_LASER:
		break;
	
	}
//  END OF SWAPPING OBJECT STRUCTURE

}

#ifndef WORDS_BIGENDIAN


// Calculates the checksum of a block of memory.
ushort netmisc_calc_checksum(void * vptr, int len)
{
	ubyte *ptr = (ubyte *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	while(len--) {
		sum1 += *ptr++;
		if (sum1 >= 255) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;

	return ((sum1<<8)+ sum2);
}

#endif /* WORDS_BIGENDIAN */

#endif //ifdef NETWORK
