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

#ifdef NETWORK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strutil.h"
#include "pstypes.h"
#include "args.h"
#include "timer.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "error.h"
#include "netpkt.h"
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
#include "vers_id.h"
#include "gamefont.h"
#include "playsave.h"
#include "songs.h"

void network_send_rejoin_sync(int player_num);
void network_update_netgame(void);
void network_send_endlevel_sub(int player_num);
void network_send_endlevel_packet(void);
void network_read_endlevel_packet( ubyte *data );
void network_read_object_packet( ubyte *data );
void network_read_sync_packet( ubyte * sp, int d1x );
void network_flush();
void network_listen();
void network_ping(ubyte flag, int pnum);
void network_process_pdata(char *data);
void network_read_pdata_packet(ubyte *data);
void network_read_pdata_short_packet(short_frame_info *pd);
int network_compare_players(netplayer_info *pl1, netplayer_info *pl2);
void DoRefuseStuff(sequence_packet *their);
int show_game_stats(int choice);

netgame_info Active_games[MAX_ACTIVE_NETGAMES];
int num_active_games = 0;

int	Network_debug=0;
int	Network_active=0;

int  	Network_status = 0;
int 	Network_games_changed = 0;

int	Network_short_packets = 0;

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

int restrict_mode = 0;

int IPX_Socket=0;
int     Network_allow_socket_changes = 1;

sequence_packet My_Seq;

extern obj_position Player_init[MAX_PLAYERS];
extern ubyte SurfingNet;

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
	memcpy(My_Seq.player.node, NetDrvGetMyLocalAddress(), 6);
	memcpy(My_Seq.player.server, NetDrvGetMyServerAddress(), 4 );
	#ifndef SHAREWARE
	My_Seq.player.sub_protocol = MULTI_PROTO_D1X_MINOR;
	#endif

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();

	Player_num = save_pnum;		
	multi_new_game();
	Network_new_game = 1;
	Fuelcen_control_center_destroyed = 0;
	network_flush();

	Netgame.packets_per_sec = 10;
}

// Change socket to new_socket, returns 1 if really changed
int network_change_socket(int new_socket)
{
        if ( new_socket+IPX_DEFAULT_SOCKET > 0x8000 )
                new_socket  = 0x8000 - IPX_DEFAULT_SOCKET;
        if ( new_socket+IPX_DEFAULT_SOCKET < 0 )
                new_socket  = IPX_DEFAULT_SOCKET;
        if (new_socket != IPX_Socket) {
                IPX_Socket = new_socket;
                network_listen();
                NetDrvChangeDefaultSocket( IPX_DEFAULT_SOCKET + IPX_Socket );
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
		}
		if (Players[i].connected == 1)
		{
			// Check timeout for idle players
			if (timer_get_approx_seconds() > LastPacketTime[i]+ENDLEVEL_IDLE_TIME)
			{
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
		}
		else
		{
			sprintf(menus[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Fuelcen_seconds_left, TXT_SECONDS);
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

	choice=newmenu_do3(NULL, text, N_players+1, m, network_endlevel_poll, 0, STARS_BACKGROUND, 300*(SWIDTH/320), 160*(SHEIGHT/200));

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

	if (playernum==0 && NetDrvType() == NETPROTO_UDP) // Host has left - Quit game!
	{
		Function_mode = FMODE_MENU;
		nm_messagebox(NULL, 1, TXT_OK, "Game was closed by host!");
		Function_mode = FMODE_GAME;
		multi_quit_game = 1;
		multi_leave_menu = 1;
		multi_reset_stuff();
		Function_mode = FMODE_MENU;
	}
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
		NetDrvGetLocalTarget( their->player.server, their->player.node, Players[pnum].net_address );
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

	hud_message(MSGC_MULTI_INFO, "'%s' %s",their->player.callsign, TXT_JOINING);
	
	multi_make_ghost_player(pnum);

#ifndef SHAREWARE
	multi_send_score();
#endif
}

char RefuseThisPlayer=0,WaitForRefuseAnswer=0;
char RefusePlayerName[12];
fix RefuseTimeLimit=0;

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
		network_dump_player(their->player.server, their->player.node, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	memset(&Network_player_rejoining, 0, sizeof(sequence_packet));
	Network_player_added = 0;

#ifndef SHAREWARE
	if ( (*(uint *)their->player.server) != 0 )
		NetDrvGetLocalTarget( their->player.server, their->player.node, local_address );
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
			return;
		}

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);
#endif

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		hud_message(MSGC_MULTI_INFO, "'%s' %s", Players[player_num].callsign, TXT_REJOIN);
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
				}
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
#endif

void network_stop_resync(sequence_packet *their)
{
	if ( (!memcmp(Network_player_rejoining.player.node, their->player.node, 6)) &&
		  (!memcmp(Network_player_rejoining.player.server, their->player.server, 4)) &&
	     (!strcasecmp(Network_player_rejoining.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_send_objnum = -1;
	}
}

ubyte object_buffer[MAX_DATA_SIZE];

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
			swap_object((object *)&object_buffer[loc]);
		}

		if (obj_count_frame) // Send any objects we've buffered
		{
			frame_num++;
	
			Network_send_objnum = i;
			object_buffer[1] = obj_count_frame;
			object_buffer[2] = frame_num;

			Assert(loc <= MAX_DATA_SIZE);

			NetDrvSendInternetworkPacketData( object_buffer, loc, Network_player_rejoining.player.server, Network_player_rejoining.player.node );
			// OLD NetDrvSendPacketData(object_buffer, loc, &Network_player_rejoining.player.node);
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
				//OLD NetDrvSendPacketData(object_buffer, 8, &Network_player_rejoining.player.node);
				NetDrvSendInternetworkPacketData(object_buffer, 8, Network_player_rejoining.player.server, Network_player_rejoining.player.node);
			
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
				send_sequence_packet( Network_player_rejoining, Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address);
			}
		}
	}

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

	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );
	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );
	send_netgame_packet(Network_player_rejoining.player.server, Network_player_rejoining.player.node );

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

	for (i=0; i<N_players; i++ )	{
		if ( !memcmp( Netgame.players[i].node, p->player.node, 6) && !memcmp(Netgame.players[i].server, p->player.server, 4)){
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )	{
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

	My_Seq.type = PID_DUMP;
	My_Seq.player.connected = why;

	send_sequence_packet( My_Seq, server, node, NULL);
}

void
network_send_game_list_request(void)
{
	// Send a broadcast request for game info

	sequence_packet me;

	memset(&me, 0, sizeof(sequence_packet));
	memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	memcpy( me.player.node, NetDrvGetMyLocalAddress(), 6 );
	memcpy( me.player.server, NetDrvGetMyServerAddress(), 4 );
	me.type = PID_GAME_LIST;
	
	send_sequence_packet( me, NULL, NULL, NULL);
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

	if (Players[player_num].connected == 1) // Still playing
	{
		Assert(Fuelcen_control_center_destroyed);
	 	end.seconds_left = Fuelcen_seconds_left;
	}
	//added 05/18/99 Matt Mueller - similarly, its not used if we aren't connected, but checker complains.
	else
		end.seconds_left = 127;
	//end addition -MM

	for (i = 0; i < N_players; i++)
	{	
		if ((i != Player_num) && (i!=player_num) && (Players[i].connected))
		{
			NetDrvSendPacketData((ubyte *)&end, sizeof(endlevel_info), Netgame.players[i].server, Netgame.players[i].node, Players[i].net_address);
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

	network_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.type;
	old_status = Netgame.game_status;

	Netgame.type = (Netgame.protocol_version == MULTI_PROTO_D1X_VER ?
		(light ? PID_D1X_GAME_LITE : PID_D1X_GAME_INFO)
		: PID_GAME_INFO);
	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

	if (!their)
		send_netgame_packet(NULL, NULL);
	else
		send_netgame_packet(their->player.server, their->player.node);

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

	My_Seq.type = (Netgame.protocol_version == MULTI_PROTO_D1X_VER)
		? PID_D1X_REQUEST : PID_REQUEST;
	My_Seq.player.connected = Current_level_num;

	send_sequence_packet(My_Seq, Netgame.players[i].server, Netgame.players[i].node, NULL);

	return i;
}
	
void network_process_gameinfo(ubyte *data, int d1x)
{
	int i, j;
	netgame_info *new;

	netgame_info netgame;
	new = &netgame;
	receive_netgame_packet(data, &netgame, d1x);

	Network_games_changed = 1;

	for (i = 0; i < num_active_games; i++)
		if (!strcasecmp(Active_games[i].game_name, new->game_name) &&
		    (Active_games[i].type == PID_D1X_GAME_LITE ||
		     new->type == PID_D1X_GAME_LITE ||
		     (!memcmp(Active_games[i].players[0].node, new->players[0].node, 6) &&
		      !memcmp(Active_games[i].players[0].server, new->players[0].server, 4))))
			break;

	if (i == MAX_ACTIVE_NETGAMES)
	{
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

	// Begin addition by GF
        if (Network_status == NETSTAT_PLAYING)
	{
          Function_mode = FMODE_MENU;
          nm_messagebox(NULL, 1, TXT_OK, "You have been kicked from the game!");
          multi_quit_game = 1;
          multi_leave_menu = 1;
          multi_reset_stuff();

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

	for (i = 0; i < N_players; i++)
		if (!memcmp(their->player.server, Netgame.players[i].server, 4) && !memcmp(their->player.node, Netgame.players[i].node, 6) && (!strcasecmp(their->player.callsign, Netgame.players[i].callsign))) {
			Players[i].connected = 1;
			break;
		}
}

void network_process_packet(ubyte *data, int length )
{
	sequence_packet *their = (sequence_packet *)data;

#ifdef WORDS_BIGENDIAN
	sequence_packet tmp_packet;

	memset(&tmp_packet, 0, sizeof(sequence_packet));

	receive_sequence_packet(data, &tmp_packet);
	their = &tmp_packet;                                            // reassign their to point to correctly alinged structure
#endif

	switch( their->type )	{
	
	case PID_GAME_INFO:
                if (Network_status == NETSTAT_BROWSING)
                          network_process_gameinfo(data, 0);
		break;
	case PID_GAME_LIST:
		// Someone wants a list of games
		if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
			if (network_i_am_master())
				network_send_game_info(their, 1);
		break;
	case PID_ADDPLAYER:
		network_new_player(their);
		break;
	case PID_D1X_REQUEST:
	case PID_REQUEST:
		if (Network_status == NETSTAT_STARTING)
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
			if (restrict_mode)
				DoRefuseStuff (their);
			else
				network_welcome_player(their);
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
			if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING)) { 
				network_process_pdata((char *)data);
			}
			break;
        case PID_OBJECT_DATA:
		if (Network_status == NETSTAT_WAITING) 
			network_read_object_packet(data);
		break;
	case PID_ENDLEVEL:
		if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
			network_read_endlevel_packet(data);
		break;
	// D1X packets
        case PID_D1X_GAME_INFO_REQ:
		// Someone wants full info of our D1X game
		if ((Netgame.protocol_version == MULTI_PROTO_D1X_VER) &&
		    ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL)))
			if (network_i_am_master())
				network_send_game_info(their, 0);
		break;
	case PID_D1X_GAME_LITE_REQ:
                // Someone wants a list of D1X games
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
//added 03/04/99 Matt Mueller - new direct data packets..
	  case PID_DIRECTDATA:
		if ((Game_mode&GM_NETWORK) && ((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) )) { 
		    multi_process_bigdata( (char *)data+1, length-1);
		}	    
	    break;
//end addition -MM
	case PID_PING_SEND:
			network_ping (PID_PING_RETURN,data[1]);
			break;
	case PID_PING_RETURN:
			network_handle_ping_return(data[1]);  // data[1] is player who told us of their ping time
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
network_read_endlevel_packet( ubyte *data )
{
	// Special packet for end of level syncing

	int playernum;
	endlevel_info *end = (endlevel_info *)data;
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

	if ((Network_status == NETSTAT_PLAYING) && (end->connected != 0))
		return; // Only accept disconnect packets if we're not out of the level yet

	Players[playernum].connected = end->connected;
	memcpy(&kill_matrix[playernum][0], end->kill_matrix, MAX_PLAYERS*sizeof(short));
	Players[playernum].net_kills_total = end->kills;
	Players[playernum].net_killed_total = end->killed;

	if ((Players[playernum].connected == 1) && (end->seconds_left < Fuelcen_seconds_left))
		Fuelcen_seconds_left = end->seconds_left;

	LastPacketTime[playernum] = timer_get_approx_seconds();
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
				network_pack_objects();
				mode = 0;
			}
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
				memcpy(obj,data+loc,sizeof(object));
				swap_object(obj);
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

	if (Network_status != NETSTAT_MENU && !Network_rejoined && (timer_get_approx_seconds() > t1+F1_0*2))
	{
		int i;

		// Poll time expired, re-send request
		
		t1 = timer_get_approx_seconds();

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
	network_listen();

	if (n < Netgame.numplayers ) 	
	{
		sprintf( menus[N_players-1].text, "%d. %-16s", N_players, Netgame.players[N_players-1].callsign );

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
		}
		for (i=N_players; i<n; i++ )	
		{
			sprintf( menus[i].text, "%d. ", i+1 );		// Clear out the deleted entries...
			menus[i].value = 0;
		}
   }
}

void network_set_power (void)
{
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	int i;

	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++) {
                m[i].type = NM_TYPE_CHECK; m[i].text = multi_allow_powerup_text[i]; m[i].value = (Netgame.flags >> i) & 1;
	}
	i = newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, NULL, 0 );
       if (i > -1) {
               Netgame.flags &= ~NETFLAG_DOPOWERUP;
		for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
                        if (m[i].value)
                                Netgame.flags |= (1 << i);
       }
}

static int opt_cinvul, opt_team_anarchy, opt_coop, opt_show_on_map, opt_closed, opt_refuse, opt_maxnet;
static int opt_setpower,opt_show_on_map, opt_difficulty,opt_packets,opt_short_packets, opt_socket;
static int last_maxnet, last_cinvul=0;

void network_more_options_poll( int nitems, newmenu_item * menus, int * key, int citem );

void network_more_game_options ()
{
	int opt=0,i;
	int opt_protover=-1;
	char srinvul[50],packstring[5],socket_string[5];
	newmenu_item m[21];

	sprintf (socket_string,"%d",IPX_Socket);
	sprintf (packstring,"%d",Netgame.packets_per_sec);
	
	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;

	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.control_invul_time/5/F1_0/60; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_show_on_map=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP); opt_show_on_map=opt; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Packets per second (2 - 20)"; opt++;
	opt_packets=opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text=packstring; m[opt].text_len=2; opt++;

	if (NetDrvType() != NETPROTO_UDP)
	{
		opt_protover=opt;
		m[opt].type = NM_TYPE_CHECK; m[opt].text = "VERSION CHECK"; m[opt].value=(Netgame.protocol_version == MULTI_PROTO_D1X_VER); opt++;
		m[opt].type = NM_TYPE_TEXT; m[opt].text = "Options below need version check"; opt++;
		m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network socket"; opt++;
		opt_socket = opt;
		m[opt].type = NM_TYPE_INPUT; m[opt].text = socket_string; m[opt].text_len=4; opt++;
	}
	opt_short_packets=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Short packets"; m[opt].value=(Netgame.flags & NETFLAG_SHORTPACKETS); opt++;
	opt_setpower = opt;
	m[opt].type = NM_TYPE_MENU; m[opt].text = "Set objects allowed..."; opt++;

menu:
	i = newmenu_do1( NULL, "Advanced netgame options", opt, m, network_more_options_poll, 0 );

	Netgame.control_invul_time = m[opt_cinvul].value*5*F1_0*60;

	if (i==opt_setpower)
	{
		network_set_power ();
		goto menu;
	}

	Netgame.packets_per_sec=atoi(packstring);
	
	if (Netgame.packets_per_sec>20)
	{
		Netgame.packets_per_sec=20;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 20");
	}

	if (Netgame.packets_per_sec<2)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 2");
		Netgame.packets_per_sec=2;
	}

	if (NetDrvType() != NETPROTO_UDP)
	{
		if (m[opt_protover].value)
			Netgame.protocol_version = MULTI_PROTO_D1X_VER;
		else
			Netgame.protocol_version = MULTI_PROTO_VERSION;

		if ((atoi(socket_string))!=IPX_Socket)
		{
			IPX_Socket=atoi(socket_string);
			NetDrvChangeDefaultSocket( IPX_DEFAULT_SOCKET + IPX_Socket );
		}
	}
	
	if (m[opt_short_packets].value && Netgame.protocol_version == MULTI_PROTO_D1X_VER)
	{
		Network_short_packets = 1;
		Netgame.flags |= NETFLAG_SHORTPACKETS;
	}
	else
	{
		Network_short_packets = 0;
		Netgame.flags &= ~NETFLAG_SHORTPACKETS;
	}

	Netgame.difficulty=Difficulty_level = m[opt_difficulty].value;
	if (m[opt_show_on_map].value)
		Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
	else
		Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
}

void network_more_options_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	menus = menus;
	citem = citem;      // kills compile warnings
	nitems = nitems;
	key = key;

	if ( last_cinvul != menus[opt_cinvul].value )   {
		sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
		last_cinvul = menus[opt_cinvul].value;
	}
}

void network_game_param_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
	static int oldmaxnet=0;

	if (menus[opt_team_anarchy].value) 
	{
		menus[opt_closed].value = 1;
		menus[opt_closed-1].value = 0;
		menus[opt_closed+1].value = 0;
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

		if (!Netgame.game_flags & NETGAME_FLAG_SHOW_MAP) 
			Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;

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
}

int network_get_game_params()
{
	int i;
	int opt, opt_name, opt_level, opt_mode,opt_moreopts;
	newmenu_item m[20];
	char slevel[5];
	char level_text[32];
	char srmaxnet[50];

	for (i=0;i<MAX_PLAYERS;i++)
		if (i!=Player_num)
			Players[i].callsign[0]=0;

	restrict_mode=0;
	MaxNumNetPlayers = MAX_NUM_NET_PLAYERS;
	sprintf( Netgame.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.max_numplayers=MaxNumNetPlayers;
	Netgame.protocol_version = MULTI_PROTO_VERSION;
	Netgame.flags = NETFLAG_DOPOWERUP; // enable all powerups
	Netgame.protocol_version = MULTI_PROTO_D1X_VER;

	if (GameArg.MplGameProfile)
	{
		if (!nm_messagebox(NULL, 2, TXT_YES,TXT_NO, "do you want to load\na multiplayer profile?"))
		{
			char mprofile_file[13]="";
			if (newmenu_get_filename("Select profile\n<ESC> to abort", ".mpx", mprofile_file, 1))
			{
				PHYSFS_file *outfile;
		
				outfile = PHYSFSX_openReadBuffered(mprofile_file);
				PHYSFS_read(outfile,&Netgame,sizeof(netgame_info),1);
				PHYSFS_close(outfile);
			}
		}
	}

#ifndef SHAREWARE
	if (!select_mission(1, TXT_MULTI_MISSION))
		return -1;

	strcpy(Netgame.mission_name, Current_mission_filename);
	strcpy(Netgame.mission_title, Current_mission_longname);
#else
	strcpy(Netgame.mission_name, "");
	strcpy(Netgame.mission_title, "");
#endif

	sprintf( slevel, "1" );

	opt = 0;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_DESCRIPTION; opt++;

	opt_name = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = Netgame.game_name; m[opt].text_len = NETGAME_NAME_LEN; opt++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
	if (Last_secret_level < -1)
		sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
	else if (Last_secret_level == -1)
		sprintf(level_text+strlen(level_text)-1, ", S1)");

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

	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Open game"; m[opt].group=1; m[opt].value=(!restrict_mode && !Netgame.game_flags & NETGAME_FLAG_CLOSED); opt++;
	opt_closed = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_CLOSED_GAME; m[opt].group=1; m[opt].value=Netgame.game_flags & NETGAME_FLAG_CLOSED; opt++;
	opt_refuse = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Restricted Game              "; m[opt].group=1; m[opt].value=restrict_mode; opt++;

	opt_maxnet = opt;
	sprintf( srmaxnet, "Maximum players: %d", MaxNumNetPlayers);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.max_numplayers-2; m[opt].text= srmaxnet; m[opt].min_value=0; 
	m[opt].max_value=MaxNumNetPlayers-2; opt++;
	last_maxnet=MaxNumNetPlayers-2;
	
	opt_moreopts=opt;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Advanced options"; opt++;

	Assert(opt <= 20);

menu:
	i = newmenu_do1( NULL, NULL, opt, m, network_game_param_poll, 1 );

	if (i==opt_moreopts)
	{
		if ( m[opt_mode+3].value )
			Game_mode=GM_MULTI_COOP;
		network_more_game_options();
		Game_mode=0;
		goto menu;
	}
	restrict_mode=m[opt_refuse].value;

	if ( i > -1 )   {
		int j;

		MaxNumNetPlayers = m[opt_maxnet].value+2;
		Netgame.max_numplayers=MaxNumNetPlayers;

		for (j = 0; j < num_active_games; j++)
			if (!stricmp(Active_games[j].game_name, Netgame.game_name))
			{
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DUPLICATE_NAME);
				goto menu;
			}

		Netgame.levelnum = atoi(slevel);

		if (!strnicmp(slevel, "s", 1))
			Netgame.levelnum = -atoi(slevel+1);
		else
			Netgame.levelnum = atoi(slevel);

		if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
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
// 		else if (ANARCHY_ONLY_MISSION) {
// 			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
// 			m[opt_mode+2].value = 0;
// 			m[opt_mode+3].value = 0;
// 			m[opt_mode].value = 1;
// 			goto menu;
// 		}
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
}

int
network_find_game(void)
{
	// Find out whether or not there is space left on this socket

	fix t1;

	Network_status = NETSTAT_BROWSING;

	num_active_games = 0;

	show_boxed_message(TXT_WAIT, 0);

	network_send_game_list_request();
	t1 = timer_get_approx_seconds() + F1_0*2;

	while (timer_get_approx_seconds() < t1) // Wait 3 seconds for replies
		network_listen();

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

	if (data)
	{
		receive_netgame_packet( data, &Netgame, d1x );
	}

	N_players = sp->numplayers;
	Difficulty_level = sp->difficulty;
	Network_status = sp->game_status;

	Assert(Function_mode != FMODE_GAME);

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
		if ((!memcmp( sp->players[i].node, My_Seq.player.node, 6 )) &&
                         (!strcasecmp( sp->players[i].callsign, temp_callsign)) )
		{
			Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
			change_playernum_to(i);
		}
		memcpy( Players[i].callsign, sp->players[i].callsign, CALLSIGN_LEN+1 );

#ifndef SHAREWARE
#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy(&server, TempPlayersInfo->players[i].server, 4);
		if (server != 0)
			NetDrvGetLocalTarget((ubyte *)&server, sp->players[i].node, Players[i].net_address);
#else // WORDS_NEED_ALIGNMENT
		if ( (*(uint *)sp->players[i].server) != 0 )
			NetDrvGetLocalTarget( sp->players[i].server, sp->players[i].node, Players[i].net_address );
#endif // WORDS_NEED_ALIGNMENT
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
        PlayerCfg.NetlifeKills -= Players[Player_num].net_kills_total;
        PlayerCfg.NetlifeKilled -= Players[Player_num].net_killed_total;
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
	} else {
		if (data) { // adb: master does have this info
			Network_short_packets = 0;
			Network_pps = 10;
		}
		multi_allow_powerup = NETFLAG_DOPOWERUP;
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

		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
		send_netgame_packet(Netgame.players[i].server, Netgame.players[i].node);
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
	int i;

	Assert( sizeof(frame_info) < MAX_DATA_SIZE );

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

	i = network_get_game_params();

	if (i<0) return;

	if (IPX_Socket) {
		NetDrvChangeDefaultSocket( IPX_DEFAULT_SOCKET + IPX_Socket );
	}

	N_players = 0;

        Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	network_set_game_mode(Netgame.gamemode);
	MaxNumNetPlayers = Netgame.max_numplayers;

	if (GameArg.MplGameProfile)
	{
		char mprofile_file[13]="";
		newmenu_item m[1];
		m[0].type=NM_TYPE_INPUT; m[0].text_len = 8; m[0].text = mprofile_file;
		if (!newmenu_do( NULL, "save profile as", 1, &(m[0]), NULL))
		{
			PHYSFS_file *infile;
			strcat(mprofile_file,".mpx");
			infile = PHYSFSX_openWriteBuffered(mprofile_file);
			PHYSFS_write(infile,&Netgame,sizeof(netgame_info),1);
			PHYSFS_close(infile);
		}
	}

	Network_status = NETSTAT_STARTING;

	//adb: the following will be overwritten for D1X only games
#ifndef SHAREWARE
	Network_pps = Netgame.packets_per_sec;
	Network_short_packets = (Netgame.flags & NETFLAG_SHORTPACKETS) ? 2 : 0;
#endif

	if(network_select_players())
	{
		StartNewLevel(Netgame.levelnum);
        }
	else
		Game_mode = GM_GAME_OVER;
	
}

void restart_net_searching(newmenu_item * m)
{
	int i;
	N_players = 0;
	num_active_games = 0;

	memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++)
	{
		sprintf(m[i+2].text, "%d.                                                         ",i+1);
	}

	Network_games_changed = 1;	
}

void network_join_poll( int nitems, newmenu_item * menus, int * key, int citem )
{
#ifdef todel
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int i, new_socket;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	if (Network_allow_socket_changes )	{
		new_socket = IPX_Socket;
	
		if ( *key==KEY_PAGEUP ) 	{ new_socket--; *key = 0; }
		if ( *key==KEY_PAGEDOWN )	{ new_socket++; *key = 0; }
	
		if ( network_change_socket(new_socket) )	 {
			sprintf( menus[0].text, "%s %+d", TXT_CURRENT_IPX_SOCKET, IPX_Socket );
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
	}

	for (i = num_active_games; i < MAX_ACTIVE_NETGAMES; i++)
	{
		sprintf(menus[(2*i)+1].text, "%d.                                       ", i+1);
		sprintf(menus[(2*i)+2].text, " \n");
	}
#endif
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int i, osocket,join_status,temp;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	if ( Network_allow_socket_changes ) {
		osocket = IPX_Socket;

		if ( *key==KEY_PAGEDOWN )       { IPX_Socket--; *key = 0; }
		if ( *key==KEY_PAGEUP )         { IPX_Socket++; *key = 0; }

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
			network_listen();
			NetDrvChangeDefaultSocket( IPX_DEFAULT_SOCKET + IPX_Socket );
			restart_net_searching(menus);
			network_send_game_list_request();
			return;
		}
	}

	// send a request for game info every 3 seconds
	if (timer_get_approx_seconds() > t1+F1_0*3) {
		t1 = timer_get_approx_seconds();
		network_send_game_list_request();
	}

	temp=num_active_games;

	network_listen();

	if (!Network_games_changed)
		return;

	if (temp!=num_active_games)
		digi_play_sample (SOUND_HUD_MESSAGE,F1_0);

	Network_games_changed = 0;

	// Copy the active games data into the menu options
	for (i = 0; i < num_active_games; i++)
	{
		int game_status = Active_games[i].game_status;
		int j,x, k,tx,ty,ta,nplayers = 0;
		char levelname[8],MissName[25],GameName[25],thold[2];
		thold[1]=0;

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_games[i].mission_title[j]=='\t')
				continue;
			thold[0]=Active_games[i].mission_title[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				MissName[k]=MissName[k+1]=MissName[k+2]='.';
				k+=3;
				break;
			}

			MissName[k++]=Active_games[i].mission_title[j];
		}
		MissName[k]=0;

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_games[i].game_name[j]=='\t')
				continue;
			thold[0]=Active_games[i].game_name[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				GameName[k]=GameName[k+1]=GameName[k+2]='.';
				k+=3;
				break;
			}
			GameName[k++]=Active_games[i].game_name[j];
		}
		GameName[k]=0;


		for (j = 0; j < Active_games[i].numplayers; j++)
			if (Active_games[i].players[j].connected)
				nplayers++;

		if (Active_games[i].levelnum < 0)
			sprintf(levelname, "S%d", -Active_games[i].levelnum);
		else
			sprintf(levelname, "%d", Active_games[i].levelnum);

		if (game_status == NETSTAT_STARTING)
		{
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
					 Active_games[i].max_numplayers,MissName,levelname,"Forming");
		}
		else if (game_status == NETSTAT_PLAYING)
		{
			join_status=can_join_netgame(&Active_games[i]);

			if (join_status==1)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
						 Active_games[i].max_numplayers,MissName,levelname,"Open");
			else if (join_status==2)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
						 Active_games[i].max_numplayers,MissName,levelname,"Full");
			else if (join_status==3)
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
						 Active_games[i].max_numplayers,MissName,levelname,"Restrict");
			else
				sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
						 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
						 Active_games[i].max_numplayers,MissName,levelname,"Closed");

		}
		else
			sprintf (menus[i+2].text,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",
					 i+1,GameName,MODE_NAMES(Active_games[i].gamemode),nplayers,
					 Active_games[i].max_numplayers,MissName,levelname,"Between");


		Assert(strlen(menus[i+2].text) < 100);
	}

	for (i = num_active_games; i < MAX_ACTIVE_NETGAMES; i++)
	{
		sprintf(menus[i+2].text, "%d.                                                     ",i+1);
	}
}

int
network_wait_for_sync(void)
{
	char text[60];
	newmenu_item m[2];
	int i, choice=0;
	
	Network_status = NETSTAT_WAITING;
	m[0].type=NM_TYPE_TEXT; m[0].text = text;
	m[1].type=NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;
	
	i = network_send_request();

	if (i < 0)
		return(-1);

	sprintf( m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER );

	while (choice > -1)
		choice=newmenu_do( NULL, TXT_WAIT, 2, m, network_sync_poll );


	if (Network_status != NETSTAT_PLAYING)	
	{
		sequence_packet me;

		memset(&me, 0, sizeof(sequence_packet));
		me.type = PID_QUIT_JOINING;
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		memcpy( me.player.node, NetDrvGetMyLocalAddress(), 6 );
		memcpy( me.player.server, NetDrvGetMyServerAddress(), 4 );
		send_sequence_packet( me, Netgame.players[0].server, Netgame.players[0].node, NULL );
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

void nm_draw_background1(char * filename);

int network_do_join_game(int choice)
{
	if (Active_games[choice].game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	if ((Active_games[choice].protocol_version != MULTI_PROTO_VERSION) &&
	    (Active_games[choice].protocol_version != MULTI_PROTO_D1X_VER))
	{
                nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		return 0;
	}

#ifndef SHAREWARE
	if (Active_games[choice].protocol_version == MULTI_PROTO_D1X_VER &&
	    Active_games[choice].required_subprotocol > MULTI_PROTO_D1X_MINOR)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, "This game uses features\nnot present in this version.");
		return 0;
	}

	if (!load_mission_by_name(Active_games[choice].mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}
#endif

	if (!can_join_netgame(&Active_games[choice]))
	{
		if (Active_games[choice].numplayers == Active_games[choice].max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return 0;
	}

	// Choice is valid, prepare to join in
	memcpy(&Netgame, &Active_games[choice], sizeof(netgame_info));

	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	network_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);

	return 1;     // look ma, we're in a game!!!
}

void network_join_game()
{
	int choice, i;
	char menu_text[(MAX_ACTIVE_NETGAMES*2)+1][70];
	
	newmenu_item m[((MAX_ACTIVE_NETGAMES)*2)+1];

	if ( !Network_active )
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	network_init();

	N_players = 0;

	setjmp(LeaveGame);

	Network_send_objects = 0; 
	Network_rejoined=0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	network_flush();
	network_listen();  // Throw out old info

	network_send_game_list_request(); // broadcast a request for lists

	num_active_games = 0;

	memset(m, 0, sizeof(newmenu_item)*MAX_ACTIVE_NETGAMES);
	memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	m[0].text = menu_text[0];
	m[0].type = NM_TYPE_TEXT;
	if (Network_allow_socket_changes)
		sprintf( m[0].text, "\tCurrent IPX Socket is default %+d (PgUp/PgDn to change)", IPX_Socket );
	else
		strcpy( m[0].text, "" ); //sprintf( m[0].text, "" );

	m[1].text=menu_text[1];
	m[1].type=NM_TYPE_TEXT;
	sprintf (m[1].text,"\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		m[i+2].text = menu_text[i+2];
		m[i+2].type = NM_TYPE_MENU;
		sprintf(m[i+2].text, "%d.                                                                  ", i+1);
	}

	Network_games_changed = 1;
remenu:
	SurfingNet=1;
	nm_draw_background1(Menu_pcx_name);             //load this here so if we abort after loading level, we restore the palette
	gr_palette_load(gr_palette);
	choice=newmenu_dotiny("NETGAMES", NULL,MAX_ACTIVE_NETGAMES+2, m, network_join_poll);
	SurfingNet=0;

	if (choice==-1)	{
		Network_status = NETSTAT_MENU;
		return;	// they cancelled		
        }
	choice-=2;

	if (choice >=num_active_games)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
		goto remenu;
	}

	if (show_game_stats(choice)==0)
		goto remenu;

	// Choice has been made and looks legit
	if (network_do_join_game(choice)==0)
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
	ubyte packet[MAX_DATA_SIZE];

	if (!Network_active)
		return;

	while (NetDrvGetPacketData(packet) > 0)
		;
}

void network_listen()
{
	int size;
	ubyte packet[MAX_DATA_SIZE];

	if (!Network_active) return;

	size = NetDrvGetPacketData( packet );
	while ( size > 0 )	{
		network_process_packet( packet, size );
		size = NetDrvGetPacketData( packet );
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
			Int3();
		}
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

	hud_message(MSGC_MULTI_INFO, "%s %s", Players[playernum].callsign, TXT_DISCONNECTING);
	for (i = 0; i < N_players; i++)
		if (Players[i].connected) 
			n++;

	if (n == 1)
	{
//added/changed on 10/11/98 by Victor Rachels cuz this is annoying as a box
//-killed-                nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
          hud_message(MSGC_GAME_FEEDBACK, "You are the only person remaining in this netgame");
//end this section change - VR
	}
}

fix last_send_time = 0;
fix last_timeout_check = 0;

void network_do_frame(int force, int listen)
{
	int i;
	short_frame_info ShortSyncPack;

	if (!(Game_mode&GM_NETWORK)) return;

	if ((Network_status != NETSTAT_PLAYING) || (Endlevel_sequence)) // Don't send postion during escape sequence...
		goto listen;

	if (WaitForRefuseAnswer && timer_get_approx_seconds()>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	last_send_time += FrameTime;
	last_timeout_check += FrameTime;

	// Send out packet 10 times per second maximum... unless they fire, then send more often...
        if ( (last_send_time > (F1_0 / Network_pps)) ||
	     (Network_laser_fired) || force || PacketUrgent )	    {
		if ( Players[Player_num].connected )	{
			int objnum = Players[Player_num].objnum;
			PacketUrgent = 0;

			if (listen) {
#ifndef SHAREWARE
				multi_send_robot_frame(0);
#endif
                multi_send_fire(100);              // Do firing if needed..
			}

			last_send_time = 0;

			if (Netgame.flags & NETFLAG_SHORTPACKETS)
			{
				int i;

				memset(&ShortSyncPack,0,sizeof(short_frame_info));
				create_shortpos(&ShortSyncPack.thepos, Objects+objnum, 1);
				ShortSyncPack.type                                      = PID_PDATA;
				ShortSyncPack.playernum                         = Player_num;
				ShortSyncPack.obj_render_type           = Objects[objnum].render_type;
				ShortSyncPack.level_num                         = Current_level_num;
				ShortSyncPack.data_size                         = INTEL_SHORT(MySyncPack.data_size);
				memcpy (&ShortSyncPack.data[0],&MySyncPack.data[0],MySyncPack.data_size);

				MySyncPack.numpackets = INTEL_INT(Players[0].n_packets_sent++);
				ShortSyncPack.numpackets = MySyncPack.numpackets;
// 				NetDrvSendGamePacket((ubyte*)&ShortSyncPack, sizeof(short_frame_info) - NET_XDATA_SIZE + MySyncPack.data_size);
				for(i=0; i<N_players; i++) {
					if(Players[i].connected && (i != Player_num))
						NetDrvSendPacketData((ubyte*)&ShortSyncPack, sizeof(short_frame_info) - NET_XDATA_SIZE + MySyncPack.data_size, Netgame.players[i].server, Netgame.players[i].node,Players[i].net_address);
				}

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

// 				NetDrvSendGamePacket((ubyte*)&MySyncPack, sizeof(frame_info) - NET_XDATA_SIZE + send_data_size);
				for(i=0; i<N_players; i++)
				{
					if(Players[i].connected && (i != Player_num))
					{
						send_frameinfo_packet(&MySyncPack, Netgame.players[i].server, Netgame.players[i].node,Players[i].net_address);
					}
				}
			}
			
			
			
			
			MySyncPack.data_size = 0;		// Start data over at 0 length.
			if (Fuelcen_control_center_destroyed)
				network_send_endlevel_packet();
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
                          hud_message(MSGC_MULTI_INFO, "%s has left.", Players[i].callsign);
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

#if 0
void network_read_pdata_packet(ubyte *data, int short_packet)
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;
	frame_info *pd;

	frame_info frame_data;
	pd = &frame_data;
	receive_frameinfo_packet(data, &frame_data, short_packet);

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
			multi_process_bigdata( (char*)pd->data, pd->data_size );
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
	LastPacketTime[TheirPlayernum] = timer_get_approx_seconds();
//edited 03/04/99 Matt Mueller - short_packet type 1 is the type without missed_packets
	if  ( short_packet != 1 && pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
//end edit -MM
		missed_packets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------
	if (short_packet == 1) {
		extract_shortpos(TheirObj, (shortpos *)(data + 4),1);
	} else if (short_packet == 2) {
		extract_shortpos(TheirObj, (shortpos *)(data + 2),1);
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
		Players[TheirPlayernum].connected = 1;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);
#endif

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
		hud_message( MSGC_MULTI_INFO, "'%s' %s",Players[TheirPlayernum].callsign, TXT_REJOIN );

#ifndef SHAREWARE
		multi_send_score();
#endif
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )	{
		// pass pd->data to some parser function....
		multi_process_bigdata( (char*)pd->data, pd->data_size );
	}

}
#endif

void network_process_pdata (char *data)
 {
  Assert (Game_mode & GM_NETWORK);
 
  if (Netgame.flags & NETFLAG_SHORTPACKETS)
	network_read_pdata_short_packet ((short_frame_info *)data);
  else
	network_read_pdata_packet ((ubyte *) data);
 }

void network_read_pdata_packet(ubyte *data )
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;
	frame_info *pd;
	frame_info frame_data;

	pd = &frame_data;
	receive_frameinfo_packet(data, &frame_data);

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
			network_consistency_error();
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
	LastPacketTime[TheirPlayernum] = timer_get_approx_seconds();
	if  ( pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
		missed_packets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

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
		Players[TheirPlayernum].connected = 1;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
		
		HUD_init_message( "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );

		multi_send_score();
	}

	//------------ Parse the extra data at the end ---------------

	if ( pd->data_size>0 )  {
		// pass pd->data to some parser function....
		multi_process_bigdata( pd->data, pd->data_size );
	}

}

#ifdef WORDS_BIGENDIAN
void get_short_frame_info(ubyte *old_info, short_frame_info *new_info)
{
	int loc = 0;
	
	new_info->type = old_info[loc];                                     loc++;
	/* skip three for pad byte */                                       loc += 3;
	memcpy(&(new_info->numpackets), &(old_info[loc]), 4);               loc += 4;
	new_info->numpackets = INTEL_INT(new_info->numpackets);
	memcpy(new_info->thepos.bytemat, &(old_info[loc]), 9);              loc += 9;
	memcpy(&(new_info->thepos.xo), &(old_info[loc]), 2);                loc += 2;
	memcpy(&(new_info->thepos.yo), &(old_info[loc]), 2);                loc += 2;
	memcpy(&(new_info->thepos.zo), &(old_info[loc]), 2);                loc += 2;
	memcpy(&(new_info->thepos.segment), &(old_info[loc]), 2);           loc += 2;
	memcpy(&(new_info->thepos.velx), &(old_info[loc]), 2);              loc += 2;
	memcpy(&(new_info->thepos.vely), &(old_info[loc]), 2);              loc += 2;
	memcpy(&(new_info->thepos.velz), &(old_info[loc]), 2);              loc += 2;
	new_info->thepos.xo = INTEL_SHORT(new_info->thepos.xo);
	new_info->thepos.yo = INTEL_SHORT(new_info->thepos.yo);
	new_info->thepos.zo = INTEL_SHORT(new_info->thepos.zo);
	new_info->thepos.segment = INTEL_SHORT(new_info->thepos.segment);
	new_info->thepos.velx = INTEL_SHORT(new_info->thepos.velx);
	new_info->thepos.vely = INTEL_SHORT(new_info->thepos.vely);
	new_info->thepos.velz = INTEL_SHORT(new_info->thepos.velz);

	memcpy(&(new_info->data_size), &(old_info[loc]), 2);                loc += 2;
	new_info->data_size = INTEL_SHORT(new_info->data_size);
	new_info->playernum = old_info[loc];                                loc++;
	new_info->obj_render_type = old_info[loc];                          loc++;
	new_info->level_num = old_info[loc];                                loc++;
	memcpy(new_info->data, &(old_info[loc]), new_info->data_size);
}
#else
#define get_short_frame_info(old_info, new_info) \
	memcpy(new_info, old_info, sizeof(short_frame_info))
#endif

void network_read_pdata_short_packet(short_frame_info *pd )
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;
	short_frame_info new_pd;

// short frame info is not aligned because of shortpos.  The mac
// will call totally hacked and gross function to fix this up.

	get_short_frame_info((ubyte *)pd, &new_pd);

	TheirPlayernum = new_pd.playernum;
	TheirObjnum = Players[new_pd.playernum].objnum;

	if (TheirPlayernum < 0) {
		Int3(); // This packet is bogus!!
		return;
	}
	if (!multi_quit_game && (TheirPlayernum >= N_players)) {
		if (Network_status!=NETSTAT_WAITING)
		 {
			Int3(); // We missed an important packet!
			network_consistency_error();
			return;
		 }
		else
		 return;
	}

	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL) ) {
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		if ( new_pd.data_size>0 )       {
			// pass pd->data to some parser function....
			multi_process_bigdata( new_pd.data, new_pd.data_size );
		}
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	if ((sbyte)new_pd.level_num != Current_level_num)
	{
		return;
	}

	TheirObj = &Objects[TheirObjnum];

	//------------- Keep track of missed packets -----------------
	Players[TheirPlayernum].n_packets_got++;
	LastPacketTime[TheirPlayernum] = timer_get_approx_seconds();
	if  ( pd->numpackets != Players[TheirPlayernum].n_packets_got ) {
		missed_packets += pd->numpackets-Players[TheirPlayernum].n_packets_got;

		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------

	extract_shortpos(TheirObj, &new_pd.thepos, 0);

	if ((TheirObj->render_type != new_pd.obj_render_type) && (new_pd.obj_render_type == RT_POLYOBJ))
		multi_make_ghost_player(TheirPlayernum);

	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);

	//------------ Welcome them back if reconnecting --------------
	if (!Players[TheirPlayernum].connected) {
		Players[TheirPlayernum].connected = 1;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
		
		HUD_init_message( "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );

		multi_send_score();
	}

	//------------ Parse the extra data at the end ---------------

	if ( new_pd.data_size>0 )       {
		// pass pd->data to some parser function....
		multi_process_bigdata( new_pd.data, new_pd.data_size );
	}

}


int network_who_is_master(void)
{
	// Who is the master of this game?

	int i;

	if (!(Game_mode & GM_NETWORK))
		return (Player_num == 0);

	for (i = 0; i < N_players; i++)
		if (Players[i].connected)
			return i;
	return Player_num;
}

void network_ping (ubyte flag,int pnum)
{
	ubyte mybuf[2];

	mybuf[0]=flag;
	mybuf[1]=Player_num;
	*(u_int32_t*)(multibuf+2)=INTEL_INT(timer_get_fixed_seconds());
	NetDrvSendPacketData( (ubyte *)mybuf, 7, Netgame.players[pnum].server, Netgame.players[pnum].node,Players[pnum].net_address );
}

static fix PingLaunchTime[MAX_PLAYERS],PingReturnTime[MAX_PLAYERS];
fix PingTable[MAX_PLAYERS];

void network_handle_ping_return (ubyte pnum)
{
	if (PingLaunchTime[pnum]==0 || pnum>=N_players)
	{
		return;
	}

	PingReturnTime[pnum]=timer_get_fixed_seconds();
	PingTable[pnum]=f2i(fixmul(PingReturnTime[pnum]-PingLaunchTime[pnum],i2f(1000)));
	PingLaunchTime[pnum]=0;
}

// ping all connected players (except yourself) in 3sec interval and update ping_table
void network_ping_all()
{
	int i;
	static fix PingTime=0;

	PingTable[Player_num] = -1; // Set mine to fancy -1 because I am super fast! Weeee

	if (PingTime+(F1_0*3)<GameTime || PingTime > GameTime)
	{
		for (i=0; i<MAX_PLAYERS; i++)
		{
			if (Players[i].connected && i != Player_num)
			{
				PingLaunchTime[i]=timer_get_fixed_seconds();
				network_ping (PID_PING_SEND,i);
			}
		}
		PingTime=GameTime;
	}
}

void DoRefuseStuff (sequence_packet *their)
{
	int i;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		if (!strcmp (their->player.callsign,Players[i].callsign))
		{
			network_welcome_player(their);
				return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
			{
				network_welcome_player(their);
					return;
			}
		}
	
		digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
	
		HUD_init_message ("%s wants to join (accept: F6)",their->player.callsign);
	
		strcpy (RefusePlayerName,their->player.callsign);
		RefuseTimeLimit=timer_get_approx_seconds();
		RefuseThisPlayer=0;
		WaitForRefuseAnswer=1;
	}
	else
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
			{
				network_welcome_player(their);
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
			network_welcome_player(their);
			return;
		}

		if ((timer_get_approx_seconds()) > RefuseTimeLimit+REFUSE_INTERVAL)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (!strcmp (their->player.callsign,RefusePlayerName))
			{
				network_dump_player(their->player.server,their->player.node, DUMP_DORK);
			}
			return;
		}
	}
}

// Send request for game information. Resend to keep connection alive.
// Function arguments not used, but needed to call while nm_messagebox1
void network_info_req( int nitems, newmenu_item * menus, int * key, int citem )
{
	sequence_packet me;
	static fix nextsend, curtime;
	ubyte null_addr[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;
	curtime=timer_get_fixed_seconds();
	
	if (nextsend<curtime)
	{
		nextsend=curtime+F1_0*3;
		memset(&me, 0, sizeof(sequence_packet));
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		memcpy( me.player.node, NetDrvGetMyLocalAddress(), 6 );
		memcpy( me.player.server, NetDrvGetMyServerAddress(), 4 );
		me.type = PID_D1X_GAME_INFO_REQ;//get full info.

		send_sequence_packet( me, null_addr,Active_games[0].players[0].node,NULL);
	}
}

void show_game_rules(int choice)
{
	int done,k;
	grs_canvas canvas;
	int w = FSPACX(280), h = FSPACY(130);

	gr_set_current_canvas(NULL);

	gr_init_sub_canvas(&canvas, &grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200));

	game_flush_inputs();

	done = 0;

	while(!done)	{
		network_info_req( 0, NULL, 0, 0 );
		timer_delay2(50);
		gr_set_current_canvas(NULL);
#ifdef OGL
		gr_flip();
		nm_draw_background1(NULL);
#endif
		nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

		gr_set_current_canvas(&canvas);

		gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);
		grd_curcanv->cv_font = MEDIUM3_FONT;
	
		gr_string( 0x8000, FSPACY(35), "NETGAME INFO" );
	
		grd_curcanv->cv_font = GAME_FONT;
	

		gr_printf( FSPACX( 25),FSPACY( 55), "Reactor Life:");
		gr_printf( FSPACX( 25),FSPACY( 61), "Short Packets:");
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
		gr_printf( FSPACX(170),FSPACY( 55), "%i Min", Active_games[choice].control_invul_time/F1_0/60);
		gr_printf( FSPACX(170),FSPACY( 61), "%s", Active_games[choice].flags & NETFLAG_SHORTPACKETS?"ON":"OFF");
		gr_printf( FSPACX(170),FSPACY( 67), "%i", Active_games[choice].packets_per_sec);
		gr_printf( FSPACX(170),FSPACY( 73), Active_games[choice].game_flags&NETGAME_FLAG_SHOW_MAP?"ON":"OFF");


		gr_printf( FSPACX(130),FSPACY(110), Active_games[choice].flags&NETFLAG_DOLASER?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(116), Active_games[choice].flags&NETFLAG_DOQUAD?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(122), Active_games[choice].flags&NETFLAG_DOVULCAN?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(128), Active_games[choice].flags&NETFLAG_DOSPREAD?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(134), Active_games[choice].flags&NETFLAG_DOPLASMA?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(140), Active_games[choice].flags&NETFLAG_DOFUSION?"YES":"NO");
		gr_printf( FSPACX(275),FSPACY(110), Active_games[choice].flags&NETFLAG_DOHOMING?"YES":"NO");
		gr_printf( FSPACX(275),FSPACY(116), Active_games[choice].flags&NETFLAG_DOPROXIM?"YES":"NO");
		gr_printf( FSPACX(275),FSPACY(122), Active_games[choice].flags&NETFLAG_DOSMART?"YES":"NO");
		gr_printf( FSPACX(275),FSPACY(128), Active_games[choice].flags&NETFLAG_DOMEGA?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(150), Active_games[choice].flags&NETFLAG_DOINVUL?"YES":"NO");
		gr_printf( FSPACX(130),FSPACY(156), Active_games[choice].flags&NETFLAG_DOCLOAK?"YES":"NO");

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
		k = key_inkey();
		switch( k )	{
			case KEY_PRINT_SCREEN:
				save_screen_shot(0); k = 0;
				break;
			case KEY_ENTER:
			case KEY_SPACEBAR:
			case KEY_ESC:
				done=1;
				break;
		}
	}

	gr_set_current_canvas(NULL);
	game_flush_inputs();
}

int show_game_stats(int choice)
{
	char rinfo[512],*info=rinfo;
	char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Unknown"};
	int c;

	memset(info,0,sizeof(char)*256);

	info+=sprintf(info,"\nConnected to\n\"%s\"\n",Active_games[choice].game_name);

#ifndef SHAREWARE
	if(!Active_games[choice].mission_title)
		info+=sprintf(info,"Descent: First Strike");
	else
		info+=sprintf(info,Active_games[choice].mission_title);

   if( Active_games[choice].levelnum >= 0 )
   {
	   info+=sprintf (info," - Lvl %i",Active_games[choice].levelnum);
   }
   else
   {
      info+=sprintf (info," - Lvl S%i",(Active_games[choice].levelnum*-1));
   }
#endif
	info+=sprintf (info,"\n\nDifficulty: %s",MENU_DIFFICULTY_TEXT(Active_games[choice].difficulty));
	info+=sprintf (info,"\nGame Mode: %s",NetworkModeNames[Active_games[choice].gamemode]);
	info+=sprintf (info,"\nPlayers: %i/%i",Active_games[choice].numplayers,Active_games[choice].max_numplayers);

	while (1){
		c=nm_messagebox1("WELCOME", network_info_req, 2, "JOIN GAME", "GAME INFO", rinfo);
		if (c==0)
			return 1;
		else if (c==1)
			show_game_rules(choice);
		else
			return 0;
	}
}

int get_and_show_netgame_info(ubyte *server, ubyte *node, ubyte *net_address)
{
	if (setjmp(LeaveGame))
		return 0;

	num_active_games = 0;
	Network_games_changed = 0;
	Network_status = NETSTAT_BROWSING;
	memset(Active_games, 0, sizeof(netgame_info)*MAX_ACTIVE_NETGAMES);

	while (1){
		network_info_req( 0, NULL, 0, 0 );

		network_listen();

		if (Network_games_changed){
			if (num_active_games<1){
				Network_games_changed=0;
				continue;
			}
			if (show_game_stats(0))
				return 1;
			else
				return 0;
		}
		if (key_inkey()==KEY_ESC)
			return 0;

	}
	return 0;
}

#endif
