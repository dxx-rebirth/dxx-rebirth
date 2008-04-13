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
#include "digi.h"
#include "newmenu.h"
#include "palette.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "state.h"
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "u_mem.h"
#include "strio.h"
#include "physfsx.h"
#include "vers_id.h"
#include "newdemo.h"

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

#define SAVE_FILE_ID MAKE_SIG('D','P','L','R')
#define PLAYER_FILE_VERSION 25 //increment this every time the player file changes
#define COMPATIBLE_PLAYER_FILE_VERSION 17

struct player_config PlayerCfg;
int get_lifetime_checksum (int a,int b);
extern void InitWeaponOrdering();

int new_player_config()
{
	int i,j;
	int mct=CONTROL_MAX_TYPES;
 
	mct--;

	PlayerCfg.ControlType=CONTROL_NONE; // Assume keyboard
	InitWeaponOrdering (); //setup default weapon prioritiess

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			PlayerCfg.KeySettings[i][j] = DefaultKeySettings[i][j];
	for(i=0; i < MAX_D2X_CONTROLS; i++)
		PlayerCfg.KeySettingsD2X[i] = DefaultKeySettingsD2X[i];
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = 1;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	PlayerCfg.JoystickSensitivity = 8;
	PlayerCfg.MouseSensitivity = 8;
        PlayerCfg.JoystickDeadzone = 0;
	PlayerCfg.CockpitMode = CM_FULL_COCKPIT;
	PlayerCfg.Cockpit3DView[0]=CV_NONE;
	PlayerCfg.Cockpit3DView[1]=CV_NONE;
	PlayerCfg.ReticleOn = 1;
	PlayerCfg.MissileViewEnabled = 1;
	PlayerCfg.HeadlightActiveDefault = 1;
	PlayerCfg.GuidedInBigWindow = 0;
	strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");
	strcpy(PlayerCfg.GuidebotNameReal,"GUIDE-BOT");
	PlayerCfg.HudMode = 0;
	PlayerCfg.EscortHotKeys = 1;
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.OglAlphaEffects = 0;
	PlayerCfg.OglReticle = 0;
	PlayerCfg.OglPRShot = 0;

	// Default taunt macros
	#ifdef NETWORK
	strcpy(PlayerCfg.NetworkMessageMacro[0], "Why can't we all just get along?");
	strcpy(PlayerCfg.NetworkMessageMacro[1], "Hey, I got a present for ya");
	strcpy(PlayerCfg.NetworkMessageMacro[2], "I got a hankerin' for a spankerin'");
	strcpy(PlayerCfg.NetworkMessageMacro[3], "This one's headed for Uranus");
	PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	#endif
	
	return 1;
}

int read_player_d2x(char *filename)
{
	PHYSFS_file *f;
	int rc = 0;
	char line[20],*word;
	int Stop=0;

	f = PHYSFSX_openReadBuffered(filename);

	if(!f || PHYSFS_eof(f))
		return errno;

	while(!Stop && !PHYSFS_eof(f))
	{
		cfgets(line,50,f);
		word=splitword(line,':');
		strupr(word);
		if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"DEADZONE"))
					sscanf(line,"%i",&PlayerCfg.JoystickDeadzone);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"MOUSE"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY"))
				{
					int tmp;
					sscanf(line,"%i",&tmp);
					PlayerCfg.MouseSensitivity = (ubyte) tmp;
				}
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"COCKPIT"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"HUD"))
					sscanf(line,"%i",&PlayerCfg.HudMode);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"TOGGLES"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"ESCORTHOTKEYS"))
					sscanf(line,"%i",&PlayerCfg.EscortHotKeys);
				else if(!strcmp(word,"PERSISTENTDEBRIS"))
					sscanf(line,"%i",&PlayerCfg.PersistentDebris);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"OPENGL"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"OGLALPHAEFFECTS"))
					sscanf(line,"%i",&PlayerCfg.OglAlphaEffects);
				else if(!strcmp(word,"OGLRETICLE"))
					sscanf(line,"%i",&PlayerCfg.OglReticle);
				else if(!strcmp(word,"OGLPRSHOT"))
					sscanf(line,"%i",&PlayerCfg.OglPRShot);
				d_free(word);
				cfgets(line,50,f);
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
					cfgets(line,50,f);
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
	PHYSFS_file *fout;
	int rc=0;
	char tempfile[PATH_MAX];

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
		PHYSFSX_printf(fout,"[D2X OPTIONS]\n");
		PHYSFSX_printf(fout,"[mouse]\n");
		PHYSFSX_printf(fout,"sensitivity=%d\n",PlayerCfg.MouseSensitivity);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[joystick]\n");
		PHYSFSX_printf(fout,"deadzone=%d\n", PlayerCfg.JoystickDeadzone);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cockpit]\n");
		PHYSFSX_printf(fout,"hud=%i\n",PlayerCfg.HudMode);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[toggles]\n");
		PHYSFSX_printf(fout,"escorthotkeys=%i\n",PlayerCfg.EscortHotKeys);
		PHYSFSX_printf(fout,"persistentdebris=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[opengl]\n");
		PHYSFSX_printf(fout,"oglaplhaeffects=%i\n",PlayerCfg.OglAlphaEffects);
		PHYSFSX_printf(fout,"oglreticle=%i\n",PlayerCfg.OglReticle);
		PHYSFSX_printf(fout,"oglprshot=%i\n",PlayerCfg.OglPRShot);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[plx version]\n");
		PHYSFSX_printf(fout,"plx version=%s\n", VERSION);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[end]\n");
		
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

ubyte control_type_dos,control_type_win;

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[32];
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

	new_player_config(); // Set defaults!

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

	PHYSFS_seek(file,PHYSFS_tell(file)+2*sizeof(short)); //skip Game_window_w,Game_window_h
	PlayerCfg.DefaultDifficulty = cfile_read_byte(file);
	PlayerCfg.AutoLeveling       = cfile_read_byte(file);
	PlayerCfg.ReticleOn                = cfile_read_byte(file);
	PlayerCfg.CockpitMode              = cfile_read_byte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Default_display_mode
	PlayerCfg.MissileViewEnabled      = cfile_read_byte(file);
	PlayerCfg.HeadlightActiveDefault  = cfile_read_byte(file);
	PlayerCfg.GuidedInBigWindow      = cfile_read_byte(file);
	if (player_file_version >= 19)
		PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Automap_always_hires

	//read new highest level info

	PlayerCfg.NHighestLevels = cfile_read_short(file);
	if (swap)
		PlayerCfg.NHighestLevels = SWAPSHORT(PlayerCfg.NHighestLevels);
	Assert(PlayerCfg.NHighestLevels <= MAX_MISSIONS);

	if (PHYSFS_read(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
		goto read_player_file_failed;

	//read taunt macros
	{
#ifdef NETWORK
		int i,len;

		len = MAX_MESSAGE_LEN;

		for (i = 0; i < 4; i++)
			if (PHYSFS_read(file, PlayerCfg.NetworkMessageMacro[i], len, 1) != 1)
				goto read_player_file_failed;
#else
		char dummy[4][MAX_MESSAGE_LEN];

		cfread(dummy, MAX_MESSAGE_LEN, 4, file);
#endif
	}

	//read kconfig data
	{
		int n_control_types = (player_file_version<20)?7:CONTROL_MAX_TYPES;

		if (PHYSFS_read(file, PlayerCfg.KeySettings, MAX_CONTROLS*n_control_types, 1) != 1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, (ubyte *)&control_type_dos, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (player_file_version >= 21 && PHYSFS_read(file, (ubyte *)&control_type_win, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &PlayerCfg.JoystickSensitivity, sizeof(ubyte), 1) !=1 )
			goto read_player_file_failed;

		PlayerCfg.ControlType = control_type_dos;
	
		for (i=0;i<11;i++)
		{
			PlayerCfg.PrimaryOrder[i] = cfile_read_byte(file);
			PlayerCfg.SecondaryOrder[i] = cfile_read_byte(file);
		}

		if (player_file_version>=16)
		{
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[0]);
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[1]);
			if (swap)
			{
				PlayerCfg.Cockpit3DView[0] = SWAPINT(PlayerCfg.Cockpit3DView[0]);
				PlayerCfg.Cockpit3DView[1] = SWAPINT(PlayerCfg.Cockpit3DView[1]);
			}
		}

		kc_set_controls();

	}

	if (player_file_version>=22)
	{
#ifdef NETWORK
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKills);
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKilled);
		if (swap) {
			PlayerCfg.NetlifeKills = SWAPINT(PlayerCfg.NetlifeKills);
			PlayerCfg.NetlifeKilled = SWAPINT(PlayerCfg.NetlifeKilled);
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
		PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	}
#endif

	if (player_file_version>=23)
	{
		PHYSFS_readSLE32(file, &i);
		if (swap)
			i = SWAPINT(i);
#ifdef NETWORK
		if (i!=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled))
		{
			PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
			nm_messagebox(NULL, 1, "Shame on me", "Trying to cheat eh?");
			rewrite_it=1;
		}
#endif
	}

	//read guidebot name
	if (player_file_version >= 18)
		PHYSFSX_readString(file, PlayerCfg.GuidebotName);
	else
		strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");

	strcpy(PlayerCfg.GuidebotNameReal,PlayerCfg.GuidebotName);

	{
		char buf[128];

		if (player_file_version >= 24) 
			PHYSFSX_readString(file, buf);			// Just read it in fpr DPS.
	}

	if (player_file_version >= 25)
		PHYSFS_read(file, PlayerCfg.KeySettingsD2X, MAX_D2X_CONTROLS, 1);
	else
		for(i=0; i < MAX_D2X_CONTROLS; i++)
			PlayerCfg.KeySettingsD2X[i] = DefaultKeySettingsD2X[i];

	if (!PHYSFS_close(file))
		goto read_player_file_failed;

	if (rewrite_it)
		write_player_file();

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
	read_player_d2x(filename);

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

	for (i=0;i<PlayerCfg.NHighestLevels;i++)
		if (!stricmp(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename))
			break;

	if (i==PlayerCfg.NHighestLevels) {		//not found.  create entry

		if (i==MAX_MISSIONS)
			i--;		//take last entry
		else
			PlayerCfg.NHighestLevels++;

		strcpy(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename);
		PlayerCfg.HighestLevels[i].LevelNum			= 0;
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

	if (levelnum > PlayerCfg.HighestLevels[i].LevelNum)
		PlayerCfg.HighestLevels[i].LevelNum			= levelnum;

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
		for (i=0;i<PlayerCfg.NHighestLevels;i++)
			if (!stricmp(PlayerCfg.HighestLevels[i].Shortname, "DESTSAT")) 	//	Destination Saturn.
		 		highest_saturn_level			= PlayerCfg.HighestLevels[i].LevelNum; 
	}
#endif
   i			= PlayerCfg.HighestLevels[find_hli_entry()].LevelNum;
	if ( highest_saturn_level > i )
   	i			= highest_saturn_level;
	return i;
}

//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	char filename[32];
	PHYSFS_file *file;
	int i;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return -1;

	WriteConfigFile();

	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", Players[Player_num].callsign);
	write_player_d2x(filename);
	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return -1;

	//Write out player's info
	PHYSFS_writeULE32(file, SAVE_FILE_ID);
	PHYSFS_writeULE16(file, PLAYER_FILE_VERSION);

	
	PHYSFS_seek(file,PHYSFS_tell(file)+2*(sizeof(PHYSFS_uint16))); // skip Game_window_w, Game_window_h
	PHYSFSX_writeU8(file, PlayerCfg.DefaultDifficulty);
	PHYSFSX_writeU8(file, PlayerCfg.AutoLeveling);
	PHYSFSX_writeU8(file, PlayerCfg.ReticleOn);
	PHYSFSX_writeU8(file, (PlayerCfg.CockpitMode==1?0:PlayerCfg.CockpitMode));
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Default_display_mode
	PHYSFSX_writeU8(file, PlayerCfg.MissileViewEnabled);
	PHYSFSX_writeU8(file, PlayerCfg.HeadlightActiveDefault);
	PHYSFSX_writeU8(file, PlayerCfg.GuidedInBigWindow);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Automap_always_hires

	//write higest level info
	PHYSFS_writeULE16(file, PlayerCfg.NHighestLevels);
	if ((PHYSFS_write(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels))
		goto write_player_file_failed;

#ifdef NETWORK
	if ((PHYSFS_write(file, PlayerCfg.NetworkMessageMacro, MAX_MESSAGE_LEN, 4) != 4))
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

		control_type_dos = PlayerCfg.ControlType;

		if (PHYSFS_write(file, PlayerCfg.KeySettings, MAX_CONTROLS*CONTROL_MAX_TYPES, 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &control_type_win, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		else if (PHYSFS_write(file, &PlayerCfg.JoystickSensitivity, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;

		for (i = 0; i < 11; i++)
		{
			PHYSFS_write(file, &PlayerCfg.PrimaryOrder[i], sizeof(ubyte), 1);
			PHYSFS_write(file, &PlayerCfg.SecondaryOrder[i], sizeof(ubyte), 1);
		}

		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[0]);
		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[1]);

#ifdef NETWORK
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKills);
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKilled);
		i=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
#else
		PHYSFS_writeULE32(file, 0);
		PHYSFS_writeULE32(file, 0);
		i = get_lifetime_checksum (0, 0);
#endif
		PHYSFS_writeULE32(file, i);
	}

	//write guidebot name
	PHYSFSX_writeString(file, PlayerCfg.GuidebotNameReal);

	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		PHYSFSX_writeString(file, buf);		// Write out current joystick for player.
	}

	PHYSFS_write(file, PlayerCfg.KeySettingsD2X, MAX_D2X_CONTROLS, 1);

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

int get_lifetime_checksum (int a,int b)
{
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
}
