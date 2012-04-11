/* $Id: keypress.c,v 1.1.1.1 2006/03/17 19:52:24 zicodxx Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef RCS
static char rcsid[] = "$Id: keypress.c,v 1.1.1.1 2006/03/17 19:52:24 zicodxx Exp $";
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "ui.h"
#include "key.h"
#include "window.h"

char * KeyDesc[256] = {         \
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
"","","","","","","" };




void GetKeyDescription( char * text, int keypress )
{
	char Ctrl[10];
	char Alt[10];
	char Shift[10];

	if (keypress & KEY_CTRLED)
		strcpy( Ctrl, "{Ctrl}");
	else
		strcpy( Ctrl, "");

	if (keypress & KEY_ALTED)
		strcpy( Alt, "{Alt}");
	else
		strcpy( Alt, "");

	if (keypress & KEY_SHIFTED)
		strcpy( Shift, "{Shift}");
	else
		strcpy( Shift, "");

	sprintf( text, "%s%s%s%s", Ctrl, Alt, Shift, KeyDesc[keypress & 255 ]  );
}


int DecodeKeyText( char * text )
{
	int i, code = 0;

	if (strstr(text, "{Ctrl}") )
		code |= KEY_CTRLED;
	if (strstr(text, "{Alt}") )
		code |= KEY_ALTED;
	if (strstr(text, "{Shift}") )
		code |= KEY_SHIFTED;

	for (i=0; i<256; i++ )
	{
		if ( strlen(KeyDesc[i])>0 && strstr(text, KeyDesc[i]) )
		{
			code |= i;
			return code;
		}
	}
	return -1;
}


typedef struct key_dialog
{
	UI_GADGET_BUTTON *DoneButton;
	char text[100];
} key_dialog;

static int key_dialog_handler(UI_DIALOG *dlg, d_event *event, key_dialog *k)
{
	int keypress = 0;
	int rval = 0;
	
	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	else if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dprintf_at( dlg, 10, 100, "%s     ", k->text  );
		return 1;
	}

	if (keypress > 0)
	{
		
		GetKeyDescription( k->text, keypress );
		rval = 1;
	}
	
	if (GADGET_PRESSED(k->DoneButton))
	{
		ui_close_dialog(dlg);
		rval = 1;
	}
	
	return rval;
}

int GetKeyCode(char * text)
{
	UI_DIALOG * dlg;
	window *wind;
	key_dialog k;

	text = text;

	dlg = ui_create_dialog( 200, 200, 400, 200, DF_DIALOG | DF_MODAL, (int (*)(UI_DIALOG *, d_event *, void *))key_dialog_handler, &k );

	k.DoneButton = ui_add_gadget_button( dlg, 170, 165, 60, 25, "Ok", NULL );
	strcpy(k.text, "");

	ui_gadget_calc_keys(dlg);

	//key_flush();

	dlg->keyboard_focus_gadget = (UI_GADGET *)k.DoneButton;
	wind = ui_dialog_get_window(dlg);

	while (window_exists(wind))
		event_process();

	return 0;
}

