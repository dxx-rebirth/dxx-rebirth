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
 * Functions to load & save player games
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif
#include "error.h"
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
#include "u_mem.h"
#include "network.h"
#include "strutil.h"
#include "strio.h"
#include "vers_id.h"
#include "byteswap.h"
#include "physfsx.h"
#include "newdemo.h"

//version 5 -> 6: added new highest level information
//version 6 -> 7: stripped out the old saved_game array.
//version 7 -> 8: readded the old saved_game array since this is needed
//                for shareware saved games
//the shareware is level 4

#define SAVE_FILE_ID 0x44504c52 /* 'DPLR' */
#define SAVED_GAME_VERSION 8 //increment this every time saved_game struct changes
#define COMPATIBLE_SAVED_GAME_VERSION 4
#define COMPATIBLE_PLAYER_STRUCT_VERSION 16

struct player_config PlayerCfg;
saved_game saved_games[N_SAVE_SLOTS];
extern void InitWeaponOrdering();

int new_player_config()
{
	int i,j;
	int mct=CONTROL_MAX_TYPES;

	for (i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;

	mct--;

	PlayerCfg.ControlType=CONTROL_NONE; // Assume keyboard
	InitWeaponOrdering (); //setup default weapon priorities

	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			PlayerCfg.KeySettings[i][j] = DefaultKeySettings[i][j];
	for(i=0; i < MAX_D1X_CONTROLS; i++)
		PlayerCfg.KeySettingsD1X[i] = DefaultKeySettingsD1X[i];
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = 1;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	PlayerCfg.JoystickSensitivityX = 8;
	PlayerCfg.JoystickSensitivityY = 8;
	PlayerCfg.MouseSensitivityX = 8;
	PlayerCfg.MouseSensitivityY = 8;
	PlayerCfg.MouseFilter = 0;
	PlayerCfg.CockpitMode = CM_FULL_COCKPIT;
	PlayerCfg.ReticleOn = 1;
	PlayerCfg.HudMode = 0;
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.PRShot = 0;
	PlayerCfg.OglAlphaEffects = 0;
	PlayerCfg.OglReticle = 0;

	// Default taunt macros
	#ifdef NETWORK
	strcpy(PlayerCfg.NetworkMessageMacro[0], TXT_DEF_MACRO_1);
	strcpy(PlayerCfg.NetworkMessageMacro[1], TXT_DEF_MACRO_2);
	strcpy(PlayerCfg.NetworkMessageMacro[2], TXT_DEF_MACRO_3);
	strcpy(PlayerCfg.NetworkMessageMacro[3], TXT_DEF_MACRO_4);
	PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	#endif
	
	return 1;
}

int read_player_d1x(char *filename)
{
	PHYSFS_file *f;
	int rc = 0;
	char line[60],*word;
	int Stop=0;

	plyr_read_stats();

	f = PHYSFSX_openReadBuffered(filename);
	if(!f || PHYSFS_eof(f) ) 
		return errno;

	while( !Stop && !PHYSFS_eof(f) )
	{
		cfgets(line,50,f);
		word=splitword(line,':');
		strupr(word);

		if (strstr(word,"WEAPON REORDER"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				unsigned int wo0=0,wo1=0,wo2=0,wo3=0,wo4=0,wo5=0;
				if(!strcmp(word,"PRIMARY"))
				{
					sscanf(line,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",&wo0, &wo1, &wo2, &wo3, &wo4, &wo5);
					PlayerCfg.PrimaryOrder[0]=wo0; PlayerCfg.PrimaryOrder[1]=wo1; PlayerCfg.PrimaryOrder[2]=wo2; PlayerCfg.PrimaryOrder[3]=wo3; PlayerCfg.PrimaryOrder[4]=wo4; PlayerCfg.PrimaryOrder[5]=wo5;
				}
				else if(!strcmp(word,"SECONDARY"))
				{
					sscanf(line,"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",&wo0, &wo1, &wo2, &wo3, &wo4, &wo5);
					PlayerCfg.SecondaryOrder[0]=wo0; PlayerCfg.SecondaryOrder[1]=wo1; PlayerCfg.SecondaryOrder[2]=wo2; PlayerCfg.SecondaryOrder[3]=wo3; PlayerCfg.SecondaryOrder[4]=wo4; PlayerCfg.SecondaryOrder[5]=wo5;
				}
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"WEAPON KEYS"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				int kc1=0,kc2=0;
				int i=atoi(word);
				
				if(i==0) i=10;
					i=(i-1)*2;
		
				sscanf(line,"0x%x,0x%x",&kc1,&kc2);
				PlayerCfg.KeySettingsD1X[i]   = kc1;
				PlayerCfg.KeySettingsD1X[i+1] = kc2;
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"CYCLE KEYS"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				int kc1=0, kc2=0;
				if(!strcmp(word,"CYCLE PRIMARY"))
				{
					sscanf(line,"0x%x,0x%x",&kc1,&kc2);
					PlayerCfg.KeySettingsD1X[20]=kc1;
					PlayerCfg.KeySettingsD1X[21]=kc2;
				}
				else if(!strcmp(word,"CYCLE SECONDARY"))
				{
					sscanf(line,"0x%x,0x%x",&kc1,&kc2);
					PlayerCfg.KeySettingsD1X[22]=kc1;
					PlayerCfg.KeySettingsD1X[23]=kc2;
				}
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITYX"))
					PlayerCfg.JoystickSensitivityX = atoi(line);
				if(!strcmp(word,"SENSITIVITYY"))
					PlayerCfg.JoystickSensitivityY = atoi(line);
				if(!strcmp(word,"DEADZONE"))
					PlayerCfg.JoystickDeadzone = atoi(line);
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
				if(!strcmp(word,"SENSITIVITYX"))
					PlayerCfg.MouseSensitivityX = atoi(line);
				if(!strcmp(word,"SENSITIVITYY"))
					PlayerCfg.MouseSensitivityY = atoi(line);
				if(!strcmp(word,"FILTER"))
					PlayerCfg.MouseFilter = atoi(line);
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
				if(!strcmp(word,"MODE"))
					PlayerCfg.CockpitMode = atoi(line);
				else if(!strcmp(word,"HUD"))
					PlayerCfg.HudMode = atoi(line);
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
				if(!strcmp(word,"RETICLE"))
					PlayerCfg.ReticleOn = atoi(line);
				else if(!strcmp(word,"PERSISTENTDEBRIS"))
					PlayerCfg.PersistentDebris = atoi(line);
				else if(!strcmp(word,"PRSHOT"))
					PlayerCfg.PRShot = atoi(line);
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
					PlayerCfg.OglAlphaEffects = atoi(line);
				else if(!strcmp(word,"OGLRETICLE"))
					PlayerCfg.OglReticle = atoi(line);
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
			if(word[0]=='['&&!strstr(word,"D1X OPTIONS"))
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

char effcode1[]="d1xrocks_SKCORX!D";
char effcode2[]="AObe)7Rn1 -+/zZ'0";
char effcode3[]="aoeuidhtnAOEUIDH6";
char effcode4[]="'/.;]<{=,+?|}->[3";

unsigned char * decode_stat(unsigned char *p,int *v,char *effcode)
{
	unsigned char c;int neg,i,I;
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
	if (!p[i*2])return NULL;
	return p+(i*2);
}

void plyr_read_stats_v(int *k, int *d){
	char filename[14];
	int k1=-1,k2=0,d1=-1,d2=0;
	PHYSFS_file *f;
	
	*k=0;*d=0;//in case the file doesn't exist.
		
	sprintf(filename,GameArg.SysUsePlayersDir?"Players/%s.eff":"%s.eff",Players[Player_num].callsign);
	f = PHYSFSX_openReadBuffered(filename);

	if(f)
	{
		char line[256],*word;
		if(!PHYSFS_eof(f))
		{
			 cfgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"kills"))
				*k=atoi(line);
			 d_free(word);
		}
		if(!PHYSFS_eof(f))
                {
			 cfgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"deaths"))
				*d=atoi(line);
			 d_free(word);
		 }
		if(!PHYSFS_eof(f))
		{
			 cfgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"key") && strlen(line)>10){
				 unsigned char *p;
				 if (line[0]=='0' && line[1]=='1'){
					 if ((p=decode_stat((unsigned char*)line+3,&k1,effcode1))&&
					     (p=decode_stat(p+1,&k2,effcode2))&&
					     (p=decode_stat(p+1,&d1,effcode3))){
						 decode_stat(p+1,&d2,effcode4);
					 }
				 }
			 }
			 d_free(word);
		}
		if (k1!=k2 || k1!=*k || d1!=d2 || d1!=*d)
		{
			*k=0;*d=0;
		}
	}

	if(f)
		PHYSFS_close(f);
}

void plyr_read_stats()
{
	plyr_read_stats_v(&PlayerCfg.NetlifeKills,&PlayerCfg.NetlifeKilled);
}

void plyr_save_stats()
{
	int kills,deaths,neg;
	char filename[14];
	unsigned char buf[16],buf2[16],a;
	int i;
	PHYSFS_file *f;
	kills=0;
	deaths=0;
	
	kills=PlayerCfg.NetlifeKills;
	deaths=PlayerCfg.NetlifeKilled;
	
	sprintf(filename,GameArg.SysUsePlayersDir?"Players/%s.eff":"%s.eff",Players[Player_num].callsign);
	f = PHYSFSX_openWriteBuffered(filename);

	if(!f)
		return; //broken!
	
	PHYSFSX_printf(f,"kills:%i\n",kills);
	PHYSFSX_printf(f,"deaths:%i\n",deaths);
	PHYSFSX_printf(f,"key:01 ");
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
	PHYSFSX_printf(f,"%c%s %c%s ",i,buf,i,buf2);

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
	PHYSFSX_printf(f,"%c%s %c%s\n",i,buf,i,buf2);
	
	PHYSFS_close(f);
}

int write_player_d1x(char *filename)
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
		PHYSFSX_printf(fout,"[D1X Options]\n");
		PHYSFSX_printf(fout,"[weapon reorder]\n");
		PHYSFSX_printf(fout,"primary=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",PlayerCfg.PrimaryOrder[0], PlayerCfg.PrimaryOrder[1], PlayerCfg.PrimaryOrder[2],PlayerCfg.PrimaryOrder[3], PlayerCfg.PrimaryOrder[4], PlayerCfg.PrimaryOrder[5]);
		PHYSFSX_printf(fout,"secondary=0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",PlayerCfg.SecondaryOrder[0], PlayerCfg.SecondaryOrder[1], PlayerCfg.SecondaryOrder[2],PlayerCfg.SecondaryOrder[3], PlayerCfg.SecondaryOrder[4], PlayerCfg.SecondaryOrder[5]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[weapon keys]\n");
		PHYSFSX_printf(fout,"1=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[0],PlayerCfg.KeySettingsD1X[1]);
		PHYSFSX_printf(fout,"2=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[2],PlayerCfg.KeySettingsD1X[3]);
		PHYSFSX_printf(fout,"3=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[4],PlayerCfg.KeySettingsD1X[5]);
		PHYSFSX_printf(fout,"4=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[6],PlayerCfg.KeySettingsD1X[7]);
		PHYSFSX_printf(fout,"5=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[8],PlayerCfg.KeySettingsD1X[9]);
		PHYSFSX_printf(fout,"6=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[10],PlayerCfg.KeySettingsD1X[11]);
		PHYSFSX_printf(fout,"7=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[12],PlayerCfg.KeySettingsD1X[13]);
		PHYSFSX_printf(fout,"8=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[14],PlayerCfg.KeySettingsD1X[15]);
		PHYSFSX_printf(fout,"9=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[16],PlayerCfg.KeySettingsD1X[17]);
		PHYSFSX_printf(fout,"0=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[18],PlayerCfg.KeySettingsD1X[19]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cycle keys]\n");
		PHYSFSX_printf(fout,"cycle primary=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[20],PlayerCfg.KeySettingsD1X[21]);
		PHYSFSX_printf(fout,"cycle secondary=0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[22],PlayerCfg.KeySettingsD1X[23]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[joystick]\n");
		PHYSFSX_printf(fout,"sensitivityx=%d\n",PlayerCfg.JoystickSensitivityX);
		PHYSFSX_printf(fout,"sensitivityy=%d\n",PlayerCfg.JoystickSensitivityY);
		PHYSFSX_printf(fout,"deadzone=%i\n",PlayerCfg.JoystickDeadzone);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[mouse]\n");
		PHYSFSX_printf(fout,"sensitivityx=%d\n",PlayerCfg.MouseSensitivityX);
		PHYSFSX_printf(fout,"sensitivityy=%d\n",PlayerCfg.MouseSensitivityY);
		PHYSFSX_printf(fout,"filter=%d\n",PlayerCfg.MouseFilter);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cockpit]\n");
		PHYSFSX_printf(fout,"mode=%i\n",(PlayerCfg.CockpitMode==1?0:PlayerCfg.CockpitMode));
		PHYSFSX_printf(fout,"hud=%i\n",PlayerCfg.HudMode);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[toggles]\n");
		PHYSFSX_printf(fout,"reticle=%i\n",PlayerCfg.ReticleOn);
		PHYSFSX_printf(fout,"persistentdebris=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,"prshot=%i\n",PlayerCfg.PRShot);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[opengl]\n");
		PHYSFSX_printf(fout,"oglalphaeffects=%i\n",PlayerCfg.OglAlphaEffects);
		PHYSFSX_printf(fout,"oglreticle=%i\n",PlayerCfg.OglReticle);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[plx version]\n");
		PHYSFSX_printf(fout,"plx version=%s\n",D1X_VERSION);
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

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[32];
	PHYSFS_file *file;
	int player_file_size, shareware_file = -1, id = 0;
	short saved_game_version, player_struct_version;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	if (!PHYSFS_exists(filename))
		return ENOENT;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		goto read_player_file_failed;

	new_player_config(); // Set defaults!

	// Unfortunatly d1x has been writing both shareware and registered
	// player files with a saved_game_version of 7 and 8, whereas the
	// original decent used 4 for shareware games and 7 for registered
	// games. Because of this the player files didn't get properly read
	// when reading d1x shareware player files in d1x registered or
	// vica versa. The problem is that the sizeof of the taunt macros
	// differ between the share and registered versions, causing the
	// reading of the player file to go wrong. Thus we now determine the
	// sizeof the player file to determine what kinda player file we are
	// dealing with so that we can do the right thing
	PHYSFS_seek(file, 0);
	player_file_size = PHYSFS_fileLength(file);

	PHYSFS_readSLE32(file, &id);
	saved_game_version = cfile_read_short(file);
	player_struct_version = cfile_read_short(file);
	PlayerCfg.NHighestLevels = cfile_read_int(file);
	PlayerCfg.DefaultDifficulty = cfile_read_int(file);
	PlayerCfg.AutoLeveling = cfile_read_int(file);

	if (id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		PHYSFS_close(file);
		return -1;
	}

	if (saved_game_version<COMPATIBLE_SAVED_GAME_VERSION || player_struct_version<COMPATIBLE_PLAYER_STRUCT_VERSION) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		PHYSFS_close(file);
		return -1;
	}

	/* determine if we're dealing with a shareware or registered playerfile */
	switch (saved_game_version)
	{
		case 4:
			shareware_file = 1;
			break;
		case 5:
		case 6:
			shareware_file = 0;
			break;
		case 7:
			/* version 7 doesn't have the saved games array */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 - sizeof(saved_games)))
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 - sizeof(saved_games)))
				shareware_file = 0;
			break;
		case 8:
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2212)
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2252)
				shareware_file = 0;
			/* d1x-rebirth v0.31 to v0.42 broke things by adding stuff to the
			   player struct without thinking (sigh) */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 + 2*sizeof(int)))
			{

				shareware_file = 1;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 + 2*sizeof(int)))
			{
				shareware_file = 0;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
	}

	if (shareware_file == -1) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Error invalid or unknown\nplayerfile-size");
		PHYSFS_close(file);
		return -1;
	}

	if (saved_game_version <= 5) {

		//deal with old-style highest level info

		PlayerCfg.HighestLevels[0].Shortname[0] = 0;							//no name for mission 0
		PlayerCfg.HighestLevels[0].LevelNum = PlayerCfg.NHighestLevels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (PlayerCfg.NHighestLevels==7)
			PlayerCfg.HighestLevels[0].LevelNum = 8;
		
	}
	else {	//read new highest level info
		if (PHYSFS_read(file,PlayerCfg.HighestLevels,sizeof(hli),PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
			goto read_player_file_failed;
	}

	if ( saved_game_version != 7 ) {	// Read old & SW saved games.
		if (PHYSFS_read(file,saved_games,sizeof(saved_games),1) != 1)
			goto read_player_file_failed;
	}

	//read taunt macros
	{
		int i;
		int len = shareware_file? 25:35;

		#ifdef NETWORK
		for (i = 0; i < 4; i++)
			if (PHYSFS_read(file, PlayerCfg.NetworkMessageMacro[i], len, 1) != 1)
				goto read_player_file_failed;
		#else
		i = 0;
		PHYSFS_seek( file, PHYSFS_tell(file)+4*len );
		#endif
	}

	//read kconfig data
	{
		int i,j;
		ubyte dummy_joy_sens;
		for(i=0;i<CONTROL_MAX_TYPES;i++)
			for(j=0;j<MAX_NOND1X_CONTROLS;j++)
				if(PHYSFS_read(file, &PlayerCfg.KeySettings[i][j], sizeof(ubyte),1)!=1)
					goto read_player_file_failed;
		if (PHYSFS_read(file, &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &dummy_joy_sens, sizeof(ubyte), 1 )!=1)
			goto read_player_file_failed;
	}

	if ( saved_game_version != 7 ) 	{
		int i, found=0;
		
		Assert( N_SAVE_SLOTS == 10 );

		for (i=0; i<N_SAVE_SLOTS; i++ )	{
			if ( saved_games[i].name[0] )	{
				state_save_old_game(i, saved_games[i].name, &saved_games[i].player, 
			saved_games[i].difficulty_level, saved_games[i].primary_weapon, 
				saved_games[i].secondary_weapon, saved_games[i].next_level_num );
				// make sure we do not do this again, which would possibly overwrite
				// a new newstyle savegame
				saved_games[i].name[0] = 0;
				found++;
			}
		}
		if (found)
			write_player_file();
	}

	if (!PHYSFS_close(file))
		goto read_player_file_failed;

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
	read_player_d1x(filename);
	kc_set_controls();

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

	if (i==PlayerCfg.NHighestLevels) { //not found. create entry

		if (i==MAX_MISSIONS)
			i--; //take last entry
		else
			PlayerCfg.NHighestLevels++;

		strcpy(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename);
		PlayerCfg.HighestLevels[i].LevelNum = 0;
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

	i = find_hli_entry();

	if (levelnum > PlayerCfg.HighestLevels[i].LevelNum)
		PlayerCfg.HighestLevels[i].LevelNum = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();
#ifndef SHAREWARE
#ifndef SATURN
	if (strlen(Current_mission_filename)==0 )	{
		for (i=0;i<PlayerCfg.NHighestLevels;i++)
			if (!stricmp(PlayerCfg.HighestLevels[i].Shortname, "DESTSAT")) // Destination Saturn.
				highest_saturn_level = PlayerCfg.HighestLevels[i].LevelNum;
	}
#endif
#endif
	i = PlayerCfg.HighestLevels[find_hli_entry()].LevelNum;
	if ( highest_saturn_level > i )
		i = highest_saturn_level;
	return i;
}


//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	char filename[32];
	PHYSFS_file *file;
	int errno_ret;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return -1;

	errno_ret = WriteConfigFile();

        sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", Players[Player_num].callsign);
	write_player_d1x(filename);

	sprintf(filename, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return errno;

	PHYSFS_writeULE32(file, SAVE_FILE_ID);
	PHYSFS_writeULE16(file, SAVED_GAME_VERSION);
	PHYSFS_writeULE16(file, PLAYER_STRUCT_VERSION);
	PHYSFS_writeSLE32(file, PlayerCfg.NHighestLevels);
	PHYSFS_writeSLE32(file, PlayerCfg.DefaultDifficulty);
	PHYSFS_writeSLE32(file, PlayerCfg.AutoLeveling);
	errno_ret = EZERO;

	//write higest level info
	if ((PHYSFS_write( file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)) {
		errno_ret = errno;
		PHYSFS_close(file);
		return errno_ret;
	}

	if (PHYSFS_write( file, saved_games,sizeof(saved_games),1) != 1) {
		errno_ret = errno;
		PHYSFS_close(file);
		return errno_ret;
	}

	#ifdef NETWORK
	if ((PHYSFS_write( file, PlayerCfg.NetworkMessageMacro, MAX_MESSAGE_LEN, 4) != 4)) {
		errno_ret = errno;
		PHYSFS_close(file);
		return errno_ret;
	}
	#else
	{
		//PHYSFS_seek( file, PHYSFS_tell(file)+MAX_MESSAGE_LEN * 4 );	// Seeking is bad for Mac OS 9
		char dummy[MAX_MESSAGE_LEN][4];
		
		if ((PHYSFS_write( file, dummy, MAX_MESSAGE_LEN, 4) != 4)) {
			errno_ret = errno;
			PHYSFS_close(file);
			return errno_ret;
		}
	}
	#endif

	//write kconfig info
	{
		int i,j;
		for(i=0;i<CONTROL_MAX_TYPES;i++) {
			for(j=0;j<MAX_NOND1X_CONTROLS;j++) {
				if(PHYSFS_write( file,  &PlayerCfg.KeySettings[i][j], sizeof(PlayerCfg.KeySettings[i][j]), 1)!=1)
					errno_ret=errno;
			}
		}
	
		if(errno_ret == EZERO)
		{
			ubyte old_avg_joy_sensitivity = ((PlayerCfg.JoystickSensitivityX+PlayerCfg.JoystickSensitivityY+1)/2);
			if (PHYSFS_write( file,  &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
			else if (PHYSFS_write( file, &old_avg_joy_sensitivity, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
		}
	}

	if (!PHYSFS_close(file))
		errno_ret = errno;

	if (errno_ret != EZERO) {
		PHYSFS_delete(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}

	return errno_ret;
}
