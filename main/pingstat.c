/* pingstat.c by Victor Rachels for constant ping/stat info */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "fix.h"
#include "args.h"
#include "network.h"
#include "multi.h"
#include "player.h"
#include "timer.h"
//added 03/04/99 Matt Mueller - use generic pingfunc.  duplication==bad.
#include "nncoms.h"
//end addition -MM

#ifdef NETWORK

int ping_stats_on = 0;

fix ping_last_time = 0;
fix ping_next_reset = 0;
int ping_list[MAX_NUM_NET_PLAYERS];
int ping_received_list[MAX_NUM_NET_PLAYERS][2];

void ping_stats_frame()
{
   if(!ping_stats_on)
    return; //shouldn't be here

   if(ping_next_reset<GameTime)
    {
     int i;
       for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
        {
          ping_received_list[i][0]=0;
          ping_received_list[i][1]=0;
        }
      ping_next_reset=GameTime+F1_0*300;
    }

   if((ping_last_time+F1_0*3)<GameTime)
    {
     int i;
      for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
       if(Players[i].connected)
        {
//edited 03/04/99 Matt Mueller - use generic pingfunc.  duplication==bad.
//--killed--         char old_message[MAX_MESSAGE_LEN];
//--killed--          strcpy(old_message,Network_message);
//--killed--          Network_message_reciever = i;
//--killed--          sprintf(Network_message, "PING:%lu %i", timer_get_fixed_seconds(),i);
//--killed--          multi_send_message();
//--killed--          strcpy(Network_message,old_message);
           if(Net_D1xPlayer[i].iver)
            {
              ping_by_player_num(i,0);
//end edit -MM
              ping_received_list[i][0]++;
            }
        }
       else
        {
          ping_list[i] = 0;
          ping_received_list[i][0]=0;
          ping_received_list[i][1]=0;
        }
      ping_last_time = GameTime;
    }
}

void ping_stats_received(int pl,int pingtime)
{
   if(!ping_stats_on)
    return;

  ping_list[pl]=pingtime;
  ping_received_list[pl][1]++;
}

int ping_stats_getping(int pl)
{
  return ping_list[pl];
}

int ping_stats_getsent(int pl)
{
 if(ping_received_list[pl][0])
  return ping_received_list[pl][0];
 else
  return 1;
}

int ping_stats_getgot(int pl)
{
  return ping_received_list[pl][1];
}

//added on 4/17/99 by Victor Rachels for ping while pingstats on
void ping_stats_sentping(int pl)
{
  ping_received_list[pl][0]++;
  return;
}
//end this addition - VR

void ping_stats_init()
{
  memset(ping_list,0,MAX_NUM_NET_PLAYERS*sizeof(fix));
  memset(ping_received_list,0,MAX_NUM_NET_PLAYERS*2*sizeof(int));
  ping_last_time = GameTime;
  ping_next_reset = GameTime;
}

#endif
