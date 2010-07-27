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
 * Routines for managing IPX-protocol network play.
 *
 */

#ifdef NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pstypes.h"
#include "window.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "error.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "net_ipx.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "kmatrix.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "vers_id.h"
#include "byteswap.h"
#include "playsave.h"
#include "gamefont.h"
#include "rbaudio.h"

// Prototypes
void net_ipx_send_rejoin_sync(int player_num);
void net_ipx_update_netgame(void);
void net_ipx_send_endlevel_sub(int player_num);
void net_ipx_send_endlevel_packet(void);
void net_ipx_read_endlevel_packet( ubyte *data );
void net_ipx_read_object_packet( ubyte *data );
void net_ipx_read_sync_packet( ubyte * sp );
void net_ipx_flush();
void net_ipx_listen();
void net_ipx_ping_all(fix time);
void net_ipx_ping(ubyte flag, int pnum);
void net_ipx_handle_ping_return(ubyte pnum);
void net_ipx_process_pdata(char *data);
void net_ipx_more_game_options();
void net_ipx_read_pdata_packet(IPX_frame_info *pd);
int net_ipx_compare_players(netplayer_info *pl1, netplayer_info *pl2);
void net_ipx_do_refuse_stuff(IPX_sequence_packet *their);
int net_ipx_show_game_stats(int choice);

// Variables
netgame_info Active_ipx_games[IPX_MAX_NETGAMES];
int num_active_ipx_games = 0;
int	IPX_active=0;
int 	num_active_ipx_changed = 0;
IPX_sequence_packet	IPX_sync_player; // Who is rejoining now?
IPX_frame_info 	MySyncPack;
ubyte		MySyncPackInitialized = 0;		// Set to 1 if the MySyncPack is zeroed.
int IPX_Socket=0;
int     IPX_allow_socket_changes = 1;
IPX_sequence_packet IPX_Seq;
extern obj_position Player_init[MAX_PLAYERS];
extern void game_disable_cheats();
int net_ipx_wait_for_snyc();

/* General IPX functions - START */
ubyte broadcast_addr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
ubyte null_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
ubyte ipxdrv_installed=0;
u_int32_t ipx_network = 0;
ubyte MyAddress[10];
int ipx_packetnum = 0;			/* Sequence number */

/* User defined routing stuff */
typedef struct user_address {
	ubyte network[4];
	ubyte node[6];
	ubyte address[6];
} user_address;

socket_t socket_data;
#define MAX_USERS 64
int Ipx_num_users = 0;
user_address Ipx_users[MAX_USERS];
#define MAX_NETWORKS 64
int Ipx_num_networks = 0;
uint Ipx_networks[MAX_NETWORKS];

int ipxdrv_general_packet_ready(int fd)
{
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	tv.tv_sec = tv.tv_usec = 0;
	if (select(fd + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

struct net_driver *driver = NULL;

ubyte * ipxdrv_get_my_server_address()
{
	return (ubyte *)&ipx_network;
}

ubyte * ipxdrv_get_my_local_address()
{
	return (ubyte *)(MyAddress + 4);
}

void ipxdrv_close()
{
	if (ipxdrv_installed)
	{
#ifdef _WIN32
		WSACleanup();
#endif
		driver->close_socket(&socket_data);
	}

	ipxdrv_installed = 0;
}

//---------------------------------------------------------------
// Initializes all driver internals.
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//		-1 if socket already open.
//		-2 if socket table full.
//		-3 if driver not installed.
//		-4 if couldn't allocate low dos memory
//		-5 if error with getting internetwork address
int ipxdrv_init( int socket_number )
{
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
#endif

	if (!driver)
		return -1;

#ifdef _WIN32
	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup( wVersionRequested, &wsaData))
	{
		return -1;
	}
#endif
	memset(MyAddress,0,10);

	if (GameArg.MplIpxNetwork)
	{
		unsigned long n = strtol(GameArg.MplIpxNetwork, NULL, 16);
		MyAddress[0] = n >> 24; MyAddress[1] = (n >> 16) & 255;
		MyAddress[2] = (n >> 8) & 255; MyAddress[3] = n & 255;
		con_printf(CON_DEBUG,"IPX: Using network %08x\n", (unsigned int)n);
	}

	if (driver->open_socket(&socket_data, socket_number))
	{
		return -3;
	}

	memcpy(&ipx_network, MyAddress, 4);
	Ipx_num_networks = 0;
	memcpy( &Ipx_networks[Ipx_num_networks++], &ipx_network, 4 );

	ipxdrv_installed = 1;

	return 0;
}

int ipxdrv_set(int arg)
{
	int ipxdrv_err;

	ipxdrv_close();

	con_printf(CON_VERBOSE, "\n%s ", TXT_INITIALIZING_NETWORK);

	switch (arg)
	{
#ifndef __APPLE__
		case NETPROTO_IPX:
			driver = &ipxdrv_ipx;
			break;
#endif
#ifdef __LINUX__
		case NETPROTO_KALINIX:
			driver = &ipxdrv_kali;
			break;
#endif
		default:
			driver = NULL;
			break;
	}

	if ((ipxdrv_err=ipxdrv_init(IPX_DEFAULT_SOCKET))==0)
	{
		con_printf(CON_VERBOSE, "%s %d.\n", TXT_IPX_CHANNEL, IPX_DEFAULT_SOCKET );
		IPX_active = 1;
	}
	else
	{
		switch (ipxdrv_err)
		{
			case 3:
				con_printf(CON_VERBOSE, "%s\n", TXT_NO_NETWORK);
				break;
			case -2:
				con_printf(CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET);
				break;
			case -4:
				con_printf(CON_VERBOSE, "%s\n", TXT_MEMORY_IPX );
				break;
			default:
				con_printf(CON_VERBOSE, "%s %d", TXT_ERROR_IPX, ipxdrv_err );
				break;
		}

		con_printf(CON_VERBOSE, "%s\n",TXT_NETWORK_DISABLED);
		IPX_active = 0;		// Assume no network
	}

	return ipxdrv_installed?0:-1;
}

int ipxdrv_get_packet_data( ubyte * data )
{
	struct recv_data rd;
	char *buf;
	int size;

	if (driver->usepacketnum)
		buf=alloca(MAX_IPX_DATA);
	else
		buf=(char *)data;

	memset(rd.src_network,1,4);

	while (driver->packet_ready(&socket_data))
	{
		if ((size = driver->receive_packet(&socket_data, buf, MAX_IPX_DATA, &rd)) > 4)
		{
			if (!memcmp(rd.src_network, MyAddress, 10))
			{
				continue;	/* don't get own pkts */
			}

			if (driver->usepacketnum)
			{
				memcpy(data, buf + 4, size - 4);
				return size-4;
			}
			else
			{
				return size;
			}
		}
	}
	return 0;
}

void ipxdrv_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address )
{
	IPXPacket_t ipx_header;

	if (!ipxdrv_installed)
		return;

	memcpy(ipx_header.Destination.Network, network, 4);
	memcpy(ipx_header.Destination.Node, immediate_address, 6);
	ipx_header.PacketType = 4; /* Packet Exchange */

	if (driver->usepacketnum)
	{
		ubyte buf[MAX_IPX_DATA];
		*(uint *)buf = ipx_packetnum++;

		memcpy(buf + 4, data, datasize);
		driver->send_packet(&socket_data, &ipx_header, buf, datasize + 4);
	}
	else
		driver->send_packet(&socket_data, &ipx_header, data, datasize);//we can save 4 bytes
}

void ipxdrv_get_local_target( ubyte * server, ubyte * node, ubyte * local_target )
{
	memcpy( local_target, node, 6 );
}

void ipxdrv_send_broadcast_packet_data( ubyte * data, int datasize )
{
	int i, j;
	ubyte local_address[6];

	if (!ipxdrv_installed)
		return;

	// Set to all networks besides mine
	for (i=0; i<Ipx_num_networks; i++ )
	{
		if ( memcmp( &Ipx_networks[i], &ipx_network, 4 ) )
		{
			ipxdrv_get_local_target( (ubyte *)&Ipx_networks[i], broadcast_addr, local_address );
			ipxdrv_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast_addr, local_address );
		}
		else
		{
			ipxdrv_send_packet_data( data, datasize, (ubyte *)&Ipx_networks[i], broadcast_addr, broadcast_addr );
		}
	}

	// Send directly to all users not on my network or in the network list.
	for (i=0; i<Ipx_num_users; i++ )
	{
		if ( memcmp( Ipx_users[i].network, &ipx_network, 4 ) )
		{
			for (j=0; j<Ipx_num_networks; j++ )
			{
				if (!memcmp( Ipx_users[i].network, &Ipx_networks[j], 4 ))
					goto SkipUser;
			}

			ipxdrv_send_packet_data( data, datasize, Ipx_users[i].network, Ipx_users[i].node, Ipx_users[i].address );
SkipUser:
			j = 0;
		}
	}
}

// Sends a non-localized packet... needs 4 byte server, 6 byte address
void ipxdrv_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address )
{
	ubyte local_address[6];

	if (!ipxdrv_installed)
		return;

#ifdef WORDS_NEED_ALIGNMENT
	int zero = 0;
	if (memcmp(server, &zero, 4))
#else // WORDS_NEED_ALIGNMENT
	if ((*(uint *)server) != 0)
#endif // WORDS_NEED_ALIGNMENT
	{
		ipxdrv_get_local_target( server, address, local_address );
		ipxdrv_send_packet_data( data, datasize, server, address, local_address );
	}
	else
	{
		// Old method, no server info.
		ipxdrv_send_packet_data( data, datasize, server, address, address );
	}
}

int ipxdrv_change_default_socket( ushort socket_number )
{
	if ( !ipxdrv_installed )
		return -3;

	driver->close_socket(&socket_data);

	if (driver->open_socket(&socket_data, socket_number))
	{
		return -3;
	}

	return 0;
}

// Return type of net_driver
int ipxdrv_type(void)
{
	return driver->type;
}
/* General IPX functions - END */

void net_ipx_send_sequence_packet(IPX_sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	short tmps;
	int loc;
	ubyte out_buffer[MAX_DATA_SIZE];

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.type;			loc++;
	memcpy(&(out_buffer[loc]), seq.player.callsign, CALLSIGN_LEN+1);		loc += CALLSIGN_LEN+1;
	memcpy(&(out_buffer[loc]), seq.player.protocol.ipx.server, 4);	  loc += 4;
	memcpy(&(out_buffer[loc]), seq.player.protocol.ipx.node, 6); 	  loc += 6;
	tmps = INTEL_SHORT(seq.player.protocol.ipx.socket);
	memcpy(&(out_buffer[loc]), &tmps, 2);		loc += 2;
	out_buffer[loc] = seq.player.connected;	loc++;
	out_buffer[loc] = MULTI_PROTO_D1X_MINOR; loc++;

	if (net_address != NULL)
		ipxdrv_send_packet_data( out_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		ipxdrv_send_broadcast_packet_data( out_buffer, loc );
	else
		ipxdrv_send_internetwork_packet_data( out_buffer, loc, server, node);
}

void net_ipx_receive_sequence_packet(ubyte *data, IPX_sequence_packet *seq)
{
	int loc = 0;

	seq->type = data[0];                        loc++;
	memcpy(seq->player.callsign, &(data[loc]), CALLSIGN_LEN+1);       loc += CALLSIGN_LEN+1;
	memcpy(&(seq->player.protocol.ipx.server), &(data[loc]), 4);       loc += 4;
	memcpy(&(seq->player.protocol.ipx.node), &(data[loc]), 6);         loc += 6;
	memcpy(&(seq->player.protocol.ipx.socket), &(data[loc]), 2);                   loc += 2;
	seq->player.connected = data[loc];                                loc++;
}

void send_netgame_packet(ubyte *server, ubyte *node)
{
	IPX_netgame_info netpkt;
	int i, j;

	netpkt.type = Netgame.protocol.ipx.Game_pkt_type;
	memcpy(&netpkt.game_name, &Netgame.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
	memcpy(&netpkt.team_name, &Netgame.team_name, 2*(CALLSIGN_LEN+1));
	netpkt.gamemode = Netgame.gamemode;
	netpkt.difficulty = Netgame.difficulty;
	netpkt.game_status = Netgame.game_status;
	netpkt.numplayers = Netgame.numplayers;
	netpkt.max_numplayers = Netgame.max_numplayers;
	netpkt.game_flags = Netgame.game_flags;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(&netpkt.players[i].callsign, &Netgame.players[i].callsign, CALLSIGN_LEN);
		memcpy(&netpkt.players[i].server, &Netgame.players[i].protocol.ipx.server, 4);
		memcpy(&netpkt.players[i].node, &Netgame.players[i].protocol.ipx.node, 6);
		netpkt.players[i].socket = Netgame.players[i].protocol.ipx.socket;
		netpkt.players[i].connected = Netgame.players[i].connected;
	}
	for (i = 0; i < MAX_PLAYERS; i++)
		netpkt.locations[i] = Netgame.locations[i];
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netpkt.kills[i][j] = Netgame.kills[i][j];
	netpkt.levelnum = Netgame.levelnum;
	netpkt.protocol_version = Netgame.protocol.ipx.protocol_version;
	netpkt.team_vector = Netgame.team_vector;
	netpkt.segments_checksum = Netgame.segments_checksum;
	netpkt.team_kills[0] = Netgame.team_kills[0];
	netpkt.team_kills[1] = Netgame.team_kills[1];
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		netpkt.killed[i] = Netgame.killed[i];
		netpkt.player_kills[i] = Netgame.player_kills[i];
	}
	netpkt.level_time = Netgame.level_time;
	netpkt.control_invul_time = Netgame.control_invul_time;
	netpkt.monitor_vector = Netgame.monitor_vector;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		netpkt.player_score[i] = Netgame.player_score[i];
		netpkt.player_flags[i] = Netgame.player_flags[i];
	}
	memcpy(&netpkt.mission_name, &Netgame.mission_name, sizeof(char)*9);
	memcpy(&netpkt.mission_title, &Netgame.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));

	if (server == NULL && node == NULL)
		ipxdrv_send_broadcast_packet_data((ubyte *)&netpkt, sizeof(IPX_netgame_info));
	else
		ipxdrv_send_internetwork_packet_data((ubyte *)&netpkt, sizeof(IPX_netgame_info), server, node);
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame)
{
	IPX_netgame_info netpkt;
	int i, j;

	memcpy(&netpkt, data, sizeof(IPX_netgame_info));

	netgame->protocol.ipx.Game_pkt_type = netpkt.type;
	memcpy(netgame->game_name, &netpkt.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
	memcpy(netgame->team_name, &netpkt.team_name, 2*(CALLSIGN_LEN+1));
	netgame->gamemode = netpkt.gamemode;
	netgame->difficulty = netpkt.difficulty;
	netgame->game_status = netpkt.game_status;
	netgame->numplayers = netpkt.numplayers;
	netgame->max_numplayers = netpkt.max_numplayers;
	netgame->game_flags = netpkt.game_flags;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memcpy(netgame->players[i].callsign, &netpkt.players[i].callsign, CALLSIGN_LEN);
		memcpy(netgame->players[i].protocol.ipx.server, &netpkt.players[i].server, 4);
		memcpy(netgame->players[i].protocol.ipx.node, &netpkt.players[i].node, 6);
		netgame->players[i].protocol.ipx.socket = netpkt.players[i].socket;
		netgame->players[i].connected = netpkt.players[i].connected;
	}
	for (i = 0; i < MAX_PLAYERS; i++)
		netgame->locations[i] = netpkt.locations[i];
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netgame->kills[i][j] = netpkt.kills[i][j];
	netgame->levelnum = netpkt.levelnum;
	netgame->protocol.ipx.protocol_version = netpkt.protocol_version;
	netgame->team_vector = netpkt.team_vector;
	netgame->segments_checksum = netpkt.segments_checksum;
	netgame->team_kills[0] = netpkt.team_kills[0];
	netgame->team_kills[1] = netpkt.team_kills[1];
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		netgame->killed[i] = netpkt.killed[i];
		netgame->player_kills[i] = netpkt.player_kills[i];
	}
	netgame->level_time = netpkt.level_time;
	netgame->control_invul_time = netpkt.control_invul_time;
	netgame->monitor_vector = netpkt.monitor_vector;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		netgame->player_score[i] = netpkt.player_score[i];
		netgame->player_flags[i] = netpkt.player_flags[i];
	}
	memcpy(netgame->mission_name, &netpkt.mission_name, sizeof(char)*9);
	memcpy(netgame->mission_title, &netpkt.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));
}



void
net_ipx_init(void)
{
	// So you want to play a netgame, eh?  Let's a get a few things
	// straight

	int save_pnum = Player_num;

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&IPX_Seq, 0, sizeof(IPX_sequence_packet));
	IPX_Seq.type = PID_REQUEST;
	memcpy(IPX_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	memcpy(IPX_Seq.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6);
	memcpy(IPX_Seq.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();

	Player_num = save_pnum;
	multi_new_game();
	Network_new_game = 1;
	Control_center_destroyed = 0;
	net_ipx_flush();

	Netgame.PacketsPerSec = 10;
}

// Change socket to new_socket, returns 1 if really changed
int net_ipx_change_socket(int new_socket)
{
        if ( new_socket+IPX_DEFAULT_SOCKET > 0x8000 )
                new_socket  = 0x8000 - IPX_DEFAULT_SOCKET;
        if ( new_socket+IPX_DEFAULT_SOCKET < 0 )
                new_socket  = IPX_DEFAULT_SOCKET;
        if (new_socket != IPX_Socket) {
                IPX_Socket = new_socket;
                net_ipx_listen();
                ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
                return 1;
        }
        return 0;
}

#define ENDLEVEL_SEND_INTERVAL F1_0*2
#define ENDLEVEL_IDLE_TIME	F1_0*10

int net_ipx_endlevel_poll2( newmenu *menu, d_event *event, int *secret );

int net_ipx_endlevel_poll( newmenu *menu, d_event *event, int *secret )
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int num_escaped = 0;
	int goto_secret = 0;

	int previous_state[MAX_NUM_NET_PLAYERS];
	int previous_seconds_left;
	newmenu_item *menus = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			if (((d_event_keycommand *)event)->keycode != KEY_ESC)
				return 0;

			{
				newmenu_item m2[2];
				int choice;

				m2[0].type = m2[1].type = NM_TYPE_MENU;
				m2[0].text = TXT_YES; m2[1].text = TXT_NO;
				choice = newmenu_do1(NULL, TXT_SURE_LEAVE_GAME, 2, m2, (int (*)( newmenu *, d_event *, void * ))net_ipx_endlevel_poll2, secret, 1);
				if (choice == 0)
				{
					Players[Player_num].connected = CONNECT_DISCONNECTED;
					return 0;
				}
				if (choice > -2)
					return 1;	// stay in endlevel screen
				else
					return 0;	// go to next level
			}


		case EVENT_IDLE:
			// Send our endlevel packet at regular intervals

			if (timer_get_fixed_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
			{
				net_ipx_send_endlevel_packet();
				t1 = timer_get_fixed_seconds();
			}

			for (i = 0; i < N_players; i++)
				previous_state[i] = Players[i].connected;

			previous_seconds_left = Countdown_seconds_left;

			net_ipx_listen();

			for (i = 0; i < N_players; i++)
			{
				if (previous_state[i] != Players[i].connected)
				{
					sprintf(menus[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
				}
				if (Players[i].connected == CONNECT_PLAYING)
				{
					// Check timeout for idle players
					if (timer_get_fixed_seconds() > Netgame.players[i].LastPacketTime+ENDLEVEL_IDLE_TIME)
					{
						Players[i].connected = CONNECT_DISCONNECTED;
						net_ipx_send_endlevel_sub(i);
					}
				}

				if ((Players[i].connected != CONNECT_PLAYING) && (Players[i].connected != CONNECT_ESCAPE_TUNNEL) && (Players[i].connected != CONNECT_END_MENU))
					num_ready++;
				if (Players[i].connected != CONNECT_PLAYING)
					num_escaped++;
				if (Players[i].connected == CONNECT_FOUND_SECRET)
					goto_secret = 1;
			}

			if (num_escaped == N_players) // All players are out of the mine
			{
				Countdown_seconds_left = -1;
			}

			if (previous_seconds_left != Countdown_seconds_left)
			{
				if (Countdown_seconds_left < 0)
				{
					sprintf(menus[N_players].text, TXT_REACTOR_EXPLODED);
				}
				else
				{
					sprintf(menus[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
				}
			}

			if (num_ready == N_players) // All players have checked in or are disconnected
			{
				if (goto_secret)
					*secret = 1; // If any player went to the secret level, we go to the secret level

				return -2;
			}
			break;

		case EVENT_WINDOW_CLOSE:
			net_ipx_send_endlevel_packet();
			net_ipx_send_endlevel_packet();
			MySyncPackInitialized = 0;

			net_ipx_update_netgame();

			d_free(menus[0].text);	// frees all text
			d_free(menus);
			break;

		default:
			break;
	}

	return 0;
}

int net_ipx_endlevel_poll2( newmenu *menu, d_event *event, int *secret )
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int goto_secret = 0;

	menu = menu;

	if (event->type != EVENT_IDLE)
		return 0;

	// Send our endlevel packet at regular intervals

	if (timer_get_fixed_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
	{
		net_ipx_send_endlevel_packet();
		t1 = timer_get_fixed_seconds();
	}

	net_ipx_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected != CONNECT_PLAYING) && (Players[i].connected != CONNECT_ESCAPE_TUNNEL) && (Players[i].connected != CONNECT_END_MENU))
			num_ready++;
		if (Players[i].connected == CONNECT_FOUND_SECRET)
			goto_secret = 1;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			*secret = 1;

		return -2;
	}

	return 0;
}

int net_ipx_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int goto_secret = 0;

	menu = menu;
	userdata = userdata;

	if (event->type != EVENT_IDLE)
		return 0;

	// Send our endlevel packet at regular intervals

	if (timer_get_fixed_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
	{
		net_ipx_send_endlevel_packet();
		t1 = timer_get_fixed_seconds();
	}

	net_ipx_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected != CONNECT_PLAYING) && (Players[i].connected != CONNECT_ESCAPE_TUNNEL) && (Players[i].connected != CONNECT_END_MENU))
			num_ready++;
		if (Players[i].connected == CONNECT_FOUND_SECRET)
			goto_secret = 1;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			return -3;
		else
			return -2;
	}

	return 0;
}

int net_ipx_endlevel(int *secret)
{
	// Do whatever needs to be done between levels

	newmenu *menu;
	window *wind;
	newmenu_item *m;
	char *menu_text;
	char *title;
	int i;

	MALLOC(m, newmenu_item, MAX_NUM_NET_PLAYERS+1);
	if (!m)
		return 0;
	MALLOC(menu_text, char, (MAX_NUM_NET_PLAYERS+2)*80);
	if (!menu_text)
	{
		d_free(m);
		return 0;
	}

	net_ipx_flush();

	Network_status = NETSTAT_ENDLEVEL; // We are between levels

	net_ipx_listen();

	net_ipx_send_endlevel_packet();

	// Setup menu text pointers and zero them
	for (i=0; i<N_players; i++)
	{
		m[i].type = NM_TYPE_TEXT;
		m[i].text = menu_text + i*80;
		sprintf(m[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
		Netgame.players[i].LastPacketTime = timer_get_fixed_seconds();
	}
	m[N_players].type = NM_TYPE_TEXT;
	m[N_players].text = menu_text + N_players*80;
	title = menu_text + (N_players + 1)*80;

	if (Countdown_seconds_left < 0)
		sprintf(m[N_players].text, TXT_REACTOR_EXPLODED);
	else
		sprintf(m[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);

	sprintf(title, "%s\n%s", TXT_WAITING, TXT_ESC_ABORT);

	menu = newmenu_do3(NULL, title, N_players+1, m, (int (*)( newmenu *, d_event *, void * ))net_ipx_endlevel_poll, secret,
					   0, NULL);

	// Stay here until finished
	wind = newmenu_get_window(menu);
	while (window_exists(wind))
		event_process();

	// Player canceled between levels
	if (Players[Player_num].connected == CONNECT_DISCONNECTED)
	{
		if (Game_wind)
			window_close(Game_wind);
		show_menus();
		ipxdrv_close();
	}

	return(0);
}

int
net_ipx_can_join_netgame(netgame_info *game)
{
	// Can this player rejoin a netgame in progress?

	int i, num_players;

	if (game->game_status == NETSTAT_STARTING)
		return 1;

	if (game->game_status != NETSTAT_PLAYING)
		return 0;

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	// Search to see if we were already in this closed netgame in progress

	for (i = 0; i < num_players; i++)
		if ( (!strcasecmp(Players[Player_num].callsign, game->players[i].callsign)) &&
			  (!memcmp(IPX_Seq.player.protocol.ipx.node, game->players[i].protocol.ipx.node, 6)) &&
			  (!memcmp(IPX_Seq.player.protocol.ipx.server, game->players[i].protocol.ipx.server, 4)) )
			break;

	if (i != num_players)
		return 1;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
		// Look for player that is not connected
		if (game->numplayers < game->max_numplayers)
			return 1;

		for (i = 0; i < num_players; i++) {
			if (game->players[i].connected == CONNECT_DISCONNECTED)
				return 1;
		}
		return 0;
	}

	return 0;
}

void
net_ipx_disconnect_player(int playernum)
{
	// A player has disconnected from the net game, take whatever steps are
	// necessary

	if (playernum == Player_num)
	{
		Int3(); // Weird, see Rob
		return;
	}

	Players[playernum].connected = CONNECT_DISCONNECTED;
	Netgame.players[playernum].connected = CONNECT_DISCONNECTED;

//	create_player_appearance_effect(&Objects[Players[playernum].objnum]);
	multi_make_player_ghost(playernum);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_disconnect(playernum);

	multi_strip_robots(playernum);
}

void
net_ipx_new_player(IPX_sequence_packet *their)
{
	int objnum;
	int pnum;

	pnum = their->player.connected;

	Assert(pnum >= 0);
	Assert(pnum < MaxNumNetPlayers);

	objnum = Players[pnum].objnum;

	if (Newdemo_state == ND_STATE_RECORDING) {
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their->player.callsign);
	}

	memcpy(Players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	memcpy(Netgame.players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);

	if ( (*(uint *)their->player.protocol.ipx.server) != 0 )
		ipxdrv_get_local_target( their->player.protocol.ipx.server, their->player.protocol.ipx.node, Players[pnum].net_address );
	else
		memcpy(Players[pnum].net_address, their->player.protocol.ipx.node, 6);

	memcpy(Netgame.players[pnum].protocol.ipx.node, their->player.protocol.ipx.node, 6);
	memcpy(Netgame.players[pnum].protocol.ipx.server, their->player.protocol.ipx.server, 4);

	Players[pnum].n_packets_got = 0;
	Players[pnum].connected = CONNECT_PLAYING;
	Players[pnum].net_kills_total = 0;
	Players[pnum].net_killed_total = 0;
	memset(kill_matrix[pnum], 0, MAX_PLAYERS*sizeof(short));
	Players[pnum].score = 0;
	Players[pnum].flags = 0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	HUD_init_message(HM_MULTI, "'%s' %s",their->player.callsign, TXT_JOINING);

	multi_make_ghost_player(pnum);
	multi_send_score();
}

void net_ipx_welcome_player(IPX_sequence_packet *their)
{
 	// Add a player to a game already in progress
	ubyte local_address[6];
	int player_num;
	int i;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Control_center_destroyed))
	{
		net_ipx_dump_player(their->player.protocol.ipx.server,their->player.protocol.ipx.node, DUMP_ENDLEVEL);
		return;
	}

	if (Network_send_objects)
	{
		// Ignore silently, we're already responding to someone and we can't
		// do more than one person at a time.  If we don't dump them they will
		// re-request in a few seconds.
		return;
	}

	if (their->player.connected != Current_level_num)
	{
		net_ipx_dump_player(their->player.protocol.ipx.server, their->player.protocol.ipx.node, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	memset(&IPX_sync_player, 0, sizeof(IPX_sequence_packet));
	Network_player_added = 0;

	if ( (*(uint *)their->player.protocol.ipx.server) != 0 )
		ipxdrv_get_local_target( their->player.protocol.ipx.server, their->player.protocol.ipx.node, local_address );
	else
		memcpy(local_address, their->player.protocol.ipx.node, 6);

	for (i = 0; i < N_players; i++)
	{
		if ( (!strcasecmp(Players[i].callsign, their->player.callsign )) && (!memcmp(Players[i].net_address,local_address, 6)) )
		{
			player_num = i;
			break;
		}
	}

	if (player_num == -1)
	{
		// Player is new to this game

		if ( !(Netgame.game_flags & NETGAME_FLAG_CLOSED) && (N_players < MaxNumNetPlayers))
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			Network_player_added = 1;
		}
		else if (Netgame.game_flags & NETGAME_FLAG_CLOSED)
		{
			// Slots are open but game is closed

			net_ipx_dump_player(their->player.protocol.ipx.server, their->player.protocol.ipx.node, DUMP_CLOSED);
			return;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one

			int oldest_player = -1;
			fix oldest_time = timer_get_fixed_seconds();
			int activeplayers = 0;

			Assert(N_players == MaxNumNetPlayers);

			for (i = 0; i < Netgame.numplayers; i++)
				if (Netgame.players[i].connected)
					activeplayers++;

			if (activeplayers == Netgame.max_numplayers)
			{
				// Game is full.
				net_ipx_dump_player(their->player.protocol.ipx.server, their->player.protocol.ipx.node, DUMP_FULL);
				return;
			}

			for (i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected

				net_ipx_dump_player(their->player.protocol.ipx.server, their->player.protocol.ipx.node, DUMP_FULL);
				return;
			}
			else
			{
				// Found a slot!

				player_num = oldest_player;
				Network_player_added = 1;
			}
		}
	}
	else
	{
		// Player is reconnecting

		if (Players[player_num].connected)
		{
			return;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		HUD_init_message(HM_MULTI, "'%s' %s", Players[player_num].callsign, TXT_REJOIN);
	}

	// Send updated Objects data to the new/returning player

	IPX_sync_player = *their;
	IPX_sync_player.player.connected = player_num;
	Network_send_objects = 1;
	Network_send_objnum = -1;

	net_ipx_send_objects();
}

int net_ipx_objnum_is_past(int objnum)
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.

	int player_num = IPX_sync_player.player.connected;
	int obj_mode = !((object_owner[objnum] == -1) || (object_owner[objnum] == player_num));

	if (!Network_send_objects)
		return 0; // We're not sending objects to a new player

	if (obj_mode > Network_send_object_mode)
		return 0;
	else if (obj_mode < Network_send_object_mode)
	 	return 1;
	else if (objnum < Network_send_objnum)
		return 1;
	else
		return 0;
}

#define IPX_OBJ_PACKETS_PER_FRAME 1

void net_ipx_send_door_updates(void)
{
	// Send door status when new player joins

	int i;

	for (i = 0; i < Num_walls; i++)
	{
		if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING)))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
	}

}

void net_ipx_process_monitor_vector(int vector)
{
	int i, j;
	int count = 0;
	segment *seg;

	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec, bm;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ( ((tm = seg->sides[j].tmap_num2) != 0) &&
				  ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
 				  ((bm = Effects[ec].dest_bm_num) != -1) )
			{
				if (vector & (1 << count))
				{
					seg->sides[j].tmap_num2 = bm | (tm&0xc000);
				}
				count++;
				Assert(count < 32);
			}
		}
	}
}

int net_ipx_create_monitor_vector(void)
{
	int i, j, k;
	int num_blown_bitmaps = 0;
	int monitor_num = 0;
	int blown_bitmaps[7];
	int vector = 0;
	segment *seg;

	for (i=0; i < Num_effects; i++)
	{
		if (Effects[i].dest_bm_num > 0) {
			for (j = 0; j < num_blown_bitmaps; j++)
				if (blown_bitmaps[j] == Effects[i].dest_bm_num)
					break;
			if (j == num_blown_bitmaps)
				blown_bitmaps[num_blown_bitmaps++] = Effects[i].dest_bm_num;
		}
	}


	Assert(num_blown_bitmaps <= 7);

	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ((tm = seg->sides[j].tmap_num2) != 0)
			{
				if ( ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
 					  (Effects[ec].dest_bm_num != -1) )
				{
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					for (k = 0; k < num_blown_bitmaps; k++)
					{
						if ((tm&0x3fff) == blown_bitmaps[k])
						{
							vector |= (1 << monitor_num);
							monitor_num++;
							Assert(monitor_num < 32);
							break;
						}
					}
				}
			}
		}
	}
	return(vector);
}

void net_ipx_stop_resync(IPX_sequence_packet *their)
{
	if ( (!memcmp(IPX_sync_player.player.protocol.ipx.node, their->player.protocol.ipx.node, 6)) &&
		  (!memcmp(IPX_sync_player.player.protocol.ipx.server, their->player.protocol.ipx.server, 4)) &&
	     (!strcasecmp(IPX_sync_player.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_send_objnum = -1;
	}
}

void net_ipx_swap_object(object *obj)
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

ubyte object_buffer[MAX_DATA_SIZE];

void net_ipx_send_objects(void)
{
	short remote_objnum;
	sbyte owner;
	int loc, i, h;

	static int obj_count = 0;
	static int frame_num = 0;

	int obj_count_frame = 0;
	int player_num = IPX_sync_player.player.connected;

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < MaxNumNetPlayers);

	if (Endlevel_sequence || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		net_ipx_dump_player(IPX_sync_player.player.protocol.ipx.server,IPX_sync_player.player.protocol.ipx.node, DUMP_ENDLEVEL);
		Network_send_objects = 0;
		return;
	}

	for (h = 0; h < IPX_OBJ_PACKETS_PER_FRAME; h++) // Do more than 1 per frame, try to speed it up without
															  // over-stressing the receiver.
	{
		obj_count_frame = 0;
		memset(object_buffer, 0, MAX_DATA_SIZE);
		object_buffer[0] = PID_OBJECT_DATA;
		loc = 3;

		if (Network_send_objnum == -1)
		{
			obj_count = 0;
			Network_send_object_mode = 0;
			*(short *)(object_buffer+loc) = INTEL_SHORT(-1);	loc += 2;
			object_buffer[loc] = player_num; 		loc += 1;
													loc += 2; // Placeholder for remote_objnum, not used here
			Network_send_objnum = 0;
			obj_count_frame = 1;
			frame_num = 0;
		}

		for (i = Network_send_objnum; i <= Highest_object_index; i++)
		{
			if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				 (Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				 (Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE))
				continue;
			if ((Network_send_object_mode == 0) && ((object_owner[i] != -1) && (object_owner[i] != player_num)))
				continue;
			if ((Network_send_object_mode == 1) && ((object_owner[i] == -1) || (object_owner[i] == player_num)))
				continue;

			if ( ((MAX_DATA_SIZE-1) - loc) < (sizeof(object)+5) )
				break; // Not enough room for another object

			obj_count_frame++;
			obj_count++;

			remote_objnum = objnum_local_to_remote((short)i, &owner);
			Assert(owner == object_owner[i]);

			*(short *)(object_buffer+loc) = INTEL_SHORT(i);								loc += 2;
			object_buffer[loc] = owner;											loc += 1;
			*(short *)(object_buffer+loc) = INTEL_SHORT(remote_objnum); 				loc += 2;
			memcpy(object_buffer+loc, &Objects[i], sizeof(object));	loc += sizeof(object);
			net_ipx_swap_object((object *)&object_buffer[loc]);
		}

		if (obj_count_frame) // Send any objects we've buffered
		{
			frame_num++;

			Network_send_objnum = i;
			object_buffer[1] = obj_count_frame;
			object_buffer[2] = frame_num;

			Assert(loc <= MAX_DATA_SIZE);

			ipxdrv_send_internetwork_packet_data( object_buffer, loc, IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node );
			// OLD ipxdrv_send_packet_data(object_buffer, loc, &IPX_sync_player.player.protocol.ipx.node);
		}

		if (i > Highest_object_index)
		{
			if (Network_send_object_mode == 0)
			{
				Network_send_objnum = 0;
				Network_send_object_mode = 1; // go to next mode
			}
			else
			{
				Assert(Network_send_object_mode == 1);

				frame_num++;
				// Send count so other side can make sure he got them all
				object_buffer[0] = PID_OBJECT_DATA;
				object_buffer[1] = 1;
				object_buffer[2] = frame_num;
				*(short *)(object_buffer+3) = INTEL_SHORT(-2);
				*(short *)(object_buffer+6) = INTEL_SHORT(obj_count);
				//OLD ipxdrv_send_packet_data(object_buffer, 8, &IPX_sync_player.player.protocol.ipx.node);
				ipxdrv_send_internetwork_packet_data(object_buffer, 8, IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node);

				// Send sync packet which tells the player who he is and to start!
				net_ipx_send_rejoin_sync(player_num);

				// Turn off send object mode
				Network_send_objnum = -1;
				Network_send_objects = 0;
				obj_count = 0;
				return;
			} // mode == 1;
		} // i > Highest_object_index
	} // For PACKETS_PER_FRAME
}

void net_ipx_send_rejoin_sync(int player_num)
{
	int i, j;

	Players[player_num].connected = CONNECT_PLAYING; // connect the new guy
	Netgame.players[player_num].LastPacketTime = timer_get_fixed_seconds();

	if (Endlevel_sequence || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		net_ipx_dump_player(IPX_sync_player.player.protocol.ipx.server,IPX_sync_player.player.protocol.ipx.node, DUMP_ENDLEVEL);
		Network_send_objects = 0;
		return;
	}

	if (Network_player_added)
	{
		IPX_sync_player.type = PID_ADDPLAYER;
		IPX_sync_player.player.connected = player_num;
		net_ipx_new_player(&IPX_sync_player);

		for (i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
			{
				net_ipx_send_sequence_packet( IPX_sync_player, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address);
			}
		}
	}

	// Send sync packet to the new guy

	net_ipx_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_ipx_create_monitor_vector();

	send_netgame_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node );
	send_netgame_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node );
	send_netgame_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node );
	net_ipx_send_door_updates();

	return;
}

char * net_ipx_get_player_name( int objnum )
{
	if ( objnum < 0 ) return NULL;
	if ( Objects[objnum].type != OBJ_PLAYER ) return NULL;
	if ( Objects[objnum].id >= MAX_PLAYERS ) return NULL;
	if ( Objects[objnum].id >= N_players ) return NULL;

	return Players[Objects[objnum].id].callsign;
}


void net_ipx_add_player(IPX_sequence_packet *p)
{
	int i;

	for (i=0; i<N_players; i++ )	{
		if ( !memcmp( Netgame.players[i].protocol.ipx.node, p->player.protocol.ipx.node, 6) && !memcmp(Netgame.players[i].protocol.ipx.server, p->player.protocol.ipx.server, 4)){
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )	{
		return;		// too many of em
	}

	memcpy( Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN+1 );
	memcpy( Netgame.players[N_players].protocol.ipx.node, p->player.protocol.ipx.node, 6 );
	memcpy( Netgame.players[N_players].protocol.ipx.server, p->player.protocol.ipx.server, 4 );
	Netgame.players[N_players].connected = CONNECT_PLAYING;
	Players[N_players].connected = CONNECT_PLAYING;
	Netgame.players[N_players].LastPacketTime = timer_get_fixed_seconds();
	N_players++;
	Netgame.numplayers = N_players;

	// Broadcast updated info

	net_ipx_send_game_info(NULL);
}

// One of the players decided not to join the game

void net_ipx_remove_player(IPX_sequence_packet *p)
{
	int i,pn;

	pn = -1;
	for (i=0; i<N_players; i++ )	{
		if (!memcmp(Netgame.players[i].protocol.ipx.node, p->player.protocol.ipx.node, 6) && !memcmp(Netgame.players[i].protocol.ipx.server, p->player.protocol.ipx.server, 4))		{
			pn = i;
			break;
		}
	}

	if (pn < 0 ) return;

	for (i=pn; i<N_players-1; i++ )	{
		memcpy( Netgame.players[i].callsign, Netgame.players[i+1].callsign, CALLSIGN_LEN+1 );
		memcpy( Netgame.players[i].protocol.ipx.node, Netgame.players[i+1].protocol.ipx.node, 6 );
		memcpy( Netgame.players[i].protocol.ipx.server, Netgame.players[i+1].protocol.ipx.server, 4 );
	}

	N_players--;
	Netgame.numplayers = N_players;

	// Broadcast new info

	net_ipx_send_game_info(NULL);

}

void
net_ipx_dump_player(ubyte * server, ubyte *node, int why)
{
	// Remove player from game (not chosen, kicked, ...)

	IPX_Seq.type = PID_DUMP;
	IPX_Seq.player.connected = why;

	net_ipx_send_sequence_packet( IPX_Seq, server, node, NULL);
}

void
net_ipx_send_game_list_request(void)
{
	// Send a broadcast request for game info

	IPX_sequence_packet me;

	memset(&me, 0, sizeof(IPX_sequence_packet));
	memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	memcpy( me.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6 );
	memcpy( me.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );
	me.type = PID_GAME_LIST;

	net_ipx_send_sequence_packet( me, NULL, NULL, NULL);
}

void
net_ipx_update_netgame(void)
{
	// Update the netgame struct with current game variables

	int i, j;

	if (Network_status == NETSTAT_STARTING)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;
	Netgame.max_numplayers = MaxNumNetPlayers;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		Netgame.players[i].connected = Players[i].connected;
		for(j = 0; j < MAX_NUM_NET_PLAYERS; j++)
			Netgame.kills[i][j] = kill_matrix[i][j];
		Netgame.killed[i] = Players[i].net_killed_total;
		Netgame.player_kills[i] = Players[i].net_kills_total;
		Netgame.player_score[i] = Players[i].score;
		Netgame.player_flags[i] = (Players[i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
	Netgame.team_kills[0] = team_kills[0];
	Netgame.team_kills[1] = team_kills[1];
	Netgame.levelnum = Current_level_num;
}

void
net_ipx_send_endlevel_sub(int player_num)
{
	IPX_endlevel_info end;
	int i;
#ifdef WORDS_BIGENDIAN
	int j;
#endif

	// Send an endlevel packet for a player
	end.type 		= PID_ENDLEVEL;
	end.player_num = player_num;
	end.connected	= Players[player_num].connected;
	end.kills       = INTEL_SHORT(Players[player_num].net_kills_total);
	end.killed      = INTEL_SHORT(Players[player_num].net_killed_total);
	memcpy(end.kill_matrix, kill_matrix[player_num], MAX_PLAYERS*sizeof(short));
	//added 05/18/99 Matt Mueller - it doesn't use the rest, but we should at least initialize it :)
	memset(end.kill_matrix[1], 0, (MAX_PLAYERS-1)*MAX_PLAYERS*sizeof(short));
	//end addition -MM
#ifdef WORDS_BIGENDIAN
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			end.kill_matrix[i][j] = INTEL_SHORT(end.kill_matrix[i][j]);
#endif

	if (Players[player_num].connected == CONNECT_PLAYING) // Still playing
	{
		Assert(Control_center_destroyed);
	 	end.seconds_left = Countdown_seconds_left;
	}
	//added 05/18/99 Matt Mueller - similarly, its not used if we aren't connected, but checker complains.
	else
		end.seconds_left = 127;
	//end addition -MM

	for (i = 0; i < N_players; i++)
	{
		if ((i != Player_num) && (i!=player_num) && (Players[i].connected))
		{
			ipxdrv_send_packet_data((ubyte *)&end, sizeof(IPX_endlevel_info), Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address);
		}
	}
}

void
net_ipx_send_endlevel_packet(void)
{
	// Send an updated endlevel status to other hosts

	net_ipx_send_endlevel_sub(Player_num);
}

void
net_ipx_send_game_info(IPX_sequence_packet *their)
{
 	// Send game info to someone who requested it

	char old_type, old_status;

	net_ipx_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.protocol.ipx.Game_pkt_type;
	old_status = Netgame.game_status;

	Netgame.protocol.ipx.Game_pkt_type = PID_GAME_INFO;
	if (Endlevel_sequence || Control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

	if (!their)
		send_netgame_packet(NULL, NULL);
	else
		send_netgame_packet(their->player.protocol.ipx.server, their->player.protocol.ipx.node);

	Netgame.protocol.ipx.Game_pkt_type = old_type;
	Netgame.game_status = old_status;
}

int net_ipx_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

	Assert(Netgame.numplayers > 0);

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		if (Netgame.players[i].connected)
			break;

	Assert(i < MAX_NUM_NET_PLAYERS);

	IPX_Seq.type = PID_REQUEST;
	IPX_Seq.player.connected = Current_level_num;

	net_ipx_send_sequence_packet(IPX_Seq, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, NULL);

	return i;
}

void net_ipx_process_gameinfo(ubyte *data)
{
	int i, j;
	netgame_info *new;

	netgame_info netgame;
	new = &netgame;
	receive_netgame_packet(data, &netgame);

	num_active_ipx_changed = 1;

	for (i = 0; i < num_active_ipx_games; i++)
		if (!strcasecmp(Active_ipx_games[i].game_name, new->game_name) &&
		     (!memcmp(Active_ipx_games[i].players[0].protocol.ipx.node, new->players[0].protocol.ipx.node, 6) &&
		      !memcmp(Active_ipx_games[i].players[0].protocol.ipx.server, new->players[0].protocol.ipx.server, 4)))
			break;

	if (i == IPX_MAX_NETGAMES)
	{
		return;
	}
	memcpy(&Active_ipx_games[i], new, sizeof(netgame_info));
	if (i == num_active_ipx_games)
		num_active_ipx_games++;

	/* Workaround for bug in Descent 1 */
	if (Active_ipx_games[i].max_numplayers == 255)
         {
		if (Active_ipx_games[i].gamemode & GM_MULTI_ROBOTS)
			Active_ipx_games[i].max_numplayers = 4;
		else
			Active_ipx_games[i].max_numplayers = 8;
         }

	if (Active_ipx_games[i].numplayers == 0)
	{
		// Delete this game
		for (j = i; j < num_active_ipx_games-1; j++)
			memcpy(&Active_ipx_games[j], &Active_ipx_games[j+1], sizeof(netgame_info));
		num_active_ipx_games--;
	}
}

void net_ipx_process_dump(IPX_sequence_packet *their)
{
	// Our request for join was denied.  Tell the user why.

	int i;

	if (their->player.connected==DUMP_KICKED)
	{
		for (i=0;i<N_players;i++)
		{
			if (!stricmp (their->player.callsign,Players[i].callsign))
			{
				if (i==multi_who_is_master())
				{
					if (Network_status==NETSTAT_PLAYING)
						multi_leave_game();
					window_set_visible(Game_wind, 0);
					nm_messagebox(NULL, 1, TXT_OK, "%s has kicked you out!",their->player.callsign);
					window_set_visible(Game_wind, 1);
					multi_quit_game = 1;
					game_leave_menus();
					multi_reset_stuff();
				}
				else
				{
					HUD_init_message(HM_MULTI, "%s attempted to kick you out.",their->player.callsign);
				}
			}
		}
	}
	else
	{
		nm_messagebox(NULL, 1, TXT_OK, NET_DUMP_STRINGS(their->player.connected));
		Network_status = NETSTAT_MENU;
 	}
}

void net_ipx_process_request(IPX_sequence_packet *their)
{
	// Player is ready to receieve a sync packet
	int i;

	for (i = 0; i < N_players; i++)
		if (!memcmp(their->player.protocol.ipx.server, Netgame.players[i].protocol.ipx.server, 4) && !memcmp(their->player.protocol.ipx.node, Netgame.players[i].protocol.ipx.node, 6) && (!strcasecmp(their->player.callsign, Netgame.players[i].callsign))) {
			Players[i].connected = CONNECT_PLAYING;
			break;
		}
}

void net_ipx_process_packet(ubyte *data, int length )
{
	IPX_sequence_packet *their = (IPX_sequence_packet *)data;
	IPX_sequence_packet tmp_packet;

	memset(&tmp_packet, 0, sizeof(IPX_sequence_packet));

	net_ipx_receive_sequence_packet(data, &tmp_packet);
	their = &tmp_packet;                                            // reassign their to point to correctly alinged structure

	switch( their->type )	{

	case PID_GAME_INFO:
                if (Network_status == NETSTAT_BROWSING)
                          net_ipx_process_gameinfo(data);
		break;
	case PID_GAME_LIST:
		// Someone wants a list of games
		if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
			if (multi_i_am_master())
				net_ipx_send_game_info(their);
		break;
	case PID_ADDPLAYER:
		net_ipx_new_player(their);
		break;
	case PID_REQUEST:
		if (Network_status == NETSTAT_STARTING)
		{
			// Someone wants to join our game!
			net_ipx_add_player(their);
		}
		else if (Network_status == NETSTAT_WAITING)
		{
			// Someone is ready to recieve a sync packet
			net_ipx_process_request(their);
		}
		else if (Network_status == NETSTAT_PLAYING)
		{
			if (Netgame.RefusePlayers)
				net_ipx_do_refuse_stuff (their);
			else
				net_ipx_welcome_player(their);
		}
		break;
		// Begin addition by GF
	case PID_DUMP:
		if ((Network_status == NETSTAT_WAITING) || (Network_status == NETSTAT_PLAYING))
			net_ipx_process_dump(their);
		break;
		// End addition by GF
        case PID_QUIT_JOINING:
		if (Network_status == NETSTAT_STARTING)
			net_ipx_remove_player( their );
		else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
			net_ipx_stop_resync( their );
		break;
        case PID_SYNC:
                if (Network_status == NETSTAT_WAITING)
        		net_ipx_read_sync_packet(data);
		break;
		case PID_PDATA:
			if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING)) {
				net_ipx_process_pdata((char *)data);
			}
			break;
        case PID_OBJECT_DATA:
		if (Network_status == NETSTAT_WAITING)
			net_ipx_read_object_packet(data);
		break;
	case PID_ENDLEVEL:
		if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
			net_ipx_read_endlevel_packet(data);
		break;
	case PID_PING_SEND:
			net_ipx_ping (PID_PING_RETURN,data[1]);
			break;
	case PID_PING_RETURN:
			net_ipx_handle_ping_return(data[1]);  // data[1] is player who told us of their ping time
			break;
        default:
		Int3(); // Invalid network packet type, see ROB
	}
}

#ifndef NDEBUG
void dump_segments()
{
	PHYSFS_file *fp;

	fp = PHYSFS_openWrite("test.dmp");
	PHYSFS_write(fp, Segments, sizeof(segment), Highest_segment_index + 1);
	PHYSFS_close(fp);
}
#endif

void
net_ipx_read_endlevel_packet( ubyte *data )
{
	// Special packet for end of level syncing

	int playernum;
	IPX_endlevel_info *end = (IPX_endlevel_info *)data;
#ifdef WORDS_BIGENDIAN
	int i, j;

	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			end->kill_matrix[i][j] = INTEL_SHORT(end->kill_matrix[i][j]);
	end->kills = INTEL_SHORT(end->kills);
	end->killed = INTEL_SHORT(end->killed);
#endif

	playernum = end->player_num;

	Assert(playernum != Player_num);
	Assert(playernum < N_players);

	if ((Network_status == NETSTAT_PLAYING) && (end->connected != CONNECT_DISCONNECTED))
		return; // Only accept disconnect packets if we're not out of the level yet

	Players[playernum].connected = end->connected;
	memcpy(&kill_matrix[playernum][0], end->kill_matrix, MAX_PLAYERS*sizeof(short));
	Players[playernum].net_kills_total = end->kills;
	Players[playernum].net_killed_total = end->killed;

	if ((Players[playernum].connected == CONNECT_PLAYING) && (end->seconds_left < Countdown_seconds_left))
		Countdown_seconds_left = end->seconds_left;

	Netgame.players[playernum].LastPacketTime = timer_get_fixed_seconds();
}

void
net_ipx_pack_objects(void)
{
	// Switching modes, pack the object array

	special_reset_objects();
}

int
net_ipx_verify_objects(int remote, int local)
{
	int i;
	int nplayers, got_controlcen=0;

	if ((remote-local) > 10)
		return(-1);

	if (Game_mode & GM_MULTI_ROBOTS)
		got_controlcen = 1;

	nplayers = 0;

	for (i = 0; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))
			nplayers++;
		if (Objects[i].type == OBJ_CNTRLCEN)
			got_controlcen=1;
	}

	if (got_controlcen && (nplayers >= MaxNumNetPlayers))
		return(0);

	return(1);
}

void
net_ipx_read_object_packet( ubyte *data )
{
	// Object from another net player we need to sync with

	short objnum, remote_objnum;
	sbyte obj_owner;
	int segnum, i;
	object *obj;

	static int my_pnum = 0;
	static int mode = 0;
	static int object_count = 0;
	static int frame_num = 0;
	int nobj = data[1];
	int loc = 3;
	int remote_frame_num = data[2];

	frame_num++;

	for (i = 0; i < nobj; i++)
	{
		objnum = INTEL_SHORT(*(short *)(data+loc));			loc += 2;
		obj_owner = data[loc];								loc += 1;
		remote_objnum = INTEL_SHORT(*(short *)(data+loc));	loc += 2;

		if (objnum == -1)
		{
			// Clear object array
			init_objects();
			Network_rejoined = 1;
			my_pnum = obj_owner;
			change_playernum_to(my_pnum);
			mode = 1;
			object_count = 0;
			frame_num = 1;
		}
		else if (objnum == -2)
		{
			// Special debug checksum marker for entire send
			if (mode == 1)
			{
				net_ipx_pack_objects();
				mode = 0;
			}
			if (remote_objnum != object_count) {
				Int3();
			}
			if (net_ipx_verify_objects(remote_objnum, object_count))
			{
				// Failed to sync up
				nm_messagebox(NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
				Network_status = NETSTAT_MENU;
				return;
			}
			frame_num = 0;
		}
		else
		{
			if (frame_num != remote_frame_num)
				Int3();

			object_count++;
			if ((obj_owner == my_pnum) || (obj_owner == -1))
			{
				if (mode != 1)
					Int3(); // SEE ROB
				objnum = remote_objnum;
				//if (objnum > Highest_object_index)
				//{
				//	Highest_object_index = objnum;
				//	num_objects = Highest_object_index+1;
				//}
			}
			else {
				if (mode == 1)
				{
					net_ipx_pack_objects();
					mode = 0;
				}
				objnum = obj_allocate();
			}
			if (objnum != -1) {
				obj = &Objects[objnum];
				if (obj->segnum != -1)
					obj_unlink(objnum);
				Assert(obj->segnum == -1);
				Assert(objnum < MAX_OBJECTS);
				memcpy(obj,data+loc,sizeof(object));
				net_ipx_swap_object(obj);
				loc += sizeof(object);
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;
				obj->attached_obj = -1;
				if (segnum > -1)
					obj_link(obj-Objects,segnum);
				if (obj_owner == my_pnum)
					map_objnum_local_to_local(objnum);
				else if (obj_owner != -1)
					map_objnum_local_to_remote(objnum, remote_objnum, obj_owner);
				else
					object_owner[objnum] = -1;
			}
		} // For a standard onbject
	} // For each object in packet
}

/* Polling loop waiting for sync packet to start game
 * after having sent request
 */
int net_ipx_sync_poll( newmenu *menu, d_event *event, void *userdata )
{
	static fix t1 = 0;
	int rval = 0;

	menu = menu;
	userdata = userdata;

	if (event->type != EVENT_IDLE)
		return 0;

	net_ipx_listen();

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		rval = -2;

	if (Network_status != NETSTAT_MENU && !Network_rejoined && (timer_get_fixed_seconds() > t1+F1_0*2))
	{
		int i;

		// Poll time expired, re-send request

		t1 = timer_get_fixed_seconds();

		i = net_ipx_send_request();
		if (i < 0)
			rval = -2;
	}

	return rval;
}

int net_ipx_start_poll( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int nitems = newmenu_get_nitems(menu);
	int i,n,nm;

	if (event->type != EVENT_IDLE)
		return 0;

	userdata = userdata;

	Assert(Network_status == NETSTAT_STARTING);

	if (!menus[0].value) {
			menus[0].value = 1;
	}

	for (i=1; i<nitems; i++ )	{
		if ( (i>= N_players) && (menus[i].value) )	{
			menus[i].value = 0;
		}
	}

	nm = 0;
	for (i=0; i<nitems; i++ )	{
		if ( menus[i].value )	{
			nm++;
			if ( nm > N_players )	{
				menus[i].value = 0;
			}
		}
	}

	if ( nm > MaxNumNetPlayers )	{
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, MaxNumNetPlayers, TXT_NETPLAYERS_IN );
		// Turn off the last player highlighted
		for (i = N_players; i > 0; i--)
			if (menus[i].value == 1)
			{
				menus[i].value = 0;
				break;
			}
	}

   //added/killed by Victor Rachels to eventually add msging
           //since nitems should not be changing, anyway
//        if (nitems > MAX_PLAYERS ) return;
   //end this section kill - VR

	n = Netgame.numplayers;
	net_ipx_listen();

	if (n < Netgame.numplayers )
	{
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		sprintf( menus[N_players-1].text, "%d. %-16s", N_players, Netgame.players[N_players-1].callsign );

		if (N_players <= MaxNumNetPlayers)
		{
			menus[N_players-1].value = 1;
		}
	}
	else if ( n > Netgame.numplayers )
	{
		// One got removed...

		digi_play_sample(SOUND_HUD_KILL,F1_0);

		for (i=0; i<N_players; i++ )
		{
			sprintf( menus[i].text, "%d. %-16s", i+1, Netgame.players[i].callsign );
			if (i < MaxNumNetPlayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
		}
		for (i=N_players; i<n; i++ )
		{
			sprintf( menus[i].text, "%d. ", i+1 );		// Clear out the deleted entries...
			menus[i].value = 0;
		}
   }

	return 0;
}

static int opt_cinvul, opt_show_on_map;
static int opt_show_on_map, opt_difficulty, opt_socket;

typedef struct param_opt
{
	int name, level, mode, moreopts;
	int closed, refuse, maxnet, coop, team_anarchy;
} param_opt;

int net_ipx_start_game(void);

int net_ipx_game_param_handler( newmenu *menu, d_event *event, param_opt *opt )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	static int oldmaxnet=0;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt->team_anarchy)
			{
				menus[opt->closed].value = 1;
				menus[opt->closed-1].value = 0;
				menus[opt->closed+1].value = 0;
			}

			if (menus[opt->coop].value)
			{
				oldmaxnet=1;

				if (menus[opt->maxnet].value>2)
				{
					menus[opt->maxnet].value=2;
				}

				if (menus[opt->maxnet].max_value>2)
				{
					menus[opt->maxnet].max_value=2;
				}
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				Netgame.max_numplayers = MaxNumNetPlayers = menus[opt->maxnet].value+2;

				if (!(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP))
					Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
			}
			else // if !Coop game
			{
				if (oldmaxnet)
				{
					oldmaxnet=0;
					menus[opt->maxnet].value=6;
					menus[opt->maxnet].max_value=6;
				}
			}

			if (citem == opt->level)
			{
				char *slevel = menus[opt->level].text;

				Netgame.levelnum = atoi(slevel);

				if (!strnicmp(slevel, "s", 1))
					Netgame.levelnum = -atoi(slevel+1);
				else
					Netgame.levelnum = atoi(slevel);

				if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
				{
					nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
					sprintf(slevel, "1");
					return 0;
				}
			}

			if (citem == opt->refuse)
				Netgame.RefusePlayers=menus[opt->refuse].value;

			if (citem == opt->maxnet)
			{
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				MaxNumNetPlayers = menus[opt->maxnet].value+2;
				Netgame.max_numplayers=MaxNumNetPlayers;
			}

			if ((citem >= opt->mode) && (citem <= opt->mode + 3))
			{
				if ( menus[opt->mode].value )
					Netgame.gamemode = NETGAME_ANARCHY;

				else if (menus[opt->mode+1].value) {
					Netgame.gamemode = NETGAME_TEAM_ANARCHY;
				}
		// 		else if (ANARCHY_ONLY_MISSION) {
		// 			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
		// 			menus[opt->mode+2].value = 0;
		// 			menus[opt->mode+3].value = 0;
		// 			menus[opt->mode].value = 1;
		// 			goto menu;
		// 		}
				else if ( menus[opt->mode+2].value )
					Netgame.gamemode = NETGAME_ROBOT_ANARCHY;
				else if ( menus[opt->mode+3].value )
					Netgame.gamemode = NETGAME_COOPERATIVE;
				else Int3(); // Invalid mode -- see Rob
			}

			if (citem == opt->closed)
			{
				if (menus[opt->closed].value)
					Netgame.game_flags |= NETGAME_FLAG_CLOSED;
				else
					Netgame.game_flags &= ~NETGAME_FLAG_CLOSED;
			}
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem==opt->moreopts)
			{
				if ( menus[opt->mode+3].value )
					Game_mode=GM_MULTI_COOP;
				net_ipx_more_game_options();
				Game_mode=0;
				return 1;
			}

			{
				int j;

				for (j = 0; j < num_active_ipx_games; j++)
					if (!stricmp(Active_ipx_games[j].game_name, Netgame.game_name))
					{
						nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DUPLICATE_NAME);
						return 1;
					}

			}

			return !net_ipx_start_game();

		default:
			break;
	}

	return 0;
}

int net_ipx_setup_game()
{
	int i;
	int optnum;
	param_opt opt;
	newmenu_item m[20];
	char slevel[5];
	char level_text[32];
	char srmaxnet[50];

	net_ipx_init();

	change_playernum_to(0);

	for (i=0;i<MAX_PLAYERS;i++)
		if (i!=Player_num)
			Players[i].callsign[0]=0;

	MaxNumNetPlayers = MAX_NUM_NET_PLAYERS;
	Netgame.RefusePlayers=0;
	sprintf( Netgame.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.max_numplayers=MaxNumNetPlayers;
	Netgame.AllowedItems = NETFLAG_DOPOWERUP; // enable all powerups
	Netgame.protocol.ipx.protocol_version = MULTI_PROTO_VERSION;

	strcpy(Netgame.mission_name, Current_mission_filename);
	strcpy(Netgame.mission_title, Current_mission_longname);

	sprintf( slevel, "1" ); Netgame.levelnum = 1;

	optnum = 0;
	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_DESCRIPTION; optnum++;

	opt.name = optnum;
	m[optnum].type = NM_TYPE_INPUT; m[optnum].text = Netgame.game_name; m[optnum].text_len = NETGAME_NAME_LEN; optnum++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
	if (Last_secret_level < -1)
		sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
	else if (Last_secret_level == -1)
		sprintf(level_text+strlen(level_text)-1, ", S1)");

	Assert(strlen(level_text) < 32);

	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = level_text; optnum++;

	opt.level = optnum;
	m[optnum].type = NM_TYPE_INPUT; m[optnum].text = slevel; m[optnum].text_len=4; optnum++;
	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_OPTIONS; optnum++;

	opt.mode = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_ANARCHY); m[optnum].group=0; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_TEAM_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_TEAM_ANARCHY); m[optnum].group=0; opt.team_anarchy=optnum; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY_W_ROBOTS; m[optnum].value=(Netgame.gamemode == NETGAME_ROBOT_ANARCHY); m[optnum].group=0; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_COOPERATIVE; m[optnum].value=(Netgame.gamemode == NETGAME_COOPERATIVE); m[optnum].group=0; opt.coop=optnum; optnum++;

	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = ""; optnum++;

	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Open game"; m[optnum].group=1; m[optnum].value=(!Netgame.RefusePlayers && !Netgame.game_flags & NETGAME_FLAG_CLOSED); optnum++;
	opt.closed = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_CLOSED_GAME; m[optnum].group=1; m[optnum].value=Netgame.game_flags & NETGAME_FLAG_CLOSED; optnum++;
	opt.refuse = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Restricted Game              "; m[optnum].group=1; m[optnum].value=Netgame.RefusePlayers; optnum++;

	opt.maxnet = optnum;
	sprintf( srmaxnet, "Maximum players: %d", MaxNumNetPlayers);
	m[optnum].type = NM_TYPE_SLIDER; m[optnum].value=Netgame.max_numplayers-2; m[optnum].text= srmaxnet; m[optnum].min_value=0;
	m[optnum].max_value=MaxNumNetPlayers-2; optnum++;

	opt.moreopts=optnum;
	m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Advanced options"; optnum++;

	Assert(optnum <= 20);

	i = newmenu_do1( NULL, NULL, optnum, m, (int (*)( newmenu *, d_event *, void * ))net_ipx_game_param_handler, &opt, 1 );

	if (i < 0)
		ipxdrv_close();

	return i >= 0;
}

void
net_ipx_set_game_mode(int gamemode)
{
	Show_kill_list = 1;

	if ( gamemode == NETGAME_ANARCHY )
		Game_mode = GM_NETWORK;
	else if ( gamemode == NETGAME_ROBOT_ANARCHY )
		Game_mode = GM_NETWORK | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_COOPERATIVE )
		Game_mode = GM_NETWORK | GM_MULTI_COOP | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_TEAM_ANARCHY )
	{
		Game_mode = GM_NETWORK | GM_TEAM;
                Show_kill_list = 3;
	}
	else
		Int3();
}

int
net_ipx_find_game(void)
{
	// Find out whether or not there is space left on this socket

	fix t1;

	Network_status = NETSTAT_BROWSING;

	num_active_ipx_games = 0;

	show_boxed_message(TXT_WAIT, 0);

	net_ipx_send_game_list_request();
	t1 = timer_get_fixed_seconds() + F1_0*2;

	while (timer_get_fixed_seconds() < t1) // Wait 3 seconds for replies
		net_ipx_listen();

	if (num_active_ipx_games < IPX_MAX_NETGAMES)
		return 0;
	return 1;
}

void net_ipx_read_sync_packet( ubyte * data )
{
	int i, j;
	netgame_info *sp = &Netgame;

	char temp_callsign[CALLSIGN_LEN+1];

	// This function is now called by all people entering the netgame.

	if (data)
	{
		receive_netgame_packet( data, &Netgame );
	}

	N_players = sp->numplayers;
	Difficulty_level = sp->difficulty;
	Network_status = sp->game_status;

	// New code, 11/27

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
#ifdef NDEBUG
		return;
#endif
	}

	// Discover my player number

	memcpy(temp_callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);

	Player_num = -1;

	for (i=0; i<MAX_NUM_NET_PLAYERS; i++ )	{
		Players[i].net_kills_total = 0;
//added/uncommented on 11/12/98 by Victor Rachels... who knows
                Players[i].net_killed_total = 0;
//end this section uncomment - VR
	}


	for (i=0; i<N_players; i++ )	{
		if ((!memcmp( sp->players[i].protocol.ipx.node, IPX_Seq.player.protocol.ipx.node, 6 )) &&
                         (!strcasecmp( sp->players[i].callsign, temp_callsign)) )
		{
			Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
			change_playernum_to(i);
		}
		memcpy( Players[i].callsign, sp->players[i].callsign, CALLSIGN_LEN+1 );

#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy(&server, TempPlayersInfo->players[i].protocol.ipx.server, 4);
		if (server != 0)
			ipxdrv_get_local_target((ubyte *)&server, sp->players[i].protocol.ipx.node, Players[i].net_address);
#else // WORDS_NEED_ALIGNMENT
		if ( (*(uint *)sp->players[i].protocol.ipx.server) != 0 )
			ipxdrv_get_local_target( sp->players[i].protocol.ipx.server, sp->players[i].protocol.ipx.node, Players[i].net_address );
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy( Players[i].net_address, sp->players[i].protocol.ipx.node, 6 );

		Players[i].n_packets_got=0;				// How many packets we got from them
		Players[i].n_packets_sent=0;				// How many packets we sent to them
		Players[i].connected = sp->players[i].connected;
                //added/edited on 11/12/98 by Victor Rachels += -> =
                Players[i].net_kills_total = sp->player_kills[i];
                //end this section edit - VR

		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = sp->player_score[i];

		for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		{
			kill_matrix[i][j] = sp->kills[i][j];
		}
	}

	if ( Player_num < 0 )	{
		Network_status = NETSTAT_MENU;
		return;
	}

//added/edited on 11/12/98 by Victor Rachels to hopefully fix joining
//I wonder how much else is foobarred like this?
//        if (Network_rejoined)
		for (i=0; i<N_players;i++)
			Players[i].net_killed_total = sp->killed[i];
//added/killed on 1/5/99 by Victor Rachels to fix next level
//-killed         if(!Network_rejoined)
//-killed          Players[Player_num].net_killed_total = 0;
//end this section kill
//end this section addition/edit - VR
//added/edited on 11/12/98 by Victor Rachels to hopefully fix eff
        PlayerCfg.NetlifeKills -= Players[Player_num].net_kills_total;
        PlayerCfg.NetlifeKilled -= Players[Player_num].net_killed_total;
//end this section addition - VR

	if (Network_rejoined) {
		net_ipx_process_monitor_vector(sp->monitor_vector);
		Players[Player_num].time_level = sp->level_time;
	}

	team_kills[0] = sp->team_kills[0];
	team_kills[1] = sp->team_kills[1];

	Players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].connected = CONNECT_PLAYING;

	if (!Network_rejoined)
		for (i=0; i<MaxNumNetPlayers; i++) {
			Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
			Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
		 	obj_relink(Players[i].objnum,Player_init[Netgame.locations[i]].segnum);
		}

	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	if (data) { // adb: master does have this info
		Netgame.PacketsPerSec = 10;
	}
	multi_allow_powerup = NETFLAG_DOPOWERUP;

        Network_status = NETSTAT_PLAYING;
	multi_sort_kill_list();

}

void
net_ipx_abort_game(void)
{
        int i;
        for (i=1; i<N_players; i++)
                net_ipx_dump_player(Netgame.players[i].protocol.ipx.server,Netgame.players[i].protocol.ipx.node, DUMP_ABORTED);

	N_players = Netgame.numplayers = 0;
	net_ipx_send_game_info(NULL); // Tell everyone we're bailing
}

int
net_ipx_send_sync(void)
{
	int i, j, np;

	// Randomize their starting locations...

	if (NumNetPlayerPositions < MaxNumNetPlayers) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Not enough start positions\n(need %d got %d)\nNetgame aborted", MaxNumNetPlayers, NumNetPlayerPositions);
		net_ipx_abort_game();
		return -1;
	}

        //added/changed on 9/13/98 by adb to remove TICKER
        d_srand( timer_get_fixed_seconds() );
        //end change - adb

        for (i=0; i<MaxNumNetPlayers; i++ )
	{
		if (Players[i].connected)
			Players[i].connected = CONNECT_PLAYING; // Get rid of endlevel connect statuses
		if (Game_mode & GM_MULTI_COOP)
			Netgame.locations[i] = i;
		else {
			do
			{
				np = d_rand() % NumNetPlayerPositions;
				for (j=0; j<i; j++ )
				{
					if (Netgame.locations[j]==np)
					{
						np =-1;
						break;
					}
				}
			} while (np<0);
			// np is a location that is not used anywhere else..
			Netgame.locations[i]=np;
		}
	}

	// Push current data into the sync packet

	net_ipx_update_netgame();
	Netgame.game_status = NETSTAT_PLAYING;
	Netgame.protocol.ipx.Game_pkt_type = PID_SYNC;
	Netgame.segments_checksum = my_segments_checksum;

	for (i=0; i<N_players; i++ )	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;

		send_netgame_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node);
		send_netgame_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node);
		send_netgame_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node);
	}
	net_ipx_read_sync_packet(NULL); // Read it myself, as if I had sent it
	return 0;
}

int
net_ipx_select_teams(void)
{
	newmenu_item m[MAX_PLAYERS+4];
	int choice, opt, opt_team_b;
	ubyte team_vector = 0;
	char team_names[2][CALLSIGN_LEN+1];
	int i;
	int pnums[MAX_PLAYERS+2];

	// One-time initialization

	for (i = N_players/2; i < N_players; i++) // Put first half of players on team A
	{
		team_vector |= (1 << i);
	}

	sprintf(team_names[0], "%s", TXT_BLUE);
	sprintf(team_names[1], "%s", TXT_RED);

	// Here comes da menu
menu:
	m[0].type = NM_TYPE_INPUT; m[0].text = team_names[0]; m[0].text_len = CALLSIGN_LEN;

	opt = 1;
	for (i = 0; i < N_players; i++)
	{
		if (!(team_vector & (1 << i)))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	opt_team_b = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = team_names[1]; m[opt].text_len = CALLSIGN_LEN; opt++;
	for (i = 0; i < N_players; i++)
	{
		if (team_vector & (1 << i))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;
	m[opt].type = NM_TYPE_MENU; m[opt].text = TXT_ACCEPT; opt++;

	Assert(opt <= MAX_PLAYERS+4);

	choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, NULL, NULL);

	if (choice == opt-1)
	{
		if ((opt-2-opt_team_b < 2) || (opt_team_b == 1))
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
			goto menu;
		}

		Netgame.team_vector = team_vector;
		strcpy(Netgame.team_name[0], team_names[0]);
		strcpy(Netgame.team_name[1], team_names[1]);
		return 1;
	}

	else if ((choice > 0) && (choice < opt_team_b)) {
		team_vector |= (1 << pnums[choice]);
	}
	else if ((choice > opt_team_b) && (choice < opt-2)) {
		team_vector &= ~(1 << pnums[choice]);
	}
	else if (choice == -1)
		return 0;
	goto menu;
}

int
net_ipx_select_players(void)
{
        int i, j, opts, opt_msg;
        newmenu_item m[MAX_PLAYERS+1];
	char text[MAX_PLAYERS][25];
	char title[50];
	int save_nplayers;

	net_ipx_add_player( &IPX_Seq );

	for (i=0; i< MAX_PLAYERS; i++ )	{
		sprintf( text[i], "%d.  %-16s", i+1, "" );
		m[i].type = NM_TYPE_CHECK; m[i].text = text[i]; m[i].value = 0;
	}
//added/edited on 11/7/98 by Victor Rachels in an attempt to get msgs going.
        opts=MAX_PLAYERS;
        opt_msg = opts;
//killed for now to not raise people's hopes - 11/10/98 - VR
//        m[opts].type = NM_TYPE_MENU; m[opts].text = "Send message..."; opts++;

	m[0].value = 1;				// Assume server will play...

	sprintf( text[0], "%d. %-16s", 1, Players[Player_num].callsign );
	sprintf( title, "%s %d %s", TXT_TEAM_SELECT, MaxNumNetPlayers, TXT_TEAM_PRESS_ENTER );

GetPlayersAgain:

        j=opt_msg;
         while(j==opt_msg)
          {
            j=newmenu_do1( NULL, title, opts, m, net_ipx_start_poll, NULL, 1 );

            if(j==opt_msg)
             {
              multi_send_message_dialog();
               if (Network_message_reciever != -1)
                multi_send_message();
             }
          }
//end this section addition
	save_nplayers = N_players;

	if (j<0)
	{
		// Aborted!
		// Dump all players and go back to menu mode

abort:
		net_ipx_abort_game();

		Network_status = NETSTAT_MENU;
		return(0);
	}

	// Count number of players chosen

	N_players = 0;
	for (i=0; i<save_nplayers; i++ )
	{
		if (m[i].value)
			N_players++;
	}

	if ( N_players > MaxNumNetPlayers) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, MaxNumNetPlayers, TXT_NETPLAYERS_IN );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

#ifdef NDEBUG
        if ( N_players < 2 )    {
                nm_messagebox( TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO );
		N_players = save_nplayers;
		goto GetPlayersAgain;
        }
#endif

#ifdef NDEBUG
	if ( (Netgame.gamemode == NETGAME_TEAM_ANARCHY) && (N_players < 3) ) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_THREE );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
	for (i=0; i<save_nplayers; i++ )	{
		if (m[i].value)
		{
			if (i > N_players)
			{
				memcpy(Netgame.players[N_players].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1);
				memcpy(Netgame.players[N_players].protocol.ipx.node, Netgame.players[i].protocol.ipx.node, 6);
				memcpy(Netgame.players[N_players].protocol.ipx.server, Netgame.players[i].protocol.ipx.server, 4);
			}
			Players[N_players].connected = CONNECT_PLAYING;
			N_players++;
		}
		else
		{
			net_ipx_dump_player(Netgame.players[i].protocol.ipx.server,Netgame.players[i].protocol.ipx.node, DUMP_DORK);
		}
	}

	for (i = N_players; i < MAX_NUM_NET_PLAYERS; i++) {
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN+1);
		memset(Netgame.players[i].protocol.ipx.node, 0, 6);
		memset(Netgame.players[i].protocol.ipx.server, 0, 4);
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
		if (!net_ipx_select_teams())
			goto abort;

	return(1);
}

int net_ipx_start_game(void)
{
	Assert( sizeof(IPX_frame_info) < MAX_DATA_SIZE );

	if ( !IPX_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND );
		return 0;
	}

	if (net_ipx_find_game())
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_NET_FULL);
		return 0;
	}

	if (IPX_Socket) {
		ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
	}

	N_players = 0;

        Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	net_ipx_set_game_mode(Netgame.gamemode);
	MaxNumNetPlayers = Netgame.max_numplayers;
	Netgame.protocol.ipx.protocol_version = MULTI_PROTO_VERSION;

	Network_status = NETSTAT_STARTING;

	if(net_ipx_select_players())
	{
		StartNewLevel(Netgame.levelnum);
	}
	else
	{
		Game_mode = GM_GAME_OVER;
		return 0;	// see if we want to tweak the game we setup
	}

	return 1;	// don't keep params menu or mission listbox (may want to join a game next time)
}

void restart_net_searching(newmenu_item * m)
{
	int i;
	N_players = 0;
	num_active_ipx_games = 0;

	memset(Active_ipx_games, 0, sizeof(netgame_info)*IPX_MAX_NETGAMES);

	for (i = 0; i < IPX_MAX_NETGAMES; i++)
	{
		sprintf(m[i+2].text, "%d.                                                         ",i+1);
	}

	num_active_ipx_changed = 1;
}

void net_ipx_join_listen(newmenu *menu)
{
	newmenu_item *menus = newmenu_get_items(menu);
	static fix t1 = 0;
	int i,join_status,temp;

	// send a request for game info every 3 seconds
	if (timer_get_fixed_seconds() > t1+F1_0*3) {
		t1 = timer_get_fixed_seconds();
		net_ipx_send_game_list_request();
	}

	temp=num_active_ipx_games;

	net_ipx_listen();

	if (!num_active_ipx_changed)
		return;

	if (temp!=num_active_ipx_games)
		digi_play_sample (SOUND_HUD_MESSAGE,F1_0);

	num_active_ipx_changed = 0;

	// Copy the active games data into the menu options
	for (i = 0; i < num_active_ipx_games; i++)
	{
		int game_status = Active_ipx_games[i].game_status;
		int j,x, k,tx,ty,ta,nplayers = 0;
		char levelname[8],MissName[25],GameName[25],thold[2];
		thold[1]=0;

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_ipx_games[i].mission_title[j]=='\t')
				continue;
			thold[0]=Active_ipx_games[i].mission_title[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				MissName[k]=MissName[k+1]=MissName[k+2]='.';
				k+=3;
				break;
			}

			MissName[k++]=Active_ipx_games[i].mission_title[j];
		}
		MissName[k]=0;

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_ipx_games[i].game_name[j]=='\t')
				continue;
			thold[0]=Active_ipx_games[i].game_name[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				GameName[k]=GameName[k+1]=GameName[k+2]='.';
				k+=3;
				break;
			}
			GameName[k++]=Active_ipx_games[i].game_name[j];
		}
		GameName[k]=0;


		for (j = 0; j < Active_ipx_games[i].numplayers; j++)
			if (Active_ipx_games[i].players[j].connected)
				nplayers++;

		if (Active_ipx_games[i].levelnum < 0)
			sprintf(levelname, "S%d", -Active_ipx_games[i].levelnum);
		else
			sprintf(levelname, "%d", Active_ipx_games[i].levelnum);

		if (game_status == NETSTAT_STARTING)
		{
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
					 Active_ipx_games[i].max_numplayers,MissName,levelname,"Forming");
		}
		else if (game_status == NETSTAT_PLAYING)
		{
			join_status=net_ipx_can_join_netgame(&Active_ipx_games[i]);

			if (join_status==1)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Open");
			else if (join_status==2)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Full");
			else if (join_status==3)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Restrict");
			else
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Closed");

		}
		else
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,MODE_NAMES(Active_ipx_games[i].gamemode),nplayers,
					 Active_ipx_games[i].max_numplayers,MissName,levelname,"Between");


		Assert(strlen(menus[i+2].text) < 100);
	}

	for (i = num_active_ipx_games; i < IPX_MAX_NETGAMES; i++)
	{
		sprintf(menus[i+2].text, "%d.                                                                      ",i+1);
	}
}

int net_ipx_do_join_game(int choice);

int net_ipx_join_poll( newmenu *menu, d_event *event, void *menu_text )
{
	// Polling loop for Join Game menu
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	int key = 0;

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			key = ((d_event_keycommand *)event)->keycode;

			if ( IPX_allow_socket_changes ) {
				int osocket;
				int rval = 0;

				osocket = IPX_Socket;

				if ( key==KEY_PAGEDOWN )       { IPX_Socket--; rval = 1; }
				if ( key==KEY_PAGEUP )         { IPX_Socket++; rval = 1; }

				if (IPX_Socket>99)
					IPX_Socket=99;
				if (IPX_Socket<-99)
					IPX_Socket=-99;

				if ( IPX_Socket+IPX_DEFAULT_SOCKET > 0x8000 )
					IPX_Socket  = 0x8000 - IPX_DEFAULT_SOCKET;

				if ( IPX_Socket+IPX_DEFAULT_SOCKET < 0 )
					IPX_Socket  = IPX_DEFAULT_SOCKET;

				if (IPX_Socket != osocket )         {
					sprintf( menus[0].text, "\t%s %+d (PgUp/PgDn to change)", TXT_CURRENT_IPX_SOCKET, IPX_Socket );
					net_ipx_listen();
					ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
					restart_net_searching(menus);
					net_ipx_send_game_list_request();
					return rval;
				}
			}
			break;

		case EVENT_IDLE:
			net_ipx_join_listen(menu);
			break;

		case EVENT_NEWMENU_SELECTED:
			citem-=2;

			if (citem >=num_active_ipx_games)
			{
				nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
				return 1;
			}

			if (net_ipx_show_game_stats(citem)==0)
				return 1;

			// Choice has been made and looks legit
			if (net_ipx_do_join_game(citem)==0)
				return 1;

			// look ma, we're in a game!!!
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(menu_text);
			d_free(menus);
			if (!Game_wind)
			{
				ipxdrv_close();
				Network_status = NETSTAT_MENU;	// they cancelled
			}
			break;

		default:
			break;
	}

	return 0;
}

int
net_ipx_wait_for_sync(void)
{
	char text[60];
	newmenu_item m[2];
	int i, choice=0;

	Network_status = NETSTAT_WAITING;
	m[0].type=NM_TYPE_TEXT; m[0].text = text;
	m[1].type=NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;

	i = net_ipx_send_request();

	if (i < 0)
		return(-1);

	sprintf( m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER );

	while (choice > -1)
		choice=newmenu_do( NULL, TXT_WAIT, 2, m, net_ipx_sync_poll, NULL );


	if (Network_status != NETSTAT_PLAYING)
	{
		IPX_sequence_packet me;

		memset(&me, 0, sizeof(IPX_sequence_packet));
		me.type = PID_QUIT_JOINING;
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		memcpy( me.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6 );
		memcpy( me.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );
		net_ipx_send_sequence_packet( me, Netgame.players[0].protocol.ipx.server, Netgame.players[0].protocol.ipx.node, NULL );
		N_players = 0;
		Game_mode = GM_GAME_OVER;
		return(-1);	// they cancelled
	}
	return(0);
}

int net_ipx_request_poll( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

	if (event->type != EVENT_IDLE)
		return 0;

	menu = menu;
	userdata = userdata;

	// Send our endlevel packet at regular intervals

//	if (timer_get_fixed_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
//	{
//		net_ipx_send_endlevel_packet();
//		t1 = timer_get_fixed_seconds();
//	}

	net_ipx_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected == CONNECT_PLAYING) || (Players[i].connected == CONNECT_DISCONNECTED))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		return -2;
	}

	return 0;
}

int net_ipx_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	newmenu_item m[1];

	Network_status = NETSTAT_WAITING;

	m[0].type=NM_TYPE_TEXT; m[0].text = TXT_NET_LEAVE;

	Network_status = NETSTAT_WAITING;
	net_ipx_flush();

	Players[Player_num].connected = CONNECT_PLAYING;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 1, m, net_ipx_request_poll, NULL);

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return 0;
		if (choice != 0)
			goto menu;

		// User confirmed abort

		for (i=0; i < N_players; i++)
			if ((Players[i].connected != CONNECT_DISCONNECTED) && (i != Player_num))
				net_ipx_dump_player(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, DUMP_ABORTED);

		return -1;
	}
	else if (choice != -2)
		goto menu;

	return 0;
}

int
net_ipx_level_sync(void)
{
 	// Do required syncing between (before) levels

	int result = 0;

	MySyncPackInitialized = 0;

//	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));

	net_ipx_flush(); // Flush any old packets

	if (N_players == 0)
		result = net_ipx_wait_for_sync();
	else if (multi_i_am_master())
	{
		result = net_ipx_wait_for_requests();
		if (!result)
			result = net_ipx_send_sync();
	}
	else
		result = net_ipx_wait_for_sync();

	if (result)
	{
		Players[Player_num].connected = CONNECT_DISCONNECTED;
		net_ipx_send_endlevel_packet();
		if (Game_wind)
			window_close(Game_wind);
		show_menus();
		ipxdrv_close();
		return -1;
	}
	return(0);
}

int net_ipx_do_join_game(int choice)
{
	if (Active_ipx_games[choice].game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	if (Active_ipx_games[choice].protocol.ipx.protocol_version != MULTI_PROTO_VERSION)
	{
        nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		return 0;
	}

	if (!load_mission_by_name(Active_ipx_games[choice].mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}

	if (!net_ipx_can_join_netgame(&Active_ipx_games[choice]))
	{
		if (Active_ipx_games[choice].numplayers == Active_ipx_games[choice].max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return 0;
	}

	// Choice is valid, prepare to join in
	memcpy(&Netgame, &Active_ipx_games[choice], sizeof(netgame_info));

	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	net_ipx_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);

	return 1;     // look ma, we're in a game!!!
}

void net_ipx_join_game()
{
	int i;
	char *menu_text;
	newmenu_item *m;

	if ( !IPX_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	MALLOC(m, newmenu_item, ((IPX_MAX_NETGAMES)*2)+1);
	if (!m)
		return;
	MALLOC(menu_text, char, (((IPX_MAX_NETGAMES)*2)+1)*74);
	if (!menu_text)
	{
		d_free(m);
		return;
	}

	net_ipx_init();

	N_players = 0;

	Network_send_objects = 0;
	Network_rejoined=0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	net_ipx_flush();
	net_ipx_listen();  // Throw out old info

	net_ipx_send_game_list_request(); // broadcast a request for lists

	num_active_ipx_games = 0;

	memset(m, 0, sizeof(newmenu_item)*IPX_MAX_NETGAMES);
	memset(Active_ipx_games, 0, sizeof(netgame_info)*IPX_MAX_NETGAMES);

	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	m[0].text = menu_text;
	m[0].type = NM_TYPE_TEXT;
	if (IPX_allow_socket_changes)
		sprintf( m[0].text, "\tCurrent IPX Socket is default %+d (PgUp/PgDn to change)", IPX_Socket );
	else
		strcpy( m[0].text, "" ); //sprintf( m[0].text, "" );

	m[1].text=menu_text + 74*1;
	m[1].type=NM_TYPE_TEXT;
	sprintf (m[1].text,"\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (i = 0; i < IPX_MAX_NETGAMES; i++) {
		m[i+2].text = menu_text + 74*(i+2);
		m[i+2].type = NM_TYPE_MENU;
		sprintf(m[i+2].text, "%d.                                                                      ", i+1);
	}

	num_active_ipx_changed = 1;
	newmenu_dotiny("NETGAMES", NULL,IPX_MAX_NETGAMES+2, m, 1, net_ipx_join_poll, menu_text);
}

void net_ipx_leave_game()
{
	net_ipx_do_frame(1, 1);

	if ((multi_i_am_master()) &&
	    (Netgame.numplayers == 1 || Network_status == NETSTAT_STARTING))
	{
		N_players = Netgame.numplayers = 0;
		net_ipx_send_game_info(NULL);
	}

	Players[Player_num].connected = CONNECT_DISCONNECTED;
	net_ipx_send_endlevel_packet();
	change_playernum_to(0);
	net_ipx_flush();
	ipxdrv_close();
}

void net_ipx_flush()
{
	ubyte packet[MAX_DATA_SIZE];

	if (!IPX_active)
		return;

	while (ipxdrv_get_packet_data(packet) > 0)
		;
}

void net_ipx_listen()
{
	int size;
	ubyte packet[MAX_DATA_SIZE];

	if (!IPX_active) return;

	size = ipxdrv_get_packet_data( packet );
	while ( size > 0 )	{
		net_ipx_process_packet( packet, size );
		size = ipxdrv_get_packet_data( packet );
	}
}

void net_ipx_send_data( ubyte * ptr, int len, int urgent )
{
	char check;

	if (Endlevel_sequence)
		return;

	if (!MySyncPackInitialized)	{
		MySyncPackInitialized = 1;
		memset( &MySyncPack, 0, sizeof(IPX_frame_info) );
	}

	if (urgent)
		PacketUrgent = 1;

	if ((MySyncPack.data_size+len) > NET_XDATA_SIZE )	{
		check = ptr[0];
		net_ipx_do_frame(1, 0);
		if (MySyncPack.data_size != 0) {
			Int3();
		}
		Assert(check == ptr[0]);
	}

	Assert(MySyncPack.data_size+len <= NET_XDATA_SIZE);

	memcpy( &MySyncPack.data[MySyncPack.data_size], ptr, len );
	MySyncPack.data_size += len;
}

void net_ipx_timeout_player(int playernum)
{
	// Remove a player from the game if we haven't heard from them in
	// a long time.
	int i, n = 0;

	Assert(playernum < N_players);
	Assert(playernum > -1);

	net_ipx_disconnect_player(playernum);
	create_player_appearance_effect(&Objects[Players[playernum].objnum]);

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	HUD_init_message(HM_MULTI, "%s %s", Players[playernum].callsign, TXT_DISCONNECTING);
	for (i = 0; i < N_players; i++)
		if (Players[i].connected)
			n++;

	if (n == 1)
	{
//added/changed on 10/11/98 by Victor Rachels cuz this is annoying as a box
//-killed-                nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
          HUD_init_message(HM_MULTI, "You are the only person remaining in this netgame");
//end this section change - VR
	}
}

void net_ipx_do_frame(int force, int listen)
{
	int i;
	static fix last_send_time = 0, last_timeout_check = 0;

	if (!(Game_mode&GM_NETWORK)) return;

	net_ipx_ping_all(timer_get_fixed_seconds());

	if ((Network_status != NETSTAT_PLAYING) || (Endlevel_sequence)) // Don't send postion during escape sequence...
		goto listen;

	if (WaitForRefuseAnswer && timer_get_fixed_seconds()>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	last_send_time += FrameTime;
	last_timeout_check += FrameTime;

	// Send out packet 10 times per second maximum... unless they fire, then send more often...
        if ( (last_send_time > (F1_0 / Netgame.PacketsPerSec)) ||
	     (Network_laser_fired) || force || PacketUrgent )	    {
		if ( Players[Player_num].connected )	{
			int objnum = Players[Player_num].objnum;

			if (listen) {
				multi_send_robot_frame(0);
                multi_send_fire();              // Do firing if needed..
			}

			last_send_time = 0;

			{
				int send_data_size, i;

				MySyncPack.numpackets					= Players[0].n_packets_sent++;
				MySyncPack.type                                 = PID_PDATA;
				MySyncPack.playernum                    = Player_num;
				MySyncPack.obj_render_type              = Objects[objnum].render_type;
				MySyncPack.level_num                    = Current_level_num;
				MySyncPack.obj_segnum                   = Objects[objnum].segnum;
				MySyncPack.obj_pos                              = Objects[objnum].pos;
				MySyncPack.obj_orient                   = Objects[objnum].orient;
				MySyncPack.phys_velocity                = Objects[objnum].mtype.phys_info.velocity;
				MySyncPack.phys_rotvel                  = Objects[objnum].mtype.phys_info.rotvel;

				send_data_size = MySyncPack.data_size;                  // do this so correct size data is sent

// 				ipxdrvSendGamePacket((ubyte*)&MySyncPack, sizeof(IPX_frame_info) - NET_XDATA_SIZE + send_data_size);
				for(i=0; i<N_players; i++)
				{
					if(Players[i].connected && (i != Player_num))
					{
						ipxdrv_send_packet_data((ubyte *)&MySyncPack, sizeof(IPX_frame_info) - NET_XDATA_SIZE + (&MySyncPack)->data_size, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address);
					}
				}
			}

			PacketUrgent = 0;
			MySyncPack.data_size = 0;		// Start data over at 0 length.

			if (Control_center_destroyed)
				net_ipx_send_endlevel_packet();
		}
	}

	if (!listen)
		return;

	if ((last_timeout_check > F1_0) && !(Control_center_destroyed))
	{
		fix approx_time = timer_get_fixed_seconds();
		// Check for player timeouts
		for (i = 0; i < N_players; i++)
		{
			if ((i != Player_num) && (Players[i].connected == CONNECT_PLAYING))
			{
				if ((Netgame.players[i].LastPacketTime == 0) || (Netgame.players[i].LastPacketTime > approx_time))
				{
					Netgame.players[i].LastPacketTime = approx_time;
					continue;
				}
				if ((approx_time - Netgame.players[i].LastPacketTime) > IPX_TIMEOUT)
					net_ipx_timeout_player(i);
			}

//added on 11/18/98 by Victor Rachels to hack ghost disconnects
                        if(Players[i].connected != CONNECT_PLAYING && Objects[Players[i].objnum].type != OBJ_GHOST)
                         {
                          HUD_init_message(HM_MULTI, "%s has left.", Players[i].callsign);
                          multi_make_player_ghost(i);
                         }
//end this section addition - VR

		}
		last_timeout_check = 0;
	}

listen:
	if (!listen)
	{
		MySyncPack.data_size = 0;
		return;
	}
	net_ipx_listen();

	if (Network_send_objects)
		net_ipx_send_objects();
}

void net_ipx_process_pdata (char *data)
 {
  Assert (Game_mode & GM_NETWORK);

	net_ipx_read_pdata_packet ((IPX_frame_info *) data);
 }

void net_ipx_read_pdata_packet(IPX_frame_info *pd)
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;

	TheirPlayernum = pd->playernum;
	TheirObjnum = Players[pd->playernum].objnum;

	if (TheirPlayernum < 0) {
		Int3(); // This packet is bogus!!
		return;
	}

	if (!multi_quit_game && (TheirPlayernum >= N_players))
	{
		if (Network_status!=NETSTAT_WAITING)
		{
			Int3(); // We missed an important packet!
			multi_consistency_error(0);
			return;
		}
		else
			return;
	}

	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL) ) {
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		if ( pd->data_size>0 )
		{
			// pass pd->data to some parser function....
			multi_process_bigdata( pd->data, pd->data_size );
		}
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	if ((sbyte)pd->level_num != Current_level_num)
	{
		return;
	}

	TheirObj = &Objects[TheirObjnum];

	//------------- Keep track of missed packets -----------------
	Players[TheirPlayernum].n_packets_got++;
	Netgame.players[TheirPlayernum].LastPacketTime = timer_get_fixed_seconds();
	if  ( pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------
	TheirObj->pos                           = pd->obj_pos;
	TheirObj->orient                        = pd->obj_orient;
	TheirObj->mtype.phys_info.velocity = pd->phys_velocity;
	TheirObj->mtype.phys_info.rotvel = pd->phys_rotvel;

	if ((TheirObj->render_type != pd->obj_render_type) && (pd->obj_render_type == RT_POLYOBJ))
		multi_make_ghost_player(TheirPlayernum);

	obj_relink(TheirObjnum,pd->obj_segnum);

	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);

	//------------ Welcome them back if reconnecting --------------
	if (!Players[TheirPlayernum].connected) {
		Players[TheirPlayernum].connected = CONNECT_PLAYING;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0);

		HUD_init_message(HM_MULTI,  "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );

		multi_send_score();
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )  {
		// pass pd->data to some parser function....
		multi_process_bigdata( pd->data, pd->data_size );
	}

}

int net_ipx_more_options_handler( newmenu *menu, d_event *event, void *userdata );

void net_ipx_more_game_options ()
{
	int opt=0,i;
	char srinvul[50],socket_string[5];
	newmenu_item m[21];

	sprintf (socket_string,"%d",IPX_Socket);

	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;

	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.control_invul_time/5/F1_0/60; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_show_on_map=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP); opt_show_on_map=opt; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network socket"; opt++;
	opt_socket = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = socket_string; m[opt].text_len=4; opt++;

	i = newmenu_do1( NULL, "Advanced netgame options", opt, m, net_ipx_more_options_handler, NULL, 0 );

	Netgame.control_invul_time = m[opt_cinvul].value*5*F1_0*60;

	if ((atoi(socket_string))!=IPX_Socket)
	{
		IPX_Socket=atoi(socket_string);
		ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
	}

	Netgame.difficulty=Difficulty_level = m[opt_difficulty].value;
	if (m[opt_show_on_map].value)
		Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
	else
		Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
}

int net_ipx_more_options_handler( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (newmenu_get_citem(menu) == opt_cinvul)
				sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
			break;

		default:
			break;
	}

	userdata = userdata;

	return 0;
}

void net_ipx_ping (ubyte flag,int pnum)
{
	ubyte mybuf[2];

	mybuf[0]=flag;
	mybuf[1]=Player_num;
	*(u_int32_t*)(multibuf+2)=INTEL_INT(GameTime);
	ipxdrv_send_packet_data( (ubyte *)mybuf, 7, Netgame.players[pnum].protocol.ipx.server, Netgame.players[pnum].protocol.ipx.node,Players[pnum].net_address );
}

static fix PingLaunchTime[MAX_PLAYERS],PingReturnTime[MAX_PLAYERS];

void net_ipx_handle_ping_return (ubyte pnum)
{
	if (PingLaunchTime[pnum]==0 || pnum>=N_players)
	{
		return;
	}

	PingReturnTime[pnum]=GameTime;
	Netgame.players[pnum].ping=f2i(fixmul(PingReturnTime[pnum]-PingLaunchTime[pnum],i2f(1000)));
	PingLaunchTime[pnum]=0;
}

// ping all connected players (except yourself) in 3sec interval and update ping_table
void net_ipx_ping_all(fix time)
{
	int i;
	static fix PingTime=0;

	Netgame.players[Player_num].ping = -1; // Set mine to fancy -1 because I am super fast! Weeee

	if (PingTime+(F1_0*3)<time || PingTime > time)
	{
		for (i=0; i<MAX_PLAYERS; i++)
		{
			if (Players[i].connected && i != Player_num)
			{
				PingLaunchTime[i]=time;
				net_ipx_ping (PID_PING_SEND,i);
			}
		}
		PingTime=time;
	}
}

void net_ipx_do_refuse_stuff (IPX_sequence_packet *their)
{
	int i;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		if (!strcmp (their->player.callsign,Players[i].callsign))
		{
			net_ipx_welcome_player(their);
				return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
			{
				net_ipx_welcome_player(their);
					return;
			}
		}

		digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);

		HUD_init_message(HM_MULTI, "%s wants to join (accept: F6)",their->player.callsign);

		strcpy (RefusePlayerName,their->player.callsign);
		RefuseTimeLimit=timer_get_fixed_seconds();
		RefuseThisPlayer=0;
		WaitForRefuseAnswer=1;
	}
	else
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
			{
				net_ipx_welcome_player(their);
					return;
			}
		}

		if (strcmp(their->player.callsign,RefusePlayerName))
			return;

		if (RefuseThisPlayer)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			net_ipx_welcome_player(their);
			return;
		}

		if ((timer_get_fixed_seconds()) > RefuseTimeLimit+REFUSE_INTERVAL)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (!strcmp (their->player.callsign,RefusePlayerName))
			{
				net_ipx_dump_player(their->player.protocol.ipx.server,their->player.protocol.ipx.node, DUMP_DORK);
			}
			return;
		}
	}
}

static int show_game_rules_handler(window *wind, d_event *event, netgame_info *netgame)
{
	int k;
	int w = FSPACX(280), h = FSPACY(130);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;

		case EVENT_KEY_COMMAND:
			k = ((d_event_keycommand *)event)->keycode;
			switch (k)
			{
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					window_close(wind);
					return 1;
			}
			break;

		case EVENT_IDLE:
			timer_delay2(50);
			break;

		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

			gr_set_current_canvas(window_get_canvas(wind));

			grd_curcanv->cv_font = MEDIUM3_FONT;

			gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);
			gr_string( 0x8000, FSPACY(35), "NETGAME INFO" );

			grd_curcanv->cv_font = GAME_FONT;


			gr_printf( FSPACX( 25),FSPACY( 55), "Reactor Life:");
			gr_printf( FSPACX( 25),FSPACY( 67), "Pakets per second:");
			gr_printf( FSPACX( 25),FSPACY( 73), "Show All Players On Automap:");

			gr_printf( FSPACX( 25),FSPACY(100), "Allowed Objects");
			gr_printf( FSPACX( 25),FSPACY(110), "Laser Upgrade:");
			gr_printf( FSPACX( 25),FSPACY(116), "Quad Laser:");
			gr_printf( FSPACX( 25),FSPACY(122), "Vulcan Cannon:");
			gr_printf( FSPACX( 25),FSPACY(128), "Spreadfire Cannon:");
			gr_printf( FSPACX( 25),FSPACY(134), "Plasma Cannon:");
			gr_printf( FSPACX( 25),FSPACY(140), "Fusion Cannon:");
			gr_printf( FSPACX(170),FSPACY(110), "Homing Missile:");
			gr_printf( FSPACX(170),FSPACY(116), "Proximity Bomb:");
			gr_printf( FSPACX(170),FSPACY(122), "Smart Missile:");
			gr_printf( FSPACX(170),FSPACY(128), "Mega Missile:");
			gr_printf( FSPACX( 25),FSPACY(150), "Invulnerability:");
			gr_printf( FSPACX( 25),FSPACY(156), "Cloak:");

			gr_set_fontcolor(gr_find_closest_color_current(255,255,255),-1);
			gr_printf( FSPACX(170),FSPACY( 55), "%i Min", netgame->control_invul_time/F1_0/60);
			gr_printf( FSPACX(170),FSPACY( 67), "%i", netgame->PacketsPerSec);
			gr_printf( FSPACX(170),FSPACY( 73), netgame->game_flags&NETGAME_FLAG_SHOW_MAP?"ON":"OFF");


			gr_printf( FSPACX(130),FSPACY(110), netgame->AllowedItems&NETFLAG_DOLASER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(116), netgame->AllowedItems&NETFLAG_DOQUAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(122), netgame->AllowedItems&NETFLAG_DOVULCAN?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(128), netgame->AllowedItems&NETFLAG_DOSPREAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(134), netgame->AllowedItems&NETFLAG_DOPLASMA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(140), netgame->AllowedItems&NETFLAG_DOFUSION?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(110), netgame->AllowedItems&NETFLAG_DOHOMING?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(116), netgame->AllowedItems&NETFLAG_DOPROXIM?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(122), netgame->AllowedItems&NETFLAG_DOSMART?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(128), netgame->AllowedItems&NETFLAG_DOMEGA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(150), netgame->AllowedItems&NETFLAG_DOINVUL?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(156), netgame->AllowedItems&NETFLAG_DOCLOAK?"YES":"NO");
			gr_set_current_canvas(NULL);
			break;

		default:
			break;
	}

	return 0;
}

void net_ipx_show_game_rules(netgame_info *netgame)
{
	gr_set_current_canvas(NULL);

	window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200),
				  (int (*)(window *, d_event *, void *))show_game_rules_handler, netgame);
}

static int show_game_info_handler(newmenu *menu, d_event *event, netgame_info *netgame)
{
	if (event->type != EVENT_NEWMENU_SELECTED)
		return 0;

	if (newmenu_get_citem(menu) != 1)
		return 0;

	net_ipx_show_game_rules(netgame);

	return 1;
}

int net_ipx_show_game_stats(int choice)
{
	char rinfo[512],*info=rinfo;
	char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Unknown"};
	int c;
	netgame_info *netgame = &Active_ipx_games[choice];

	memset(info,0,sizeof(char)*256);

	info+=sprintf(info,"\nConnected to\n\"%s\"\n",netgame->game_name);

	if(!netgame->mission_title)
		info+=sprintf(info,"Descent: First Strike");
	else
		info+=sprintf(info,netgame->mission_title);

   if( netgame->levelnum >= 0 )
   {
	   info+=sprintf (info," - Lvl %i",netgame->levelnum);
   }
   else
   {
      info+=sprintf (info," - Lvl S%i",(netgame->levelnum*-1));
   }

	info+=sprintf (info,"\n\nDifficulty: %s",MENU_DIFFICULTY_TEXT(netgame->difficulty));
	info+=sprintf (info,"\nGame Mode: %s",NetworkModeNames[netgame->gamemode]);
	info+=sprintf (info,"\nPlayers: %i/%i",netgame->numplayers,netgame->max_numplayers);

	c=nm_messagebox1("WELCOME", (int (*)(newmenu *, d_event *, void *))show_game_info_handler, netgame, 2, "JOIN GAME", "GAME INFO", rinfo);
	if (c==0)
		return 1;
	//else if (c==1)
	// handled in above callback
	else
		return 0;
}
#endif
