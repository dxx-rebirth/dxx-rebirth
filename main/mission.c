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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/mission.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:22 $
 * 
 * Code to handle multiple missions
 * 
 * $Log: mission.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:22  zicodxx
 * initial import
 *
 * Revision 1.2  1999/09/05 11:22:50  sekmu
 * windows mission and hudlog altdirs
 *
 * Revision 1.1.1.1  1999/06/14 22:08:47  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.9  1995/05/26  16:16:32  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 * 
 * Revision 2.8  1995/03/20  15:49:31  mike
 * Remove eof char from comment which confused make depend, causing
 * no mission.obj: line in makefile.  Pretty stupid tool, huh?
 * 
 * Revision 2.7  1995/03/20  12:12:11  john
 * Added ifdef SATURN.
 * 
 * Revision 2.6  1995/03/15  14:32:49  john
 * Added code to force the Descent CD-rom in the drive.
 * 
 * Revision 2.5  1995/03/15  11:41:15  john
 * Better Saturn CD-ROM support.
 * 
 * Revision 2.4  1995/03/14  18:24:14  john
 * Force Destination Saturn to use CD-ROM drive.
 * 
 * Revision 2.3  1995/03/07  14:19:41  mike
 * More destination saturn stuff.
 * 
 * Revision 2.2  1995/03/06  23:09:03  mike
 * more saturn stuff: make briefings work for saturn mission.
 * 
 * Revision 2.1  1995/03/06  16:47:48  mike
 * destination saturn
 * 
 * Revision 2.0  1995/02/27  11:27:49  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.14  1995/02/15  11:30:37  john
 * Fixed bug with potential mem overwrite on line 160.
 * 
 * Revision 1.13  1995/02/10  17:53:20  matt
 * Changed mission name again
 * 
 * Revision 1.12  1995/02/10  17:35:38  matt
 * Changed name of built-in mission
 * 
 * Revision 1.11  1995/02/07  17:13:51  allender
 * removed return statement in load_mission so that built in mission will
 * actually set the Current_mission_name and _filename
 * 
 * Revision 1.10  1995/01/31  01:19:45  matt
 * Made build_mission_list() sort missions by name
 * 
 * Revision 1.9  1995/01/30  13:49:58  allender
 * changed build_mission_list in load_mission_by_name to include
 * anarchy levels.
 * 
 * Revision 1.8  1995/01/30  13:03:51  matt
 * Fixed dumb mistake
 * 
 * Revision 1.7  1995/01/30  12:55:22  matt
 * Added vars to point to mission names
 * 
 * Revision 1.6  1995/01/22  18:57:28  matt
 * Made player highest level work with missions
 * 
 * Revision 1.5  1995/01/22  14:13:08  matt
 * Added flag in mission list for anarchy-only missions
 * 
 * Revision 1.4  1995/01/21  23:13:12  matt
 * Made high scores with (not work, really) with loaded missions
 * Don't give player high score when quit game
 * 
 * Revision 1.3  1995/01/21  16:27:12  matt
 * Made endlevel briefing work with missions
 * 
 * Revision 1.2  1995/01/20  22:47:50  matt
 * Mission system implemented, though imcompletely
 * 
 * Revision 1.1  1995/01/20  13:42:19  matt
 * Initial revision
 * 
 * 
 */

#ifndef SHAREWARE
#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: mission.c,v 1.1.1.1 2006/03/17 19:42:22 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cfile.h"

#include "inferno.h"
#include "mission.h"
#include "gameseq.h"
#include "titles.h"
#include "mono.h"
#include "error.h"
#include "d_glob.h"

//added on 9/5/99 by Victor Rachels
#include "d_slash.h"
//end this section addition - VR

mle Mission_list[MAX_MISSIONS];

int Current_mission_num;
char *Current_mission_filename,*Current_mission_longname;

//this stuff should get defined elsewhere

char Level_names[MAX_LEVELS_PER_MISSION][13];
char Secret_level_names[MAX_SECRET_LEVELS_PER_MISSION][13];

//strips damn newline from end of line
char *mfgets(char *s,int n,FILE *f)
{
	char *r;

	r = fgets(s,n,f);
	if (r && s[strlen(s)-1] == '\n')
		s[strlen(s)-1] = 0;

	return r;
}

//compare a string for a token. returns true if match
int istok(char *buf,char *tok)
{
	return strncasecmp(buf,tok,strlen(tok)) == 0;

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
char *get_parm_value(char *parm,FILE *f)
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
	return strcmp(e0->mission_name,e1->mission_name);

}


//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.
//if there is only one mission, this function will call load_mission on it.
int build_mission_list(int anarchy_mode)
{
	int count=0;
	char missionspec[256];
	d_glob_t glob_ret;

	//fill in built-in level

#ifndef DEST_SAT
		strcpy(Mission_list[0].filename,"");		//no filename for builtin
		strcpy(Mission_list[0].mission_name,"Descent: First Strike");
		count = 1;
#endif


	strcpy(missionspec, AltHogDir);
	strcat(missionspec, "*.msn");

	//now search for levels on disk

	if ( !d_glob(missionspec, &glob_ret) && glob_ret.gl_pathc) {
		int j;
		for (j = 0; j < glob_ret.gl_pathc && (count < MAX_MISSIONS); j++) {
			FILE *mfile;
			int is_anarchy;
			char temp[13], *t;
                        char openfile[128];
//added/edited on 9/5/99 by Victor Rachels for \ or /
                        if ((t = strrchr(glob_ret.gl_pathv[j], USEDSLASH)))
//end this section edit - VR
				t++;
			else
				t = glob_ret.gl_pathv[j];
			if (strlen(t) > 12)
				continue;
			strcpy(temp, t);

			if ((t = strchr(temp,'.')) == NULL)
				continue;
			*t = 0;			//kill extension

			strncpy( Mission_list[count].filename, temp, MISSION_FILENAME_LEN );
			Mission_list[count].anarchy_only_flag = is_anarchy = 0;

//added/edited on 9/5/99 by Victor Rachels for windows open
                   #ifdef __WINDOWS__
                   sprintf(openfile,"%s%s",AltHogDir,glob_ret.gl_pathv[j]);
                   #else
                   sprintf(openfile,"%s",glob_ret.gl_pathv[j]);
                   #endif
                   mfile = fopen(openfile, "rt");
//end this section add/edit - VR

			if (mfile) {
				char *p;

				p = get_parm_value("name",mfile);

				if (p) {
					char *t;
					if ((t=strchr(p,';'))!=NULL)
						*t=0;
					t = p + strlen(p)-1;
					while (isspace(*t)) t--;
					strncpy(Mission_list[count].mission_name,p,MISSION_NAME_LEN);
				}
				else {
					fclose(mfile);
					continue;			//abort this mission file
				}

				p = get_parm_value("type",mfile);

				//get mission type 
				if (p)
					Mission_list[count].anarchy_only_flag = is_anarchy = istok(p,"anarchy");

				fclose(mfile);

				if (!anarchy_mode && is_anarchy)
					continue;		//skip this mission

				count++;
			}

		}
		d_globfree(&glob_ret);
	}
#ifdef USE_CD
	if ( strlen(destsat_cdpath) )	{
		int i;
		char temp_spec[128];
		strcpy( temp_spec, destsat_cdpath );
		strcat( temp_spec, "*.msn" );
		if ( !d_glob(temp_spec, &glob_ret) && glob_ret.gl_pathc) {
			int j;
			for (j = 0; j < glob_ret.gl_pathc && (count < MAX_MISSIONS); j++) {
                                FILE *mfile;
				int is_anarchy;
				char temp[13], *t;

//added/edited on 9/5/99 by Victor Rachels for \ or /
                                if ((t = strrchr(glob_ret.gl_pathv[j], USEDSLASH)))
//end this section edit - VR
					t++;
				else
					t = glob_ret.gl_pathv[j];
                                if (strlen(t) > 12)
					continue;
				strcpy(temp, t);

                                if ((t = strchr(temp,'.')) == NULL)
					continue;
				*t = 0;			//kill extension

				for (i=0; i<count; i++ )	{
					if (!strcasecmp( Mission_list[i].filename, temp))
						break;
				}
				if ( i < count ) continue;		// Don't use same mission twice!
		
				strncpy( Mission_list[count].filename, temp, MISSION_FILENAME_LEN );
				Mission_list[count].anarchy_only_flag = is_anarchy = 0;
	
				mfile = fopen(glob_ret.gl_pathv[j], "rt");
				if (mfile) {
					char *p;
	
					p = get_parm_value("name",mfile);
	
					if (p) {
						char *t;
						if ((t=strchr(p,';'))!=NULL)
							*t=0;
						t = p + strlen(p)-1;
						while (isspace(*t)) t--;
						strncpy(Mission_list[count].mission_name,p,MISSION_NAME_LEN);
					}
					else {
						fclose(mfile);
						continue;			//abort this mission file
					}
	
					p = get_parm_value("type",mfile);
	
					//get mission type 
					if (p)
						Mission_list[count].anarchy_only_flag = is_anarchy = istok(p,"anarchy");
	
					fclose(mfile);
	
					if (!anarchy_mode && is_anarchy)
						continue;		//skip this mission
	
					count++;
				}
	
			}
			d_glob_free(&glob_ret);
		}
	}
#endif

	if (count>1)
		qsort(&Mission_list[1],count-1,sizeof(*Mission_list),(int (*)(const void *, const void *))ml_sort_func);

	load_mission(0);			//set built-in mission as default

	return count;
}

//values for built-in mission

#define BIM_LAST_LEVEL			27
#define BIM_LAST_SECRET_LEVEL	-3
#define BIM_BRIEFING_FILE		"briefing.tex"
#define BIM_ENDING_FILE			"endreg.tex"

//loads the specfied mission from the mission list.  build_mission_list()
//must have been called.  If build_mission_list() returns 0, this function
//does not need to be called.  Returns true if mission loaded ok, else false.
int load_mission(int mission_num)
{
	Current_mission_num = mission_num;

	mprintf(( 0, "Loading mission %d\n", mission_num ));

#ifndef DEST_SAT
	if (mission_num == 0) {		//built-in mission
		int i;

#ifdef ROCKWELL_CODE
		Last_level = 7;
		Last_secret_level = 0;

		//build level names
		for (i=0;i<Last_level;i++)
			sprintf(Level_names[i], "level%02d.rdl", i+1);
#else
		Last_level = BIM_LAST_LEVEL;
		Last_secret_level = BIM_LAST_SECRET_LEVEL;

		//build level names
		for (i=0;i<Last_level;i++)
			sprintf(Level_names[i], "level%02d.rdl", i+1);
		for (i=0;i<-Last_secret_level;i++)
			sprintf(Secret_level_names[i], "levels%1d.rdl", i+1);

		Secret_level_table[0] = 10;
		Secret_level_table[1] = 21;
		Secret_level_table[2] = 24;
#endif
		strcpy(Briefing_text_filename,BIM_BRIEFING_FILE);
		strcpy(Ending_text_filename,BIM_ENDING_FILE);
		cfile_use_alternate_hogfile(NULL);		//disable alternate
	} else 
#endif
	{		 //NOTE LINK TO ABOVE IF!!!!!
			//read mission from file 
		FILE *mfile;
		char buf[80], tmp[80], *v;

		strcpy(buf,Mission_list[mission_num].filename);
		strcat(buf,".msn");

		strcpy(tmp,Mission_list[mission_num].filename);
		strcat(tmp,".hog");
		cfile_use_alternate_hogfile(tmp);

		mfile = fopen(buf,"rt");

		if ((mfile == NULL) && AltHogdir_initialized) {
			char temp_spec[128];
			strcpy( temp_spec, AltHogDir );
			strcat( temp_spec, buf );
			mfile = fopen( temp_spec, "rt" );
		}
#ifdef USE_CD
		if (mfile == NULL) {
			if ( strlen(destsat_cdpath) )	{
				char temp_spec[128];
				strcpy( temp_spec, destsat_cdpath );
				strcat( temp_spec, buf );
				mfile = fopen( temp_spec, "rt" );
			}
		}
#endif
		if (mfile == NULL) {
			Current_mission_num = -1;
			return 0;		//error!
		}

		//init vars
		Last_level = 		0;
		Last_secret_level = 0;
		Briefing_text_filename[0] = 0;
		Ending_text_filename[0] = 0;
	
#ifdef DEST_SAT
		if (!strcasecmp(Mission_list[mission_num].filename, "DESTSAT")) {               //      Destination Saturn.
			strcpy(Briefing_text_filename,"briefsat.tex");
			strcpy(Ending_text_filename,"endsat.tex");
		}
#endif

		while (mfgets(buf,80,mfile)) {

			if (istok(buf,"name"))
				continue;						//already have name, go to next line
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
			} else if (istok(buf,"briefing")) {
				if ((v = get_value(buf)) != NULL) {
					add_term(v);
					if (strlen(v) < 13)
						strcpy(Briefing_text_filename,v);
				}
			}
			else if (istok(buf,"ending")) {
				if ((v = get_value(buf)) != NULL) {
					add_term(v);
					if (strlen(v) < 13)
						strcpy(Ending_text_filename,v);
				}
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
					int n_secret_levels,i;

					n_secret_levels = atoi(v);

					for (i=0;i<n_secret_levels && mfgets(buf,80,mfile);i++) {
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

		fclose(mfile);

		if (Last_level <= 0) {
			Current_mission_num = -1;		//no valid mission loaded 
			return 0;
		}
	}

	Current_mission_filename = Mission_list[Current_mission_num].filename;
	Current_mission_longname = Mission_list[Current_mission_num].mission_name;

	return 1;
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(char *mission_name)
{
	int n,i;

	n = build_mission_list(1);

	for (i=0;i<n;i++)
		if (!strcasecmp(mission_name,Mission_list[i].filename))
			return load_mission(i);

	return 0;		//couldn't find mission
}
#endif
