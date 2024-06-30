/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Header for songs.c
 *
 */

#pragma once

#include <array>
#include <climits>
#include <SDL_version.h>
#include "dxxsconf.h"

namespace dcx {

struct bim_song_info
{
	std::array<char, 16> filename;
};

enum class song_number : unsigned
{
	None = UINT_MAX,
	title = 0,
	briefing = 1,
	endlevel = 2,
	endgame = 3,
	credits = 4,
	first_level_song = 5,
	/* additional values define songs for levels 2..N */
};

// Return which song is playing, or song_number::None if not playing anything
[[nodiscard]]
song_number songs_is_playing();

}

#define SONG_EXT_HMP            "hmp"
#if DXX_USE_SDLMIXER
#define SONG_EXT_MID            "mid"
#define SONG_EXT_OGG            "ogg"
#define SONG_EXT_FLAC           "flac"
#define SONG_EXT_MP3            "mp3"
#endif

#if !DXX_USE_SDLMIXER
#ifdef _WIN32
#define songs_play_file(filename,repeat,hook_finished_track)	songs_play_file(filename,repeat)
#else
#define songs_play_file(filename,repeat,hook_finished_track)	songs_play_file()
#endif
#if SDL_MAJOR_VERSION == 2
#define songs_play_song(songnum,repeat)	songs_play_song(songnum)
#endif
#endif
int songs_play_file(const char *filename, int repeat, void (*hook_finished_track)());
#ifdef dsx
namespace dsx {
void songs_play_song(song_number songnum, int repeat);
void songs_play_level_song(int levelnum, int offset);

//stop any songs - midi, redbook or jukebox - that are currently playing
}
#endif
void songs_stop_all(void);

void songs_pause(void);
void songs_resume(void);
void songs_pause_resume(void);

namespace dcx {

// set volume for selected music playback system
void songs_set_volume(int volume);

}

void songs_uninit();
