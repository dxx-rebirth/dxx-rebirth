/*
 * $Source: /cvs/cvsroot/d2x/arch/win32/winnet.c,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 00:23:56 $
 *
 * Win32 net
 *
 * $Log: not supported by cvs2svn $
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
