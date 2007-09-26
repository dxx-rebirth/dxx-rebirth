/* New network commands, implemented by GRiM FisH: kick, discon, ghost,
   unghost, recon. Kevin Bentley: ping  Victor Rachels: kick,mute,etc.
   Put in also are the "by-number" equivalents, which operate the command
   based on the position on the kill list. These "by-number" commands are
   designated by an "N" on the end of the normal name of the command.
   It is used in the following format: (command)N:n)
   where (command) is the command, and the lowercase "n" is the player's
   position on the kill list. This allows you to invoke commands on players
   with special characters in front of their names, duplicate player names
   and players with names of the commands. (i.e. a player named kick.) */
#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h" // Used for lots of things
#include "multi.h"   // Used also for lots of things
#include "player.h"  // Maybe used for some things here
#include "hudmsg.h"  // HUD_init_message
#include "timer.h"
//added 03/04/99 Matt Mueller
#include "nncoms.h"
#include "byteswap.h"
#include "types.h"
//end addition -MM
#include "vers_id.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added

extern void network_disconnect_player(int playernum);

extern void network_dump_player(ubyte * server, ubyte *node, int why);

extern int multi_message_index;

void boot (char * bootname)  //Kick command...
{
	int iPlayerLoop;
        char name[CALLSIGN_LEN+1];

        //Kick the specified player out of the current netgame...  Hopefully.
        if(!stricmp(bootname, Players[Player_num].callsign))
        {
              hud_message(MSGC_GAME_FEEDBACK,"You cannot kick yourself!");
              return;
        }
        if(!network_i_am_master())
        {
              hud_message(MSGC_GAME_FEEDBACK,"You cannot kick players! You are not master!");
              strcpy(name, bootname);
              Network_message_reciever = 100;         // Let everyone know that I'm trying to kick someone, but am not master...
              snprintf( Network_message, MAX_MESSAGE_LEN, "I'm trying to kick %s!", name);
              multi_send_message();
              return;
        }
        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
                if(!stricmp(bootname, Players[iPlayerLoop].callsign))
                {
                        //Found user, givin' player da' boot...
                        hud_message(MSGC_GAME_FEEDBACK,"Kicking %s!",bootname);
                        network_dump_player(Netgame.players[iPlayerLoop].server,Netgame.players[iPlayerLoop].node,0);
                        return;
                }

        }

        //Bad username
        hud_message(MSGC_GAME_FEEDBACK,"KICK: %s doesn't exist!",bootname);
	Network_message[0]= 0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;                     
}

void boot_by_number (int bootnum)  //Kick command...
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((bootnum <= 0) || (bootnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"KICKN: Player number %i doesn't exist!",bootnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;
        }

        //Kick the specified player out of the current netgame.
        if(player_list[bootnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot kick yourself!");
                return;
        }
        if(!network_i_am_master())
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot kick players! You are not master!");
                Network_message_reciever = 100;         // Let everyone know that I'm trying to kick someone, but am not master...
                snprintf( Network_message, MAX_MESSAGE_LEN, "I'm trying to kick %s!", Players[player_list[bootnum - 1]].callsign);
                multi_send_message();
                return;
        }
        //Found user, givin' player da' boot...
        hud_message(MSGC_GAME_FEEDBACK,"Kicking %s!",Players[player_list[bootnum - 1]].callsign);
        network_dump_player(Netgame.players[player_list[bootnum - 1]].server,Netgame.players[player_list[bootnum - 1]].node,0);
        return;
        
}

void discon (char * disconname) //Disconnect command...
{
	int iPlayerLoop;
        //Disconnect the selected user temporarily, used when player doesn't go away when you ignore them...
        if(!stricmp(disconname, Players[Player_num].callsign))
        {
               hud_message(MSGC_GAME_FEEDBACK,"You cannot disconnect yourself!");
               return;
        }
        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
                if(!stricmp(disconname, Players[iPlayerLoop].callsign))
                {
                        //Found user, disconnecting the dude.
                        hud_message(MSGC_GAME_FEEDBACK,"Disconnecting %s!",disconname);
                        network_disconnect_player(iPlayerLoop);
                        return;
                }
        }                         
        //Bad username
        hud_message(MSGC_GAME_FEEDBACK,"DISCON: %s doesn't exist!",disconname);
	Network_message[0]= 0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;
}

void discon_by_number (int disconnum) //Disconnect command...
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);
                                                                       
        if((disconnum <= 0) || (disconnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"DISCONN: Player number %i doesn't exist!",disconnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;                     
        }
        
        //Disconnect the selected user temporarily, used when player doesn't go away when you ignore them...
        if(player_list[disconnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot disconnect yourself!");
                return;
        }
        //Found user, disconnecting the dude.
        hud_message(MSGC_GAME_FEEDBACK,"Disconnecting %s!",Players[player_list[disconnum - 1]].callsign);
        network_disconnect_player(player_list[disconnum - 1]);
        return;
}


void ghost (char * ghostname) //Ghost command...
{
	int iPlayerLoop;
        //Ghost selected user (Very useful for the phantom ship syndrome...)
        if(!stricmp(ghostname, Players[Player_num].callsign))
        {
              hud_message(MSGC_GAME_FEEDBACK,"You cannot ghost yourself!");
              return;
        }

        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
                if(!stricmp(ghostname, Players[iPlayerLoop].callsign))
                {
                        //Found user, ghosting the dude...
                        hud_message(MSGC_GAME_FEEDBACK,"Ghosting %s!",ghostname);
                        multi_make_player_ghost(iPlayerLoop);
                        return;
                }
        }                         
        //Bad username
        hud_message(MSGC_GAME_FEEDBACK,"GHOST: %s doesn't exist!",ghostname);
	Network_message[0]= 0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;
}

void ghost_by_number (int ghostnum) //Ghost command...
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((ghostnum <= 0) || (ghostnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"GHOSTN: Player number %i doesn't exist!",ghostnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;                     
        }

        //Ghost selected user (Very useful for the phantom ship syndrome...)
        if(player_list[ghostnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot ghost yourself!");
                return;
        }
        //Found user, ghosting the dude...
        hud_message(MSGC_GAME_FEEDBACK,"Ghosting %s!",Players[player_list[ghostnum - 1]].callsign);
        multi_make_player_ghost(player_list[ghostnum - 1]);
        return;
}


void unghost (char * unghostname) //Unghost command...
{
	int iPlayerLoop;
        //Unghost the selected user (I wonder why I put this here{??}...)
        if(!stricmp(unghostname, Players[Player_num].callsign))
        {
           hud_message(MSGC_GAME_FEEDBACK,"You cannot unghost yourself!");
           return;
        }
        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
                if(!stricmp(unghostname, Players[iPlayerLoop].callsign))
                {
                        //Found user, unghosting the dude.
                        hud_message(MSGC_GAME_FEEDBACK,"Unghosting %s!",unghostname);
                        multi_make_ghost_player(iPlayerLoop);
                        return;
                }
        }                         
        //Bad username
        hud_message(MSGC_GAME_FEEDBACK,"UNGHOST: %s doesn't exist!",unghostname);
	Network_message[0]= 0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;
}
                
void unghost_by_number (int unghostnum) //Unghost command...
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((unghostnum <= 0) || (unghostnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"UNGHOSTN: Player number %i doesn't exist!",unghostnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;                     
        }

        //Unghost the selected user...
        if(player_list[unghostnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot unghost yourself!");
                return;
        }
        //Found user, unghosting the dude.
        hud_message(MSGC_GAME_FEEDBACK,"Unghosting %s!",Players[player_list[unghostnum - 1]].callsign);
        multi_make_ghost_player(player_list[unghostnum - 1]);
        return;
}

void recon (char * reconname) //Reconnect command...
{
	int iPlayerLoop;
        //Attempt to reconnect the selected user. Useful if you happen to drop a player from a netgame.
        if(!stricmp(reconname, Players[Player_num].callsign))
        {
               hud_message(MSGC_GAME_FEEDBACK,"You cannot reconnect yourself!");
               return;
        }
        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
                if(!stricmp(reconname, Players[iPlayerLoop].callsign))
                {
                        //Found user, reconnecting the dude.
                        hud_message(MSGC_GAME_FEEDBACK,"Reconnecting %s!",reconname);
                        Players[iPlayerLoop].connected = 1;
                        Netgame.players[iPlayerLoop].connected = 1;
                        return;
                }
        }                         
        //Bad username
        hud_message(MSGC_GAME_FEEDBACK,"RECON: %s doesn't exist!",reconname);
	Network_message[0]= 0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;
}

void recon_by_number (int reconnum) //reconnect by player number command...
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((reconnum <= 0) || (reconnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"RECONN: Player number %i doesn't exist!",reconnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;                     
        }
        
        //Attempt to reconnect the selected user. Useful if you happen to drop a player from a netgame.
        if(player_list[reconnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot reconnect yourself!");
                return;
        }
        
        //Found user, reconnecting the dude.
        hud_message(MSGC_GAME_FEEDBACK,"Reconnecting %s!",Players[player_list[reconnum - 1]].callsign);
        Players[player_list[reconnum - 1]].connected = 1;
        Netgame.players[player_list[reconnum - 1]].connected = 1;
        return;
}
#endif //ifdef NETWORK
