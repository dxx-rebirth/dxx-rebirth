/* $Id: gamesave.h,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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
 * Headers for gamesave.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:57:10  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:30:25  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.11  1994/11/23  12:19:32  mike
 * detail level menu.
 *
 * Revision 1.10  1994/10/20  12:47:30  matt
 * Replace old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.9  1994/09/27  17:08:47  mike
 * Message boxes when you load bogus mines.
 *
 * Revision 1.8  1994/09/27  15:43:05  mike
 * Prototype write_game_text.
 *
 * Revision 1.7  1994/09/14  15:46:39  matt
 * Added function load_mine_only()
 *
 * Revision 1.6  1994/07/22  12:36:28  matt
 * Cleaned up editor/game interactions some more.
 *
 * Revision 1.5  1994/07/20  13:38:14  matt
 * Added get_level_name() prototype
 *
 * Revision 1.4  1994/06/20  22:19:41  john
 * Added Gamesave_num_org_robots.
 *
 * Revision 1.3  1994/06/14  11:32:49  john
 * Made Newdemo record & restore the current mine.
 *
 * Revision 1.2  1994/05/14  17:16:25  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.1  1994/05/14  16:01:26  matt
 * Initial revision
 *
 *
 */


#ifndef _GAMESAVE_H
#define _GAMESAVE_H

void LoadGame(void);
void SaveGame(void);
void get_level_name(void);

extern int load_level(char *filename);
extern int save_level(char *filename);

// called in place of load_game() to only load the .min data
extern void load_mine_only(char * filename);

extern char Gamesave_current_filename[];

extern int Gamesave_current_version;

extern int Gamesave_num_org_robots;

// In dumpmine.c
extern void write_game_text_file(char *filename);

extern int Errors_in_mine;

#endif /* _GAMESAVE_H */
