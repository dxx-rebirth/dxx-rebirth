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
static char rcsid[] = "$Id: windigi.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include "desw.h"
#include "win\ds.h"
#include <mmsystem.h>
#include <mmreg.h>

#include<stdlib.h>
#include<stdio.h>
#include<dos.h>
#include<fcntl.h> 
#include<malloc.h> 
#include<bios.h>
#include<io.h>
#include<string.h>
#include<ctype.h>

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
#include "error.h"
#include "wall.h"
#include "cfile.h"
#include "piggy.h"
#include "text.h"

#pragma pack (4);						// Use 32-bit packing!
#pragma off (check_stack);			// No stack checking!

#include "win\winmidi.h"
#include "config.h"

#define MAX_CHANNELS 32

typedef struct digi_channel {
	ubyte		used;				// Non-zero if a sound is playing on this channel 
	int		soundnum;		// Which sound effect is on this channel, -1 if none
	WORD		handle;			// What SS handle this is playing on
	int		soundobj;		// Which soundobject is on this channel
	int 		persistant;		// This can't be pre-empted
	int		volume;			// the volume of this channel
} digi_channel;


//	Defines
//	----------------------------------------------------------------------------

#define DIGI_PAUSE_BROKEN 1		//if this is defined, don't pause MIDI songs
#define _DIGI_SAMPLE_FLAGS (0)
#define _DIGI_MAX_VOLUME (16384)
#define MAX_SOUND_OBJECTS 22



//	Variables
//	----------------------------------------------------------------------------

int	Digi_initialized 				= 0;
static int	digi_atexit_called	= 0;						// Set to 1 if atexit(digi_close) was called

int digi_sample_rate					= SAMPLE_RATE_22K;	// rate to use driver at
int digi_max_channels		= 8;

int digi_total_locks = 0;

static int digi_volume				= _DIGI_MAX_VOLUME;	// Max volume
static int digi_system_initialized		= 0;
static int digi_sound_locks[MAX_SOUNDS];
BOOL DIGIDriverInit = FALSE;


// MIDI stuff
//	----------------------------------------------------------------------------
WMIDISONG WMidiSong;
int		WMidiHandle=0;
ubyte		*SongData=NULL;
HGLOBAL	hSongData=NULL;
int		SongSize;

char digi_last_midi_song[16] = "";

static BOOL 							MIDIDriverInit	= FALSE;

static int midi_system_initialized	= 0;
static int midi_volume					= 128/2;	// Max volume

extern int Redbook_playing;


/* Obsolete */
WORD		hSOSDigiDriver = 0xffff;				// handle for the SOS driver being used 
WORD		hSOSMidiDriver = 0xffff;			// handle for the loaded MIDI driver
WORD		hTimerEventHandle = 0xffff;			// handle for the timer function
int digi_driver_dma											= 0;
int digi_driver_board										= 0;
int digi_driver_port             						= 0;
int digi_driver_irq              						= 0;
int digi_midi_type               						= 0;
int digi_midi_port											= 0;
LPSTR digi_driver_path = NULL;
/* Not Obsolete */


//	Sound Handle stuff
//	----------------------------------------------------------------------------
digi_channel channels[MAX_CHANNELS];
int next_channel = 0;
int channels_inited = 0;


//	Functions
//	----------------------------------------------------------------------------
int verify_sound_channel_free(int channel);

void * digi_load_file( char * szFileName, HGLOBAL *hmem, int * length );



//	FUNCTIONS!!!
//		DIGI SYSTEM
//			Initialization
//			Play
//			Sound System
//
//		MIDI SYSTEM
//	----------------------------------------------------------------------------

int digi_init(void)
{
	int i;

	Digi_initialized = 1;

//	Perform MIDI Detection

// Initialize MIDI driver
	if (! FindArg( "-nomusic" )) {
		midi_system_initialized = 1;
		if (digi_init_midi()) {
			mprintf((1, "Couldn't initialize MIDI system.\n"));
			midi_system_initialized = 0;
			return 1;
		}
	}

// Initialize DIGI driver
	if (! FindArg( "-nosound" )) {
		digi_system_initialized = 1;
		if (digi_init_digi()) {
			mprintf((1, "Couldn't initialize digital sound system.\n"));
			digi_close();
	  		return 2;
		}
	}
		
	if (!digi_atexit_called) {
		atexit(digi_close);
		digi_atexit_called = 1;
	}

//	Miscellaneous Initialization
	digi_init_sounds();

	for (i = 0; i < MAX_SOUNDS; i++)
		digi_sound_locks[i] = 0;

	digi_stop_all_channels();

	return 0; 
}


void digi_close(void)
{

	if (!Digi_initialized) return;
	Digi_initialized = 0;

	digi_close_midi();
	digi_close_digi();

	digi_system_initialized = 0;
	midi_system_initialized	= 0;
}


int digi_init_digi(void)
{
	SSCaps sscaps;

	Assert(digi_sample_rate == SAMPLE_RATE_11K || digi_sample_rate == SAMPLE_RATE_22K);

	if (!digi_system_initialized) 
		return 1;

//	Determine board type?
	digi_driver_board = 1;

//	Initialize Sound System
	if (!SSInit(_hAppWnd, 32, DSSCL_NORMAL)) {
		DIGIDriverInit = FALSE;
		return 1;
	}

	SSGetCaps(&sscaps);
	if (sscaps.sample_rate < SAMPLE_RATE_22K) 
		digi_sample_rate = SAMPLE_RATE_11K;

//	logentry("Detected sound card using (%d Hz).  Using (%d Hz) samples.\n", sscaps.sample_rate, digi_sample_rate);

//	Crank up the WAV volume for Wave-Table and full range of FX volumes
	DIGIDriverInit = TRUE;
	return 0;
}


void digi_close_digi()
{
   if (DIGIDriverInit)   {
		SSDestroy();	 						// resets WAV vol to SSInit value.
		DIGIDriverInit = FALSE;
	}
}


int digi_init_midi(void)
{
	DWORD res;

//	check to see if MIDI is going to be used.
	if (!midi_system_initialized) 
		return 1;

//	Initialize MIDI system and driver
	res = wmidi_init(MIDI_MAPPER);

	if (!res) {
		MIDIDriverInit = FALSE;
		return 1;
	}
	else {
//@@		switch(sMIDICaps.wTechnology) 
//@@		{
//@@			case MOD_SYNTH:
//@@				mprintf((0, "Using SB/SYNTH for MIDI.\n"));	break;
//@@
//@@			case MOD_FMSYNTH:
//@@				mprintf((0, "Using ADLIB FM for MIDI.\n")); break;
//@@			
//@@			case MOD_MAPPER:
//@@				mprintf((0, "Using MIDI mapping.\n")); break;
//@@
//@@			case MOD_MIDIPORT:
//@@				mprintf((0, "Using SB/External MIDI port.\n")); break;
//@@
//@@			case MOD_SQSYNTH:
//@@				mprintf((0, "Using Wave Synthesis for MIDI.\n")); break;
//@@
//@@			default:
//@@				mprintf((0, "Using another method (%d) for MIDI.\n", sMIDICaps.wMID)); break;
//@@		}

		digi_midi_type = 1;
	
		MIDIDriverInit = TRUE;
	}

	digi_set_midi_volume(Config_midi_volume*16);

	return 0;
}		


void digi_close_midi()
{
	if (!midi_system_initialized) return;

	if (MIDIDriverInit)   {
      if (WMidiHandle)  {
         // stop the last MIDI song from playing
			wmidi_stop();
			wmidi_close_song();
         WMidiHandle = 0;
      }
      if (SongData)  {
			GlobalUnlock(SongData);
			GlobalFree(hSongData);
         SongData = NULL;
			hSongData = NULL;
      }

		wmidi_close();
		MIDIDriverInit = FALSE;
   }
}


void digi_reset()	
{
	if ( Digi_initialized )	{
		digi_stop_all_channels();
		digi_close();
		mprintf( (0, "Sound system DISABLED.\n" ));
	} else {
		digi_init();
		mprintf( (0, "Sound system ENABLED.\n" ));
	}
}


//	Channel Functions
//	----------------------------------------------------------------------------

void digi_stop_all_channels()
{
	int i;

	for (i=0; i<MAX_CHANNELS; i++ )
		digi_stop_sound(i);

	for (i=0; i<MAX_SOUNDS; i++ )	{
		Assert( digi_sound_locks[i] == 0 );
	}
}

void digi_set_max_channels(int n)
{
	digi_max_channels	= n;

	if ( digi_max_channels < 1 ) 
		digi_max_channels = 1;
	if ( digi_max_channels > 32 ) 
		digi_max_channels = 32;

	if ( !Digi_initialized ) return;
	if ( !DIGIDriverInit )	return;

	digi_stop_all_channels();
}

int digi_get_max_channels()
{
	return digi_max_channels;
}

int digi_is_channel_playing( int c )
{
	if (!Digi_initialized) return 0;
	if (!DIGIDriverInit) return 0;

	if ( channels[c].used && SSChannelPlaying((int)channels[c].handle)) 
		return 1;
	return 0;
}

void digi_set_channel_volume( int c, int volume )
{
	if (!Digi_initialized) return;
	if (!DIGIDriverInit) return;

	if ( !channels[c].used ) return;

	SSSetChannelVolume(channels[c].handle, (unsigned short)fixmuldiv(volume,digi_volume,F1_0));
}
	
void digi_set_channel_pan( int c, int pan )
{
	if (!Digi_initialized) return;
	if (!DIGIDriverInit) return;

	if ( !channels[c].used ) return;

	SSSetChannelPan(channels[c].handle, (unsigned short)pan);
}

void digi_stop_sound( int c )
{
	if (!Digi_initialized) return;
	if (!DIGIDriverInit) return;

	if (!channels[c].used) return;

	if ( digi_is_channel_playing(c)  )		{
		SSStopChannel(channels[c].handle);
	}
	channels[c].used = 0;

	channels[c].soundobj = -1;
	channels[c].persistant = 0;
}

void digi_end_sound( int c )
{
	if (!Digi_initialized) return;
	if (!DIGIDriverInit) return;

	if (!channels[c].used) return;

	channels[c].soundobj = -1;
	channels[c].persistant = 0;
}


// Returns the channel a sound number is playing on, or
// -1 if none.
int digi_find_channel(int soundno)
{
	int i, is_playing;

	if (!Digi_initialized) return -1;
	if (!DIGIDriverInit) return -1;

	if (soundno < 0 ) return -1;
	if (GameSounds[soundno].data==NULL) {
		Int3();
		return -1;
	}

	is_playing = 0;
	for (i=0; i<digi_max_channels; i++ )	{
		if ( channels[i].used && (channels[i].soundnum==soundno) )
			if ( digi_is_channel_playing(i) )
				return i;
	}	
	return -1;
}

extern void digi_end_soundobj(int channel);	
extern int SoundQ_channel;
extern void SoundQ_end();


//	DIGI Start Sample function
//	----------------------------------------------------------------------------
int digi_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj)
{
	int i, starting_channel;
	int sHandle;
	SSoundBuffer ssb;

	if ( !Digi_initialized ) return -1;
	if ( !DIGIDriverInit )	return -1;

	Assert(GameSounds[soundnum].data != -1);

	memset(&ssb, 0, sizeof(SSoundBuffer));
	
	ssb.data = (char *)GameSounds[soundnum].data;
	ssb.length = GameSounds[soundnum].length;
	ssb.pan = (unsigned short)pan+1;
//	ssb.id = soundnum;
	ssb.vol = (unsigned short)fixmuldiv(volume, digi_volume, F1_0);
	ssb.vol = (ssb.vol << 2);
	ssb.rate = digi_sample_rate;
	ssb.channels = 1;
	ssb.bits_per_sample = 8;

	if (looping) {
		ssb.looping = 1;
		ssb.loop_start = ssb.loop_end = -1;
	}	
	if (loop_start != -1) {
		Assert( loop_end != -1);
		ssb.loop_start = loop_start;
		ssb.loop_end = loop_end;
		ssb.loop_length = loop_end - loop_start;
	}		

	starting_channel = next_channel;

	while(1)	{
		if ( !channels[next_channel].used ) break;
		
		if (!SSChannelPlaying((int)channels[next_channel].handle)) break;

		if ( !channels[next_channel].persistant )	{
			SSStopChannel(channels[next_channel].handle);
			break;	// use this channel!	
		}
		next_channel++;
		if ( next_channel >= digi_max_channels )
			next_channel = 0;
		if ( next_channel == starting_channel ) {
			mprintf(( 1, "OUT OF SOUND CHANNELS!!!\n" ));
			return -1;
		}
	}
	if ( channels[next_channel].used )	{
		channels[next_channel].used = 0;
		if ( channels[next_channel].soundobj > -1 )	{
			digi_end_soundobj(channels[next_channel].soundobj);	
		}
		if (SoundQ_channel==next_channel)
			SoundQ_end();
	}

	sHandle = SSInitChannel(&ssb);
	if (sHandle == -1) {
		mprintf(( 1, "NOT ENOUGH SOUND SLOTS!!!\n" ));
		return -1;
	}

	#ifndef NDEBUG
	verify_sound_channel_free(next_channel);
	#endif

	//free up any sound objects that were using this handle
	for (i=0; i<digi_max_channels; i++ )	{
		if ( channels[i].used && (channels[i].handle == sHandle)  )	{
			channels[i].used = 0;
			if ( channels[i].soundobj > -1 )	{
				digi_end_soundobj(channels[i].soundobj);	
			}
			if (SoundQ_channel==i)
				SoundQ_end();
		}
	}

	channels[next_channel].used = 1;
	channels[next_channel].soundnum = soundnum;
	channels[next_channel].soundobj = soundobj;
	channels[next_channel].handle = sHandle;
	channels[next_channel].volume = volume;
	channels[next_channel].persistant = 0;
	if ( (soundobj > -1) || (looping) || (volume>F1_0) )
		channels[next_channel].persistant = 1;

	i = next_channel;
	next_channel++;
	if ( next_channel >= digi_max_channels )
		next_channel = 0;

	return i;
}


//	Volume Functions
//	----------------------------------------------------------------------------
void digi_set_midi_volume( int mvolume )
{

	int old_volume = midi_volume;

	if ( mvolume > 127 )
		midi_volume = 127;
	else if ( mvolume < 0 )
		midi_volume = 0;
	else
		midi_volume = mvolume;

	if ( (MIDIDriverInit) )		{
		if (!Redbook_playing && (old_volume < 1) && ( midi_volume > 1 ) )	{
			if (WMidiHandle == 0 )
				digi_play_midi_song( digi_last_midi_song, NULL, NULL, 1 );
		}
		wmidi_set_volume(midi_volume);
	}

	mprintf((0, "Changing midi volume=%d.\n", midi_volume));
}

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
	if ( !digi_system_initialized )	return;

	//digi_sync_sounds();
}


// 0-0x7FFF 
void digi_set_volume( int dvolume, int mvolume )	
{
	digi_set_midi_volume( mvolume );
	digi_set_digi_volume( dvolume );
	mprintf(( 1, "Volume: 0x%x and 0x%x\n", digi_volume, midi_volume ));
}

// allocate memory for file, load fil
void * digi_load_file( char * szFileName, HGLOBAL *hmem, int * length )
{
   PSTR  pDataPtr;
	CFILE * fp;
	
   // open file
   fp  =  cfopen( szFileName, "rb" );
	if ( !fp ) return NULL;

   *length  =  cfilelength(fp);

	*hmem = GlobalAlloc(GPTR, *length);
	if (!(*hmem)) return NULL;
	pDataPtr = GlobalLock(*hmem);

   // read in driver
   cfread( pDataPtr, *length, 1, fp);

   // close driver file
   cfclose( fp );

   // return 
   return( pDataPtr );
}


void digi_stop_current_song()
{
	if (!Digi_initialized) return;

	if ( MIDIDriverInit )	{
		// Stop last song...
		if (WMidiHandle > 0 )	{
		   // stop the last MIDI song from playing
			wmidi_stop();
			wmidi_close_song();
			WMidiHandle = 0;
		}
		if (SongData) 	{
			GlobalUnlock(SongData);
			GlobalFree(hSongData);
			SongData = NULL;
			hSongData = NULL;
		}
	}
}


void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop )
{
	char fname[128];
	CFILE		*fp;
	int sl;

	if (!Digi_initialized) return;
	if ( !MIDIDriverInit )	return;

	digi_stop_current_song();

	if ( filename == NULL )	return;

	strcpy( digi_last_midi_song, filename );

	fp = NULL;

	sl = strlen( filename );
	strcpy( fname, filename );	
	fname[sl-3] = 'm';
	fname[sl-2] = 'i';
	fname[sl-1] = 'd';

	fp = cfopen( fname, "rb" );
	mprintf((0, "Loading %s\n", fname));

	if ( !fp  )	{
	 	mprintf( (1, "Error opening midi file, '%s'", filename ));
	 	return;
	}
	if ( midi_volume < 1 )		{
		cfclose(fp);
		return;				// Don't play song if volume == 0;
	}
	SongSize = cfilelength( fp );
	hSongData = GlobalAlloc(GPTR, SongSize);
	if (hSongData==NULL)	{
		cfclose(fp);
		mprintf( (1, "Error allocating %d bytes for '%s'", SongSize, filename ));
		return;
	}
	SongData = GlobalLock(hSongData);
	if ( cfread (  SongData, SongSize, 1, fp )!=1)	{
		mprintf( (1, "Error reading midi file, '%s'", filename ));
		cfclose(fp);
		GlobalUnlock(SongData);
		GlobalFree(hSongData);
		SongData=NULL;
		hSongData = NULL;
		return;
	}

	cfclose(fp);

// setup the song initialization structure
	WMidiSong.data = SongData;
	WMidiSong.length = SongSize;

	if ( loop )
		WMidiSong.looping = 1;
	else
		WMidiSong.looping = 0;

//	initialize the song
	WMidiHandle = wmidi_init_song(&WMidiSong);
	if (!WMidiHandle) {
		mprintf((1, "Unable to initialize MIDI song.\n"));		
		GlobalUnlock(SongData);
		GlobalFree(hSongData);
		SongData=NULL;
		hSongData = NULL;
		return;
	}

	Assert( WMidiHandle == 1 );
	 
// start the song playing
	mprintf((0, "Playing song %x.\n", WMidiHandle));

	if (!wmidi_play()) {
		mprintf( (1, "\nUnable to play midi song.\n"));
		wmidi_close_song();
		GlobalUnlock(SongData);
		GlobalFree(hSongData);
		SongData=NULL;
		hSongData = NULL;
		return;
   }
}


int sound_paused = 0;

void digi_pause_midi()
{
	if (!Digi_initialized) return;

	if (sound_paused==0)	{
		if ( digi_midi_type > 0 && WMidiHandle > 0)	{
			// pause here
				wmidi_pause();
		}
	}
	sound_paused++;
}



void digi_resume_midi()
{
	if (!Digi_initialized) return;

	Assert( sound_paused > 0 );

	if (sound_paused==1)	{
		// resume sound here
		if ( digi_midi_type > 0 && WMidiHandle > 0)	{
			wmidi_resume();
		}
	}
	sound_paused--;
}


#ifndef NDEBUG
void digi_debug()
{
	int i;
	int n_voices=0;

	if (!Digi_initialized) return;
	if (!DIGIDriverInit) return;

	for (i=0; i<digi_max_channels; i++ )	{
		if ( digi_is_channel_playing(i) )
			n_voices++;
	}

	mprintf_at(( 0, 2, 0, "DIGI: Active Sound Channels: %d/%d ", n_voices, digi_max_channels));
}


void digi_midi_debug()
{

}
	
#endif


extern BOOL WMidi_NewStream;

void digi_midi_wait()
{
	fix ftime;
	int tech;

	tech = wmidi_get_tech();
	if (tech) return;
	
  	ftime = timer_get_fixed_seconds() + 0x50000;
	WMidi_NewStream = FALSE;
	while ((WMidi_NewStream < 2 && WMidiHandle) || (timer_get_fixed_seconds() < ftime)) Sleep(0);
}


