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
#ifdef USE_SDLMIXER
#include "digi_mixer_music.h"
#include "jukebox.h"
#endif
#include "config.h"
#include "timer.h"

song_info Songs[MAX_NUM_SONGS];
int Songs_initialized = 0;

int Num_songs;

static int Song_playing = 0; // 0 if no song playing, else the Descent song number
static int Redbook_playing = 0; // Redbook track num differs from Song num. Store here so we know with which track we deal with.
// NOTE: Custom music song number is stored in GameCfg.CMLevelMusicTrack[0]

#define NumLevelSongs (Num_songs - SONG_FIRST_LEVEL_SONG)

#define EXTMUSIC_VOLUME_SCALE	(255)

//takes volume in range 0..8
void songs_set_volume(int volume)
{
#ifdef _WIN32
	if (GameArg.SndDisableSdlMixer)
		digi_win32_set_midi_volume((volume*128)/8);
#endif
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
	{
		RBASetVolume(0);
		RBASetVolume(volume*EXTMUSIC_VOLUME_SCALE/8);
	}
#ifdef USE_SDLMIXER
	mix_set_music_volume(volume*EXTMUSIC_VOLUME_SCALE/8);
#endif
}

void songs_init()
{
	int i = 0;
	char inputline[80+1];
	CFILE * fp = NULL;
	char sng_file[PATH_MAX];

	memset(Songs, '\0', sizeof(Songs));

	memset(sng_file, '\0', sizeof(sng_file));
	if (Current_mission != NULL)
	{
		snprintf(sng_file, strlen(Current_mission_filename)+5, "%s.sng", Current_mission_filename);
		fp = cfopen(sng_file, "rb");
	}

	if (fp == NULL)
		fp = cfopen( "descent.sng", "rb" );

	if ( fp == NULL ) // No descent.sng available. Define a default song-set
	{
		for (i = 0; i < MAX_NUM_SONGS; i++) {
			strcpy(Songs[i].melodic_bank_file, "melodic.bnk");
			strcpy(Songs[i].drum_bank_file, "drum.bnk");
			if (i >= SONG_FIRST_LEVEL_SONG)
			{
				sprintf(Songs[i].filename, "game%02d.hmp", i - SONG_FIRST_LEVEL_SONG + 1);
				if (!cfexist(Songs[i].filename))
					sprintf(Songs[i].filename, "game%d.hmp", i - SONG_FIRST_LEVEL_SONG);
				if (!cfexist(Songs[i].filename))
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

				if (strrchr(Songs[i].filename, '.'))
					if (!stricmp(strrchr(Songs[i].filename, '.'), ".hmp") ||
						!stricmp(strrchr(Songs[i].filename, '.'), ".mp3") ||
						!stricmp(strrchr(Songs[i].filename, '.'), ".ogg") ||
						!stricmp(strrchr(Songs[i].filename, '.'), ".aif") ||
						!stricmp(strrchr(Songs[i].filename, '.'), ".mid")
						)
						i++;
			}
		}
	}

	Num_songs = i;
	Songs_initialized = 1;
	if (fp != NULL)	cfclose(fp);

	// Now each song will get it's own number which will serve custom music (and maybe others) as track number
	if (Num_songs > 0)
	{
		int i = 0, j = 0, c = 0;

		for (i = 0; i < Num_songs; i++)
		{
			Songs[i].id = -1;
			for (j = 0; j < i; j++)
				if (stricmp(Songs[i].filename, Songs[j].filename) == 0)
					Songs[i].id = Songs[j].id;

			if (Songs[i].id == -1)
				Songs[i].id = c++;
		}
	}

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
#ifdef _WIN32
	if (GameArg.SndDisableSdlMixer)
		digi_win32_stop_current_song();	// Stop midi song, if playing
#endif
	RBAStop();
//	RBAExit();
#ifdef USE_SDLMIXER
	mix_stop_music();
	jukebox_unload();
#endif

	Song_playing = 0;
	Songs_initialized = 0;
}

//stop any songs - builtin, redbook or jukebox - that are currently playing
void songs_stop_all(void)
{
#ifdef _WIN32
	if (GameArg.SndDisableSdlMixer)
		digi_win32_stop_current_song();	// Stop midi song, if playing
#endif
	RBAStop();
#ifdef USE_SDLMIXER
	mix_stop_music();
#endif

	Song_playing = 0;
}

void songs_pause(void)
{
	if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
		RBAPause();
#ifdef USE_SDLMIXER
	mix_pause_music();
#endif
}

void songs_resume(void)
{
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

void redbook_repeat_func()
{
	stop_time();
	RBAPlayTracks(Redbook_playing, 0, redbook_repeat_func);
	start_time();
}

int songs_play_song( int songnum, int repeat )
{
	songs_init();

	//stop any music already playing

	songs_stop_all();

	switch (GameCfg.MusicType)
	{
		case MUSIC_TYPE_BUILTIN:
		{
#ifdef _WIN32
			if (GameArg.SndDisableSdlMixer)
			{
				if (digi_win32_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat ))
				{
					Song_playing = songnum;
				}
			}
			else
#endif
#ifdef USE_SDLMIXER
			{
				if (mix_play_file(Songs[songnum].filename, repeat, NULL))
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

			if ((songnum == SONG_TITLE) && (REDBOOK_TITLE_TRACK <= num_tracks))
			{
				if (RBAPlayTracks(REDBOOK_TITLE_TRACK, 0, repeat ? redbook_repeat_func : NULL))
				{
					Redbook_playing = REDBOOK_TITLE_TRACK;
					Song_playing = songnum;
				}
			}
			else if ((songnum == SONG_CREDITS) && (REDBOOK_CREDITS_TRACK <= num_tracks))
			{
				if (RBAPlayTracks(REDBOOK_CREDITS_TRACK, 0, repeat ? redbook_repeat_func : NULL))
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
			if (mix_play_file(GameCfg.CMMiscMusic[songnum], repeat, NULL))
				Song_playing = songnum;
			break;
		}
#endif
		default:
			Song_playing = 0;
			break;
	}

	return Song_playing;
}

void redbook_first_song_func()
{
	stop_time();
	songs_play_level_song(1, 0);
	start_time();
}

// play track given by levelnum (depending on the music type and it's playing behaviour) or increment/decrement current track number via offset value
int songs_play_level_song( int levelnum, int offset )
{
	int songnum;

	Assert( levelnum != 0 );

	// Track changing not allowed for builtin music
	if (offset && GameCfg.MusicType == MUSIC_TYPE_BUILTIN)
		return Song_playing;

	songs_init();

	songs_stop_all();

	songnum = (levelnum>0)?(levelnum-1):(-levelnum);

	switch (GameCfg.MusicType)
	{
		case MUSIC_TYPE_BUILTIN:
		{
			if (NumLevelSongs > 0)
			{
				songnum = SONG_FIRST_LEVEL_SONG + (songnum % NumLevelSongs);
#ifdef _WIN32
				if (GameArg.SndDisableSdlMixer)
				{
					if (digi_win32_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1 ))
					{
						Song_playing = songnum;
					}
				}
				else
#endif
#ifdef USE_SDLMIXER
				{
					if (mix_play_file(Songs[songnum].filename, 1, NULL))
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
				tracknum = REDBOOK_FIRST_LEVEL_TRACK + ((n_tracks+1<=REDBOOK_FIRST_LEVEL_TRACK) ? 0 : (songnum % (n_tracks-REDBOOK_FIRST_LEVEL_TRACK+1)));
			else
			{
				tracknum = Redbook_playing+offset;
				if (tracknum < REDBOOK_FIRST_LEVEL_TRACK)
					tracknum = n_tracks - (REDBOOK_FIRST_LEVEL_TRACK - tracknum) + 1;
				else if (tracknum > n_tracks)
					tracknum = REDBOOK_FIRST_LEVEL_TRACK + (tracknum - n_tracks) - 1;
			}


			if (RBAEnabled() && (tracknum <= n_tracks))
			{
				if (RBAPlayTracks(tracknum, n_tracks, redbook_first_song_func))
				{
					Song_playing = songnum;
					Redbook_playing = tracknum;
				}
			}
			break;
		}
#ifdef USE_SDLMIXER
		case MUSIC_TYPE_CUSTOM:
		{
			if (!offset)
			{
				if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)
				{
					static int last_songnum = -1;

					// As soon as we start a new level, go to next track
					if (last_songnum != -1 && songnum != last_songnum)
						((GameCfg.CMLevelMusicTrack[0]+1>=GameCfg.CMLevelMusicTrack[1])?GameCfg.CMLevelMusicTrack[0]=0:GameCfg.CMLevelMusicTrack[0]++);
					last_songnum = songnum;
				}
				else if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVELDEP)
					GameCfg.CMLevelMusicTrack[0] = ((Songs[songnum+SONG_FIRST_LEVEL_SONG].id - SONG_FIRST_LEVEL_SONG) % (GameCfg.CMLevelMusicTrack[1]));
				else if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVELALPHA)
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

			if (jukebox_play())
				Song_playing = songnum;

			break;
		}
#endif
		default:
			Song_playing = 0;
			break;
	}

	return Song_playing;
}

// check which song is playing
int songs_is_playing()
{
	return Song_playing;
}

