/* $Id: playsave.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Header for playsave.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:01:19  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:24  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.10  1995/01/22  18:57:04  matt
 * Made player highest level work with missions
 *
 * Revision 1.9  1994/12/12  11:37:15  matt
 * Fixed auto leveling defaults & saving
 *
 * Revision 1.8  1994/12/08  10:01:37  john
 * Changed the way the player callsign stuff works.
 *
 * Revision 1.7  1994/11/25  22:46:56  matt
 * Made saved game descriptions longer
 *
 * Revision 1.6  1994/10/24  20:00:02  john
 * Added prototype for read_player_file.
 *
 * Revision 1.5  1994/10/17  13:07:12  john
 * Moved the descent.cfg info into the player config file.
 *
 * Revision 1.4  1994/10/09  14:54:32  matt
 * Made player cockpit state & window size save/restore with saved games & automap
 *
 * Revision 1.3  1994/10/08  23:08:09  matt
 * Added error check & handling for game load/save disk io
 *
 * Revision 1.2  1994/09/28  17:25:06  matt
 * Added first draft of game save/load system
 *
 * Revision 1.1  1994/09/27  15:47:23  matt
 * Initial revision
 *
 *
 */


#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#ifndef EZERO
#define EZERO 0
#endif

extern int Default_leveling_on;

// update the player's highest level.  returns errno (0 == no error)
int update_player_file();

// Used to save kconfig values to disk.
int write_player_file();

int new_player_config();

int read_player_file();

// set a new highest level for player for this mission
void set_highest_level(int levelnum);

// gets the player's highest level from the file for this mission
int get_highest_level(void);

#endif /* _PLAYSAVE_H */
