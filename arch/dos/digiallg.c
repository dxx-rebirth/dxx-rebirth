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
int digi_lomem 						= 0;
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

#define SOF_USED			1		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once

typedef int WORD;

typedef struct sound_object {
	short		signature;		// A unique signature to this sound
	ubyte		flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	fix			max_volume;		// Max volume that this sound is playing at
	fix			max_distance;	// The max distance that this sound can be heard at...
	int			volume;			// Volume that this sound is playing at
	int 		pan;			// Pan value that this sound is playing at
	WORD		handle; 		// What handle this sound is playing on.  Valid only if SOF_PLAYING is set.
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

int digi_sounds_initialized=0;


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

int digi_xlat_sound(int soundno)
{
	if ( soundno < 0 ) return -1;

	if ( digi_lomem )	{
		soundno = AltSounds[soundno];
		if ( soundno == 255 ) return -1;
	}
	return Sounds[soundno];
}

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

void reset_slots_on_channel( int channel ) {
	int i;

	if ( !Digi_initialized ) return;
	if ( digi_driver_board <= 0 )	return;

	for (i=0; i<DIGI_SLOTS; i++) {
		if (SampleHandles[i] == channel) {
			SampleHandles[i] = INVALID_CHANNEL;
			if (SoundNums[i] != -1)   {
				digi_unlock_sound_data(SoundNums[i]);
				SoundNums[i] = -1;
			}
		}
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

int digi_is_sound_playing(int soundno)
{
	int i;

	soundno = digi_xlat_sound(soundno);

	for (i = 0; i < DIGI_VOICES; i++)
		  if (voice_check(i) == &GameSounds[soundno])
			return 1;
	return 0;
}

void digi_play_sample_once( int soundno, fix max_volume ) {
	int i;

#ifdef NEWDEMO
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );
#endif
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	if (soundno < 0 ) return;

	for (i = 0; i < DIGI_VOICES; i++)
	    if (voice_check(i) == &GameSounds[soundno])
			deallocate_voice(i);

	digi_start_sound(soundno, max_volume, F0_5);
}

void digi_play_sample( int soundno, fix max_volume ) {
#ifdef NEWDEMO
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );
#endif
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	if (soundno < 0 ) return;

	digi_start_sound(soundno, max_volume, F0_5);
}

void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups )
{
	no_dups = 1;

#ifdef NEWDEMO
	if ( Newdemo_state == ND_STATE_RECORDING )		{
		if ( no_dups )
			newdemo_record_sound_3d_once( soundno, angle, volume );
		else
			newdemo_record_sound_3d( soundno, angle, volume );
	}
#endif
	soundno = digi_xlat_sound(soundno);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;
	if (soundno < 0 ) return;

	if (volume < MIN_VOLUME ) return;

	digi_start_sound(soundno, volume, angle);
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

void digi_init_sounds()
{
	int i;

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	digi_reset_digi_sounds();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if (digi_sounds_initialized) {
			if ( SoundObjects[i].flags & SOF_PLAYING )	{
				deallocate_voice(SoundObjects[i].handle);
			}
		}
		SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	digi_sounds_initialized = 1;
}

void digi_start_sound_object(int i)
{
	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	// this doesn't use digi_lock_sound because these sounds are
	// never unlocked, so we couldn't check if we unlocked all other sounds then
	if (!dpmi_lock_region( GameSounds[SoundObjects[i].soundnum].data, GameSounds[SoundObjects[i].soundnum].len ))
		Error( "Error locking sound object %d\n", SoundObjects[i].soundnum );
	
	SoundObjects[i].signature=next_signature++;

	SoundObjects[i].handle = play_sample(&GameSounds[SoundObjects[i].soundnum],
	 fixmuldiv(SoundObjects[i].volume,digi_volume,F1_0), /* 0-255 */
	 SoundObjects[i].pan >> 8, /* 0-255 */
	 1000,
	 (SoundObjects[i].flags & SOF_PLAY_FOREVER) ? 1 : 0
	);
	if (SoundObjects[i].handle < 0) {
		mprintf(( 1, "NOT ENOUGH SOUND SLOTS!!! (SoundObject)\n" ));
		//digi_unlock_sound(SoundObjects[i].soundnum); //never unlocked...
	} else {
		SoundObjects[i].flags |= SOF_PLAYING;
		reset_slots_on_channel( SoundObjects[i].handle );
	}
}

int digi_link_sound_to_object2( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance )
{
	int i,volume,pan;
	object * objp;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!Digi_initialized) return -1;
	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return -1;
	}
	if ((objnum<0)||(objnum>Highest_object_index))
		return -1;
	if (digi_driver_board<1) return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume,&volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;
	
	if (i==MAX_SOUND_OBJECTS) {
		mprintf((1, "Too many sound objects!\n" ));
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
{																									// 10 segs away
	return digi_link_sound_to_object2( soundnum, objnum, forever, max_volume, 256*F1_0	);
}

int digi_link_sound_to_pos2( int org_soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance )
{
	int i, volume, pan;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (!Digi_initialized) return -1;
	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return -1;
	}
	if (digi_driver_board<1) return -1;

	if ((segnum<0)||(segnum>Highest_segment_index))
		return -1;

	if ( !forever )	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;
	
	if (i==MAX_SOUND_OBJECTS) {
		mprintf((1, "Too many sound objects!\n" ));
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

	soundnum = digi_xlat_sound(soundnum);

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_POS) )	{
			if ((SoundObjects[i].lp_segnum == segnum) && (SoundObjects[i].soundnum==soundnum ) && (SoundObjects[i].lp_sidenum==sidenum) ) {
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					deallocate_voice(SoundObjects[i].handle);
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
	// If this assert happens, it means that there were 2 sounds
	// that got deleted. Weird, get John.
	if ( killed > 1 )	{
		mprintf( (1, "ERROR: More than 1 sounds were deleted from seg %d\n", segnum ));
	}
}

void digi_kill_sound_linked_to_object( int objnum )
{
	int i,killed;

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	{
			if (SoundObjects[i].lo_objnum == objnum)   {
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					deallocate_voice(SoundObjects[i].handle );
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

	if (!Digi_initialized) return;
	if (digi_driver_board<1) return;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			oldvolume = SoundObjects[i].volume;
			oldpan = SoundObjects[i].pan;

			if ( !(SoundObjects[i].flags & SOF_PLAY_FOREVER) )	{
			 	// Check if its done.
				if (SoundObjects[i].flags & SOF_PLAYING) {
					if (voice_get_position(SoundObjects[i].handle) < 0) {
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
						deallocate_voice(SoundObjects[i].handle);
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
						deallocate_voice(SoundObjects[i].handle);
						SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
					}
				} else {
					if (!(SoundObjects[i].flags & SOF_PLAYING))	{
						digi_start_sound_object(i);
					} else {
						voice_set_volume(SoundObjects[i].handle, fixmuldiv(SoundObjects[i].volume,digi_volume,F1_0));
					}
				}
			}
				
			if (oldpan != SoundObjects[i].pan) 	{
				if (SoundObjects[i].flags & SOF_PLAYING)
					voice_set_pan(SoundObjects[i].handle, SoundObjects[i].pan >> 8);
			}
		}
	}
}

int sound_paused = 0;

void digi_pause_all()
{
	int i;

	if (!Digi_initialized) return;

	if (sound_paused==0)	{
//                 if ( digi_midi_type > 0 )       {
//                         if (SongHandle)
//                                midi_pause();
//                }
                digi_midi_pause();
		if (digi_driver_board>0)	{
			for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
				if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_PLAYING)&& (SoundObjects[i].flags & SOF_PLAY_FOREVER) )	{
					deallocate_voice(SoundObjects[i].handle );
					SoundObjects[i].flags &= ~SOF_PLAYING;		// Mark sound as not playing
				}			
			}
		}
	}
	sound_paused++;
}

void digi_resume_all()
{
	if (!Digi_initialized) return;

	Assert( sound_paused > 0 );

	if (sound_paused==1)	{
		// resume sound here
//                if ( digi_midi_type > 0 )       {
//                        if (SongHandle)
//                                midi_resume();
//                }
                  digi_midi_resume();
	}
	sound_paused--;
}

void digi_stop_all()
{
	int i;

	if (!Digi_initialized) return;

//        if ( digi_midi_type > 0 )       {
//                if (SongHandle)  {
//                   destroy_midi(SongHandle);
//                   SongHandle = NULL;
//                }
//        }
        digi_midi_stop();

	if (digi_driver_board>0)	{
		for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
			if ( SoundObjects[i].flags & SOF_USED )	{
				if ( SoundObjects[i].flags & SOF_PLAYING )	{
					deallocate_voice(SoundObjects[i].handle );
				}
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
			}
		}
	}
}

#ifndef NDEBUG
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
