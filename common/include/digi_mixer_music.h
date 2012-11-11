/*
 * Header file for music playback through SDL_mixer
 *
 *  -- MD2211 (2006-04-24)
 */

#ifndef _SDLMIXER_MUSIC_H
#define _SDLMIXER_MUSIC_H

#ifdef __cplusplus
extern "C" {
#endif

int mix_play_music(const char *, int);
int mix_play_file(const char *, int, void (*)());
void mix_set_music_volume(int);
void mix_stop_music();
void mix_pause_music();
void mix_resume_music();
void mix_pause_resume_music();
void mix_free_music();

#ifdef __cplusplus
}
#endif

#endif
