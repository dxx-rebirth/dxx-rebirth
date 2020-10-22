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

__attribute_warn_unused_result
arch_atexit arch_init();
}
#endif
namespace dcx {

class window;

template <typename T>
using window_subfunction = window_event_result (*)(window *menu,const d_event &event, T *userdata);

template <typename T>
using window_subclass_subfunction = window_event_result (*)(T *menu,const d_event &event, void*);
	
class unused_window_userdata_t;

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
int window_is_modal(window &wind);

#define WINDOW_SEND_EVENT(w)	((WINDOW_SEND_EVENT)(*w, event, __FILE__, __LINE__))

}

#endif
