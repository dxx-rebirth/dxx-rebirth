/*
 * $Source: /cvs/cvsroot/d2x/arch/linux/linuxnet.c,v $
 * $Revision: 1.5 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 07:39:26 $
 *
 * Linux net
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2001/10/19 07:29:37  bradleyb
 * Brought linux networking in line with d1x, moved some arch/linux_* stuff to arch/linux/
 *
 * Revision 1.3  2001/01/29 13:35:09  bradleyb
 * Fixed build system, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "pstypes.h"

#include "ipx_drv.h"
#include "ipx_bsd.h"
#include "ipx_kali.h"
#include "ipx_udp.h"

#include <string.h>

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
