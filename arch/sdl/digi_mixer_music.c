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
#include "hmp.h"
#include "digi_mixer_music.h"
#include "cfile.h"
#include "u_mem.h"


Mix_Music *current_music = NULL;

/*
 *  Plays a music file from an absolute path
 */

int mix_play_file(char *filename, int loop, void (*hook_finished_track)())
{
	SDL_RWops *rw = NULL;
	PHYSFS_file *filehandle = NULL;
	char tmp_file[PATH_MAX], real_filename[PATH_MAX];
	char *basedir = "music", *fptr, *buf = NULL;
	int bufsize = 0;

	mix_free_music();	// stop and free what we're already playing, if anything

	fptr = strrchr(filename, '.');

	if (fptr == NULL)
		return 0;

	snprintf(tmp_file, sizeof(char)*(strlen(filename)+1), filename);

	// It's a .hmp. Convert to mid, store ind music/ and pass the new filename.
	if (!stricmp(fptr, ".hmp"))
	{
		strcpy(tmp_file+strlen(tmp_file)-4,".mid");
		if (!PHYSFS_isDirectory(basedir))
			PHYSFS_mkdir(basedir);
		snprintf(real_filename, strlen(basedir)+strlen(tmp_file)+6, "%s/%s", basedir, tmp_file);
		hmp2mid(filename, real_filename);
	}
	else
	{
		snprintf(real_filename, sizeof(char)*(strlen(filename)+1), filename);
	}

	loop = loop ? -1 : 1;	// loop means loop infinitely, otherwise play once

	// try loading music via given filename
	current_music = Mix_LoadMUS(real_filename);

	// no luck. so either it's in an archive or Searchpath
	if (!current_music)
		filehandle = PHYSFS_openRead(real_filename);
	if (filehandle != NULL)
	{
		buf = realloc(buf, sizeof(char *)*PHYSFS_fileLength(filehandle));
		bufsize = PHYSFS_read(filehandle, buf, sizeof(char), PHYSFS_fileLength(filehandle));
		rw = SDL_RWFromConstMem(buf,bufsize*sizeof(char));
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
		con_printf(CON_CRITICAL,"Music %s could not be loaded\n", real_filename);
		Mix_HaltMusic();
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
}

void mix_set_music_volume(int vol)
{
	Mix_VolumeMusic(vol);
}

void mix_stop_music()
{
	Mix_HaltMusic();
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
