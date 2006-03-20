//multiver.h added 4/18/99 - Matt Mueller

//We send ver packet several times to make sure it doesn't get dropped.  If
//they all fail, fall back to sending the old msg based info.  Once we have
//someones ver, we can use ack'd packets, so theres no need to worry about
//sending repeatedly to them.

#include <stdio.h>
#include "vers_id.h"
#include "multi.h"
#include "network.h"
#include "byteswap.h"
#include "multiver.h"
#include "hudmsg.h"
#include "mono.h"

#ifdef NETWORK
//#define LASTTIMEOUT (F1_0 * 5)
#define MULTIVER_RESENDTIME (F1_0 * 2)
#define MULTIVER_RESENDNUM 5
struct d1x_ver_item {
	fix last;
	fix lastrecv;
	int numleft;
	int mode;
}d1x_ver_queue[MAX_NUM_NET_PLAYERS];

void multi_do_d1x_ver_set(int src,int shp, int pps){
	Net_D1xPlayer[src].shp=shp;
	if(shp) {
		if (shp>Network_short_packets)
			Network_short_packets=shp;
		hud_message(MSGC_MULTI_INFO, "enabling short packets for %s",Players[src].callsign);
	}
	Net_D1xPlayer[src].pps=pps;
	if (pps!=Network_pps && pps>=2 && pps<=20){
		Network_pps=pps;
		//Network_packet_interval = F1_0 / Network_pps;
		hud_message(MSGC_MULTI_INFO, "setting pps to %i", Network_pps);
	}
}

void multi_do_d1x_ver(char * buf){
	int loc=1,pl,mode,shp,pps;
	pl=buf[loc++];
	mode=buf[loc++];
	Net_D1xPlayer[pl].iver=swapint(*(u_int32_t*)(buf+loc));loc+=4;
	shp=buf[loc++];	
	pps=buf[loc++];
	multi_do_d1x_ver_set(pl,shp,pps);
	sprintf(Net_D1xPlayer[pl].ver,"D1X v%i.%i",Net_D1xPlayer[pl].iver/1000,(Net_D1xPlayer[pl].iver%1000)/10);
	hud_message(MSGC_MULTI_INFO, "%s is using %s (%i,%i,%i,%i)", Players[pl].callsign,Net_D1xPlayer[pl].ver,Net_D1xPlayer[pl].iver,shp,pps,mode);

	//	if (mode==1 && d1x_ver_queue[pl].lastrecv+LASTTIMEOUT<GameTime)multi_d1x_ver_queue_init(MAX_NUM_NET_PLAYERS,2);
	if (mode==1)
		multi_d1x_ver_queue_init(MAX_NUM_NET_PLAYERS,2);
	multi_d1x_ver_queue_remove(pl);
	if (mode!=3)multi_d1x_ver_send(pl,3);
	d1x_ver_queue[pl].lastrecv=GameTime;
}

void multi_d1x_ver_queue_init(int host,int mode){
	int i;
    for (i=0;i<MAX_NUM_NET_PLAYERS;i++){
		if (i<host && i!=Player_num)
			multi_d1x_ver_queue_send(i,mode);
		else
			multi_d1x_ver_queue_remove(i);
	}
}

void multi_d1x_ver_queue_remove(int dest){
	d1x_ver_queue[dest].numleft=0;
	d1x_ver_queue[dest].mode=0;
	d1x_ver_queue[dest].lastrecv=0;
}

void multi_d1x_ver_queue_send(int dest, int mode){
	mprintf((0,"d1x_ver_queue_send: dest=%i mode=%i\n",dest,mode));
	if (mode==1)
		d1x_ver_queue[dest].last=GameTime;//wait abit after the game starts for bandwidth to even out somewhat
	else
		d1x_ver_queue[dest].last=GameTime-MULTIVER_RESENDTIME;//reply immediatly
	d1x_ver_queue[dest].numleft=MULTIVER_RESENDNUM;
	d1x_ver_queue[dest].mode=mode;
}

void multi_d1x_ver_send(int dest, int mode){
	int loc=0;
	mprintf((0,"d1x_ver_send: dest=%i mode=%i\n",dest,mode));
	d1x_ver_queue[dest].last=GameTime;
	multibuf[loc++]=MULTI_D1X_VER_PACKET;
	multibuf[loc++]=Player_num;
	multibuf[loc++]=mode;
	*(u_int32_t*)(multibuf+loc)=swapint(D1X_IVER);loc+=4;
	multibuf[loc++]=Network_short_packets;
	multibuf[loc++]=Network_pps;
	mekh_send_direct_reg_data(multibuf,loc,dest);
}

void multi_d1x_ver_frame(void){
	int i;
	for (i=0;i<MAX_NUM_NET_PLAYERS;i++){
		if (Players[i].connected && i!=Player_num){
			if (d1x_ver_queue[i].numleft && d1x_ver_queue[i].last+MULTIVER_RESENDTIME<GameTime){
				mprintf((0,"d1x_ver_frame: p=%i left=%i mode=%i\n",i,d1x_ver_queue[i].numleft,d1x_ver_queue[i].mode));
				d1x_ver_queue[i].numleft--;
				if(d1x_ver_queue[i].numleft==0)
					network_send_config_messages(i,d1x_ver_queue[i].mode);
				else
					multi_d1x_ver_send(i,d1x_ver_queue[i].mode);
			}
		}
	}
}

//added/changed 8/6/98 by Matt Mueller
//added 8/5/98 by Matt Mueller
void network_send_config_messages(int dest, int mode)
{
	mprintf((0,"network_send_config_messages: dest=%i mode=%i\n",dest,mode));
//added/modified on 8/13/98 by Matt Mueller to fix messaging bugs
   multibuf[0] = (char)MULTI_MESSAGE;
   multibuf[1] = (char)Player_num;
   sprintf(multibuf+2, "Nd1x:%i %i %i %i", dest, mode, Network_short_packets, Network_pps);
    
 //added 03/07/99 Matt Mueller - send directly when appropriate -- assumes messages are acked!
    if (dest==100)
	   multi_send_data(multibuf, message_length[MULTI_MESSAGE], 1);
    else
	   mekh_send_direct_reg_data(multibuf, message_length[MULTI_MESSAGE], dest);

   sprintf(multibuf+2, "Vd1x:%s", DESCENT_VERSION);
    if (dest==100)
	   multi_send_data(multibuf, message_length[MULTI_MESSAGE], 1);
    else
	   mekh_send_direct_reg_data(multibuf, message_length[MULTI_MESSAGE], dest);
 //end addition -MM
//end modified section - Matt Mueller
//   if (mode==1 || mode==4)
//	   hud_message(MSGC_MULTI_INFO, "Sending d1x config info to %s", dest==100?"all":Netgame.players[dest].callsign);
//  if (Network_short_packets){
//      Network_message_reciever = dest;
//      sprintf(Network_message, "Mshp:%i", dest);
//      multi_send_message();
//        if (dest==100)
//           HUD_init_message("Sending global short_packets notification");
//        else
//           HUD_init_message("Sending short_packets notification to %s", Netgame.players[dest].callsign);
//	printf("Sending short_packets notification to %s", dest==100?"all":Netgame.players[dest].callsign);
//      }
//   if (Network_pps!=10){
//      Network_message_reciever = dest;
//      sprintf(Network_message, "Npps:%i", Network_pps);
//      multi_send_message();
//      if (dest==100)
//         HUD_init_message("Sending global pps notification");
//      else
//         HUD_init_message("Sending pps notification to %s",Netgame.players[dest].callsign);
//	printf("Sending pps notification to %s", dest==100?"all":Netgame.players[dest].callsign);
//      }
}
//end modified section - Matt Mueller
//end modified section - Matt Mueller
#endif // NETWORK
