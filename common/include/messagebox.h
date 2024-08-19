/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *  messagebox.h
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#pragma once

#include <span>
#include <SDL.h>

namespace dcx {

#if defined(WIN32) || defined(__APPLE__) || defined(__MACH__) || SDL_MAJOR_VERSION == 2
// Display a warning in a messagebox
void msgbox_warning(std::span<const char> message);

// Display an error in a messagebox
extern void msgbox_error(const char *message);
#else
#define msgbox_error(M)	(static_cast<void>(M))
#endif

}
