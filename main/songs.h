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

int songs_play_song( int songnum, int repeat );
int songs_play_level_song( int levelnum, int offset );

//stop any songs - midi, redbook or jukebox - that are currently playing
void songs_stop_all(void);

void songs_pause(void);
void songs_resume(void);
void songs_pause_resume(void);

// check which song is playing, or -1 if not playing anything
int songs_is_playing();

// set volume for selected music playback system
void songs_set_volume(int volume);

void songs_uninit();

#endif

