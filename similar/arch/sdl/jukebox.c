/*
 * DXX Rebirth "jukebox" code
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "physfsx.h"
#include "args.h"
#include "hudmsg.h"
#include "songs.h"
#include "jukebox.h"
#include "dxxerror.h"
#include "console.h"
#include "config.h"

#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

typedef struct jukebox_songs
{
	char **list;	// the actual list
	char *list_buf;	// buffer containing song file path text
	int num_songs;	// number of jukebox songs
	int max_songs;	// maximum number of pointers that 'list' can hold, i.e. size of list / size of one pointer
	int max_buf;	// size of list_buf
} jukebox_songs;

static jukebox_songs JukeboxSongs = { NULL, NULL, 0, 0, 0 };
char hud_msg_buf[MUSIC_HUDMSG_MAXLEN+4];


void jukebox_unload()
{
	if (JukeboxSongs.list_buf)
	{
		d_free(JukeboxSongs.list_buf);
		
		if (JukeboxSongs.list)
			d_free(JukeboxSongs.list);
	}
	else if (JukeboxSongs.list)
	{
		PHYSFS_freeList(JukeboxSongs.list);
		JukeboxSongs.list = NULL;
	}

	JukeboxSongs.num_songs = JukeboxSongs.max_songs = JukeboxSongs.max_buf = 0;
}

const char *const jukebox_exts[7] = { SONG_EXT_HMP, SONG_EXT_MID, SONG_EXT_OGG, SONG_EXT_FLAC, SONG_EXT_MP3, NULL };

int read_m3u(void)
{
	FILE *fp;
	int length;
	char *buf;
	char *abspath;
	
	MALLOC(abspath, char, PATH_MAX);
	if (!abspath)
		return 0;
	
	if (PHYSFSX_exists(GameCfg.CMLevelMusicPath,0)) // it's a child of Sharepath, build full path
		PHYSFSX_getRealPath(GameCfg.CMLevelMusicPath, abspath);
	else
	{
		strncpy(abspath, GameCfg.CMLevelMusicPath, PATH_MAX - 1);
		abspath[PATH_MAX - 1] = '\0';
	}
	
	fp = fopen(abspath, "rb");
	d_free(abspath);
	if (!fp)
		return 0;
	
	fseek( fp, -1, SEEK_END );
	length = ftell(fp) + 1;
	MALLOC(JukeboxSongs.list_buf, char, length + 1);
	if (!JukeboxSongs.list_buf)
	{
		fclose(fp);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	if (!fread(JukeboxSongs.list_buf, length, 1, fp))
	{
		d_free(JukeboxSongs.list_buf);
		fclose(fp);
		return 0;
	}

	fclose(fp);		// Finished with it

	// The growing string list is allocated last, hopefully reducing memory fragmentation when it grows
	MALLOC(JukeboxSongs.list, char *, 1024);
	if (!JukeboxSongs.list)
	{
		d_free(JukeboxSongs.list_buf);
		return 0;
	}
	JukeboxSongs.max_songs = 1024;

	JukeboxSongs.list_buf[length] = '\0';	// make sure the last string is terminated
	JukeboxSongs.max_buf = length + 1;
	buf = JukeboxSongs.list_buf;
	
	while (buf < JukeboxSongs.list_buf + length - 1)
	{
		while (*buf == 0 || *buf == 10 || *buf == 13)	// find new line - support DOS, Unix and Mac line endings
			buf++;
		
		if (*buf != '#')	// ignore comments / extra info
		{
			if (JukeboxSongs.num_songs >= JukeboxSongs.max_songs)
			{
				char **new_list = d_realloc(JukeboxSongs.list, JukeboxSongs.max_buf*sizeof(char *)*MEM_K);
				if (new_list == NULL)
					break;
				JukeboxSongs.max_buf *= MEM_K;
				JukeboxSongs.list = new_list;
			}
			
			JukeboxSongs.list[JukeboxSongs.num_songs++] = buf;
		}
		
		while (*buf != 0 && *buf != 10 && *buf != 13)	// find end of line
			buf++;
		
		*buf = 0;
	}
	
	return 1;
}

/* Loads music file names from a given directory or M3U playlist */
void jukebox_load()
{
	static int jukebox_init = 1;

	// initialize JukeboxSongs structure once per runtime
	if (jukebox_init)
	{
		JukeboxSongs.list = NULL;
		JukeboxSongs.num_songs = JukeboxSongs.max_songs = JukeboxSongs.max_buf = 0;
		jukebox_init = 0;
	}

	jukebox_unload();

	// Check if it's an M3U file
	if (!d_stricmp(&GameCfg.CMLevelMusicPath[strlen(GameCfg.CMLevelMusicPath) - 4], ".m3u"))
		read_m3u();
	else	// a directory
	{
		int new_path = 0;
		char *p;
		const char *sep = PHYSFS_getDirSeparator();
		int i;

		// stick a separator on the end if necessary.
		if (strlen(GameCfg.CMLevelMusicPath) >= strlen(sep))
		{
			p = GameCfg.CMLevelMusicPath + strlen(GameCfg.CMLevelMusicPath) - strlen(sep);
			if (strcmp(p, sep))
				strncat(GameCfg.CMLevelMusicPath, sep, PATH_MAX - 1 - strlen(GameCfg.CMLevelMusicPath));
		}

		// Read directory using PhysicsFS
		if (PHYSFS_isDirectory(GameCfg.CMLevelMusicPath))	// find files in relative directory
			JukeboxSongs.list = PHYSFSX_findFiles(GameCfg.CMLevelMusicPath, jukebox_exts);
		else
		{
			new_path = PHYSFSX_isNewPath(GameCfg.CMLevelMusicPath);
			PHYSFS_addToSearchPath(GameCfg.CMLevelMusicPath, 0);

			// as mountpoints are no option (yet), make sure only files originating from GameCfg.CMLevelMusicPath are aded to the list.
			JukeboxSongs.list = PHYSFSX_findabsoluteFiles("", GameCfg.CMLevelMusicPath, jukebox_exts);
		}

		if (!JukeboxSongs.list)
		{
			if (new_path)
				PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath);
			return;
		}
		
		for (i = 0; JukeboxSongs.list[i]; i++) {}
		JukeboxSongs.num_songs = i;

		if (new_path)
			PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath);
	}

	if (JukeboxSongs.num_songs)
	{
		con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s\n", JukeboxSongs.num_songs, GameCfg.CMLevelMusicPath);
		if (GameCfg.CMLevelMusicTrack[1] != JukeboxSongs.num_songs)
		{
			GameCfg.CMLevelMusicTrack[1] = JukeboxSongs.num_songs;
			GameCfg.CMLevelMusicTrack[0] = 0; // number of songs changed so start from beginning.
		}
	}
	else
	{
		GameCfg.CMLevelMusicTrack[0] = -1;
		GameCfg.CMLevelMusicTrack[1] = -1;
		con_printf(CON_DEBUG,"Jukebox music could not be found!\n");
	}
}

// To proceed tru our playlist. Usually used for continous play, but can loop as well.
void jukebox_hook_next()
{
	if (!JukeboxSongs.list || GameCfg.CMLevelMusicTrack[0] == -1) return;

	if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND)
		GameCfg.CMLevelMusicTrack[0] = d_rand() % GameCfg.CMLevelMusicTrack[1]; // simply a random selection - no check if this song has already been played. But that's how I roll!
	else
		GameCfg.CMLevelMusicTrack[0]++;
	if (GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		GameCfg.CMLevelMusicTrack[0] = 0;

	jukebox_play();
}

// Play tracks from Jukebox directory. Play track specified in GameCfg.CMLevelMusicTrack[0] and loop depending on GameCfg.CMLevelMusicPlayOrder
int jukebox_play()
{
	char *music_filename, *full_filename;
	unsigned long size_full_filename = 0;

	if (!JukeboxSongs.list)
		return 0;

	if (GameCfg.CMLevelMusicTrack[0] < 0 || GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		return 0;

	music_filename = JukeboxSongs.list[GameCfg.CMLevelMusicTrack[0]];
	if (!music_filename)
		return 0;

	size_full_filename = strlen(GameCfg.CMLevelMusicPath)+strlen(music_filename)+1;
	CALLOC(full_filename, char, size_full_filename);
	if (!d_stricmp(&GameCfg.CMLevelMusicPath[strlen(GameCfg.CMLevelMusicPath) - 4], ".m3u"))	// if it's from an M3U playlist
		strcpy(full_filename, music_filename);
	else											// if it's from a specified path
		snprintf(full_filename, size_full_filename, "%s%s", GameCfg.CMLevelMusicPath, music_filename);

	if (!songs_play_file(full_filename, ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL)?1:0), ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL)?NULL:jukebox_hook_next)))
	{
		d_free(full_filename);
		return 0;	// whoops, got an error
	}

	// Formatting a pretty message
	if (strlen(music_filename) >= MUSIC_HUDMSG_MAXLEN) {
		strcpy(hud_msg_buf, "...");
		strncat(hud_msg_buf, &music_filename[strlen(music_filename) - MUSIC_HUDMSG_MAXLEN], MUSIC_HUDMSG_MAXLEN);
		hud_msg_buf[MUSIC_HUDMSG_MAXLEN+3] = '\0';
	} else {
		strcpy(hud_msg_buf, music_filename);
	}

	HUD_init_message(HM_DEFAULT, "%s %s", JUKEBOX_HUDMSG_PLAYING, hud_msg_buf);

	d_free(full_filename);

	return 1;
}

char *jukebox_current() {
	return JukeboxSongs.list[GameCfg.CMLevelMusicTrack[0]];
}

int jukebox_is_loaded() { return (JukeboxSongs.list != NULL); }
int jukebox_is_playing() { return GameCfg.CMLevelMusicTrack[0] + 1; }
int jukebox_numtracks() { return GameCfg.CMLevelMusicTrack[1]; }

void jukebox_list() {
	int i;
	if (!JukeboxSongs.list) return;
	if (!(*JukeboxSongs.list)) {
		con_printf(CON_DEBUG,"* No songs have been found\n");
	}
	else {
		for (i = 0; i < GameCfg.CMLevelMusicTrack[1]; i++)
			con_printf(CON_DEBUG,"* %s\n", JukeboxSongs.list[i]);
	}
}
