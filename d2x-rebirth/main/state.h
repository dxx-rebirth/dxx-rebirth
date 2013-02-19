/*
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

#define SECRETB_FILENAME	GameArg.SysUsePlayersDir? "Players/secret.sgb" : "secret.sgb"
#define SECRETC_FILENAME	GameArg.SysUsePlayersDir? "Players/secret.sgc" : "secret.sgc"

int state_save_all(int secret_save, char *filename_override, int blind_save);
int state_restore_all(int in_game, int secret_restore, char *filename_override);

extern uint state_game_id;
extern int state_quick_item;

int state_save_all_sub(char *filename, char *desc);
int state_restore_all_sub(char *filename, int secret_restore);

int state_get_save_file(char *fname, char * dsc, int blind_save);
int state_get_restore_file(char *fname);
int state_get_game_id(char *filename);

#endif /* _STATE_H */
