/* For command parsing from console or f8 "$..."  by Victor Rachels*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "ban.h"
#include "timer.h"
#include "hudmsg.h"
#include "multi.h"
#include "network.h"
#include "nncoms.h"
#include "ignore.h"

#include "strutil.h"

//added 06/27/99 Victor Rachels
#include "vlcnfire.h"
#include "reconfig.h"
#include "multipow.h"
//end this section addition - VR

//added 04/19/99 Matt Mueller
#include "multiver.h"
//end addition -MM

int Command_parse(char *command)
{
#ifdef NETWORK
 int cmdlng = strlen(command);
#endif
 char *colon = strchr(command,':');
 char *space = strchr(command,' ');

   if(!colon && !space)
    {
#ifdef NETWORK
       if((cmdlng==12)&&!strnicmp("shortpackets",command,12))
        {
           if(!Network_short_packets)
            {
              hud_message(MSGC_GAME_FEEDBACK, "now using Short Packets");
              Network_short_packets = 1;
              network_send_config_messages(100,1);
            }
           else      
            hud_message(MSGC_GAME_FEEDBACK, "already using Short Packets");
          return 1;
        }
       if((cmdlng==11)&&!strnicmp("clearignore",command,11))
        {
          clearignore();
          return 1;
        }
       if((cmdlng==10) && (!strnicmp("listignore",command,10) ||
                           !strnicmp("ignorelist",command,10)) )
        {
          listignore();
          return 1;
        }
#ifndef SHAREWARE //FIXME: shareware multiplay really needs help
//added 06/27/99 Victor Rachels for altvulcan reconfig
       if((cmdlng==9)&&!strnicmp("altvulcan",command,9))
        {
           if(!use_alt_vulcanfire)
            {
              hud_message(MSGC_GAME_FEEDBACK, "now using alternate vulcanfire");
              use_alt_vulcanfire=1;
              Netgame.flags |= NETFLAG_ENABLE_ALT_VULCAN;
              reconfig_send_gameinfo(100,1);
            }
           else
            hud_message(MSGC_GAME_FEEDBACK, "already using alternate vulcanfire");
          return 1;
        }
//end this section addition - VR
#endif
       if((cmdlng==9)&&!strnicmp("clearbans",command,9))
        {
          clearbans();
          hud_message(MSGC_GAME_FEEDBACK,"Bans cleared");
          return 1;
        }
       if(((cmdlng==8)&&!strnicmp("listbans",command,8)) ||
          ((cmdlng==7)&&!strnicmp("banlist" ,command,7)) )
        {
          listbans();
          return 1;
        }
#endif //#ifdef NETWORK
    }
 
   if(!colon && space)
    *space = ':';

#ifdef NETWORK
   if((cmdlng>10)&&!strnicmp("unignoren:",command,10))
    {
       if(!isdigit(command[10]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"UNIGNOREN: Please use a number as input!");
          return 1;
        }
      eraseignore_by_number(atoi(command+10));
      return 1;
    }
   if((cmdlng>9)&&!strnicmp("handicap:",command,9))
    {
     int t;
      t=atoi(command+9);
       if(t==100)
        {
          handicap = MAX_SHIELDS;
          Network_message_reciever = 100;
          snprintf(Network_message, MAX_MESSAGE_LEN, "I am no longer handicapped.");
          hud_message(MSGC_GAME_FEEDBACK, "Handicap reset. Other players notified");
          multi_send_message();
          multi_message_feedback();
        }
       else
        if(t>0 && (t<100 || (network_i_am_master()&&t<201)))
         {
           handicap = i2f(t);             
            if(network_i_am_master())
             Lhandicap = 1;
            else
             Lhandicap = 0;
           Network_message_reciever = 100;
           snprintf(Network_message, MAX_MESSAGE_LEN, "I am using a handicap of %i.",t);
           hud_message(MSGC_GAME_FEEDBACK, "Handicap set. Other players notified");
           multi_send_message();
           multi_message_feedback();
          }
         else if(t > 100 && t <= 200)
          {
           handicap = i2f(t);
            if(Lhandicap)
             {
               Network_message_reciever = 100;
               snprintf(Network_message, MAX_MESSAGE_LEN, "I am using handicap of %i.",t);
               hud_message(MSGC_GAME_FEEDBACK, "Handicap set. Other players notified");
               multi_send_message();
               multi_message_feedback();
             }
            else
             {
              int netmaster=network_whois_master();
               hud_message(MSGC_GAME_FEEDBACK, "Master permission required. Requesting...");
               Network_message_reciever = netmaster;
               snprintf(Network_message, MAX_MESSAGE_LEN, "%s requests handicap of %i",Players[Player_num].callsign,t);
               multi_send_message();
               multi_message_feedback();
               Network_message_reciever = netmaster;
               snprintf(Network_message, MAX_MESSAGE_LEN, "Send %s:handicap %i to allow",Players[Player_num].callsign,t);
               multi_send_message();
               multi_message_feedback();
             }
          }
         else
          hud_message(MSGC_GAME_FEEDBACK, "Invalid handicap value");
      return 1;
    }
   if((cmdlng>9)&&!strnicmp("unignore:",command,9))
    {
      eraseignore(command+9);
      return 1;
    }
   if((cmdlng>9)&&!strnicmp("unghostn:",command,9))
    {
       if(!isdigit(command[9]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"UNGHOSTN: Please use a number as input!");
          return 1;
        }
      unghost_by_number(atoi(command+9));
      return 1;
    }
   if((cmdlng>8)&&!strnicmp("ignoren:",command,8))
    {
       if(!isdigit(command[8]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"IGNOREN: Please use a number as input!");
          return 1;
        }
      
      addignore_by_number(atoi(command+8));
      return 1;
    }
   if((cmdlng>8)&&!strnicmp("disconn:",command,8))
    {
       if(!isdigit(command[8]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"DISCONN: Please use a number as input!");
          return 1;
        }
      discon_by_number(atoi(command+8));
      return 1;
    }
   if((cmdlng>8)&&!strnicmp("unghost:",command,8))
    {
      unghost(command+8);
      return 1;
    }
   if((cmdlng>7)&&!strnicmp("ignore:",command,7))
    {
      addignore(command+7);
      return 1;
    }
   if((cmdlng>7)&&!strnicmp("discon:",command,7))
    {
      discon(command+7);
      return 1;
    }

   if((cmdlng>7)&&!strnicmp("reconn:",command,7))
    {
       if(!isdigit(command[7]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"RECONN: Please use a number as input!");
          return 1;
        }
      recon_by_number(atoi(command+7));
      return 1;
    }
   if((cmdlng>7)&&!strnicmp("ghostn:",command,7))
    {
       if(!isdigit(command[7]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"GHOSTN: Please use a number as input!");
          return 1;
        }
      ghost_by_number(atoi(command+7));
      return 1;
    }
   if((cmdlng>7)&&!strnicmp("unbann:",command,7))
    {
       if(!isdigit(command[7]))
        {
           hud_message(MSGC_GAME_FEEDBACK,"UNBANN: Please use a number as input!");
          return 1;
        }
      unban_by_number(atoi(command+7));
      return 1;
    }
   if((cmdlng>6)&&!strnicmp("unban:",command,6))
    {
      unban(command+6);
      return 1;
    }
   if((cmdlng>6)&&!strnicmp("ghost:",command,6))
    {
      ghost(command+6);
      return 1;
    }
   if((cmdlng>6)&&!strnicmp("recon:",command,6))
    {
      recon(command+6);
      return 1;
    }
   if((cmdlng>6)&&!strnicmp("kickn:",command,6))
    {
       if(!isdigit(command[6]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"KICKN: Please use a number as input!");
          return 1;
        }
      boot_by_number(atoi(command+6));
      return 1;
    }
   if((cmdlng>6)&&!strnicmp("pingn:",command, 6))
     {
       if(!isdigit(command[6]))
        {
          hud_message(MSGC_GAME_FEEDBACK,"PINGN: Please use a number as input!");
          return 1;
        }
      ping_by_number(atoi(command+6));
      return 1;
    }
   if((cmdlng>5)&&!strnicmp("kick:",command,5))
    {
      boot(command+5);
      return 1;
    }
   if(((cmdlng>=5)&&!strnicmp("ping:", command, 5) ) ||
      ((cmdlng==4)&&!strnicmp("ping" , command, 4) ) )
    {
       //added/modified on 8/13/98 by Matt Mueller to fix ping bug, and allow ping all
       if(*(command + 5) && *(command+4))
        {
		//edited 03/04/99 Matt Mueller - moved to seperate function
		ping_by_name(command+5);
        }
       else
        {
		ping_all(1);
		//end edit -MM
        }
       //end modified section - Matt Mueller
      return 1;
    }
/*   if((cmdlng>5)&&!strnicmp("mute:",command,5))
    {
      addmute(command+5);
      return 1;
    }*/
   if((cmdlng>5)&&!strnicmp("bann:",command,5))
    {
       if(!isdigit(command[5]))
        {
           hud_message(MSGC_GAME_FEEDBACK,"BANN: Please use a number as input!");
          return 1;
        }
      addban_by_number(atoi(command+5));
      return 1;
    }
   if((cmdlng>4)&&!strnicmp("ban:",command,4))
    {
      addipban(command+4);
      return 1;
    }
   if(((cmdlng>=4)&&!strnicmp("d1x:", command, 4)) ||
      ((cmdlng==3)&&!strnicmp("d1x" , command, 3)) )
    {
//edited 04/19/99 Matt Mueller - use new multiver stuff
       if (*(command + 4) && *(command+3))
        {
         int pl;
           for(pl = 0; pl < MAX_NUM_NET_PLAYERS; pl++)
            if (!strnicmp(Players[pl].callsign, command + 4, strlen(command + 4)))
             {
				 multi_d1x_ver_queue_send(pl,2);
				 hud_message(MSGC_MULTI_INFO, "Sending d1x config info to %s", Netgame.players[pl].callsign);
//killed               network_send_config_messages(pl,1);
               return 1;
             }
          hud_message(MSGC_GAME_FEEDBACK, "d1x: %s doesn't exist!", command + 4);
        }
       else{
		   multi_d1x_ver_queue_init(MAX_NUM_NET_PLAYERS,2);
		   hud_message(MSGC_MULTI_INFO, "Sending d1x config info to %s", "all");
	   }
//killed        network_send_config_messages(100,1);
//end edit -MM
      return 1;
    }

   if(!strnicmp(command,"me:",3))
    {
      sprintf(command,"me %s",command+3);
      return 0;
    }
#endif //ifdef NETWORK

  hud_message(MSGC_GAME_FEEDBACK,"No such command");
  return 0;
}  
   
