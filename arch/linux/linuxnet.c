
#include "types.h"
#include <string.h>

#include "ipx_drv.h"
#include "ipx_bsd.h"
#include "ipx_kali.h"
#include "ipx_udp.h"

extern int Network_DOS_compability;

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	if (strcmp(arg,"kali")==0)
	{
		Network_DOS_compability=1;
		return &ipx_kali;
	}
	else if (strcmp(arg,"ip")==0)
	{
		Network_DOS_compability=0;
		return &ipx_ip;
	}
	else
	{
		Network_DOS_compability=0;
		return &ipx_bsd;
	}
}
