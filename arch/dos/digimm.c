// SDL digital audio support

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mm_drv.h"
#include "timer.h"

//#include "SDL.h"
//#include "SDL_audio.h"

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
#include "midiallg.h"

int digi_driver_board                                   = 0;
int digi_driver_port					= 0;
int digi_driver_irq					= 0;
int digi_driver_dma					= 0;
//int digi_midi_type                                    = 0;                    // Midi driver type
//int digi_midi_port                                    = 0;                    // Midi driver port
int digi_timer_rate					= 9943;			// rate for the timer to go off to handle the driver system (120 Hz)

#ifndef ALLG_MIDI
/* stub vars/functions for midi */
int digi_midi_type                                      = 0;
int digi_midi_port                                     = 0;

void digi_set_midi_volume( int mvolume ) {}
void digi_play_midi_song( char * filename, char * melodic_bank,
char * drum_bank, int loop ) {}
void digi_midi_pause() {}
void digi_midi_resume() {}
void digi_midi_stop() {}
#endif


//added on 980905 by adb to add inline fixmul for mixer on i386
#ifdef __i386__
#define do_fixmul(x,y)				\
({						\
	int _ax, _dx;				\
	asm("imull %2\n\tshrdl %3,%1,%0"	\
	    : "=a"(_ax), "=d"(_dx)		\
	    : "rm"(y), "i"(16), "0"(x));	\
	_ax;					\
})
extern inline fix fixmul(fix x, fix y) { return do_fixmul(x,y); }
#endif
//end edit by adb

//changed on 980905 by adb to increase number of concurrent sounds
#define MAX_SOUND_SLOTS 32
//end changes by adb
#define SOUND_BUFFER_SIZE 512

#define MIN_VOLUME 10

/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
static const unsigned char mix8[] =
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


//added/changed on 980905 by adb to make sfx volume work
#define SOUND_MAX_VOLUME (F1_0/2) // don't use real max like in original
int digi_volume = SOUND_MAX_VOLUME;
//end edit by adb

static int digi_initialised = 0;
static int timer_system_initialized = 0;

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
	int soundobj;   // Which soundobject is on this channel
	int persistent; // This can't be pre-empted
} SoundSlots[MAX_SOUND_SLOTS];

//static SDL_AudioSpec WaveSpec;

static int digi_max_channels = 16;

static int next_channel = 0;

/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add pan support and optimize mixer
ULONG VC_WriteBytes(SBYTE *stream, ULONG len)
{
 unsigned char *streamend;
 struct sound_slot *sl;

//    if (grd_curscreen)
//	  grd_curscreen->sc_canvas.cv_bitmap.bm_data[8]++;
 len &= ~1; /* stereo -> always write 2 byte pairs */
 streamend = stream + len;

#if 0
{
 static int n = 0;
 while(stream < streamend) {
    *(stream++) = (n & 256) ? 256 - (n & 255) : (n & 255);
    n++;
 }
 return len;
}
#endif

 memset(stream, 0x80, len);

 for (sl = SoundSlots; sl < SoundSlots + MAX_SOUND_SLOTS; sl++)
 {
  if (sl->playing)
  {
   unsigned char *sldata = sl->samples + sl->position, *slend = sl->samples + sl->length;
   unsigned char *sp = stream;
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
    *(sp++) = mix8[ *sp + fixmul(v, vl) + 0x80 ];
    *(sp++) = mix8[ *sp + fixmul(v, vr) + 0x80 ];
   }
   sl->position = sldata - sl->samples;
  }
 }
 return len;
}
//end changes by adb

extern MDRIVER drv_sb;
MDRIVER *drv = &drv_sb;
char allegro_error[128];

int md_mode = DMODE_STEREO;
int md_mixfreq = digi_sample_rate; //11025;
int md_dmabufsize = 1024;

void install_int_ex(void (*)(), long speed);
void remove_int(void(*)());

void mm_timer() {
    drv->Update();
//    if (grd_curscreen)
//	  (*grd_curscreen->sc_canvas.cv_bitmap.bm_data)++;
}

/* Initialise audio devices. */
int digi_init()
{
 #if 0
 WaveSpec.freq = digi_sample_rate; //11025;
 WaveSpec.format = AUDIO_U8 | AUDIO_STEREO;
 WaveSpec.samples = SOUND_BUFFER_SIZE;
 WaveSpec.callback = audio_mixcallback;

 if ( SDL_OpenAudio(&WaveSpec, NULL) < 0 ) {
  printf("Couldn't open audio: %s\n", SDL_GetError());
  exit(2);
 }
 SDL_PauseAudio(0);
#endif
 if (!drv->Init()) {
	printf("Couldn't open audio: %s", myerr);
	return -1;
 }
 drv->PlayStart();

 if (!timer_system_initialized)
 {
  #ifdef ALLG_MIDI
  install_int_ex(mm_timer, digi_timer_rate);
  #else
  timer_set_function(mm_timer);
  #endif
  timer_system_initialized = 1;
 }

 #ifdef ALLG_MIDI
 digi_midi_init();
 #endif

 atexit(digi_close);
 digi_initialised = 1;
 return 0;
}

/* Toggle audio */
void digi_reset() { }

/* Shut down audio */
void digi_close()
{
 if (!digi_initialised) return;
 digi_initialised = 0;
 drv->PlayStop();
 drv->Exit();
 if (timer_system_initialized)
 {
  #ifdef ALLG_MIDI
  remove_int(mm_timer);
  #else
  timer_set_function(NULL);
  #endif
  timer_system_initialized = 0;
 }
 #ifdef ALLG_MIDI
 digi_midi_close();
 #endif
}

void digi_stop_all_channels()
{
	int i;

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		digi_stop_sound(i);
}


extern void digi_end_soundobj(int channel);	
extern int SoundQ_channel;
extern void SoundQ_end();
int verify_sound_channel_free(int channel);

// Volume 0-F1_0
int digi_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj)
{
	int i, starting_channel;

	if (!digi_initialised) return -1;

	if (soundnum < 0) return -1;

	Assert(GameSounds[soundnum].data != (void *)-1);

	starting_channel = next_channel;

	while(1)
	{
		if (!SoundSlots[next_channel].playing)
			break;

		if (!SoundSlots[next_channel].persistent)
			break;	// use this channel!	

		next_channel++;
		if (next_channel >= digi_max_channels)
			next_channel = 0;
		if (next_channel == starting_channel)
		{
			mprintf((1, "OUT OF SOUND CHANNELS!!!\n"));
			return -1;
		}
	}
	if (SoundSlots[next_channel].playing)
	{
		SoundSlots[next_channel].playing = 0;
		if (SoundSlots[next_channel].soundobj > -1)
		{
			digi_end_soundobj(SoundSlots[next_channel].soundobj);
		}
		if (SoundQ_channel == next_channel)
			SoundQ_end();
	}

#ifndef NDEBUG
	verify_sound_channel_free(next_channel);
#endif

	SoundSlots[next_channel].soundno = soundnum;
	SoundSlots[next_channel].samples = GameSounds[soundnum].data;
	SoundSlots[next_channel].length = GameSounds[soundnum].length;
	SoundSlots[next_channel].volume = fixmul(digi_volume, volume);
	SoundSlots[next_channel].pan = pan;
	SoundSlots[next_channel].position = 0;
	SoundSlots[next_channel].looped = looping;
	SoundSlots[next_channel].playing = 1;
	SoundSlots[next_channel].soundobj = soundobj;
	SoundSlots[next_channel].persistent = 0;
	if ((soundobj > -1) || (looping) || (volume > F1_0))
		SoundSlots[next_channel].persistent = 1;

	i = next_channel;
	next_channel++;
	if (next_channel >= digi_max_channels)
		next_channel = 0;

	return i;
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
 digi_set_midi_volume(mvolume);
 digi_set_digi_volume(dvolume);
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

	digi_stop_all_channels();
}

int digi_get_max_channels() { 
	return digi_max_channels; 
}
// end edit by adb

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


// mikmod stubs...
BOOL	VC_Init(void) { return 1; }
void    VC_Exit(void) { }
BOOL    VC_SetNumVoices(void) { return 0; }
ULONG   VC_SampleSpace(int type) { return 0; }
ULONG   VC_SampleLength(int type, SAMPLE *s) { return 0; }

BOOL    VC_PlayStart(void) { return 0; }
void    VC_PlayStop(void) { }

#if 0
SWORD   VC_SampleLoad(SAMPLOAD *sload, int type, FILE *fp) { return 0; }
#else
SWORD	VC_SampleLoad(FILE *fp,ULONG size,ULONG reppos,ULONG repend,UWORD flags) { return 0; }
#endif
void    VC_SampleUnload(SWORD handle) { }

void    VC_WriteSamples(SBYTE *buf,ULONG todo) { }
void    VC_SilenceBytes(SBYTE *buf,ULONG todo) { }

#if 0
void	VC_VoiceSetVolume(UBYTE voice, UWORD vol) { }
void    VC_VoiceSetPanning(UBYTE voice, ULONG pan) { }
#else
void	VC_VoiceSetVolume(UBYTE voice, UBYTE vol) { }
void	VC_VoiceSetPanning(UBYTE voice, UBYTE pan) { }
#endif
void    VC_VoiceSetFrequency(UBYTE voice, ULONG frq) { }
void    VC_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags) { }

void    VC_VoiceStop(UBYTE voice) { }
BOOL    VC_VoiceStopped(UBYTE voice) { return 0; }
void    VC_VoiceReleaseSustain(UBYTE voice) { }
SLONG   VC_VoiceGetPosition(UBYTE voice) { return 0; }
ULONG   VC_VoiceRealVolume(UBYTE voice) { return 0; }
char *myerr;
