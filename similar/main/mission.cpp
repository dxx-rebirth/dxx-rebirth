/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#include <algorithm>
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
#include "dxxerror.h"
#include "config.h"
#include "newmenu.h"
#include "text.h"
#include "u_mem.h"
#include "ignorecase.h"
#include "physfsx.h"
#include "bm.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif

using std::min;

//values that describe where a mission is located
enum mle_loc
{
	ML_CURDIR = 0,
	ML_MISSIONDIR = 1
};

//mission list entry
struct mle
{
	char    *filename;          // filename without extension
	int     builtin_hogsize;    // if it's the built-in mission, used for determining the version
	char    mission_name[MISSION_NAME_LEN+1];
#if defined(DXX_BUILD_DESCENT_II)
	ubyte   descent_version;    // descent 1 or descent 2?
#endif
	ubyte   anarchy_only_flag;  // if true, mission is anarchy only
	char	*path;				// relative file path
	enum mle_loc	location;           // where the mission is
};

static int num_missions = -1;

Mission_ptr Current_mission; // currently loaded mission

// Allocate the Level_names, Secret_level_names and Secret_level_table arrays
static int allocate_levels(void)
{
	Level_names.reset(new d_fname[Last_level]);
	if (!Level_names)
		return 0;
	
	if (Last_secret_level)
	{
		N_secret_levels = -Last_secret_level;

		Secret_level_names.reset(new d_fname[N_secret_levels]);
		if (!Secret_level_names)
			return 0;
		
		Secret_level_table.reset(new ubyte[N_secret_levels]);
		if (!Secret_level_table)
			return 0;
	}
	
	return 1;
}

//
//  Special versions of mission routines for d1 builtins
//

static int load_mission_d1(void)
{
	int i;

	switch (PHYSFSX_fsize("descent.hog"))
	{
		case D1_SHAREWARE_MISSION_HOGSIZE:
		case D1_SHAREWARE_10_MISSION_HOGSIZE:
			N_secret_levels = 0;
	
			Last_level = 7;
			Last_secret_level = 0;
			
			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}
	
			//build level names
			for (i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0], Level_names[i].size(), "level%02d.sdl", i+1);
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = BIMD1_ENDING_FILE_SHARE;
			break;
		case D1_MAC_SHARE_MISSION_HOGSIZE:
			N_secret_levels = 0;
	
			Last_level = 3;
			Last_secret_level = 0;
	
			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}
			
			//build level names
			for (i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0], Level_names[i].size(), "level%02d.sdl", i+1);
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = BIMD1_ENDING_FILE_SHARE;
			break;
		case D1_OEM_MISSION_HOGSIZE:
		case D1_OEM_10_MISSION_HOGSIZE:
			N_secret_levels = 1;
	
			Last_level = 15;
			Last_secret_level = -1;
	
			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}
			
			//build level names
			for (i=0; i < Last_level - 1; i++)
				snprintf(&Level_names[i][0], Level_names[i].size(), "level%02d.rdl", i+1);
			snprintf(&Level_names[Last_level - 1][0], Level_names[Last_level - 1].size(), "saturn%02d.rdl", Last_level);
			for (i=0; i < -Last_secret_level; i++)
				snprintf(&Secret_level_names[i][0], Secret_level_names[i].size(), "levels%1d.rdl", i+1);
			Secret_level_table[0] = 10;
			Briefing_text_filename = BIMD1_BRIEFING_FILE_OEM;
			Ending_text_filename = BIMD1_ENDING_FILE_OEM;
			break;
		default:
			Int3(); // fall through
		case D1_MISSION_HOGSIZE:
		case D1_10_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			N_secret_levels = 3;
	
			Last_level = BIMD1_LAST_LEVEL;
			Last_secret_level = BIMD1_LAST_SECRET_LEVEL;
	
			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}

			//build level names
			for (i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0], Level_names[i].size(), "level%02d.rdl", i+1);
			for (i=0;i<-Last_secret_level;i++)
				snprintf(&Secret_level_names[i][0], Secret_level_names[i].size(), "levels%1d.rdl", i+1);
			Secret_level_table[0] = 10;
			Secret_level_table[1] = 21;
			Secret_level_table[2] = 24;
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = BIMD1_ENDING_FILE;
			break;
	}

	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
//
//  Special versions of mission routines for shareware
//

static int load_mission_shareware(void)
{
    strcpy(Current_mission->mission_name, SHAREWARE_MISSION_NAME);
    Current_mission->descent_version = 2;
    Current_mission->anarchy_only_flag = 0;
    
    switch (Current_mission->builtin_hogsize)
	{
		case MAC_SHARE_MISSION_HOGSIZE:
			N_secret_levels = 1;

			Last_level = 4;
			Last_secret_level = -1;

			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}
			
			// mac demo is using the regular hog and rl2 files
			Level_names[0] = "d2leva-1.rl2";
			Level_names[1] = "d2leva-2.rl2";
			Level_names[2] = "d2leva-3.rl2";
			Level_names[3] = "d2leva-4.rl2";
			Secret_level_names[0] = "d2leva-s.rl2";
			break;
		default:
			Int3(); // fall through
		case SHAREWARE_MISSION_HOGSIZE:
			N_secret_levels = 0;

			Last_level = 3;
			Last_secret_level = 0;

			if (!allocate_levels())
			{
				Current_mission.reset();
				return 0;
			}
			Level_names[0] = "d2leva-1.sl2";
			Level_names[1] = "d2leva-2.sl2";
			Level_names[2] = "d2leva-3.sl2";
	}

	return 1;
}


//
//  Special versions of mission routines for Diamond/S3 version
//

static int load_mission_oem(void)
{
    strcpy(Current_mission->mission_name, OEM_MISSION_NAME);
    Current_mission->descent_version = 2;
    Current_mission->anarchy_only_flag = 0;
    
	N_secret_levels = 2;

	Last_level = 8;
	Last_secret_level = -2;

	if (!allocate_levels())
	{
		Current_mission.reset();
		return 0;
	}
	Level_names[0] = "d2leva-1.rl2";
	Level_names[1] = "d2leva-2.rl2";
	Level_names[2] = "d2leva-3.rl2";
	Level_names[3] = "d2leva-4.rl2";
	Secret_level_names[0] = "d2leva-s.rl2";
	Level_names[4] = "d2levb-1.rl2";
	Level_names[5] = "d2levb-2.rl2";
	Level_names[6] = "d2levb-3.rl2";
	Level_names[7] = "d2levb-4.rl2";
	Secret_level_names[1] = "d2levb-s.rl2";
	Secret_level_table[0] = 1;
	Secret_level_table[1] = 5;
	return 1;
}
#endif

//compare a string for a token. returns true if match
static int istok(const char *buf,const char *tok)
{
	return d_strnicmp(buf,tok,strlen(tok)) == 0;

}

//adds a terminating 0 after a string at the first white space
static void add_term(char *s)
{
	while (*s && !isspace(*s)) s++;

	*s = 0;		//terminate!
}

//returns ptr to string after '=' & white space, or NULL if no '='
//adds 0 after parm at first white space
static char *get_value(char *buf)
{
	char *t = strchr(buf,'=');

	if (t) {
		while (isspace(static_cast<unsigned>(*++t)));

		if (*t)
			return t;
	}

	return NULL;		//error!
}

//reads a line, returns ptr to value of passed parm.  returns NULL if none
static char *get_parm_value(const char *parm,PHYSFS_file *f)
{
	static char buf[80];

	if (!PHYSFSX_fgets(buf,f))
		return NULL;

	if (istok(buf,parm))
		return get_value(buf);
	else
		return NULL;
}

static int ml_sort_func(mle *e0,mle *e1)
{
	return d_stricmp(e0->mission_name,e1->mission_name);

}

//returns 1 if file read ok, else 0
static int read_mission_file(mle *mission, const char *filename, enum mle_loc location)
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
#if defined(DXX_BUILD_DESCENT_II)
		// look if it's .mn2 or .msn
		mission->descent_version = (ext[3] == '2') ? 2 : 1;
#endif
		*ext = 0;			//kill extension

		mission->path = d_strdup(temp);
		mission->anarchy_only_flag = 0;
		mission->filename = mission->path + (p - temp);
		mission->location = location;

		p = get_parm_value("name",mfile);

#if defined(DXX_BUILD_DESCENT_II)
		if (!p) {		//try enhanced mission
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value("xname",mfile);
		}

		if (!p) {       //try super-enhanced mission!
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value("zname",mfile);
		}

		if (!p) {       //try extensible-enhanced mission!
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value("!name",mfile);
		}
#endif

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

		if (PHYSFSX_fgets(temp,mfile))
		{
			if (istok(temp,"type"))
			{
				p = get_value(temp);
				//get mission type
				if (p)
					mission->anarchy_only_flag = istok(p,"anarchy");
			}
		}

		PHYSFS_close(mfile);

		return 1;
	}

	return 0;
}

static void add_d1_builtin_mission_to_list(mle *mission)
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
#if defined(DXX_BUILD_DESCENT_I)
	mission->builtin_hogsize = size;
#elif defined(DXX_BUILD_DESCENT_II)
	mission->descent_version = 1;
	mission->builtin_hogsize = 0;
#endif
	mission->path = mission->filename;
	num_missions++;
}

#if defined(DXX_BUILD_DESCENT_II)
static void add_builtin_mission_to_list(mle *mission, char *name)
{
    int size = PHYSFSX_fsize("descent2.hog");
    
	if (size == -1)
		size = PHYSFSX_fsize("d2demo.hog");

	switch (size) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		mission->filename = d_strdup(SHAREWARE_MISSION_FILENAME);
		strcpy(mission->mission_name,SHAREWARE_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	case OEM_MISSION_HOGSIZE:
		mission->filename = d_strdup(OEM_MISSION_FILENAME);
		strcpy(mission->mission_name,OEM_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	default:
		Warning("Unknown hogsize %d, trying %s\n", size, FULL_MISSION_FILENAME ".mn2");
		Int3(); //fall through
	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		if (!read_mission_file(mission, FULL_MISSION_FILENAME ".mn2", ML_CURDIR))
			Error("Could not find required mission file <%s>", FULL_MISSION_FILENAME ".mn2");
	}

	mission->path = mission->filename;
	strcpy(name, mission->filename);
    mission->builtin_hogsize = size;
	mission->descent_version = 2;
	mission->anarchy_only_flag = 0;
	num_missions++;
}
#endif


static void add_missions_to_list(mle *mission_list, char *path, char *rel_path, int anarchy_mode)
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
		else if ((ext = strrchr(*i, '.')) && (!d_strnicmp(ext, ".msn", 4) || !d_strnicmp(ext, ".mn2", 4)))
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
static void promote (mle *mission_list, const char * mission_name, int * top_place)
{
	int i;
	char name[FILENAME_LEN], * t;
	strcpy(name, mission_name);
	if ((t = strchr(name,'.')) != NULL)
		*t = 0; //kill extension
	for (i = *top_place; i < num_missions; i++)
		if (!d_stricmp(mission_list[i].filename, name)) {
			//swap mission positions
			mle temp;

			temp = mission_list[*top_place];
			mission_list[*top_place] = mission_list[i];
			mission_list[i] = temp;
			++(*top_place);
			break;
		}
}

void free_mission(std::unique_ptr<Mission> Current_mission)
{
    // May become more complex with the editor
    if (Current_mission)
	{
		if (Current_mission->path && !PLAYING_BUILTIN_MISSION)
		{
			char hogpath[PATH_MAX];

			sprintf(hogpath, MISSION_DIR "%s.hog", Current_mission->path);
			PHYSFSX_contfile_close(hogpath);
		}

		if (Current_mission->path)
			d_free(Current_mission->path);
    }
}



//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode is set, then also add anarchy-only missions.

static mle *build_mission_list(int anarchy_mode)
{
	mle *mission_list;
	int top_place;
#if defined(DXX_BUILD_DESCENT_II)
    char	builtin_mission_filename[FILENAME_LEN];
#endif
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
	
#if defined(DXX_BUILD_DESCENT_II)
	add_builtin_mission_to_list(mission_list + num_missions, builtin_mission_filename);  //read built-in first
#endif
	add_d1_builtin_mission_to_list(mission_list + num_missions);
	add_missions_to_list(mission_list, search_str, search_str + strlen(search_str), anarchy_mode);
	
	// move original missions (in story-chronological order)
	// to top of mission list
	top_place = 0;
#if defined(DXX_BUILD_DESCENT_I)
	promote(mission_list, "", &top_place); // original descent 1 mission
#elif defined(DXX_BUILD_DESCENT_II)
	promote(mission_list, "descent", &top_place); // original descent 1 mission
	promote(mission_list, builtin_mission_filename, &top_place); // d2 or d2demo
	promote(mission_list, "d2x", &top_place); // vertigo
#endif

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

static void free_mission_list(mle *mission_list)
{
	int i;

	for (i = 0; i < num_missions; i++)
		d_free(mission_list[i].path);
	
	d_free(mission_list);
	num_missions = 0;
}

#if defined(DXX_BUILD_DESCENT_II)
//values for built-in mission

int load_mission_ham()
{
	read_hamfile();
	if (Current_mission->enhanced == 3 && Current_mission->alternate_ham_file) {
		/*
		 * If an alternate HAM is specified, map a HOG of the same name
		 * (if it exists) so that users can reference a HAM within a
		 * HOG.  This is required to let users reference the D2X.HAM
		 * file provided by Descent II: Vertigo.
		 *
		 * Try both plain NAME and missions/NAME, in that order.
		 */
		auto &altham = Current_mission->alternate_ham_file;
		unsigned l = strlen(*altham);
		char althog[PATH_MAX];
		snprintf(althog, sizeof(althog), MISSION_DIR "%.*s.hog", l - 4, static_cast<const char *>(*altham));
		char *p = althog + sizeof(MISSION_DIR) - 1;
		int exists = PHYSFSX_exists(p,1);
		if (!exists) {
			p = althog;
			exists = PHYSFSX_exists(p,1);
		}
		if (exists)
			PHYSFSX_contfile_init(p, 0);
		bm_read_extra_robots(*Current_mission->alternate_ham_file, 2);
		if (exists)
			PHYSFSX_contfile_close(p);
		return 1;
	} else if (Current_mission->enhanced) {
		char t[50];
		snprintf(t,sizeof(t),"%s.ham",Current_mission_filename);
		bm_read_extra_robots(t, Current_mission->enhanced);
		return 1;
	} else
		return 0;
}
#endif

static void set_briefing_filename(d_fname &f, const char *const v)
{
	using std::copy;
	using std::next;
	auto &tex = ".tex";
	auto o = copy(v, std::find(v, next(v, f.size() - sizeof(tex)), '.'), begin(f));
	copy(begin(tex), end(tex), o);
	auto &txb = "txb";
	if (!PHYSFSX_exists(&f[0],1) && !(copy(begin(txb), end(txb), next(o)), PHYSFSX_exists(&f[0],1))) // check if this file exists ...
		f = {};
}

static void record_briefing(d_fname &f, char (&buf)[PATH_MAX])
{
	char *const v = get_value(buf);
	if (v && (add_term(v), *v))
		set_briefing_filename(f, v);
	else
		f = {};
}

//loads the specfied mission from the mission list.
//build_mission_list() must have been called.
//Returns true if mission loaded ok, else false.
static int load_mission(mle *mission)
{
	PHYSFS_file *mfile;
	char buf[PATH_MAX], *v;

	Current_mission.reset(new Mission);
	Current_mission->builtin_hogsize = mission->builtin_hogsize;
	strcpy(Current_mission->mission_name, mission->mission_name);
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
#endif
	Current_mission->anarchy_only_flag = mission->anarchy_only_flag;
	Current_mission->path = d_strdup(mission->path);
	Current_mission->filename = Current_mission->path + (mission->filename - mission->path);
	Current_mission->n_secret_levels = 0;
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->enhanced = 0;
	Current_mission->alternate_ham_file = NULL;
#endif

	//init vars
	Last_level = 0;
	Last_secret_level = 0;
	Briefing_text_filename = {};
	Ending_text_filename = {};
	Secret_level_table.reset();
	Level_names.reset();
	Secret_level_names.reset();

	// for Descent 1 missions, load descent.hog
#if defined(DXX_BUILD_DESCENT_II)
	if (EMULATING_D1)
#endif
	{
		if (!PHYSFSX_contfile_init("descent.hog", 1))
#if defined(DXX_BUILD_DESCENT_I)
			Error("descent.hog not available!\n");
#elif defined(DXX_BUILD_DESCENT_II)
			Warning("descent.hog not available, this mission may be missing some files required for briefings and exit sequence\n");
#endif
		if (!d_stricmp(Current_mission_filename, D1_MISSION_FILENAME))
			return load_mission_d1();
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (PLAYING_BUILTIN_MISSION) {
		switch (Current_mission->builtin_hogsize) {
		case SHAREWARE_MISSION_HOGSIZE:
		case MAC_SHARE_MISSION_HOGSIZE:
			Briefing_text_filename = BIMD2_BRIEFING_FILE_SHARE;
			Ending_text_filename = BIMD2_ENDING_FILE_SHARE;
			return load_mission_shareware();
		case OEM_MISSION_HOGSIZE:
			Briefing_text_filename = BIMD2_BRIEFING_FILE_OEM;
			Ending_text_filename = BIMD2_ENDING_FILE_OEM;
			return load_mission_oem();
		default:
			Int3(); // fall through
		case FULL_MISSION_HOGSIZE:
		case FULL_10_MISSION_HOGSIZE:
		case MAC_FULL_MISSION_HOGSIZE:
			Briefing_text_filename = BIMD2_BRIEFING_FILE;
			// continue on... (use d2.mn2 from hogfile)
			break;
		}
	}
#endif

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
#if defined(DXX_BUILD_DESCENT_II)
	if (mission->descent_version == 2)
		strcat(buf,".mn2");
	else
#endif
		strcat(buf,".msn");

	PHYSFSEXT_locateCorrectCase(buf);

	mfile = PHYSFSX_openReadBuffered(buf);
	if (mfile == NULL) {
		Current_mission.reset();
		return 0;		//error!
	}

	//for non-builtin missions, load HOG
#if defined(DXX_BUILD_DESCENT_II)
	if (!PLAYING_BUILTIN_MISSION)
#endif
	{
		strcpy(buf+strlen(buf)-4,".hog");		//change extension
		PHYSFSEXT_locateCorrectCase(buf);
		if (PHYSFSX_exists(buf,1))
			PHYSFSX_contfile_init(buf, 0);
		set_briefing_filename(Briefing_text_filename, Current_mission_filename);
		Ending_text_filename = Briefing_text_filename;
	}

	while (PHYSFSX_fgets(buf,mfile)) {
#if defined(DXX_BUILD_DESCENT_II)
		if (istok(buf,"name") && !Current_mission->enhanced) {
			Current_mission->enhanced = 0;
			continue;						//already have name, go to next line
		}
		if (istok(buf,"xname") && !Current_mission->enhanced) {
			Current_mission->enhanced = 1;
			continue;						//already have name, go to next line
		}
		if (istok(buf,"zname") && !Current_mission->enhanced) {
			Current_mission->enhanced = 2;
			continue;						//already have name, go to next line
		}
		if (istok(buf,"!name") && !Current_mission->enhanced) {
			Current_mission->enhanced = 3;
			continue;						//already have name, go to next line
		}
#endif
		if (istok(buf,"type"))
			continue;						//already have name, go to next line
		else if (istok(buf,"briefing")) {
			record_briefing(Briefing_text_filename, buf);
		}
		else if (istok(buf,"ending")) {
			record_briefing(Ending_text_filename, buf);
		}
		else if (istok(buf,"num_levels")) {

			if ((v=get_value(buf))!=NULL) {
				int n_levels,i;

				n_levels = atoi(v);
				
				Assert(n_levels <= MAX_LEVELS_PER_MISSION);
				n_levels = min(n_levels, MAX_LEVELS_PER_MISSION);
				
				Level_names.reset(new d_fname[n_levels]);
				if (!Level_names)
				{
					Current_mission.reset();
					return 0;
				}

				for (i=0;i<n_levels;i++) {
					PHYSFSX_fgets(buf,mfile);
					add_term(buf);
					if (Level_names[i].copy_if(buf)) {
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
				N_secret_levels = min(N_secret_levels, MAX_SECRET_LEVELS_PER_MISSION);

				Secret_level_names.reset(new d_fname[N_secret_levels]);
				if (!Secret_level_names)
				{
					Current_mission.reset();
					return 0;
				}
				
				Secret_level_table.reset(new ubyte[N_secret_levels]);
				if (!Secret_level_table)
				{
					Current_mission.reset();
					return 0;
				}
				
				for (i=0;i<N_secret_levels;i++) {
					char *t;

					PHYSFSX_fgets(buf,mfile);
					if ((t=strchr(buf,','))!=NULL) *t++=0;
					else
						break;

					add_term(buf);
					if (Secret_level_names[i].copy_if(buf)) {
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
#if defined(DXX_BUILD_DESCENT_II)
		else if (Current_mission->enhanced == 3 && buf[0] == '!') {
			if (istok(buf+1,"ham")) {
				Current_mission->alternate_ham_file.reset(new d_fname);
				if ((v=get_value(buf))!=NULL) {
					unsigned l = strlen(v);
					if (l <= 4)
						con_printf(CON_URGENT, "Mission %s has short HAM \"%s\".", Current_mission->path, v);
					else if (l >= sizeof(*Current_mission->alternate_ham_file))
						con_printf(CON_URGENT, "Mission %s has excessive HAM \"%s\".", Current_mission->path, v);
					else {
						Current_mission->alternate_ham_file->copy_if(v, l + 1);
						con_printf(CON_VERBOSE, "Mission %s will use HAM %s.", Current_mission->path, static_cast<const char *>(*Current_mission->alternate_ham_file));
					}
				}
				else
					con_printf(CON_URGENT, "Mission %s has no HAM.", Current_mission->path);
			}
			else {
				con_printf(CON_URGENT, "Mission %s uses unsupported critical directive \"%s\".", Current_mission->path, buf);
				Last_level = 0;
				break;
			}
		}
#endif

	}

	PHYSFS_close(mfile);

	if (Last_level <= 0) {
		Current_mission.reset();		//no valid mission loaded
		return 0;
	}

#if defined(DXX_BUILD_DESCENT_II)
	// re-read default HAM file, in case this mission brings it's own version of it
	free_polygon_models();

	if (load_mission_ham())
		init_extra_robot_movie(Current_mission_filename);
#endif

	return 1;
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(const char *mission_name)
{
	int i;
	mle *mission_list = build_mission_list(1);
	bool found = 0;

	for (i = 0; i < num_missions; i++)
		if (!d_stricmp(mission_name, mission_list[i].filename))
			found = load_mission(mission_list + i);

	free_mission_list(mission_list);
	return found;
}

struct mission_menu
{
	mle *mission_list;
	int (*when_selected)(void);
};

static int mission_menu_handler(listbox *lb, d_event *event, mission_menu *mm)
{
	const char **list = listbox_get_items(lb);
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

int select_mission(int anarchy_mode, const char *message, int (*when_selected)(void))
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
        const char **m;
		
		MALLOC(m, const char *, num_missions);
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
            if ( !d_stricmp( m[i], GameCfg.LastMission ) )
                default_mission = i;
        }

        newmenu_listbox1( message, num_missions, m, 1, default_mission, mission_menu_handler, mm );
    }

    return 1;	// presume success
}

#ifdef EDITOR
void create_new_mission(void)
{
	Current_mission.reset(new Mission{});
	Current_mission->path = d_strdup("new_mission");
	if (!Current_mission->path)
	{
		Current_mission.reset();
		return;
	}

	Current_mission->filename = Current_mission->path;
	
	Level_names.reset(new d_fname[1]);
	if (!Level_names)
	{
		Current_mission.reset();
		return;
	}

	Level_names[0] = "GAMESAVE.LVL";
}
#endif
