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
 * Prototypes for auto-map stuff.
 *
 */

#pragma once

#include "pstypes.h"

#ifdef __cplusplus
#include <cstddef>
#include "fwd-segment.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include <array>

namespace dcx {
extern int Automap_active;
}
#ifdef dsx
namespace dsx {
void do_automap();
void automap_clear_visited(d_level_unique_automap_state &LevelUniqueAutomapState);
}
#endif

#if defined(DXX_BUILD_DESCENT_II)
#include "object.h"
#include "ntstring.h"
#include "d_array.h"
#include "d_range.h"
#include "fwd-event.h"
#include "segment.h"

namespace dsx {
void DropBuddyMarker(object &objp);
void InitMarkerInput();
window_event_result MarkerInputMessage(int key, control_info &Controls);

constexpr std::integral_constant<std::size_t, 16> NUM_MARKERS{};
constexpr std::integral_constant<std::size_t, 40> MARKER_MESSAGE_LEN{};
struct marker_message_text_t : ntstring<MARKER_MESSAGE_LEN - 1>
{
	constexpr marker_message_text_t() :
		ntstring()
	{
	}
};

struct d_marker_object_numbers
{
	enumerated_array<imobjidx_t, NUM_MARKERS, game_marker_index> imobjidx = {init_object_number_array<imobjidx_t>(std::make_index_sequence<NUM_MARKERS>())};
};

struct d_marker_state : d_marker_object_numbers
{
	player_marker_index MarkerBeingDefined = player_marker_index::None;
	game_marker_index HighlightMarker = game_marker_index::None;
	player_marker_index LastMarkerDropped = player_marker_index::None;
	enumerated_array<marker_message_text_t, NUM_MARKERS, game_marker_index> message;
	bool DefiningMarkerMessage() const
	{
		return MarkerBeingDefined != player_marker_index::None;
	}
	static unsigned get_markers_per_player(unsigned game_mode, unsigned netgame_max_players);
};

game_marker_index convert_player_marker_index_to_game_marker_index(unsigned game_mode, unsigned max_numplayers, unsigned player_num, player_marker_index base_marker_num);
xrange<player_marker_index> get_player_marker_range(unsigned maxdrop);
extern marker_message_text_t Marker_input;
extern d_marker_state MarkerState;
}
#endif

#endif
