/* $Id: midi.c,v 1.4 2005-04-04 08:54:12 btb Exp $ */
// MIDI stuff follows.
#include <stdio.h>

#include "error.h"
#include "hmpfile.h"
#include "args.h"

hmp_file *hmp = NULL;

int midi_volume = 255;
int digi_midi_song_playing = 0;


void digi_stop_current_song()
{
	if (digi_midi_song_playing)
	{
		hmp_close(hmp);
		hmp = NULL;
		digi_midi_song_playing = 0;
	}
}

void digi_set_midi_volume(int n)
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

void digi_play_midi_song(char *filename, char *melodic_bank, char *drum_bank, int loop)
{
#if 0
	if (!digi_initialised)
		return;
#endif

	if (FindArg("-nosound"))
		return;

	digi_stop_current_song();

	if (filename == NULL)
		return;
	if (midi_volume < 1)
		return;

	if ((hmp = hmp_open(filename)))
	{
		hmp_play(hmp);
		digi_midi_song_playing = 1;
		digi_set_midi_volume(midi_volume);
	}
	else
		printf("hmp_open failed\n");
}


int sound_paused = 0;

void digi_pause_midi()
{
#if 0
	if (!digi_initialised)
		return;
#endif

	if (sound_paused == 0)
	{
		// pause here
	}
	sound_paused++;
}

void digi_resume_midi()
{
#if 0
	if (!digi_initialised)
		return;
#endif

	Assert(sound_paused > 0);

	if (sound_paused == 1)
	{
		// resume sound here
	}
	sound_paused--;
}
