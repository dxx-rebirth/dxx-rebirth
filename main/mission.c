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
 * Code to handle multiple missions
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "mission.h"
#include "gameseq.h"
#include "titles.h"
#include "songs.h"
#include "error.h"
#include "config.h"
#include "newmenu.h"
#include "text.h"
#include "u_mem.h"
#include "ignorecase.h"

//mission list entry
typedef struct mle {
	char    *filename;          // filename without extension
	int     builtin_hogsize;    // if it's the built-in mission, used for determining the version
	char    mission_name[MISSION_NAME_LEN+1];
	ubyte   anarchy_only_flag;  // if true, mission is anarchy only
	char	*path;				// relative file path
	int		location;           // see defines below
} mle;

//values that describe where a mission is located
#define ML_CURDIR       0
#define ML_MISSIONDIR   1

int num_missions = -1;

Mission *Current_mission = NULL; // currently loaded mission

//
//  Special versions of mission routines for d1 builtins
//

int load_mission_d1(void)
{
	int i;

	switch (PHYSFSX_fsize("descent.hog"))
	{
		case D1_SHAREWARE_MISSION_HOGSIZE:
		case D1_SHAREWARE_10_MISSION_HOGSIZE:
			N_secret_levels = 0;
	
			Last_level = 7;
			Last_secret_level = 0;
	
			//build level names
			for (i=0;i<Last_level;i++)
				sprintf(Level_names[i], "level%02d.sdl", i+1);

			strcpy(Briefing_text_filename,BIMD1_BRIEFING_FILE);
			strcpy(Ending_text_filename,BIMD1_ENDING_FILE_SHARE);
	
			break;
		case D1_MAC_SHARE_MISSION_HOGSIZE:
			N_secret_levels = 0;
	
			Last_level = 3;
			Last_secret_level = 0;
	
			//build level names
			for (i=0;i<Last_level;i++)
				sprintf(Level_names[i], "level%02d.sdl", i+1);

			strcpy(Briefing_text_filename,BIMD1_BRIEFING_FILE);
			strcpy(Ending_text_filename,BIMD1_ENDING_FILE_SHARE);
	
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

			strcpy(Briefing_text_filename,BIMD1_BRIEFING_FILE_OEM);
			strcpy(Ending_text_filename,BIMD1_ENDING_FILE_OEM);
	
			break;
		default:
			Int3(); // fall through
		case D1_MISSION_HOGSIZE:
		case D1_10_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			N_secret_levels = 3;
	
			Last_level = BIMD1_LAST_LEVEL;
			Last_secret_level = BIMD1_LAST_SECRET_LEVEL;
	
			//build level names
			for (i=0;i<Last_level;i++)
				sprintf(Level_names[i], "level%02d.rdl", i+1);
			for (i=0;i<-Last_secret_level;i++)
				sprintf(Secret_level_names[i], "levels%1d.rdl", i+1);
	
			Secret_level_table[0] = 10;
			Secret_level_table[1] = 21;
			Secret_level_table[2] = 24;

			strcpy(Briefing_text_filename,BIMD1_BRIEFING_FILE);
			strcpy(Ending_text_filename,BIMD1_ENDING_FILE);
	
			break;
	}

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
char *get_parm_value(char *parm,PHYSFS_file *f)
{
	static char buf[80];

	if (!PHYSFSX_fgets(buf,80,f))
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

//returns 1 if file read ok, else 0
int read_mission_file(mle *mission, char *filename, int location)
{
	char filename2[100];
	PHYSFS_file *mfile;

	switch (location) {
		case ML_MISSIONDIR:
			strcpy(filename2,MISSION_DIR);
			break;

		default:
			Int3();		//fall through

		case ML_CURDIR:
			strcpy(filename2,"");
			break;
	}
	strcat(filename2,filename);

	mfile = PHYSFSX_openReadBuffered(filename2);

	if (mfile) {
		char *p;
		char temp[PATH_MAX], *ext;

		strcpy(temp,filename);
		p = strrchr(temp, '/');	// get the filename at the end of the path
		if (!p)
			p = temp;
		else p++;
		
		if ((ext = strchr(p, '.')) == NULL)
			return 0;	//missing extension

		*ext = 0;			//kill extension

		mission->path = d_strdup(temp);
		mission->anarchy_only_flag = 0;
		mission->filename = mission->path + (p - temp);
		mission->location = location;

		p = get_parm_value("name",mfile);

		if (p) {
			char *t;
			if ((t=strchr(p,';'))!=NULL)
				*t=0;
			t = p + strlen(p)-1;
			while (isspace(*t))
				*t-- = 0; // remove trailing whitespace
			if (strlen(p) > MISSION_NAME_LEN)
				p[MISSION_NAME_LEN] = 0;
			strncpy(mission->mission_name, p, MISSION_NAME_LEN + 1);
		}
		else {
			PHYSFS_close(mfile);
			d_free(mission->path);
			return 0;
		}

		p = get_parm_value("type",mfile);

		//get mission type
		if (p)
			mission->anarchy_only_flag = istok(p,"anarchy");

		PHYSFS_close(mfile);

		return 1;
	}

	return 0;
}

void add_d1_builtin_mission_to_list(mle *mission)
{
    int size;
    
	size = PHYSFSX_fsize("descent.hog");
	if (size == -1)
		return;

	switch (size) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		mission->filename = d_strdup(D1_MISSION_FILENAME);
		strcpy(mission->mission_name, D1_SHAREWARE_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		mission->filename = d_strdup(D1_MISSION_FILENAME);
		strcpy(mission->mission_name, D1_OEM_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	default:
		Warning("Unknown D1 hogsize %d\n", size);
		Int3();
		// fall through
	case D1_MISSION_HOGSIZE:
	case D1_MISSION_HOGSIZE2:
	case D1_10_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
		mission->filename = d_strdup(D1_MISSION_FILENAME);
		strcpy(mission->mission_name, D1_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	}

	mission->anarchy_only_flag = 0;
	mission->builtin_hogsize = size;
	mission->path = mission->filename;
	num_missions++;
}

void add_missions_to_list(mle *mission_list, char *path, char *rel_path, int anarchy_mode)
{
	char **find, **i, *ext;

	find = PHYSFS_enumerateFiles(path);

	for (i = find; *i != NULL; i++)
	{
		if (strlen(path) + strlen(*i) + 1 >= PATH_MAX)
			continue;	// path is too long

		strcat(rel_path, *i);
		if (PHYSFS_isDirectory(path))
		{
			strcat(rel_path, "/");
			add_missions_to_list(mission_list, path, rel_path, anarchy_mode);
			*(strrchr(path, '/')) = 0;
		}
		else if ((ext = strrchr(*i, '.')) && (!strnicmp(ext, ".msn", 4) || !strnicmp(ext, ".mn2", 4)))
			if (read_mission_file(&mission_list[num_missions], rel_path, ML_MISSIONDIR))
			{
				if (anarchy_mode || !mission_list[num_missions].anarchy_only_flag)
				{
					mission_list[num_missions].builtin_hogsize = 0;
					num_missions++;
				}
				else
					d_free(mission_list[num_missions].path);
			}
		
		if (num_missions >= MAX_MISSIONS)
		{
			break;
		}

		(strrchr(path, '/'))[1] = 0;	// chop off the entry
	}

	PHYSFS_freeList(find);
}

/* move <mission_name> to <place> on mission list, increment <place> */
void promote (mle *mission_list, char * mission_name, int * top_place)
{
	int i;
	char name[FILENAME_LEN], * t;
	strcpy(name, mission_name);
	if ((t = strchr(name,'.')) != NULL)
		*t = 0; //kill extension
	for (i = *top_place; i < num_missions; i++)
		if (!stricmp(mission_list[i].filename, name)) {
			//swap mission positions
			mle temp;

			temp = mission_list[*top_place];
			mission_list[*top_place] = mission_list[i];
			mission_list[i] = temp;
			++(*top_place);
			break;
		}
}

void free_mission(void)
{
    // May become more complex with the editor
    if (Current_mission)
	{
		if (!PLAYING_BUILTIN_MISSION)
		{
			char hogpath[PATH_MAX];

			sprintf(hogpath, MISSION_DIR "%s.hog", Current_mission->path);
			PHYSFSX_contfile_close(hogpath);
		}
		d_free(Current_mission->path);
        d_free(Current_mission);
    }
}



//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode is set, then also add anarchy-only missions.

mle *build_mission_list(int anarchy_mode)
{
	mle *mission_list;
	int top_place;
	char	search_str[PATH_MAX] = MISSION_DIR;

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

	MALLOC(mission_list, mle, MAX_MISSIONS);
	num_missions = 0;
	
	add_d1_builtin_mission_to_list(mission_list + num_missions);
	add_missions_to_list(mission_list, search_str, search_str + strlen(search_str), anarchy_mode);
	
	// move original missions (in story-chronological order)
	// to top of mission list
	top_place = 0;
	promote(mission_list, "", &top_place); // original descent 1 mission

	if (num_missions > top_place)
		qsort(&mission_list[top_place],
		      num_missions - top_place,
		      sizeof(*mission_list),
 				(int (*)( const void *, const void * ))ml_sort_func);


	if (num_missions > top_place)
		qsort(&mission_list[top_place],
		      num_missions - top_place,
		      sizeof(*mission_list),
		      (int (*)( const void *, const void * ))ml_sort_func);

	return mission_list;
}

void free_mission_list(mle *mission_list)
{
	int i;

	for (i = 0; i < num_missions; i++)
		d_free(mission_list[i].path);
	
	d_free(mission_list);
	num_missions = 0;
}

void init_extra_robot_movie(char *filename);

//values for built-in mission

//loads the specfied mission from the mission list.
//build_mission_list() must have been called.
//Returns true if mission loaded ok, else false.
int load_mission(mle *mission)
{
	PHYSFS_file *mfile;
	char buf[PATH_MAX], *v;
	
	if (Current_mission)
		free_mission();
	Current_mission = d_malloc(sizeof(Mission));
	if (!Current_mission) return 0;
	*(mle *) Current_mission = *mission;
	Current_mission->path = d_strdup(mission->path);
	Current_mission->filename = Current_mission->path + (mission->filename - mission->path);
	Current_mission->n_secret_levels = 0;

	//init vars
	Last_level = 0;
	Last_secret_level = 0;
	memset(&Briefing_text_filename, '\0', sizeof(Briefing_text_filename));
	memset(&Ending_text_filename, '\0', sizeof(Ending_text_filename));

	// for Descent 1 missions, load descent.hog
	if (!PHYSFSX_contfile_init("descent.hog", 1))
		Error("descent.hog not available!\n");
	if (!stricmp(Current_mission_filename, D1_MISSION_FILENAME))
		return load_mission_d1();

	//read mission from file

	switch (mission->location) {
	case ML_MISSIONDIR:
		strcpy(buf,MISSION_DIR);
		break;
	default:
		Int3();							//fall through
	case ML_CURDIR:
		strcpy(buf,"");
		break;
	}
	strcat(buf, mission->path);
	strcat(buf,".msn");

	PHYSFSEXT_locateCorrectCase(buf);

	mfile = PHYSFSX_openReadBuffered(buf);
	if (mfile == NULL) {
        	free_mission();
		return 0;		//error!
	}

	//for non-builtin missions, load HOG
	strcpy(buf+strlen(buf)-4,".hog");		//change extension
	PHYSFSEXT_locateCorrectCase(buf);
	if (PHYSFSX_exists(buf,1))
		PHYSFSX_contfile_init(buf, 0);

	snprintf(Briefing_text_filename, sizeof(Briefing_text_filename), "%s.tex",Current_mission_filename);
	if (!PHYSFSX_exists(Briefing_text_filename,1))
		snprintf(Briefing_text_filename, sizeof(Briefing_text_filename), "%s.txb",Current_mission_filename);
	snprintf(Ending_text_filename, sizeof(Ending_text_filename), "%s.tex",Current_mission_filename);
	if (!PHYSFSX_exists(Ending_text_filename,1))
		snprintf(Ending_text_filename, sizeof(Ending_text_filename), "%s.txb",Current_mission_filename);

	while (PHYSFSX_fgets(buf,80,mfile)) {
		if (istok(buf,"type"))
			continue;						//already have name, go to next line
		else if (istok(buf,"briefing")) {
			if ((v = get_value(buf)) != NULL) {
				add_term(v);
				if (strlen(v) < FILENAME_LEN && strlen(v) > 0)
				{
					char *tmp, *ptr;
					MALLOC(tmp, char, FILENAME_LEN);
					snprintf(tmp, FILENAME_LEN, "%s", v);
					if ((ptr = strrchr(tmp, '.'))) // if there's a filename extension, kill it. No one knows it's the right one.
						*ptr = '\0';
					strncat(tmp, ".tex", sizeof(char)*FILENAME_LEN); // apply tex-extenstion
					if (PHYSFSX_exists(tmp,1)) // check if this file exists ...
						snprintf(Briefing_text_filename, FILENAME_LEN, "%s", tmp); // ... and apply ...
					else // ... otherwise ...
					{
						if ((ptr = strrchr(tmp, '.')))
							*ptr = '\0';
						strncat(tmp, ".txb", sizeof(char)*FILENAME_LEN); // apply txb extension
						if (PHYSFSX_exists(tmp,1)) // check if this file exists ...
							snprintf(Briefing_text_filename, FILENAME_LEN, "%s", tmp); // ... and apply ...
					}
					d_free(tmp);
				}
			}
		}
		else if (istok(buf,"ending")) {
			if ((v = get_value(buf)) != NULL) {
				add_term(v);
				if (strlen(v) < FILENAME_LEN && strlen(v) > 0)
				{
					char *tmp, *ptr;
					MALLOC(tmp, char, FILENAME_LEN);
					snprintf(tmp, FILENAME_LEN, "%s", v);
					if ((ptr = strrchr(tmp, '.'))) // if there's a filename extension, kill it. No one knows it's the right one.
						*ptr = '\0';
					strncat(tmp, ".tex", sizeof(char)*FILENAME_LEN); // apply tex-extenstion
					if (PHYSFSX_exists(tmp,1)) // check if this file exists ...
						snprintf(Briefing_text_filename, FILENAME_LEN, "%s", tmp); // ... and apply ...
					else // ... otherwise ...
					{
						if ((ptr = strrchr(tmp, '.')))
							*ptr = '\0';
						strncat(tmp, ".txb", sizeof(char)*FILENAME_LEN); // apply txb extension
						if (PHYSFSX_exists(tmp,1)) // check if this file exists ...
							snprintf(Ending_text_filename, FILENAME_LEN, "%s", tmp); // ... and apply ...
					}
					d_free(tmp);
				}
			}
		}
		else if (istok(buf,"num_levels")) {

			if ((v=get_value(buf))!=NULL) {
				int n_levels,i;

				n_levels = atoi(v);

				for (i=0;i<n_levels;i++) {
					PHYSFSX_fgets(buf,80,mfile);
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

				for (i=0;i<N_secret_levels;i++) {
					char *t;

					PHYSFSX_fgets(buf,80,mfile);
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

	PHYSFS_close(mfile);

	if (Last_level <= 0) {
		free_mission();		//no valid mission loaded
		return 0;
	}

	return 1;
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(char *mission_name)
{
	int i;
	mle *mission_list = build_mission_list(1);
	bool found = 0;

	for (i = 0; i < num_missions; i++)
		if (!stricmp(mission_name, mission_list[i].filename))
			found = load_mission(mission_list + i);

	free_mission_list(mission_list);
	return found;
}

typedef struct mission_menu
{
	mle *mission_list;
	int (*when_selected)(void);
} mission_menu;

int mission_menu_handler(listbox *lb, d_event *event, mission_menu *mm)
{
	char **list = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event->type)
	{
		case EVENT_NEWMENU_SELECTED:
			if (citem >= 0)
			{
				// Chose a mission
				strcpy(GameCfg.LastMission, list[citem]);
				
				if (!load_mission(mm->mission_list + citem))
				{
					nm_messagebox( NULL, 1, TXT_OK, TXT_MISSION_ERROR);
					return 1;	// stay in listbox so user can select another one
				}
			}
			return !(*mm->when_selected)();
			break;

		case EVENT_WINDOW_CLOSE:
			free_mission_list(mm->mission_list);
			d_free(list);
			d_free(mm);
			break;
			
		default:
			break;
	}
	
	return 0;
}

int select_mission(int anarchy_mode, char *message, int (*when_selected)(void))
{
    mle *mission_list = build_mission_list(anarchy_mode);
	int new_mission_num;

    if (num_missions <= 1)
	{
        new_mission_num = load_mission(mission_list) ? 0 : -1;
		free_mission_list(mission_list);
		(*when_selected)();
		
		return (new_mission_num >= 0);
    }
	else
	{
		mission_menu *mm;
        int i, default_mission;
        char **m;
		
		MALLOC(m, char *, num_missions);
		if (!m)
		{
			free_mission_list(mission_list);
			return 0;
		}
		
		MALLOC(mm, mission_menu, 1);
		if (!mm)
		{
			d_free(m);
			free_mission_list(mission_list);
			return 0;
		}

		mm->mission_list = mission_list;
		mm->when_selected = when_selected;
		
        default_mission = 0;
        for (i = 0; i < num_missions; i++) {
            m[i] = mission_list[i].mission_name;
            if ( !stricmp( m[i], GameCfg.LastMission ) )
                default_mission = i;
        }

        newmenu_listbox1( message, num_missions, m, 1, default_mission, (int (*)(listbox *, d_event *, void *))mission_menu_handler, mm );
    }

    return 1;	// presume success
}

#ifdef EDITOR
void create_new_mission(void)
{
	if (Current_mission)
		free_mission();
	
	Current_mission = d_malloc(sizeof(Mission));
	if (!Current_mission)
		return;
	memset(Current_mission, 0, sizeof(Mission));
	
	Current_mission->path = d_strdup("new_mission");
	Current_mission->filename = Current_mission->path;
	
	strcpy(Level_names[0], "GAMESAVE.LVL");
}
#endif
