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
 * Prototypes for reading controls
 *
 */


#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "key.h"
#include "joy.h"
#include "mouse.h"
#include "dxxsconf.h"

#ifdef __cplusplus
#include <vector>
#include "compiler-array.h"

struct d_event;

struct control_info {
	template <typename T>
	struct ramp_controls_t
	{
		T	key_pitch_forward,
			key_pitch_backward,
			key_heading_left,
			key_heading_right,
			key_slide_left,
			key_slide_right,
			key_slide_up,
			key_slide_down,
			key_bank_left,
			key_bank_right;
	};
	struct state_controls_t : public ramp_controls_t<ubyte>
	{
		ubyte btn_slide_left, btn_slide_right,
			btn_slide_up, btn_slide_down,
			btn_bank_left, btn_bank_right,
			slide_on, bank_on,
			accelerate, reverse,
			cruise_plus, cruise_minus, cruise_off,
			rear_view,
			fire_primary, fire_secondary, fire_flare, drop_bomb,
			automap,
			cycle_primary, cycle_secondary, select_weapon;
#if defined(DXX_BUILD_DESCENT_II)
		ubyte toggle_bomb,
			afterburner, headlight, energy_to_shield;
#endif
	};
	ramp_controls_t<float> down_time; // to scale movement depending on how long the key is pressed
	fix pitch_time, vertical_thrust_time, heading_time, sideways_thrust_time, bank_time, forward_thrust_time;
	state_controls_t state; // to scale movement for keys only we need them to be separate from joystick/mouse buttons
	fix joy_axis[JOY_MAX_AXES], raw_joy_axis[JOY_MAX_AXES], mouse_axis[3], raw_mouse_axis[3];
};

#define CONTROL_USING_JOYSTICK	1
#define CONTROL_USING_MOUSE		2
#define MOUSEFS_DELTA_RANGE 512
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_DXX_REBIRTH_CONTROLS    30
#define MAX_CONTROLS 50
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_DXX_REBIRTH_CONTROLS    30
#define MAX_CONTROLS        60		// there are actually 48, so this leaves room for more
#endif

extern control_info Controls;
extern void kconfig_read_controls(const d_event &event, int automap_flag);
extern void kconfig(int n, const char *title);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern const array<array<ubyte, MAX_CONTROLS>, 3> DefaultKeySettings;
extern const array<ubyte, MAX_DXX_REBIRTH_CONTROLS> DefaultKeySettingsRebirth;
#endif

extern void kc_set_controls();

//set the cruise speed to zero
extern void reset_cruise(void);

extern fix Cruise_speed;


template <std::size_t N>
struct joystick_text_length
{
	enum { value = ((N >= 10) ? (joystick_text_length<N / 10>::value + 1) : 1) };
};
template <>
struct joystick_text_length<0>
{
	enum { value = 1 };
};

template <std::size_t N>
class joystick_text_t
{
	typedef std::vector<array<char, N> > vector_type;
	typedef typename vector_type::size_type size_type;
	typedef typename vector_type::reference reference;
	vector_type text;
public:
	void clear() { text.clear(); }
	size_type size() const { return text.size(); }
	void resize(size_type s) { text.resize(s); }
	reference operator[](size_type s) { return text.at(s); }
};

class joyaxis_text_t : public joystick_text_t<sizeof("J A") + joystick_text_length<MAX_JOYSTICKS>::value + joystick_text_length<MAX_AXES_PER_JOYSTICK>::value>
{
};

class joybutton_text_t : public joystick_text_t<sizeof("J H ") + joystick_text_length<MAX_JOYSTICKS>::value + joystick_text_length<MAX_BUTTONS_PER_JOYSTICK>::value>
{
};

extern joybutton_text_t joybutton_text;
extern joyaxis_text_t joyaxis_text;
#endif

#endif /* _KCONFIG_H */
