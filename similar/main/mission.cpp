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

#include "dxxsconf.h"
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ranges>
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

#include "compiler-cf_assert.h"
#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_underlying_value.h"
#include "d_zip.h"
#include <memory>

#define BIMD1_BRIEFING_FILE		"briefing.txb"

using std::min;

#define MISSION_EXTENSION_DESCENT_I	".msn"
#if defined(DXX_BUILD_DESCENT_II)
#define MISSION_EXTENSION_DESCENT_II	".mn2"
#endif

#define CON_PRIORITY_DEBUG_MISSION_LOAD	CON_DEBUG

namespace dcx {

char descent_hog_basename[]{"descent.hog"};

namespace {

using mission_candidate_search_path = std::array<char, PATH_MAX>;

auto prepare_mission_list_count_dirbuf(const std::size_t immediate_directories)
{
	std::array<char, sizeof("DIR:99999; ")> dirbuf;
	/* Limit the count of directories to what can be formatted
	 * successfully without truncation.  If a user has more than this
	 * many directories, an empty string will be used instead of showing
	 * the actual count.
	 */
	if (immediate_directories && immediate_directories <= 99999)
		std::snprintf(std::data(dirbuf), std::size(dirbuf), "DIR:%u; ", DXX_size_t_cast_unsigned_int(immediate_directories));
	else
		dirbuf.front() = 0;
	return dirbuf;
}

}

}

namespace dsx {

char descent2_hog_basename[]{"descent2.hog"};
char d2demo_hog_basename[]{"d2demo.hog"};

namespace {

struct mle;
using mission_list_type = std::vector<mle>;

//mission list entry
struct mle : Mission_path
{
	static constexpr std::size_t maximum_mission_name_length{75};
	descent_hog_size builtin_hogsize;    // if it's the built-in mission, used for determining the version
#if defined(DXX_BUILD_DESCENT_II)
	descent_version_type descent_version;    // descent 1 or descent 2?
#endif
	Mission::anarchy_only_level anarchy_only_flag;  // if true, mission is anarchy only
	mission_list_type directory;
	ntstring<maximum_mission_name_length> mission_name;
	mle(Mission_path &&m,
		const descent_hog_size builtin_hogsize,
#if defined(DXX_BUILD_DESCENT_II)
		const descent_version_type descent_version,
#endif
		const Mission::anarchy_only_level anarchy_only_flag,
		const std::span<const char> mission_name
		) :
		Mission_path{std::move(m)},
		builtin_hogsize{builtin_hogsize},
#if defined(DXX_BUILD_DESCENT_II)
		descent_version{descent_version},
#endif
		anarchy_only_flag{anarchy_only_flag}
	{
		this->mission_name.copy_if(mission_name.data(), mission_name.size());
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
	const Mission::descent_version_type descent_version{};
#endif
	const char *const name{};
	mission_name_and_version() = default;
	mission_name_and_version(Mission::descent_version_type, const char *);
};

mission_name_and_version::mission_name_and_version(Mission::descent_version_type const v, const char *const n) :
#if defined(DXX_BUILD_DESCENT_II)
	descent_version{v},
#endif
	name{n}
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)v;
#endif
}

mle::mle(const char *const name, std::vector<mle> &&d) :
	Mission_path(name, 0), directory(std::move(d))
{
	mission_subdir_stats ss;
	ss.count(directory);
	snprintf(mission_name.data(), mission_name.size(), "%s/ [%sMSN:L%zu;T%zu]", name, std::data(prepare_mission_list_count_dirbuf(ss.immediate_directories)), ss.immediate_missions, ss.total_missions);
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

Mission_ptr Current_mission; // currently loaded mission

}

namespace {

static bool null_or_space(char c)
{
	return !c || isspace(static_cast<unsigned>(c));
}

// Allocate the Current_mission->level_names, Current_mission->secret_level_names and Current_mission->secret_level_table arrays
static void allocate_levels(const unsigned count_regular_level, const unsigned count_secret_level)
{
	Current_mission->level_names = std::make_unique<d_fname[]>(count_regular_level);
	Current_mission->last_level = count_regular_level;
	Current_mission->n_secret_levels = count_secret_level;
	Current_mission->last_secret_level = -static_cast<signed>(count_secret_level);
	if (count_secret_level)
	{
		auto secret_level_names = std::make_unique<d_fname[]>(count_secret_level);
		auto secret_level_table = std::make_unique<uint8_t[]>(count_secret_level);
		Current_mission->secret_level_names = std::move(secret_level_names);
		Current_mission->secret_level_table = std::move(secret_level_table);
	}
}

static void allocate_shareware_levels(const unsigned count_regular_level, const unsigned count_secret_level)
{
	Current_mission->briefing_text_filename = BIMD1_BRIEFING_FILE;
	Current_mission->ending_text_filename = BIMD1_ENDING_FILE_SHARE;
	allocate_levels(count_regular_level, count_secret_level);
	//build level names
	for (const auto &&[idx, name] : enumerate(std::span(Current_mission->level_names.get(), count_regular_level), 1u))
	{
		cf_assert(idx <= count_regular_level);
		snprintf(name.data(), name.size(), "level%02u.sdl", idx);
	}
}

static void build_rdl_regular_level_names(const unsigned count_regular_level, std::unique_ptr<d_fname[]> &names)
{
	for (auto &&[idx, name] : enumerate(std::span(names.get(), count_regular_level), 1u))
	{
		cf_assert(idx <= count_regular_level);
		snprintf(name.data(), name.size(), "level%02u.rdl", idx);
	}
}

static void build_rdl_secret_level_names(const unsigned count_secret_level, std::unique_ptr<d_fname[]> &names)
{
	for (auto &&[idx, name] : enumerate(std::span(names.get(), count_secret_level), 1u))
	{
		cf_assert(idx <= count_secret_level);
		snprintf(name.data(), name.size(), "levels%1u.rdl", idx);
	}
}

//
//  Special versions of mission routines for d1 builtins
//

static void load_mission_d1()
{
	switch (descent_hog_size{PHYSFSX_fsize(descent_hog_basename)})
	{
		case descent_hog_size::pc_shareware_v14:
		case descent_hog_size::pc_shareware_v10:
			allocate_shareware_levels(7, 0);
			break;
		case descent_hog_size::mac_shareware:
			allocate_shareware_levels(3, 0);
			break;
		case descent_hog_size::pc_oem_v14:
		case descent_hog_size::pc_oem_v10:
			{
			constexpr unsigned last_level = 15;
			constexpr unsigned last_secret_level = 1;
			allocate_levels(last_level, last_secret_level);
			//build level names
			build_rdl_regular_level_names(last_level - 1, Current_mission->level_names);
			{
				auto &ln = Current_mission->level_names[last_level - 1];
				snprintf(ln.data(), ln.size(), "saturn%02d.rdl", last_level);
			}
			build_rdl_secret_level_names(last_secret_level, Current_mission->secret_level_names);
			Current_mission->secret_level_table[0] = 10;
			Current_mission->briefing_text_filename = "briefsat.txb";
			Current_mission->ending_text_filename = BIMD1_ENDING_FILE_OEM;
			}
			break;
		default:
			Int3();
			[[fallthrough]];
		case descent_hog_size::pc_retail_v15:
		case descent_hog_size::pc_retail_v15_alt1:
		case descent_hog_size::pc_retail_v10:
		case descent_hog_size::mac_retail:
			{
			constexpr unsigned last_level = 27;
			constexpr unsigned last_secret_level = 3;
			allocate_levels(last_level, last_secret_level);

			//build level names
			build_rdl_regular_level_names(last_level, Current_mission->level_names);
			build_rdl_secret_level_names(last_secret_level, Current_mission->secret_level_names);
			Current_mission->secret_level_table[0] = 10;
			Current_mission->secret_level_table[1] = 21;
			Current_mission->secret_level_table[2] = 24;
			Current_mission->briefing_text_filename = BIMD1_BRIEFING_FILE;
			Current_mission->ending_text_filename = "endreg.txb";
			break;
			}
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//
//  Special versions of mission routines for shareware
//

static void load_mission_shareware()
{
    Current_mission->mission_name.copy_if(SHAREWARE_MISSION_NAME);
    Current_mission->descent_version = Mission::descent_version_type::descent2;
    Current_mission->anarchy_only_flag = Mission::anarchy_only_level::allow_any_game;
    
    switch (Current_mission->builtin_hogsize)
	{
		case descent_hog_size::d2_mac_shareware:
			{
				constexpr unsigned last_level = 4;
				allocate_levels(last_level, 1);
			}
			// mac demo is using the regular hog and rl2 files
			Current_mission->level_names[0] = "d2leva-1.rl2";
			Current_mission->level_names[1] = "d2leva-2.rl2";
			Current_mission->level_names[2] = "d2leva-3.rl2";
			Current_mission->level_names[3] = "d2leva-4.rl2";
			Current_mission->secret_level_names[0] = "d2leva-s.rl2";
			break;
		default:
			Int3();
			[[fallthrough]];
		case descent_hog_size::d2_shareware:
			{
				constexpr unsigned last_level = 3;
				allocate_levels(last_level, 0);
			}
			Current_mission->level_names[0] = "d2leva-1.sl2";
			Current_mission->level_names[1] = "d2leva-2.sl2";
			Current_mission->level_names[2] = "d2leva-3.sl2";
			break;
	}
}


//
//  Special versions of mission routines for Diamond/S3 version
//

static void load_mission_oem()
{
    Current_mission->mission_name.copy_if(OEM_MISSION_NAME);
    Current_mission->descent_version = Mission::descent_version_type::descent2;
    Current_mission->anarchy_only_flag = Mission::anarchy_only_level::allow_any_game;
    
	{
		constexpr unsigned last_level = 8;
		allocate_levels(last_level, 2);
	}
	Current_mission->level_names[0] = "d2leva-1.rl2";
	Current_mission->level_names[1] = "d2leva-2.rl2";
	Current_mission->level_names[2] = "d2leva-3.rl2";
	Current_mission->level_names[3] = "d2leva-4.rl2";
	Current_mission->secret_level_names[0] = "d2leva-s.rl2";
	Current_mission->level_names[4] = "d2levb-1.rl2";
	Current_mission->level_names[5] = "d2levb-2.rl2";
	Current_mission->level_names[6] = "d2levb-3.rl2";
	Current_mission->level_names[7] = "d2levb-4.rl2";
	Current_mission->secret_level_names[1] = "d2levb-s.rl2";
	Current_mission->secret_level_table[0] = 1;
	Current_mission->secret_level_table[1] = 5;
}
#endif

//compare a string for a token. returns true if match
static int istok(const char *buf,const char *tok)
{
	return d_strnicmp(buf,tok,strlen(tok)) == 0;
}

//returns ptr to string after '=' & white space, or NULL if no '='
static const char *get_value(const char *buf)
{
	if (auto t = strchr(buf, '='))
	{
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
	for (auto &&[name, descent_version] : mission_name_type_values)
	{
		if (istok(buf, name))
			return {descent_version, get_value(buf)};
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

}

//returns 1 if file read ok, else 0
namespace dsx {

namespace {

static const mle *read_mission_file(mission_list_type &mission_list, std::string_view str_pathname, const descent_hog_size descent_hog_size, const mission_filter_mode mission_filter)
{
	if (const auto mfile = PHYSFSX_openReadBuffered(str_pathname.data()).first)
	{
		const auto idx_last_slash{str_pathname.find_last_of('/')};
		/* If no slash is found, the filename starts at the beginning of the
		 * view.  If a slash is found, the filename starts at the next
		 * character after the slash.
		 */
		const auto idx_filename{(idx_last_slash == str_pathname.npos) ? 0 : idx_last_slash + 1};
		const auto idx_file_extension{str_pathname.find_first_of('.', {idx_filename})};
		if (idx_file_extension == str_pathname.npos)
			return nullptr;	//missing extension
		if (idx_file_extension >= DXX_MAX_MISSION_PATH_LENGTH)
			return nullptr;	// path too long, would be truncated in save game files
#if defined(DXX_BUILD_DESCENT_I)
		constexpr auto descent_version = Mission::descent_version_type::descent1;
#elif defined(DXX_BUILD_DESCENT_II)
		// look if it's .mn2 or .msn
		const auto descent_version = (str_pathname[idx_file_extension + 3] == MISSION_EXTENSION_DESCENT_II[3])
			? Mission::descent_version_type::descent2
			: Mission::descent_version_type::descent1;
#endif
		str_pathname.remove_suffix(str_pathname.size() - idx_file_extension);

		PHYSFSX_gets_line_t<80> buf;
		const auto &&nv = get_any_mission_type_name_value(buf, mfile, descent_version);
		const auto &p = nv.name;
		if (!p)
			return nullptr;

		const auto semicolon = strchr(p, ';');
		/* If a semicolon exists, point to it.  Otherwise, point to the
		 * null byte terminating the buffer.
		 */
		auto t = semicolon ? semicolon : std::next(p, strlen(p));
		/* Iterate backward until either the beginning of the buffer or the
		 * first non-whitespace character.
		 */
		for (; t != p && isspace(static_cast<unsigned>(*t));)
		{
			-- t;
		}

		auto anarchy_only_flag{Mission::anarchy_only_level::allow_any_game};
		if (PHYSFSX_gets_line_t<64> temp; PHYSFSX_fgets(temp, mfile))
		{
			if (istok(temp,"type"))
			{
				//get mission type
				if (const auto p = get_value(temp))
				{
					if (istok(p, "anarchy"))
					{
						if (mission_filter == mission_filter_mode::exclude_anarchy)
							return nullptr;
						anarchy_only_flag = Mission::anarchy_only_level::only_anarchy_games;
					}
				}
			}
		}
		return &mission_list.emplace_back(
			/* Cast to ptrdiff_t is safe, because
			 * `if (idx_file_extension >= DXX_MAX_MISSION_PATH_LENGTH)` is
			 * true, then execution does not reach this line.  All values in
			 * [0, DXX_MAX_MISSION_PATH_LENGTH) can be represented by
			 * `std::ptrdiff_t`, so no narrowing occurs.
			 */
			Mission_path{str_pathname, static_cast<std::ptrdiff_t>(idx_filename)},
			descent_hog_size,
#if defined(DXX_BUILD_DESCENT_II)
			nv.descent_version,
#endif
			anarchy_only_flag,
			std::span(p, std::min<std::size_t>(mle::maximum_mission_name_length, std::distance(p, t)))
		);
	}
	return nullptr;
}

static std::span<const char> get_d1_mission_name_from_descent_hog_size(const descent_hog_size size)
{
	switch (size) {
		case descent_hog_size::pc_shareware_v14:
		case descent_hog_size::pc_shareware_v10:
		case descent_hog_size::mac_shareware:
			return D1_SHAREWARE_MISSION_NAME;
		case descent_hog_size::pc_oem_v14:
		case descent_hog_size::pc_oem_v10:
			return D1_OEM_MISSION_NAME;
		default:
			Warning("Unknown D1 hogsize %d", underlying_value(size));
			Int3();
			[[fallthrough]];
		case descent_hog_size::pc_retail_v15:
		case descent_hog_size::pc_retail_v15_alt1:
		case descent_hog_size::pc_retail_v10:
		case descent_hog_size::mac_retail:
			return D1_MISSION_NAME;
	}
}

static void add_d1_builtin_mission_to_list(mission_list_type &mission_list)
{
	const descent_hog_size size{PHYSFSX_fsize(descent_hog_basename)};
	if (size == descent_hog_size{-1})
		return;

	mission_list.emplace_back(
		Mission_path{D1_MISSION_FILENAME, 0},
		size,
#if defined(DXX_BUILD_DESCENT_II)
		Mission::descent_version_type::descent1,	/* The Descent 1 builtin campaign is always treated as a Descent 1 mission. */
#endif
		Mission::anarchy_only_level::allow_any_game,
		get_d1_mission_name_from_descent_hog_size(size)
	);
}

#if defined(DXX_BUILD_DESCENT_II)
template <std::size_t N1, std::size_t N2>
static const mle *set_hardcoded_mission(mission_list_type &mission_list, const char (&path)[N1], const char (&mission_name)[N2], const descent_hog_size descent_hog_size)
{
	return &mission_list.emplace_back(
		Mission_path{path, 0},
		descent_hog_size,
		Mission::descent_version_type::descent2,
		Mission::anarchy_only_level::allow_any_game,
		mission_name
	);
}

static void add_builtin_mission_to_list(mission_list_type &mission_list, d_fname &name)
{
    descent_hog_size size{PHYSFSX_fsize(descent2_hog_basename)};
	if (size == descent_hog_size{-1})
		size = descent_hog_size{PHYSFSX_fsize(d2demo_hog_basename)};

	const mle *mission;
	switch (size) {
		case descent_hog_size::d2_shareware:
		case descent_hog_size::d2_mac_shareware:
			mission = set_hardcoded_mission(mission_list, SHAREWARE_MISSION_FILENAME, SHAREWARE_MISSION_NAME, size);
			break;
		case descent_hog_size::d2_oem_v11:
			mission = set_hardcoded_mission(mission_list, OEM_MISSION_FILENAME, OEM_MISSION_NAME, size);
			break;
	default:
			Warning("Unknown hogsize %d, trying %s", underlying_value(size), FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II);
		Int3();
		[[fallthrough]];
	case descent_hog_size::d2_full:
	case descent_hog_size::d2_full_v10:
	case descent_hog_size::d2_mac_full:
		if (const std::string_view full_mission_filename{{FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II}}; !(mission = read_mission_file(mission_list, full_mission_filename, size, mission_filter_mode::include_anarchy)))
				Error("Could not find required mission file <%s>", FULL_MISSION_FILENAME MISSION_EXTENSION_DESCENT_II);
	}
	name.copy_if(mission->path.c_str(), FILENAME_LEN);
}
#endif

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
	const PHYSFSX_uncounted_list s{PHYSFS_enumerateFiles(path.data())};
	if (!s)
		return;
	for (const auto i : s)
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
			read_mission_file(mission_list, path.data(), descent_hog_size::None, mission_filter);
		if (mission_list.size() >= MAX_MISSIONS)
		{
			break;
		}
		*rel_path = 0;	// chop off the entry
		DXX_POISON_MEMORY(std::span(std::next(rel_path), path.end()), 0xcc);
	}
}

}

}

namespace {

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

}

Mission::~Mission()
{
    // May become more complex with the editor
	if (!path.empty() && builtin_hogsize == descent_hog_size::None)
		{
			char hogpath[PATH_MAX];
			snprintf(hogpath, sizeof(hogpath), "%s.hog", path.c_str());
			PHYSFSX_removeRelFromSearchPath(hogpath);
		}
}

//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode is set, then also add anarchy-only missions.

namespace dsx {

namespace {

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
	mission_candidate_search_path search_str{{MISSION_DIR}};
	DXX_POISON_MEMORY(std::span(search_str).subspan<sizeof(MISSION_DIR)>(), 0xcc);
	add_missions_to_list(mission_list, search_str, std::next(search_str.begin(), sizeof(MISSION_DIR) - 1), mission_filter);
	
	// move original missions (in story-chronological order)
	// to top of mission list
	std::size_t top_place{};
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
	read_hamfile(LevelSharedRobotInfoState); // intentionally can also read from the HOG

	if (Piggy_hamfile_version >= pig_hamfile_version::_3)
	{
		// re-read sounds in case mission has custom .sXX
		Num_sound_files = 0;
		read_sndfile(0);
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
		const auto mount_hog = make_PHYSFSX_ComputedPathMount(p, althog, physfs_search_path::prepend);
		bm_read_extra_robots(*altham, Mission::descent_version_type::descent2z);
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

namespace {

#define tex ".tex"
static void set_briefing_filename(d_fname &f, const std::span<const char> v)
{
	f.copy_if(v.data(), v.size());
	f.copy_if(v.size(), tex);
	if (!PHYSFSX_exists_ignorecase(f.data()) &&
		!(f.copy_if(v.size() + 1, "txb"), PHYSFSX_exists_ignorecase(f.data()))) // check if this file exists ...
		f = {};
}

static void set_briefing_filename(d_fname &f, const char *const v)
{
	using std::next;
	auto a = [](char c) {
		return !c || c == '.';
	};
	const auto &&i{std::ranges::find_if(v, next(v, f.size() - sizeof(tex)), a)};
	std::size_t d = std::distance(v, i);
	set_briefing_filename(f, {v, d});
}

static void record_briefing(d_fname &f, const std::array<char, PATH_MAX> &buf)
{
	const auto v = get_value(buf.data());
	if (!v)
		return;
	const std::size_t d = std::distance(v, std::ranges::find_if(v, buf.end(), null_or_space));
	if (d >= FILENAME_LEN)
		return;
	{
		set_briefing_filename(f, {v, std::min(d, f.size() - sizeof(tex))});
	}
}
#undef tex

}

//loads the specfied mission from the mission list.
//build_mission_list() must have been called.
//Returns true if mission loaded ok, else false.
namespace dsx {

namespace {

static const char *load_mission(const mle *const mission)
{
	Current_mission = std::make_unique<Mission>(static_cast<const Mission_path &>(*mission));
	Current_mission->builtin_hogsize = mission->builtin_hogsize;
	Current_mission->mission_name.copy_if(mission->mission_name);
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
#endif
	Current_mission->anarchy_only_flag = mission->anarchy_only_flag;

	// for Descent 1 missions, load descent.hog
#if defined(DXX_BUILD_DESCENT_II)
	if (EMULATING_D1)
#endif
	{
		std::array<char, PATH_MAX> pathname;
		if (const auto r = PHYSFSX_addRelToSearchPath(descent_hog_basename, pathname, physfs_search_path::prepend); r != PHYSFS_ERR_OK)
#if defined(DXX_BUILD_DESCENT_I)
			Error("descent.hog not available!\n%s", PHYSFS_getErrorByCode(r));
#elif defined(DXX_BUILD_DESCENT_II)
			Warning("descent.hog not available!\n%s\nThis mission may be missing some files required for briefings and exit sequence.", PHYSFS_getErrorByCode(r));
#endif
		if (!d_stricmp(Current_mission->path.c_str(), D1_MISSION_FILENAME))
		{
			load_mission_d1();
			return nullptr;
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else
		PHYSFSX_removeRelFromSearchPath(descent_hog_basename);
#endif

#if defined(DXX_BUILD_DESCENT_II)
	if (PLAYING_BUILTIN_MISSION) {
		switch (Current_mission->builtin_hogsize) {
			case descent_hog_size::d2_shareware:
			case descent_hog_size::d2_mac_shareware:
			Current_mission->briefing_text_filename = "brief2.txb";
			Current_mission->ending_text_filename = BIMD2_ENDING_FILE_SHARE;
			load_mission_shareware();
			return nullptr;
			case descent_hog_size::d2_oem_v11:
			Current_mission->briefing_text_filename = "brief2o.txb";
			Current_mission->ending_text_filename = BIMD2_ENDING_FILE_OEM;
			load_mission_oem();
			return nullptr;
		default:
			Int3();
			[[fallthrough]];
		case descent_hog_size::d2_full:
		case descent_hog_size::d2_full_v10:
		case descent_hog_size::d2_mac_full:
			Current_mission->briefing_text_filename = "robot.txb";
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

	auto &&[mfile, physfserr] = PHYSFSX_openReadBuffered_updateCase(mission_filename.data());
	if (!mfile) {
		Current_mission.reset();
		con_printf(CON_NORMAL, DXX_STRINGIZE_FL(__FILE__, __LINE__, "error: failed to open mission \"%s\": %s"), mission_filename.data(), PHYSFS_getErrorByCode(physfserr));
		return "Failed to open mission file";		//error!
	}

	//for non-builtin missions, load HOG
#if defined(DXX_BUILD_DESCENT_II)
	Current_mission->descent_version = mission->descent_version;
	if (!PLAYING_BUILTIN_MISSION)
#endif
	{
		strcpy(&mission_filename[mission->path.size() + 1], "hog");		//change extension
		std::array<char, PATH_MAX> pathname;
		PHYSFSX_addRelToSearchPath(mission_filename.data(), pathname, physfs_search_path::prepend);
		set_briefing_filename(Current_mission->briefing_text_filename, &*Current_mission->filename);
		Current_mission->ending_text_filename = Current_mission->briefing_text_filename;
	}

	for (PHYSFSX_gets_line_t<PATH_MAX> buf; PHYSFSX_fgets(buf,mfile);)
	{
		if (istok(buf,"type"))
			continue;						//already have name, go to next line
		else if (istok(buf,"briefing")) {
			record_briefing(Current_mission->briefing_text_filename, buf.line());
		}
		else if (istok(buf,"ending")) {
			record_briefing(Current_mission->ending_text_filename, buf.line());
		}
		else if (istok(buf,"num_levels")) {
			if (const auto v{get_value(buf)})
			{
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
				auto names = std::make_unique<d_fname[]>(n_levels);
				uint8_t level_names_loaded = 0;
				for (auto &i : unchecked_partial_range(names.get(), n_levels))
				{
					if (!PHYSFSX_fgets(buf, mfile))
						break;
					auto &line = buf.line();
					const auto &&s{std::ranges::find_if(line, null_or_space)};
					if (i.copy_if(line, std::distance(line.begin(), s)))
					{
						++level_names_loaded;
					}
					else
						break;
				}
				Current_mission->last_level = level_names_loaded;
				Current_mission->level_names = std::move(names);
			}
		}
		else if (istok(buf,"num_secrets")) {
			if (const auto v{get_value(buf)})
			{
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
				Current_mission->n_secret_levels = n_levels;
				uint8_t level_names_loaded = 0;
				auto names = std::make_unique<d_fname[]>(n_levels);
				auto table = std::make_unique<uint8_t[]>(n_levels);
				for (auto &&[name, table_cell] : zip(unchecked_partial_range(names.get(), n_levels), unchecked_partial_range(table.get(), n_levels)))
				{
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
					const auto &&s{std::ranges::find_if(lb, t, a)};
					if (name.copy_if(line, std::distance(lb, s)))
					{
						unsigned long ls = strtoul(t + 1, &ip, 10);
						if (ls < 1 || ls > Current_mission->last_level)
							break;
						table_cell = ls;
						++ level_names_loaded;
					}
					else
						break;
				}
				Current_mission->last_secret_level = -static_cast<signed>(level_names_loaded);
				Current_mission->secret_level_names = std::move(names);
				Current_mission->secret_level_table = std::move(table);
			}
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (Current_mission->descent_version == Mission::descent_version_type::descent2a && buf[0u] == '!') {
			if (istok(buf+1,"ham")) {
				Current_mission->alternate_ham_file = std::make_unique<d_fname>();
				if (const auto v{get_value(buf)})
				{
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
				Current_mission->last_level = 0;
				break;
			}
		}
#endif

	}
	mfile.reset();
	if (Current_mission->last_level <= 0)
	{
		Current_mission.reset();		//no valid mission loaded
		return "Failed to parse mission file";
	}

#if defined(DXX_BUILD_DESCENT_II)
	// re-read default HAM file, in case this mission brings it's own version of it
	free_polygon_models(LevelSharedPolygonModelState);

	if (load_mission_ham())
		Current_mission->extra_robot_movie = init_extra_robot_movie(&*Current_mission->filename);
#endif
	return nullptr;
}

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
	mission_menu(const mission_list_type &rml, const std::size_t count_name_pointer_strings, std::unique_ptr<const char *[]> &&name_pointer_strings, const char *const message, callback_type when_selected, mission_menu *const parent, const int default_item, grs_canvas &canvas) :
		mission_menu(rml, count_name_pointer_strings, std::move(name_pointer_strings), prepare_title(message, rml), when_selected, parent, default_item, canvas)
	{
	}
protected:
	mission_menu *parent = nullptr;
	mission_menu(const mission_list_type &rml, const std::size_t count_name_pointer_strings, std::unique_ptr<const char *[]> &&name_pointer_strings, unique_menu_tagged_string<menu_title_tag> title_parameter, callback_type when_selected, mission_menu *const parent, const int default_item, grs_canvas &canvas) :
		listbox(default_item, count_name_pointer_strings, name_pointer_strings.get(), title_parameter, canvas, 1),
		ml(rml), listbox_strings(std::move(name_pointer_strings)),
		title(std::move(title_parameter)),
		when_selected(when_selected), parent(parent)
	{
	}
	static unique_menu_tagged_string<menu_title_tag> prepare_title(const char *const message, const mission_list_type &ml)
	{
		mission_subdir_stats ss;
		ss.count(ml);
		char buf[128];
		const auto r = 1u + std::snprintf(buf, sizeof(buf), "%s\n[%sMSN:LOCAL %zu; TOTAL %zu]", message, std::data(prepare_mission_list_count_dirbuf(ss.immediate_directories)), ss.immediate_missions, ss.total_missions);
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
		mission_menu(mission_list_storage, mission_list_storage.size(), std::move(name_pointer_strings), message, when_selected, nullptr /* no parent for toplevel menu */, default_item, canvas)
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
		case event_type::window_created:
			break;
		case event_type::newmenu_selected:
		{
			const auto raw_citem = static_cast<const d_select_event &>(event).citem;
			auto citem = raw_citem;
			if (parent)
			{
				if (citem == 0)
				{
					/* Clear parent pointer so that the parent window is
					 * not implicitly closed during handling of
					 * event_type::window_close.
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
					/* Increment by 1 to allow the pseudo-entry
					 * `listbox_go_up`.
					 */
					const std::size_t count_name_pointer_strings = mli.directory.size() + 1;
					auto listbox_strings = std::make_unique<const char *[]>(count_name_pointer_strings);
					listbox_strings[0] = listbox_go_up;
					const auto a = [](const mle &m) -> const char * {
						return m.mission_name;
					};
					std::transform(mli.directory.begin(), mli.directory.end(), &listbox_strings[1], a);
					auto submm = window_create<subdirectory_mission_menu>(mli.directory, count_name_pointer_strings, std::move(listbox_strings), mli.path.c_str(), when_selected, this, 0, grd_curscreen->sc_canvas);
					(void)submm;
					return window_event_result::handled;
				}
				// Chose a mission
				else if (const auto errstr = load_mission(&mli))
				{
					nm_messagebox(menu_title{nullptr}, {TXT_OK}, "%s\n\n%s\n\n%s", TXT_MISSION_ERROR, errstr, mli.path.c_str());
					return window_event_result::handled;	// stay in listbox so user can select another one
				}
				CGameCfg.LastMission.copy_if(listbox_strings[raw_citem]);
			}
			return (*when_selected)();
		}
		case event_type::window_close:
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

namespace {

/* Reserve an element to allow the pseudo-entry `listbox_go_up`.
 */
constexpr std::integral_constant<std::size_t, 1> mission_subdirectory_extra_strings{};

static std::unique_ptr<mission_menu_create_state> prepare_mission_menu_state(const mission_list_type &mission_list, const char *const LastMission, const std::size_t extra_strings)
{
	auto mission_name_to_select = LastMission;
	auto p = std::make_unique<mission_menu_create_state>(mission_list.size() + extra_strings);
	auto &create_state = *p.get();
	auto listbox_strings = create_state.listbox_strings.get();
	std::fill_n(listbox_strings, extra_strings, nullptr);
	listbox_strings += extra_strings;
	for (auto &&[idx, mli] : enumerate(mission_list))
	{
		const char *const mission_name = mli.mission_name;
		*listbox_strings++ = mission_name;
		if (!mission_name_to_select)
			/* Once the mission to select has been found, copy subsequent
			 * mission names, but skip all the other logic.  There is no need
			 * to examine subdirectories, because that examination is only for
			 * finding the mission to select.
			 */
			continue;
		if (!mli.directory.empty())
		{
			/* If this entry represents a subdirectory, prepare a corresponding
			 * inner state for it.
			 */
			auto &&substate = prepare_mission_menu_state(mli.directory, mission_name_to_select, mission_subdirectory_extra_strings);
			if (substate->initial_selection == UINT_MAX)
				/* If the subdirectory did not contain the target mission, then
				 * discard the subdirectory state.  State is only needed and
				 * maintained for the directories that must be traversed on the
				 * path to the target mission, because only those directories
				 * will be automatically opened in the mission choosing dialog.
				 */
				continue;
			substate->listbox_strings[0] = mission_menu::listbox_go_up;
			create_state.submenu = std::move(substate);
		}
		else if (strcmp(mission_name, mission_name_to_select))
			continue;
		create_state.initial_selection = idx;
		mission_name_to_select = nullptr;
	}
	return p;
}

}

namespace dsx {

void select_mission(const mission_filter_mode mission_filter, const menu_title message, window_event_result (*when_selected)(void))
{
	auto &&mission_list = build_mission_list(mission_filter);

	if (mission_list.empty())
		return;
    if (mission_list.size() <= 1)
	{
        load_mission(&mission_list.front());
		(*when_selected)();
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
			auto submm = window_create<subdirectory_mission_menu>(substate_mission_list.directory, substate_mission_list.directory.size() + mission_subdirectory_extra_strings, std::move(substate->listbox_strings), substate_mission_list.path.c_str(), when_selected, parent_mission_menu, 0, grd_curscreen->sc_canvas);
			parent_mission_menu = submm;
		}
    }
}

#if DXX_USE_EDITOR
namespace {
static int write_mission(void)
{
	auto &msn_extension =
#if defined(DXX_BUILD_DESCENT_II)
	(Current_mission->descent_version != Mission::descent_version_type::descent1) ? MISSION_EXTENSION_DESCENT_II :
#endif
	MISSION_EXTENSION_DESCENT_I;
	std::array<char, PATH_MAX> mission_filename;
	snprintf(mission_filename.data(), mission_filename.size(), "%s%s", Current_mission->path.c_str(), msn_extension);
	
	auto &&mfile = PHYSFSX_openWriteBuffered(mission_filename.data()).first;
	if (!mfile)
	{
		PHYSFS_mkdir(MISSION_DIR);	//try making directory - in *write* path
		mfile = PHYSFSX_openWriteBuffered(mission_filename.data()).first;
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

	PHYSFSX_printf(mfile, "type = %s\n", Current_mission->anarchy_only_flag == Mission::anarchy_only_level::only_anarchy_games ? "anarchy" : "normal");

	if (Current_mission->briefing_text_filename.front())
		PHYSFSX_printf(mfile, "briefing = %s\n", static_cast<const char *>(Current_mission->briefing_text_filename));

	if (Current_mission->ending_text_filename.front())
		PHYSFSX_printf(mfile, "ending = %s\n", static_cast<const char *>(Current_mission->ending_text_filename));

	PHYSFSX_printf(mfile, "num_levels = %i\n", Current_mission->last_level);

	for (const char *const i : unchecked_partial_range(Current_mission->level_names.get(), Current_mission->last_level))
		PHYSFSX_printf(mfile, "%s\n", i);

	if (const auto n_secret_levels = Current_mission->n_secret_levels)
	{
		PHYSFSX_printf(mfile, "num_secrets = %i\n", n_secret_levels);
		for (const auto &&[name, table_cell] : zip(unchecked_partial_range(Current_mission->secret_level_names.get(), n_secret_levels), unchecked_partial_range(Current_mission->secret_level_table.get(), n_secret_levels)))
			PHYSFSX_printf(mfile, "%s,%i\n", static_cast<const char *>(name), table_cell);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (Current_mission->alternate_ham_file)
		PHYSFSX_printf(mfile, "ham = %s\n", static_cast<const char *>(*Current_mission->alternate_ham_file.get()));
#endif

	return 1;
}
}

void create_new_mission(void)
{
	Current_mission = std::make_unique<Mission>(Mission_path(MISSION_DIR "new_miss", sizeof(MISSION_DIR) - 1));		// limited to eight characters because of savegame format
	Current_mission->mission_name.copy_if("Untitled");
	Current_mission->builtin_hogsize = descent_hog_size::None;
	Current_mission->anarchy_only_flag = Mission::anarchy_only_level::allow_any_game;
	
	Current_mission->level_names = std::make_unique<d_fname[]>(1);
	if (!Current_mission->level_names)
	{
		Current_mission.reset();
		return;
	}

	Current_mission->level_names[0] = "GAMESAVE.LVL";
	Current_mission->last_level = 1;
	Current_mission->n_secret_levels = 0;
	Current_mission->last_secret_level = 0;
	Current_mission->briefing_text_filename = {};
	Current_mission->ending_text_filename = {};
	Current_mission->secret_level_table.reset();
	Current_mission->secret_level_names.reset();

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
