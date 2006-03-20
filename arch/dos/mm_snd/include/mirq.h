#ifndef MIRQ_H
#define MIRQ_H

#include "mtypes.h"


#ifdef __WATCOMC__
	#define MIRQARGS void
	typedef void (interrupt far *PVI)(MIRQARGS);
#endif

#ifdef __DJGPP__
	#define MIRQARGS void
	typedef void (*PVI)(MIRQARGS);
#endif

#ifdef __BORLANDC__

	#ifdef __cplusplus
		#define MIRQARGS ...
	#else
		#define MIRQARGS
	#endif

	typedef void interrupt (far *PVI)(MIRQARGS);

#endif


BOOL MIrq_IsEnabled(UBYTE irqno);
BOOL MIrq_OnOff(UBYTE irqno,UBYTE onoff);
PVI  MIrq_SetHandler(UBYTE irqno,PVI handler);
void MIrq_EOI(UBYTE irqno);

#endif
