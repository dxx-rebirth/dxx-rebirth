/* $Id: playsave.c,v 1.13 2003-06-06 19:04:27 btb Exp $ */
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
 * Functions to load & save player games
 *
 * Old Log:
 * Revision 1.1  1995/12/05  16:05:47  allender
 * Initial revision
 *
 * Revision 1.10  1995/11/03  12:53:24  allender
 * shareware changes
 *
 * Revision 1.9  1995/10/31  10:19:12  allender
 * shareware stuff
 *
 * Revision 1.8  1995/10/23  14:50:11  allender
 * set control type for new player *before* calling kc_set_controls
 *
 * Revision 1.7  1995/10/21  22:25:31  allender
 * *** empty log message ***
 *
 * Revision 1.6  1995/10/17  15:57:42  allender
 * removed line setting wrong COnfig_control_type
 *
 * Revision 1.5  1995/10/17  13:16:44  allender
 * new controller support
 *
 * Revision 1.4  1995/08/24  16:03:38  allender
 * call joystick code when player file uses joystick
 *
 * Revision 1.3  1995/08/03  15:15:39  allender
 * got player save file working (more to go for shareware)
 *
 * Revision 1.2  1995/08/01  13:57:20  allender
 * macified the player file stuff -- in a seperate folder
 *
 * Revision 1.1  1995/05/16  15:30:00  allender
 * Initial revision
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
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#include <mmsystem.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "error.h"

#include "pa_enabl.h"
#include "strutil.h"
#include "game.h"
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
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "fileutil.h"
#include "escort.h"

#define SAVE_FILE_ID			MAKE_SIG('D','P','L','R')

#ifdef MACINTOSH
	#include <Files.h>
	#include <Errors.h>			// mac doesn't have "normal" error numbers -- must use mac equivs
	#ifndef ENOENT
		#define ENOENT fnfErr
	#endif
	#ifdef POLY_ACC
		#include "poly_acc.h"
	#endif
	#include "isp.h"
#endif

int get_lifetime_checksum (int a,int b);

typedef struct hli {
	char	shortname[9];
	ubyte	level_num;
} hli;

short n_highest_levels;

hli highest_levels[MAX_MISSIONS];

#define PLAYER_FILE_VERSION	25			//increment this every time the player file changes

//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_struct_version
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added kconfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats 
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.
//version 24 -> 25: add d2x keys array

#define COMPATIBLE_PLAYER_FILE_VERSION          17


int Default_leveling_on=1;
extern ubyte SecondaryOrder[],PrimaryOrder[];
extern void InitWeaponOrdering();

#ifdef MACINTOSH
extern ubyte default_firebird_settings[];
extern ubyte default_mousestick_settings[];
#endif

int new_player_config()
{
	int nitems;
	int i,j,control_choice;
	newmenu_item m[8];
   int mct=CONTROL_MAX_TYPES;
 
   #ifndef WINDOWS
 	 mct--;
	#endif

   InitWeaponOrdering ();		//setup default weapon priorities 

#if defined(MACINTOSH) && defined(USE_ISP)
	if (!ISpEnabled())
	{
#endif
RetrySelection:
		#if !defined(MACINTOSH) && !defined(WINDOWS)
		for (i=0; i<mct; i++ )	{
			m[i].type = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i);
		}
		#elif defined(WINDOWS)
			m[0].type = NM_TYPE_MENU; m[0].text = CONTROL_TEXT(0);
	 		m[1].type = NM_TYPE_MENU; m[1].text = CONTROL_TEXT(5);
			m[2].type = NM_TYPE_MENU; m[2].text = CONTROL_TEXT(7);
		 	i = 3;
		#else
		for (i = 0; i < 6; i++) {
			m[i].type = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i);
		}
		m[4].text = "Gravis Firebird/Mousetick II";
		m[3].text = "Thrustmaster";
		#endif
		
		nitems = i;
		m[0].text = TXT_CONTROL_KEYBOARD;
	
		#ifdef WINDOWS
			if (Config_control_type==CONTROL_NONE) control_choice = 0;
			else if (Config_control_type == CONTROL_MOUSE) control_choice = 1;
			else if (Config_control_type == CONTROL_WINJOYSTICK) control_choice = 2;
			else control_choice = 0;
		#else
			control_choice = Config_control_type;				// Assume keyboard
		#endif
	
		#ifndef APPLE_DEMO
			control_choice = newmenu_do1( NULL, TXT_CHOOSE_INPUT, i, m, NULL, control_choice );
		#else
			control_choice = 0;
		#endif
		
		if ( control_choice < 0 )
			return 0;

#if defined(MACINTOSH) && defined(USE_ISP)
	}
	else	// !!!!NOTE ... link to above if (!ISpEnabled()), this is a really crappy function
	{
		control_choice = 0;
	}
#endif

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
	//added on 2/4/99 by Victor Rachels for new keys
	for(i=0; i < MAX_D2X_CONTROLS; i++)
		kconfig_d2x_settings[i] = default_kconfig_d2x_settings[i];
	//end this section addition - VR
	kc_set_controls();

	Config_control_type = control_choice;

#ifdef WINDOWS
	if (control_choice == 1) Config_control_type = CONTROL_MOUSE;
	else if (control_choice == 2) Config_control_type = CONTROL_WINJOYSTICK;

//	if (Config_control_type == CONTROL_WINJOYSTICK) 
//		joydefs_calibrate();
#else
	#ifndef MACINTOSH
	if ( Config_control_type==CONTROL_THRUSTMASTER_FCS)	{
		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, TXT_FCS );
		if (i==0) goto RetrySelection;
	}
	
	if ( (Config_control_type>0) && 	(Config_control_type<5))	{
		joydefs_calibrate();
	}
	#else		// some macintosh only stuff here
	if ( Config_control_type==CONTROL_THRUSTMASTER_FCS)	{
		extern char *tm_warning;
		
		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, tm_warning );
		if (i==0) goto RetrySelection;
	} else 	if ( Config_control_type==CONTROL_FLIGHTSTICK_PRO )	{
		extern char *ch_warning;

		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, ch_warning );
		if (i==0) goto RetrySelection;
	} else 	if ( Config_control_type==CONTROL_GRAVIS_GAMEPAD )	{
		extern char *ms_warning;

		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, ms_warning );
		if (i==0) goto RetrySelection;
		// stupid me -- get real default setting for either mousestick or firebird
		joydefs_set_type( Config_control_type );
		if (joy_have_firebird())
			for (i=0; i<NUM_OTHER_CONTROLS; i++ )
				kconfig_settings[Config_control_type][i] = default_firebird_settings[i];
		else
			for (i=0; i<NUM_OTHER_CONTROLS; i++ )
				kconfig_settings[Config_control_type][i] = default_mousestick_settings[i];
		kc_set_controls();		// reset the joystick control
	}
	if ( (Config_control_type>0) && (Config_control_type<5)  ) {
		joydefs_set_type( Config_control_type );
		joydefs_calibrate();
	}

	#endif
#endif
	
	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;
	Cockpit_3d_view[0]=CV_NONE;
	Cockpit_3d_view[1]=CV_NONE;

	// Default taunt macros
	#ifdef NETWORK
	strcpy(Network_message_macro[0], "Why can't we all just get along?");
	strcpy(Network_message_macro[1], "Hey, I got a present for ya");
	strcpy(Network_message_macro[2], "I got a hankerin' for a spankerin'");
	strcpy(Network_message_macro[3], "This one's headed for Uranus");
	Netlife_kills=0; Netlife_killed=0;	
	#endif
	
	#ifdef MACINTOSH
		#ifdef POLY_ACC
			if (PAEnabled)
			{
				Scanline_double = 0;		// no pixel doubling for poly_acc
			}
			else
			{
				Scanline_double = 1;		// should be default for new player
			}
		#else
			Scanline_double = 1;			// should be default for new player
		#endif
	#endif

	return 1;
}

extern int Guided_in_big_window,Automap_always_hires;

//this length must match the value in escort.c
#define GUIDEBOT_NAME_LEN 9
extern char guidebot_name[];
extern char real_guidebot_name[];

WIN(extern char win95_current_joyname[]);

ubyte control_type_dos,control_type_win;

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	#ifdef MACINTOSH
	char filename[FILENAME_LEN+15];
	#else
	char filename[FILENAME_LEN];
	#endif
	FILE *file;
	int errno_ret = EZERO;
	int id, i;
	short player_file_version;
	int rewrite_it=0;
	int swap = 0;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

#ifndef MACINTOSH
	sprintf(filename,"%.8s.plr",Players[Player_num].callsign);
#else
	sprintf(filename, ":Players:%.8s.plr",Players[Player_num].callsign);
#endif
	file = fopen(filename,"rb");

#ifndef MACINTOSH
	//check filename
	if (file && isatty(fileno(file))) {
		//if the callsign is the name of a tty device, prepend a char
		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
		file = fopen(filename,"rb");
	}
#endif

	if (!file) {
		return errno;
	}

	id = file_read_int(file);

        // SWAPINT added here because old versions of d2x
        // used the wrong byte order.
	if (id!=SAVE_FILE_ID && id!=SWAPINT(SAVE_FILE_ID)) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		fclose(file);
		return -1;
	}

	player_file_version = file_read_short(file);

	if (player_file_version > 255) // bigendian file?
		swap = 1;

	if (swap)
		player_file_version = SWAPSHORT(player_file_version);

	if (player_file_version<COMPATIBLE_PLAYER_FILE_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		fclose(file);
		return -1;
	}

	Game_window_w					= file_read_short(file);
	Game_window_h					= file_read_short(file);

	if (swap) {
		Game_window_w = SWAPSHORT(Game_window_w);
		Game_window_h = SWAPSHORT(Game_window_h);
	}

	Player_default_difficulty	= file_read_byte(file);
	Default_leveling_on			= file_read_byte(file);
	Reticle_on						= file_read_byte(file);
	Cockpit_mode					= file_read_byte(file);
	#ifdef POLY_ACC
	 #ifdef PA_3DFX_VOODOO
		if (Cockpit_mode<2)
		{
	   	Cockpit_mode=2;
			Game_window_w  = 640;
			Game_window_h	= 480;
		}
	 #endif
	#endif
 
	Default_display_mode			= file_read_byte(file);
	Missile_view_enabled			= file_read_byte(file);
	Headlight_active_default	= file_read_byte(file);
	Guided_in_big_window			= file_read_byte(file);

	if (player_file_version >= 19)
		Automap_always_hires			= file_read_byte(file);
          
	Auto_leveling_on = Default_leveling_on;

	//read new highest level info

	n_highest_levels = file_read_short(file);
	if (swap)
		n_highest_levels = SWAPSHORT(n_highest_levels);

	if (fread(highest_levels,sizeof(hli),n_highest_levels,file) != n_highest_levels) {
		errno_ret			= errno;
		fclose(file);
		return errno_ret;
	}

	//read taunt macros
	{
#ifdef NETWORK
		int i,len;

		len			= MAX_MESSAGE_LEN;

		for (i			= 0; i < 4; i++)
			if (fread(Network_message_macro[i], len, 1, file) != 1)
				{errno_ret			= errno; break;}
#else
		char dummy[4][MAX_MESSAGE_LEN];
		fread(dummy, MAX_MESSAGE_LEN, 4, file);

#endif
	}

	//read kconfig data
	{
		int n_control_types = (player_file_version<20)?7:CONTROL_MAX_TYPES;

		if (fread( kconfig_settings, MAX_CONTROLS*n_control_types, 1, file )!=1)
			errno_ret=errno;
		else if (fread((ubyte *)&control_type_dos, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (player_file_version >= 21 && fread((ubyte *)&control_type_win, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fread(&Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;

		#ifdef WINDOWS
		Config_control_type = control_type_win;
		#else
		Config_control_type = control_type_dos;
		#endif
		
		#ifdef MACINTOSH
		joydefs_set_type(Config_control_type);
		#endif

		for (i=0;i<11;i++)
		 {
			PrimaryOrder[i]=file_read_byte (file);
			SecondaryOrder[i]=file_read_byte(file);
		 }

		if (player_file_version>=16)
		 {
		  Cockpit_3d_view[0]=file_read_int(file);
		  Cockpit_3d_view[1]=file_read_int(file);
		  if (swap) {
			  Cockpit_3d_view[0] = SWAPINT(Cockpit_3d_view[0]);
			  Cockpit_3d_view[1] = SWAPINT(Cockpit_3d_view[1]);
		  }
		 }	
		
                  
		if (errno_ret==EZERO)	{
			kc_set_controls();
		}

	}

   if (player_file_version>=22)
	 {
#ifdef NETWORK
		Netlife_kills=file_read_int (file);
		Netlife_killed=file_read_int (file);
		if (swap) {
			Netlife_kills = SWAPINT(Netlife_kills);
			Netlife_killed = SWAPINT(Netlife_killed);
		}
#else
		file_read_int(file); file_read_int(file);
#endif
	 }
#ifdef NETWORK
   else
	 {
		 Netlife_kills=0; Netlife_killed=0;
	 }
#endif

	if (player_file_version>=23)
	 {
	  i=file_read_int (file);	
	  if (swap)
		  i = SWAPINT(i);
#ifdef NETWORK
	  mprintf ((0,"Reading: lifetime checksum is %d\n",i));
	  if (i!=get_lifetime_checksum (Netlife_kills,Netlife_killed))
		{
		 Netlife_kills=0; Netlife_killed=0;
 		 nm_messagebox(NULL, 1, "Shame on me", "Trying to cheat eh?");
		 rewrite_it=1;
		}
#endif
	 }

	//read guidebot name
	if (player_file_version >= 18)
		file_read_string(guidebot_name,file);
	else
		strcpy(guidebot_name,"GUIDE-BOT");

	strcpy(real_guidebot_name,guidebot_name);

	{
		char buf[128];

	#ifdef WINDOWS
		joy95_get_name(JOYSTICKID1, buf, 127);
		if (player_file_version >= 24) 
			file_read_string(win95_current_joyname, file);
		else
			strcpy(win95_current_joyname, "Old Player File");
		
		mprintf((0, "Detected joystick: %s\n", buf));
		mprintf((0, "Player's joystick: %s\n", win95_current_joyname));

		if (strcmp(win95_current_joyname, buf)) {
			for (i = 0; i < MAX_CONTROLS; i++)
				kconfig_settings[CONTROL_WINJOYSTICK][i] = 
					default_kconfig_settings[CONTROL_WINJOYSTICK][i];
		}	 
	#else
		if (player_file_version >= 24) 
			file_read_string(buf, file);			// Just read it in fpr DPS.
	#endif
	}

	if (player_file_version >= 25)
		fread(kconfig_d2x_settings, MAX_D2X_CONTROLS, 1, file);
	else
		for(i=0; i < MAX_D2X_CONTROLS; i++)
			kconfig_d2x_settings[i] = default_kconfig_d2x_settings[i];

	if (fclose(file) && errno_ret==EZERO)
		errno_ret			= errno;

	if (rewrite_it)
	 write_player_file();

	return errno_ret;

}


//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int find_hli_entry()
{
	int i;

	for (i=0;i<n_highest_levels;i++)
		if (!stricmp(highest_levels[i].shortname,Mission_list[Current_mission_num].filename))
			break;

	if (i==n_highest_levels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			n_highest_levels++;

		strcpy(highest_levels[i].shortname,Mission_list[Current_mission_num].filename);
		highest_levels[i].level_num			= 0;
	}

	return i;
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret,i;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i			= find_hli_entry();

	if (levelnum > highest_levels[i].level_num)
		highest_levels[i].level_num			= levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level			= 0;
	read_player_file();
#ifndef SATURN
	if (strlen(Mission_list[Current_mission_num].filename)==0 )	{
		for (i=0;i<n_highest_levels;i++)
			if (!stricmp(highest_levels[i].shortname, "DESTSAT")) 	//	Destination Saturn.
		 		highest_saturn_level			= highest_levels[i].level_num; 
	}
#endif
   i			= highest_levels[find_hli_entry()].level_num;
	if ( highest_saturn_level > i )
   	i			= highest_saturn_level;
	return i;
}

extern int Cockpit_mode_save;

//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	#ifdef MACINTOSH
	char filename[FILENAME_LEN+15];
	#else
	char filename[FILENAME_LEN];		// because of ":Players:" path
	#endif
	FILE *file;
	int errno_ret,i;

//	#ifdef APPLE_DEMO		// no saving of player files in Apple OEM version
//	return 0;
//	#endif

	errno_ret			= WriteConfigFile();

#ifndef MACINTOSH
	sprintf(filename,"%s.plr",Players[Player_num].callsign);
#else
	sprintf(filename, ":Players:%.8s.plr",Players[Player_num].callsign);
#endif
	file			= fopen(filename,"wb");

#ifndef MACINTOSH
	//check filename
	if (file && isatty(fileno(file))) {

		//if the callsign is the name of a tty device, prepend a char

		fclose(file);
		sprintf(filename,"$%.7s.plr",Players[Player_num].callsign);
		file			= fopen(filename,"wb");
	}
#endif

	if (!file)
              return errno;

	errno_ret			= EZERO;

	//Write out player's info
	file_write_int(SAVE_FILE_ID,file);
	file_write_short(PLAYER_FILE_VERSION,file);

	file_write_short(Game_window_w,file);
	file_write_short(Game_window_h,file);

	file_write_byte(Player_default_difficulty,file);
	file_write_byte(Auto_leveling_on,file);
	file_write_byte(Reticle_on,file);
	file_write_byte((Cockpit_mode_save!=-1)?Cockpit_mode_save:Cockpit_mode,file);	//if have saved mode, write it instead of letterbox/rear view
	file_write_byte(Default_display_mode,file);
	file_write_byte(Missile_view_enabled,file);
	file_write_byte(Headlight_active_default,file);
	file_write_byte(Guided_in_big_window,file);
	file_write_byte(Automap_always_hires,file);

	//write higest level info
	file_write_short(n_highest_levels,file);
	if ((fwrite(highest_levels, sizeof(hli), n_highest_levels, file) != n_highest_levels)) {
		errno_ret			= errno;
		fclose(file);
		return errno_ret;
	}

	#ifdef NETWORK
	if ((fwrite(Network_message_macro, MAX_MESSAGE_LEN, 4, file) != 4)) {
		errno_ret			= errno;
		fclose(file);
		return errno_ret;
	}
	#else
	fseek( file, MAX_MESSAGE_LEN * 4, SEEK_CUR );
	#endif

	//write kconfig info
	{

		#ifdef WINDOWS
		control_type_win = Config_control_type;
		#else
		control_type_dos = Config_control_type;
		#endif

		if (fwrite( kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1, file )!=1)
			errno_ret=errno;
		else if (fwrite( &control_type_dos, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fwrite( &control_type_win, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;
		else if (fwrite( &Config_joystick_sensitivity, sizeof(ubyte), 1, file )!=1)
			errno_ret=errno;

      for (i=0;i<11;i++)
       {
        fwrite (&PrimaryOrder[i],sizeof(ubyte),1,file);
        fwrite (&SecondaryOrder[i],sizeof(ubyte),1,file);
       }

		file_write_int (Cockpit_3d_view[0],file);
		file_write_int (Cockpit_3d_view[1],file);

#ifdef NETWORK
		file_write_int (Netlife_kills,file);
		file_write_int (Netlife_killed,file);
		i=get_lifetime_checksum (Netlife_kills,Netlife_killed);
		mprintf ((0,"Writing: Lifetime checksum is %d\n",i));
#else
	   file_write_int(0, file);
	   file_write_int(0, file);
	   i = get_lifetime_checksum (0, 0);
#endif
	   file_write_int (i,file);
	}

	//write guidebot name
	file_write_string(real_guidebot_name,file);

	{
		char buf[128];
		#ifdef WINDOWS
		joy95_get_name(JOYSTICKID1, buf, 127);
		#else
		strcpy(buf, "DOS joystick");
		#endif
		file_write_string(buf, file);		// Write out current joystick for player.
	}

	fwrite(kconfig_d2x_settings, MAX_D2X_CONTROLS, 1, file);

	if (fclose(file))
		errno_ret			= errno;

	if (errno_ret != EZERO) {
		remove(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}

	#ifdef MACINTOSH		// set filetype and creator for playerfile
	{
		FInfo finfo;
		Str255 pfilename;
		OSErr err;

		strcpy(pfilename, filename);
		c2pstr(pfilename);
		err = HGetFInfo(0, 0, pfilename, &finfo);
		finfo.fdType = 'PLYR';
		finfo.fdCreator = 'DCT2';
		err = HSetFInfo(0, 0, pfilename, &finfo);
	}
	#endif

	return errno_ret;

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

int get_lifetime_checksum (int a,int b)
 {
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
 }
  

