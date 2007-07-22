
#include "types.h"

#include "ipx_drv.h"

extern struct ipx_driver ipx_win;
extern int Network_DOS_compability;

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	if (strcmp(arg,"ip")==0)
	{
		Network_DOS_compability=0;
		return &ipx_ip;
	}
	else
	{
		Network_DOS_compability=1;
		return &ipx_win;
	}
}
