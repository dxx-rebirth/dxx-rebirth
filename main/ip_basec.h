/*
 * $Source: /cvs/cvsroot/d2x/main/ip_basec.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-02-06 09:22:41 $
 *
 * ip_basec.h - interface from cpp to c funcs
 * added 2000/02/07 Matt Mueller
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifndef ___IP_BASEC_H
#define ___IP_BASEC_H

#ifdef __cplusplus
extern "C"{
#endif

#include "pstypes.h"
int ip_sendtoid(ubyte *id,const void *buf,int len);

int get_MAX_PLAYERS(void);
int get_Netgame_player_connected(int pn);
ubyte * get_Netgame_player_node(int pn);

#ifdef __cplusplus
}
#endif

#endif
