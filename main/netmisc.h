/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/



#ifndef _NETMISC_H
#define _NETMISC_H

#include "multi.h"
#include "network.h"

//Returns a checksum of a block of memory.
extern ushort netmisc_calc_checksum( void * vptr, int len );

//Finds the difference between block1 and block2.  Fills in diff_buffer and returns the size of diff_buffer.
extern int netmisc_find_diff( void *block1, void *block2, int block_size, void *diff_buffer );

//Applies diff_buffer to block1 to create a new block1.  Returns the final size of block1.
extern int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size );

#ifdef WORDS_BIGENDIAN

// some mac only routines to deal with incorrectly aligned network structures

void receive_netplayer_info(ubyte *data, AllNetPlayers_info *info);

void receive_netplayers_packet(ubyte *data, AllNetPlayers_info *pinfo);
void send_netplayers_packet(ubyte *server, ubyte *node);

void send_sequence_packet(sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address);
void receive_sequence_packet(ubyte *data, sequence_packet *seq);

void send_netgame_packet(ubyte *server, ubyte *node, ubyte *net_address, int lite_flag);
void receive_netgame_packet(ubyte *data, netgame_info *netgame, int lite_flag);

void swap_object(object *obj);

#endif

#endif
