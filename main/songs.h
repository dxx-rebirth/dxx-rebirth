/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _SONGS_H
#define _SONGS_H

typedef struct song_info {
	char	filename[16];
	char	melodic_bank_file[16];
	char	drum_bank_file[16];
} song_info;

extern song_info Songs[];

#define SONG_TITLE				0
#ifdef MACINTOSH
#define SONG_BRIEFING			3		// endgame and briefing the same
#else
#define SONG_BRIEFING			1
#endif
#define SONG_ENDLEVEL			2
#define SONG_ENDGAME			3
#define SONG_CREDITS			4
#define SONG_FIRST_LEVEL_SONG	5


#ifdef MACINTOSH
#define MAX_NUM_SONGS			9
#define Num_songs					9
#else
#define MAX_NUM_SONGS			30
extern int Num_songs;	//how many MIDI songs
#endif

//whether or not redbook audio should be played
extern int Redbook_enabled;
extern int Redbook_playing;		// track that is currently playing

void songs_play_song( int songnum, int repeat );
void songs_play_level_song( int levelnum );

//stop the redbook, so we can read off the CD
void songs_stop_redbook(void);

//stop any songs - midi or redbook - that are currently playing
void songs_stop_all(void);

//this should be called regularly to check for redbook restart
void songs_check_redbook_repeat(void);

#endif
