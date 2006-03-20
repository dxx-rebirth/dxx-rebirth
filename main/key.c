//main/key.c : arch-independant key stuff. Added 2000/01/16 Matt Mueller
// Previously there was a huge amount of duplication (and usually unhandled keys I had to fix whenever I started using a new arch).
// Now they only have to define a few arch_ funcs to get keys, ARCH_KEY defines, and this will handle the rest.  It is basically what arch/sdl/key.c used to be, minus the SDL specific stuff (and with correct key repeat)

#include "key.h"
//arch-specific defines
#include "key_arch.h"

//arch specific prototypes
void arch_key_close(void);
void arch_key_init(void);
void arch_key_flush(void);
void arch_key_poll(void);

//added on 9/3/98 by Matt Mueller to free some cpu instead of hogging during menus and such
#include "d_delay.h"
//end this section addition - Matt Mueller

#include "timer.h"

#include "mono.h"

#define KEY_BUFFER_SIZE 16

static unsigned char Installed = 0;


//-------- Variable accessed by outside functions ---------
unsigned char		keyd_buffer_type;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
unsigned char		keyd_repeat;
unsigned char		keyd_fake_repeat;
unsigned char		keyd_editor_mode;
volatile unsigned char	keyd_last_pressed;
volatile unsigned char	keyd_last_released;
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
	int sym;
} key_props;

key_props key_properties[256] = {
	{ "",       255,    255,    -1                     },
	{ "ESC",    255,    255,    ARCH_KEY_ESC           },
	{ "1",      '1',    '!',    ARCH_KEY_1             },
	{ "2",      '2',    '@',    ARCH_KEY_2             },
	{ "3",      '3',    '#',    ARCH_KEY_3             },
	{ "4",      '4',    '$',    ARCH_KEY_4             },
	{ "5",      '5',    '%',    ARCH_KEY_5             },
	{ "6",      '6',    '^',    ARCH_KEY_6             },
	{ "7",      '7',    '&',    ARCH_KEY_7             },
	{ "8",      '8',    '*',    ARCH_KEY_8             },
	{ "9",      '9',    '(',    ARCH_KEY_9             },
	{ "0",      '0',    ')',    ARCH_KEY_0             },
	{ "-",      '-',    '_',    ARCH_KEY_MINUS         },
	{ "=",      '=',    '+',    ARCH_KEY_EQUAL         },
	{ "BSPC",   255,    255,    ARCH_KEY_BACKSP        },
	{ "TAB",    255,    255,    ARCH_KEY_TAB           },
	{ "Q",      'q',    'Q',    ARCH_KEY_Q             },
	{ "W",      'w',    'W',    ARCH_KEY_W             },
	{ "E",      'e',    'E',    ARCH_KEY_E             },
	{ "R",      'r',    'R',    ARCH_KEY_R             },
	{ "T",      't',    'T',    ARCH_KEY_T             },
	{ "Y",      'y',    'Y',    ARCH_KEY_Y             },
	{ "U",      'u',    'U',    ARCH_KEY_U             },
	{ "I",      'i',    'I',    ARCH_KEY_I             },
	{ "O",      'o',    'O',    ARCH_KEY_O             },
	{ "P",      'p',    'P',    ARCH_KEY_P             },
	{ "[",      '[',    '{',    ARCH_KEY_LBRACKET      },
	{ "]",      ']',    '}',    ARCH_KEY_RBRACKET      },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "",      255,    255,    ARCH_KEY_ENTER         },
	//end edit -MM
	{ "LCTRL",  255,    255,    ARCH_KEY_LCTRL         },
	{ "A",      'a',    'A',    ARCH_KEY_A             },
	{ "S",      's',    'S',    ARCH_KEY_S             },
	{ "D",      'd',    'D',    ARCH_KEY_D             },
	{ "F",      'f',    'F',    ARCH_KEY_F             },
	{ "G",      'g',    'G',    ARCH_KEY_G             },
	{ "H",      'h',    'H',    ARCH_KEY_H             },
	{ "J",      'j',    'J',    ARCH_KEY_J             },
	{ "K",      'k',    'K',    ARCH_KEY_K             },
	{ "L",      'l',    'L',    ARCH_KEY_L             },
	//edited 06/08/99 Matt Mueller - set to correct sym
	{ ";",      ';',    ':',    ARCH_KEY_SEMICOL       },
	//end edit -MM
	{ "'",      '\'',   '"',    ARCH_KEY_RAPOSTRO      },
	//edited 06/08/99 Matt Mueller - set to correct sym
	{ "`",      '`',    '~',    ARCH_KEY_LAPOSTRO      },
	//end edit -MM
	{ "LSHFT",  255,    255,    ARCH_KEY_LSHIFT        },
	{ "\\",     '\\',   '|',    ARCH_KEY_SLASH         },
	{ "Z",      'z',    'Z',    ARCH_KEY_Z             },
	{ "X",      'x',    'X',    ARCH_KEY_X             },
	{ "C",      'c',    'C',    ARCH_KEY_C             },
	{ "V",      'v',    'V',    ARCH_KEY_V             },
	{ "B",      'b',    'B',    ARCH_KEY_B             },
	{ "N",      'n',    'N',    ARCH_KEY_N             },
	{ "M",      'm',    'M',    ARCH_KEY_M             },
	//edited 06/08/99 Matt Mueller - set to correct syms
	{ ",",      ',',    '<',    ARCH_KEY_COMMA         },
	{ ".",      '.',    '>',    ARCH_KEY_PERIOD        },
	{ "/",      '/',    '?',    ARCH_KEY_DIVIDE        },
	//end edit -MM
	{ "RSHFT",  255,    255,    ARCH_KEY_RSHIFT        },
	{ "PAD*",   '*',    255,    ARCH_KEY_PADMULTIPLY   },
	{ "LALT",   255,    255,    ARCH_KEY_LALT          },
	{ "SPC",    ' ',    ' ',    ARCH_KEY_SPACEBAR      },
	{ "CPSLK",  255,    255,    ARCH_KEY_CAPSLOCK      },
	{ "F1",     255,    255,    ARCH_KEY_F1            },
	{ "F2",     255,    255,    ARCH_KEY_F2            },
	{ "F3",     255,    255,    ARCH_KEY_F3            },
	{ "F4",     255,    255,    ARCH_KEY_F4            },
	{ "F5",     255,    255,    ARCH_KEY_F5            },
	{ "F6",     255,    255,    ARCH_KEY_F6            },
	{ "F7",     255,    255,    ARCH_KEY_F7            },
	{ "F8",     255,    255,    ARCH_KEY_F8            },
	{ "F9",     255,    255,    ARCH_KEY_F9            },
	{ "F10",    255,    255,    ARCH_KEY_F10           },
	{ "NMLCK",  255,    255,    ARCH_KEY_NUMLOCK       },
	{ "SCLK",   255,    255,    ARCH_KEY_SCROLLOCK     },
	{ "PAD7",   255,    255,    ARCH_KEY_PAD7          },
	{ "PAD8",   255,    255,    ARCH_KEY_PAD8          },
	{ "PAD9",   255,    255,    ARCH_KEY_PAD9          },
	{ "PAD-",   255,    255,    ARCH_KEY_PADMINUS      },
	{ "PAD4",   255,    255,    ARCH_KEY_PAD4          },
	{ "PAD5",   255,    255,    ARCH_KEY_PAD5          },
	{ "PAD6",   255,    255,    ARCH_KEY_PAD6          },
	{ "PAD+",   255,    255,    ARCH_KEY_PADPLUS       },
	{ "PAD1",   255,    255,    ARCH_KEY_PAD1          },
	{ "PAD2",   255,    255,    ARCH_KEY_PAD2          },
	{ "PAD3",   255,    255,    ARCH_KEY_PAD3          },
	{ "PAD0",   255,    255,    ARCH_KEY_PAD0          },
	{ "PAD.",   255,    255,    ARCH_KEY_PADPERIOD     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "F11",    255,    255,    ARCH_KEY_F11           },
	{ "F12",    255,    255,    ARCH_KEY_F12           },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - add pause ability
	{ "PAUSE",  255,    255,    ARCH_KEY_PAUSE         },
	//end edit -MM
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "PAD",   255,    255,    ARCH_KEY_PADENTER      },
	//end edit -MM
	//edited 06/08/99 Matt Mueller - set to correct sym
	{ "RCTRL",  255,    255,    ARCH_KEY_RCTRL         },
	//end edit -MM
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "PAD/",   255,    255,    ARCH_KEY_PADDIVIDE     },
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - add printscreen ability
	{ "PRSCR",  255,    255,    ARCH_KEY_PRINT_SCREEN  },
	//end edit -MM
	{ "RALT",   255,    255,    ARCH_KEY_RALT          },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "HOME",   255,    255,    ARCH_KEY_HOME          },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "UP",		255,    255,    ARCH_KEY_UP            },
	//end edit -MM
	{ "PGUP",   255,    255,    ARCH_KEY_PAGEUP        },
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "LEFT",	255,    255,    ARCH_KEY_LEFT          },
	//end edit -MM
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "RIGHT",	255,    255,    ARCH_KEY_RIGHT         },
	//end edit -MM
	{ "",       255,    255,    -1                     },
	//edited 06/08/99 Matt Mueller - set to correct key_text
	{ "END",    255,    255,    ARCH_KEY_END           },
	//end edit -MM
	{ "DOWN",	255,    255,    ARCH_KEY_DOWN          },
	{ "PGDN",	255,    255,    ARCH_KEY_PAGEDOWN      },
	{ "INS",	255,    255,    ARCH_KEY_INSERT        },
	{ "DEL",	255,    255,    ARCH_KEY_DELETE        },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
	{ "",       255,    255,    -1                     },
};

char *key_text[256];

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
//16 repeats per sec.
#define REPEAT_RATE (F1_0/16)
//500 ms for first repeat (subtract repeat rate here for easier runtime calculations.)
#define REPEAT_DELAY (F1_0/2 - REPEAT_RATE)
void generic_key_handler(int event_key,int key_state)
{
	ubyte state;
	int i,keycode;
	Key_info *key;
	unsigned char temp;
	fix curtime=timer_get_fixed_seconds();

	//=====================================================
	//Here a translation from arch/* keycodes to dos-ish keycodes
	//=====================================================

	for (i = 0; i < 256 ; i++) {

		keycode=i;

		key = &(key_data.keys[keycode]);
		if (event_key == -2)
			state = key->last_state;
		else if (key_properties[keycode].sym == event_key){
			state = key_state;
			i=256;//if we find it, we don't need to look anymore
		}else
			continue;
//			state = key->last_state;

		if ( key->last_state == state )	{
			if (state){
				if (keyd_repeat) {
					if ((!keyd_fake_repeat) || (key->timewentdown+REPEAT_DELAY+REPEAT_RATE*key->counter<curtime)){
						key->counter++;
						keyd_last_pressed = keycode;
						keyd_time_when_last_pressed = curtime;
					}else
						continue;
				}else
					continue;
			}
		} else {
			if (state)	{
				keyd_last_pressed = keycode;
				keyd_pressed[keycode] = 1;
				key->downcount++;
				key->state = 1;
				key->timewentdown = keyd_time_when_last_pressed = curtime;
				key->counter = 1;
			} else {
				keyd_pressed[keycode] = 0;
				keyd_last_released = keycode;
				key->upcount++;
				key->state = 0;
				key->counter = 0;
				key->timehelddown += curtime - key->timewentdown;
			}
		}
		if ( state ) {
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

void key_poll(void)
{
	if (!Installed) key_init();
	arch_key_poll();
	if (keyd_fake_repeat)
		generic_key_handler(-2,0);//check for repeated keys.
}

void key_close()
{
	arch_key_close();
	Installed = 0;
}

void key_init()
{
	int i;

	if (Installed) return;

	keyd_fake_repeat = 0;//do before arch_key_init

	arch_key_init();

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

	arch_key_flush();

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

	key_poll();
	if (key_data.keytail!=key_data.keyhead)
		is_one_waiting = 1;
	return is_one_waiting;
}

int key_inkey()
{
	int key = 0;

	key_poll();
	if (key_data.keytail!=key_data.keyhead) {
		key = key_data.keybuffer[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}
#ifdef SUPPORTS_NICEFPS
	//added 9/3/98 by Matt Mueller to free cpu time instead of hogging during menus and such
	else d_delay(1);
	//end addition - Matt Mueller
#endif

	return key;
}


int key_inkey_time(fix * time)
{
	int key = 0;

	key_poll();
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

	key_poll();
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

	while (!key_checkch()){
		dummy++;
#ifdef SUPPORTS_NICEFPS
		d_delay(1);
#endif
	}
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

	if ((scancode<0)|| (scancode>255)) return 0;
	key_poll();

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
	key_poll();
	if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.keys[scancode].downcount;
	key_data.keys[scancode].downcount = 0;

	return n;
}

unsigned int key_up_count(int scancode)
{
	int n;
	key_poll();
	if ((scancode<0)|| (scancode>255)) return 0;

	n = key_data.keys[scancode].upcount;
	key_data.keys[scancode].upcount = 0;

	return n;
}


