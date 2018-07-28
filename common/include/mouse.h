/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

#include "pstypes.h"
#include "maths.h"

#ifdef __cplusplus
#include <SDL_version.h>
#include <cassert>
#include "fwd-window.h"
#include "event.h"

struct SDL_MouseButtonEvent;
struct SDL_MouseMotionEvent;

namespace dcx {

#define MOUSE_MAX_BUTTONS       16
#define Z_SENSITIVITY		100
#define MBTN_LEFT		0
#define MBTN_RIGHT		1
#define MBTN_MIDDLE		2
#define MBTN_Z_UP		3
#define MBTN_Z_DOWN		4
#define MBTN_PITCH_BACKWARD	5
#define MBTN_PITCH_FORWARD	6
#define MBTN_BANK_LEFT		7
#define MBTN_BANK_RIGHT		8
#define MBTN_HEAD_LEFT		9
#define MBTN_HEAD_RIGHT		10
#define MBTN_11			11
#define MBTN_12			12
#define MBTN_13			13
#define MBTN_14			14
#define MBTN_15			15
#define MBTN_16			16
#define MOUSE_LBTN		1
#define MOUSE_RBTN		2
#define MOUSE_MBTN		4

extern void mouse_flush();	// clears all mice events...
extern void mouse_init(void);
extern void mouse_close(void);
extern void mouse_get_pos( int *x, int *y, int *z );
window_event_result mouse_in_window(class window *wind);
extern void mouse_get_delta( int *dx, int *dy, int *dz );
void mouse_enable_cursor();
void mouse_disable_cursor();
window_event_result mouse_button_handler(struct SDL_MouseButtonEvent *mbe);
window_event_result mouse_motion_handler(struct SDL_MouseMotionEvent *mme);
void mouse_cursor_autohide();

class d_event_mousebutton : public d_event
{
public:
	d_event_mousebutton(event_type type, unsigned b);
	const unsigned button;
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

static inline int event_mouse_get_button(const d_event &event)
{
	auto &e = static_cast<const d_event_mousebutton &>(event);
	assert(e.type == EVENT_MOUSE_BUTTON_DOWN || e.type == EVENT_MOUSE_BUTTON_UP);
	return e.button;
}

static inline void event_mouse_get_delta(const d_event &event, int *dx, int *dy, int *dz)
{
	auto &e = static_cast<const d_event_mouse_moved &>(event);
	assert(e.type == EVENT_MOUSE_MOVED);
	*dx = e.dx;
	*dy = e.dy;
	*dz = e.dz;
}

}

#endif
