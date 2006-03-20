/*

Name:
DRV_SB.C

Description:
Mikmod driver for output on Creative Labs Soundblasters & compatibles
(through DSP)

Portability:

MSDOS:	BC(y)	Watcom(y)	DJGPP(y)
Win95:	n
Os2:	n
Linux:	n

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
//#include <malloc.h>
//#include <conio.h>
#include <string.h>
//#ifndef __DJGPP__
//#include <mem.h>
//#endif

#include "mikmod.h"
#include "mdma.h"
#include "mirq.h"

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> Lowlevel SB stuff <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static UWORD sb_port;          /* sb base port */

/*
	Define some important SB i/o ports:
*/

#define MIXER_ADDRESS 		(sb_port+0x4)
#define MIXER_DATA			(sb_port+0x5)
#define DSP_RESET			(sb_port+0x6)
#define DSP_READ_DATA		(sb_port+0xa)
#define DSP_WRITE_DATA		(sb_port+0xc)
#define DSP_WRITE_STATUS	(sb_port+0xc)
#define DSP_DATA_AVAIL		(sb_port+0xe)


static void SB_MixerStereo(void)
/*
	Enables stereo output for DSP versions 3.00 >= ver < 4.00
*/
{
	outportb(MIXER_ADDRESS,0xe);
	outportb(MIXER_DATA,inportb(MIXER_DATA)|2);
}



static void SB_MixerMono(void)
/*
	Disables stereo output for DSP versions 3.00 >= ver < 4.00
*/
{
	outportb(MIXER_ADDRESS,0xe);
	outportb(MIXER_DATA,inportb(MIXER_DATA)&0xfd);
}



static BOOL SB_WaitDSPWrite(void)
/*
	Waits until the DSP is ready to be written to.

	returns FALSE on timeout
*/
{
	UWORD timeout=32767;

	while(timeout--){
		if(!(inportb(DSP_WRITE_STATUS)&0x80)) return 1;
	}
	return 0;
}



static BOOL SB_WaitDSPRead(void)
/*
	Waits until the DSP is ready to read from.

	returns FALSE on timeout
*/
{
	UWORD timeout=32767;

	while(timeout--){
		if(inportb(DSP_DATA_AVAIL)&0x80) return 1;
	}
	return 0;
}



static BOOL SB_WriteDSP(UBYTE data)
/*
	Writes byte 'data' to the DSP.

	returns FALSE on timeout.
*/
{
	if(!SB_WaitDSPWrite()) return 0;
	outportb(DSP_WRITE_DATA,data);
	return 1;
}



static UWORD SB_ReadDSP(void)
/*
	Reads a byte from the DSP.

	returns 0xffff on timeout.
*/
{
	if(!SB_WaitDSPRead()) return 0xffff;
	return(inportb(DSP_READ_DATA));
}



static void SB_SpeakerOn(void)
/*
	Enables DAC speaker output.
*/
{
	SB_WriteDSP(0xd1);
}



static void SB_SpeakerOff(void)
/*
	Disables DAC speaker output
*/
{
	SB_WriteDSP(0xd3);
}



static void SB_ResetDSP(void)
/*
	Resets the DSP.
*/
{
	int t;
	/* reset the DSP by sending 1, (delay), then 0 */
	outportb(DSP_RESET,1);
	for(t=0;t<8;t++) inportb(DSP_RESET);
	outportb(DSP_RESET,0);
}



static BOOL SB_Ping(void)
/*
	Checks if a SB is present at the current baseport by
	resetting the DSP and checking if it returned the value 0xaa.

	returns: TRUE   => SB is present
			 FALSE  => No SB detected
*/
{
	SB_ResetDSP();
	return(SB_ReadDSP()==0xaa);
}



static UWORD SB_GetDSPVersion(void)
/*
	Gets SB-dsp version. returns 0xffff if dsp didn't respond.
*/
{
	UWORD hi,lo;

	if(!SB_WriteDSP(0xe1)) return 0xffff;

	hi=SB_ReadDSP();
	lo=SB_ReadDSP();

	return((hi<<8)|lo);
}



/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual SB driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

static DMAMEM *SB_DMAMEM;
static char *SB_DMABUF;

static UBYTE SB_TIMECONSTANT;

static UWORD sb_int;           /* interrupt vector that belongs to sb_irq */
static UWORD sb_ver;           /* DSP version number */
static UBYTE sb_irq;           /* sb irq */
static UBYTE sb_lodma;         /* 8 bit dma channel (1.0/2.0/pro) */
static UBYTE sb_hidma;         /* 16 bit dma channel (16/16asp) */
static UBYTE sb_dma;           /* current dma channel */


static BOOL SB_IsThere(void)
{
	char *envptr,c;
	static char *endptr;

	sb_port =0xffff;
	sb_irq  =0xff;
	sb_lodma=0xff;
	sb_hidma=0xff;

	if((envptr=getenv("BLASTER"))==NULL) return 0;

	while(1){

		/* skip whitespace */

		do c=*(envptr++); while(c==' ' || c=='\t');

		/* reached end of string? -> exit */

		if(c==0) break;

		switch(c){

			case 'a':
			case 'A':
				sb_port=strtol(envptr,&endptr,16);
				break;

			case 'i':
			case 'I':
				sb_irq=strtol(envptr,&endptr,10);
                 //added on 11/5/98 by Victor Rachels to hardwire 2->9
                               if(sb_irq==0x02)
                                 sb_irq=0x09;
                 //end this section addition - VR
				break;

			case 'd':
			case 'D':
				sb_lodma=strtol(envptr,&endptr,10);
				break;

			case 'h':
			case 'H':
				sb_hidma=strtol(envptr,&endptr,10);
				break;

			default:
				strtol(envptr,&endptr,16);
				break;
		}
		envptr=endptr;
	}

	if(sb_port==0xffff || sb_irq==0xff || sb_lodma==0xff) return 0;

	/* determine interrupt vector */

	sb_int = (sb_irq>7) ? sb_irq+104 : sb_irq+8;

	if(!SB_Ping()) return 0;

	/* get dsp version. */

	if((sb_ver=SB_GetDSPVersion())==0xffff) return 0;

	return 1;
}


static void interrupt newhandler(MIRQARGS)
{
	if(sb_ver<0x200){
		SB_WriteDSP(0x14);
		SB_WriteDSP(0xff);
		SB_WriteDSP(0xfe);
	}

	if(md_mode & DMODE_16BITS)
		inportb(sb_port+0xf);
	else
		inportb(DSP_DATA_AVAIL);

	MIrq_EOI(sb_irq);
}


static PVI oldhandler;


static BOOL SB_Init(void)
{
	ULONG t;

	if(!SB_IsThere()){
		myerr="No such hardware detected, check your 'BLASTER' env. variable";
		return 0;
	}

/*      printf("SB version %x\n",sb_ver); */
/*      if(sb_ver>0x200) sb_ver=0x200; */

	if(sb_ver>=0x400 && sb_hidma==0xff){
		myerr="High-dma setting in 'BLASTER' variable is required for SB-16";
		return 0;
	}

	if(sb_ver<0x400 && md_mode&DMODE_16BITS){
		/* DSP versions below 4.00 can't do 16 bit sound. */
		md_mode&=~DMODE_16BITS;
	}

	if(sb_ver<0x300 && md_mode&DMODE_STEREO){
		/* DSP versions below 3.00 can't do stereo sound. */
		md_mode&=~DMODE_STEREO;
	}

	/* Use low dma channel for 8 bit, high dma for 16 bit */

	sb_dma=(md_mode & DMODE_16BITS) ? sb_hidma : sb_lodma;

	if(sb_ver<0x400){

		t=md_mixfreq;
		if(md_mode & DMODE_STEREO) t<<=1;

		SB_TIMECONSTANT=256-(1000000L/t);

		if(sb_ver<0x201){
			if(SB_TIMECONSTANT>210) SB_TIMECONSTANT=210;
		}
		else{
			if(SB_TIMECONSTANT>233) SB_TIMECONSTANT=233;
		}

		md_mixfreq=1000000L/(256-SB_TIMECONSTANT);
		if(md_mode & DMODE_STEREO) md_mixfreq>>=1;
	}

	if(!VC_Init()) return 0;

	SB_DMAMEM=MDma_AllocMem(md_dmabufsize);

	if(SB_DMAMEM==NULL){
		VC_Exit();
		myerr="Couldn't allocate page-contiguous dma-buffer";
		return 0;
	}

	SB_DMABUF=(char *)MDma_GetPtr(SB_DMAMEM);

	oldhandler=MIrq_SetHandler(sb_irq,newhandler);
	return 1;
}



static void SB_Exit(void)
{
	MIrq_SetHandler(sb_irq,oldhandler);
	MDma_FreeMem(SB_DMAMEM);
	VC_Exit();
}


static UWORD last=0;
static UWORD curr=0;


static void SB_Update(void)
{
	UWORD todo,index;

	curr=(md_dmabufsize-MDma_Todo(sb_dma))&0xfffc;

	if(curr==last) return;

	if(curr>last){
		todo=curr-last; index=last;
		last+=VC_WriteBytes(&SB_DMABUF[index],todo);
		MDma_Commit(SB_DMAMEM,index,todo);
		if(last>=md_dmabufsize) last=0;
	}
	else{
		todo=md_dmabufsize-last;
		VC_WriteBytes(&SB_DMABUF[last],todo);
		MDma_Commit(SB_DMAMEM,last,todo);
		last=VC_WriteBytes(SB_DMABUF,curr);
		MDma_Commit(SB_DMAMEM,0,curr);
	}
}




static void SB_PlayStart(void)
{
	VC_PlayStart();

	MIrq_OnOff(sb_irq,1);

	if(sb_ver>=0x300 && sb_ver<0x400){
		if(md_mode & DMODE_STEREO){
			SB_MixerStereo();
		}
		else{
			SB_MixerMono();
		}
	}

	/* clear the dma buffer to zero (16 bits
	   signed ) or 0x80 (8 bits unsigned) */

	if(md_mode & DMODE_16BITS)
		memset(SB_DMABUF,0,md_dmabufsize);
	else
		memset(SB_DMABUF,0x80,md_dmabufsize);

	MDma_Commit(SB_DMAMEM,0,md_dmabufsize);

	if(!MDma_Start(sb_dma,SB_DMAMEM,md_dmabufsize,INDEF_WRITE)){
		return;
	}

	if(sb_ver<0x400){
		SB_SpeakerOn();

		SB_WriteDSP(0x40);
		SB_WriteDSP(SB_TIMECONSTANT);

		if(sb_ver<0x200){
			SB_WriteDSP(0x14);
			SB_WriteDSP(0xff);
			SB_WriteDSP(0xfe);
		}
		else if(sb_ver==0x200){
			SB_WriteDSP(0x48);
			SB_WriteDSP(0xff);
			SB_WriteDSP(0xfe);
			SB_WriteDSP(0x1c);
		}
		else{
			SB_WriteDSP(0x48);
			SB_WriteDSP(0xff);
			SB_WriteDSP(0xfe);
			SB_WriteDSP(0x90);
		}
	}
	else{
		SB_WriteDSP(0x41);

		SB_WriteDSP(md_mixfreq>>8);
		SB_WriteDSP(md_mixfreq&0xff);

		if(md_mode & DMODE_16BITS){
			SB_WriteDSP(0xb6);
			SB_WriteDSP((md_mode & DMODE_STEREO) ? 0x30 : 0x10);
		}
		else{
			SB_WriteDSP(0xc6);
			SB_WriteDSP((md_mode & DMODE_STEREO) ? 0x20 : 0x00);
		}

		SB_WriteDSP(0xff);
		SB_WriteDSP(0xef);
	}
}


static void SB_PlayStop(void)
{
	VC_PlayStop();
	SB_SpeakerOff();
	SB_ResetDSP();
	SB_ResetDSP();
	MDma_Stop(sb_dma);
	MIrq_OnOff(sb_irq,0);
}


MDRIVER drv_sb={
	NULL,
	"Soundblaster & compatibles",
	"MikMod Soundblaster Driver v2.1 for 1.0 / 2.0 / Pro / 16",
	SB_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	SB_Init,
	SB_Exit,
	SB_PlayStart,
	SB_PlayStop,
	SB_Update,
	VC_VoiceSetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceSetPanning,
	VC_VoicePlay
};
