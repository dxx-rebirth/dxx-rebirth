/*
 *
 * SDL keyboard input support
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "event.h"
#include "dxxerror.h"
#include "key.h"
#include "timer.h"
#include "window.h"
#include "console.h"
#include "args.h"

static unsigned char Installed = 0;

//-------- Variable accessed by outside functions ---------
int			keyd_repeat = 0; // 1 = use repeats, 0 no repeats
volatile unsigned char 	keyd_last_pressed;
volatile unsigned char 	keyd_last_released;
volatile unsigned char	keyd_pressed[256];
fix64			keyd_time_when_last_pressed;
unsigned char		unicode_frame_buffer[KEY_BUFFER_SIZE] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

typedef struct keyboard	{
	ubyte state[256];
} keyboard;

static keyboard key_data;

const key_props key_properties[256] = {
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
{ "NMLCK",  255,    SDLK_NUMLOCK       },
{ "SCLK",   255,    SDLK_SCROLLOCK     }, // 70
{ "PAD7",   255,    SDLK_KP7           },
{ "PAD8",   255,    SDLK_KP8           },
{ "PAD9",   255,    SDLK_KP9           },
{ "PAD-",   255,    SDLK_KP_MINUS      },
{ "PAD4",   255,    SDLK_KP4           },
{ "PAD5",   255,    SDLK_KP5           },
{ "PAD6",   255,    SDLK_KP6           },
{ "PAD+",   255,    SDLK_KP_PLUS       },
{ "PAD1",   255,    SDLK_KP1           },
{ "PAD2",   255,    SDLK_KP2           }, // 80
{ "PAD3",   255,    SDLK_KP3           },
{ "PAD0",   255,    SDLK_KP0           },
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
{ "PRSCR",  255,    SDLK_PRINT         },
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
};

typedef struct d_event_keycommand
{
	event_type	type;	// EVENT_KEY_COMMAND/RELEASE
	int			keycode;
} d_event_keycommand;

int key_ismodlck(int keycode)
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
	static unsigned char unibuffer[KEY_BUFFER_SIZE] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
	int i=0, offset=0, count=0;
	
	offset=strlen((const char*)unibuffer);

	// move temporal chars from unicode_frame_buffer to empty space behind last unibuffer char (if any)
	for (i=offset; i < KEY_BUFFER_SIZE; i++)
		if (unicode_frame_buffer[count] != '\0')
		{
			unibuffer[i]=unicode_frame_buffer[count];
			unicode_frame_buffer[count]='\0';
			count++;
		}

	// unibuffer is not empty. store first char, remove it, shift all chars one step left and then print our char
	if (unibuffer[0] != '\0')
	{
		unsigned char retval = unibuffer[0];
		unsigned char unibuffer_shift[KEY_BUFFER_SIZE];
		memset(unibuffer_shift,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
		memcpy(unibuffer_shift,unibuffer+1,sizeof(unsigned char)*(KEY_BUFFER_SIZE-1));
		memcpy(unibuffer,unibuffer_shift,sizeof(unsigned char)*KEY_BUFFER_SIZE);
		return retval;
	}
	else
		return 255;
}

void key_handler(SDL_KeyboardEvent *kevent)
{
	int keycode, event_keysym=-1, key_state;

	// Read SDLK symbol and state
        event_keysym = kevent->keysym.sym;
		if (event_keysym == SDLK_UNKNOWN)
			return;
        key_state = (kevent->state == SDL_PRESSED)?1:0;

	// fill the unicode frame-related unicode buffer 
	if (key_state && kevent->keysym.unicode > 31 && kevent->keysym.unicode < 255)
	{
		int i = 0;
		for (i = 0; i < KEY_BUFFER_SIZE; i++)
			if (unicode_frame_buffer[i] == '\0')
			{
				unicode_frame_buffer[i] = kevent->keysym.unicode;
				break;
			}
	}

	//=====================================================
	for (keycode = 255; keycode > 0; keycode--)
		if (key_properties[keycode].sym == event_keysym)
			break;

	if (keycode == 0)
		return;

	/* 
	 * process the key if:
	 * - it's a valid key AND
	 * - if the keystate has changed OR
	 * - key state same as last one and game accepts key repeats but keep out mod/lock keys
	 */
	if (key_state != keyd_pressed[keycode] || (keyd_repeat && !key_ismodlck(keycode)))
	{
		d_event_keycommand event;

		// now update the key props
		if (key_state) {
			keyd_last_pressed = keycode;
			keyd_pressed[keycode] = key_data.state[keycode] = 1;
		} else {
			keyd_pressed[keycode] = key_data.state[keycode] = 0;
		}

		if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
			keycode |= KEY_SHIFTED;
		if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
			keycode |= KEY_ALTED;
		if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
			keycode |= KEY_CTRLED;
		if ( keyd_pressed[KEY_DELETE] )
			keycode |= KEY_DEBUGGED;
		if ( keyd_pressed[KEY_LMETA] || keyd_pressed[KEY_RMETA])
			keycode |= KEY_METAED;

		// We allowed the key to be added to the queue for now,
		// because there are still input loops without associated windows
		event.type = key_state?EVENT_KEY_COMMAND:EVENT_KEY_RELEASE;
		event.keycode = keycode;
		con_printf(CON_DEBUG, "Sending event %s: %s %s %s %s %s %s\n",
				(key_state)                  ? "EVENT_KEY_COMMAND": "EVENT_KEY_RELEASE",
				(keycode & KEY_METAED)	? "META" : "",
				(keycode & KEY_DEBUGGED)	? "DEBUG" : "",
				(keycode & KEY_CTRLED)	? "CTRL" : "",
				(keycode & KEY_ALTED)	? "ALT" : "",
				(keycode & KEY_SHIFTED)	? "SHIFT" : "",
				key_properties[keycode & 0xff].key_text
				);
		event_send((d_event *)&event);
	}
}

void key_close()
{
      Installed = 0;
}

void key_init()
{
	if (Installed) return;

	Installed=1;
	SDL_EnableUNICODE(1);
	key_toggle_repeat(1);

	keyd_time_when_last_pressed = timer_query();
	// Clear the keyboard array
	key_flush();
}

void key_flush()
{
 	int i;
	Uint8 *keystate = SDL_GetKeyState(NULL);

	if (!Installed)
		key_init();

	//Clear the unicode buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )
		unicode_frame_buffer[i] = '\0';

	for (i=0; i<256; i++ )	{
		if (key_ismodlck(i) == KEY_ISLCK && keystate[key_properties[i].sym] && !GameArg.CtlNoStickyKeys) // do not flush status of sticky keys
		{
			keyd_pressed[i] = 1;
			key_data.state[i] = 0;
		}
		else
		{
			keyd_pressed[i] = 0;
			key_data.state[i] = 1;
		}
	}
}

int event_key_get(d_event *event)
{
	Assert(event->type == EVENT_KEY_COMMAND || event->type == EVENT_KEY_RELEASE);
	return ((d_event_keycommand *)event)->keycode;
}

// same as above but without mod states
int event_key_get_raw(d_event *event)
{
	int keycode = ((d_event_keycommand *)event)->keycode;
	Assert(event->type == EVENT_KEY_COMMAND || event->type == EVENT_KEY_RELEASE);
	if ( keycode & KEY_SHIFTED ) keycode &= ~KEY_SHIFTED;
	if ( keycode & KEY_ALTED ) keycode &= ~KEY_ALTED;
	if ( keycode & KEY_CTRLED ) keycode &= ~KEY_CTRLED;
	if ( keycode & KEY_DEBUGGED ) keycode &= ~KEY_DEBUGGED;
	if ( keycode & KEY_METAED ) keycode &= ~KEY_METAED;
	return keycode;
}

void key_toggle_repeat(int enable)
{
	if (enable)
	{
		if (SDL_EnableKeyRepeat(KEY_REPEAT_DELAY, KEY_REPEAT_INTERVAL) == 0)
			keyd_repeat = 1;
	}
	else
	{
		SDL_EnableKeyRepeat(0, 0);
		keyd_repeat = 0;
	}
	key_flush();
}
