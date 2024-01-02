/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <cstdint>
#include "dxxsconf.h"

namespace dcx {

struct d_event;
struct d_change_event;
struct d_select_event;

enum class event_type : uint8_t;
enum class window_event_result : uint8_t;

// Sends input events to event handlers
window_event_result event_poll();

// Set and call the default event handler
window_event_result call_default_handler(const d_event &event);

// Send an event to the front window as first priority, then to the windows behind if it's not modal (editor), then the default handler
window_event_result event_send(const d_event &event);

// Sends input, idle and draw events to event handlers
window_event_result event_process();

void event_enable_focus();
void event_disable_focus();
static inline void event_toggle_focus(int activate_focus)
{
	if (activate_focus)
		event_enable_focus();
	else
		event_disable_focus();
}

#if DXX_USE_EDITOR
// See how long we were idle for
void event_reset_idle_seconds();
#endif

}
