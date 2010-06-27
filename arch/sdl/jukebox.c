/*
 * DXX Rebirth "jukebox" code
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <stdlib.h>
#include <string.h>
#include <SDL/SDL_mixer.h>
#include "physfsx.h"
#include "args.h"
#include "dl_list.h"
#include "hudmsg.h"
#include "digi_mixer_music.h"
#include "jukebox.h"
#include "error.h"
#include "console.h"
#include "config.h"

#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

static char **JukeboxSongs = NULL;
char hud_msg_buf[MUSIC_HUDMSG_MAXLEN+4];


void jukebox_unload()
{
	int i;

	if (JukeboxSongs == NULL)
		return;

	for (i = 0; JukeboxSongs[i]!=NULL; i++)
	{
		free(JukeboxSongs[i]);
	}
	free(JukeboxSongs);
	JukeboxSongs = NULL;
}

/* Loads music file names from a given directory */
void jukebox_load()
{
	int count;
	char *music_exts[] = { ".mp3", ".ogg", ".wav", ".aif", ".mid", NULL };
	static char curpath[PATH_MAX+1];

	if (memcmp(curpath,GameCfg.CMLevelMusicPath,PATH_MAX) || (GameCfg.MusicType != MUSIC_TYPE_CUSTOM))
	{
		PHYSFS_removeFromSearchPath(curpath);
		jukebox_unload();
	}

	if (JukeboxSongs)
		return;

	if (GameCfg.MusicType == MUSIC_TYPE_CUSTOM)
	{
		char *p;
		const char *sep = PHYSFS_getDirSeparator();

		// build path properly.
		if (strlen(GameCfg.CMLevelMusicPath) >= strlen(sep))
		{
			char abspath[PATH_MAX+1];

			p = GameCfg.CMLevelMusicPath + strlen(GameCfg.CMLevelMusicPath) - strlen(sep);
			if (strcmp(p, sep))
				strncat(GameCfg.CMLevelMusicPath, sep, PATH_MAX - 1 - strlen(GameCfg.CMLevelMusicPath));

			if (PHYSFS_isDirectory(GameCfg.CMLevelMusicPath)) // it's a child of Sharepath, build full path
			{
				PHYSFSX_getRealPath(GameCfg.CMLevelMusicPath,abspath);
				snprintf(GameCfg.CMLevelMusicPath,sizeof(char)*PATH_MAX,abspath);
			}
		}

		PHYSFS_addToSearchPath(GameCfg.CMLevelMusicPath, 0);
		// as mountpoints are no option (yet), make sure only files originating from GameCfg.CMLevelMusicPath are aded to the list.
		JukeboxSongs = PHYSFSX_findabsoluteFiles("", GameCfg.CMLevelMusicPath, music_exts);

		if (JukeboxSongs != NULL)
		{
			for (count = 0; JukeboxSongs[count]!=NULL; count++) {}
			if (count)
			{
				con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s\n", count, GameCfg.CMLevelMusicPath);
				memcpy(curpath,GameCfg.CMLevelMusicPath,sizeof(char)*PATH_MAX);
				if (GameCfg.CMLevelMusicTrack[1] != count)
				{
					GameCfg.CMLevelMusicTrack[1] = count;
					GameCfg.CMLevelMusicTrack[0] = 0; // number of songs changed so start from beginning.
				}
			}
			else
			{
				GameCfg.CMLevelMusicTrack[0] = -1;
				GameCfg.CMLevelMusicTrack[1] = -1;
				PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath);
				con_printf(CON_DEBUG,"Jukebox music could not be found!\n");
			}
		}
	}
}

// To proceed tru our playlist. Usually used for continous play, but can loop as well.
void jukebox_hook_next()
{
	if (!JukeboxSongs || GameCfg.CMLevelMusicTrack[0] == -1) return;

	GameCfg.CMLevelMusicTrack[0]++;
	if (GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		GameCfg.CMLevelMusicTrack[0] = 0;

	jukebox_play();
}

// Play tracks from Jukebox directory. Play track specified in GameCfg.CMLevelMusicTrack[0] and loop depending on GameCfg.CMLevelMusicPlayOrder
int jukebox_play()
{
	char *music_filename;

	if (!JukeboxSongs)
		return 0;

	if (GameCfg.CMLevelMusicTrack[0] < 0 || GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		return 0;

	music_filename = JukeboxSongs[GameCfg.CMLevelMusicTrack[0]];
	if (!music_filename)
		return 0;

	if (!mix_play_file(music_filename, ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)?0:1), ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)?jukebox_hook_next:NULL)))
		return 0;	// whoops, got an error

	// Formatting a pretty message
	if (strlen(music_filename) >= MUSIC_HUDMSG_MAXLEN) {
		strncpy(hud_msg_buf, music_filename, MUSIC_HUDMSG_MAXLEN);
		strcpy(hud_msg_buf+MUSIC_HUDMSG_MAXLEN, "...");
		hud_msg_buf[MUSIC_HUDMSG_MAXLEN+3] = '\0';
	} else {
		strcpy(hud_msg_buf, music_filename);
	}

	hud_message(MSGC_GAME_FEEDBACK, "%s %s", JUKEBOX_HUDMSG_PLAYING, hud_msg_buf);

	return 1;
}

char *jukebox_current() {
	return JukeboxSongs[GameCfg.CMLevelMusicTrack[0]];
}

int jukebox_is_loaded() { return (JukeboxSongs != NULL); }
int jukebox_is_playing() { return GameCfg.CMLevelMusicTrack[0] + 1; }
int jukebox_numtracks() { return GameCfg.CMLevelMusicTrack[1]; }

void jukebox_list() {
	int i;
	if (!JukeboxSongs) return;
	if (!(*JukeboxSongs)) {
		con_printf(CON_DEBUG,"* No songs have been found\n");
	}
	else {
		for (i = 0; i < GameCfg.CMLevelMusicTrack[1]; i++)
			con_printf(CON_DEBUG,"* %s\n", JukeboxSongs[i]);
	}
}
