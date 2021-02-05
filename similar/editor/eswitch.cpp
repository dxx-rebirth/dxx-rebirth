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
 * Editor switch functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"
#include "editor/kdefs.h"
#include "eswitch.h"
#include "segment.h"
#include "dxxerror.h"
#include "event.h"
#include "wall.h"
#include "medwall.h"
#include "textures.h"
#include "texmerge.h"
#include "medrobot.h"
#include "key.h"
#include "ehostage.h"
#include "centers.h"
#include "piggy.h"
#include "u_mem.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include <memory>

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
#define NUM_TRIGGER_FLAGS 10

namespace dsx {

namespace {

struct trigger_dialog : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	std::unique_ptr<UI_GADGET_USERBOX> wallViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, remove_trigger, bind_wall, bind_matcen, enable_all_triggers;
#if defined(DXX_BUILD_DESCENT_I)
	std::array<std::unique_ptr<UI_GADGET_CHECKBOX>, NUM_TRIGGER_FLAGS> triggerFlag;
#endif
	int old_trigger_num;
	virtual window_event_result callback_handler(const d_event &) override;
};

static trigger_dialog *MainWindow;

#if defined(DXX_BUILD_DESCENT_I)
//-----------------------------------------------------------------
// Adds a trigger to wall, and returns the trigger number. 
// If there is a trigger already present, it returns the trigger number. (To be replaced)
static trgnum_t add_trigger(trigger_array &Triggers, fvcvertptr &vcvertptr, wall_array &Walls, const shared_segment &seg, const unsigned side)
{
	trgnum_t trigger_num = Triggers.get_count();

	Assert(trigger_num < MAX_TRIGGERS);
	if (trigger_num>=MAX_TRIGGERS) return trigger_none;

	auto wall_num = seg.sides[side].wall_num;
	wall *wp;
	auto &vmwallptr = Walls.vmptr;
	if (wall_num == wall_none) {
		wall_add_to_markedside(vcvertptr, Walls, WALL_OPEN);
		wall_num = seg.sides[side].wall_num;
		wp = vmwallptr(wall_num);
		// Set default values first time trigger is added
	} else {
		auto &w = *vmwallptr(wall_num);
		if (w.trigger != trigger_none)
			return w.trigger;

		wp = &w;
		// Create new trigger.
	}
	wp->trigger = trigger_num;
	auto &t = *Triggers.vmptr(trigger_num);
	t.flags = {};
	t.value = F1_0*5;
	t.num_links = 0;
	Triggers.set_count(trigger_num + 1);
	return trigger_num;
}		

//-----------------------------------------------------------------
// Adds a specific trigger flag to Markedsegp/Markedside if it is possible.
// Automatically adds flag to Connectside if possible unless it is a control trigger.
// Returns 1 if trigger flag added.
// Returns 0 if trigger flag cannot be added.
static int trigger_flag_Markedside(const TRIGGER_FLAG flag, const int value)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (!Markedsegp) {
		editor_status("No Markedside.");
		return 0;
	}

	// If no child on Markedside return
	shared_segment &markedseg = Markedsegp;
	if (!IS_CHILD(markedseg.children[Markedside])) return 0;

	// If no wall just return
	const auto wall_num = markedseg.sides[Markedside].wall_num;
	if (!value && wall_num == wall_none) return 0;
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto trigger_num = value ? add_trigger(Triggers, vcvertptr, Walls, markedseg, Markedside) : vcwallptr(wall_num)->trigger;

	if (trigger_num == trigger_none) {
		editor_status(value ? "Cannot add trigger at Markedside." : "No trigger at Markedside.");
		return 0;
	}

	auto &vmtrgptr = Triggers.vmptr;
	auto &flags = vmtrgptr(trigger_num)->flags;
 	if (value)
		flags |= flag;
	else
		flags &= ~flag;

	return 1;
}
#endif

static int bind_matcen_to_trigger() {

	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}

	const auto wall_num = Markedsegp->shared_segment::sides[Markedside].wall_num;
	if (wall_num == wall_none) {
		editor_status("No wall at Markedside.");
		return 0;
	}
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto trigger_num = vcwallptr(wall_num)->trigger;
	if (trigger_num == trigger_none) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (!(Cursegp->special & SEGMENT_IS_ROBOTMAKER))
	{
		editor_status("No Matcen at Cursegp.");
		return 0;
	}

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	const auto &&t = vmtrgptr(trigger_num);
	const auto link_num = t->num_links;
	for (int i=0;i<link_num;i++)
		if (Cursegp == t->seg[i])
		{
			editor_status("Matcen already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	t->seg[link_num] = Cursegp;
	t->num_links++;

	editor_status("Matcen linked to trigger");

	return 1;
}

}

int bind_wall_to_trigger() {
	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}
	const auto wall_num = Markedsegp->shared_segment::sides[Markedside].wall_num;
	if (wall_num == wall_none) {
		editor_status("No wall at Markedside.");
		return 0;
	}
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto trigger_num = vcwallptr(wall_num)->trigger;
	if (trigger_num == trigger_none) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (Cursegp->shared_segment::sides[Curside].wall_num == wall_none)
	{
		editor_status("No wall at Curside.");
		return 0;
	}

	if ((Cursegp==Markedsegp) && (Curside==Markedside)) {
		editor_status("Cannot bind wall to itself.");
		return 0;
	}

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	const auto &&t = vmtrgptr(trigger_num);
	const auto link_num = t->num_links;
	for (int i=0;i<link_num;i++)
		if (Cursegp == t->seg[i] && Curside == t->side[i])
		{
			editor_status("Curside already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	t->seg[link_num] = Cursegp;
	t->side[link_num] = Curside;
	t->num_links++;

	editor_status("Wall linked to trigger");

	return 1;
}

int remove_trigger_num(int trigger_num)
{
	if (trigger_num != trigger_none)
	{
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
		auto r = partial_range(Triggers, static_cast<unsigned>(trigger_num), Triggers.get_count());
		Triggers.set_count(Triggers.get_count() - 1);
		std::move(std::next(r.begin()), r.end(), r.begin());
	
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vmwallptr = Walls.vmptr;
		range_for (const auto &&w, vmwallptr)
		{
			auto &trigger = w->trigger;
			if (trigger == trigger_num)
				trigger = trigger_none;	// a trigger can be shared by multiple walls
			else if (trigger > trigger_num && trigger != trigger_none)
				--trigger;
		}

		return 1;
	}

	editor_status("No trigger to remove");
	return 0;
}

unsigned remove_trigger(shared_segment &seg, const unsigned side)
{    	
	const auto wall_num = seg.sides[side].wall_num;
	if (wall_num == wall_none)
	{
		return 0;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	return remove_trigger_num(vcwallptr(wall_num)->trigger);
}

static int trigger_remove()
{
	remove_trigger(Markedsegp, Markedside);
	Update_flags = UF_WORLD_CHANGED;
	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
static int trigger_turn_all_ON()
{
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	range_for (const auto t, vmtrgptr)
		t->flags &= ~trigger_behavior_flags::disabled;
	return 1;
}
#endif

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the trigger dialog box
//-------------------------------------------------------------------------
int do_trigger_dialog()
{
	if (!Markedsegp) {
		editor_status("Trigger requires Marked Segment & Side.");
		return 0;
	}

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows.	
	robot_close_window();
	close_wall_window();
	close_centers_window();
#if defined(DXX_BUILD_DESCENT_I)
	hostage_close_window();
#endif

	// Open a window with a quit button
	MainWindow = window_create<trigger_dialog>(TMAPBOX_X + 20, TMAPBOX_Y + 20, 765 - TMAPBOX_X, 545 - TMAPBOX_Y, DF_DIALOG);
	return 1;
}

static window_event_result trigger_dialog_created(trigger_dialog *const t)
{
	// These are the checkboxes for each door flag.
	int i = 44;
#if defined(DXX_BUILD_DESCENT_I)
	t->triggerFlag[0] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Door Control");  	i+=22;
	t->triggerFlag[1] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Shield damage"); 	i+=22;
	t->triggerFlag[2] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Energy drain");		i+=22;
	t->triggerFlag[3] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Exit");					i+=22;
	t->triggerFlag[4] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "One-shot");			i+=22;
	t->triggerFlag[5] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Illusion ON");		i+=22;
	t->triggerFlag[6] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Illusion OFF");		i+=22;
	t->triggerFlag[7] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Unused");			i+=22;
	t->triggerFlag[8] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Matcen Trigger"); 	i+=22;
	t->triggerFlag[9] = ui_add_gadget_checkbox(*t, 22, i, 16, 16, 0, "Secret Exit"); 		i+=22;
#endif

	t->quitButton = ui_add_gadget_button(*t, 20, i, 48, 40, "Done", nullptr);
																				 
	// The little box the wall will appear in.
	t->wallViewBox = ui_add_gadget_userbox(*t, 155, 5, 64, 64 );

	// A bunch of buttons...
	i = 80;
	t->remove_trigger = ui_add_gadget_button(*t, 155, i, 140, 26, "Remove Trigger", trigger_remove); i += 29;
	t->bind_wall = ui_add_gadget_button(*t, 155, i, 140, 26, "Bind Wall", bind_wall_to_trigger); i += 29;
	t->bind_matcen = ui_add_gadget_button(*t, 155, i, 140, 26, "Bind Matcen", bind_matcen_to_trigger); i += 29;
#if defined(DXX_BUILD_DESCENT_II)
	t->enable_all_triggers = ui_add_gadget_button(*t, 155, i, 140, 26, "All Triggers ON", trigger_turn_all_ON); i += 29;
#endif

	t->old_trigger_num = -2;		// Set to some dummy value so everything works ok on the first frame.
	return window_event_result::handled;
}

void close_trigger_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog(*std::exchange(MainWindow, nullptr));
	}
}

window_event_result trigger_dialog::callback_handler(const d_event &event)
{
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return trigger_dialog_created(this);
		case EVENT_WINDOW_CLOSE:
			MainWindow = NULL;
			return window_event_result::ignored;
		default:
			break;
	}
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;

	Assert(MainWindow != NULL);
	if (!Markedsegp) {
		return window_event_result::close;
	}

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	msmusegment &&markedseg = Markedsegp;
	const auto Markedwall = markedseg.s.sides[Markedside].wall_num;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto trigger_num = (Markedwall != wall_none) ? vcwallptr(Markedwall)->trigger : trigger_none;

	if (old_trigger_num != trigger_num)
	{
		if (trigger_num != trigger_none)
		{
#if defined(DXX_BUILD_DESCENT_I)
			auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
			auto &vctrgptr = Triggers.vcptr;
			const auto &&trig = vctrgptr(trigger_num);

  			ui_checkbox_check(triggerFlag[0].get(), trig->flags & TRIGGER_CONTROL_DOORS);
 			ui_checkbox_check(triggerFlag[1].get(), trig->flags & TRIGGER_SHIELD_DAMAGE);
 			ui_checkbox_check(triggerFlag[2].get(), trig->flags & TRIGGER_ENERGY_DRAIN);
 			ui_checkbox_check(triggerFlag[3].get(), trig->flags & TRIGGER_EXIT);
 			ui_checkbox_check(triggerFlag[4].get(), trig->flags & TRIGGER_ONE_SHOT);
 			ui_checkbox_check(triggerFlag[5].get(), trig->flags & TRIGGER_ILLUSION_ON);
 			ui_checkbox_check(triggerFlag[6].get(), trig->flags & TRIGGER_ILLUSION_OFF);
			ui_checkbox_check(triggerFlag[7].get(), 0);
 			ui_checkbox_check(triggerFlag[8].get(), trig->flags & TRIGGER_MATCEN);
 			ui_checkbox_check(triggerFlag[9].get(), trig->flags & TRIGGER_SECRET_EXIT);
#endif
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the cooresponding wall flag.
	//------------------------------------------------------------
	if (IS_CHILD(markedseg.s.children[Markedside]))
	{
#if defined(DXX_BUILD_DESCENT_I)
		rval = window_event_result::handled;
		if (GADGET_PRESSED(triggerFlag[0].get())) 
			trigger_flag_Markedside(TRIGGER_CONTROL_DOORS, triggerFlag[0]->flag); 
		else if (GADGET_PRESSED(triggerFlag[1].get()))
			trigger_flag_Markedside(TRIGGER_SHIELD_DAMAGE, triggerFlag[1]->flag); 
		else if (GADGET_PRESSED(triggerFlag[2].get()))
			trigger_flag_Markedside(TRIGGER_ENERGY_DRAIN, triggerFlag[2]->flag); 
		else if (GADGET_PRESSED(triggerFlag[3].get()))
			trigger_flag_Markedside(TRIGGER_EXIT, triggerFlag[3]->flag); 
		else if (GADGET_PRESSED(triggerFlag[4].get()))
			trigger_flag_Markedside(TRIGGER_ONE_SHOT, triggerFlag[4]->flag); 
		else if (GADGET_PRESSED(triggerFlag[5].get()))
			trigger_flag_Markedside(TRIGGER_ILLUSION_ON, triggerFlag[5]->flag); 
		else if (GADGET_PRESSED(triggerFlag[6].get()))
			trigger_flag_Markedside(TRIGGER_ILLUSION_OFF, triggerFlag[6]->flag);
		else if (GADGET_PRESSED(triggerFlag[7].get()))
		{
		}
		else if (GADGET_PRESSED(triggerFlag[8].get())) 
			trigger_flag_Markedside(TRIGGER_MATCEN, triggerFlag[8]->flag);
		else if (GADGET_PRESSED(triggerFlag[9].get())) 
			trigger_flag_Markedside(TRIGGER_SECRET_EXIT, triggerFlag[9]->flag);
		else
#endif
			rval = window_event_result::ignored;

	} else
	{
#if defined(DXX_BUILD_DESCENT_I)
		range_for (auto &i, triggerFlag)
			ui_checkbox_check(i.get(), 0);
#endif
	}

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		gr_set_current_canvas( wallViewBox->canvas );
		auto &canvas = *grd_curcanv;

		const auto wall_num = markedseg.s.sides[Markedside].wall_num;
		if (wall_num == wall_none || vcwallptr(wall_num)->trigger == trigger_none)
			gr_clear_canvas(canvas, CBLACK);
		else {
			auto &us = markedseg.u.sides[Markedside];
			if (us.tmap_num2 != texture2_value::None) 
			{
				gr_ubitmap(canvas, texmerge_get_cached_bitmap(us.tmap_num, us.tmap_num2));
			} else {
				const auto tmap_num = us.tmap_num;
				if (tmap_num != texture1_value::None)
				{
					auto &t = Textures[get_texture_index(tmap_num)];
					PIGGY_PAGE_IN(t);
					gr_ubitmap(canvas, GameBitmaps[t.index]);
				} else
					gr_clear_canvas(canvas, CGREY);
			}
		}
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		if (markedseg.s.sides[Markedside].wall_num != wall_none)
		{
			ui_dprintf_at( MainWindow, 12, 6, "Trigger: %d    ", trigger_num);
		}	else {
			ui_dprintf_at( MainWindow, 12, 6, "Trigger: none ");
		}
	}
	
	if (ui_button_any_drawn || (old_trigger_num != trigger_num) )
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(quitButton.get()) || keypress == KEY_ESC)
	{
		return window_event_result::close;
	}		

	old_trigger_num = trigger_num;
	
	return rval;
}

}
