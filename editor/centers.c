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
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "timer.h"
#include "objpage.h"
#include "fix.h"
#include "dxxerror.h"
#include "kdefs.h"
#include	"object.h"
#include "polyobj.h"
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

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_DIALOG 				*MainWindow = NULL;

typedef struct centers_dialog
{
	UI_GADGET_BUTTON 	*quitButton;
	UI_GADGET_RADIO		*centerFlag[MAX_CENTER_TYPES];
	UI_GADGET_CHECKBOX	*robotMatFlag[MAX_ROBOT_TYPES];
	int old_seg_num;
} centers_dialog;


char	center_names[MAX_CENTER_TYPES][CENTER_STRING_LENGTH] = {
	"Nothing",
	"FuelCen",
	"RepairCen",
	"ControlCen",
	"RobotMaker"
};

int centers_dialog_handler(UI_DIALOG *dlg, d_event *event, centers_dialog *c);

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the centers dialog box
//-------------------------------------------------------------------------
int do_centers_dialog()
{
	centers_dialog *c;
	int i;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	close_trigger_window();
	hostage_close_window();
	close_wall_window();
	robot_close_window();

	MALLOC(c, centers_dialog, 1);
	if (!c)
		return 0;

	// Open a window with a quit button
	MainWindow = ui_create_dialog( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, DF_DIALOG, (int (*)(UI_DIALOG *, d_event *, void *))centers_dialog_handler, c );
	c->quitButton = ui_add_gadget_button( MainWindow, 20, 252, 48, 40, "Done", NULL );

	// These are the checkboxes for each door flag.
	i = 80;
	c->centerFlag[0] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "NONE" ); 			i += 24;
	c->centerFlag[1] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "FuelCen" );		i += 24;
	c->centerFlag[2] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "RepairCen" );	i += 24;
	c->centerFlag[3] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "ControlCen" );	i += 24;
	c->centerFlag[4] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "RobotCen" );		i += 24;

	// These are the checkboxes for each robot flag.
	for (i=0; i<N_robot_types; i++)
		c->robotMatFlag[i] = ui_add_gadget_checkbox( MainWindow, 128 + (i%2)*92, 20+(i/2)*24, 16, 16, 0, Robot_names[i]);
																									  
	c->old_seg_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_centers_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog( MainWindow );
		MainWindow = NULL;
	}
}

int centers_dialog_handler(UI_DIALOG *dlg, d_event *event, centers_dialog *c)
{
	int i;
//	int robot_flags;
	int keypress = 0;
	int rval = 0;

	Assert(MainWindow != NULL);

	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;

	//------------------------------------------------------------
	// If we change centers, we need to reset the ui code for all
	// of the checkboxes that control the center flags.  
	//------------------------------------------------------------
	if (c->old_seg_num != Cursegp-Segments)
	{
		for (i = 0; i < MAX_CENTER_TYPES; i++)
			ui_radio_set_value(c->centerFlag[i], 0);

		Assert(Cursegp->special < MAX_CENTER_TYPES);
		ui_radio_set_value(c->centerFlag[Cursegp->special], 1);

		//	Read materialization center robot bit flags
		for (i = 0; i < N_robot_types; i++)
			ui_checkbox_check(c->robotMatFlag[i], RobotCenters[Cursegp->matcen_num].robot_flags[0] & (1 << i));
	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the corresponding center.
	//------------------------------------------------------------

	for (	i=0; i < MAX_CENTER_TYPES; i++ )
	{
		if ( GADGET_PRESSED(c->centerFlag[i]) )
		{
			if ( i == 0)
				fuelcen_delete(Cursegp);
			else if (Cursegp->special != i)
			{
				fuelcen_delete(Cursegp);
				Update_flags |= UF_WORLD_CHANGED;
				fuelcen_activate( Cursegp, i );
			}
			rval = 1;
		}
	}

	for (i = 0; i < N_robot_types; i++)
	{
		if ( GADGET_PRESSED(c->robotMatFlag[i]) )
		{
			if (c->robotMatFlag[i]->flag)
				RobotCenters[Cursegp->matcen_num].robot_flags[0] |= (1 << i);
			else
				RobotCenters[Cursegp->matcen_num].robot_flags[0] &= ~(1 << i);
			rval = 1;
		}
	}
	
	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
//		int	i;
//		char	temp_text[CENTER_STRING_LENGTH];
	
		ui_dprintf_at( dlg, 12, 6, "Seg: %3ld", Cursegp-Segments );

//		for (i=0; i<CENTER_STRING_LENGTH; i++)
//			temp_text[i] = ' ';
//		temp_text[i] = 0;

//		Assert(Cursegp->special < MAX_CENTER_TYPES);
//		strncpy(temp_text, Center_names[Cursegp->special], strlen(Center_names[Cursegp->special]));
//		ui_dprintf_at( dlg, 12, 23, " Type: %s", temp_text );
	}

	if (c->old_seg_num != Cursegp-Segments)
		Update_flags |= UF_WORLD_CHANGED;
		
	if (event->type == EVENT_WINDOW_CLOSE)
	{
		d_free(c);
		MainWindow = NULL;
		return 0;	// we're not cancelling the close
	}

	if ( GADGET_PRESSED(c->quitButton) || (keypress==KEY_ESC) )
	{
		close_centers_window();
		return 1;
	}		

	c->old_seg_num = Cursegp-Segments;
	
	return rval;
}



