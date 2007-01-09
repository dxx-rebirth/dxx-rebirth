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

#define ADV_WEAPON_ORDER 1
#define NEW_KEYS 2
#define WEAPON_KEYS 4
#define JOYSTICK 8
#define NEWER_KEYS 16
#define WEAPON_ORDER 32
#define WINDOWSIZE 64
#define RESOLUTION 128
#define MOUSE_SENSITIVITY 256

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

static int Player_Game_window_w = 0;
static int Player_Game_window_h = 0;
static int Player_render_width = 0;
static int Player_render_height = 0;

void init_game_list()
{
	int i;

	for (i=0;i<N_SAVE_SLOTS;i++)
		saved_games[i].name[0] = 0;
}

int new_player_config()
{
	int i,j,control_choice;
	newmenu_item m[8];
	int mct=CONTROL_MAX_TYPES;
 
	 mct--;

RetrySelection:
	for (i=0; i<CONTROL_MAX_TYPES; i++ )	{
		m[i].type = NM_TYPE_MENU; m[i].text = CONTROL_TEXT(i);
	}
	m[0].text = TXT_CONTROL_KEYBOARD;
	
	control_choice = Config_control_type;				// Assume keyboard

	control_choice = newmenu_do1( NULL, TXT_CHOOSE_INPUT, i, m, NULL, control_choice );
		
		if ( control_choice < 0 )
			return 0;


	for (i=0;i<CONTROL_MAX_TYPES; i++ )
		for (j=0;j<MAX_CONTROLS; j++ )
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
	//added on 2/4/99 by Victor Rachels for new keys
	for(i=0; i < MAX_D1X_CONTROLS; i++)
		kconfig_d1x_settings[i] = default_kconfig_d1x_settings[i];
	//end this section addition - VR
	kc_set_controls();

	Config_control_type = control_choice;


	if ( Config_control_type==CONTROL_THRUSTMASTER_FCS)	{
		i = nm_messagebox( TXT_IMPORTANT_NOTE, 2, "Choose another", TXT_OK, TXT_FCS );
		if (i==0) goto RetrySelection;
	}
	
	if ( (Config_control_type>0) && 	(Config_control_type<5))	{
		joydefs_calibrate();
	}

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;
	Config_mouse_sensitivity = 8;

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
                       sscanf(line,"%i.%i",&maj,&min);
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
            else if (strstr(word,"WINDOWSIZE"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"WIDTH"))
                     sscanf(line,"%i",&Player_Game_window_w);
                    if(!strcmp(word,"HEIGHT"))
                     sscanf(line,"%i",&Player_Game_window_h);
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"RESOLUTION"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                    if(!strcmp(word,"WIDTH"))
                     sscanf(line,"%i",&Player_render_width);
                    if(!strcmp(word,"HEIGHT"))
                     sscanf(line,"%i",&Player_render_height);
                   free(line); free(word);
                   line=fsplitword(f,'\n');
                   word=splitword(line,'=');
                   strupr(word);
                 }
               free(line);
             }
            else if (strstr(word,"MOUSE"))
             {
               free(line); free(word);
               line=fsplitword(f,'\n');
               word=splitword(line,'=');
               strupr(word);

                while(!strstr(word,"END") && !feof(f))
                 {
                   if(!strcmp(word,"SENSITIVITY"))
                   {
                     int tmp;
                     sscanf(line,"%i",&tmp);
                     Config_mouse_sensitivity = (ubyte) tmp;
                   }
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
					 if ((p=decode_stat((unsigned char*)line+3,&k1,effcode1))&&
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
          fprintf(fout,"[windowsize]\n");
          fprintf(fout,"width=%d\n", Game_window_w);
          fprintf(fout,"height=%d\n", Game_window_h);
          fprintf(fout,"[end]\n");
          fprintf(fout,"[resolution]\n");
          fprintf(fout,"width=%d\n", VR_render_width);
          fprintf(fout,"height=%d\n", VR_render_height);
          fprintf(fout,"[end]\n");
          fprintf(fout,"[mouse]\n");
          fprintf(fout,"sensitivity=%d\n",Config_mouse_sensitivity);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[plx version]\n");
          fprintf(fout,"plx version=%s\n",D1X_VERSION);
          fprintf(fout,"[end]\n");

          fprintf(fout,"[end]\n");
        }
       else
        {
          int printed=0;

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
               else if (strstr(line,"WINDOWSIZE"))
                {
		  fprintf(fout,"[windowsize]\n");
		  fprintf(fout,"width=%d\n", Game_window_w);
		  fprintf(fout,"height=%d\n", Game_window_h);
		  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= WINDOWSIZE;
                }
               else if (strstr(line,"RESOLUTION"))
                {
		  fprintf(fout,"[resolution]\n");
		  fprintf(fout,"width=%d\n", VR_render_width);
		  fprintf(fout,"height=%d\n", VR_render_height);
		  fprintf(fout,"[end]\n");
                   while(!strstr(line,"END")&&!feof(fin))
                    {
                      free(line);
                      line=fsplitword(fin,'\n');
                      strupr(line);
                    }
                  free(line);
                  printed |= RESOLUTION;
                }
               else if (strstr(line,"MOUSE"))
               {
                  fprintf(fout,"[mouse]\n");
                  fprintf(fout,"sensitivity=%d\n",Config_mouse_sensitivity);
                  fprintf(fout,"[end]\n");
                  while(!strstr(line,"END")&&!feof(fin))
                  {
                    free(line);
                    line=fsplitword(fin,'\n');
                    strupr(line);
                  }
                  free(line);
                  printed |= MOUSE_SENSITIVITY;
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
           if(!(printed&WINDOWSIZE))
            {
	      fprintf(fout,"[windowsize]\n");
	      fprintf(fout,"width=%d\n", Game_window_w);
	      fprintf(fout,"height=%d\n", Game_window_h);
	      fprintf(fout,"[end]\n");
	    }

           if(!(printed&RESOLUTION))
            {
	      fprintf(fout,"[resolution]\n");
	      fprintf(fout,"width=%d\n", VR_render_width);
	      fprintf(fout,"height=%d\n", VR_render_height);
	      fprintf(fout,"[end]\n");
	    }
           if(!(printed&MOUSE_SENSITIVITY))
            {
              fprintf(fout,"[mouse]\n");
              fprintf(fout,"sensitivity=%d\n",Config_mouse_sensitivity);
              fprintf(fout,"[end]\n");
            }

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
	int player_file_size;
	int shareware_file = -1;

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
	fseek(file, 0, SEEK_END);
	player_file_size = ftell(file);
	rewind(file);

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

	/* determine if we're dealing with a shareware or registered playerfile */
	switch (info.saved_game_version)
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
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == (2212 - sizeof(saved_games)))
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == (2252 - sizeof(saved_games)))
				shareware_file = 0;
			break;
		case 8:
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == 2212)
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == 2252)
				shareware_file = 0;
			/* d1x-rebirth v0.31 to v0.42 broke things by adding stuff to the
			   player struct without thinking (sigh) */
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == (2212 + 2*sizeof(int)))
			{
				shareware_file = 1;
				/* skip the cruft added to the player_info struct */
				fseek(file, 2*sizeof(int), SEEK_CUR);
			}
			if ((player_file_size - (sizeof(hli)*n_highest_levels)) == (2252 + 2*sizeof(int)))
			{
				shareware_file = 0;
				/* skip the cruft added to the player_info struct */
				fseek(file, 2*sizeof(int), SEEK_CUR);
			}
			shareware_file=0;
	}

	if (shareware_file == -1) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Error invalid or unknown\nplayerfile-size");
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
		int i;
		#if defined SHAREWARE && defined NETWORK
		for (i = 0; i < 4; i++)
		{
			if (fread(Network_message_macro[i], MAX_MESSAGE_LEN, 1, file) != 1)
				{errno_ret = errno; break;}
			/* if this is not a shareware file, make sure what we've
			   read is 0 terminated and skip the 10 additional bytes
			   of the registered version */
			if (!shareware_file)
			{
				Network_message_macro[i][MAX_MESSAGE_LEN-1] = 0;
				fseek(file, 10, SEEK_CUR);
			}
		}
		#else
		int len = shareware_file? 25:35;

		#ifdef NETWORK
		for (i = 0; i < 4; i++)
			if (fread(Network_message_macro[i], len, 1, file) != 1)
				{errno_ret = errno; break;}
		#else
		i = 0;
		fseek( file, 4*len, SEEK_CUR );
		#endif
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

	if ( info.saved_game_version != 7 ) 	{
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

	if (Player_render_width && Player_render_height && Game_screen_mode != SM(Player_render_width, Player_render_height))
	{
		Game_screen_mode = SM(Player_render_width,Player_render_height);
		game_init_render_buffers(
			SM(Player_render_width,Player_render_height),
			Player_render_width,
			Player_render_height, VR_NONE);
	}
	Game_window_w = Player_Game_window_w;
	Game_window_h = Player_Game_window_h;

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
                 for(i=0;i<CONTROL_MAX_TYPES;i++) {
                  for(j=0;j<MAX_NOND1X_CONTROLS;j++) {
                   if(fwrite( &kconfig_settings[i][j], sizeof(kconfig_settings[i][j]), 1, file)!=1)
                    errno_ret=errno;
                  }
                 }

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
