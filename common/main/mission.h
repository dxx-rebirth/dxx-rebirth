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
#include "fwd-window.h"

#ifdef __cplusplus
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
#define D1_MISSION_HOGSIZE              6856701 // v1.4 - 1.5
#define D1_MISSION_HOGSIZE2             6856183 // v1.4 - 1.5 - different patch-way
#define D1_10_MISSION_HOGSIZE           7261423 // v1.0
#define D1_MAC_MISSION_HOGSIZE          7456179
#define D1_OEM_MISSION_NAME             "Destination Saturn"
#define D1_OEM_MISSION_HOGSIZE          4492107 // v1.4a
#define D1_OEM_10_MISSION_HOGSIZE       4494862 // v1.0
#define D1_SHAREWARE_MISSION_NAME       "Descent Demo"
#define D1_SHAREWARE_MISSION_HOGSIZE    2339773 // v1.4
#define D1_SHAREWARE_10_MISSION_HOGSIZE 2365676 // v1.0 - 1.2
#define D1_MAC_SHARE_MISSION_HOGSIZE    3370339

#if defined(DXX_BUILD_DESCENT_II)
#define SHAREWARE_MISSION_FILENAME  "d2demo"
#define SHAREWARE_MISSION_NAME      "Descent 2 Demo"
#define SHAREWARE_MISSION_HOGSIZE   2292566 // v1.0 (d2demo.hog)
#define MAC_SHARE_MISSION_HOGSIZE   4292746

#define OEM_MISSION_FILENAME        "d2"
#define OEM_MISSION_NAME            "D2 Destination:Quartzon"
#define OEM_MISSION_HOGSIZE         6132957 // v1.1

#define FULL_MISSION_FILENAME       "d2"
#define FULL_MISSION_HOGSIZE        7595079 // v1.1 - 1.2
#define FULL_10_MISSION_HOGSIZE     7107354 // v1.0
#define MAC_FULL_MISSION_HOGSIZE    7110007 // v1.1 - 1.2
#endif

//where the missions go
#define MISSION_DIR "missions/"

/* Path and filename must be kept in sync. */
class Mission_path
{
public:
	Mission_path(const Mission_path &m) :
		path(m.path),
		filename(std::next(path.cbegin(), std::distance(m.path.cbegin(), m.filename)))
	{
	}
	Mission_path &operator=(const Mission_path &m)
	{
		path = m.path;
		filename = std::next(path.begin(), std::distance(m.path.cbegin(), m.filename));
		return *this;
	}
	Mission_path(std::string &&p, const std::size_t offset) :
		path(std::move(p)),
		filename(std::next(path.cbegin(), offset))
	{
	}
	Mission_path(Mission_path &&m) :
		Mission_path(std::move(m).path, std::distance(m.path.cbegin(), m.filename))
	{
	}
	Mission_path &operator=(Mission_path &&rhs)
	{
		std::size_t offset = std::distance(rhs.path.cbegin(), rhs.filename);
		path = std::move(rhs.path);
		filename = std::next(path.begin(), offset);
		return *this;
	}
	/* Must be in this order for move constructor to work properly */
	std::string path;				// relative file path
	std::string::const_iterator filename;          // filename without extension
#if defined(DXX_BUILD_DESCENT_II)
	enum class descent_version_type : uint8_t
	{
		descent2a,	// !name
		descent2z,	// zname
		descent2x,	// xname
		descent2,
		descent1,
	};
#endif
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct Mission : Mission_path
{
	std::unique_ptr<ubyte[]>	secret_level_table; // originating level no for each secret level 
	// arrays of names of the level files
	std::unique_ptr<d_fname[]>	level_names;
	std::unique_ptr<d_fname[]>	secret_level_names;
	int     builtin_hogsize;    // the size of the hogfile for a builtin mission, and 0 for an add-on mission
	ntstring<MISSION_NAME_LEN> mission_name;
	d_fname	briefing_text_filename; // name of briefing file
	d_fname	ending_text_filename; // name of ending file
	ubyte   anarchy_only_flag;  // if true, mission is only for anarchy
	ubyte	last_level;
	sbyte	last_secret_level;
	ubyte	n_secret_levels;
#if defined(DXX_BUILD_DESCENT_II)
	descent_version_type descent_version;	// descent 1 or descent 2?
	std::unique_ptr<d_fname> alternate_ham_file;
#endif
	/* Explicitly default move constructor and move operator=
	 *
	 * Without this, gcc (tested gcc-4.9, gcc-5) tries to use
	 * a synthetic operator=(const Mission &) to implement `instance =
	 * {};`, which fails because Mission contains std::unique_ptr, a
	 * movable but noncopyable type.
	 *
	 * With the explicit default, gcc uses operator=(Mission &&), which
	 * works.
	 *
	 * Explicitly delete copy constructor and copy operator= for
	 * thoroughness.
	 */
	Mission(Mission &&) = default;
	Mission &operator=(Mission &&) = default;
	Mission(const Mission &) = delete;
	Mission &operator=(const Mission &) = delete;
	explicit Mission(const Mission_path &m) :
		Mission_path(m)
	{
	}
	explicit Mission(Mission_path &&m) :
		Mission_path(std::move(m))
	{
	}
	~Mission();
};

typedef std::unique_ptr<Mission> Mission_ptr;
extern Mission_ptr Current_mission; // current mission

#define Current_mission_longname	Current_mission->mission_name
#define Current_mission_filename	&*Current_mission->filename
#define Briefing_text_filename		Current_mission->briefing_text_filename
#define Ending_text_filename		Current_mission->ending_text_filename
#define Last_level			Current_mission->last_level
#define Last_secret_level		Current_mission->last_secret_level
#define N_secret_levels			Current_mission->n_secret_levels
#define Secret_level_table		Current_mission->secret_level_table
#define Level_names			Current_mission->level_names
#define Secret_level_names		Current_mission->secret_level_names

#if defined(DXX_BUILD_DESCENT_II)
/* Wrap in parentheses to avoid precedence problems.  Put constant on
 * the left to silence clang's overzealous -Wparentheses-equality messages.
 */
#define is_SHAREWARE (SHAREWARE_MISSION_HOGSIZE == Current_mission->builtin_hogsize)
#define is_MAC_SHARE (MAC_SHARE_MISSION_HOGSIZE == Current_mission->builtin_hogsize)
#define is_D2_OEM (OEM_MISSION_HOGSIZE == Current_mission->builtin_hogsize)

#define EMULATING_D1		(Mission::descent_version_type::descent1 == Current_mission->descent_version)
#endif
#define PLAYING_BUILTIN_MISSION	(Current_mission->builtin_hogsize != 0)
#define ANARCHY_ONLY_MISSION	(1 == Current_mission->anarchy_only_flag)
#endif

//values for d1 built-in mission
#define BIMD1_LAST_LEVEL		27
#define BIMD1_LAST_SECRET_LEVEL		-3
#define BIMD1_ENDING_FILE_OEM		"endsat.txb"
#define BIMD1_ENDING_FILE_SHARE		"ending.txb"

#if defined(DXX_BUILD_DESCENT_II)
//values for d2 built-in mission
#define BIMD2_ENDING_FILE_OEM		"end2oem.txb"
#define BIMD2_ENDING_FILE_SHARE		"ending2.txb"

int load_mission_ham();
void bm_read_extra_robots(const char *fname, Mission::descent_version_type type);
#endif

//loads the named mission if it exists.
//Returns nullptr if mission loaded ok, else error string.
const char *load_mission_by_name (const char *mission_name);

//Handles creating and selecting from the mission list.
//Returns 1 if a mission was loaded.
int select_mission (int anarchy_mode, const char *message, window_event_result (*when_selected)(void));

#if DXX_USE_EDITOR
void create_new_mission(void);
#endif

#endif

#endif
