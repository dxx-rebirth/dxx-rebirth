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
 * Header file for Inferno.  Should be included in all source files.
 *
 */

#pragma once

#include "dxxsconf.h"
#include "fwd-event.h"
#include "dsx-ns.h"
#include "ntstring.h"
#include "player-callsign.h"

namespace dcx {

#if defined(__APPLE__) || defined(macintosh)
#define KEY_MAC(x) x
#else
// do not use MAC, it will break MSVC compilation somewhere in rpcdce.h
#define KEY_MAC(x)
#endif

/**
 **	Constants
 **/

//	How close two points must be in all dimensions to be considered the same point.
#define	FIX_EPSILON	10

// the maximum length of a filename
constexpr std::integral_constant<std::size_t, 13> FILENAME_LEN{};

// Default event handler for everything except the editor
window_event_result standard_handler(const d_event &event);

// a filename, useful for declaring arrays of filenames
struct d_fname : ntstring<FILENAME_LEN - 1>
{
	d_fname() = default;
	d_fname(const d_fname &) = default;
	d_fname &operator=(const d_fname &) = default;
	template <std::size_t N>
		void operator=(char (&i)[N]) const = delete;
	template <std::size_t N>
		void operator=(const char (&i)[N])
		{
			copy_if(i);
		}
};

struct d_interface_unique_state
{
	callsign_t PilotName;
#if DXX_HAVE_POISON
	d_interface_unique_state();
#endif
	void update_window_title();
};

extern d_interface_unique_state InterfaceUniqueState;

struct d_game_view_unique_state
{
	uint8_t Death_sequence_aborted;
};

extern d_game_view_unique_state GameViewUniqueState;

}

#ifdef dsx
namespace dsx {

/**
 **	Global variables
 **/

extern int Quitting;
extern int Screen_mode;			// editor screen or game screen?
#ifdef DXX_BUILD_DESCENT_I
extern int MacHog;
#endif

}
#endif
