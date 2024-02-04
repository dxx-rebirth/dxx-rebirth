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
 * Interface to piggy functions.
 *
 */

#pragma once

#include "dsx-ns.h"
#include "backports-ranges.h"
#include "fwd-inferno.h"
#include "fwd-piggy.h"
#include "fwd-robot.h"
#include "gr.h"
#include "physfsx.h"

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct alias
{
	std::array<char, FILENAME_LEN> alias_name;
	std::array<char, FILENAME_LEN> file_name;
};
}
#endif

// an index into the bitmap collection of the piggy file
enum class bitmap_index : uint16_t
{
	None = UINT16_MAX
};

namespace dcx {
struct BitmapFile
{
	std::array<char, 13> name;
};

void bitmap_index_read_n(NamedPHYSFS_File fp, std::ranges::subrange<bitmap_index *> r);
}

#ifdef dsx
namespace dsx {

#  define  PIGGY_PAGE_IN(bmp) _piggy_page_in(GameBitmaps, bmp)
static inline void _piggy_page_in(GameBitmaps_array &GameBitmaps, bitmap_index bmp)
{
	if (GameBitmaps[bmp].get_flag_mask(BM_FLAG_PAGED_OUT))
        piggy_bitmap_page_in(GameBitmaps, bmp);
}

#if defined(DXX_BUILD_DESCENT_I)
enum class properties_init_result : int8_t
{
	skip_gamedata_read_tbl,
	use_gamedata_read_tbl,
	shareware,
};
#elif defined(DXX_BUILD_DESCENT_II)
enum class properties_init_result : bool
{
	failure,
	success,
};
#endif

properties_init_result properties_init(d_level_shared_robot_info_state &LevelSharedRobotInfoState);

#if defined(DXX_BUILD_DESCENT_II)
enum class pig_hamfile_version : uint8_t
{
	_0,
	_3 = 3,
};

int read_hamfile(d_level_shared_robot_info_state &LevelSharedRobotInfoState);
#endif
}

#endif
