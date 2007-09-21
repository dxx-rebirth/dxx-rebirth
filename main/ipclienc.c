/*
 * ipclienc.h - interface from cpp to c funcs
 * added 2000/02/07 Matt Mueller
 */
#include "ip_basec.h"
#include "ipx.h"
#include "network.h"

void ip_sendtoall(char *buf,int len){
	int j;
	for (j=0;j<MAX_PLAYERS;j++){
		if (NetPlayers.players[j].connected && j!=Player_num){
			ip_sendtoid(NetPlayers.players[j].network.ipx.node,buf,len);
		}
	}
}

int get_MAX_PLAYERS(void){return MAX_PLAYERS;}
int get_Netgame_player_connected(int pn){return NetPlayers.players[pn].connected;}
ubyte * get_Netgame_player_node(int pn){return NetPlayers.players[pn].network.ipx.node;}
