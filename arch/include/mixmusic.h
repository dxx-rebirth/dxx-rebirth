/*
 * Header file for music playback through SDL_mixer
 *
 *  -- MD2211 (2006-04-24)
 */

#ifndef _SDLMIXER_MUSIC_H
#define _SDLMIXER_MUSIC_H

void mix_play_music(char *filename, int loop);
void mix_set_music_volume(int vol);
void mix_stop_music();

#endif
