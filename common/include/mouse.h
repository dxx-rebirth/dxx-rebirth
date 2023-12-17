/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL mouse driver header
 *
 */

#pragma once

#include <array>
#include <tuple>
#include "pstypes.h"
#include "maths.h"
#include <SDL_version.h>
#include <cassert>
#include "event.h"

struct SDL_MouseButtonEvent;
struct SDL_MouseMotionEvent;

namespace dcx {

#define MOUSE_MAX_BUTTONS       17
#define Z_SENSITIVITY		100

enum class mbtn : uint8_t
{
	left = 0,
	right = 1,
	middle = 2,
	z_up = 3,
	z_down = 4,
	pitch_backward = 5,
	pitch_forward = 6,
	bank_left = 7,
	bank_right = 8,
	head_left = 9,
	head_right = 10,
	_11 = 11,
	_12 = 12,
	_13 = 13,
	_14 = 14,
	_15 = 15,
	_16 = 16,
};

#define MOUSE_LBTN		1
#define MOUSE_RBTN		2
#define MOUSE_MBTN		4

extern void mouse_flush();	// clears all mice events...
extern void mouse_init(void);
extern void mouse_close(void);
[[nodiscard]]
std::tuple<int /* x */, int /* y */, int /* z */> mouse_get_pos();
window_event_result mouse_in_window(class window *wind);
#if 0
[[nodiscard]]
std::tuple<int /* x */, int /* y */, int /* z */> mouse_get_delta();
#endif
void mouse_enable_cursor();
void mouse_disable_cursor();
window_event_result mouse_button_handler(const SDL_MouseButtonEvent *mbe);
window_event_result mouse_motion_handler(const SDL_MouseMotionEvent *mme);
void mouse_cursor_autohide();

class d_event_mousebutton : public d_event
{
public:
	d_event_mousebutton(event_type type, mbtn b);
	const mbtn button;
};

class d_event_mouse_moved : public d_event
{
public:
#if SDL_MAJOR_VERSION == 1
#define SDL_MOUSE_MOVE_INT_TYPE	Sint16
#elif SDL_MAJOR_VERSION == 2
#define SDL_MOUSE_MOVE_INT_TYPE	Sint32
#endif
	const SDL_MOUSE_MOVE_INT_TYPE dx, dy;
	const int16_t dz;
	constexpr d_event_mouse_moved(const event_type t, const SDL_MOUSE_MOVE_INT_TYPE x, const SDL_MOUSE_MOVE_INT_TYPE y, const int16_t z) :
		d_event(t), dx(x), dy(y), dz(z)
	{
	}
#undef SDL_MOUSE_MOVE_INT_TYPE
};

static inline mbtn event_mouse_get_button(const d_event &event)
{
	auto &e = static_cast<const d_event_mousebutton &>(event);
	assert(e.type == event_type::mouse_button_down || e.type == event_type::mouse_button_up);
	return e.button;
}

[[nodiscard]]
static inline std::array<int, 3> event_mouse_get_delta(const d_event &event)
{
	auto &e = static_cast<const d_event_mouse_moved &>(event);
	assert(e.type == event_type::mouse_moved);
	return {{
		e.dx,
		e.dy,
		e.dz,
	}};
}

}
