/* $Id: mission.c,v 1.29 2004-10-24 12:46:49 schaffner Exp $ */
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
 * Code to handle multiple missions
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
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
#include "config.h"
#include "newmenu.h"
#include "text.h"
#include "u_mem.h"

//values for d1 built-in mission
#define BIM_LAST_LEVEL          27
#define BIM_LAST_SECRET_LEVEL   -3
#define BIM_BRIEFING_FILE       "briefing.tex"
#define BIM_ENDING_FILE         "endreg.tex"

//mission list entry
typedef struct mle {
    char    mission_name[MISSION_NAME_LEN+1];
    char    filename[9];                // filename without extension
    ubyte   descent_version;            // descent 1 or descent 2?
    bool	anarchy_only_flag;          // if true, mission is anarchy only
    int		builtin_hogsize;	// This is used to determine the version. Only for the built in mission.
    ubyte   location;                   // see defines below
} mle;

//values that describe where a mission is located
#define ML_CURDIR       0
#define ML_MISSIONDIR   1
#define ML_CDROM        2

mle Mission_list[MAX_MISSIONS];

int num_missions = -1;

Mission *Current_mission = NULL; // currently loaded mission

//
//  Special versions of mission routines for d1 builtins
//

int load_mission_d1(void)
{
	int i;

	switch (cfile_size("descent.hog")) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
		N_secret_levels = 0;

		Last_level = 7;
		Last_secret_level = 0;

		//build level names
		for (i=0;i<Last_level;i++)
			sprintf(Level_names[i], "level%02d.sdl", i+1);

		break;
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		N_secret_levels = 0;

		Last_level = 3;
		Last_secret_level = 0;

		//build level names
		for (i=0;i<Last_level;i++)
			sprintf(Level_names[i], "level%02d.sdl", i+1);

		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		N_secret_levels = 1;

		Last_level = 15;
		Last_secret_level = -1;

		//build level names
		for (i=0; i < Last_level - 1; i++)
			sprintf(Level_names[i], "level%02d.rdl", i+1);
		sprintf(Level_names[i], "saturn%02d.rdl", i+1);
		for (i=0; i < -Last_secret_level; i++)
			sprintf(Secret_level_names[i], "levels%1d.rdl", i+1);

		Secret_level_table[0] = 10;

		break;
	default:
		Int3(); // fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		N_secret_levels = 3;

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

		break;
	}
	strcpy(Briefing_text_filename,BIM_BRIEFING_FILE);
	strcpy(Ending_text_filename,BIM_ENDING_FILE);

	return 1;
}


//
//  Special versions of mission routines for shareware
//

int load_mission_shareware(void)
{
    strcpy(Current_mission->mission_name, SHAREWARE_MISSION_NAME);
    Current_mission->descent_version = 2;
    Current_mission->anarchy_only_flag = 0;
    
    switch (Current_mission->builtin_hogsize) {
	case MAC_SHARE_MISSION_HOGSIZE:
		N_secret_levels = 1;

		Last_level = 4;
		Last_secret_level = -1;

		// mac demo is using the regular hog and rl2 files
		strcpy(Level_names[0],"d2leva-1.rl2");
		strcpy(Level_names[1],"d2leva-2.rl2");
		strcpy(Level_names[2],"d2leva-3.rl2");
		strcpy(Level_names[3],"d2leva-4.rl2");
		strcpy(Secret_level_names[0],"d2leva-s.rl2");
		break;
	default:
		Int3(); // fall through
	case SHAREWARE_MISSION_HOGSIZE:
		N_secret_levels = 0;

		Last_level = 3;
		Last_secret_level = 0;

		strcpy(Level_names[0],"d2leva-1.sl2");
		strcpy(Level_names[1],"d2leva-2.sl2");
		strcpy(Level_names[2],"d2leva-3.sl2");
	}

	return 1;
}


//
//  Special versions of mission routines for Diamond/S3 version
//

int load_mission_oem(void)
{
    strcpy(Current_mission->mission_name, OEM_MISSION_NAME);
    Current_mission->descent_version = 2;
    Current_mission->anarchy_only_flag = 0;
    
	N_secret_levels = 2;

	Last_level = 8;
	Last_secret_level = -2;

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

	if (!cfgets(buf,80,f))
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

//returns 1 if file read ok, else 0
int read_mission_file(char *filename,int count,int location)
{
	char filename2[100];
	CFILE *mfile;

	//printf("reading: %s\n", filename);

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
		// look if it's .mn2 or .msn
		Mission_list[count].descent_version = (t[3] == '2') ? 2 : 1;
		*t = 0;			//kill extension

		strncpy( Mission_list[count].filename, temp, 9 );
		Mission_list[count].anarchy_only_flag = 0;
		Mission_list[count].location = location;

		p = get_parm_value("name",mfile);

		if (!p) {		//try enhanced mission
			cfseek(mfile,0,SEEK_SET);
			p = get_parm_value("xname",mfile);
		}

		if (!p) {       //try super-enhanced mission!
			cfseek(mfile,0,SEEK_SET);
			p = get_parm_value("zname",mfile);
		}

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

void add_d1_builtin_mission_to_list(int *count)
{
    int size;
    
	if (!cfexist("descent.hog"))
		return;

	size = cfile_size("descent.hog");

	switch (size) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		strcpy(Mission_list[*count].filename, D1_MISSION_FILENAME);
		strcpy(Mission_list[*count].mission_name, D1_SHAREWARE_MISSION_NAME);
		Mission_list[*count].anarchy_only_flag = 0;
		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		strcpy(Mission_list[*count].filename, D1_MISSION_FILENAME);
		strcpy(Mission_list[*count].mission_name, D1_OEM_MISSION_NAME);
		Mission_list[*count].anarchy_only_flag = 0;
		break;
	default:
		Warning("Unknown D1 hogsize %d\n", size);
		Int3();
		// fall through
	case D1_MISSION_HOGSIZE:
	case D1_10_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		strcpy(Mission_list[*count].filename, D1_MISSION_FILENAME);
		strcpy(Mission_list[*count].mission_name, D1_MISSION_NAME);
		Mission_list[*count].anarchy_only_flag = 0;
		break;
	}

	Mission_list[*count].descent_version = 1;
	Mission_list[*count].anarchy_only_flag = 0;
	++(*count);
}


void add_builtin_mission_to_list(int *count, char *name)
{
    int size = cfile_size("descent2.hog");
    
	if (size == -1)
		size = cfile_size("d2demo.hog");

	switch (size) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		strcpy(Mission_list[*count].filename,SHAREWARE_MISSION_FILENAME);
		strcpy(Mission_list[*count].mission_name,SHAREWARE_MISSION_NAME);
		Mission_list[*count].anarchy_only_flag = 0;
		break;
	case OEM_MISSION_HOGSIZE:
		strcpy(Mission_list[*count].filename,OEM_MISSION_FILENAME);
		strcpy(Mission_list[*count].mission_name,OEM_MISSION_NAME);
		Mission_list[*count].anarchy_only_flag = 0;
		break;
	default:
		Warning("Unknown hogsize %d, trying %s\n", size, FULL_MISSION_FILENAME ".mn2");
		Int3(); //fall through
	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		if (!read_mission_file(FULL_MISSION_FILENAME ".mn2", 0, ML_CURDIR))
			Error("Could not find required mission file <%s>", FULL_MISSION_FILENAME ".mn2");
	}

	strcpy(name, Mission_list[*count].filename);
    Mission_list[*count].builtin_hogsize = size;
	Mission_list[*count].descent_version = 2;
	Mission_list[*count].anarchy_only_flag = 0;
	++(*count);
}


void add_missions_to_list(char *search_name, int *count, int anarchy_mode)
{
	FILEFINDSTRUCT find;
	if( !FileFindFirst( search_name, &find ) ) {
		do	{
			if (read_mission_file( find.name, *count, ML_MISSIONDIR )) {

				if (anarchy_mode || !Mission_list[*count].anarchy_only_flag)
					++(*count);
			}

		} while( !FileFindNext( &find ) && *count < MAX_MISSIONS);
		FileFindClose();
		if (*count >= MAX_MISSIONS)
			mprintf((0, "Warning: more missions than d2x can handle\n"));
	}
}

/* move <mission_name> to <place> on mission list, increment <place> */
void promote (char * mission_name, int * top_place, int num_missions)
{
	int i;
	char name[FILENAME_LEN], * t;
	strcpy(name, mission_name);
	if ((t = strchr(name,'.')) != NULL)
		*t = 0; //kill extension
	//printf("promoting: %s\n", name);
	for (i = *top_place; i < num_missions; i++)
		if (!stricmp(Mission_list[i].filename, name)) {
			//swap mission positions
			mle temp;

			temp = Mission_list[*top_place];
			Mission_list[*top_place] = Mission_list[i];
			Mission_list[i] = temp;
			++(*top_place);
			break;
		}
}

void free_mission(void)
{
    // May become more complex with the editor
    if (Current_mission) {
        d_free(Current_mission);
    }
}



//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode is set, then also add anarchy-only missions.

extern char CDROM_dir[];
extern char AltHogDir[];
extern char AltHogdir_initialized;

int build_mission_list(int anarchy_mode)
{
	static int num_missions=-1;
	int count = 0;
	int top_place;
    char	builtin_mission_filename[FILENAME_LEN];

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

	add_builtin_mission_to_list(&count, builtin_mission_filename);  //read built-in first
	add_d1_builtin_mission_to_list(&count);
	add_missions_to_list(MISSION_DIR "*.mn2", &count, anarchy_mode);
	add_missions_to_list(MISSION_DIR "*.msn", &count, anarchy_mode);

	if (AltHogdir_initialized) {
		char search_name[PATH_MAX + 5];
		strcpy(search_name, AltHogDir);
		strcat(search_name, "/" MISSION_DIR "*.mn2");
		add_missions_to_list(search_name, &count, anarchy_mode);
		strcpy(search_name, AltHogDir);
		strcat(search_name, "/" MISSION_DIR "*.msn");
		add_missions_to_list(search_name, &count, anarchy_mode);
	}

	// move original missions (in story-chronological order)
	// to top of mission list
	top_place = 0;
	promote("descent", &top_place, count); // original descent 1 mission
	promote(builtin_mission_filename, &top_place, count); // d2 or d2demo
	promote("d2x", &top_place, count); // vertigo

	if (count > top_place)
		qsort(&Mission_list[top_place],
		      count - top_place,
		      sizeof(*Mission_list),
 				(int (*)( const void *, const void * ))ml_sort_func);


	if (count > top_place)
		qsort(&Mission_list[top_place],
		      count - top_place,
		      sizeof(*Mission_list),
		      (int (*)( const void *, const void * ))ml_sort_func);

	//load_mission(0);   //set built-in mission as default

	num_missions = count;

    atexit(free_mission);

	return count;
}

void init_extra_robot_movie(char *filename);

//values for built-in mission

//loads the specfied mission from the mission list.
//build_mission_list() must have been called.
//Returns true if mission loaded ok, else false.
int load_mission(int mission_num)
{
	CFILE *mfile;
	char buf[80], *v;
    int found_hogfile;
	int enhanced_mission = 0;

    if (Current_mission)
        free_mission();
    Current_mission = d_malloc(sizeof(Mission));
    if (!Current_mission) return 0;
    *(mle *) Current_mission = Mission_list[mission_num];

    // for Descent 1 missions, load descent.hog
    if (EMULATING_D1) {
        if (!cfile_use_descent1_hogfile("descent.hog"))
            Warning("descent.hog not available, this mission may be missing some files required for briefings and exit sequence\n");
        if (!stricmp(Current_mission_filename, D1_MISSION_FILENAME))
            return load_mission_d1();
    }

    if (PLAYING_BUILTIN_MISSION) {
		switch (Current_mission->builtin_hogsize) {
		case SHAREWARE_MISSION_HOGSIZE:
		case MAC_SHARE_MISSION_HOGSIZE:
			return load_mission_shareware();
			break;
		case OEM_MISSION_HOGSIZE:
			return load_mission_oem();
			break;
		default:
			Int3(); // fall through
		case FULL_MISSION_HOGSIZE:
		case FULL_10_MISSION_HOGSIZE:
		case MAC_FULL_MISSION_HOGSIZE:
			// continue on... (use d2.mn2 from hogfile)
			break;
		}
    }

	mprintf(( 0, "Loading mission %s\n", Current_mission_filename ));

	//read mission from file

	switch (Mission_list[mission_num].location) {
	case ML_MISSIONDIR:
		strcpy(buf,MISSION_DIR);
		break;
	case ML_CDROM:
		strcpy(buf,CDROM_dir);
		break;
	default:
		Int3();							//fall through
	case ML_CURDIR:
		strcpy(buf,"");
		break;
	}
	strcat(buf,Mission_list[mission_num].filename);
	if (Mission_list[mission_num].descent_version == 2)
		strcat(buf,".mn2");
	else
		strcat(buf,".msn");

	mfile = cfopen(buf,"rb");
	if (mfile == NULL) {
        free_mission();
		return 0;		//error!
	}

    //for non-builtin missions, load HOG
    if (!PLAYING_BUILTIN_MISSION) {

        strcpy(buf+strlen(buf)-4,".hog");		//change extension

        found_hogfile = cfile_use_alternate_hogfile(buf);

#ifdef RELEASE				//for release, require mission to be in hogfile
        if (! found_hogfile) {
            cfclose(mfile);
            free_mission();
            return 0;
        }
#endif
    }

    //init vars
	Last_level = 0;
	Last_secret_level = 0;
	Briefing_text_filename[0] = 0;
	Ending_text_filename[0] = 0;

	while (cfgets(buf,80,mfile)) {

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
		else if (istok(buf,"briefing")) {
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

				for (i=0;i<n_levels && cfgets(buf,80,mfile);i++) {

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

				for (i=0;i<N_secret_levels && cfgets(buf,80,mfile);i++) {
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
		free_mission();		//no valid mission loaded
		return 0;
	}

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

int select_mission(int anarchy_mode, char *message)
{
    int n_missions;
    
    n_missions = build_mission_list(anarchy_mode);

    if (n_missions <= 1) {
        load_mission(0);
    } else {
        int new_mission_num,i, default_mission;
        char * m[MAX_MISSIONS];

        default_mission = 0;
        for (i=0;i<n_missions;i++) {
            m[i] = Mission_list[i].mission_name;
            if ( !stricmp( m[i], config_last_mission ) )
                default_mission = i;
        }

        new_mission_num = newmenu_listbox1( message, n_missions, m, 1, default_mission, NULL );

        if (new_mission_num == -1)
            return 0;         //abort!

        strcpy(config_last_mission, m[new_mission_num]  );

        if (!load_mission(new_mission_num)) {
            nm_messagebox( NULL, 1, TXT_OK, TXT_MISSION_ERROR);
            return 0;
        }
    }

    return 1;
}
