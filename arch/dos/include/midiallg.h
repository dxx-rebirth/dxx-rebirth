#ifndef _MIDIALLG_H
#define _MIDIALLG_H

void digi_midi_pause();
void digi_midi_resume();
void digi_midi_stop();
#ifndef ALLEGRO
int digi_midi_init();
void digi_midi_close();
#endif

#endif
