/* $Id: playsave.c,v 1.24 2005/07/30 01:50:17 chris Exp $ */
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
 * Functions to load & save player's settings (*.plr file)
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "error.h"

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
#include "escort.h"
#include "u_mem.h"
#include "strio.h"
#include "physfsx.h"
#include "vers_id.h"

#define SAVE_FILE_ID			MAKE_SIG('D','P','L','R')

#if defined(_WIN32_WCE)
# define errno -1
# define ENOENT -1
# define strerror(x) "Unknown Error"
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
#define WINDOWSIZE 1
#define RESOLUTION 2
#define MOUSE_SENSITIVITY 4
#define JOY_DEADZONE 8

int Default_leveling_on=1;
extern ubyte SecondaryOrder[],PrimaryOrder[];
extern void InitWeaponOrdering();

static int Player_render_width = 0;
static int Player_render_height = 0;

int new_player_config()
{
	int i,j,control_choice;
	int mct=CONTROL_MAX_TYPES;
 
	mct--;

	InitWeaponOrdering ();		//setup default weapon priorities 

	control_choice = Config_control_type;	// Assume keyboard

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
	for(i=0; i < MAX_D2X_CONTROLS; i++)
		kconfig_d2x_settings[i] = default_kconfig_d2x_settings[i];
	kc_set_controls();

	Config_control_type = control_choice;

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;
	Config_mouse_sensitivity = 8;
        joy_deadzone = 0;
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
	
	return 1;
}

int read_player_d2x(char *filename)
{
	PHYSFS_file *f;
	int rc = 0;
	char line[20],*word;
	int Stop=0;
	char plxver[6];

	sprintf(plxver,"v0.00");

	f = PHYSFSX_openReadBuffered(filename);

	if(!f || PHYSFS_eof(f))
		return errno;

	while(!Stop && !PHYSFS_eof(f))
	{
		cfgets(line,20,f);
		word=splitword(line,':');
		strupr(word);
		if (strstr(word,"PLX VERSION"))
		{
			d_free(word);
			cfgets(line,20,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"PLX VERSION"))
				{
					int maj=0,min=0;
					sscanf(line,"%i.%i",&maj,&min);
					sprintf(plxver,"v%i.%d",maj,min);
				}
				d_free(word);
				cfgets(line,20,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"RESOLUTION"))
		{
			d_free(word);
			cfgets(line,20,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"WIDTH"))
					sscanf(line,"%i",&Player_render_width);
				if(!strcmp(word,"HEIGHT"))
					sscanf(line,"%i",&Player_render_height);
				d_free(word);
				cfgets(line,20,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"MOUSE"))
		{
			d_free(word);
			cfgets(line,20,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY"))
				{
					int tmp;
					sscanf(line,"%i",&tmp);
					Config_mouse_sensitivity = (ubyte) tmp;
				}
				d_free(word);
				cfgets(line,20,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			cfgets(line,20,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"DEADZONE"))
				{
					int tmp;
					sscanf(line,"%d",&tmp);
					joy_deadzone = tmp;
				}
				d_free(word);
				cfgets(line,20,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"END") || PHYSFS_eof(f))
		{
			Stop=1;
		}
		else
		{
			if(word[0]=='['&&!strstr(word,"D2X OPTIONS"))
			{
				while(!strstr(line,"END") && !PHYSFS_eof(f))
				{
					cfgets(line,20,f);
					strupr(line);
				}
			}
		}

		if(word)
			d_free(word);
	}

	PHYSFS_close(f);

	return rc;
}

int write_player_d2x(char *filename)
{
	PHYSFS_file *fin, *fout;
	int rc=0;
	int Stop=0;
	char line[20];
	char tempfile[PATH_MAX];
	char str[256];

	strcpy(tempfile,filename);
	tempfile[strlen(tempfile)-4]=0;
	strcat(tempfile,".pl$");
	fout=PHYSFSX_openWriteBuffered(tempfile);
	
	if (!fout && GameArg.SysUsePlayersDir)
	{
		PHYSFS_mkdir("Players/");	//try making directory
		fout=PHYSFSX_openWriteBuffered(tempfile);
	}
	
	if(fout)
	{
		fin=PHYSFSX_openReadBuffered(filename);
		if(!fin)
		{
			sprintf (str, "[D2X OPTIONS]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "[resolution]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "width=%i\n", SM_W(Game_screen_mode));
			PHYSFSX_puts(fout, str);
			sprintf (str, "height=%d\n", SM_H(Game_screen_mode));
			PHYSFSX_puts(fout, str);
			sprintf (str, "[end]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "[mouse]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "sensitivity=%d\n",Config_mouse_sensitivity);
			PHYSFSX_puts(fout, str);
			sprintf (str, "[end]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "[joystick]\n");
                        PHYSFSX_puts(fout, str);
                        sprintf (str, "deadzone=%d\n", joy_deadzone);
                        PHYSFSX_puts(fout, str);
                        sprintf (str, "[end]\n");
                        PHYSFSX_puts(fout, str);
			sprintf (str, "[plx version]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "plx version=%s\n", VERSION);
			PHYSFSX_puts(fout, str);
			sprintf (str, "[end]\n");
			PHYSFSX_puts(fout, str);
			sprintf (str, "[end]\n");
			PHYSFSX_puts(fout, str);
		}
		else
		{
			int printed=0;
		
			while(!Stop && !PHYSFS_eof(fin))
			{
				cfgets(line,20,fin);
				strupr(line);
				if (strstr(line,"PLX VERSION"))
				{
					while(!strstr(line,"END")&&!PHYSFS_eof(fin))
					{
						cfgets(line,20,fin);
						strupr(line);
					}
				}
				else if (strstr(line,"RESOLUTION"))
				{
					sprintf (str, "[resolution]\n");
					PHYSFSX_puts(fout, str);
					sprintf (str, "width=%i\n", SM_W(Game_screen_mode));
					PHYSFSX_puts(fout, str);
					sprintf (str, "height=%d\n", SM_H(Game_screen_mode));
					PHYSFSX_puts(fout, str);
					sprintf (str, "[end]\n");
					PHYSFSX_puts(fout, str);
					while(!strstr(line,"END")&&!PHYSFS_eof(fin))
					{
						cfgets(line,20,fin);
						strupr(line);
					}
					printed |= RESOLUTION;
				}
				else if (strstr(line,"MOUSE"))
				{
					sprintf (str, "[mouse]\n");
					PHYSFSX_puts(fout, str);
					sprintf (str, "sensitivity=%d\n",Config_mouse_sensitivity);
					PHYSFSX_puts(fout, str);
					sprintf (str, "[end]\n");
					PHYSFSX_puts(fout, str);
					while(!strstr(line,"END")&&!PHYSFS_eof(fin))
					{
						cfgets(line,20,fin);
						strupr(line);
					}
					printed |= MOUSE_SENSITIVITY;
				}
				else if (strstr(line,"JOYSTICK"))
                                {
					sprintf (str, "[joystick]\n");
                        		PHYSFSX_puts(fout, str);
                        		sprintf (str, "deadzone=%d\n", joy_deadzone);
                        		PHYSFSX_puts(fout, str);
                        		sprintf (str, "[end]\n");
                        		PHYSFSX_puts(fout, str);
					while(!strstr(line,"END")&&!PHYSFS_eof(fin))
					{
						cfgets(line,20,fin);
						strupr(line);
					}
					printed |= JOY_DEADZONE;
          			}
				else if (strstr(line,"END"))
				{
					Stop=1;
				}
				else
				{
					if(line[0]=='['&&!strstr(line,"D2X OPTIONS"))
						while(!strstr(line,"END") && !PHYSFS_eof(fin))
						{
							sprintf (str, "%s\n", line);
							PHYSFSX_puts(fout, str);
							cfgets(line,20,fin);
							strupr(line);
						}
					sprintf (str, "%s\n", line);
					PHYSFSX_puts(fout, str);
				}
			
				if(!Stop&&PHYSFS_eof(fin))
					Stop=1;
			}
		
			if(!(printed&RESOLUTION))
			{
				sprintf (str, "[resolution]\n");
				PHYSFSX_puts(fout, str);
				sprintf (str, "width=%i\n", SM_W(Game_screen_mode));
				PHYSFSX_puts(fout, str);
				sprintf (str, "height=%d\n", SM_H(Game_screen_mode));
				PHYSFSX_puts(fout, str);
				sprintf (str, "[end]\n");
				PHYSFSX_puts(fout, str);
			}
			if(!(printed&MOUSE_SENSITIVITY))
			{
				sprintf (str, "[mouse]\n");
				PHYSFSX_puts(fout, str);
				sprintf (str, "sensitivity=%d\n",Config_mouse_sensitivity);
				PHYSFSX_puts(fout, str);
				sprintf (str, "[end]\n");
				PHYSFSX_puts(fout, str);
			}
			if(!(printed&JOY_DEADZONE))
			{
				sprintf (str, "[joystick]\n");
				PHYSFSX_puts(fout, str);
				sprintf (str, "deadzone=%d\n",joy_deadzone);
				PHYSFSX_puts(fout, str);
				sprintf (str, "[end]\n");
				PHYSFSX_puts(fout, str);
			}
		
				sprintf (str, "[plx version]\n");
				PHYSFSX_puts(fout, str);
				sprintf (str, "plx version=%s\n", VERSION);
				PHYSFSX_puts(fout, str);
				sprintf (str, "[end]\n");
				PHYSFSX_puts(fout, str);
				sprintf (str, "[end]\n");
				PHYSFSX_puts(fout, str);
			
				PHYSFS_close(fin);
		}
		
		PHYSFS_close(fout);
		if(rc==0)
		{
			PHYSFS_delete(filename);
			rc = PHYSFSX_rename(tempfile,filename);
		}
		return rc;
	}
	else
		return errno;

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
	char filename[FILENAME_LEN];
	PHYSFS_file *file;
	int id, i;
	short player_file_version;
	int rewrite_it=0;
	int swap = 0;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	if (!PHYSFS_exists(filename))
		return ENOENT;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		goto read_player_file_failed;

	PHYSFS_readSLE32(file, &id);

	if (id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		PHYSFS_close(file);
		return -1;
	}

	player_file_version = cfile_read_short(file);

	if (player_file_version > 255) // bigendian file?
		swap = 1;

	if (swap)
		player_file_version = SWAPSHORT(player_file_version);

	if (player_file_version<COMPATIBLE_PLAYER_FILE_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		PHYSFS_close(file);
		return -1;
	}

	//skip Game_window_w,Game_window_h
	PHYSFS_seek(file,PHYSFS_tell(file)+2*sizeof(short));

	Player_default_difficulty = cfile_read_byte(file);
	Default_leveling_on       = cfile_read_byte(file);
	Reticle_on                = cfile_read_byte(file);
	Cockpit_mode              = cfile_read_byte(file);
	Default_display_mode      = cfile_read_byte(file);
	Missile_view_enabled      = cfile_read_byte(file);
	Headlight_active_default  = cfile_read_byte(file);
	Guided_in_big_window      = cfile_read_byte(file);
		
	if (player_file_version >= 19)
		Automap_always_hires = cfile_read_byte(file);

	Auto_leveling_on = Default_leveling_on;

	//read new highest level info

	n_highest_levels = cfile_read_short(file);
	if (swap)
		n_highest_levels = SWAPSHORT(n_highest_levels);
	Assert(n_highest_levels <= MAX_MISSIONS);

	if (PHYSFS_read(file, highest_levels, sizeof(hli), n_highest_levels) != n_highest_levels)
		goto read_player_file_failed;

	//read taunt macros
	{
#ifdef NETWORK
		int i,len;

		len = MAX_MESSAGE_LEN;

		for (i = 0; i < 4; i++)
			if (PHYSFS_read(file, Network_message_macro[i], len, 1) != 1)
				goto read_player_file_failed;
#else
		char dummy[4][MAX_MESSAGE_LEN];

		cfread(dummy, MAX_MESSAGE_LEN, 4, file);
#endif
	}

	//read kconfig data
	{
		int n_control_types = (player_file_version<20)?7:CONTROL_MAX_TYPES;

		if (PHYSFS_read(file, kconfig_settings, MAX_CONTROLS*n_control_types, 1) != 1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, (ubyte *)&control_type_dos, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (player_file_version >= 21 && PHYSFS_read(file, (ubyte *)&control_type_win, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &Config_joystick_sensitivity, sizeof(ubyte), 1) !=1 )
			goto read_player_file_failed;

		Config_control_type = control_type_dos;
	
		for (i=0;i<11;i++)
		{
			PrimaryOrder[i] = cfile_read_byte(file);
			SecondaryOrder[i] = cfile_read_byte(file);
		}

		if (player_file_version>=16)
		{
			PHYSFS_readSLE32(file, &Cockpit_3d_view[0]);
			PHYSFS_readSLE32(file, &Cockpit_3d_view[1]);
			if (swap)
			{
				Cockpit_3d_view[0] = SWAPINT(Cockpit_3d_view[0]);
				Cockpit_3d_view[1] = SWAPINT(Cockpit_3d_view[1]);
			}
		}

		kc_set_controls();

	}

	if (player_file_version>=22)
	{
#ifdef NETWORK
		PHYSFS_readSLE32(file, &Netlife_kills);
		PHYSFS_readSLE32(file, &Netlife_killed);
		if (swap) {
			Netlife_kills = SWAPINT(Netlife_kills);
			Netlife_killed = SWAPINT(Netlife_killed);
		}
#else
		{
			int dummy;
			PHYSFS_readSLE32(file, &dummy);
			PHYSFS_readSLE32(file, &dummy);
		}
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
		PHYSFS_readSLE32(file, &i);
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
		PHYSFSX_readString(file, guidebot_name);
	else
		strcpy(guidebot_name,"GUIDE-BOT");

	strcpy(real_guidebot_name,guidebot_name);

	{
		char buf[128];

		if (player_file_version >= 24) 
			PHYSFSX_readString(file, buf);			// Just read it in fpr DPS.
	}

	if (player_file_version >= 25)
		PHYSFS_read(file, kconfig_d2x_settings, MAX_D2X_CONTROLS, 1);
	else
		for(i=0; i < MAX_D2X_CONTROLS; i++)
			kconfig_d2x_settings[i] = default_kconfig_d2x_settings[i];

	if (!PHYSFS_close(file))
		goto read_player_file_failed;

	if (rewrite_it)
		write_player_file();

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
        strlwr(filename);
	read_player_d2x(filename);

	if (Player_render_width && Player_render_height && Game_screen_mode != SM(Player_render_width, Player_render_height))
	{
		Game_screen_mode = SM(Player_render_width,Player_render_height);
		game_init_render_buffers(
			Player_render_width,
			Player_render_height, VR_NONE, 0);
	}

	return EZERO;

 read_player_file_failed:
	nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s", "Error reading PLR file", PHYSFS_getLastError());
	if (file)
		PHYSFS_close(file);

	return -1;
}


//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int find_hli_entry()
{
	int i;

	for (i=0;i<n_highest_levels;i++)
		if (!stricmp(highest_levels[i].shortname, Current_mission_filename))
			break;

	if (i==n_highest_levels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			n_highest_levels++;

		strcpy(highest_levels[i].shortname, Current_mission_filename);
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
	if (strlen(Current_mission_filename)==0 )	{
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
	char filename[FILENAME_LEN];
	PHYSFS_file *file;
	int i;

	WriteConfigFile();

	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", Players[Player_num].callsign);
	strlwr(filename);
	write_player_d2x(filename);
	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return -1;

	//Write out player's info
	PHYSFS_writeULE32(file, SAVE_FILE_ID);
	PHYSFS_writeULE16(file, PLAYER_FILE_VERSION);

	// skip Game_window_w, Game_window_h
	PHYSFS_seek(file,PHYSFS_tell(file)+2*(sizeof(PHYSFS_uint16)));

	PHYSFSX_writeU8(file, Player_default_difficulty);
	PHYSFSX_writeU8(file, Auto_leveling_on);
	PHYSFSX_writeU8(file, Reticle_on);
	PHYSFSX_writeU8(file, (Cockpit_mode_save!=-1)?Cockpit_mode_save:Cockpit_mode);	//if have saved mode, write it instead of letterbox/rear view
	PHYSFSX_writeU8(file, Default_display_mode);
	PHYSFSX_writeU8(file, Missile_view_enabled);
	PHYSFSX_writeU8(file, Headlight_active_default);
	PHYSFSX_writeU8(file, Guided_in_big_window);
	PHYSFSX_writeU8(file, Automap_always_hires);

	//write higest level info
	PHYSFS_writeULE16(file, n_highest_levels);
	if ((PHYSFS_write(file, highest_levels, sizeof(hli), n_highest_levels) != n_highest_levels))
		goto write_player_file_failed;

#ifdef NETWORK
	if ((PHYSFS_write(file, Network_message_macro, MAX_MESSAGE_LEN, 4) != 4))
		goto write_player_file_failed;
#else
	{
		char dummy[4][MAX_MESSAGE_LEN];	// Pull the messages from a hat! ;-)

		if ((PHYSFS_write(file, dummy, MAX_MESSAGE_LEN, 4) != 4))
			goto write_player_file_failed;
	}
#endif

	//write kconfig info
	{

		control_type_dos = Config_control_type;

		if (PHYSFS_write(file, kconfig_settings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &control_type_win, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &Config_joystick_sensitivity, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;

		for (i = 0; i < 11; i++)
		{
			PHYSFS_write(file, &PrimaryOrder[i], sizeof(ubyte), 1);
			PHYSFS_write(file, &SecondaryOrder[i], sizeof(ubyte), 1);
		}

		PHYSFS_writeULE32(file, Cockpit_3d_view[0]);
		PHYSFS_writeULE32(file, Cockpit_3d_view[1]);

#ifdef NETWORK
		PHYSFS_writeULE32(file, Netlife_kills);
		PHYSFS_writeULE32(file, Netlife_killed);
		i=get_lifetime_checksum (Netlife_kills,Netlife_killed);
		mprintf ((0,"Writing: Lifetime checksum is %d\n",i));
#else
		PHYSFS_writeULE32(file, 0);
		PHYSFS_writeULE32(file, 0);
		i = get_lifetime_checksum (0, 0);
#endif
		PHYSFS_writeULE32(file, i);
	}

	//write guidebot name
	PHYSFSX_writeString(file, real_guidebot_name);

	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		PHYSFSX_writeString(file, buf);		// Write out current joystick for player.
	}

	PHYSFS_write(file, kconfig_d2x_settings, MAX_D2X_CONTROLS, 1);

	if (!PHYSFS_close(file))
		goto write_player_file_failed;

	return EZERO;

 write_player_file_failed:
	nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s", TXT_ERROR_WRITING_PLR, PHYSFS_getLastError());
	if (file)
	{
		PHYSFS_close(file);
		PHYSFS_delete(filename);        //delete bogus file
	}

	return -1;
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
