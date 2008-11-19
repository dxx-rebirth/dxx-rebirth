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
#include "jukebox.h"

song_info Songs[MAX_SONGS];
int Songs_initialized = 0;
int cGameSongsAvailable = 0;

//0 if redbook is no playing, else the track number
static int Redbook_playing = 0;

#ifndef MACINTOSH
#define REDBOOK_VOLUME_SCALE  (255/3)		//255 is MAX
#else
#define REDBOOK_VOLUME_SCALE	(255)
#endif

//takes volume in range 0..8
void set_redbook_volume(int volume)
{
#ifndef MACINTOSH
	RBASetVolume(0);		// makes the macs sound really funny
#endif
	RBASetVolume(volume*REDBOOK_VOLUME_SCALE/8);
}

void songs_init()
{
	int i;
	char inputline[80+1];
	CFILE * fp;

	fp = cfopen( "descent.sng", "rb" );
	if ( fp == NULL )	{
		int i;
		
		for (i = 0; i < SONG_LEVEL_MUSIC + NUM_GAME_SONGS; i++) {
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
		cGameSongsAvailable = i - SONG_LEVEL_MUSIC;
		return;
	}

	i = 0;
	while (cfgets(inputline, 80, fp )) {
		char *p = strchr(inputline,'\n');
		if (p) *p = '\0';
		if ( strlen( inputline ) )	{
			Assert( i < MAX_SONGS );
			sscanf( inputline, "%15s %15s %15s", Songs[i].filename, Songs[i].melodic_bank_file, Songs[i].drum_bank_file );
			i++;
		}
	}

	// HACK: If Descent.hog is patched from 1.0 to 1.5, descent.sng is broken and will not exceed 12 songs. So let's HACK it here.
	if (i==12)
	{
		sprintf(Songs[i].filename,"game08.hmp"); sprintf(Songs[i].melodic_bank_file,"rickmelo.bnk"); sprintf(Songs[i].drum_bank_file,"rickdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game09.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game10.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game11.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game12.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game13.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game14.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game15.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game16.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game17.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game18.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game19.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game20.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game21.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game22.hmp"); sprintf(Songs[i].melodic_bank_file,"hammelo.bnk"); sprintf(Songs[i].drum_bank_file,"hamdrum.bnk");
		i++;
	}

	cGameSongsAvailable = i - SONG_LEVEL_MUSIC;
	Songs_initialized = 1;
	cfclose(fp);
	
	if ( Songs_initialized ) return;
	
	//	RBA Hook
#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )
	if (GameCfg.SndEnableRedbook)
	{
		RBAInit();
		set_redbook_volume(GameCfg.MusicVolume);
	}
#endif	// endof ifndef SHAREWARE, ie ifdef SHAREWARE
}

#define FADE_TIME (f1_0/2)

//stop the redbook, so we can read off the CD
void songs_stop_redbook(void)
{
	int old_volume = GameCfg.MusicVolume*REDBOOK_VOLUME_SCALE/8;
	fix old_time = timer_get_fixed_seconds();
	
	if (Redbook_playing) {		//fade out volume
		int new_volume;
		do {
			fix t = timer_get_fixed_seconds();
			
			new_volume = fixmuldiv(old_volume,(FADE_TIME - (t-old_time)),FADE_TIME);
			
			if (new_volume < 0)
				new_volume = 0;
			
			RBASetVolume(new_volume);
			
		} while (new_volume > 0);
	}
	
	RBAStop();              	// Stop CD, if playing
	
	RBASetVolume(old_volume);	//restore volume
	
	Redbook_playing = 0;		
	
}

//stop any songs - midi or redbook - that are currently playing
void songs_stop_all(void)
{
	digi_stop_current_song();	// Stop midi song, if playing
	
	songs_stop_redbook();			// Stop CD, if playing
}

void reinit_redbook()
{
	RBAInit();
	set_redbook_volume(GameCfg.MusicVolume);
}


//returns 1 if track started sucessfully
//start at tracknum.  if keep_playing set, play to end of disc.  else
//play only specified track
int play_redbook_track(int tracknum,int keep_playing)
{
	Redbook_playing = 0;
	
	if (!RBAEnabled() && GameCfg.SndEnableRedbook)
		reinit_redbook();
	
	if (GameCfg.SndEnableRedbook) {
		int num_tracks = RBAGetNumberOfTracks();
		if (tracknum <= num_tracks)
			if (RBAPlayTracks(tracknum,keep_playing?num_tracks:tracknum))  {
				Redbook_playing = tracknum;
			}
	}
		
		return (Redbook_playing != 0);
}

/*
 * This list may not be exhaustive!!
 */
#define D1_MAC_OEM_DISCID       0xde0feb0e // Descent CD that came with the Mac Performa 6400, hope mine isn't scratched [too much]

#define REDBOOK_FIRST_LEVEL_TRACK	  (songs_haved1_cd()?6:1)
#define REDBOOK_ENDLEVEL_TRACK		  4
#define REDBOOK_ENDGAME_TRACK         14

// songs_haved1_cd returns 1 if the descent 1 Mac CD is in the drive and
// 0 otherwise

#if 1
int songs_haved1_cd()
{
	int discid;
	
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

void songs_play_song( int songnum, int repeat )
{
	songs_init();

	//stop any music already playing
	
	songs_stop_all();
	
	// The endgame track is the last track...
	if (songnum < SONG_ENDGAME)
		play_redbook_track(songnum + 2,0);
	else if (songnum == SONG_ENDGAME)
		play_redbook_track(REDBOOK_ENDGAME_TRACK,0);
	else if (songnum > SONG_ENDGAME)
		play_redbook_track(songnum + 1,0);
	
	if (!Redbook_playing)		//not playing redbook, so play midi
		digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat );
}

int current_song_level;

void songs_play_level_song( int levelnum )
{
	int songnum;
	int n_tracks;

	Assert( levelnum != 0 );

	songs_init();

	songs_stop_all();
	
	if (cGameSongsAvailable < 1)
		return;

	current_song_level = levelnum;

	if (levelnum < 0)
		songnum = (-levelnum) % cGameSongsAvailable;
	else
		songnum = (levelnum-1) % cGameSongsAvailable;

	if (!RBAEnabled() && GameCfg.SndEnableRedbook)	// need this to determine if we currently have the official CD
		reinit_redbook();

	n_tracks = RBAGetNumberOfTracks();
	
	if (RBAEnabled() && GameCfg.SndEnableRedbook) {
		
		//try to play redbook
		
		play_redbook_track(REDBOOK_FIRST_LEVEL_TRACK + (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK+1)),!songs_haved1_cd());
	}
	
	if (! Redbook_playing) {			//not playing redbook, so play midi
		songnum += SONG_LEVEL_MUSIC;
		digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1 );
	}
}

//this should be called regularly to check for redbook restart
//ideally, this would be handled by a hook
void songs_check_redbook_repeat()
{
	static fix last_check_time;
	fix current_time;
	
	if (!Redbook_playing || GameCfg.MusicVolume==0) return;
	
	current_time = timer_get_fixed_seconds();
	if (current_time < last_check_time || (current_time - last_check_time) >= F2_0) {
		if (!RBAPeekPlayStatus() && (Redbook_playing != REDBOOK_ENDLEVEL_TRACK)) {
			stop_time();
			play_redbook_track(Redbook_playing, 0);
			start_time();
		}
		last_check_time = current_time;
	}
}

//goto the next level song
void songs_goto_next_song()
{
	if (Redbook_playing) 		//get correct track
		current_song_level = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;
#ifdef USE_SDLMIXER
	else if (GameCfg.JukeboxOn)
	{
		jukebox_next();
		return;
	}
#endif
	
	songs_play_level_song(current_song_level+1);
	
}

//goto the previous level song
void songs_goto_prev_song()
{
	if (Redbook_playing) 		//get correct track
		current_song_level = RBAGetTrackNum() - REDBOOK_FIRST_LEVEL_TRACK + 1;
#ifdef USE_SDLMIXER
	else if (GameCfg.JukeboxOn)
	{
		jukebox_prev();
		return;
	}
#endif
	
	if (current_song_level > 1)
		songs_play_level_song(current_song_level-1);
	
}

