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
 * Header for mission.h
 *
 */

#ifndef _MISSION_H
#define _MISSION_H

#include <memory>
#include <string>
#include "pstypes.h"
#include "inferno.h"
#include "dxxsconf.h"
#include "dsx-ns.h"

#include "ntstring.h"

#define MAX_MISSIONS                    5000 // ZICO - changed from 300 to get more levels in list
// KREATOR - increased from 30 (limited by Demo and Multiplayer code)
constexpr std::integral_constant<uint8_t, 127> MAX_LEVELS_PER_MISSION{};
constexpr std::integral_constant<uint8_t, 127> MAX_SECRET_LEVELS_PER_MISSION{};	// KREATOR - increased from 6 (limited by Demo and Multiplayer code)
#define MISSION_NAME_LEN                25

#if defined(DXX_BUILD_DESCENT_I)
#define D1_MISSION_FILENAME             ""
#elif defined(DXX_BUILD_DESCENT_II)
#define D1_MISSION_FILENAME             "descent"
#endif
#define D1_MISSION_NAME                 "Descent: First Strike"
#define D1_OEM_MISSION_NAME             "Destination Saturn"
#define D1_SHAREWARE_MISSION_NAME       "Descent Demo"

#ifdef dsx
namespace dsx {

enum class descent_hog_size : int
{
	None = 0,
	pc_retail_v15 = 6856701, // v1.4 - 1.5
	pc_retail_v15_alt1 = 6856183, // v1.4 - 1.5 - different patch-way
	pc_retail_v10 = 7261423, // v1.0
	mac_retail = 7456179,
	pc_oem_v14 = 4492107, // v1.4a
	pc_oem_v10 = 4494862, // v1.0
	pc_shareware_v14 = 2339773, // v1.4
	pc_shareware_v10 = 2365676, // v1.0 - 1.2
	mac_shareware = 3370339,
#if defined(DXX_BUILD_DESCENT_II)
#define SHAREWARE_MISSION_FILENAME  "d2demo"
#define SHAREWARE_MISSION_NAME      "Descent 2 Demo"
	d2_shareware = 2292566, // v1.0 (d2demo.hog)
	d2_mac_shareware = 4292746,
	d2_oem_v11 = 6132957, // v1.1
	d2_full = 7595079, // v1.1 - 1.2
	d2_full_v10 = 7107354, // v1.0
	d2_mac_full = 7110007, // v1.1 - 1.2

#define OEM_MISSION_FILENAME        "d2"
#define OEM_MISSION_NAME            "D2 Destination:Quartzon"

#define FULL_MISSION_FILENAME       "d2"
#endif
};

}
#endif

#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif

//where the missions go
#define MISSION_DIR "missions/"

constexpr std::integral_constant<std::size_t, 128> DXX_MAX_MISSION_PATH_LENGTH{};

/* Path and filename must be kept in sync. */
class Mission_path
{
public:
	Mission_path(const Mission_path &m) :
		path{m.path},
		filename{std::next(path.cbegin(), std::distance(m.path.cbegin(), m.filename))}
	{
	}
	Mission_path &operator=(const Mission_path &m)
	{
		path = m.path;
		filename = std::next(path.begin(), std::distance(m.path.cbegin(), m.filename));
		return *this;
	}
	Mission_path(std::string_view p, const std::ptrdiff_t offset) :
		path{p},
		filename{std::next(path.cbegin(), {offset})}
	{
	}
	Mission_path(Mission_path &&m) :
		Mission_path{std::move(m).path, std::distance(m.path.cbegin(), m.filename)}
	{
	}
	Mission_path &operator=(Mission_path &&rhs)
	{
		const auto offset{std::distance(rhs.path.cbegin(), rhs.filename)};
		path = std::move(rhs.path);
		filename = std::next(path.begin(), {offset});
		return *this;
	}
	/* Must be in this order for move constructor to work properly */
	std::string path;				// relative file path
	std::string::const_iterator filename;          // filename without extension
	enum class descent_version_type : uint8_t
	{
#if defined(DXX_BUILD_DESCENT_II)
		/* These values are written to the binary savegame as part of
		 * the mission name.  If the values are reordered or renumbered,
		 * old savegames will be unable to find the matching mission
		 * file.
		 */
		descent2a,	// !name
		descent2z,	// zname
		descent2x,	// xname
		descent2,
#endif
		descent1,
	};
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
namespace dsx {

struct Mission : Mission_path
{
	enum class anarchy_only_level : bool
	{
		allow_any_game,
		only_anarchy_games,
	};
	std::unique_ptr<ubyte[]>	secret_level_table; // originating level no for each secret level 
	// arrays of names of the level files
	std::unique_ptr<d_fname[]>	level_names;
	std::unique_ptr<d_fname[]>	secret_level_names;
	descent_hog_size builtin_hogsize;    // the size of the hogfile for a builtin mission, and 0 for an add-on mission
	ntstring<MISSION_NAME_LEN> mission_name;
	d_fname	briefing_text_filename{}; // name of briefing file
	d_fname	ending_text_filename{}; // name of ending file
	anarchy_only_level anarchy_only_flag;  // if true, mission is only for anarchy
	uint8_t	last_level{};
	int8_t	last_secret_level{};
	uint8_t	n_secret_levels{};
#if defined(DXX_BUILD_DESCENT_II)
	descent_version_type descent_version;	// descent 1 or descent 2?
	std::unique_ptr<d_fname> alternate_ham_file;
	std::unique_ptr<LoadedMovieWithResolution> extra_robot_movie;
#endif
	Mission(Mission &&) = default;
	Mission &operator=(Mission &&) = default;
	Mission(const Mission &) = delete;
	Mission &operator=(const Mission &) = delete;
	explicit Mission(const Mission_path &m) :
		Mission_path{m}
	{
	}
	explicit Mission(Mission_path &&m) :
		Mission_path{std::move(m)}
	{
	}
	~Mission();
};

typedef std::unique_ptr<Mission> Mission_ptr;
extern Mission_ptr Current_mission; // current mission

#if defined(DXX_BUILD_DESCENT_II)
/* Wrap in parentheses to avoid precedence problems.  Put constant on
 * the left to silence clang's overzealous -Wparentheses-equality messages.
 */
#define is_SHAREWARE (descent_hog_size::d2_shareware == Current_mission->builtin_hogsize)
#define is_MAC_SHARE (descent_hog_size::d2_mac_shareware == Current_mission->builtin_hogsize)
#define is_D2_OEM (descent_hog_size::d2_oem_v11 == Current_mission->builtin_hogsize)

#define EMULATING_D1		(Mission::descent_version_type::descent1 == Current_mission->descent_version)
#endif
#define PLAYING_BUILTIN_MISSION	(Current_mission->builtin_hogsize != descent_hog_size::None)
#define ANARCHY_ONLY_MISSION	(Mission::anarchy_only_level::only_anarchy_games == Current_mission->anarchy_only_flag)

}
#endif

//values for d1 built-in mission
#define BIMD1_ENDING_FILE_OEM		"endsat.txb"
#define BIMD1_ENDING_FILE_SHARE		"ending.txb"

#ifdef dsx

namespace dcx {

enum class mission_name_type
{
	basename,
	pathname,
	guess,
};

extern char descent_hog_basename[12];

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
//values for d2 built-in mission
#define BIMD2_ENDING_FILE_OEM		"end2oem.txb"
#define BIMD2_ENDING_FILE_SHARE		"ending2.txb"

int load_mission_ham();
void bm_read_extra_robots(const char *fname, Mission::descent_version_type type);
#endif

struct mission_entry_predicate
{
	/* May be a basename or may be a path relative to the root of the
	 * PHYSFS virtual filesystem, depending on what the caller provides.
	 *
	 * In both cases, the file extension is omitted.
	 */
	const char *filesystem_name;
#if defined(DXX_BUILD_DESCENT_II)
	bool check_version;
	Mission::descent_version_type descent_version;
#endif
	mission_entry_predicate with_filesystem_name(const char *fsname) const
	{
		mission_entry_predicate m = *this;
		m.filesystem_name = fsname;
		return m;
	}
};

//loads the named mission if it exists.
//Returns nullptr if mission loaded ok, else error string.
const char *load_mission_by_name (mission_entry_predicate mission_name, mission_name_type);

#if DXX_USE_EDITOR
void create_new_mission(void);
#endif

extern char descent2_hog_basename[13];
extern char d2demo_hog_basename[11];

}

#endif

#endif
