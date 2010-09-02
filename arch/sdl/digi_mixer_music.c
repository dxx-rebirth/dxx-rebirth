/*
 * This is an alternate backend for the music system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 *  -- MD2211 (2006-04-24)
 */

#include <SDL/SDL.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "args.h"
#include "hmp.h"
#include "digi_mixer_music.h"
#include "cfile.h"
#include "u_mem.h"


Mix_Music *current_music = NULL;
char *current_music_hndlbuf = NULL;

/*
 *  Plays a music file from an absolute path or a relative path
 */

int mix_play_file(char *filename, int loop, void (*hook_finished_track)())
{
	SDL_RWops *rw = NULL;
	PHYSFS_file *filehandle = NULL;
	char midi_filename[PATH_MAX], full_path[PATH_MAX];
	char *basedir = "music", *fptr;
	int bufsize = 0;

	mix_free_music();	// stop and free what we're already playing, if anything

	fptr = strrchr(filename, '.');

	if (fptr == NULL)
		return 0;

	// It's a .hmp. Convert to mid, store in music/ and pass the new filename.
	if (!stricmp(fptr, ".hmp"))
	{
		PHYSFS_mkdir(basedir);		// ALWAYS attempt to make 'Music': might exist in some searchpath but not the write directory
		snprintf(midi_filename, PATH_MAX, "%s/%s", basedir, filename);
		midi_filename[PATH_MAX - 1] = '\0';
		change_filename_extension(midi_filename, midi_filename, ".mid");
		hmp2mid(filename, midi_filename);
		filename = midi_filename;	// finished with hmp
	}

	loop = loop ? -1 : 1;	// loop means loop infinitely, otherwise play once

	// try loading music via given filename
	current_music = Mix_LoadMUS(filename);

	// no luck. so either it's in an archive or Searchpath
	if (!current_music)
	{
		PHYSFSX_getRealPath(filename, full_path);
		current_music = Mix_LoadMUS(full_path);
		if (current_music)
			filename = full_path;	// used later for possible error reporting
	}

	if (!current_music)
		filehandle = PHYSFS_openRead(filename);
	if (filehandle != NULL)
	{
		current_music_hndlbuf = d_realloc(current_music_hndlbuf, sizeof(char *)*PHYSFS_fileLength(filehandle));
		bufsize = PHYSFS_read(filehandle, current_music_hndlbuf, sizeof(char), PHYSFS_fileLength(filehandle));
		rw = SDL_RWFromConstMem(current_music_hndlbuf,bufsize*sizeof(char));
		PHYSFS_close(filehandle);
		current_music = Mix_LoadMUS_RW(rw);
	}

	if (current_music)
	{
		Mix_PlayMusic(current_music, loop);
		Mix_HookMusicFinished(hook_finished_track ? hook_finished_track : mix_free_music);
		return 1;
	}
	else
	{
		con_printf(CON_CRITICAL,"Music %s could not be loaded\n", filename);
		Mix_HaltMusic();
		if (current_music_hndlbuf)
		{
			d_free(current_music_hndlbuf);
			current_music_hndlbuf = NULL;
		}
	}

	return 0;
}

// What to do when stopping song playback
void mix_free_music()
{
	Mix_HaltMusic();
	if (current_music)
	{
		Mix_FreeMusic(current_music);
		current_music = NULL;
	}
	if (current_music_hndlbuf)
	{
		d_free(current_music_hndlbuf);
		current_music_hndlbuf = NULL;
	}
}

void mix_set_music_volume(int vol)
{
	vol *= MIX_MAX_VOLUME/8;
	Mix_VolumeMusic(vol);
}

void mix_stop_music()
{
	Mix_HaltMusic();
	if (current_music_hndlbuf)
	{
		d_free(current_music_hndlbuf);
		current_music_hndlbuf = NULL;
	}
}

void mix_pause_music()
{
	Mix_PauseMusic();
}

void mix_resume_music()
{
	Mix_ResumeMusic();
}

void mix_pause_resume_music()
{
	if (Mix_PausedMusic())
		Mix_ResumeMusic();
	else if (Mix_PlayingMusic())
		Mix_PauseMusic();
}
