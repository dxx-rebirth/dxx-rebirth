/*
 *
 * SDL mouse driver header
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "pstypes.h"
#include "fix.h"

struct d_event;
struct window;

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
extern int event_mouse_get_button(struct d_event *event);
extern void mouse_get_pos( int *x, int *y, int *z );
extern int mouse_in_window(struct window *wind);
extern void mouse_get_delta( int *dx, int *dy, int *dz );
extern void event_mouse_get_delta(struct d_event *event, int *dx, int *dy, int *dz);
extern int mouse_get_btns();
extern void mouse_toggle_cursor(int activate);

#endif
