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
 * Info on canvases, screens, etc.
 *
 */

#pragma once

#include "fwd-gr.h"

// What graphics modes the game & editor open

// for Screen_mode variable
#define SCREEN_MENU		0	// viewing the menu screen
#define SCREEN_GAME		1	// viewing the menu screen
#define SCREEN_EDITOR		2	// viewing the editor screen
#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 2
#define SCREEN_MOVIE		3	// viewing a movie
#endif

//from game.c
namespace dsx {
int set_screen_mode(int sm);	// True = editor screen
}
#endif

namespace dcx {
extern const std::array<int8_t, 64> fades;

//About the screen
extern grs_subcanvas		Screen_3d_window;		// The rectangle for rendering the mine to
}
