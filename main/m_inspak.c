// M_INSPAK.C - Geoff Coovert 8/28/98
// Allows multiplayer messages to get to their destination and
// interpreted even if dropped.

#ifdef NETWORK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game.h"
#include "modem.h"
#include "multi.h"
#include "network.h"
#include "hudmsg.h"
#include "ipx.h"
#include "netpkt.h"
#include "vers_id.h"

//added on 03/05/99 Matt Mueller
#include "mono.h"
//end addition

mekh_packet_stats mekh_acks[MEKH_PACKET_MEM];
        //Info for packets that need an ack to come back

mekh_ackedpacket_history mekh_packet_history[MEKH_PACKET_MEM];
        //Remember last regdpackets we've gotten in case ack is late/dropped
        //and the packet gets resent, so we can ignore it.
fix     MEKH_RESEND_DELAY = F1_0;

int ackdebugmsg=0;

void
mekh_send_direct_packet(ubyte *buffer, int len, int plnum)
{
    //edited 03/04/99 Matt Mueller - new DIRECTDATA sending mode, and modem code.
   if (/*(Players[plnum].connected) && */(plnum!=Player_num)) 
	{
//	    mprintf((0,"mekh_send_direct_packet %i len=%i (%i)\n",buffer[0],len,plnum));
//	    mprintf((0,"*\n"));
	    if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM)){
                    mprintf((0,"sending com data, len=%i (%i)\n",len,buffer[0]));
	  		com_send_data(buffer, len, 0);		//this will be needed for pings and
		  return;				//req'd packets and such to work on serial connects.
	    }
          //edited 03/05/99 Matt Mueller - quick hack for multi_fire.. maybe should have a force_long flag instead?
	    if (Net_D1xPlayer[plnum].iver>=D1X_DIRECTDATA_IVER && buffer[0]!=MULTI_FIRE)
          //end edit -MM
		{
		    mprintf((0,"sending DIRECTDATA to %i, len=%i (%i)\n",plnum,len,buffer[0]));
//		    memset(out_buffer, 0, IPX_MAX_DATA_SIZE);//is this really needed?
		    out_buffer[0]=PID_DIRECTDATA;
                memcpy(&(out_buffer[1]), buffer, len);
		    
		    ipx_send_packet_data( out_buffer, len+1, Netgame.players[plnum].server, Netgame.players[plnum].node, Players[plnum].net_address);
		}
	    else{
		    mprintf((0,"sending concat to %i, len=%i (%i)\n",plnum,len,buffer[0]));
		   mekh_prep_concat_send(buffer, len, Netgame.players[plnum].server, Netgame.players[plnum].node, Players[plnum].net_address, Net_D1xPlayer[plnum].shp);
		}
	}//else
	   // mprintf((0,"&\n"));
    //end edit -MM

//   multi_send_data(buffer, len, 1);
}
//added 03/04/99 Matt Mueller
void
mekh_send_direct_broadcast(ubyte *buffer, int len)
{
    int plnum;
//    mprintf((0,"mekh_send_direct_broadcast %i len=%i\n",buffer[0],len));
    if (mekh_insured_packets[buffer[0]])
	   mekh_send_reg_data_needver(int32_greaterorequal,-1,buffer,len);
    else
	   for (plnum=0;plnum<MAX_NUM_NET_PLAYERS;plnum++)
		  if ((Players[plnum].connected))
			 mekh_send_direct_packet(buffer,len,plnum);
}
//end addition -MM
//added 03/05/99 Matt Mueller - allows packets to be sent only to those that can use them
//Will handle the case of none, one, or both of the packets needing acks
void
mekh_send_broadcast_needver(int ver, ubyte *buf1, int len1, ubyte *buf2,int len2)
{
    int plnum;
    if (mekh_insured_packets[buf1[0]])
	   mekh_send_reg_data_needver(int32_greaterorequal,ver,buf1,len1);
    else
	   for (plnum=0;plnum<MAX_NUM_NET_PLAYERS;plnum++)
		  if ((Players[plnum].connected) && Net_D1xPlayer[plnum].iver>=ver)
			 mekh_send_direct_packet(buf1,len1,plnum);
    
    if (mekh_insured_packets[buf2[0]])
	   mekh_send_reg_data_needver(int32_lessthan,ver,buf2,len2);
    else
	   for (plnum=0;plnum<MAX_NUM_NET_PLAYERS;plnum++)
		  if ((Players[plnum].connected) && Net_D1xPlayer[plnum].iver<ver)
			 mekh_send_direct_packet(buf2,len2,plnum);    
}
//end addition -MM

//======================================================
//Sends an ack
//======================================================
void
mekh_send_ack(mekh_ackedpacket_history p_info)
{
      int dest;
      int tmp;
      ubyte ackpacket[MAX_MULTI_MESSAGE_LEN+4];

      dest = p_info.player_num;
      tmp = 0;      
      ackpacket[tmp]              = MEKH_PACKET_ACK;    tmp++;
      ackpacket[tmp]              = p_info.packet_type; tmp++;
      ackpacket[tmp]              = (ubyte)Player_num;  tmp++;
      *(fix *)&ackpacket[tmp]     = p_info.timestamp;   tmp+=4;

      mekh_send_direct_packet(ackpacket, tmp, dest);

     if(ackdebugmsg)
      hud_message(MSGC_MULTI_INFO, "Sent ack for %d of %d to %s", (int)p_info.timestamp, p_info.packet_type, Players[dest].callsign);
}

//======================================================
//We've received an ack
//Check if it's in mekh_acks list.  If so, unflag that player.
//======================================================
void
mekh_gotack(unsigned char *buf)
{
        int found, i, tmp;
        mekh_ackedpacket_history p_info;

        p_info.packet_type      = buf[1];
        p_info.player_num       = (int)buf[2];
        p_info.timestamp        = *(fix *)&buf[3];

  found = -1;

  for(i=0; i<MEKH_PACKET_MEM; i++)
   {
        if(     (mekh_acks[i].packet_type == p_info.packet_type) &&
                (mekh_acks[i].timestamp == p_info.timestamp))
         {
           mekh_acks[i].players_left[p_info.player_num] = 0;
            if(ackdebugmsg)
             hud_message(MSGC_MULTI_INFO, "Got ack: %d of %d from %s", (int)p_info.timestamp, p_info.packet_type, Players[p_info.player_num].callsign);
           found = i;
            for(tmp=0; tmp<MAX_NUM_NET_PLAYERS; tmp++)
             {
              if ((mekh_acks[i].players_left[tmp]) && (Players[tmp].connected))
               return; //Still have acks waiting from other plyrs
             }
         }
   }

  if (found > -1)
   {
     for(i=found;i<MEKH_PACKET_MEM-1;i++)
      {
       mekh_acks[i] = mekh_acks[i+1]; //drop down everything in list above ack we got in
      }
   }
  else if(ackdebugmsg)//We've got an ack for a message we're not expecting to be acked
     hud_message(MSGC_DEBUG, "Weirdack! %d of %d from %s", (int)p_info.timestamp, p_info.packet_type, Players[p_info.player_num].callsign);
}

//added 03/07/99 Matt Mueller - moved to its own func to avoid duplication
int mekh_prepare_packet(ubyte *packet, ubyte *buf, int len)
{
      int tmp,i;

      tmp = 0;
      packet[tmp]         = (ubyte)MEKH_PACKET_NEEDACK; tmp++;
      packet[tmp]         = (ubyte)Player_num;          tmp++;
      *(fix*)&packet[tmp] = GameTime;                   tmp+=4;
      packet[tmp]         = (ubyte)len;                 tmp++;
      memcpy(packet+tmp, buf, len);                     tmp+=len;

    for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
	   if (mekh_acks[0].players_left[i])break;//if the first ack is empty, use it. -MM
    if (i<MAX_NUM_NET_PLAYERS)
	   for(i=MEKH_PACKET_MEM-1;i>0; i--) {
		 mekh_acks[i] = mekh_acks[i-1]; //Move up
	   }
    else
	   mprintf((0,"first ack empty, so using that\n"));
  
      mekh_acks[0].packet_type = buf[0];
      mekh_acks[0].timestamp = GameTime;
      memcpy(mekh_acks[0].packet_contents, buf, len);
      mekh_acks[0].len = len;
    
    return tmp;
}
//end addition -MM

//added 03/07/99 Matt Mueller - send regdata to a single player
void
mekh_send_direct_reg_data(unsigned char *buf, int len,int plnum)
{
    if (Players[plnum].connected && plnum!=Player_num/* && compare(Net_D1xPlayer[i].iver,ver)*/)
	{
	    int i,tmp;
	    ubyte packet[MAX_MULTI_MESSAGE_LEN+4];
	    tmp=mekh_prepare_packet(packet,buf,len);
	    
	    for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
		   mekh_acks[0].players_left[i] = 0;
	   //edit 04/19/99 Matt Mueller - fixed a rather dumb bug, was using i instead of plnum 
	    if (((Game_mode & GM_NETWORK) && Net_D1xPlayer[plnum].iver<1200)||(!(Game_mode & GM_NETWORK) && Net_D1xPlayer[plnum].iver<1350))
		{
		    if(ackdebugmsg)
			   hud_message(MSGC_MULTI_INFO, "No ackdpkt, %s ! d1x 1.2+", Players[plnum].callsign);
		    mekh_send_direct_packet(buf, len, plnum);
		    mekh_acks[0].players_left[plnum] = 0;
		    //Add newest to top
		}
	    else
		{
		    mekh_acks[0].players_left[plnum] = 1;
		    //Flags player plnum to receive resends
		    //Unflagged when ack comes in from them
		    mprintf((0,"sending req pack to %i, len=%i (%i)\n",plnum,len,buf[0]));
		    mekh_acks[0].send_time[plnum] = GameTime + MEKH_RESEND_DELAY;
		    mekh_send_direct_packet(packet,tmp,plnum);
		}
		//end edit -MM
	}    
}
//end addition -MM

//======================================================
//Sends a packet that needs an ack
//Adds header, sticks it into waitingacks list
//======================================================
//edit 03/05/99 Matt Mueller - compare func to allow packets to be sent only to those that
//can use them, and check to see if first ack is empty before moving up the list.
void
mekh_send_reg_data_needver(compare_int32_func compare,int ver,unsigned char *buf, int len)
{
    int i,tmp;
    ubyte packet[MAX_MULTI_MESSAGE_LEN+4];
    tmp=mekh_prepare_packet(packet,buf,len);
    
      for(i=0;i<MAX_NUM_NET_PLAYERS;i++)
       {
           if ((Players[i].connected) && (i!=Player_num) && compare(Net_D1xPlayer[i].iver,ver))
            {
		    //edited 03/06/99 Matt Mueller - add check for using acks serial games
		    //edited 03/04/99 Matt Mueller - use new iver variable
                //Check if other player is using D1x
                if// ( (   ( (int)Net_D1xPlayer[i].ver[5] - (int)'0' ) * 10 +
                   //      ( (int)Net_D1xPlayer[i].ver[7] - (int)'0' ) ) < 12)
			   (((Game_mode & GM_NETWORK) && Net_D1xPlayer[i].iver<1200)||(!(Game_mode & GM_NETWORK) && Net_D1xPlayer[i].iver<1350))
                 {
		     //end edit -MM
		     //end edit -MM
                                if(ackdebugmsg)
                                 hud_message(MSGC_MULTI_INFO, "No ackdpkt, %s ! d1x 1.2+", Players[i].callsign);
                                mekh_send_direct_packet(buf, len, i);
                                mekh_acks[0].players_left[i] = 0;
                                        //Add newest to top
                 }
                else
                 {
                        mekh_acks[0].players_left[i] = 1;
                                //Flags player i to receive resends
                                //Unflagged when ack comes in from them
			   mprintf((0,"sending req pack to %i, len=%i (%i)\n",i,len,buf[0]));
                        mekh_acks[0].send_time[i] = GameTime + MEKH_RESEND_DELAY;
                        mekh_send_direct_packet(packet,tmp,i);
                 }
            }else
		    mekh_acks[0].players_left[i] = 0;//just making sure -MM
       }

//    if(ackdebugmsg)
//     hud_message(MSGC_MULTI_INFO, "Sent ackreq for %d of %d is %d", (int)GameTime, buf[0], mekh_waitingack);
//    hud_message(MSGC_MULTI_INFO, "Size of packet is %d, total %d", len, tmp);
//    multi_send_data(packet, tmp, repeat);
}
//end edit -MM

//======================================================
//Process incoming message that needs an ack
//Strips off ack header, sends an ack on it and processes the rest
//as a normal incoming message
//======================================================
void
mekh_process_packet(unsigned char *buf) {

      int   len;
      int   tmp;
      mekh_ackedpacket_history p_info;
//edit 03/14/99 Matt Mueller - make it a bit more effecient.. theres still
//a lot in this file that could be prettied up, but this was an easy one :)
//--killed--      ubyte packet[MAX_MULTI_MESSAGE_LEN+4];//don't need it -MM
                                                //buf[0] = MEKH_PACKET_NEEDACK
      p_info.player_num         = (int)buf[1];
      p_info.timestamp          = *(fix *)&buf[2];
                                                //4 bytes
      len                       = (int)buf[6];
      p_info.packet_type        = buf[7];

      for(tmp=MEKH_PACKET_MEM-1; tmp >= 0; tmp--) {
            if ((mekh_packet_history[tmp].player_num == p_info.player_num) &&
                (mekh_packet_history[tmp].timestamp == p_info.timestamp) &&
                (mekh_packet_history[tmp].packet_type == p_info.packet_type))
            {
                mekh_send_ack(p_info);
                 if(ackdebugmsg)
                  hud_message(MSGC_DEBUG, "Ignored AR for %d of %d", (int)p_info.timestamp, p_info.packet_type);
                return; //We've already gotten this packet, so ack it and abort processing
                        //Ack must have been late or dropped
            }

            if (tmp > 0)
                mekh_packet_history[tmp] = mekh_packet_history[tmp-1];
                        //Move each history level up
      }
      mekh_packet_history[0] = p_info;
                        //Insert the newest at the top

       if(ackdebugmsg)
        hud_message(MSGC_MULTI_INFO, "Ackreq for %d of %d from %s(%i)", (int)p_info.timestamp, p_info.packet_type, Players[p_info.player_num].callsign,p_info.player_num);
//    hud_message(MSGC_MULTI_INFO, "Size should be %d, is %d", message_length[p_info.packet_type], len);

//--killed--      memcpy(packet, buf+7, len); //wasteful -MM
         //Copy the contents of the real message into 'packet'

      multi_process_data(/*packet*/buf+7, len);//aaaah, much better -MM
//end edit -MM
         //Feed the message to the rest of Descent's incoming protocols

      mekh_send_ack(p_info);
         //And toss an ack at the player that sent it.  Tada.
} 
  


//======================================================
//Checks if any acks are waiting, if so, sends the oldest to any players
//that haven't acked yet, assuming they're still connected.
//======================================================
void
mekh_resend_needack() {
    int tmp, i, pos, len;
    fix timestamp;
    ubyte packet[MAX_MULTI_MESSAGE_LEN+4];

    pos = -1;
    
    for(i=0; i<MEKH_PACKET_MEM; i++) {
        for(tmp=0;tmp<MAX_NUM_NET_PLAYERS;tmp++) {
                if ((mekh_acks[i].players_left[tmp]==1) && (Players[tmp].connected))
                        pos = i;   //Finds oldest msg that still has ack
                                   //waiting from a live player.
        }
    }

    if (pos == -1) //No acks waiting
        return;

    len       = mekh_acks[pos].len;
    timestamp = mekh_acks[pos].timestamp;

      tmp = 0;
      packet[tmp]         = (ubyte)MEKH_PACKET_NEEDACK; tmp++;
      packet[tmp]         = (ubyte)Player_num;          tmp++;
      *(fix*)&packet[tmp] = timestamp;                  tmp+=4;
      packet[tmp]         = (ubyte)len;                 tmp++;
      memcpy(packet+tmp, mekh_acks[pos].packet_contents, len);     tmp+=len;

      for(i=0;i<MAX_NUM_NET_PLAYERS;i++) {
        if (mekh_acks[pos].players_left[i]==1) {
            if( mekh_acks[pos].send_time[i] < GameTime) {
                        mekh_send_direct_packet(packet, tmp, i);
                         if(ackdebugmsg)
                          hud_message(MSGC_DEBUG, "Resent reqack %d of %d to %s (%i)", (int)timestamp, mekh_acks[pos].packet_contents[0], Players[i].callsign,i);
                        mekh_acks[pos].send_time[i] = GameTime + MEKH_RESEND_DELAY;
            }
        }
      }
}

//======================================================
//Descent only interprets game messages if they're tacked onto
//a movement packet.  That's what this does.  Generates a movement
//packet and tacks on _only_ extradata.
//======================================================
void mekh_prep_concat_send(ubyte *extradata, int len, ubyte *server, ubyte *node, ubyte *address, int short_packet)
{
//Cut & pasted & touched up from netpkt.c

	int loc, tmpi;
	short tmps;
	ushort tmpus;

	object *pl_obj = &Objects[Players[Player_num].objnum];

	loc = 0;
	memset(out_buffer, 0, IPX_MAX_DATA_SIZE);
	if (short_packet == 1) {
		loc = 0;
		out_buffer[loc] = PID_SHORTPDATA; loc++;
		out_buffer[loc] = Player_num; loc++;
		out_buffer[loc] = pl_obj->render_type; loc++;
                out_buffer[loc] = Current_level_num; loc++;
                create_shortpos((shortpos *)(out_buffer + loc), pl_obj);
                loc += 9+2*3+2+2*3; // go past shortpos structure
                *(ushort *)(out_buffer + loc) = len; loc += 2;
                memcpy(out_buffer + loc, extradata, len);
                loc += len;
        } else if (short_packet == 2) {
		loc = 0;
		out_buffer[loc] = PID_PDATA_SHORT2; loc++;
                out_buffer[loc] = MySyncPack.numpackets & 255; loc++;
                create_shortpos((shortpos *)(out_buffer + loc), pl_obj);
                loc += 9+2*3+2+2*3; // go past shortpos structure
                tmpus = len | (Player_num << 12) | (pl_obj->render_type << 15);
                *(ushort *)(out_buffer + loc) = tmpus; loc += 2;
                out_buffer[loc] = Current_level_num; loc++;
                memcpy(out_buffer + loc, extradata, len);
                loc += len;
	} else {
		out_buffer[0] = PID_PDATA;		loc++;	loc += 3;		// skip three for pad byte
		tmpi = swapint(MySyncPack.numpackets);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->pos.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->pos.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->pos.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->orient.rvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.rvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.rvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.uvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->orient.fvec.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.velocity.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.x);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.y);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;
		tmpi = swapint((int)pl_obj->mtype.phys_info.rotvel.z);
		memcpy(&(out_buffer[loc]), &tmpi, 4);	loc += 4;

		tmps = swapshort(pl_obj->segnum);
		memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;
                tmps = swapshort(len);
		memcpy(&(out_buffer[loc]), &tmps, 2);	loc += 2;

		out_buffer[loc] = Player_num; loc++;
		out_buffer[loc] = pl_obj->render_type; loc++;
		out_buffer[loc] = Current_level_num; loc++;
                memcpy(&(out_buffer[loc]), extradata, len);
                loc += len;
	}
		ipx_send_packet_data( out_buffer, loc, server, node, address);
}

#endif



