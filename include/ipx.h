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
 * $Source: /cvs/cvsroot/d2x/include/ipx.h,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2002-02-13 10:42:22 $
 * 
 * Prototype for IPX communications.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/10/19 00:31:51  bradleyb
 * Trying to get network working on win32
 *
 * Revision 1.1.1.1  2001/01/19 03:30:16  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.1.1.1  1999/06/14 22:02:16  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.6  1995/03/29  11:19:32  john
 * Added broadcasting over a net.
 * 
 * Revision 2.5  1995/03/28  20:04:43  john
 * Took away alternate server stuff.
 * 
 * Revision 2.4  1995/03/23  19:00:10  john
 * Added user list capabitly.
 * 
 * Revision 2.3  1995/03/23  12:26:57  john
 * Move IPX into bios lib.
 * 
 * Revision 2.2  1995/03/22  19:08:14  john
 * Added code to fix sending packets over router... now
 * we just need to make broadcasts go over router!!
 * 
 * Revision 2.1  1995/03/21  08:39:56  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.0  1995/02/27  11:30:16  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.16  1995/02/16  17:34:52  john
 * Added code to allow dynamic socket changing.
 * 
 * Revision 1.15  1995/01/04  21:43:27  rob
 * Remove SPX size definition.
 * 
 * Revision 1.14  1995/01/03  13:46:18  john
 * Added code that should make ipx work over different servers,
 * but ifdef'd it out with SHAREWARE in ipx.c.  I haven't tested
 * this, and I hope it doesn't introduce net bugs.
 * 
 * Revision 1.13  1994/11/02  11:37:16  rob
 * Changed default socket number to a higher regions.
 * 
 * Revision 1.12  1994/11/01  15:56:51  rob
 * Added defines for SPX socketsx.
 * 
 * Revision 1.11  1994/10/31  19:23:31  rob
 * Added a prototype for the new object send function.
 * 
 * Revision 1.10  1994/09/07  13:37:25  john
 * Changed default socket to 0x4000, because 
 * the ipx/spx book says that we can only use
 * sockets 0x4000 - 0x7fff.
 * 
 * Revision 1.9  1994/08/25  18:14:45  matt
 * Changed socket because of packet change
 * 
 * Revision 1.8  1994/08/12  22:42:24  john
 * Took away Player_stats; added Players array.
 * 
 * Revision 1.7  1994/08/09  19:31:47  john
 * Networking changes.
 * 
 * Revision 1.6  1994/08/05  16:11:46  john
 * Psuedo working version of networking.
 * 
 * Revision 1.5  1994/08/04  19:17:20  john
 * Inbetween version of network stuff.
 * 
 * Revision 1.4  1994/07/29  16:08:59  john
 * *** empty log message ***
 * 
 * Revision 1.3  1994/07/25  12:33:22  john
 * Network "pinging" in.
 * 
 * Revision 1.2  1994/07/20  15:58:29  john
 * First installment of ipx stuff.
 * 
 * Revision 1.1  1994/07/19  15:43:05  john
 * Initial revision
 * 
 * 
 */

#ifndef _IPX_H
#define _IPX_H

// The default socket to use.
#ifdef SHAREWARE
         #define IPX_DEFAULT_SOCKET 0x5110
#else
         #define IPX_DEFAULT_SOCKET 0x5130
#endif

//---------------------------------------------------------------
// Initializes all IPX internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//				-1 if socket already open.
//				-2	if socket table full.
//				-3 if IPX not installed.
//				-4 if couldn't allocate low dos memory
//				-5 if error with getting internetwork address
extern int ipx_init( int socket_number);

//select which "ipx" driver to use.
extern int ipx_set_driver(char *name);

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

#define IPX_MAX_DATA_SIZE (542)		//(546-4)

extern void ipx_read_user_file(char * filename);
extern void ipx_read_network_file(char * filename);

#endif

 
