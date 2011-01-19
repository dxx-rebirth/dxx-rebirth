// ALSA digital audio support

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include "error.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"
#include "digi.h"
#include "sounds.h"
#include "wall.h"
#include "newdemo.h"
#include "kconfig.h"
#include "altsound.h"

#define MAX_SOUND_SLOTS 32
#define SOUND_BUFFER_SIZE 512
#define MIN_VOLUME 10

/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
static const ubyte mix8[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
  0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C,
  0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92,
  0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
  0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
  0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
  0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE,
  0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
  0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
  0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
  0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

#define SOF_USED			1		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
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

//added/changed on 980905 by adb to make sfx volume work, on 990221 by adb changed F1_0 to F1_0 / 2
#define SOUND_MAX_VOLUME (F1_0 / 2)

int digi_volume = SOUND_MAX_VOLUME;
//end edit by adb

int digi_lomem = 0;

static int digi_initialised = 0;

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
} SoundSlots[MAX_SOUND_SLOTS];

static int digi_sounds_initialized = 0;

//added on 980905 by adb to add rotating/volume based sound kill system
static int digi_max_channels = 16;
static int next_handle = 0;
int SampleHandles[32];
void reset_sounds_on_channel(int channel);
//end edit by adb

/* Threading/ALSA stuff */
#define LOCK() pthread_mutex_lock(&mutex)
#define UNLOCK() pthread_mutex_unlock(&mutex)
snd_pcm_t *snd_devhandle;
pthread_t thread_id;
pthread_mutex_t mutex;


void digi_reset_digi_sounds(void);


/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add pan support and optimize mixer
static void audio_mixcallback(void *userdata, ubyte *stream, int len)
{
 ubyte *streamend = stream + len;
 struct sound_slot *sl;

 for (sl = SoundSlots; sl < SoundSlots + MAX_SOUND_SLOTS; sl++)
 {
  if (sl->playing)
  {
   ubyte *sldata = sl->samples + sl->position, *slend = sl->samples + sl->length;
   ubyte *sp = stream;
   signed char v;
   fix vl, vr;
   int x;

   if ((x = sl->pan) & 0x8000)
   {
    vl = 0x20000 - x * 2;
    vr = 0x10000;
   }
   else
   {
    vl = 0x10000;
    vr = x * 2;
   }
   vl = fixmul(vl, (x = sl->volume));
   vr = fixmul(vr, x);
   while (sp < streamend) 
   {
    if (sldata == slend)
    {
     if (!sl->looped)
     {
      sl->playing = 0;
      break;
     }
     sldata = sl->samples;
    }
    v = *(sldata++) - 0x80;

    *sp = mix8[ *sp + fixmul(v, vl) + 0x80 ];
    sp++;

    *sp = mix8[ *sp + fixmul(v, vr) + 0x80 ];
    sp++;
   }
   sl->position = sldata - sl->samples;
  }
 }
}
//end changes by adb

void *mixer_thread(void *data) {
 int err;
 ubyte buffer[SOUND_BUFFER_SIZE];

 /* Allow ourselves to be asynchronously cancelled */
 pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
 while (1) {

   memset(buffer, 0x80, SOUND_BUFFER_SIZE);
   LOCK();
   audio_mixcallback(NULL,buffer,SOUND_BUFFER_SIZE);
   UNLOCK();
again:
   err = snd_pcm_writei(snd_devhandle, buffer, SOUND_BUFFER_SIZE/2);

   if (err == -EPIPE) {
        // Sound buffer underrun
        err = snd_pcm_prepare(snd_devhandle);
        if (err < 0) {
            con_printf(CON_CRITICAL, "Can't recover from underrun: %s\n",
                    snd_strerror(err));
        }
   } else if (err == -EAGAIN) {
        goto again;
   } else if (err != SOUND_BUFFER_SIZE/2) {
        // Each frame has size 2 bytes - hence we expect SOUND_BUFFER_SIZE/2
        // frames to be written.
        con_printf(CON_CRITICAL, "Unknown err %d: %s\n", err, snd_strerror(err));
   }
 } 
 return 0;
}


/* Initialise audio devices. */
int digi_init()
{
 int err, tmp;
 char *device = "plughw:0,0";
 snd_pcm_hw_params_t *params;
 pthread_attr_t attr;
 pthread_mutexattr_t mutexattr;

 //added on 980905 by adb to init sound kill system
 memset(SampleHandles, 255, sizeof(SampleHandles));
 //end edit by adb

 /* Open the ALSA sound device */
 if ((err = snd_pcm_open(&snd_devhandle,device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {  
     con_printf(CON_CRITICAL, "open failed: %s\n", snd_strerror( err ));  
     return -1; 
 } 

 snd_pcm_hw_params_alloca(&params);
 err = snd_pcm_hw_params_any(snd_devhandle, params);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }
 err = snd_pcm_hw_params_set_access(snd_devhandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }
 err = snd_pcm_hw_params_set_format(snd_devhandle, params, SND_PCM_FORMAT_U8);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }
 err = snd_pcm_hw_params_set_channels(snd_devhandle, params, 2);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }
 tmp = 11025;
 err = snd_pcm_hw_params_set_rate_near(snd_devhandle, params, &tmp, NULL);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }
 snd_pcm_hw_params_set_periods(snd_devhandle, params, 3, 0);
 snd_pcm_hw_params_set_buffer_size(snd_devhandle,params, (SOUND_BUFFER_SIZE*3)/2);

 err = snd_pcm_hw_params(snd_devhandle, params);
 if (err < 0) {
     con_printf(CON_CRITICAL,"ALSA: Error %s\n", snd_strerror(err));
     return -1;
 }

 /* Start the mixer thread */

 /* We really should check the results of these */
 pthread_mutexattr_init(&mutexattr);
 pthread_mutex_init(&mutex,&mutexattr);
 pthread_mutexattr_destroy(&mutexattr);
 
 if (pthread_attr_init(&attr) != 0) {
  con_printf(CON_CRITICAL, "failed to init attr\n");
  snd_pcm_close( snd_devhandle ); 
  return -1;
 }

 pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

 pthread_create(&thread_id,&attr,mixer_thread,NULL);
 pthread_attr_destroy(&attr);

 digi_initialised = 1;
 return 0;
}

/* Toggle audio */
void digi_reset() { }

/* Shut down audio */
void digi_close()
{
 if (!digi_initialised) return;
 pthread_cancel(thread_id);
 digi_initialised = 0;
 pthread_mutex_destroy(&mutex);
 snd_pcm_close(snd_devhandle);
}

static int get_free_slot()
{
 int i;
 for (i=0; i<MAX_SOUND_SLOTS; i++)
 {
  if (!SoundSlots[i].playing) return i;
 }
 return -1;
}

int digi_start_sound(int soundnum, fix volume, fix pan)
{
 int ntries;
 int slot;

 if (!digi_initialised) return -1;
 LOCK();
 //added on 980905 by adb from original source to add sound kill system
 // play at most digi_max_channel samples, if possible kill sample with low volume
 ntries = 0;

TryNextChannel:
 if ( (SampleHandles[next_handle] >= 0) && (SoundSlots[SampleHandles[next_handle]].playing)  )
 {
  if ( (SoundSlots[SampleHandles[next_handle]].volume > digi_volume) && (ntries<digi_max_channels) )
  {
   next_handle++;
   if ( next_handle >= digi_max_channels )
    next_handle = 0;
   ntries++;
   goto TryNextChannel;
  }
  SoundSlots[SampleHandles[next_handle]].playing = 0;
  SampleHandles[next_handle] = -1;
 }
 //end edit by adb

 slot = get_free_slot();
 if (slot<0) return -1;

 SoundSlots[slot].soundno = soundnum;
 SoundSlots[slot].samples = Sounddat(soundnum)->data;
 SoundSlots[slot].length = Sounddat(soundnum)->length;
 SoundSlots[slot].volume = fixmul(digi_volume, volume);
 SoundSlots[slot].pan = pan;
 SoundSlots[slot].position = 0;
 SoundSlots[slot].looped = 0;
 SoundSlots[slot].playing = 1;

 //added on 980905 by adb to add sound kill system from original sos digi.c
 reset_sounds_on_channel(slot);
 SampleHandles[next_handle] = slot;
 next_handle++;
 if ( next_handle >= digi_max_channels )
  next_handle = 0;
 //end edit by adb
 UNLOCK();
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

 if (!digi_initialised) return -1;
 LOCK();
 slot = get_free_slot();

 if (slot<0) { UNLOCK(); return -1; }


 SoundSlots[slot].soundno = SoundObjects[obj].soundnum;
 SoundSlots[slot].samples = Sounddat(SoundObjects[obj].soundnum)->data;
 SoundSlots[slot].length = Sounddat(SoundObjects[obj].soundnum)->length;
 SoundSlots[slot].volume = fixmul(digi_volume, SoundObjects[obj].volume);
 SoundSlots[slot].pan = SoundObjects[obj].pan;
 SoundSlots[slot].position = 0;
 SoundSlots[slot].looped = (SoundObjects[obj].flags & SOF_PLAY_FOREVER);
 SoundSlots[slot].playing = 1;

 SoundObjects[obj].signature = next_signature++;
 SoundObjects[obj].handle = slot;

 SoundObjects[obj].flags |= SOF_PLAYING;
 //added on 980905 by adb to add sound kill system from original sos digi.c
 reset_sounds_on_channel(slot);
 //end edit by adb
 
 UNLOCK();
 return 0;
}


// Play the given sound number.
// Volume is max at F1_0.
void digi_play_sample( int soundno, fix max_volume )
{
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );

	if (!digi_initialised) return;

        if (digi_xlat_sound(soundno) < 0 ) return;

	digi_start_sound(soundno, max_volume, F0_5);
}

// Play the given sound number. If the sound is already playing,
// restart it.
void digi_play_sample_once( int soundno, fix max_volume )
{
	int i;

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );

	if (!digi_initialised) return;

        if (digi_xlat_sound(soundno) < 0 ) return;

	LOCK();
        for (i=0; i < MAX_SOUND_SLOTS; i++)
          if (SoundSlots[i].soundno == soundno)
            SoundSlots[i].playing = 0;
	UNLOCK();
	digi_start_sound(soundno, max_volume, F0_5);
}

void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups ) // Volume from 0-0x7fff
{
	no_dups = 1;

	if ( Newdemo_state == ND_STATE_RECORDING )		{
		if ( no_dups )
			newdemo_record_sound_3d_once( soundno, angle, volume );
		else
			newdemo_record_sound_3d( soundno, angle, volume );
	}

	if (!digi_initialised) return;
        if (digi_xlat_sound(soundno) < 0 ) return;

	if (volume < MIN_VOLUME ) return;
	digi_start_sound(soundno, volume, angle);
}

void digi_get_sound_loc( vms_matrix * listener, vms_vector * listener_pos, int listener_seg, vms_vector * sound_pos, int sound_seg, fix max_volume, int *volume, int *pan, fix max_distance )
{	  
	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick( &vector_to_sound, sound_pos, listener_pos );
		
	if (distance < max_distance )	{
		int num_search_segs = f2i(max_distance/20);
		if ( num_search_segs < 1 ) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG );
		if ( path_distance > -1 )	{
			*volume = max_volume - fixdiv(path_distance,max_distance);
			if (*volume > 0 )	{
				angle_from_ear = vm_vec_delta_ang_norm(&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos(angle_from_ear,&sinang,&cosang);
				if (GameCfg.ReverseStereo) cosang *= -1;
				*pan = (cosang + F1_0)/2;
			} else {
				*volume = 0;
			}
		}
	}																					  
}

int digi_link_sound_to_object2( int soundnum, short objnum, int forever, fix max_volume, fix  max_distance )
{
	int i,volume,pan;
	object * objp;

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!digi_initialised) return -1;
        if (digi_xlat_sound(soundnum) < 0 ) return -1;
        if (Sounddat(soundnum)->data==NULL) {
		Int3();
		return -1;
	}
	if ((objnum<0)||(objnum>Highest_object_index))
		return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume,&volume, &pan, max_distance );
                digi_play_sample_3d( soundnum, pan, volume, 0 );
		return -1;
	}

       	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
        	if (SoundObjects[i].flags==0)
	           break;

	if (i==MAX_SOUND_OBJECTS) {
		return -1;
	}

	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_OBJ;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].lo_objnum = objnum;
	SoundObjects[i].lo_objsignature = Objects[objnum].signature;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	SoundObjects[i].soundnum = soundnum;

	objp = &Objects[SoundObjects[i].lo_objnum];
	digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, 
                       &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

	if (!forever || SoundObjects[i].volume >= MIN_VOLUME)
	       digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume )
{ return digi_link_sound_to_object2( soundnum, objnum, forever, max_volume, 256*F1_0); }

int digi_link_sound_to_pos2( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance )
{
	int i, volume, pan;
//	int soundnum;

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!digi_initialised) return -1;
        if (digi_xlat_sound(soundnum) < 0 ) return -1;
        if (Sounddat(soundnum)->data==NULL) {
                Int3();
		return -1;
	}

	if ((segnum<0)||(segnum>Highest_segment_index))
		return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance );
                digi_play_sample_3d( soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;
	
	if (i==MAX_SOUND_OBJECTS) {
		return -1;
	}


	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_POS;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].lp_segnum = segnum;
	SoundObjects[i].lp_sidenum = sidenum;
	SoundObjects[i].lp_position = *pos;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, 
					   &SoundObjects[i].lp_position, SoundObjects[i].lp_segnum,
					   SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );
	
	if (!forever || SoundObjects[i].volume >= MIN_VOLUME)
		digi_start_sound_object(i);

	return SoundObjects[i].signature;
}

int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume )
{
	return digi_link_sound_to_pos2( soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256 );
}

void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum )
{
	int i,killed;

	if (!digi_initialised) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_POS) )	{
			if ((SoundObjects[i].lp_segnum == segnum) && (SoundObjects[i].soundnum==soundnum ) && (SoundObjects[i].lp_sidenum==sidenum) ) {
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					LOCK();
				        SoundSlots[SoundObjects[i].handle].playing = 0;
					UNLOCK();
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
}

void digi_kill_sound_linked_to_object( int objnum )
{
	int i,killed;

	if (!digi_initialised) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	{
			if (SoundObjects[i].lo_objnum == objnum)   {
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
				     LOCK();
                                     SoundSlots[SoundObjects[i].handle].playing = 0;
				     UNLOCK();
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
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
					LOCK();
					if (!SoundSlots[SoundObjects[i].handle].playing) {
						UNLOCK();
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
						continue;		// Go on to next sound...
					}
					UNLOCK();
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
					     LOCK();
					     SoundSlots[SoundObjects[i].handle].playing = 0;
					     UNLOCK();
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
						LOCK();
                                        	SoundSlots[SoundObjects[i].handle].playing = 0;
						UNLOCK();
						SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
					}
				} else {
					if (!(SoundObjects[i].flags & SOF_PLAYING))	{
						digi_start_sound_object(i);
					} else {
						LOCK();
					        SoundSlots[SoundObjects[i].handle].volume = fixmuldiv(SoundObjects[i].volume,digi_volume,F1_0);
						UNLOCK();
					}
				}
			}
				
			if (oldpan != SoundObjects[i].pan) 	{
				if (SoundObjects[i].flags & SOF_PLAYING) {
					LOCK();
                                        SoundSlots[SoundObjects[i].handle].pan = SoundObjects[i].pan;
					UNLOCK();
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
				LOCK();
			        SoundSlots[SoundObjects[i].handle].playing=0;
				UNLOCK();
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

void digi_set_volume( int dvolume, int mvolume ) { }

int digi_is_sound_playing(int soundno)
{
	int i;

	LOCK();
	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		  //changed on 980905 by adb: added SoundSlots[i].playing &&
		  if (SoundSlots[i].playing && SoundSlots[i].soundno == soundno)
		  //end changes by adb
		  { UNLOCK();	return 1; }
	UNLOCK();
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

 LOCK();
 for (i=0; i< MAX_SOUND_SLOTS; i++)
  SoundSlots[i].playing=0;
 UNLOCK();
 
 //added on 980905 by adb to reset sound kill system
 memset(SampleHandles, 255, sizeof(SampleHandles));
 next_handle = 0;
 //end edit by adb
}
