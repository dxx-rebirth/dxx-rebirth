/* reconfig.c  -  ingame reconfig by Victor Rachels */
//added 06/09/99 Matt Mueller - fix nonetwork compile
#ifdef NETWORK
//end addition -MM

#include <string.h>

#include "types.h"
#include "byteswap.h"
#include "network.h"
#include "mlticntl.h"
#include "mono.h"
#include "multi.h"
#include "player.h"

#include "hudmsg.h"

#include "reconfig.h"

#include "error.h"

void reconfig_send_gameinfo()
{
	//added 06/09/99 Matt Mueller - "fix" shareware compile
#ifndef SHAREWARE
	//end addition -MM
	int i=0;
	int32_t tmpi=0;
	ubyte r_packet[3+64];

	r_packet[i] = (ubyte)MULTI_INGAME_CONFIG;          i++;
	r_packet[i] = (ubyte)Player_num;                   i++;

	r_packet[i] = (ubyte)RECONFIG_VERSION;             i++;
	r_packet[i] = (ubyte)Netgame.gamemode;             i++;

	r_packet[i] = (ubyte)Netgame.game_flags;           i++;

	r_packet[i] = (ubyte)Netgame.difficulty;           i++;

	r_packet[i] = (ubyte)restrict_mode;                i++;

	r_packet[i] = (ubyte)Netgame.packets_per_sec;      i++;

	r_packet[i] = (ubyte)Netgame.protocol_version;     i++;

	r_packet[i] = (ubyte)Netgame.max_numplayers;       i++;

	tmpi=swapint(Netgame.control_invul_time);
	memcpy(&(r_packet[i]),&tmpi,4);                    i+=4;

	tmpi=swapint(Netgame.flags);
	memcpy(&(r_packet[i]),&tmpi,4);                    i+=4;

	mprintf((0,"sending reconfig (l%i,v%i) ",i,RECONFIG_VERSION));
	mprintf((0,"GAMEMODE:%i",Netgame.gamemode));
	mprintf((0,"GAME_FLAGS:%i",Netgame.game_flags));
	mprintf((0,"DIFFICULTY:%i",Netgame.difficulty));
	mprintf((0,"RESTRICT_MODE:%i",restrict_mode));
	mprintf((0,"PPS:%i",Netgame.packets_per_sec));
	mprintf((0,"PROTOCOL_VERSION:%i",Netgame.protocol_version));
	mprintf((0,"MAX_NUMPLAYERS:%i",Netgame.max_numplayers));
	mprintf((0,"CONTROL_INVUL_TIME:%i",Netgame.control_invul_time));
	mprintf((0,"FLAGS:%i",Netgame.flags));
	if (i>message_length[MULTI_INGAME_CONFIG])
		Error("reconfig_send_gameinfo: packet too long\n");
	
	memset(r_packet+i,0,message_length[MULTI_INGAME_CONFIG]-i);//set the rest to zero, so it'll compress easily
	mekh_send_direct_broadcast(r_packet,message_length[MULTI_INGAME_CONFIG]);
	//added 06/09/99 Matt Mueller - "fix" shareware compile
#endif
	//end addition -MM
}

void reconfig_receive(ubyte *values,int len)
{
	//added 06/09/99 Matt Mueller - "fix" shareware compile
#ifndef SHAREWARE
	//end addition -MM
	int i=2,version;
	int32_t t;

	version=values[i++];
	hud_message(MSGC_GAME_FEEDBACK,"Received reconfig from %i (v%i)",values[1],version);
	
	if (version < 128){//thinking ahead a bit, just in case we want to make some new additions we can still have them work on older versions of d1x, but we can also make new reconfig types that don't have any of the old stuff.
		Netgame.gamemode = values[i++];
		Netgame.game_flags = values[i++];
		Netgame.difficulty = values[i++];
		restrict_mode = values[i++];
		Netgame.packets_per_sec = values[i++];
		Netgame.protocol_version = values[i++];
		Netgame.max_numplayers = values[i++];
		memcpy(&t,&(values[i]),sizeof(t)); i+= sizeof(t);
		Netgame.control_invul_time = swapint(t);
		memcpy(&t,&(values[i]),sizeof(t)); i+= sizeof(t);
		Netgame.flags = swapint(t);
	}

//	if (version==?) then process_additional_info
	
	mprintf((0,"GAMEMODE:%i",Netgame.gamemode));
	mprintf((0,"GAME_FLAGS:%i",Netgame.game_flags));
	mprintf((0,"DIFFICULTY:%i",Netgame.difficulty));
	mprintf((0,"RESTRICT_MODE:%i",restrict_mode));
	mprintf((0,"PPS:%i",Netgame.packets_per_sec));
	mprintf((0,"PROTOCOL_VERSION:%i",Netgame.protocol_version));
	mprintf((0,"MAX_NUMPLAYERS:%i",Netgame.max_numplayers));
	mprintf((0,"CONTROL_INVUL_TIME:%i",Netgame.control_invul_time));
	mprintf((0,"FLAGS:%i",Netgame.flags));
	//added 06/09/99 Matt Mueller - "fix" shareware compile
#endif
	//end addition -MM
}
//added 06/09/99 Matt Mueller - fix nonetwork compile
#endif
//end addition -MM
