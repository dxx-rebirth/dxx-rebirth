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


#ifdef RCS
static char rcsid[] = "$Id: netmisc.c,v 1.1.1.1 2001-01-19 03:30:01 bradleyb Exp $";
#endif

#include <conf.h>

#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "pstypes.h"
#include "mono.h"

#ifdef MACINTOSH

#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"

// routine to calculate the checksum of the segments.  We add these specialized routines
// since the current way is byte order dependent.

void mac_do_checksum_calc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--)	{
		*s1 += *b++;
		if (*s1 >= 255 ) *s1 -= 255;
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
			mac_do_checksum_calc(&(Segments[i].sides[j].type), 1, &sum1, &sum2);
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
		t = INTEL_INT(Segments[i].objects);
		mac_do_checksum_calc((ubyte *)&t, 4, &sum1, &sum2);
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}

// this routine totally and completely relies on the fact that the network
//  checksum must be calculated on the segments!!!!!

ushort netmisc_calc_checksum( void * vptr, int len )
{
	vptr = vptr;
	len = len;
	return mac_calc_segment_checksum();
}

// Calculates the checksum of a block of memory.
ushort netmisc_calc_checksum_pc( void * vptr, int len )
{
	ubyte * ptr = (ubyte *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	while(len--)	{
		sum1 += *ptr++;
		if (sum1 >= 255 ) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;
	
	return ((sum1<<8)+ sum2);
}

// following are routine for macintosh only that will swap the elements of
// structures send through the networking code.  The structures and
// this code must be kept in total sync

#include "ipx.h"
#include "byteswap.h"
#include "multi.h"
#include "network.h"
#include "object.h"
#include "powerup.h"
#include "error.h"

byte out_buffer[IPX_MAX_DATA_SIZE];		// used for tmp netgame packets as well as sending object data

void receive_netplayer_info(ubyte *data, netplayer_info *info)
{
	int loc = 0;
	
	memcpy(info->callsign, &(data[loc]), CALLSIGN_LEN+1);		loc += CALLSIGN_LEN+1;
	memcpy(&(info->network.ipx.server), &(data[loc]), 4);					loc += 4;
	memcpy(&(info->network.ipx.node), &(data[loc]), 6);						loc += 6;
	info->version_major = data[loc];							loc++;
	info->version_minor = data[loc];							loc++;
	memcpy(&(info->computer_type), &(data[loc]), 1);			loc++;		// memcpy to avoid compile time warning about enum
	info->connected = data[loc];								loc++;
	memcpy(&(info->socket), &(data[loc]), 2);					loc += 2; 
   memcpy (&(info->rank),&(data[loc]),1);					   loc++;
//MWA  don't think we need to swap this because we need it in high order	info->socket = swapshort(info->socket);
}

void send_netplayers_packet(ubyte *server, ubyte *node)
{
	int i, tmpi;
	int loc = 0;
	short tmps;
	
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = NetPlayers.type;						loc++;
	tmpi = INTEL_INT(NetPlayers.Security);
	memcpy(&(out_buffer[loc]), &tmpi, 4);					loc += 4;
	for (i = 0; i < MAX_PLAYERS+4; i++) {
		memcpy(&(out_buffer[loc]), NetPlayers.players[i].callsign, CALLSIGN_LEN+1);	loc += CALLSIGN_LEN+1;
		memcpy(&(out_buffer[loc]), NetPlayers.players[i].network.ipx.server, 4);				loc += 4;
		memcpy(&(out_buffer[loc]), NetPlayers.players[i].network.ipx.node, 6);					loc += 6;
		memcpy(&(out_buffer[loc]), &(NetPlayers.players[i].version_major), 1);		loc++;
		memcpy(&(out_buffer[loc]), &(NetPlayers.players[i].version_minor), 1);		loc++;
		memcpy(&(out_buffer[loc]), &(NetPlayers.players[i].computer_type), 1);		loc++;
		memcpy(&(out_buffer[loc]), &(NetPlayers.players[i].connected), 1);			loc++;
		tmps = INTEL_SHORT(NetPlayers.players[i].socket);
		memcpy(&(out_buffer[loc]), &tmps, 2);										loc += 2;		
		memcpy(&(out_buffer[loc]), &(NetPlayers.players[i].rank), 1);			loc++;
	}
	
	if ((server == NULL) && (node == NULL))
		ipx_send_broadcast_packet_data( out_buffer, loc );
	else
		ipx_send_internetwork_packet_data( out_buffer, loc, server, node);

}

void receive_netplayers_packet(ubyte *data, AllNetPlayers_info *pinfo)
{
	int i, loc = 0;

	pinfo->type = data[loc];							loc++;
	memcpy(&(pinfo->Security), &(data[loc]), 4);		loc += 4;
	pinfo->Security = INTEL_INT(pinfo->Security);
	for (i = 0; i < MAX_PLAYERS+4; i++) {
		receive_netplayer_info(&(data[loc]), &(pinfo->players[i]));
		loc += 26;			// sizeof(netplayer_info) on the PC
	}
}

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	short tmps;
	int loc, tmpi;

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.type;										loc++;
	tmpi = INTEL_INT(seq.Security);
	memcpy(&(out_buffer[loc]), &tmpi, 4);							loc += 4;		loc += 3;
	memcpy(&(out_buffer[loc]), seq.player.callsign, CALLSIGN_LEN+1);loc += CALLSIGN_LEN+1;
	memcpy(&(out_buffer[loc]), seq.player.network.ipx.server, 4);				loc += 4;
	memcpy(&(out_buffer[loc]), seq.player.network.ipx.node, 6);					loc += 6;
	out_buffer[loc] = seq.player.version_major;						loc++;
	out_buffer[loc] = seq.player.version_minor;						loc++;
	out_buffer[loc] = seq.player.computer_type;						loc++;
	out_buffer[loc] = seq.player.connected;							loc++;
	tmps = swapshort(seq.player.socket);
	memcpy(&(out_buffer[loc]), &tmps, 2);							loc += 2;
   out_buffer[loc]=seq.player.rank;									loc++;		// for pad byte
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
	
	seq->type = data[0];						loc++;
	memcpy(&(seq->Security), &(data[loc]), 4);	loc += 4;	loc += 3;		// +3 for pad byte
	seq->Security = INTEL_INT(seq->Security);
	receive_netplayer_info(&(data[loc]), &(seq->player));
}

void send_netgame_packet(ubyte *server, ubyte *node, ubyte *net_address, int lite_flag)		// lite says shorter netgame packets
{
	uint tmpi;
	ushort tmps, p;
	int i, j;
	int loc = 0;
	
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
	memcpy(&(out_buffer[loc]), &(Netgame.type), 1);	loc++;
	tmpi = INTEL_INT(Netgame.Security);
	memcpy(&(out_buffer[loc]), &tmpi, 4);		loc += 4;
	memcpy(&(out_buffer[loc]), Netgame.game_name, NETGAME_NAME_LEN+1); loc += (NETGAME_NAME_LEN+1);
	memcpy(&(out_buffer[loc]), Netgame.mission_title, MISSION_NAME_LEN+1);	loc += (MISSION_NAME_LEN+1);
	memcpy(&(out_buffer[loc]), Netgame.mission_name, 9);			loc += 9;
	tmpi = INTEL_INT(Netgame.levelnum);
	memcpy(&(out_buffer[loc]), &tmpi, 4); 							loc += 4;
	memcpy(&(out_buffer[loc]), &(Netgame.gamemode), 1);				loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.RefusePlayers), 1);		loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.difficulty), 1);			loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.game_status), 1);			loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.numplayers), 1);			loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.max_numplayers), 1);		loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.numconnected), 1);			loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.game_flags), 1);			loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.protocol_version), 1);		loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.version_major), 1);		loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.version_minor), 1);		loc++;
	memcpy(&(out_buffer[loc]), &(Netgame.team_vector), 1);			loc++;
	
	if (lite_flag)
		goto do_send;

// will this work -- damn bitfields -- totally bogus when trying to do this type of stuff
// Watcom makes bitfields from left to right.  CW7 on the mac goes from right to left.
// then they are endian swapped

	tmps = *(ushort *)((ubyte *)(&Netgame.team_vector) + 1);			// get the values for the first short bitfield
	tmps = INTEL_SHORT(tmps);
	memcpy(&(out_buffer[loc]), &tmps, 2);		loc += 2;

	tmps = *(ushort *)((ubyte *)(&Netgame.team_vector) + 3);			// get the values for the second short bitfield
	tmps = INTEL_SHORT(tmps);
	memcpy(&(out_buffer[loc]), &tmps, 2);		loc += 2;

#if 0		// removed since I reordered bitfields on mac
	p = *(ushort *)((ubyte *)(&Netgame.team_vector) + 1);			// get the values for the first short bitfield
	tmps = 0;
	for (i = 15; i >= 0; i--) {
		if ( p & (1 << i) )
			tmps |= (1 << (15 - i));
	}
	tmps = INTEL_SHORT(tmps);
	memcpy(&(out_buffer[loc]), &tmps, 2);							loc += 2;
	p = *(ushort *)((ubyte *)(&Netgame.team_vector) + 3);			// get the values for the second short bitfield
	tmps = 0;
	for (i = 15; i >= 0; i--) {
		if ( p & (1 << i) )
			tmps |= (1 << (15 - i));
	}
	tmps = INTEL_SHORT(tmps);
	memcpy(&(out_buffer[loc]), &tmps, 2);							loc += 2;
#endif
	
	memcpy(&(out_buffer[loc]), Netgame.team_name, 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmpi = INTEL_INT(Netgame.locations[i]);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;			// SWAP HERE!!!
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			tmps = INTEL_SHORT(Netgame.kills[i][j]);
			memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
		}
	}

	tmps = INTEL_SHORT(Netgame.segments_checksum);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	tmps = INTEL_SHORT(Netgame.team_kills[0]);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	tmps = INTEL_SHORT(Netgame.team_kills[1]);
	memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;				// SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = INTEL_SHORT(Netgame.killed[i]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = INTEL_SHORT(Netgame.player_kills[i]);
		memcpy(&(out_buffer[loc]), &tmps, 2); loc += 2;			// SWAP HERE!!!
	}

	tmpi = INTEL_INT(Netgame.KillGoal);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = INTEL_INT(Netgame.PlayTimeAllowed);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = INTEL_INT(Netgame.level_time);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = INTEL_INT(Netgame.control_invul_time);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	tmpi = INTEL_INT(Netgame.monitor_vector);
	memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmpi = INTEL_INT(Netgame.player_score[i]);
		memcpy(&(out_buffer[loc]), &tmpi, 4); loc += 4;				// SWAP_HERE
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(out_buffer[loc]), &(Netgame.player_flags[i]), 1); loc++;
	}
	tmps = INTEL_SHORT(Netgame.PacketsPerSec);
	memcpy(&(out_buffer[loc]), &tmps, 2);					loc += 2;
	memcpy(&(out_buffer[loc]), &(Netgame.ShortPackets), 1);	loc ++;

do_send:
	if (net_address != NULL)	
		ipx_send_packet_data( out_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		ipx_send_broadcast_packet_data( out_buffer, loc );
	else
		ipx_send_internetwork_packet_data( out_buffer, loc, server, node);
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame, int lite_flag)
{
	int i, j;
	int loc = 0;
	short bitfield, new_field;
	
	memcpy(&(netgame->type), &(data[loc]), 1);						loc++;
	memcpy(&(netgame->Security), &(data[loc]), 4);					loc += 4;
	netgame->Security = INTEL_INT(netgame->Security);
	memcpy(netgame->game_name, &(data[loc]), NETGAME_NAME_LEN+1);	loc += (NETGAME_NAME_LEN+1);
	memcpy(netgame->mission_title, &(data[loc]), MISSION_NAME_LEN+1); loc += (MISSION_NAME_LEN+1);
	memcpy(netgame->mission_name, &(data[loc]), 9);					loc += 9;
	memcpy(&(netgame->levelnum), &(data[loc]), 4);						loc += 4;
	netgame->levelnum = INTEL_INT(netgame->levelnum);
	memcpy(&(netgame->gamemode), &(data[loc]), 1);					loc++;
	memcpy(&(netgame->RefusePlayers), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->difficulty), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->game_status), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->numplayers), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->max_numplayers), &(data[loc]), 1);			loc++;
	memcpy(&(netgame->numconnected), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->game_flags), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->protocol_version), &(data[loc]), 1);			loc++;
	memcpy(&(netgame->version_major), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->version_minor), &(data[loc]), 1);				loc++;
	memcpy(&(netgame->team_vector), &(data[loc]), 1);				loc++;

	if (lite_flag)
		return;

	memcpy(&bitfield, &(data[loc]), 2);	loc += 2;
	bitfield = INTEL_SHORT(bitfield);
	memcpy(((ubyte *)(&netgame->team_vector) + 1), &bitfield, 2);

	memcpy(&bitfield, &(data[loc]), 2);	loc += 2;
	bitfield = INTEL_SHORT(bitfield);
	memcpy(((ubyte *)(&netgame->team_vector) + 3), &bitfield, 2);

#if 0		// not used since reordering mac bitfields
	memcpy(&bitfield, &(data[loc]), 2);								loc += 2;
	new_field = 0;
	for (i = 15; i >= 0; i--) {
		if ( bitfield & (1 << i) )
			new_field |= (1 << (15 - i));
	}
	new_field = INTEL_SHORT(new_field);
	memcpy(((ubyte *)(&netgame->team_vector) + 1), &new_field, 2);

	memcpy(&bitfield, &(data[loc]), 2);								loc += 2;
	new_field = 0;
	for (i = 15; i >= 0; i--) {
		if ( bitfield & (1 << i) )
			new_field |= (1 << (15 - i));
	}
	new_field = INTEL_SHORT(new_field);
	memcpy(((ubyte *)(&netgame->team_vector) + 3), &new_field, 2);
#endif

	memcpy(netgame->team_name, &(data[loc]), 2*(CALLSIGN_LEN+1));		loc += 2*(CALLSIGN_LEN+1);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->locations[i]), &(data[loc]), 4);				loc += 4;
		netgame->locations[i] = INTEL_INT(netgame->locations[i]);
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			memcpy(&(netgame->kills[i][j]), &(data[loc]), 2);			loc += 2;
			netgame->kills[i][j] = INTEL_SHORT(netgame->kills[i][j]);
		}
	}

	memcpy(&(netgame->segments_checksum), &(data[loc]), 2);				loc += 2;
	netgame->segments_checksum = INTEL_SHORT(netgame->segments_checksum);
	memcpy(&(netgame->team_kills[0]), &(data[loc]), 2); 				loc += 2;
	netgame->team_kills[0] = INTEL_SHORT(netgame->team_kills[0]);
	memcpy(&(netgame->team_kills[1]), &(data[loc]), 2);					loc += 2;
	netgame->team_kills[1] = INTEL_SHORT(netgame->team_kills[1]);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->killed[i]), &(data[loc]), 2);					loc += 2;
		netgame->killed[i] = INTEL_SHORT(netgame->killed[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_kills[i]), &(data[loc]), 2); 			loc += 2;
		netgame->player_kills[i] = INTEL_SHORT(netgame->player_kills[i]);
	}
	memcpy(&(netgame->KillGoal), &(data[loc]), 4);						loc += 4;
	netgame->KillGoal = INTEL_INT(netgame->KillGoal);
	memcpy(&(netgame->PlayTimeAllowed), &(data[loc]), 4);				loc += 4;
	netgame->PlayTimeAllowed = INTEL_INT(netgame->PlayTimeAllowed);

	memcpy(&(netgame->level_time), &(data[loc]), 4);					loc += 4;
	netgame->level_time = INTEL_INT(netgame->level_time);
	memcpy(&(netgame->control_invul_time), &(data[loc]), 4);			loc += 4;
	netgame->control_invul_time = swapint(netgame->control_invul_time);
	memcpy(&(netgame->monitor_vector), &(data[loc]), 4);				loc += 4;
	netgame->monitor_vector = swapint(netgame->monitor_vector);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_score[i]), &(data[loc]), 4);			loc += 4;
		netgame->player_score[i] = swapint(netgame->player_score[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_flags[i]), &(data[loc]), 1); loc++;
	}
	memcpy(&(netgame->PacketsPerSec), &(data[loc]), 2);					loc += 2;
	netgame->PacketsPerSec = INTEL_SHORT(netgame->PacketsPerSec);
	memcpy(&(netgame->ShortPackets), &(data[loc]), 1);					loc ++;

}

void swap_object(object *obj)
{
// swap the short and int entries for this object
	obj->signature 			= INTEL_INT(obj->signature);
	obj->next				= INTEL_SHORT(obj->next);
	obj->prev				= INTEL_SHORT(obj->prev);
	obj->segnum				= INTEL_SHORT(obj->segnum);
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
		obj->ctype.laser_info.parent_signature	= INTEL_INT(obj->ctype.laser_info.parent_signature);
		obj->ctype.laser_info.creation_time		= INTEL_INT(obj->ctype.laser_info.creation_time);
		obj->ctype.laser_info.last_hitobj		= INTEL_SHORT(obj->ctype.laser_info.last_hitobj);
		obj->ctype.laser_info.track_goal		= INTEL_SHORT(obj->ctype.laser_info.track_goal);
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
		obj->ctype.ai_info.danger_laser_num		= INTEL_SHORT(obj->ctype.ai_info.danger_laser_num);
		obj->ctype.ai_info.danger_laser_signature = INTEL_INT(obj->ctype.ai_info.danger_laser_signature);
		obj->ctype.ai_info.dying_start_time 	= INTEL_INT(obj->ctype.ai_info.dying_start_time);
		break;
	
	case CT_LIGHT:
		obj->ctype.light_info.intensity = INTEL_INT(obj->ctype.light_info.intensity);
		break;
	
	case CT_POWERUP:
		obj->ctype.powerup_info.count = INTEL_INT(obj->ctype.powerup_info.count);
		obj->ctype.powerup_info.creation_time = INTEL_INT(obj->ctype.powerup_info.creation_time);
		//Below commented out 5/2/96 by Matt.  I asked Allender why it was
		//here, and he didn't know, and it looks like it doesn't belong.
		//if (obj->id == POW_VULCAN_WEAPON)
		//	obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
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

#else


// Calculates the checksum of a block of memory.
ushort netmisc_calc_checksum( void * vptr, int len )
{
	ubyte * ptr = (ubyte *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	while(len--)	{
		sum1 += *ptr++;
		if (sum1 >= 255 ) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;
	
	return ((sum1<<8)+ sum2);
}

#endif
//--unused-- //Finds the difference between block1 and block2.  Fills in diff_buffer and 
//--unused-- //returns the size of diff_buffer.
//--unused-- int netmisc_find_diff( void *block1, void *block2, int block_size, void *diff_buffer )
//--unused-- {
//--unused-- 	int mode;
//--unused-- 	ushort *c1, *c2, *diff_start, *c3;
//--unused-- 	int i, j, size, diff, n , same;
//--unused-- 
//--unused-- 	size=(block_size+1)/sizeof(ushort);
//--unused-- 	c1 = (ushort *)block1;
//--unused-- 	c2 = (ushort *)block2;
//--unused-- 	c3 = (ushort *)diff_buffer;
//--unused-- 
//--unused-- 	mode = same = diff = n = 0;
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 
//--unused-- 	for (i=0; i<size; i++, c1++, c2++ )	{
//--unused-- 		if (*c1 != *c2 ) {
//--unused-- 			if (mode==0)	{
//--unused-- 				mode = 1;
//--unused-- 				//mprintf( 0, "%ds ", same );
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			*c1 = *c2;
//--unused-- 			diff++;
//--unused-- 			if (diff==65535) {
//--unused-- 				mode = 0;
//--unused-- 				// send how many diff ones.
//--unused-- 				//mprintf( 0, "%dd ", diff );
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++ )
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		} else {
//--unused-- 			if (mode==1)	{
//--unused-- 				mode=0;
//--unused-- 				// send how many diff ones.
//--unused-- 				//mprintf( 0, "%dd ", diff );
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++ )
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			same++;
//--unused-- 			if (same==65535)	{
//--unused-- 				mode=1;
//--unused-- 				// send how many the same
//--unused-- 				//mprintf( 0, "%ds ", same );
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		}
//--unused-- 	
//--unused-- 	}
//--unused-- 	if (mode==0)	{
//--unused-- 		// send how many the same
//--unused-- 		//mprintf( 0, "%ds ", same );
//--unused-- 		c3[n++] = same;
//--unused-- 	} else {
//--unused-- 		// send how many diff ones.
//--unused-- 		//mprintf( 0, "%dd ", diff );
//--unused-- 		c3[n++]=diff;
//--unused-- 		// send all the diff ones.
//--unused-- 		for (j=0; j<diff; j++ )
//--unused-- 			c3[n++] = diff_start[j];
//--unused-- 	}
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 	
//--unused-- 	return n*2;
//--unused-- }

//--unused-- //Applies diff_buffer to block1 to create a new block1.  Returns the final
//--unused-- //size of block1.
//--unused-- int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size )	
//--unused-- {
//--unused-- 	unsigned int i, j, n, size;
//--unused-- 	ushort *c1, *c2;
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 	c1 = (ushort *)diff_buffer;
//--unused-- 	c2 = (ushort *)block1;
//--unused-- 
//--unused-- 	size = diff_size/2;
//--unused-- 
//--unused-- 	i=j=0;
//--unused-- 	while (1)	{
//--unused-- 		j += c1[i];			// Same
//--unused-- 		//mprintf( 0, "%ds ", c1[i] );
//--unused-- 		i++;
//--unused-- 		if ( i>=size) break;
//--unused-- 		n = c1[i];			// ndiff
//--unused-- 		//mprintf( 0, "%dd ", c1[i] );
//--unused-- 		i++;
//--unused-- 		if (n>0)	{
//--unused-- 			//Assert( n* < 256 );
//--unused-- 			memcpy( &c2[j], &c1[i], n*2 );
//--unused-- 			i += n;
//--unused-- 			j += n;
//--unused-- 		}
//--unused-- 		if ( i>=size) break;
//--unused-- 	}
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 
//--unused-- 	return j*2;
//--unused-- }

