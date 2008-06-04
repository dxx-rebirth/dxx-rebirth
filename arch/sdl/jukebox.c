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

static int jukebox_loaded = 0;
static int jukebox_playing = 0;
static dl_list *JukeboxSongs;
char hud_msg_buf[MUSIC_HUDMSG_MAXLEN+4];


char *select_prev_song(dl_list *list) {
	char *ret;
	if (dl_is_empty(list)) {
		ret = NULL;
	}
	else {
		if (list->first == list->current) {
			list->current = list->last;
		}
		else {
			dl_backward(list);
		}
		ret = (char *) list->current->data;
	}
	return ret;
}

char *select_next_song(dl_list *list) {
	char *ret;
	if (dl_is_empty(list)) {
		ret = NULL;
	}
	else {
		if (list->last == list->current) {
			list->current = list->first;
		}
		else {
			dl_forward(list);
		}
		ret = (char *) list->current->data;
	}
	return ret;
}

void jukebox_unload()
{
	if (JukeboxSongs == NULL)
		return;

	while (JukeboxSongs->first!=NULL)
	{
		dl_remove(JukeboxSongs,JukeboxSongs->first);
	}
	d_free(JukeboxSongs);
	jukebox_loaded = 0;
}

/* Loads music file names from a given directory */
void jukebox_load() {
	int count = 0;
	char **files;
	char *music_exts[] = { ".mp3", ".ogg", ".wav", ".aif", NULL };
	static char curpath[PATH_MAX+1];

	if (memcmp(curpath,GameCfg.JukeboxPath,PATH_MAX) || !GameCfg.JukeboxOn)
	{
		PHYSFS_removeFromSearchPath(curpath);
		jukebox_unload();
	}

	if (jukebox_loaded || GameArg.SndNoSound)
		return;

	if (GameCfg.JukeboxOn) {
		// Adding as a mount point is an option, but wouldn't work for older versions of PhysicsFS
		PHYSFS_addToSearchPath(GameCfg.JukeboxPath, 1);

		JukeboxSongs = dl_init();
		files = PHYSFSX_findFiles("", music_exts);

		if (files != NULL && *files != NULL) {
			char **i;

			for (i=files; *i!=NULL; i++)
			{
				dl_add(JukeboxSongs, *i);
				count++;
			}
			if (count)
			{
				con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s\n", count, GameCfg.JukeboxPath);
				memcpy(curpath,GameCfg.JukeboxPath,PATH_MAX);
				jukebox_loaded = 1;
			}
			else { con_printf(CON_DEBUG,"Jukebox music could not be found!\n"); }
		}
		else
			{ Int3(); }	// should at least find a directory in some search path, otherwise how did D2X load?

		if (files != NULL)
			free(files);
	}
}

void jukebox_play(int loop) {
	char *music_filename;

	if (!jukebox_loaded) return;
	music_filename = (char *) JukeboxSongs->current->data;

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
	jukebox_playing = 1;
}

void jukebox_stop() {
	if (!jukebox_loaded) return;	// since this function is also used for stopping MIDI
	mix_stop_music();
	if (jukebox_playing)
		hud_message(MSGC_GAME_FEEDBACK, JUKEBOX_HUDMSG_STOPPED);
	jukebox_playing = 0;
}

void jukebox_pause_resume() {
	if (!jukebox_loaded) return;

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
	if (!jukebox_loaded) return;
}

void jukebox_hook_next() {
	if (!jukebox_loaded) return;
	if (jukebox_playing)	jukebox_next();
}

void jukebox_next() {
	if (!jukebox_loaded) return;
	select_next_song(JukeboxSongs);
	if (jukebox_playing) jukebox_play(1);
}

void jukebox_prev() {
	if (!jukebox_loaded) return;
	select_prev_song(JukeboxSongs);
	if (jukebox_playing) jukebox_play(1);
}

char *jukebox_current() {
	return JukeboxSongs->current->data;
}

int jukebox_is_loaded() { return jukebox_loaded; }
int jukebox_is_playing() { return jukebox_playing; }

void jukebox_list() {
	dl_item *curr;
	if (!jukebox_loaded) return;
	if (dl_is_empty(JukeboxSongs)) {
		con_printf(CON_DEBUG,"* No songs have been found\n");
	}
	else {
		for (curr = JukeboxSongs->first; curr != NULL; curr = curr->next) {
			con_printf(CON_DEBUG,"* %s\n", (char *) curr->data);
		}
	}
}
