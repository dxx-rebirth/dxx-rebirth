/*
 * $Source: /cvs/cvsroot/d2x/arch/win32/winnet.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 10:52:38 $
 *
 * Win32 net
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/10/19 00:23:56  bradleyb
 * Moved win32_* to win32/ (a la d1x), starting to get net working.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "pstypes.h"

#include "ipx_drv.h"

extern struct ipx_driver ipx_win;

struct ipx_driver * arch_ipx_set_driver(char *arg)
{
	return &ipx_win;
}
