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

void ping_by_name(char * pingname)
{
//edited 3/04/99 Matt Mueller - replaced outdated ver with new one
        int pl;
//--killed--        //Send only to the recepient in the message..
//--killed--        if(!stricmp(pingname, Players[Player_num].callsign))
//--killed--        {
//--killed--                hud_message(MSGC_GAME_FEEDBACK,"You cannot ping yourself!");
//--killed--                return;
//--killed--        }
//--killed--        for(pl = 0; pl < MAX_NUM_NET_PLAYERS; pl++)
//--killed--        {
//--killed--                if (!stricmp(Players[pl].callsign, pingname))
//--killed--                {
//--killed--                        //send only to the specified user.
//--killed--                        Network_message_reciever = pl;
//--killed--                        sprintf(Network_message, "PING:%lu", timer_get_fixed_seconds());
//--killed--                        multi_send_message();
//--killed--                        hud_message(MSGC_GAME_FEEDBACK,"Pinging %s...", Players[pl].callsign);
//--killed--                        return;
//--killed--                }
//--killed--        }
//--killed--        //Bad username
           //Send only to the recepient in the message..
           for(pl = 0; pl < MAX_NUM_NET_PLAYERS; pl++)
            if (pl!=Player_num && !strnicmp(Players[pl].callsign, pingname, strlen(pingname)))
             {
               //send only to the specified user.
		     ping_by_player_num(pl,1);
               return;
             }
          //Bad username
          hud_message(MSGC_GAME_FEEDBACK, "PING: %s doesn't exist!", pingname);
//end edit -MM
        return;
}

void ping_by_number (int pingnum)
{
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((pingnum <= 0) || (pingnum > n))
        {
                //Bad usernum
                hud_message(MSGC_GAME_FEEDBACK,"PINGN: Player number %i doesn't exist!",pingnum);
                Network_message[0]= 0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;                     
        }
        
        //Send only to the recepient in the message..
        if(player_list[pingnum - 1] == Player_num)
        {
                hud_message(MSGC_GAME_FEEDBACK,"You cannot ping yourself!");
                return;
        }
        //send only to the specified user.
//edited 03/04/99 Matt Mueller - use generic func
//--killed--        Network_message_reciever = player_list[pingnum - 1];
//--killed--        sprintf(Network_message, "PING:%lu %i", timer_get_fixed_seconds(),player_list[pingnum - 1];);
//--killed--        multi_send_message();
//--killed--        hud_message(MSGC_GAME_FEEDBACK,"Pinging %s...", Players[player_list[pingnum - 1]].callsign);
    ping_by_player_num(player_list[pingnum - 1],1);
//end edit -MM
        return;
}

//added 03/04/99 Matt Mueller - move it all into one func.  duplication==bad.
void ping_by_player_num(int pl,int noisy)
{
//    static char tmp_message[MAX_MESSAGE_LEN];
    //Send only to the recepient in the message..
    if ((pl < MAX_NUM_NET_PLAYERS) && Players[pl].connected && pl!=(Player_num))
	{
	    if (Net_D1xPlayer[pl].iver>=D1X_DIRECTPING_IVER){
		  multibuf[0]=MULTI_PING;
		  multibuf[1]=Player_num;
		  *(u_int32_t*)(multibuf+2)=swapint(timer_get_fixed_seconds());
		  //		    ipx_send_packet_data( multibuf, 2+sizeof(long), Netgame.players[pl].server, Netgame.players[pl].node, Players[pl].net_address);
                  //printf("blah. %i %i %i\n",multibuf[0],multibuf[1],2+4);
		  mekh_send_direct_packet(multibuf,2+4,pl);
	    }else{
		  //send only to the specified user.
//		    strcpy(tmp_message,Network_message);
//		    Network_message_reciever = pl;
//		    sprintf(Network_message, "PING:%lu %i", timer_get_fixed_seconds(),pl);
//		    multi_send_message();
		  multibuf[0] = (char)MULTI_MESSAGE;
		  multibuf[1] = (char)Player_num;
		  sprintf(multibuf+2, "PING:%u %i", timer_get_fixed_seconds(),pl);
		  //edit 04/19/99 Matt Mueller - use direct sending
		  //--killed-- multi_send_data(multibuf,message_length[MULTI_MESSAGE],1);
		  mekh_send_direct_packet(multibuf,message_length[MULTI_MESSAGE],pl);
		  //end edit -MM

//		    strcpy(Network_message,tmp_message);
	    }	    
	    if (noisy)
		   hud_message(MSGC_GAME_FEEDBACK, "Pinging %s...", Players[pl].callsign);
	    return;// 1;
	}
    //Bad username
    if (noisy)
	   hud_message(MSGC_GAME_FEEDBACK, "PING: Player_num %i doesn't exist!", pl);
    
    return;
}

void ping_all(int noisy)
{
//    Network_message_reciever = 100;
//    sprintf(Network_message, "PING:%lu", timer_get_fixed_seconds());
//    multi_send_message();
    int i;
    for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
	   ping_by_player_num(i,0);//it handles all error cases, so don't worry here.
    if (noisy)
	   hud_message(MSGC_GAME_FEEDBACK,"Pinging...");
}
//end addition -MM
#endif //ifdef NETWORK
