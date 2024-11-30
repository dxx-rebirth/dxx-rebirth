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
 * Header for titles.c
 *
 */

#pragma once

#include <cstdint>
#include "dxxsconf.h"
#include "dsx-ns.h"

namespace dcx {

struct d_fname;

}

#ifdef DXX_BUILD_DESCENT
namespace dsx {
extern void show_titles(void);
void do_briefing_screens(const d_fname &filename, int level_num);
void do_end_briefing_screens(const d_fname &filename);
}
#endif
extern char * get_briefing_screen( int level_num );
#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 2
void show_loading_screen(uint8_t *title_pal);
extern void show_endgame_briefing(void);
namespace dsx {
extern int intro_played;
}
#endif
#endif
