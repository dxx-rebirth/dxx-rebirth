#ifndef MDMA_H
#define MDMA_H

#include "mtypes.h"

#define READ_DMA                0
#define WRITE_DMA               1
#define INDEF_READ              2
#define INDEF_WRITE             3

#ifdef __WATCOMC__

typedef struct{
	void *continuous;	/* the pointer to a page-continous dma buffer */
	UWORD raw_selector;	/* the raw allocated dma selector */
} DMAMEM;

#elif defined(__DJGPP__)

typedef struct{
		void *continuous;       /* the pointer to a page-continous dma buffer */
		_go32_dpmi_seginfo raw; /* points to the memory that was allocated */
} DMAMEM;

#else

typedef struct{
	void *continuous;	/* the pointer to a page-continous dma buffer */
	void *raw;			/* points to the memory that was allocated */
} DMAMEM;

#endif

DMAMEM *MDma_AllocMem(UWORD size);
void    MDma_FreeMem(DMAMEM *dm);
int     MDma_Start(int channel,DMAMEM *dm,UWORD size,int type);
void    MDma_Stop(int channel);
void   *MDma_GetPtr(DMAMEM *dm);
void    MDma_Commit(DMAMEM *dm,UWORD index,UWORD count);
UWORD   MDma_Todo(int channel);

#endif
