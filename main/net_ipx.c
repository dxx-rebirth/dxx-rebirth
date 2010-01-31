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
 * Routines for managing IPX-protocol network play.
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define PATCH12

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
#include "powerup.h"
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
#include "switch.h"
#include "automap.h"
#include "byteswap.h"
#include "kconfig.h"
#include "playsave.h"
#include "cfile.h"
#include "gamefont.h"
#include "rbaudio.h"

// Prototypes
extern void multi_send_drop_marker (int player,vms_vector position,char messagenum,char text[]);
extern void multi_send_kill_goal_counts();
void net_ipx_process_naked_pdata (char *,int);
void net_ipx_flush();
void net_ipx_listen();
void net_ipx_update_netgame();
void net_ipx_check_for_old_version(char pnum);
void net_ipx_send_objects();
void net_ipx_send_rejoin_sync(int player_num);
void net_ipx_send_game_info(IPX_sequence_packet *their);
void net_ipx_send_endlevel_short_sub(int from_player_num, int to_player);
void net_ipx_read_sync_packet(netgame_info * sp, int rsinit);
int  net_ipx_wait_for_playerinfo();
void net_ipx_process_pdata(char *data);
void net_ipx_read_object_packet(ubyte *data );
void net_ipx_read_endlevel_packet(ubyte *data );
void net_ipx_read_endlevel_short_packet(ubyte *data );
void net_ipx_ping_all(fix time);
void net_ipx_ping(ubyte flat, int pnum);
void net_ipx_handle_ping_return(ubyte pnum);
void net_ipx_process_names_return(ubyte *data);
void net_ipx_send_player_names(IPX_sequence_packet *their);
void net_ipx_more_game_options();
void net_ipx_count_powerups_in_mine();
int  net_ipx_wait_for_all_info(int choice);
void net_ipx_do_big_wait(int choice);
void net_ipx_send_extras();
void net_ipx_read_pdata_packet(IPX_frame_info *pd);
void net_ipx_read_pdata_short_packet(IPX_short_frame_info *pd);
void ClipRank(ubyte *rank);
void net_ipx_do_refuse_stuff(IPX_sequence_packet *their);
int  net_ipx_get_new_player_num(IPX_sequence_packet *their);
int net_ipx_show_game_stats(int choice);
extern void multi_send_stolen_items();
int net_ipx_wait_for_snyc();
extern void multi_send_wall_status (int,ubyte,ubyte,ubyte);
extern void multi_send_wall_status_specific (int,int,ubyte,ubyte,ubyte);
extern void game_disable_cheats();

// Variables
int num_active_ipx_games = 0;
int     IPX_active=0;
int     num_active_ipx_changed = 0;
int     IPX_allow_socket_changes = 1;
int     NetSecurityFlag=NETSECURITY_OFF;
int     NetSecurityNum=0;
int     VerifyPlayerJoined=-1;
IPX_sequence_packet IPX_sync_player; // Who is rejoining now?
int     IPX_TotalMissedPackets=0,IPX_TotalPacketsGot=0;
IPX_frame_info      MySyncPack,UrgentSyncPack;
ubyte           MySyncPackInitialized = 0;              // Set to 1 if the MySyncPack is zeroed.
IPX_sequence_packet IPX_Seq;
char WantPlayersInfo=0;
char WaitingForPlayerInfo=0;
int IPX_Socket=0;
extern obj_position Player_init[MAX_PLAYERS];
extern int force_cockpit_redraw;
extern ubyte Version_major,Version_minor;
extern ubyte SurfingNet;
extern char MaxPowerupsAllowed[MAX_POWERUP_TYPES];
extern char PowerupsInMine[MAX_POWERUP_TYPES];
extern int Final_boss_is_dead;
typedef struct IPX_endlevel_info {
	ubyte                               type;
	ubyte                               player_num;
	sbyte                               connected;
	ubyte                               seconds_left;
	short                              kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
	short                               kills;
	short                               killed;
} IPX_endlevel_info;

typedef struct IPX_endlevel_info_short {
	ubyte                               type;
	ubyte                               player_num;
	sbyte                               connected;
	ubyte                               seconds_left;
} IPX_endlevel_info_short;

#define SEQUENCE_PACKET_SIZE    (sizeof(IPX_sequence_packet)-sizeof(netplayer_info)+sizeof(IPX_netplayer_info))
#define FRAME_INFO_SIZE         sizeof(IPX_frame_info)
#define IPX_SHORT_INFO_SIZE     sizeof(IPX_short_frame_info)

netgame_info Active_ipx_games[IPX_MAX_NETGAMES];
netgame_info *TempPlayersInfo,TempPlayersBase;
int NamesInfoSecurity=-1;
netgame_info TempNetInfo;

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
	static int cleanup = 0;
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

	if (!cleanup)
		atexit(ipxdrv_close);
	cleanup = 1;

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
	int loc, tmpi;
	short tmps;
	ubyte out_buffer[MAX_DATA_SIZE];

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.type;                                       loc++;
	tmpi = INTEL_INT(seq.Security);
	memcpy(&(out_buffer[loc]), &tmpi, 4);                           loc += 4;       loc += 3;
	memcpy(&out_buffer[loc], seq.player.callsign, CALLSIGN_LEN+1); loc += CALLSIGN_LEN+1;
	memcpy(&out_buffer[loc], seq.player.protocol.ipx.server, 4);    loc += 4;
	memcpy(&out_buffer[loc], seq.player.protocol.ipx.node, 6);      loc += 6;
	out_buffer[loc] = seq.player.version_major;                  loc++;
	out_buffer[loc] = seq.player.version_minor;                  loc++;
	out_buffer[loc] = seq.player.protocol.ipx.computer_type;                  loc++;
	out_buffer[loc] = seq.player.connected;                      loc++;
	tmps = INTEL_SHORT(seq.player.protocol.ipx.socket);
	memcpy(&(out_buffer[loc]), &tmps, 2);		loc += 2;
	out_buffer[loc] = seq.player.rank;                           loc++;
	
	if (net_address != NULL)
		ipxdrv_send_packet_data(out_buffer, loc, server, node, net_address);
	else if ((server == NULL) && (node == NULL))
		ipxdrv_send_broadcast_packet_data(out_buffer, loc);
	else
		ipxdrv_send_internetwork_packet_data(out_buffer, loc, server, node);
}

void net_ipx_receive_sequence_packet(ubyte *data, IPX_sequence_packet *seq)
{
	int loc = 0;

	seq->type = data[0];                        loc++;
	memcpy(&(seq->Security), &(data[loc]), 4);  loc += 4;   loc += 3;   // +3 for pad byte
	seq->Security = INTEL_INT(seq->Security);
	memcpy(seq->player.callsign, &(data[loc]), CALLSIGN_LEN+1);       loc += CALLSIGN_LEN+1;
	memcpy(&(seq->player.protocol.ipx.server), &(data[loc]), 4);       loc += 4;
	memcpy(&(seq->player.protocol.ipx.node), &(data[loc]), 6);         loc += 6;
	seq->player.version_major = data[loc];                            loc++;
	seq->player.version_minor = data[loc];                            loc++;
	memcpy(&(seq->player.protocol.ipx.computer_type), &(data[loc]), 1);            loc++;      // memcpy to avoid compile time warning about enum
	seq->player.connected = data[loc];                                loc++;
	memcpy(&(seq->player.protocol.ipx.socket), &(data[loc]), 2);                   loc += 2;
	memcpy (&(seq->player.rank),&(data[loc]),1);                      loc++;
}

void send_netgame_packet(ubyte *server, ubyte *node, ubyte *net_address, int lite_flag)
{
	if (lite_flag)
	{
		IPX_lite_info netpkt;
		
		netpkt.type = Netgame.protocol.ipx.Game_pkt_type;
		netpkt.Security = Netgame.protocol.ipx.Game_Security;
		memcpy(&netpkt.game_name, &Netgame.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
		memcpy(&netpkt.mission_title, &Netgame.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));
		memcpy(&netpkt.mission_name, &Netgame.mission_name, sizeof(char)*9);
		netpkt.levelnum = Netgame.levelnum;
		netpkt.gamemode = Netgame.gamemode;
		netpkt.RefusePlayers = Netgame.RefusePlayers;
		netpkt.difficulty = Netgame.difficulty;
		netpkt.game_status = Netgame.game_status;
		netpkt.numplayers = Netgame.numplayers;
		netpkt.max_numplayers = Netgame.max_numplayers;
		netpkt.numconnected = Netgame.numconnected;
		netpkt.game_flags = Netgame.game_flags;
		netpkt.protocol_version = Netgame.protocol.ipx.protocol_version;
		netpkt.version_major = Netgame.version_major;
		netpkt.version_minor = Netgame.version_minor;
		netpkt.team_vector = Netgame.team_vector;
		
		if (net_address != NULL)
			ipxdrv_send_packet_data((ubyte *)&netpkt, sizeof(IPX_lite_info), server, node, net_address);
		else if ((server == NULL) && (node == NULL))
			ipxdrv_send_broadcast_packet_data((ubyte *)&netpkt, sizeof(IPX_lite_info));
		else
			ipxdrv_send_internetwork_packet_data((ubyte *)&netpkt, sizeof(IPX_lite_info), server, node);
	}
	else
	{
		IPX_netgame_info netpkt;
		int i, j;
		
		netpkt.type = Netgame.protocol.ipx.Game_pkt_type;
		netpkt.Security = Netgame.protocol.ipx.Game_Security;
		memcpy(&netpkt.game_name, &Netgame.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
		memcpy(&netpkt.mission_title, &Netgame.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));
		memcpy(&netpkt.mission_name, &Netgame.mission_name, sizeof(char)*9);
		netpkt.levelnum = Netgame.levelnum;
		netpkt.gamemode = Netgame.gamemode;
		netpkt.RefusePlayers = Netgame.RefusePlayers;
		netpkt.difficulty = Netgame.difficulty;
		netpkt.game_status = Netgame.game_status;
		netpkt.numplayers = Netgame.numplayers;
		netpkt.max_numplayers = Netgame.max_numplayers;
		netpkt.numconnected = Netgame.numconnected;
		netpkt.game_flags = Netgame.game_flags;
		netpkt.protocol_version = Netgame.protocol.ipx.protocol_version;
		netpkt.version_major = Netgame.version_major;
		netpkt.version_minor = Netgame.version_minor;
		netpkt.team_vector = Netgame.team_vector;
		netpkt.DoMegas = (Netgame.AllowedItems & NETFLAG_DOMEGA);
		netpkt.DoSmarts = (Netgame.AllowedItems & NETFLAG_DOSMART);
		netpkt.DoFusions = (Netgame.AllowedItems & NETFLAG_DOFUSION);
		netpkt.DoHelix = (Netgame.AllowedItems & NETFLAG_DOHELIX);
		netpkt.DoPhoenix = (Netgame.AllowedItems & NETFLAG_DOPHOENIX);
		netpkt.DoAfterburner = (Netgame.AllowedItems & NETFLAG_DOAFTERBURNER);
		netpkt.DoInvulnerability = (Netgame.AllowedItems & NETFLAG_DOINVUL);
		netpkt.DoCloak = (Netgame.AllowedItems & NETFLAG_DOCLOAK);
		netpkt.DoGauss = (Netgame.AllowedItems & NETFLAG_DOGAUSS);
		netpkt.DoVulcan = (Netgame.AllowedItems & NETFLAG_DOVULCAN);
		netpkt.DoPlasma = (Netgame.AllowedItems & NETFLAG_DOPLASMA);
		netpkt.DoOmega = (Netgame.AllowedItems & NETFLAG_DOOMEGA);
		netpkt.DoSuperLaser = (Netgame.AllowedItems & NETFLAG_DOSUPERLASER);
		netpkt.DoProximity = (Netgame.AllowedItems & NETFLAG_DOPROXIM);
		netpkt.DoSpread = (Netgame.AllowedItems & NETFLAG_DOSPREAD);
		netpkt.DoSmartMine = (Netgame.AllowedItems & NETFLAG_DOSMARTMINE);
		netpkt.DoFlash = (Netgame.AllowedItems & NETFLAG_DOFLASH);
		netpkt.DoGuided = (Netgame.AllowedItems & NETFLAG_DOGUIDED);
		netpkt.DoEarthShaker = (Netgame.AllowedItems & NETFLAG_DOSHAKER);
		netpkt.DoMercury = (Netgame.AllowedItems & NETFLAG_DOMERCURY);
		netpkt.Allow_marker_view = Netgame.Allow_marker_view;
		netpkt.AlwaysLighting = Netgame.AlwaysLighting;
		netpkt.DoAmmoRack = (Netgame.AllowedItems & NETFLAG_DOAMMORACK);
		netpkt.DoConverter = (Netgame.AllowedItems & NETFLAG_DOCONVERTER);
		netpkt.DoHeadlight = (Netgame.AllowedItems & NETFLAG_DOHEADLIGHT);
		netpkt.DoHoming = (Netgame.AllowedItems & NETFLAG_DOHOMING);
		netpkt.DoLaserUpgrade = (Netgame.AllowedItems & NETFLAG_DOLASER);
		netpkt.DoQuadLasers = (Netgame.AllowedItems & NETFLAG_DOQUAD);
		netpkt.ShowAllNames = Netgame.ShowAllNames;
		netpkt.BrightPlayers = Netgame.BrightPlayers;
		netpkt.invul = Netgame.InvulAppear;
		memcpy(&netpkt.team_name, &Netgame.team_name, 2*(CALLSIGN_LEN+1));
		for (i = 0; i < MAX_PLAYERS; i++)
			netpkt.locations[i] = Netgame.locations[i];
		for (i = 0; i < MAX_PLAYERS; i++)
			for (j = 0; j < MAX_PLAYERS; j++)
				netpkt.kills[i][j] = Netgame.kills[i][j];
		netpkt.segments_checksum = Netgame.segments_checksum;
		netpkt.team_kills[0] = Netgame.team_kills[0];
		netpkt.team_kills[1] = Netgame.team_kills[1];
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netpkt.killed[i] = Netgame.killed[i];
			netpkt.player_kills[i] = Netgame.player_kills[i];
		}
		netpkt.KillGoal = Netgame.KillGoal;
		netpkt.PlayTimeAllowed = Netgame.PlayTimeAllowed;
		netpkt.level_time = Netgame.level_time;
		netpkt.control_invul_time = Netgame.control_invul_time;
		netpkt.monitor_vector = Netgame.monitor_vector;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netpkt.player_score[i] = Netgame.player_score[i];
			netpkt.player_flags[i] = Netgame.player_flags[i];
		}
		netpkt.PacketsPerSec = Netgame.PacketsPerSec;
		netpkt.ShortPackets = Netgame.protocol.ipx.ShortPackets;
		
		if (net_address != NULL)
			ipxdrv_send_packet_data((ubyte *)&netpkt, sizeof(IPX_netgame_info), server, node, net_address);
		else if ((server == NULL) && (node == NULL))
			ipxdrv_send_broadcast_packet_data((ubyte *)&netpkt, sizeof(IPX_netgame_info));
		else
			ipxdrv_send_internetwork_packet_data((ubyte *)&netpkt, sizeof(IPX_netgame_info), server, node);
	}
}

void receive_netgame_packet(ubyte *data, netgame_info *netgame, int lite_flag)
{
	if (lite_flag)
	{
		IPX_lite_info netpkt;

		memcpy(&netpkt, data, sizeof(IPX_lite_info));
		
		netgame->protocol.ipx.Game_pkt_type = netpkt.type;
		netgame->protocol.ipx.Game_Security = netpkt.Security;
		memcpy(netgame->game_name, &netpkt.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
		memcpy(netgame->mission_title, &netpkt.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));
		memcpy(netgame->mission_name, &netpkt.mission_name, sizeof(char)*9);
		netgame->levelnum = netpkt.levelnum;
		netgame->gamemode = netpkt.gamemode;
		netgame->RefusePlayers = netpkt.RefusePlayers;
		netgame->difficulty = netpkt.difficulty;
		netgame->game_status = netpkt.game_status;
		netgame->numplayers = netpkt.numplayers;
		netgame->max_numplayers = netpkt.max_numplayers;
		netgame->numconnected = netpkt.numconnected;
		netgame->game_flags = netpkt.game_flags;
		netgame->protocol.ipx.protocol_version = netpkt.protocol_version;
		netgame->version_major = netpkt.version_major;
		netgame->version_minor = netpkt.version_minor;
		netgame->team_vector = netpkt.team_vector;
	}
	else
	{
		IPX_netgame_info netpkt;
		int i, j;
		
		memcpy(&netpkt, data, sizeof(IPX_netgame_info));
		
		netgame->protocol.ipx.Game_pkt_type = netpkt.type;
		netgame->protocol.ipx.Game_Security = netpkt.Security;
		memcpy(netgame->game_name, &netpkt.game_name, sizeof(char)*(NETGAME_NAME_LEN+1));
		memcpy(netgame->mission_title, &netpkt.mission_title, sizeof(char)*(MISSION_NAME_LEN+1));
		memcpy(netgame->mission_name, &netpkt.mission_name, sizeof(char)*9);
		netgame->levelnum = netpkt.levelnum;
		netgame->gamemode = netpkt.gamemode;
		netgame->RefusePlayers = netpkt.RefusePlayers;
		netgame->difficulty = netpkt.difficulty;
		netgame->game_status = netpkt.game_status;
		netgame->numplayers = netpkt.numplayers;
		netgame->max_numplayers = netpkt.max_numplayers;
		netgame->numconnected = netpkt.numconnected;
		netgame->game_flags = netpkt.game_flags;
		netgame->protocol.ipx.protocol_version = netpkt.protocol_version;
		netgame->version_major = netpkt.version_major;
		netgame->version_minor = netpkt.version_minor;
		netgame->team_vector = netpkt.team_vector;
		netgame->AllowedItems = 0;
		if (netpkt.DoMegas) netgame->AllowedItems |= NETFLAG_DOMEGA;
		if (netpkt.DoSmarts) netgame->AllowedItems |= NETFLAG_DOSMART;
		if (netpkt.DoFusions) netgame->AllowedItems |= NETFLAG_DOFUSION;
		if (netpkt.DoHelix) netgame->AllowedItems |= NETFLAG_DOHELIX;
		if (netpkt.DoPhoenix) netgame->AllowedItems |= NETFLAG_DOPHOENIX;
		if (netpkt.DoAfterburner) netgame->AllowedItems |= NETFLAG_DOAFTERBURNER;
		if (netpkt.DoInvulnerability) netgame->AllowedItems |= NETFLAG_DOINVUL;
		if (netpkt.DoCloak) netgame->AllowedItems |= NETFLAG_DOCLOAK;
		if (netpkt.DoGauss) netgame->AllowedItems |= NETFLAG_DOGAUSS;
		if (netpkt.DoVulcan) netgame->AllowedItems |= NETFLAG_DOVULCAN;
		if (netpkt.DoPlasma) netgame->AllowedItems |= NETFLAG_DOPLASMA;
		if (netpkt.DoOmega) netgame->AllowedItems |= NETFLAG_DOOMEGA;
		if (netpkt.DoSuperLaser) netgame->AllowedItems |= NETFLAG_DOSUPERLASER;
		if (netpkt.DoProximity) netgame->AllowedItems |= NETFLAG_DOPROXIM;
		if (netpkt.DoSpread) netgame->AllowedItems |= NETFLAG_DOSPREAD;
		if (netpkt.DoSmartMine) netgame->AllowedItems |= NETFLAG_DOSMARTMINE;
		if (netpkt.DoFlash) netgame->AllowedItems |= NETFLAG_DOFLASH;
		if (netpkt.DoGuided) netgame->AllowedItems |= NETFLAG_DOGUIDED;
		if (netpkt.DoEarthShaker) netgame->AllowedItems |= NETFLAG_DOSHAKER;
		if (netpkt.DoMercury) netgame->AllowedItems |= NETFLAG_DOMERCURY;
		if (netpkt.DoAmmoRack) netgame->AllowedItems |= NETFLAG_DOAMMORACK;
		if (netpkt.DoConverter) netgame->AllowedItems |= NETFLAG_DOCONVERTER;
		if (netpkt.DoHeadlight) netgame->AllowedItems |= NETFLAG_DOHEADLIGHT;
		if (netpkt.DoHoming) netgame->AllowedItems |= NETFLAG_DOHOMING;
		if (netpkt.DoLaserUpgrade) netgame->AllowedItems |= NETFLAG_DOLASER;
		if (netpkt.DoQuadLasers) netgame->AllowedItems |= NETFLAG_DOQUAD;
		netgame->Allow_marker_view = netpkt.Allow_marker_view;
		netgame->AlwaysLighting = netpkt.AlwaysLighting;
		netgame->ShowAllNames = netpkt.ShowAllNames;
		netgame->BrightPlayers = netpkt.BrightPlayers;
		netgame->InvulAppear = netpkt.invul;
		memcpy(netgame->team_name, &netpkt.team_name, 2*(CALLSIGN_LEN+1));
		for (i = 0; i < MAX_PLAYERS; i++)
			netgame->locations[i] = netpkt.locations[i];
		for (i = 0; i < MAX_PLAYERS; i++)
			for (j = 0; j < MAX_PLAYERS; j++)
				netgame->kills[i][j] = netpkt.kills[i][j];
		netgame->segments_checksum = netpkt.segments_checksum;
		netgame->team_kills[0] = netpkt.team_kills[0];
		netgame->team_kills[1] = netpkt.team_kills[1];
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netgame->killed[i] = netpkt.killed[i];
			netgame->player_kills[i] = netpkt.player_kills[i];
		}
		netgame->KillGoal = netpkt.KillGoal;
		netgame->PlayTimeAllowed = netpkt.PlayTimeAllowed;
		netgame->level_time = netpkt.level_time;
		netgame->control_invul_time = netpkt.control_invul_time;
		netgame->monitor_vector = netpkt.monitor_vector;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netgame->player_score[i] = netpkt.player_score[i];
			netgame->player_flags[i] = netpkt.player_flags[i];
		}
		netgame->PacketsPerSec = netpkt.PacketsPerSec;
		netgame->protocol.ipx.ShortPackets = netpkt.ShortPackets;
	}
}

void send_netplayers_packet(ubyte *server, ubyte *node)
{
	IPX_AllNetPlayers_info netplrs;
	int i;
	
	netplrs.type = Netgame.protocol.ipx.Player_pkt_type;
	netplrs.Security = Netgame.protocol.ipx.Player_Security;
	for (i = 0; i < MAX_PLAYERS+4; i++)
	{
		memcpy(&netplrs.players[i].callsign, &Netgame.players[i].callsign, CALLSIGN_LEN+1);
		memcpy(&netplrs.players[i].network.ipx.server, &Netgame.players[i].protocol.ipx.server, 4);
		memcpy(&netplrs.players[i].network.ipx.node, &Netgame.players[i].protocol.ipx.node, 6);
		netplrs.players[i].version_major = Netgame.players[i].version_major;
		netplrs.players[i].version_minor = Netgame.players[i].version_minor;
		netplrs.players[i].computer_type = Netgame.players[i].protocol.ipx.computer_type;
		netplrs.players[i].connected = Netgame.players[i].connected;
		netplrs.players[i].socket = Netgame.players[i].protocol.ipx.socket;
		netplrs.players[i].rank = Netgame.players[i].rank;
	}
	
	if ((server == NULL) && (node == NULL))
		ipxdrv_send_broadcast_packet_data((ubyte *)&netplrs, sizeof(IPX_AllNetPlayers_info));
	else
		ipxdrv_send_internetwork_packet_data((ubyte *)&netplrs, sizeof(IPX_AllNetPlayers_info), server, node);
}

void receive_netplayers_packet(ubyte *data, netgame_info *pinfo)
{
	IPX_AllNetPlayers_info netplrs;
	int i;
	
	memcpy(&netplrs, data, sizeof(IPX_AllNetPlayers_info));
	
	pinfo->protocol.ipx.Player_pkt_type = netplrs.type;
	pinfo->protocol.ipx.Player_Security = netplrs.Security;
	for (i = 0; i < MAX_PLAYERS+4; i++)
	{
		memcpy(pinfo->players[i].callsign, &netplrs.players[i].callsign, CALLSIGN_LEN+1);
		memcpy(pinfo->players[i].protocol.ipx.server, &netplrs.players[i].network.ipx.server, 4);
		memcpy(pinfo->players[i].protocol.ipx.node, &netplrs.players[i].network.ipx.node, 6);
		pinfo->players[i].version_major = netplrs.players[i].version_major;
		pinfo->players[i].version_minor = netplrs.players[i].version_minor;
		pinfo->players[i].protocol.ipx.computer_type = netplrs.players[i].computer_type;
		pinfo->players[i].connected = netplrs.players[i].connected;
		pinfo->players[i].protocol.ipx.socket = netplrs.players[i].socket;
		pinfo->players[i].rank = netplrs.players[i].rank;
	}
}


void
net_ipx_init(void)
{
	// So you want to play a netgame, eh?  Let's a get a few things straight
	int t;
	int save_pnum = Player_num;

	game_disable_cheats();
	Final_boss_is_dead=0;
	NamesInfoSecurity=-1;


#ifdef NETPROFILING
	OpenSendLog();
	OpenRecieveLog(); 
#endif
	
	for (t=0;t<MAX_POWERUP_TYPES;t++)
	{
		MaxPowerupsAllowed[t]=0;
		PowerupsInMine[t]=0;
	}

	IPX_TotalMissedPackets=0; IPX_TotalPacketsGot=0;

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&IPX_Seq, 0, sizeof(IPX_sequence_packet));
	IPX_Seq.type = PID_REQUEST;
	memcpy(IPX_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);

	IPX_Seq.player.version_major=Version_major;
	IPX_Seq.player.version_minor=Version_minor;
	IPX_Seq.player.rank=GetMyNetRanking();	

	memcpy(IPX_Seq.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6);
	memcpy(IPX_Seq.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );
	IPX_Seq.player.protocol.ipx.computer_type = 1;

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();

	Player_num = save_pnum;
	multi_new_game();
	Network_new_game = 1;
	Control_center_destroyed = 0;
	net_ipx_flush();

	Netgame.PacketsPerSec=10;
	Netgame.protocol.ipx.ShortPackets=0;
}

int net_ipx_how_many_connected()
 {
  int num=0,i;
 
	for (i = 0; i < N_players; i++)
		if (Players[i].connected)
			num++;
   return (num);
 }

#define ENDLEVEL_SEND_INTERVAL  (F1_0*2)
#define ENDLEVEL_IDLE_TIME      (F1_0*20)

  
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


extern fix StartAbortMenuTime;

int net_ipx_kmatrix_poll2( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for End-of-level menu

   int num_ready=0, i, rval = 0;
 
	menu = menu;
	userdata = userdata;
	
	if (event->type != EVENT_IDLE)
		return 0;
	
	if (timer_get_fixed_seconds() > (StartAbortMenuTime+(F1_0 * 8)))
		rval = -2;


	net_ipx_listen();


	for (i = 0; i < N_players; i++)
		if ((Players[i].connected != CONNECT_PLAYING) && (Players[i].connected != CONNECT_ESCAPE_TUNNEL) && (Players[i].connected != CONNECT_END_MENU))
			num_ready++;

	if (num_ready == N_players) // All players have checked in or are disconnected
		rval = -2;

	return rval;
}


int
net_ipx_endlevel(int *secret)
{
	// Do whatever needs to be done between levels

   int i;

   *secret=0;

	//net_ipx_flush();

	Network_status = NETSTAT_ENDLEVEL; // We are between levels

	net_ipx_listen();

	net_ipx_send_endlevel_packet();

	for (i=0; i<N_players; i++) 
	{
		Netgame.players[i].LastPacketTime = timer_get_fixed_seconds();
	}
   
	net_ipx_send_endlevel_packet();
	net_ipx_send_endlevel_packet();
	MySyncPackInitialized = 0;

	net_ipx_update_netgame();

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
	{
		return 0;
	}

	if (game->version_major==0 && Version_major>0)
	{
		return (0);
	}

	if (game->version_major>0 && Version_major==0)
	{
		return (0);
	}

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
		// Look for player that is not connected
		
		if (game->numconnected==game->max_numplayers)
			return (2);
		
		if (game->RefusePlayers)
			return (3);
		
		if (game->numplayers < game->max_numplayers)
			return 1;

		if (game->numconnected<num_players)
			return 1;
	}
	
	// Search to see if we were already in this closed netgame in progress

	for (i = 0; i < num_players; i++) {
		if ( (!stricmp(Players[Player_num].callsign, game->players[i].callsign)) &&
			  (!memcmp(IPX_Seq.player.protocol.ipx.node, game->players[i].protocol.ipx.node, 6)) &&
			  (!memcmp(IPX_Seq.player.protocol.ipx.server, game->players[i].protocol.ipx.server, 4)) )
			break;
	}

	if (i != num_players)
		return 1;
 
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
   if (VerifyPlayerJoined==playernum)
	  VerifyPlayerJoined=-1;

//      create_player_appearance_effect(&Objects[Players[playernum].objnum]);
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
	

	ClipRank (&their->player.rank);
	Netgame.players[pnum].rank=their->player.rank;
	Netgame.players[pnum].version_major=their->player.version_major;
	Netgame.players[pnum].version_minor=their->player.version_minor;
	net_ipx_check_for_old_version(pnum);

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
	Players[pnum].KillGoalCount=0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	ClipRank (&their->player.rank);

	if (GameArg.MplNoRankings)
		HUD_init_message("'%s' %s\n",their->player.callsign, TXT_JOINING);
	else
		HUD_init_message("%s'%s' %s\n",RankStrings[their->player.rank],their->player.callsign, TXT_JOINING);
	
	multi_make_ghost_player(pnum);

	multi_send_score();
	multi_sort_kill_list();
}

void net_ipx_welcome_player(IPX_sequence_packet *their)
{
	// Add a player to a game already in progress
	ubyte local_address[6];
	int player_num;
	int i;

	WaitForRefuseAnswer=0;

	if (HoardEquipped())
	{
		// If hoard game, and this guy isn't D2 Christmas (v1.2), dump him
		if ((Game_mode & GM_HOARD) && ((their->player.version_minor & 0x0F)<2))
		{
			net_ipx_dump_player(their->player.protocol.ipx.server, their->player.protocol.ipx.node, DUMP_DORK);
			return;
		}
	}

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Control_center_destroyed))
	{
		net_ipx_dump_player(their->player.protocol.ipx.server,their->player.protocol.ipx.node, DUMP_ENDLEVEL);
		return; 
	}

	if (Network_send_objects || Network_sending_extras)
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
		if ( (!stricmp(Players[i].callsign, their->player.callsign )) && (!memcmp(Players[i].net_address,local_address, 6)) ) 
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
		
		if (GameArg.MplNoRankings)
			HUD_init_message("'%s' %s", Players[player_num].callsign, TXT_REJOIN);
		else
			HUD_init_message("%s'%s' %s", RankStrings[Netgame.players[player_num].rank],Players[player_num].callsign, TXT_REJOIN);
	}

	Players[player_num].KillGoalCount=0;

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
extern void multi_send_active_door(char);
extern void multi_send_door_open_specific(int,int,int,ubyte);


void net_ipx_send_door_updates(int pnum)
{
	// Send door status when new player joins
	
	int i;
   
   pnum=pnum;

//   Assert (pnum>-1 && pnum<N_players);

	for (i = 0; i < Num_walls; i++)
	{
      if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING) || (Walls[i].state == WALL_DOOR_OPEN)))
			multi_send_door_open_specific(pnum,Walls[i].segnum, Walls[i].sidenum,Walls[i].flags);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open_specific(pnum,Walls[i].segnum, Walls[i].sidenum,Walls[i].flags);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
		else
			multi_send_wall_status_specific(pnum,i,Walls[i].type,Walls[i].flags,Walls[i].state);
	}
}

extern vms_vector MarkerPoint[];
void net_ipx_send_markers()
 {
  // send marker positions/text to new player

  
  int i;

  for (i = 0; i < N_players; i++)
   {
    if (MarkerObject[(i*2)]!=-1)
     multi_send_drop_marker (i,MarkerPoint[(i*2)],0,MarkerMessage[i*2]);
    if (MarkerObject[(i*2)+1]!=-1)
     multi_send_drop_marker (i,MarkerPoint[(i*2)+1],1,MarkerMessage[(i*2)+1]);
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
	#define NUM_BLOWN_BITMAPS 20
	int blown_bitmaps[NUM_BLOWN_BITMAPS];
	int vector = 0;
	segment *seg;

	for (i=0; i < Num_effects; i++)
	{
		if (Effects[i].dest_bm_num > 0) {
			for (j = 0; j < num_blown_bitmaps; j++)
				if (blown_bitmaps[j] == Effects[i].dest_bm_num)
					break;
			if (j == num_blown_bitmaps) {
				blown_bitmaps[num_blown_bitmaps++] = Effects[i].dest_bm_num;
				Assert(num_blown_bitmaps < NUM_BLOWN_BITMAPS);
			}
		}
	}               
		
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
		(!stricmp(IPX_sync_player.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_sending_extras=0;
		Network_rejoined=0;
		Player_joining_extras=-1;
		Network_send_objnum = -1;
	}
}

void net_ipx_swap_object(object *obj)
{
	// swap the short and int entries for this object
	obj->signature     = INTEL_INT(obj->signature);
	obj->next          = INTEL_SHORT(obj->next);
	obj->prev          = INTEL_SHORT(obj->prev);
	obj->segnum        = INTEL_SHORT(obj->segnum);
	obj->pos.x         = INTEL_INT(obj->pos.x);
	obj->pos.y         = INTEL_INT(obj->pos.y);
	obj->pos.z         = INTEL_INT(obj->pos.z);

	obj->orient.rvec.x = INTEL_INT(obj->orient.rvec.x);
	obj->orient.rvec.y = INTEL_INT(obj->orient.rvec.y);
	obj->orient.rvec.z = INTEL_INT(obj->orient.rvec.z);
	obj->orient.fvec.x = INTEL_INT(obj->orient.fvec.x);
	obj->orient.fvec.y = INTEL_INT(obj->orient.fvec.y);
	obj->orient.fvec.z = INTEL_INT(obj->orient.fvec.z);
	obj->orient.uvec.x = INTEL_INT(obj->orient.uvec.x);
	obj->orient.uvec.y = INTEL_INT(obj->orient.uvec.y);
	obj->orient.uvec.z = INTEL_INT(obj->orient.uvec.z);

	obj->size          = INTEL_INT(obj->size);
	obj->shields       = INTEL_INT(obj->shields);

	obj->last_pos.x    = INTEL_INT(obj->last_pos.x);
	obj->last_pos.y    = INTEL_INT(obj->last_pos.y);
	obj->last_pos.z    = INTEL_INT(obj->last_pos.z);

	obj->lifeleft      = INTEL_INT(obj->lifeleft);

	switch (obj->movement_type) {

	case MT_PHYSICS:

		obj->mtype.phys_info.velocity.x = INTEL_INT(obj->mtype.phys_info.velocity.x);
		obj->mtype.phys_info.velocity.y = INTEL_INT(obj->mtype.phys_info.velocity.y);
		obj->mtype.phys_info.velocity.z = INTEL_INT(obj->mtype.phys_info.velocity.z);

		obj->mtype.phys_info.thrust.x   = INTEL_INT(obj->mtype.phys_info.thrust.x);
		obj->mtype.phys_info.thrust.y   = INTEL_INT(obj->mtype.phys_info.thrust.y);
		obj->mtype.phys_info.thrust.z   = INTEL_INT(obj->mtype.phys_info.thrust.z);

		obj->mtype.phys_info.mass       = INTEL_INT(obj->mtype.phys_info.mass);
		obj->mtype.phys_info.drag       = INTEL_INT(obj->mtype.phys_info.drag);
		obj->mtype.phys_info.brakes     = INTEL_INT(obj->mtype.phys_info.brakes);

		obj->mtype.phys_info.rotvel.x   = INTEL_INT(obj->mtype.phys_info.rotvel.x);
		obj->mtype.phys_info.rotvel.y   = INTEL_INT(obj->mtype.phys_info.rotvel.y);
		obj->mtype.phys_info.rotvel.z   = INTEL_INT(obj->mtype.phys_info.rotvel.z);

		obj->mtype.phys_info.rotthrust.x = INTEL_INT(obj->mtype.phys_info.rotthrust.x);
		obj->mtype.phys_info.rotthrust.y = INTEL_INT(obj->mtype.phys_info.rotthrust.y);
		obj->mtype.phys_info.rotthrust.z = INTEL_INT(obj->mtype.phys_info.rotthrust.z);

		obj->mtype.phys_info.turnroll   = INTEL_INT(obj->mtype.phys_info.turnroll);
		obj->mtype.phys_info.flags      = INTEL_SHORT(obj->mtype.phys_info.flags);

		break;

	case MT_SPINNING:

		obj->mtype.spin_rate.x = INTEL_INT(obj->mtype.spin_rate.x);
		obj->mtype.spin_rate.y = INTEL_INT(obj->mtype.spin_rate.y);
		obj->mtype.spin_rate.z = INTEL_INT(obj->mtype.spin_rate.z);
		break;
	}

	switch (obj->control_type) {

	case CT_WEAPON:
		obj->ctype.laser_info.parent_type       = INTEL_SHORT(obj->ctype.laser_info.parent_type);
		obj->ctype.laser_info.parent_num        = INTEL_SHORT(obj->ctype.laser_info.parent_num);
		obj->ctype.laser_info.parent_signature  = INTEL_INT(obj->ctype.laser_info.parent_signature);
		obj->ctype.laser_info.creation_time     = INTEL_INT(obj->ctype.laser_info.creation_time);
		obj->ctype.laser_info.last_hitobj       = INTEL_SHORT(obj->ctype.laser_info.last_hitobj);
		obj->ctype.laser_info.track_goal        = INTEL_SHORT(obj->ctype.laser_info.track_goal);
		obj->ctype.laser_info.multiplier        = INTEL_INT(obj->ctype.laser_info.multiplier);
		break;

	case CT_EXPLOSION:
		obj->ctype.expl_info.spawn_time     = INTEL_INT(obj->ctype.expl_info.spawn_time);
		obj->ctype.expl_info.delete_time    = INTEL_INT(obj->ctype.expl_info.delete_time);
		obj->ctype.expl_info.delete_objnum  = INTEL_SHORT(obj->ctype.expl_info.delete_objnum);
		obj->ctype.expl_info.attach_parent  = INTEL_SHORT(obj->ctype.expl_info.attach_parent);
		obj->ctype.expl_info.prev_attach    = INTEL_SHORT(obj->ctype.expl_info.prev_attach);
		obj->ctype.expl_info.next_attach    = INTEL_SHORT(obj->ctype.expl_info.next_attach);
		break;

	case CT_AI:
		obj->ctype.ai_info.hide_segment         = INTEL_SHORT(obj->ctype.ai_info.hide_segment);
		obj->ctype.ai_info.hide_index           = INTEL_SHORT(obj->ctype.ai_info.hide_index);
		obj->ctype.ai_info.path_length          = INTEL_SHORT(obj->ctype.ai_info.path_length);
		obj->ctype.ai_info.danger_laser_num     = INTEL_SHORT(obj->ctype.ai_info.danger_laser_num);
		obj->ctype.ai_info.danger_laser_signature = INTEL_INT(obj->ctype.ai_info.danger_laser_signature);
		obj->ctype.ai_info.dying_start_time     = INTEL_INT(obj->ctype.ai_info.dying_start_time);
		break;

	case CT_LIGHT:
		obj->ctype.light_info.intensity = INTEL_INT(obj->ctype.light_info.intensity);
		break;

	case CT_POWERUP:
		obj->ctype.powerup_info.count = INTEL_INT(obj->ctype.powerup_info.count);
		obj->ctype.powerup_info.creation_time = INTEL_INT(obj->ctype.powerup_info.creation_time);
		// Below commented out 5/2/96 by Matt.  I asked Allender why it was
		// here, and he didn't know, and it looks like it doesn't belong.
		// if (obj->id == POW_VULCAN_WEAPON)
		// obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
		break;

	}

	switch (obj->render_type) {

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;

		obj->rtype.pobj_info.model_num      = INTEL_INT(obj->rtype.pobj_info.model_num);

		for (i=0;i<MAX_SUBMODELS;i++) {
			obj->rtype.pobj_info.anim_angles[i].p = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].p);
			obj->rtype.pobj_info.anim_angles[i].b = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].b);
			obj->rtype.pobj_info.anim_angles[i].h = INTEL_INT(obj->rtype.pobj_info.anim_angles[i].h);
		}

		obj->rtype.pobj_info.subobj_flags   = INTEL_INT(obj->rtype.pobj_info.subobj_flags);
		obj->rtype.pobj_info.tmap_override  = INTEL_INT(obj->rtype.pobj_info.tmap_override);
		obj->rtype.pobj_info.alt_textures   = INTEL_INT(obj->rtype.pobj_info.alt_textures);
		break;
	}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		obj->rtype.vclip_info.vclip_num = INTEL_INT(obj->rtype.vclip_info.vclip_num);
		obj->rtype.vclip_info.frametime = INTEL_INT(obj->rtype.vclip_info.frametime);
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

	for (h = 0; h < IPX_OBJ_PACKETS_PER_FRAME; h++) // Do more than 1 per frame, try to speed it up without over-stressing the receiver.
	{
		obj_count_frame = 0;
		memset(object_buffer, 0, MAX_DATA_SIZE);
		object_buffer[0] = PID_OBJECT_DATA;
		loc = 3;
	
		if (Network_send_objnum == -1)
		{
			obj_count = 0;
			Network_send_object_mode = 0;
			*(short *)(object_buffer+loc) = INTEL_SHORT(-1);            loc += 2;
			object_buffer[loc] = player_num;                            loc += 1;
			/* Placeholder for remote_objnum, not used here */          loc += 2;
			Network_send_objnum = 0;
			obj_count_frame = 1;
			frame_num = 0;
		}
		
		for (i = Network_send_objnum; i <= Highest_object_index; i++)
		{
			if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				 (Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				 (Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE) &&
				 !(Objects[i].type==OBJ_WEAPON && Objects[i].id==PMINE_ID))
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

			*(short *)(object_buffer+loc) = INTEL_SHORT((short)i);      loc += 2;
			object_buffer[loc] = owner;                                 loc += 1;
			*(short *)(object_buffer+loc) = INTEL_SHORT(remote_objnum); loc += 2;
			memcpy(&object_buffer[loc], &Objects[i], sizeof(object));
			net_ipx_swap_object((object *)&object_buffer[loc]);
			loc += sizeof(object);
		}

		if (obj_count_frame) // Send any objects we've buffered
		{
			frame_num++;
	
			Network_send_objnum = i;
			object_buffer[1] = obj_count_frame;  object_buffer[2] = frame_num;

			Assert(loc <= MAX_DATA_SIZE);

			ipxdrv_send_internetwork_packet_data( object_buffer, loc, IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node );
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
				ipxdrv_send_internetwork_packet_data(object_buffer, 8, IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node);

				// Send sync packet which tells the player who he is and to start!
				net_ipx_send_rejoin_sync(player_num);
				VerifyPlayerJoined=player_num;

				// Turn off send object mode
				Network_send_objnum = -1;
				Network_send_objects = 0;
				obj_count = 0;

				//if (!multi_i_am_master ())
				// Int3();  // Bad!! Get Jason.  Someone goofy is trying to get ahold of the game!

				Network_sending_extras=40; // start to send extras
			   Player_joining_extras=player_num;

				return;
			} // mode == 1;
		} // i > Highest_object_index
	} // For PACKETS_PER_FRAME
}

extern void multi_send_powerup_update();

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
		Network_sending_extras=0;
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
				net_ipx_send_sequence_packet( IPX_sync_player, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address);
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

	send_netgame_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node, NULL, 0);
	send_netplayers_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node);

	return;
}

void resend_sync_due_to_packet_loss_for_allender ()
{
	int i,j;
	
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

	send_netgame_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node, NULL, 0);
	send_netplayers_packet(IPX_sync_player.player.protocol.ipx.server, IPX_sync_player.player.protocol.ipx.node);
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
	
	for (i=0; i<N_players; i++ )    {
		if ( !memcmp( Netgame.players[i].protocol.ipx.node, p->player.protocol.ipx.node, 6) && !memcmp(Netgame.players[i].protocol.ipx.server, p->player.protocol.ipx.server, 4)) 
			return;         // already got them
	}

	memcpy( Netgame.players[N_players].protocol.ipx.node, p->player.protocol.ipx.node, 6 );
	memcpy( Netgame.players[N_players].protocol.ipx.server, p->player.protocol.ipx.server, 4 );

	ClipRank (&p->player.rank);

	memcpy( Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN+1 );
	Netgame.players[N_players].version_major=p->player.version_major;
	Netgame.players[N_players].version_minor=p->player.version_minor;
	Netgame.players[N_players].rank=p->player.rank;
	Netgame.players[N_players].connected = CONNECT_PLAYING;
	net_ipx_check_for_old_version (N_players);

	Players[N_players].KillGoalCount=0;
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
	for (i=0; i<N_players; i++ )    {
		if (!memcmp(Netgame.players[i].protocol.ipx.node, p->player.protocol.ipx.node, 6) && !memcmp(Netgame.players[i].protocol.ipx.server, p->player.protocol.ipx.server, 4)) {
			pn = i;
			break;
		}
	}
	
	if (pn < 0 ) return;

	for (i=pn; i<N_players-1; i++ ) {
		memcpy( Netgame.players[i].protocol.ipx.node, Netgame.players[i+1].protocol.ipx.node, 6 );
		memcpy( Netgame.players[i].protocol.ipx.server, Netgame.players[i+1].protocol.ipx.server, 4 );
		memcpy( Netgame.players[i].callsign, Netgame.players[i+1].callsign, CALLSIGN_LEN+1 );
		Netgame.players[i].version_major=Netgame.players[i+1].version_major;
		Netgame.players[i].version_minor=Netgame.players[i+1].version_minor;

		Netgame.players[i].rank=Netgame.players[i+1].rank;
		ClipRank (&Netgame.players[i].rank);
		net_ipx_check_for_old_version(i);	
	}
		
	N_players--;
	Netgame.numplayers = N_players;

	// Broadcast new info

	net_ipx_send_game_info(NULL);

}

void
net_ipx_dump_player(ubyte * server, ubyte *node, int why)
{
	// Inform player that he was not chosen for the netgame

	IPX_sequence_packet temp;

	memset(&temp, 0, sizeof(IPX_sequence_packet));
	temp.type = PID_DUMP;
	memcpy(temp.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	temp.player.connected = why;
	net_ipx_send_sequence_packet( temp, server, node, NULL);
}

void
net_ipx_send_game_list_request()
{
	// Send a broadcast request for game info

	IPX_sequence_packet me;

	memset(&me, 0, sizeof(IPX_sequence_packet));
	me.type = PID_GAME_LIST;
	memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );

	memcpy( me.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6 );
	memcpy( me.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );

	net_ipx_send_sequence_packet( me, NULL, NULL, NULL);
}

void net_ipx_send_all_info_request(char type,int which_security)
{
	// Send a broadcast request for game info

	IPX_sequence_packet me;

	memset(&me, 0, sizeof(IPX_sequence_packet));
	me.Security=which_security;
	me.type = type;
	memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	
	memcpy( me.player.protocol.ipx.node, ipxdrv_get_my_local_address(), 6 );
	memcpy( me.player.protocol.ipx.server, ipxdrv_get_my_server_address(), 4 );
	net_ipx_send_sequence_packet( me, NULL, NULL, NULL);
}


void
net_ipx_update_netgame(void)
{
	// Update the netgame struct with current game variables

	int i, j;

   Netgame.numconnected=0;
   for (i=0;i<N_players;i++)
	 if (Players[i].connected)
		Netgame.numconnected++;

// This is great: D2 1.0 and 1.1 ignore upper part of the game_flags field of
//	the lite_info struct when you're sitting on the join netgame screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.


	if (HoardEquipped())
	{
		if (Game_mode & GM_HOARD)
		{
			Netgame.game_flags |=NETGAME_FLAG_HOARD;
			if (Game_mode & GM_TEAM)
				Netgame.game_flags |=NETGAME_FLAG_TEAM_HOARD;
		}
	}
 
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
	memset(&end,0,sizeof(IPX_endlevel_info));
	end.type                = PID_ENDLEVEL;
	end.player_num = player_num;
	end.connected   = Players[player_num].connected;
	end.kills               = INTEL_SHORT(Players[player_num].net_kills_total);
	end.killed              = INTEL_SHORT(Players[player_num].net_killed_total);
	memcpy(end.kill_matrix, kill_matrix[player_num], MAX_PLAYERS*sizeof(short));
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

	for (i = 0; i < N_players; i++)
	{
		if ((i != Player_num) && (i!=player_num) && (Players[i].connected)) {
			if (Players[i].connected==CONNECT_PLAYING) {
				net_ipx_send_endlevel_short_sub(player_num,i);
			} else {
				ipxdrv_send_packet_data((ubyte *)&end, sizeof(IPX_endlevel_info), Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node,Players[i].net_address);
			}
		}
	}
}

/* Send an updated endlevel status to other hosts */
void
net_ipx_send_endlevel_packet(void)
{
	net_ipx_send_endlevel_sub(Player_num);
}


/* Send an endlevel packet for a player */
void
net_ipx_send_endlevel_short_sub(int from_player_num,int to_player)
{
	IPX_endlevel_info_short end;

	end.type = PID_ENDLEVEL_SHORT;
	end.player_num = from_player_num;
	end.connected = Players[from_player_num].connected;
	end.seconds_left = Countdown_seconds_left;


	if (Players[from_player_num].connected == CONNECT_PLAYING) // Still playing
	{
		Assert(Control_center_destroyed);
	}

	if ((to_player != Player_num) && (to_player!=from_player_num) && (Players[to_player].connected))
	{
		ipxdrv_send_packet_data((ubyte *)&end, sizeof(IPX_endlevel_info_short), Netgame.players[to_player].protocol.ipx.server, Netgame.players[to_player].protocol.ipx.node,Players[to_player].net_address);
	}
}

extern fix ThisLevelTime;

void
net_ipx_send_game_info(IPX_sequence_packet *their)
{
	// Send game info to someone who requested it

	char old_type, old_status;
	fix timevar;
	int i;

	net_ipx_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.protocol.ipx.Game_pkt_type;
	old_status = Netgame.game_status;

	Netgame.protocol.ipx.Game_pkt_type = PID_GAME_INFO;
	Netgame.protocol.ipx.Player_pkt_type = PID_PLAYERSINFO;

	Netgame.protocol.ipx.Player_Security=Netgame.protocol.ipx.Game_Security;
	Netgame.version_major=Version_major;
	Netgame.version_minor=Version_minor;

	if (Endlevel_sequence || Control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

	if (Netgame.PlayTimeAllowed)
	{
		timevar=i2f (Netgame.PlayTimeAllowed*5*60);
		i=f2i(timevar-ThisLevelTime);
		if (i<30)
			Netgame.game_status=NETSTAT_ENDLEVEL;
	}

	if (!their) {
		send_netgame_packet(NULL, NULL, NULL, 0);
		send_netplayers_packet(NULL, NULL);
	} else {
		send_netgame_packet(their->player.protocol.ipx.server, their->player.protocol.ipx.node, NULL, 0);
		send_netplayers_packet(their->player.protocol.ipx.server, their->player.protocol.ipx.node);
	}

	Netgame.protocol.ipx.Game_pkt_type = old_type;
	Netgame.game_status = old_status;
}

void net_ipx_send_lite_info(IPX_sequence_packet *their)
{
	// Send game info to someone who requested it

	char old_type, old_status,oldstatus;

	net_ipx_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.protocol.ipx.Game_pkt_type;
	old_status = Netgame.game_status;

	Netgame.protocol.ipx.Game_pkt_type = PID_LITE_INFO;

	if (Endlevel_sequence || Control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

	// If hoard mode, make this game look closed even if it isn't
	if (HoardEquipped())
	{
		if (Game_mode & GM_HOARD)
		{
			oldstatus=Netgame.game_status;
			Netgame.game_status=NETSTAT_ENDLEVEL;
			Netgame.gamemode=NETGAME_CAPTURE_FLAG;
			if (oldstatus==NETSTAT_ENDLEVEL)
				Netgame.game_flags|=NETGAME_FLAG_REALLY_ENDLEVEL;
			if (oldstatus==NETSTAT_STARTING)
				Netgame.game_flags|=NETGAME_FLAG_REALLY_FORMING;
		}
	}

	if (!their) {
		send_netgame_packet(NULL, NULL, NULL, 1);
	} else {
		send_netgame_packet(their->player.protocol.ipx.server, their->player.protocol.ipx.node, NULL, 1);
	}

	//  Restore the pre-hoard mode
	if (HoardEquipped())
	{
		if (Game_mode & GM_HOARD)
		{
			if (!(Game_mode & GM_TEAM))
				Netgame.gamemode=NETGAME_HOARD;
			else
				Netgame.gamemode=NETGAME_TEAM_HOARD;
			Netgame.game_flags&=~NETGAME_FLAG_REALLY_ENDLEVEL;
			Netgame.game_flags&=~NETGAME_FLAG_REALLY_FORMING;
			Netgame.game_flags&=~NETGAME_FLAG_TEAM_HOARD;
		}
	}

	Netgame.protocol.ipx.Game_pkt_type = old_type;
	Netgame.game_status = old_status;

}

/* Send game info to all players in this game */
void net_ipx_send_netgame_update()
{
	char old_type, old_status;
	int i;

	net_ipx_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.protocol.ipx.Game_pkt_type;
	old_status = Netgame.game_status;

	Netgame.protocol.ipx.Game_pkt_type = PID_GAME_UPDATE;

	if (Endlevel_sequence || Control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;
 
	for (i=0; i<N_players; i++ )    {
		if ( (Players[i].connected) && (i!=Player_num ) )       {
			send_netgame_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address, 1);
		}
	}

	Netgame.protocol.ipx.Game_pkt_type = old_type;
	Netgame.game_status = old_status;
}

int net_ipx_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

	if (Netgame.numplayers < 1)
	 return 1;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	  if (Netgame.players[i].connected)
	      break;

	Assert(i < MAX_NUM_NET_PLAYERS);

	IPX_Seq.type = PID_REQUEST;
	IPX_Seq.player.connected = Current_level_num;

	net_ipx_send_sequence_packet(IPX_Seq, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, NULL);

	return i;
}

int SecurityCheck=0;
	
void net_ipx_process_gameinfo(ubyte *data)
{
	int i, j;
	netgame_info *new = (netgame_info *)data;
	netgame_info tmp_info;

	receive_netgame_packet(data, &tmp_info, 0);                     // get correctly aligned structure
	new = &tmp_info;

	WaitingForPlayerInfo=0;
	
	if (new->protocol.ipx.Game_Security !=TempPlayersInfo->protocol.ipx.Game_Security)
	{
		Int3();     // Get Jason
		return;     // If this first half doesn't go with the second half
	}

	num_active_ipx_changed = 1;

	Assert (TempPlayersInfo!=NULL);

	for (i = 0; i < num_active_ipx_games; i++)
	{
		if (!stricmp(Active_ipx_games[i].game_name, new->game_name) && Active_ipx_games[i].protocol.ipx.Game_Security==new->protocol.ipx.Game_Security)
			break;
	}

	if (i == IPX_MAX_NETGAMES)
	{
		return;
	}
	
	memcpy(&Active_ipx_games[i], (ubyte *)new, sizeof(netgame_info));
	memcpy (&Active_ipx_games[i].players,TempPlayersInfo->players,sizeof(netplayer_info)*(MAX_PLAYERS+4));

	if (SecurityCheck)
		if (Active_ipx_games[i].protocol.ipx.Game_Security==SecurityCheck)
			SecurityCheck=-1;

	if (i == num_active_ipx_games)
		num_active_ipx_games++;

	if (Active_ipx_games[i].numplayers == 0)
	{
		// Delete this game
		for (j = i; j < num_active_ipx_games-1; j++)
		{
			memcpy(&Active_ipx_games[j], &Active_ipx_games[j+1], sizeof(netgame_info));
		}
		num_active_ipx_games--;
		SecurityCheck=0;
	}
}

void net_ipx_process_lite_info(ubyte *data)
{
	int i, j;
	netgame_info *new = (netgame_info *)data;
	netgame_info tmp_info;

	receive_netgame_packet(data, (netgame_info *)&tmp_info, 1);
	new = &tmp_info;

	num_active_ipx_changed = 1;

	for (i = 0; i < num_active_ipx_games; i++)
	{
		if ((!stricmp(Active_ipx_games[i].game_name, new->game_name)) && Active_ipx_games[i].protocol.ipx.Game_Security==new->protocol.ipx.Game_Security)
			break;
	}

	if (i == IPX_MAX_NETGAMES)
	{
		return;
	}
	
	memcpy(&Active_ipx_games[i], (ubyte *)new, sizeof(netgame_info));

	// See if this is really a Hoard game
	// If so, adjust all the data accordingly
	if (HoardEquipped())
	{
		if (Active_ipx_games[i].game_flags & NETGAME_FLAG_HOARD)
		{
			Active_ipx_games[i].gamemode=NETGAME_HOARD;
			Active_ipx_games[i].game_status=NETSTAT_PLAYING;
			
			if (Active_ipx_games[i].game_flags & NETGAME_FLAG_TEAM_HOARD)
				Active_ipx_games[i].gamemode=NETGAME_TEAM_HOARD;
			if (Active_ipx_games[i].game_flags & NETGAME_FLAG_REALLY_ENDLEVEL)
				Active_ipx_games[i].game_status=NETSTAT_ENDLEVEL;
			if (Active_ipx_games[i].game_flags & NETGAME_FLAG_REALLY_FORMING)
				Active_ipx_games[i].game_status=NETSTAT_STARTING;
		}
	}

	if (i == num_active_ipx_games)
		num_active_ipx_games++;

	if (Active_ipx_games[i].numplayers == 0)
	{
		// Delete this game
		for (j = i; j < num_active_ipx_games-1; j++)
		{
			memcpy(&Active_ipx_games[j], &Active_ipx_games[j+1], sizeof(netgame_info));
		}
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
					Function_mode = FMODE_MENU;
					nm_messagebox(NULL, 1, TXT_OK, "%s has kicked you out!",their->player.callsign);
					Function_mode = FMODE_GAME;
					multi_quit_game = 1;
					game_leave_menus();
					multi_reset_stuff();
					Function_mode = FMODE_MENU;
				}
				else
				{
					HUD_init_message ("%s attempted to kick you out.",their->player.callsign);
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

	for (i = 0; i < N_players; i++) {
		if (!memcmp(their->player.protocol.ipx.server, Netgame.players[i].protocol.ipx.server, 4) && !memcmp(their->player.protocol.ipx.node, Netgame.players[i].protocol.ipx.node, 6) && (!stricmp(their->player.callsign, Netgame.players[i].callsign))) {
			Players[i].connected = CONNECT_PLAYING;
			break;
		}
	}
}

extern void multi_reset_object_texture (object *);

void net_ipx_process_packet(ubyte *data, int length )
{
	IPX_sequence_packet *their = (IPX_sequence_packet *)data;
	IPX_sequence_packet tmp_packet;

	memset(&tmp_packet, 0, sizeof(IPX_sequence_packet));

	net_ipx_receive_sequence_packet(data, &tmp_packet);
	their = &tmp_packet;                                            // reassign their to point to correctly alinged structure

	length = length;

	switch( data[0] )
	{
	
		case PID_GAME_INFO:		// Jason L. says we can safely ignore this type.
			break;

		case PID_PLAYERSINFO:
			if (Network_status==NETSTAT_WAITING)
			{
				receive_netplayers_packet(data, &TempPlayersBase);
	
				if (TempPlayersBase.protocol.ipx.Player_Security!=Netgame.protocol.ipx.Game_Security)
				{
				break;
				}	
			
				TempPlayersInfo=&TempPlayersBase;
				WaitingForPlayerInfo=0;
				NetSecurityNum=TempPlayersInfo->protocol.ipx.Player_Security;
				NetSecurityFlag=NETSECURITY_WAIT_FOR_SYNC;
			}
		
			break;

		case PID_LITE_INFO:
			if (Network_status == NETSTAT_BROWSING)
				net_ipx_process_lite_info (data);

			break;

		case PID_GAME_LIST:
			// Someone wants a list of games
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}

			if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
				if (multi_i_am_master())
					net_ipx_send_lite_info(their);
	
			break;

		case PID_SEND_ALL_GAMEINFO:
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}
	
			if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
				if (multi_i_am_master() && their->Security==Netgame.protocol.ipx.Game_Security)
					net_ipx_send_game_info(their);
			break;
		
		case PID_ADDPLAYER:
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}

			net_ipx_new_player(their);

			break;
		case PID_REQUEST:
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}

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
				// Someone wants to join a game in progress!
				if (Netgame.RefusePlayers)
					net_ipx_do_refuse_stuff (their);
				else
					net_ipx_welcome_player(their);
			}

			break;

		case PID_DUMP:
	
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}
	
			if (Network_status == NETSTAT_WAITING || Network_status==NETSTAT_PLAYING )
				net_ipx_process_dump(their);

			break;
		case PID_QUIT_JOINING:
	
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}
			if (Network_status == NETSTAT_STARTING)
				net_ipx_remove_player( their );
			else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
				net_ipx_stop_resync( their );

			break;

		case PID_SYNC:
	
			if (Network_status == NETSTAT_WAITING)
			{
				receive_netgame_packet(data, &TempNetInfo, 0);
	
				if (TempNetInfo.protocol.ipx.Game_Security!=Netgame.protocol.ipx.Game_Security)
				{
					break;
				}
	
				if (NetSecurityFlag==NETSECURITY_WAIT_FOR_SYNC)
				{
					if (TempNetInfo.protocol.ipx.Game_Security==TempPlayersInfo->protocol.ipx.Player_Security)
					{
						net_ipx_read_sync_packet (&TempNetInfo,0);
						NetSecurityFlag=0;
						NetSecurityNum=0;
					}
				}
				else
				{
					NetSecurityFlag=NETSECURITY_WAIT_FOR_PLAYERS;
					NetSecurityNum=TempNetInfo.protocol.ipx.Game_Security;
		
					if ( net_ipx_wait_for_playerinfo())
						net_ipx_read_sync_packet((netgame_info *)data,0);
		
					NetSecurityFlag=0;
					NetSecurityNum=0;
				}
			}
			break;
	
		case PID_PDATA:
			if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING)) { 
				net_ipx_process_pdata((char *)data);
			}
			break;

		case PID_NAKED_PDATA:
			if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING)) 
				net_ipx_process_naked_pdata((char *)data,length);
			break;
	
		case PID_OBJECT_DATA:
			if (Network_status == NETSTAT_WAITING) 
				net_ipx_read_object_packet(data);
			break;
			
		case PID_ENDLEVEL:
			if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
				net_ipx_read_endlevel_packet(data);
			break;

		case PID_ENDLEVEL_SHORT:
			if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
				net_ipx_read_endlevel_short_packet(data);
			break;
	
		case PID_GAME_UPDATE:
			if (Network_status==NETSTAT_PLAYING)
			{
				memcpy ((ubyte *)&TempNetInfo,&Netgame,sizeof(netgame_info));
				receive_netgame_packet(data, &TempNetInfo, 1);

				if (TempNetInfo.protocol.ipx.Game_Security==Netgame.protocol.ipx.Game_Security)
					memcpy (&Netgame,(ubyte *)&TempNetInfo,sizeof(netgame_info));
			}

			if (Game_mode & GM_TEAM)
			{
				int i;
	
				for (i=0;i<N_players;i++)
				if (Players[i].connected)
				multi_reset_object_texture (&Objects[Players[i].objnum]);
				reset_cockpit();
			}
			break;

		case PID_PING_SEND:
			net_ipx_ping (PID_PING_RETURN,data[1]);
			break;

		case PID_PING_RETURN:
			net_ipx_handle_ping_return(data[1]);  // data[1] is player who told us of their ping time
			break;

		case PID_NAMES_RETURN:
			if (Network_status==NETSTAT_BROWSING && NamesInfoSecurity!=-1)
			net_ipx_process_names_return ((ubyte *) data);
			break;

		case PID_GAME_PLAYERS:
			// Someone wants a list of players in this game
	
			if (length != SEQUENCE_PACKET_SIZE)
			{
				return;
			}
				
			if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
				if (multi_i_am_master() && their->Security==Netgame.protocol.ipx.Game_Security)
					net_ipx_send_player_names(their);
			break;
	
		default:
			Int3(); // Invalid network packet type, see ROB
			break;
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
    
	if (playernum>=N_players)
	 {              
		Int3(); // weird, but it an happen in a coop restore game
		return; // if it happens in a coop restore, don't worry about it
	 }

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
net_ipx_read_endlevel_short_packet( ubyte *data )
{
	// Special packet for end of level syncing

	int playernum;
	IPX_endlevel_info_short *end;       

	end = (IPX_endlevel_info_short *)data;

	playernum = end->player_num;
	
	Assert(playernum != Player_num);
    
	if (playernum>=N_players)
	 {              
		Int3(); // weird, but it can happen in a coop restore game
		return; // if it happens in a coop restore, don't worry about it
	 }

	if ((Network_status == NETSTAT_PLAYING) && (end->connected != CONNECT_DISCONNECTED))
	 {
		return; // Only accept disconnect packets if we're not out of the level yet
	 }

	Players[playernum].connected = end->connected;

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

    if (got_controlcen && (MaxNumNetPlayers<=nplayers))
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
		objnum = INTEL_SHORT( *(short *)(data+loc) );                   loc += 2;
		obj_owner = data[loc];                                          loc += 1;
		remote_objnum = INTEL_SHORT( *(short *)(data+loc) );            loc += 2;

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
				//      Highest_object_index = objnum;
				//      num_objects = Highest_object_index+1;
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
				memcpy(obj, &data[loc], sizeof(object));
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

		if (GameArg.MplNoRankings)
	      sprintf( menus[N_players-1].text, "%d. %-20s", N_players,Netgame.players[N_players-1].callsign );
		else
	      sprintf( menus[N_players-1].text, "%d. %s%-20s", N_players, RankStrings[Netgame.players[N_players-1].rank],Netgame.players[N_players-1].callsign );

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
			if (GameArg.MplNoRankings)	
				sprintf( menus[i].text, "%d. %-20s", i+1, Netgame.players[i].callsign );
			else
				sprintf( menus[i].text, "%d. %s%-20s", i+1, RankStrings[Netgame.players[i].rank],Netgame.players[i].callsign );
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
	
	return 1;
}

static fix LastPTA;
static int LastKillGoal;
static int opt_cinvul, opt_team_anarchy, opt_coop, opt_show_on_map, opt_closed,opt_maxnet;
static int last_cinvul=0,last_maxnet,opt_team_hoard;
static int opt_refuse,opt_capture;
static int opt_setpower,opt_playtime,opt_killgoal,opt_marker_view,opt_light;
static int opt_difficulty,opt_packets, opt_bright,opt_start_invul, opt_show_names, opt_short_packets, opt_socket;

int net_ipx_game_param_poll( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	static int oldmaxnet=0;

	if (event->type != EVENT_IDLE)	// FIXME: Should become EVENT_ITEM_CHANGED in future
		return 0;
	
	if (((HoardEquipped() && menus[opt_team_hoard].value) || (menus[opt_team_anarchy].value || menus[opt_capture].value)) && !menus[opt_closed].value && !menus[opt_refuse].value) 
	{
		menus[opt_refuse].value = 1;
		menus[opt_refuse-1].value = 0;
		menus[opt_refuse-2].value = 0;
	}

	if (menus[opt_coop].value)
	{
		oldmaxnet=1;

		if (menus[opt_maxnet].value>2) 
		{
			menus[opt_maxnet].value=2;
		}

		if (menus[opt_maxnet].max_value>2)
		{
			menus[opt_maxnet].max_value=2;
		}

		if (!(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP))
			Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;

		if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
		{
		    Netgame.PlayTimeAllowed=0;
		    Netgame.KillGoal=0;
		}

	}
	else // if !Coop game
	{
		if (oldmaxnet)
		{
			oldmaxnet=0;
			menus[opt_maxnet].value=6;
			menus[opt_maxnet].max_value=6;
		}
	}

	if ( last_maxnet != menus[opt_maxnet].value )   
	{
		sprintf( menus[opt_maxnet].text, "Maximum players: %d", menus[opt_maxnet].value+2 );
		last_maxnet = menus[opt_maxnet].value;
	}
	
	return 0;
}

int net_ipx_get_game_params()
{
	int i;
	int opt, opt_name, opt_level, opt_mode,opt_moreopts;
	newmenu_item m[20];
	char slevel[5];
	char level_text[32];
	char srmaxnet[50];

	NamesInfoSecurity=-1;

	for (i=0;i<MAX_PLAYERS;i++)
		if (i!=Player_num)
			Players[i].callsign[0]=0;

	MaxNumNetPlayers = MAX_NUM_NET_PLAYERS;
	Netgame.KillGoal=0;
	Netgame.PlayTimeAllowed=0;
	Netgame.Allow_marker_view=1;
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.max_numplayers=MaxNumNetPlayers;
	sprintf( Netgame.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	last_cinvul = 0;
	Netgame.BrightPlayers = Netgame.InvulAppear = 1;
	Netgame.AllowedItems = 0;
	Netgame.AllowedItems |= NETFLAG_DOPOWERUP;

	if (!select_mission(1, TXT_MULTI_MISSION))
		return -1;

	strcpy(Netgame.mission_name, Current_mission_filename);
	strcpy(Netgame.mission_title, Current_mission_longname);

	sprintf( slevel, "1" );

	opt = 0;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_DESCRIPTION; opt++;

	opt_name = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = Netgame.game_name; m[opt].text_len = NETGAME_NAME_LEN; opt++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);

	Assert(strlen(level_text) < 32);

	m[opt].type = NM_TYPE_TEXT; m[opt].text = level_text; opt++;

	opt_level = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = slevel; m[opt].text_len=4; opt++;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_OPTIONS; opt++;

	opt_mode = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY; m[opt].value=(Netgame.gamemode == NETGAME_ANARCHY); m[opt].group=0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_TEAM_ANARCHY; m[opt].value=(Netgame.gamemode == NETGAME_TEAM_ANARCHY); m[opt].group=0; opt_team_anarchy=opt; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY_W_ROBOTS; m[opt].value=(Netgame.gamemode == NETGAME_ROBOT_ANARCHY); m[opt].group=0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_COOPERATIVE; m[opt].value=(Netgame.gamemode == NETGAME_COOPERATIVE); m[opt].group=0; opt_coop=opt; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Capture the flag"; m[opt].value=(Netgame.gamemode == NETGAME_CAPTURE_FLAG); m[opt].group=0; opt_capture=opt; opt++;

	if (HoardEquipped())
	{
		m[opt].type = NM_TYPE_RADIO; m[opt].text = "Hoard"; m[opt].value=(Netgame.gamemode & NETGAME_HOARD); m[opt].group=0; opt++;
		m[opt].type = NM_TYPE_RADIO; m[opt].text = "Team Hoard"; m[opt].value=(Netgame.gamemode & NETGAME_TEAM_HOARD); m[opt].group=0; opt_team_hoard=opt; opt++;
	}

	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Open game"; m[opt].group=1; m[opt].value=(!Netgame.RefusePlayers && !Netgame.game_flags & NETGAME_FLAG_CLOSED); opt++;
	opt_closed = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_CLOSED_GAME; m[opt].group=1; m[opt].value=Netgame.game_flags & NETGAME_FLAG_CLOSED; opt++;
	opt_refuse = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Restricted Game              "; m[opt].group=1; m[opt].value=Netgame.RefusePlayers; opt++;

	opt_maxnet = opt;
	sprintf( srmaxnet, "Maximum players: %d", MaxNumNetPlayers);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.max_numplayers-2; m[opt].text= srmaxnet; m[opt].min_value=0; 
	m[opt].max_value=MaxNumNetPlayers-2; opt++;
	last_maxnet=MaxNumNetPlayers-2;
	
	opt_moreopts=opt;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Advanced Options"; opt++;

	Assert(opt <= 20);

menu:
	i = newmenu_do1( NULL, NULL, opt, m, net_ipx_game_param_poll, NULL, 1 );

	if (i==opt_moreopts)
	{
		if ( m[opt_mode+3].value )
			Game_mode=GM_MULTI_COOP;
		net_ipx_more_game_options();
		Game_mode=0;
		goto menu;
	}
	Netgame.RefusePlayers=m[opt_refuse].value;

	if ( i > -1 )   {
		int j;

		MaxNumNetPlayers = m[opt_maxnet].value+2;
		Netgame.max_numplayers=MaxNumNetPlayers;

		for (j = 0; j < num_active_ipx_games; j++)
			if (!stricmp(Active_ipx_games[j].game_name, Netgame.game_name))
			{
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DUPLICATE_NAME);
				goto menu;
			}

		Netgame.levelnum = atoi(slevel);

		if ((Netgame.levelnum < 1) || (Netgame.levelnum > Last_level))
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
			sprintf(slevel, "1");
			goto menu;
		}
		if ( m[opt_mode].value )
			Netgame.gamemode = NETGAME_ANARCHY;
		else if (m[opt_mode+1].value) {
			Netgame.gamemode = NETGAME_TEAM_ANARCHY;
		}
		else if (m[opt_capture].value)
			Netgame.gamemode = NETGAME_CAPTURE_FLAG;
		else if (HoardEquipped() && m[opt_capture+1].value)
				Netgame.gamemode = NETGAME_HOARD;
		else if (HoardEquipped() && m[opt_capture+2].value)
				Netgame.gamemode = NETGAME_TEAM_HOARD;
		else if (ANARCHY_ONLY_MISSION) {
			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
			m[opt_mode+2].value = 0;
			m[opt_mode+3].value = 0;
			m[opt_mode].value = 1;
			goto menu;
		}
		else if ( m[opt_mode+2].value ) 
			Netgame.gamemode = NETGAME_ROBOT_ANARCHY;
		else if ( m[opt_mode+3].value ) 
			Netgame.gamemode = NETGAME_COOPERATIVE;
		else Int3(); // Invalid mode -- see Rob

		if (m[opt_closed].value)
			Netgame.game_flags |= NETGAME_FLAG_CLOSED;
		else
			Netgame.game_flags &= ~NETGAME_FLAG_CLOSED;
	}

	return i;
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
	else if (gamemode == NETGAME_CAPTURE_FLAG)
		{
		 Game_mode = GM_NETWORK | GM_TEAM | GM_CAPTURE;
		 Show_kill_list=3;
		}

	else if (HoardEquipped() && gamemode == NETGAME_HOARD)
		 Game_mode = GM_NETWORK | GM_HOARD;
	else if (HoardEquipped() && gamemode == NETGAME_TEAM_HOARD)
		 {
		  Game_mode = GM_NETWORK | GM_HOARD | GM_TEAM;
 		  Show_kill_list=3;
		 }
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
	t1 = timer_get_fixed_seconds() + F1_0*3;

	while (timer_get_fixed_seconds() < t1) // Wait 3 seconds for replies
		net_ipx_listen();

	if (num_active_ipx_games < IPX_MAX_NETGAMES)
		return 0;
	return 1;
}

void net_ipx_read_sync_packet( netgame_info * sp, int rsinit)
{
	int i, j;
	char temp_callsign[CALLSIGN_LEN+1];
	netgame_info tmp_info;

	if (sp != &Netgame)
	{
		receive_netgame_packet((ubyte *)sp, &tmp_info, 0);
		sp = &tmp_info;
	}

	if (rsinit)
		TempPlayersInfo=&Netgame;
	
	// This function is now called by all people entering the netgame.

	if (sp != &Netgame)
	{
		memcpy( &Netgame, sp, sizeof(netgame_info) );
		memcpy (&Netgame.players,TempPlayersInfo->players,sizeof(netplayer_info)*(MAX_PLAYERS+4));
	}

	N_players = sp->numplayers;
	Difficulty_level = sp->difficulty;
	Network_status = sp->game_status;

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
#ifdef RELEASE
		return;
#endif
	}

	// Discover my player number

	memcpy(temp_callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	
	Player_num = -1;

	for (i=0; i<MAX_NUM_NET_PLAYERS; i++ )  {
		Players[i].net_kills_total = 0;
	}

	for (i=0; i<N_players; i++ )    {
		if ( (!memcmp( TempPlayersInfo->players[i].protocol.ipx.node, IPX_Seq.player.protocol.ipx.node, 6 )) && (!stricmp( TempPlayersInfo->players[i].callsign, temp_callsign)) ) {
			if (Player_num!=-1) {
				Int3(); // Hey, we've found ourselves twice
				Network_status = NETSTAT_MENU;
				return; 
			}
			change_playernum_to(i);
		}
		memcpy( Players[i].callsign, TempPlayersInfo->players[i].callsign, CALLSIGN_LEN+1 );

#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy(&server, TempPlayersInfo->players[i].protocol.ipx.server, 4);
		if (server != 0)
			ipxdrv_get_local_target((ubyte *)&server, TempPlayersInfo->players[i].protocol.ipx.node, Players[i].net_address);
#else // WORDS_NEED_ALIGNMENT
		if ((*(uint *)TempPlayersInfo->players[i].protocol.ipx.server) != 0)
			ipxdrv_get_local_target(TempPlayersInfo->players[i].protocol.ipx.server, TempPlayersInfo->players[i].protocol.ipx.node, Players[i].net_address);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy( Players[i].net_address, TempPlayersInfo->players[i].protocol.ipx.node, 6 );

		Players[i].n_packets_got=0;                             // How many packets we got from them
		Players[i].n_packets_sent=0;                            // How many packets we sent to them
		Players[i].connected = TempPlayersInfo->players[i].connected;
		Players[i].net_kills_total = sp->player_kills[i];
		Players[i].net_killed_total = sp->killed[i];
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = sp->player_score[i];
		for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		{
			kill_matrix[i][j] = sp->kills[i][j];
		}
	}

	if ( Player_num < 0 )   {
		Network_status = NETSTAT_MENU;
		return;
	}

	if (Network_rejoined) 
		for (i=0; i<N_players;i++)
			Players[i].net_killed_total = sp->killed[i];

	if (Network_rejoined) {
		net_ipx_process_monitor_vector(sp->monitor_vector);
		Players[Player_num].time_level = sp->level_time;
	}

	team_kills[0] = sp->team_kills[0];
	team_kills[1] = sp->team_kills[1];
	
	Players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].rank=GetMyNetRanking();

	if (!Network_rejoined)
		for (i=0; i<NumNetPlayerPositions; i++) {
			Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
			Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
			obj_relink(Players[i].objnum,Player_init[Netgame.locations[i]].segnum);
		}

	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	Network_status = NETSTAT_PLAYING;
	Function_mode = FMODE_GAME;
	multi_sort_kill_list();

}

void
net_ipx_send_sync(void)
{
	int i, j, np;

	// Randomize their starting locations...

	d_srand( timer_get_fixed_seconds() );
	for (i=0; i<NumNetPlayerPositions; i++ )        
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

	for (i=0; i<N_players; i++ )    {
		if ((!Players[i].connected) || (i == Player_num))
			continue;

		// Send several times, extras will be ignored
		send_netgame_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, NULL, 0);
		send_netplayers_packet(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node);
	}

	net_ipx_read_sync_packet(&Netgame,1); // Read it myself, as if I had sent it
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
			#ifdef RELEASE
				goto menu;
			#endif
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
	int i, j;
	newmenu_item m[MAX_PLAYERS+4];
	char text[MAX_PLAYERS+4][45];
	char title[50];
	int save_nplayers;              //how may people would like to join

	net_ipx_add_player( &IPX_Seq );
		
	for (i=0; i< MAX_PLAYERS+4; i++ ) {
		sprintf( text[i], "%d.  %-20s", i+1, "" );
		m[i].type = NM_TYPE_CHECK; m[i].text = text[i]; m[i].value = 0;
	}

	m[0].value = 1;                         // Assume server will play...

	if (GameArg.MplNoRankings)
		sprintf( text[0], "%d. %-20s", 1, Players[Player_num].callsign );
	else
		sprintf( text[0], "%d. %s%-20s", 1, RankStrings[Netgame.players[Player_num].rank],Players[Player_num].callsign );

	sprintf( title, "%s %d %s", TXT_TEAM_SELECT, MaxNumNetPlayers, TXT_TEAM_PRESS_ENTER );

GetPlayersAgain:
	j=newmenu_do1( NULL, title, MAX_PLAYERS+4, m, net_ipx_start_poll, NULL, 1 );

	save_nplayers = N_players;

	if (j<0) 
	{
		// Aborted!
		// Dump all players and go back to menu mode

abort:
		for (i=1; i<save_nplayers; i++) {
			net_ipx_dump_player(Netgame.players[i].protocol.ipx.server,Netgame.players[i].protocol.ipx.node, DUMP_ABORTED);
		}

		Netgame.numplayers = 0;
		net_ipx_send_game_info(0); // Tell everyone we're bailing

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
	
	if ( N_players > Netgame.max_numplayers) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, MaxNumNetPlayers, TXT_NETPLAYERS_IN );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

#ifdef RELEASE
	if ( N_players < 2 )    {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

#ifdef RELEASE
	if ( (Netgame.gamemode == NETGAME_TEAM_ANARCHY || Netgame.gamemode == NETGAME_CAPTURE_FLAG || Netgame.gamemode == NETGAME_TEAM_HOARD) && (N_players < 2) )
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "You must select at least two\nplayers to start a team game" );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
	for (i=0; i<save_nplayers; i++ )
	{
		if (m[i].value) 
		{
			if (i > N_players)
			{
				memcpy(Netgame.players[N_players].protocol.ipx.node, Netgame.players[i].protocol.ipx.node, 6);
				memcpy(Netgame.players[N_players].protocol.ipx.server, Netgame.players[i].protocol.ipx.server, 4);
				memcpy(Netgame.players[N_players].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1);
				Netgame.players[N_players].version_major=Netgame.players[i].version_major;
				Netgame.players[N_players].version_minor=Netgame.players[i].version_minor;
				Netgame.players[N_players].rank=Netgame.players[i].rank;
				ClipRank (&Netgame.players[N_players].rank);
				net_ipx_check_for_old_version(i);
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
		memset(Netgame.players[i].protocol.ipx.node, 0, 6);
		memset(Netgame.players[i].protocol.ipx.server, 0, 4);
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN+1);
		Netgame.players[i].version_major=0;
		Netgame.players[i].version_minor=0;
		Netgame.players[i].rank=0;
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY ||
	    Netgame.gamemode == NETGAME_CAPTURE_FLAG ||
		 Netgame.gamemode == NETGAME_TEAM_HOARD)
		 if (!net_ipx_select_teams())
			goto abort;

	return(1);
}

void 
net_ipx_start_game()
{
	int i;

	Assert( FRAME_INFO_SIZE < MAX_DATA_SIZE );
	if ( !IPX_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND );
		return;
	}

	net_ipx_init();
	change_playernum_to(0);

	if (net_ipx_find_game())
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_NET_FULL);
		return;
	}

	i = net_ipx_get_game_params();

	if (i<0) return;

	if (IPX_Socket) {
		ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
	}

	if (is_D2_OEM)
		IPX_Seq.player.version_minor|=0x10;
	N_players = 0;
	Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	Netgame.protocol.ipx.protocol_version = MULTI_PROTO_VERSION;

	Network_status = NETSTAT_STARTING;

	net_ipx_set_game_mode(Netgame.gamemode);

	d_srand( timer_get_fixed_seconds() );
	Netgame.protocol.ipx.Game_Security=d_rand();  // For syncing Netgames with player packets
	
	if(net_ipx_select_players())
	{
		StartNewLevel(Netgame.levelnum, 0);
	}
	else
		Game_mode = GM_GAME_OVER;
	
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

	NamesInfoSecurity=-1;
	num_active_ipx_changed = 1;      
}

char *ModeLetters[]={"ANRCHY","TEAM","ROBO","COOP","FLAG","HOARD","TMHOARD"};

int net_ipx_join_poll( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for Join Game menu
	newmenu_item *menus = newmenu_get_items(menu);
	static fix t1 = 0;
	int i, osocket,join_status,temp;
	int key = 0;
	int rval = 0;

	if (event->type == EVENT_KEY_COMMAND)
		key = ((d_event_keycommand *)event)->keycode;
	else if (event->type != EVENT_IDLE)
		return 0;
	
	userdata = userdata;

	if ( IPX_allow_socket_changes ) {
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

	// send a request for game info every 3 seconds

	if (timer_get_fixed_seconds() > t1+F1_0*3) {
		t1 = timer_get_fixed_seconds();
		net_ipx_send_game_list_request();
	}

	temp=num_active_ipx_games;

	net_ipx_listen();

	if (!num_active_ipx_changed)
		return rval;

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


		nplayers=Active_ipx_games[i].numconnected;

		if (Active_ipx_games[i].levelnum < 0)
			sprintf(levelname, "S%d", -Active_ipx_games[i].levelnum);
		else
			sprintf(levelname, "%d", Active_ipx_games[i].levelnum);

		if (game_status == NETSTAT_STARTING)
		{
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
					 Active_ipx_games[i].max_numplayers,MissName,levelname,"Forming");
		}
		else if (game_status == NETSTAT_PLAYING)
		{
			join_status=net_ipx_can_join_netgame(&Active_ipx_games[i]);

			if (join_status==1)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Open");
			else if (join_status==2)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Full");
			else if (join_status==3)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Restrict");
			else
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
						 Active_ipx_games[i].max_numplayers,MissName,levelname,"Closed");

		}
		else
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,ModeLetters[Active_ipx_games[i].gamemode],nplayers,
					 Active_ipx_games[i].max_numplayers,MissName,levelname,"Between");


		Assert(strlen(menus[i+2].text) < 100);
	}

	for (i = num_active_ipx_games; i < IPX_MAX_NETGAMES; i++)
	{
		sprintf(menus[i+2].text, "%d.                                                     ",i+1);
	}
	
	return rval;
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
		net_ipx_send_sequence_packet(me, Netgame.players[0].protocol.ipx.server, Netgame.players[0].protocol.ipx.node, NULL);
		N_players = 0;
		Function_mode = FMODE_MENU;
		Game_mode = GM_GAME_OVER;
		return(-1);     // they cancelled
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

//      if (timer_get_fixed_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
//      {
//              net_ipx_send_endlevel_packet();
//              t1 = timer_get_fixed_seconds();
//      }

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

void
net_ipx_wait_for_requests(void)
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
		if (choice == 2) {
			N_players = 1;
			return;
		}
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (i=0; i < N_players; i++) {
			if ((Players[i].connected != CONNECT_DISCONNECTED) && (i != Player_num)) {
				net_ipx_dump_player(Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, DUMP_ABORTED);
			}
		}
		if (Game_wind)
			window_close(Game_wind);
	}
	else if (choice != -2)
		goto menu;
}

/* Do required syncing after each level, before starting new one */
int
net_ipx_level_sync(void)
{
	int result;

	MySyncPackInitialized = 0;

	net_ipx_flush(); // Flush any old packets

	if (N_players == 0)
		result = net_ipx_wait_for_sync();
	else if (multi_i_am_master())
	{
		net_ipx_wait_for_requests();
		net_ipx_send_sync();
		result = 0;
	}
	else
		result = net_ipx_wait_for_sync();

   net_ipx_count_powerups_in_mine();

	if (result)
	{
		Players[Player_num].connected = CONNECT_DISCONNECTED;
		net_ipx_send_endlevel_packet();
		if (Game_wind)
			window_close(Game_wind);
	}
	return(0);
}

void net_ipx_count_powerups_in_mine(void)
 {
  int i;

  for (i=0;i<MAX_POWERUP_TYPES;i++)
	PowerupsInMine[i]=0;
	
  for (i=0;i<=Highest_object_index;i++) 
	{
	 if (Objects[i].type==OBJ_POWERUP)
	  {
		PowerupsInMine[Objects[i].id]++;
		if (multi_powerup_is_4pack(Objects[i].id))
		   PowerupsInMine[Objects[i].id-1]+=4;
	  }
	}
		  
 }

int net_ipx_do_join_game(int choice)
{
	if (Active_ipx_games[choice].protocol.ipx.protocol_version != MULTI_PROTO_VERSION)
	{
        nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		return 0;
	}

	if (Active_ipx_games[choice].game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	// Check for valid mission name
	if (!load_mission_by_name(Active_ipx_games[choice].mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}

	if (is_D2_OEM)
	{
		IPX_Seq.player.version_minor|=0x10;
		if (Active_ipx_games[choice].levelnum>8)
		{
			nm_messagebox(NULL, 1, TXT_OK, "This OEM version only supports\nthe first 8 levels!");
			return 0;
		}
	}

	if (is_MAC_SHARE)
	{
		if (Active_ipx_games[choice].levelnum > 4)
		{
			nm_messagebox(NULL, 1, TXT_OK, "This SHAREWARE version only supports\nthe first 4 levels!");
			return 0;
		}
	}

	if (!net_ipx_wait_for_all_info (0))
	{
		nm_messagebox (TXT_SORRY,1,TXT_OK,"There was a join error!");
		Network_status = NETSTAT_BROWSING; // We are looking at a game menu
		return 0;
	}

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	if (!net_ipx_can_join_netgame(&Active_ipx_games[choice]))
	{
		if (Active_ipx_games[0].numplayers == Active_ipx_games[0].max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return 0;
	}

	// Choice is valid, prepare to join in
	memcpy (&Netgame, &Active_ipx_games[choice], sizeof(netgame_info));
	memcpy (&Netgame.players,TempPlayersInfo->players,sizeof(netplayer_info)*(MAX_PLAYERS+4));

	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	net_ipx_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum, 0);
	
	return 1;     // look ma, we're in a game!!!
}


void nm_draw_background1(char * filename);
int net_ipx_show_game_stats(int choice);
int net_ipx_do_join_game(int choice);

void net_ipx_join_game()
{
	int choice, i;
	char menu_text[IPX_MAX_NETGAMES+2][200];
	
	newmenu_item m[IPX_MAX_NETGAMES+2];

	if ( !IPX_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	net_ipx_init();

	N_players = 0;

	// FIXME: Keep browsing window to go back to
	//setjmp(LeaveGame);
	
	Network_send_objects = 0; 
	Network_sending_extras=0;
	Network_rejoined=0;
	  
	Network_status = NETSTAT_BROWSING; // We are looking at a game menu
	
	net_ipx_flush();
	net_ipx_listen();  // Throw out old info

	net_ipx_send_game_list_request(); // broadcast a request for lists

	num_active_ipx_games = 0;

	memset(m, 0, sizeof(newmenu_item)*IPX_MAX_NETGAMES);
	memset(Active_ipx_games, 0, sizeof(netgame_info)*IPX_MAX_NETGAMES);
	
	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	m[0].text = menu_text[0];
	m[0].type = NM_TYPE_TEXT;
	if (IPX_allow_socket_changes)
		sprintf( m[0].text, "\tCurrent IPX Socket is default %+d (PgUp/PgDn to change)", IPX_Socket );
	else
		strcpy( m[0].text, "" ); //sprintf( m[0].text, "" );

	m[1].text=menu_text[1];
	m[1].type=NM_TYPE_TEXT;
	sprintf (m[1].text,"\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (i = 0; i < IPX_MAX_NETGAMES; i++) {
		m[i+2].text = menu_text[i+2];
		m[i+2].type = NM_TYPE_MENU;
		sprintf(m[i+2].text, "%d.                                                                   ", i+1);
	}

	num_active_ipx_changed = 1;
remenu:
	SurfingNet=1;
	nm_draw_background1(Menu_pcx_name);             //load this here so if we abort after loading level, we restore the palette
	gr_palette_load(gr_palette);
	choice=newmenu_dotiny("NETGAMES", NULL,IPX_MAX_NETGAMES+2, m, net_ipx_join_poll, NULL);
	SurfingNet=0;

	if (choice==-1) {
		Network_status = NETSTAT_MENU;
		return; // they cancelled
	}
	choice-=2;

	if (choice >=num_active_ipx_games)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
		goto remenu;
	}

	if (net_ipx_show_game_stats(choice)==0)
		goto remenu;

	// Choice has been made and looks legit
	if (net_ipx_do_join_game(choice)==0)
		goto remenu;

	return;         // look ma, we're in a game!!!
}

fix StartWaitAllTime=0;
int WaitAllChoice=0;
#define ALL_INFO_REQUEST_INTERVAL F1_0*3

int net_ipx_wait_all_poll( newmenu *menu, d_event *event, void *userdata )
 {
  static fix t1=0;

  menu=menu;
  userdata=userdata;

	 if (event->type != EVENT_IDLE)
		 return 0;
	 
  if (timer_get_fixed_seconds() > t1+ALL_INFO_REQUEST_INTERVAL)
	{
		net_ipx_send_all_info_request(PID_SEND_ALL_GAMEINFO,SecurityCheck);
		t1 = timer_get_fixed_seconds();
	}

  net_ipx_do_big_wait(WaitAllChoice);  
   
  if(SecurityCheck==-1)
	  return -2;
	 
	 return 0;
 }
 
int net_ipx_wait_for_all_info (int choice)
 {
  int pick;
  
  newmenu_item m[2];

  m[0].type=NM_TYPE_TEXT; m[0].text = "Press Escape to cancel";

  WaitAllChoice=choice;
  StartWaitAllTime=timer_get_fixed_seconds();
  SecurityCheck=Active_ipx_games[choice].protocol.ipx.Game_Security;
  NetSecurityFlag=0;

  GetMenu:
  pick=newmenu_do( NULL, "Connecting...", 1, m, net_ipx_wait_all_poll, NULL );

  if (pick>-1 && SecurityCheck!=-1)
	goto GetMenu;

  if (SecurityCheck==-1)
    {   
	   SecurityCheck=0;     
	   return (1);
	 }
  SecurityCheck=0;      
  return (0);
 }

void net_ipx_do_big_wait(int choice)
{
	int size;
	ubyte packet[MAX_DATA_SIZE],*data;
	netgame_info *temp_info;
	netgame_info info_struct;

	size=ipxdrv_get_packet_data( packet );
	
	if (size>0)
	{
		data = packet;

		switch (data[0])
		{
			case PID_GAME_INFO:
				receive_netgame_packet(data, &TempNetInfo, 0);

				if (TempNetInfo.protocol.ipx.Game_Security !=SecurityCheck)
				{
					break;
				}
						
				if (NetSecurityFlag==NETSECURITY_WAIT_FOR_GAMEINFO)
				{
					if (TempPlayersInfo->protocol.ipx.Player_Security==TempNetInfo.protocol.ipx.Game_Security)
					{
						if (TempPlayersInfo->protocol.ipx.Player_Security==SecurityCheck)
						{
							memcpy (&Active_ipx_games[choice],(ubyte *)&TempNetInfo,sizeof(netgame_info));
							memcpy (&Active_ipx_games[choice].players, TempPlayersInfo->players, sizeof(netplayer_info)*(MAX_PLAYERS+4));
							SecurityCheck=-1;
						}
					}
				}
				else
				{
					NetSecurityFlag=NETSECURITY_WAIT_FOR_PLAYERS;
					NetSecurityNum=TempNetInfo.protocol.ipx.Game_Security;
	
					if (net_ipx_wait_for_playerinfo())
					{
						memcpy (&Active_ipx_games[choice],(ubyte *)&TempNetInfo,sizeof(netgame_info));
						memcpy (&Active_ipx_games[choice].players, TempPlayersInfo->players, sizeof(netplayer_info)*(MAX_PLAYERS+4));
						SecurityCheck=-1;
					}
	
					NetSecurityFlag=0;
					NetSecurityNum=0;
				}
				break;

			case PID_PLAYERSINFO:
				receive_netplayers_packet(data, &info_struct);
				temp_info = &info_struct;

				if (temp_info->protocol.ipx.Player_Security!=SecurityCheck) 
					break;     // If this isn't the guy we're looking for, move on
	
				memcpy (&TempPlayersBase,(ubyte *)&temp_info,sizeof(netgame_info));
				TempPlayersInfo=&TempPlayersBase;
				WaitingForPlayerInfo=0;
				NetSecurityNum=TempPlayersInfo->protocol.ipx.Player_Security;
				NetSecurityFlag=NETSECURITY_WAIT_FOR_GAMEINFO;
				break;
		}
	}
}

void net_ipx_leave_game()
{
	int nsave;

	net_ipx_do_frame(1, 1);

#ifdef NETPROFILING
	fclose (SendLogFile);
	fclose (RecieveLogFile);
#endif

	if ((multi_i_am_master()))
	{
		while (Network_sending_extras>1 && Player_joining_extras!=-1)
			net_ipx_send_extras();

		Netgame.numplayers = 0;
		nsave=N_players;
		N_players=0;
		net_ipx_send_game_info(NULL);
		N_players=nsave;
	
	}

	Players[Player_num].connected = CONNECT_DISCONNECTED;	
	net_ipx_send_endlevel_packet();
	change_playernum_to(0);
	Game_mode = GM_GAME_OVER;
	write_player_file();

	net_ipx_flush();
}

void net_ipx_flush()
{
	ubyte packet[MAX_DATA_SIZE];

	if (!IPX_active) return;

	while (ipxdrv_get_packet_data(packet) > 0) ;
}

void net_ipx_listen()
{
	int size;
	ubyte packet[MAX_DATA_SIZE];
	int i,loopmax=999;

	if (Network_status==NETSTAT_PLAYING && Netgame.protocol.ipx.ShortPackets && !Network_send_objects)
	{
		loopmax=N_players*Netgame.PacketsPerSec;
	}
	
	if (!IPX_active) return;

	WaitingForPlayerInfo=1;
	NetSecurityFlag=NETSECURITY_OFF;

	i=1;
	size = ipxdrv_get_packet_data( packet );
	while ( size > 0)       {
		net_ipx_process_packet( packet, size );
		if (++i>loopmax)
			break;
		size = ipxdrv_get_packet_data( packet );
	}
}

int net_ipx_wait_for_playerinfo()
{
	int size,retries=0;
	ubyte packet[MAX_DATA_SIZE];
	struct netgame_info *TempInfo;
	fix basetime;
	ubyte id;
	netgame_info info_struct;

	if (!IPX_active) return(0);

	if (Network_status==NETSTAT_PLAYING)
	{
		Int3(); //MY GOD! Get Jason...this is the source of many problems
		return (0);
	}

	basetime=timer_get_fixed_seconds();

	while (WaitingForPlayerInfo && retries<50 && (timer_get_fixed_seconds()<(basetime+(F1_0*5))))
	{
		size = ipxdrv_get_packet_data( packet );
		id = packet[0];

		if (size>0 && id==PID_PLAYERSINFO)
		{
			receive_netplayers_packet(packet, &info_struct);
			TempInfo = &info_struct;
			retries++;
	
			if (NetSecurityFlag==NETSECURITY_WAIT_FOR_PLAYERS)
			{
				if (NetSecurityNum==TempInfo->protocol.ipx.Player_Security)
				{
					memcpy (&TempPlayersBase,(ubyte *)TempInfo,sizeof(netgame_info));
					TempPlayersInfo=&TempPlayersBase;
					NetSecurityFlag=NETSECURITY_OFF;
					NetSecurityNum=0;
					WaitingForPlayerInfo=0;
					return (1);
				}
				else
					continue;
			}
			else
			{
				NetSecurityNum=TempInfo->protocol.ipx.Player_Security;
				NetSecurityFlag=NETSECURITY_WAIT_FOR_GAMEINFO;
				
				memcpy (&TempPlayersBase,(ubyte *)TempInfo,sizeof(netgame_info));
				TempPlayersInfo=&TempPlayersBase;
				WaitingForPlayerInfo=0;
				return (1);
			}
		}
	}

	return (0);
}


void net_ipx_send_data( ubyte * ptr, int len, int urgent )
{
	char check;

   #ifdef NETPROFILING
	   TTSent[ptr[0]]++;  
	   fprintf (SendLogFile,"Packet type: %d Len:%d Urgent:%d TT=%d\n",ptr[0],len,urgent,TTSent[ptr[0]]);
	   fflush (SendLogFile);
	#endif
    
	if (Endlevel_sequence)
		return;

	if (!MySyncPackInitialized)     {
		MySyncPackInitialized = 1;
		memset( &MySyncPack, 0, sizeof(IPX_frame_info) );
	}
	
	if (urgent)
		PacketUrgent = 1;

	if ((MySyncPack.data_size+len) > NET_XDATA_SIZE ) {
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

	HUD_init_message("%s %s", Players[playernum].callsign, TXT_DISCONNECTING);
	for (i = 0; i < N_players; i++)
		if (Players[i].connected) 
			n++;

	if (n == 1)
	{
		HUD_init_message("You are the only person remaining in this netgame");
		//nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
	}
}

#ifdef WORDS_BIGENDIAN
void squish_short_frame_info(IPX_short_frame_info old_info, ubyte *data)
{
	int loc = 0;
	int tmpi;
	short tmps;
	
	data[0] = old_info.type;                                            loc++;
	/* skip three for pad byte */                                       loc += 3;
	tmpi = INTEL_INT(old_info.numpackets);
	memcpy(&(data[loc]), &tmpi, 4);                                     loc += 4;

	memcpy(&(data[loc]), old_info.thepos.bytemat, 9);                   loc += 9;
	tmps = INTEL_SHORT(old_info.thepos.xo);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.yo);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.zo);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.segment);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.velx);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.vely);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;
	tmps = INTEL_SHORT(old_info.thepos.velz);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;

	tmps = INTEL_SHORT(old_info.data_size);
	memcpy(&(data[loc]), &tmps, 2);                                     loc += 2;

	data[loc] = old_info.playernum;                                     loc++;
	data[loc] = old_info.obj_render_type;                               loc++;
	data[loc] = old_info.level_num;                                     loc++;
	memcpy(&(data[loc]), old_info.data, old_info.data_size);
}
#endif

char NakedBuf[NET_XDATA_SIZE+4];
int NakedPacketLen=0;
int NakedPacketDestPlayer=-1;

void net_ipx_do_frame(int force, int listen)
{
	int i;
	IPX_short_frame_info ShortSyncPack;
	static fix LastEndlevel=0;
	static fix last_send_time = 0, last_timeout_check = 0;

	if (!(Game_mode&GM_NETWORK)) return;

	net_ipx_ping_all(timer_get_fixed_seconds());

	if ((Network_status != NETSTAT_PLAYING) || (Endlevel_sequence)) // Don't send postion during escape sequence...
		goto listen;

	if (NakedPacketLen)
	{
		Assert (NakedPacketDestPlayer>-1);
		ipxdrv_send_packet_data( (ubyte *)NakedBuf, NakedPacketLen, Netgame.players[NakedPacketDestPlayer].protocol.ipx.server, Netgame.players[NakedPacketDestPlayer].protocol.ipx.node,Players[NakedPacketDestPlayer].net_address );
		NakedPacketLen=0;
		NakedPacketDestPlayer=-1;
	}

	if (WaitForRefuseAnswer && timer_get_fixed_seconds()>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	last_send_time += FrameTime;
	last_timeout_check += FrameTime;

	// Send out packet PacksPerSec times per second maximum... unless they fire, then send more often...
	if ( (last_send_time>F1_0/Netgame.PacketsPerSec) || (Network_laser_fired) || force || PacketUrgent )
	{
		if ( Players[Player_num].connected )    {
			int objnum = Players[Player_num].objnum;

			if (listen) {
				multi_send_robot_frame(0);
				multi_send_fire();              // Do firing if needed..
			}

			last_send_time = 0;

			if (Netgame.protocol.ipx.ShortPackets)
			{
#ifdef WORDS_BIGENDIAN
				ubyte send_data[MAX_DATA_SIZE];
				//int squished_size;
#endif
				int i;

				memset(&ShortSyncPack,0,sizeof(IPX_short_frame_info));
				create_shortpos(&ShortSyncPack.thepos, Objects+objnum, 0);
				ShortSyncPack.type                                      = PID_PDATA;
				ShortSyncPack.playernum                         = Player_num;
				ShortSyncPack.obj_render_type           = Objects[objnum].render_type;
				ShortSyncPack.level_num                         = Current_level_num;
				ShortSyncPack.data_size                         = MySyncPack.data_size;
				memcpy (&ShortSyncPack.data[0],&MySyncPack.data[0],MySyncPack.data_size);

				MySyncPack.numpackets = INTEL_INT(Players[0].n_packets_sent++);
				ShortSyncPack.numpackets = MySyncPack.numpackets;
#ifndef WORDS_BIGENDIAN
// 				ipxdrvSendGamePacket((ubyte*)&ShortSyncPack, sizeof(IPX_short_frame_info) - MaxXDataSize + MySyncPack.data_size);
				for(i=0; i<N_players; i++) {
					if(Players[i].connected && (i != Player_num))
						ipxdrv_send_packet_data((ubyte*)&ShortSyncPack, sizeof(IPX_short_frame_info) - NET_XDATA_SIZE + MySyncPack.data_size, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node,Players[i].net_address);
				}
#else
				squish_short_frame_info(ShortSyncPack, send_data);
// 				ipxdrvSendGamePacket((ubyte*)send_data, IPX_SHORT_INFO_SIZE-MaxXDataSize+MySyncPack.data_size);
				for(i=0; i<N_players; i++) {
					if(Players[i].connected && (i != Player_num))
						ipxdrv_send_packet_data((ubyte*)send_data, IPX_SHORT_INFO_SIZE-NET_XDATA_SIZE+MySyncPack.data_size, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node,Players[i].net_address);
				}
#endif

			}
			else  // If long packets
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

// 				ipxdrvSendGamePacket((ubyte*)&MySyncPack, sizeof(IPX_frame_info) - MaxXDataSize + send_data_size);
				for(i=0; i<N_players; i++)
				{
					if(Players[i].connected && (i != Player_num))
					{
						ipxdrv_send_packet_data((ubyte *)&MySyncPack, sizeof(IPX_frame_info) - NET_XDATA_SIZE + (&MySyncPack)->data_size, Netgame.players[i].protocol.ipx.server, Netgame.players[i].protocol.ipx.node, Players[i].net_address);
					}
				}
			}
			
			PacketUrgent = 0;
			MySyncPack.data_size = 0;               // Start data over at 0 length.
			
			if (Control_center_destroyed)
			{
				if (Player_is_dead)
					Players[Player_num].connected=CONNECT_DIED_IN_MINE;
				if (timer_get_fixed_seconds() > (LastEndlevel+(F1_0/2)))
				{
					net_ipx_send_endlevel_packet();
					LastEndlevel=timer_get_fixed_seconds();
				}
			}
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
			if(Players[i].connected != CONNECT_PLAYING && Objects[Players[i].objnum].type != OBJ_GHOST)
			{
				HUD_init_message( "'%s' has left.", Players[i].callsign );
				multi_make_player_ghost(i);
			}
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

	if (VerifyPlayerJoined!=-1 && !(FrameCount & 63))
		resend_sync_due_to_packet_loss_for_allender(); // This will resend to IPX_sync_player
 
	if (Network_send_objects)
		net_ipx_send_objects();

	if (Network_sending_extras && VerifyPlayerJoined==-1)
	{
		net_ipx_send_extras();
		return;
	}
}

void net_ipx_process_pdata (char *data)
 {
  Assert (Game_mode & GM_NETWORK);
 
  if (Netgame.protocol.ipx.ShortPackets)
	net_ipx_read_pdata_short_packet ((IPX_short_frame_info *)data);
  else
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

	if (VerifyPlayerJoined!=-1 && TheirPlayernum==VerifyPlayerJoined)
	{
		// Hurray! Someone really really got in the game (I think).
		VerifyPlayerJoined=-1;
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
	IPX_TotalPacketsGot++;
	Netgame.players[TheirPlayernum].LastPacketTime = timer_get_fixed_seconds();

	if  ( pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
		if ((pd->numpackets-Players[TheirPlayernum].n_packets_got)>0)
			IPX_TotalMissedPackets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

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
		
		ClipRank (&Netgame.players[TheirPlayernum].rank);

		if (GameArg.MplNoRankings)      
			HUD_init_message( "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );
		else
			HUD_init_message( "%s'%s' %s", RankStrings[Netgame.players[TheirPlayernum].rank],Players[TheirPlayernum].callsign, TXT_REJOIN );


		multi_send_score();
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )  {
		// pass pd->data to some parser function....
		multi_process_bigdata( pd->data, pd->data_size );
	}

}

void net_ipx_read_pdata_short_packet(IPX_short_frame_info *pd)
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
	if (!multi_quit_game && (TheirPlayernum >= N_players)) {
		if (Network_status!=NETSTAT_WAITING)
		 {
			Int3(); // We missed an important packet!
			multi_consistency_error(0);
			return;
		 }
		else
		 return;
	}

	if (VerifyPlayerJoined!=-1 && TheirPlayernum==VerifyPlayerJoined)
	{
		// Hurray! Someone really really got in the game (I think).
		VerifyPlayerJoined=-1;
	}

	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL) ) {
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		if ( pd->data_size>0 )       {
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
	IPX_TotalPacketsGot++;
	Netgame.players[TheirPlayernum].LastPacketTime = timer_get_fixed_seconds();

	if  ( pd->numpackets != Players[TheirPlayernum].n_packets_got )      {
		if ((pd->numpackets-Players[TheirPlayernum].n_packets_got)>0)
			IPX_TotalMissedPackets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------

	extract_shortpos(TheirObj, &pd->thepos, 0);

	if ((TheirObj->render_type != pd->obj_render_type) && (pd->obj_render_type == RT_POLYOBJ))
		multi_make_ghost_player(TheirPlayernum);

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
		ClipRank (&Netgame.players[TheirPlayernum].rank);
		
		if (GameArg.MplNoRankings)
			HUD_init_message( "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );
		else
			HUD_init_message( "%s'%s' %s", RankStrings[Netgame.players[TheirPlayernum].rank],Players[TheirPlayernum].callsign, TXT_REJOIN );


		multi_send_score();
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )       {
		// pass pd->data to some parser function....
		multi_process_bigdata( pd->data, pd->data_size );
	}

}

void net_ipx_set_power (void)
{
/*
	int opt=0,choice,opt_primary,opt_second,opt_power;
	newmenu_item m[40];

	opt_primary=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Laser upgrade"; m[opt].value=Netgame.DoLaserUpgrade; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Super lasers"; m[opt].value=Netgame.DoSuperLaser; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Quad Lasers"; m[opt].value=Netgame.DoQuadLasers; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Vulcan cannon"; m[opt].value=Netgame.DoVulcan; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Spreadfire cannon"; m[opt].value=Netgame.DoSpread; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Plasma cannon"; m[opt].value=Netgame.DoPlasma; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Fusion cannon"; m[opt].value=Netgame.DoFusions; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Gauss cannon"; m[opt].value=Netgame.DoGauss; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Helix cannon"; m[opt].value=Netgame.DoHelix; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Phoenix cannon"; m[opt].value=Netgame.DoPhoenix; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Omega cannon"; m[opt].value=Netgame.DoOmega; opt++;

	opt_second=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Homing Missiles"; m[opt].value=Netgame.DoHoming; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Proximity Bombs"; m[opt].value=Netgame.DoProximity; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Smart Missiles"; m[opt].value=Netgame.DoSmarts; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Mega Missiles"; m[opt].value=Netgame.DoMegas; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Flash Missiles"; m[opt].value=Netgame.DoFlash; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Guided Missiles"; m[opt].value=Netgame.DoGuided; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Smart Mines"; m[opt].value=Netgame.DoSmartMine; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Mercury Missiles"; m[opt].value=Netgame.DoMercury; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "EarthShaker Missiles"; m[opt].value=Netgame.DoEarthShaker; opt++;

	opt_power=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Invulnerability"; m[opt].value=Netgame.DoInvulnerability; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Cloaking"; m[opt].value=Netgame.DoCloak; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Afterburners"; m[opt].value=Netgame.DoAfterburner; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Ammo rack"; m[opt].value=Netgame.DoAmmoRack; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Energy Converter"; m[opt].value=Netgame.DoConverter; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Headlight"; m[opt].value=Netgame.DoHeadlight; opt++;

	choice = newmenu_do(NULL, "Objects to allow", opt, m, NULL);

	Netgame.DoLaserUpgrade=m[opt_primary].value; 
	Netgame.DoSuperLaser=m[opt_primary+1].value;
	Netgame.DoQuadLasers=m[opt_primary+2].value;  
	Netgame.DoVulcan=m[opt_primary+3].value;
	Netgame.DoSpread=m[opt_primary+4].value;
	Netgame.DoPlasma=m[opt_primary+5].value;
	Netgame.DoFusions=m[opt_primary+6].value;
	Netgame.DoGauss=m[opt_primary+7].value;
	Netgame.DoHelix=m[opt_primary+8].value;
	Netgame.DoPhoenix=m[opt_primary+9].value;
	Netgame.DoOmega=m[opt_primary+10].value;

	Netgame.DoHoming=m[opt_second].value;
	Netgame.DoProximity=m[opt_second+1].value;
	Netgame.DoSmarts=m[opt_second+2].value;
	Netgame.DoMegas=m[opt_second+3].value;
	Netgame.DoFlash=m[opt_second+4].value;
	Netgame.DoGuided=m[opt_second+5].value;
	Netgame.DoSmartMine=m[opt_second+6].value;
	Netgame.DoMercury=m[opt_second+7].value;
	Netgame.DoEarthShaker=m[opt_second+8].value;

	Netgame.DoInvulnerability=m[opt_power].value;
	Netgame.DoCloak=m[opt_power+1].value;
	Netgame.DoAfterburner=m[opt_power+2].value;
	Netgame.DoAmmoRack=m[opt_power+3].value;
	Netgame.DoConverter=m[opt_power+4].value;
	Netgame.DoHeadlight=m[opt_power+5].value;
*/	
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	int i;

	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
	{
		m[i].type = NM_TYPE_CHECK; m[i].text = multi_allow_powerup_text[i]; m[i].value = (Netgame.AllowedItems >> i) & 1;
	}

	i = newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, NULL, NULL, 0 );

	if (i > -1)
	{
		Netgame.AllowedItems &= ~NETFLAG_DOPOWERUP;
		for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
			if (m[i].value)
				Netgame.AllowedItems |= (1 << i);
	}
}

int net_ipx_more_options_poll( newmenu *menu, d_event *event, void *userdata );

void net_ipx_more_game_options ()
{
	int opt=0,i;
	char PlayText[80],KillText[80],srinvul[50],packstring[5],socket_string[5];
	newmenu_item m[21];

	sprintf (socket_string,"%d",IPX_Socket);
	sprintf (packstring,"%d",Netgame.PacketsPerSec);
	
	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;

	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.control_invul_time/5/F1_0/60; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_playtime=opt;
	sprintf( PlayText, "Max time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.PlayTimeAllowed; m[opt].text= PlayText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_killgoal=opt;
	sprintf( KillText, "Kill Goal: %d kills", Netgame.KillGoal*5);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.KillGoal; m[opt].text= KillText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_start_invul=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Invulnerable when reappearing"; m[opt].value=Netgame.InvulAppear; opt++;

	opt_marker_view = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Allow Marker camera views"; m[opt].value=Netgame.Allow_marker_view; opt++;
	opt_light = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Indestructible lights"; m[opt].value=Netgame.AlwaysLighting; opt++;

	opt_bright = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Bright player ships"; m[opt].value=Netgame.BrightPlayers; opt++;
	
	opt_show_names=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Show player names on HUD"; m[opt].value=Netgame.ShowAllNames; opt++;
	
	opt_show_on_map=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP); opt_show_on_map=opt; opt++;
	
	opt_short_packets=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Short packets"; m[opt].value=Netgame.protocol.ipx.ShortPackets; opt++;
	
	opt_setpower = opt;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Set Objects allowed..."; opt++;
	
	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Packets per second (2 - 20)"; opt++;
	opt_packets=opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text=packstring; m[opt].text_len=2; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network socket"; opt++;
	opt_socket = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = socket_string; m[opt].text_len=4; opt++;
	
	LastKillGoal=Netgame.KillGoal;
	LastPTA=Netgame.PlayTimeAllowed;

menu:
	i = newmenu_do1( NULL, "Advanced netgame options", opt, m, net_ipx_more_options_poll, NULL, 0 );

	Netgame.control_invul_time = m[opt_cinvul].value*5*F1_0*60;

	if (i==opt_setpower)
	{
		net_ipx_set_power ();
		goto menu;
	}

	Netgame.PacketsPerSec=atoi(packstring);
	
	if (Netgame.PacketsPerSec>20)
	{
		Netgame.PacketsPerSec=20;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 20");
	}

	if (Netgame.PacketsPerSec<2)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 2");
		Netgame.PacketsPerSec=2;
	}

	if ((atoi(socket_string))!=IPX_Socket)
	{
		IPX_Socket=atoi(socket_string);
		ipxdrv_change_default_socket( IPX_DEFAULT_SOCKET + IPX_Socket );
	}
	
	Netgame.InvulAppear=m[opt_start_invul].value;	
	Netgame.BrightPlayers=m[opt_bright].value;
	Netgame.protocol.ipx.ShortPackets=m[opt_short_packets].value;
	Netgame.ShowAllNames=m[opt_show_names].value;
	
	Netgame.Allow_marker_view=m[opt_marker_view].value;   
	Netgame.AlwaysLighting=m[opt_light].value;     
	Netgame.difficulty=Difficulty_level = m[opt_difficulty].value;
	if (m[opt_show_on_map].value)
		Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
	else
		Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
}

int net_ipx_more_options_poll( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);

	if (event->type != EVENT_IDLE)	// FIXME: Should become EVENT_ITEM_CHANGED in future
		return 0;
	
	userdata = userdata;
		
	if ( last_cinvul != menus[opt_cinvul].value )   {
		sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
		last_cinvul = menus[opt_cinvul].value;
	}

	if (menus[opt_playtime].value!=LastPTA)
	{
		if (Game_mode & GM_MULTI_COOP)
		{
			LastPTA=0;
			nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
			menus[opt_playtime].value=0;
			return 0;
		}
		
		Netgame.PlayTimeAllowed=menus[opt_playtime].value;
		sprintf( menus[opt_playtime].text, "Max Time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
		LastPTA=Netgame.PlayTimeAllowed;
	}

	if (menus[opt_killgoal].value!=LastKillGoal)
	{
		if (Game_mode & GM_MULTI_COOP)
		{
			nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
			menus[opt_killgoal].value=0;
			LastKillGoal=0;
			return 0;
		}

		Netgame.KillGoal=menus[opt_killgoal].value;
		sprintf( menus[opt_killgoal].text, "Kill Goal: %d kills", Netgame.KillGoal*5);
		LastKillGoal=Netgame.KillGoal;
	}
	
	return 0;
}

extern void multi_send_light (int,ubyte);
extern void multi_send_light_specific (int,int,ubyte);

void net_ipx_send_smash_lights (int pnum) 
 {
  // send the lights that have been blown out

  int i;

  pnum=pnum;
  
  for (i=0;i<=Highest_segment_index;i++)
   if (Light_subtracted[i])
    multi_send_light_specific(pnum,i,Light_subtracted[i]);
 }

extern int Num_triggers;
extern void multi_send_trigger_specific (char pnum,char);

void net_ipx_send_fly_thru_triggers (int pnum) 
 {
  // send the fly thru triggers that have been disabled

  int i;

  for (i=0;i<Num_triggers;i++)
   if (Triggers[i].flags & TF_DISABLED)
    multi_send_trigger_specific(pnum,i);
 }

void net_ipx_send_player_flags()
 {
  int i;

  for (i=0;i<N_players;i++)
	multi_send_flags(i);
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
	int i,new_player_num;
	
	ClipRank (&their->player.rank);
		
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
	
		digi_play_sample (SOUND_HUD_JOIN_REQUEST,F1_0*2);
	
		if (Game_mode & GM_TEAM)
		{
			if (!GameArg.MplNoRankings)
			{
				HUD_init_message ("%s %s wants to join",RankStrings[their->player.rank],their->player.callsign);
			}
			else
			{
				HUD_init_message ("%s wants to join",their->player.callsign);
			}
			HUD_init_message ("Alt-1 assigns to team %s. Alt-2 to team %s",their->player.callsign,Netgame.team_name[0],Netgame.team_name[1]);
		}
		else
		{
			HUD_init_message ("%s wants to join (accept: F6)",their->player.callsign);
		}
	
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
			if (Game_mode & GM_TEAM)
			{
				new_player_num=net_ipx_get_new_player_num (their);
	
				Assert (RefuseTeam==1 || RefuseTeam==2);        
			
				if (RefuseTeam==1)      
				Netgame.team_vector &=(~(1<<new_player_num));
				else
				Netgame.team_vector |=(1<<new_player_num);
				net_ipx_welcome_player(their);
				net_ipx_send_netgame_update (); 
			}
			else
			{
				net_ipx_welcome_player(their);
			}
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

int net_ipx_get_new_player_num (IPX_sequence_packet *their)
  {
	 int i;
	
	 their=their;
	
		if ( N_players < MaxNumNetPlayers)
			return (N_players);
		
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix oldest_time = timer_get_fixed_seconds();

			Assert(N_players == MaxNumNetPlayers);

			for (i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}
	    return (oldest_player);
	  }
  }

void net_ipx_send_extras ()
 {
	Assert (Player_joining_extras>-1);

   if (!multi_i_am_master())
	 {
	 // Int3();     
	 // Network_sending_extras=0;
	 // return;
    }


   if (Network_sending_extras==40)
		net_ipx_send_fly_thru_triggers(Player_joining_extras);
   if (Network_sending_extras==38)
	net_ipx_send_door_updates(Player_joining_extras);
   if (Network_sending_extras==35)
		net_ipx_send_markers();
   if (Network_sending_extras==30 && (Game_mode & GM_MULTI_ROBOTS))
		multi_send_stolen_items();
	if (Network_sending_extras==25 && (Netgame.PlayTimeAllowed || Netgame.KillGoal))
		multi_send_kill_goal_counts();
   if (Network_sending_extras==20)
		net_ipx_send_smash_lights(Player_joining_extras);
   if (Network_sending_extras==15)
		net_ipx_send_player_flags();    
   if (Network_sending_extras==10)
		multi_send_powerup_update();  
 //  if (Network_sending_extras==5)
//		net_ipx_send_door_updates(Player_joining_extras); // twice!

	Network_sending_extras--;
   if (!Network_sending_extras)
	 Player_joining_extras=-1;
 }


void net_ipx_send_naked_packet(char *buf, short len, int who)
{
	if (!(Game_mode&GM_NETWORK)) return;

	if (NakedPacketLen==0)
	{
		NakedBuf[0]=PID_NAKED_PDATA;
		NakedBuf[1]=Player_num;
		NakedPacketLen=2;
	}

	if (len+NakedPacketLen>NET_XDATA_SIZE)
	{
		ipxdrv_send_packet_data( (ubyte *)NakedBuf, NakedPacketLen, Netgame.players[who].protocol.ipx.server, Netgame.players[who].protocol.ipx.node,Players[who].net_address );
		NakedPacketLen=2;
		memcpy (&NakedBuf[NakedPacketLen],buf,len);     
		NakedPacketLen+=len;
		NakedPacketDestPlayer=who;
	}
	else
	{
		memcpy (&NakedBuf[NakedPacketLen],buf,len);     
		NakedPacketLen+=len;
		NakedPacketDestPlayer=who;
	}
}

void net_ipx_process_naked_pdata (char *data,int len)
 {
   int pnum=data[1]; 
   Assert (data[0]=PID_NAKED_PDATA);

	if (pnum < 0) {
		Int3(); // This packet is bogus!!
		return;
	}

	if (!multi_quit_game && (pnum >= N_players)) {
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
		multi_process_bigdata( data+2, len-2);
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	multi_process_bigdata( data+2, len-2 );
 }

void net_ipx_check_for_old_version (char pnum)
 {  
  if (Netgame.players[(int)pnum].version_major==1 && (Netgame.players[(int)pnum].version_minor & 0x0F)==0)
	Netgame.players[(int)pnum].rank=0;
 }

void net_ipx_request_player_names (int n)
 {
  net_ipx_send_all_info_request (PID_GAME_PLAYERS,Active_ipx_games[n].protocol.ipx.Game_Security);
  NamesInfoSecurity=Active_ipx_games[n].protocol.ipx.Game_Security;
 }

void net_ipx_process_names_return (ubyte *data)
 {
	newmenu_item m[15];
   char mtext[15][50],temp[50];
	int i,t,l,gnum,num=0,count=5,numplayers;
   
   if (NamesInfoSecurity!=(*(int *)(data+1)))
	 {
	  return;
	 }

   numplayers=data[count]; count++;

   if (numplayers==255)
	 {
		 SurfingNet=0;	
		 NamesInfoSecurity=-1;
 		 nm_messagebox(NULL, 1, "OK", "That game is refusing\nname requests.\n");
		 SurfingNet=1;
		 return;
	 }

   Assert (numplayers>0 && numplayers<MAX_NUM_NET_PLAYERS);
	
   for (i=0;i<12;i++)
	{
	 m[i].text=(char *)&mtext[i];
    m[i].type=NM_TYPE_TEXT;		
	}

   for (gnum=-1,i=0;i<num_active_ipx_games;i++)
	 {
	  if (NamesInfoSecurity==Active_ipx_games[i].protocol.ipx.Game_Security)
		{
		 gnum=i;
		 break;
		}
	 }
	
	if (gnum==-1)
    {
       SurfingNet=0;
		 NamesInfoSecurity=-1;
 		 nm_messagebox(NULL, 1, "OK", "The game you have requested\nnames from is gone.\n");
		 SurfingNet=1;
		 return;
	 }
 
   sprintf (mtext[num],"Players of game '%s':",Active_ipx_games[gnum].game_name); num++;
   for (i=0;i<numplayers;i++)
	 {
	  l=data[count++];

	  for (t=0;t<CALLSIGN_LEN+1;t++)
		 temp[t]=data[count++];	  
     if (GameArg.MplNoRankings)	
	     sprintf (mtext[num],"%s",temp);
	  else
	     sprintf (mtext[num],"%s%s",RankStrings[l],temp);
	
	  num++;	
	 }

	if (data[count]==99)
	{
	 sprintf (mtext[num++]," ");
	 sprintf (mtext[num++],"Short packets: %s",data[count+1]?"On":"Off");
	 sprintf (mtext[num++],"Packets Per Second: %d",data[count+2]);
   }

	newmenu_dotiny( NULL, NULL, num, m, NULL, NULL);
 }

char NameReturning=1;

void net_ipx_send_player_names (IPX_sequence_packet *their)
 {
  int numconnected=0,count=0,i,t;
  char buf[80];

  if (!their)
   {
	 return;
	}

   buf[0]=PID_NAMES_RETURN; count++;
   (*(int *)(buf+1))=Netgame.protocol.ipx.Game_Security; count+=4;

   if (!NameReturning)
	 {
     buf[count]=255; count++; 
	  goto sendit;
	 }
 
   for (i=0;i<N_players;i++)
	 if (Players[i].connected)
		numconnected++;

   buf[count]=numconnected; count++; 
   for (i=0;i<N_players;i++)
	 if (Players[i].connected)
	  {
		buf[count++]=Netgame.players[i].rank; 
 		
		for (t=0;t<CALLSIGN_LEN+1;t++)
		 {
		  buf[count]=Netgame.players[i].callsign[t];
		  count++;
		 }
	  }

   buf[count++]=99;
	buf[count++]=Netgame.protocol.ipx.ShortPackets;		
	buf[count++]=Netgame.PacketsPerSec;
  
   sendit:
   				;		// empty statement allows compilation on mac which does not have the
   						// statement below and kills the compiler when there is no
   						// statement following the label "sendit"
	   
	   ipxdrv_send_internetwork_packet_data((ubyte *)buf, count, their->player.protocol.ipx.server, their->player.protocol.ipx.node);
 }

static int show_game_rules_handler(window *wind, d_event *event, netgame_info *netgame)
{
	int k;
	int w = FSPACX(280), h = FSPACY(170);
	
	switch (event->type)
	{
		case EVENT_IDLE:
			timer_delay2(50);
			
			//see if redbook song needs to be restarted
			RBACheckFinishedHook();
			
			k = key_inkey();
			switch( k )	{
				case KEY_PRINT_SCREEN:
					save_screen_shot(0); k = 0;
					break;
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					window_close(wind);
					break;
			}
			return 0;
			break;

		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
#ifdef OGL
			nm_draw_background1(NULL);
#endif
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);
			
			gr_set_current_canvas(window_get_canvas(wind));
			
			grd_curcanv->cv_font = MEDIUM3_FONT;
			
			gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);	
			gr_string( 0x8000, FSPACY(15), "NETGAME INFO");
			
			grd_curcanv->cv_font = GAME_FONT;
			gr_printf( FSPACX( 25),FSPACY( 35), "Reactor Life:");
			gr_printf( FSPACX( 25),FSPACY( 41), "Max Time:");
			gr_printf( FSPACX( 25),FSPACY( 47), "Kill Goal:");
			gr_printf( FSPACX( 25),FSPACY( 53), "Short Packets:");
			gr_printf( FSPACX( 25),FSPACY( 59), "Pakets per second:");
			gr_printf( FSPACX(155),FSPACY( 35), "Invul when reappearing:");
			gr_printf( FSPACX(155),FSPACY( 41), "Marker camera views:");
			gr_printf( FSPACX(155),FSPACY( 47), "Indestructible lights:");
			gr_printf( FSPACX(155),FSPACY( 53), "Bright player ships:");
			gr_printf( FSPACX(155),FSPACY( 59), "Show enemy names on hud:");
			gr_printf( FSPACX(155),FSPACY( 65), "Show players on automap:");
			gr_printf( FSPACX( 25),FSPACY( 80), "Allowed Objects");
			gr_printf( FSPACX( 25),FSPACY( 90), "Laser Upgrade:");
			gr_printf( FSPACX( 25),FSPACY( 96), "Super Laser:");
			gr_printf( FSPACX( 25),FSPACY(102), "Quad Laser:");
			gr_printf( FSPACX( 25),FSPACY(108), "Vulcan Cannon:");
			gr_printf( FSPACX( 25),FSPACY(114), "Gauss Cannon:");
			gr_printf( FSPACX( 25),FSPACY(120), "Spreadfire Cannon:");
			gr_printf( FSPACX( 25),FSPACY(126), "Helix Cannon:");
			gr_printf( FSPACX( 25),FSPACY(132), "Plasma Cannon:");
			gr_printf( FSPACX( 25),FSPACY(138), "Phoenix Cannon:");
			gr_printf( FSPACX( 25),FSPACY(144), "Fusion Cannon:");
			gr_printf( FSPACX( 25),FSPACY(150), "Omega Cannon:");
			gr_printf( FSPACX(170),FSPACY( 90), "Flash Missile:");
			gr_printf( FSPACX(170),FSPACY( 96), "Homing Missile:");
			gr_printf( FSPACX(170),FSPACY(102), "Guided Missile:");
			gr_printf( FSPACX(170),FSPACY(108), "Proximity Bomb:");
			gr_printf( FSPACX(170),FSPACY(114), "Smart Mine:");
			gr_printf( FSPACX(170),FSPACY(120), "Smart Missile:");
			gr_printf( FSPACX(170),FSPACY(126), "Mercury Missile:");
			gr_printf( FSPACX(170),FSPACY(132), "Mega Missile:");
			gr_printf( FSPACX(170),FSPACY(138), "Earthshaker Missile:");
			gr_printf( FSPACX( 25),FSPACY(160), "Afterburner:");
			gr_printf( FSPACX( 25),FSPACY(166), "Headlight:");
			gr_printf( FSPACX( 25),FSPACY(172), "Energy->Shield Conv:");
			gr_printf( FSPACX(170),FSPACY(160), "Invulnerability:");
			gr_printf( FSPACX(170),FSPACY(166), "Cloaking Device:");
			gr_printf( FSPACX(170),FSPACY(172), "Ammo Rack:");
			
			gr_set_fontcolor(BM_XRGB(255,255,255),-1);
			gr_printf( FSPACX(115),FSPACY( 35), "%i Min", netgame->control_invul_time/F1_0/60);
			gr_printf( FSPACX(115),FSPACY( 41), "%i Min", netgame->PlayTimeAllowed*5);
			gr_printf( FSPACX(115),FSPACY( 47), "%i", netgame->KillGoal);
			gr_printf( FSPACX(115),FSPACY( 53), "%s", netgame->protocol.ipx.ShortPackets?"ON":"OFF");
			gr_printf( FSPACX(115),FSPACY( 59), "%i", netgame->PacketsPerSec);
			gr_printf( FSPACX(275),FSPACY( 35), netgame->InvulAppear?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 41), netgame->Allow_marker_view?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 47), netgame->AlwaysLighting?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 53), netgame->BrightPlayers?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 59), netgame->ShowAllNames?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 65), netgame->game_flags & NETGAME_FLAG_SHOW_MAP?"ON":"OFF");
			gr_printf( FSPACX(130),FSPACY( 90), netgame->AllowedItems & NETFLAG_DOLASER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY( 96), netgame->AllowedItems & NETFLAG_DOSUPERLASER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(102), netgame->AllowedItems & NETFLAG_DOQUAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(108), netgame->AllowedItems & NETFLAG_DOVULCAN?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(114), netgame->AllowedItems & NETFLAG_DOGAUSS?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(120), netgame->AllowedItems & NETFLAG_DOSPREAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(126), netgame->AllowedItems & NETFLAG_DOHELIX?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(132), netgame->AllowedItems & NETFLAG_DOPLASMA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(138), netgame->AllowedItems & NETFLAG_DOPHOENIX?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(144), netgame->AllowedItems & NETFLAG_DOFUSION?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(150), netgame->AllowedItems & NETFLAG_DOOMEGA?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY( 90), netgame->AllowedItems & NETFLAG_DOFLASH?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY( 96), netgame->AllowedItems & NETFLAG_DOHOMING?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(102), netgame->AllowedItems & NETFLAG_DOGUIDED?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(108), netgame->AllowedItems & NETFLAG_DOPROXIM?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(114), netgame->AllowedItems & NETFLAG_DOSMARTMINE?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(120), netgame->AllowedItems & NETFLAG_DOSMART?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(126), netgame->AllowedItems & NETFLAG_DOMERCURY?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(132), netgame->AllowedItems & NETFLAG_DOMEGA?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(138), netgame->AllowedItems & NETFLAG_DOSHAKER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(160), netgame->AllowedItems & NETFLAG_DOAFTERBURNER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(166), netgame->AllowedItems & NETFLAG_DOHEADLIGHT?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(172), netgame->AllowedItems & NETFLAG_DOCONVERTER?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(160), netgame->AllowedItems & NETFLAG_DOINVUL?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(166), netgame->AllowedItems & NETFLAG_DOCLOAK?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(172), netgame->AllowedItems & NETFLAG_DOAMMORACK?"YES":"NO");
			gr_set_current_canvas(NULL);
			break;

		case EVENT_WINDOW_CLOSE:
			game_flush_inputs();
			return 0;	// continue closing
			break;
			
		default:
			return 0;
			break;
	}
	
	return 1;
}

void net_ipx_show_game_rules(netgame_info *netgame)
{
	gr_set_current_canvas(NULL);

	game_flush_inputs();
	
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
	char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Unknown"};
	int c;
	netgame_info *netgame = &Active_ipx_games[choice];

	memset(info,0,sizeof(char)*256);

	info+=sprintf(info,"\nConnected to\n\"%s\"\n",netgame->game_name);

	if(!netgame->mission_title)
		info+=sprintf(info,"Descent2: CounterStrike");
	else
		info+=sprintf(info,netgame->mission_title);

	info+=sprintf (info," - Lvl %i",netgame->levelnum);
	info+=sprintf (info,"\n\nDifficulty: %s",MENU_DIFFICULTY_TEXT(netgame->difficulty));
	info+=sprintf (info,"\nGame Mode: %s",NetworkModeNames[netgame->gamemode]);
	info+=sprintf (info,"\nPlayers: %i/%i",netgame->numconnected,netgame->max_numplayers);

	c=nm_messagebox1("WELCOME", (int (*)(newmenu *, d_event *, void *))show_game_info_handler, netgame, 2, "JOIN GAME", "GAME INFO", rinfo);
	if (c==0)
		return 1;
	//else if (c==1)
	// handled in above callback
	else
		return 0;
}
