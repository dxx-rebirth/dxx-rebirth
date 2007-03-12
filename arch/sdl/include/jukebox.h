#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

void jukebox_load();
void jukebox_play();
void jukebox_stop();
void jukebox_stop_hook();
void jukebox_next();
void jukebox_prev();
char *jukebox_current();
int jukebox_is_loaded();
int jukebox_is_playing();
void jukebox_list();

#endif
