//hudlog.c added 11/01/98 Matthew Mueller - log messages and score grid to files.
#include <time.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#define access(a,b) _access(a,b)
#endif
#include "hudmsg.h"
#include "multi.h"
#include "hudlog.h"

//added on 9/5/99 by Victor Rachels for \ or / usage
#include "d_slash.h"
//end this section addition -VR

int HUD_log_messages = 0;
int HUD_log_multi_autostart = 0;
int HUD_log_autostart = 0;
int fhudmulti = 0;
FILE *fhudlog=NULL;
char hudlogdir[128]="";
char hudlogname[143];

void hud_log_setdir(char *dir){
	int l=strlen(dir);
	strcpy(hudlogdir,dir);
//added/edited on 9/5/99 by Victor Rachels for \ or / usage
        if (l && hudlogdir[l-1]!=USEDSLASH){
                hudlogdir[l++]=USEDSLASH;
//end this section edit - VR
		hudlogdir[l]=0;
	}
}

void hud_log_check_multi_start(void){
#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && HUD_log_multi_autostart){
		if (!fhudlog || !fhudmulti)
			open_hud_log();//don't reopen the log file (for example, when changing levels in a multi player game)
	}
#endif
}
void hud_log_check_multi_stop(void){
#ifdef NETWORK
	if (HUD_log_multi_autostart && fhudmulti){
		close_hud_log();
	}
#endif
}

void open_hud_log(void){
	static int num=0;
//edited 02/06/99 Matthew Mueller - changed filename to include date
	time_t t;
	struct tm *lt;

	if (fhudlog)close_hud_log();

#ifdef NETWORK
	fhudmulti = Game_mode & GM_MULTI;
#endif

	t=time(NULL);
	lt=localtime(&t);

	do{
//		sprintf(hudlogname,"%shud%05d.log",hudlogdir,num++);
 //edited 03/22/99 Matthew Mueller - tm_mon is 0 based.
		sprintf(hudlogname,"%s%04d%02d%02d.%03d",hudlogdir,lt->tm_year+1900,lt->tm_mon+1,lt->tm_mday,num++);
 //end edit -MM
//end edit -MM
	}while (!access(hudlogname,0));
	if (!(fhudlog=fopen(hudlogname,"w"))){
		hud_message(MSGC_GAME_FEEDBACK,"error opening %s",hudlogname);
		return;
	}
	hud_message(MSGC_GAME_FEEDBACK,"logging messages to %s",hudlogname);
}

static int recurse_flag=0;
void close_hud_log(void){
	if (fhudlog){
		++recurse_flag;
		kmatrix_log(1);
		hud_message(MSGC_GAME_FEEDBACK,"closing hud log file");
		fclose(fhudlog);
		fhudlog=NULL;
		--recurse_flag;
	}
}

void toggle_hud_log(void){
	if (fhudlog)close_hud_log();
	else open_hud_log();
}
void hud_log_message(char * message){
//	printf("hud_log_message %i %i %i %p: %s\n",recurse_flag,HUD_log_autostart,HUD_log_multi_autostart,fhudlog,message);
	if (++recurse_flag==1){//avoid infinite loop.  doh.
		if (fhudlog){
#ifdef NETWORK
			if (HUD_log_multi_autostart && ((Game_mode & GM_MULTI) != fhudmulti))
				close_hud_log();//if we are using -hudlog_multi, presumably we would want multiplayer games in seperate log files than single player, so close the log file and let a new one be reopened if needed.
#endif
		}
		if (!fhudlog){
			if (HUD_log_autostart)
				open_hud_log();
#ifdef NETWORK
			else if ((Game_mode & GM_MULTI) && HUD_log_multi_autostart){
				open_hud_log();
			}
#endif
		}
	}
	--recurse_flag;
	if (HUD_log_messages||fhudlog){
		time_t t;
		struct tm *lt;
		t=time(NULL);
		lt=localtime(&t);
//02/06/99 Matthew Mueller - added zero padding to hour
		if (HUD_log_messages)
		     printf("%02i:%02i:%02i ",lt->tm_hour,lt->tm_min,lt->tm_sec);
		if (fhudlog)
			fprintf(fhudlog,"%02i:%02i:%02i ",lt->tm_hour,lt->tm_min,lt->tm_sec);
		while (*message){
			if (*message>=0x01 && *message<=0x03){//filter out color codes
				message++;
				if (!*message)break;
			}else if (*message>=0x04 && *message<=0x06){//filter out color reset code
			}else{
				if (HUD_log_messages)
					printf("%c",*message);
				if (fhudlog)
					fprintf(fhudlog,"%c",*message);
			}
			message++;
		}
		if (HUD_log_messages)
			printf("\n");
		if (fhudlog){
			fprintf(fhudlog,"\n");
			//added 05/17/99 Matt Mueller - flush file to make sure it all gets out there
			fflush(fhudlog);
			//end addition -MM

		}
//end edit -MM
	}
}

void kmatrix_print(FILE* out,int *sorted){
#ifdef NETWORK
	int i,j;
	time_t t;
	struct tm *lt;
	t=time(NULL);
	lt=localtime(&t);
	//added 05/19/99 Matt Mueller - print mission name
	fprintf(out,"Mission: ");
#ifndef SHAREWARE
   if(!Netgame.mission_name)
#endif
    fprintf(out,"Descent: First Strike\n");
#ifndef SHAREWARE
   else
    fprintf(out,"%s\n",Netgame.mission_name);
#endif
   //end addition -MM
	fprintf(out,"%2i:%02i:%02i ",lt->tm_hour,lt->tm_min,lt->tm_sec);
	for (j=0; j<N_players; j++)
	     fprintf(out,"%9s",Players[sorted[j]].callsign);
	fprintf(out,"%9s\n","kills");

	for (i=0; i<N_players; i++){
		fprintf(out,"%9s",Players[sorted[i]].callsign);
		for (j=0; j<N_players; j++){
			if (sorted[i]==sorted[j]) {
				if (kill_matrix[sorted[i]][sorted[j]] == 0)
				     fprintf(out,"%9d",kill_matrix[sorted[i]][sorted[j]]);
				else
				     fprintf(out,"%9d",-kill_matrix[sorted[i]][sorted[j]]);
			}else{
				fprintf(out,"%9d",kill_matrix[sorted[i]][sorted[j]]);
			}
		}
		fprintf(out,"%9d\n",Players[sorted[i]].net_kills_total);
	}
	fprintf(out,"%9s","deaths:");
	for (j=0; j<N_players; j++)
	     fprintf(out,"%9d",Players[sorted[j]].net_killed_total);
	fprintf(out,"\n");
	//added 05/19/99 Matt Mueller - flush file to make sure it all gets out there
	fflush(out);
	//end addition -MM
#endif 
}

void kmatrix_log(int fhudonly){
#ifdef NETWORK
	int sorted[MAX_NUM_NET_PLAYERS];

	if (!(Game_mode & GM_MULTI)) return;

	multi_sort_kill_list();

	multi_get_kill_list(sorted);
	
	if (fhudlog)
	     kmatrix_print(fhudlog,sorted);
	if (HUD_log_messages && !fhudonly)
	     kmatrix_print(stdout,sorted);
#endif
}
