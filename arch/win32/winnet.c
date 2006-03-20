
#include "types.h"

#include "ipx_drv.h"

extern struct ipx_driver ipx_win;

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	return &ipx_win;
}
