/* $Id: digi.c,v 1.11 2004-05-22 23:01:20 btb Exp $ */
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


#define SOF_USED                1               // Set if this sample is used
#define SOF_PLAYING             2               // Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once

typedef struct sound_object {
	short		signature;		// A unique signature to this sound
	ubyte		flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	fix		max_volume;		// Max volume that this sound is playing at
	fix		max_distance;	        // The max distance that this sound can be heard at...
	int		volume;			// Volume that this sound is playing at
	int 		pan;			// Pan value that this sound is playing at
	int		handle; 		// What handle this sound is playing on.  Valid only if SOF_PLAYING is set.
	short		soundnum;		// The sound number that is playing
	union {	
		struct {
			short		segnum; 			// Used if SOF_LINK_TO_POS field is used
			short		sidenum;
			vms_vector	position;
		}pos;
		struct {
			short		 objnum;			 // Used if SOF_LINK_TO_OBJ field is used
			short		 objsignature;
		}obj;
	}link;
} sound_object;
#define lp_segnum link.pos.segnum
#define lp_sidenum link.pos.sidenum
#define lp_position link.pos.position

#define lo_objnum link.obj.objnum
#define lo_objsignature link.obj.objsignature

#define MAX_SOUND_OBJECTS 16
sound_object SoundObjects[MAX_SOUND_OBJECTS];
short next_signature=0;


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


int digi_lomem = 0;
int midi_volume = 255;
int digi_midi_song_playing = 0;
int digi_last_midi_song = 0;
int digi_last_midi_song_loop = 0;

static int digi_initialised = 0;
static int digi_atexit_initialised=0;

static int digi_sounds_initialized = 0;

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

int digi_start_sound(int soundnum, fix volume, fix pan)
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

 //added on 980905 by adb to add sound kill system from original sos digi.c
void reset_sounds_on_channel( int channel )
{
 int i;

 for (i=0; i<digi_max_channels; i++)
  if (SampleHandles[i] == channel)
   SampleHandles[i] = -1;
}
//end edit by adb

int digi_start_sound_object(int obj)
{
 int slot;
 HRESULT hr;

	if (!digi_initialised)
		return -1;

	slot = get_free_slot();

	if (slot < 0)
		return -1;


	SoundSlots[slot].soundno = SoundObjects[obj].soundnum;
	SoundSlots[slot].samples = Sounddat(SoundObjects[obj].soundnum)->data;
	SoundSlots[slot].length = Sounddat(SoundObjects[obj].soundnum).length;
	SoundSlots[slot].volume = fixmul(digi_volume, SoundObjects[obj].volume);
	SoundSlots[slot].pan = SoundObjects[obj].pan;
	SoundSlots[slot].position = 0;
	SoundSlots[slot].looped = (SoundObjects[obj].flags & SOF_PLAY_FOREVER);
	SoundSlots[slot].playing = 1;

	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.wBitsPerSample = Sounddat(SoundObjects[obj].soundnum)->bits;
	waveformat.nChannels = 1;
	waveformat.nSamplesPerSec = Sounddat(SoundObjects[obj].soundnum)->freq; //digi_sample_rate;
	waveformat.nBlockAlign = waveformat.nChannels * (waveformat.wBitsPerSample / 8);
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwReserved = 0;
	dsbd.dwBufferBytes = SoundSlots[slot].length;
	dsbd.lpwfxFormat = &waveformat;

	hr = IDirectSound_CreateSoundBuffer(lpds, &dsbd, &SoundSlots[slot].lpsb, NULL);
	if (hr != DS_OK)
	{
		abort();
	}

	{
		void *ptr1, *ptr2;
		DWORD len1, len2;

		IDirectSoundBuffer_Lock(SoundSlots[slot].lpsb, 0, SoundSlots[slot].length,
		                        &ptr1, &len1, &ptr2, &len2, 0);
		memcpy(ptr1, SoundSlots[slot].samples, MIN(len1, (int)SoundSlots[slot].length));
		IDirectSoundBuffer_Unlock(SoundSlots[slot].lpsb, ptr1, len1, ptr2, len2);
	}
	IDirectSoundBuffer_SetPan(SoundSlots[slot].lpsb, ((int)(f2fl(SoundSlots[slot].pan) * 20000)) - 10000);
	IDirectSoundBuffer_SetVolume(SoundSlots[slot].lpsb, D1vol2DSvol(SoundSlots[slot].volume));
	IDirectSoundBuffer_Play(SoundSlots[slot].lpsb, 0, 0, SoundSlots[slot].looped ? DSBPLAY_LOOPING : 0);

	SoundObjects[obj].signature = next_signature++;
	SoundObjects[obj].handle = slot;

	SoundObjects[obj].flags |= SOF_PLAYING;
	// added on 980905 by adb to add sound kill system from original sos digi.c
	reset_sounds_on_channel(slot);
	// end edit by adb

	return 0;
}


// Play the given sound number.
// Volume is max at F1_0.
void digi_play_sample( int soundno, fix max_volume )
{
#ifdef NEWDEMO
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_sound(soundno);
#endif
	if (!digi_initialised)
		return;
	if (digi_xlat_sound(soundno) < 0)
		return;

	if (soundno < 0 ) return;

	digi_start_sound(soundno, max_volume, F0_5);
}

// Play the given sound number. If the sound is already playing,
// restart it.
void digi_play_sample_once( int soundno, fix max_volume )
{
	int i;

#ifdef NEWDEMO
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_sound(soundno);
#endif
	if (!digi_initialised)
		return;
	if(digi_xlat_sound(soundno) < 0)
		return;

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		if (SoundSlots[i].soundno == soundno)
		{
			DS_release_slot(i, 1);
		}

	digi_start_sound(soundno, max_volume, F0_5);
}

void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups ) // Volume from 0-0x7fff
{
	no_dups = 1;

#ifdef NEWDEMO
	if (Newdemo_state == ND_STATE_RECORDING)
	{
		if (no_dups)
			newdemo_record_sound_3d_once(soundno, angle, volume);
		else
			newdemo_record_sound_3d(soundno, angle, volume);
	}
#endif
	if (!digi_initialised)
		return;
	if (digi_xlat_sound(soundno) < 0)
		return;
	if (volume < MIN_VOLUME)
		return;

	digi_start_sound(soundno, volume, angle);
}

void digi_get_sound_loc( vms_matrix * listener, vms_vector * listener_pos, int listener_seg, vms_vector * sound_pos, int sound_seg, fix max_volume, int *volume, int *pan, fix max_distance )
{	  
	vms_vector vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	max_distance = (max_distance * 5) / 4;  // Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick( &vector_to_sound, sound_pos, listener_pos );
		
	if (distance < max_distance )	{
		int num_search_segs = f2i(max_distance/20);
		if ( num_search_segs < 1 ) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG );
		if ( path_distance > -1 )	{
			*volume = max_volume - fixdiv(path_distance,max_distance);
			//mprintf( (0, "Sound path distance %.2f, volume is %d / %d\n", f2fl(distance), *volume, max_volume ));
			if (*volume > 0 )	{
				angle_from_ear = vm_vec_delta_ang_norm(&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos(angle_from_ear,&sinang,&cosang);
				//mprintf( (0, "volume is %.2f\n", f2fl(*volume) ));
				if (Config_channels_reversed) cosang *= -1;
				*pan = (cosang + F1_0)/2;
			} else {
				*volume = 0;
			}
		}
	}																					  
}

int digi_link_sound_to_object2(int soundnum, short objnum, int forever, fix max_volume, fix max_distance)
{
	int i, volume, pan;
	object *objp;

	if (max_volume < 0)
		return -1;
	if (!digi_initialised)
		return -1;
	if (digi_xlat_sound(soundnum) < 0)
		return -1;
	if (Sounddat(soundnum)->data == NULL)
	{
		Int3();
		return -1;
	}
	if ((objnum < 0) || (objnum > Highest_object_index))
		return -1;

	if (!forever)
	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc(&Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume, &volume, &pan, max_distance);
		digi_play_sample_3d(soundnum, pan, volume, 0);
		return -1;
	}

	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if (SoundObjects[i].flags == 0)
			break;

	if (i == MAX_SOUND_OBJECTS)
	{
		mprintf((1, "Too many sound objects!\n"));
		return -1;
	}

	SoundObjects[i].signature = next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_OBJ;
	if (forever)
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].lo_objnum = objnum;
	SoundObjects[i].lo_objsignature = Objects[objnum].signature;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	SoundObjects[i].soundnum = soundnum;
	objp = &Objects[SoundObjects[i].lo_objnum];
	digi_get_sound_loc(&Viewer->orient, &Viewer->pos, Viewer->segnum,
	                   &objp->pos, objp->segnum, SoundObjects[i].max_volume,
	                   &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance);

	if (!forever || SoundObjects[i].volume >= MIN_VOLUME)
	       digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume )
{
	return digi_link_sound_to_object2(soundnum, objnum, forever, max_volume, 256 * F1_0);
}

int digi_link_sound_to_pos2(int soundnum, short segnum, short sidenum, vms_vector *pos, int forever, fix max_volume, fix max_distance)
{
	int i, volume, pan;

	if (max_volume < 0 )
		return -1;
	if (!digi_initialised)
		return -1;
	if (digi_xlat_sound(soundnum) < 0)
		return -1;
	if (Sounddat(soundnum)->data == NULL)
	{
		Int3();
		return -1;
	}

	if (!forever)
	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc(&Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance);
		digi_play_sample_3d(org_soundnum, pan, volume, 0);
		return -1;
	}

	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if (SoundObjects[i].flags == 0)
			break;

	if (i == MAX_SOUND_OBJECTS)
	{
		mprintf((1, "Too many sound objects!\n"));
		return -1;
	}


	SoundObjects[i].signature = next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_POS;
	if (forever)
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].lp_segnum = segnum;
	SoundObjects[i].lp_sidenum = sidenum;
	SoundObjects[i].lp_position = *pos;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	digi_get_sound_loc(&Viewer->orient, &Viewer->pos, Viewer->segnum,
					   &SoundObjects[i].lp_position, SoundObjects[i].lp_segnum,
					   SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance);

	if (!forever || SoundObjects[i].volume >= MIN_VOLUME)
		digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume )
{
	return digi_link_sound_to_pos2(soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256);
}

void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum )
{
	int i, killed;

	if (!digi_initialised)
		return;

	killed = 0;

	for (i = 0; i < MAX_SOUND_OBJECTS; i++)
		if ((SoundObjects[i].flags & SOF_USED) &&
		    (SoundObjects[i].flags & SOF_LINK_TO_POS) &&
		    (SoundObjects[i].lp_segnum == segnum) &&
		    (SoundObjects[i].soundnum == soundnum) &&
		    (SoundObjects[i].lp_sidenum==sidenum))
		{
			if (SoundObjects[i].flags & SOF_PLAYING)
			{
				DS_release_slot(SoundObjects[i].handle, 1);
			}
			SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
			killed++;
		}

		// If this assert happens, it means that there were 2 sounds
		// that got deleted. Weird, get John.
		if (killed > 1)
		{
			mprintf((1, "ERROR: More than 1 sounds were deleted from seg %d\n", segnum));
		}
}

void digi_kill_sound_linked_to_object( int objnum )
{
	int i, killed;

	if (!digi_initialised) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	{
			if (SoundObjects[i].lo_objnum == objnum)   {
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					DS_release_slot(SoundObjects[i].handle, 1);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from object %d\n", objnum ));
	}
}

void digi_sync_sounds()
{
	int i;
	int oldvolume, oldpan;

	if (!digi_initialised) return;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			oldvolume = SoundObjects[i].volume;
			oldpan = SoundObjects[i].pan;

			if ( !(SoundObjects[i].flags & SOF_PLAY_FOREVER) )	{
			 	// Check if its done.
				if (SoundObjects[i].flags & SOF_PLAYING) {
					if (!SoundSlots[SoundObjects[i].handle].playing) {
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
						continue;		// Go on to next sound...
					}
				}
			}			
		
			if ( SoundObjects[i].flags & SOF_LINK_TO_POS )	{
				digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, 
								&SoundObjects[i].lp_position, SoundObjects[i].lp_segnum,
								SoundObjects[i].max_volume,
                                &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

			} else if ( SoundObjects[i].flags & SOF_LINK_TO_OBJ )	{
				object * objp;
	
				objp = &Objects[SoundObjects[i].lo_objnum];
		
				if ((objp->type==OBJ_NONE) || (objp->signature!=SoundObjects[i].lo_objsignature))  {
					// The object that this is linked to is dead, so just end this sound if it is looping.
					if ( (SoundObjects[i].flags & SOF_PLAYING)  && (SoundObjects[i].flags & SOF_PLAY_FOREVER))	{
						DS_release_slot(SoundObjects[i].handle, 1);
					}
					SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;		// Go on to next sound...
				} else {
					digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, 
	                                &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                                   &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );
				}
			}
			 
			if (oldvolume != SoundObjects[i].volume) 	{
				if ( SoundObjects[i].volume < MIN_VOLUME )	 {
					// Sound is too far away, so stop it from playing.
					if ((SoundObjects[i].flags & SOF_PLAYING)&&(SoundObjects[i].flags & SOF_PLAY_FOREVER))	{
						DS_release_slot(SoundObjects[i].handle, 1);
						SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
					}
				} else {
					if (!(SoundObjects[i].flags & SOF_PLAYING))	{
						digi_start_sound_object(i);
					} else {
					        SoundSlots[SoundObjects[i].handle].volume = fixmuldiv(SoundObjects[i].volume,digi_volume,F1_0);
							IDirectSoundBuffer_SetVolume(SoundSlots[SoundObjects[i].handle].lpsb, D1vol2DSvol(SoundSlots[SoundObjects[i].handle].volume));
					}
				}
			}
				
			if (oldpan != SoundObjects[i].pan) 	{
				if (SoundObjects[i].flags & SOF_PLAYING)
				{
					SoundSlots[SoundObjects[i].handle].pan = SoundObjects[i].pan;
					IDirectSoundBuffer_SetPan(SoundSlots[SoundObjects[i].handle].lpsb, ((int)(f2fl(SoundObjects[i].pan) * 20000.0)) - 10000);
				}
			}
		}
	}
}

void digi_init_sounds()
{
	int i;

	if (!digi_initialised) return;

	digi_reset_digi_sounds();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if (digi_sounds_initialized) {
			if ( SoundObjects[i].flags & SOF_PLAYING )	{
				DS_release_slot(SoundObjects[i].handle, 1);
			}
		}
		SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	digi_sounds_initialized = 1;
}

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


void digi_pause_all() { }
void digi_resume_all() { }
void digi_stop_all() { }

 //added on 980905 by adb to make sound channel setting work
void digi_set_max_channels(int n) { 
	digi_max_channels	= n;

	if ( digi_max_channels < 1 ) 
		digi_max_channels = 1;
	if ( digi_max_channels > (MAX_SOUND_SLOTS-MAX_SOUND_OBJECTS) ) 
		digi_max_channels = (MAX_SOUND_SLOTS-MAX_SOUND_OBJECTS);

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
#else
int digi_lomem = 0;
int digi_midi_song_playing = 0;
static int digi_initialised = 0;
int midi_volume = 255;

int digi_get_settings() { return 0; }
int digi_init() { digi_initialised = 1; return 0; }
void digi_reset() {}
void digi_close() {}

void digi_play_sample( int sndnum, fix max_volume ) {}
void digi_play_sample_once( int sndnum, fix max_volume ) {}
int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume ) { return 0; }
int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume ) { return 0; }
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for max_distance.
int digi_link_sound_to_object2( int soundnum, short objnum, int forever, fix max_volume, fix  max_distance ) { return 0; }
int digi_link_sound_to_pos2( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance ) { return 0; }

void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups ) {} // Volume from 0-0x7fff

void digi_init_sounds() {}
void digi_sync_sounds() {}
void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum ) {}
void digi_kill_sound_linked_to_object( int objnum ) {}

void digi_set_digi_volume( int dvolume ) {}
void digi_set_volume( int dvolume, int mvolume ) {}

int digi_is_sound_playing(int soundno) { return 0; }

void digi_pause_all() {}
void digi_resume_all() {}
void digi_stop_all() {}

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
#else
void digi_stop_current_song() {}
void digi_set_midi_volume( int n ) {}
void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop ) {}
#endif
