/*
 * $Source: /cvs/cvsroot/d2x/input/sdl_key.c,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-01-29 14:03:57 $
 *
 * SDL keyboard input support
 *
 * $Log: not supported by cvs2svn $
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "event.h"
#include "error.h"
#include "key.h"
#include "timer.h"

//added on 9/3/98 by Matt Mueller to free some cpu instead of hogging during menus and such
#include "d_delay.h"
//end this section addition - Matt Mueller

#define KEY_BUFFER_SIZE 16

static unsigned char Installed = 0;

//-------- Variable accessed by outside functions ---------
unsigned char 		keyd_buffer_type;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
unsigned char 		keyd_repeat;
unsigned char 		keyd_editor_mode;
volatile unsigned char 	keyd_last_pressed;
volatile unsigned char 	keyd_last_released;
volatile unsigned char	keyd_pressed[256];
volatile int		keyd_time_when_last_pressed;

typedef struct Key_info {
	ubyte		state;			// state of key 1 == down, 0 == up
	ubyte		last_state;		// previous state of key
	int		counter;		// incremented each time key is down in handler
	fix		timewentdown;	// simple counter incremented each time in interrupt and key is down
	fix		timehelddown;	// counter to tell how long key is down -- gets reset to 0 by key routines
	ubyte		downcount;		// number of key counts key was down
	ubyte		upcount;		// number of times key was released
} Key_info;

typedef struct keyboard	{
	unsigned short		keybuffer[KEY_BUFFER_SIZE];
	Key_info		keys[256];
	fix			time_pressed[KEY_BUFFER_SIZE];
	unsigned int 		keyhead, keytail;
} keyboard;

static keyboard key_data;

typedef struct key_props {
	char *key_text;
	unsigned char ascii_value;
	unsigned char shifted_ascii_value;
	SDLKey sym;
} key_props;

key_props key_properties[256] = {
{ "",       255,    255,    -1                 },
{ "ESC",    255,    255,    SDLK_ESCAPE        },
{ "1",      '1',    '!',    SDLK_1             },
{ "2",      '2',    '@',    SDLK_2             },
{ "3",      '3',    '#',    SDLK_3             },
{ "4",      '4',    '$',    SDLK_4             },
{ "5",      '5',    '%',    SDLK_5             },
{ "6",      '6',    '^',    SDLK_6             },
{ "7",      '7',    '&',    SDLK_7             },
{ "8",      '8',    '*',    SDLK_8             },
{ "9",      '9',    '(',    SDLK_9             },
{ "0",      '0',    ')',    SDLK_0             },
{ "-",      '-',    '_',    SDLK_MINUS         },
{ "=",      '=',    '+',    SDLK_EQUALS        },
{ "BSPC",   255,    255,    SDLK_BACKSPACE     },
{ "TAB",    255,    255,    SDLK_TAB           },
{ "Q",      'q',    'Q',    SDLK_q             },
{ "W",      'w',    'W',    SDLK_w             },
{ "E",      'e',    'E',    SDLK_e             },
{ "R",      'r',    'R',    SDLK_r             },
{ "T",      't',    'T',    SDLK_t             },
{ "Y",      'y',    'Y',    SDLK_y             },
{ "U",      'u',    'U',    SDLK_u             },
{ "I",      'i',    'I',    SDLK_i             },
{ "O",      'o',    'O',    SDLK_o             },
{ "P",      'p',    'P',    SDLK_p             },
{ "[",      '[',    '{',    SDLK_LEFTBRACKET   },
{ "]",      ']',    '}',    SDLK_RIGHTBRACKET  },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "ƒ",      255,    255,    SDLK_RETURN        },
//end edit -MM
{ "LCTRL",  255,    255,    SDLK_LCTRL         },
{ "A",      'a',    'A',    SDLK_a             },
{ "S",      's',    'S',    SDLK_s             },
{ "D",      'd',    'D',    SDLK_d             },
{ "F",      'f',    'F',    SDLK_f             },
{ "G",      'g',    'G',    SDLK_g             },
{ "H",      'h',    'H',    SDLK_h             },
{ "J",      'j',    'J',    SDLK_j             },
{ "K",      'k',    'K',    SDLK_k             },
{ "L",      'l',    'L',    SDLK_l             },
//edited 06/08/99 Matt Mueller - set to correct sym
{ ";",      ';',    ':',    SDLK_SEMICOLON         },
//end edit -MM
{ "'",      '\'',   '"',    SDLK_QUOTE         },
//edited 06/08/99 Matt Mueller - set to correct sym
{ "`",      '`',    '~',    SDLK_BACKQUOTE     },
//end edit -MM
{ "LSHFT",  255,    255,    SDLK_LSHIFT        },
{ "\\",     '\\',   '|',    SDLK_BACKSLASH     },
{ "Z",      'z',    'Z',    SDLK_z             },
{ "X",      'x',    'X',    SDLK_x             },
{ "C",      'c',    'C',    SDLK_c             },
{ "V",      'v',    'V',    SDLK_v             },
{ "B",      'b',    'B',    SDLK_b             },
{ "N",      'n',    'N',    SDLK_n             },
{ "M",      'm',    'M',    SDLK_m             },
//edited 06/08/99 Matt Mueller - set to correct syms
{ ",",      ',',    '<',    SDLK_COMMA	},
{ ".",      '.',    '>',    SDLK_PERIOD	},
{ "/",      '/',    '?',    SDLK_SLASH	},
//end edit -MM
{ "RSHFT",  255,    255,    SDLK_RSHIFT	},
{ "PAD*",   '*',    255,    SDLK_KP_MULTIPLY   },
{ "LALT",   255,    255,    SDLK_LALT          },
{ "SPC",    ' ',    ' ',    SDLK_SPACE         },
{ "CPSLK",  255,    255,    SDLK_CAPSLOCK      },
{ "F1",     255,    255,    SDLK_F1            },
{ "F2",     255,    255,    SDLK_F2            },
{ "F3",     255,    255,    SDLK_F3            },
{ "F4",     255,    255,    SDLK_F4            },
{ "F5",     255,    255,    SDLK_F5            },
{ "F6",     255,    255,    SDLK_F6            },
{ "F7",     255,    255,    SDLK_F7            },
{ "F8",     255,    255,    SDLK_F8            },
{ "F9",     255,    255,    SDLK_F9            },
{ "F10",    255,    255,    SDLK_F10           },
{ "NMLCK",  255,    255,    SDLK_NUMLOCK       },
{ "SCLK",   255,    255,    SDLK_SCROLLOCK     },
{ "PAD7",   255,    255,    SDLK_KP7           },
{ "PAD8",   255,    255,    SDLK_KP8           },
{ "PAD9",   255,    255,    SDLK_KP9           },
{ "PAD-",   255,    255,    SDLK_KP_MINUS      },
{ "PAD4",   255,    255,    SDLK_KP4           },
{ "PAD5",   255,    255,    SDLK_KP5           },
{ "PAD6",   255,    255,    SDLK_KP6           },
{ "PAD+",   255,    255,    SDLK_KP_PLUS       },
{ "PAD1",   255,    255,    SDLK_KP1           },
{ "PAD2",   255,    255,    SDLK_KP2           },
{ "PAD3",   255,    255,    SDLK_KP3           },
{ "PAD0",   255,    255,    SDLK_KP0           },
{ "PAD.",   255,    255,    SDLK_KP_PERIOD     },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "F11",    255,    255,    SDLK_F11           },
{ "F12",    255,    255,    SDLK_F12           },
{ "",       255,    255,    -1                 },	
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - add pause ability
{ "PAUSE",       255,    255,    SDLK_PAUSE                 },
//end edit -MM
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "PADƒ",   255,    255,    SDLK_KP_ENTER      },
//end edit -MM
//edited 06/08/99 Matt Mueller - set to correct sym
{ "RCTRL",  255,    255,    SDLK_RCTRL            },
//end edit -MM
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "PAD/",   255,    255,    SDLK_KP_DIVIDE     },
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - add printscreen ability
{ "PRSCR",       255,    255,    SDLK_PRINT                 },
//end edit -MM
{ "RALT",   255,    255,    SDLK_RALT          },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "HOME",   255,    255,    SDLK_HOME          },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "UP",		255,    255,    SDLK_UP            },
//end edit -MM
{ "PGUP",   255,    255,    SDLK_PAGEUP        },
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "LEFT",	255,    255,    SDLK_LEFT          },
//end edit -MM
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "RIGHT",	255,    255,    SDLK_RIGHT         },
//end edit -MM
{ "",       255,    255,    -1                 },
//edited 06/08/99 Matt Mueller - set to correct key_text
{ "END",    255,    255,    SDLK_END           },
//end edit -MM
{ "DOWN",	255,    255,    SDLK_DOWN          },
{ "PGDN",	255,    255,    SDLK_PAGEDOWN      },
{ "INS",	255,    255,    SDLK_INSERT        },
{ "DEL",	255,    255,    SDLK_DELETE        },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
{ "",       255,    255,    -1                 },
};

char *key_text[256];

void key_buid_key_text(void)
{
}

unsigned char key_to_ascii(int keycode )
{
	int shifted;

	shifted = keycode & KEY_SHIFTED;
	keycode &= 0xFF;

	if (shifted)
		return key_properties[keycode].shifted_ascii_value;
	else
		return key_properties[keycode].ascii_value;
}

void key_handler(SDL_KeyboardEvent *event)
{
	ubyte state;
	int i, keycode, event_key, key_state;
	Key_info *key;
	unsigned char temp;

        event_key = event->keysym.sym;

        key_state = (event->state == SDL_PRESSED); //  !(wInfo & KF_UP);
	//=====================================================
	//Here a translation from win keycodes to mac keycodes!
	//=====================================================

	for (i = 255; i >= 0; i--) {

		keycode = i;
		key = &(key_data.keys[keycode]);
                if (key_properties[i].sym == event_key)
			state = key_state;
		else
			state = key->last_state;
			
		if ( key->last_state == state )	{
			if (state) {
				key->counter++;
				keyd_last_pressed = keycode;
				keyd_time_when_last_pressed = timer_get_fixed_seconds();
			}
		} else {
			if (state)	{
				keyd_last_pressed = keycode;
				keyd_pressed[keycode] = 1;
				key->downcount += state;
				key->state = 1;
				key->timewentdown = keyd_time_when_last_pressed = timer_get_fixed_seconds();
				key->counter++;
			} else {	
				keyd_pressed[keycode] = 0;
				keyd_last_released = keycode;
				key->upcount += key->state;
				key->state = 0;
				key->counter = 0;
				key->timehelddown += timer_get_fixed_seconds() - key->timewentdown;
			}
		}
		if ( (state && !key->last_state) || (state && key->last_state && (key->counter > 30) && (key->counter & 0x01)) ) {
			if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
				keycode |= KEY_SHIFTED;
			if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
				keycode |= KEY_ALTED;
			if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
				keycode |= KEY_CTRLED;
                        if ( keyd_pressed[KEY_DELETE] )
                                keycode |= KEY_DEBUGGED;
			temp = key_data.keytail+1;
			if ( temp >= KEY_BUFFER_SIZE ) temp=0;
			if (temp!=key_data.keyhead)	{
				key_data.keybuffer[key_data.keytail] = keycode;
				key_data.time_pressed[key_data.keytail] = keyd_time_when_last_pressed;
				key_data.keytail = temp;
			}
		}
		key->last_state = state;
	}
}

void key_close()
{
 Installed = 0;
}

void key_init()
{
  int i;
  
  if (Installed) return;

  Installed=1;

  keyd_time_when_last_pressed = timer_get_fixed_seconds();
  keyd_buffer_type = 1;
  keyd_repeat = 1;
  
  for(i=0; i<256; i++)
     key_text[i] = key_properties[i].key_text;
     
  // Clear the keyboard array
  key_flush();
  atexit(key_close);
}

void key_flush()
{
 	int i;
	fix curtime;

	if (!Installed)
		key_init();

	key_data.keyhead = key_data.keytail = 0;

	//Clear the keyboard buffer
	for (i=0; i<KEY_BUFFER_SIZE; i++ )	{
		key_data.keybuffer[i] = 0;
		key_data.time_pressed[i] = 0;
	}

//use gettimeofday here:
	curtime = timer_get_fixed_seconds();

	for (i=0; i<256; i++ )	{
		keyd_pressed[i] = 0;
		key_data.keys[i].state = 1;
		key_data.keys[i].last_state = 0;
		key_data.keys[i].timewentdown = curtime;
		key_data.keys[i].downcount=0;
		key_data.keys[i].upcount=0;
		key_data.keys[i].timehelddown = 0;
		key_data.keys[i].counter = 0;
	}
}

int add_one(int n)
{
 n++;
 if ( n >= KEY_BUFFER_SIZE ) n=0;
 return n;
}

int key_checkch()
{
	int is_one_waiting = 0;
	event_poll();
	if (key_data.keytail!=key_data.keyhead)
		is_one_waiting = 1;
	return is_one_waiting;
}

int key_inkey()
{
	int key = 0;
	if (!Installed)
		key_init();
        event_poll();
	if (key_data.keytail!=key_data.keyhead) {
		key = key_data.keybuffer[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}
//added 9/3/98 by Matt Mueller to free cpu time instead of hogging during menus and such
	else d_delay(1);
//end addition - Matt Mueller
	     
        return key;
}

int key_inkey_time(fix * time)
{
	int key = 0;

	if (!Installed)
		key_init();
        event_poll();
	if (key_data.keytail!=key_data.keyhead)	{
		key = key_data.keybuffer[key_data.keyhead];
		*time = key_data.time_pressed[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}
	return key;
}

int key_peekkey()
{
	int key = 0;
        event_poll();
	if (key_data.keytail!=key_data.keyhead)
		key = key_data.keybuffer[key_data.keyhead];

	return key;
}

int key_getch()
{
	int dummy=0;

	if (!Installed)
		return 0;
//		return getch();

	while (!key_checkch())
		dummy++;
	return key_inkey();
}

unsigned int key_get_shift_status()
{
	unsigned int shift_status = 0;

	if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT] )
		shift_status |= KEY_SHIFTED;

	if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT] )
		shift_status |= KEY_ALTED;

	if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL] )
		shift_status |= KEY_CTRLED;

#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE])
		shift_status |=KEY_DEBUGGED;
#endif

	return shift_status;
}

// Returns the number of seconds this key has been down since last call.
fix key_down_time(int scancode)
{
	fix time_down, time;

	event_poll();
        if ((scancode<0)|| (scancode>255)) return 0;

	if (!keyd_pressed[scancode]) {
		time_down = key_data.keys[scancode].timehelddown;
		key_data.keys[scancode].timehelddown = 0;
	} else {
		time = timer_get_fixed_seconds();
		time_down = time - key_data.keys[scancode].timewentdown;
		key_data.keys[scancode].timewentdown = time;
	}

	return time_down;
}

unsigned int key_down_count(int scancode)
{
	int n;
        event_poll();
        if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.keys[scancode].downcount;
	key_data.keys[scancode].downcount = 0;

	return n;
}

unsigned int key_up_count(int scancode)
{
	int n;
        event_poll();
        if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.keys[scancode].upcount;
	key_data.keys[scancode].upcount = 0;

	return n;
}
