/* $Id: state.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.1  1995/05/16  16:03:40  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/27  21:40:35  john
 * Added code to verify that the proper multi save file
 * is used when restoring a network game.
 *
 * Revision 2.0  1995/02/27  11:28:44  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.6  1995/02/07  10:54:05  john
 * *** empty log message ***
 *
 * Revision 1.5  1995/02/03  10:58:12  john
 * Added code to save shareware style saved games into new format...
 * Also, made new player file format not have the saved game array in it.
 *
 * Revision 1.4  1995/01/19  17:00:51  john
 * Made save game work between levels.
 *
 * Revision 1.3  1995/01/05  11:51:44  john
 * Added better Abort game menu.
 * Made save state return success or nopt.
 *
 * Revision 1.2  1994/12/29  15:26:39  john
 * Put in hooks for saving/restoring game state.
 *
 * Revision 1.1  1994/12/29  15:15:47  john
 * Initial revision
 *
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
