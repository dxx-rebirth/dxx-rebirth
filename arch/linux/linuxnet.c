
#include "types.h"
#include <string.h>

#include "ipx_drv.h"
#include "ipx_bsd.h"
#include "ipx_kali.h"
#include "ipx_udp.h"

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	if (strcmp(arg,"kali")==0){
		return &ipx_kali;
#if 0
	}else if (strcmp(arg,"udp")==0){
		return &ipx_udp;
#endif
	}else {
		return &ipx_bsd;
	}
}
