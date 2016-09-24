/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
void arch_init();
}
#endif
namespace dcx {

struct window;
enum class window_event_result : uint8_t;

template <typename T>
using window_subfunction = window_event_result (*)(window *menu,const d_event &event, T *userdata);

class unused_window_userdata_t;

/* No declaration for embed_window_pointer_t or ignore_window_pointer_t
 * since every user needs the full definition.
 */

window *window_create(grs_canvas *src, int x, int y, int w, int h, window_subfunction<void> event_callback, void *userdata, const void *createdata);

int window_close(window *wind);
int window_exists(window *wind);
window *window_get_front();
window *window_get_first();
window *window_get_next(window &wind);
window *window_get_prev(window &wind);
void window_select(window &wind);
window *window_set_visible(window &wind, int visible);
int window_is_visible(window &wind);
grs_canvas &window_get_canvas(window &wind);
#if !DXX_USE_OGL
void window_update_canvases();
#endif
window_event_result window_send_event(window &wind,const d_event &event);
void window_set_modal(window &wind, int modal);
int window_is_modal(window &wind);

#define WINDOW_SEND_EVENT(w, e)	(event.type = e, (WINDOW_SEND_EVENT)(*w, event, __FILE__, __LINE__, #e))

}

#endif
