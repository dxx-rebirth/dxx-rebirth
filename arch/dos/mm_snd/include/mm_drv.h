#ifndef _MM_DRV
#define _MM_DRV
#include <stdio.h>

#if 1
#include "mtypes.h"
#else
typedef unsigned char UBYTE;
typedef signed char SBYTE;
typedef int BOOL;
typedef int SWORD;
typedef unsigned int UWORD;
typedef unsigned int ULONG;
typedef int SLONG;
#endif
typedef char CHAR;
typedef int SAMPLOAD;
typedef int SAMPLE;

#if 1
typedef struct MDRIVER{
	struct MDRIVER *next;
	char    *Name;
	char    *Version;
	BOOL    (*IsPresent)            (void);
	SWORD   (*SampleLoad)           (FILE *fp,ULONG size,ULONG reppos,ULONG repend,UWORD flags);
	void    (*SampleUnLoad)         (SWORD handle);
	BOOL    (*Init)                 (void);
	void    (*Exit)                 (void);
	void    (*PlayStart)            (void);
	void    (*PlayStop)             (void);
	void    (*Update)               (void);
	void 	(*VoiceSetVolume)		(UBYTE voice,UBYTE vol);
	void 	(*VoiceSetFrequency)	(UBYTE voice,ULONG frq);
	void 	(*VoiceSetPanning)		(UBYTE voice,UBYTE pan);
	void	(*VoicePlay)			(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags);
} MDRIVER;
#else
typedef struct MDRIVER
{   struct MDRIVER *next;
    CHAR    *Name;
    CHAR    *Version;
    UBYTE   HardVoiceLimit,       /* Limit of hardware mixer voices for this driver */
            SoftVoiceLimit;       /* Limit of software mixer voices for this driver */

    BOOL    (*IsPresent)          (void);
    SWORD   (*SampleLoad)         (SAMPLOAD *s, int type, FILE *fp);
    void    (*SampleUnLoad)       (SWORD handle);
    ULONG   (*FreeSampleSpace)    (int type);
    ULONG   (*RealSampleLength)   (int type, SAMPLE *s);
    BOOL    (*Init)               (void);
    void    (*Exit)               (void);
    BOOL    (*Reset)              (void);
    BOOL    (*SetNumVoices)       (void);
    BOOL    (*PlayStart)          (void);
    void    (*PlayStop)           (void);
    void    (*Update)             (void);
    void    (*VoiceSetVolume)     (UBYTE voice, UWORD vol);
    void    (*VoiceSetFrequency)  (UBYTE voice, ULONG frq);
    void    (*VoiceSetPanning)    (UBYTE voice, ULONG pan);
    void    (*VoicePlay)          (UBYTE voice, SWORD handle, ULONG start, ULONG size, ULONG reppos, ULONG repend, UWORD flags);
    void    (*VoiceStop)          (UBYTE voice);
    BOOL    (*VoiceStopped)       (UBYTE voice);
    void    (*VoiceReleaseSustain)(UBYTE voice);
    SLONG   (*VoiceGetPosition)   (UBYTE voice);
    ULONG   (*VoiceRealVolume)    (UBYTE voice);

    BOOL    (*StreamInit)         (ULONG speed, UWORD flags);
    void    (*StreamExit)         (void);
    void    (*StreamSetSpeed)     (ULONG speed);
    SLONG   (*StreamGetPosition)  (void);
    void    (*StreamLoadFP)       (FILE *fp);
    void	(*Wait)               (void);		
} MDRIVER;
#endif

extern BOOL    VC_Init(void);
extern void    VC_Exit(void);
extern BOOL    VC_SetNumVoices(void);
extern ULONG   VC_SampleSpace(int type);
extern ULONG   VC_SampleLength(int type, SAMPLE *s);

extern BOOL    VC_PlayStart(void);
extern void    VC_PlayStop(void);

#if 1
extern SWORD   VC_SampleLoad(FILE *fp,ULONG size,ULONG reppos,ULONG repend,UWORD flags);
#else
extern SWORD   VC_SampleLoad(SAMPLOAD *sload, int type, FILE *fp);
#endif
extern void    VC_SampleUnload(SWORD handle);

extern void    VC_WriteSamples(SBYTE *buf,ULONG todo);
extern ULONG   VC_WriteBytes(SBYTE *buf,ULONG todo);
extern void    VC_SilenceBytes(SBYTE *buf,ULONG todo);

#if 1
extern void    VC_VoiceSetVolume(UBYTE voice, UBYTE vol);
extern void    VC_VoiceSetPanning(UBYTE voice, UBYTE pan);
#else
extern void    VC_VoiceSetVolume(UBYTE voice, UWORD vol);
extern void    VC_VoiceSetPanning(UBYTE voice, ULONG pan);
#endif
extern void    VC_VoiceSetFrequency(UBYTE voice, ULONG frq);
extern void    VC_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags);

extern void    VC_VoiceStop(UBYTE voice);
extern BOOL    VC_VoiceStopped(UBYTE voice);
extern void    VC_VoiceReleaseSustain(UBYTE voice);
extern SLONG   VC_VoiceGetPosition(UBYTE voice);
extern ULONG   VC_VoiceRealVolume(UBYTE voice);

extern int md_mode;
extern int md_mixfreq;
extern int md_dmabufsize;
extern char *myerr;

#define DMODE_16BITS 1
#define DMODE_STEREO 2

char *_mm_malloc(int size);
void _mm_free(char *buf);

#endif
