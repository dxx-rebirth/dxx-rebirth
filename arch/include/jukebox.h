#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

void jukebox_unload();
void jukebox_load();
int jukebox_play_tracks(int first, int last, void (*hook_finished)(void));
void jukebox_stop();
void jukebox_pause();
int jukebox_resume();
int jukebox_pause_resume();
char *jukebox_current();
int jukebox_is_loaded();
int jukebox_is_playing();
int jukebox_numtracks();
void jukebox_set_volume(int);
void jukebox_list();

#endif
