/* $Id: hoard.c,v 1.1 2002-08-30 00:55:57 btb Exp $ */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "hoard.h"
#include "cfile.h"

int HoardEquipped()
{
	static int checked=-1;

#ifdef WINDOWS
		return 0;
#endif

	if (checked==-1)
	{
		if (cfexist("hoard.ham"))
			checked=1;
		else
			checked=0;
	}
	return (checked);
}
