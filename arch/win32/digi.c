/* $Id: digi.c,v 1.12 2005-02-25 10:49:48 btb Exp $ */
#define DIGI_SOUND
#define MIDI_SOUND

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include <math.h>

#include "error.h"
#include "mono.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"
#include "digi.h"
#include "sounds.h"
#include "wall.h"
#include "newdemo.h"
#include "kconfig.h"
#include "hmpfile.h"

#include "altsound.h"

hmp_file *hmp = NULL;

#ifdef DIGI_SOUND
#define MAX_SOUND_SLOTS 32
#define MIN_VOLUME 10


//added/changed on 980905 by adb to make sfx volume work
#define SOUND_MAX_VOLUME F1_0
int digi_volume = SOUND_MAX_VOLUME;
//end edit by adb

LPDIRECTSOUND lpds;
WAVEFORMATEX waveformat;
DSBUFFERDESC dsbd;

extern HWND g_hWnd;

struct sound_slot {
 int soundno;
 int playing;   // Is there a sample playing on this channel?
 int looped;    // Play this sample looped?
 fix pan;       // 0 = far left, 1 = far right
 fix volume;    // 0 = nothing, 1 = fully on
 //changed on 980905 by adb from char * to unsigned char * 
 unsigned char *samples;
 //end changes by adb
 unsigned int length; // Length of the sample
 unsigned int position; // Position we are at at the moment.
 LPDIRECTSOUNDBUFFER lpsb;
} SoundSlots[MAX_SOUND_SLOTS];


int midi_volume = 255;
int digi_midi_song_playing = 0;
int digi_last_midi_song = 0;
int digi_last_midi_song_loop = 0;

static int digi_initialised = 0;
static int digi_atexit_initialised=0;

//added on 980905 by adb to add rotating/volume based sound kill system
static int digi_max_channels = 16;
static int next_handle = 0;
int SampleHandles[32];
void reset_sounds_on_channel(int channel);
//end edit by adb

void digi_reset_digi_sounds(void);

void digi_reset() { }

void digi_close(void)
{
	if(digi_initialised)
	{
		digi_reset_digi_sounds();
		IDirectSound_Release(lpds);
	}
	digi_initialised = 0;
}

/* Initialise audio devices. */
int digi_init()
{
 HRESULT hr;
 
 if (!digi_initialised && g_hWnd){

	 memset(&waveformat, 0, sizeof(waveformat));
	 waveformat.wFormatTag=WAVE_FORMAT_PCM;
	 waveformat.wBitsPerSample=8;
	 waveformat.nChannels = 1;
	 waveformat.nSamplesPerSec = digi_sample_rate; //11025;
	 waveformat.nBlockAlign = waveformat.nChannels * (waveformat.wBitsPerSample / 8);
	 waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	  if ((hr = DirectSoundCreate(NULL, &lpds, NULL)) != DS_OK)
	   return -1;

	  if ((hr = IDirectSound_SetCooperativeLevel(lpds, g_hWnd, DSSCL_PRIORITY)) //hWndMain
	       != DS_OK)
	   {
	    IDirectSound_Release(lpds);
	    return -1;
	   }
	
	 memset(&dsbd, 0, sizeof(dsbd));
	 dsbd.dwSize = sizeof(dsbd);
	 dsbd.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
	 dsbd.dwBufferBytes = 8192;
	 dsbd.dwReserved=0;
	 dsbd.lpwfxFormat = &waveformat;

	 digi_initialised = 1;
}

	if (!digi_atexit_initialised){
		atexit(digi_close);
		digi_atexit_initialised=1;
	}
 return 0;
}

// added 2000/01/15 Matt Mueller -- remove some duplication (and fix a big memory leak, in the kill=0 one case)
static int DS_release_slot(int slot, int kill)
{
	if (SoundSlots[slot].lpsb)
	{
		DWORD s;

		IDirectSoundBuffer_GetStatus(SoundSlots[slot].lpsb, &s);
		if (s & DSBSTATUS_PLAYING)
		{
			if (kill)
				IDirectSoundBuffer_Stop(SoundSlots[slot].lpsb);
			else
				return 0;
		}
		IDirectSoundBuffer_Release(SoundSlots[slot].lpsb);
		SoundSlots[slot].lpsb = NULL;
	}
	SoundSlots[slot].playing = 0;
	return 1;
}

void digi_stop_all_channels()
{
	int i;

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		digi_stop_sound(i);
}


static int get_free_slot()
{
 int i;

 for (i=0; i<MAX_SOUND_SLOTS; i++)
 {
		if (DS_release_slot(i, 0))
			return i;
 }
 return -1;
}

int D1vol2DSvol(fix d1v){
//multiplying by 1.5 doesn't help.  DirectSound uses dB for volume, rather than a linear scale like d1 wants.
//I had to pull out a math book, but here is the code to fix it :)  -Matt Mueller
//log x=y  <==>  x=a^y  
//   a
	 if (d1v<=0)
		 return -10000;
	 else
//		 return log2(f2fl(d1v))*1000;//no log2? hm.
		 return log(f2fl(d1v))/log(2)*1000.0;
}

// Volume 0-F1_0
int digi_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj)
{
 int ntries;
 int slot;
 HRESULT hr;

	if (!digi_initialised)
		return -1;

	// added on 980905 by adb from original source to add sound kill system
	// play at most digi_max_channel samples, if possible kill sample with low volume
	ntries = 0;

TryNextChannel:
	if ((SampleHandles[next_handle] >= 0) && (SoundSlots[SampleHandles[next_handle]].playing))
	{
		if ((SoundSlots[SampleHandles[next_handle]].volume > digi_volume) && (ntries < digi_max_channels))
		{
			//mprintf((0, "Not stopping loud sound %d.\n", next_handle));
			next_handle++;
			if (next_handle >= digi_max_channels)
				next_handle = 0;
			ntries++;
			goto TryNextChannel;
		}
		//mprintf((0, "[SS:%d]", next_handle));
		SampleHandles[next_handle] = -1;
	}
	// end edit by adb

	slot = get_free_slot();
	if (slot < 0)
		return -1;

	SoundSlots[slot].soundno = soundnum;
	SoundSlots[slot].samples = Sounddat(soundnum)->data;
	SoundSlots[slot].length = Sounddat(soundnum)->length;
	SoundSlots[slot].volume = fixmul(digi_volume, volume);
	SoundSlots[slot].pan = pan;
	SoundSlots[slot].position = 0;
	SoundSlots[slot].looped = 0;
	SoundSlots[slot].playing = 1;

	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.wBitsPerSample = Sounddat(soundnum)->bits;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = Sounddat(soundnum)->freq; //digi_sample_rate;
	waveformat.nBlockAlign = waveformat.nChannels * (waveformat.wBitsPerSample / 8);
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwReserved=0;
	dsbd.dwBufferBytes = SoundSlots[slot].length;
	dsbd.lpwfxFormat = &waveformat;

	hr = IDirectSound_CreateSoundBuffer(lpds, &dsbd, &SoundSlots[slot].lpsb, NULL);
	if (hr != DS_OK)
	{
		printf("Createsoundbuffer failed! hr=0x%X\n", (int)hr);
		abort();
	}

	{
		void *ptr1, *ptr2;
		DWORD len1, len2;

		IDirectSoundBuffer_Lock(SoundSlots[slot].lpsb, 0, Sounddat(soundnum)->length,
		                        &ptr1, &len1, &ptr2, &len2, 0);
		memcpy(ptr1, Sounddat(soundnum)->data, MIN(len1, Sounddat(soundnum)->length));
		IDirectSoundBuffer_Unlock(SoundSlots[slot].lpsb, ptr1, len1, ptr2, len2);
	}

	IDirectSoundBuffer_SetPan(SoundSlots[slot].lpsb, ((int)(f2fl(pan) * 20000.0)) - 10000);
	IDirectSoundBuffer_SetVolume(SoundSlots[slot].lpsb, D1vol2DSvol(SoundSlots[slot].volume));
	IDirectSoundBuffer_Play(SoundSlots[slot].lpsb, 0, 0, 0);

	// added on 980905 by adb to add sound kill system from original sos digi.c
	reset_sounds_on_channel(slot);
	SampleHandles[next_handle] = slot;
	next_handle++;
	if (next_handle >= digi_max_channels)
		next_handle = 0;
	// end edit by adb

	return slot;
}

// Returns the channel a sound number is playing on, or
// -1 if none.
int digi_find_channel(int soundno)
{
	if (!digi_initialised)
		return -1;

	if (soundno < 0 )
		return -1;

	if (GameSounds[soundno].data == NULL)
	{
		Int3();
		return -1;
	}

	//FIXME: not implemented
	return -1;
}

 //added on 980905 by adb to add sound kill system from original sos digi.c
void reset_sounds_on_channel( int channel )
{
 int i;

 for (i=0; i<digi_max_channels; i++)
  if (SampleHandles[i] == channel)
   SampleHandles[i] = -1;
}
//end edit by adb

//added on 980905 by adb from original source to make sfx volume work
void digi_set_digi_volume( int dvolume )
{
	dvolume = fixmuldiv( dvolume, SOUND_MAX_VOLUME, 0x7fff);
	if ( dvolume > SOUND_MAX_VOLUME )
		digi_volume = SOUND_MAX_VOLUME;
	else if ( dvolume < 0 )
		digi_volume = 0;
	else
		digi_volume = dvolume;

	if ( !digi_initialised ) return;

	digi_sync_sounds();
}
//end edit by adb

void digi_set_volume( int dvolume, int mvolume ) 
{ 
	digi_set_digi_volume(dvolume);
	digi_set_midi_volume(mvolume);
}

int digi_is_sound_playing(int soundno)
{
	int i;

	soundno = digi_xlat_sound(soundno);

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		  //changed on 980905 by adb: added SoundSlots[i].playing &&
		  if (SoundSlots[i].playing && SoundSlots[i].soundno == soundno)
		  //end changes by adb
			return 1;
	return 0;
}


 //added on 980905 by adb to make sound channel setting work
void digi_set_max_channels(int n) { 
	digi_max_channels	= n;

	if ( digi_max_channels < 1 ) 
		digi_max_channels = 1;
	if (digi_max_channels > MAX_SOUND_SLOTS)
		digi_max_channels = MAX_SOUND_SLOTS;

	if ( !digi_initialised ) return;

	digi_reset_digi_sounds();
}

int digi_get_max_channels() { 
	return digi_max_channels; 
}
// end edit by adb

void digi_reset_digi_sounds() {
 int i;

 for (i=0; i< MAX_SOUND_SLOTS; i++) {
		DS_release_slot(i, 1);
 }
 
 //added on 980905 by adb to reset sound kill system
 memset(SampleHandles, 255, sizeof(SampleHandles));
 next_handle = 0;
 //end edit by adb
}

int digi_is_channel_playing(int channel)
{
	if (!digi_initialised)
		return 0;

	return SoundSlots[channel].playing;
}

void digi_set_channel_volume(int channel, int volume)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].volume = fixmuldiv(volume, digi_volume, F1_0);
}

void digi_set_channel_pan(int channel, int pan)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].pan = pan;
}

void digi_stop_sound(int channel)
{
	SoundSlots[channel].playing=0;
	SoundSlots[channel].soundobj = -1;
	SoundSlots[channel].persistent = 0;
}

void digi_end_sound(int channel)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].soundobj = -1;
	SoundSlots[channel].persistent = 0;
}

#else
int digi_midi_song_playing = 0;
static int digi_initialised = 0;
int midi_volume = 255;

int digi_get_settings() { return 0; }
int digi_init() { digi_initialised = 1; return 0; }
void digi_reset() {}
void digi_close() {}

void digi_set_digi_volume( int dvolume ) {}
void digi_set_volume( int dvolume, int mvolume ) {}

int digi_is_sound_playing(int soundno) { return 0; }

void digi_set_max_channels(int n) {}
int digi_get_max_channels() { return 0; }

#endif

#ifdef MIDI_SOUND
// MIDI stuff follows.

void digi_stop_current_song()
{
	if ( digi_midi_song_playing ) {
	    hmp_close(hmp);
	    hmp = NULL;
	    digi_midi_song_playing = 0;
	}
}

void digi_set_midi_volume( int n )
{
	int mm_volume;

	if (n < 0)
		midi_volume = 0;
	else if (n > 127)
		midi_volume = 127;
	else
		midi_volume = n;

	// scale up from 0-127 to 0-0xffff
	mm_volume = (midi_volume << 1) | (midi_volume & 1);
	mm_volume |= (mm_volume << 8);

	if (hmp)
		midiOutSetVolume((HMIDIOUT)hmp->hmidi, mm_volume | mm_volume << 16);
}

void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop )
{       
	if (!digi_initialised) return;

        digi_stop_current_song();

       //added on 5/20/99 by Victor Rachels to fix crash/etc
        if(filename == NULL) return;
        if(midi_volume < 1) return;
       //end this section addition - VR

	if ((hmp = hmp_open(filename))) {
	    hmp_play(hmp);
	    digi_midi_song_playing = 1;
	    digi_set_midi_volume(midi_volume);
	}
	else
		printf("hmp_open failed\n");
}
void digi_pause_midi() {}
void digi_resume_midi() {}

#else
void digi_stop_current_song() {}
void digi_set_midi_volume( int n ) {}
void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop ) {}
void digi_pause_midi() {}
void digi_resume_midi() {}
#endif

#ifndef NDEBUG
void digi_debug()
{
	int i;
	int n_voices = 0;

	if (!digi_initialised)
		return;

	for (i = 0; i < digi_max_channels; i++)
	{
		if (digi_is_channel_playing(i))
			n_voices++;
	}

	mprintf_at((0, 2, 0, "DIGI: Active Sound Channels: %d/%d (HMI says %d/32)      ", n_voices, digi_max_channels, -1));
	//mprintf_at((0, 3, 0, "DIGI: Number locked sounds:  %d                          ", digi_total_locks ));
}
#endif
