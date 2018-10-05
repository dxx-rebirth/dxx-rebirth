/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#include "adlmidi.h"
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

struct ADLMIDI_delete
{
	void operator()(ADL_MIDIPlayer *x)
	{
		adl_close(x);
	}
};

typedef std::unique_ptr<ADL_MIDIPlayer, ADLMIDI_delete> ADL_MIDIPlayer_t;

}

static current_music_t current_music;
static ADL_MIDIPlayer_t current_adlmidi;
static std::vector<uint8_t> current_music_hndlbuf;

static ADL_MIDIPlayer *get_adlmidi()
{
	ADL_MIDIPlayer *adlmidi = current_adlmidi.get();
	if (!adlmidi)
	{
		int sample_rate;
		Mix_QuerySpec(&sample_rate, nullptr, nullptr);
		adlmidi = adl_init(sample_rate);
		adl_switchEmulator(adlmidi, ADLMIDI_EMU_DOSBOX);
		adl_setNumChips(adlmidi, 6);
		adl_setBank(adlmidi, 31);
		adl_setSoftPanEnabled(adlmidi, 1);
		adl_setLoopEnabled(adlmidi, 1);
		current_adlmidi.reset(adlmidi);
	}
	return adlmidi;
}

enum class CurrentMusicType {
	None,
	ADLMIDI,
	SDLMixer,
};

static CurrentMusicType current_music_type = CurrentMusicType::None;

static CurrentMusicType load_mus_data(const uint8_t *data, size_t size);
static CurrentMusicType load_mus_file(const char *filename);

static void mix_adlmidi(void *udata, Uint8 *stream, int len);


/*
 *  Plays a music file from an absolute path or a relative path
 */

int mix_play_file(const char *filename, int loop, void (*hook_finished_track)())
{
	array<char, PATH_MAX> full_path;
	const char *fptr;
	unsigned int bufsize = 0;

	// fprintf(stderr, "Load music: %s\n", filename);

	mix_free_music();	// stop and free what we're already playing, if anything

	fptr = strrchr(filename, '.');

	if (fptr == NULL)
		return 0;

	// It's a .hmp!
	if (!d_stricmp(fptr, ".hmp"))
	{
		hmp2mid(filename, current_music_hndlbuf);
		current_music_type = load_mus_data(current_music_hndlbuf.data(), current_music_hndlbuf.size());
	}

	// try loading music via given filename
	if (current_music_type == CurrentMusicType::None)
		current_music_type = load_mus_file(filename);

	// allow the shell convention tilde character to mean the user's home folder
	// chiefly used for default jukebox level song music referenced in 'descent.m3u' for Mac OS X
	if (current_music_type == CurrentMusicType::None && *filename == '~')
	{
		const auto sep = PHYSFS_getDirSeparator();
		const auto lensep = strlen(sep);
		snprintf(full_path.data(), PATH_MAX, "%s%s", PHYSFS_getUserDir(),
				 &filename[1 + (!strncmp(&filename[1], sep, lensep)
			? lensep
			: 0)]);
		current_music_type = load_mus_file(full_path.data());
		if (current_music_type != CurrentMusicType::None)
			filename = full_path.data();	// used later for possible error reporting
	}

	// no luck. so it might be in Searchpath. So try to build absolute path
	if (current_music_type == CurrentMusicType::None)
	{
		PHYSFSX_getRealPath(filename, full_path);
		current_music_type = load_mus_file(full_path.data());
		if (current_music_type != CurrentMusicType::None)
			filename = full_path.data();	// used later for possible error reporting
	}

	// still nothin'? Let's open via PhysFS in case it's located inside an archive
	if (current_music_type == CurrentMusicType::None)
	{
		if (RAIIPHYSFS_File filehandle{PHYSFS_openRead(filename)})
		{
			unsigned len = PHYSFS_fileLength(filehandle);
			current_music_hndlbuf.resize(len);
			bufsize = PHYSFS_read(filehandle, &current_music_hndlbuf[0], sizeof(char), len);
			current_music_type = load_mus_data(current_music_hndlbuf.data(), bufsize*sizeof(char));
		}
	}

	switch (current_music_type)
	{

	case CurrentMusicType::ADLMIDI:
	{
		Mix_HookMusic(&mix_adlmidi, nullptr);
		Mix_HookMusicFinished(hook_finished_track ? hook_finished_track : mix_free_music);
		return 1;
	}

	case CurrentMusicType::SDLMixer:
	{
		Mix_PlayMusic(current_music.get(), (loop ? -1 : 1));
		Mix_HookMusicFinished(hook_finished_track ? hook_finished_track : mix_free_music);
		return 1;
	}

	default:
	{
		con_printf(CON_CRITICAL,"Music %s could not be loaded: %s", filename, Mix_GetError());
		mix_stop_music();
	}

	}

	return 0;
}

// What to do when stopping song playback
void mix_free_music()
{
	Mix_HaltMusic();
	Mix_HookMusic(nullptr, nullptr);
	current_music.reset();
	current_music_hndlbuf.clear();
	current_music_type = CurrentMusicType::None;
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

static CurrentMusicType load_mus_data(const uint8_t *data, size_t size)
{
	CurrentMusicType type = CurrentMusicType::None;
	ADL_MIDIPlayer *adlmidi = get_adlmidi();
	SDL_RWops *rw = NULL;

	if (adl_openData(adlmidi, data, size) == 0)
		type = CurrentMusicType::ADLMIDI;
	else
	{
		rw = SDL_RWFromConstMem(data, size);
		current_music.reset(Mix_LoadMUS_RW(rw DXX_SDL_MIXER_Mix_LoadMUS_MANAGE_RWOPS) DXX_SDL_MIXER_Mix_LoadMUS_PASS_RWOPS(rw));
		if (current_music)
			type = CurrentMusicType::SDLMixer;
	}

	return type;
}

static CurrentMusicType load_mus_file(const char *filename)
{
	CurrentMusicType type = CurrentMusicType::None;
	ADL_MIDIPlayer *adlmidi = get_adlmidi();

	if (adl_openFile(adlmidi, filename) == 0)
		type = CurrentMusicType::ADLMIDI;
	else
	{
		current_music.reset(Mix_LoadMUS(filename));
		if (current_music)
			type = CurrentMusicType::SDLMixer;
	}

	return type;
}

static int16_t sat16(int32_t x)
{
	x = (x < INT16_MIN) ? INT16_MIN : x;
	x = (x > INT16_MAX) ? INT16_MAX : x;
	return x;
}

static void mix_adlmidi(void *, Uint8 *stream, int len)
{
	ADLMIDI_AudioFormat format;
	format.containerSize = sizeof(int16_t);
	format.sampleOffset = 2 * format.containerSize;
	format.type = ADLMIDI_SampleType_S16;

	ADL_MIDIPlayer *adlmidi = get_adlmidi();
	int sampleCount = len / format.containerSize;
	adl_playFormat(adlmidi, sampleCount, stream, stream + format.containerSize, &format);

	const auto samples = reinterpret_cast<int16_t *>(stream);
	const auto amplify = [](int16_t i) { return sat16(2 * i); };
	std::transform(samples, samples + sampleCount, samples, amplify);
}

}
