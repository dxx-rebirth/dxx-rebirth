
#include "types.h"
#include "ipx_drv.h"

extern struct ipx_driver ipx_win;

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	if (strcmp(arg,"ip")==0)
		return &ipx_ip;
	else
		return &ipx_win;
}
