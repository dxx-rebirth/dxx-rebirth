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
static char rcsid[] = "$Id: ds.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#define WIN32_LEAN_AND_MEAN
#define _WIN32
#define WIN95
#include <windows.h>
#include <mmsystem.h>

#include <stdlib.h>
#include <math.h>
#include <mem.h>

#include "fix.h"
#include	"ds.h"
#include "mono.h"


//	Samir's Hacked Musical Interface (HMI) mixer

struct SSMixerObject {
	LPDIRECTSOUND lpds;
	DWORD old_master_vol;
	int active;
	int ch_num;
	int ch_cur;
	SSoundBuffer *ch_list;
} SSMixer = { NULL, 0, 0, 0, NULL };		


long XlatSSToDSPan(unsigned short pan);
long XlatSSToDSVol(unsigned vol);
DWORD XlatSSToWAVVol(unsigned short vol);
unsigned short XlatWAVToSSVol(DWORD vol);



//	Functions
//	----------------------------------------------------------------------------

BOOL SSInit(HWND hWnd, int channels, unsigned flags)
{
	LPDIRECTSOUNDBUFFER lpPrimaryBuffer;
	LPDIRECTSOUND lpDS;
	DSBUFFERDESC dsbd;

	if (SSMixer.lpds) return TRUE;

//	Perform Direct Sound Initialization
	if (DirectSoundCreate(NULL, &lpDS, NULL) != DS_OK) 
		return FALSE;

	SSMixer.lpds = lpDS;

	if (IDirectSound_SetCooperativeLevel(lpDS, hWnd, DSSCL_NORMAL) != DS_OK) {
		SSDestroy();
		return FALSE;
	}

//	Start Mixer
	memset(&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (IDirectSound_CreateSoundBuffer(SSMixer.lpds, &dsbd, &lpPrimaryBuffer, NULL) == DS_OK) {
		if (IDirectSoundBuffer_Play(lpPrimaryBuffer, 0, 0, DSBPLAY_LOOPING) != DS_OK) {
			IDirectSoundBuffer_Release(lpPrimaryBuffer);
			SSDestroy();
			return FALSE;
		}
		IDirectSoundBuffer_Release(lpPrimaryBuffer);
	}
	else {
		SSDestroy();
		return FALSE;
	}


//	Finish initializing SSMixer.
	SSMixer.ch_cur = 0;
	SSMixer.ch_list = (SSoundBuffer *)malloc(sizeof(SSoundBuffer)*channels);
	if (!SSMixer.ch_list) return FALSE;
	
	memset(SSMixer.ch_list, 0, sizeof(SSoundBuffer)*channels);
	
	SSMixer.ch_num = channels;

//	Determine Sound technology and volume caps
	waveOutGetVolume((HWAVEOUT)WAVE_MAPPER, (LPDWORD)&SSMixer.old_master_vol);
//	waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, 0x40004000);
	return TRUE;
}
		
		
void SSDestroy()
{
	int i;
	int j = 0;

	if (!SSMixer.lpds) return;
	
//	Free sound buffers currently allocated.
	for (i=0; i<SSMixer.ch_num; i++)
		if (SSMixer.ch_list[i].obj) {
			j++;
			IDirectSoundBuffer_Release(SSMixer.ch_list[i].obj);
		}

	if (j) mprintf((1, "SS: Releasing %d sound buffers!\n", j));
	free(SSMixer.ch_list);

//	Restore old WAV volume
	waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, SSMixer.old_master_vol);

//	Turn off DirectSound
	if (SSMixer.lpds) IDirectSound_Release(SSMixer.lpds);
	
	memset(&SSMixer, 0, sizeof(SSMixer));
} 


LPDIRECTSOUND SSGetObject()
{
	return SSMixer.lpds;
}


void SSGetCaps(SSCaps *caps)
{
	DSCAPS dscaps;

	dscaps.dwSize = sizeof(DSCAPS);
	IDirectSound_GetCaps(SSMixer.lpds, &dscaps);
	
	if ((dscaps.dwFlags&DSCAPS_SECONDARY16BIT)) caps->bits_per_sample = 16;
	else caps->bits_per_sample = 8;

	caps->sample_rate = dscaps.dwMaxSecondarySampleRate;
}


void SSSetVolume(DWORD vol)
{
	vol = XlatSSToWAVVol((WORD)vol);
	waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, vol);
}


WORD SSGetVolume()
{
	DWORD vol;

	waveOutGetVolume((HWAVEOUT)WAVE_MAPPER, &vol);
	
	return XlatWAVToSSVol(vol);
}
 
	

//	SSInitBuffer
//		Must Create a DirectSound Secondary Buffer for the sound info.

BOOL SSInitBuffer(SSoundBuffer *sample)
{
	
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wav;
	HRESULT dsresult;
	LPVOID databuf;
	LPVOID databuf2;
	DWORD writesize1, writesize2;
	char *data;
	char *auxdata, *aux2data;
	int length;
	int auxlength, aux2length;

	data = sample->data;
	length = sample->length;

//	Separate buffer into two for looping effects.
	if (sample->looping) {
		if (sample->loop_start > -1) {
			auxdata = sample->data + sample->loop_start;
			length = sample->loop_start;
			auxlength = sample->loop_end - sample->loop_start;
			aux2data	= sample->data + sample->loop_end;
			aux2length = sample->length - sample->loop_end;
		}
	}

//	Create sound buffer
	sample->obj = NULL;
	sample->auxobj = NULL;
	sample->auxobj2 = NULL;

	wav.wFormatTag = WAVE_FORMAT_PCM;
	wav.nChannels = sample->channels;
	wav.nSamplesPerSec = sample->rate;
	wav.nBlockAlign = (sample->channels * sample->bits_per_sample) >> 3;
	wav.nAvgBytesPerSec = wav.nBlockAlign * wav.nSamplesPerSec;
	wav.wBitsPerSample = sample->bits_per_sample;

	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLDEFAULT;
	dsbd.lpwfxFormat = &wav;
	dsbd.dwBufferBytes = length;
	
	if (IDirectSound_CreateSoundBuffer(SSMixer.lpds, &dsbd, &sample->obj, NULL)  
			!= DS_OK) {
		return FALSE;
	}

//	Copy main data to buffer
	if (data) {
		dsresult = IDirectSoundBuffer_Lock(sample->obj, 0, length, &databuf,  		
				  					&writesize1, &databuf2, &writesize2, 0); 
		{
			if (dsresult != DS_OK) return FALSE;
			memcpy(databuf, data, writesize1);
			if (databuf2) memcpy(databuf2, data+writesize1, writesize2);
		}
		IDirectSoundBuffer_Unlock(sample->obj, databuf, writesize1, databuf2, writesize2);
	}

//	Take care of looping buffer
	if (sample->looping && sample->loop_start>-1) {
		memset(&dsbd, 0, sizeof(dsbd));

		wav.wFormatTag = WAVE_FORMAT_PCM;
		wav.nChannels = sample->channels;
		wav.nSamplesPerSec = sample->rate;
		wav.nBlockAlign = (sample->channels * sample->bits_per_sample) >> 3;
		wav.nAvgBytesPerSec = wav.nBlockAlign * wav.nSamplesPerSec;
		wav.wBitsPerSample = sample->bits_per_sample;

		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLDEFAULT;
		dsbd.lpwfxFormat = &wav;
		dsbd.dwBufferBytes = auxlength;
	
		if (IDirectSound_CreateSoundBuffer(SSMixer.lpds, &dsbd, &sample->auxobj, NULL)  
			!= DS_OK) {
			mprintf((1, "SS: Unable to create aux-buffer.\n"));
			return FALSE;
		}

		dsresult = IDirectSoundBuffer_Lock(sample->auxobj, 0, auxlength, &databuf,  		
				  					&writesize1, &databuf2, &writesize2, 0); 
		{
			if (dsresult != DS_OK) return FALSE;
			memcpy(databuf, auxdata, writesize1);
			if (databuf2) memcpy(databuf2, auxdata+writesize1, writesize2);
		}
		IDirectSoundBuffer_Unlock(sample->auxobj, databuf, writesize1, databuf2, writesize2);

//@@		memset(&dsbd, 0, sizeof(dsbd));
//@@
//@@		wav.wFormatTag = WAVE_FORMAT_PCM;
//@@		wav.nChannels = sample->channels;
//@@		wav.nSamplesPerSec = sample->rate;
//@@		wav.nBlockAlign = (sample->channels * sample->bits_per_sample) >> 3;
//@@		wav.nAvgBytesPerSec = wav.nBlockAlign * wav.nSamplesPerSec;
//@@		wav.wBitsPerSample = sample->bits_per_sample;
//@@
//@@		dsbd.dwSize = sizeof(dsbd);
//@@		dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLDEFAULT;
//@@		dsbd.lpwfxFormat = &wav;
//@@		dsbd.dwBufferBytes = aux2length;
//@@	
//@@		if (IDirectSound_CreateSoundBuffer(SSMixer.lpds, &dsbd, &sample->auxobj2, NULL)  
//@@			!= DS_OK) {
//@@			mprintf((1, "SS: Unable to create aux-buffer.\n"));
//@@			return FALSE;
//@@		}
//@@
//@@		dsresult = IDirectSoundBuffer_Lock(sample->auxobj2, 0, aux2length, &databuf,  		
//@@				  					&writesize1, &databuf2, &writesize2, 0); 
//@@		{
//@@			if (dsresult != DS_OK) return FALSE;
//@@			memcpy(databuf, aux2data, writesize1);
//@@			if (databuf2) memcpy(databuf2, aux2data+writesize1, writesize2);
//@@		}
//@@		IDirectSoundBuffer_Unlock(sample->auxobj2, databuf, writesize1, databuf2, writesize2);

	}												 

	return TRUE;
}


void SSDestroyBuffer(SSoundBuffer *sample)
{
	if (sample->obj) IDirectSoundBuffer_Release(sample->obj);
	if (sample->auxobj) IDirectSoundBuffer_Release(sample->auxobj);
	if (sample->auxobj2) IDirectSoundBuffer_Release(sample->auxobj2);
	sample->obj = NULL;
	sample->auxobj = NULL;
	sample->auxobj2 = NULL;
}	


int SSInitChannel(SSoundBuffer *sample)
{
	HRESULT dsresult;
	int start_channel, this_channel;
	LPDIRECTSOUNDBUFFER lpdsb;
	DWORD flags;

//	Find Free channel
	start_channel = SSMixer.ch_cur;
	
	while (1)
	{
		if (!SSMixer.ch_list[SSMixer.ch_cur].obj) 
			break;
		else if (!SSChannelPlaying(SSMixer.ch_cur)) {
			SSDestroyBuffer(&SSMixer.ch_list[SSMixer.ch_cur]);
			break;
		}
		SSMixer.ch_cur++;
		if (SSMixer.ch_cur >= SSMixer.ch_num) SSMixer.ch_cur=0;
		if (SSMixer.ch_cur == start_channel) return -1;
	}
  
//	Create sound object for mixer.		
	flags = 0;

	if (sample->looping) {
		if (sample->loop_start == -1) {
			flags = DSBPLAY_LOOPING;
//			mprintf((0,"SS: looping sample (%d).\n", sample->loop_start));
		}
	}

	if (!SSInitBuffer(sample)) return -1;

//	Set up mixing parameters
	lpdsb = sample->obj;
	IDirectSoundBuffer_SetVolume(lpdsb, XlatSSToDSVol(sample->vol));
	IDirectSoundBuffer_SetPan(lpdsb, XlatSSToDSPan(sample->pan));

//	Mix in main sound object.
	dsresult = IDirectSoundBuffer_Play(lpdsb, 0, 0, flags);
	if (dsresult != DS_OK) return -1;

//	Mix in auxillary object (loop portion)
	lpdsb = sample->auxobj;
	if (lpdsb) {
		if (sample->looping) flags = DSBPLAY_LOOPING;
		IDirectSoundBuffer_SetVolume(lpdsb, XlatSSToDSVol(sample->vol));
		IDirectSoundBuffer_SetPan(lpdsb, XlatSSToDSPan(sample->pan));

		dsresult = IDirectSoundBuffer_Play(lpdsb, 0, 0, flags);
		if (dsresult != DS_OK) return -1;
		mprintf((0, "SS: looping midsample (%d).\n", sample->loop_start));
	}
	
//	Add to mixer list.
	this_channel = SSMixer.ch_cur;
	memcpy(&SSMixer.ch_list[SSMixer.ch_cur++], sample, sizeof(SSoundBuffer));
	if (SSMixer.ch_cur >= SSMixer.ch_num) SSMixer.ch_cur = 0; 

	return this_channel;
}


BOOL SSChannelPlaying(int channel)
{
	HRESULT dsresult;
	DWORD status;
	int i;

	i = channel;

	if (SSMixer.ch_list[i].obj) {
		dsresult = IDirectSoundBuffer_GetStatus(SSMixer.ch_list[i].obj,
							&status);
		if (dsresult != DS_OK) return FALSE;
		if (status & DSBSTATUS_PLAYING) {
			return TRUE;
		}

		if (SSMixer.ch_list[i].auxobj) {
			dsresult = IDirectSoundBuffer_GetStatus(SSMixer.ch_list[i].auxobj, 
															&status);
			if (dsresult != DS_OK) return FALSE;
			if (status & DSBSTATUS_PLAYING) {
				return TRUE;
			}
		} 
	}
	return FALSE;
}
		

BOOL SSStopChannel(int channel)
{
	if (SSMixer.ch_list[channel].obj) {
		if (SSMixer.ch_list[channel].auxobj) {
			IDirectSoundBuffer_Stop(SSMixer.ch_list[channel].auxobj);
			mprintf((0, "DS: stopping midsample looping!\n"));
		}

		if (IDirectSoundBuffer_Stop(SSMixer.ch_list[channel].obj) != DS_OK) {
			return TRUE;
		}
		
		SSDestroyBuffer(&SSMixer.ch_list[channel]);
	}

	return TRUE;
}


BOOL SSSetChannelPan(int channel, unsigned short pan)
{
	HRESULT dsresult;
	int i;

	i = channel;

	if (SSMixer.ch_list[i].obj) {
		dsresult = IDirectSoundBuffer_SetPan(SSMixer.ch_list[i].obj,
							XlatSSToDSPan(pan));
		if (dsresult != DS_OK) return FALSE;
		return TRUE;
		
	}
	return FALSE;
}	


BOOL SSSetChannelVolume(int channel, unsigned short vol)
{
	HRESULT dsresult;
	int i;

	i = channel;

	if (SSMixer.ch_list[i].obj) {
		dsresult = IDirectSoundBuffer_SetVolume(SSMixer.ch_list[i].obj,
							XlatSSToDSVol(vol));
		if (dsresult != DS_OK) return FALSE;
		return TRUE;
		
	}
	return FALSE;
}	


//	----------------------------------------------------------------------------

long XlatSSToDSPan(unsigned short pan)
{
	fix pan1, pan2;
	long panr;

	pan1 = fixdiv(pan,0x8000);
	pan2 = fixmul(pan1, i2f(10000));
		
	panr = (long)f2i(pan2);
	panr = -10000+panr;
	
	return panr;
}


long XlatSSToDSVol(unsigned vol)
{
	float atten;
	float fvol;	

	atten = 32768.0	/ ((float)(vol));
	fvol = log(atten) / log(2.0);
	fvol = -1.0 * (fvol * 1000.0);

	if (fvol < -10000.0) fvol = -10000.0;
	if (fvol > 0.0) fvol = 0.0;

	return (int)(fvol);
}


DWORD XlatSSToWAVVol(unsigned short vol)
{
	DWORD dw;
	vol=vol*2;
	dw = (vol<<16)+vol;
	return (long)dw;
}


unsigned short XlatWAVToSSVol(DWORD vol)
{
	WORD wvol;
	wvol = (WORD)vol;
	return (WORD)(wvol/2);
}
