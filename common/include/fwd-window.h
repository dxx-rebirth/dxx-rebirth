/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#ifdef __cplusplus
#include "fwd-event.h"
#include "fwd-gr.h"

#ifdef dsx
namespace dsx {

struct arch_atexit
{
	~arch_atexit();
};

[[nodiscard]]
arch_atexit arch_init();
}
#endif
namespace dcx {

class window;

int window_close(window *wind);
window *window_get_front();
window *window_get_first();
window *window_get_next(window &wind);
window *window_get_prev(window &wind);
void window_select(window &wind);
window *window_set_visible(window &wind, int visible);
#if !DXX_USE_OGL
void window_update_canvases();
#endif

#define WINDOW_SEND_EVENT(w)	((WINDOW_SEND_EVENT)(*w, event, __FILE__, __LINE__))

}

#endif
