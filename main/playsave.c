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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/playsave.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:10 $
 * 
 * Functions to load & save player games
 * 
 * $Log: playsave.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:10  zicodxx
 * initial import
 *
 * Revision 1.3  2003/02/16 10:44:56  donut
 * fix the weird bug where joystick fire button would do all the d1x key functions at once, when joining a udp game. (but going into config menu would fix it.)  It was due to kc_set_controls() being called before read_player_d1x, so the d1x settings weren't getting noticed.
 *
 * Revision 1.2  1999/06/14 23:44:12  donut
 * Orulz' svgalib/ggi/noerror patches.
 *
 * Revision 1.1.1.1  1999/06/14 22:11:04  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.3  1995/05/26  16:16:23  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 * 
 * Revision 2.2  1995/03/24  17:48:21  john
 * Made player files from saturn excrement the highest level for
 * normal descent levels.
 * 
 * Revision 2.1  1995/03/21  14:38:49  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.0  1995/02/27  11:27:59  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.57  1995/02/13  20:34:55  john
 * Lintized
 * 
 * Revision 1.56  1995/02/13  13:23:24  john
 * Fixed bug with new player joystick selection.
 * 
 * Revision 1.55  1995/02/13  12:01:19  john
 * Fixed bug with joystick throttle still asking for 
 * calibration with new pilots.
 * 
 * Revision 1.54  1995/02/13  10:29:12  john
 * Fixed bug with creating new player not resetting everything to default.
 * 
 * Revision 1.53  1995/02/03  10:58:46  john
 * Added code to save shareware style saved games into new format...
 * Also, made new player file format not have the saved game array in it.
 * 
 * Revision 1.52  1995/02/02  21:09:28  matt
 * Let player start of level 8 if he made it to level 7 in the shareware
 * 
 * Revision 1.51  1995/02/02  18:50:14  john
 * Added warning for FCS when new pilot chooses.
 * 
 * Revision 1.50  1995/02/02  11:21:34  john
 * Made joystick calibrate when new user selects.
 * 
 * Revision 1.49  1995/02/01  18:06:38  rob
 * Put defaults macros into descent.tex
 * 
 * Revision 1.48  1995/01/25  14:37:53  john
 * Made joystick only prompt for calibration once...
 * 
 * Revision 1.47  1995/01/24  19:37:12  matt
 * Took out incorrect mprintf
 * 
 * Revision 1.46  1995/01/22  18:57:22  matt
 * Made player highest level work with missions
 * 
 * Revision 1.45  1995/01/21  16:36:05  matt
 * Made starting level system work for now, pending integration with
 * mission code.
 * 
 * Revision 1.44  1995/01/20  22:47:32  matt
 * Mission system implemented, though imcompletely
 * 
 * Revision 1.43  1995/01/04  14:58:39  rob
 * Fixed for shareware build.
 * 
 * Revision 1.42  1995/01/04  11:36:43  rob
 * Added compatibility with older shareware pilot files.
 * 
 * Revision 1.41  1995/01/03  11:01:58  rob
 * fixed a default macro.
 * 
 * Revision 1.40  1995/01/03  10:44:06  rob
 * Added default taunt macros.
 * 
 * Revision 1.39  1994/12/13  10:01:16  allender
 * pop up message box when unable to correctly save player file
 * 
 * Revision 1.38  1994/12/12  11:37:14  matt
 * Fixed auto leveling defaults & saving
 * 
 * Revision 1.37  1994/12/12  00:26:59  matt
 * Added support for no-levelling option
 * 
 * Revision 1.36  1994/12/10  19:09:54  matt
 * Added assert for valid player number when loading game
 * 
 * Revision 1.35  1994/12/08  10:53:07  rob
 * Fixed a bug in highest_level tracking.
 * 
 * Revision 1.34  1994/12/08  10:01:36  john
 * Changed the way the player callsign stuff works.
 * 
 * Revision 1.33  1994/12/07  18:30:38  rob
 * Load highest level along with player (used to be only if higher)
 * Capped at LAST_LEVEL in case a person loads a registered player in shareware.
 * 
 * Revision 1.32  1994/12/03  16:01:12  matt
 * When player file has bad version, force player to choose another
 * 
 * Revision 1.31  1994/12/02  19:54:00  yuan
 * Localization.
 * 
 * Revision 1.30  1994/12/02  11:01:36  yuan
 * Localization.
 * 
 * Revision 1.29  1994/11/29  03:46:28  john
 * Added joystick sensitivity; Added sound channels to detail menu.  Removed -maxchannels
 * command line arg.
 * 
 * Revision 1.28  1994/11/29  01:10:23  john
 * Took out code that allowed new players to
 * configure keyboard.
 * 
 * Revision 1.27  1994/11/25  22:47:10  matt
 * Made saved game descriptions longer
 * 
 * Revision 1.26  1994/11/22  12:10:42  rob
 * Fixed file handle left open if player file versions don't
 * match.
 * 
 * Revision 1.25  1994/11/21  19:35:30  john
 * Replaced calls to joy_init with if (joy_present)
 * 
 * Revision 1.24  1994/11/21  17:29:34  matt
 * Cleaned up sequencing & game saving for secret levels
 * 
 * Revision 1.23  1994/11/21  11:10:01  john
 * Fixed bug with read-only .plr file making the config file 
 * not update.
 * 
 * Revision 1.22  1994/11/20  19:03:08  john
 * Fixed bug with if not having a joystick, default 
 * player input device is cyberman.
 * 
 * Revision 1.21  1994/11/17  12:24:07  matt
 * Made an array the right size, to fix error loading games
 * 
 * Revision 1.20  1994/11/14  17:52:54  allender
 * add call to WriteConfigFile when player files gets written
 * 
 * Revision 1.19  1994/11/14  17:19:23  rob
 * Removed gamma, joystick calibration, and sound settings from player file.
 * Added default difficulty and multi macros.
 * 
 * Revision 1.18  1994/11/07  14:01:23  john
 * Changed the gamma correction sequencing.
 * 
 * Revision 1.17  1994/11/05  17:22:49  john
 * Fixed lots of sequencing problems with newdemo stuff.
 * 
 * Revision 1.16  1994/11/01  16:40:11  john
 * Added Gamma correction.
 * 
 * Revision 1.15  1994/10/24  19:56:50  john
 * Made the new user setup prompt for config options.
 * 
 * Revision 1.14  1994/10/24  17:44:21  john
 * Added stereo channel reversing.
 * 
 * Revision 1.13  1994/10/24  16:05:12  matt
 * Improved handling of player names that are the names of DOS devices
 * 
 * Revision 1.12  1994/10/22  00:08:51  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 * 
 * Revision 1.11  1994/10/19  19:59:57  john
 * Added bonus points at the end of level based on skill level.
 * 
 * Revision 1.10  1994/10/19  15:14:34  john
 * Took % hits out of player structure, made %kills work properly.
 * 
 * Revision 1.9  1994/10/19  12:44:26  john
 * Added hours field to player structure.
 * 
 * Revision 1.8  1994/10/17  17:24:34  john
 * Added starting_level to player struct.
 * 
 * Revision 1.7  1994/10/17  13:07:15  john
 * Moved the descent.cfg info into the player config file.
 * 
 * Revision 1.6  1994/10/09  14:54:31  matt
 * Made player cockpit state & window size save/restore with saved games & automap
 * 
 * Revision 1.5  1994/10/08  23:08:09  matt
 * Added error check & handling for game load/save disk io
 * 
 * Revision 1.4  1994/10/05  17:40:54  rob
 * Bumped save_file_version to 5 due to change in player.h
 * 
 * Revision 1.3  1994/10/03  23:00:54  matt
 * New file version for shorter callsigns
 * 
 * Revision 1.2  1994/09/28  17:25:05  matt
 * Added first draft of game save/load system
 * 
 * Revision 1.1  1994/09/27  14:39:12  matt
 * Initial revision
 * 
 * 
 */

#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: playsave.c,v 1.1.1.1 2006/03/17 19:42:10 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "error.h"

#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "kconfig.h"
#include "digi.h"
#include "newmenu.h"
#include "joydefs.h"
#include "palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "mono.h"
#include "state.h"
#include "reorder.h"
#include "d_io.h"
//added on 9/16/98 by adb to add memory tracking for this module
#include "u_mem.h"
//end additions - adb
//added on 11/12/98 by Victor Rachels to add networkrejoin thing
#include "network.h"
//end this section addition - VR
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added


#include "strio.h"
#include "vers_id.h"

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#define SAVE_FILE_ID			0x44504c52 /* 'DPLR' */

//this is for version 5 and below
typedef struct save_info_v5 {
	int	id;
	short	saved_game_version,player_struct_version;
	int 	highest_level;
	int	default_difficulty_level;
	int	default_leveling_on;
} __pack__ save_info_v5;

//this is for version 6 and above 
typedef struct save_info {
	int	id;
	short	saved_game_version,player_struct_version;
	int	n_highest_levels;				//how many highest levels are saved
	int	default_difficulty_level;
	int	default_leveling_on;
	int	VR_render_w;
	int	VR_render_h;
	int	Game_window_w;
	int	Game_window_h;
} __pack__ save_info;

typedef struct hli {
	char	shortname[9];
	ubyte	level_num;
} __pack__ hli;

int n_highest_levels;

hli highest_levels[MAX_MISSIONS];

#define SAVED_GAME_VERSION		8		//increment this every time saved_game struct changes

//version 5 -> 6: added new highest level information
//version 6 -> 7: stripped out the old saved_game array.
//version 7 -> 8: readded the old saved_game array since this is needed
//                for shareware saved games

//the shareware is level 4

#define COMPATIBLE_SAVED_GAME_VERSION		4
#define COMPATIBLE_PLAYER_STRUCT_VERSION	16

typedef struct saved_game {
	char		name[GAME_NAME_LEN+1];		//extra char for terminating zero
	player	player;
	int		difficulty_level;		//which level game is played at
	int		primary_weapon;		//which weapon selected
	int		secondary_weapon;		//which weapon selected
	int		cockpit_mode;			//which cockpit mode selected
	int		window_w,window_h;	//size of player's window
	int		next_level_num;		//which level we're going to
	int		auto_leveling_on;		//does player have autoleveling on?
} __pack__ saved_game;

saved_game saved_games[N_SAVE_SLOTS];

int Default_leveling_on=1;

void init_game_list()
{
	int i;

	for (i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;
}

int new_player_config()
{
	int i,j,control_choice;
	newmenu_item m[7];

RetrySelection:
	for (i=0; i<CONTROL_MAX_TYPES; i++ )	{
		m[i].type = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i);
	}
	m[0].text = TXT_CONTROL_KEYBOARD;

	control_choice = Config_control_type;				// Assume keyboard

	control_choice = newmenu_do1( NULL, TXT_CHOOSE_INPUT, CONTROL_MAX_TYPES, m, NULL, control_choice );

	if ( control_choice < 0 )
		return 0;

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
        //added on 2/4/99 by Victor Rachels for new keys
         for(i=0;i<MAX_D1X_CONTROLS;i++)
          kconfig_d1x_settings[i] = default_kconfig_d1x_settings[i];
        //end this section addition - VR
        kc_set_controls();
        Config_control_type = control_choice;

        if ( Config_control_type==CONTROL_THRUSTMASTER_FCS)     {
//end addition/edit - Victor Rachels
                i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, TXT_FCS );
		if (i==0) goto RetrySelection;
	}

	
//        if ( (Config_control_type>0) &&  (Config_control_type<5) )       {
//                joydefs_calibrate();
//        }

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;

	memcpy(primary_order, default_primary_order, sizeof(primary_order));
	memcpy(secondary_order, default_secondary_order, sizeof(secondary_order));

	// Default taunt macros
	#ifdef NETWORK
	strcpy(Network_message_macro[0], TXT_DEF_MACRO_1);
	strcpy(Network_message_macro[1], TXT_DEF_MACRO_2);
	strcpy(Network_message_macro[2], TXT_DEF_MACRO_3);
	strcpy(Network_message_macro[3], TXT_DEF_MACRO_4);
	#endif

	return 1;
}

int read_player_d1x(const char *filename)
{
	FILE *f;
	int rc = 0;
        char *line,*word;
        int Stop=0;
        int i;
        char plxver[6];

        sprintf(plxver,"v0.00");

        //added on 10/15/98 by Victor Rachels for effeciency stuff
        plyr_read_stats();
        //end this section addition - Victor Rachels

	// set defaults for when nothing is specified
        memcpy(primary_order, default_primary_order, sizeof(primary_order));
	memcpy(secondary_order, default_secondary_order, sizeof(secondary_order));

        //added/killed on 2/4/99 by Victor Rachels for new keys
//-killed-        kconfig_settings[0][46]=255;
//-killed-        kconfig_settings[0][47]=255;
//-killed-        kconfig_settings[0][48]=255;
//-killed-        kconfig_settings[0][49]=255;
//-killed-        kconfig_settings[0][50]=255;
//-killed-        kconfig_settings[0][51]=255;

         for(i=0;i<MAX_D1X_CONTROLS;i++)
          kconfig_d1x_settings[i] = default_kconfig_d1x_settings[i];
        //end this section addition/kill - VR



        //changed on 9/16/98 by adb to fix disappearing flare key
        // change all joystick entries
        for (i = 1; i < 5; i++)
         {
          kconfig_settings[i][27]=255;
          kconfig_settings[i][28]=255;
          kconfig_settings[i][29]=255;
         }
        //end changes - adb

        f = fopen(filename, "r");
         if(!f || feof(f) ) 
          return errno;

        while( !Stop && !feof(f) )
         {
          //added on 11/04/98 by Victor Rachels to fix crappy                   
          int Screwed=0;

            if(!Screwed)
             line=fsplitword(f,'\n');
           Screwed = 0;
          //end this section addition - VR
           word=splitword(line,':');
           strupr(word);
            if (strstr(word,"PLX VERSION"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);
                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"PLX VERSION"))
                     {
                      int maj,min;
                       sscanf(line,"v%i.%i",&maj,&min);
                       sprintf(plxver,"v%i.%i",maj,min);
                     }
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"ADVANCED ORDERING"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);
                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"PRIMARY+"))
                     sscanf(line,"%d,%d,%d,%d,%d,%d,%d,%d",&primary_order[5],&primary_order[6],&primary_order[7],&primary_order[8],&primary_order[9],&primary_order[10],&primary_order[11],&primary_order[12]);
                    else if(!strcmp(word,"PRIMARY"))
                     sscanf(line,"%d,%d,%d,%d,%d",&primary_order[0], &primary_order[1], &primary_order[2], &primary_order[3], &primary_order[4]);
                    else if(!strcmp(word,"SECONDARY"))
                     sscanf(line,"%d,%d,%d,%d,%d",&secondary_order[0], &secondary_order[1], &secondary_order[2], &secondary_order[3], &secondary_order[4]);
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                    //added on 11/04/98 by Victor Rachels to fix crappy
                    if(line[0]=='['&&!strstr(line,"END"))
                     { Screwed = 1; break; }
                    //end this section addition
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"WEAPON ORDER")) //we don't want this info
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);               
             }
            else if (strstr(word,"NEWER KEYS"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);
                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"PRIMARY AUTOSELECT TOGGLE") ||
                       !strcmp(word,"SECONDARY AUTOSELECT TOGGLE") )
                     {
                      int kc1, kc2, kc3;

                       sscanf(line,"0x%x,0x%x,0x%x",&kc1,&kc2,&kc3);
//added/changed on 2/5/99 by Victor Rachels to change to d1x new keys
                        if(!strcmp(word,"PRIMARY AUTOSELECT TOGGLE"))
                         i=0;              //was 2
                        else if(!strcmp(word,"SECONDARY AUTOSELECT TOGGLE"))
                         i=2;              //was 3
//                       kconfig_settings[0][46 + i*2] = kc1;
//                       kconfig_settings[0][47 + i*2] = kc2;
                       kconfig_d1x_settings[24+i] = kc2;
                        if(kc1 != 255) kconfig_d1x_settings[24+i] = kc1;

                        if (Config_control_type != 0)
//                         kconfig_settings[Config_control_type][27 + i] = kc3;
                         kconfig_d1x_settings[25+i] = kc3;
//end this section change - VR
                     }
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"CYCLE KEYS")||strstr(word,"NEW KEYS"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                    //changed on 9/16/98 by adb to fix disappearing accelerate key
                    if(!strcmp(word,"CYCLE PRIMARY") ||
                       !strcmp(word,"CYCLE SECONDARY") ||
                    //   !strcmp(word,"PRIMARY AUTOSELECT TOGGLE") ||
                    //   !strcmp(word,"SECONDARY AUTOSELECT TOGGLE") ||
                       !strcmp(word,"AUTOSELECT TOGGLE"))
                     {
                      int kc1, kc2, kc3;
                      sscanf(line,"0x%x,0x%x,0x%x",&kc1,&kc2,&kc3);
//added/changed on 2/5/99 by Victor Rachels to change to d1x new keys
                      if (!strcmp(word,"CYCLE PRIMARY"))
                       i = 0;
                      else if (!strcmp(word,"CYCLE SECONDARY"))
                       i = 2;        //was 1
                      else if (!strcmp(word,"AUTOSELECT TOGGLE"))
                       i = 4;        //was 2
               //       else if (!strcmp(word,"PRIMARY AUTOSELECT TOGGLE"))
               //        i = 2;
               //       else if (!strcmp(word,"SECONDARY AUTOSELECT TOGGLE"))
               //        i = 3;
//                      kconfig_settings[0][46 + i * 2] = kc1;
//                      kconfig_settings[0][47 + i * 2] = kc2;
                     kconfig_d1x_settings[20+i] = kc2;
                         if(kc1 != 255) kconfig_d1x_settings[20+i] = kc1;
                      if (Config_control_type != 0)
//                          kconfig_settings[Config_control_type][27 + i] = kc3;
                       kconfig_d1x_settings[21+i] = kc3;
//end this section change - VR
                     }
                    //-killed if(!strcmp(word,"CYCLE PRIMARY"))
                    //-killed  sscanf(line,"0x%x,0x%x,0x%x",(unsigned int *)&kconfig_settings[0][46],(unsigned int *)&kconfig_settings[0][47],(unsigned int *)&kconfig_settings[Config_control_type][27]);
                    //-killed else if(!strcmp(word,"CYCLE SECONDARY"))
                    //-killed  sscanf(line,"0x%x,0x%x,0x%x",(unsigned int *)&kconfig_settings[0][48],(unsigned int *)&kconfig_settings[0][49],(unsigned int *)&kconfig_settings[Config_control_type][28]);
                    //-killed else if(!strcmp(word,"AUTOSELECT TOGGLE"))
                    //-killed  sscanf(line,"0x%x,0x%x,0x%x",(unsigned int *)&kconfig_settings[0][50],(unsigned int *)&kconfig_settings[0][51],(unsigned int *)&kconfig_settings[Config_control_type][29]);
                    //end changes - adb

                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"WEAPON KEYS"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);
                while(!strstr(word,"END") && !feof(f))
                 {
                  int kc1,kc2;
                  int i=atoi(word);
                  
                    if(i==0) i=10;
                   i=(i-1)*2;

                   sscanf(line,"0x%x,0x%x",&kc1,&kc2);
                   kconfig_d1x_settings[i]   = kc1;
                   kconfig_d1x_settings[i+1] = kc2;
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"JOYSTICK"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"DEADZONE"))
                     sscanf(line,"%i",&joy_deadzone);
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"END") || feof(f))
             {
              Stop=1;
              free(line);
             }
            else
             {
              if(word[0]=='['&&!strstr(word,"D1X OPTIONS"))
               {
                 while(!strstr(line,"END") && !feof(f))
                    {
                      free(line);
                      line=fsplitword(f,'\n');
                      strupr(line);
                    }
               }
              free(line);
             }

           if(word)
            free(word);
         }

	if (ferror(f))
                rc = errno;
        fclose(f);

	return rc;
}

//added 11/11/98 by Matthew Mueller - discourage those lamers a bit.
char effcode1[]="d1xrocks_SKCORX!D";
//char effcode1[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char effcode2[]="AObe)7Rn1 -+/zZ'0";
//char effcode2[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
char effcode3[]="aoeuidhtnAOEUIDH6";
char effcode4[]="'/.;]<{=,+?|}->[3";
unsigned char * decode_stat(unsigned char *p,int *v,char *effcode){
	unsigned char c;int neg,i,I;
//	printf("decode_stat:%s effcode:%s\n",p,effcode);
	if (p[0]==0)return NULL;
	else if (p[0]>='a'){
		neg=1;I=p[0]-'a';
	}else{
		neg=0;I=p[0]-'A';
	}
	i=0;*v=0;
	p++;
	while (p[i*2] && p[i*2+1] && p[i*2]!=' '){
		c=(p[i*2]-33)+((p[i*2+1]-33)<<4);
		c^=effcode[i+neg];
		*v+=c << (i*8);
		i++;
	}
	if (neg)
	     *v *= -1;
//	printf("decode_stat: i=%i neg=%i v=%i\n",i,neg,*v);
	if (!p[i*2])return NULL;
	return p+(i*2);
}
void plyr_read_stats_v(int *k, int *d){
  char filename[14];
  int k1=-1,k2=0,d1=-1,d2=0;
  FILE *f;

  *k=0;*d=0;//in case the file doesn't exist.
	
   sprintf(filename,"%s.eff",Players[Player_num].callsign);
   strlwr(filename);
   f=fopen(filename, "rt");

    if(f && isatty(fileno(f)))
     {
      fclose(f);
      sprintf(filename,"$%.7s.pl$",Players[Player_num].callsign);
      strlwr(filename);
      f=fopen(filename,"rt");
     }

    if(f)
     {
	char *line,*word;
	     if(!feof(f))
		 {
			 line=fsplitword(f,'\n');
			 word=splitword(line,':');
			 if(!strcmp(word,"kills"))
				*k=atoi(line);
			 free(line); free(word);
		 }
	     if(!feof(f))
                 {
			 line=fsplitword(f,'\n');
			 word=splitword(line,':');
			 if(!strcmp(word,"deaths"))
				*d=atoi(line);
			 free(line); free(word);
		 }
	     if(!feof(f))
                 {
			 line=fsplitword(f,'\n');
			 word=splitword(line,':');
			 if(!strcmp(word,"key") && strlen(line)>10){
				 unsigned char *p;
				 if (line[0]=='0' && line[1]=='1'){
					 if ((p=decode_stat(line+3,&k1,effcode1))&&
					     (p=decode_stat(p+1,&k2,effcode2))&&
					     (p=decode_stat(p+1,&d1,effcode3))){
						 decode_stat(p+1,&d2,effcode4);
					 }
				 }
			 }
			 free(line); free(word);
		 }
	     if (k1!=k2 || k1!=*k || d1!=d2 || d1!=*d){
		     *k=0;*d=0;//printf("cheater!\n");
	     }
     }

    if(f)
     fclose(f);
}
//end addition -MM


int multi_kills_stat=0;
int multi_deaths_stat=0;

//edited 11/11/98 by Matthew Mueller - some stuff cut out, some moved into above function
//added on 10/15/98 by Victor Rachels for effeciency stuff
void plyr_read_stats()
{
	plyr_read_stats_v(&multi_kills_stat,&multi_deaths_stat);
}
//end this section addition - Victor Rachels

//added on 10/15/98 by Victor Rachels to add player stats

void plyr_save_stats()
{
  int kills,deaths,neg;
  char filename[14];
  unsigned char buf[16],buf2[16],a;
  int i;
  FILE *f;
   kills=0;
   deaths=0;

  //added/edited on 11/12/98 by Victor Rachels to fix
   kills=multi_kills_stat;
   deaths=multi_deaths_stat;
  //end this section addition - VR

   sprintf(filename,"%s.eff",Players[Player_num].callsign);
   strlwr(filename);
   f=fopen(filename, "rt");

    if(f && isatty(fileno(f)))
     {
      fclose(f);
      sprintf(filename,"$%.7s.pl$",Players[Player_num].callsign);
      strlwr(filename);
      f=fopen(filename,"rt");
     }

   f=fopen(filename, "wt");
    if(!f)
     return; //broken!

   fprintf(f,"kills:%i\n",kills);
   fprintf(f,"deaths:%i\n",deaths);
	fprintf(f,"key:01 ");
	if (kills<0){
		neg=1;
		kills*=-1;
	}else neg=0;
	for (i=0;kills;i++){
		a=(kills & 0xFF) ^ effcode1[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(kills & 0xFF) ^ effcode2[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		kills>>=8;
	}
	buf[i*2]=0;
	buf2[i*2]=0;
	if (neg)i+='a';
	else i+='A';
	fprintf(f,"%c%s %c%s ",i,buf,i,buf2);

	if (deaths<0){
		neg=1;
		deaths*=-1;
	}else neg=0;
	for (i=0;deaths;i++){
		a=(deaths & 0xFF) ^ effcode3[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(deaths & 0xFF) ^ effcode4[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		deaths>>=8;
	}
	buf[i*2]=0;
	buf2[i*2]=0;
	if (neg)i+='a';
	else i+='A';
	fprintf(f,"%c%s %c%s\n",i,buf,i,buf2);
	
   fclose(f);
}
//end this section addition - Victor Rachels
//end edit -MM

// this mess tries to preserve unknown settings in the file...
int write_player_d1x(const char *filename)
{
 FILE *fin, *fout;
 int rc=0;
 int Stop=0;
 char *line;
 char tempfile[PATH_MAX];



  strcpy(tempfile,filename);
  tempfile[strlen(tempfile)-4]=0;
  strcat(tempfile,".pl$");

  fout=fopen(tempfile,"wt");

   if (fout && isatty(fileno(fout)))
    {
     //if the callsign is the name of a tty device, prepend a char
      fclose(fout);
      sprintf(tempfile,"$%.7s.pl$",Players[Player_num].callsign);
      strlwr(tempfile);
      fout = fopen(tempfile,"wt");
    }

   if(fout)
    {
      fin=fopen(filename,"rt");
       if(!fin)
        {
          fprintf(fout,"[D1X Options]\n");

          fprintf(fout,"[new keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
          fprintf(fout,"cycle primary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[20],kconfig_d1x_settings[21]);
          fprintf(fout,"cycle secondary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[22],kconfig_d1x_settings[23]);
          fprintf(fout,"autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
//          fprintf(fout,"cycle primary=0x%x,0x%x,0x%x\n",kconfig_settings[0][46],kconfig_settings[0][47],kconfig_settings[Config_control_type][27]);
//          fprintf(fout,"cycle secondary=0x%x,0x%x,0x%x\n",kconfig_settings[0][48],kconfig_settings[0][49],kconfig_settings[Config_control_type][28]);
//          fprintf(fout,"autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//end this section change - VR
          fprintf(fout,"[end]\n");

          fprintf(fout,"[joystick]\n");
          fprintf(fout,"deadzone=%i\n",joy_deadzone);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[weapon order]\n");
          fprintf(fout,"primary=1,2,3,4,5\n");
          fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[advanced ordering]\n");
          fprintf(fout,"primary=%d,%d,%d,%d,%d\n",primary_order[0], primary_order[1], primary_order[2],primary_order[3], primary_order[4]);
          fprintf(fout,"primary+=%d,%d,%d,%d,%d,%d,%d,%d\n",primary_order[5],primary_order[6],primary_order[7],primary_order[8],primary_order[9],primary_order[10],primary_order[11],primary_order[12]);
          fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[newer keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
          fprintf(fout,"primary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
          fprintf(fout,"secondary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[26],kconfig_d1x_settings[27]);
//          fprintf(fout,"primary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//          fprintf(fout,"secondary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][52],kconfig_settings[0][53],kconfig_settings[Config_control_type][30]);
//end this section change - VR
          fprintf(fout,"[end]\n");

//added on 2/5/99 by Victor Rachels for d1x new keys
          fprintf(fout,"[weapon keys]\n");
          fprintf(fout,"1=0x%x,0x%x\n",kconfig_d1x_settings[0],kconfig_d1x_settings[1]);
          fprintf(fout,"2=0x%x,0x%x\n",kconfig_d1x_settings[2],kconfig_d1x_settings[3]);
          fprintf(fout,"3=0x%x,0x%x\n",kconfig_d1x_settings[4],kconfig_d1x_settings[5]);
          fprintf(fout,"4=0x%x,0x%x\n",kconfig_d1x_settings[6],kconfig_d1x_settings[7]);
          fprintf(fout,"5=0x%x,0x%x\n",kconfig_d1x_settings[8],kconfig_d1x_settings[9]);
          fprintf(fout,"6=0x%x,0x%x\n",kconfig_d1x_settings[10],kconfig_d1x_settings[11]);
          fprintf(fout,"7=0x%x,0x%x\n",kconfig_d1x_settings[12],kconfig_d1x_settings[13]);
          fprintf(fout,"8=0x%x,0x%x\n",kconfig_d1x_settings[14],kconfig_d1x_settings[15]);
          fprintf(fout,"9=0x%x,0x%x\n",kconfig_d1x_settings[16],kconfig_d1x_settings[17]);
          fprintf(fout,"0=0x%x,0x%x\n",kconfig_d1x_settings[18],kconfig_d1x_settings[19]);
          fprintf(fout,"[end]\n");
//end this section change - VR

          fprintf(fout,"[plx version]\n");
          fprintf(fout,"plx version=%s\n",D1X_VERSION);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[end]\n");
        }
       else
        {
          int printed=0;

#define ADV_WEAPON_ORDER 1
#define NEW_KEYS 2
#define WEAPON_KEYS 4
#define JOYSTICK 8
#define NEWER_KEYS 16
#define WEAPON_ORDER 32

           while(!Stop && !feof(fin))
            {
              line=fsplitword(fin,'\n');
              strupr(line);

               if (strstr(line,"PLX VERSION")) // we don't want to keep this
                {
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                     free(line);
                     line=fsplitword(fin,'\n');
                     strupr(line);
                    }
                  free(line);
                }
               else if (strstr(line,"WEAPON ORDER"))
                {
                  fprintf(fout,"[weapon order]\n");
                  fprintf(fout,"primary=1,2,3,4,5\n");
                  fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
                  fprintf(fout,"[end]\n");

                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= WEAPON_ORDER;
                }
//added on 2/5/99 by Victor Rachels for d1x new keys
               else if (strstr(line,"WEAPON KEYS"))
                {
                  fprintf(fout,"[weapon keys]\n");
                  fprintf(fout,"1=0x%x,0x%x\n",kconfig_d1x_settings[0],kconfig_d1x_settings[1]);
                  fprintf(fout,"2=0x%x,0x%x\n",kconfig_d1x_settings[2],kconfig_d1x_settings[3]);
                  fprintf(fout,"3=0x%x,0x%x\n",kconfig_d1x_settings[4],kconfig_d1x_settings[5]);
                  fprintf(fout,"4=0x%x,0x%x\n",kconfig_d1x_settings[6],kconfig_d1x_settings[7]);
                  fprintf(fout,"5=0x%x,0x%x\n",kconfig_d1x_settings[8],kconfig_d1x_settings[9]);
                  fprintf(fout,"6=0x%x,0x%x\n",kconfig_d1x_settings[10],kconfig_d1x_settings[11]);
                  fprintf(fout,"7=0x%x,0x%x\n",kconfig_d1x_settings[12],kconfig_d1x_settings[13]);
                  fprintf(fout,"8=0x%x,0x%x\n",kconfig_d1x_settings[14],kconfig_d1x_settings[15]);
                  fprintf(fout,"9=0x%x,0x%x\n",kconfig_d1x_settings[16],kconfig_d1x_settings[17]);
                  fprintf(fout,"0=0x%x,0x%x\n",kconfig_d1x_settings[18],kconfig_d1x_settings[19]);
                  fprintf(fout,"[end]\n");

                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= WEAPON_KEYS;
                }
//end this section addition - VR
               else if (strstr(line,"ADVANCED ORDERING"))
                {
                  fprintf(fout,"[advanced ordering]\n");
                  fprintf(fout,"primary=%d,%d,%d,%d,%d\n",primary_order[0], primary_order[1], primary_order[2],primary_order[3], primary_order[4]);
                  fprintf(fout,"primary+=%d,%d,%d,%d,%d,%d,%d,%d\n",primary_order[5],primary_order[6],primary_order[7],primary_order[8],primary_order[9],primary_order[10],primary_order[11],primary_order[12]);
                  fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
                  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= ADV_WEAPON_ORDER;
                }
               else if (strstr(line,"CYCLE KEYS")||strstr(line,"NEW KEYS"))
                {
                  fprintf(fout,"[new keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
                  fprintf(fout,"cycle primary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[20],kconfig_d1x_settings[21]);
                  fprintf(fout,"cycle secondary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[22],kconfig_d1x_settings[23]);
                  fprintf(fout,"autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
//                  fprintf(fout,"cycle primary=0x%x,0x%x,0x%0x\n",kconfig_settings[0][46],kconfig_settings[0][47],kconfig_settings[Config_control_type][27]);
//                  fprintf(fout,"cycle secondary=0x%x,0x%x,0x%x\n",kconfig_settings[0][48],kconfig_settings[0][49],kconfig_settings[Config_control_type][28]);
//                  fprintf(fout,"autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//end this section addition - VR
                  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= NEW_KEYS;
                }
               else if (strstr(line,"NEWER KEYS"))
                {
                  fprintf(fout,"[newer keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
                  fprintf(fout,"primary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
                  fprintf(fout,"secondary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[26],kconfig_d1x_settings[27]);
//                  fprintf(fout,"primary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//                  fprintf(fout,"secondary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][52],kconfig_settings[0][53],kconfig_settings[Config_control_type][30]);
//end this section addition - VR
                  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= NEWER_KEYS;
                }
               else if (strstr(line,"JOYSTICK"))
                {
                  fprintf(fout,"[joystick]\n");
                  fprintf(fout,"deadzone=%i\n",joy_deadzone);
                  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= JOYSTICK;
                }
               else if (strstr(line,"END"))
                {
                  Stop=1;
                  free(line);
                }
               else
                {
                  if(line[0]=='['&&!strstr(line,"D1X OPTIONS"))
                   while(!strstr(line,"END") && !feof(fin))
                    {
                      fprintf(fout,"%s\n",line);
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  fprintf(fout,"%s\n",line);
                  free(line);
                }
             
               if(!Stop&&feof(fin))
                Stop=1;
            }

           if(!(printed&NEW_KEYS))
            {
              fprintf(fout,"[new keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
              fprintf(fout,"cycle primary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[20],kconfig_d1x_settings[21]);
              fprintf(fout,"cycle secondary=0x%x,0xff,0x%x\n",kconfig_d1x_settings[22],kconfig_d1x_settings[23]);
              fprintf(fout,"autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
//              fprintf(fout,"cycle primary=0x%x,0x%x,0x%x\n",kconfig_settings[0][46],kconfig_settings[0][47],kconfig_settings[Config_control_type][27]);
//              fprintf(fout,"cycle secondary=0x%x,0x%x,0x%x\n",kconfig_settings[0][48],kconfig_settings[0][49],kconfig_settings[Config_control_type][28]);
//              fprintf(fout,"autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//end this section addition - VR
              fprintf(fout,"[end]\n");
            }
           if(!(printed&JOYSTICK))
            {
              fprintf(fout,"[joystick]\n");
              fprintf(fout,"deadzone=%i\n",joy_deadzone);
              fprintf(fout,"[end]\n");
            }
           if(!(printed&WEAPON_ORDER))
            {
              fprintf(fout,"[weapon order]\n");
              fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
              fprintf(fout,"[end]\n");
            }
           if(!(printed&ADV_WEAPON_ORDER))
            {
              fprintf(fout,"[advanced ordering]\n");
              fprintf(fout,"primary=%d,%d,%d,%d,%d\n",primary_order[0], primary_order[1], primary_order[2],primary_order[3],primary_order[4]);
              fprintf(fout,"primary+=%d,%d,%d,%d,%d,%d,%d,%d\n",primary_order[5],primary_order[6],primary_order[7],primary_order[8],primary_order[9],primary_order[10],primary_order[11],primary_order[12]);
              fprintf(fout,"secondary=%d,%d,%d,%d,%d\n",secondary_order[0], secondary_order[1], secondary_order[2],secondary_order[3], secondary_order[4]);
              fprintf(fout,"[end]\n");
            }
           if(!(printed&NEWER_KEYS))
            {
              fprintf(fout,"[newer keys]\n");
//added/changed on 2/5/99 by Victor Rachels for d1x new keys
              fprintf(fout,"primary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[24],kconfig_d1x_settings[25]);
              fprintf(fout,"secondary autoselect toggle=0x%x,0xff,0x%x\n",kconfig_d1x_settings[26],kconfig_d1x_settings[27]);
//              fprintf(fout,"primary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][50],kconfig_settings[0][51],kconfig_settings[Config_control_type][29]);
//              fprintf(fout,"secondary autoselect toggle=0x%x,0x%x,0x%x\n",kconfig_settings[0][52],kconfig_settings[0][53],kconfig_settings[Config_control_type][30]);
//end this section addition - VR
              fprintf(fout,"[end]\n");
            }
//added on 2/5/99 by Victor Rachels for d1x new keys
           if(!(printed&WEAPON_KEYS))
            {
              fprintf(fout,"[weapon keys]\n");
              fprintf(fout,"1=0x%x,0x%x\n",kconfig_d1x_settings[0],kconfig_d1x_settings[1]);
              fprintf(fout,"2=0x%x,0x%x\n",kconfig_d1x_settings[2],kconfig_d1x_settings[3]);
              fprintf(fout,"3=0x%x,0x%x\n",kconfig_d1x_settings[4],kconfig_d1x_settings[5]);
              fprintf(fout,"4=0x%x,0x%x\n",kconfig_d1x_settings[6],kconfig_d1x_settings[7]);
              fprintf(fout,"5=0x%x,0x%x\n",kconfig_d1x_settings[8],kconfig_d1x_settings[9]);
              fprintf(fout,"6=0x%x,0x%x\n",kconfig_d1x_settings[10],kconfig_d1x_settings[11]);
              fprintf(fout,"7=0x%x,0x%x\n",kconfig_d1x_settings[12],kconfig_d1x_settings[13]);
              fprintf(fout,"8=0x%x,0x%x\n",kconfig_d1x_settings[14],kconfig_d1x_settings[15]);
              fprintf(fout,"9=0x%x,0x%x\n",kconfig_d1x_settings[16],kconfig_d1x_settings[17]);
              fprintf(fout,"0=0x%x,0x%x\n",kconfig_d1x_settings[18],kconfig_d1x_settings[19]);
              fprintf(fout,"[end]\n");
            }
//end this section addition - VR

          fprintf(fout,"[plx version]\n");
          fprintf(fout,"plx version=%s\n",D1X_VERSION);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[end]\n");

          fclose(fin);
        }

       if (ferror(fout))
        rc = errno;
      fclose(fout);
       if(rc==0)
        {
         unlink(filename);
         rc = rename(tempfile,filename);
        }
      return rc;
    }
   else
    return errno;

}

	extern int screen_width;
	extern int screen_height;

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[13];
	FILE *file;
	save_info info;
	int errno_ret = EZERO;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	// adb: I hope that this prevents the missing weapon ordering bug
	memcpy(primary_order, default_primary_order, sizeof(primary_order));
        memcpy(secondary_order, default_secondary_order, sizeof(secondary_order));

	sprintf(filename,"%s.plr",Players[Player_num].callsign);
        strlwr(filename);
	file = fopen(filename,"rb");

	//check filename
	if (file && isatty(fileno(file))) {
		//if the callsign is the name of a tty device, prepend a char
		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
                strlwr(filename);
		file = fopen(filename,"rb");
	}

	if (!file) {
		return errno;
	}

	if (fread(&info,sizeof(info),1,file) != 1) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	if (info.id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		fclose(file);
		return -1;
	}

	if (info.saved_game_version<COMPATIBLE_SAVED_GAME_VERSION || info.player_struct_version<COMPATIBLE_PLAYER_STRUCT_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		fclose(file);
		return -1;
	}

	if (info.saved_game_version <= 5) {

		//deal with old-style highest level info

		n_highest_levels = 1;

		highest_levels[0].shortname[0] = 0;							//no name for mission 0
		highest_levels[0].level_num = info.n_highest_levels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (info.n_highest_levels==7)
			highest_levels[0].level_num = 8;
		
	}
	else {	//read new highest level info

		n_highest_levels = info.n_highest_levels;

		if (fread(highest_levels,sizeof(hli),n_highest_levels,file) != n_highest_levels) {
			errno_ret = errno;
			fclose(file);
			return errno_ret;
		}
	}

	Player_default_difficulty = info.default_difficulty_level;
	Default_leveling_on = info.default_leveling_on;

	if ( info.saved_game_version != 7 ) {	// Read old & SW saved games.
		if (fread(saved_games,sizeof(saved_games),1,file) != 1) {
			errno_ret = errno;
			fclose(file);
			return errno_ret;
		}
	}

	//read taunt macros
	{
		int i,len;

		#ifdef SHAREWARE
		len = MAX_MESSAGE_LEN;
		#else
		len = (info.saved_game_version == 4)?SHAREWARE_MAX_MESSAGE_LEN:MAX_MESSAGE_LEN;
		#endif

		#ifdef NETWORK
		for (i = 0; i < 4; i++)
			if (fread(Network_message_macro[i], len, 1, file) != 1)
				{errno_ret = errno; break;}
		#else
		i = 0;
		fseek( file, 4*len, SEEK_CUR );
		#endif
	}



	//read kconfig data
	{
        //        if (fread( kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1, file )!=1)
        //                errno_ret=errno;
                int i,j;
                 for(i=0;i<CONTROL_MAX_TYPES;i++)
                  for(j=0;j<MAX_NOND1X_CONTROLS;j++)
                   if(fread( &kconfig_settings[i][j], sizeof(ubyte),1,file)!=1)
                    errno_ret=errno;
                if(errno_ret == EZERO)
                {
                if (fread(&Config_control_type, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fread(&Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
                }
	}

	if (fclose(file) && errno_ret==EZERO)
		errno_ret = errno;

#ifndef SHAREWARE
	if ( info.saved_game_version == COMPATIBLE_SAVED_GAME_VERSION ) 	{
		int i;
		
		Assert( N_SAVE_SLOTS == 10 );

		for (i=0; i<N_SAVE_SLOTS; i++ )	{
			if ( saved_games[i].name[0] )	{
				state_save_old_game(i, saved_games[i].name, &saved_games[i].player, 
             		saved_games[i].difficulty_level, saved_games[i].primary_weapon, 
          			saved_games[i].secondary_weapon, saved_games[i].next_level_num );
			}
		}
		write_player_file();
	}
#endif

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
        strlwr(filename);
	read_player_d1x(filename);

         {
          int i;
          highest_primary=0;
          highest_secondary=0;
           for(i=0; i<MAX_PRIMARY_WEAPONS+NEWPRIMS; i++)
            if(primary_order[i]>0)
              highest_primary++;
           for(i=0; i<MAX_SECONDARY_WEAPONS+NEWSECS; i++)
            if(secondary_order[i]>0)
             highest_secondary++;
         }

	if (errno_ret==EZERO)	{
		kc_set_controls();
	}

	// ZICO - also set VR_render to saved resolution because it won't get screwed up by any screen changes like window shrink etc.
	VR_render_width = info.VR_render_w;
	VR_render_height = info.VR_render_h;
	Game_window_w = /*VR_render_width =*/ info.Game_window_w;
	Game_window_h = /*VR_render_height =*/ info.Game_window_h;

	return errno_ret;

}

//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int find_hli_entry()
{
#ifdef SHAREWARE
	return 0;
#else
	int i;

	for (i=0;i<n_highest_levels;i++)
		if (!strcasecmp(highest_levels[i].shortname,Mission_list[Current_mission_num].filename))
			break;

	if (i==n_highest_levels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			n_highest_levels++;

		strcpy(highest_levels[i].shortname,Mission_list[Current_mission_num].filename);
		highest_levels[i].level_num = 0;
	}

	return i;
#endif
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret,i;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i = find_hli_entry();

	if (levelnum > highest_levels[i].level_num)
		highest_levels[i].level_num = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();
#ifndef SHAREWARE
#ifndef DEST_SAT
	if (strlen(Mission_list[Current_mission_num].filename)==0 )	{
		for (i=0;i<n_highest_levels;i++)
			if (!strcasecmp(highest_levels[i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 		highest_saturn_level = highest_levels[i].level_num; 
	}
#endif
#endif
   i = highest_levels[find_hli_entry()].level_num;
	if ( highest_saturn_level > i )
   	i = highest_saturn_level;
	return i;
}


//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	char filename[13];
	FILE *file;
	save_info info;
	int errno_ret;

	errno_ret = WriteConfigFile();

	info.id = SAVE_FILE_ID;
	info.saved_game_version = SAVED_GAME_VERSION;
	info.player_struct_version = PLAYER_STRUCT_VERSION;
	info.default_difficulty_level = Player_default_difficulty;
	info.default_leveling_on = Auto_leveling_on;
	info.VR_render_w = VR_render_width;/*Game_window_w*/;
	info.VR_render_h = VR_render_height;/*Game_window_h*/;
	info.Game_window_w = Game_window_w;
	info.Game_window_h = Game_window_h;

	info.n_highest_levels = n_highest_levels;

        sprintf(filename,"%s.plx",Players[Player_num].callsign);
        strlwr(filename);
	write_player_d1x(filename);

	sprintf(filename,"%s.plr",Players[Player_num].callsign);
        strlwr(filename);
	file = fopen(filename,"wb");

	//check filename
	if (file && isatty(fileno(file))) {

		//if the callsign is the name of a tty device, prepend a char

		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
                strlwr(filename);
		file = fopen(filename,"wb");
	}

	if (!file)
		return errno;

	errno_ret = EZERO;

	if (fwrite(&info,sizeof(info),1,file) != 1) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	//write higest level info
	if ((fwrite(highest_levels, sizeof(hli), n_highest_levels, file) != n_highest_levels)) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	if (fwrite(saved_games,sizeof(saved_games),1,file) != 1) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	#ifdef NETWORK
	if ((fwrite(Network_message_macro, MAX_MESSAGE_LEN, 4, file) != 4)) {
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}
	#else
	fseek( file, MAX_MESSAGE_LEN * 4, SEEK_CUR );
	#endif

	//write kconfig info
	{
//                if (fwrite( kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1, file )!=1)
//                        errno_ret=errno;
                int i,j;
                 for(i=0;i<CONTROL_MAX_TYPES;i++)
                  for(j=0;j<MAX_NOND1X_CONTROLS;j++)
                   if(fwrite( &kconfig_settings[i][j], sizeof(kconfig_settings[i][j]), 1, file)!=1)
                    errno_ret=errno;

                if(errno_ret == EZERO)
                {
                 if (fwrite( &Config_control_type, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
                 else if (fwrite( &Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
                }
	}

	if (fclose(file))
		errno_ret = errno;

	if (errno_ret != EZERO) {
		remove(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}

	return errno_ret;

}

//returns errno (0 == no error)
int save_player_game(int slot_num,char *text)
{
	int ret;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	Assert(slot_num < N_SAVE_SLOTS);

	strcpy(saved_games[slot_num].name,text);

	saved_games[slot_num].player = Players[Player_num];

	saved_games[slot_num].difficulty_level	= Difficulty_level;
	saved_games[slot_num].auto_leveling_on	= Auto_leveling_on;
	saved_games[slot_num].primary_weapon	= Primary_weapon;
	saved_games[slot_num].secondary_weapon	= Secondary_weapon;
	saved_games[slot_num].cockpit_mode		= Cockpit_mode;
	saved_games[slot_num].window_w			= Game_window_w;
	saved_games[slot_num].window_h			= Game_window_h;
	saved_games[slot_num].next_level_num	= Next_level_num;

	return write_player_file();
}


//returns errno (0 == no error)
int load_player_game(int slot_num)
{
	char save_callsign[CALLSIGN_LEN+1];
	int ret;

	Assert(slot_num < N_SAVE_SLOTS);

	if ((ret=read_player_file()) != EZERO)
		return ret;

	Assert(saved_games[slot_num].name[0] != 0);

	strcpy(save_callsign,Players[Player_num].callsign);
	Players[Player_num] = saved_games[slot_num].player;
	strcpy(Players[Player_num].callsign,save_callsign);

	Difficulty_level	= saved_games[slot_num].difficulty_level;
	Auto_leveling_on	= saved_games[slot_num].auto_leveling_on;
	Primary_weapon		= saved_games[slot_num].primary_weapon;
	Secondary_weapon	= saved_games[slot_num].secondary_weapon;
	Cockpit_mode		= saved_games[slot_num].cockpit_mode;
	Game_window_w		= saved_games[slot_num].window_w;
	Game_window_h		= saved_games[slot_num].window_h;

	Players[Player_num].level = saved_games[slot_num].next_level_num;

	return EZERO;
}

//fills in a list of pointers to strings describing saved games
//returns the number of non-empty slots
//returns -1 if this is a new player
int get_game_list(char *game_text[N_SAVE_SLOTS])
{
	int i,count,ret;

	ret = read_player_file();

	for (i=count=0;i<N_SAVE_SLOTS;i++) {
		if (game_text)
			game_text[i] = saved_games[i].name;

		if (saved_games[i].name[0])
			count++;
	}

	return (ret==EZERO)?count:-1;		//-1 means new file was created

}

//update the player's highest level.  returns errno (0 == no error)
int update_player_file()
{
	int ret;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	return write_player_file();
}

// returns 1 if player exists (.plr file exists), 0 otherwise
int player_exists(const char *callsign)
{
	char filename[14];
	FILE *fp;

	sprintf(filename, "%s.plr", callsign);

	fp = fopen(filename, "rb");

        //if the callsign is the name of a tty device, prepend a char
        if (fp && isatty(fileno(fp))) {
                fclose(fp);
		sprintf(filename, "$%.7s.plr", callsign );
		fp = fopen(filename, "rb");
        }
        
        if ( fp )       {
                fclose(fp);
		return 1;
        }
	return 0;
}
