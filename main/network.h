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

/*
 * $Source: /cvs/cvsroot/d2x/main/network.h,v $
 * $Revision: 1.6 $
 * $Author: bradleyb $
 * $Date: 2002-02-14 09:24:19 $
 *
 * FIXME: put description here
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2002/02/13 10:39:22  bradleyb
 * Lotsa networking stuff from d1x
 *
 * Revision 1.4  2001/10/25 02:15:57  bradleyb
 * conditionalize including multi.h and network.h, fix backslashes
 *
 *
 */

#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
#include "multi.h"
#include "newmenu.h"

#define NETSTAT_MENU					0
#define NETSTAT_PLAYING				1
#define NETSTAT_BROWSING			2
#define NETSTAT_WAITING				3
#define NETSTAT_STARTING			4
#define NETSTAT_ENDLEVEL			5

#define CONNECT_DISCONNECTED		0
#define CONNECT_PLAYING				1
#define CONNECT_WAITING				2
#define CONNECT_DIED_IN_MINE		3
#define CONNECT_FOUND_SECRET		4
#define CONNECT_ESCAPE_TUNNEL		5
#define CONNECT_END_MENU			6

#define MAX_ACTIVE_NETGAMES                     12

#define NETGAMEIPX                              1
#define NETGAMETCP                              2

// defines and other things for appletalk/ipx games on mac

#define IPX_GAME		1
#define APPLETALK_GAME	2
#ifdef MACINTOSH
extern int Network_game_type;
#else
#define Network_game_type IPX_GAME
#endif

typedef struct sequence_packet {
	ubyte					type;
	int 					Security;
   ubyte pad1[3];
	netplayer_info		player;
} __pack__ sequence_packet;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//		bytes on byte boundries
//		shorts on even byte boundries
//		ints on even byte boundries

typedef struct frame_info {
	ubyte				type;						// What type of packet
	ubyte				pad[3];					// Pad out length of frame_info packet
	int				numpackets;			
	vms_vector		obj_pos;
	vms_matrix		obj_orient;
	vms_vector		phys_velocity;
	vms_vector		phys_rotvel;
	short				obj_segnum;
	ushort			data_size;		// Size of data appended to the net packet
	ubyte				playernum;
	ubyte				obj_render_type;
	ubyte				level_num;
	ubyte				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} __pack__ frame_info;

// short_frame_info is not aligned -- 01/18/96 -- MWA
// won't align because of shortpos.  Shortpos needs
// to stay in current form.

typedef struct short_frame_info {
	ubyte				type;						// What type of packet
	ubyte				pad[3];					// Pad out length of frame_info packet
	int				numpackets;			
	shortpos			thepos;
	ushort			data_size;		// Size of data appended to the net packet
	ubyte				playernum;
	ubyte				obj_render_type;
	ubyte				level_num;
	ubyte				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} __pack__ short_frame_info;

void network_start_game();
void network_join_game();
void network_rejoin_game();
void network_leave_game();
int network_endlevel(int *secret);
void network_endlevel_poll2( int nitems, struct newmenu_item * menus, int * key, int citem );

int network_level_sync();
void network_send_endlevel_packet();

int network_delete_extra_objects();
int network_find_max_net_players();
int network_objnum_is_past(int objnum);
char * network_get_player_name( int objnum );
void network_send_endlevel_sub(int player_num);

void network_disconnect_player(int playernum);

extern int NetGameType;
extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern fix LastPacketTime[MAX_PLAYERS];

extern ushort my_segments_checksum;

extern int Network_initial_pps;
extern int Network_initial_shortpackets;

// By putting an up-to-20-char-message into Network_message and 
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void network_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void network_send_data( ubyte * ptr, int len, int urgent );

void network_send_objects(void);
void network_dump_player(ubyte * server, ubyte *node, int why);
void network_send_game_info(sequence_packet *their);

int GetMyNetRanking();
int HoardEquipped();

#define PID_LITE_INFO                           43
#define PID_SEND_ALL_GAMEINFO                   44
#define PID_PLAYERSINFO                         45
#define PID_REQUEST                             46
#define PID_SYNC                                47
#define PID_PDATA                               48
#define PID_ADDPLAYER                           49
#define PID_DUMP                                51
#define PID_ENDLEVEL                            52
#define PID_QUIT_JOINING                        54
#define PID_OBJECT_DATA                         55
#define PID_GAME_LIST                           56
#define PID_GAME_INFO                           57
#define PID_PING_SEND                           58
#define PID_PING_RETURN                         59
#define PID_GAME_UPDATE                         60
#define PID_ENDLEVEL_SHORT                      61
#define PID_NAKED_PDATA                         62
#define PID_GAME_PLAYERS                        63
#define PID_NAMES_RETURN                        64

#define PID_SHORTPDATA                          42
#define PID_D2X_GAME_INFO_REQ                   65
#define PID_D2X_GAME_LITE                       68
#define PID_PDATA_SHORT2                        70

#define NETGAME_ANARCHY                         0
#define NETGAME_TEAM_ANARCHY                    1
#define NETGAME_ROBOT_ANARCHY                   2
#define NETGAME_COOPERATIVE                     3
#define NETGAME_CAPTURE_FLAG                    4
#define NETGAME_HOARD                           5
#define NETGAME_TEAM_HOARD                      6

typedef struct endlevel_info {
	ubyte                                   type;
	ubyte                                   player_num;
	byte                                    connected;
	ubyte                                   seconds_left;
	short                                   kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
	short                                   kills;
	short                                   killed;
} __pack__ endlevel_info;

#endif
