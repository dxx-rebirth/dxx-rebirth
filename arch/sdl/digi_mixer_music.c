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
#include <stdlib.h>

#include "args.h"
#include "hmp2mid.h"
#include "digi_mixer_music.h"
#include "jukebox.h"
#include "cfile.h"

#define MIX_MUSIC_DEBUG 0
#define MUSIC_FADE_TIME 500 //milliseconds
#define MUSIC_EXTENSION_ARG "-music_ext"

Mix_Music *current_music = NULL;

void music_done() {
  Mix_HaltMusic();
  Mix_FreeMusic(current_music);
  current_music = NULL;
  jukebox_stop_hook();
}

void convert_hmp(char *filename, char *mid_filename) {

  if (!PHYSFS_exists(mid_filename))
  {
    const char *err;
    PHYSFS_file *hmp_in;
    PHYSFS_file *mid_out = PHYSFSX_openWriteBuffered(mid_filename);

    if (!mid_out) {
      fprintf(stderr, "Error could not open: %s for writing: %s\n", mid_filename, PHYSFS_getLastError());
      return;
    }

    if (MIX_MUSIC_DEBUG) printf("convert_hmp: converting %s to %s\n", filename, mid_filename);

    hmp_in = PHYSFSX_openReadBuffered(filename);

    if (!hmp_in) {
      fprintf(stderr, "Error could not open: %s\n", filename);
      PHYSFS_close(mid_out);
      return;
    }

    err = hmp2mid(hmp_in, mid_out);

    PHYSFS_close(mid_out);
    PHYSFS_close(hmp_in);

    if (err) {
      fprintf(stderr, "%s\n", err);
      PHYSFS_delete(mid_filename);
      return;
    }
  }
  else {
    if (MIX_MUSIC_DEBUG) printf("convert_hmp: %s already exists\n", mid_filename);
  }
}

/*
 *  Plays a music given its name (regular game songs)
 */

void mix_play_music(char *filename, int loop) {

  loop *= -1; 
  int i, got_end=0;

  char real_filename[PATH_MAX];
  char rel_filename[32];	// just the filename of the actual music file used
  char music_title[16];
  char *basedir = "Music";

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

  if (!PHYSFS_isDirectory(basedir))
	  PHYSFS_mkdir(basedir);	// tidy up those files

  // What is the extension of external files? If none, default to internal MIDI
  if (GameArg.SndExternalMusic) {
    sprintf(rel_filename, "%s/%s.%3s", basedir, music_title, GameArg.SndExternalMusic); // add extension
  }
  else {
    sprintf(rel_filename, "%s/%s.mid", basedir, music_title);
    convert_hmp(filename, rel_filename);
  }

  PHYSFSX_getRealPath(rel_filename, real_filename);

  mix_play_file("", real_filename, loop);
}


/*
 *  Plays a music file from an absolute path (used by jukebox)
 */

void mix_play_file(char *basedir, char *filename, int loop) {

  int fn_buf_len = strlen(basedir) + strlen(filename) + 1;
  char real_filename[fn_buf_len];
  sprintf(real_filename, "%s%s", basedir, filename); // build absolute path

  if ((current_music = Mix_LoadMUS(real_filename))) {
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
    fprintf(stderr, "Music %s could not be loaded%s\n", filename, basedir);
    Mix_HaltMusic();
  }
}

void mix_set_music_volume(int vol) {
  //printf("mix_set_music_volume %d\n", vol);
  Mix_VolumeMusic(vol);
}

void mix_stop_music() {
  Mix_HaltMusic();
}
