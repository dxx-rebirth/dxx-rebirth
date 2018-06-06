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
#include "compiler-array.h"

namespace dcx {
extern int Automap_active;
}
#ifdef dsx
namespace dsx {
void do_automap();
extern void automap_clear_visited();
}
#endif
namespace dcx {
extern array<ubyte, MAX_SEGMENTS> Automap_visited;
}

#if defined(DXX_BUILD_DESCENT_II)
#include "fwd-object.h"
#include "ntstring.h"
#include "fwd-window.h"
#include "segment.h"

namespace dsx {
void DropBuddyMarker(vmobjptr_t objp);
void InitMarkerInput();
window_event_result MarkerInputMessage(int key);

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
	array<imobjidx_t, NUM_MARKERS> imobjidx;
	constexpr d_marker_object_numbers();
};

struct d_marker_state : d_marker_object_numbers
{
	uint8_t MarkerBeingDefined = UINT8_MAX;
	uint8_t HighlightMarker = UINT8_MAX;
	uint8_t LastMarkerDropped = 0;
	array<marker_message_text_t, NUM_MARKERS> message;
	bool DefiningMarkerMessage() const
	{
		return MarkerBeingDefined < message.size();
	}
	static unsigned get_biased_marker_num(unsigned game_mode, unsigned player_num, unsigned base_marker_num);
	static unsigned get_markers_per_player(unsigned game_mode);
};

extern marker_message_text_t Marker_input;
extern d_marker_state MarkerState;
}
#endif

#endif
