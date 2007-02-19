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
 * Routines for managing network play.
 * 
 */

#ifdef RCS
static char rcsid[] = "$Id: network.c,v 1.1.1.1 2006/03/17 19:43:36 zicodxx Exp $";
#endif

#ifdef NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "args.h"
#include "timer.h"
#include "mono.h"
#include "ipx.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "error.h"
#include "netmisc.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "network.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "fuelcen.h"
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

#include "netpkt.h"
#include "multipow.h"

//added on 2/1/99 by Victor Rachels to add bans
#include "ban.h"
//end this section addition - VR

//added on 10/18/98 by Victor Rachels to add multi profile stuff
#include "mprofile.h"
//end this section addition - Victor

//added on 11/12/98 by Victor Rachels for network radar option
#include "radar.h" //int Network_allow_radar = 0;
//end this section addition - VR

//added on 11/16/98 by Victor Rachels (from GF) for more multi-control
#include "mlticntl.h"
//end this section addition - VR (from GF)

// Begin addition by GRiM FisH
#include "ignore.h"
// End addition by GRiM FisH

//added 03/04/99 Matt Mueller
#include "pingstat.h"
//end addition -MM

//added 04/23/99 Victor Rachels for alt vulcan fire
#include "vlcnfire.h"
//end addition -MM

//added 11/28/99 Victor Rachels for observer mode
#include "observer.h"
//end addition -MM

#include "vers_id.h"

//added 04/19/99 Matt Mueller
#include "multiver.h"
//end addition -MM

//-moved- void network_send_objects(void);
void network_send_rejoin_sync(int player_num);
//-moved- void network_dump_player(ubyte * server, ubyte *node, int why);
void network_update_netgame(void);
void network_send_endlevel_sub(int player_num);
void network_send_endlevel_packet(void);
//-moved- void network_send_game_info(sequence_packet *their, int light);
void network_read_endlevel_packet( ubyte *data );
void network_read_object_packet( ubyte *data );
void network_read_sync_packet( ubyte * sp, int d1x );
void network_flush();
void network_listen();
void network_read_pdata_packet( ubyte *data, int short_pos );
int network_compare_players(netplayer_info *pl1, netplayer_info *pl2);

#define NETWORK_NEW_LIST
#ifdef NETWORK_NEW_LIST
extern int network_join_game_menu();
#endif

#define NETWORK_TIMEOUT (10*F1_0) // 10 seconds disconnect timeout

netgame_info Active_games[MAX_ACTIVE_NETGAMES];
int num_active_games = 0;

int	Network_debug=0;
int	Network_active=0;

int  	Network_status = 0;
int 	Network_games_changed = 0;

int 	Network_socket = 0;
//int	  Network_allow_socket_changes = 0;
//fix	  Network_packet_interval = F1_0 / 10;
int	Network_initial_pps = 10;
int	Network_initial_shortpackets = 1;
int	Network_enable_ignore_ghost = 0;

//added on 8/4/98 by Matt Mueller for short packets
int	Network_short_packets = 0;
//added/changed on 8/6/98 by Matt Mueller
network_d1xplayer_info Net_D1xPlayer[MAX_NUM_NET_PLAYERS];
//end modified section - Matt Mueller
//end modified section - Matt Mueller

//added on 8/5/98 by Victor Rachels to make global pps setting
int     Network_pps = 10;
//end edit - Victor Rachels

// For rejoin object syncing

int	Network_rejoined = 0;       // Did WE rejoin this game?
int	Network_new_game = 0;		 // Is this the first level of a new game?
int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int 	Network_send_objnum = -1;   // What object are we sending next?
int	Network_player_added = 0;   // Is this a new player or a returning player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
sequence_packet	Network_player_rejoining; // Who is rejoining now?

fix	LastPacketTime[MAX_PLAYERS]; // For timeouts of idle/crashed players

int 	PacketUrgent = 0;

frame_info 	MySyncPack;
ubyte		MySyncPackInitialized = 0;		// Set to 1 if the MySyncPack is zeroed.
ushort 		my_segments_checksum = 0;

sequence_packet My_Seq;

extern obj_position Player_init[MAX_PLAYERS];

//-moved- #define DUMP_CLOSED 0
//-moved- #define DUMP_FULL 1
//-moved- #define DUMP_ENDLEVEL 2
//-moved- #define DUMP_DORK 3
//-moved- #define DUMP_ABORTED 4
//-moved- #define DUMP_CONNECTED 5
//-moded- #define DUMP_LEVEL 6

int network_wait_for_snyc();

void
network_init(void)
{
	// So you want to play a netgame, eh?  Let's a get a few things
	// straight

	int save_pnum = Player_num;

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&My_Seq, 0, sizeof(sequence_packet));
	My_Seq.type = PID_REQUEST;
	memcpy(My_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	memcpy(My_Seq.player.node, ipx_get_my_local_address(), 6);
	memcpy(My_Seq.player.server, ipx_get_my_server_address(), 4 );
	#ifndef SHAREWARE
	My_Seq.player.sub_protocol = MULTI_PROTO_D1X_MINOR;
	#endif

	//added/changed 8/6/98 by Matt Mueller
	memset( &Net_D1xPlayer, 0, sizeof(Net_D1xPlayer) );
	//end modified section - Matt Mueller
	//added 04/19/99 Matt Mueller
	multi_d1x_ver_queue_init(0,0);
	//end addition -MM

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();

	Player_num = save_pnum;		
	multi_new_game();
	Network_new_game = 1;
	Fuelcen_control_center_destroyed = 0;
	network_flush();
}

// Change socket to new_socket, returns 1 if really changed
int network_change_socket(int new_socket)
{
        if ( new_socket+IPX_DEFAULT_SOCKET > 0x8000 )
                new_socket  = 0x8000 - IPX_DEFAULT_SOCKET;
        if ( new_socket+IPX_DEFAULT_SOCKET < 0 )
                new_socket  = IPX_DEFAULT_SOCKET;
        if (new_socket != Network_socket) {
                Network_socket = new_socket;
                mprintf(( 0, "Changing to socket %d\n", Network_socket ));
                network_listen();
                ipx_change_default_socket( IPX_DEFAULT_SOCKET + Network_socket );
                return 1;
        }
        return 0;
}

#define ENDLEVEL_SEND_INTERVAL F1_0*2
#define ENDLEVEL_IDLE_TIME	F1_0*10
	
void 
network_endlevel_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int num_escaped = 0;
	int goto_secret = 0;

	int previous_state[MAX_NUM_NET_PLAYERS];
	int previous_seconds_left;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

	if (timer_get_approx_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
	{
		network_send_endlevel_packet();
		t1 = timer_get_approx_seconds();
	}

	for (i = 0; i < N_players; i++)
		previous_state[i] = Players[i].connected;

	previous_seconds_left = Fuelcen_seconds_left;

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if (previous_state[i] != Players[i].connected)		
		{
			sprintf(menus[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
			menus[i].redraw = 1;
		}
		if (Players[i].connected == 1)
		{
			// Check timeout for idle players
			if (timer_get_approx_seconds() > LastPacketTime[i]+ENDLEVEL_IDLE_TIME)
			{
				mprintf((0, "idle timeout for player %d.\n", i));
				Players[i].connected = 0;
				network_send_endlevel_sub(i);
			}				
		}

		if ((Players[i].connected != 1) && (Players[i].connected != 5) && (Players[i].connected != 6))
			num_ready++;
		if (Players[i].connected != 1)
			num_escaped++;
		if (Players[i].connected == 4)
			goto_secret = 1;
	}

	if (num_escaped == N_players) // All players are out of the mine
	{
		Fuelcen_seconds_left = -1;
	}

	if (previous_seconds_left != Fuelcen_seconds_left)
	{
		if (Fuelcen_seconds_left < 0)
		{
			sprintf(menus[N_players].text, TXT_REACTOR_EXPLODED);
			menus[N_players].redraw = 1;
		}
		else
		{
			sprintf(menus[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Fuelcen_seconds_left, TXT_SECONDS);
			menus[N_players].redraw = 1;
		}
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			*key = -3;
		else
			*key = -2;
	}
}

void 
network_endlevel_poll2( int nitems, newmenu_item * menus, int * key, int citem )
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int goto_secret = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

	if (timer_get_approx_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
	{
		network_send_endlevel_packet();
		t1 = timer_get_approx_seconds();
	}

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected != 1) && (Players[i].connected != 5) && (Players[i].connected != 6))
			num_ready++;
		if (Players[i].connected == 4)
			goto_secret = 1;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			*key = -3;
		else
			*key = -2;
	}
}

int
network_endlevel(int *secret)
{
	// Do whatever needs to be done between levels

	newmenu_item m[MAX_NUM_NET_PLAYERS+1];
	char menu_text[MAX_NUM_NET_PLAYERS+1][80];
	int i, choice;

	char text[80];

        Function_mode = FMODE_MENU;

	network_flush();

	Network_status = NETSTAT_ENDLEVEL; // We are between levels

	network_listen();

	network_send_endlevel_packet();

newmenu:
	// Setup menu text pointers and zero them
	for (i=0; i<N_players; i++) 
	{
		m[i].type = NM_TYPE_TEXT;
		m[i].text = menu_text[i];
		sprintf(m[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
		LastPacketTime[i] = timer_get_approx_seconds();
	}
	m[N_players].type = NM_TYPE_TEXT;
	m[N_players].text = menu_text[N_players];

	if (Fuelcen_seconds_left < 0)
		sprintf(m[N_players].text, TXT_REACTOR_EXPLODED);
	else
		sprintf(m[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Fuelcen_seconds_left, TXT_SECONDS);

menu:
	sprintf(text, "%s\n%s", TXT_WAITING, TXT_ESC_ABORT);

	choice=newmenu_do3(NULL, text, N_players+1, m, network_endlevel_poll, 0, "STARS.PCX", 300*(SWIDTH/320), 160*(SHEIGHT/200));

	if (choice==-1) {
		newmenu_item m2[2];

		m2[0].type = m2[1].type = NM_TYPE_MENU;
		m2[0].text = TXT_YES; m2[1].text = TXT_NO;
		choice = newmenu_do1(NULL, TXT_SURE_LEAVE_GAME, 2, m2, network_endlevel_poll2, 1);
		if (choice == 0)
		{
			Players[Player_num].connected = 0;
			network_send_endlevel_packet();
			network_send_endlevel_packet();
			longjmp(LeaveGame,1);
		}	
		if (choice > -2)
			goto newmenu;
	}

//	kmatrix_view();

	if (choice > -2)
		goto menu;
	
	if (choice == -3)
		*secret = 1; // If any player went to the secret level, we go to the secret level

	network_send_endlevel_packet();
	network_send_endlevel_packet();
	MySyncPackInitialized = 0;

	network_update_netgame();

	return(0);
}

int 
can_join_netgame(netgame_info *game)
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
			  (!memcmp(My_Seq.player.node, game->players[i].node, 6)) &&
			  (!memcmp(My_Seq.player.server, game->players[i].server, 4)) )
			break;

	if (i != num_players)
		return 1;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
		// Look for player that is not connected
		if (game->numplayers < game->max_numplayers)
			return 1;

		for (i = 0; i < num_players; i++) {
			if (game->players[i].connected == 0)
				return 1;
		}
		return 0;
	}

	return 0;
}

void
network_disconnect_player(int playernum)
{
	// A player has disconnected from the net game, take whatever steps are
	// necessary 

	if (playernum == Player_num) 
	{
		Int3(); // Weird, see Rob
		return;
	}

	Players[playernum].connected = 0;
	Netgame.players[playernum].connected = 0;

//	create_player_appearance_effect(&Objects[Players[playernum].objnum]);
	multi_make_player_ghost(playernum);

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_disconnect(playernum);

	multi_strip_robots(playernum);
#endif
}

//network_send_config_messages(int dest, int mode) moved to multiver.c 4/18/99 Matt Mueller
                
// initialize player settings
// called after player joins
void network_get_player_settings(int pnum) {
	//added 8/6/98 by Matt Mueller
        memset( &Net_D1xPlayer[pnum], 0, sizeof(network_d1xplayer_info) );
	//edit 04/19/99 Matt Mueller
//--killed--        network_send_config_messages(pnum,4);
		multi_d1x_ver_queue_send(pnum,4);
	//end edit -MM
	//end edit - Matt Mueller
	//added 980815 by adb
#ifndef SHAREWARE
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER)
		Net_D1xPlayer[pnum].shp =
			(Netgame.flags & NETFLAG_SHORTPACKETS) ? 2 : 0;
#endif
	//end edit - adb
}

void
network_new_player(sequence_packet *their)
{
	int objnum;
	int pnum;

	pnum = their->player.connected;

	Assert(pnum >= 0);
	Assert(pnum < MaxNumNetPlayers);	
	
	objnum = Players[pnum].objnum;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING) {
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their->player.callsign);
	}
#endif

	memcpy(Players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	memcpy(Netgame.players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);

#ifndef SHAREWARE
	if ( (*(uint *)their->player.server) != 0 )
		ipx_get_local_target( their->player.server, their->player.node, Players[pnum].net_address );
	else
#endif
		memcpy(Players[pnum].net_address, their->player.node, 6);

	memcpy(Netgame.players[pnum].node, their->player.node, 6);
	memcpy(Netgame.players[pnum].server, their->player.server, 4);

	Players[pnum].n_packets_got = 0;
	Players[pnum].connected = 1;
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

	hud_message(MSGC_MULTI_INFO, "'\002%c%s\004' %s",//removed a \n here.. I don't think its supposed to be there. -MPM 
			gr_getcolor(player_rgb[pnum].r,player_rgb[pnum].g,player_rgb[pnum].b)+1,
			their->player.callsign, TXT_JOINING);
	
	multi_make_ghost_player(pnum);

#ifndef SHAREWARE
	multi_send_score();
#endif

	network_get_player_settings(pnum);

//added on 11/24/99 by Victor Rachels to make observer ghosted
        multi_send_observerghost(pnum); // function checks I_am_observer
//end this section addition - VR
//	create_player_appearance_effect(&Objects[objnum]);
}

void network_welcome_player(sequence_packet *their)
{
 	// Add a player to a game already in progress
	ubyte local_address[6];
	int player_num;
	int i;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Fuelcen_control_center_destroyed))
	{
		mprintf((0, "Ignored request from new player to join during endgame.\n"));
		network_dump_player(their->player.server,their->player.node, DUMP_ENDLEVEL);
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
		mprintf((0, "Dumping player due to old level number.\n"));
		network_dump_player(their->player.server, their->player.node, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	memset(&Network_player_rejoining, 0, sizeof(sequence_packet));
	Network_player_added = 0;

#ifndef SHAREWARE
	if ( (*(uint *)their->player.server) != 0 )
		ipx_get_local_target( their->player.server, their->player.node, local_address );
	else
#endif
		memcpy(local_address, their->player.node, 6);

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

			network_dump_player(their->player.server, their->player.node, DUMP_CLOSED);
			return;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix oldest_time = timer_get_approx_seconds();

                        //added on 11/16/98 by Victor Rachels from GriM FisH
                        int activeplayers = 0;

			Assert(N_players == MaxNumNetPlayers);

                         for (i = 0; i < Netgame.numplayers; i++)
                          if (Netgame.players[i].connected)
                           activeplayers++;

                         if (activeplayers == Netgame.max_numplayers)
                         {
                                 // Game is full.
                         
                                 network_dump_player(their->player.server, their->player.node, DUMP_FULL);
                                 return;
                         }
                         //End addition by GF


			for (i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (LastPacketTime[i] < oldest_time))
				{
					oldest_time = LastPacketTime[i];
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected 

				network_dump_player(their->player.server, their->player.node, DUMP_FULL);
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
			mprintf((0, "Extra REQUEST from player ignored.\n"));
			return;
		}

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);
#endif

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		hud_message(MSGC_MULTI_INFO, "'\002%c%s\004' %s", 
				gr_getcolor(player_rgb[player_num].r,player_rgb[player_num].g,player_rgb[player_num].b)+1,
				Players[player_num].callsign, TXT_REJOIN);
	}

	// Send updated Objects data to the new/returning player

	Network_player_rejoining = *their;
	Network_player_rejoining.player.connected = player_num;
	Network_send_objects = 1;
	Network_send_objnum = -1;

	network_send_objects();
}

int network_objnum_is_past(int objnum)
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.
	
	int player_num = Network_player_rejoining.player.connected;
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

#define OBJ_PACKETS_PER_FRAME 1

#ifndef SHAREWARE
void network_send_door_updates(void)
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

void network_process_monitor_vector(int vector)
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
					mprintf((0, "Monitor %d blown up.\n", count));
				}
				else
					mprintf((0, "Monitor %d intact.\n", count));
				count++;
				Assert(count < 32);
			}
		}
	}
}

int network_create_monitor_vector(void)
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
		
	for (i = 0; i < num_blown_bitmaps; i++)
		mprintf((0, "Blown bitmap #%d = %d.\n", i, blown_bitmaps[i]));
	
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
					mprintf((0, "Monitor %d intact.\n", monitor_num));
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					for (k = 0; k < num_blown_bitmaps; k++)
					{
						if ((tm&0x3fff) == blown_bitmaps[k])
						{
							mprintf((0, "Monitor %d destroyed.\n", monitor_num));
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
	mprintf((0, "Final monitor vector %x.\n", vector));
	return(vector);
}
#endif

void network_stop_resync(sequence_packet *their)
{
	if ( (!memcmp(Network_player_rejoining.player.node, their->player.node, 6)) &&
		  (!memcmp(Network_player_rejoining.player.server, their->player.server, 4)) &&
	     (!strcasecmp(Network_player_rejoining.player.callsign, their->player.callsign)) )
	{
		mprintf((0, "Aborting resync for player %s.\n", their->player.callsign));
		Network_send_objects = 0;
		Network_send_objnum = -1;
	}
}

ubyte object_buffer[IPX_MAX_DATA_SIZE];

void network_send_objects(void)
{
	short remote_objnum;
	sbyte owner;
	int loc, i, h;

	static int obj_count = 0;
	static int frame_num = 0;

	int obj_count_frame = 0;
	int player_num = Network_player_rejoining.player.connected;

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < MaxNumNetPlayers);

	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		network_dump_player(Network_player_rejoining.player.server,Network_player_rejoining.player.node, DUMP_ENDLEVEL);
		Network_send_objects = 0; 
		return;
	}

	for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) // Do more than 1 per frame, try to speed it up without
															  // over-stressing the receiver.
	{
		obj_count_frame = 0;
		memset(object_buffer, 0, IPX_MAX_DATA_SIZE);
		object_buffer[0] = PID_OBJECT_DATA;
		loc = 3;
	
		if (Network_send_objnum == -1)
		{
			obj_count = 0;
			Network_send_object_mode = 0;
//			mprintf((0, "Sending object array to player %d.\n", player_num));
			*(short *)(object_buffer+loc) = -1;	loc += 2;
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
	
			if ( ((IPX_MAX_DATA_SIZE-1) - loc) < (sizeof(object)+5) )
				break; // Not enough room for another object

			obj_count_frame++;
			obj_count++;
	
			remote_objnum = objnum_local_to_remote((short)i, &owner);
			Assert(owner == object_owner[i]);

			*(short *)(object_buffer+loc) = i;								loc += 2;
			object_buffer[loc] = owner;											loc += 1;
			*(short *)(object_buffer+loc) = remote_objnum; 				loc += 2;
			memcpy(object_buffer+loc, &Objects[i], sizeof(object));	loc += sizeof(object);
//			mprintf((0, "..packing object %d, remote %d\n", i, remote_objnum));
		}

		if (obj_count_frame) // Send any objects we've buffered
		{
			frame_num++;
	
			Network_send_objnum = i;
			object_buffer[1] = obj_count_frame;
			object_buffer[2] = frame_num;
//			mprintf((0, "Object packet %d contains %d objects.\n", frame_num, obj_count_frame));

			Assert(loc <= IPX_MAX_DATA_SIZE);

			ipx_send_internetwork_packet_data( object_buffer, loc, Network_player_rejoining.player.server, Network_player_rejoining.player.node );
			// OLD ipx_send_packet_data(object_buffer, loc, &Network_player_rejoining.player.node);
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
//				mprintf((0, "Sending end marker in packet #%d.\n", frame_num));
				mprintf((0, "Sent %d objects.\n", obj_count));
				object_buffer[0] = PID_OBJECT_DATA;
				object_buffer[1] = 1;
				object_buffer[2] = frame_num;
				*(short *)(object_buffer+3) = -2;	
				*(short *)(object_buffer+6) = obj_count;
				//OLD ipx_send_packet_data(object_buffer, 8, &Network_player_rejoining.player.node);
				ipx_send_internetwork_packet_data(object_buffer, 8, Network_player_rejoining.player.server, Network_player_rejoining.player.node);
			
				// Send sync packet which tells the player who he is and to start!
				network_send_rejoin_sync(player_num);

				// Turn off send object mode
				Network_send_objnum = -1;
				Network_send_objects = 0;
				obj_count = 0;
				return;
			} // mode == 1;
		} // i > Highest_object_index
	} // For PACKETS_PER_FRAME
}

void network_send_rejoin_sync(int player_num)
{
	int i, j;

	Players[player_num].connected = 1; // connect the new guy
	LastPacketTime[player_num] = timer_get_approx_seconds();

	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		network_dump_player(Network_player_rejoining.player.server,Network_player_rejoining.player.node, DUMP_ENDLEVEL);
		Network_send_objects = 0; 
		return;
	}

	if (Network_player_added)
	{
		Network_player_rejoining.type = PID_ADDPLAYER;
		Network_player_rejoining.player.connected = player_num;
		network_new_player(&Network_player_rejoining);

		for (i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
			{
#ifdef NATIVE_PACKETS
				ipx_send_packet_data( (ubyte *)&Network_player_rejoining, sizeof(sequence_packet), Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address);
#else
				send_sequence_packet( Network_player_rejoining, Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address);
#endif
			}
		}
//added/changed 8/6/98 by Matt Mueller
	}else {
		//changed 980815 by adb
		//-killed memset( &Net_D1xPlayer[player_num], 0, sizeof(network_d1xplayer_info) );
		//-killed network_send_config_messages(player_num,4);
		network_get_player_settings(player_num);
		//end edit - adb
	}
//end edit - Matt Mueller

	// Send sync packet to the new guy

	network_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
#ifndef SHAREWARE
		Netgame.player_score[j] = Players[j].score;
#endif
	}	

#ifndef SHAREWARE
	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = network_create_monitor_vector();
#endif

	mprintf((0, "Sending rejoin sync packet!!!\n"));
	
#ifdef NATIVE_PACKETS
	ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Network_player_rejoining.player.server, Network_player_rejoining.player.node );
 	ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Network_player_rejoining.player.server, Network_player_rejoining.player.node ); // repeat for safety
 	ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Network_player_rejoining.player.server, Network_player_rejoining.player.node ); // repeat for safety
#else
	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );
	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );
	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );
#endif

#ifndef SHAREWARE
	network_send_door_updates();
#endif
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER)
	    multi_send_start_powerup_count();

	return;
}

char * network_get_player_name( int objnum )
{
	if ( objnum < 0 ) return NULL; 
	if ( Objects[objnum].type != OBJ_PLAYER ) return NULL;
	if ( Objects[objnum].id >= MAX_PLAYERS ) return NULL;
	if ( Objects[objnum].id >= N_players ) return NULL;
	
	return Players[Objects[objnum].id].callsign;
}


void network_add_player(sequence_packet *p)
{
	int i;

	mprintf((0, "Got add player request!\n"));

	for (i=0; i<N_players; i++ )	{
		if ( !memcmp( Netgame.players[i].node, p->player.node, 6) && !memcmp(Netgame.players[i].server, p->player.server, 4)){
			mprintf((0, "already have em as player %i!\n",i));
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )	{
		mprintf((0, "too many players %i!\n",N_players));
		return;		// too many of em
	}

	memcpy( Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN+1 );
	memcpy( Netgame.players[N_players].node, p->player.node, 6 );
	memcpy( Netgame.players[N_players].server, p->player.server, 4 );
	Netgame.players[N_players].connected = 1;
	#ifndef SHAREWARE
	Netgame.players[N_players].sub_protocol = p->player.sub_protocol;
	#endif
	Players[N_players].connected = 1;
	LastPacketTime[N_players] = timer_get_approx_seconds();
	N_players++;
	Netgame.numplayers = N_players;

	// Broadcast updated info

	network_send_game_info(NULL, 0);
}

// One of the players decided not to join the game

void network_remove_player(sequence_packet *p)
{
	int i,pn;
	
	pn = -1;
	for (i=0; i<N_players; i++ )	{
		if (!memcmp(Netgame.players[i].node, p->player.node, 6) && !memcmp(Netgame.players[i].server, p->player.server, 4))		{
			pn = i;
			break;
		}
	}
	
	if (pn < 0 ) return;

	for (i=pn; i<N_players-1; i++ )	{
		memcpy( Netgame.players[i].callsign, Netgame.players[i+1].callsign, CALLSIGN_LEN+1 );
		memcpy( Netgame.players[i].node, Netgame.players[i+1].node, 6 );
		memcpy( Netgame.players[i].server, Netgame.players[i+1].server, 4 );
	}
		
	N_players--;
	Netgame.numplayers = N_players;

	// Broadcast new info

	network_send_game_info(NULL, 0);

}

void
network_dump_player(ubyte * server, ubyte *node, int why)
{
	// Remove player from game (not chosen, kicked, ...)

	Assert(MySyncPackInitialized);
	My_Seq.type = PID_DUMP;
	My_Seq.player.connected = why;
#ifdef NATIVE_PACKETS
	ipx_send_internetwork_packet_data( (ubyte *)&My_Seq, sizeof(sequence_packet), server, node);
#else
	send_sequence_packet( My_Seq, server, node, NULL);
#endif
}

void
network_send_game_list_request(void)
{
	// Send a broadcast request for game info

	sequence_packet me;

	mprintf((0, "Sending game_list request.\n"));
	memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	memcpy( me.player.node, ipx_get_my_local_address(), 6 );
	memcpy( me.player.server, ipx_get_my_server_address(), 4 );
	me.type = PID_GAME_LIST;
	
#ifdef NATIVE_PACKETS
	ipx_send_broadcast_packet_data( (ubyte *)&me, sizeof(sequence_packet) );
#else
	send_sequence_packet( me, NULL, NULL, NULL);
#endif
}

void
network_update_netgame(void)
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
#ifndef SHAREWARE
		Netgame.player_score[i] = Players[i].score;
		Netgame.player_flags[i] = (Players[i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
#endif
	}
	Netgame.team_kills[0] = team_kills[0];
	Netgame.team_kills[1] = team_kills[1];
	Netgame.levelnum = Current_level_num;
}

void
network_send_endlevel_sub(int player_num)
{
	endlevel_info end;
	int i;

	// Send an endlevel packet for a player
	end.type 		= PID_ENDLEVEL;
	end.player_num = player_num;
	end.connected	= Players[player_num].connected;
	end.kills		= Players[player_num].net_kills_total;
	end.killed		= Players[player_num].net_killed_total;
	memcpy(end.kill_matrix, kill_matrix[player_num], MAX_PLAYERS*sizeof(short));
	//added 05/18/99 Matt Mueller - it doesn't use the rest, but we should at least initialize it :)
	memset(end.kill_matrix[1], 0, (MAX_PLAYERS-1)*MAX_PLAYERS*sizeof(short));
	//end addition -MM

	if (Players[player_num].connected == 1) // Still playing
	{
		Assert(Fuelcen_control_center_destroyed);
	 	end.seconds_left = Fuelcen_seconds_left;
	}
	//added 05/18/99 Matt Mueller - similarly, its not used if we aren't connected, but checker complains.
	else
		end.seconds_left = 127;
	//end addition -MM

//	mprintf((0, "Sending endlevel packet.\n"));

	for (i = 0; i < N_players; i++)
	{	
		if ((i != Player_num) && (i!=player_num) && (Players[i].connected))
		{
#ifdef NATIVE_PACKETS
			ipx_send_packet_data((ubyte *)&end, sizeof(endlevel_info), Netgame.players[i].server, Netgame.players[i].node,Players[i].net_address);
#else
			send_endlevel_packet(&end, Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address);
#endif
		}
	}
}

void
network_send_endlevel_packet(void)
{
	// Send an updated endlevel status to other hosts

	network_send_endlevel_sub(Player_num);
}

void
network_send_game_info(sequence_packet *their, int light)
{
 	// Send game info to someone who requested it

	char old_type, old_status;

	mprintf((0, "Sending game info.\n"));

	network_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.type;
	old_status = Netgame.game_status;

	Netgame.type = (Netgame.protocol_version == MULTI_PROTO_D1X_VER ?
		(light ? PID_D1X_GAME_LITE : PID_D1X_GAME_INFO)
		: PID_GAME_INFO);
	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

#ifdef NATIVE_PACKETS
	if (!their)
		ipx_send_broadcast_packet_data((ubyte *)&Netgame, sizeof(netgame_info));
	else	
		ipx_send_internetwork_packet_data((ubyte *)&Netgame, sizeof(netgame_info), their->player.server, their->player.node);
#else
	if (!their)
		send_netgame_packet(NULL, NULL);
	else
		send_netgame_packet(their->player.server, their->player.node);
#endif

	Netgame.type = old_type;
	Netgame.game_status = old_status;
}	

int network_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

	Assert(Netgame.numplayers > 0);

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		if (Netgame.players[i].connected)
			break;

	Assert(i < MAX_NUM_NET_PLAYERS);

	mprintf((0, "Sending game enroll request to player %d.  Level = %d\n", i, Netgame.levelnum));

	My_Seq.type = (Netgame.protocol_version == MULTI_PROTO_D1X_VER)
		? PID_D1X_REQUEST : PID_REQUEST;
	My_Seq.player.connected = Current_level_num;

#ifdef NATIVE_PACKETS
	ipx_send_internetwork_packet_data((ubyte *)&My_Seq, sizeof(sequence_packet), Netgame.players[i].server, Netgame.players[i].node);
#else
	send_sequence_packet(My_Seq, Netgame.players[i].server, Netgame.players[i].node, NULL);
#endif
	return i;
}
	
void network_process_gameinfo(ubyte *data, int d1x)
{
	int i, j;
	netgame_info *new;

#ifdef NATIVE_PACKETS
	new = (netgame_info *)data;
#else
	netgame_info netgame;
	new = &netgame;
	receive_netgame_packet(data, &netgame, d1x);
#endif

	Network_games_changed = 1;

	mprintf((0, "Got game data for game %s.\n", new->game_name));

	for (i = 0; i < num_active_games; i++)
		if (!strcasecmp(Active_games[i].game_name, new->game_name) &&
		    (Active_games[i].type == PID_D1X_GAME_LITE ||
		     new->type == PID_D1X_GAME_LITE ||
		     (!memcmp(Active_games[i].players[0].node, new->players[0].node, 6) &&
		      !memcmp(Active_games[i].players[0].server, new->players[0].server, 4))))
			break;

	if (i == MAX_ACTIVE_NETGAMES)
	{
		mprintf((0, "Too many netgames.\n"));
		return;
	}
	memcpy(&Active_games[i], new, sizeof(netgame_info));
	if (i == num_active_games)
		num_active_games++;

	/* Workaround for bug in Descent 1 */
	if (Active_games[i].max_numplayers == 255)
         {
		if (Active_games[i].gamemode & GM_MULTI_ROBOTS)
			Active_games[i].max_numplayers = 4;
		else
			Active_games[i].max_numplayers = 8;
         }

	if (Active_games[i].numplayers == 0)
	{
		// Delete this game
		for (j = i; j < num_active_games-1; j++)
			memcpy(&Active_games[j], &Active_games[j+1], sizeof(netgame_info));
		num_active_games--;
	}
}

void network_process_dump(sequence_packet *their)
{
	// Our request for join was denied.  Tell the user why.

	mprintf((0, "Dumped by player %s, type %d.\n", their->player.callsign, their->player.connected));

	// Begin addition by GF
        if (Network_status == NETSTAT_PLAYING)
	{
          Function_mode = FMODE_MENU;
          nm_messagebox(NULL, 1, TXT_OK, "You have been kicked from the game!");
          multi_quit_game = 1;
          multi_leave_menu = 1;
          multi_reset_stuff();

          longjmp(LeaveGame,1);  // because the other crap didn't work right
          return;
	}
	// End addition by GF

	nm_messagebox(NULL, 1, TXT_OK, NET_DUMP_STRINGS(their->player.connected));
	Network_status = NETSTAT_MENU;
} 
	
void network_process_request(sequence_packet *their)
{
	// Player is ready to receieve a sync packet
	int i;

	mprintf((0, "Player %s ready for sync.\n", their->player.callsign));

	for (i = 0; i < N_players; i++)
		if (!memcmp(their->player.server, Netgame.players[i].server, 4) && !memcmp(their->player.node, Netgame.players[i].node, 6) && (!strcasecmp(their->player.callsign, Netgame.players[i].callsign))) {
			Players[i].connected = 1;
			break;
		}
}

void network_process_packet(ubyte *data, int length )
{
	sequence_packet *their = (sequence_packet *)data;

//	mprintf( (0, "Got packet of length %d, type %d\n", length, their->type ));
	
//	if ( length < sizeof(sequence_packet) ) return;

//edited 03/04/99 Matt Mueller - its used now
//	length = length;
//end edit -MM

	switch( their->type )	{
	
	case PID_GAME_INFO:
		mprintf((0, "GOT a PID_GAME_INFO!\n"));
		//if (length != sizeof(netgame_info))
		//	  mprintf((0, " Invalid size %d for netgame packet.\n", length));
                if (Network_status == NETSTAT_BROWSING)
                          network_process_gameinfo(data, 0);
		break;
	case PID_GAME_LIST:
		// Someone wants a list of games
		mprintf((0, "Got a PID_GAME_LIST!\n"));
		if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
			if (network_i_am_master())
				network_send_game_info(their, 1);
		break;
	case PID_ADDPLAYER:
		mprintf( (0, "Got NEWPLAYER message from %s.\n", their->player.callsign));
		network_new_player(their);
		break;
	case PID_D1X_REQUEST:
	case PID_REQUEST:
		mprintf( (0, "Got REQUEST from '%s'\n", their->player.callsign ));

                if (!ipx_check_ready_to_join(their->player.server,their->player.node))
                   break;

                //added on 2/1/99 by Victor Rachels for bans
                if(checkban(their->player.node))
                 {
                   network_dump_player(their->player.server, their->player.node, DUMP_DORK);
                   hud_message(MSGC_GAME_FEEDBACK, "%s tried to join at banned ip %d.%d.%d.%d", their->player.callsign,their->player.node[3],their->player.node[2],their->player.node[1],their->player.node[0] );
                 }
                //end this section addition - VR


                else if (Network_status == NETSTAT_STARTING)
                 {
			// Someone wants to join our game!
                  network_add_player(their);
                 }
                else if (Network_status == NETSTAT_WAITING)
                 {
			// Someone is ready to recieve a sync packet
			network_process_request(their);
                 }
		else if (Network_status == NETSTAT_PLAYING)
                 {

                 //Begin addition by GF                  
                 if(!restrict_mode)
                    network_welcome_player(their);
                 else if(restrict_mode)
                    lamer_network_welcome_player_restricted(their);


			// Someone wants to join a game in progress!
                 //-killed-        network_welcome_player(their);
                 //End addition by GF

                 }
		break;
		// Begin addition by GF
	case PID_DUMP:
		if ((Network_status == NETSTAT_WAITING) || (Network_status == NETSTAT_PLAYING))
			network_process_dump(their);
		break;
		// End addition by GF
        case PID_QUIT_JOINING:
		if (Network_status == NETSTAT_STARTING)
			network_remove_player( their );
		else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
			network_stop_resync( their );
		break;
        case PID_SYNC:
                if (Network_status == NETSTAT_WAITING)
        		network_read_sync_packet(data, 0);
		break;
	case PID_PDATA:	
		if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) )) { 
			network_read_pdata_packet(data, 0);
		}
		break;
//added on 8/4/98 by Matt Mueller
        case PID_SHORTPDATA:
                if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) )) {
			network_read_pdata_packet(data, 1);
                }
                break;
//end modified section - Matt Mueller
        case PID_OBJECT_DATA:
		if (Network_status == NETSTAT_WAITING) 
			network_read_object_packet(data);
		break;
	case PID_ENDLEVEL:
		if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
			network_read_endlevel_packet(data);
		else
			mprintf((0, "Junked endlevel packet.\n"));
		break;
	// D1X packets
        case PID_D1X_GAME_INFO_REQ:
		// Someone wants full info of our D1X game
		mprintf((0, "Got a PID_D1X_GAME_INFO_REQ!\n"));
		if ((Netgame.protocol_version == MULTI_PROTO_D1X_VER) &&
		    ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL)))
			if (network_i_am_master())
				network_send_game_info(their, 0);
		break;
	case PID_D1X_GAME_LITE_REQ:
                // Someone wants a list of D1X games
		mprintf((0, "Got a PID_D1X_GAME_LITE_REQ!\n"));
                if ((Netgame.protocol_version == MULTI_PROTO_D1X_VER) &&
                    ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL)))
                        if (network_i_am_master())
                                network_send_game_info(their, 1);
                break;
	case PID_D1X_GAME_INFO:
	case PID_D1X_GAME_LITE:
                if (Network_status == NETSTAT_BROWSING)
			network_process_gameinfo(data, 1);
                break;
	case PID_D1X_SYNC:
                if (Network_status == NETSTAT_WAITING)
			network_read_sync_packet(data, 1);
                break;
	case PID_PDATA_SHORT2:
		if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) )) { 
			network_read_pdata_packet(data, 2);
		}break;
//added 03/04/99 Matt Mueller - new direct data packets..
	  case PID_DIRECTDATA:
		if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) )) { 
		    mprintf((0,"got DIRECTDATA, len=%i (%i)\n",length,data[1]));
		    multi_process_bigdata( (char *)data+1, length-1);
		}	    
	    break;
//end addition -MM
        default:
		mprintf((0, "Ignoring invalid packet type.\n"));
		Int3(); // Invalid network packet type, see ROB
	}
}

#ifndef NDEBUG
void dump_segments()
{
	FILE * fp;

	fp = fopen( "TEST.DMP", "wb" );
	fwrite( Segments, sizeof(segment)*(Highest_segment_index+1),1, fp ); 	
	fclose(fp);
	mprintf( (0, "SS=%d\n", sizeof(segment) ));
}
#endif

void
network_read_endlevel_packet( ubyte *data )
{
	// Special packet for end of level syncing

	int playernum;
	endlevel_info *end;	

#ifdef NATIVE_PACKETS
	end = (endlevel_info *)data;
#else
	endlevel_info end_data;
	end = &end_data;
	receive_endlevel_packet(data, &end_data);
#endif

	playernum = end->player_num;
	
	Assert(playernum != Player_num);
	Assert(playernum < N_players);

	if ((Network_status == NETSTAT_PLAYING) && (end->connected != 0))
		return; // Only accept disconnect packets if we're not out of the level yet

	Players[playernum].connected = end->connected;
	memcpy(&kill_matrix[playernum][0], end->kill_matrix, MAX_PLAYERS*sizeof(short));
	Players[playernum].net_kills_total = end->kills;
	Players[playernum].net_killed_total = end->killed;

	if ((Players[playernum].connected == 1) && (end->seconds_left < Fuelcen_seconds_left))
		Fuelcen_seconds_left = end->seconds_left;

	LastPacketTime[playernum] = timer_get_approx_seconds();

//	mprintf((0, "Got endlevel packet from player %d.\n", playernum));
}

void
network_pack_objects(void)
{
	// Switching modes, pack the object array

	special_reset_objects();
}				

int
network_verify_objects(int remote, int local)
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
network_read_object_packet( ubyte *data )
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

//	mprintf((0, "Object packet %d (remote #%d) contains %d objects.\n", frame_num, remote_frame_num, nobj));

	for (i = 0; i < nobj; i++)
	{
		objnum = *(short *)(data+loc);			loc += 2;
		obj_owner = data[loc];						loc += 1;
		remote_objnum = *(short *)(data+loc);	loc += 2;

		if (objnum == -1) 
		{
			// Clear object array
			mprintf((0, "Clearing object array.\n"));

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
				network_pack_objects();
				mode = 0;
			}
			mprintf((0, "Objnum -2 found in frame local %d remote %d.\n", frame_num, remote_frame_num));
			mprintf((0, "Got %d objects, expected %d.\n", object_count, remote_objnum));
			if (remote_objnum != object_count) {
				Int3();
			}
			if (network_verify_objects(remote_objnum, object_count))
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
					network_pack_objects();
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
				memcpy(obj,data+loc,sizeof(object));		loc += sizeof(object);
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
	
void network_sync_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	// Polling loop waiting for sync packet to start game


	static fix t1 = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;

	network_listen();

//added/to-be-added on someday by someone for receiving msgs in waitlist

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		*key = -2;

	if (!Network_rejoined && (timer_get_approx_seconds() > t1+F1_0*2))
	{
		int i;

		// Poll time expired, re-send request
		
		t1 = timer_get_approx_seconds();

		mprintf((0, "Re-sending join request.\n"));
		i = network_send_request();
		if (i < 0)
			*key = -2;
	}
}

void network_start_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	int i,n,nm;

	key=key;
	citem=citem;

	Assert(Network_status == NETSTAT_STARTING);

	if (!menus[0].value) {
			menus[0].value = 1;
			menus[0].redraw = 1;
	}

	for (i=1; i<nitems; i++ )	{
		if ( (i>= N_players) && (menus[i].value) )	{
			menus[i].value = 0;
			menus[i].redraw = 1;
		}
	}

	nm = 0;
	for (i=0; i<nitems; i++ )	{
		if ( menus[i].value )	{
			nm++;
			if ( nm > N_players )	{
				menus[i].value = 0;
				menus[i].redraw = 1;
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
				menus[i].redraw = 1;
				break;
			}
	}

   //added/killed by Victor Rachels to eventually add msging
           //since nitems should not be changing, anyway
//        if (nitems > MAX_PLAYERS ) return;
   //end this section kill - VR
	
	n = Netgame.numplayers;
	network_listen();

	if (n < Netgame.numplayers ) 	
	{
		sprintf( menus[N_players-1].text, "%d. %-16s", N_players, Netgame.players[N_players-1].callsign );
		menus[N_players-1].redraw = 1;

		//Begin addition by GF
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);  //A noise to alert you when someone joins a starting game...
		//End addition by GF

		if (N_players <= MaxNumNetPlayers)
		{
			menus[N_players-1].value = 1;
		}
	} 
	else if ( n > Netgame.numplayers )	
	{
		// One got removed...

		//Begin addition by GF
		// <Taken out for now due to lack of testing> digi_play_sample(SOUND_HUD_KILL, F1_0);  //A noise to alert you when someone leaves a starting game...
		//End addition by GF

		for (i=0; i<N_players; i++ )	
		{
			sprintf( menus[i].text, "%d. %-16s", i+1, Netgame.players[i].callsign );
			if (i < MaxNumNetPlayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
			menus[i].redraw = 1;
		}
		for (i=N_players; i<n; i++ )	
		{
			sprintf( menus[i].text, "%d. ", i+1 );		// Clear out the deleted entries...
			menus[i].value = 0;
			menus[i].redraw = 1;
		}
   }
}

#ifndef SHAREWARE
void network_get_allowed_objects(int multivalues[40]) { //uint *flags) {
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	int i;

	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++) {
                m[i].type = NM_TYPE_CHECK; m[i].text = multi_allow_powerup_text[i]; m[i].value = multivalues[i+20];/*(*flags >> i) & 1;*/
	}
	i = newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, NULL, 0 );
//        if (i > -1) {
//                *flags &= ~NETFLAG_DOPOWERUP;
		for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
                        multivalues[i+20] = m[i].value;
                        /*if (m[i].value)
                                *flags |= (1 << i);*/
//        }
}
#endif

#ifndef SHAREWARE
int last_maxplayers;
void network_d1x_param_poll(int nitems, newmenu_item * menus, int * key, int citem) {
	if (last_maxplayers != menus[7].value) {
		last_maxplayers = menus[7].value;
		sprintf(menus[7].text, "Maximum players: %d", menus[7].value + 2);
		menus[7].redraw = 1;
	}
}


void network_get_d1x_params(netgame_info *temp_game, int *new_socket, int multivalues[40])   {
	int i, pps, socket;
//        uint flags;
	int opt;
	newmenu_item m[16];
	char spps[10];
//        char smaxplayers[32];
	char ssocket[10];
//added 11/01/98  Matthew Mueller
        char smaxplayers[30];
//end addition -MM

//        flags = temp_game->flags;
        last_maxplayers = -1;

        sprintf(spps, "%d", multivalues[11]); //
        sprintf(ssocket, "%d", multivalues[12]);  //*new_socket
//        sprintf(smaxplayers, "Maximum players: %d", multivalues[14]);

         {
          m[0].value  = multivalues[10];
          m[2].text   = spps;
          m[4].text   = ssocket;
          m[6].value  = multivalues[13];
          m[7].value  = multivalues[14]-2;
//added 11/01/98  Matthew Mueller
        sprintf(smaxplayers,"Maximum players: %d",multivalues[14]);
//end addition -MM
          m[9].value  = multivalues[15];
          m[10].value = multivalues[16];
//added on 11/12/98 by Victor Rachels for radar
          m[11].value = multivalues[17];
//end this section addition - VR
//added on 11/12/98 by Victor Rachels for alt vulcanfire
          m[12].value = multivalues[18];
//end this section addition - VR
         }

	opt = 0;
        /* 0*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "Short packets"; opt++;
	/* 1*/ m[opt].type = NM_TYPE_TEXT; m[opt].text = "Packets per second (2 to 20)"; opt++;
        /* 2*/ m[opt].type = NM_TYPE_INPUT; m[opt].text_len = 2; opt++;
	/* 3*/ m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network socket (-99 to 99)"; opt++;
        /* 4*/ m[opt].type = NM_TYPE_INPUT; m[opt].text_len = 3; opt++;

	/* 5*/ m[opt].type = NM_TYPE_TEXT; m[opt].text = "Options below need D1X only games"; opt++;
       /* 6*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "D1X only game"; opt++;
//added/edited on 11/1/98 by Matthew Mueller
        /* 7*/ m[opt].type = NM_TYPE_SLIDER; m[opt].text = smaxplayers; m[opt].min_value = 0; m[opt].max_value = MaxNumNetPlayers - 2; opt++;
//end edit -MM
	/* 8*/ m[opt].type = NM_TYPE_MENU; m[opt].text = "Set objects allowed..."; opt++;
        /* 9*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "Drop vulcan ammo"; opt++;
        /*10*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "Enable ignore/ghost"; opt++;

//added on 11/12/98 by Victor Rachels for radar
        /*11*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "Enable radar"; opt++;
//end this section addition -VR
        /*12*/ m[opt].type = NM_TYPE_CHECK; m[opt].text = "Short Vulcanfire"; opt++;

	i = 0;
	for (;;) {
		i = newmenu_do1( NULL, "D1X options", opt, m, &network_d1x_param_poll, i );
		if (i == 8) {
                        network_get_allowed_objects(multivalues); //&flags);
			continue;
                } else {// if (i > -1) {
			pps = atoi(spps);
			if (pps < 2 || pps > 20) {
			    nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid value for packets per second");
			    i = 2; // select pps field
                            sprintf(spps, "10");
			    continue;
			}
			socket = atoi(ssocket);
			if (socket < -99 || socket > 99) {
			    nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid value for network socket");
			    i = 4; // select socket field
                            sprintf(ssocket, "0");
			    continue;
			}
                } 
               
               break;
	}
//        if (i > -1) {

         {
          multivalues[10] = m[0].value;
          multivalues[11] = atoi(spps);
          multivalues[12] = atoi(ssocket);
          multivalues[13] = m[6].value;
          multivalues[14] = m[7].value + 2;
          multivalues[15] = m[9].value;
          multivalues[16] = m[10].value;
//added on 11/12/98 by Victor Rachels for radar
          multivalues[17] = m[11].value;
//end this section addition - VR
//added on 11/12/98 by Victor Rachels for alt vulcanfire
          multivalues[18] = m[12].value;
//end this section addition - VR

         }

/*                flags &= ~NETFLAG_SHORTPACKETS;
		if (m[0].value)
			flags |= NETFLAG_SHORTPACKETS;
                temp_game->packets_per_sec = pps;
		*new_socket = socket;
		temp_game->protocol_version = (m[6].value ?
			MULTI_PROTO_D1X_VER : MULTI_PROTO_VERSION);
		temp_game->max_numplayers = m[7].value + 2;
                flags &= ~NETFLAG_DROP_VULCAN_AMMO;
		if (m[9].value)
                        flags |= NETFLAG_DROP_VULCAN_AMMO;
                flags &= ~NETFLAG_ENABLE_IGNORE_GHOST;
		if (m[10].value)
                        flags |= NETFLAG_ENABLE_IGNORE_GHOST;
                temp_game->flags = flags;*/
//        }
}
#endif

int opt_cinvul;
int last_cinvul=0;
int opt_mode;
int last_mode;
netgame_info *cur_temp_game;

void network_game_param_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
#ifndef SHAREWARE
#ifndef ROCKWELL_CODE
	int i;
	for (i = 0; i < 4; i++)
		if (menus[opt_mode + i].value && last_mode != i) { // mode changed?
			last_mode = i;
			MaxNumNetPlayers = (i > 1) ? // robo-anarchy/cooperative
				4 : 8;
			cur_temp_game->max_numplayers = MaxNumNetPlayers;
		}
	if (menus[opt_mode+1].value && !menus[opt_mode+5].value) {
		menus[opt_mode+5].value = 1;
		menus[opt_mode+5].redraw = 1;
        }
	if (menus[opt_mode+3].value) {
                if (!menus[opt_mode+7].value) {
                        menus[opt_mode+7].value = 1;
                        menus[opt_mode+7].redraw = 1;
                }
        }
#endif

	if ( last_cinvul != menus[opt_cinvul].value )	{
                sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
                last_cinvul = menus[opt_cinvul].value;
                menus[opt_cinvul].redraw = 1;
        }               
#endif
}

int network_get_game_params( netgame_info *netgame, int *new_socket )
{
	int i;
        int opt, opt_name, opt_level, opt_closed, opt_difficulty;
	newmenu_item m[16];
	netgame_info temp_game;
	//char name[NETGAME_NAME_LEN+1];
	char slevel[5];
	char level_text[32];
#ifndef SHAREWARE
        int opt_showmap;
	char srinvul[32];

	int new_mission_num;
	int anarchy_only;
#endif

        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
        int multivalues[40];

        memset(multivalues,0,sizeof(int) * 40);
        //end this section addition - Victor

#ifndef SHAREWARE
        new_mission_num = multi_choose_mission(&anarchy_only);

	if (new_mission_num < 0)
		return -1;
#endif
        restrict_mode = 0;

	cur_temp_game = &temp_game;
	memset(&temp_game, 0, sizeof(temp_game));
	temp_game.protocol_version = MULTI_PROTO_VERSION;
        temp_game.difficulty = Player_default_difficulty;
#ifndef SHAREWARE
	strcpy(temp_game.mission_name, Mission_list[new_mission_num].filename);
        strcpy(temp_game.mission_title, Mission_list[new_mission_num].mission_name);
        temp_game.control_invul_time = control_invul_time;
        temp_game.flags = NETFLAG_DOPOWERUP | // enable all powerups
            (Network_initial_shortpackets ? NETFLAG_SHORTPACKETS : 0) |
	    NETFLAG_ENABLE_IGNORE_GHOST;
	temp_game.packets_per_sec = Network_initial_pps;
#endif
	temp_game.max_numplayers = 8;

	sprintf( temp_game.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	sprintf( slevel, "1" );
	last_mode = -1;

//         if((i=FindArg("-mpg")))
//          read_profile(gameinfo);

        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
        putto_multivalues(multivalues,&temp_game,new_socket);

         if((i=FindArg("-mprofile")))
          get_multi_profile(multivalues,Args[i+1]);
        //end this section addition - Victor

	opt = 0;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_DESCRIPTION; opt++;

	opt_name = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = temp_game.game_name; m[opt].text_len = NETGAME_NAME_LEN; opt++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
	if (Last_secret_level < -1)
		sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
	else if (Last_secret_level == -1)
		sprintf(level_text+strlen(level_text)-1, ", S1)");

	Assert(strlen(level_text) < 32);

	m[opt].type = NM_TYPE_TEXT; m[opt].text = level_text; opt++;

	opt_level = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = slevel; m[opt].text_len=4; opt++;

#ifdef ROCKWELL_CODE	
	opt_mode = 0;
#else
	//m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_MODE; opt++;
	opt_mode = opt;
        m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY; m[opt].value=1; m[opt].group=0; opt++;
        m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_TEAM_ANARCHY; m[opt].value=0; m[opt].group=0; opt++;
        m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY_W_ROBOTS; m[opt].value=0; m[opt].group=0; opt++;
        m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_COOPERATIVE; m[opt].value=0; m[opt].group=0; opt++;

        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
        m[opt_mode].value = multivalues[0];
        m[opt_mode+1].value = multivalues[1];
        m[opt_mode+2].value = multivalues[2];
        m[opt_mode+3].value = multivalues[3];
        //end this section addition - Victor
#endif
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_OPTIONS; opt++;

	opt_closed = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_CLOSED_GAME; m[opt].value=0; opt++;
        //added on 11/16/98 by Victor Rachels for restricted games
        m[opt].type = NM_TYPE_CHECK; m[opt].text = "Restricted Game"; m[opt].value=multivalues[8]; opt++;
        //end this section addition - Victor
        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
         m[opt_closed].value = multivalues[4];
        //end this section addition - Victor

#ifndef SHAREWARE
//	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_IDS; m[opt].value=0; opt++;
        opt_showmap = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=0; opt++;
        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
         m[opt_showmap].value = multivalues[5];
        //end this section addition - Victor

#endif

	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Player_default_difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;
        //added on 10/18/98 by Victor Rachels to add multiplayer profiles
         m[opt_difficulty].value = multivalues[6];
        //end this section addition - Victor

//	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Reactor Invulnerability (mins)"; opt++;
//	opt_cinvul = opt;
//	sprintf( srinvul, "%d", control_invul_time );
//	m[opt].type = NM_TYPE_INPUT; m[opt].text = srinvul; m[opt].text_len=2; opt++;
		
#ifndef SHAREWARE
	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, 5*control_invul_time, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=control_invul_time; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=15; opt++;
        //added/changed on 10/18/98 by Victor Rachels to add multiplayer profiles
        m[opt_cinvul].value = multivalues[7];
        last_cinvul = multivalues[7];
        //end this section addition - Victor

	//m[opt].type = NM_TYPE_CHECK; m[opt].text = "D1X only"; m[opt].value=0; opt++;
	m[opt].type = NM_TYPE_MENU; m[opt].text = "D1X options..."; opt++;
        //added/changed on 10/18/98 by Victor Rachels to add multiplayer profiles
        m[opt].type = NM_TYPE_MENU; m[opt].text = "Multi Profile..."; opt++;
        //end this section addition - Victor
#endif

	Assert(opt <= 16);

//added on 11/18/98 bu Victor Rachels to add -startnetgame       
       i = 0;

        if(start_net_immediately)
         start_net_immediately = 0;
        else
//end this section addition
menu:
       
	do  {
		i = newmenu_do1( NULL, NULL /*TXT_NETGAME_SETUP*/, opt, m, network_game_param_poll, 1 );
		#ifndef SHAREWARE
                if (i == 14) {
			//if (!m[13].value)
			//	  nm_messagebox(TXT_ERROR, 1, TXT_OK, "You can only set D1X options\nfor D1X only games");
			//else
                        network_get_d1x_params(&temp_game, new_socket, multivalues);
		}
        //added/changed on 10/18/98 by Victor Rachels to add multiplayer profiles
                if (i == 15) {
                        if (do_multi_profile(multivalues))
                         {
                           m[opt_mode].value = multivalues[0];
                           m[opt_mode+1].value = multivalues[1];
                           m[opt_mode+2].value = multivalues[2];
                           m[opt_mode+3].value = multivalues[3];
                           m[opt_closed].value = multivalues[4];
                           #ifndef SHAREWARE
                           m[opt_showmap].value = multivalues[5];
                           #endif
                           m[opt_difficulty].value = multivalues[6];
                           #ifndef SHAREWARE
                           m[opt_cinvul].value = multivalues[7];
                           #endif
                           m[opt_closed+1].value = multivalues[8];
                         }
                }
        //end this section addition - Victor
		#endif
        } while (i > 13);

	if ( i > -1 )	{
		int j;

		for (j = 0; j < num_active_games; j++)
			if (!strcasecmp(Active_games[j].game_name, temp_game.game_name))
			{
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DUPLICATE_NAME);
				goto menu;
			}

		//strcpy( game_name, name );

    //added/changed on 10/23/98 by Victor Rachels to add multiplayer profiles
                 multivalues[0] = m[opt_mode].value;
                 multivalues[1] = m[opt_mode+1].value;
                 multivalues[2] = m[opt_mode+2].value;
                 multivalues[3] = m[opt_mode+3].value;
                 multivalues[4] = m[opt_closed].value;
                 multivalues[6] = m[opt_difficulty].value;
    #ifndef SHAREWARE
                 multivalues[5] = m[opt_showmap].value;
                 multivalues[7] = m[opt_cinvul].value;
    #endif
                 multivalues[8] = m[opt_closed+1].value;
    //end this section addition - Victor Rachels

		if (!strncasecmp(slevel, "s", 1))
			temp_game.levelnum = -atoi(slevel+1);
		else
			temp_game.levelnum = atoi(slevel);

		if ((temp_game.levelnum < Last_secret_level) || (temp_game.levelnum > Last_level) || (temp_game.levelnum == 0))
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
			sprintf(slevel, "1");
			goto menu;
		}
//added/killed on 10/18/98 by Victor Rachels to add multiplayer profiles
//-killed- #ifdef ROCKWELL_CODE
//-killed-                 temp_game.mode = NETGAME_COOPERATIVE;
//-killed- #else
//-killed-                 if ( m[opt_mode].value )
//-killed-                         temp_game.gamemode = NETGAME_ANARCHY;
//-killed- #ifndef SHAREWARE
//-killed-                 else if (m[opt_mode+1].value)
//-killed-                         temp_game.gamemode = NETGAME_TEAM_ANARCHY;
//-killed-                 else if (anarchy_only) {
//-killed-                         nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
//-killed-                         m[opt_mode+2].value = 0;
//-killed-                         m[opt_mode+3].value = 0;
//-killed-                         m[opt_mode].value = 1;
//-killed-                         goto menu;
//-killed-                 } else if ( m[opt_mode+2].value )
//-killed-                         temp_game.gamemode = NETGAME_ROBOT_ANARCHY;
//-killed-                 else if ( m[opt_mode+3].value )
//-killed-                         temp_game.gamemode = NETGAME_COOPERATIVE;
//-killed-                 else Int3(); // Invalid mode -- see Rob
//-killed- #else
//-killed-                 else {
//-killed-                         nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_REGISTERED_ONLY);
//-killed-                         m[opt_mode+2].value = 0;
//-killed-                         m[opt_mode+3].value = 0;
//-killed-                         m[opt_mode].value = 1;
//-killed-                         goto menu;
//-killed-                 }
//-killed- #endif
//-killed- #endif  // ifdef ROCKWELL
//-killed- 
//-killed-                 temp_game.game_flags = 0;
//-killed-                 if (m[opt_closed].value)
//-killed-                         temp_game.game_flags |= NETGAME_FLAG_CLOSED;
//-killed- #ifndef SHAREWARE
//-killed- //              if (m[opt_closed+1].value)
//-killed- //                      temp_game.game_flags |= NETGAME_FLAG_SHOW_ID;
//-killed-                 if (m[opt_showmap].value)
//-killed-                         temp_game.game_flags |= NETGAME_FLAG_SHOW_MAP;
//-killed- #endif
//-killed- 
//-killed-                 temp_game.difficulty = Difficulty_level = m[opt_difficulty].value;

//-killed- #ifndef SHAREWARE
		//control_invul_time = atoi( srinvul )*60*F1_0;
//-killed                control_invul_time = m[opt_cinvul].value;
//-killed                temp_game.control_invul_time = control_invul_time*5*F1_0*60;
		//temp_game.protocol_version = (m[13].value ?
		//	  MULTI_PROTO_D1X_VER : MULTI_PROTO_VERSION);

//end this section kill - Victor

 //added/changed on 10/18/98 by Victor Rachels to add multiplayer profiles
                putfrom_multivalues(multivalues,&temp_game,new_socket);
#ifndef SHAREWARE
                control_invul_time = temp_game.control_invul_time;
                temp_game.control_invul_time = control_invul_time*5*F1_0*60;
 //end this section addition - Victor

		temp_game.subprotocol = MULTI_PROTO_D1X_MINOR;
		temp_game.required_subprotocol = 0;
#endif
                *netgame = temp_game;

		if ((netgame->gamemode == 2 || netgame->gamemode == 3) && netgame->max_numplayers > 4) // restrict players in robo-anarchy/cooperative if player hasn't done so far...
			netgame->max_numplayers = 4;
	}
	return i;
}

void
network_set_game_mode(int gamemode)
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
	#if 0 // adb: now done elsewhere
	if (Game_mode & GM_MULTI_ROBOTS)
		MaxNumNetPlayers = 4;
	else
		MaxNumNetPlayers = 8;
	#endif
}

int
network_find_game(void)
{
	// Find out whether or not there is space left on this socket

	fix t1;

	Network_status = NETSTAT_BROWSING;

	num_active_games = 0;

	show_boxed_message(TXT_WAIT);

	network_send_game_list_request();
	t1 = timer_get_approx_seconds() + F1_0*2;

	while (timer_get_approx_seconds() < t1) // Wait 3 seconds for replies
		network_listen();

	clear_boxed_message();

//	mprintf((0, "%s %d %s\n", TXT_FOUND, num_active_games, TXT_ACTIVE_GAMES));

	if (num_active_games < MAX_ACTIVE_NETGAMES)
		return 0;
	return 1;
}
	
void network_read_sync_packet( ubyte * data, int d1x )
{	
	int i, j;
	netgame_info *sp = &Netgame;

	char temp_callsign[CALLSIGN_LEN+1];
	
	// This function is now called by all people entering the netgame.

	// mprintf( (0, "%s %d\n", TXT_STARTING_NETGAME, sp->levelnum ));

	if (data)
	{
#ifdef NATIVE_PACKETS
		memcpy( &Netgame, data, sizeof(netgame_info) );
#else
		receive_netgame_packet( data, &Netgame, d1x );
#endif
	}

	N_players = sp->numplayers;
	Difficulty_level = sp->difficulty;
	Network_status = sp->game_status;

	Assert(Function_mode != FMODE_GAME);

	// New code, 11/27

	mprintf((1, "Netgame.checksum = %d, calculated checksum = %d.\n", Netgame.segments_checksum, my_segments_checksum));

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
		if ((!memcmp( sp->players[i].node, My_Seq.player.node, 6 )) &&
                         (!strcasecmp( sp->players[i].callsign, temp_callsign)) )
		{
			Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
			change_playernum_to(i);
		}
		memcpy( Players[i].callsign, sp->players[i].callsign, CALLSIGN_LEN+1 );

#ifndef SHAREWARE
		if ( (*(uint *)sp->players[i].server) != 0 )
			ipx_get_local_target( sp->players[i].server, sp->players[i].node, Players[i].net_address );
		else
#endif
			memcpy( Players[i].net_address, sp->players[i].node, 6 );

		Players[i].n_packets_got=0;				// How many packets we got from them
		Players[i].n_packets_sent=0;				// How many packets we sent to them
		Players[i].connected = sp->players[i].connected;
                //added/edited on 11/12/98 by Victor Rachels += -> =
                Players[i].net_kills_total = sp->player_kills[i];
                //end this section edit - VR

#ifndef SHAREWARE
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = sp->player_score[i];
#endif		
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
        multi_kills_stat -= Players[Player_num].net_kills_total;
        multi_deaths_stat -= Players[Player_num].net_killed_total;
//end this section addition - VR

#ifndef SHAREWARE
	if (Network_rejoined) {
		network_process_monitor_vector(sp->monitor_vector);
		Players[Player_num].time_level = sp->level_time;
	}
#endif

	team_kills[0] = sp->team_kills[0];
	team_kills[1] = sp->team_kills[1];
	
	Players[Player_num].connected = 1;
	Netgame.players[Player_num].connected = 1;

	if (!Network_rejoined)
		for (i=0; i<MaxNumNetPlayers; i++) {
			Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
			Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
		 	obj_relink(Players[i].objnum,Player_init[Netgame.locations[i]].segnum);
		}

	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

#ifndef SHAREWARE
	// process D1X options
	if (Netgame.protocol_version == MULTI_PROTO_D1X_VER) {
		multi_allow_powerup = Netgame.flags & NETFLAG_DOPOWERUP;
		Network_short_packets = (Netgame.flags & NETFLAG_SHORTPACKETS) ? 2 : 0;
                Network_pps = Netgame.packets_per_sec;
                Laser_drop_vulcan_ammo = (Netgame.flags & NETFLAG_DROP_VULCAN_AMMO) != 0;
		Network_enable_ignore_ghost = (Netgame.flags & NETFLAG_ENABLE_IGNORE_GHOST) != 0;
		// in D1X only games everybody has the same shp setting
                //added on 11/12/98 by Victor Rachels to add radar
                Network_allow_radar = (Netgame.flags & NETFLAG_ENABLE_RADAR) != 0;
                 if (Network_allow_radar)
                  show_radar = 1;
                //end this section addition - VR
                //added on 4/23/99 by Victor Rachels to add altvlcnfire
                use_alt_vulcanfire = (Netgame.flags & NETFLAG_ENABLE_ALT_VULCAN) != 0;
                //end this section addition - VR
		for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		    Net_D1xPlayer[i].shp = Network_short_packets;
	} else {
		if (data) { // adb: master does have this info
			Network_short_packets = 0;
			Network_pps = 10;
		}
		multi_allow_powerup = NETFLAG_DOPOWERUP;
		Laser_drop_vulcan_ammo = 0;
		Network_enable_ignore_ghost = 1; // Might not be what everybody wants, but v0.08 does it too.
	}
	#endif

        Network_status = NETSTAT_PLAYING;
	Function_mode = FMODE_GAME;
	multi_sort_kill_list();

}

void
network_abort_game(void)
{
        int i;
        for (i=1; i<N_players; i++)
                network_dump_player(Netgame.players[i].server,Netgame.players[i].node, DUMP_ABORTED);

	N_players = Netgame.numplayers = 0;
	network_send_game_info(NULL, 1); // Tell everyone we're bailing
}

int
network_send_sync(void)
{
	int i, j, np;

	// Randomize their starting locations...

	if (NumNetPlayerPositions < MaxNumNetPlayers) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Not enough start positions\n(need %d got %d)\nNetgame aborted", MaxNumNetPlayers, NumNetPlayerPositions);
		network_abort_game();
		//return -1;
		longjmp(LeaveGame, 1);
	}

        //added/changed on 9/13/98 by adb to remove TICKER
        d_srand( timer_get_approx_seconds() );
        //end change - adb

        for (i=0; i<MaxNumNetPlayers; i++ )
	{
		if (Players[i].connected)
			Players[i].connected = 1; // Get rid of endlevel connect statuses
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
//			mprintf((0, "Player %d starting in location %d\n" ,i ,np ));
		}
	}

	// Push current data into the sync packet

	network_update_netgame();
	Netgame.game_status = NETSTAT_PLAYING;
	Netgame.type = (Netgame.protocol_version == MULTI_PROTO_D1X_VER ? PID_D1X_SYNC : PID_SYNC);
	Netgame.segments_checksum = my_segments_checksum;

	for (i=0; i<N_players; i++ )	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;

#ifdef NATIVE_PACKETS
		// Send several times, extras will be ignored
		ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Netgame.players[i].server, Netgame.players[i].node);
		ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Netgame.players[i].server, Netgame.players[i].node);
		ipx_send_internetwork_packet_data( (ubyte *)&Netgame, sizeof(netgame_info), Netgame.players[i].server, Netgame.players[i].node);
#else
		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
#endif
	}
	network_read_sync_packet(NULL, 1); // Read it myself, as if I had sent it
	return 0;
}

int
network_select_teams(void)
{
#ifndef SHAREWARE
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
	
	choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, NULL);

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
#else
	return 0;
#endif
}

int
network_select_players(void)
{
        int i, j, opts, opt_msg;
        newmenu_item m[MAX_PLAYERS+1];
	char text[MAX_PLAYERS][25];
	char title[50];
	int save_nplayers;

	network_add_player( &My_Seq );
		
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
            j=newmenu_do1( NULL, title, opts, m, network_start_poll, 1 );

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
		network_abort_game();

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
				memcpy(Netgame.players[N_players].node, Netgame.players[i].node, 6);
				memcpy(Netgame.players[N_players].server, Netgame.players[i].server, 4);
			}
			Players[N_players].connected = 1;
			N_players++;
		}
		else
		{
			network_dump_player(Netgame.players[i].server,Netgame.players[i].node, DUMP_DORK);
		}
	}

	for (i = N_players; i < MAX_NUM_NET_PLAYERS; i++) {
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN+1);
		memset(Netgame.players[i].node, 0, 6);
		memset(Netgame.players[i].server, 0, 4);
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
		if (!network_select_teams())
			goto abort;

	return(1); 
}

void network_start_game(void)	
{
	int i, new_socket;
	//char game_name[NETGAME_NAME_LEN+1];
	//int chosen_game_mode, game_flags, level;

	Assert( sizeof(frame_info) < IPX_MAX_DATA_SIZE );

	mprintf((0, "Using frame_info len %d, max %d.\n", sizeof(frame_info), IPX_MAX_DATA_SIZE));

	if ( !Network_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND );
		return;
	}

	if (setjmp(LeaveGame)) {
               Game_mode = GM_GAME_OVER;
		return;
	}

	network_init();
	change_playernum_to(0);

	if (network_find_game())
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_NET_FULL);
		return;
	}

	//game_flags = 0;
	new_socket = Network_socket;
	i = network_get_game_params( &Netgame, &new_socket );

	if (i<0) return;

	network_change_socket(new_socket);

	N_players = 0;

// LoadLevel(level); Old, no longer used.


	//adb: this is now set in network_get_game_params
	//Netgame.difficulty = Difficulty_level;
	//Netgame.gamemode = chosen_game_mode;
	//Netgame.levelnum = level;
        //Netgame.game_flags = game_flags;
        //Netgame.protocol_version = MULTI_PROTO_VERSION;
        //strcpy(Netgame.game_name, game_name);
        Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	network_set_game_mode(Netgame.gamemode);
	MaxNumNetPlayers = Netgame.max_numplayers;

	Network_status = NETSTAT_STARTING;

	//adb: the following will be overwritten for D1X only games
#ifndef SHAREWARE
	Network_pps = Netgame.packets_per_sec;
	Network_short_packets = (Netgame.flags & NETFLAG_SHORTPACKETS) ? 1 : 0;
#endif

	if(network_select_players())
	{
		StartNewLevel(Netgame.levelnum);
		//added 8/5/98 by Matt Mueller- moved stuff to a function
		//edited 04/19/99 Matt Mueller
	//-killed-	network_send_config_messages(100,1);
		multi_d1x_ver_queue_init(MaxNumNetPlayers,1);
		//end edit -MM
		//end modified section - Matt Mueller
        }
	else
		Game_mode = GM_GAME_OVER;
	
}

#ifndef NETWORK_NEW_LIST
void restart_net_searching(newmenu_item * m)
{
	int i;
	N_players = 0;
	num_active_games = 0;

	memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		sprintf(m[(2*i)+1].text, "%d.                                       ", i+1);
		sprintf(m[(2*i)+2].text, " \n");
		m[(2*i)+1].redraw = 1;
		m[(2*i)+2].redraw = 1;
	}
	Network_games_changed = 1;	
}

void network_join_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int i, new_socket;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	if (Network_allow_socket_changes )	{
		new_socket = Network_socket;
	
		if ( *key==KEY_PAGEUP ) 	{ new_socket--; *key = 0; }
		if ( *key==KEY_PAGEDOWN )	{ new_socket++; *key = 0; }
	
		if ( network_change_socket(new_socket) )	 {
			sprintf( menus[0].text, "%s %+d", TXT_CURRENT_IPX_SOCKET, Network_socket );
			menus[0].redraw = 1;
			restart_net_searching(menus);
			network_send_game_list_request();
			return;
		}
	}

	if (timer_get_approx_seconds() > t1+F1_0*4)
	{
		t1 = timer_get_approx_seconds();
		network_send_game_list_request();
	}				
	
	network_listen();

	if (!Network_games_changed)
		return;

	Network_games_changed = 0;

	// Copy the active games data into the menu options
	for (i = 0; i < num_active_games; i++)
	{
		int game_status = Active_games[i].game_status;
		int j, nplayers = 0;
		char levelname[4];

		for (j = 0; j < Active_games[i].numplayers; j++)
			if (Active_games[i].players[j].connected)
				nplayers++;

		if (Active_games[i].levelnum < 0)
			sprintf(levelname, "S%d", -Active_games[i].levelnum);
		else 
			sprintf(levelname, "%d", Active_games[i].levelnum);

		sprintf(menus[(2*i)+1].text, "%d. %s (%s)", i+1, Active_games[i].game_name, MODE_NAMES(Active_games[i].gamemode));

		if (game_status == NETSTAT_STARTING) 
		{
			sprintf(menus[(2*i)+2].text, "%s%s  %s%d\n", TXT_NET_FORMING, levelname, TXT_NET_PLAYERS, nplayers);
		}
		else if (game_status == NETSTAT_PLAYING)
		{
			if (can_join_netgame(&Active_games[i]))
				sprintf(menus[(2*i)+2].text, "%s%s  %s%d\n", TXT_NET_JOIN, levelname, TXT_NET_PLAYERS, nplayers);
			else
				sprintf(menus[(2*i)+2].text, "%s\n", TXT_NET_CLOSED);
		}
		else
			sprintf(menus[(2*i)+2].text, "%s\n", TXT_NET_BETWEEN);

#ifndef SHAREWARE
		if (strlen(Active_games[i].mission_name) > 0)
			sprintf(menus[(2*i)+2].text+strlen(menus[(2*i)+2].text), "%s%s", TXT_MISSION, Active_games[i].mission_title);
#endif

		Assert(strlen(menus[(2*i)+2].text) < 70);
		menus[(2*i)+1].redraw = 1;
		menus[(2*i)+2].redraw = 1;
	}

	for (i = num_active_games; i < MAX_ACTIVE_NETGAMES; i++)
	{
		sprintf(menus[(2*i)+1].text, "%d.                                       ", i+1);
		sprintf(menus[(2*i)+2].text, " \n");
		menus[(2*i)+1].redraw = 1;
		menus[(2*i)+2].redraw = 1;
	}
}
#endif

int
network_wait_for_sync(void)
{
	char text[60];
	newmenu_item m[2];
	int i, choice;
	
	Network_status = NETSTAT_WAITING;
	m[0].type=NM_TYPE_TEXT; m[0].text = text;
	m[1].type=NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;
	
	i = network_send_request();

	if (i < 0)
		return(-1);

	sprintf( m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER );

menu:	
	choice=newmenu_do( NULL, TXT_WAIT, 2, m, network_sync_poll );

	if (choice > -1)
		goto menu;

	if (Network_status != NETSTAT_PLAYING)	
	{
		sequence_packet me;

//		if (Network_status == NETSTAT_ENDLEVEL)
//		{
//		 	network_send_endlevel_packet(0);
//			longjmp(LeaveGame, 1);
//		}		

		mprintf((0, "Aborting join.\n"));
		me.type = PID_QUIT_JOINING;
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		memcpy( me.player.node, ipx_get_my_local_address(), 6 );
		memcpy( me.player.server, ipx_get_my_server_address(), 4 );
#ifdef NATIVE_PACKETS
		ipx_send_internetwork_packet_data( (ubyte *)&me, sizeof(sequence_packet), Netgame.players[0].server, Netgame.players[0].node );
#else
		send_sequence_packet( me, Netgame.players[0].server, Netgame.players[0].node, NULL );
#endif
		N_players = 0;
		Function_mode = FMODE_MENU;
		Game_mode = GM_GAME_OVER;
		return(-1);	// they cancelled		
	}
	return(0);
}

void 
network_request_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

//	if (timer_get_approx_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
//	{
//		network_send_endlevel_packet();
//		t1 = timer_get_approx_seconds();
//	}

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected == 1) || (Players[i].connected == 0))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		*key = -2;
	}
}

void
network_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	newmenu_item m[1];
	
	Network_status = NETSTAT_WAITING;

	m[0].type=NM_TYPE_TEXT; m[0].text = TXT_NET_LEAVE;

	mprintf((0, "Entered wait_for_requests : N_players = %d.\n", N_players));

	for (choice = 0; choice < N_players; choice++)
		mprintf((0, "Players[%d].connected = %d.\n", choice, Players[choice].connected));

	Network_status = NETSTAT_WAITING;
	network_flush();

	Players[Player_num].connected = 1;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 1, m, network_request_poll);	

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return;
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (i=0; i < N_players; i++)
			if ((Players[i].connected != 0) && (i != Player_num))
				network_dump_player(Netgame.players[i].server, Netgame.players[i].node, DUMP_ABORTED);

		longjmp(LeaveGame, 1);
	}
	else if (choice != -2)
		goto menu;
}

int
network_level_sync(void)
{
 	// Do required syncing between (before) levels

	int result;

	mprintf((0, "Player %d entering network_level_sync.\n", Player_num));

	MySyncPackInitialized = 0;

//	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));

	network_flush(); // Flush any old packets

	if (N_players == 0)
		result = network_wait_for_sync();
	else if (network_i_am_master())
	{
		network_wait_for_requests();
		result = network_send_sync();
		if (result)
			return -1;
	}
	else
		result = network_wait_for_sync();

	if (result)
	{
		Players[Player_num].connected = 0;
		network_send_endlevel_packet();
		longjmp(LeaveGame, 1);
	}
	return(0);
}

//moved 2000/02/07 Matt Mueller - clipped stuff from network_join_game into new network_do_join_game to allow easy joining from other funcs too.
int network_do_join_game(netgame_info *jgame){
	if (jgame->game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	if ((jgame->protocol_version != MULTI_PROTO_VERSION) &&
	    (jgame->protocol_version != MULTI_PROTO_D1X_VER))
	{
                nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		return 0;
	}
#ifndef SHAREWARE
	if (jgame->protocol_version == MULTI_PROTO_D1X_VER &&
	    jgame->required_subprotocol > MULTI_PROTO_D1X_MINOR)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, "This game uses features\nnot present in this version.");
		return 0;
	}
	{	
		// Check for valid mission name
			mprintf((0, "Loading mission:%s.\n", jgame->mission_name));
			if (!load_mission_by_name(jgame->mission_name))
			{
				nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);

                                //add getlevel functionality here - Victor Rachels

				return 0;
			}
	}
#endif

	if (!can_join_netgame(jgame))
	{
		if (jgame->numplayers == jgame->max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return 0;
	}

	// Choice is valid, prepare to join in

	memcpy(&Netgame, jgame, sizeof(netgame_info));

	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	network_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);

	return 1;     // look ma, we're in a game!!!
}

void network_join_game()
{
	int choice;
#ifndef NETWORK_NEW_LIST
	int i;
	char menu_text[(MAX_ACTIVE_NETGAMES*2)+1][70];
	
	newmenu_item m[((MAX_ACTIVE_NETGAMES)*2)+1];
#endif

	if ( !Network_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	network_init();

	N_players = 0;

	setjmp(LeaveGame);

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu
	
	network_listen();  // Throw out old info

	network_send_game_list_request(); // broadcast a request for lists

	num_active_games = 0;
	memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

#ifndef NETWORK_NEW_LIST
	memset(m, 0, sizeof(newmenu_item)*(MAX_ACTIVE_NETGAMES*2));
	
	m[0].text = menu_text[0];
	m[0].type = NM_TYPE_TEXT;
	if (Network_allow_socket_changes)
		sprintf( m[0].text, "Current IPX Socket is default%+d", Network_socket );
	else
		sprintf( m[0].text, "" );

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		m[2*i+1].text = menu_text[2*i+1];
		m[2*i+2].text = menu_text[2*i+2];
		m[2*i+1].type = NM_TYPE_MENU;
		m[2*i+2].type = NM_TYPE_TEXT;
		sprintf(m[(2*i)+1].text, "%d.                                       ", i+1);
		sprintf(m[(2*i)+2].text, " \n");
		m[(2*i)+1].redraw = 1;
		m[(2*i)+2].redraw = 1;
	}

	Network_games_changed = 1;	
remenu:
	choice=newmenu_do1(NULL, TXT_NET_SEARCHING, (MAX_ACTIVE_NETGAMES)*2+1, m, network_join_poll, 0 );

	if (choice==-1)	{
		Network_status = NETSTAT_MENU;
		return;	// they cancelled		
        }
	choice--;
	choice /= 2;
#else
remenu:
	choice = network_join_game_menu();
	if (choice==-1) {
		Network_status = NETSTAT_MENU;
		return;	// they cancelled		
	}
#endif

	if (choice >=num_active_games)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
		goto remenu;
	}

	// Choice has been made and looks legit


	if (network_do_join_game(&Active_games[choice])==0)
		goto remenu;

	return;		// look ma, we're in a game!!!
}

void network_leave_game()
{
	network_do_frame(1, 1);

	if ((network_i_am_master()) &&
	    (Netgame.numplayers == 1 || Network_status == NETSTAT_STARTING))
	{
		N_players = Netgame.numplayers = 0;
		network_send_game_info(NULL, 1);
	}
	
	Players[Player_num].connected = 0;
	network_send_endlevel_packet();
	change_playernum_to(0);
	Game_mode = GM_GAME_OVER;
	network_flush();
}

void network_flush()
{
	ubyte packet[IPX_MAX_DATA_SIZE];

	if (!Network_active)
		return;

	while (ipx_get_packet_data(packet) > 0)
		;
}

void network_listen()
{
	int size;
	ubyte packet[IPX_MAX_DATA_SIZE];

	if (!Network_active) return;

	if (!(Game_mode & GM_NETWORK) && (Function_mode == FMODE_GAME))
		mprintf((0, "Calling network_listen() when not in net game.\n"));

	size = ipx_get_packet_data( packet );
	while ( size > 0 )	{
//	    mprintf((0,"{%i}",Player_num));
		network_process_packet( packet, size );
		size = ipx_get_packet_data( packet );
	}
}

void network_send_data( ubyte * ptr, int len, int urgent )
{
	char check;

	if (Endlevel_sequence)
		return;

	if (!MySyncPackInitialized)	{
		MySyncPackInitialized = 1;
		memset( &MySyncPack, 0, sizeof(frame_info) );
	}
	
	if (urgent)
		PacketUrgent = 1;

	if ((MySyncPack.data_size+len) > NET_XDATA_SIZE )	{
		check = ptr[0];
		network_do_frame(1, 0);
		if (MySyncPack.data_size != 0) {
			mprintf((0, "%d bytes were added to data by network_do_frame!\n", MySyncPack.data_size));
			Int3();
		}
//		Int3();		// Trying to send too much!
//		return;
		mprintf((0, "Packet overflow, sending additional packet, type %d len %d.\n", ptr[0], len));
		Assert(check == ptr[0]);
	}

	Assert(MySyncPack.data_size+len <= NET_XDATA_SIZE);

	memcpy( &MySyncPack.data[MySyncPack.data_size], ptr, len );
	MySyncPack.data_size += len;
}

void network_timeout_player(int playernum)
{
	// Remove a player from the game if we haven't heard from them in 
	// a long time.
	int i, n = 0;

	Assert(playernum < N_players);
	Assert(playernum > -1);

	network_disconnect_player(playernum);
	create_player_appearance_effect(&Objects[Players[playernum].objnum]);

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	hud_message(MSGC_MULTI_INFO, "\002%c%s\004 %s",
			gr_getcolor(player_rgb[playernum].r,player_rgb[playernum].g,player_rgb[playernum].b)+1,
			Players[playernum].callsign, TXT_DISCONNECTING);
	for (i = 0; i < N_players; i++)
		if (Players[i].connected) 
			n++;

	if (n == 1)
	{
//added/changed on 10/11/98 by Victor Rachels cuz this is annoying as a box
//-killed-                nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
          hud_message(MSGC_GAME_FEEDBACK, TXT_YOU_ARE_ONLY);
//end this section change - VR
	}
}

fix last_send_time = 0;
fix last_timeout_check = 0;

//added on 11/29/99 by Victor Rachels to resend ghosting
fix last_observer_send_time = 0;
//end this section addition - VR

void network_do_frame(int force, int listen)
{
	int i;
#ifdef NATIVE_PACKETS
	int objnum;
#endif

	if (!(Game_mode&GM_NETWORK)) return;

	if ((Network_status != NETSTAT_PLAYING) || (Endlevel_sequence)) // Don't send postion during escape sequence...
		goto listen;

	last_send_time += FrameTime;
	last_timeout_check += FrameTime;

//added on 11/29/99 by Victor Rachels to resend ghosting
         if(I_am_observer)
          {
            last_observer_send_time += FrameTime;
             if(last_observer_send_time > (OBSERVER_RESEND_DELAY*F1_0))
              {
                multi_send_observerghost(100);
                last_observer_send_time = 0;
              }
          }
//end this section addition - VR


	// Send out packet 10 times per second maximum... unless they fire, then send more often...
//added/edited on 11/28/99 by Victor Rachels to use less bandwidth for observer
        if ( (last_send_time > (F1_0 / (I_am_observer?1:Network_pps))) ||
	     (Network_laser_fired) || force || PacketUrgent )	    {
		if ( Players[Player_num].connected )	{
//		    printf(",");//####
			PacketUrgent = 0;

			if (listen) {
#ifndef SHAREWARE
				multi_send_robot_frame(0);
#endif
                                multi_send_fire(100);              // Do firing if needed..
			}

//			mprintf((0, "Send packet, %f secs, %d bytes.\n", f2fl(last_send_time), MySyncPack.data_size));

			last_send_time = 0;

#ifdef NATIVE_PACKETS
			objnum = Players[Player_num].objnum;
                        MySyncPack.type                         = PID_PDATA;
			MySyncPack.playernum 			= Player_num;
#ifdef SHAREWARE
                        MySyncPack.objnum                       = Players[Player_num].objnum;
#endif
			MySyncPack.obj_segnum			= Objects[objnum].segnum;
			MySyncPack.obj_pos				= Objects[objnum].pos;
			MySyncPack.obj_orient			= Objects[objnum].orient;
#ifdef SHAREWARE
			MySyncPack.obj_phys_info		= Objects[objnum].mtype.phys_info;
#else
			MySyncPack.phys_velocity 		= Objects[objnum].mtype.phys_info.velocity;
			MySyncPack.phys_rotvel			= Objects[objnum].mtype.phys_info.rotvel;
#endif
			MySyncPack.obj_render_type		= Objects[objnum].render_type;
                        MySyncPack.level_num                    = Current_level_num;
#endif

			for (i=0; i<N_players; i++ )	{
				if ( (Players[i].connected) && (i!=Player_num ) )	{
					MySyncPack.numpackets = ++Players[i].n_packets_sent; // adb: changed ...++ to ++... to prevent bogus missed packets msg
//					mprintf((0, "sync pack is %d bytes long.\n", sizeof(frame_info)));
#ifdef NATIVE_PACKETS
					ipx_send_packet_data( (ubyte *)&MySyncPack, sizeof(frame_info)-NET_XDATA_SIZE+MySyncPack.data_size, Netgame.players[i].server, Netgame.players[i].node,Players[i].net_address );
#else
					send_frameinfo_packet(Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address, Net_D1xPlayer[i].shp);
#endif
				}
			}
			MySyncPack.data_size = 0;		// Start data over at 0 length.
			if (Fuelcen_control_center_destroyed)
				network_send_endlevel_packet();
			//mprintf( (0, "Packet has %d bytes appended (TS=%d)\n", MySyncPack.data_size, sizeof(frame_info)-NET_XDATA_SIZE+MySyncPack.data_size ));
		}
	}

	if (!listen)
		return;

	if ((last_timeout_check > F1_0) && !(Fuelcen_control_center_destroyed))
	{
		fix approx_time = timer_get_approx_seconds();
		// Check for player timeouts
		for (i = 0; i < N_players; i++)
		{
			if ((i != Player_num) && (Players[i].connected == 1))
			{
				if ((LastPacketTime[i] == 0) || (LastPacketTime[i] > approx_time))
				{
					LastPacketTime[i] = approx_time;
					continue;
				}
				if ((approx_time - LastPacketTime[i]) > NETWORK_TIMEOUT)
					network_timeout_player(i);
			}

//added on 11/18/98 by Victor Rachels to hack ghost disconnects
                        if(Players[i].connected != 1 && Objects[Players[i].objnum].type != OBJ_GHOST)
                         {
                          hud_message(MSGC_MULTI_INFO, "\002%c%s\004 has left.", 
								gr_getcolor(player_rgb[i].r,player_rgb[i].g,player_rgb[i].b)+1,
								  Players[i].callsign);
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
	network_listen();

	if (Network_send_objects)
		network_send_objects();
}

int missed_packets = 0;

void network_consistency_error(void)
{
	static int count = 0;

	if (count++ < 10)
		return;

	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
	Function_mode = FMODE_GAME;
	count = 0;
	multi_quit_game = 1;
	multi_leave_menu = 1;
	multi_reset_stuff();
}

void network_read_pdata_packet(ubyte *data, int short_packet)
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;
	frame_info *pd;

#ifdef NATIVE_PACKETS
	pd = (frame_info *)data; // doesn't work with short_pos
#else
	frame_info frame_data;
	pd = &frame_data;
	receive_frameinfo_packet(data, &frame_data, short_packet);
#endif
//    printf("[%i,%i]",Player_num,pd->playernum);//#####
	if ((TheirPlayernum = pd->playernum) < 0) {
		Int3(); // This packet is bogus!!
		return;
	}
#ifdef SHAREWARE
        TheirObjnum = pd->objnum;
#else
	TheirObjnum = Players[TheirPlayernum].objnum;
#endif
        if (!multi_quit_game && (TheirPlayernum >= N_players)) {
		Int3(); // We missed an important packet!
		network_consistency_error();
		return;
	}
	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL) )	{
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		if ( pd->data_size>0 )	{
			// pass pd->data to some parser function....
			multi_process_bigdata( pd->data, pd->data_size );
		}
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}
//	mprintf((0, "Gametime = %d, Frametime = %d.\n", GameTime, FrameTime));

	if ((sbyte)pd->level_num != Current_level_num)
	{
		mprintf((0, "Got frame packet from player %d wrong level %d!\n", pd->playernum, pd->level_num));
		return;
	}

	TheirObj = &Objects[TheirObjnum];

	//------------- Keep track of missed packets -----------------
	Players[TheirPlayernum].n_packets_got++;
	LastPacketTime[TheirPlayernum] = timer_get_approx_seconds();
//edited 03/04/99 Matt Mueller - short_packet type 1 is the type without missed_packets
	if  ( short_packet != 1 && pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
//end edit -MM
		//mprintf((0,"packet count: remote says %d, we say %d\n", pd->numpackets, Players[TheirPlayernum].n_packets_got));
		missed_packets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

		if ( missed_packets > 0 )	
			mprintf( (0, "Missed %d packets from player #%d (%d total)\n", pd->numpackets-Players[TheirPlayernum].n_packets_got, TheirPlayernum, missed_packets ));
		else
			mprintf( (0, "Got %d late packets from player #%d (%d total)\n", Players[TheirPlayernum].n_packets_got-pd->numpackets, TheirPlayernum, missed_packets ));

		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------
	if (short_packet == 1) {
		extract_shortpos(TheirObj, (shortpos *)(data + 4));
	} else if (short_packet == 2) {
		extract_shortpos(TheirObj, (shortpos *)(data + 2));
	} else {
		TheirObj->pos				= pd->obj_pos;
		TheirObj->orient			= pd->obj_orient;
#ifdef SHAREWARE
		TheirObj->mtype.phys_info		= pd->obj_phys_info;
#else
		TheirObj->mtype.phys_info.velocity = pd->phys_velocity;
		TheirObj->mtype.phys_info.rotvel = pd->phys_rotvel;
#endif
#if 0 // adb: why this? screws up ignore command
		if ((TheirObj->render_type != pd->obj_render_type) && (pd->obj_render_type == RT_POLYOBJ))
			multi_make_ghost_player(TheirPlayernum);
#endif

		obj_relink(TheirObjnum,pd->obj_segnum);

       }
       if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);

	//------------ Welcome them back if reconnecting --------------
	if (!Players[TheirPlayernum].connected)	{
		// Begin addition by GF
		//Check to see if we have this player on ignore.
		//hud_message(MSGC_DEBUG, "Rejoining Player = %s",Players[TheirPlayernum].callsign);
		if (Network_enable_ignore_ghost &&
		    checkignore(Players[TheirPlayernum].callsign))
		{
			//network_disconnect_player(TheirPlayernum);
			//hud_message(MSGC_DEBUG, "#2 ignore...");
			return; //Player is on ignore.
		}
		// End addition by GF

//added/changed on 8/6/98 by Matt Mueller
//added on 8/5/98 by Matt Mueller to make sure someone doesn't accidentally get short packets
//		  NetWantShort[TheirPlayernum]=0;
	       //changed on 980815 by adb: don't remove version info for non shp games
	       //-killed memset( &Net_D1xPlayer[TheirPlayernum], 0, sizeof(network_d1xplayer_info) );
	       Net_D1xPlayer[TheirPlayernum].shp = 0;
	       //end edit - adb
//end edit - Matt Mueller
//end edit - Matt Mueller

		Players[TheirPlayernum].connected = 1;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);
#endif

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
		hud_message( MSGC_MULTI_INFO, "'\002%c%s\004' %s",
				gr_getcolor(player_rgb[TheirPlayernum].r,player_rgb[TheirPlayernum].g,player_rgb[TheirPlayernum].b)+1,
				Players[TheirPlayernum].callsign, TXT_REJOIN );

#ifndef SHAREWARE
		multi_send_score();
#endif
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )	{
		// pass pd->data to some parser function....
		multi_process_bigdata( pd->data, pd->data_size );
	}
//	mprintf( (0, "Got packet with %d bytes on it!\n", pd->data_size ));

}

// get player number of master
// (the lowest numbered connected player)
int network_whois_master() {
	int i;
	for (i = 0; i < Netgame.numplayers; i++)
		if (Netgame.players[i].connected)
			return i;
	Error("No players in netgame");
}

#if 0 // currently not used
// compare two players
// players are considered equal if they have the same callsign and
// the same network address
// returns 1 if equal, 0 if not.
int network_compare_players(netplayer_info *pl1, netplayer_info *pl2) {
	return ((strcasecmp(pl1->callsign, pl2->callsign) == 0) &&
		(memcmp(pl1->node, pl2->node, 6) == 0) &&
		(memcmp(pl1->server, pl2->server, 4) == 0));
}
#endif

#endif
