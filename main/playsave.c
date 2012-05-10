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
#include <ctype.h>

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
#include "strutil.h"
#include "strio.h"
#include "vers_id.h"
#include "byteswap.h"
#include "physfsx.h"
#include "newdemo.h"
#include "gauges.h"

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
saved_game_sw saved_games[N_SAVE_SLOTS];
extern void InitWeaponOrdering();

int new_player_config()
{
	int i=0;
	
	for (i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;

	InitWeaponOrdering (); //setup default weapon priorities
	PlayerCfg.ControlType=0; // Assume keyboard
	memcpy(PlayerCfg.KeySettings, DefaultKeySettings, sizeof(DefaultKeySettings));
	memcpy(PlayerCfg.KeySettingsD1X, DefaultKeySettingsD1X, sizeof(DefaultKeySettingsD1X));
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = 1;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	PlayerCfg.JoystickSens[0] = PlayerCfg.JoystickSens[1] = PlayerCfg.JoystickSens[2] = PlayerCfg.JoystickSens[3] = PlayerCfg.JoystickSens[4] = PlayerCfg.JoystickSens[5] = 8;
	PlayerCfg.JoystickDead[0] = PlayerCfg.JoystickDead[1] = PlayerCfg.JoystickDead[2] = PlayerCfg.JoystickDead[3] = PlayerCfg.JoystickDead[4] = PlayerCfg.JoystickDead[5] = 0;
	PlayerCfg.MouseFlightSim = 0;
	PlayerCfg.MouseSens[0] = PlayerCfg.MouseSens[1] = PlayerCfg.MouseSens[2] = PlayerCfg.MouseSens[3] = PlayerCfg.MouseSens[4] = PlayerCfg.MouseSens[5] = 8;
	PlayerCfg.MouseFSDead = 0;
	PlayerCfg.MouseFSIndicator = 1;
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = CM_FULL_COCKPIT;
	PlayerCfg.ReticleType = RET_TYPE_CLASSIC;
	PlayerCfg.ReticleRGBA[0] = RET_COLOR_DEFAULT_R; PlayerCfg.ReticleRGBA[1] = RET_COLOR_DEFAULT_G; PlayerCfg.ReticleRGBA[2] = RET_COLOR_DEFAULT_B; PlayerCfg.ReticleRGBA[3] = RET_COLOR_DEFAULT_A;
	PlayerCfg.ReticleSize = 0;
	PlayerCfg.HudMode = 0;
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.PRShot = 0;
	PlayerCfg.NoRedundancy = 0;
	PlayerCfg.MultiMessages = 0;
	PlayerCfg.BombGauge = 1;
	PlayerCfg.AutomapFreeFlight = 0;
	PlayerCfg.NoFireAutoselect = 0;
	PlayerCfg.AlphaEffects = 0;
	PlayerCfg.DynLightColor = 0;

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
	char line[50],*word;
	int Stop=0;

	plyr_read_stats();

	f = PHYSFSX_openReadBuffered(filename);
	if(!f || PHYSFS_eof(f) ) 
		return errno;

	while( !Stop && !PHYSFS_eof(f) )
	{
		PHYSFSX_fgets(line,50,f);
		word=splitword(line,':');
		strupr(word);

		if (strstr(word,"WEAPON REORDER"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
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
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.JoystickSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.JoystickSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.JoystickSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.JoystickSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.JoystickSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.JoystickSens[5] = atoi(line);
				if(!strcmp(word,"DEADZONE0"))
					PlayerCfg.JoystickDead[0] = atoi(line);
				if(!strcmp(word,"DEADZONE1"))
					PlayerCfg.JoystickDead[1] = atoi(line);
				if(!strcmp(word,"DEADZONE2"))
					PlayerCfg.JoystickDead[2] = atoi(line);
				if(!strcmp(word,"DEADZONE3"))
					PlayerCfg.JoystickDead[3] = atoi(line);
				if(!strcmp(word,"DEADZONE4"))
					PlayerCfg.JoystickDead[4] = atoi(line);
				if(!strcmp(word,"DEADZONE5"))
					PlayerCfg.JoystickDead[5] = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"MOUSE"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"FLIGHTSIM"))
					PlayerCfg.MouseFlightSim = atoi(line);
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.MouseSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.MouseSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.MouseSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.MouseSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.MouseSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.MouseSens[5] = atoi(line);
				if(!strcmp(word,"FSDEAD"))
					PlayerCfg.MouseFSDead = atoi(line);
				if(!strcmp(word,"FSINDI"))
					PlayerCfg.MouseFSIndicator = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"WEAPON KEYS V2"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				int kc1=0,kc2=0,kc3=0;
				int i=atoi(word);
				
				if(i==0) i=10;
					i=(i-1)*3;
		
				sscanf(line,"0x%x,0x%x,0x%x",&kc1,&kc2,&kc3);
				PlayerCfg.KeySettingsD1X[i]   = kc1;
				PlayerCfg.KeySettingsD1X[i+1] = kc2;
				PlayerCfg.KeySettingsD1X[i+2] = kc3;
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"COCKPIT"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"MODE"))
					PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = atoi(line);
				else if(!strcmp(word,"HUD"))
					PlayerCfg.HudMode = atoi(line);
				else if(!strcmp(word,"RETTYPE"))
					PlayerCfg.ReticleType = atoi(line);
				else if(!strcmp(word,"RETRGBA"))
					sscanf(line,"%i,%i,%i,%i",&PlayerCfg.ReticleRGBA[0],&PlayerCfg.ReticleRGBA[1],&PlayerCfg.ReticleRGBA[2],&PlayerCfg.ReticleRGBA[3]);
				else if(!strcmp(word,"RETSIZE"))
					PlayerCfg.ReticleSize = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"TOGGLES"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"PERSISTENTDEBRIS"))
					PlayerCfg.PersistentDebris = atoi(line);
				if(!strcmp(word,"PRSHOT"))
					PlayerCfg.PRShot = atoi(line);
				if(!strcmp(word,"NOREDUNDANCY"))
					PlayerCfg.NoRedundancy = atoi(line);
				if(!strcmp(word,"MULTIMESSAGES"))
					PlayerCfg.MultiMessages = atoi(line);
				if(!strcmp(word,"BOMBGAUGE"))
					PlayerCfg.BombGauge = atoi(line);
				if(!strcmp(word,"AUTOMAPFREEFLIGHT"))
					PlayerCfg.AutomapFreeFlight = atoi(line);
				if(!strcmp(word,"NOFIREAUTOSELECT"))
					PlayerCfg.NoFireAutoselect = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"GRAPHICS"))
		{
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"ALPHAEFFECTS"))
					PlayerCfg.AlphaEffects = atoi(line);
				if(!strcmp(word,"DYNLIGHTCOLOR"))
					PlayerCfg.DynLightColor = atoi(line);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"PLX VERSION")) // know the version this pilot was used last with - allow modifications
		{
			int v1=0,v2=0,v3=0;
			d_free(word);
			PHYSFSX_fgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				sscanf(line,"%i.%i.%i",&v1,&v2,&v3);
				d_free(word);
				PHYSFSX_fgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
			if (v1 == 0 && v2 == 56 && v3 == 0) // was 0.56.0
				if (D1XMAJORi != v1 || D1XMINORi != v2 || D1XMICROi != v3) // newer (presumably)
				{
					// reset mouse cycling fields
					PlayerCfg.KeySettings[1][44] = 255;
					PlayerCfg.KeySettings[1][45] = 255;
					PlayerCfg.KeySettings[1][46] = 255;
					PlayerCfg.KeySettings[1][47] = 255;
					PlayerCfg.KeySettings[2][27] = 255;
					PlayerCfg.KeySettings[2][28] = 255;
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
					PHYSFSX_fgets(line,50,f);
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
	unsigned char c;
	int neg,i;

	if (p[0]==0)
		return NULL;
	else if (p[0]>='a'){
		neg=1;/*I=p[0]-'a';*/
	}else{
		neg=0;/*I=p[0]-'A';*/
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
	if (!p[i*2])
		return NULL;
	return p+(i*2);
}

void plyr_read_stats_v(int *k, int *d){
	char filename[PATH_MAX];
	int k1=-1,k2=0,d1=-1,d2=0;
	PHYSFS_file *f;
	
	*k=0;*d=0;//in case the file doesn't exist.

	memset(filename, '\0', PATH_MAX);
	snprintf(filename,PATH_MAX,GameArg.SysUsePlayersDir?"Players/%s.eff":"%s.eff",Players[Player_num].callsign);
	f = PHYSFSX_openReadBuffered(filename);

	if(f)
	{
		char line[256],*word;
		if(!PHYSFS_eof(f))
		{
			 PHYSFSX_fgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"kills"))
				*k=atoi(line);
			 d_free(word);
		}
		if(!PHYSFS_eof(f))
                {
			 PHYSFSX_fgets(line,50,f);
			 word=splitword(line,':');
			 if(!strcmp(word,"deaths"))
				*d=atoi(line);
			 d_free(word);
		 }
		if(!PHYSFS_eof(f))
		{
			 PHYSFSX_fgets(line,50,f);
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
	int kills = PlayerCfg.NetlifeKills,deaths = PlayerCfg.NetlifeKilled, neg, i;
	char filename[PATH_MAX];
	unsigned char buf[16],buf2[16],a;
	PHYSFS_file *f;

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir?"Players/%s.eff":"%s.eff", Players[Player_num].callsign);
	f = PHYSFSX_openWriteBuffered(filename);

	if(!f)
		return; //broken!

	PHYSFSX_printf(f,"kills:%i\n",kills);
	PHYSFSX_printf(f,"deaths:%i\n",deaths);
	PHYSFSX_printf(f,"key:01 ");

	if (kills<0)
	{
		neg=1;
		kills*=-1;
	}
	else
		neg=0;

	for (i=0;kills;i++)
	{
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

	if (neg)
		i+='a';
	else
		i+='A';

	PHYSFSX_printf(f,"%c%s %c%s ",i,buf,i,buf2);

	if (deaths<0)
	{
		neg=1;
		deaths*=-1;
	}else
		neg=0;

	for (i=0;deaths;i++)
	{
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

	if (neg)
		i+='a';
	else
		i+='A';

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
		PHYSFSX_printf(fout,"[joystick]\n");
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.JoystickSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.JoystickSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.JoystickSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.JoystickSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.JoystickSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.JoystickSens[5]);
		PHYSFSX_printf(fout,"deadzone0=%d\n",PlayerCfg.JoystickDead[0]);
		PHYSFSX_printf(fout,"deadzone1=%d\n",PlayerCfg.JoystickDead[1]);
		PHYSFSX_printf(fout,"deadzone2=%d\n",PlayerCfg.JoystickDead[2]);
		PHYSFSX_printf(fout,"deadzone3=%d\n",PlayerCfg.JoystickDead[3]);
		PHYSFSX_printf(fout,"deadzone4=%d\n",PlayerCfg.JoystickDead[4]);
		PHYSFSX_printf(fout,"deadzone5=%d\n",PlayerCfg.JoystickDead[5]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[mouse]\n");
		PHYSFSX_printf(fout,"flightsim=%d\n",PlayerCfg.MouseFlightSim);
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.MouseSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.MouseSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.MouseSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.MouseSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.MouseSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.MouseSens[5]);
		PHYSFSX_printf(fout,"fsdead=%d\n",PlayerCfg.MouseFSDead);
		PHYSFSX_printf(fout,"fsindi=%d\n",PlayerCfg.MouseFSIndicator);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[weapon keys v2]\n");
		PHYSFSX_printf(fout,"1=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[0],PlayerCfg.KeySettingsD1X[1],PlayerCfg.KeySettingsD1X[2]);
		PHYSFSX_printf(fout,"2=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[3],PlayerCfg.KeySettingsD1X[4],PlayerCfg.KeySettingsD1X[5]);
		PHYSFSX_printf(fout,"3=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[6],PlayerCfg.KeySettingsD1X[7],PlayerCfg.KeySettingsD1X[8]);
		PHYSFSX_printf(fout,"4=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[9],PlayerCfg.KeySettingsD1X[10],PlayerCfg.KeySettingsD1X[11]);
		PHYSFSX_printf(fout,"5=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[12],PlayerCfg.KeySettingsD1X[13],PlayerCfg.KeySettingsD1X[14]);
		PHYSFSX_printf(fout,"6=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[15],PlayerCfg.KeySettingsD1X[16],PlayerCfg.KeySettingsD1X[17]);
		PHYSFSX_printf(fout,"7=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[18],PlayerCfg.KeySettingsD1X[19],PlayerCfg.KeySettingsD1X[20]);
		PHYSFSX_printf(fout,"8=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[21],PlayerCfg.KeySettingsD1X[22],PlayerCfg.KeySettingsD1X[23]);
		PHYSFSX_printf(fout,"9=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[24],PlayerCfg.KeySettingsD1X[25],PlayerCfg.KeySettingsD1X[26]);
		PHYSFSX_printf(fout,"0=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD1X[27],PlayerCfg.KeySettingsD1X[28],PlayerCfg.KeySettingsD1X[29]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cockpit]\n");
		PHYSFSX_printf(fout,"mode=%i\n",PlayerCfg.CockpitMode[0]);
		PHYSFSX_printf(fout,"hud=%i\n",PlayerCfg.HudMode);
		PHYSFSX_printf(fout,"rettype=%i\n",PlayerCfg.ReticleType);
		PHYSFSX_printf(fout,"retrgba=%i,%i,%i,%i\n",PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2],PlayerCfg.ReticleRGBA[3]);
		PHYSFSX_printf(fout,"retsize=%i\n",PlayerCfg.ReticleSize);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[toggles]\n");
		PHYSFSX_printf(fout,"persistentdebris=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,"prshot=%i\n",PlayerCfg.PRShot);
		PHYSFSX_printf(fout,"noredundancy=%i\n",PlayerCfg.NoRedundancy);
		PHYSFSX_printf(fout,"multimessages=%i\n",PlayerCfg.MultiMessages);
		PHYSFSX_printf(fout,"bombgauge=%i\n",PlayerCfg.BombGauge);
		PHYSFSX_printf(fout,"automapfreeflight=%i\n",PlayerCfg.AutomapFreeFlight);
		PHYSFSX_printf(fout,"nofireautoselect=%i\n",PlayerCfg.NoFireAutoselect);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[graphics]\n");
		PHYSFSX_printf(fout,"alphaeffects=%i\n",PlayerCfg.AlphaEffects);
		PHYSFSX_printf(fout,"dynlightcolor=%i\n",PlayerCfg.DynLightColor);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[plx version]\n");
		PHYSFSX_printf(fout,"plx version=%s\n",VERSION);
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
	char filename[PATH_MAX];
	PHYSFS_file *file;
	int player_file_size, shareware_file = -1, id = 0;
	short saved_game_version, player_struct_version;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	if (!PHYSFSX_exists(filename,0))
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
	saved_game_version = PHYSFSX_readShort(file);
	player_struct_version = PHYSFSX_readShort(file);
	PlayerCfg.NHighestLevels = PHYSFSX_readInt(file);
	PlayerCfg.DefaultDifficulty = PHYSFSX_readInt(file);
	PlayerCfg.AutoLeveling = PHYSFSX_readInt(file);

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
		ubyte dummy_joy_sens;

		if (PHYSFS_read(file, &PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]),1)!=1)
			goto read_player_file_failed;
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS*3) ); // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Cyberman map field
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
				state_save_old_game(i, saved_games[i].name, &saved_games[i].sg_player, saved_games[i].difficulty_level, saved_games[i].primary_weapon, saved_games[i].secondary_weapon, saved_games[i].next_level_num );
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
	char filename[PATH_MAX];
	PHYSFS_file *file;
	int errno_ret, i;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return -1;

	errno_ret = WriteConfigFile();

	memset(filename, '\0', PATH_MAX);
        snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", Players[Player_num].callsign);
	write_player_d1x(filename);

	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
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
		if (PHYSFS_write(file, PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]), 1) != 1)
			errno_ret=errno;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]), 1) != 1)
			errno_ret=errno;
		for (i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				errno_ret=errno;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]), 1) != 1)
			errno_ret=errno;
		for (i = 0; i < MAX_CONTROLS; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Cyberman map field
				errno_ret=errno;
	
		if(errno_ret == EZERO)
		{
			ubyte old_avg_joy_sensitivity = 8;
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

// read stored values from ngp file to netgame_info
void read_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX], line[50], *token, *value, *ptr;
	PHYSFS_file *file;

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.ngp" : "%.8s.ngp", Players[Player_num].callsign);
	if (!PHYSFSX_exists(filename,0))
		return;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		return;

	// NOTE that we do not set any defaults here or even initialize netgame_info. For flexibility, leave that to the function calling this.
	while (!PHYSFS_eof(file))
	{
		memset(line, 0, 50);
		PHYSFSX_gets(file, line);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!value)
				value = "";
			if (!strcmp(token, "game_name"))
			{
				char * p;
				strncpy( ng->game_name, value, NETGAME_NAME_LEN+1 );
				p = strchr( ng->game_name, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, "gamemode"))
				ng->gamemode = strtol(value, NULL, 10);
			else if (!strcmp(token, "RefusePlayers"))
				ng->RefusePlayers = strtol(value, NULL, 10);
			else if (!strcmp(token, "difficulty"))
				ng->difficulty = strtol(value, NULL, 10);
			else if (!strcmp(token, "game_flags"))
				ng->game_flags = strtol(value, NULL, 10);
			else if (!strcmp(token, "AllowedItems"))
				ng->AllowedItems = strtol(value, NULL, 10);
			else if (!strcmp(token, "ShowEnemyNames"))
				ng->ShowEnemyNames = strtol(value, NULL, 10);
			else if (!strcmp(token, "BrightPlayers"))
				ng->BrightPlayers = strtol(value, NULL, 10);
			else if (!strcmp(token, "InvulAppear"))
				ng->InvulAppear = strtol(value, NULL, 10);
			else if (!strcmp(token, "KillGoal"))
				ng->KillGoal = strtol(value, NULL, 10);
			else if (!strcmp(token, "PlayTimeAllowed"))
				ng->PlayTimeAllowed = strtol(value, NULL, 10);
			else if (!strcmp(token, "control_invul_time"))
				ng->control_invul_time = strtol(value, NULL, 10);
			else if (!strcmp(token, "PacketsPerSec"))
				ng->PacketsPerSec = strtol(value, NULL, 10);
			else if (!strcmp(token, "NoFriendlyFire"))
				ng->NoFriendlyFire = strtol(value, NULL, 10);
#ifdef USE_TRACKER
			else if (!strcmp(token, "Tracker"))
				ng->Tracker = strtol(value, NULL, 10);
#endif
		}
	}

	PHYSFS_close(file);
}

// write values from netgame_info to ngp file
void write_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX];
	PHYSFS_file *file;

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.ngp" : "%.8s.ngp", Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return;

	PHYSFSX_printf(file, "game_name=%s\n", ng->game_name);
	PHYSFSX_printf(file, "gamemode=%i\n", ng->gamemode);
	PHYSFSX_printf(file, "RefusePlayers=%i\n", ng->RefusePlayers);
	PHYSFSX_printf(file, "difficulty=%i\n", ng->difficulty);
	PHYSFSX_printf(file, "game_flags=%i\n", ng->game_flags);
	PHYSFSX_printf(file, "AllowedItems=%i\n", ng->AllowedItems);
	PHYSFSX_printf(file, "ShowEnemyNames=%i\n", ng->ShowEnemyNames);
	PHYSFSX_printf(file, "BrightPlayers=%i\n", ng->BrightPlayers);
	PHYSFSX_printf(file, "InvulAppear=%i\n", ng->InvulAppear);
	PHYSFSX_printf(file, "KillGoal=%i\n", ng->KillGoal);
	PHYSFSX_printf(file, "PlayTimeAllowed=%i\n", ng->PlayTimeAllowed);
	PHYSFSX_printf(file, "control_invul_time=%i\n", ng->control_invul_time);
	PHYSFSX_printf(file, "PacketsPerSec=%i\n", ng->PacketsPerSec);
	PHYSFSX_printf(file, "NoFriendlyFire=%i\n", ng->NoFriendlyFire);
#ifdef USE_TRACKER
	PHYSFSX_printf(file, "Tracker=%i\n", ng->Tracker);
#else
	PHYSFSX_printf(file, "Tracker=0\n");
#endif
	PHYSFSX_printf(file, "ngp version=%s\n",VERSION);

	PHYSFS_close(file);
}
