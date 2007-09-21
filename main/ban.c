/* New network commands, banning and muting by Victor Rachels */
#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strio.h"
#include "args.h"
#include "types.h"
#include "ban.h"
#include "network.h"
#include "multi.h"
#include "player.h"
#include "hudmsg.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added

extern void network_dump_player(ubyte * server, ubyte *node, int why);

extern int multi_message_index;

int bansused = 0;
ubyte baniplist[MAX_BANS][3];

void addipban(char * banname) // ban command
{
 int pl;

   if(!bansused)
    memset(baniplist,0,MAX_BANS*3);

   if(!stricmp(banname,Players[Player_num].callsign))
    {
      hud_message(MSGC_GAME_FEEDBACK,"You cannot ban yourself!");
      return;
    }
   if(!network_i_am_master())
    {
      hud_message(MSGC_GAME_FEEDBACK,"You cannot ban players! You are not master!");
      Network_message_reciever=100;
      snprintf( Network_message, MAX_MESSAGE_LEN, "I'm trying to ban %s!", banname);
      multi_send_message();
      return;
    }

   for(pl=0;pl<MAX_NUM_NET_PLAYERS;pl++)
    {
       if(!stricmp(banname, Players[pl].callsign))
        {
         int banindex;

           if(checkban(Players[pl].net_address))
            {
              hud_message(MSGC_GAME_FEEDBACK,"You've already banned that ip...");
              network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
              return;
            }

           for(banindex=0;banindex<MAX_BANS;banindex++)
            if(baniplist[banindex][0]==0)
             {
               baniplist[banindex][0]=Players[pl].net_address[3];
               baniplist[banindex][1]=Players[pl].net_address[2];
               baniplist[banindex][2]=Players[pl].net_address[1];
               bansused++;
               hud_message(MSGC_GAME_FEEDBACK,"Adding %s:%d.%d.%d.* to bans",banname,(int)baniplist[banindex][0],(int)baniplist[banindex][1],(int)baniplist[banindex][2]);
               network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
               return;
             }
          hud_message(MSGC_GAME_FEEDBACK,"Banlist is full. Kicking...");
          network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
          return;
        }
    }
  hud_message(MSGC_GAME_FEEDBACK,"BAN: No such player");
}

void addban_by_number(int bannum)
{
 int player_list[MAX_NUM_NET_PLAYERS];
 int n = multi_get_kill_list(player_list);
 int pl;

   if(!bansused)
    memset(baniplist,0,MAX_BANS*3);
 
   if((bannum <=0) || (bannum > n))
    {
      hud_message(MSGC_GAME_FEEDBACK,"BANN: Player number %i doesn't exist",bannum);
      Network_message[0]= 0;
      multi_message_index = 0;
      multi_sending_message = 0;
      return;
    }

  pl = player_list[bannum - 1];

   if(pl == Player_num)
    {
      hud_message(MSGC_GAME_FEEDBACK,"You cannot ban yourself");
      return;
    }

   if(!network_i_am_master())
    {
      hud_message(MSGC_GAME_FEEDBACK,"You cannot ban players. You are not master");
      Network_message_reciever=100;
      snprintf( Network_message, MAX_MESSAGE_LEN, "I'm trying to ban %s!", Players[pl].callsign);
      multi_send_message();
      return;
    }

   if(checkban(Players[pl].net_address))
    {
      hud_message(MSGC_GAME_FEEDBACK,"You've already banned that ip...");
      network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
      return;
    }
   else
    {
     int banindex;
 
       for(banindex=0;banindex<MAX_BANS;banindex++)
        if(baniplist[banindex][0]==0)
         {
           baniplist[banindex][0]=Players[pl].net_address[3];
           baniplist[banindex][1]=Players[pl].net_address[2];
           baniplist[banindex][2]=Players[pl].net_address[1];
           bansused++;
           hud_message(MSGC_GAME_FEEDBACK,"Adding player %i:%d.%d.%d.* to bans",bannum,(int)baniplist[banindex][0],(int)baniplist[banindex][1],(int)baniplist[banindex][2]);
           network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
           return;
         }
      hud_message(MSGC_GAME_FEEDBACK,"Banlist is full. Kicking...");
      network_dump_player(Netgame.players[pl].server,Netgame.players[pl].node,0);
    }
}

int checkban(ubyte banip[6])
{
 int i;

   if(!bansused)
    return 0;

   for(i=0;i<MAX_BANS;i++)
    {
       if((baniplist[i][0]==banip[3]) &&
          (baniplist[i][1]==banip[2]) &&
          (baniplist[i][2]==banip[1]))
        return i+1;

    }

  return 0;
}

void unban(char * banname)
{
 int pl;

   if(!bansused)
    {
      hud_message(MSGC_GAME_FEEDBACK,"There are no bans set");
      return;
    }

   if(!network_i_am_master())
    {                                            
      hud_message(MSGC_GAME_FEEDBACK,"UNBAN: You are not master");
      return;
    }

   for(pl=0;pl<MAX_NUM_NET_PLAYERS;pl++)
    {
       if(!stricmp(banname, Players[pl].callsign))
        {
         int banindex;

           if(!(banindex = checkban(Players[pl].net_address)))
            {
              hud_message(MSGC_GAME_FEEDBACK,"UNBAN: That player is not banned");
              return;
            }

          hud_message(MSGC_GAME_FEEDBACK,"%s:%d.%d.%d.* unbanned",banname,(int)baniplist[banindex-1][0],(int)baniplist[banindex-1][1],(int)baniplist[banindex-1][2]);
          baniplist[banindex-1][0]=0;
          baniplist[banindex-1][1]=0;
          baniplist[banindex-1][2]=0;
          bansused--;

          return;
        }
    }
  hud_message(MSGC_GAME_FEEDBACK,"UNBAN: No such player");
}


void unban_by_number(int bannum)
{
 int player_list[MAX_NUM_NET_PLAYERS];

   if(!bansused)
    {
      hud_message(MSGC_GAME_FEEDBACK,"There are no bans set");
      return;
    }
 
   if((bannum <=0) || (bannum > multi_get_kill_list(player_list)))
    {
      hud_message(MSGC_GAME_FEEDBACK,"UNBANN: Player number %i doesn't exist",bannum);
      Network_message[0]= 0;
      multi_message_index = 0;
      multi_sending_message = 0;
      return;
    }

   if(!network_i_am_master())
    {
      hud_message(MSGC_GAME_FEEDBACK,"UNBANN: You are not master");
      return;
    }

    {
     int banindex;

       if(!(banindex = checkban(Players[player_list[bannum - 1]].net_address)))
        {
          hud_message(MSGC_GAME_FEEDBACK,"UNBANN: That player is not banned");
          return;
        }

      hud_message(MSGC_GAME_FEEDBACK,"%i:%d.%d.%d.* unbanned",bannum,(int)baniplist[banindex-1][0],(int)baniplist[banindex-1][1],(int)baniplist[banindex-1][2]);
      baniplist[banindex-1][0]=0;
      baniplist[banindex-1][1]=0;
      baniplist[banindex-1][2]=0;
      bansused--;
    }
}

void clearbans()
{
  bansused = 0;
  memset(baniplist,0,MAX_BANS*3);
}

void listbans()
{
 int i;
 char onban[MAX_BANS * (15)];

   if(!bansused)
    hud_message(MSGC_GAME_FEEDBACK,"The ban list is empty");

  memset(onban,0,MAX_BANS * (2 + CALLSIGN_LEN) * sizeof(char) );
   for(i=0;i<MAX_BANS;i++)
    {
     if(baniplist[i][0])
      {
       char banip[14];
        sprintf(banip," %d.%d.%d.*",baniplist[i][0],baniplist[i][1],baniplist[i][2]);
        strcat(onban,banip);
      }
    }

  hud_message(MSGC_GAME_FEEDBACK,"Ban list:%s",onban);
}

int readbans()
{
 FILE *f;

  clearbans();

   if(GameArg.MplNoBans)
    return 0;

   if(!(f=fopen("bans.d1x","rt")))
    return 0;

  bansused=0;

   while(!feof(f))
    {
     int ip0,ip1,ip2;
      fscanf(f,"%d.%d.%d.*\n",&ip0,&ip1,&ip2);
      baniplist[bansused][0]=ip0;
      baniplist[bansused][1]=ip1;
      baniplist[bansused][2]=ip2;
       if(baniplist[bansused][0])
        bansused++;
    }
  fclose(f);
  return bansused;
}

int writebans()
{
 FILE *f;
 int i;

  if(!(f=fopen("bans.d1x","wt")))
   return 0;

   if(bansused)
    for(i=0;i<MAX_BANS;i++)
     if(baniplist[i][0])
      fprintf(f,"%d.%d.%d.*\n",baniplist[i][0],baniplist[i][1],baniplist[i][2]);

  fclose(f);
  return bansused;
}

#endif //ifdef NETWORK
