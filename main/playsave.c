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
 * $Source: /cvs/cvsroot/d2x/main/playsave.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-11-11 23:39:22 $
 *
 * Functions to load & save player games
 *
 * $Log: not supported by cvs2svn $
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

int n_highest_levels;

hli highest_levels[MAX_MISSIONS];

#define PLAYER_FILE_VERSION	24			//increment this every time the player file changes

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

static int read_int(FILE *file)
{
	int i;

	if (fread( &i, sizeof(i), 1, file) != 1)
		Error( "Error reading int in gamesave.c" );

	return i;
}

static short read_short(FILE *file)
{
	short s;

	if (fread( &s, sizeof(s), 1, file) != 1)
		Error( "Error reading short in gamesave.c" );

	return s;
}

static byte read_byte(FILE *file)
{
	byte s;

	if (fread( &s, sizeof(s), 1, file) != 1)
		Error( "Error reading byte in gamesave.c" );

	return s;
}

static void write_int(int i,FILE *file)
{
	if (fwrite( &i, sizeof(i), 1, file) != 1)
		Error( "Error writing int in gamesave.c" );

}

static void write_short(short s,FILE *file)
{
	if (fwrite( &s, sizeof(s), 1, file) != 1)
		Error( "Error writing short in gamesave.c" );

}

static void write_byte(byte i,FILE *file)
{
	if (fwrite( &i, sizeof(i), 1, file) != 1)
		Error( "Error writing byte in gamesave.c" );

}

extern int Guided_in_big_window,Automap_always_hires;

//this length must match the value in escort.c
#define GUIDEBOT_NAME_LEN 9
extern char guidebot_name[];
extern char real_guidebot_name[];

WIN(extern char win95_current_joyname[]);

void read_string(char *s, FILE *f)
{
	if (feof(f))
		*s = 0;
	else
		do
			*s = fgetc(f);
		while (!feof(f) && *s++!=0);
}

void write_string(char *s, FILE *f)
{
	do
		fputc(*s,f);
	while (*s++!=0);
}

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
	int id,player_file_version,i;
	int rewrite_it=0;

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

	id = read_int(file);

	if (id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		fclose(file);
		return -1;
	}

	player_file_version = read_short(file);

	if (player_file_version<COMPATIBLE_PLAYER_FILE_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		fclose(file);
		return -1;
	}

	Game_window_w					= read_short(file);
	Game_window_h					= read_short(file);

	Player_default_difficulty	= read_byte(file);
	Default_leveling_on			= read_byte(file);
	Reticle_on						= read_byte(file);
	Cockpit_mode					= read_byte(file);
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
 
	Default_display_mode			= read_byte(file);
	Missile_view_enabled			= read_byte(file);
	Headlight_active_default	= read_byte(file);
	Guided_in_big_window			= read_byte(file);

	if (player_file_version >= 19)
		Automap_always_hires			= read_byte(file);
          
	Auto_leveling_on = Default_leveling_on;

	//read new highest level info

	n_highest_levels = read_short(file);
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
			PrimaryOrder[i]=read_byte (file);
			SecondaryOrder[i]=read_byte(file);
		 }
		
		if (player_file_version>=16)
		 {
		  Cockpit_3d_view[0]=read_int(file);
		  Cockpit_3d_view[1]=read_int(file);
		 }	
		
                  
		if (errno_ret==EZERO)	{
			kc_set_controls();
		}

	}

   if (player_file_version>=22)
	 {
#ifdef NETWORK
		Netlife_kills=read_int (file);
		Netlife_killed=read_int (file);
#else
		read_int(file); read_int(file);
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
	  i=read_int (file);	
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
		read_string(guidebot_name,file);
	else
		strcpy(guidebot_name,"GUIDE-BOT");

	strcpy(real_guidebot_name,guidebot_name);

	{
		char buf[128];

	#ifdef WINDOWS
		joy95_get_name(JOYSTICKID1, buf, 127);
		if (player_file_version >= 24) 
			read_string(win95_current_joyname, file);
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
			read_string(buf, file);			// Just read it in fpr DPS.
	#endif
	}

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
	write_int(SAVE_FILE_ID,file);
	write_short(PLAYER_FILE_VERSION,file);

	write_short(Game_window_w,file);
	write_short(Game_window_h,file);

	write_byte(Player_default_difficulty,file);
	write_byte(Auto_leveling_on,file);
	write_byte(Reticle_on,file);
	write_byte((Cockpit_mode_save!=-1)?Cockpit_mode_save:Cockpit_mode,file);	//if have saved mode, write it instead of letterbox/rear view
	write_byte(Default_display_mode,file);
	write_byte(Missile_view_enabled,file);
	write_byte(Headlight_active_default,file);
	write_byte(Guided_in_big_window,file);
	write_byte(Automap_always_hires,file);

	//write higest level info
	write_short(n_highest_levels,file);
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
		write_int (Cockpit_3d_view[0],file);
		write_int (Cockpit_3d_view[1],file);

#ifdef NETWORK
		write_int (Netlife_kills,file);
	   write_int (Netlife_killed,file);
		i=get_lifetime_checksum (Netlife_kills,Netlife_killed);
		mprintf ((0,"Writing: Lifetime checksum is %d\n",i));
#else
	   write_int(0, file);
	   write_int(0, file);
	   i = get_lifetime_checksum (0, 0);
#endif
	   write_int (i,file);
	}

	//write guidebot name
	write_string(real_guidebot_name,file);

	{
		char buf[128];
		#ifdef WINDOWS
		joy95_get_name(JOYSTICKID1, buf, 127);
		#else
		strcpy(buf, "DOS joystick");
		#endif
		write_string(buf, file);		// Write out current joystick for player.
	}

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
  

