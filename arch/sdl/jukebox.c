/*
 * DXX Rebirth "jukebox" code
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <sys/types.h>
#include <sys/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.h"
#include "dl_list.h"
#include "hudmsg.h"
#include "digi_mixer_music.h"
#include "jukebox.h"

#define JUKEBOX_ARG "-jukebox"
#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

static int jukebox_loaded = 0;
static int jukebox_playing = 0;
static char *jukebox_path;
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

# if !(defined(__APPLE__) && defined(__MACH__))		// this is why scandir should be part of POSIX. Grrrr.
# define DIRENT_PTR const struct dirent *
#else
# define DIRENT_PTR struct dirent *
#endif

/* Filter for scandir(); selects MP3 and OGG, files, rejects the rest */
int file_select_all(DIRENT_PTR entry) {
	char *fn = (char *) entry->d_name;
	char *ext = strrchr(fn, '.');
	int ext_ok = (ext != NULL && (!strcmp(ext, ".mp3") || !strcmp(ext, ".ogg")));

        return strcmp(fn, ".") && strcmp(entry->d_name, "..") && ext_ok;
		
}

/* Loads music file names from a given directory */
void jukebox_load() {
        int count, i;
        struct dirent **files;
        int (*file_select)(DIRENT_PTR) = file_select_all;

	if (!jukebox_loaded) {
		if (GameArg.SndJukebox) {
			jukebox_path = GameArg.SndJukebox;

			JukeboxSongs = dl_init();
			count = scandir(jukebox_path, &files, file_select, alphasort);

			if (count > 0) {
				printf("Jukebox: %d music file(s) found in %s\n", count, jukebox_path);
				for (i=0; i<count; i++)	dl_add(JukeboxSongs, files[i]->d_name);
				jukebox_loaded = 1;
			}
			else { printf("Jukebox music could not be found!\n"); }
		}
	}
	else { printf("Jukebox already loaded\n"); }
}

void jukebox_play() {
	char *music_filename;

	if (!jukebox_loaded) return;
	music_filename = (char *) JukeboxSongs->current->data;

	mix_play_file(jukebox_path, music_filename, 0);

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
	if (!jukebox_loaded) return;
	mix_stop_music();
	hud_message(MSGC_GAME_FEEDBACK, JUKEBOX_HUDMSG_STOPPED);
	jukebox_playing = 0;
}

void jukebox_stop_hook() {
	if (!jukebox_loaded) return;
	if (jukebox_playing)	jukebox_next();
}

void jukebox_next() {
	if (!jukebox_loaded) return;
	select_next_song(JukeboxSongs);
	if (jukebox_playing) jukebox_play();
}

void jukebox_prev() {
	if (!jukebox_loaded) return;
	select_prev_song(JukeboxSongs);
	if (jukebox_playing) jukebox_play();
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
		printf("* No songs have been found\n");
	}
	else {
		for (curr = JukeboxSongs->first; curr != NULL; curr = curr->next) {
			printf("* %s\n", (char *) curr->data);
		}
	}
}
