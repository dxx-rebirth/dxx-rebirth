//multiplayer profiles - Victor Rachels 10/18/98
#ifdef NETWORK

#include <stdio.h>
#include <string.h>
#include "strio.h"
#include "multipow.h"
#include "network.h"
#include "text.h"
#include "mlticntl.h"
#include "d_io.h"
#include "args.h"
//added 05/17/99 Matt Mueller 
#include "u_mem.h"
//end addition -MM
//added 6-15-99 Owen Evans
#include "strutil.h"
//end added

void save_multi_profile(int multivalues[40],char *filename)
{
 int i;
 FILE *f;

  removeext(filename,filename);
  strcat(filename,".pfl");
  f=fopen(filename,"wt");
  
   if(f)
    {
       for(i=0; i < 40; i++)
        fprintf(f,"%i=%i\n",i,multivalues[i]);
      nm_messagebox(NULL,1,TXT_OK,"Save Successful");
    }
   else
    nm_messagebox(NULL,1,TXT_OK,"Save Failed");

  fclose(f);
}

void get_multi_profile(int multivalues[40], char *filename)
{
 int i;
 char *line,*word;
 FILE *f;

  removeext(filename,filename);
  strcat(filename,".pfl");
   if(!(f=fopen(filename,"rt")))
    nm_messagebox(TXT_ERROR,1,TXT_OK, "No such profile.");
 
  line=fsplitword(f,'\n');
   if(line)
    do
     {
      word=splitword(line,'=');
      i = atoi(word);
      multivalues[i]=atoi(line);
      free(line); free(word);
      line=fsplitword(f,'\n');
     } while(!feof(f));
   if(line)
    free(line);
  fclose(f);
}

int do_multi_profile(int multivalues[40])
{
 int i;
 char filename[13];

  i=nm_messagebox(NULL,2,"Load Profile","Save Profile","Multi Profile");

   if(i==0) //load profile : dirlist of *.pfl then load
    {
       if(newmenu_get_filename("Select profile", "*.pfl", filename, 1))
        {
         strlwr(filename);
         get_multi_profile(multivalues,filename);
         return 1;
        }
    }
   if(i==1) //save profile : get filename from user then save
    {
     newmenu_item m[2];
     char profile[9];

      memset(profile,0,sizeof(char)*9);
      m[0].type = NM_TYPE_TEXT;  m[0].text = "Profile name:";
      m[1].type = NM_TYPE_INPUT; m[1].text = profile; m[1].text_len=8;

       if(newmenu_do(NULL, "Save Profile", 2, m, NULL) != -1)
        {
         sprintf(filename,"%s%s",profile,".pfl");
         strlwr(filename);
         save_multi_profile(multivalues,filename);
         return 0;
        }
    }
  return 0;
}


void putto_multivalues(int multivalues[40],netgame_info *temp_game, int *socket)
{
#ifndef SHAREWARE
 int i;
#endif

 multivalues[0]=temp_game->gamemode == NETGAME_ANARCHY;
 multivalues[1]=temp_game->gamemode == NETGAME_TEAM_ANARCHY;
 multivalues[2]=temp_game->gamemode == NETGAME_ROBOT_ANARCHY;
 multivalues[3]=temp_game->gamemode == NETGAME_COOPERATIVE;
 multivalues[4]=(temp_game->game_flags & NETGAME_FLAG_CLOSED) != 0;
#ifndef SHAREWARE
 multivalues[5]=(temp_game->game_flags & NETGAME_FLAG_SHOW_MAP) != 0;
#endif
 multivalues[6]=temp_game->difficulty;
#ifndef SHAREWARE
 multivalues[7]=temp_game->control_invul_time;
#endif
 multivalues[8]=restrict_mode;
 multivalues[9]=0;
#ifndef SHAREWARE
 multivalues[10]=(temp_game->flags & NETFLAG_SHORTPACKETS) != 0;
 multivalues[11]=temp_game->packets_per_sec;
 multivalues[12]=*socket;
 multivalues[13]=temp_game->protocol_version == MULTI_PROTO_D1X_VER;
#endif
 multivalues[14]=temp_game->max_numplayers;
#ifndef SHAREWARE
 multivalues[15]=(temp_game->flags & NETFLAG_DROP_VULCAN_AMMO) != 0;
 multivalues[16]=(temp_game->flags & NETFLAG_ENABLE_IGNORE_GHOST) != 0;
//added on 11/12/98 by Victor Rachels to add radar
 multivalues[17]=(temp_game->flags & NETFLAG_ENABLE_RADAR) != 0;
//end this section addition - VR
 multivalues[18]=(temp_game->flags & NETFLAG_ENABLE_ALT_VULCAN) != 0;
 multivalues[19]=0;
  for(i=0;i<MULTI_ALLOW_POWERUP_MAX;i++)
    multivalues[20+i] = (temp_game->flags & (1 << i) ) ? 1 : 0;
#endif
}

void putfrom_multivalues(int multivalues[40], netgame_info *temp_game, int *socket)
{
#ifndef SHAREWARE
 int i;
#endif

  if(multivalues[0]) temp_game->gamemode = NETGAME_ANARCHY;
  else if(multivalues[1]) temp_game->gamemode = NETGAME_TEAM_ANARCHY;
  else if(multivalues[2]) temp_game->gamemode = NETGAME_ROBOT_ANARCHY;
  else if(multivalues[3]) temp_game->gamemode = NETGAME_COOPERATIVE;
 temp_game->game_flags = 0;
  if(multivalues[4])
   temp_game->game_flags |= NETGAME_FLAG_CLOSED;
#ifndef SHAREWARE
  if(multivalues[5])
   temp_game->game_flags |= NETGAME_FLAG_SHOW_MAP;
#endif
 temp_game->difficulty = multivalues[6];
#ifndef SHAREWARE
 temp_game->control_invul_time = multivalues[7];
#endif

 restrict_mode = multivalues[8];
//= multivalues[9];
#ifndef SHAREWARE
 temp_game->flags &= 0;

  if(multivalues[10])
   temp_game->flags |= NETFLAG_SHORTPACKETS;
 temp_game->packets_per_sec = multivalues[11];
 {
  int t;
   if((t=FindArg("-socket")))
    {
     t=atoi(Args[t+1]);
      if(t>-100 && t<100)
       *socket = t;
      else
       *socket = multivalues[12];
    }
   else
    *socket = multivalues[12];
 }


 temp_game->protocol_version = (multivalues[13] ? MULTI_PROTO_D1X_VER : MULTI_PROTO_VERSION);
#endif
 temp_game->max_numplayers =  multivalues[14];
#ifndef SHAREWARE
  if(multivalues[15])
   temp_game->flags |= NETFLAG_DROP_VULCAN_AMMO;
  if(multivalues[16])
   temp_game->flags |= NETFLAG_ENABLE_IGNORE_GHOST;
//added on 11/12/98 by Victor Rachels to add radar
  if(multivalues[17])
   temp_game->flags |= NETFLAG_ENABLE_RADAR;
//end this section addition
  if(multivalues[18])
   temp_game->flags |= NETFLAG_ENABLE_ALT_VULCAN;
//= multivalues[19];
   for(i=0;i<MULTI_ALLOW_POWERUP_MAX;i++)
    {
     if(multivalues[20+i])
      temp_game->flags |= (1 << i);
    }
#endif
}

#endif //ifdef NETWORK
