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

static int jukebox_playing = -1;
static int jukebox_numsongs = 0;
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
void jukebox_load() {
	int count;
	char *music_exts[] = { ".mp3", ".ogg", ".wav", ".aif", NULL };
	static char curpath[PATH_MAX+1];

	if (memcmp(curpath,GameCfg.JukeboxPath,PATH_MAX) || !GameCfg.JukeboxOn)
	{
		PHYSFS_removeFromSearchPath(curpath);
		jukebox_unload();
	}

	if (JukeboxSongs)
		return;

	if (GameCfg.JukeboxOn) {
		// Adding as a mount point is an option, but wouldn't work for older versions of PhysicsFS
		PHYSFS_addToSearchPath(GameCfg.JukeboxPath, 1);

		JukeboxSongs = PHYSFSX_findFiles("", music_exts);

		if (JukeboxSongs != NULL) {
			for (count = 0; JukeboxSongs[count]!=NULL; count++) {}
			if (count)
			{
				con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s\n", count, GameCfg.JukeboxPath);
				memcpy(curpath,GameCfg.JukeboxPath,PATH_MAX);
				jukebox_numsongs = count;
			}
			else { con_printf(CON_DEBUG,"Jukebox music could not be found!\n"); }
		}
		else
			{ Int3(); }	// should at least find a directory in some search path, otherwise how did D2X load?
	}
}

void jukebox_play(int loop) {
	char *music_filename;

	if (!JukeboxSongs) return;

	if (jukebox_playing == -1)
		jukebox_playing = 0;	// For now - will allow track numbers as arguments in future
	music_filename = JukeboxSongs[jukebox_playing];

	mix_play_file(music_filename, loop);

	// Formatting a pretty message
	if (strlen(music_filename) >= MUSIC_HUDMSG_MAXLEN) {
		strncpy(hud_msg_buf, music_filename, MUSIC_HUDMSG_MAXLEN);
		strcpy(hud_msg_buf+MUSIC_HUDMSG_MAXLEN, "...");
		hud_msg_buf[MUSIC_HUDMSG_MAXLEN+3] = '\0';
	} else {
		strcpy(hud_msg_buf, music_filename);
	}

	hud_message(MSGC_GAME_FEEDBACK, "%s %s", JUKEBOX_HUDMSG_PLAYING, hud_msg_buf);
}

void jukebox_stop() {
	if (!JukeboxSongs) return;	// since this function is also used for stopping MIDI
	mix_stop_music();
	if (jukebox_playing != -1)
		hud_message(MSGC_GAME_FEEDBACK, JUKEBOX_HUDMSG_STOPPED);
	jukebox_playing = -1;
}

void jukebox_pause_resume() {
	if (!JukeboxSongs) return;

	if (Mix_PausedMusic())
	{
		Mix_ResumeMusic();
		hud_message(MSGC_GAME_FEEDBACK, "Jukebox playback resumed");
	}
	else
	{
		Mix_PauseMusic();
		hud_message(MSGC_GAME_FEEDBACK, "Jukebox playback paused");
	}
}

void jukebox_hook_stop() {
	if (!JukeboxSongs) return;
}

void jukebox_hook_next() {
	if (!JukeboxSongs) return;
	if (jukebox_playing != -1)	jukebox_next();
}

void jukebox_next() {
	if (!JukeboxSongs || jukebox_playing == -1) return;

	jukebox_playing++;
	if (jukebox_playing == jukebox_numsongs)
		jukebox_playing = 0;
	jukebox_play(1);
}

void jukebox_prev() {
	if (!JukeboxSongs || jukebox_playing == -1) return;

	jukebox_playing--;
	if (jukebox_playing == -1)
		jukebox_playing = jukebox_numsongs - 1;
	jukebox_play(1);
}

char *jukebox_current() {
	return JukeboxSongs[jukebox_playing];
}

int jukebox_is_loaded() { return (JukeboxSongs != NULL); }
int jukebox_is_playing() { return jukebox_playing + 1; }

void jukebox_list() {
	int i;
	if (!JukeboxSongs) return;
	if (!(*JukeboxSongs)) {
		con_printf(CON_DEBUG,"* No songs have been found\n");
	}
	else {
		for (i = 0; i < jukebox_numsongs; i++)
			con_printf(CON_DEBUG,"* %s\n", JukeboxSongs[i]);
	}
}
