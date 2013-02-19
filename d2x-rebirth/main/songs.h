/*
 *
 * Header for songs.c
 *
 */

#ifndef _SONGS_H
#define _SONGS_H

typedef struct bim_song_info {
	char    filename[16];
} bim_song_info;

#define SONG_TITLE              0
#define SONG_BRIEFING           1
#define SONG_ENDLEVEL           2
#define SONG_ENDGAME            3
#define SONG_CREDITS            4
#define SONG_FIRST_LEVEL_SONG   5

#define SONG_EXT_HMP            ".hmp"
#ifdef USE_SDLMIXER
#define SONG_EXT_MID            ".mid"
#define SONG_EXT_OGG            ".ogg"
#define SONG_EXT_FLAC           ".flac"
#define SONG_EXT_MP3            ".mp3"
#endif

int songs_play_file(char *filename, int repeat, void (*hook_finished_track)());
int songs_play_song( int songnum, int repeat );
int songs_play_level_song( int levelnum, int offset );

//stop any songs - midi, redbook or jukebox - that are currently playing
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

