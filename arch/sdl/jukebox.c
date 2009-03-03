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
static int jukebox_play_last = -1;
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

void jukebox_hook_next();
void (*jukebox_hook_finished)() = NULL;

int jukebox_play_tracks(int first, int last, void (*hook_finished)(void)) {
	char *music_filename;

	if (!JukeboxSongs) return 0;

	jukebox_playing = first - 1;	// start with the first track, then play until last track
	if (jukebox_playing < 0)
		jukebox_playing = 0;

	if ((last < first) || (last >= jukebox_numsongs))
		jukebox_play_last = jukebox_numsongs - 1;
	else
		jukebox_play_last = last - 1;

	music_filename = JukeboxSongs[jukebox_playing];

	jukebox_hook_finished = hook_finished ? hook_finished : mix_free_music;
	mix_play_file(music_filename, 0, jukebox_hook_next);	// have our function handle looping

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

void jukebox_stop() {
	if (!JukeboxSongs) return;	// since this function is also used for stopping MIDI
	mix_stop_music();
	if (jukebox_playing != -1)
		hud_message(MSGC_GAME_FEEDBACK, JUKEBOX_HUDMSG_STOPPED);
	jukebox_playing = -1;
}

// need this mess because pausing the game is meant to pause the music
void jukebox_pause() {
	if (!JukeboxSongs) return;

	Mix_PauseMusic();
}

int jukebox_resume() {
	if (!JukeboxSongs) return -1;
	
	Mix_ResumeMusic();
	return 1;
}

int jukebox_pause_resume() {
	if (!JukeboxSongs) return 0;

	if (Mix_PausedMusic())
	{
		Mix_ResumeMusic();
		hud_message(MSGC_GAME_FEEDBACK, "Jukebox playback resumed");
	}
	else if (Mix_PlayingMusic())
	{
		Mix_PauseMusic();
		hud_message(MSGC_GAME_FEEDBACK, "Jukebox playback paused");
	}
	else
		return 0;

	return 1;
}

void jukebox_hook_next() {
	if (!JukeboxSongs || jukebox_playing == -1) return;
	
	jukebox_playing++;
	if ((jukebox_playing > jukebox_play_last) && jukebox_hook_finished)
		jukebox_hook_finished();
	else if (jukebox_playing <= jukebox_play_last)
		jukebox_play_tracks(jukebox_playing + 1, jukebox_play_last + 1, jukebox_hook_finished);
}

char *jukebox_current() {
	return JukeboxSongs[jukebox_playing];
}

int jukebox_is_loaded() { return (JukeboxSongs != NULL); }
int jukebox_is_playing() { return jukebox_playing + 1; }
int jukebox_numtracks() { return jukebox_numsongs; }

void jukebox_set_volume(int vol)
{ 
	mix_set_music_volume(vol);
}

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
