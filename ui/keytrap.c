/* $Id: keytrap.c,v 1.2 2004-12-19 15:21:11 btb Exp $ */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "key.h"


UI_GADGET_KEYTRAP * ui_add_gadget_keytrap( UI_WINDOW * wnd, int key_to_trap, int (*function_to_call)(void)  )
{
	UI_GADGET_KEYTRAP * keytrap;

	keytrap = (UI_GADGET_KEYTRAP *)ui_gadget_add( wnd, 8, 0, 0, 0, 0 );
	keytrap->parent = (UI_GADGET *)keytrap;

	keytrap->trap_key = key_to_trap;
	keytrap->user_function = function_to_call;

	return keytrap;

}

void ui_keytrap_do( UI_GADGET_KEYTRAP * keytrap, int keypress )
{
	int result;

	if ( keypress == keytrap->trap_key )
	{
		result = keytrap->user_function();
	}
}
