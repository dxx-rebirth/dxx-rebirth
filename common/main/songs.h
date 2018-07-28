/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Header for songs.c
 *
 */

#ifndef _SONGS_H
#define _SONGS_H

#include <SDL_version.h>
#ifdef __cplusplus
#include "dxxsconf.h"

struct bim_song_info
{
	char    filename[16];
};

#define SONG_TITLE              0
#define SONG_BRIEFING           1
#define SONG_ENDLEVEL           2
#define SONG_ENDGAME            3
#define SONG_CREDITS            4
#define SONG_FIRST_LEVEL_SONG   5

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
int songs_play_song( int songnum, int repeat );
int songs_play_level_song( int levelnum, int offset );

//stop any songs - midi, redbook or jukebox - that are currently playing
}
#endif
void songs_stop_all(void);

// check which song is playing, or -1 if not playing anything
int songs_is_playing();

void songs_pause(void);
void songs_resume(void);
void songs_pause_resume(void);

// set volume for selected music playback system
void songs_set_volume(int volume);

void songs_uninit();

#endif

#endif

