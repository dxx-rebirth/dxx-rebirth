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

#include "dxxerror.h"
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
#include "args.h"

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

	if (fp == NULL) // try dxx-r.sng - a songfile specifically for dxx which level authors CAN use (dxx does not care if descent.sng contains MP3/OGG/etc. as well) besides the normal descent.sng containing files other versions of the game cannot play. this way a mission can contain a DOS-Descent compatible OST (hmp files) as well as a OST using MP3, OGG, etc.
		fp = PHYSFSX_openReadBuffered( "dxx-r.sng" );

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
			PHYSFSX_fgets(inputline, sizeof(inputline), fp );
			if ( strlen( inputline ) )
			{
				BIMSongs = d_realloc(BIMSongs, sizeof(bim_song_info)*(i+1));
				memset(BIMSongs[i].filename, '\0', sizeof(BIMSongs[i].filename));
				sscanf( inputline, "%15s", BIMSongs[i].filename );

				if (strrchr(BIMSongs[i].filename, '.'))
					if (!d_stricmp(strrchr(BIMSongs[i].filename, '.'), SONG_EXT_HMP)
#ifdef USE_SDLMIXER
						||
						!d_stricmp(strrchr(BIMSongs[i].filename, '.'), SONG_EXT_MID) ||
						!d_stricmp(strrchr(BIMSongs[i].filename, '.'), SONG_EXT_OGG) ||
						!d_stricmp(strrchr(BIMSongs[i].filename, '.'), SONG_EXT_FLAC) ||
						!d_stricmp(strrchr(BIMSongs[i].filename, '.'), SONG_EXT_MP3)
#endif
						)
						i++;
			}
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

#define REDBOOK_TITLE_TRACK         2
#define REDBOOK_CREDITS_TRACK       3
#define REDBOOK_FIRST_LEVEL_TRACK   (songs_haved2_cd() ? 4 : 1)

// songs_haved2_cd returns 1 if the descent 2 CD is in the drive and
// 0 otherwise
int songs_haved2_cd()
{
	int discid;

	if (GameCfg.OrigTrackOrder)
		return 1;

	if (!(GameCfg.MusicType == MUSIC_TYPE_REDBOOK))
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

void play_credits_track()
{
	stop_time();
	songs_play_song(SONG_CREDITS, 1);
	start_time();
}

// play a filename as music, depending on filename extension.
int songs_play_file(char *filename, int repeat, void (*hook_finished_track)())
{
	char *fptr = strrchr(filename, '.');

	songs_stop_all();
	
	if (fptr == NULL)
		return 0;

	if (!d_stricmp(fptr, SONG_EXT_HMP))
	{
#if defined(_WIN32)
		return digi_win32_play_midi_song( filename, repeat );
#elif defined(USE_SDLMIXER)
		return mix_play_file( filename, repeat, hook_finished_track );
#else
		return 0;
#endif
	}
#if defined(USE_SDLMIXER)
	else if ( !d_stricmp(fptr, SONG_EXT_MID) ||
			!d_stricmp(fptr, SONG_EXT_OGG) ||
			!d_stricmp(fptr, SONG_EXT_FLAC) ||
			!d_stricmp(fptr, SONG_EXT_MP3) )
	{
		return mix_play_file( filename, repeat, hook_finished_track );
	}
#endif
	return 0;
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
			if (songs_play_file(BIMSongs[songnum].filename, repeat, NULL))
				Song_playing = songnum;
			break;
		}
		case MUSIC_TYPE_REDBOOK:
		{
			int num_tracks = RBAGetNumberOfTracks();

			//Song_playing = -1;		// keep playing current music if chosen song is unavailable (e.g. SONG_ENDLEVEL)
			if ((songnum == SONG_TITLE) && (REDBOOK_TITLE_TRACK <= num_tracks))
			{
				if (RBAPlayTracks(REDBOOK_TITLE_TRACK, REDBOOK_TITLE_TRACK, repeat ? play_credits_track : NULL))
				{
					Redbook_playing = REDBOOK_TITLE_TRACK;
					Song_playing = songnum;
				}
			}
			else if ((songnum == SONG_CREDITS) && (REDBOOK_CREDITS_TRACK <= num_tracks))
			{
				if (RBAPlayTracks(REDBOOK_CREDITS_TRACK, REDBOOK_CREDITS_TRACK, repeat ? play_credits_track : NULL))
				{
					Redbook_playing = REDBOOK_CREDITS_TRACK;
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
			if (songs_play_file(GameCfg.CMMiscMusic[songnum],
							  // Play the credits track after the title track and loop the credits track if original CD track order was chosen
							  (songnum == SONG_TITLE && GameCfg.OrigTrackOrder) ? 0 : repeat,
							  (songnum == SONG_TITLE && GameCfg.OrigTrackOrder) ? play_credits_track : NULL))
				Song_playing = songnum;
			break;
		}
#endif
		default:
			Song_playing = -1;
			break;
	}

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
				if (songs_play_file(BIMSongs[songnum].filename, 1, NULL))
					Song_playing = songnum;
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

				tracknum = REDBOOK_FIRST_LEVEL_TRACK + ((n_tracks+1<=REDBOOK_FIRST_LEVEL_TRACK) ? 0 : (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK+1)));
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
				if (RBAPlayTracks(tracknum, n_tracks, redbook_first_song_func))
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

	return Song_playing;
}

// check which song is playing, or -1 if not playing anything
int songs_is_playing()
{
	return Song_playing;
}

