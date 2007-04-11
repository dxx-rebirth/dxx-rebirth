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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/gameseq.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:45:04 $
 * 
 * Routines for EndGame, EndLevel, etc.
 * 
 * $Log: gameseq.c,v $
 * Revision 1.1.1.1  2006/03/17 19:45:04  zicodxx
 * initial import
 *
 * Revision 1.7  1999/11/21 14:05:00  sekmu
 * observer mode
 *
 * Revision 1.6  1999/10/07 02:27:14  donut
 * OGL includes to remove warnings
 *
 * Revision 1.5  1999/09/24 03:29:01  donut
 * added #ifdef OGL around ogl_cache_textures
 *
 * Revision 1.4  1999/09/22 02:11:16  donut
 * ogl texture pre-loading when starting level
 *
 * Revision 1.3  1999/07/10 02:59:07  donut
 * more from orulz
 *
 * Revision 1.2  1999/06/14 23:44:12  donut
 * Orulz' svgalib/ggi/noerror patches.
 *
 * Revision 1.1.1.1  1999/06/14 22:07:31  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.10  1995/12/19  15:48:25  john
 * Made screen reset when loading new level.
 * 
 * Revision 2.9  1995/07/07  16:47:52  john
 * Fixed bug with reactor time..
 * 
 * Revision 2.8  1995/06/15  12:14:18  john
 * Made end game, win game and title sequences all go
 * on after 5 minutes automatically.
 * 
 * Revision 2.7  1995/05/26  16:16:25  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 * 
 * Revision 2.6  1995/03/24  13:11:20  john
 * Added save game during briefing screens.
 * 
 * Revision 2.5  1995/03/23  17:56:20  allender
 * added code to record old laser level and weapons when player gets
 * new ship
 * 
 * Revision 2.4  1995/03/21  08:39:14  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.3  1995/03/15  14:33:33  john
 * Added code to force the Descent CD-rom in the drive.
 * 
 * Revision 2.2  1995/03/06  16:47:26  mike
 * destination saturn
 * 
 * Revision 2.1  1995/03/06  15:23:23  john
 * New screen techniques.
 * 
 * Revision 2.0  1995/02/27  11:28:53  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.310  1995/02/14  10:48:09  mike
 * zero bonus if you are a cheater.
 * 
 * Revision 1.309  1995/02/11  19:17:08  rob
 * Fixed bug in laser fire rate after demo playback.
 * 
 * Revision 1.308  1995/02/11  14:34:08  rob
 * Added include of netmisc.c
 * 
 * Revision 1.307  1995/02/11  14:29:04  rob
 * Fixes for invul. controlcen.
 * 
 * Revision 1.306  1995/02/11  13:47:00  mike
 * fix cheats.
 * 
 * Revision 1.305  1995/02/11  13:10:52  rob
 * Fixed end of anarchy mission problems.
 * 
 * Revision 1.304  1995/02/11  12:46:12  mike
 * initialize Robot_firing_enabled, part of AHIMSA cheat.
 * 
 * Revision 1.303  1995/02/11  12:42:03  john
 * Added new song method, with FM bank switching..
 * 
 * Revision 1.302  1995/02/10  17:39:29  matt
 * Changed secret exit message to be centered
 * 
 * Revision 1.301  1995/02/10  16:17:33  mike
 * init Last_level_path_shown.
 * 
 * Revision 1.300  1995/02/09  22:18:22  john
 * Took out between level saves.
 * 
 * Revision 1.299  1995/02/09  12:11:42  rob
 * Get rid of high scores thing for multiplayer games.
 * 
 * Revision 1.298  1995/02/08  20:34:24  rob
 * Took briefing screens back OUT of coop games (per Interplay request)
 * 
 * Revision 1.297  1995/02/08  19:20:09  rob
 * Moved checksum calc.
 * 
 * Revision 1.296  1995/02/05  14:39:24  rob
 * Changed object mapping to be more efficient.
 * 
 * Revision 1.295  1995/02/02  19:05:38  john
 * Made end level menu for 27 not overwrite descent title..
 * 
 * Revision 1.294  1995/02/02  16:36:42  adam
 * *** empty log message ***
 * 
 * Revision 1.293  1995/02/02  15:58:02  john
 * Added turbo mode cheat.
 * 
 * Revision 1.292  1995/02/02  15:29:34  matt
 * Changed & localized secret level text
 * 
 * Revision 1.291  1995/02/02  10:50:03  adam
 * messed with secret level message
 * 
 * Revision 1.290  1995/02/02  01:20:28  adam
 * changed endgame song temporarily.
 * 
 * Revision 1.289  1995/02/01  23:19:43  rob
 * Fixed up endlevel stuff for multiplayer.
 * Put in palette fades around areas that didn't have them before.
 * 
 * Revision 1.288  1995/02/01  17:12:34  mike
 * Make score come after endgame screens.
 * 
 * Revision 1.287  1995/01/30  18:34:30  rob
 * Put briefing screens back into coop games.
 * 
 * Revision 1.286  1995/01/27  13:07:59  rob
 * Removed erroneous warning message.
 * 
 * Revision 1.285  1995/01/27  11:47:43  rob
 * Removed new secret level menu from multiplayer games.
 * 
 * Revision 1.284  1995/01/26  22:11:11  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 * 
 * Revision 1.283  1995/01/26  16:55:13  rob
 * Removed ship bonus from cooperative endgame.
 * 
 * Revision 1.282  1995/01/26  16:45:24  mike
 * Add autofire fusion cannon stuff.
 * 
 * Revision 1.281  1995/01/26  14:44:44  rob
 * Removed unnecessary #ifdefs around mprintfs.
 * Changed NumNetPlayerPositions to be independant of MaxNumNetPlayers to
 * accomodate 4-player robo-archy games with 8 start positions.
 * 
 * Revision 1.280  1995/01/26  12:19:01  rob
 * Changed network_do_frame call.
 * 
 * Revision 1.279  1995/01/26  00:35:03  matt
 * Changed numbering convention for HMP files for levels
 * 
 * Revision 1.278  1995/01/25  16:07:59  matt
 * Added message (prototype) when going to secret level
 * 
 * Revision 1.277  1995/01/22  18:57:23  matt
 * Made player highest level work with missions
 * 
 * Revision 1.276  1995/01/21  23:13:08  matt
 * Made high scores with (not work, really) with loaded missions
 * Don't give player high score when quit game
 * 
 * Revision 1.275  1995/01/21  17:17:39  john
 * *** empty log message ***
 * 
 * Revision 1.274  1995/01/21  17:15:38  john
 * Added include for state.h
 * 
 * Revision 1.273  1995/01/21  16:21:14  matt
 * Fixed bugs in secret level sequencing
 * 
 * Revision 1.272  1995/01/20  22:47:29  matt
 * Mission system implemented, though imcompletely
 * 
 * Revision 1.271  1995/01/19  17:00:48  john
 * Made save game work between levels.
 * 
 * Revision 1.270  1995/01/17  17:49:10  rob
 * Added key syncing for coop.
 * 
 * Revision 1.269  1995/01/17  14:27:37  john
 * y
 * 
 * Revision 1.268  1995/01/17  13:36:33  john
 * Moved pig loading into StartNewLevelSub.
 * 
 * Revision 1.267  1995/01/16  16:53:55  john
 * Added code to save cheat state during save game.
 * 
 * Revision 1.266  1995/01/15  19:42:10  matt
 * Ripped out hostage faces for registered version
 * 
 * Revision 1.265  1995/01/15  16:55:06  john
 * Improved mine texture parsing.
 * 
 * Revision 1.264  1995/01/15  11:56:24  john
 * Working version of paging.
 * 
 * Revision 1.263  1995/01/14  19:16:40  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.262  1995/01/13  17:38:58  yuan
 * Removed Int3() for number players check.
 * 
 * Revision 1.261  1995/01/12  12:09:52  yuan
 * Added coop object capability.
 * 
 * Revision 1.260  1995/01/05  17:16:08  yuan
 * Removed Int3s.
 * 
 * Revision 1.259  1995/01/05  11:34:29  john
 * Took out endlevel save stuff for registered.
 * 
 * Revision 1.258  1995/01/04  19:00:16  rob
 * Added some debugging for two bugs.
 * 
 * Revision 1.257  1995/01/04  13:18:18  john
 * Added cool 6 game save.
 * 
 * Revision 1.256  1995/01/04  08:46:18  rob
 * JOHN CHECKED IN FOR ROB !!!
 * 
 * Revision 1.255  1995/01/02  20:07:35  rob
 * Added score syncing.
 * Get rid of endlevel score for coop games (put it back in elsewhere)
 * 
 * Revision 1.254  1995/01/02  16:17:43  mike
 * init super boss.
 * 
 * Revision 1.253  1994/12/21  21:08:47  rob
 * fixed a bug in coop player ship positions.
 * 
 * Revision 1.252  1994/12/21  12:57:08  rob
 * Handle additional player ships in mines.
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: gameseq.c,v 1.1.1.1 2006/03/17 19:45:04 zicodxx Exp $";
#endif

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "network.h"
#include "playsave.h"
#include "ctype.h"
#include "multi.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "modem.h"
#include "text.h"
#include "cfile.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "netmisc.h"
#include "joydefs.h"

//added on 3/15/99 by Victor Rachels for LaserPowSelected
#include "reorder.h"
//end this section addition - VR
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "custom.h"
#ifdef SCRIPT
#include "script.h"
#endif

//added on 11/20/99 by Victor Rachels to add observer mode
#include "observer.h"
//end this section addition - VR


void init_player_stats_new_ship();
void copy_defaults_to_robot_all(void);
int AdvanceLevel(int secret_flag);
void StartLevel(int random);

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num=0,Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];		

#ifndef SHAREWARE
int Last_level,Last_secret_level;
#endif

// Global variables describing the player
int 				N_players=1;						// Number of players ( >1 means a net game, eh?)
int 				Player_num=0;						// The player number who is on the console.
player			Players[MAX_PLAYERS];			// Misc player info
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int MaxNumNetPlayers = -1;
int NumNetPlayerPositions = -1;

// Extern from game.c to fix a bug in the cockpit!

extern int last_drawn_cockpit;
extern int Last_level_path_created;

void HUD_clear_messages(); // From hud.c


void verify_console_object()
{
	Assert( Player_num > -1 );
	Assert( Players[Player_num].objnum > -1 );
	ConsoleObject = &Objects[Players[Player_num].objnum];
        //added/edited on 11/20/99 by Victor Rachels to add observer mode
        Assert( ConsoleObject->type==OBJ_PLAYER ||
               (ConsoleObject->type==OBJ_GHOST && I_am_observer));
        //end this section edit - VR

	Assert( ConsoleObject->id==Player_num );
}

int count_number_of_robots() 
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


int count_number_of_hostages() 
{
	int count;
	int i;

	count = 0;
	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}


void
gameseq_init_network_players()
{
	int i,k,j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	for (i=0;i<=Highest_object_index;i++) {

		if (( Objects[i].type==OBJ_PLAYER )	|| (Objects[i].type == OBJ_GHOST) || (Objects[i].type == OBJ_COOP))
		{
#ifndef SHAREWARE
			if ( (!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER)||(Objects[i].type==OBJ_GHOST))) ||
	           ((Game_mode & GM_MULTI_COOP) && ((j == 0) || ( Objects[i].type==OBJ_COOP ))) )
			{
				mprintf((0, "Created Cooperative multiplayer object\n"));
				Objects[i].type=OBJ_PLAYER;
#endif
				mprintf((0, "Player init %d is ship %d.\n", k, j));
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
#ifndef SHAREWARE
			}
			else
				obj_delete(i);
			j++;
#endif
		}
	}
	NumNetPlayerPositions = k;

#ifndef NDEBUG
	if ( ((Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 4)) ||
		  (!(Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 8)) )
	{
		mprintf((1, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n"));
		//Int3(); // Not enough positions!!
	}
#endif
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		for (i=0; i < NumNetPlayerPositions; i++)
		{
			if ((!Players[i].connected) || (i >= N_players))
			{
				#ifndef NDEBUG
//				mprintf((0, "Ghosting player ship %d.\n", i+1));
				#endif
				multi_make_player_ghost(i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
		#ifndef NDEBUG
		mprintf((0, "Removing player objects numbered %d-%d.\n", 1, NumNetPlayerPositions));
		#endif
		for (i=1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

// Setup player for new game
//added on 11/9/98
fix handicap=MAX_SHIELDS;
int Lhandicap=0;

void init_player_stats_game()
{
	Players[Player_num].score = 0;
	Players[Player_num].last_score = 0;
	Players[Player_num].lives = INITIAL_LIVES;
	Players[Player_num].level = 1;

	Players[Player_num].time_level = 0;
	Players[Player_num].time_total = 0;
	Players[Player_num].hours_level = 0;
	Players[Player_num].hours_total = 0;

//added/killed on 11/9/98 by Victor Rachels because of init_player_stats_new_ship()
//-killed-        Players[Player_num].energy = MAX_ENERGY;
//-killed-        Players[Player_num].shields = MAX_SHIELDS;
//end this kill - VR

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].net_killed_total = 0;
	Players[Player_num].net_kills_total = 0;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_kills_total = 0;
	Players[Player_num].num_robots_level = 0;
	Players[Player_num].num_robots_total = 0;
	
	Players[Player_num].hostages_rescued_total = 0;
	Players[Player_num].hostages_level = 0;
	Players[Player_num].hostages_total = 0;

	Players[Player_num].laser_level = 0;
	Players[Player_num].flags = 0;

	init_player_stats_new_ship();

}

void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < MAX_ENERGY)
		Players[Player_num].energy = MAX_ENERGY;
	if (Players[Player_num].shields < MAX_SHIELDS)
		Players[Player_num].shields = MAX_SHIELDS;

//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
//		if (Players[Player_num].primary_ammo[i] < Default_primary_ammo_level[i])
//			Players[Player_num].primary_ammo[i] = Default_primary_ammo_level[i];

//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
//		if (Players[Player_num].secondary_ammo[i] < Default_secondary_ammo_level[i])
//			Players[Player_num].secondary_ammo[i] = Default_secondary_ammo_level[i];
	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

// Setup player for new level (After completion of previous level)
void init_player_stats_level()
{
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

	#ifdef NETWORK
	if (!Network_rejoined)
	#endif
		Players[Player_num].time_level = 0;	//Note link to above if !!!!!!

	init_ammo_and_energy();

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

	Players[Player_num].flags &= (~KEY_BLUE);
	Players[Player_num].flags &= (~KEY_RED);
	Players[Player_num].flags &= (~KEY_GOLD);

	Players[Player_num].flags &= (~PLAYER_FLAGS_INVULNERABLE);
	Players[Player_num].flags &= (~PLAYER_FLAGS_CLOAKED);

	Players[Player_num].cloak_time = 0;
	Players[Player_num].invulnerable_time = 0;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);

	Player_is_dead = 0; // Added by RH
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	Last_laser_fired_time = Next_laser_fire_time = GameTime; // added by RH, solved demo playback bug

	init_gauges();
}

// Setup player for a brand-new ship
void init_player_stats_new_ship()
{
	int	i;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_laser_level(Players[Player_num].laser_level, 0);
		newdemo_record_player_weapon(0, 0);
		newdemo_record_player_weapon(1, 0);
	}

	Players[Player_num].energy = MAX_ENERGY;

        Players[Player_num].shields = MAX_SHIELDS;

//added on 11/9/98 by Victor Rachels for handicapping
         if(Lhandicap)
          Players[Player_num].shields = handicap;
        if(Game_mode & GM_MULTI)
         if(Players[Player_num].shields > MAX_SHIELDS && !Lhandicap)
          Players[Player_num].shields = MAX_SHIELDS;
//end this section addition - VR

//added on 3/15/99 by Victor Rachels to maybe fix respawn-shoot
        Global_laser_firing_count=0;
//end this section addition - VR

	Players[Player_num].laser_level = 0;
	Players[Player_num].killer_objnum = -1;
	Players[Player_num].hostages_on_board = 0;

	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
		Players[Player_num].primary_ammo[i] = 0;

	for (i=1; i<MAX_SECONDARY_WEAPONS; i++)
		Players[Player_num].secondary_ammo[i] = 0;
	Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;

	Players[Player_num].primary_weapon_flags = HAS_LASER_FLAG;
	Players[Player_num].secondary_weapon_flags = HAS_CONCUSSION_FLAG;

	Primary_weapon = 0;
	Secondary_weapon = 0;

	Players[Player_num].flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);

	Players[Player_num].cloak_time = 0;
	Players[Player_num].invulnerable_time = 0;

	Player_is_dead = 0;		//player no longer dead

	Players[Player_num].homing_object_dist = -F1_0; // Added by RH
}

#ifdef NETWORK
void reset_network_objects()
{
	memset(local_to_remote, -1, MAX_OBJECTS*sizeof(short));
	memset(remote_to_local, -1, MAX_NUM_NET_PLAYERS*MAX_OBJECTS*sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
}
#endif

#ifdef EDITOR

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level();
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	ConsoleObject->id=Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Fuelcen_control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship();
}
#endif

void reset_player_object();


static fix time_out_value;
void DoEndLevelScoreGlitzPoll( int nitems, newmenu_item * menus, int * key, int citem )
{
	if ( timer_get_approx_seconds() > time_out_value ) 	{
		*key = -2;
	}
}

//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
	time_out_value = timer_get_approx_seconds() + i2f(60*5);
// 	nm_messagebox1( TXT_GAME_OVER, DoEndLevelScoreGlitzPoll, 1, TXT_OK, "" );

#ifndef SHAREWARE
	if (Current_mission_num == 0)
#endif
		scores_maybe_add_player(0);

	Function_mode = FMODE_MENU;
	Game_mode = GM_GAME_OVER;
	longjmp( LeaveGame, 1 );		// Exit out of game loop

}

extern void do_save_game_menu();

//update various information about the player
void update_player_stats()
{
// I took out this 'if' because it was causing the reactor invul time to be
// off for players that sit in the death screen. -JS jul 6,95
//	if (!Player_exploded) {
		Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
		if ( Players[Player_num].time_level > i2f(3600) )	{
			Players[Player_num].time_level -= i2f(3600);
			Players[Player_num].hours_level++;
		}

		Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
		if ( Players[Player_num].time_total > i2f(3600) )	{
			Players[Player_num].time_total -= i2f(3600);
			Players[Player_num].hours_total++;
		}
//	}

//	Players[Player_num].energy += FrameTime*Energy_regen_ratio;	//slowly regenerate energy

//MK1015:	//slowly reduces player's energy & shields if over max
//MK1015:
//MK1015:	if (Players[Player_num].energy > MAX_ENERGY) {
//MK1015:		Players[Player_num].energy -= FrameTime/8;
//MK1015:		if (Players[Player_num].energy < MAX_ENERGY)
//MK1015:			Players[Player_num].energy = MAX_ENERGY;
//MK1015:	}
//MK1015:
//MK1015:	if (Players[Player_num].shields > MAX_SHIELDS) {
//MK1015:		Players[Player_num].shields -= FrameTime/8;
//MK1015:		if (Players[Player_num].shields < MAX_SHIELDS)
//MK1015:			Players[Player_num].shields = MAX_SHIELDS;
//MK1015:	}
}

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum,sidenum;
	segment *seg;

	digi_init_sounds();		//clear old sounds

	for (seg=&Segments[0],segnum=0;segnum<=Highest_segment_index;seg++,segnum++)
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

			if ((tm=seg->sides[sidenum].tmap_num2) != 0)
				if ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)
					if ((sn=Effects[ec].sound_num)!=-1) {
						vms_vector pnt;

						compute_center_point_on_side(&pnt,seg,sidenum);
						digi_link_sound_to_pos(sn,segnum,sidenum,&pnt,1, F1_0/2);

					}
		}

}


//fix flash_dist=i2f(1);
fix flash_dist=fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(object *player_obj)
{
	vms_vector pos;
	object *effect_obj;

#ifndef NDEBUG
	{
		int objnum = player_obj-Objects;
		if ( (objnum < 0) || (objnum > Highest_object_index) )
			Int3(); // See Rob, trying to track down weird network bug
	}
#endif

	if (player_obj == Viewer)
		vm_vec_scale_add(&pos, &player_obj->pos, &player_obj->orient.fvec, fixmul(player_obj->size,flash_dist));
	else
		pos = player_obj->pos;

	effect_obj = object_create_explosion(player_obj->segnum, &pos, player_obj->size, VCLIP_PLAYER_APPEARANCE );

	if (effect_obj) {
		effect_obj->orient = player_obj->orient;

		if ( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1 )
			digi_link_sound_to_object( Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj-Objects, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	newmenu_item m;
	char text[CALLSIGN_LEN+1]="";

	strncpy(text, Players[Player_num].callsign,CALLSIGN_LEN);

try_again:
	m.type=NM_TYPE_INPUT; m.text_len = 8; m.text = text;

	Newmenu_allowed_chars = playername_allowed_chars;
	x = newmenu_do( NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL );
	Newmenu_allowed_chars = NULL;

	if ( x < 0 ) {
		if ( allow_abort ) return 0;
		goto try_again;
	}

	if (text[0]==0)	//null string
		goto try_again;

	if ( player_exists(text) ) {
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS );
                goto try_again;
	}

	if ( !new_player_config() )
		goto try_again;			// They hit Esc during New player config

	strncpy(Players[Player_num].callsign, text, CALLSIGN_LEN);
	strlwr(Players[Player_num].callsign);

	init_game_list();				//init to defaults

	write_player_file();

	return 1;
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer()
{
	int i,j;
	char filename[14];
	int allow_abort_flag = 1;

        if ( Players[Player_num].callsign[0] == 0 )     {
		//---------------------------------------------------------------------
		// Set default config options in case there is no config file
		// kc_keyboard, kc_joystick, kc_mouse are statically defined.
		Config_joystick_sensitivity = 8;
		Config_mouse_sensitivity = 8;
		Config_control_type =CONTROL_NONE;
		for (i=0; i<CONTROL_MAX_TYPES; i++ )
			for (j=0; j<MAX_CONTROLS; j++ )
				kconfig_settings[i][j] = default_kconfig_settings[i][j];
                //added on 2/4/99 by Victor Rachels for new keys
                 for(i=0; i<MAX_D1X_CONTROLS; i++)
                  kconfig_d1x_settings[i]=default_kconfig_d1x_settings[i];
                //end this section addition - VR
		kc_set_controls();
		//----------------------------------------------------------------

		// Read the last player's name from config file, not lastplr.txt
		strncpy( Players[Player_num].callsign, config_last_player, CALLSIGN_LEN );

		if (config_last_player[0]==0)
			allow_abort_flag = 0;
        }

do_menu_again:
	;

	if (!newmenu_get_filename( TXT_SELECT_PILOT, "*.plr", filename, allow_abort_flag ))	{
		goto do_menu_again;		// They hit Esc in file selector
	}

	if ( filename[0] == '<' )	{
		// They selected 'create new pilot'
		if (!MakeNewPlayerFile(allow_abort_flag))
			//return 0;		// They hit Esc during enter name stage
			goto do_menu_again;
	} else {
		strncpy(Players[Player_num].callsign,filename, CALLSIGN_LEN);
		strlwr(Players[Player_num].callsign);
	}

	read_player_file();

	Auto_leveling_on = Default_leveling_on;

	if (menu_use_game_res) { // ZICO - set players resolution after player is selected
		gr_set_mode(Game_screen_mode);
		set_screen_mode(SCREEN_GAME);
	}

	WriteConfigFile();		// Update lastplr

        return 1;
}

extern int descent_critical_error;

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
char *get_level_file(int level_num)
{
#ifdef SHAREWARE
        {
                static char t[13];
                sprintf(t, "level%02d.sdl", level_num);
		return t;
        }
#else
        if (level_num<0)                //secret level
		return Secret_level_names[-level_num-1];
        else                                    //normal level
		return Level_names[level_num-1];
#endif
}

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num) 
{
	char *level_name;
	player save_player;

#ifdef REQUIRE_CD
	{
		FILE *fp;
		int i;
		char fname[128];
		strcpy( fname, destsat_cdpath );
#ifdef DEST_SAT
		strcat( fname, "saturn.hog" );
#else
		strcat( fname, "descent.hog" );
#endif
		do {
			descent_critical_error = 0;
			fp = fopen( fname, "rb" );
			if ( fp==NULL || descent_critical_error )	{
				if ( fp )	{
					fclose(fp);
					fp = NULL;
				}
				gr_set_current_canvas(NULL);
				gr_clear_canvas( gr_find_closest_color_current(0,0,0) );
				gr_palette_load( gr_palette );
				i = nm_messagebox( "Insert CD", 2, "Retry", "Exit", "Please put the\nDescent CD\nin your CD-ROM drive!\n" );
				if ( i==1 )
					exit(0);
			}
		} while ( fp == NULL );
		fclose( fp );
	}
#endif

	save_player = Players[Player_num];	

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);

	level_name = get_level_file(level_num);

	show_boxed_message(TXT_LOADING);

	if (!load_level(level_name))
		Current_level_num=level_num;

	#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));
	#endif

	load_endlevel_data(level_num);

	clear_boxed_message();

	#ifdef NETWORK
	reset_network_objects();
	#endif

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song( Current_level_num );

}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	if (Player_num != 0 )	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = 0;

	ConsoleObject = &Objects[Players[Player_num].objnum];

        ConsoleObject->type             = OBJ_PLAYER;
        ConsoleObject->id               = Player_num;
	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;
}

extern void game_disable_cheats();

//starts a new game on the given level
void StartNewGame(int start_level)
{
	Game_mode = GM_NORMAL;
	Function_mode = FMODE_GAME;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game();		//clear all stats

	N_players = 1;
	#ifdef NETWORK
	Network_new_game = 0;
	#endif

#ifdef SCRIPT
	script_reset();
	script_load("default.scr");
#endif

	StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();
}

//starts a resumed game loaded from disk
void ResumeSavedGame(int start_level)
{
	Game_mode = GM_NORMAL;
	Function_mode = FMODE_GAME;

	N_players = 1;
	#ifdef NETWORK
	Network_new_game = 0;
	#endif

	InitPlayerObject();				//make sure player's object set up

	StartNewLevel(start_level);

	game_disable_cheats();
}

#ifdef NETWORK
extern void network_endlevel_poll2( int nitems, newmenu_item * menus, int * key, int citem ); // network.c
#endif


//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz(int network)
{ 
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
	#define N_GLITZITEMS 9
	char				m_str[N_GLITZITEMS][30];
	newmenu_item	m[9];
	int				i,c;
	char				title[128];
	int				is_last_level;

	gr_palette_load( gr_palette ); // ZICO - added to be sure right palette is loaded after endgame

        set_screen_mode(SCREEN_MENU);

	level_points = Players[Player_num].score-Players[Player_num].last_score;

	if (!Cheats_enabled) {
		if (Difficulty_level > 1) {
			skill_points = level_points*(Difficulty_level-1)/2;
			skill_points -= skill_points % 100;
		} else
			skill_points = 0;

		shield_points = f2i(Players[Player_num].shields) * 10 * (Difficulty_level+1);
		energy_points = f2i(Players[Player_num].energy) * 5 * (Difficulty_level+1);
		hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level+1);
	} else {
		skill_points = 0;
		shield_points = 0;
		energy_points = 0;
		hostage_points = 0;
	}

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (!Cheats_enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level)) {
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level+1);
		sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	} else
		all_hostage_points = 0;

	if (!Cheats_enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) {		//player has finished the game!
		endgame_points = Players[Player_num].lives * 10000;
		sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level=1;
	} else
		endgame_points = is_last_level = 0;

	add_bonus_points_to_score(skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);

	c = 0;
	sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
	sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	sprintf(m_str[c++], "%s%i", TXT_SKILL_BONUS, skill_points);

	sprintf(m_str[c++], "%s", all_hostage_text);
	if (!(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))
		sprintf(m_str[c++], "%s", endgame_text);

	sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points+energy_points+hostage_points+skill_points+all_hostage_points+endgame_points);
	sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);

	for (i=0; i<c; i++) {
		m[i].type = NM_TYPE_TEXT;
		m[i].text = m_str[i];
	}

	// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";

	if (Current_level_num < 0)
		sprintf(title,"%s%s %d %s\n %s %s",is_last_level?"\n\n\n":"\n",TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
	else
		sprintf(title,"%s%s %d %s\n%s %s",is_last_level?"\n\n\n":"\n",TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	gr_palette_fade_out(gr_palette, 32, 0);

	time_out_value = timer_get_approx_seconds() + i2f(60*5);

#ifdef NETWORK
	if ( network && (Game_mode & GM_NETWORK) )
		newmenu_do2(NULL, title, c, m, network_endlevel_poll2, 0, "MENU.PCX");
	else
#endif	// Note link!
		newmenu_do2(NULL, title, c, m, DoEndLevelScoreGlitzPoll, 0, "MENU.PCX");
}

//give the player the opportunity to save his game
void DoEndlevelMenu()
{
#ifdef SHAREWARE
	if (!Cheats_enabled)
		do_save_game_menu();
#else
//No between level saves......!!!	state_save_all(1);
#endif
}

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	int	rval;
	int 	was_multi = 0;

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

#ifndef SHAREWARE
	if (!(Game_mode & GM_MULTI) && (secret_flag)) {
		newmenu_item	m[1];

		m[0].type = NM_TYPE_TEXT;
		m[0].text = " ";			//TXT_SECRET_EXIT;

		newmenu_do2(NULL, TXT_SECRET_EXIT, 1, m, NULL, 0, "MENU.PCX");
	}
#endif

// -- mk mk mk -- used to be here -- mk mk mk --

	if (Game_mode & GM_NETWORK)
         {
		if (secret_flag)
			Players[Player_num].connected = 4; // Finished and went to secret level
		else
			Players[Player_num].connected = 2; // Finished but did not die
         }
	last_drawn_cockpit = -1;

	if (Current_level_num == Last_level) {
		#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			was_multi = 1;
			multi_endlevel_score();
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
		else 
		#endif
		{	// Note link to above else!
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
			DoEndLevelScoreGlitz(0);		//give bonuses
		}
	} else {
		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else 
		#endif	// Note link!!
			DoEndLevelScoreGlitz(0);		//give bonuses
		rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
	}

	if (!was_multi && rval) {
#ifndef SHAREWARE
		if (Current_mission_num == 0)
#endif
			scores_maybe_add_player(0);
		longjmp( LeaveGame, 1 );		// Exit out of game loop
	}
	else if (rval)
		longjmp( LeaveGame, 1 );
}


extern void do_end_game(void);

//from which level each do you get to each secret level 
int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
int AdvanceLevel(int secret_flag)
{
	Fuelcen_control_center_destroyed = 0;

	#ifdef EDITOR
	if (Current_level_num == 0)
		return 0;		//not a real level
	#endif

	#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
          int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
			return (Current_level_num == Last_level);
	}
	#endif

	if (Current_level_num == Last_level) {		//player has finished the game!
		
		Function_mode = FMODE_MENU;
		if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording();

		#ifndef SHAREWARE
		songs_play_song( SONG_ENDGAME, 0 );
		#endif

		do_end_game();
		return 1;

	} else {

		Next_level_num = Current_level_num+1;		//assume go to next normal level

		if (secret_flag) {			//go to secret level instead
			int i;

			for (i=0;i<-Last_secret_level;i++)
				if (Secret_level_table[i]==Current_level_num) {
					Next_level_num = -(i+1);
					break;
				}
			Assert(i<-Last_secret_level);		//couldn't find which secret level
		}

		if (Current_level_num < 0)	{			//on secret level, where to go?

			Assert(!secret_flag);				//shouldn't be going to secret level
			Assert(Current_level_num<=-1 && Current_level_num>=Last_secret_level);

			Next_level_num = Secret_level_table[(-Current_level_num)-1]+1;
		}

		if (!(Game_mode & GM_MULTI))
			DoEndlevelMenu(); // Let use save their game

		StartNewLevel(Next_level_num);

	}

	return 0;
}


void
died_in_mine_message(void)
{
	// Tell the player he died in the mine, explain why
	int old_fmode, pcx_error;

	if (Game_mode & GM_MULTI)
		return;

	gr_palette_fade_out(gr_palette, 32, 0);

	gr_set_current_canvas(NULL);
	
	pcx_error = pcx_read_bitmap("STARS.PCX",&grd_curcanv->cv_bitmap,grd_curcanv->cv_bitmap.bm_type,NULL);
	Assert(pcx_error == PCX_ERROR_NONE);

	old_fmode = Function_mode;
	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK, TXT_DIED_IN_MINE);
	Function_mode = old_fmode;
}

//called when the player has died
void DoPlayerDead()
{
	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

#ifdef HOSTAGE_FACES
	stop_all_hostage_clips();
#endif

	#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object * player = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		load_level("gamesave.lvl");
		init_player_stats_new_ship();
		player->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
	#endif

	#ifdef NETWORK
	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else 
	#endif		
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{	
			DoGameOver();
			return;
		}
	}
				
	if ( Fuelcen_control_center_destroyed ) {
		int	rval;

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = 3;

		died_in_mine_message(); // Give them some indication of what happened

		if (Current_level_num == Last_level) {
			#ifdef NETWORK
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			{
				multi_endlevel_score();
				rval = AdvanceLevel(0);			//if finished, go on to next level
			}
			else
			#endif	
			{			// Note link to above else!
				rval = AdvanceLevel(0);			//if finished, go on to next level
				DoEndLevelScoreGlitz(0);	
			}
			init_player_stats_new_ship();
			last_drawn_cockpit = -1;
		} else {
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_endlevel_score();
			else
			#endif
				DoEndLevelScoreGlitz(0);		// Note above link!

			rval = AdvanceLevel(0);			//if finished, go on to next level
			init_player_stats_new_ship();
			last_drawn_cockpit = -1;
		}

		if (rval) {
#ifndef SHAREWARE
			if (Current_mission_num == 0)
#endif
				scores_maybe_add_player(0);
			longjmp( LeaveGame, 1 );		// Exit out of game loop
		}
	} else {
		init_player_stats_new_ship();
		StartLevel(1);
	}

}

//called when the player is starting a new level for normal game mode and restore state
void StartNewLevelSub(int level_num, int page_in_textures)
{
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameCount, FrameTime );
	}

	if (Game_mode & GM_MULTI)
		Function_mode = FMODE_MENU; // Cheap fix to prevent problems with errror dialogs in loadlevel.

	LoadLevel(level_num);

	if ( page_in_textures )	{
		piggy_load_level_data();
	}

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for 
											  // this level

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if(network_level_sync()) // After calling this, Player_num is set
			return;
	}
	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
	{
		if(com_level_sync())
			return;
	}
#endif

	Assert(Function_mode == FMODE_GAME);

	#ifndef NDEBUG
	mprintf((0, "Player_num = %d, N_players = %d.\n", Player_num, N_players)); // DEBUG
	#endif

	HUD_clear_messages();

	automap_clear_visited();

	#ifdef NETWORK
	if (Network_new_game == 1)
	{
		Network_new_game = 0;
		init_player_stats_new_ship();
	}
	#endif
	init_player_stats_level();

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}
#endif
#endif

	Viewer = &Objects[Players[Player_num].objnum];

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}
#endif

	gameseq_remove_unused_players();

	count_powerup_start_level();

	Game_suspended = 0;

	Fuelcen_control_center_destroyed = 0;

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();

	if (!(Game_mode & GM_MULTI) && !Cheats_enabled)
		set_highest_level(Current_level_num);

	reset_special_effects();

#ifdef OGL
	ogl_cache_level_textures();
#endif

#ifdef NETWORK
	if (Network_rejoined == 1)
	{
		#ifndef NDEBUG
		mprintf((0, "Restarting - joining in random location.\n"));
		#endif
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
#endif
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;
}

//called when the player is starting a new level for normal game model
void StartNewLevel(int level_num)
{
	show_boxed_message(TXT_LOADING);

	load_custom_data(get_level_file(level_num));

        clear_boxed_message();

        if (!(Game_mode & GM_MULTI)) {
		do_briefing_screens(level_num);
	}
	StartNewLevelSub(level_num, 1 );		

}

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random)
{
        int NewPlayer=0;

	if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
#ifdef NETWORK
	else if (random == 1)
	{
		int i, closest = -1, trys=0;
		fix closest_dist = 0x7ffffff, dist;

		d_srand(clock());

#ifndef NDEBUG
		if (NumNetPlayerPositions != MAX_NUM_NET_PLAYERS)
		{
			mprintf((1, "WARNING: There are only %d start positions!\n"));
			//Int3();
		}
#endif

		do {
			if (trys > 0)	
			{
				mprintf((0, "Can't start in location %d because its too close to player %d\n", NewPlayer, closest ));
			}
			trys++;
			NewPlayer = d_rand() % NumNetPlayerPositions;

			closest = -1;
			closest_dist = 0x7fffffff;
	
			for (i=0; i<N_players; i++ )	{
				if ( (i!=Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER) )	{
					dist = find_connected_distance(&Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, &Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 5, WID_FLY_FLAG );
//					mprintf((0, "Distance from start location %d to player %d is %f.\n", NewPlayer, i, f2fl(dist)));
					if ( (dist < closest_dist) && (dist >= 0) )	{
						closest_dist = dist;
						closest = i;
					}
				}
			}
			mprintf((0, "Closest from pos %d is %f to plr %d.\n", NewPlayer, f2fl(closest_dist), closest));
		} while ( (closest_dist<i2f(10*20)) && (trys<MAX_NUM_NET_PLAYERS*2) );
	} 
#endif
	else {
		mprintf((0, "Starting position is not being changed.\n"));
		goto done; // If deathmatch and not random, positions were already determined by sync packet
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);

	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;

	mprintf((0, "Re-starting in location %d of %d.\n", NewPlayer+1, NumNetPlayerPositions));

 	obj_relink(ConsoleObject-Objects,Player_init[NewPlayer].segnum);

done:
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(object *objp)
{
	robot_info	*robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = objp->id;
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	objp->shields = robptr->strength;

}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all(void)
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);
}

int	Do_appearance_effect=0;

extern int Rear_view;
extern void vr_reset_display();

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	disable_matcens();

	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
#ifndef SHAREWARE
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
#endif
		multi_send_position(Players[Player_num].objnum);
	 	multi_send_reappear();
	}		

	if (Game_mode & GM_NETWORK)
		network_do_frame(1, 1);
#endif

	ai_reset_all_paths();
	ai_init_boss_for_ship();
	reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	Robot_firing_enabled = 1;

	if (Game_screen_mode == SCREEN_MENU)
		vr_reset_display();
}


