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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: mission.c,v 1.3 2002-01-29 10:11:31 bradleyb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "pstypes.h"
#include "cfile.h"

#include "strutil.h"
#include "inferno.h"
#include "mission.h"
#include "gameseq.h"
#include "titles.h"
#include "songs.h"
#include "mono.h"
#include "error.h"
#include "findfile.h"

mle Mission_list[MAX_MISSIONS];

int Current_mission_num;
int	N_secret_levels;		//	Made a global by MK for scoring purposes.  August 1, 1995.
char *Current_mission_filename,*Current_mission_longname;

//this stuff should get defined elsewhere

char Level_names[MAX_LEVELS_PER_MISSION][FILENAME_LEN];
char Secret_level_names[MAX_SECRET_LEVELS_PER_MISSION][FILENAME_LEN];

//where the missions go
#ifndef EDITOR
#define MISSION_DIR "missions/"
#else
#define MISSION_DIR "./"
#endif

#ifdef SHAREWARE

//
//  Special versions of mission routines for shareware
//

#define SHAREWARE_MISSION_FILENAME	"d2demo"
#define SHAREWARE_MISSION_NAME		"Descent 2 Demo"

int build_mission_list(int anarchy_mode)
{
	anarchy_mode++;		//kill warning

	strcpy(Mission_list[0].filename,SHAREWARE_MISSION_FILENAME);
	strcpy(Mission_list[0].mission_name,SHAREWARE_MISSION_NAME);
	Mission_list[0].anarchy_only_flag = 0;

	return load_mission(0);
}

int load_mission(int mission_num)
{
	Assert(mission_num == 0);

	Current_mission_num = 0;
	Current_mission_filename = Mission_list[0].filename;
	Current_mission_longname = Mission_list[0].mission_name;

	N_secret_levels = 0;

	Assert(Last_level == 3);

#ifdef MACINTOSH	// mac demo is using the regular hog and rl2 files
	strcpy(Level_names[0],"d2leva-1.rl2");
	strcpy(Level_names[1],"d2leva-2.rl2");
	strcpy(Level_names[2],"d2leva-3.rl2");
#else
	strcpy(Level_names[0],"d2leva-1.sl2");
	strcpy(Level_names[1],"d2leva-2.sl2");
	strcpy(Level_names[2],"d2leva-3.sl2");
#endif 	// end of ifdef macintosh

	return 1;
}

int load_mission_by_name(char *mission_name)
{
	if (strcmp(mission_name,SHAREWARE_MISSION_FILENAME))
		return 0;		//cannot load requested mission
	else
		return load_mission(0);

}



#else

#if defined(D2_OEM)

//
//  Special versions of mission routines for Diamond/S3 version
//

#define OEM_MISSION_FILENAME	"d2"
#define OEM_MISSION_NAME		"D2 Destination:Quartzon"

int build_mission_list(int anarchy_mode)
{
	anarchy_mode++;		//kill warning

	strcpy(Mission_list[0].filename,OEM_MISSION_FILENAME);
	strcpy(Mission_list[0].mission_name,OEM_MISSION_NAME);
	Mission_list[0].anarchy_only_flag = 0;

	return load_mission(0);
}

int load_mission(int mission_num)
{
	Assert(mission_num == 0);

	Current_mission_num = 0;
	Current_mission_filename = Mission_list[0].filename;
	Current_mission_longname = Mission_list[0].mission_name;

	N_secret_levels = 2;

	Assert(Last_level == 8);

	strcpy(Level_names[0],"d2leva-1.rl2");
	strcpy(Level_names[1],"d2leva-2.rl2");
	strcpy(Level_names[2],"d2leva-3.rl2");
	strcpy(Level_names[3],"d2leva-4.rl2");

	strcpy(Secret_level_names[0],"d2leva-s.rl2");

	strcpy(Level_names[4],"d2levb-1.rl2");
	strcpy(Level_names[5],"d2levb-2.rl2");
	strcpy(Level_names[6],"d2levb-3.rl2");
	strcpy(Level_names[7],"d2levb-4.rl2");

	strcpy(Secret_level_names[1],"d2levb-s.rl2");

	Secret_level_table[0] = 1;
	Secret_level_table[1] = 5;

	return 1;
}

int load_mission_by_name(char *mission_name)
{
	if (strcmp(mission_name,OEM_MISSION_FILENAME))
		return 0;		//cannot load requested mission
	else
		return load_mission(0);

}

#else

//strips damn newline from end of line
char *mfgets(char *s,int n,CFILE *f)
{
	char *r;

	r = cfgets(s,n,f);
	if (r && s[strlen(s)-1] == '\n')
		s[strlen(s)-1] = 0;

	return r;
}

//compare a string for a token. returns true if match
int istok(char *buf,char *tok)
{
	return strnicmp(buf,tok,strlen(tok)) == 0;

}

//adds a terminating 0 after a string at the first white space
void add_term(char *s)
{
	while (*s && !isspace(*s)) s++;

	*s = 0;		//terminate!
}

//returns ptr to string after '=' & white space, or NULL if no '='
//adds 0 after parm at first white space
char *get_value(char *buf)
{
	char *t;

	t = strchr(buf,'=')+1;

	if (t) {
		while (*t && isspace(*t)) t++;

		if (*t)
			return t;
	}

	return NULL;		//error!
}

//reads a line, returns ptr to value of passed parm.  returns NULL if none
char *get_parm_value(char *parm,CFILE *f)
{
	static char buf[80];
	
	if (!mfgets(buf,80,f))
		return NULL;

	if (istok(buf,parm))
		return get_value(buf);
	else
		return NULL;
}

int ml_sort_func(mle *e0,mle *e1)
{
	return stricmp(e0->mission_name,e1->mission_name);

}

extern char CDROM_dir[];
extern int HoardEquipped();

#define BUILTIN_MISSION "d2.mn2"

//returns 1 if file read ok, else 0
int read_mission_file(char *filename,int count,int location)
{
	char filename2[100];
	CFILE *mfile;

	switch (location) {
		case ML_MISSIONDIR:
			strcpy(filename2,MISSION_DIR);	
			break;

		case ML_CDROM:			
			songs_stop_redbook();		//so we can read from the CD
			strcpy(filename2,CDROM_dir);		
			break;

		default:					
			Int3();		//fall through

		case ML_CURDIR:		
			strcpy(filename2,"");
			break;
	}
	strcat(filename2,filename);

	mfile = cfopen(filename2,"rb");

	if (mfile) {
		char *p;
		char temp[FILENAME_LEN],*t;

		strcpy(temp,filename);
		if ((t = strchr(temp,'.')) == NULL)
			return 0;	//missing extension
		*t = 0;			//kill extension

		strncpy( Mission_list[count].filename, temp, 9 );
		Mission_list[count].anarchy_only_flag = 0;
		Mission_list[count].location = location;

		p = get_parm_value("name",mfile);

		if (!p) {		//try enhanced mission
			cfseek(mfile,0,SEEK_SET);
			p = get_parm_value("xname",mfile);
		}

#ifdef NETWORK
		if (HoardEquipped())
		{
			if (!p) {		//try super-enhanced mission!
				cfseek(mfile,0,SEEK_SET);
				p = get_parm_value("zname",mfile);
			}
		}
#endif

		if (p) {
			char *t;
			if ((t=strchr(p,';'))!=NULL)
				*t=0;
			t = p + strlen(p)-1;
			while (isspace(*t)) t--;
			strncpy(Mission_list[count].mission_name,p,MISSION_NAME_LEN);
		}
		else {
			cfclose(mfile);
			return 0;
		}

		p = get_parm_value("type",mfile);

		//get mission type 
		if (p)
			Mission_list[count].anarchy_only_flag = istok(p,"anarchy");

		cfclose(mfile);

		return 1;
	}

	return 0;
}


//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.
//if there is only one mission, this function will call load_mission on it.

extern char CDROM_dir[];
extern char AltHogDir[];
extern char AltHogdir_initialized;

int build_mission_list(int anarchy_mode)
{
	static int num_missions=-1;
	int count=0,special_count=0;
	FILEFINDSTRUCT find;
	char search_name[PATH_MAX + 5] = MISSION_DIR "*.mn2";

	//now search for levels on disk

//@@Took out this code because after this routine was called once for
//@@a list of single-player missions, a subsequent call for a list of
//@@anarchy missions would not scan again, and thus would not find the
//@@anarchy-only missions.  If we retain the minimum level of install,
//@@we may want to put the code back in, having it always scan for all
//@@missions, and have the code that uses it sort out the ones it wants.
//@@	if (num_missions != -1) {
//@@		if (Current_mission_num != 0)
//@@			load_mission(0);				//set built-in mission as default
//@@		return num_missions;
//@@	}

	if (!read_mission_file(BUILTIN_MISSION,0,ML_CURDIR))		//read built-in first
		Error("Could not find required mission file <%s>",BUILTIN_MISSION);

	special_count = count=1; 

	if( !FileFindFirst( search_name, &find ) ) {
		do	{
			if (stricmp(find.name,BUILTIN_MISSION)==0)
				continue;		//skip the built-in

			if (read_mission_file(find.name,count,ML_MISSIONDIR)) {

				if (anarchy_mode || !Mission_list[count].anarchy_only_flag)
					count++;
			}

		} while( !FileFindNext( &find ) && count<MAX_MISSIONS);
		FileFindClose();
	}

	if (AltHogdir_initialized) {
		strcpy(search_name, AltHogDir);
		strcat(search_name, "/" MISSION_DIR "*.mn2");
		if( !FileFindFirst( search_name, &find ) ) {
			do	{
				if (stricmp(find.name,BUILTIN_MISSION)==0)
					continue;		//skip the built-in

				if (read_mission_file(find.name,count,ML_MISSIONDIR)) {

					if (anarchy_mode || !Mission_list[count].anarchy_only_flag)
						count++;
				}

			} while( !FileFindNext( &find ) && count<MAX_MISSIONS);
			FileFindClose();
		}
	}

	//move vertigo to top of mission list
	{
		int i;

		for (i=special_count;i<count;i++)
			if (!stricmp(Mission_list[i].filename,"D2X")) {		//swap!
				mle temp;

				temp = Mission_list[special_count];
				Mission_list[special_count] = Mission_list[i];
				Mission_list[i] = temp;

				special_count++;

				break;
			}
	}


	if (count>special_count)
		qsort(&Mission_list[special_count],count-special_count,sizeof(*Mission_list),
				(int (*)( const void *, const void * ))ml_sort_func);

	load_mission(0);			//set built-in mission as default

	num_missions = count;

	return count;
}

void init_extra_robot_movie(char *filename);

//values for built-in mission

//loads the specfied mission from the mission list.  build_mission_list()
//must have been called.  If build_mission_list() returns 0, this function
//does not need to be called.  Returns true if mission loaded ok, else false.
int load_mission(int mission_num)
{
	CFILE *mfile;
	char buf[80], *v;
	int found_hogfile;
	int enhanced_mission = 0;

	Current_mission_num = mission_num;

	mprintf(( 0, "Loading mission %d\n", mission_num ));

	//read mission from file 

	switch (Mission_list[mission_num].location) {
		case ML_MISSIONDIR:	strcpy(buf,MISSION_DIR);	break;
		case ML_CDROM:			strcpy(buf,CDROM_dir);		break;
		default:					Int3();							//fall through
		case ML_CURDIR:		strcpy(buf,"");				break;
	}
	strcat(buf,Mission_list[mission_num].filename);
	strcat(buf,".mn2");

	mfile = cfopen(buf,"rb");
	if (mfile == NULL) {
		Current_mission_num = -1;
		return 0;		//error!
	}

	if (mission_num != 0) {		//for non-builtin missions, load HOG

		strcpy(buf+strlen(buf)-4,".hog");		//change extension

		found_hogfile = cfile_use_alternate_hogfile(buf);

		#ifdef RELEASE				//for release, require mission to be in hogfile
		if (! found_hogfile) {
			cfclose(mfile);
			Current_mission_num = -1;
			return 0;
		}
		#endif
	}

	//init vars
	Last_level = 		0;
	Last_secret_level = 0;

	while (mfgets(buf,80,mfile)) {

		if (istok(buf,"name"))
			continue;						//already have name, go to next line
		if (istok(buf,"xname")) {
			enhanced_mission = 1;
			continue;						//already have name, go to next line
		}
		if (istok(buf,"zname")) {
			enhanced_mission = 2;
			continue;						//already have name, go to next line
		}
		else if (istok(buf,"type"))
			continue;						//already have name, go to next line				
		else if (istok(buf,"hog")) {
			char	*bufp = buf;

			while (*(bufp++) != '=')
				;

			if (*bufp == ' ')
				while (*(++bufp) == ' ')
					;

			cfile_use_alternate_hogfile(bufp);
			mprintf((0, "Hog file override = [%s]\n", bufp));
		}
		else if (istok(buf,"num_levels")) {

			if ((v=get_value(buf))!=NULL) {
				int n_levels,i;

				n_levels = atoi(v);

				for (i=0;i<n_levels && mfgets(buf,80,mfile);i++) {

					add_term(buf);
					if (strlen(buf) <= 12) {
						strcpy(Level_names[i],buf);
						Last_level++;
					}
					else
						break;
				}

			}
		}
		else if (istok(buf,"num_secrets")) {
			if ((v=get_value(buf))!=NULL) {
				int i;

				N_secret_levels = atoi(v);

				Assert(N_secret_levels <= MAX_SECRET_LEVELS_PER_MISSION);

				for (i=0;i<N_secret_levels && mfgets(buf,80,mfile);i++) {
					char *t;

					
					if ((t=strchr(buf,','))!=NULL) *t++=0;
					else
						break;

					add_term(buf);
					if (strlen(buf) <= 12) {
						strcpy(Secret_level_names[i],buf);
						Secret_level_table[i] = atoi(t);
						if (Secret_level_table[i]<1 || Secret_level_table[i]>Last_level)
							break;
						Last_secret_level--;
					}
					else
						break;
				}

			}
		}

	}

	cfclose(mfile);

	if (Last_level <= 0) {
		Current_mission_num = -1;		//no valid mission loaded
		return 0;
	}

	Current_mission_filename = Mission_list[Current_mission_num].filename;
	Current_mission_longname = Mission_list[Current_mission_num].mission_name;

	if (enhanced_mission) {
		char t[50];
		extern void bm_read_extra_robots();
		sprintf(t,"%s.ham",Current_mission_filename);
		bm_read_extra_robots(t,enhanced_mission);
		strncpy(t,Current_mission_filename,6);
		strcat(t,"-l.mvl");
		init_extra_robot_movie(t);
	}

	return 1;
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(char *mission_name)
{
	int n,i;

	n = build_mission_list(1);

	for (i=0;i<n;i++)
		if (!stricmp(mission_name,Mission_list[i].filename))
			return load_mission(i);

	return 0;		//couldn't find mission
}

#endif
#endif

