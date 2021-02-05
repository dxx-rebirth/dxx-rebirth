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
#include "gamesave.h"
#include "piggy.h"
#include "console.h"
#include "polyobj.h"
#include "dxxerror.h"
#include "config.h"
#include "newmenu.h"
#include "text.h"
#include "u_mem.h"
#include "ignorecase.h"
#include "physfsx.h"
#include "physfs_list.h"
#include "event.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif
#include "null_sentinel_iterator.h"

#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include <memory>

#define BIMD1_BRIEFING_FILE		"briefing.txb"

using std::min;

#define MISSION_EXTENSION_DESCENT_I	".msn"
#if defined(DXX_BUILD_DESCENT_II)
#define MISSION_EXTENSION_DESCENT_II	".mn2"
#endif

#define CON_PRIORITY_DEBUG_MISSION_LOAD	CON_DEBUG

namespace {

using mission_candidate_search_path = std::array<char, PATH_MAX>;

}

namespace dsx {

namespace {

struct mle;
using mission_list_type = std::vector<mle>;

//mission list entry
struct mle : Mission_path
{
	int     builtin_hogsize;    // if it's the built-in mission, used for determining the version
	ntstring<75> mission_name;
#if defined(DXX_BUILD_DESCENT_II)
	descent_version_type descent_version;    // descent 1 or descent 2?
#endif
	ubyte   anarchy_only_flag;  // if true, mission is anarchy only
	mission_list_type directory;
	mle(Mission_path &&m) :
		Mission_path(std::move(m))
	{
	}
	mle(const char *const name, std::vector<mle> &&d);
};

struct mission_subdir_stats
{
	std::size_t immediate_directories = 0, immediate_missions = 0, total_missions = 0;
	static std::size_t count_missions(const mission_list_type &directory)
	{
		std::size_t total_missions = 0;
		range_for (auto &&i, directory)
		{
			if (i.directory.empty())
				++ total_missions;
			else
				total_missions += count_missions(i.directory);
		}
		return total_missions;
	}
	void count(const mission_list_type &directory)
	{
		range_for (auto &&i, directory)
		{
			if (i.directory.empty())
			{
				++ total_missions;
				++ immediate_missions;
			}
			else
			{
				++ immediate_directories;
				total_missions += count_missions(i.directory);
			}
		}
	}
};

struct mission_name_and_version
{
#if defined(DXX_BUILD_DESCENT_II)
	const Mission::descent_version_type descent_version = {};
#endif
	char *const name = nullptr;
	mission_name_and_version() = default;
	mission_name_and_version(Mission::descent_version_type, char *);
};

mission_name_and_version::mission_name_and_version(Mission::descent_version_type const v, char *const n) :
#if defined(DXX_BUILD_DESCENT_II)
	descent_version(v),
#endif
	name(n)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)v;
#endif
}

const char *prepare_mission_list_count_dirbuf(std::array<char, 12> &dirbuf, const std::size_t immediate_directories)
{
	/* Limit the count of directories to what can be formatted
	 * successfully without truncation.  If a user has more than this
	 * many directories, an empty string will be used instead of showing
	 * the actual count.
	 */
	if (immediate_directories && immediate_directories <= 99999)
	{
		snprintf(dirbuf.data(), dirbuf.size(), "DIR:%zu; ", immediate_directories);
		return dirbuf.data();
	}
	return "";
}

mle::mle(const char *const name, std::vector<mle> &&d) :
	Mission_path(name, 0), directory(std::move(d))
{
	mission_subdir_stats ss;
	ss.count(directory);
	std::array<char, 12> dirbuf;
	snprintf(mission_name.data(), mission_name.size(), "%s/ [%sMSN:L%zu;T%zu]", name, prepare_mission_list_count_dirbuf(dirbuf, ss.immediate_directories), ss.immediate_missions, ss.total_missions);
}

static const mle *compare_mission_predicate_to_leaf(const mission_entry_predicate mission_predicate, const mle &candidate, const char *candidate_filesystem_name)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (mission_predicate.check_version && mission_predicate.descent_version != candidate.descent_version)
	{
		con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "mission version check requires %u, but found %u; skipping string comparison for mission \"%s\""), static_cast<unsigned>(mission_predicate.descent_version), static_cast<unsigned>(candidate.descent_version), candidate.path.data());
		return nullptr;
	}
#endif
	if (!d_stricmp(mission_predicate.filesystem_name, candidate_filesystem_name))
	{
		con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "found mission \"%s\"[\"%s\"] at %p"), candidate.path.data(), &*candidate.filename, &candidate);
		return &candidate;
	}
	con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "want mission \"%s\", no match for mission \"%s\"[\"%s\"] at %p"), mission_predicate.filesystem_name, candidate.path.data(), &*candidate.filename, &candidate);
	return nullptr;
}

static const mle *compare_mission_by_guess(const mission_entry_predicate mission_predicate, const mle &candidate)
{
	if (candidate.directory.empty())
		return compare_mission_predicate_to_leaf(mission_predicate, candidate, &*candidate.filename);
	{
		const unsigned long size = candidate.directory.size();
		con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "want mission \"%s\", check %lu missions under \"%s\""), mission_predicate.filesystem_name, size, candidate.path.data());
	}
	range_for (auto &i, candidate.directory)
	{
		if (const auto r = compare_mission_by_guess(mission_predicate, i))
			return r;
	}
	con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "no matches under \"%s\""), candidate.path.data());
	return nullptr;
}

static const mle *compare_mission_by_pathname(const mission_entry_predicate mission_predicate, const mle &candidate)
{
	if (candidate.directory.empty())
		return compare_mission_predicate_to_leaf(mission_predicate, candidate, candidate.path.data());
	const auto mission_name = mission_predicate.filesystem_name;
	const auto path_length = candidate.path.size();
	if (!strncmp(mission_name, candidate.path.data(), path_length) && mission_name[path_length] == '/')
	{
		{
			const unsigned long size = candidate.directory.size();
			con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "want mission pathname \"%s\", check %lu missions under \"%s\""), mission_predicate.filesystem_name, size, candidate.path.data());
		}
		range_for (auto &i, candidate.directory)
		{
			if (const auto r = compare_mission_by_pathname(mission_predicate, i))
				return r;
		}
		con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "no matches under \"%s\""), candidate.path.data());
	}
	else
		con_printf(CON_PRIORITY_DEBUG_MISSION_LOAD, DXX_STRINGIZE_FL(__FILE__, __LINE__, "want mission pathname \"%s\", ignore non-matching directory \"%s\""), mission_predicate.filesystem_name, candidate.path.data());
	return nullptr;
}

}

}

Mission_ptr Current_mission; // currently loaded mission

static bool null_or_space(char c)
{
	return !c || isspace(static_cast<unsigned>(c));
}

// Allocate the Level_names, Secret_level_names and Secret_level_table arrays
static int allocate_levels(void)
{
	Level_names = std::make_unique<d_fname[]>(Last_level);
	if (Last_secret_level)
	{
		N_secret_levels = -Last_secret_level;
		Secret_level_names = std::make_unique<d_fname[]>(N_secret_levels);
		Secret_level_table = std::make_unique<ubyte[]>(N_secret_levels);
	}
	
	return 1;
}

//
//  Special versions of mission routines for d1 builtins
//

static const char *load_mission_d1()
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
				return "Failed to allocate level memory for Descent 1 shareware";
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
				return "Failed to allocate level memory for Descent 1 Mac shareware";
			}
			
			//build level names
			for (int i=0;i<Last_level;i++)
				snprintf(&Level_names[i][0u], Level_names[i].size(), "level%02d.sdl", i+1);
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = BIMD1_ENDING_FILE_SHARE;
			break;
		case D1_OEM_MISSION_HOGSIZE:
		case D1_OEM_10_MISSION_HOGSIZE:
			{
			N_secret_levels = 1;
	
			constexpr unsigned last_level = 15;
			constexpr int last_secret_level = -1;
			Last_level = last_level;
			Last_secret_level = last_secret_level;
	
			if (!allocate_levels())
			{
				Current_mission.reset();
				return "Failed to allocate level memory for Descent 1 OEM";
			}
			
			//build level names
			for (unsigned i = 0; i < last_level - 1; ++i)
			{
				auto &ln = Level_names[i];
				snprintf(&ln[0u], ln.size(), "level%02u.rdl", i + 1);
			}
			{
				auto &ln = Level_names[last_level - 1];
				snprintf(&ln[0u], ln.size(), "saturn%02d.rdl", last_level);
			}
			for (int i = 0; i < -last_secret_level; ++i)
			{
				auto &sn = Secret_level_names[i];
				snprintf(&sn[0u], sn.size(), "levels%1d.rdl", i + 1);
			}
			Secret_level_table[0] = 10;
			Briefing_text_filename = "briefsat.txb";
			Ending_text_filename = BIMD1_ENDING_FILE_OEM;
			}
			break;
		default:
			Int3();
			DXX_BOOST_FALLTHROUGH;
		case D1_MISSION_HOGSIZE:
		case D1_MISSION_HOGSIZE2:
		case D1_10_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			{
			N_secret_levels = 3;
	
			constexpr unsigned last_level = BIMD1_LAST_LEVEL;
			constexpr int last_secret_level = BIMD1_LAST_SECRET_LEVEL;
			Last_level = last_level;
			Last_secret_level = last_secret_level;
	
			if (!allocate_levels())
			{
				Current_mission.reset();
				return "Failed to allocate level memory for Descent 1";
			}

			//build level names
			for (unsigned i = 0; i < last_level; ++i)
			{
				auto &ln = Level_names[i];
				snprintf(&ln[0u], ln.size(), "level%02u.rdl", i + 1);
			}
			for (int i = 0; i < -last_secret_level; ++i)
			{
				auto &sn = Secret_level_names[i];
				snprintf(&sn[0u], sn.size(), "levels%1d.rdl", i + 1);
			}
			Secret_level_table[0] = 10;
			Secret_level_table[1] = 21;
			Secret_level_table[2] = 24;
			Briefing_text_filename = BIMD1_BRIEFING_FILE;
			Ending_text_filename = "endreg.txb";
			break;
			}
	}
	return nullptr;
}

#if defined(DXX_BUILD_DESCENT_II)
//
//  Special versions of mission routines for shareware
//

static const char *load_mission_shareware()
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
				return "Failed to allocate level memory for Descent 2 Mac shareware";
			}
			
			// mac demo is using the regular hog and rl2 files
			Level_names[0] = "d2leva-1.rl2";
			Level_names[1] = "d2leva-2.rl2";
			Level_names[2] = "d2leva-3.rl2";
			Level_names[3] = "d2leva-4.rl2";
			Secret_level_names[0] = "d2leva-s.rl2";
			break;
		default:
			Int3();
			DXX_BOOST_FALLTHROUGH;
		case SHAREWARE_MISSION_HOGSIZE:
			N_secret_levels = 0;

			Last_level = 3;
			Last_secret_level = 0;

			if (!allocate_levels())
			{
				Current_mission.reset();
				return "Failed to allocate level memory for Descent 2 shareware";
			}
			Level_names[0] = "d2leva-1.sl2";
			Level_names[1] = "d2leva-2.sl2";
			Level_names[2] = "d2leva-3.sl2";
	}
	return nullptr;
}


//
//  Special versions of mission routines for Diamond/S3 version
//

static const char *load_mission_oem()
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
		return "Failed to allocate level memory for Descent 2 OEM";
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
	return nullptr;
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

static mission_name_and_version get_any_mission_type_name_value(PHYSFSX_gets_line_t<80> &buf, PHYSFS_File *const f, const Mission::descent_version_type descent_version)
{
	if (!PHYSFSX_fgets(buf,f))
		return {};
	if (istok(buf, "name"))
		return {descent_version, get_value(buf)};
#if defined(DXX_BUILD_DESCENT_II)
	if (descent_version == Mission::descent_version_type::descent1)
		/* If reading a Descent 1 `.msn` file, do not check for the
		 * extended mission types.  D1X-Rebirth would ignore them, so
		 * D2X-Rebirth should also ignore them.
		 */
		return {};
	struct name_type_pair
	{
		/* std::pair cannot be used here because direct initialization
		 * from a string literal fails to compile.
		 */
		char name[7];
		Mission::descent_version_type descent_version;
	};
	static constexpr name_type_pair mission_name_type_values[] = {
		{"xname", Mission::descent_version_type::descent2x},	// enhanced mission
		{"zname", Mission::descent_version_type::descent2z},	// super-enhanced mission
		{"!name", Mission::descent_version_type::descent2a},	// extensible-enhanced mission
	};
	range_for (const auto &parm, mission_name_type_values)
	{
		if (istok(buf, parm.name))
			return {parm.descent_version, get_value(buf)};
	}
#endif
	return {};
}

static bool ml_sort_func(const mle &e0,const mle &e1)
{
	const auto d0 = e0.directory.empty();
	const auto d1 = e1.directory.empty();
	if (d0 != d1)
		/* If d0 is a directory and d1 is a mission, or if d0 is a
		 * mission and d1 is a directory, then apply a special case.
		 *
		 * Consider d0 to be less (and therefore ordered earlier) if d1
		 * is a mission.  This moves directories to the top of the list.
		 */
		return d1;
	/* If both d0 and d1 are directories, or if both are missions, then
	 * apply the usual sorting rule.  This makes directories sort
	 * as usual relative to each other.
	 */
	return d_stricmp(e0.mission_name,e1.mission_name) < 0;
}

//returns 1 if file read ok, else 0
namespace dsx {
static int read_mission_file(mission_list_type &mission_list, mission_candidate_search_path &pathname)
{
	if (const auto mfile = PHYSFSX_openReadBuffered(pathname.data()))
	{
		std::string str_pathname = pathname.data();
		const auto idx_last_slash = str_pathname.find_last_of('/');
		const auto idx_filename = (idx_last_slash == str_pathname.npos) ? 0 : idx_last_slash + 1;
		const auto idx_file_extension = str_pathname.find_first_of('.', idx_filename);
		if (idx_file_extension == str_pathname.npos)
			return 0;	//missing extension
		if (idx_file_extension >= DXX_MAX_MISSION_PATH_LENGTH)
			return 0;	// path too long, would be truncated in save game files
		str_pathname.resize(idx_file_extension);
		mission_list.emplace_back(Mission_path(std::move(str_pathname), idx_filename));
		mle *mission = &mission_list.back();
#if defined(DXX_BUILD_DESCENT_I)
		constexpr auto descent_version = Mission::descent_version_type::descent1;
#elif defined(DXX_BUILD_DESCENT_II)
		// look if it's .mn2 or .msn
		auto descent_version = (pathname[idx_file_extension + 3] == MISSION_EXTENSION_DESCENT_II[3])
			? Mission::descent_version_type::descent2
			: Mission::descent_version_type::descent1;
#endif
		mission->anarchy_only_flag = 0;

		PHYSFSX_gets_line_t<80> buf;
		const auto &&nv = get_any_mission_type_name_value(buf, mfile, descent_version);

		if (const auto p = nv.name) {
#if defined(DXX_BUILD_DESCENT_II)
			mission->descent_version = nv.descent_version;
#endif
			char *t;
			if ((t=strchr(p,';'))!=NULL)
			{
				*t=0;
				--t;
			}
			else
				t = p + strlen(p) - 1;
			while (isspace(static_cast<unsigned>(*t)))
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
				const auto p = get_value(temp);
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

	mission_list.emplace_back(Mission_path(D1_MISSION_FILENAME, 0));
	mle *mission = &mission_list.back();
	switch (size) {
	case D1_SHAREWARE_MISSION_HOGSIZE:
	case D1_SHAREWARE_10_MISSION_HOGSIZE:
	case D1_MAC_SHARE_MISSION_HOGSIZE:
		mission->mission_name.copy_if(D1_SHAREWARE_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	case D1_OEM_MISSION_HOGSIZE:
	case D1_OEM_10_MISSION_HOGSIZE:
		mission->mission_name.copy_if(D1_OEM_MISSION_NAME);
		mission->anarchy_only_flag = 0;
		break;
	default:
		Warning("Unknown D1 hogsize %d\n", size);
		Int3();
		DXX_BOOST_FALLTHROUGH;
	case D1_MISSION_HOGSIZE:
	case D1_MISSION_HOGSIZE2:
	case D1_10_MISSION_HOGSIZE:
	case D1_MAC_MISSION_HOGSIZE:
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
}
}

#if defined(DXX_BUILD_DESCENT_II)
template <std::size_t N1, std::size_t N2>
static void set_hardcoded_mission(mission_list_type &mission_list, const char (&path)[N1], const char (&mission_name)[N2])
{
	mission_list.emplace_back(Mission_path(path, 0));
	mle *mission = &mission_list.back();
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
		Int3();
		DXX_BOOST_FALLTHROUGH;
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

static void add_missions_to_list(mission_list_type &mission_list, mission_candidate_search_path &path, const mission_candidate_search_path::iterator rel_path, const mission_filter_mode mission_filter)
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
	assert(std::distance(path.begin(), rel_path) < path.size() - 1);
	assert(!*rel_path);
	assert(path.begin() == rel_path || *std::prev(rel_path) == '/');
	const std::size_t space_remaining = std::distance(rel_path, path.end());
	*rel_path = '.';
	*std::next(rel_path) = 0;
	range_for (const auto i, PHYSFSX_uncounted_list{PHYSFS_enumerateFiles(path.data())})
	{
		/* Add 1 to include the terminating null. */
		const std::size_t il = strlen(i) + 1;
		/* Add 2 for the slash+dot in case it is a directory. */
		if (il + 2 >= space_remaining)
			continue;	// path is too long

		auto j = std::copy_n(i, il, rel_path);
		const char *ext;
		if (PHYSFS_isDirectory(path.data()))
		{
			const auto null = std::prev(j);
			*j = 0;
			*null = '/';
			mission_list_type sublist;
			add_missions_to_list(sublist, path, j, mission_filter);
			*null = 0;
			const auto found = sublist.size();
			if (!found)
			{
				/* Ignore empty directories */
			}
			else if (found == 1)
			{
				/* If only one found, promote it up to the next level so
				 * the user does not need to navigate into a
				 * single-element directory.
				 */
				auto &sli = sublist.front();
				mission_list.emplace_back(std::move(sli));
			}
			else
			{
				std::sort(sublist.begin(), sublist.end(), ml_sort_func);
				mission_list.emplace_back(path.data(), std::move(sublist));
			}
		}
		else if (il > 5 &&
			((ext = &i[il - 5], !d_strnicmp(ext, MISSION_EXTENSION_DESCENT_I))
#if defined(DXX_BUILD_DESCENT_II)
				|| !d_strnicmp(ext, MISSION_EXTENSION_DESCENT_II)
#endif
			))
			if (read_mission_file(mission_list, path))
			{
				if (mission_filter != mission_filter_mode::exclude_anarchy || !mission_list.back().anarchy_only_flag)
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
		DXX_POISON_MEMORY(std::next(rel_path), path.end(), 0xcc);
	}
}
}

/* move <mission_name> to <place> on mission list, increment <place> */
static void promote (mission_list_type &mission_list, const char *const name, std::size_t &top_place)
{
	range_for (auto &i, partial_range(mission_list, top_place, mission_list.size()))
		if (!d_stricmp(&*i.filename, name)) {
			//swap mission positions
			auto &j = mission_list[top_place++];
			if (&j != &i)
				std::swap(j, i);
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

static mission_list_type build_mission_list(const mission_filter_mode mission_filter)
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
	DXX_POISON_MEMORY(std::next(search_str.begin(), sizeof(MISSION_DIR)), search_str.end(), 0xcc);
	add_missions_to_list(mission_list, search_str, search_str.begin() + sizeof(MISSION_DIR) - 1, mission_filter);
	
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

#if defined(DXX_BUILD_DESCENT_II)
//values for built-in mission

int load_mission_ham()
{
	read_hamfile(); // intentionally can also read from the HOG

	if (Piggy_hamfile_version >= 3)
	{
		// re-read sounds in case mission has custom .sXX
		Num_sound_files = 0;
		read_sndfile();
		piggy_read_sounds();
	}

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
		snprintf(t,sizeof(t), "%s.ham", &*Current_mission->filename);
		bm_read_extra_robots(t, Current_mission->descent_version);
		return 1;
	} else
		return 0;
}
#endif
}

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

static void record_briefing(d_fname &f, std::array<char, PATH_MAX> &buf)
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

static const char *load_mission(const mle *const mission)
{
	char *v;

#if defined(DXX_BUILD_DESCENT_II)
	close_extra_robot_movie();
#endif
	Current_mission = std::make_unique<Mission>(static_cast<const Mission_path &>(*mission));
	Current_mission->builtin_hogsize = mission->builtin_hogsize;
	Current_mission->mission_name.copy_if(mission->mission_name);
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
#endif
	Current_mission->anarchy_only_flag = mission->anarchy_only_flag;
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
		if (!d_stricmp(Current_mission->path.c_str(), D1_MISSION_FILENAME))
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
			Int3();
			DXX_BOOST_FALLTHROUGH;
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
	(mission->descent_version != Mission::descent_version_type::descent1) ? MISSION_EXTENSION_DESCENT_II :
#endif
		MISSION_EXTENSION_DESCENT_I;
	std::array<char, PATH_MAX> mission_filename;
	snprintf(mission_filename.data(), mission_filename.size(), "%s%s", mission->path.c_str(), msn_extension);

	PHYSFSEXT_locateCorrectCase(mission_filename.data());

	auto &&mfile = PHYSFSX_openReadBuffered(mission_filename.data());
	if (!mfile) {
		Current_mission.reset();
		con_printf(CON_NORMAL, DXX_STRINGIZE_FL(__FILE__, __LINE__, "error: failed to open mission \"%s\""), mission_filename.data());
		return "Failed to open mission file";		//error!
	}

	//for non-builtin missions, load HOG
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
	if (!PLAYING_BUILTIN_MISSION)
#endif
	{
		strcpy(&mission_filename[mission->path.size() + 1], "hog");		//change extension
			PHYSFSX_contfile_init(mission_filename.data(), 0);
		set_briefing_filename(Briefing_text_filename, &*Current_mission->filename);
		Ending_text_filename = Briefing_text_filename;
	}

	for (PHYSFSX_gets_line_t<PATH_MAX> buf; PHYSFSX_fgets(buf,mfile);)
	{
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
				Level_names = std::make_unique<d_fname[]>(n_levels);
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
				Secret_level_names = std::make_unique<d_fname[]>(n_levels);
				Secret_level_table = std::make_unique<uint8_t[]>(n_levels);
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
				Current_mission->alternate_ham_file = std::make_unique<d_fname>();
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
		return "Failed to parse mission file";
	}

#if defined(DXX_BUILD_DESCENT_II)
	// re-read default HAM file, in case this mission brings it's own version of it
	free_polygon_models(LevelSharedPolygonModelState);

	if (load_mission_ham())
		init_extra_robot_movie(&*Current_mission->filename);
#endif
	return nullptr;
}

//loads the named mission if exists.
//Returns nullptr if mission loaded ok, else error string.
const char *load_mission_by_name (const mission_entry_predicate mission_name, const mission_name_type name_match_mode)
{
	auto &&mission_list = build_mission_list(mission_filter_mode::include_anarchy);
	{
		range_for (auto &i, mission_list)
		{
			switch (name_match_mode)
			{
				case mission_name_type::basename:
					if (!d_stricmp(mission_name.filesystem_name, &*i.filename))
						return load_mission(&i);
					continue;
				case mission_name_type::pathname:
				case mission_name_type::guess:
					if (const auto r = compare_mission_by_pathname(mission_name, i))
						return load_mission(r);
					continue;
				default:
					return "Unhandled load mission type";
			}
		}
	}
	if (name_match_mode == mission_name_type::guess)
	{
		const auto p = strrchr(mission_name.filesystem_name, '/');
		const auto &guess_predicate = p
			? mission_name.with_filesystem_name(p + 1)
			: mission_name;
		range_for (auto &i, mission_list)
		{
			if (const auto r = compare_mission_by_guess(guess_predicate, i))
			{
				con_printf(CON_NORMAL, "%s:%u: request for guessed mission name \"%s\" found \"%s\"", __FILE__, __LINE__, mission_name.filesystem_name, r->path.c_str());
				return load_mission(r);
			}
		}
	}
	return "No matching mission found in\ninstalled mission list.";
}

}

namespace {

template <typename tag>
class unique_menu_tagged_string : std::unique_ptr<char[]>
{
public:
	unique_menu_tagged_string(std::unique_ptr<char[]> p) :
		std::unique_ptr<char[]>(std::move(p))
	{
	}
	using std::unique_ptr<char[]>::get;
	operator menu_tagged_string<tag>() const &
	{
		return {get()};
	}
	operator menu_tagged_string<tag>() const && = delete;
};

struct mission_menu : listbox
{
	static constexpr char listbox_go_up[] = "<..>";
	using callback_type = window_event_result (*)(void);
	/* The top level menu stores the mission data in a member variable.
	 * When this class is the base of toplevel_mission_menu, this
	 * reference points to that member variable in
	 * toplevel_mission_menu.
	 *
	 * Subdirectory menus do not store a copy of the mission data, and
	 * instead store a reference to the mission data in the top level
	 * menu.  When this class is the base of subdirectory_mission_menu,
	 * this reference points to the member variable in the
	 * toplevel_mission_menu that created this subdirectory_mission_menu.
	 *
	 * The data is not reference counted, because the top level menu
	 * always outlives the subdirectory menus.
	 */
	const mission_list_type &ml;
	/* listbox stores an unowned pointer to the string pointers that it
	 * uses.  This member variable stores an owned pointer to the same
	 * string pointers, so that the string pointers persist for the life
	 * of the listbox.  The strings are stored elsewhere.
	 */
	const std::unique_ptr<const char *[]> listbox_strings;
	/* listbox stores an unowned pointer to the title string, since some
	 * listboxes use statically allocated strings.  This member variable
	 * owns the storage for this listbox's title.
	 */
	const unique_menu_tagged_string<menu_title_tag> title;
	const callback_type when_selected;
	virtual window_event_result callback_handler(const d_event &, window_event_result) override;
	mission_menu(const mission_list_type &rml, std::unique_ptr<const char *[]> &&name_pointer_strings, const char *const message, callback_type when_selected, mission_menu *const parent, const int default_item, grs_canvas &canvas) :
		mission_menu(rml, std::move(name_pointer_strings), prepare_title(message, rml), when_selected, parent, default_item, canvas)
	{
	}
protected:
	mission_menu *parent = nullptr;
	mission_menu(const mission_list_type &rml, std::unique_ptr<const char *[]> &&name_pointer_strings, unique_menu_tagged_string<menu_title_tag> title_parameter, callback_type when_selected, mission_menu *const parent, const int default_item, grs_canvas &canvas) :
		listbox(default_item, rml.size(), name_pointer_strings.get(), title_parameter, canvas, 1),
		ml(rml), listbox_strings(std::move(name_pointer_strings)),
		title(std::move(title_parameter)),
		when_selected(when_selected), parent(parent)
	{
	}
	static unique_menu_tagged_string<menu_title_tag> prepare_title(const char *const message, const mission_list_type &ml)
	{
		mission_subdir_stats ss;
		ss.count(ml);
		std::array<char, 12> dirbuf;
		char buf[128];
		const auto r = 1u + std::snprintf(buf, sizeof(buf), "%s\n[%sMSN:LOCAL %zu; TOTAL %zu]", message, prepare_mission_list_count_dirbuf(dirbuf, ss.immediate_directories), ss.immediate_missions, ss.total_missions);
		unique_menu_tagged_string<menu_title_tag> p = std::make_unique<char[]>(r);
		std::memcpy(p.get(), buf, r);
		return p;
	}
};

struct toplevel_mission_menu_storage
{
protected:
	const mission_list_type mission_list_storage;
	toplevel_mission_menu_storage(mission_list_type &&rml) :
		mission_list_storage(std::move(rml))
	{
	}
};

struct toplevel_mission_menu : toplevel_mission_menu_storage, mission_menu
{
public:
	toplevel_mission_menu(mission_list_type &&rml, std::unique_ptr<const char *[]> &&name_pointer_strings, const char *const message, callback_type when_selected, const int default_item, grs_canvas &canvas) :
		toplevel_mission_menu_storage(std::move(rml)),
		mission_menu(mission_list_storage, std::move(name_pointer_strings), message, when_selected, nullptr /* no parent for toplevel menu */, default_item, canvas)
	{
	}
};

struct subdirectory_mission_menu : mission_menu
{
	using mission_menu::mission_menu;
};

constexpr char mission_menu::listbox_go_up[];

struct mission_menu_create_state
{
	std::unique_ptr<const char *[]> listbox_strings;
	unsigned initial_selection = UINT_MAX;
	std::unique_ptr<mission_menu_create_state> submenu;
	mission_menu_create_state(const std::size_t len) :
		listbox_strings(std::make_unique<const char *[]>(len))
	{
	}
	mission_menu_create_state(mission_menu_create_state &&) = default;
};

}

window_event_result mission_menu::callback_handler(const d_event &event, window_event_result)
{
	switch (event.type)
	{
		case EVENT_WINDOW_CREATED:
			break;
		case EVENT_NEWMENU_SELECTED:
		{
			const auto raw_citem = static_cast<const d_select_event &>(event).citem;
			auto citem = raw_citem;
			if (parent)
			{
				if (citem == 0)
				{
					/* Clear parent pointer so that the parent window is
					 * not implicitly closed during handling of
					 * EVENT_WINDOW_CLOSE.
					 */
					parent = nullptr;
					return window_event_result::close;
				}
				/* Adjust for the "Go up" placeholder item */
				-- citem;
			}
			if (citem >= 0)
			{
				auto &mli = ml[citem];
				if (!mli.directory.empty())
				{
					auto listbox_strings = std::make_unique<const char *[]>(mli.directory.size() + 1);
					listbox_strings[0] = listbox_go_up;
					const auto a = [](const mle &m) -> const char * {
						return m.mission_name;
					};
					std::transform(mli.directory.begin(), mli.directory.end(), &listbox_strings[1], a);
					auto submm = window_create<subdirectory_mission_menu>(ml, std::move(listbox_strings), mli.path.c_str(), when_selected, this, 0, grd_curscreen->sc_canvas);
					(void)submm;
					return window_event_result::handled;
				}
				// Chose a mission
				else if (const auto errstr = load_mission(&mli))
				{
					nm_messagebox(menu_title{nullptr}, 1, TXT_OK, "%s\n\n%s\n\n%s", TXT_MISSION_ERROR, errstr, mli.path.c_str());
					return window_event_result::handled;	// stay in listbox so user can select another one
				}
				CGameCfg.LastMission.copy_if(listbox_strings[raw_citem]);
			}
			return (*when_selected)();
		}
		case EVENT_WINDOW_CLOSE:
			/* If the user dismisses the listbox by pressing ESCAPE,
			 * do not close the parent listbox.
			 */
			if (listbox_get_citem(*this) != -1)
				if (parent)
				{
					window_close(parent);
				}
			break;
		default:
			break;
	}
	
	return window_event_result::ignored;
}

using mission_menu_create_state_ptr = std::unique_ptr<mission_menu_create_state>;

static mission_menu_create_state_ptr prepare_mission_menu_state(const mission_list_type &mission_list, const char *const LastMission, const std::size_t extra_strings)
{
	auto mission_name_to_select = LastMission;
	auto p = std::make_unique<mission_menu_create_state>(mission_list.size() + extra_strings);
	auto &create_state = *p.get();
	auto listbox_strings = create_state.listbox_strings.get();
	std::fill_n(listbox_strings, extra_strings, nullptr);
	listbox_strings += extra_strings;
	range_for (auto &&e, enumerate(mission_list))
	{
		auto &mli = e.value;
		const char *const mission_name = mli.mission_name;
		*listbox_strings++ = mission_name;
		if (!mission_name_to_select)
			continue;
		if (!mli.directory.empty())
		{
			auto &&substate = prepare_mission_menu_state(mli.directory, mission_name_to_select, 1);
			if (substate->initial_selection == UINT_MAX)
				continue;
			substate->listbox_strings[0] = mission_menu::listbox_go_up;
			create_state.submenu = std::move(substate);
		}
		else if (strcmp(mission_name, mission_name_to_select))
			continue;
		create_state.initial_selection = e.idx;
		mission_name_to_select = nullptr;
	}
	return p;
}

namespace dsx {

int select_mission(const mission_filter_mode mission_filter, const menu_title message, window_event_result (*when_selected)(void))
{
	auto &&mission_list = build_mission_list(mission_filter);
	int new_mission_num;

    if (mission_list.size() <= 1)
	{
        new_mission_num = !mission_list.empty() && !load_mission(&mission_list.front()) ? 0 : -1;
		(*when_selected)();
		
		return (new_mission_num >= 0);
    }
	else
	{
		auto &&create_state_ptr = prepare_mission_menu_state(mission_list, CGameCfg.LastMission, 0);
		auto &create_state = *create_state_ptr.get();
		mission_menu *parent_mission_menu;
		{
			auto mm = window_create<toplevel_mission_menu>(std::move(mission_list), std::move(create_state.listbox_strings), message, when_selected, create_state.initial_selection == UINT_MAX ? 0 : create_state.initial_selection, grd_curscreen->sc_canvas);
			parent_mission_menu = mm;
		}
		for (auto parent_state = &create_state; const auto substate = parent_state->submenu.get(); parent_state = substate)
		{
			const auto parent_initial_selection = parent_state->initial_selection;
			const auto parent_mission_list_size = parent_mission_menu->ml.size();
			assert(parent_initial_selection < parent_mission_list_size);
			if (parent_initial_selection >= parent_mission_list_size)
				break;
			const auto &substate_mission_list = parent_mission_menu->ml[parent_initial_selection];
			auto submm = window_create<subdirectory_mission_menu>(substate_mission_list.directory, std::move(substate->listbox_strings), substate_mission_list.path.c_str(), when_selected, parent_mission_menu, 0, grd_curscreen->sc_canvas);
			parent_mission_menu = submm;
		}
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
	std::array<char, PATH_MAX> mission_filename;
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
	Current_mission = std::make_unique<Mission>(Mission_path(MISSION_DIR "new_miss", sizeof(MISSION_DIR) - 1));		// limited to eight characters because of savegame format
	Current_mission->mission_name.copy_if("Untitled");
	Current_mission->builtin_hogsize = 0;
	Current_mission->anarchy_only_flag = 0;
	
	Level_names = std::make_unique<d_fname[]>(1);
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

}
