/*
 * Header file for music playback through SDL_mixer
 *
 *  -- MD2211 (2006-04-24)
 */

#ifndef _SDLMIXER_MUSIC_H
#define _SDLMIXER_MUSIC_H

int mix_play_music(char *, int);
int mix_play_file(char *, int, void (*)());
int mix_music_exists(const char *filename);
void mix_set_music_volume(int);
void mix_stop_music();
void mix_free_music();

#endif
