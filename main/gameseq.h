/* $Id: gameseq.h,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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
 * Prototypes for functions for game sequencing.
 *
 * Old Log:
 * Revision 1.4  1995/10/31  10:22:55  allender
 * shareware stuff
 *
 * Revision 1.3  1995/09/14  14:13:14  allender
 * initplayerobject have void return
 *
 * Revision 1.2  1995/08/24  15:36:17  allender
 * fixed prototypes warnings
 *
 * Revision 1.1  1995/05/16  15:57:26  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:03  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.41  1995/02/07  10:51:54  rob
 * fix typo.
 *
 * Revision 1.40  1995/02/06  20:10:16  rob
 * Extern'ed DoEndLevelScoreGlitz.
 *
 * Revision 1.39  1995/02/01  16:34:13  john
 * Linted.
 *
 * Revision 1.38  1995/01/27  11:15:03  rob
 * Added extern for player position vars.
 *
 * Revision 1.37  1995/01/20  22:47:38  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.36  1995/01/17  13:36:08  john
 * Moved pig loading into StartNewLevelSub.
 *
 * Revision 1.35  1995/01/04  12:21:28  john
 * *** empty log message ***
 *
 * Revision 1.34  1995/01/04  12:20:47  john
 * Declearations to work better with game state save.
 *
 *
 * Revision 1.33  1994/12/08  09:46:35  matt
 * Made level name len a multiple of 4 for alignment
 *
 * Revision 1.32  1994/11/29  16:33:29  rob
 * Added new defines for last_secret_level based on shareware or not shareware.
 *
 * Revision 1.31  1994/11/26  15:30:20  matt
 * Allow escape out of change pilot menu
 *
 * Revision 1.30  1994/11/21  17:29:38  matt
 * Cleaned up sequencing & game saving for secret levels
 *
 * Revision 1.29  1994/11/21  15:55:03  matt
 * Corrected LAST_LEVEL
 *
 * Revision 1.28  1994/11/20  22:12:43  mike
 * set LAST_LEVEL based on SHAREWARE.
 *
 * Revision 1.27  1994/11/09  10:55:51  matt
 * Cleaned up initialization for editor -> game transitions
 *
 * Revision 1.26  1994/11/08  17:50:48  rob
 * ADded prototype for StartNewLEvel.
 *
 *
 * Revision 1.25  1994/11/07  17:50:57  rob
 * Added extern prototype for init_player_stats_level called for
 * network games.
 *
 * Revision 1.24  1994/10/25  15:40:03  yuan
 * *** empty log message ***
 *
 * Revision 1.23  1994/10/22  00:08:52  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 *
 * Revision 1.22  1994/10/18  18:57:08  matt
 * Added main menu option to enter new player name
 *
 * Revision 1.21  1994/10/07  23:37:32  matt
 * Added prototype
 *
 * Revision 1.20  1994/10/07  16:02:53  matt
 * Loading saved game no longer clears players weapons & other stats
 *
 * Revision 1.19  1994/10/06  14:12:46  matt
 * Added flash effect when player appears
 *
 * Revision 1.18  1994/10/03  13:34:44  matt
 * Added new (and hopefully better) game sequencing functions
 *
 * Revision 1.17  1994/09/30  15:19:53  matt
 * Added new game sequencing functions, but left them disabled for now.
 *
 * Revision 1.16  1994/09/28  17:24:34  matt
 * Added first draft of game save/load system
 *
 * Revision 1.15  1994/09/27  12:29:42  matt
 * Changed level naming
 *
 * Revision 1.14  1994/09/02  11:53:55  mike
 * Rename init_player_stats to init_player_stats_game.
 *
 * Revision 1.13  1994/08/31  20:57:34  matt
 * Cleaned up endlevel/death code
 *
 * Revision 1.12  1994/08/23  18:45:06  yuan
 * Added level 10 capability.. (LEDGES)
 *
 * Revision 1.11  1994/08/18  10:47:38  john
 * Cleaned up game sequencing and player death stuff
 * in preparation for making the player explode into
 * pieces when dead.
 *
 * Revision 1.10  1994/08/15  15:24:45  john
 * Made players know who killed them; Disabled cheat menu
 * during net player; fixed bug with not being able to turn
 * of invulnerability; Made going into edit/starting new leve
 * l drop you out of a net game; made death dialog box.
 *
 * Revision 1.9  1994/08/13  12:20:56  john
 * Made the networking uise the Players array.
 *
 * Revision 1.8  1994/07/22  12:36:24  matt
 * Cleaned up editor/game interactions some more.
 *
 * Revision 1.7  1994/07/19  20:15:33  matt
 * Name for each level now saved in the .SAV file & stored in Current_level_name
 *
 * Revision 1.6  1994/07/02  13:49:33  matt
 * Cleaned up includes
 *
 * Revision 1.5  1994/07/02  13:09:52  matt
 * Moved player stats struct from gameseq.h to player.h
 *
 * Revision 1.4  1994/07/01  16:35:35  yuan
 * Added key system
 *
 * Revision 1.3  1994/06/26  14:07:35  matt
 * Added prototypes
 *
 * Revision 1.2  1994/06/24  17:03:56  john
 * Added VFX support. Also took all game sequencing stuff like
 * EndGame out and put it into gameseq.c
 *
 * Revision 1.1  1994/06/24  14:13:53  john
 * Initial revision
 *
 *
 */


#ifndef _GAMESEQ_H
#define _GAMESEQ_H

#include "player.h"
#include "mission.h"

#define SUPER_MISSILE       0
#define SUPER_SEEKER        1
#define SUPER_SMARTBOMB     2
#define SUPER_SHOCKWAVE     3

extern int Last_level, Last_secret_level;   //set by mission code

extern int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

#define LEVEL_NAME_LEN 36       //make sure this is multiple of 4!

// Current_level_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
extern int Current_level_num, Next_level_num;
extern char Current_level_name[LEVEL_NAME_LEN];
extern obj_position Player_init[MAX_PLAYERS];


// This is the highest level the player has ever reached
extern int Player_highest_level;

//
// New game sequencing functions
//

// called once at program startup to get the player's name
int RegisterPlayer();

// Inputs the player's name, without putting up the background screen
int RegisterPlayerSub(int allow_abort_flag);

// starts a new game on the given level
void StartNewGame(int start_level);

// starts the next level
void StartNewLevel(int level_num, int secret_flag);
void StartLevel(int random_flag);

// Actually does the work to start new level
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag);

void InitPlayerObject();            //make sure player's object set up
void init_player_stats_game();      //clear all stats

// starts a resumed game loaded from disk
void ResumeSavedGame(int start_level);

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secret_flag);

// called when the player has died
void DoPlayerDead(void);

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
void LoadLevel(int level_num, int page_in_textures);

extern void gameseq_remove_unused_players();

extern void show_help();
extern void update_player_stats();

// from scores.c

extern void show_high_scores(int place);
extern void draw_high_scores(int place);
extern int add_player_to_high_scores(player *pp);
extern void input_name (int place);
extern int reset_high_scores();
extern void init_player_stats_level(int secret_flag);

void open_message_window(void);
void close_message_window(void);

// create flash for player appearance
extern void create_player_appearance_effect(object *player_obj);

// goto whatever secrect level is appropriate given the current level
extern void goto_secret_level();

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level();

// Show endlevel bonus scores
extern void DoEndLevelScoreGlitz(int network);

// stuff for multiplayer
extern int MaxNumNetPlayers;
extern int NumNetPlayerPositions;

void bash_to_shield(int, char *);

#endif /* _GAMESEQ_H */
