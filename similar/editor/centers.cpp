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
 * Dialog box stuff for control centers, material centers, etc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fuelcen.h"
#include "screens.h"
#include "inferno.h"
#include "event.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "timer.h"
#include "objpage.h"
#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"
#include	"object.h"
#include "robot.h"
#include "game.h"
#include "powerup.h"
#include "ai.h"
#include "hostage.h"
#include "eobject.h"
#include "medwall.h"
#include "eswitch.h"
#include "ehostage.h"
#include "key.h"
#include "medrobot.h"
#include "bm.h"
#include "centers.h"
#include "u_mem.h"

#include "compiler-range_for.h"
#include <memory>

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------

namespace dsx {

namespace {

struct centers_dialog : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton;
	std::array<std::unique_ptr<UI_GADGET_RADIO>, MAX_CENTER_TYPES> centerFlag;
	std::array<std::unique_ptr<UI_GADGET_CHECKBOX>, MAX_ROBOT_TYPES> robotMatFlag;
	int old_seg_num;
	virtual window_event_result callback_handler(const d_event &) override;
};

static centers_dialog *MainWindow;

}

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the centers dialog box
//-------------------------------------------------------------------------
int do_centers_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	close_trigger_window();
#if defined(DXX_BUILD_DESCENT_I)
	hostage_close_window();
#endif
	close_wall_window();
	robot_close_window();

	// Open a window with a quit button
#if defined(DXX_BUILD_DESCENT_I)
	const unsigned x = TMAPBOX_X+20;
	const unsigned width = 765-TMAPBOX_X;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned x = 20;
	const unsigned width = 740;
#endif
	MainWindow = window_create<centers_dialog>(x, TMAPBOX_Y + 20, width, 545 - TMAPBOX_Y, DF_DIALOG);
	return 1;
}

static window_event_result centers_dialog_created(centers_dialog *const c)
{
#if defined(DXX_BUILD_DESCENT_I)
	int i = 80;
#elif defined(DXX_BUILD_DESCENT_II)
	int i = 40;
#endif
	c->quitButton = ui_add_gadget_button(*c, 20, 252, 48, 40, "Done", nullptr);
	// These are the checkboxes for each door flag.
	c->centerFlag[0] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "NONE"); 			i += 24;
	c->centerFlag[1] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "FuelCen");		i += 24;
	c->centerFlag[2] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "RepairCen");	i += 24;
	c->centerFlag[3] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "ControlCen");	i += 24;
	c->centerFlag[4] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "RobotCen");		i += 24;
#if defined(DXX_BUILD_DESCENT_II)
	c->centerFlag[5] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "Blue Goal");		i += 24;
	c->centerFlag[6] = ui_add_gadget_radio(*c, 18, i, 16, 16, 0, "Red Goal");		i += 24;
#endif
	// These are the checkboxes for each robot flag.
#if defined(DXX_BUILD_DESCENT_I)
	const unsigned d = 2;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned d = 6;
#endif
	const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types;
	for (i=0; i < N_robot_types; i++)
		c->robotMatFlag[i] = ui_add_gadget_checkbox(*c, 128 + (i % d) * 92, 20 + (i / d) * 24, 16, 16, 0, Robot_names[i].data());
	c->old_seg_num = -2;		// Set to some dummy value so everything works ok on the first frame.
	return window_event_result::handled;
}

void close_centers_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog(*std::exchange(MainWindow, nullptr));
	}
}

window_event_result centers_dialog::callback_handler(const d_event &event)
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return centers_dialog_created(this);
		case EVENT_WINDOW_CLOSE:
			MainWindow = NULL;
			return window_event_result::ignored;
		default:
			break;
	}
//	int robot_flags;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;

	Assert(MainWindow != NULL);

	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;

	const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types;

	//------------------------------------------------------------
	// If we change centers, we need to reset the ui code for all
	// of the checkboxes that control the center flags.  
	//------------------------------------------------------------
	if (old_seg_num != Cursegp)
	{
		range_for (auto &i, centerFlag)
			ui_radio_set_value(*i, 0);

		Assert(Cursegp->special < MAX_CENTER_TYPES);
		ui_radio_set_value(*centerFlag[Cursegp->special], 1);

		//	Read materialization center robot bit flags
		for (unsigned i = 0, n = N_robot_types; i < n; ++i)
			ui_checkbox_check(robotMatFlag[i].get(), RobotCenters[Cursegp->matcen_num].robot_flags[i / 32] & (1 << (i % 32)));
	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the corresponding center.
	//------------------------------------------------------------

	for (unsigned i = 0; i < MAX_CENTER_TYPES; ++i)
	{
		if (GADGET_PRESSED(centerFlag[i].get()))
		{
			if ( i == 0)
				fuelcen_delete(Cursegp);
			else if (Cursegp->special != i)
			{
				fuelcen_delete(Cursegp);
				Update_flags |= UF_WORLD_CHANGED;
				Cursegp->special = i;
				fuelcen_activate(Cursegp);
			}
			rval = window_event_result::handled;
		}
	}

	for (unsigned i = 0, n = N_robot_types; i < n; ++i)
	{
		if (GADGET_PRESSED(robotMatFlag[i].get()))
		{
			auto &f = RobotCenters[Cursegp->matcen_num].robot_flags[i / 32];
			const auto mask = 1 << (i % 32);
			if (robotMatFlag[i]->flag)
				f |= mask;
			else
				f &= ~mask;
			rval = window_event_result::handled;
		}
	}
	
	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
//		int	i;
	
		ui_dprintf_at(this, 12, 6, "Seg: %3hu", static_cast<segnum_t>(Cursegp));
	}

	if (old_seg_num != Cursegp)
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(quitButton.get()) || keypress == KEY_ESC)
	{
		return window_event_result::close;
	}		

	old_seg_num = Cursegp;
	return rval;
}

}
