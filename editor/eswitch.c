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
#include "eswitch.h"
#include "segment.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "wall.h"
#include "medwall.h"
#include "screens.h"
#include "textures.h"
#include "texmerge.h"
#include "medrobot.h"
#include "timer.h"
#include "key.h"
#include "ehostage.h"
#include "centers.h"
#include "piggy.h"

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
#define NUM_TRIGGER_FLAGS 10

static UI_DIALOG 				*MainWindow = NULL;

typedef struct trigger_dialog
{
	UI_GADGET_USERBOX	*wallViewBox;
	UI_GADGET_BUTTON 	*quitButton;
	UI_GADGET_CHECKBOX	*triggerFlag[NUM_TRIGGER_FLAGS];
	int old_trigger_num;
} trigger_dialog;


//-----------------------------------------------------------------
// Adds a trigger to wall, and returns the trigger number. 
// If there is a trigger already present, it returns the trigger number. (To be replaced)
int add_trigger(segment *seg, short side)
{
	int trigger_num = Num_triggers;
	int wall_num = seg->sides[side].wall_num;

	Assert(trigger_num < MAX_TRIGGERS);
	if (trigger_num>=MAX_TRIGGERS) return -1;

	if (wall_num == -1) {
		wall_add_to_markedside(WALL_OPEN);
		wall_num = seg->sides[side].wall_num;
		Walls[wall_num].trigger = trigger_num;
		
		// Set default values first time trigger is added
		Triggers[trigger_num].flags = 0;
		Triggers[trigger_num].value = F1_0*5;
		Triggers[trigger_num].num_links = 0;
		Triggers[trigger_num].flags &= TRIGGER_ON;		

		Num_triggers++;
		return trigger_num;
	} else {
		if (Walls[wall_num].trigger != -1)
			return Walls[wall_num].trigger;

		// Create new trigger.
		Walls[wall_num].trigger = trigger_num;

		// Set default values first time trigger is added
		Triggers[trigger_num].flags = 0;
		Triggers[trigger_num].value = F1_0*5;
		Triggers[trigger_num].num_links = 0;
		Triggers[trigger_num].flags &= TRIGGER_ON;

		Num_triggers++;
		return trigger_num;
	}
}		

//-----------------------------------------------------------------
// Adds a specific trigger flag to Markedsegp/Markedside if it is possible.
// Automatically adds flag to Connectside if possible unless it is a control trigger.
// Returns 1 if trigger flag added.
// Returns 0 if trigger flag cannot be added.
int trigger_flag_Markedside(short flag, int value)
{
	int trigger_num; //, ctrigger_num;
	int wall_num;
	
	if (!Markedsegp) {
		editor_status("No Markedside.");
		return 0;
	}

	// If no child on Markedside return
	if (!IS_CHILD(Markedsegp->children[Markedside])) return 0;

	// If no wall just return
	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (!value && wall_num == -1) return 0;
	trigger_num = value ? add_trigger(Markedsegp, Markedside) : Walls[wall_num].trigger;

	if (trigger_num == -1) {
		editor_status(value ? "Cannot add trigger at Markedside." : "No trigger at Markedside.");
		return 0;
	}

 	if (value)
		Triggers[trigger_num].flags |= flag;
	else
		Triggers[trigger_num].flags &= ~flag;

	return 1;
}

int bind_matcen_to_trigger() {

	int wall_num, trigger_num, link_num;
	int i;

	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}

	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (wall_num == -1) {
		editor_status("No wall at Markedside.");
		return 0;
	}

	trigger_num = Walls[wall_num].trigger;	

	if (trigger_num == -1) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (!(Curseg2p->special & SEGMENT_IS_ROBOTMAKER))
	{
		editor_status("No Matcen at Cursegp.");
		return 0;
	}

	link_num = Triggers[trigger_num].num_links;
	for (i=0;i<link_num;i++)
		if (Cursegp-Segments == Triggers[trigger_num].seg[i]) {
			editor_status("Matcen already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	Triggers[trigger_num].seg[link_num] = Cursegp - Segments;
	Triggers[trigger_num].num_links++;

	editor_status("Matcen linked to trigger");

	return 1;
}


int bind_wall_to_trigger() {

	int wall_num, trigger_num, link_num;
	int i;

	if (!Markedsegp) {
		editor_status("No marked segment.");
		return 0;
	}

	wall_num = Markedsegp->sides[Markedside].wall_num;
	if (wall_num == -1) {
		editor_status("No wall at Markedside.");
		return 0;
	}

	trigger_num = Walls[wall_num].trigger;	

	if (trigger_num == -1) {
		editor_status("No trigger at Markedside.");
		return 0;
	}

	if (Cursegp->sides[Curside].wall_num == -1) {
		editor_status("No wall at Curside.");
		return 0;
	}

	if ((Cursegp==Markedsegp) && (Curside==Markedside)) {
		editor_status("Cannot bind wall to itself.");
		return 0;
	}

	link_num = Triggers[trigger_num].num_links;
	for (i=0;i<link_num;i++)
		if ((Cursegp-Segments == Triggers[trigger_num].seg[i]) && (Curside == Triggers[trigger_num].side[i])) {
			editor_status("Curside already bound to Markedside.");
			return 0;
		}

	// Error checking completed, actual binding begins
	Triggers[trigger_num].seg[link_num] = Cursegp - Segments;
	Triggers[trigger_num].side[link_num] = Curside;
	Triggers[trigger_num].num_links++;

	editor_status("Wall linked to trigger");

	return 1;
}

int remove_trigger_num(int trigger_num)
{
	if (trigger_num != -1)
	{
		int t, w;
	
		Num_triggers--;
		for (t = trigger_num; t < Num_triggers; t++)
			Triggers[t] = Triggers[t + 1];
	
		for (w = 0; w < Num_walls; w++)
		{
			if (Walls[w].trigger == trigger_num)
				Walls[w].trigger = -1;	// a trigger can be shared by multiple walls
			else if (Walls[w].trigger > trigger_num) 
				Walls[w].trigger--;
		}

		return 1;
	}

	editor_status("No trigger to remove");
	return 0;
}

int remove_trigger(segment *seg, short side)
{    	
	if (seg->sides[side].wall_num == -1)
	{
		return 0;
	}

	return remove_trigger_num(Walls[seg->sides[side].wall_num].trigger);
}


int add_trigger_control()
{
	trigger_flag_Markedside(TRIGGER_CONTROL_DOORS, 1);
	Update_flags = UF_WORLD_CHANGED;
	return 1;
}

int trigger_remove()
{
	remove_trigger(Markedsegp, Markedside);
	Update_flags = UF_WORLD_CHANGED;
	return 1;
}

int trigger_turn_all_ON()
{
	int t;

	for (t=0;t<Num_triggers;t++)
		Triggers[t].flags &= TRIGGER_ON;
	return 1;
}

int trigger_dialog_handler(UI_DIALOG *dlg, d_event *event, trigger_dialog *t);

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the trigger dialog box
//-------------------------------------------------------------------------
int do_trigger_dialog()
{
	int i;
	trigger_dialog *t;

	if (!Markedsegp) {
		editor_status("Trigger requires Marked Segment & Side.");
		return 0;
	}

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	MALLOC(t, trigger_dialog, 1);
	if (!t)
		return 0;

	// Close other windows.	
	robot_close_window();
	close_wall_window();
	close_centers_window();
	hostage_close_window();

	// Open a window with a quit button
	MainWindow = ui_create_dialog( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, DF_DIALOG, (int (*)(UI_DIALOG *, d_event *, void *))trigger_dialog_handler, t );

	// These are the checkboxes for each door flag.
	i = 44;
	t->triggerFlag[0] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Door Control" );  	i+=22;
	t->triggerFlag[1] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Shield damage" ); 	i+=22;
	t->triggerFlag[2] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Energy drain" );		i+=22;
	t->triggerFlag[3] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Exit" );					i+=22;
	t->triggerFlag[4] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "One-shot" );			i+=22;
	t->triggerFlag[5] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion ON" );		i+=22;
	t->triggerFlag[6] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion OFF" );		i+=22;
	t->triggerFlag[7] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Trigger ON" );			i+=22;
	t->triggerFlag[8] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Matcen Trigger" ); 	i+=22;
	t->triggerFlag[9] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Secret Exit" ); 		i+=22;

	t->quitButton = ui_add_gadget_button( MainWindow, 20, i, 48, 40, "Done", NULL );
																				 
	// The little box the wall will appear in.
	t->wallViewBox = ui_add_gadget_userbox( MainWindow, 155, 5, 64, 64 );

	// A bunch of buttons...
	i = 80;
//	ui_add_gadget_button( MainWindow,155,i,140, 26, "Add Door Control", add_trigger_control ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Remove Trigger", trigger_remove ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Bind Wall", bind_wall_to_trigger ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "Bind Matcen", bind_matcen_to_trigger ); i += 29;
	ui_add_gadget_button( MainWindow,155,i,140, 26, "All Triggers ON", trigger_turn_all_ON ); i += 29;

	t->old_trigger_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_trigger_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog( MainWindow );
		MainWindow = NULL;
	}
}

int trigger_dialog_handler(UI_DIALOG *dlg, d_event *event, trigger_dialog *t)
{
	int i;
	short Markedwall, trigger_num;
	int keypress = 0;
	int rval = 0;

	Assert(MainWindow != NULL);
	if (!Markedsegp) {
		close_trigger_window();
		return 0;
	}

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	
	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	Markedwall = Markedsegp->sides[Markedside].wall_num;
	if (Markedwall != -1)
		trigger_num = Walls[Markedwall].trigger;
	else trigger_num = -1;

	if (t->old_trigger_num != trigger_num)
	{
		if (trigger_num != -1)
		{
			trigger *trig = &Triggers[trigger_num];

  			ui_checkbox_check(t->triggerFlag[0], trig->flags & TRIGGER_CONTROL_DOORS);
 			ui_checkbox_check(t->triggerFlag[1], trig->flags & TRIGGER_SHIELD_DAMAGE);
 			ui_checkbox_check(t->triggerFlag[2], trig->flags & TRIGGER_ENERGY_DRAIN);
 			ui_checkbox_check(t->triggerFlag[3], trig->flags & TRIGGER_EXIT);
 			ui_checkbox_check(t->triggerFlag[4], trig->flags & TRIGGER_ONE_SHOT);
 			ui_checkbox_check(t->triggerFlag[5], trig->flags & TRIGGER_ILLUSION_ON);
 			ui_checkbox_check(t->triggerFlag[6], trig->flags & TRIGGER_ILLUSION_OFF);
 			ui_checkbox_check(t->triggerFlag[7], trig->flags & TRIGGER_ON);
 			ui_checkbox_check(t->triggerFlag[8], trig->flags & TRIGGER_MATCEN);
 			ui_checkbox_check(t->triggerFlag[9], trig->flags & TRIGGER_SECRET_EXIT);
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the cooresponding wall flag.
	//------------------------------------------------------------
	if (IS_CHILD(Markedsegp->children[Markedside]))
	{
		rval = 1;
		
		if (GADGET_PRESSED(t->triggerFlag[0])) 
			trigger_flag_Markedside(TRIGGER_CONTROL_DOORS, t->triggerFlag[0]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[1]))
			trigger_flag_Markedside(TRIGGER_SHIELD_DAMAGE, t->triggerFlag[1]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[2]))
			trigger_flag_Markedside(TRIGGER_ENERGY_DRAIN, t->triggerFlag[2]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[3]))
			trigger_flag_Markedside(TRIGGER_EXIT, t->triggerFlag[3]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[4]))
			trigger_flag_Markedside(TRIGGER_ONE_SHOT, t->triggerFlag[4]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[5]))
			trigger_flag_Markedside(TRIGGER_ILLUSION_ON, t->triggerFlag[5]->flag); 
		else if (GADGET_PRESSED(t->triggerFlag[6]))
			trigger_flag_Markedside(TRIGGER_ILLUSION_OFF, t->triggerFlag[6]->flag);
		else if (GADGET_PRESSED(t->triggerFlag[7]))
			trigger_flag_Markedside(TRIGGER_ON, t->triggerFlag[7]->flag);
		else if (GADGET_PRESSED(t->triggerFlag[8])) 
			trigger_flag_Markedside(TRIGGER_MATCEN, t->triggerFlag[8]->flag);
		else if (GADGET_PRESSED(t->triggerFlag[9])) 
			trigger_flag_Markedside(TRIGGER_SECRET_EXIT, t->triggerFlag[9]->flag);
		else
			rval = 0;

	} else
		for (i = 0; i < NUM_TRIGGER_FLAGS; i++ )
			ui_checkbox_check(t->triggerFlag[i], 0);

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		gr_set_current_canvas( t->wallViewBox->canvas );

		if ((Markedsegp->sides[Markedside].wall_num == -1) || (Walls[Markedsegp->sides[Markedside].wall_num].trigger) == -1)
			gr_clear_canvas( CBLACK );
		else {
			if (Markedsegp->sides[Markedside].tmap_num2 > 0)  {
				gr_ubitmap(0,0, texmerge_get_cached_bitmap( Markedsegp->sides[Markedside].tmap_num, Markedsegp->sides[Markedside].tmap_num2));
			} else {
				if (Markedsegp->sides[Markedside].tmap_num > 0)	{
					PIGGY_PAGE_IN(Textures[Markedsegp->sides[Markedside].tmap_num]);
					gr_ubitmap(0,0, &GameBitmaps[Textures[Markedsegp->sides[Markedside].tmap_num].index]);
				} else
					gr_clear_canvas( CGREY );
			}
		}
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		if ( Markedsegp->sides[Markedside].wall_num > -1 )	{
			ui_dprintf_at( MainWindow, 12, 6, "Trigger: %d    ", trigger_num);
		}	else {
			ui_dprintf_at( MainWindow, 12, 6, "Trigger: none ");
		}
	}
	
	if (ui_button_any_drawn || (t->old_trigger_num != trigger_num) )
		Update_flags |= UF_WORLD_CHANGED;
		
	if (event->type == EVENT_WINDOW_CLOSE)
	{
		d_free(t);
		MainWindow = NULL;
		return 0;
	}

	if ( GADGET_PRESSED(t->quitButton) || (keypress==KEY_ESC))
	{
		close_trigger_window();
		return 1;
	}		

	t->old_trigger_num = trigger_num;
	
	return rval;
}





