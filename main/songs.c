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
#include "digi.h"
#include "rbaudio.h"
#ifdef USE_SDLMIXER
#include "digi_mixer_music.h"
#include "jukebox.h"
#endif
#include "config.h"
#include "timer.h"

int Songs_initialized = 0;
static int Song_playing = -1; // -1 if no song playing, else the Descent song number
static int Redbook_playing = 0; // Redbook track num differs from Song_playing. We need this for Redbook repeat hooks.

bim_song_info *BIMSongs = NULL;
int Num_bim_songs;

#define EXTMUSIC_VOLUME_SCALE	(255)

//takes volume in range 0..8
//NOTE that we do not check what is playing right now (except Redbook) This is because here we don't (want) know WHAT we're playing - let the subfunctions do it (i.e. digi_win32_set_music_volume() knows if a MIDI plays or not)
void songs_set_volume(int volume)
{
#ifdef _WIN32
	digi_win32_set_midi_volume(volume);
#endif
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
	{
		RBASetVolume(0);
		RBASetVolume(volume);
	}
#ifdef USE_SDLMIXER
	mix_set_music_volume(volume);
#endif
}

// Set up everything for our music
// NOTE: you might think this is done once per runtime but it's not! It's done for EACH song so that each mission can have it's own descent.sng structure. We COULD optimize that by only doing this once per mission.
void songs_init()
{
	int i = 0;
	char inputline[80+1];
	PHYSFS_file * fp = NULL;
	char sng_file[PATH_MAX];

	Songs_initialized = 0;

	if (BIMSongs != NULL)
		d_free(BIMSongs);

	memset(sng_file, '\0', sizeof(sng_file));
	if (Current_mission != NULL) // try MISSION_NAME.sngdxx - might be rarely used but handy if you want a songfile for a specific mission outside of the mission hog file. use special extension to not crash with other ports of the game
	{
		snprintf(sng_file, strlen(Current_mission_filename)+8, "%s.sngdxx", Current_mission_filename);
		fp = PHYSFSX_openReadBuffered(sng_file);
	}

	if (fp == NULL) // try descent.sngdxx - a songfile specifically for dxx which level authors CAN use (dxx does not care if descent.sng contains MP3/OGG/etc. as well) besides the normal descent.sng containing files other versions of the game cannot play. this way a mission can contain a DOS-Descent compatible OST (hmp files) as well as a OST using MP3, OGG, etc.
		fp = PHYSFSX_openReadBuffered( "descent.sngdxx" );

	if (fp == NULL) // try to open regular descent.sng
		fp = PHYSFSX_openReadBuffered( "descent.sng" );

	if ( fp == NULL ) // No descent.sng available. Define a default song-set
	{
		int predef=30; // define 30 songs - period

		MALLOC(BIMSongs, bim_song_info, predef);
		if (!BIMSongs)
			return;

		strncpy(BIMSongs[SONG_TITLE].filename, "descent.hmp",sizeof(BIMSongs[SONG_TITLE].filename));
		strncpy(BIMSongs[SONG_BRIEFING].filename, "briefing.hmp",sizeof(BIMSongs[SONG_BRIEFING].filename));
		strncpy(BIMSongs[SONG_CREDITS].filename, "credits.hmp",sizeof(BIMSongs[SONG_CREDITS].filename));
		strncpy(BIMSongs[SONG_ENDLEVEL].filename, "endlevel.hmp",sizeof(BIMSongs[SONG_ENDLEVEL].filename));	// can't find it? give a warning
		strncpy(BIMSongs[SONG_ENDGAME].filename, "endgame.hmp",sizeof(BIMSongs[SONG_ENDGAME].filename));	// ditto

		for (i = SONG_FIRST_LEVEL_SONG; i < predef; i++) {
			snprintf(BIMSongs[i].filename, sizeof(BIMSongs[i].filename), "game%02d.hmp", i - SONG_FIRST_LEVEL_SONG + 1);
			if (!PHYSFSX_exists(BIMSongs[i].filename,1))
				snprintf(BIMSongs[i].filename, sizeof(BIMSongs[i].filename), "game%d.hmp", i - SONG_FIRST_LEVEL_SONG);
			if (!PHYSFSX_exists(BIMSongs[i].filename,1))
			{
				memset(BIMSongs[i].filename, '\0', sizeof(BIMSongs[i].filename)); // music not available
				break;
			}
		}
	}
	else
	{
		while (!PHYSFS_eof(fp))
		{
			PHYSFSX_fgets(inputline, 80, fp );
			if ( strlen( inputline ) )
			{
				BIMSongs = d_realloc(BIMSongs, sizeof(bim_song_info)*(i+1));
				memset(BIMSongs[i].filename, '\0', sizeof(BIMSongs[i].filename));
				sscanf( inputline, "%15s", BIMSongs[i].filename );

				if (strrchr(BIMSongs[i].filename, '.'))
					if (!stricmp(strrchr(BIMSongs[i].filename, '.'), ".hmp") ||
						!stricmp(strrchr(BIMSongs[i].filename, '.'), ".mp3") ||
						!stricmp(strrchr(BIMSongs[i].filename, '.'), ".ogg") ||
						!stricmp(strrchr(BIMSongs[i].filename, '.'), ".aif") ||
						!stricmp(strrchr(BIMSongs[i].filename, '.'), ".mid") ||
						!stricmp(strrchr(BIMSongs[i].filename, '.'), ".flac")
						)
						i++;
			}
		}

		// HACK: If Descent.hog is patched from 1.0 to 1.5, descent.sng is turncated. So let's patch it up here
		if (i==12 && PHYSFSX_fsize("descent.sng")==422)
		{
			BIMSongs = d_realloc(BIMSongs, sizeof(bim_song_info)*(i+15));
			for (i = 12; i <= 26; i++)
				snprintf(BIMSongs[i].filename, sizeof(BIMSongs[i].filename), "game%02d.hmp", i-4);
		}
	}

	Num_bim_songs = i;
	Songs_initialized = 1;
	if (fp != NULL)
		PHYSFS_close(fp);

	if (GameArg.SndNoMusic)
		GameCfg.MusicType = MUSIC_TYPE_NONE;

	// If SDL_Mixer is not supported (or deactivated), switch to no-music type if SDL_mixer-related music type was selected
#ifdef USE_SDLMIXER
	if (GameArg.SndDisableSdlMixer)
#else
	if (1)
#endif
	{
#ifndef _WIN32
		if (GameCfg.MusicType == MUSIC_TYPE_BUILTIN)
			GameCfg.MusicType = MUSIC_TYPE_NONE;
#endif
		if (GameCfg.MusicType == MUSIC_TYPE_CUSTOM)
			GameCfg.MusicType = MUSIC_TYPE_NONE;
	}

	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
		RBAInit();
#ifdef USE_SDLMIXER
	else if (GameCfg.MusicType == MUSIC_TYPE_CUSTOM)
		jukebox_load();
#endif

	songs_set_volume(GameCfg.MusicVolume);
}

void songs_uninit()
{
	songs_stop_all();
#ifdef USE_SDLMIXER
	jukebox_unload();
#endif
	if (BIMSongs != NULL)
		d_free(BIMSongs);
	Songs_initialized = 0;
}

//stop any songs - builtin, redbook or jukebox - that are currently playing
void songs_stop_all(void)
{
#ifdef _WIN32
	digi_win32_stop_midi_song();	// Stop midi song, if playing
#endif
	RBAStop();
#ifdef USE_SDLMIXER
	mix_stop_music();
#endif

	Song_playing = -1;
}

void songs_pause(void)
{
#ifdef _WIN32
	digi_win32_pause_midi_song();
#endif
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
		RBAPause();
#ifdef USE_SDLMIXER
	mix_pause_music();
#endif
}

void songs_resume(void)
{
#ifdef _WIN32
	digi_win32_resume_midi_song();
#endif
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
		RBAResume();
#ifdef USE_SDLMIXER
	mix_resume_music();
#endif
}

void songs_pause_resume(void)
{
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
		RBAPauseResume();
#ifdef USE_SDLMIXER
	mix_pause_resume_music();
#endif
}

/*
 * This list may not be exhaustive!!
 */
#define D1_MAC_OEM_DISCID       0xde0feb0e // Descent CD that came with the Mac Performa 6400, hope mine isn't scratched [too much]

#define REDBOOK_ENDLEVEL_TRACK		4
#define REDBOOK_ENDGAME_TRACK		(RBAGetNumberOfTracks())
#define REDBOOK_FIRST_LEVEL_TRACK	(songs_haved1_cd() ? 6 : 1)

// songs_haved1_cd returns 1 if the descent 1 Mac CD is in the drive and
// 0 otherwise
int songs_haved1_cd()
{
	int discid;

	if (GameCfg.OrigTrackOrder)
		return 1;

	if (!(GameCfg.MusicType == MUSIC_TYPE_REDBOOK))
		return 0;

	discid = RBAGetDiscID();

	switch (discid) {
		case D1_MAC_OEM_DISCID:	// Doesn't work with your Mac Descent CD? Please tell!
			return 1;
		default:
			return 0;
	}
}

void redbook_repeat_func()
{
	stop_time();
	RBAPlayTracks(Redbook_playing, 0, redbook_repeat_func);
	start_time();
}

int songs_play_song( int songnum, int repeat )
{
	songs_init();
	if (!Songs_initialized)
		return 0;

	switch (GameCfg.MusicType)
	{
		case MUSIC_TYPE_BUILTIN:
		{
			// EXCEPTION: If SONG_ENDLEVEL is not available, continue playing level song.
			if (Song_playing >= SONG_FIRST_LEVEL_SONG && songnum == SONG_ENDLEVEL && !PHYSFSX_exists(BIMSongs[songnum].filename, 1))
				return Song_playing;

			Song_playing = -1;
#ifdef _WIN32
			if (GameArg.SndDisableSdlMixer)
			{
				if (digi_win32_play_midi_song( BIMSongs[songnum].filename, repeat )) // NOTE: If SDL_mixer active, this will still be called in mix_play_file in case file is hmp
				{
					Song_playing = songnum;
				}
			}
			else
#endif
#ifdef USE_SDLMIXER
			{
				if (mix_play_file(BIMSongs[songnum].filename, repeat, NULL))
				{
					Song_playing = songnum;
				}
			}
#endif
			break;
		}
		case MUSIC_TYPE_REDBOOK:
		{
			int num_tracks = RBAGetNumberOfTracks();

			Song_playing = -1;
			if ((songnum < SONG_ENDGAME) && (songnum + 2 <= num_tracks))
			{
				if (RBAPlayTracks(songnum + 2, songnum + 2, repeat ? redbook_repeat_func : NULL))
				{
					Redbook_playing = songnum + 2;
					Song_playing = songnum;
				}
			}
			else if ((songnum == SONG_ENDGAME) && (REDBOOK_ENDGAME_TRACK <= num_tracks)) // The endgame track is the last track
			{
				if (RBAPlayTracks(REDBOOK_ENDGAME_TRACK, REDBOOK_ENDGAME_TRACK, repeat ? redbook_repeat_func : NULL))
				{
					Redbook_playing = REDBOOK_ENDGAME_TRACK;
					Song_playing = songnum;
				}
			}
			else if ((songnum > SONG_ENDGAME) && (songnum + 1 <= num_tracks))
			{
				if (RBAPlayTracks(songnum + 1, songnum + 1, repeat ? redbook_repeat_func : NULL))
				{
					Redbook_playing = songnum + 1;
					Song_playing = songnum;
				}
			}
			break;
		}
#ifdef USE_SDLMIXER
		case MUSIC_TYPE_CUSTOM:
		{
			// EXCEPTION: If SONG_ENDLEVEL is undefined, continue playing level song.
			if (Song_playing >= SONG_FIRST_LEVEL_SONG && songnum == SONG_ENDLEVEL && !strlen(GameCfg.CMMiscMusic[songnum]))
				return Song_playing;

			Song_playing = -1;
			if (mix_play_file(GameCfg.CMMiscMusic[songnum], repeat, NULL))
				Song_playing = songnum;
			break;
		}
#endif
		default:
			Song_playing = -1;
			break;
	}

	// If we couldn't play the song, most likely because it wasn't specified, play no music.
	if (Song_playing == -1)
		songs_stop_all();
	
	return Song_playing;
}

void redbook_first_song_func()
{
	stop_time();
	Song_playing = -1; // Playing Redbook tracks will not modify Song_playing. To repeat we must reset this so songs_play_level_song does not think we want to re-play the same song again.
	songs_play_level_song(1, 0);
	start_time();
}

// play track given by levelnum (depending on the music type and it's playing behaviour) or increment/decrement current track number via offset value
int songs_play_level_song( int levelnum, int offset )
{
	int songnum;

	Assert( levelnum != 0 );

	songs_init();
	if (!Songs_initialized)
		return 0;
	
	songnum = (levelnum>0)?(levelnum-1):(-levelnum);

	switch (GameCfg.MusicType)
	{
		case MUSIC_TYPE_BUILTIN:
		{
			if (offset)
				return Song_playing;

			Song_playing = -1;
			if ((Num_bim_songs - SONG_FIRST_LEVEL_SONG) > 0)
			{
				songnum = SONG_FIRST_LEVEL_SONG + (songnum % (Num_bim_songs - SONG_FIRST_LEVEL_SONG));
#ifdef _WIN32
				if (GameArg.SndDisableSdlMixer)
				{
					if (digi_win32_play_midi_song( BIMSongs[songnum].filename, 1 )) // NOTE: If SDL_mixer active, this will still be called in mix_play_file in case file is hmp
					{
						Song_playing = songnum;
					}
				}
#ifdef USE_SDLMIXER
				else
#endif
#endif
#ifdef USE_SDLMIXER
				{
					if (mix_play_file(BIMSongs[songnum].filename, 1, NULL))
					{
						Song_playing = songnum;
					}
				}
#endif
			}
			break;
		}
		case MUSIC_TYPE_REDBOOK:
		{
			int n_tracks = RBAGetNumberOfTracks();
			int tracknum;

			if (!offset)
			{
				// we have just been told to play the same as we do already -> ignore
				if (Song_playing >= SONG_FIRST_LEVEL_SONG && songnum + SONG_FIRST_LEVEL_SONG == Song_playing)
					return Song_playing;

				tracknum = REDBOOK_FIRST_LEVEL_TRACK + ((n_tracks<=REDBOOK_FIRST_LEVEL_TRACK) ? 0 : (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK)));
			}
			else
			{
				tracknum = Redbook_playing+offset;
				if (tracknum < REDBOOK_FIRST_LEVEL_TRACK)
					tracknum = n_tracks - (REDBOOK_FIRST_LEVEL_TRACK - tracknum) + 1;
				else if (tracknum > n_tracks)
					tracknum = REDBOOK_FIRST_LEVEL_TRACK + (tracknum - n_tracks) - 1;
			}

			Song_playing = -1;
			if (RBAEnabled() && (tracknum <= n_tracks))
			{
				if (RBAPlayTracks(tracknum, !songs_haved1_cd()?n_tracks:tracknum, songs_haved1_cd() ? redbook_repeat_func : redbook_first_song_func))
				{
					Song_playing = songnum + SONG_FIRST_LEVEL_SONG;
					Redbook_playing = tracknum;
				}
			}
			break;
		}
#ifdef USE_SDLMIXER
		case MUSIC_TYPE_CUSTOM:
		{
			if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND)
				GameCfg.CMLevelMusicTrack[0] = d_rand() % GameCfg.CMLevelMusicTrack[1]; // simply a random selection - no check if this song has already been played. But that's how I roll!
			else if (!offset)
			{
				if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)
				{
					static int last_songnum = -1;

					if (Song_playing >= SONG_FIRST_LEVEL_SONG)
						return Song_playing;

					// As soon as we start a new level, go to next track
					if (last_songnum != -1 && songnum != last_songnum)
						((GameCfg.CMLevelMusicTrack[0]+1>=GameCfg.CMLevelMusicTrack[1])?GameCfg.CMLevelMusicTrack[0]=0:GameCfg.CMLevelMusicTrack[0]++);
					last_songnum = songnum;
				}
				else if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL)
					GameCfg.CMLevelMusicTrack[0] = (songnum % GameCfg.CMLevelMusicTrack[1]);
			}
			else
			{
				GameCfg.CMLevelMusicTrack[0] += offset;
				if (GameCfg.CMLevelMusicTrack[0] < 0)
					GameCfg.CMLevelMusicTrack[0] = GameCfg.CMLevelMusicTrack[1] + GameCfg.CMLevelMusicTrack[0];
				if (GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
					GameCfg.CMLevelMusicTrack[0] = GameCfg.CMLevelMusicTrack[0] - GameCfg.CMLevelMusicTrack[1];
			}

			Song_playing = -1;
			if (jukebox_play())
				Song_playing = songnum + SONG_FIRST_LEVEL_SONG;

			break;
		}
#endif
		default:
			Song_playing = -1;
			break;
	}

	// If we couldn't play the song, most likely because it wasn't specified, play no music.
	if (Song_playing == -1)
		songs_stop_all();
	
	return Song_playing;
}

// check which song is playing, or -1 if not playing anything
int songs_is_playing()
{
	return Song_playing;
}

