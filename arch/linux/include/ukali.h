/* $Id: ukali.h,v 1.6 2003-03-13 00:20:21 btb Exp $ */
/*
 *
 * Header for kali support functions
 *
 */

#ifndef __UKALI_H__
#define __UKALI_H__

//added on 10/04/98 by Matt Mueller to show correct ver in kali
#include "vers_id.h"
//end addition -MM

// 4213 is the port that KaliNix is listening on
//
//		char code; // 1 == open, 2 == close, 3 == data, 5 == GetMyAddr
//      acks       // 6 == open, 7 == close				4 == GetMyAddr

// net data packets structure for send/recv
// struct {
//		char	code; == 3
//		char	sa_nodenum[6];
//		char	dport[2];
//		char	sport[2];
//		char data[];
// }

// net data packets for open/close socket
//
// process_name is a null terminated 8byte string
// struct {
//		char code; // 1/6 == open, 2/7 == close
//		char socket[2];
//		char pid[4];
//		char	process_name[9];
// }

// net myaddress struct which is returned after the GetMyAddress call
// struct {
//		char	code; == 4
//		char	sa_nodenum[6];
// }

// net data for GetMyAddress call
// struct {
//		char	code; == 5
// }

typedef struct kaliaddr_ipx_tag {
    short sa_family;
    char  sa_netnum[4];
    char  sa_nodenum[6];
    unsigned short sa_socket;
} kaliaddr_ipx;

// Process name that shows up in /whois and /games list.
// Maximum of 8 characters.
//edited on 10/04/98 by Matt Mueller to show correct ver in kali
#define KALI_PROCESS_NAME "D2X" VERSION
//end edit -MM
#define MAX_PACKET_SIZE 1500

// struct ipx_helper ipx_kali = {
//	ipx_kali_GetMyAddress,
//	ipx_kali_OpenSocket,
//	ipx_kali_CloseSocket,
//	ipx_kali_SendPacket,
//	ipx_kali_ReceivePacket,
//	ipx_general_PacketReady
// };

int KaliGetNodeNum(kaliaddr_ipx *myaddr);
int KaliOpenSocket(unsigned short port);
int KaliCloseSocket(int hand);
int KaliSendPacket(int hand, char *data, int len, kaliaddr_ipx *to);
int KaliReceivePacket(int hand, char *data, int len, kaliaddr_ipx *from);

#endif // __UKALI_H__
