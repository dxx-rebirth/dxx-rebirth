/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL keyboard input support
 *
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_version.h>
#if SDL_MAJOR_VERSION == 2
#include <SDL_keycode.h>
#endif

#include "event.h"
#include "dxxerror.h"
#include "key.h"
#include "timer.h"
#include "window.h"
#include "console.h"
#include "args.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"
#include "compiler-range_for.h"

namespace dcx {

//-------- Variable accessed by outside functions ---------
static bool keyd_repeat; // 1 = use repeats, 0 no repeats
pressed_keys keyd_pressed;
fix64			keyd_time_when_last_pressed;
array<unsigned char, KEY_BUFFER_SIZE>		unicode_frame_buffer;

const array<key_props, 256> key_properties = {{
{ "",       255,    SDLK_UNKNOWN                 }, // 0
{ "ESC",    255,    SDLK_ESCAPE        },
{ "1",      '1',    SDLK_1             },
{ "2",      '2',    SDLK_2             },
{ "3",      '3',    SDLK_3             },
{ "4",      '4',    SDLK_4             },
{ "5",      '5',    SDLK_5             },
{ "6",      '6',    SDLK_6             },
{ "7",      '7',    SDLK_7             },
{ "8",      '8',    SDLK_8             },
{ "9",      '9',    SDLK_9             }, // 10
{ "0",      '0',    SDLK_0             },
{ "-",      '-',    SDLK_MINUS         },
{ "=",      '=',    SDLK_EQUALS        },
{ "BSPC",   255,    SDLK_BACKSPACE     },
{ "TAB",    255,    SDLK_TAB           },
{ "Q",      'q',    SDLK_q             },
{ "W",      'w',    SDLK_w             },
{ "E",      'e',    SDLK_e             },
{ "R",      'r',    SDLK_r             },
{ "T",      't',    SDLK_t             }, // 20
{ "Y",      'y',    SDLK_y             },
{ "U",      'u',    SDLK_u             },
{ "I",      'i',    SDLK_i             },
{ "O",      'o',    SDLK_o             },
{ "P",      'p',    SDLK_p             },
{ "[",      '[',    SDLK_LEFTBRACKET   },
{ "]",      ']',    SDLK_RIGHTBRACKET  },
{ "ENTER",  255,    SDLK_RETURN        },
{ "LCTRL",  255,    SDLK_LCTRL         },
{ "A",      'a',    SDLK_a             }, // 30
{ "S",      's',    SDLK_s             },
{ "D",      'd',    SDLK_d             },
{ "F",      'f',    SDLK_f             },
{ "G",      'g',    SDLK_g             },
{ "H",      'h',    SDLK_h             },
{ "J",      'j',    SDLK_j             },
{ "K",      'k',    SDLK_k             },
{ "L",      'l',    SDLK_l             },
{ ";",      ';',    SDLK_SEMICOLON     },
{ "'",      '\'',   SDLK_QUOTE         }, // 40
{ "`",      '`',    SDLK_BACKQUOTE     },
{ "LSHFT",  255,    SDLK_LSHIFT        },
{ "\\",     '\\',   SDLK_BACKSLASH     },
{ "Z",      'z',    SDLK_z             },
{ "X",      'x',    SDLK_x             },
{ "C",      'c',    SDLK_c             },
{ "V",      'v',    SDLK_v             },
{ "B",      'b',    SDLK_b             },
{ "N",      'n',    SDLK_n             },
{ "M",      'm',    SDLK_m             }, // 50
{ ",",      ',',    SDLK_COMMA         },
{ ".",      '.',    SDLK_PERIOD        },
{ "/",      '/',    SDLK_SLASH         },
{ "RSHFT",  255,    SDLK_RSHIFT        },
{ "PAD*",   '*',    SDLK_KP_MULTIPLY   },
{ "LALT",   255,    SDLK_LALT          },
{ "SPC",    ' ',    SDLK_SPACE         },
{ "CPSLK",  255,    SDLK_CAPSLOCK      },
{ "F1",     255,    SDLK_F1            },
{ "F2",     255,    SDLK_F2            }, // 60
{ "F3",     255,    SDLK_F3            },
{ "F4",     255,    SDLK_F4            },
{ "F5",     255,    SDLK_F5            },
{ "F6",     255,    SDLK_F6            },
{ "F7",     255,    SDLK_F7            },
{ "F8",     255,    SDLK_F8            },
{ "F9",     255,    SDLK_F9            },
{ "F10",    255,    SDLK_F10           },
#if SDL_MAJOR_VERSION == 1
#define SDLK_NUMLOCKCLEAR	SDLK_NUMLOCK
#define SDLK_SCROLLLOCK	SDLK_SCROLLOCK
#define SDLK_KP_0	SDLK_KP0
#define SDLK_KP_1	SDLK_KP1
#define SDLK_KP_2	SDLK_KP2
#define SDLK_KP_3	SDLK_KP3
#define SDLK_KP_4	SDLK_KP4
#define SDLK_KP_5	SDLK_KP5
#define SDLK_KP_6	SDLK_KP6
#define SDLK_KP_7	SDLK_KP7
#define SDLK_KP_8	SDLK_KP8
#define SDLK_KP_9	SDLK_KP9
#endif
{ "NMLCK",  255,    SDLK_NUMLOCKCLEAR  },
{ "SCLK",   255,    SDLK_SCROLLLOCK    }, // 70
{ "PAD7",   255,    SDLK_KP_7          },
{ "PAD8",   255,    SDLK_KP_8          },
{ "PAD9",   255,    SDLK_KP_9          },
{ "PAD-",   255,    SDLK_KP_MINUS      },
{ "PAD4",   255,    SDLK_KP_4          },
{ "PAD5",   255,    SDLK_KP_5          },
{ "PAD6",   255,    SDLK_KP_6          },
{ "PAD+",   255,    SDLK_KP_PLUS       },
{ "PAD1",   255,    SDLK_KP_1          },
{ "PAD2",   255,    SDLK_KP_2          }, // 80
{ "PAD3",   255,    SDLK_KP_3          },
{ "PAD0",   255,    SDLK_KP_0          },
{ "PAD.",   255,    SDLK_KP_PERIOD     },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "F11",    255,    SDLK_F11           },
{ "F12",    255,    SDLK_F12           },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 }, // 90
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "PAUSE",  255,    SDLK_PAUSE         },
#if SDL_MAJOR_VERSION == 2
#define SDLK_WORLD_0	SDLK_UNKNOWN
#define SDLK_WORLD_1	SDLK_UNKNOWN
#define SDLK_WORLD_2	SDLK_UNKNOWN
#define SDLK_WORLD_3	SDLK_UNKNOWN
#define SDLK_WORLD_4	SDLK_UNKNOWN
#define SDLK_WORLD_5	SDLK_UNKNOWN
#define SDLK_WORLD_6	SDLK_UNKNOWN
#define SDLK_WORLD_7	SDLK_UNKNOWN
#define SDLK_WORLD_8	SDLK_UNKNOWN
#define SDLK_WORLD_9	SDLK_UNKNOWN
#define SDLK_WORLD_10	SDLK_UNKNOWN
#define SDLK_WORLD_11	SDLK_UNKNOWN
#define SDLK_WORLD_12	SDLK_UNKNOWN
#define SDLK_WORLD_13	SDLK_UNKNOWN
#define SDLK_WORLD_14	SDLK_UNKNOWN
#define SDLK_WORLD_15	SDLK_UNKNOWN
#define SDLK_WORLD_16	SDLK_UNKNOWN
#define SDLK_WORLD_17	SDLK_UNKNOWN
#define SDLK_WORLD_18	SDLK_UNKNOWN
#define SDLK_WORLD_19	SDLK_UNKNOWN
#define SDLK_WORLD_20	SDLK_UNKNOWN
#define SDLK_WORLD_21	SDLK_UNKNOWN
#define SDLK_WORLD_22	SDLK_UNKNOWN
#define SDLK_WORLD_23	SDLK_UNKNOWN
#define SDLK_WORLD_24	SDLK_UNKNOWN
#define SDLK_WORLD_25	SDLK_UNKNOWN
#define SDLK_WORLD_26	SDLK_UNKNOWN
#define SDLK_WORLD_27	SDLK_UNKNOWN
#define SDLK_WORLD_28	SDLK_UNKNOWN
#define SDLK_WORLD_29	SDLK_UNKNOWN
#define SDLK_WORLD_30	SDLK_UNKNOWN
#define SDLK_WORLD_31	SDLK_UNKNOWN
#define SDLK_WORLD_32	SDLK_UNKNOWN
#define SDLK_WORLD_33	SDLK_UNKNOWN
#define SDLK_WORLD_34	SDLK_UNKNOWN
#define SDLK_WORLD_35	SDLK_UNKNOWN
#define SDLK_WORLD_36	SDLK_UNKNOWN
#define SDLK_WORLD_37	SDLK_UNKNOWN
#define SDLK_WORLD_38	SDLK_UNKNOWN
#define SDLK_WORLD_39	SDLK_UNKNOWN
#define SDLK_WORLD_40	SDLK_UNKNOWN
#define SDLK_WORLD_41	SDLK_UNKNOWN
#define SDLK_WORLD_42	SDLK_UNKNOWN
#define SDLK_WORLD_43	SDLK_UNKNOWN
#define SDLK_WORLD_44	SDLK_UNKNOWN
#define SDLK_WORLD_45	SDLK_UNKNOWN
#define SDLK_WORLD_46	SDLK_UNKNOWN
#define SDLK_WORLD_47	SDLK_UNKNOWN
#define SDLK_WORLD_48	SDLK_UNKNOWN
#define SDLK_WORLD_49	SDLK_UNKNOWN
#define SDLK_WORLD_50	SDLK_UNKNOWN
#define SDLK_WORLD_51	SDLK_UNKNOWN
#endif
{ "W0",     255,    SDLK_WORLD_0       },
{ "W1",     255,    SDLK_WORLD_1       },
{ "W2",     255,    SDLK_WORLD_2       }, // 100
{ "W3",     255,    SDLK_WORLD_3       },
{ "W4",     255,    SDLK_WORLD_4       },
{ "W5",     255,    SDLK_WORLD_5       },
{ "W6",     255,    SDLK_WORLD_6       },
{ "W7",     255,    SDLK_WORLD_7       },
{ "W8",     255,    SDLK_WORLD_8       },
{ "W9",     255,    SDLK_WORLD_9       },
{ "W10",    255,    SDLK_WORLD_10      },
{ "W11",    255,    SDLK_WORLD_11      },
{ "W12",    255,    SDLK_WORLD_12      }, // 110
{ "W13",    255,    SDLK_WORLD_13      },
{ "W14",    255,    SDLK_WORLD_14      },
{ "W15",    255,    SDLK_WORLD_15      },
{ "W16",    255,    SDLK_WORLD_16      },
{ "W17",    255,    SDLK_WORLD_17      },
{ "W18",    255,    SDLK_WORLD_18      },
{ "W19",    255,    SDLK_WORLD_19      },
{ "W20",    255,    SDLK_WORLD_20      },
{ "W21",    255,    SDLK_WORLD_21      },
{ "W22",    255,    SDLK_WORLD_22      }, // 120
{ "W23",    255,    SDLK_WORLD_23      },
{ "W24",    255,    SDLK_WORLD_24      },
{ "W25",    255,    SDLK_WORLD_25      },
{ "W26",    255,    SDLK_WORLD_26      },
{ "W27",    255,    SDLK_WORLD_27      },
{ "W28",    255,    SDLK_WORLD_28      },
{ "W29",    255,    SDLK_WORLD_29      },
{ "W30",    255,    SDLK_WORLD_30      },
{ "W31",    255,    SDLK_WORLD_31      },
{ "W32",    255,    SDLK_WORLD_32      }, // 130
{ "W33",    255,    SDLK_WORLD_33      },
{ "W34",    255,    SDLK_WORLD_34      },
{ "W35",    255,    SDLK_WORLD_35      },
{ "W36",    255,    SDLK_WORLD_36      },
{ "W37",    255,    SDLK_WORLD_37      },
{ "W38",    255,    SDLK_WORLD_38      },
{ "W39",    255,    SDLK_WORLD_39      },
{ "W40",    255,    SDLK_WORLD_40      },
{ "W41",    255,    SDLK_WORLD_41      },
{ "W42",    255,    SDLK_WORLD_42      }, // 140
{ "W43",    255,    SDLK_WORLD_43      },
{ "W44",    255,    SDLK_WORLD_44      },
{ "W45",    255,    SDLK_WORLD_45      },
{ "W46",    255,    SDLK_WORLD_46      },
{ "W47",    255,    SDLK_WORLD_47      },
{ "W48",    255,    SDLK_WORLD_48      },
{ "W49",    255,    SDLK_WORLD_49      },
{ "W50",    255,    SDLK_WORLD_50      },
{ "W51",    255,    SDLK_WORLD_51      },
{ "",       255,    SDLK_UNKNOWN                 }, // 150
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "PAD",    255,    SDLK_KP_ENTER      },
{ "RCTRL",  255,    SDLK_RCTRL         },
#if SDL_MAJOR_VERSION == 2
#define SDLK_LMETA	SDLK_UNKNOWN
#define SDLK_RMETA	SDLK_UNKNOWN
#endif
{ "LCMD",   255,    SDLK_LMETA         },
{ "RCMD",   255,    SDLK_RMETA         },
{ "",       255,    SDLK_UNKNOWN                 }, // 160
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 }, // 170
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 }, // 180
{ "PAD/",   255,    SDLK_KP_DIVIDE     },
{ "",       255,    SDLK_UNKNOWN                 },
#if SDL_MAJOR_VERSION == 1
#define SDLK_PRINTSCREEN	SDLK_PRINT
#endif
{ "PRSCR",  255,    SDLK_PRINTSCREEN   },
{ "RALT",   255,    SDLK_RALT          },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 }, // 190
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "",       255,    SDLK_UNKNOWN                 },
{ "HOME",   255,    SDLK_HOME          },
{ "UP",     255,    SDLK_UP            }, // 200
{ "PGUP",   255,    SDLK_PAGEUP        },
{ "",       255,    SDLK_UNKNOWN                 },
{ "LEFT",   255,    SDLK_LEFT          },
{ "",       255,    SDLK_UNKNOWN                 },
{ "RIGHT",  255,    SDLK_RIGHT         },
{ "",       255,    SDLK_UNKNOWN                 },
{ "END",    255,    SDLK_END           },
{ "DOWN",   255,    SDLK_DOWN          },
{ "PGDN",   255,    SDLK_PAGEDOWN      },
{ "INS",    255,    SDLK_INSERT        }, // 210
{ "DEL",    255,    SDLK_DELETE        },
#if SDL_MAJOR_VERSION == 1
{ "W52",    255,    SDLK_WORLD_52      },
{ "W53",    255,    SDLK_WORLD_53      },
{ "W54",    255,    SDLK_WORLD_54      },
{ "W55",    255,    SDLK_WORLD_55      },
{ "W56",    255,    SDLK_WORLD_56      },
{ "W57",    255,    SDLK_WORLD_57      },
{ "W58",    255,    SDLK_WORLD_58      },
{ "W59",    255,    SDLK_WORLD_59      },
{ "W60",    255,    SDLK_WORLD_60      }, // 220
{ "W61",    255,    SDLK_WORLD_61      },
{ "W62",    255,    SDLK_WORLD_62      },
{ "W63",    255,    SDLK_WORLD_63      },
{ "W64",    255,    SDLK_WORLD_64      },
{ "W65",    255,    SDLK_WORLD_65      },
{ "W66",    255,    SDLK_WORLD_66      },
{ "W67",    255,    SDLK_WORLD_67      },
{ "W68",    255,    SDLK_WORLD_68      },
{ "W69",    255,    SDLK_WORLD_69      },
{ "W70",    255,    SDLK_WORLD_70      }, // 230
{ "W71",    255,    SDLK_WORLD_71      },
{ "W72",    255,    SDLK_WORLD_72      },
{ "W73",    255,    SDLK_WORLD_73      },
{ "W74",    255,    SDLK_WORLD_74      },
{ "W75",    255,    SDLK_WORLD_75      },
{ "W76",    255,    SDLK_WORLD_76      },
{ "W77",    255,    SDLK_WORLD_77      },
{ "W78",    255,    SDLK_WORLD_78      },
{ "W79",    255,    SDLK_WORLD_79      },
{ "W80",    255,    SDLK_WORLD_80      }, // 240
{ "W81",    255,    SDLK_WORLD_81      },
{ "W82",    255,    SDLK_WORLD_82      },
{ "W83",    255,    SDLK_WORLD_83      },
{ "W84",    255,    SDLK_WORLD_84      },
{ "W85",    255,    SDLK_WORLD_85      },
{ "W86",    255,    SDLK_WORLD_86      },
{ "W87",    255,    SDLK_WORLD_87      },
{ "W88",    255,    SDLK_WORLD_88      },
{ "W89",    255,    SDLK_WORLD_89      },
{ "W90",    255,    SDLK_WORLD_90      }, // 250
{ "W91",    255,    SDLK_WORLD_91      },
{ "W92",    255,    SDLK_WORLD_92      },
{ "W93",    255,    SDLK_WORLD_93      },
{ "W94",    255,    SDLK_WORLD_94      },
{ "W95",    255,    SDLK_WORLD_95      }, // 255
#endif
}};

namespace {

struct d_event_keycommand : d_event
{
	const unsigned keycode;
	constexpr d_event_keycommand(const event_type t, const unsigned k) :
		d_event(t), keycode(k)
	{
	}
};

}

static int key_ismodlck(int keycode)
{
	switch (keycode)
	{
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_LALT:
		case KEY_RALT:
		case KEY_LCTRL:
		case KEY_RCTRL:
		case KEY_LMETA:
		case KEY_RMETA:
			return KEY_ISMOD;
		case KEY_NUMLOCK:
		case KEY_SCROLLOCK:
		case KEY_CAPSLOCK:
			return KEY_ISLCK;
		default:
			return 0;
	}
}

unsigned char key_ascii()
{
	using std::move;
	using std::next;
	static array<unsigned char, KEY_BUFFER_SIZE> unibuffer;
	auto src = begin(unicode_frame_buffer);
	auto dst = next(begin(unibuffer), strlen(reinterpret_cast<const char *>(&unibuffer[0])));
	
	// move temporal chars from unicode_frame_buffer to empty space behind last unibuffer char (if any)
	for (; dst != end(unibuffer); ++dst)
		if (*src != '\0')
		{
			*dst = *src;
			*src = '\0';
			++src;
		}

	// unibuffer is not empty. store first char, remove it, shift all chars one step left and then print our char
	if (unibuffer[0] != '\0')
	{
		unsigned char retval = unibuffer[0];
		*move(next(unibuffer.begin()), unibuffer.end(), unibuffer.begin()) = 0;
		return retval;
	}
	else
		return 255;
}

void pressed_keys::update(const std::size_t keycode, const uint8_t down)
{
	constexpr unsigned all_modifiers_combined = KEY_SHIFTED | KEY_ALTED | KEY_CTRLED | KEY_DEBUGGED | KEY_METAED;
	constexpr unsigned all_modifiers_shifted = all_modifiers_combined >> modifier_shift;
	static_assert(all_modifiers_combined == all_modifiers_shifted << modifier_shift, "shift error");
	static_assert(all_modifiers_shifted == static_cast<uint8_t>(all_modifiers_shifted), "truncation error");
	uint8_t mask;
	keyd_pressed.update_pressed(keycode, down);
	switch (keycode)
	{
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			mask = KEY_SHIFTED >> modifier_shift;
			break;
		case KEY_LALT:
		case KEY_RALT:
			mask = KEY_ALTED >> modifier_shift;
			break;
		case KEY_LCTRL:
		case KEY_RCTRL:
			mask = KEY_CTRLED >> modifier_shift;
			break;
		case KEY_DELETE:
			mask = KEY_DEBUGGED >> modifier_shift;
			break;
		case KEY_LMETA:
		case KEY_RMETA:
			mask = KEY_METAED >> modifier_shift;
			break;
		default:
			return;
	}
	if (down)
		modifier_cache |= mask;
	else
		modifier_cache &= ~mask;
}

window_event_result key_handler(SDL_KeyboardEvent *kevent)
{
	// Read SDLK symbol and state
	const auto event_keysym = kevent->keysym.sym;
	if (event_keysym == SDLK_UNKNOWN)
		return window_event_result::ignored;
	const auto key_state = (kevent->state != SDL_RELEASED);

	// fill the unicode frame-related unicode buffer 
	if (key_state)
	{
#if SDL_MAJOR_VERSION == 1
		const auto sym = kevent->keysym.unicode;
#elif SDL_MAJOR_VERSION == 2
		const auto sym = kevent->keysym.sym;
#endif
		if (sym > 31 && sym < 255)
		{
			range_for (auto &i, unicode_frame_buffer)
				if (i == '\0')
				{
					i = sym;
					break;
				}
		}
	}

	//=====================================================
	auto re = key_properties.rend();
	auto fi = std::find_if(key_properties.rbegin(), re, [event_keysym](const key_props &k) { return k.sym == event_keysym; });
	if (fi == re)
		return window_event_result::ignored;
	unsigned keycode = std::distance(key_properties.begin(), std::next(fi).base());
	if (keycode == 0)
		return window_event_result::ignored;

	/* 
	 * process the key if:
	 * - it's a valid key AND
	 * - if the keystate has changed OR
	 * - key state same as last one and game accepts key repeats but keep out mod/lock keys
	 */
	if (key_state != keyd_pressed[keycode] || (keyd_repeat && !key_ismodlck(keycode)))
	{
		// now update the key props
		keyd_pressed.update(keycode, key_state);
		const auto raw_keycode = keycode;
		keycode |= keyd_pressed.get_modifiers();

		// We allowed the key to be added to the queue for now,
		// because there are still input loops without associated windows
		const d_event_keycommand event{key_state ? EVENT_KEY_COMMAND : EVENT_KEY_RELEASE, keycode};
		con_printf(CON_DEBUG, "Sending event %s: %s %s %s %s %s %s",
				(key_state)                  ? "EVENT_KEY_COMMAND": "EVENT_KEY_RELEASE",
				(keycode & KEY_METAED)	? "META" : "",
				(keycode & KEY_DEBUGGED)	? "DEBUG" : "",
				(keycode & KEY_CTRLED)	? "CTRL" : "",
				(keycode & KEY_ALTED)	? "ALT" : "",
				(keycode & KEY_SHIFTED)	? "SHIFT" : "",
				key_properties[raw_keycode].key_text
				);
		return event_send(event);
	}
	return window_event_result::ignored;
}

void key_init()
{
#if SDL_MAJOR_VERSION == 1
	SDL_EnableUNICODE(1);
#endif
	key_toggle_repeat(1);

	keyd_time_when_last_pressed = timer_query();
	// Clear the keyboard array
	key_flush();
}

static void restore_sticky_key(const uint8_t *keystate, const unsigned i)
{
#if SDL_MAJOR_VERSION == 1
	const auto ki = key_properties[i].sym;
#elif SDL_MAJOR_VERSION == 2
	const auto ki = i;
#endif
	const auto v = keystate[ki];	// do not flush status of sticky keys
	keyd_pressed.update_pressed(i, v);
}

void key_flush()
{
	//Clear the unicode buffer
	unicode_frame_buffer = {};
	keyd_pressed = {};
	if (unlikely(CGameArg.CtlNoStickyKeys))
		return;
#if SDL_MAJOR_VERSION == 1
	const auto &keystate = SDL_GetKeyState(nullptr);
#define DXX_SDL_STICKY_KEYS	{KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCROLLOCK}
#elif SDL_MAJOR_VERSION == 2
	const auto &keystate = SDL_GetKeyboardState(nullptr);
#define DXX_SDL_STICKY_KEYS	{SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_SCROLLLOCK, SDL_SCANCODE_NUMLOCKCLEAR}
#endif
	range_for (const auto key, DXX_SDL_STICKY_KEYS)
#undef DXX_SDL_STICKY_KEYS
		restore_sticky_key(keystate, key);
}

int event_key_get(const d_event &event)
{
	auto &e = static_cast<const d_event_keycommand &>(event);
	Assert(e.type == EVENT_KEY_COMMAND || e.type == EVENT_KEY_RELEASE);
	return e.keycode;
}

// same as above but without mod states
int event_key_get_raw(const d_event &event)
{
	return event_key_get(event) & ~(KEY_SHIFTED | KEY_ALTED | KEY_CTRLED | KEY_DEBUGGED | KEY_METAED);
}

void key_toggle_repeat1()
{
#if SDL_MAJOR_VERSION == 1
	if (SDL_EnableKeyRepeat(KEY_REPEAT_DELAY, KEY_REPEAT_INTERVAL) == 0)
#endif
		keyd_repeat = 1;
	key_flush();
}

void key_toggle_repeat0()
{
#if SDL_MAJOR_VERSION == 1
	SDL_EnableKeyRepeat(0, 0);
#endif
	keyd_repeat = 0;
	key_flush();
}

}
