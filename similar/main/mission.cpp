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
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "window.h"
#include "mission.h"
#include "gameseq.h"
#include "gamesave.h"
#include "titles.h"
#include "piggy.h"
#include "console.h"
#include "songs.h"
#include "polyobj.h"
#include "dxxerror.h"
#include "config.h"
#include "newmenu.h"
#include "text.h"
#include "u_mem.h"
#include "ignorecase.h"
#include "physfsx.h"
#include "physfs_list.h"
#include "bm.h"
#include "event.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif
#include "null_sentinel_iterator.h"

#include "compiler-make_unique.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"

#define BIMD1_BRIEFING_FILE		"briefing.txb"

using std::min;

#define MISSION_EXTENSION_DESCENT_I	".msn"
#if defined(DXX_BUILD_DESCENT_II)
#define MISSION_EXTENSION_DESCENT_II	".mn2"
#endif

namespace {

using mission_candidate_search_path = array<char, PATH_MAX>;

//mission list entry
struct mle : Mission_path
{
	int     builtin_hogsize;    // if it's the built-in mission, used for determining the version
	ntstring<MISSION_NAME_LEN> mission_name;
#if defined(DXX_BUILD_DESCENT_II)
	descent_version_type descent_version;    // descent 1 or descent 2?
#endif
	ubyte   anarchy_only_flag;  // if true, mission is anarchy only
};

using mission_list_type = std::vector<mle>;

}

Mission_ptr Current_mission; // currently loaded mission

static bool null_or_space(char c)
{
	return !c || isspace(static_cast<unsigned>(c));
}

// Allocate the Level_names, Secret_level_names and Secret_level_table arrays
static int allocate_levels(void)
{
	Level_names = make_unique<d_fname[]>(Last_level);
	if (Last_secret_level)
	{
		N_secret_levels = -Last_secret_level;
		Secret_level_names = make_unique<d_fname[]>(N_secret_levels);
		Secret_level_table = make_unique<ubyte[]>(N_secret_levels);
	}
	
	return 1;
}

//
//  Special versions of mission routines for d1 builtins
//

static int load_mission_d1(void)
{
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
			for (int i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0u], Level_names[i].size(), "level%02d.sdl", i+1);
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
			for (int i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0u], Level_names[i].size(), "level%02d.sdl", i+1);
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
			for (int i=0; i < Last_level - 1; i++)
				snprintf(&Level_names[i][0u], Level_names[i].size(), "level%02d.rdl", i+1);
			snprintf(&Level_names[Last_level - 1][0u], Level_names[Last_level - 1].size(), "saturn%02d.rdl", Last_level);
			for (int i=0; i < -Last_secret_level; i++)
				snprintf(&Secret_level_names[i][0u], Secret_level_names[i].size(), "levels%1d.rdl", i+1);
			Secret_level_table[0] = 10;
			Briefing_text_filename = "briefsat.txb";
			Ending_text_filename = BIMD1_ENDING_FILE_OEM;
			break;
		default:
			Int3(); // fall through
		case D1_MISSION_HOGSIZE:
		case D1_MISSION_HOGSIZE2:
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
			for (int i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0u], Level_names[i].size(), "level%02d.rdl", i+1);
			for (int i=0;i<-Last_secret_level;i++)
				snprintf(&Secret_level_names[i][0u], Secret_level_names[i].size(), "levels%1d.rdl", i+1);
			Secret_level_table[0] = 10;
			Secret_level_table[1] = 21;
			Secret_level_table[2] = 24;
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = "endreg.txb";
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
    Current_mission->mission_name.copy_if(SHAREWARE_MISSION_NAME);
    Current_mission->descent_version = Mission::descent_version_type::descent2;
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
    Current_mission->mission_name.copy_if(OEM_MISSION_NAME);
    Current_mission->descent_version = Mission::descent_version_type::descent2;
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
static char *get_parm_value(PHYSFSX_gets_line_t<80> &buf, const char *parm,PHYSFS_File *f)
{
	if (!PHYSFSX_fgets(buf,f))
		return NULL;

	if (istok(buf,parm))
		return get_value(buf);
	else
		return NULL;
}

static bool ml_sort_func(const mle &e0,const mle &e1)
{
	return d_stricmp(e0.mission_name,e1.mission_name) < 0;
}

//returns 1 if file read ok, else 0
namespace dsx {
static int read_mission_file(mission_list_type &mission_list, mission_candidate_search_path &pathname)
{
	if (auto mfile = PHYSFSX_openReadBuffered(pathname.data()))
	{
		char *p;
		char *ext;
		p = strrchr(pathname.data(), '/');
		if (!p)
			p = pathname.data();
		if ((ext = strchr(p, '.')) == NULL)
			return 0;	//missing extension
		mission_list.emplace_back();
		mle *mission = &mission_list.back();
		mission->path.assign(pathname.data(), ext);
#if defined(DXX_BUILD_DESCENT_II)
		// look if it's .mn2 or .msn
		mission->descent_version = (ext[3] == MISSION_EXTENSION_DESCENT_II[3])
			? Mission::descent_version_type::descent2
			: Mission::descent_version_type::descent1;
#endif
		mission->anarchy_only_flag = 0;
		mission->filename = next(begin(mission->path), mission->path.find_last_of('/') + 1);

		PHYSFSX_gets_line_t<80> buf;
		p = get_parm_value(buf, "name",mfile);

#if defined(DXX_BUILD_DESCENT_II)
		if (!p) {		//try enhanced mission
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value(buf, "xname",mfile);
		}

		if (!p) {       //try super-enhanced mission!
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value(buf, "zname",mfile);
		}

		if (!p) {       //try extensible-enhanced mission!
			PHYSFSX_fseek(mfile,0,SEEK_SET);
			p = get_parm_value(buf, "!name",mfile);
		}
#endif

		if (p) {
			char *t;
			if ((t=strchr(p,';'))!=NULL)
				*t=0;
			t = p + strlen(p)-1;
			while (isspace(*t))
				*t-- = 0; // remove trailing whitespace
			mission->mission_name.copy_if(p, mission->mission_name.size() - 1);
		}
		else {
			mission_list.pop_back();
			return 0;
		}

		{
			PHYSFSX_gets_line_t<4096> temp;
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
		}
		return 1;
	}

	return 0;
}
}

namespace dsx {
static void add_d1_builtin_mission_to_list(mission_list_type &mission_list)
{
    int size;
    
	size = PHYSFSX_fsize("descent.hog");
	if (size == -1)
		return;

	mission_list.emplace_back();
	mle *mission = &mission_list.back();
	switch (size) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		mission->path = D1_MISSION_FILENAME;
		mission->mission_name.copy_if(D1_SHAREWARE_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		mission->path = D1_MISSION_FILENAME;
		mission->mission_name.copy_if(D1_OEM_MISSION_NAME);
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
		mission->path = D1_MISSION_FILENAME;
		mission->mission_name.copy_if(D1_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	}

	mission->anarchy_only_flag = 0;
#if defined(DXX_BUILD_DESCENT_I)
	mission->builtin_hogsize = size;
#elif defined(DXX_BUILD_DESCENT_II)
	mission->descent_version = Mission::descent_version_type::descent1;
	mission->builtin_hogsize = 0;
#endif
	mission->filename = begin(mission->path);
}
}

#if defined(DXX_BUILD_DESCENT_II)
template <std::size_t N1, std::size_t N2>
static void set_hardcoded_mission(mission_list_type &mission_list, const char (&path)[N1], const char (&mission_name)[N2])
{
	mission_list.emplace_back();
	mle *mission = &mission_list.back();
	mission->path = path;
	mission->filename = begin(mission->path);
	mission->mission_name.copy_if(mission_name);
	mission->anarchy_only_flag = 0;
}

static void add_builtin_mission_to_list(mission_list_type &mission_list, d_fname &name)
{
    int size = PHYSFSX_fsize("descent2.hog");
    
	if (size == -1)
		size = PHYSFSX_fsize("d2demo.hog");

	switch (size) {
	case SHAREWARE_MISSION_HOGSIZE:
	case MAC_SHARE_MISSION_HOGSIZE:
		set_hardcoded_mission(mission_list, SHAREWARE_MISSION_FILENAME, SHAREWARE_MISSION_NAME);
		break;
	case OEM_MISSION_HOGSIZE:
		set_hardcoded_mission(mission_list, OEM_MISSION_FILENAME, OEM_MISSION_NAME);
		break;
	default:
		Warning("Unknown hogsize %d, trying %s\n", size, FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II);
		Int3(); //fall through
	case FULL_MISSION_HOGSIZE:
	case FULL_10_MISSION_HOGSIZE:
	case MAC_FULL_MISSION_HOGSIZE:
		{
			mission_candidate_search_path full_mission_filename = {{FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II}};
			if (!read_mission_file(mission_list, full_mission_filename))
				Error("Could not find required mission file <%s>", FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II);
		}
	}

	mle *mission = &mission_list.back();
	name.copy_if(mission->path.c_str(), FILENAME_LEN);
    mission->builtin_hogsize = size;
	mission->descent_version = Mission::descent_version_type::descent2;
	mission->anarchy_only_flag = 0;
}
#endif

namespace dsx {
static void add_missions_to_list(mission_list_type &mission_list, mission_candidate_search_path &path, const mission_candidate_search_path::iterator rel_path, int anarchy_mode)
{
	/* rel_path must point within the array `path`.
	 * rel_path must point to the null that follows a possibly empty
	 * directory prefix.
	 * If the directory prefix is not empty, it must end with a PHYSFS
	 * path separator, which is always slash, even on Windows.
	 *
	 * If any of these assertions fail, then the path transforms used to
	 * recurse into subdirectories and to open individual missions will
	 * not work correctly.
	 */
	assert(std::distance(path.begin(), rel_path) < path.size());
	assert(!*rel_path);
	assert(path.begin() == rel_path || *std::prev(rel_path) == '/');
	const std::size_t space_remaining = std::distance(rel_path, path.end());
	range_for (const auto i, PHYSFSX_uncounted_list{PHYSFS_enumerateFiles(path.data())})
	{
		/* Add 1 to include the terminating null. */
		const std::size_t il = strlen(i) + 1;
		/* Add 1 for the slash in case it is a directory. */
		if (il + 1 >= space_remaining)
			continue;	// path is too long

		auto j = std::copy_n(i, il, rel_path);
		const char *ext;
		if (PHYSFS_isDirectory(path.data()))
		{
			auto null = std::prev(j);
			*j = 0;
			*null = '/';
			add_missions_to_list(mission_list, path, j, anarchy_mode);
			*null = 0;
		}
		else if (il > 5 &&
			((ext = &i[il - 5], !d_strnicmp(ext, MISSION_EXTENSION_DESCENT_I))
#if defined(DXX_BUILD_DESCENT_II)
				|| !d_strnicmp(ext, MISSION_EXTENSION_DESCENT_II)
#endif
			))
			if (read_mission_file(mission_list, path))
			{
				if (anarchy_mode || !mission_list.back().anarchy_only_flag)
				{
					mission_list.back().builtin_hogsize = 0;
				}
				else
					mission_list.pop_back();
			}
		
		if (mission_list.size() >= MAX_MISSIONS)
		{
			break;
		}
		*rel_path = 0;	// chop off the entry
	}
}
}

/* move <mission_name> to <place> on mission list, increment <place> */
static void promote (mission_list_type &mission_list, const char *const name, std::size_t &top_place)
{
	range_for (auto &i, partial_range(mission_list, top_place, mission_list.size()))
		if (!d_stricmp(&*i.filename, name)) {
			//swap mission positions
			std::swap(mission_list[top_place++], i);
			break;
		}
}

Mission::~Mission()
{
    // May become more complex with the editor
	if (!path.empty() && builtin_hogsize == 0)
		{
			char hogpath[PATH_MAX];
			snprintf(hogpath, sizeof(hogpath), "%s.hog", path.c_str());
			PHYSFSX_removeRelFromSearchPath(hogpath);
		}
}



//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode is set, then also add anarchy-only missions.

namespace dsx {
static mission_list_type build_mission_list(int anarchy_mode)
{

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

	mission_list_type mission_list;
	
#if defined(DXX_BUILD_DESCENT_II)
	d_fname builtin_mission_filename;
	add_builtin_mission_to_list(mission_list, builtin_mission_filename);  //read built-in first
#endif
	add_d1_builtin_mission_to_list(mission_list);
	mission_candidate_search_path search_str = {{MISSION_DIR}};
	add_missions_to_list(mission_list, search_str, search_str.begin() + sizeof(MISSION_DIR) - 1, anarchy_mode);
	
	// move original missions (in story-chronological order)
	// to top of mission list
	std::size_t top_place = 0;
	promote(mission_list, D1_MISSION_FILENAME, top_place); // original descent 1 mission
#if defined(DXX_BUILD_DESCENT_II)
	promote(mission_list, builtin_mission_filename, top_place); // d2 or d2demo
	promote(mission_list, "d2x", top_place); // vertigo
#endif

	if (mission_list.size() > top_place)
		std::sort(next(begin(mission_list), top_place), end(mission_list), ml_sort_func);
	return mission_list;
}
}

#if defined(DXX_BUILD_DESCENT_II)
//values for built-in mission

int load_mission_ham()
{
	read_hamfile();
	if (Current_mission->descent_version == Mission::descent_version_type::descent2a &&
		Current_mission->alternate_ham_file)
	{
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
		int exists = PHYSFSX_contfile_init(p, 0);
		if (!exists) {
			exists = PHYSFSX_contfile_init(p = althog, 0);
		}
		bm_read_extra_robots(*altham, Mission::descent_version_type::descent2z);
		if (exists)
			PHYSFSX_contfile_close(p);
		return 1;
	}
	else if (Current_mission->descent_version == Mission::descent_version_type::descent2a ||
				Current_mission->descent_version == Mission::descent_version_type::descent2z ||
				Current_mission->descent_version == Mission::descent_version_type::descent2x)
	{
		char t[50];
		snprintf(t,sizeof(t), "%s.ham", Current_mission_filename);
		bm_read_extra_robots(t, Current_mission->descent_version);
		return 1;
	} else
		return 0;
}
#endif

#define tex ".tex"
static void set_briefing_filename(d_fname &f, const char *const v, std::size_t d)
{
	f.copy_if(v, d);
	f.copy_if(d, tex);
	if (!PHYSFSX_exists(static_cast<const char *>(f), 1) && !(f.copy_if(++d, "txb"), PHYSFSX_exists(static_cast<const char *>(f), 1))) // check if this file exists ...
		f = {};
}

static void set_briefing_filename(d_fname &f, const char *const v)
{
	using std::next;
	auto a = [](char c) {
		return !c || c == '.';
	};
	auto i = std::find_if(v, next(v, f.size() - sizeof(tex)), a);
	std::size_t d = std::distance(v, i);
	set_briefing_filename(f, v, d);
}

static void record_briefing(d_fname &f, array<char, PATH_MAX> &buf)
{
	const auto v = get_value(buf.data());
	if (!v)
		return;
	const std::size_t d = std::distance(v, std::find_if(v, buf.end(), null_or_space));
	if (d >= FILENAME_LEN)
		return;
	{
		set_briefing_filename(f, v, std::min(d, f.size() - sizeof(tex)));
	}
}
#undef tex

//loads the specfied mission from the mission list.
//build_mission_list() must have been called.
//Returns true if mission loaded ok, else false.
namespace dsx {
static int load_mission(const mle *mission)
{
	char *v;

#if defined(DXX_BUILD_DESCENT_II)
	close_extra_robot_movie();
#endif
	Current_mission = make_unique<Mission>();
	Current_mission->builtin_hogsize = mission->builtin_hogsize;
	Current_mission->mission_name = mission->mission_name;
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
#endif
	Current_mission->anarchy_only_flag = mission->anarchy_only_flag;
	*static_cast<Mission_path *>(Current_mission.get()) = *mission;
	Current_mission->n_secret_levels = 0;
#if defined(DXX_BUILD_DESCENT_II)
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
		if (!PHYSFSX_contfile_init("descent.hog", 0))
#if defined(DXX_BUILD_DESCENT_I)
			Error("descent.hog not available!\n");
#elif defined(DXX_BUILD_DESCENT_II)
			Warning("descent.hog not available, this mission may be missing some files required for briefings and exit sequence\n");
#endif
		if (!d_stricmp(Current_mission_filename, D1_MISSION_FILENAME))
			return load_mission_d1();
	}
#if defined(DXX_BUILD_DESCENT_II)
	else
		PHYSFSX_contfile_close("descent.hog");
#endif

#if defined(DXX_BUILD_DESCENT_II)
	if (PLAYING_BUILTIN_MISSION) {
		switch (Current_mission->builtin_hogsize) {
		case SHAREWARE_MISSION_HOGSIZE:
		case MAC_SHARE_MISSION_HOGSIZE:
			Briefing_text_filename = "brief2.txb";
			Ending_text_filename = BIMD2_ENDING_FILE_SHARE;
			return load_mission_shareware();
		case OEM_MISSION_HOGSIZE:
			Briefing_text_filename = "brief2o.txb";
			Ending_text_filename = BIMD2_ENDING_FILE_OEM;
			return load_mission_oem();
		default:
			Int3(); // fall through
		case FULL_MISSION_HOGSIZE:
		case FULL_10_MISSION_HOGSIZE:
		case MAC_FULL_MISSION_HOGSIZE:
			Briefing_text_filename = "robot.txb";
			// continue on... (use d2.mn2 from hogfile)
			break;
		}
	}
#endif

	//read mission from file

	auto &msn_extension =
#if defined(DXX_BUILD_DESCENT_II)
	(mission->descent_version == Mission::descent_version_type::descent2) ? MISSION_EXTENSION_DESCENT_II :
#endif
		MISSION_EXTENSION_DESCENT_I;
	array<char, PATH_MAX> mission_filename;
	snprintf(mission_filename.data(), mission_filename.size(), "%s%s", mission->path.c_str(), msn_extension);

	PHYSFSEXT_locateCorrectCase(mission_filename.data());

	auto &&mfile = PHYSFSX_openReadBuffered(mission_filename.data());
	if (!mfile) {
		Current_mission.reset();
		return 0;		//error!
	}

	//for non-builtin missions, load HOG
#if defined(DXX_BUILD_DESCENT_II)
	bool have_mn2_version = false;
	if (!PLAYING_BUILTIN_MISSION)
#endif
	{
		strcpy(mission_filename.data() + strlen(mission_filename.data()) - 3, "hog");		//change extension
			PHYSFSX_contfile_init(mission_filename.data(), 0);
		set_briefing_filename(Briefing_text_filename, Current_mission_filename);
		Ending_text_filename = Briefing_text_filename;
	}

	for (PHYSFSX_gets_line_t<PATH_MAX> buf; PHYSFSX_fgets(buf,mfile);)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (!have_mn2_version)
		{
			if (istok(buf,"name"))
			{
				Current_mission->descent_version = Mission::descent_version_type::descent2;
				have_mn2_version = true;
			continue;						//already have name, go to next line
			}
			if (istok(buf,"xname"))
			{
				Current_mission->descent_version = Mission::descent_version_type::descent2x;
				have_mn2_version = true;
			continue;						//already have name, go to next line
			}
			if (istok(buf,"zname"))
			{
				Current_mission->descent_version = Mission::descent_version_type::descent2z;
				have_mn2_version = true;
			continue;						//already have name, go to next line
			}
			if (istok(buf,"!name"))
			{
				Current_mission->descent_version = Mission::descent_version_type::descent2a;
				have_mn2_version = true;
			continue;						//already have name, go to next line
			}
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
				char *ip;
				const auto n_levels = strtoul(v, &ip, 10);
				Assert(n_levels <= MAX_LEVELS_PER_MISSION);
				if (n_levels > MAX_LEVELS_PER_MISSION)
					continue;
				if (*ip)
				{
					while (isspace(static_cast<unsigned>(*ip)))
						++ip;
					if (*ip && *ip != ';')
						continue;
				}
				Level_names = make_unique<d_fname[]>(n_levels);
				range_for (auto &i, unchecked_partial_range(Level_names.get(), n_levels))
				{
					if (!PHYSFSX_fgets(buf, mfile))
						break;
					auto &line = buf.line();
					auto s = std::find_if(line.begin(), line.end(), null_or_space);
					if (i.copy_if(buf.line(), std::distance(line.begin(), s)))
					{
						Last_level++;
					}
					else
						break;
				}

			}
		}
		else if (istok(buf,"num_secrets")) {
			if ((v=get_value(buf))!=NULL) {
				char *ip;
				const auto n_levels = strtoul(v, &ip, 10);
				Assert(n_levels <= MAX_SECRET_LEVELS_PER_MISSION);
				if (n_levels > MAX_SECRET_LEVELS_PER_MISSION)
					continue;
				if (*ip)
				{
					while (isspace(static_cast<unsigned>(*ip)))
						++ip;
					if (*ip && *ip != ';')
						continue;
				}
				N_secret_levels = n_levels;
				Secret_level_names = make_unique<d_fname[]>(n_levels);
				Secret_level_table = make_unique<uint8_t[]>(n_levels);
				for (int i=0;i<N_secret_levels;i++) {
					if (!PHYSFSX_fgets(buf, mfile))
						break;
					const auto &line = buf.line();
					const auto lb = line.begin();
					/* No auto: returned value must be type const char*
					 * Modern glibc maintains const-ness of the input.
					 * Apple libc++ and mingw32 do not.
					 */
					const char *const t = strchr(lb, ',');
					if (!t)
						break;
					auto a = [](char c) {
						return isspace(static_cast<unsigned>(c));
					};
					auto s = std::find_if(lb, t, a);
					if (Secret_level_names[i].copy_if(line, std::distance(lb, s)))
					{
						unsigned long ls = strtoul(t + 1, &ip, 10);
						if (ls < 1 || ls > Last_level)
							break;
						Secret_level_table[i] = ls;
						Last_secret_level--;
					}
					else
						break;
				}

			}
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (Current_mission->descent_version == Mission::descent_version_type::descent2a && buf[0] == '!') {
			if (istok(buf+1,"ham")) {
				Current_mission->alternate_ham_file = make_unique<d_fname>();
				if ((v=get_value(buf))!=NULL) {
					unsigned l = strlen(v);
					if (l <= 4)
						con_printf(CON_URGENT, "Mission %s has short HAM \"%s\".", Current_mission->path.c_str(), v);
					else if (l >= sizeof(*Current_mission->alternate_ham_file))
						con_printf(CON_URGENT, "Mission %s has excessive HAM \"%s\".", Current_mission->path.c_str(), v);
					else {
						Current_mission->alternate_ham_file->copy_if(v, l + 1);
						con_printf(CON_VERBOSE, "Mission %s will use HAM %s.", Current_mission->path.c_str(), static_cast<const char *>(*Current_mission->alternate_ham_file));
					}
				}
				else
					con_printf(CON_URGENT, "Mission %s has no HAM.", Current_mission->path.c_str());
			}
			else {
				con_printf(CON_URGENT, "Mission %s uses unsupported critical directive \"%s\".", Current_mission->path.c_str(), static_cast<const char *>(buf));
				Last_level = 0;
				break;
			}
		}
#endif

	}
	mfile.reset();
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
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(const char *mission_name)
{
	auto mission_list = build_mission_list(1);
	bool found = 0;

	range_for (auto &i, mission_list)
		if (!d_stricmp(mission_name, &*i.filename))
		{
			found = load_mission(&i);
			break;
		}
	return found;
}

struct mission_menu
{
	using callback_type = window_event_result (*)(void);
	mission_list_type ml;
	std::unique_ptr<const char *[]> mission_names;
	callback_type when_selected;
	mission_menu(mission_list_type &&rml, std::unique_ptr<const char *[]> &&mn, const callback_type ws) :
		ml(std::move(rml)), mission_names(std::move(mn)), when_selected(ws)
	{
	}
};

static window_event_result mission_menu_handler(listbox *, const d_event &event, mission_menu *const mm)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (citem >= 0)
			{
				// Chose a mission
				if (!load_mission(&mm->ml[citem]))
				{
					nm_messagebox( NULL, 1, TXT_OK, TXT_MISSION_ERROR);
					return window_event_result::handled;	// stay in listbox so user can select another one
				}
				CGameCfg.LastMission.copy_if(mm->mission_names[citem]);
			}
			return (*mm->when_selected)();
		}
		case EVENT_WINDOW_CLOSE:
			std::default_delete<mission_menu>()(mm);
			break;
		default:
			break;
	}
	
	return window_event_result::ignored;
}

int select_mission(int anarchy_mode, const char *message, window_event_result (*when_selected)(void))
{
	auto mission_list = build_mission_list(anarchy_mode);
	int new_mission_num;

    if (mission_list.size() <= 1)
	{
        new_mission_num = !mission_list.empty() && load_mission(&mission_list.front()) ? 0 : -1;
		(*when_selected)();
		
		return (new_mission_num >= 0);
    }
	else
	{
		auto m = make_unique<const char *[]>(mission_list.size());
		unsigned default_mission = 0;
		const char *LastMission = CGameCfg.LastMission;
		range_for (auto &&e, enumerate(mission_list))
		{
			const uint_fast32_t i = e.idx;
			auto &mli = e.value;
			const char *const mission_name = mli.mission_name;
			m[i] = mission_name;
			if (LastMission && !strcmp(mission_name, LastMission))
			{
				LastMission = nullptr;
                default_mission = i;
			}
        }

		const auto pm = m.get();
		auto mm = make_unique<mission_menu>(std::move(mission_list), std::move(m), when_selected);
		auto pmm = mm.get();
		newmenu_listbox1( message, pmm->ml.size(), pm, 1, default_mission, mission_menu_handler, std::move(mm));
    }

    return 1;	// presume success
}

#if DXX_USE_EDITOR
static int write_mission(void)
{
	auto &msn_extension =
#if defined(DXX_BUILD_DESCENT_II)
	(Current_mission->descent_version != Mission::descent_version_type::descent1) ? MISSION_EXTENSION_DESCENT_II :
#endif
	MISSION_EXTENSION_DESCENT_I;
	array<char, PATH_MAX> mission_filename;
	snprintf(mission_filename.data(), mission_filename.size(), "%s%s", Current_mission->path.c_str(), msn_extension);
	
	auto &&mfile = PHYSFSX_openWriteBuffered(mission_filename.data());
	if (!mfile)
	{
		PHYSFS_mkdir(MISSION_DIR);	//try making directory - in *write* path
		mfile = PHYSFSX_openWriteBuffered(mission_filename.data());
		if (!mfile)
			return 0;
	}

	const char *prefix = "";
#if defined(DXX_BUILD_DESCENT_II)
	switch (Current_mission->descent_version)
	{
		case Mission::descent_version_type::descent2x:
			prefix = "x";
			break;

		case Mission::descent_version_type::descent2z:
			prefix = "z";
			break;
			
		case Mission::descent_version_type::descent2a:
			prefix = "!";
			break;

		default:
			break;
	}
#endif

	PHYSFSX_printf(mfile, "%sname = %s\n", prefix, static_cast<const char *>(Current_mission->mission_name));

	PHYSFSX_printf(mfile, "type = %s\n", Current_mission->anarchy_only_flag ? "anarchy" : "normal");

	if (Briefing_text_filename[0])
		PHYSFSX_printf(mfile, "briefing = %s\n", static_cast<const char *>(Briefing_text_filename));

	if (Ending_text_filename[0])
		PHYSFSX_printf(mfile, "ending = %s\n", static_cast<const char *>(Ending_text_filename));

	PHYSFSX_printf(mfile, "num_levels = %i\n", Last_level);

	range_for (auto &i, unchecked_partial_range(Level_names.get(), Last_level))
		PHYSFSX_printf(mfile, "%s\n", static_cast<const char *>(i));

	if (N_secret_levels)
	{
		PHYSFSX_printf(mfile, "num_secrets = %i\n", N_secret_levels);

		for (int i = 0; i < N_secret_levels; i++)
			PHYSFSX_printf(mfile, "%s,%i\n", static_cast<const char *>(Secret_level_names[i]), Secret_level_table[i]);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (Current_mission->alternate_ham_file)
		PHYSFSX_printf(mfile, "ham = %s\n", static_cast<const char *>(*Current_mission->alternate_ham_file.get()));
#endif

	return 1;
}

void create_new_mission(void)
{
	Current_mission = make_unique<Mission>();
	*Current_mission = {};
	Current_mission->mission_name.copy_if("Untitled");
	Current_mission->path = MISSION_DIR "new_miss";		// limited to eight characters because of savegame format
	Current_mission->filename = next(begin(Current_mission->path), sizeof(MISSION_DIR) - 1);
	Current_mission->builtin_hogsize = 0;
	Current_mission->anarchy_only_flag = 0;
	
	Level_names = make_unique<d_fname[]>(1);
	if (!Level_names)
	{
		Current_mission.reset();
		return;
	}

	Level_names[0] = "GAMESAVE.LVL";
	Last_level = 1;
	N_secret_levels = 0;
	Last_secret_level = 0;
	Briefing_text_filename = {};
	Ending_text_filename = {};
	Secret_level_table.reset();
	Secret_level_names.reset();

#if defined(DXX_BUILD_DESCENT_II)
	if (Gamesave_current_version > 3)
		Current_mission->descent_version = Mission::descent_version_type::descent2;	// custom ham not supported in editor (yet)
	else
		Current_mission->descent_version = Mission::descent_version_type::descent1;

	Current_mission->alternate_ham_file = nullptr;
#endif

	write_mission();
}
#endif
