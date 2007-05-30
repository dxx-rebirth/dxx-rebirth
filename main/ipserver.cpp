/*
 * ipserver.cpp - udp/ip dedicated gamelist server
 * added 2000/02/07 Matt Mueller
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include "ip_base.h"

extern "C"{
#include "error.h"
#include "args.h"
#include "u_mem.h"
}

unsigned char ipx_MyAddress[10];

int ipx_general_PacketReady(int fd) {
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	tv.tv_usec = 50000;//50 ms
	tv.tv_sec = 0;
	if (select(fd + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

void ip_server_mainloop(void){
	struct ipx_recv_data rd;
	int size;
	char buf[1500];
	ip_addr *fromaddr;
//	fix curtime;
	while (1){
		while (arch_ip_PacketReady()) {
			if ((size =	arch_ip_recvfrom(buf, 1500, &rd)) > 4) {
				memcpy(&fromaddr,rd.src_node,sizeof(ip_addr*));
				if (memcmp(buf,D1Xcfgid,4)==0){
					ip_receive_cfg((ubyte*)buf+4,size-4,*fromaddr);
				}
			}
		}
		peer_list.handshake_frame();
//		curtime=timer_get_approx_seconds();
	}
}

void int_handler(int s){
	exit(1);
}

int main(int argc,char **argv){
	error_init(NULL);
	signal(SIGINT,int_handler);//make ctrl-c do cleanup stuff.
	InitArgs(argc,argv);
#ifndef NDEBUG
	if ( FindArg( "-showmeminfo" ) )
		show_mem_info = 1;              // Make memory statistics show
#endif
	myport=UDP_SERV_BASEPORT;
	if(arch_ip_open_socket(myport))
		return 1;
	atexit(arch_ip_close_socket);
	if (ipx_ip_GetMyAddress())
		return 2;
	ip_server_mainloop();
}
