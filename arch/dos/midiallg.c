#include <string.h>
#include <stdlib.h>
#include "u_mem.h"
#include "mono.h"
#include "allg_snd.h"
#include "digi.h"
#include "timer.h"

int midi_volume                            = 128/2;                                                // Max volume
char digi_last_midi_song[16] = "";
char digi_last_melodic_bank[16] = "";
char digi_last_drum_bank[16] = "";
int digi_midi_type                         = 0;    // Midi driver type
int digi_midi_port                         = 0;    // Midi driver port

// handle for the initialized MIDI song
MIDI *SongHandle = NULL;

void digi_set_midi_volume( int mvolume )
{
        int old_volume = midi_volume;

        if ( mvolume > 127 )
                midi_volume = 127;
        else if ( mvolume < 0 )
                midi_volume = 0;
        else
                midi_volume = mvolume;

        if ( (digi_midi_type > 0) )        {
                if (  (old_volume < 1) && ( midi_volume > 1 ) ) {
                        if (SongHandle == NULL )
                                digi_play_midi_song( digi_last_midi_song, digi_last_melodic_bank, digi_last_drum_bank, 1 );
                }
                set_volume(-1, midi_volume * 2 + (midi_volume & 1));
        }
}

void digi_stop_current_song()
{
        if (SongHandle) {
                destroy_midi(SongHandle);
                SongHandle = NULL;
        }
}

void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop )
{
        //char fname[128];

        if ( digi_midi_type <= 0 )      return;

        digi_stop_current_song();

        if ( filename == NULL ) return;

        strcpy( digi_last_midi_song, filename );
        strcpy( digi_last_melodic_bank, melodic_bank );
        strcpy( digi_last_drum_bank, drum_bank );

        if ( midi_volume < 1 )
                return;

        SongHandle = NULL;

#if 0 /* needs bank loading to sound right */
        if (midi_card <= 4) { /* FM cards */
                int sl;
                sl = strlen( filename );
                strcpy( fname, filename );      
                fname[sl-1] = 'q';
                SongHandle = load_hmp(fname);
        }
#endif

        if ( !SongHandle  )
                SongHandle = load_hmp(filename);

        if (SongHandle) {
                if (play_midi(SongHandle, loop)) {
                        destroy_midi(SongHandle);
                        SongHandle = NULL;
                }
        }
        if (!SongHandle) {
                        mprintf( (1, "\nAllegro Error : %s", allegro_error ));
        }
}

void digi_midi_pause() {
       if (digi_midi_type > 0 && SongHandle)
               midi_pause();
}

void digi_midi_resume() {
       if (digi_midi_type > 0 && SongHandle)
               midi_resume();
}

void digi_midi_stop() {
       if ( digi_midi_type > 0 )       {
                if (SongHandle)  {
                       destroy_midi(SongHandle);
                       SongHandle = NULL;
                }
        }

}

#ifndef ALLEGRO
extern int sb_hw_dsp_ver;

static int midi_timer_system_initialized    = 0;
static int digi_midi_initialized            = 0;

int digi_midi_init() {
       if (!midi_timer_system_initialized)
       {
               allg_snd_init();
               midi_timer_system_initialized = 1;
       }
       sb_hw_dsp_ver = 1; /* set SB as already detected, functionless */
       if (!digi_midi_initialized) {
               // amount of voices we need
               // 16 for normal sounds and 16 for SoundObjects (fan, boss)
               // for DIGMID we sacrify some sounds (32 is the max).
//                reserve_voices(allegro_using_digmid() ? 16 : 32, -1);
               if (install_sound(DIGI_NONE, MIDI_AUTODETECT, NULL))
                       return 1;
               //set_volume(255, -1);
               digi_midi_type = midi_card; // only for 0, !=0
       }
       digi_midi_initialized = 1;
       digi_set_midi_volume( midi_volume );
       return 0;
}

void digi_midi_close() {
       remove_sound();
       digi_midi_initialized = 0;

       if ( midi_timer_system_initialized ) {
               // Remove timer...
               timer_set_function( NULL );
               midi_timer_system_initialized = 0;
        }
}
#endif
