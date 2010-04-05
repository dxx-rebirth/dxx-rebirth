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

/*
 *
 * Routines to manage the songs in Descent.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "pstypes.h"
#include "songs.h"
#include "cfile.h"
#include "digi.h"
#include "rbaudio.h"
#include "config.h"
#include "timer.h"

song_info Songs[MAX_NUM_SONGS];
int Songs_initialized = 0;

int Num_songs;

//0 if external music is not playing, else the track number
static int Extmusic_playing = 0;

// 0 if no song playing, else the Descent song number
static int Song_playing = 0;

#define NumLevelSongs (Num_songs - SONG_FIRST_LEVEL_SONG)

#define EXTMUSIC_VOLUME_SCALE	(255)

//takes volume in range 0..8
void set_extmusic_volume(int volume)
{
	ext_music_set_volume(0);
	ext_music_set_volume(volume*EXTMUSIC_VOLUME_SCALE/8);
}

void songs_init()
{
	int i = 0;
	char inputline[80+1];
	CFILE * fp;

	memset(Songs, '\0', sizeof(Songs));

	fp = cfopen( "descent.sng", "rb" );
	if ( fp == NULL ) // No descent.sng available. Define a default song-set
	{
		for (i = 0; i < MAX_NUM_SONGS; i++) {
			strcpy(Songs[i].melodic_bank_file, "melodic.bnk");
			strcpy(Songs[i].drum_bank_file, "drum.bnk");
			if (i >= SONG_FIRST_LEVEL_SONG)
			{
				sprintf(Songs[i].filename, "game%02d.hmp", i - SONG_FIRST_LEVEL_SONG + 1);
				if (!digi_music_exists(Songs[i].filename))
					sprintf(Songs[i].filename, "game%d.hmp", i - SONG_FIRST_LEVEL_SONG);
				if (!digi_music_exists(Songs[i].filename))
				{
					Songs[i].filename[0] = '\0';	// music not available
					break;
				}
			}
		}
		strcpy(Songs[SONG_TITLE].filename, "descent.hmp");
		strcpy(Songs[SONG_BRIEFING].filename, "briefing.hmp");
		strcpy(Songs[SONG_CREDITS].filename, "credits.hmp");
		strcpy(Songs[SONG_ENDLEVEL].filename, "endlevel.hmp");	// can't find it? give a warning
		strcpy(Songs[SONG_ENDGAME].filename, "endgame.hmp");	// ditto
	}
	else
	{
		while (!PHYSFS_eof(fp))
		{
			if (i == MAX_NUM_SONGS)
				break;
				
			cfgets(inputline, 80, fp );
			if ( strlen( inputline ) )
			{
				memset(Songs[i].filename, '\0', sizeof(char)*16);
				memset(Songs[i].melodic_bank_file, '\0', sizeof(char)*16);
				memset(Songs[i].drum_bank_file, '\0', sizeof(char)*16);
				sscanf( inputline, "%15s %15s %15s",
						Songs[i].filename,
						Songs[i].melodic_bank_file,
						Songs[i].drum_bank_file );

				if (strchr(Songs[i].filename, '.'))
					if (!stricmp(strchr(Songs[i].filename, '.'), ".hmp"))
						i++;
			}
		}
		if (i <= SONG_FIRST_LEVEL_SONG)
			Error("Must have at least %d songs",SONG_FIRST_LEVEL_SONG+1);
	}

	Num_songs = i;
	Songs_initialized = 1;
	if (fp != NULL)	cfclose(fp);

	//	Set up External Music - ie Redbook/Jukebox
	if (EXT_MUSIC_ON)
	{
		ext_music_load();
		set_extmusic_volume(GameCfg.MusicVolume);
	}
}

#define FADE_TIME (f1_0/2)

//stop the external music
//only supposed to be called from within songs_stop_all,
//otherwise the value for Song_playing will be wrong
void songs_stop_extmusic(void)
{
	int old_volume = GameCfg.MusicVolume*EXTMUSIC_VOLUME_SCALE/8;
	fix old_time = timer_get_fixed_seconds();
	
	if (Extmusic_playing) {		//fade out volume
		int new_volume;
		do {
			fix t = timer_get_fixed_seconds();
			
			new_volume = fixmuldiv(old_volume,(FADE_TIME - (t-old_time)),FADE_TIME);
			
			if (new_volume < 0)
				new_volume = 0;
			
			ext_music_set_volume(new_volume);
			
		} while (new_volume > 0);
	}
	
	ext_music_stop();              	// Stop CD/Jukebox, if playing
	
	ext_music_set_volume(old_volume);	//restore volume
	
	Extmusic_playing = 0;		
	
}

//stop any songs - midi, redbook or jukebox - that are currently playing
void songs_stop_all(void)
{
	digi_stop_current_song();	// Stop midi song, if playing
	
	songs_stop_extmusic();			// Stop external music, if playing
	Song_playing = 0;
}

void reinit_extmusic()
{
	ext_music_load();
	set_extmusic_volume(GameCfg.MusicVolume);
}


//returns 1 if track started sucessfully
//start at tracknum.  if keep_playing set, play to end of disc.  else
//play only specified track
int play_extmusic_track(int tracknum,int keep_playing, void (*completion_proc)())
{
	Extmusic_playing = 0;
	
	if (!ext_music_is_loaded() && EXT_MUSIC_ON)
		reinit_extmusic();
	
	if (EXT_MUSIC_ON) {
		int num_tracks = ext_music_get_numtracks();
		if (tracknum <= num_tracks)
			if (ext_music_play_tracks(tracknum,keep_playing?num_tracks:tracknum, completion_proc))  {
				Extmusic_playing = tracknum;
			}
	}
		
		return (Extmusic_playing != 0);
}

/*
 * Some of these have different Track listings!
 * Which one is the "correct" order?
 */
#define D2_1_DISCID         0x7d0ff809 // Descent II
#define D2_2_DISCID         0xe010a30e // Descent II
#define D2_3_DISCID         0xd410070d // Descent II
#define D2_4_DISCID         0xc610080d // Descent II
#define D2_DEF_DISCID       0x87102209 // Definitive collection Disc 2
#define D2_OEM_DISCID       0xac0bc30d // Destination: Quartzon
#define D2_OEM2_DISCID      0xc40c0a0d // Destination: Quartzon
#define D2_VERTIGO_DISCID   0x53078208 // Vertigo
#define D2_VERTIGO2_DISCID  0x64071408 // Vertigo + DMB
#define D2_MAC_DISCID       0xb70ee40e // Macintosh
#define D2_IPLAY_DISCID     0x22115710 // iPlay for Macintosh

#define REDBOOK_TITLE_TRACK         (GameCfg.SndEnableRedbook ? 2 : 1)
#define REDBOOK_CREDITS_TRACK       (GameCfg.SndEnableRedbook ? 3 : 2)
#define REDBOOK_FIRST_LEVEL_TRACK   (songs_haved2_cd() ? (GameCfg.SndEnableRedbook ? 4 : 3) : 1)

// songs_haved2_cd returns 1 if the descent 2 CD is in the drive and
// 0 otherwise

#if 1
int songs_haved2_cd()
{
	int discid;

	if (GameCfg.OrigTrackOrder)
		return 1;
	
	if (!GameCfg.SndEnableRedbook)
		return 0;

	discid = RBAGetDiscID();

	switch (discid) {
	case D2_1_DISCID:
	case D2_2_DISCID:
	case D2_3_DISCID:
	case D2_4_DISCID:
	case D2_DEF_DISCID:
	case D2_OEM_DISCID:
	case D2_OEM2_DISCID:
	case D2_VERTIGO_DISCID:
	case D2_VERTIGO2_DISCID:
	case D2_MAC_DISCID:
	case D2_IPLAY_DISCID:
		return 1;
	default:
		return 0;
	}
}
#else
int songs_haved2_cd()
{
	char temp[128],cwd[128];
	
	getcwd(cwd, 128);

	strcpy(temp,CDROM_dir);
	
#ifndef MACINTOSH		//for PC, strip of trailing slash
	if (temp[strlen(temp)-1] == '\\')
		temp[strlen(temp)-1] = 0;
#endif

	if ( !chdir(temp) ) {
		chdir(cwd);
		return 1;
	}

	return 0;
}
#endif

void play_credits_track()
{
	stop_time();
	play_extmusic_track(REDBOOK_CREDITS_TRACK, 0, play_credits_track);
	start_time();
}

int songs_play_song( int songnum, int repeat )
{
	#ifndef SHAREWARE
	//Assert(songnum != SONG_ENDLEVEL && songnum != SONG_ENDGAME);	//not in full version
	#endif

	songs_init();

	//stop any music already playing

	songs_stop_all();

	//do we want any of these to be redbook songs?

	if (songnum == SONG_TITLE)
		play_extmusic_track(REDBOOK_TITLE_TRACK, 0, repeat ? play_credits_track : NULL);
	else if (songnum == SONG_CREDITS)
		play_extmusic_track(REDBOOK_CREDITS_TRACK, 0, repeat ? play_credits_track : NULL);

	if (!Extmusic_playing)		//not playing external music, so play midi
	{
		if (digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat ))
			Song_playing = songnum;
	}
	else
		Song_playing = songnum;
	
	return Song_playing;
}

int current_song_level;

void play_first_song()
{
	stop_time();
	songs_play_level_song(1);
	start_time();
}

int songs_play_level_song( int levelnum )
{
	int songnum;
	int n_tracks;

	Assert( levelnum != 0 );

	songs_init();

	songs_stop_all();

	current_song_level = levelnum;

	songnum = (levelnum>0)?(levelnum-1):(-levelnum);

	if (!ext_music_is_loaded() && EXT_MUSIC_ON)	// need this to determine if we currently have the official CD
		reinit_extmusic();

	n_tracks = ext_music_get_numtracks();
	
	if (ext_music_is_loaded() && EXT_MUSIC_ON) {
		//try to play external music (Redbook / Jukebox)
		play_extmusic_track(REDBOOK_FIRST_LEVEL_TRACK + ((n_tracks+1<=REDBOOK_FIRST_LEVEL_TRACK) ? 0 : (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK+1))), 1, play_first_song);
	}

	if (! Extmusic_playing && (NumLevelSongs > 0)) {			//not playing external music, so play midi
		songnum = SONG_FIRST_LEVEL_SONG + (songnum % NumLevelSongs);
		if (digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1 ))
			Song_playing = songnum;
	}
	else if (Extmusic_playing)
		Song_playing = songnum;
	
	return Song_playing;
}

//goto the next level song
void songs_goto_next_song()
{
	if (Extmusic_playing) 		//get correct track
		current_song_level = ext_music_get_track_playing() - REDBOOK_FIRST_LEVEL_TRACK + 1;
	
	songs_play_level_song(current_song_level+1);

}

//goto the previous level song
void songs_goto_prev_song()
{
	if (Extmusic_playing) 		//get correct track
		current_song_level = ext_music_get_track_playing() - REDBOOK_FIRST_LEVEL_TRACK + 1;
	
	if (current_song_level > 1)
		songs_play_level_song(current_song_level-1);

}

// check which song is playing
int songs_is_playing()
{
	return Song_playing;
}

