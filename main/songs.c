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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
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

#define NumLevelSongs (Num_songs - SONG_LEVEL_MUSIC)

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
			if (i >= SONG_LEVEL_MUSIC)
			{
				sprintf(Songs[i].filename, "game%02d.hmp", i - SONG_LEVEL_MUSIC + 1);
				if (!digi_music_exists(Songs[i].filename))
					sprintf(Songs[i].filename, "game%d.hmp", i - SONG_LEVEL_MUSIC);
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

		// HACK: If Descent.hog is patched from 1.0 to 1.5, descent.sng is broken and will not exceed 12 songs. So let's HACK it here.
		if (i==12 && cfile_size("descent.sng")==422)
		{
			sprintf(Songs[i].filename,"game08.hmp"); sprintf(Songs[i].melodic_bank_file,"rickmelo.bnk"); sprintf(Songs[i].drum_bank_file,"rickdrum.bnk");
			sprintf(Songs[i].filename,"game09.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game10.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game11.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
			sprintf(Songs[i].filename,"game12.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game13.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
			sprintf(Songs[i].filename,"game14.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
			sprintf(Songs[i].filename,"game15.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game16.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game17.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game18.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
			sprintf(Songs[i].filename,"game19.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game20.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
			sprintf(Songs[i].filename,"game21.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
			sprintf(Songs[i].filename,"game22.hmp"); sprintf(Songs[i].melodic_bank_file,"hammelo.bnk"); sprintf(Songs[i].drum_bank_file,"hamdrum.bnk");
			i+=15;
		}
	}

	Num_songs = i;
	Songs_initialized = 1;
	if (fp != NULL)	cfclose(fp);
	if ( Songs_initialized ) return;
	
	//	Set up External Music - ie Redbook/Jukebox
	if (EXT_MUSIC_ON)
	{
		ext_music_load();
		set_extmusic_volume(GameCfg.MusicVolume);
	}
}

#define FADE_TIME (f1_0/2)

//stop the external music
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
 * This list may not be exhaustive!!
 */
#define D1_MAC_OEM_DISCID       0xde0feb0e // Descent CD that came with the Mac Performa 6400, hope mine isn't scratched [too much]

#define REDBOOK_FIRST_LEVEL_TRACK	  (songs_haved1_cd() ? (GameCfg.SndEnableRedbook ? 6 : 5) : 1)
#define REDBOOK_ENDLEVEL_TRACK		  (GameCfg.SndEnableRedbook ? 4 : 3)
#define REDBOOK_ENDGAME_TRACK         (ext_music_get_numtracks())

// songs_haved1_cd returns 1 if the descent 1 Mac CD is in the drive and
// 0 otherwise

#if 1
int songs_haved1_cd()
{
	int discid;

	if (GameCfg.OrigTrackOrder)
		return 1;
	
	if (!GameCfg.SndEnableRedbook)
		return 0;

	discid = RBAGetDiscID();

	switch (discid) {
		case D1_MAC_OEM_DISCID:	// Doesn't work with your Mac Descent CD? Please tell!
			return 1;
		default:
			return 0;
	}
}
#else
int songs_haved1_cd()
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

void repeat_track()
{
	stop_time();
	if (Extmusic_playing)	// should be non-0, but for the sake of checking...
		play_extmusic_track(Extmusic_playing, 0, repeat_track);
	start_time();
}

void songs_play_song( int songnum, int repeat )
{
	songs_init();

	//stop any music already playing

	songs_stop_all();
	
	// The endgame track is the last track...
	if (songnum < SONG_ENDGAME)
		play_extmusic_track(songnum + 2 - GameCfg.JukeboxOn, 0, repeat ? repeat_track : NULL);
	else if (songnum == SONG_ENDGAME)
		play_extmusic_track(REDBOOK_ENDGAME_TRACK, 0, repeat ? repeat_track : NULL);
	else if (songnum > SONG_ENDGAME)
		play_extmusic_track(songnum + 1 - GameCfg.JukeboxOn, 0, repeat ? repeat_track : NULL);
	
	if (!Extmusic_playing)		//not playing external music, so play midi
		digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat );
}

int current_song_level;

void play_first_song()
{
	stop_time();
	songs_play_level_song(1);
	start_time();
}

void songs_play_level_song( int levelnum )
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
		// don't play endgame track during game

		play_extmusic_track(REDBOOK_FIRST_LEVEL_TRACK + ((n_tracks<=REDBOOK_FIRST_LEVEL_TRACK) ? 0 : (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK))), !songs_haved1_cd(), songs_haved1_cd() ? repeat_track : play_first_song);
	}

	if (! Extmusic_playing && (NumLevelSongs > 0)) {			//not playing external music, so play midi
		songnum = SONG_LEVEL_MUSIC + (songnum % NumLevelSongs);
		digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1 );
	}
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

