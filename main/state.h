/* $Id: state.h,v 1.3 2004-08-28 23:17:45 schaffner Exp $ */
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

int state_save_all(int between_levels, int secret_save, char *filename_override);
int state_restore_all(int in_game, int secret_restore, char *filename_override);

int state_save_all_sub(char *filename, char *desc, int between_levels);
int state_restore_all_sub(char *filename, int multi, int secret_restore);

extern uint state_game_id;

int state_get_save_file(char *fname, char * dsc, int multi);
int state_get_restore_file(char *fname, int multi);

#endif /* _STATE_H */
