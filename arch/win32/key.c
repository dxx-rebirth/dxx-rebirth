#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <dinput.h>
//#include "inferno.h"
#include "fix.h"
#include "timer.h"
#include "key.h"

extern void PumpMessages(void);

// These are to kludge up a bit my slightly broken GCC directx port.
#ifndef E_FAIL
#define E_FAIL (HRESULT)0x80004005L
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(a) ((HRESULT)(a) >= 0)
#endif
#ifndef S_OK
#define S_OK 0
#define S_FALSE 1
#endif

extern void PumpMessages(void);

volatile int keyd_time_when_last_pressed;
volatile unsigned char 	keyd_last_pressed;
volatile unsigned char 	keyd_last_released;
volatile unsigned char	keyd_pressed [256];
unsigned char 		keyd_repeat;
unsigned char WMKey_Handler_Ready=0;


fix g_rgtimeDown [256];
fix g_rgtimeElapsed [256];
ULONG g_rgcDowns [256];
ULONG g_rgcUps [256];

char ascii_table[256];
char shifted_ascii_table[256];

LPDIRECTINPUT g_lpdi;
LPDIRECTINPUTDEVICE g_lpdidKeybd;
extern HWND g_hWnd;

typedef int KEYCODE;

#define KEY_BUFFER_SIZE 16

ULONG g_rgbKeyQueue [KEY_BUFFER_SIZE];
fix g_rgtimeQueue [KEY_BUFFER_SIZE];
ULONG g_rgbPressedQueue [KEY_BUFFER_SIZE];

ULONG iQueueStart = 0;
ULONG iQueueEnd = 0;

KEYCODE ShiftKeyCode (KEYCODE kcKey)
{
       KEYCODE kcShifted;

       if (keyd_pressed [kcKey])
       {
               kcShifted = kcKey;

               // the key is down
               if (keyd_pressed [KEY_LSHIFT] || keyd_pressed [KEY_RSHIFT])
               {
                       kcShifted |= KEY_SHIFTED;
               }
               if (keyd_pressed [KEY_LCTRL] || keyd_pressed [KEY_RCTRL])
               {
                       kcShifted |= KEY_CTRLED;
               }
               if (keyd_pressed [KEY_LALT] || keyd_pressed [KEY_RALT])
               {
                       kcShifted |= KEY_ALTED;
               }
       }
       else
       {
               // the key is UP!
               kcShifted = 0;
       }
       return kcShifted;
}

BOOL SpaceInBuffer ()
{
	if (iQueueStart == 0)
	{
		return iQueueEnd != KEY_BUFFER_SIZE - 1;
	}
	else
	{
		return iQueueEnd != iQueueStart - 1;
	}
}

void FlushQueue (void)
{
	iQueueStart = iQueueEnd = 0;
}

void PushKey (ULONG kcKey, fix timeDown)
{
	if (SpaceInBuffer ())
	{
                g_rgbKeyQueue [iQueueEnd] = ShiftKeyCode(kcKey);
		g_rgtimeQueue [iQueueEnd] = timeDown;

		if (iQueueEnd ++ == KEY_BUFFER_SIZE)
		{
			iQueueEnd = 0;
		}
	}
}

BOOL PopKey (KEYCODE *piKey, fix *ptimeDown)
{
	if (iQueueEnd != iQueueStart)
	{
		// there are items in the queue

		if (piKey != NULL)
		{
			*piKey = g_rgbKeyQueue [iQueueStart];
		}
		if (ptimeDown != NULL)
		{
			*ptimeDown = g_rgbPressedQueue [iQueueStart];
		}

		if (iQueueStart ++ == KEY_BUFFER_SIZE)
		{
			iQueueStart = 0;
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static BOOL EnsureInit (void)
{
	if (g_lpdidKeybd == NULL)
	{
		key_init ();
	}

	return g_lpdidKeybd != NULL;
}

void key_close(void);

void key_init()
{
	HRESULT hr;
	// my kingdom, my kingdom for C++...
	if (SUCCEEDED (hr = DirectInputCreate (GetModuleHandle (NULL), DIRECTINPUT_VERSION, &g_lpdi, NULL)))
	{
               if (SUCCEEDED (hr = IDirectInput_CreateDevice (g_lpdi, (void *)&GUID_SysKeyboard, &g_lpdidKeybd, NULL)))
		{
			DIPROPDWORD dipdw;

			dipdw.diph.dwSize = sizeof (DIPROPDWORD);
			dipdw.diph.dwHeaderSize = sizeof (DIPROPHEADER);
			dipdw.diph.dwObj = 0;
			dipdw.diph.dwHow = DIPH_DEVICE;
			dipdw.dwData = 40;

			if (SUCCEEDED (hr = IDirectInputDevice_SetDataFormat (g_lpdidKeybd, &c_dfDIKeyboard)) &&
								SUCCEEDED (hr = IDirectInputDevice_SetCooperativeLevel (g_lpdidKeybd, g_hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) &&
				SUCCEEDED (hr = IDirectInputDevice_SetProperty (g_lpdidKeybd, DIPROP_BUFFERSIZE, &dipdw.diph)) &&
				SUCCEEDED (hr = IDirectInputDevice_Acquire (g_lpdidKeybd)))
			{
				// fine
				WMKey_Handler_Ready = 1;

				// Clear the keyboard array
				key_flush();

				atexit(key_close);
			}
			else
			{
				IDirectInputDevice_Release (g_lpdidKeybd);
				g_lpdidKeybd = NULL;
			}
		}
	}
}

void key_close(void)
{
	WMKey_Handler_Ready = 0;
	if (g_lpdidKeybd != NULL)
	{
		IDirectInputDevice_Unacquire (g_lpdidKeybd);
		IDirectInputDevice_Release (g_lpdidKeybd);
		g_lpdidKeybd = NULL;
	}
	if (g_lpdi != NULL)
	{
		IDirectInput_Release (g_lpdi);
		g_lpdi = NULL;
	}
}

HRESULT ReadKey (DIDEVICEOBJECTDATA *pdidod)
{
	ULONG cElements = 1;
	HRESULT hr;
	if (g_lpdidKeybd == NULL)
		return E_FAIL;
	hr = IDirectInputDevice_Acquire (g_lpdidKeybd);
	if (SUCCEEDED (hr))
	{
		hr = IDirectInputDevice_GetDeviceData (
			g_lpdidKeybd,
			sizeof (*pdidod),
			pdidod,
                        (int *) &cElements,
			0);
		if (SUCCEEDED (hr) && cElements != 1)
			hr = E_FAIL;
	}

	return hr;
}

void UpdateState (DIDEVICEOBJECTDATA *pdidod)
{
	KEYCODE kcKey = pdidod->dwOfs;

	fix timeNow = timer_get_fixed_seconds ();

	if (pdidod->dwData & 0x80)
	{
		keyd_pressed [kcKey] = 1;
		keyd_last_pressed = kcKey;
                g_rgtimeDown [kcKey] = keyd_time_when_last_pressed = timeNow;
                g_rgcDowns [kcKey] ++;
		PushKey (kcKey, keyd_time_when_last_pressed);
	}
	else
	{
		keyd_pressed [kcKey] = 0;
		keyd_last_released = kcKey;
                g_rgcUps [kcKey] ++;
		g_rgtimeElapsed [kcKey] = timeNow - g_rgtimeDown [kcKey];
	}
}

void keyboard_handler()
{
//        static int peekmsgcount = 0;
	DIDEVICEOBJECTDATA didod;
	while (SUCCEEDED (ReadKey (&didod)))
	{
		UpdateState (&didod);
		
		//added 02/20/99 by adb to prevent message overflow
		//(this should probably go somewhere else...)
//              if (++peekmsgcount == 64) // 64 = wild guess...
//              {
//                      peekmsgcount = 0;
//              }
		//end additions
	}
	PumpMessages();
}



void key_flush()
{
	if (EnsureInit ())
	{
		ULONG cElements = INFINITE;
		ULONG kcKey;
//                HRESULT hr =
                               IDirectInputDevice_GetDeviceData (
                        g_lpdidKeybd,
			sizeof (DIDEVICEOBJECTDATA),
			NULL,
                        (int *) &cElements,
			0);

		for (kcKey = 0; kcKey < 256; kcKey ++)
		{
			g_rgtimeElapsed [kcKey] = 0;
			g_rgcDowns [kcKey] = 0;
			g_rgcUps [kcKey] = 0;
			keyd_pressed [kcKey] = 0;
		}

		FlushQueue ();
	}
}

int key_getch()
{
	KEYCODE kcKey;
		keyboard_handler();
	while (!PopKey (&kcKey, NULL))
	{
		keyboard_handler ();
	}

	return kcKey;
}

KEYCODE key_inkey_time(fix * pTime)
{
	KEYCODE kcKey = 0;

	keyboard_handler ();

	PopKey (&kcKey, pTime);

	return kcKey;
}

KEYCODE key_inkey ()
{
	return key_inkey_time (NULL);
}

KEYCODE key_peekkey ()
{
		keyboard_handler();
	return 0;
}

int key_checkch()
{
  return 1; // FIXME
} 

unsigned char key_to_ascii(KEYCODE kcKey )
{
	BOOL bShifted = kcKey & KEY_SHIFTED;
	unsigned char ch;

        kcKey &= ~KEY_SHIFTED;

	if (kcKey <= 127)
	{
		if (bShifted)
		{
                        ch = shifted_ascii_table [kcKey & 0xff];
		}
		else
		{
                        ch = ascii_table [kcKey & 0xff];
		}
	}
	else
	{
		ch = 255;
	}

	return ch;
}

// Returns the number of seconds this key has been down since last call.
fix key_down_time(KEYCODE kcKey)
{
	ULONG timeElapsed;
		keyboard_handler();
	if ((kcKey<0) || (kcKey>127)) return 0;

	if (keyd_pressed [kcKey])
	{
		fix timeNow = timer_get_fixed_seconds ();
		timeElapsed = timeNow - g_rgtimeDown [kcKey];
		g_rgtimeDown [kcKey] = timeNow;
	}
	else
	{
		timeElapsed = g_rgtimeElapsed [kcKey];
		g_rgtimeElapsed [kcKey] = 0;
	}

	return timeElapsed;;
}


unsigned int key_down_count(KEYCODE kcKey)
{
	int n;
		keyboard_handler();
	if ((kcKey < 0) || (kcKey > 127))
	{
		n = 0;
	}
	else
	{
		n = g_rgcDowns [kcKey];
		g_rgcDowns [kcKey] = 0;
	}

	return n;
}

unsigned int key_up_count(KEYCODE kcKey)
{
	int n;

		keyboard_handler();
	if ((kcKey < 0) || (kcKey > 127))
	{
		n = 0;
	}
	else
	{
		n = g_rgcUps [kcKey];
		g_rgcUps [kcKey] = 0;
	}

	return n;
}


#define ___ ((char) 255)
char ascii_table[256] =
{
___, ___, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', ___, ___,
'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', ___, ___, 'a', 's',
'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`', ___,'\\', 'z', 'x', 'c', 'v',
'b', 'n', 'm', ',', '.', '/', ___, '*', ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, '7', '8', '9', '-', '4', '5', '6', '+', '1',
'2', '3', '0', '.', ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
};

char shifted_ascii_table[256] =
{
___, ___, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', ___, ___,
'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', ___, ___, 'A', 'S',
'D', 'F', 'G', 'H', 'J', 'K', 'L', ':','\"', '~', ___, '|', 'Z', 'X', 'C', 'V',
'B', 'N', 'M', '<', '>', '?', ___, '*', ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, '&', '*', '(', '_', '$', '%', '^', '=', '!',
'@', '#', ')', '>', ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___, ___,
};
#undef ___

char * key_text[256] = {
"","ESC","1","2","3","4","5","6","7","8","9","0","-",
"=","BSPC","TAB","Q","W","E","R","T","Y","U","I","O",
"P","[","]","É","LCTRL","A","S","D","F",
"G","H","J","K","L",";","'","`",
"LSHFT","\\","Z","X","C","V","B","N","M",",",
".","/","RSHFT","PAD*","LALT","SPC",
"CPSLK","F1","F2","F3","F4","F5","F6","F7","F8","F9",
"F10","NMLCK","SCLK","PAD7","PAD8","PAD9","PAD-",
"PAD4","PAD5","PAD6","PAD+","PAD1","PAD2","PAD3","PAD0",
"PAD.","","","","F11","F12","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","",
"PADÉ","RCTRL","","","","","","","","","","","","","",
"","","","","","","","","","","PAD/","","","RALT","",
"","","","","","","","","","","","","","HOME","Ç","PGUP",
"","Å","","","","END","Ä","PGDN","INS",
"DEL","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","",
"","","","","","","" };
#if 0 // what was this? -- adb
"","","","HOME","UP","PGUP","","LEFT","","RGHT","","END","DOWN","PGDN","INS","DEL",
"","Å","","","","","?","","",
"","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","",
"","","","","","","" };
#endif
