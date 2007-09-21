/* 3/15/99-4/16/99 by Victor Rachels to add new vulcan firing system
 *  basically works by sending ON/OFF packet to toggle vulcan.  Vulcan fires
 *   on local system every X ms.  ON packet is resent every Y ms. if no ON
 *   packet is received in Y+Z ms, vulcan is automatically shut off */

#include "types.h"
#include "multi.h"
#include "network.h"
#include "player.h"
#include "laser.h"
#include "weapon.h"
#include "game.h"
#include "vlcnfire.h"
#include "vers_id.h"
#include "powerup.h"
#include "fireball.h"

#include "hudmsg.h"

int use_alt_vulcanfire=0;
#ifdef NETWORK

fix LastVulcanSent=0;
int vulcan_ammo_used=0;
VulcanPlayerStruct VulcanPlayers[MAX_NUM_NET_PLAYERS];

#define VULCAN_FIRE_INTERVAL F1_0*3/64 // time between actual firing
#define VULCAN_SEND_INTERVAL F1_0/10 // time between sending on info
#define VULCAN_NEED_INTERVAL F1_0/5  // time before timeout

void vulcan_init()
{
 int i;

  use_alt_vulcanfire=0;
  LastVulcanSent = 0;
  vulcan_ammo_used = 0;
   for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
    {
     VulcanPlayers[i].vulcanon=0;
     VulcanPlayers[i].lastvulcanfired=0;
     VulcanPlayers[i].lastvulcaninfo=0;
    }
}

void send_vulcan_info(int packettype)
{
 int i;
 ubyte v_packet[2];

  v_packet[0]=packettype;
  v_packet[1]=Player_num;

   for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
    if(Players[i].connected && (i!=Player_num) && (Net_D1xPlayer[i].iver >= D1X_ALT_VULCAN_IVER))
     {
      mekh_send_direct_packet(v_packet,2,i);
     }
}

void vulcanframe()
{
 int i;

   for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
    if(Players[i].connected && VulcanPlayers[i].vulcanon)
     {
        if((i!=Player_num) && ((VulcanPlayers[i].lastvulcaninfo + VULCAN_NEED_INTERVAL) <= GameTime))
         VulcanPlayers[i].vulcanon=0;
        else if((VulcanPlayers[i].lastvulcanfired + VULCAN_FIRE_INTERVAL) <= GameTime)
         {
           VulcanPlayers[i].lastvulcanfired = GameTime;
           do_laser_firing(Players[i].objnum,VULCAN_INDEX,0,0,1);

            if(VulcanPlayers[Player_num].vulcanon)
             {
                if(i==Player_num)
                 {
                  int ammo_used;
                   ammo_used = Weapon_info[Primary_weapon_to_weapon_info[VULCAN_INDEX]].ammo_usage;

                     if(ammo_used > Players[Player_num].primary_ammo[VULCAN_INDEX])
                      {
                        Players[Player_num].primary_ammo[VULCAN_INDEX] = 0;
                        VulcanPlayers[Player_num].vulcanon = 0;
                        auto_select_weapon(0);
                      }
                     else
                      {
                        Players[Player_num].primary_ammo[VULCAN_INDEX] -= ammo_used;
                         if(Laser_drop_vulcan_ammo && ((vulcan_ammo_used += ammo_used) >= VULCAN_AMMO_AMOUNT*2))
                          {
                            vulcan_ammo_used = 0;
                             if(Players[Player_num].primary_ammo[VULCAN_INDEX] >= VULCAN_AMMO_AMOUNT*2)
                              maybe_drop_net_powerup(POW_VULCAN_AMMO);
                          }
                      }
                   Network_laser_fired = 0;
                 }
                else if(Net_D1xPlayer[i].iver < D1X_ALT_VULCAN_IVER)
                 {
                   //send normal vulcanfire info to player i
                   Network_laser_fired = 1;
                   Network_laser_gun = VULCAN_INDEX;
                   multi_send_fire(i);
                 }
             }
         }
     }

   if(VulcanPlayers[Player_num].vulcanon && ((LastVulcanSent + VULCAN_SEND_INTERVAL) <= GameTime))
    {
      LastVulcanSent = GameTime;
      send_vulcan_info(MULTI_ALT_VULCAN_ON);
    }
}

void got_vulcan_info(int vulcan_on, int pl)
{
  VulcanPlayers[pl].vulcanon = vulcan_on;
  VulcanPlayers[pl].lastvulcaninfo = GameTime;
}

void do_vulcan_fire(int vulcan_on)
{
  if(!vulcan_on || (Primary_weapon != VULCAN_INDEX) || (Players[Player_num].primary_ammo[VULCAN_INDEX] < Weapon_info[Primary_weapon_to_weapon_info[VULCAN_INDEX]].ammo_usage))
   {
      if(VulcanPlayers[Player_num].vulcanon)
       send_vulcan_info(MULTI_ALT_VULCAN_OFF);

     VulcanPlayers[Player_num].vulcanon = 0;
     VulcanPlayers[Player_num].lastvulcanfired = 0;
     return;
   }


  VulcanPlayers[Player_num].vulcanon = 1;
}

#endif //ifdef network
