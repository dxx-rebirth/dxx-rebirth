/*
 * This is an alternate backend for the music system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 *  -- MD2211 (2006-04-24)
 */

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <string.h>
#include "args.h"
#include "hmp2mid.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "cfile.h"

#define MIX_MUSIC_DEBUG 1
#define MUSIC_FADE_TIME 500 //milliseconds
#define MUSIC_EXTENSION_ARG "-music_ext"

Mix_Music *current_music = NULL;

void music_done() {
  Mix_HaltMusic();
  Mix_FreeMusic(current_music);
  current_music = NULL;
}

void convert_hmp(char *filename, char *mid_filename) {

  if (access(mid_filename, R_OK) != 0) {
    if (MIX_MUSIC_DEBUG) printf("convert_hmp: converting %s to %s\n", filename, mid_filename);

    const char *err;
    CFILE *hmp_in;
    FILE *mid_out = fopen(mid_filename, "w");

    if (!mid_out) {
      fprintf(stderr, "Error could not open: %s for writing: %s\n", mid_filename, strerror(errno));
      return;
    }

    hmp_in = cfopen(filename, "rb");

    if (!hmp_in) {
      fprintf(stderr, "Error could not open: %s\n", filename);
      fclose(mid_out);
      return;
    }

    err = hmp2mid((hmp2mid_read_func_t) cfread, hmp_in, mid_out);

    fclose(mid_out);
    cfclose(hmp_in);

    if (err) {
      fprintf(stderr, "%s\n", err);
      unlink(mid_filename);
      return;
    }
  }
  else {
    if (MIX_MUSIC_DEBUG) printf("convert_hmp: %s already exists\n", mid_filename);
  }
}

void mix_play_music(char *filename, int loop) {

  loop *= -1; 
  int i, t, got_end=0;
  int fn_buf_len = strlen(DESCENT_DATA_PATH) + strlen(filename) + 16;

  char real_filename[fn_buf_len];
  char midi_filename[16];
  char music_title[16];
  char music_file_extension[8];

  // Quick hack to filter out the .hmp extension
  for (i=0; !got_end; i++) {
    switch (filename[i]) {
    case '.':
    case '\0':
      music_title[i] = '\0';
      got_end = 1;
      break;
    default:
      music_title[i] = filename[i];
    }
  }

  // What is the extension of external files? If none, default to internal MIDI
  t = FindArg(MUSIC_EXTENSION_ARG);
  if (t > 0) {
    sprintf(music_file_extension, "%.3s", Args[t+1]);
    // Building absolute path to the file
    sprintf(real_filename, "%s%s.%s", DESCENT_DATA_PATH, music_title, music_file_extension);
  }
  else {
    sprintf(midi_filename, "%s.mid", music_title);
    convert_hmp(filename, midi_filename);
    // Relative path for now (we're using $HOME/.d1x-rebirth/ as working dir)
    strcpy(real_filename, midi_filename);
  }

  if ((current_music = Mix_LoadMUS(real_filename))) {
    printf("Now playing: %s\n", real_filename);
    if (Mix_PlayingMusic()) {
      // Fade-in effect sounds cleaner if we're already playing something
      Mix_FadeInMusic(current_music, loop, MUSIC_FADE_TIME);
    }
    else {
      Mix_PlayMusic(current_music, loop);
    }
    Mix_HookMusicFinished(music_done);
  }
  else {
    printf("Music %s could not be loaded\n", real_filename);
    Mix_HaltMusic();
  }
}

void mix_set_music_volume(int vol) {
  Mix_VolumeMusic(vol);
}

void mix_stop_music() {
  Mix_HaltMusic();
}
