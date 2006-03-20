/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/



#ifndef _MIDIFILE_H
#define _MIDIFILE_H

#include "global.h" 

typedef DWORD SMFRESULT;
typedef DWORD TICKS;
typedef TICKS FAR *PTICKS;
typedef BYTE HUGE *HPBYTE;

#define MAX_TICKS           ((TICKS)0xFFFFFFFFL)

#define SMF_SUCCESS         (0L)
#define SMF_INVALID_FILE    (1L)
#define SMF_NO_MEMORY       (2L)
#define SMF_OPEN_FAILED     (3L)
#define SMF_INVALID_TRACK   (4L)
#define SMF_META_PENDING    (5L)
#define SMF_ALREADY_OPEN    (6L)
#define SMF_END_OF_TRACK    (7L)
#define SMF_NO_META         (8L)
#define SMF_INVALID_PARM    (9L)
#define SMF_INVALID_BUFFER  (10L)
#define SMF_END_OF_FILE     (11L)
#define SMF_REACHED_TKMAX   (12L)

DECLARE_HANDLE(HSMF);

typedef struct tag_smfopenstruct
{
    LPSTR               pstrName;
    DWORD               dwTimeDivision;
    HSMF                hSmf;
}   SMFOPENFILESTRUCT,
    FAR *PSMFOPENFILESTRUCT;

extern SMFRESULT FNLOCAL smfOpenFile(
    BYTE *data, UINT length, HSMF *pph);

extern SMFRESULT FNLOCAL smfCloseFile(
    HSMF                hsmf);

typedef struct tag_smffileinfo
{
    DWORD               dwTracks;
    DWORD               dwFormat;
    DWORD               dwTimeDivision;
    TICKS               tkLength;
}   SMFFILEINFO,
    FAR *PSMFFILEINFO;

extern SMFRESULT FNLOCAL smfGetFileInfo(
    HSMF                hsmf,
    PSMFFILEINFO        psfi);

extern DWORD FNLOCAL smfTicksToMillisecs(
    HSMF                hsmf,
    TICKS               tkOffset);

extern DWORD FNLOCAL smfMillisecsToTicks(
    HSMF                hsmf,
    DWORD               msOffset);

extern SMFRESULT FNLOCAL smfReadEvents(
    HSMF                hsmf,
    LPMIDIHDR           lpmh,
    TICKS               tkMax);

extern SMFRESULT FNLOCAL smfSeek(
    HSMF                hsmf,
    TICKS               tkPosition,
    LPMIDIHDR           lpmh);

extern DWORD FNLOCAL smfGetStateMaxSize(
    void);

/* Buffer described by LPMIDIHDR is in polymsg format, except that it
** can contain meta-events (which will be ignored during playback by
** the current system). This means we can use the pack functions, etc.
*/
#define PMSG_META       ((BYTE)0xC0)




/*****************************************************************************/
/*****************************************************************************/


/* Handle structure for HSMF
*/ 

#define SMF_TF_EOT          0x00000001L
#define SMF_TF_INVALID      0x00000002L

typedef struct tag_tempomapentry
{
    TICKS           tkTempo;           
    DWORD           msBase;            
    DWORD           dwTempo;           
}   TEMPOMAPENTRY,
    *PTEMPOMAPENTRY;

typedef struct tag_smf *PSMF;

typedef struct tag_track
{
    PSMF            pSmf;

    DWORD           idxTrack;          
    
    TICKS           tkPosition;        
    DWORD           cbLeft;            
    HPBYTE          hpbImage;          
    BYTE            bRunningStatus;    
    
    DWORD           fdwTrack;          

    struct
    {
        TICKS       tkLength;
        DWORD       cbLength;
    }
    smti;                              

}   TRACK,
    *PTRACK;

#define SMF_F_EOF               0x00000001L
#define SMF_F_INSERTSYSEX       0x00000002L

#define C_TEMPO_MAP_CHK     16
typedef struct tag_smf
{
    char            szName[128];
    HPBYTE          hpbImage;
    DWORD           cbImage;
    HTASK           htask;

    TICKS           tkPosition;
    TICKS           tkLength;
    DWORD           dwFormat;
    DWORD           dwTracks;
    DWORD           dwTimeDivision;
    DWORD           fdwSMF;

    DWORD           cTempoMap;
    DWORD           cTempoMapAlloc;
    HLOCAL          hTempoMap;
    PTEMPOMAPENTRY  pTempoMap;

    DWORD           dwPendingUserEvent;
    DWORD           cbPendingUserEvent;
    HPBYTE          hpbPendingUserEvent;
    
    TRACK           rTracks[];
}   SMF;

typedef struct tagEVENT
{
    TICKS           tkDelta;           
    BYTE            abEvent[3];        
                                       
                                       
                                       
    DWORD           cbParm;            
    HPBYTE          hpbParm;           
}   EVENT,
    BSTACK *SPEVENT;

#define EVENT_TYPE(event)       ((event).abEvent[0])
#define EVENT_CH_B1(event)      ((event).abEvent[1])
#define EVENT_CH_B2(event)      ((event).abEvent[2])

#define EVENT_META_TYPE(event)  ((event).abEvent[1])

SMFRESULT FNLOCAL smfBuildFileIndex(
    PSMF BSTACK *       ppsmf);

DWORD FNLOCAL smfGetVDword(
    HPBYTE              hpbImage,
    DWORD               dwLeft,                                
    DWORD BSTACK *      pdw);

SMFRESULT FNLOCAL smfGetNextEvent(
    PSMF                psmf,
    SPEVENT             pevent,
    TICKS               tkMax);

/*
** Useful macros when dealing with hi-lo format integers
*/
#define DWORDSWAP(dw) \
    ((((dw)>>24)&0x000000FFL)|\
    (((dw)>>8)&0x0000FF00L)|\
    (((dw)<<8)&0x00FF0000L)|\
    (((dw)<<24)&0xFF000000L))

#define WORDSWAP(w) \
    ((((w)>>8)&0x00FF)|\
    (((w)<<8)&0xFF00))

#define FOURCC_RMID     mmioFOURCC('R','M','I','D')
#define FOURCC_data     mmioFOURCC('d','a','t','a')
#define FOURCC_MThd     mmioFOURCC('M','T','h','d')
#define FOURCC_MTrk     mmioFOURCC('M','T','r','k')

typedef struct tag_chunkhdr
{
    FOURCC  fourccType;
    DWORD   dwLength;
}   CHUNKHDR,
    *PCHUNKHDR;

#pragma pack(1)	// override cl32 default packing, to match disk file.
typedef struct tag_filehdr
{
    WORD    wFormat;
    WORD    wTracks;
    WORD    wDivision;
}   FILEHDR,
    *PFILEHDR;
#pragma pack()

/* NOTE: This is arbitrary and only used if there is a tempo map but no
** entry at tick 0.
*/
#define MIDI_DEFAULT_TEMPO      (500000L)

#define MIDI_MSG                ((BYTE)0x80)
#define MIDI_NOTEOFF            ((BYTE)0x80)
#define MIDI_NOTEON             ((BYTE)0x90)
#define MIDI_POLYPRESSURE       ((BYTE)0xA0)
#define MIDI_CONTROLCHANGE      ((BYTE)0xB0)
#define MIDI_PROGRAMCHANGE      ((BYTE)0xC0)
#define MIDI_CHANPRESSURE       ((BYTE)0xD0)
#define MIDI_PITCHBEND          ((BYTE)0xE0)
#define MIDI_META               ((BYTE)0xFF)
#define MIDI_SYSEX              ((BYTE)0xF0)
#define MIDI_SYSEXEND           ((BYTE)0xF7)

#define MIDI_META_TRACKNAME     ((BYTE)0x03)
#define MIDI_META_EOT           ((BYTE)0x2F)
#define MIDI_META_TEMPO         ((BYTE)0x51)
#define MIDI_META_TIMESIG       ((BYTE)0x58)
#define MIDI_META_KEYSIG        ((BYTE)0x59)
#define MIDI_META_SEQSPECIFIC   ((BYTE)0x7F)




#endif
