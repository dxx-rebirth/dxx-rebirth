/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

#ifdef __cplusplus
namespace dcx {

// Display a warning in a messagebox
extern void msgbox_warning(const char *message);

// Display an error in a messagebox
extern void msgbox_error(const char *message);

}
#endif
