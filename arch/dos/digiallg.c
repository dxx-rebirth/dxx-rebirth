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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "fix.h"
#include "object.h"
#include "mono.h"
#include "timer.h"
#include "joy.h"
#include "digi.h"
#include "sounds.h"
#include "args.h"
#include "key.h"
#include "newdemo.h"
#include "game.h"
#include "u_dpmi.h"
#include "error.h"
#include "wall.h"
#include "cfile.h"
#include "piggy.h"
#include "text.h"
#include "midiallg.h"

#include "kconfig.h"

#define _DIGI_MAX_VOLUME 128 // set lower to have difference with > F1_0 sounds

#define MIN_VOLUME 10 // minimal volume to be played (in 0-F1_0(?) range)

// patch files
#define  _MELODIC_PATCH       "melodic.bnk"
#define  _DRUM_PATCH          "drum.bnk"
#define  _DIGDRUM_PATCH       "drum32.dig"

 
static int	Digi_initialized		= 0;
static int	digi_atexit_called	= 0;			// Set to 1 if atexit(digi_close) was called

int digi_driver_board				= 0;
int digi_driver_port					= 0;
int digi_driver_irq					= 0;
int digi_driver_dma					= 0;
//int digi_midi_type                                      = 0;                    // Midi driver type
//int digi_midi_port                                      = 0;                    // Midi driver port
static int digi_max_channels		= 8;
//static int digi_driver_rate		  = 11025;			  // rate to use driver at
//static int digi_dma_buffersize  = 4096;		  // size of the dma buffer to use (4k)
int digi_timer_rate					= 9943;			// rate for the timer to go off to handle the driver system (120 Hz)
static int digi_volume				= _DIGI_MAX_VOLUME;		// Max volume
//static int midi_volume                          = 128/2;                                                // Max volume
//static int midi_system_initialized		  = 0;
//static int digi_system_initialized		  = 0;
static int timer_system_initialized		= 0;
static int digi_sound_locks[MAX_SOUNDS];
//char digi_last_midi_song[16] = "";
//char digi_last_melodic_bank[16] = "";
//char digi_last_drum_bank[16] = "";
char *digi_driver_path = NULL;

extern int midi_volume;

//static void * lpInstruments = NULL;		  // pointer to the instrument file
//static int InstrumentSize = 0;
//static void * lpDrums = NULL; 			  // pointer to the drum file
//static int DrumSize = 0;

// handle for the initialized MIDI song
//MIDI *SongHandle = NULL;


// a channel (voice) id is an int in Allegro
typedef int CHANNEL;
#define VALID_CHANNEL(ch) (ch >= 0)
#define INVALID_CHANNEL -1

#define DIGI_SLOTS DIGI_VOICES

// next_handle is a Descent internal number (hereafter reffered to as slot),
// which indexes into SampleHandles, SoundNums and SoundVolumes.
// only digi_max_channels entries are used.
// SampleHandles points to sounddriver handles (voices)
static int next_handle = 0;
static CHANNEL SampleHandles[DIGI_SLOTS] = {
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL,
    INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL };
static int SoundNums[DIGI_SLOTS] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
static uint SoundVolumes[DIGI_SLOTS] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

void digi_reset_digi_sounds();
int verify_channel_not_used_by_soundobjects( int channel );

void digi_close() {
	if (!Digi_initialized) return;

	remove_sound();
	Digi_initialized = 0;

	if ( timer_system_initialized ) {
		// Remove timer...
		timer_set_function( NULL );
    }
    timer_system_initialized = 0;
}

/* initialize sound system. 0=ok, 1=error */
int digi_init() {
	int i;

	if (!timer_system_initialized)
	{
	    allg_snd_init();
	    timer_system_initialized = 1;
	}
	if (!Digi_initialized) {
		// amount of voices we need
		// 16 for normal sounds and 16 for SoundObjects (fan, boss)
		// for DIGMID we sacrify some sounds (32 is the max).
//                reserve_voices(allegro_using_digmid() ? 16 : 32, -1);
		if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL))
			return 1;
		set_volume(255, -1);
		digi_driver_board = digi_card; // only for 0, !=0
		digi_midi_type = midi_card; // only for 0, !=0
	}
	Digi_initialized = 1;


	if (!digi_atexit_called)	{
		atexit( digi_close );
		digi_atexit_called = 1;
	}

	digi_init_sounds();
	digi_set_midi_volume( midi_volume );

	for (i=0; i<MAX_SOUNDS; i++ )
		digi_sound_locks[i] = 0;
        digi_reset_digi_sounds();

	return 0;
}

// Toggles sound system on/off
void digi_reset()	
{
	if ( Digi_initialized )	{
		digi_reset_digi_sounds();
		digi_close();
		mprintf( (0, "Sound system DISABLED.\n" ));
	} else {
		digi_init();
		mprintf( (0, "Sound system ENABLED.\n" ));
	}
}

int digi_total_locks = 0;

ubyte * digi_lock_sound_data( int soundnum )
{
	int i;

	if ( !Digi_initialized ) return NULL;
	if ( digi_driver_board <= 0 )	return NULL;

	if ( digi_sound_locks[soundnum] == 0 )	{
		digi_total_locks++;
		//mprintf(( 0, "Total sound locks = %d\n", digi_total_locks ));
		i = dpmi_lock_region( GameSounds[soundnum].data, GameSounds[soundnum].len );
		if ( !i ) Error( "Error locking sound %d\n", soundnum );
	}
	digi_sound_locks[soundnum]++;
	return GameSounds[soundnum].data;
}

void digi_unlock_sound_data( int soundnum )
{
	int i;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	Assert( digi_sound_locks[soundnum] > 0 );

	if ( digi_sound_locks[soundnum] == 1 )	{
		digi_total_locks--;
		//mprintf(( 0, "Total sound locks = %d\n", digi_total_locks ));
		i = dpmi_unlock_region( GameSounds[soundnum].data, GameSounds[soundnum].len );
		if ( !i ) Error( "Error unlocking sound %d\n", soundnum );
	}
	digi_sound_locks[soundnum]--;
}


void digi_reset_digi_sounds() {
	int i;

	for (i = 0; i < DIGI_SLOTS; i++) {
		if (VALID_CHANNEL(SampleHandles[i])) {
			deallocate_voice(SampleHandles[i]);
			SampleHandles[i] = INVALID_CHANNEL;
		}
		if (SoundNums[i] != -1) {
			digi_unlock_sound_data(SoundNums[i]);
			SoundNums[i] = -1;
        }
	}
	for (i=0; i<MAX_SOUNDS; i++ )	{
		Assert( digi_sound_locks[i] == 0 );
	}
}

void digi_set_max_channels(int n) {
	digi_max_channels	= n;

	if ( digi_max_channels < 1 ) 
		digi_max_channels = 1;
	if ( digi_max_channels > DIGI_VOICES)
		digi_max_channels = DIGI_VOICES;
}

int digi_get_max_channels()
{
	return digi_max_channels;
}

void digi_stop_all_channels()
{
	int i;

	for (i = 0; i < MAX_SOUND_SLOTS; i++)
		digi_stop_sound(i);
}

int get_free_slot() {
	int ntries = 0;
	int ret_slot;

TryNextChannel:
	// sound playing on slot <next_handle>?
	if ( (VALID_CHANNEL(SampleHandles[next_handle])) &&
	     (voice_check(SampleHandles[next_handle])) ) {
		// don't use this slot if loud sound
		if ( (SoundVolumes[next_handle] > digi_volume) &&
			 (ntries<digi_max_channels) )  {
			mprintf(( 0, "Not stopping loud sound %d (%d).\n", next_handle, SoundNums[next_handle]));
			next_handle++;
			if ( next_handle >= digi_max_channels )
				next_handle = 0;
			ntries++;
			goto TryNextChannel;
		}
		//mprintf(( 0, "[SS:%d]", next_handle ));
		if (voice_check(next_handle))
			mprintf((0,"Sound: deallocating used voice %d sound %d vol %d pos %d/%d\n", next_handle, SoundNums[next_handle], SoundVolumes[next_handle], voice_get_position(next_handle), GameSounds[SoundNums[next_handle]].len));
		deallocate_voice(SampleHandles[next_handle]);
		SampleHandles[next_handle] = INVALID_CHANNEL;
	}

	if ( SoundNums[next_handle] != -1 ) {
		digi_unlock_sound_data(SoundNums[next_handle]);
		SoundNums[next_handle] = -1;
	}
	ret_slot = next_handle;
	next_handle++;
	if ( next_handle >= digi_max_channels )
                next_handle = 0;
	return ret_slot;
}

CHANNEL digi_start_sound(int soundnum, int volume, int pan) {
	int i;
	int channel;
	int slot;

	volume = fixmul(volume, digi_volume);
	slot = get_free_slot();

	digi_lock_sound_data(soundnum);
	channel = play_sample(&GameSounds[soundnum], volume, pan >> 8, 1000, 0);
	if (channel < 0) {
		mprintf(( 1, "NOT ENOUGH SOUND SLOTS!!!\n" ));
		digi_unlock_sound_data(soundnum);
		return -1;
	}
	release_voice(channel);

	//mprintf(( 0, "Starting sound on channel %d\n", channel ));
	#ifndef NDEBUG
	verify_channel_not_used_by_soundobjects(channel);
	#endif

	// find slots pointing to just allocated channel and kill sounds on it
	// (because they can't be playing right now)
	for (i=0; i<digi_max_channels; i++ )	{
		if (SampleHandles[i] == channel)   {
			SampleHandles[i] = INVALID_CHANNEL;
			if (SoundNums[i] != -1)   {
				digi_unlock_sound_data(SoundNums[i]);
				SoundNums[i] = -1;
			}
		}
	}

	// fill slot with data of just started sample
	SampleHandles[slot] = channel;
	SoundNums[slot] = soundnum;
	SoundVolumes[slot] = volume;

	return channel;
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

int digi_is_sound_playing(int soundno)
{
	int i;

	soundno = digi_xlat_sound(soundno);

	for (i = 0; i < DIGI_VOICES; i++)
		  if (voice_check(i) == &GameSounds[soundno])
			return 1;
	return 0;
}

//-killed- void digi_set_midi_volume( int mvolume )
//-killed- {
//-killed-         int old_volume = midi_volume;
//-killed- 
//-killed-         if ( mvolume > 127 )
//-killed-                 midi_volume = 127;
//-killed-         else if ( mvolume < 0 )
//-killed-                 midi_volume = 0;
//-killed-         else
//-killed-                 midi_volume = mvolume;
//-killed- 
//-killed-         if ( (digi_midi_type > 0) )        {
//-killed-                 if (  (old_volume < 1) && ( midi_volume > 1 ) ) {
//-killed-                         if (SongHandle == NULL )
//-killed-                                 digi_play_midi_song( digi_last_midi_song, digi_last_melodic_bank, digi_last_drum_bank, 1 );
//-killed-                 }
//-killed-                 set_volume(-1, midi_volume * 2 + (midi_volume & 1));
//-killed-         }
//-killed- }

void digi_set_digi_volume( int dvolume )
{
	dvolume = fixmuldiv( dvolume, _DIGI_MAX_VOLUME, 0x7fff);
	if ( dvolume > _DIGI_MAX_VOLUME )
		digi_volume = _DIGI_MAX_VOLUME;
	else if ( dvolume < 0 )
		digi_volume = 0;
	else
		digi_volume = dvolume;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	digi_sync_sounds();
}

// 0-0x7FFF
void digi_set_volume( int dvolume, int mvolume )
{
	digi_set_midi_volume( mvolume );
	digi_set_digi_volume( dvolume );
//	mprintf(( 1, "Volume: 0x%x and 0x%x\n", digi_volume, midi_volume ));
}

int digi_is_channel_playing(int channel)
{
	int i;

	if (!digi_initialised)
		return 0;

	//FIXME: not implemented
	return 0;
}

void digi_set_channel_volume(int channel, int volume)
{
	if (!digi_initialised)
		return;

	//FIXME: not implemented
}

void digi_set_channel_pan(int channel, int pan)
{
	if (!digi_initialised)
		return;

	//FIXME: not implemented
}

void digi_stop_sound(int channel)
{
	//FIXME: not implemented
}

void digi_end_sound(int channel)
{
	if (!digi_initialised)
		return;

	//FIXME: not implemented
}

//-killed- void digi_stop_current_song()
//-killed- {
//-killed-         if (SongHandle) {
//-killed-                 destroy_midi(SongHandle);
//-killed-                 SongHandle = NULL;
//-killed-         }
//-killed- }
//-killed- 
//-killed- void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop )
//-killed- {
//-killed-         //char fname[128];
//-killed- 
//-killed-         if (!Digi_initialized) return;
//-killed-         if ( digi_midi_type <= 0 )      return;
//-killed- 
//-killed-         digi_stop_current_song();
//-killed- 
//-killed-         if ( filename == NULL ) return;
//-killed- 
//-killed-         strcpy( digi_last_midi_song, filename );
//-killed-         strcpy( digi_last_melodic_bank, melodic_bank );
//-killed-         strcpy( digi_last_drum_bank, drum_bank );
//-killed- 
//-killed-         if ( midi_volume < 1 )
//-killed- 
//-killed-         SongHandle = NULL;
//-killed- 
//-killed- #if 0 /* needs bank loading to sound right */
//-killed-         if (midi_card <= 4) { /* FM cards */
//-killed-                 int sl;
//-killed-                 sl = strlen( filename );
//-killed-                 strcpy( fname, filename ); 
//-killed-                 fname[sl-1] = 'q';
//-killed-                 SongHandle = load_hmp(fname);
//-killed-         }
//-killed- #endif
//-killed- 
//-killed-         if ( !SongHandle  )
//-killed-                 SongHandle = load_hmp(filename);
//-killed- 
//-killed-         if (SongHandle) {
//-killed-                 if (play_midi(SongHandle, loop)) {
//-killed-                         destroy_midi(SongHandle);
//-killed-                         SongHandle = NULL;
//-killed-                 }
//-killed-         }
//-killed-         if (!SongHandle) {
//-killed-                         mprintf( (1, "\nAllegro Error : %s", allegro_error ));
//-killed-         }
//-killed- }


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

int verify_channel_not_used_by_soundobjects( int channel )
{
	int i;
	if (digi_driver_board>0)	{
		for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
			if ( SoundObjects[i].flags & SOF_USED )	{
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					if ( SoundObjects[i].handle == channel )	{
						mprintf(( 0, "ERROR STARTING SOUND CHANNEL ON USED SLOT!!\n" ));
						Int3();	// Get John!
					}
				}
			}
		}
	}
	return 0;
}
#endif

#if 0 // hud sound debug info
#include "gr.h"
#include "gamefont.h"
#define VIRTUAL_VOICES 256
void show_digi_info() {
	int i, active_slots = 0, done_slots = 0, allg_used = 0;

	for (i=0; i<DIGI_SLOTS; i++)
		if (VALID_CHANNEL(SampleHandles[i])) {
			active_slots++;
			if (!voice_check(SampleHandles[i]))
				done_slots++;
		}
	for (i=0; i<VIRTUAL_VOICES; i++)
		if (voice_check(i))
			allg_used++;


	gr_set_curfont( GAME_FONT );
	gr_set_fontcolor(gr_getcolor(0,31,0),-1 );
	gr_printf(0,32,"voices: real:%d exp:%d locks:%d sluse:%d sldone:%d aused:%d slnxt:%d",
	 digi_driver->voices, digi_max_channels, digi_total_locks,
	 active_slots,done_slots,allg_used, next_handle);
}
#endif
