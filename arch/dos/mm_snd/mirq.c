/*

Name:
MDMA.C

Description:
Some general purpose IRQ routines

Portability:

MSDOS:	BC(y)	Watcom(y)	DJGPP(y)
Win95:	n
Os2:	n
Linux:	n

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/
#include <dos.h>
#include <conio.h>
#include "mirq.h"

#define OCR1	0x20			/* 8259-1 Operation control register */
#define IMR1	0x21			/* 8259-1 Mask register */

#define OCR2	0xA0			/* 8259-2 Operation control register */
#define IMR2	0xA1			/* 8259-2 Mask register */


BOOL MIrq_IsEnabled(UBYTE irqno)
/*
	Returns true if the specified hardware irq is enabled.
*/
{
	UBYTE imr=(irqno>7) ? IMR2 : IMR1;		/* interrupt mask register */
	UBYTE msk=1<<(irqno&7);					/* interrupt mask */
	return((inportb(imr) & msk) == 0);
}


BOOL MIrq_OnOff(UBYTE irqno,UBYTE onoff)
/*
	Use to enable or disable the specified irq.
*/
{
	UBYTE imr=(irqno>7) ? IMR2 : IMR1;		/* interrupt mask register */
	UBYTE ocr=(irqno>7) ? OCR2 : OCR1;		/* ocr */
	UBYTE msk=1<<(irqno&7);					/* interrupt mask */
	UBYTE eoi=0x60|(irqno&7);				/* specific end-of-interrupt */
	BOOL oldstate;

	/* save current setting of this irq */
	oldstate=((inportb(imr) & msk) == 0);

	if(onoff){
		outportb(imr,inportb(imr) & ~msk);
		outportb(ocr,eoi);
		if(irqno>7) MIrq_OnOff(2,1);
	}
	else{
		outportb(imr,inportb(imr) | msk);
	}

	return oldstate;
}


void MIrq_EOI(UBYTE irqno)
/*
	Clears the specified interrupt request at the interrupt controller.
*/
{
	UBYTE ocr=(irqno>7) ? OCR2 : OCR1;               /* ocr */
	UBYTE eoi=0x60|(irqno&7);                        /* specific end-of-interrupt */

	outportb(ocr,eoi);
	if(irqno>7) outportb(OCR1,0x20);
}


PVI MIrq_SetHandler(UBYTE irqno,PVI handler)
{
#ifdef __DJGPP__
	_go32_dpmi_seginfo seginfo;
#endif
	PVI oldvect;
	int vecno=(irqno>7) ? irqno+0x68 : irqno+0x8;
#ifdef __DJGPP__
	_go32_dpmi_get_protected_mode_interrupt_vector(vecno, &seginfo);
	oldvect = (PVI)seginfo.pm_offset;
	seginfo.pm_offset = (int)handler;
	seginfo.pm_selector = _go32_my_cs();
	_go32_dpmi_allocate_iret_wrapper(&seginfo);
	_go32_dpmi_set_protected_mode_interrupt_vector(vecno, &seginfo);
#else
	oldvect=_dos_getvect(vecno);
	_dos_setvect(vecno,handler);
#endif
	return oldvect;
}
