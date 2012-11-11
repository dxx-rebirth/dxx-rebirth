#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

#ifdef __cplusplus
extern "C" {
#endif

extern const char *const jukebox_exts[7];

void jukebox_unload();
void jukebox_load();
int jukebox_play();
char *jukebox_current();
int jukebox_is_loaded();
int jukebox_is_playing();
int jukebox_numtracks();
void jukebox_list();

#ifdef __cplusplus
}
#endif

#endif
