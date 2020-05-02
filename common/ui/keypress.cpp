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


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "ui.h"
#include "key.h"
#include "window.h"
#include "compiler-range_for.h"
#include "d_range.h"

namespace dcx {

constexpr std::array<const char *, 256> KeyDesc{{
"","{Esc}","{1}","{2}","{3}","{4}","{5}","{6}","{7}","{8}","{9}","{0}","{-}",           \
"{=}","{Backspace}","{Tab}","{Q}","{W}","{E}","{R}","{T}","{Y}","{U}","{I}","{O}",      \
"{P}","{[}","{]}","{Enter}","{LeftCtrl}","{A}","{S}","{D}","{F}",        \
"{G}","{H}","{J}","{K}","{L}","{;}","{'}","{`}",        \
"{RightShift}","{\\}","{Z}","{X}","{C}","{V}","{B}","{N}","{M}","{,}",      \
"{.}","{/}","{LeftShift}","{Pad*}","{LeftAlt}","{Spacebar}",      \
"{CapsLock}","{F1}","{F2}","{F3}","{F4}","{F5}","{F6}","{F7}","{F8}","{F9}",        \
"{F10}","{NumLock}","{ScrollLock}","{Pad7}","{Pad8}","{Pad9}","{Pad-}",   \
"{Pad4}","{Pad5}","{Pad6}","{Pad+}","{Pad1}","{Pad2}","{Pad3}","{Pad0}", \
"{Pad.}","","","","{F11}","{F12}","","","","","","","","","",         \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","",           \
"{PadEnter}","{RightCtrl}","","","","","","","","","","","","","", \
"","","","","","","","","","","{Pad/}","","","{RightAlt}","",      \
"","","","","","","","","","","","","","{Home}","{Up}","{PageUp}",     \
"","{Left}","","{Right}","","{End}","{Down}","{PageDown}","{Insert}",       \
"{Delete}","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","",""
}};




void GetKeyDescription(char (&text)[100], uint_fast32_t keypress)
{
	const char *Ctrl = keypress & KEY_CTRLED ? "{Ctrl}" : "";
	const char *Alt = keypress & KEY_ALTED ? "{Alt}" : "";
	const char *Shift = keypress & KEY_SHIFTED ? "{Shift}" : "";
	snprintf(text, sizeof(text), "%s%s%s%s", Ctrl, Alt, Shift, KeyDesc[keypress & 255 ]);
}


int DecodeKeyText( const char * text )
{
	int code = 0;

	if (strstr(text, "{Ctrl}") )
		code |= KEY_CTRLED;
	if (strstr(text, "{Alt}") )
		code |= KEY_ALTED;
	if (strstr(text, "{Shift}") )
		code |= KEY_SHIFTED;

	range_for (const int i, xrange(256u))
	{
		if ( strlen(KeyDesc[i])>0 && strstr(text, KeyDesc[i]) )
		{
			code |= i;
			return code;
		}
	}
	return -1;
}

}
