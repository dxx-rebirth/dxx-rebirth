/* $Id: centers.c,v 1.7 2005-07-01 09:52:29 chris Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: centers.c,v 1.7 2005-07-01 09:52:29 chris Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fuelcen.h"
#include "screens.h"
#include "inferno.h"
#include "segment.h"
#include "editor.h"

#include "timer.h"
#include "objpage.h"
#include "fix.h"
#include "mono.h"
#include "error.h"
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
static UI_WINDOW 				*MainWindow = NULL;
static UI_GADGET_BUTTON 	*QuitButton;
static UI_GADGET_RADIO		*CenterFlag[MAX_CENTER_TYPES];
static UI_GADGET_CHECKBOX	*RobotMatFlag[64];	// 2 ints = 64 bits

static int old_seg_num;

char	center_names[MAX_CENTER_TYPES][CENTER_STRING_LENGTH] = {
	"Nothing",
	"FuelCen",
	"RepairCen",
	"ControlCen",
	"RobotMaker"
};

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the centers dialog box
//-------------------------------------------------------------------------
int do_centers_dialog()
{
	int i;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	close_trigger_window();
	hostage_close_window();
	close_wall_window();
	robot_close_window();

	// Open a window with a quit button
	MainWindow = ui_open_window( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );
	QuitButton = ui_add_gadget_button( MainWindow, 20, 252, 48, 40, "Done", NULL );

	// These are the checkboxes for each door flag.
	i = 80;
	CenterFlag[0] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "NONE" ); 			i += 24;
	CenterFlag[1] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "FuelCen" );		i += 24;
	CenterFlag[2] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "RepairCen" );	i += 24;
	CenterFlag[3] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "ControlCen" );	i += 24;
	CenterFlag[4] = ui_add_gadget_radio( MainWindow, 18, i, 16, 16, 0, "RobotCen" );		i += 24;

	// These are the checkboxes for each robot flag.
	for (i=0; i < 64; i++)
		RobotMatFlag[i] = ui_add_gadget_checkbox( MainWindow, 128 + (i%2)*92, 20+(i/2)*24, 16, 16, 0, Robot_names[i]);
																									  
	old_seg_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_centers_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_window( MainWindow );
		MainWindow = NULL;
	}
}

void do_centers_window()
{
	int i;
	int robot_flags;
	int redraw_window;
	int robot_index;

	if ( MainWindow == NULL ) return;

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MainWindow);

	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	if (old_seg_num != Cursegp-Segments) {
		for (	i=0; i < MAX_CENTER_TYPES; i++ ) {
			CenterFlag[i]->flag = 0;		// Tells ui that this button isn't checked
			CenterFlag[i]->status = 1;		// Tells ui to redraw button
		}

		Assert(Curseg2p->special < MAX_CENTER_TYPES);
		CenterFlag[Curseg2p->special]->flag = 1;

		mprintf((0, "Curseg2p->matcen_num = %i\n", Curseg2p->matcen_num));

		//	Read materialization center robot bit flags
		for (i = 0; i < 2; i++)
		{
			robot_index = i * 32;
			robot_flags = RobotCenters[Curseg2p->matcen_num].robot_flags[i];
			while (robot_flags)
			{
				ui_checkbox_check(RobotMatFlag[i], robot_flags & 1);
				robot_flags >>= 1;
				robot_index++;
			}
		}

	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the corresponding center.
	//------------------------------------------------------------

	redraw_window=0;
	for (	i=0; i < MAX_CENTER_TYPES; i++ )	{
		if ( CenterFlag[i]->flag == 1 )
                 {
			if ( i == 0)
				fuelcen_delete(Cursegp);
			else if (Curseg2p->special != i)
			{
				fuelcen_delete(Cursegp);
				redraw_window = 1;
				fuelcen_activate( Cursegp, i );
			}
                 }
	}

	for (i = 0; i < 2; i++)
	{
		robot_flags = RobotCenters[Curseg2p->matcen_num].robot_flags[i];

		for (robot_index = 0; robot_index < 32; robot_index++)
		{
			if (RobotMatFlag[robot_index + i * 32]->flag == 1)
			{
				if (!(robot_flags & (1 << robot_index)))
				{
					robot_flags |= (1 << robot_index);
					mprintf((0, "Segment %i, matcen = %i, robot_flags[%d] = %d\n", Cursegp - Segments, Curseg2p->matcen_num, i, robot_flags));
				}
			}
			else if (robot_flags & 1 << robot_index)
			{
				robot_flags &= ~(1 << robot_index);
				mprintf((0, "Segment %i, matcen = %i, robot_flags[%d] = %d\n", Cursegp - Segments, Curseg2p->matcen_num, i, robot_flags));
			}
		}

		RobotCenters[Curseg2p->matcen_num].robot_flags[i] = robot_flags;
	}
	
	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (redraw_window || (old_seg_num != Cursegp-Segments ) ) {
//		int	i;
//		char	temp_text[CENTER_STRING_LENGTH];
	
		ui_wprintf_at( MainWindow, 12, 6, "Seg: %3d", Cursegp-Segments );

//		for (i=0; i<CENTER_STRING_LENGTH; i++)
//			temp_text[i] = ' ';
//		temp_text[i] = 0;

//		Assert(Curseg2p->special < MAX_CENTER_TYPES);
//		strncpy(temp_text, Center_names[Curseg2p->special], strlen(Center_names[Curseg2p->special]));
//		ui_wprintf_at( MainWindow, 12, 23, " Type: %s", temp_text );
		Update_flags |= UF_WORLD_CHANGED;
	}

	if ( QuitButton->pressed || (last_keypress==KEY_ESC) )	{
		close_centers_window();
		return;
	}		

	old_seg_num = Cursegp-Segments;
}



