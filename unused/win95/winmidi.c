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


#pragma off (unreferenced)
static char rcsid[] = "$Id: winmidi.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)




#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>

#include "pstypes.h"
#include "mono.h"
#include "error.h"
#include "mem.h"
#include "winapp.h"
#include "winregs.h"


#include "midifile.h"
#include "midiseq.h"
#include "winmidi.h"


//	----------------------------------------------------------------------------
//	Data
//	----------------------------------------------------------------------------

BOOL 	WMidi_NewStream = 0;

static BOOL				WMidi_initialized = FALSE;
static SEQ				MSeq;
static DWORD			wmidi_volume = 0x80008000;
static BOOL				LoopSong = FALSE;
static BOOL				MidiVolChanges = TRUE;


void wmidi_deamp_song(DWORD vol);
int wmidi_get_tech();
void CALLBACK wmidi_seq_callback(HMIDISTRM hms, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);


//	----------------------------------------------------------------------------
//	Sequencer Initialization and destruction
//	----------------------------------------------------------------------------

int wmidi_init(uint mmdev)
{
	MIDIOUTCAPS midicaps;
	//BYTE *hdr;
	//DWORD bufsize;
	//int i;
	UINT ui;
	
	memset(&MSeq, 0, sizeof(SEQ));

	WMidi_initialized = TRUE;

//	Initialize stream buffers
	MSeq.uState = SEQ_S_NOFILE;
	MSeq.lpmhFree = NULL;
	MSeq.lpbAlloc = NULL;
	MSeq.hmidi = NULL;

	MSeq.cBuffer = 4;
	MSeq.cbBuffer = 512;

	if (seqAllocBuffers(&MSeq) != MMSYSERR_NOERROR)
    	return FALSE;

//	Setup sequencer some more
	ui = midiOutGetDevCaps(mmdev, &midicaps, sizeof(midicaps));
	if ( ui != MMSYSERR_NOERROR) {
		mprintf((1, "WMIDI:(%x) Unable to find requested device.\n", MSeq.mmrcLastErr));
		return 0;
	}
	
	if ((midicaps.dwSupport & MIDICAPS_VOLUME) || (midicaps.dwSupport & MIDICAPS_LRVOLUME)) 
		MidiVolChanges = TRUE;
	else MidiVolChanges = FALSE;

	MSeq.uDeviceID = mmdev;

//	Now we must do a stupid Microsoft trick to determine whether we are
//	using WAVE TABLE volume or OPL volume.
	MSeq.uMCIDeviceID = wmidi_get_tech();

	logentry("MIDI technology: %d.\n", MSeq.uMCIDeviceID);
	
	return 1;
}
				

void wmidi_close()
{
	//MIDIHDR *hdr;

	Assert(WMidi_initialized == TRUE);

	seqFreeBuffers(&MSeq);
	
	WMidi_initialized = FALSE;
}


int wmidi_get_tech()
{
	HKEY hKey;
	long lres;
	char key[32];	
	char buf[256];
	char *pstr;
	DWORD len,type;


	lres = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap",
					0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_EXECUTE,
					&hKey);
	if (lres != ERROR_SUCCESS) return 0;

	len = 32;
	lres = RegQueryValueEx(hKey, "CurrentInstrument", NULL, &type, key, &len);
	if (lres != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}

	RegCloseKey(hKey);

	strcpy(buf, "System\\CurrentControlSet\\control\\MediaResources\\midi\\");
	strcat(buf, key);
	lres = RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 
					0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_EXECUTE,
					&hKey);
	if (lres != ERROR_SUCCESS) return 0;
	
 	len =128;
	lres = RegQueryValueEx(hKey, "Description", NULL, &type, buf, &len);
	if (lres != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return 0;
	}
	RegCloseKey(hKey);

//	Okay, look for 'wave' or 'fm' or 'opl'
	pstr = strlwr(buf);	
	if (strstr(pstr, "wave")) return 1;

	return 0;
}


int wmidi_support_volchange() {
	return (int)MidiVolChanges;
}


//	----------------------------------------------------------------------------
//	High level midi song play/stop/pause interface
//	----------------------------------------------------------------------------

int wmidi_init_song(WMIDISONG *song)
{
	DWORD bufsize;
	SMF *smf;
	MMRESULT mmrc;

	if (MSeq.uState != SEQ_S_NOFILE) return 0;

	Assert (song->data != NULL);

   mmrc = seqStop(&MSeq);
	if (mmrc != MMSYSERR_NOERROR)
	{
		mprintf((1, "WMIDI: Stop song failed.\n"));;
		return 0;
	}

	wmidi_close_song();

	if (song->looping) LoopSong = TRUE;
	else LoopSong = FALSE;

	if (smfOpenFile(song->data, song->length, (HSMF *)&smf) != SMF_SUCCESS) return 0;

//	song is confirmed to be a MIDI v1.0 compatible file
	MSeq.hSmf = smf;
	MSeq.dwTimeDivision = smf->dwTimeDivision;
	MSeq.tkLength = smf->tkLength;
	MSeq.cTrk = smf->dwTracks;

	bufsize = min(MSeq.cbBuffer, smfGetStateMaxSize());

	MSeq.uState = SEQ_S_OPENED;
	
	return 1;
}


int wmidi_play()
{
	PREROLL		preroll;
	MMRESULT 	mmrc;
	DWORD 		vol;

	preroll.tkBase = 0;
	preroll.tkEnd = MSeq.tkLength;

	mprintf((1, "wmidi_play: beginning playback.\n"));

	if ((mmrc = seqPreroll(&MSeq, &preroll)) != MMSYSERR_NOERROR) {
		mprintf((1, "wmidi_play: preroll failed! (%x)\n", mmrc));
		return 0;
	}

	
	Assert(MSeq.hmidi != NULL);

//	wmidi_deamp_song(wmidi_volume);

	mmrc = midiOutSetVolume((HMIDIOUT)MSeq.hmidi, wmidi_volume);	
	if (mmrc != MMSYSERR_NOERROR) mprintf((1, "MIDI VOL ERR: %d\n", mmrc));
	midiOutGetVolume((HMIDIOUT)MSeq.hmidi, &vol);

	if (seqStart(&MSeq) != MMSYSERR_NOERROR) return 0; 

	return 1;
}


int wmidi_stop()
{
	if (seqStop(&MSeq) != MMSYSERR_NOERROR) return 0;
	return 1;
}


int wmidi_pause()
{
	if (MSeq.uState != SEQ_S_PAUSED) {
		if (seqPause(&MSeq) != MMSYSERR_NOERROR) return 0;
	}
	return 1;
}
		
int wmidi_resume()
{
	if (MSeq.uState == SEQ_S_PAUSED) {
		if (seqRestart(&MSeq) != MMSYSERR_NOERROR) return 0;
	}
	return 1;
}
		

int wmidi_close_song()
{

	LPMIDIHDR               lpmh;
    
	if (SEQ_S_OPENED != MSeq.uState)
		return MCIERR_UNSUPPORTED_FUNCTION;

	LoopSong = FALSE;
    
	if ((HSMF)NULL != MSeq.hSmf)
 	{
		smfCloseFile(MSeq.hSmf);
		MSeq.hSmf = (HSMF)NULL;
   }

/* take care of any stream buffers or MIDI resources left open by
	the sequencer */

	if (MSeq.hmidi) {

		for (lpmh = MSeq.lpmhFree; lpmh; lpmh = lpmh->lpNext)
			midiOutUnprepareHeader(MSeq.hmidi, lpmh, sizeof(MIDIHDR));

		if (MSeq.lpmhPreroll) 
			midiOutUnprepareHeader(MSeq.hmidi, MSeq.lpmhPreroll, sizeof(MIDIHDR));

		midiStreamClose(MSeq.hmidi);
		MSeq.hmidi = NULL;
	}

	MSeq.uState = SEQ_S_NOFILE;
	
	return 1;
}


static uint funky_vol_log_table [] = {
	0x00000,				// 0
	0x0b000,				// 1
	0x0c000,				// 2
	0x0c800,				// 3
	0x0e000,		 		// 4
	0x0e800,				// 5
	0x0f000,				//	6
	0x0f800,				//	7
	0x0ffff				// 8
};



int wmidi_set_volume(uint volume)
{
	DWORD vol;
	//int i;
	MMRESULT mmrc;
	

	Assert(volume < 128);
	if (volume == 127) volume++;

	volume = volume /16;

	if (!volume) vol = 0;
	else if (MSeq.uMCIDeviceID == 1)				// WAVETABLE 
		vol = funky_vol_log_table[volume]|0x00ff;
	else {												// OPL2/OPL3 and others
		vol = volume * 0x2000;				
		if (vol > 0) vol--;
	}

	vol = (vol<<16)+vol;

	wmidi_volume = vol;


	if (MSeq.hmidi) {
//		wmidi_deamp_song(wmidi_volume);
		mmrc = midiOutSetVolume((HMIDIOUT)MSeq.hmidi, wmidi_volume);	
		if (mmrc != MMSYSERR_NOERROR) mprintf((1, "MIDI VOL ERR: %d\n", mmrc));
		midiOutGetVolume((HMIDIOUT)MSeq.hmidi, &vol);
	}

	return 1;
}


#define MIDI_CTRLCHANGE ((BYTE)0xB0)        // + ctrlr + value
#define MIDICTRL_VOLUME ((BYTE)0x07)
 

void wmidi_deamp_song(DWORD vol)
{
	float ptvol;
	DWORD ptdvol;
	DWORD dwvol, dwstatus, dwevent;
	MMRESULT mmrc;
	int i;	


	vol = vol & 0x0000ffff;

	ptvol = (float)(vol)*100.0/65536.0;
	ptdvol = (DWORD)(ptvol);

	mprintf((0, "Deamp = %d percent.\n", ptdvol));

	for (i = 0, dwstatus = MIDI_CTRLCHANGE; i < 16; i++, dwstatus++)
	{
		dwvol = ( 100 * ptdvol ) / 1000;
	   dwevent = dwstatus | ((DWORD)MIDICTRL_VOLUME << 8)
            | ((DWORD)dwvol << 16);
	  	if(( mmrc= midiOutShortMsg( (HMIDIOUT)MSeq.hmidi, dwevent ))
                            != MMSYSERR_NOERROR )
      {
			mprintf((1, "WINMIDI::BAD MIDI VOLUME CHANGE!\n"));
        	return;
      }
	}
}



/*****************************************************************************
*
*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
*  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
*  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR
*  A PARTICULAR PURPOSE.
*
*  Copyright (C) 1993-1995 Microsoft Corporation. All Rights Reserved.
*
******************************************************************************/


PRIVATE void FAR PASCAL seqMIDICallback(HMIDISTRM hms, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
PRIVATE MMRESULT FNLOCAL XlatSMFErr(SMFRESULT smfrc);

/***************************************************************************
*  
* seqAllocBuffers
*
* Allocate buffers for this instance.
*
***************************************************************************/
MMRESULT FNLOCAL seqAllocBuffers(
    PSEQ                    pSeq)
{
    DWORD                   dwEachBufferSize;
    DWORD                   dwAlloc;
    UINT                    i;
    LPBYTE                  lpbWork;

    Assert(pSeq != NULL);

    pSeq->uState    = SEQ_S_NOFILE;
    pSeq->lpmhFree  = NULL;
    pSeq->lpbAlloc  = NULL;
    pSeq->hSmf      = (HSMF)NULL;
    
    /* First make sure we can allocate the buffers they asked for
    */
    dwEachBufferSize = sizeof(MIDIHDR) + (DWORD)(pSeq->cbBuffer);
    dwAlloc          = dwEachBufferSize * (DWORD)(pSeq->cBuffer);
    
    pSeq->lpbAlloc = GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE, dwAlloc);
    if (NULL == pSeq->lpbAlloc)
        return MCIERR_OUT_OF_MEMORY;

    /* Initialize all MIDIHDR's and throw them into a free list
    */
    pSeq->lpmhFree = NULL;

    lpbWork = pSeq->lpbAlloc;
    for (i=0; i < pSeq->cBuffer; i++)
    {
        ((LPMIDIHDR)lpbWork)->lpNext            = pSeq->lpmhFree;

        ((LPMIDIHDR)lpbWork)->lpData            = lpbWork + sizeof(MIDIHDR);
        ((LPMIDIHDR)lpbWork)->dwBufferLength    = pSeq->cbBuffer;
        ((LPMIDIHDR)lpbWork)->dwBytesRecorded   = 0;
        ((LPMIDIHDR)lpbWork)->dwUser            = (DWORD)(UINT)pSeq;
        ((LPMIDIHDR)lpbWork)->dwFlags           = 0;

        pSeq->lpmhFree = (LPMIDIHDR)lpbWork;

        lpbWork += dwEachBufferSize;
    }

    return MMSYSERR_NOERROR;
}

/***************************************************************************
*  
* seqFreeBuffers
*
* Free buffers for this instance.
*
****************************************************************************/
VOID FNLOCAL seqFreeBuffers(
    PSEQ                    pSeq)
{
    LPMIDIHDR               lpmh;
    
    Assert(pSeq != NULL);

    if (NULL != pSeq->lpbAlloc)
    {
        lpmh = (LPMIDIHDR)pSeq->lpbAlloc;
        Assert(!(lpmh->dwFlags & MHDR_PREPARED));
        
        GlobalFreePtr(pSeq->lpbAlloc);
    }
}


/***************************************************************************
*  
* seqPreroll
*
* Prepares the file for playback at the given position.
*
****************************************************************************/
MMRESULT FNLOCAL seqPreroll(
    PSEQ                    pSeq,
    LPPREROLL               lpPreroll)
{
    SMFRESULT           smfrc;
    MMRESULT            mmrc        = MMSYSERR_NOERROR;
    MIDIPROPTIMEDIV     mptd;
    LPMIDIHDR           lpmh = NULL;
    LPMIDIHDR           lpmhPreroll = NULL;
    DWORD               cbPrerollBuffer;
    UINT                uDeviceID;

    Assert(pSeq != NULL);

    pSeq->mmrcLastErr = MMSYSERR_NOERROR;

    if (pSeq->uState != SEQ_S_OPENED &&
        pSeq->uState != SEQ_S_PREROLLED)
        return MCIERR_UNSUPPORTED_FUNCTION;

	pSeq->tkBase = lpPreroll->tkBase;
	pSeq->tkEnd = lpPreroll->tkEnd;

    if (pSeq->hmidi)
    {
        // Recollect buffers from MMSYSTEM back into free queue
        //
        pSeq->uState = SEQ_S_RESET;
        midiOutReset(pSeq->hmidi);

		while (pSeq->uBuffersInMMSYSTEM)
			Sleep(0);
    }
    
    pSeq->uBuffersInMMSYSTEM = 0;
    pSeq->uState = SEQ_S_PREROLLING;
    
    //
    // We've successfully opened the file and all of the tracks; now
    // open the MIDI device and set the time division.
    //
    // NOTE: seqPreroll is equivalent to seek; device might already be open
    //
    if (NULL == pSeq->hmidi)
    {
        uDeviceID = pSeq->uDeviceID;
        if ((mmrc = midiStreamOpen(&pSeq->hmidi,
                                   &uDeviceID,
                                   1,
                                   (DWORD)seqMIDICallback,
                                   0,
                                   CALLBACK_FUNCTION)) != MMSYSERR_NOERROR)
        {
            pSeq->hmidi = NULL;
            goto seq_Preroll_Cleanup;
        }
        
        mptd.cbStruct  = sizeof(mptd);
        mptd.dwTimeDiv = pSeq->dwTimeDivision;
        if ((mmrc = midiStreamProperty(
                                       (HMIDI)pSeq->hmidi,
                                       (LPBYTE)&mptd,
                                       MIDIPROP_SET|MIDIPROP_TIMEDIV)) != MMSYSERR_NOERROR)
        {
            mprintf((1, "midiStreamProperty() -> %04X\n", (WORD)mmrc));
            midiStreamClose(pSeq->hmidi);
            pSeq->hmidi = NULL;
            mmrc = MCIERR_DEVICE_NOT_READY;
            goto seq_Preroll_Cleanup;
        }
    }

    mmrc = MMSYSERR_NOERROR;

    //
    //  Allocate a preroll buffer.  Then if we don't have enough room for
    //  all the preroll info, we make the buffer larger.  
    //
    if (!pSeq->lpmhPreroll)
    {
        cbPrerollBuffer = 4096;
        lpmhPreroll = (LPMIDIHDR)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,
                                                            cbPrerollBuffer);
    }
    else
    {
        cbPrerollBuffer = pSeq->cbPreroll;
        lpmhPreroll = pSeq->lpmhPreroll;
    }

    lpmhPreroll->lpNext            = pSeq->lpmhFree;
    lpmhPreroll->lpData            = (LPBYTE)lpmhPreroll + sizeof(MIDIHDR);
    lpmhPreroll->dwBufferLength    = cbPrerollBuffer - sizeof(MIDIHDR);
    lpmhPreroll->dwBytesRecorded   = 0;
    lpmhPreroll->dwUser            = (DWORD)(UINT)pSeq;
    lpmhPreroll->dwFlags           = 0;

    do
    {
        smfrc = smfSeek(pSeq->hSmf, pSeq->tkBase, lpmhPreroll);
        if( SMF_SUCCESS != smfrc )
        {
            if( ( SMF_NO_MEMORY != smfrc )  ||
                ( cbPrerollBuffer >= 32768L ) )
            {
                mprintf((1, "smfSeek() returned %lu\n", (DWORD)smfrc));

                GlobalFreePtr(lpmhPreroll);
                pSeq->lpmhPreroll = NULL;

                mmrc = XlatSMFErr(smfrc);
                goto seq_Preroll_Cleanup;
            }
            else   //  Try to grow buffer.
            {
                cbPrerollBuffer *= 2;
                lpmh = (LPMIDIHDR)GlobalReAllocPtr( lpmhPreroll, cbPrerollBuffer, 0 );
                if( NULL == lpmh )
                {
                    mprintf((1,"seqPreroll - realloc failed, aborting preroll.\n"));
                    mmrc = MCIERR_OUT_OF_MEMORY;
                    goto seq_Preroll_Cleanup;
                }

                lpmhPreroll = lpmh;
                lpmhPreroll->lpData = (LPBYTE)lpmhPreroll + sizeof(MIDIHDR);
                lpmhPreroll->dwBufferLength = cbPrerollBuffer - sizeof(MIDIHDR);

                pSeq->lpmhPreroll = lpmhPreroll;
                pSeq->cbPreroll = cbPrerollBuffer;
            }
        }
    } while( SMF_SUCCESS != smfrc );

    if (MMSYSERR_NOERROR != (mmrc = midiOutPrepareHeader(pSeq->hmidi, lpmhPreroll, sizeof(MIDIHDR))))
    {
        mprintf((1, "midiOutPrepare(preroll) -> %lu!\n", (DWORD)mmrc));

        mmrc = MCIERR_DEVICE_NOT_READY;
        goto seq_Preroll_Cleanup;
    }

    ++pSeq->uBuffersInMMSYSTEM;

    if (MMSYSERR_NOERROR != (mmrc = midiStreamOut(pSeq->hmidi, lpmhPreroll, sizeof(MIDIHDR))))
    {
        mprintf((1, "midiStreamOut(preroll) -> %lu!\n", (DWORD)mmrc));

        mmrc = MCIERR_DEVICE_NOT_READY;
        --pSeq->uBuffersInMMSYSTEM;
        goto seq_Preroll_Cleanup;
    }
    mprintf((1,"seqPreroll: midiStreamOut(0x%x,0x%lx,%u) returned %u.\n",pSeq->hmidi,lpmhPreroll,sizeof(MIDIHDR),mmrc));

    pSeq->fdwSeq &= ~SEQ_F_EOF;
    while (pSeq->lpmhFree)
    {
        lpmh = pSeq->lpmhFree;
        pSeq->lpmhFree = lpmh->lpNext;

        smfrc = smfReadEvents(pSeq->hSmf, lpmh, pSeq->tkEnd);
        if (SMF_SUCCESS != smfrc && SMF_END_OF_FILE != smfrc)
        {
            mprintf((1, "SFP: smfReadEvents() -> %u\n", (UINT)smfrc));
            mmrc = XlatSMFErr(smfrc);
            goto seq_Preroll_Cleanup;
        }

        if (MMSYSERR_NOERROR != (mmrc = midiOutPrepareHeader(pSeq->hmidi, lpmh, sizeof(*lpmh))))
        {
            mprintf((1, "SFP: midiOutPrepareHeader failed\n"));
            goto seq_Preroll_Cleanup;
        }

        if (MMSYSERR_NOERROR != (mmrc = midiStreamOut(pSeq->hmidi, lpmh, sizeof(*lpmh))))
        {
            mprintf((1, "SFP: midiStreamOut failed\n"));
            goto seq_Preroll_Cleanup;
        }

        ++pSeq->uBuffersInMMSYSTEM; 

        if (SMF_END_OF_FILE == smfrc)
        {
            pSeq->fdwSeq |= SEQ_F_EOF;
            break;
        }
    } 

seq_Preroll_Cleanup:
    if (MMSYSERR_NOERROR != mmrc)
    {
        pSeq->uState = SEQ_S_OPENED;
        pSeq->fdwSeq &= ~SEQ_F_WAITING;
    }
    else
    {
        pSeq->uState = SEQ_S_PREROLLED;
    }

    return mmrc;
}

/***************************************************************************
*  
* seqStart
*
* Starts playback at the current position.
*       
***************************************************************************/
MMRESULT FNLOCAL seqStart(
    PSEQ                    pSeq)
{
    Assert(NULL != pSeq);

    if (SEQ_S_PREROLLED != pSeq->uState)
    {
        mprintf((1, "seqStart(): State is wrong! [%u]\n", pSeq->uState));
        return MCIERR_UNSUPPORTED_FUNCTION;
    }

    pSeq->uState = SEQ_S_PLAYING;

    return midiStreamRestart(pSeq->hmidi);
}

/***************************************************************************
*  
* seqPause
*
***************************************************************************/
MMRESULT FNLOCAL seqPause(
    PSEQ                    pSeq)
{
    Assert(NULL != pSeq);
    
    if (SEQ_S_PLAYING != pSeq->uState)
        return MCIERR_UNSUPPORTED_FUNCTION;

    pSeq->uState = SEQ_S_PAUSED;
    midiStreamPause(pSeq->hmidi);
    
    return MMSYSERR_NOERROR;
}

/***************************************************************************
*  
* seqRestart
*
***************************************************************************/
MMRESULT FNLOCAL seqRestart(
    PSEQ                    pSeq)
{
    Assert(NULL != pSeq);

    if (SEQ_S_PAUSED != pSeq->uState)
        return MCIERR_UNSUPPORTED_FUNCTION;

    pSeq->uState = SEQ_S_PLAYING;
    midiStreamRestart(pSeq->hmidi);

    return MMSYSERR_NOERROR;
}

/***************************************************************************
*  
* seqStop
*
* Totally stops playback of an instance.
*
***************************************************************************/
MMRESULT FNLOCAL seqStop(
    PSEQ                    pSeq)
{
    Assert(NULL != pSeq);

    /* Automatic success if we're already stopped
    */
    if (SEQ_S_PLAYING != pSeq->uState &&
        SEQ_S_PAUSED != pSeq->uState)
    {
        pSeq->fdwSeq &= ~SEQ_F_WAITING;
        return MMSYSERR_NOERROR;
    }

    pSeq->uState = SEQ_S_STOPPING;
    pSeq->fdwSeq |= SEQ_F_WAITING;
    
    if (MMSYSERR_NOERROR != (pSeq->mmrcLastErr = midiStreamStop(pSeq->hmidi)))
    {
        mprintf((1, "midiOutStop() returned %lu in seqStop()!\n", (DWORD)pSeq->mmrcLastErr));
        
        pSeq->fdwSeq &= ~SEQ_F_WAITING;
        return MCIERR_DEVICE_NOT_READY;
    }

	while (pSeq->uBuffersInMMSYSTEM)
		Sleep(0);
    
    return MMSYSERR_NOERROR;
}

/***************************************************************************
*  
* seqTime
*
***************************************************************************/
MMRESULT FNLOCAL seqTime(
    PSEQ                    pSeq,
    PTICKS                  pTicks)
{
    MMRESULT                mmr;
    MMTIME                  mmt;
    
    Assert(pSeq != NULL);

    if (SEQ_S_PLAYING != pSeq->uState &&
        SEQ_S_PAUSED != pSeq->uState &&
        SEQ_S_PREROLLING != pSeq->uState &&
        SEQ_S_PREROLLED != pSeq->uState &&
        SEQ_S_OPENED != pSeq->uState)
    {
        mprintf((1, "seqTime(): State wrong! [is %u]\n", pSeq->uState));
        return MCIERR_UNSUPPORTED_FUNCTION;
    }

    *pTicks = 0;
    if (SEQ_S_OPENED != pSeq->uState)
    {
        *pTicks = pSeq->tkBase;
        if (SEQ_S_PREROLLED != pSeq->uState)
        {
            mmt.wType = TIME_TICKS;
            mmr = midiStreamPosition(pSeq->hmidi, &mmt, sizeof(mmt));
            if (MMSYSERR_NOERROR != mmr)
            {
                mprintf((1, "midiStreamPosition() returned %lu\n", (DWORD)mmr));
                return MCIERR_DEVICE_NOT_READY;
            }

            *pTicks += mmt.u.ticks;
        }
    }

    return MMSYSERR_NOERROR;
}
                              
/***************************************************************************
*  
* seqMillisecsToTicks
*
* Given a millisecond offset in the output stream, returns the associated
* tick position.
*
***************************************************************************/
TICKS FNLOCAL seqMillisecsToTicks(
    PSEQ                    pSeq,
    DWORD                   msOffset)
{
    return smfMillisecsToTicks(pSeq->hSmf, msOffset);
}

/***************************************************************************
*  
* seqTicksToMillisecs
*
* Given a tick offset in the output stream, returns the associated
* millisecond position.
*
***************************************************************************/
DWORD FNLOCAL seqTicksToMillisecs(
    PSEQ                    pSeq,
    TICKS                   tkOffset)
{
    return smfTicksToMillisecs(pSeq->hSmf, tkOffset);
}

/***************************************************************************
*  
* seqMIDICallback
*
* Called by the system when a buffer is done
*
* dw1                       - The buffer that has completed playback.
*
***************************************************************************/

PRIVATE void FAR PASCAL seqMIDICallback(HMIDISTRM hms, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	LPMIDIHDR					lpmh		= (LPMIDIHDR)dw1;
    PSEQ                    pSeq;
    MMRESULT                mmrc;
    SMFRESULT               smfrc;
	//DWORD vol;

	if (uMsg != MOM_DONE)
		return;

	Assert(NULL != lpmh);

    pSeq = (PSEQ)(lpmh->dwUser);

    Assert(pSeq != NULL);

    --pSeq->uBuffersInMMSYSTEM;
    
	if (pSeq->uState == SEQ_S_NOFILE) 
		mprintf((1, "seqCallback: No file!!\n"));

    if (SEQ_S_RESET == pSeq->uState)
    {
        // We're recollecting buffers from MMSYSTEM
        //
		if (lpmh != pSeq->lpmhPreroll)
		{
        	lpmh->lpNext   = pSeq->lpmhFree;
        	pSeq->lpmhFree = lpmh;
		}

        return;
    }
    

    if ((SEQ_S_STOPPING == pSeq->uState) || (pSeq->fdwSeq & SEQ_F_EOF))
    {
        /*
        ** Reached EOF, just put the buffer back on the free
        ** list 
        */
		if (lpmh != pSeq->lpmhPreroll)
		{
        	lpmh->lpNext   = pSeq->lpmhFree;
        	pSeq->lpmhFree = lpmh;
		}
			if (!pSeq->hmidi) 
				mprintf((1, "seqCallback: NULL MIDI HANDLE.\n"));

        if (MMSYSERR_NOERROR != (mmrc = midiOutUnprepareHeader(pSeq->hmidi, lpmh, sizeof(*lpmh))))
        {
            mprintf((1, "midiOutUnprepareHeader failed in seqBufferDone! (%lu)\n", (DWORD)mmrc));
        }

        if (0 == pSeq->uBuffersInMMSYSTEM)
        {
				int stop_loop=0;
            mprintf((1, "seqBufferDone: normal sequencer shutdown.\n"));
            
            /* Totally done! Free device and notify.
            */
            midiStreamClose(pSeq->hmidi);
            
				if ((SEQ_S_STOPPING == pSeq->uState) && (pSeq->fdwSeq & SEQ_F_EOF) && LoopSong) {
					stop_loop = 1;
					mprintf((1, "seqMIDICallback: cancelling loop.\n"));
				}
            pSeq->hmidi = NULL;
            pSeq->uState = SEQ_S_OPENED;
            pSeq->mmrcLastErr = MMSYSERR_NOERROR;
            pSeq->fdwSeq &= ~SEQ_F_WAITING;

				if ((pSeq->fdwSeq & SEQ_F_EOF) && LoopSong && !stop_loop)
					wmidi_play(); 
        }
    }
    else
    {
        /*
        ** Not EOF yet; attempt to fill another buffer
        */
		  DWORD vol;

        smfrc = smfReadEvents(pSeq->hSmf, lpmh, pSeq->tkEnd);
        
        switch(smfrc)
        {
            case SMF_SUCCESS:
                break;

            case SMF_END_OF_FILE:
                pSeq->fdwSeq |= SEQ_F_EOF;
                smfrc = SMF_SUCCESS;
                break;

            default:
                mprintf((1, "smfReadEvents returned %lu in callback!\n", (DWORD)smfrc));
                pSeq->uState = SEQ_S_STOPPING;
                break;
        }

        if (SMF_SUCCESS == smfrc)
        {
            ++pSeq->uBuffersInMMSYSTEM;

				Assert(pSeq->hmidi != NULL);

//				wmidi_deamp_song(wmidi_volume);
				mmrc = midiOutSetVolume((HMIDIOUT)MIDI_MAPPER, wmidi_volume);	
				if (mmrc != MMSYSERR_NOERROR) mprintf((1, "MIDI VOL ERR: %d\n", mmrc));
				midiOutGetVolume((HMIDIOUT)MIDI_MAPPER, &vol);

            mmrc = midiStreamOut(pSeq->hmidi, lpmh, sizeof(*lpmh));
				WMidi_NewStream++;
            if (MMSYSERR_NOERROR != mmrc)
            {
                mprintf((1, "seqBufferDone(): midiStreamOut() returned %lu!\n", (DWORD)mmrc));
                
                --pSeq->uBuffersInMMSYSTEM;
                pSeq->uState = SEQ_S_STOPPING;
         --pSeq->uBuffersInMMSYSTEM;
                pSeq->uState = SEQ_S_STOPPING;
            }
        }
    }

}

/***************************************************************************
*  
* XlatSMFErr
*
* Translates an error from the SMF layer into an appropriate MCI error.
*
***************************************************************************/
PRIVATE MMRESULT FNLOCAL XlatSMFErr(
    SMFRESULT               smfrc)
{
    switch(smfrc)
    {
        case SMF_SUCCESS:
            return MMSYSERR_NOERROR;

        case SMF_NO_MEMORY:
            return MCIERR_OUT_OF_MEMORY;

        case SMF_INVALID_FILE:
        case SMF_OPEN_FAILED:
        case SMF_INVALID_TRACK:
            return MCIERR_INVALID_FILE;

        default:
            return MCIERR_UNSUPPORTED_FUNCTION;
    }
}






