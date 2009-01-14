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
#include "u_mem.h"

#define MIX_MUSIC_DEBUG 0
#define MUSIC_FADE_TIME 500 //milliseconds

Mix_Music *current_music = NULL;

void music_hook_stop();
void music_hook_next();

void convert_hmp(char *filename, char *mid_filename) {

  if (1)	//!PHYSFS_exists(mid_filename))	// allow custom MIDI in add-on hogs to be used without caching everything
  {
    const char *err;
    PHYSFS_file *hmp_in;
    PHYSFS_file *mid_out = PHYSFSX_openWriteBuffered(mid_filename);

    if (!mid_out) {
      con_printf(CON_CRITICAL," could not open: %s for writing: %s\n", mid_filename, PHYSFS_getLastError());
      return;
    }

    con_printf(CON_DEBUG,"convert_hmp: converting %s to %s\n", filename, mid_filename);

    hmp_in = PHYSFSX_openReadBuffered(filename);

    if (!hmp_in) {
      con_printf(CON_CRITICAL," could not open: %s\n", filename);
      PHYSFS_close(mid_out);
      return;
    }

    err = hmp2mid(hmp_in, mid_out);

    PHYSFS_close(mid_out);
    PHYSFS_close(hmp_in);

    if (err) {
      con_printf(CON_CRITICAL,"%s\n", err);
      PHYSFS_delete(mid_filename);
      return;
    }
  }
  else {
    con_printf(CON_DEBUG,"convert_hmp: %s already exists\n", mid_filename);
  }
}

/*
 *  Plays a music given its name (regular game songs)
 */

void mix_play_music(char *filename, int loop) {
  int i, got_end=0;
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
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
    snprintf(rel_filename, strlen(basedir)+strlen(music_title)+6, "%s/%s.%s", basedir, music_title, GameArg.SndExternalMusic); // add extension
  }
  else {
    snprintf(rel_filename, strlen(basedir)+strlen(music_title)+6, "%s/%s.mid", basedir, music_title);
    convert_hmp(filename, rel_filename);
  }

  mix_play_file(rel_filename, loop);
}


/*
 *  Plays a music file from an absolute path (used by jukebox)
 */

void mix_play_file(char *filename, int loop) {
  char real_filename[PATH_MAX];

  PHYSFSX_getRealPath(filename, real_filename); // build absolute path

  // If loop, builtin music (MIDI) should loop (-1) while in jukebox it should only play once and proceed to next track (1) or stop after track (0)
  if (!jukebox_is_loaded() && loop)
    loop = -1;

  current_music = Mix_LoadMUS(real_filename);

  if (current_music) {
    if (Mix_PlayingMusic()) {
      // Fade-in effect sounds cleaner if we're already playing something
      Mix_FadeInMusic(current_music, loop, MUSIC_FADE_TIME);
    }
    else {
      Mix_PlayMusic(current_music, loop);
    }

    Mix_HookMusicFinished(loop == 1 ? music_hook_next : music_hook_stop);
  }
  else {
    con_printf(CON_CRITICAL,"Music %s could not be loaded\n", real_filename);
    Mix_HaltMusic();
  }
}


/*
 *  See if a music file exists, taking into account possible -music_ext option
 */

int mix_music_exists(const char *filename)
{
	char rel_filename[32];	// just the filename of the actual music file used
	char music_file[16];
	char *basedir = "Music";

	if (GameArg.SndExternalMusic)
	{
		change_filename_extension(music_file, filename, GameArg.SndExternalMusic);
		sprintf(rel_filename, "%s/%s", basedir, music_file);
		return PHYSFS_exists(rel_filename);
	}

	return PHYSFS_exists(filename);
}

// What to do when stopping song playback
void music_hook_stop() {
  Mix_HaltMusic();
  if (current_music)
  {
     Mix_FreeMusic(current_music);
     current_music = NULL;
  }
  jukebox_hook_stop();
}

// What to do when going to next song / looping
void music_hook_next() {
  music_hook_stop();
  jukebox_hook_next();
}

void mix_set_music_volume(int vol) {
  Mix_VolumeMusic(vol);
}

void mix_stop_music() {
  Mix_HaltMusic();
}
