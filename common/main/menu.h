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
 * Menu prototypes and variables
 *
 */

#ifndef _MENU_H
#define _MENU_H

#include <array>
#include <chrono>
#include "dsx-ns.h"
#include "strutil.h"

namespace dcx {

template <typename Rep>
using human_readable_mmss_time = std::array<char, number_to_text_length<std::numeric_limits<Rep>::max() / 60> + number_to_text_length<60> + sizeof("ms")>;

[[nodiscard]]
human_readable_mmss_time<uint16_t> build_human_readable_time(const std::chrono::duration<uint16_t, std::chrono::seconds::period> duration);

template <typename Rep, std::size_t S>
void parse_human_readable_time(std::chrono::duration<Rep, std::chrono::seconds::period> &duration, const std::array<char, S> &buf);

}

extern int hide_menus(void);
extern void show_menus(void);

#ifdef dsx
namespace dsx {
// returns number of item chosen
extern int DoMenu();
}
#endif
extern void do_options_menu();

#if defined(DXX_BUILD_DESCENT_I)
#define Menu_pcx_name (((SWIDTH >= 640 && SHEIGHT >= 480) && PHYSFS_exists("menuh.pcx")) ? "menuh.pcx" : "menu.pcx")
#define STARS_BACKGROUND (((SWIDTH >= 640 && SHEIGHT >= 480) && PHYSFS_exists("starsb.pcx")) ? "starsb.pcx" : "stars.pcx")
#elif defined(DXX_BUILD_DESCENT_II)
#define MENU_PCX_MAC_SHARE ("menub.pcx")
#define MENU_PCX_SHAREWARE ("menud.pcx")
#define MENU_PCX_OEM (HIRESMODE?"menuob.pcx":"menuo.pcx")
#define MENU_PCX_FULL (HIRESMODE?"menub.pcx":"menu.pcx")

// name of background bitmap
#define Menu_pcx_name (PHYSFS_exists(MENU_PCX_FULL) ? MENU_PCX_FULL : (PHYSFS_exists(MENU_PCX_OEM) ? MENU_PCX_OEM : PHYSFS_exists(MENU_PCX_SHAREWARE) ? MENU_PCX_SHAREWARE : MENU_PCX_MAC_SHARE))
#define STARS_BACKGROUND ((HIRESMODE && PHYSFS_exists("starsb.pcx")) ? "starsb.pcx" : PHYSFS_exists("stars.pcx") ? "stars.pcx" : "starsb.pcx")
#endif

#endif /* _MENU_H */
