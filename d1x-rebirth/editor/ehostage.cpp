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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines for placing hostages, etc...
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "dxxerror.h"
#include "screens.h"
#include "editor/esegment.h"
#include "ehostage.h"

#include "timer.h"
#include "objpage.h"
#include "maths.h"
#include "gameseg.h"
#include "kdefs.h"
#include	"object.h"
#include "polyobj.h"
#include "game.h"
#include "powerup.h"
#include "ai.h"
#include "hostage.h"
#include "key.h"
#include "bm.h"
#include "sounds.h"
#include "centers.h"
#include "piggy.h"
#include "u_mem.h"
#include "event.h"
#include <memory>

namespace dsx {
//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------

namespace {

struct hostage_dialog : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, delete_object, new_object;
	virtual window_event_result callback_handler(const d_event &) override;
};

static hostage_dialog *MainWindow;

}

static int PlaceHostage()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int ctype,i;
	//update_due_to_new_segment();
	auto &vcvertptr = Vertices.vcptr;
	const auto cur_object_loc = compute_segment_center(vcvertptr, Cursegp);

	ctype = -1;
	for (i=0; i<Num_total_object_types; i++ )	{
		if (ObjType[i] == OL_HOSTAGE )	{
			ctype = i;	
			break;
		}
	}

	Assert( ctype != -1 );

	if (place_object(Cursegp, cur_object_loc, ctype, 0 )==0)	{
		Int3();		// Debug below
		i=place_object(Cursegp, cur_object_loc, ctype, 0 );
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the hostage dialog box
//-------------------------------------------------------------------------
int do_hostage_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows
	close_all_windows();
	
	// Open a window with a quit button
	MainWindow = window_create<hostage_dialog>(TMAPBOX_X + 10, TMAPBOX_Y + 20, 765 - TMAPBOX_X, 545 - TMAPBOX_Y, DF_DIALOG);
	return 1;
}

static window_event_result hostage_dialog_created(hostage_dialog *const h)
{
	h->quitButton = ui_add_gadget_button(*h, 20, 222, 48, 40, "Done", nullptr);
	// A bunch of buttons...
	int i = 90;
	h->delete_object = ui_add_gadget_button(*h, 155, i, 140, 26, "Delete", ObjectDelete);	i += 29;
	h->new_object = ui_add_gadget_button(*h, 155, i, 140, 26, "Create New", PlaceHostage);	i += 29;
	return window_event_result::ignored;
}

void hostage_close_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog(*std::exchange(MainWindow, nullptr));
	}
}

window_event_result hostage_dialog::callback_handler(const d_event &event)
{
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return hostage_dialog_created(this);
		case EVENT_WINDOW_CLOSE:
			MainWindow = nullptr;
			return window_event_result::ignored;
		default:
			break;
	}
	int keypress = 0;
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	Assert(MainWindow != NULL);

	//------------------------------------------------------------
	// Redraw the object in the little 64x64 box
	//------------------------------------------------------------
	if (GADGET_PRESSED(quitButton.get()) || keypress==KEY_ESC)
	{
		return window_event_result::close;
	}		
	return window_event_result::ignored;
}

}
