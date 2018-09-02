/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * Header file for music playback through SDL_mixer
 *
 *  -- MD2211 (2006-04-24)
 */

#pragma once

#ifdef __cplusplus
namespace dcx {

int mix_play_music(const char *, int);
int mix_play_file(const char *, int, void (*)());
void mix_set_music_volume(int);
void mix_stop_music();
void mix_pause_music();
void mix_resume_music();
void mix_pause_resume_music();
void mix_free_music();

}
#endif
