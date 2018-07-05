/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * This is an alternate backend for the music system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 *  -- MD2211 (2006-04-24)
 */

#include <memory>
#include <SDL.h>
#include <SDL_mixer.h>
#include <string.h>
#include <stdlib.h>

#include "args.h"
#include "hmp.h"
#include "digi_mixer_music.h"
#include "strutil.h"
#include "u_mem.h"
#include "console.h"

namespace dcx {

namespace {

#if SDL_MIXER_MAJOR_VERSION == 2
#define DXX_SDL_MIXER_MANAGES_RWOPS	1
#else
#define DXX_SDL_MIXER_MANAGES_RWOPS	0
#endif

#if DXX_SDL_MIXER_MANAGES_RWOPS
#define	DXX_SDL_MIXER_Mix_LoadMUS_MANAGE_RWOPS	, SDL_TRUE
#define DXX_SDL_MIXER_Mix_LoadMUS_PASS_RWOPS(rw)
#else
#define DXX_SDL_MIXER_Mix_LoadMUS_MANAGE_RWOPS
#define DXX_SDL_MIXER_Mix_LoadMUS_PASS_RWOPS(rw)	, rw
#endif

class current_music_t
{
#if !DXX_SDL_MIXER_MANAGES_RWOPS
	struct RWops_delete
	{
		void operator()(SDL_RWops *o)
		{
			SDL_RWclose(o);
		}
	};
	using rwops_pointer = std::unique_ptr<SDL_RWops, RWops_delete>;
	rwops_pointer m_ops;
#endif
	struct Music_delete
	{
		void operator()(Mix_Music *m) { Mix_FreeMusic(m); }
	};
	using music_pointer = std::unique_ptr<Mix_Music, Music_delete>;
	music_pointer m_music;
public:
	void reset(
		Mix_Music *const music = nullptr
#if !DXX_SDL_MIXER_MANAGES_RWOPS
		, SDL_RWops *const rwops = nullptr
#endif
		) noexcept
	{
		/* Clear music first in case it needs the old ops
		 * Clear old ops
		 * If no new music, clear new ops immediately.  This only
		 * happens if the new music fails to load.
		 */
		m_music.reset(music);
#if !DXX_SDL_MIXER_MANAGES_RWOPS
		m_ops.reset(rwops);
		if (!music)
			m_ops.reset();
#endif
	}
	bool operator!() const { return !m_music; }
	explicit operator bool() const { return static_cast<bool>(m_music); }
	typename music_pointer::pointer get() { return m_music.get(); }
};

}

static current_music_t current_music;
static std::vector<uint8_t> current_music_hndlbuf;

/*
 *  Plays a music file from an absolute path or a relative path
 */

int mix_play_file(const char *filename, int loop, void (*hook_finished_track)())
{
	SDL_RWops *rw = NULL;
	array<char, PATH_MAX> full_path;
	const char *fptr;
	unsigned int bufsize = 0;

	mix_free_music();	// stop and free what we're already playing, if anything

	fptr = strrchr(filename, '.');

	if (fptr == NULL)
		return 0;

	// It's a .hmp!
	if (!d_stricmp(fptr, ".hmp"))
	{
		hmp2mid(filename, current_music_hndlbuf);
		rw = SDL_RWFromConstMem(&current_music_hndlbuf[0], current_music_hndlbuf.size()*sizeof(char));
		current_music.reset(Mix_LoadMUS_RW(rw DXX_SDL_MIXER_Mix_LoadMUS_MANAGE_RWOPS) DXX_SDL_MIXER_Mix_LoadMUS_PASS_RWOPS(rw));
	}

	// try loading music via given filename
	if (!current_music)
		current_music.reset(Mix_LoadMUS(filename));

	// allow the shell convention tilde character to mean the user's home folder
	// chiefly used for default jukebox level song music referenced in 'descent.m3u' for Mac OS X
	if (!current_music && *filename == '~')
	{
		const auto sep = PHYSFS_getDirSeparator();
		const auto lensep = strlen(sep);
		snprintf(full_path.data(), PATH_MAX, "%s%s", PHYSFS_getUserDir(),
				 &filename[1 + (!strncmp(&filename[1], sep, lensep)
			? lensep
			: 0)]);
		current_music.reset(Mix_LoadMUS(full_path.data()));
		if (current_music)
			filename = full_path.data();	// used later for possible error reporting
	}
		

	// no luck. so it might be in Searchpath. So try to build absolute path
	if (!current_music)
	{
		PHYSFSX_getRealPath(filename, full_path);
		current_music.reset(Mix_LoadMUS(full_path.data()));
		if (current_music)
			filename = full_path.data();	// used later for possible error reporting
	}

	// still nothin'? Let's open via PhysFS in case it's located inside an archive
	if (!current_music)
	{
		if (RAIIPHYSFS_File filehandle{PHYSFS_openRead(filename)})
		{
			unsigned len = PHYSFS_fileLength(filehandle);
			current_music_hndlbuf.resize(len);
			bufsize = PHYSFS_read(filehandle, &current_music_hndlbuf[0], sizeof(char), len);
			rw = SDL_RWFromConstMem(&current_music_hndlbuf[0], bufsize*sizeof(char));
			current_music.reset(Mix_LoadMUS_RW(rw DXX_SDL_MIXER_Mix_LoadMUS_MANAGE_RWOPS) DXX_SDL_MIXER_Mix_LoadMUS_PASS_RWOPS(rw));
		}
	}

	if (current_music)
	{
		Mix_PlayMusic(current_music.get(), (loop ? -1 : 1));
		Mix_HookMusicFinished(hook_finished_track ? hook_finished_track : mix_free_music);
		return 1;
	}
	else
	{
		con_printf(CON_CRITICAL,"Music %s could not be loaded: %s", filename, Mix_GetError());
		mix_stop_music();
	}

	return 0;
}

// What to do when stopping song playback
void mix_free_music()
{
	Mix_HaltMusic();
	current_music.reset();
	current_music_hndlbuf.clear();
}

void mix_set_music_volume(int vol)
{
	vol *= MIX_MAX_VOLUME/8;
	Mix_VolumeMusic(vol);
}

void mix_stop_music()
{
	Mix_HaltMusic();
	current_music_hndlbuf.clear();
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

}
