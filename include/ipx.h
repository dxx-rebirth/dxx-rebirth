/* $Id: ipx.h,v 1.9 2004-08-28 23:17:45 schaffner Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Prototypes for lower-level network routines.
 * This file is called ipx.h and the prefix of these routines is "ipx_"
 * because orignally IPX was the only network driver.
 *
 */

#ifndef _IPX_H
#define _IPX_H

#include "pstypes.h"

// The default socket to use.
#ifdef SHAREWARE
         #define IPX_DEFAULT_SOCKET 0x5110
#else
         #define IPX_DEFAULT_SOCKET 0x5130
#endif

#define IPX_DRIVER_IPX  1 // IPX "IPX driver" :-)
#define IPX_DRIVER_KALI 2
#define IPX_DRIVER_UDP  3 // UDP/IP, user datagrams protocol over the internet
#define IPX_DRIVER_MCAST4 4 // UDP/IP, user datagrams protocol over multicast networks

/* Sets the "IPX driver" (net driver).  Takes one of the above consts as argument. */
extern void arch_ipx_set_driver(int ipx_driver);

#define IPX_INIT_OK              0
#define IPX_SOCKET_ALREADY_OPEN -1
#define IPX_SOCKET_TABLE_FULL   -2
#define IPX_NOT_INSTALLED       -3
#define IPX_NO_LOW_DOS_MEM      -4 // couldn't allocate low dos memory
#define IPX_ERROR_GETTING_ADDR  -5 // error with getting internetwork address

/* returns one of the above constants */
extern int ipx_init(int socket_number);

extern void ipx_close(void);

extern int ipx_change_default_socket( ushort socket_number );

// Returns a pointer to 6-byte address
extern ubyte * ipx_get_my_local_address();
// Returns a pointer to 4-byte server
extern ubyte * ipx_get_my_server_address();

// Determines the local address equivalent of an internetwork address.
void ipx_get_local_target( ubyte * server, ubyte * node, ubyte * local_target );

// If any packets waiting to be read in, this fills data in with the packet data and returns
// the number of bytes read.  Else returns 0 if no packets waiting.
extern int ipx_get_packet_data( ubyte * data );

// Sends a broadcast packet to everyone on this socket.
extern void ipx_send_broadcast_packet_data( ubyte * data, int datasize );

// Sends a packet to a certain address
extern void ipx_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
extern void ipx_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address );

// Sends a packet to everyone in the game
extern int ipx_send_game_packet(ubyte *data, int datasize);

// Initialize and handle the protocol-specific field of the netgame struct.
extern void ipx_init_netgame_aux_data(ubyte data[]);
extern int ipx_handle_netgame_aux_data(const ubyte data[]);
// Handle disconnecting from the game
extern void ipx_handle_leave_game();

#define IPX_MAX_DATA_SIZE (542)		//(546-4)

extern void ipx_read_user_file(char * filename);
extern void ipx_read_network_file(char * filename);

#endif
