/* cdplay.h by Victor Rachels for better management of cd player */

#ifndef _CDLIST_H
#define _CDLIST_H

int cd_playtrack(int trackno);
int cd_resume();

void cd_playprev();
void cd_playnext();
void cd_playtoggle();

void cd_stop();
void cd_cycle();

void cd_playlist_reset();
void cd_playlist_set(char *newlist);

void cd_init();
#endif
