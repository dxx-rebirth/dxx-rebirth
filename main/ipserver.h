/*
 * ipserver.h - udp/ip standalone server
 * added 2000/02/07 Matt Mueller
 */
#ifndef __IPSERVER_H_
#define __IPSERVER_H_

#include "ip_base.h"

class game_info {
	public:
		ip_id id;
		char *name;
		int players;
		int maxplayers;
		char *mission;
		int level;
		int gamemode;
		int status;
};

class serv_generic_info {
	public:
		ip_id id;
		char 
};

#endif
