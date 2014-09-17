/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes for state saving functions.
 *
 */


#ifndef _STATE_H
#define _STATE_H

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
#define SECRETB_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgb")
#define SECRETC_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgc")
#endif

#ifdef __cplusplus

int state_save_all(int secret_save, const char *filename_override, int blind_save);
int state_restore_all(int in_game, int secret_restore, const char *filename_override, int blind_save);

extern unsigned state_game_id;
extern int state_quick_item;

int state_save_all_sub(const char *filename, const char *desc);
int state_restore_all_sub(const char *filename, int secret_restore);

int state_get_save_file(char *fname, char * dsc, int blind_save);
int state_get_restore_file(char *fname, int blind_save);
int state_get_game_id(const char *filename);
#if defined(DXX_BUILD_DESCENT_I)
static inline void set_pos_from_return_segment(void)
{
}
#elif defined(DXX_BUILD_DESCENT_II)
void set_pos_from_return_segment(void);
#endif

#endif

#endif /* _STATE_H */
