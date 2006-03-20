/* Source file for the ever useful ignore command.  Basically, it just has routines
   that enable the ignorer to add to, delete from, and clear the ignore list.
   Also has a routine that reports whether or not someone is on the ignore list.
   Now added are the "by-number" equivalents of the ignore and unignore commands.
   See the top of NNCOMS.C for more info.  */
#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ignore.h"
#include "multi.h"  //For playernum (?)
#include "player.h" //For CALLSIGN_LEN and Player_num vars...
#include "network.h" //For TheirPlayernum, network_disconnect_player(), Network_player_rejoining, and iPlayerLoop.
#include "hudmsg.h"
//added on 6/15/99 by Owen Evans
#include "strutil.h"
//end added - OE

extern int multi_message_index;

static char ignorelist[MAX_IGNORE][CALLSIGN_LEN+1]; //Ignore list

static char onignore[MAX_IGNORE * (2 + CALLSIGN_LEN)];


void addignore (char * ignorename)  //Add a player to the good 'ol ignore list.
{
	int iPlayerLoop;
        int ignoreloop;
        char name[CALLSIGN_LEN+1];
        //Ignore the specified player.
	strupr(ignorename);
        for(iPlayerLoop = 0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
		if(!stricmp(Players[iPlayerLoop].callsign,ignorename))
                {
                        for(ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
                        {
				if (!strcmp(ignorename,Players[Player_num].callsign))
                                {
                                        hud_message(MSGC_GAME_FEEDBACK,"You cannot ignore yourself!");
                                        return;
                                }
				if (!strcmp(ignorelist[ignoreloop], ignorename))
                                {
                                        hud_message(MSGC_GAME_FEEDBACK,"You've already ignored %s!",ignorename);
                                        return;
                                }
                                if (ignorelist[ignoreloop][0] == '\0')
                                {
                                        strcpy(ignorelist[ignoreloop], ignorename);
                                        hud_message(MSGC_GAME_FEEDBACK,"Now ignoring %s!", ignorename);
                                        strcpy(name, ignorename);
                                        Network_message_reciever = 100; // Let everyone know that I'm ignoring someone...
                                        snprintf( Network_message, MAX_MESSAGE_LEN, "I'm now ignoring %s!", name);
                                        multi_send_message();

                                        network_disconnect_player(iPlayerLoop);
                                        return;
                                }
                        }
                        hud_message(MSGC_GAME_FEEDBACK,"Ignore slots are full!");
                        return;
                }
        }
        hud_message(MSGC_GAME_FEEDBACK,"IGNORE: %s doesn't exist!",ignorename);
        Network_message[0]=0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;

}

void addignore_by_number (int ignorenum)
{
        int ignoreloop;
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((ignorenum <= 0) || (ignorenum > n))
        {
                hud_message(MSGC_GAME_FEEDBACK,"IGNOREN: Player number %d doesn't exist!",ignorenum);
                Network_message[0]=0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;
        }

        for(ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
        {
                if(!stricmp(Players[Player_num].callsign, Players[player_list[ignorenum - 1]].callsign))
                {
                        hud_message(MSGC_GAME_FEEDBACK,"You cannot ignore yourself!");
                        return;
                }

                if(!stricmp(ignorelist[ignoreloop], Players[player_list[ignorenum - 1]].callsign))
                {
                        hud_message(MSGC_GAME_FEEDBACK,"You've already ignored %s!", Players[player_list[ignorenum - 1]].callsign);
                        return;
                }

                if(ignorelist[ignoreloop][0] == '\0')
                {
                        strcpy(ignorelist[ignoreloop], Players[player_list[ignorenum - 1]].callsign);
                        hud_message(MSGC_GAME_FEEDBACK,"Now ignoring %s!", Players[player_list[ignorenum - 1]].callsign);
                        Network_message_reciever = 100; // Let everyone know that I'm ignoring someone...
                        snprintf( Network_message, MAX_MESSAGE_LEN, "I'm now ignoring %s!", Players[player_list[ignorenum - 1]].callsign);
                        multi_send_message();

                        network_disconnect_player(player_list[ignorenum - 1]);
                        return;
                }
        }
        hud_message(MSGC_GAME_FEEDBACK,"Ignore slots are full!");
        return;
}

void eraseignore (char * ignorename)  //Delete a player from the ignore list.
{
	int iPlayerLoop;
        int ignoreloop;
	strupr(ignorename);
        for(iPlayerLoop=0;iPlayerLoop!=MAX_NUM_NET_PLAYERS;iPlayerLoop++)
        {
		if(!stricmp(Players[iPlayerLoop].callsign,ignorename))
                {
                        for (ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
                        {
				if(!strcmp(ignorelist[ignoreloop], ignorename))
                                {
                                        ignorelist[ignoreloop][0] = '\0';
                                        hud_message(MSGC_GAME_FEEDBACK,"Now unignoring %s!", ignorename);
                                        Players[iPlayerLoop].connected = 1;
                                        return;
                                }
                        }
                        hud_message(MSGC_GAME_FEEDBACK,"Player %s not currently ignored!", ignorename);
                        return;
                }
        }
        hud_message(MSGC_GAME_FEEDBACK,"UNIGNORE: %s doesn't exist!",ignorename);
        Network_message[0]=0;
        multi_message_index = 0;
        multi_sending_message = 0;
        return;
}

void eraseignore_by_number (int ignorenum)
{
        int ignoreloop;
        int player_list[MAX_NUM_NET_PLAYERS];
        int n = multi_get_kill_list(player_list);

        if((ignorenum <= 0) || (ignorenum > n))
        {
                hud_message(MSGC_GAME_FEEDBACK,"UNIGNOREN: Player number %d doesn't exist!",ignorenum);
                Network_message[0]=0;
                multi_message_index = 0;
                multi_sending_message = 0;
                return;
        }

        for (ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
        {
                if(!strcmp(ignorelist[ignoreloop], Players[player_list[ignorenum - 1]].callsign))
                {
                        ignorelist[ignoreloop][0] = '\0';
                        hud_message(MSGC_GAME_FEEDBACK,"Now unignoring %s!", Players[player_list[ignorenum - 1]].callsign);
                        Players[player_list[ignorenum - 1]].connected = 1;
                        return;
                }
        }
        hud_message(MSGC_GAME_FEEDBACK,"Player %s not currently ignored!", Players[player_list[ignorenum - 1]].callsign);
        return;
}


int checkignore (char * ignorename)  //Checks to see if a player is ignored (returns 1 if so.)
{
        int ignoreloop;
        for (ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
	{
                if(!stricmp(ignorelist[ignoreloop], ignorename))
			return 1;
	}

        return 0;
}

void clearignore (void)  //Clears the ignore list of all players on it.
{
        int ignoreloop;
        int iPlayerLoop;

        for(ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
        {
                if(ignorelist[ignoreloop][0] != '\0')
                {       
                        for(iPlayerLoop = 0; iPlayerLoop < MAX_NUM_NET_PLAYERS; iPlayerLoop++)
                        {
                                if(!stricmp(Players[iPlayerLoop].callsign,ignorelist[ignoreloop]))
                                        Players[iPlayerLoop].connected = 1;
                        }

                        ignorelist[ignoreloop][0] = '\0';

                }
        }
        hud_message(MSGC_GAME_FEEDBACK,"Clearing ignore list!");
        return;
}

void listignore (void)  //Lists all players on ignore.
{
        int ignoreloop;
        for(ignoreloop = 0; ignoreloop < MAX_IGNORE; ignoreloop++)
        {
                if(ignorelist[ignoreloop][0] != '\0')
                {
                        strcat(onignore, ignorelist[ignoreloop]);
                        strcat(onignore, ",");
                        strcat(onignore, " ");
                }

        }
        if(onignore[0] == '\0')
	{
                hud_message(MSGC_GAME_FEEDBACK,"There is no one on the ignore list!");
        } else {
                hud_message(MSGC_GAME_FEEDBACK,"%s",onignore);
        }

        //Clear the onignore var to prevent overloading...
        onignore[0] = '\0';
        return;
}

#endif //ifdef NETWORK
