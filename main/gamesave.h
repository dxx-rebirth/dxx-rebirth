/* $Id: gamesave.h,v 1.1.1.1 2006/03/17 19:57:20 zicodxx Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Headers for gamesave.c
 *
 */


#ifndef _GAMESAVE_H
#define _GAMESAVE_H

#define	NUM_SHAREWARE_LEVELS	7
#define	NUM_REGISTERED_LEVELS	23

extern char *Shareware_level_names[NUM_SHAREWARE_LEVELS];
extern char *Registered_level_names[NUM_REGISTERED_LEVELS];

int convert_tmap(int tmap);	// for gamemine.c
void LoadGame(void);
void SaveGame(void);
int get_level_name(void);

extern int load_level(char *filename);
extern int save_level(char *filename);

extern char Gamesave_current_filename[];

extern int Gamesave_current_version;

extern int Gamesave_num_org_robots;

// In dumpmine.c
extern void write_game_text_file(char *filename);

extern int Errors_in_mine;

#endif /* _GAMESAVE_H */
