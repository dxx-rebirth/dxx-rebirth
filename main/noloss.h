#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "strutil.h"
#include "pstypes.h"
#include "netpkt.h"
#include "network.h"
#include "game.h"
#include "multi.h"
#include "netpkt.h"
#include "netdrv.h"
#include "byteswap.h"

#define NOLOSS_QUEUE_SIZE	512 // Store up to 512 packets

// the structure list keeping the data we may want to resend
// this does only contain the extra data field of a PDATA packet (if that isn't enough, the whole PDATA struct info still could be added later)
typedef struct pdata_noloss_store {
	int 		used;
	fix			pkt_initial_timestamp;		// initial timestamp to see if packet is outdated
	fix			pkt_timestamp;				// Packet timestamp
	int			pkt_num;					// Packet number
	ubyte		player_ack[MAX_PLAYERS]; 	// 0 if player has not ACK'd this packet, 1 if ACK'd or not connected
	int 		N_players;					// Save N_players, too - so if a player disconnects we will resend to make sure all acks are related to the right players
	char        data[NET_XDATA_SIZE];		// extra data of a packet - contains all multibuf data we don't want to loose
	ushort		data_size;
} __pack__ pdata_noloss_store;

// ACK signal packet
typedef struct noloss_ack {
	ubyte		type;
	ubyte		sender_pnum;
	ubyte		receiver_pnum;
	int 		pkt_num;
} __pack__ noloss_ack;

void noloss_add_packet_to_queue(int urgent, int pkt_num, char *data, ushort data_size);
void noloss_send_ack(int pkt_num, ubyte receiver_pnum);
void noloss_got_ack(ubyte *data);
void noloss_init_queue(void);
void noloss_process_queue(void);
