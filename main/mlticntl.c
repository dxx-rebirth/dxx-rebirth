/* mlticntl.c - multi control , by GriM FisH */
#ifdef NETWORK

#include <string.h>
#include <stdio.h>

#include "network.h"
#include "multi.h"
#include "text.h"
#include "endlevel.h"
#include "fuelcen.h"
#include "mono.h"
#include "error.h"
#include "ipx.h"
#include "timer.h"
#include "newdemo.h"
#include "hudmsg.h"
#include "collide.h"
#include "ban.h"
#include "reconfig.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added

#define RESTRICT_WAIT (F1_0/4)*30
#define RESTRICT_TIMEOUT (F1_0/4)*70

//============ Vars ================

int last_cinvul;  //Last reactor invuln time.
int last_max_players;  //Last number of max players.

int restrict_mode = 0;  //Are we using restrict mode?
fix restrict_start_time = 0;  //When did we get the restrict alert?
int waiting_to_join = 0;  //Are we restricting a player now?
char restricted_callsign[CALLSIGN_LEN+1] = "";  //What is the currently restricted player's name?
int accept_join = 0;  //Are we accepting the currently restricted player's join?
int dump_join = 0;  //Are we gonna dump this dude who is trying to join our game?

//========== Main Code ==============
int opt_checks,opt_sliders,opt_menus;

void lamer_network_menu_update (int nitems, newmenu_item * menus, int * last_key, int citem)  //This function updates the sliders in the previous function.
{
        if ( last_max_players != menus[opt_sliders].value )   {
                last_max_players = menus[4].value;
                sprintf( menus[opt_sliders].text, "%s: %d", "Max Players", menus[opt_sliders].value+2 );
                menus[opt_sliders].redraw = 1;
        }

#ifndef SHAREWARE
        if ( last_cinvul != menus[opt_sliders+1].value )   {
                sprintf( menus[opt_sliders+1].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_sliders+1].value*5, TXT_MINUTES_ABBREV );
                last_cinvul = menus[opt_sliders+1].value;
                menus[opt_sliders+1].redraw = 1;
        }
#endif
}

void lamer_do_netgame_menu (void)  //This function brings up a menu that lets you modify some of a running netgame's properties real-time.  Only takes effect if you are master.
{
        newmenu_item m[10];
        int item,opt;
        char max_player_text[32];
        char name[NETGAME_NAME_LEN+1];
#ifndef SHAREWARE
        char srinvul[32];
#endif

        if(!(Game_mode & GM_MULTI))
                return;

        if(!network_i_am_master())
                return;
        
        //Netgame.max_numplayers = MaxNumNetPlayers;

        opt=0;

        sprintf( name, "%s", Netgame.game_name);
        m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_DESCRIPTION; opt++;
        m[opt].type = NM_TYPE_INPUT; m[opt].text = name; m[opt].text_len = NETGAME_NAME_LEN; opt++;
        opt_checks=opt;
        m[opt].type = NM_TYPE_CHECK; m[opt].value = Netgame.game_flags & NETGAME_FLAG_CLOSED; m[opt].text = TXT_CLOSED_GAME; opt++;
        m[opt].type = NM_TYPE_CHECK; m[opt].value = restrict_mode; m[opt].text = "Restricted"; opt++;
//        m[opt].type = NM_TYPE_CHECK; m[opt].value = Netgame.game_flags & NETGAME_FLAG_SHOW_MAP; m[opt].text = TXT_SHOW_ON_MAP; opt++;

        opt_sliders = opt;
        sprintf( max_player_text, "%s: %d", "Max Players", last_max_players+2 );
        last_max_players = Netgame.max_numplayers;
        m[opt].type = NM_TYPE_SLIDER; m[opt].value = Netgame.max_numplayers-2; m[opt].text = max_player_text; m[opt].min_value = 0; m[opt].max_value = MAX_NUM_NET_PLAYERS-2; opt++;

#ifndef SHAREWARE
        control_invul_time = f2i(Netgame.control_invul_time/5/60);
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, 5*control_invul_time, TXT_MINUTES_ABBREV );
	last_cinvul = control_invul_time;
        m[opt].type = NM_TYPE_SLIDER; m[opt].value = control_invul_time; m[opt].text = srinvul; m[opt].min_value = 0; m[opt].max_value = 15; opt++;
#endif
//added on 11/17/98 by Victor Rachels to blow up mine
        opt_menus = opt;
        m[opt].type = NM_TYPE_MENU; m[opt].text = "Blow up Mine."; opt++;
        m[opt].type = NM_TYPE_MENU; m[opt].text = "Save Bans now."; opt++;
//end this section addition - VR

        item = newmenu_do1( NULL, TXT_NETGAME_SETUP, opt, m, lamer_network_menu_update, 1 );

        if ( item > -1 )
        {

                strcpy( Netgame.game_name, name );

                if (m[opt_checks].value)
                        Netgame.game_flags |= NETGAME_FLAG_CLOSED;
                else
                        Netgame.game_flags &= ~NETGAME_FLAG_CLOSED;

                if (m[opt_checks+1].value)
                        restrict_mode = 1;
                else
                        restrict_mode = 0;



/*                if (m[opt_checks+2].value)
                        Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
                else
                        Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
*/
                MaxNumNetPlayers = m[opt_sliders].value+2;
                Netgame.max_numplayers = MaxNumNetPlayers;

#ifndef SHAREWARE
                control_invul_time = m[opt_sliders+1].value;
                Netgame.control_invul_time = control_invul_time*5*F1_0*60;
#endif

//added on 11/17/98 by Victor Rachels to blow up mine
                 if(item==opt_menus && !Fuelcen_control_center_destroyed)
                  {
                    net_destroy_controlcen(NULL);
                    multi_send_destroy_controlcen(-1,Player_num);
                  }
//end this section addition - VR
//added on 2/2/99 by Victor Rachels to save bans
                 if(item==opt_menus+1)
                  nm_messagebox(NULL,1,TXT_OK,"%i Bans saved",writebans());
//end this section addition - BR


//added on 6/7/99 by Victor Rachels for ingame reconfig
                reconfig_send_gameinfo();
        }
}

void lamer_do_restrict_alert (sequence_packet * their)  //This function lets you know that someone wants to join a restricted netgame.  Displays joining player's callsign and IP.
{
        ubyte ipaddress[6];

        if(!restrict_mode)
                return;

        if((dump_join) && (!stricmp(their->player.callsign, restricted_callsign)))
        {
                network_dump_player(their->player.server, their->player.node, DUMP_DORK);
                dump_join = 0;
                return;
        }       

        if((restrict_start_time+RESTRICT_WAIT > GameTime) && (waiting_to_join) && (!stricmp(restricted_callsign, their->player.callsign)))
                return;

        if(accept_join)
                return;

        //if ( (*(uint *)their->player.server) != 0 )
        //        ipx_get_local_target( their->player.server, their->player.node, ipaddress );
        //else
                memcpy(ipaddress, their->player.node, 6);

        digi_play_sample(SOUND_CONTROL_CENTER_WARNING_SIREN, F1_0*4);
        hud_message(MSGC_GAME_FEEDBACK,"%s, ip %d.%d.%d.%d, wants to join the game!", their->player.callsign, /*(int)ipaddress[5], (int)ipaddress[4],*/ (int)ipaddress[3], (int)ipaddress[2], (int)ipaddress[1], (int)ipaddress[0]);

        restrict_start_time = GameTime;
        waiting_to_join = 1;
        strcpy(restricted_callsign, their->player.callsign);
        //accept_join = 0;
        dump_join = 0;

        return;
}

void lamer_do_restrict_frame (void)  //This function times out a request to join a restricted netgame if the time has been longer than RESTRICT_TIMEOUT.
{
        if(!restrict_mode)
                return;

        if(!waiting_to_join)
                return;

        if(restrict_start_time+RESTRICT_TIMEOUT < GameTime)
        {
                waiting_to_join = 0;
                accept_join = 0;
        }

        return;
}

void lamer_accept_joining_player (void)  //This function allows a player that wants to join a restricted game in.
{
        if(!restrict_mode)
                return;

        if(!waiting_to_join)
                return;

        accept_join = 1;

        hud_message(MSGC_GAME_FEEDBACK,"Joining in player %s!", restricted_callsign);

        return;
}

void lamer_dump_joining_player (void)  //This function dumps a player join.
{
        if(!restrict_mode)
                return;

        if(!waiting_to_join)
                return;

        dump_join = 1;

        hud_message(MSGC_GAME_FEEDBACK,"Dumping player %s!", restricted_callsign);

        return;
}
        
void lamer_network_welcome_player_restricted (sequence_packet * their)  //This function is a modified network.c join player function that has been modified to allow you to selectively join in players.
{
 	// Add a player to a game already in progress
	ubyte local_address[6];
	int player_num;
	int i;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

        if(!restrict_mode)
                return;

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

                        lamer_do_restrict_alert(their);

                        if ((waiting_to_join) && (!accept_join))
                                return;
                
                        if (!waiting_to_join)
                                return;
                
                        if(accept_join)
                                if(stricmp(their->player.callsign, restricted_callsign) != 0)
                                        return;

                        waiting_to_join = 0;
                        accept_join = 0;

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

                                lamer_do_restrict_alert(their);

                                if ((waiting_to_join) && (!accept_join))
                                        return;
                        
                                if (!waiting_to_join)
                                        return;
                        
                                if(accept_join)
                                        if(stricmp(their->player.callsign, restricted_callsign) != 0)
                                                return;

                                waiting_to_join = 0;
                                accept_join = 0;

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

                lamer_do_restrict_alert(their);

                if ((waiting_to_join) && (!accept_join))
                        return;
        
                if (!waiting_to_join)
                        return;
                
                if(accept_join)
                        if(stricmp(their->player.callsign, restricted_callsign) != 0)
                                return;

                waiting_to_join = 0;
                accept_join = 0;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);
#endif
                Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

                hud_message(MSGC_GAME_FEEDBACK,"'%s' %s", Players[player_num].callsign, TXT_REJOIN);
	}

	// Send updated Objects data to the new/returning player

        Network_player_rejoining = *their;
	Network_player_rejoining.player.connected = player_num;
	Network_send_objects = 1;
	Network_send_objnum = -1;

	network_send_objects();
}

#endif //ifdef NETWORK
