/* $Id: medrobot.c,v 1.8 2005-07-25 03:50:07 chris Exp $ */
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
 * Dialog box to edit robot properties.
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: medrobot.c,v 1.8 2005-07-25 03:50:07 chris Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#ifdef __MSDOS__
#include <conio.h>
#include <direct.h>
#include <dos.h>
#endif
#include <math.h>
#include <string.h>

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
#include "centers.h"
#include "bm.h"

#define	NUM_BOXES		6			//	Number of boxes, AI modes

int GoodyNextID();
int GoodyPrevID();
void robot_close_window();
//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_WINDOW 				*MainWindow = NULL;
static UI_GADGET_USERBOX	*RobotViewBox;
static UI_GADGET_USERBOX	*ContainsViewBox;
static UI_GADGET_BUTTON 	*QuitButton;
static UI_GADGET_RADIO		*InitialMode[NUM_BOXES];

static int old_object;
static fix Time;
static vms_angvec angles={0,0,0}, goody_angles={0,0,0};

void call_init_ai_object(object *objp, int behavior)
{
	int	hide_segment;

	if (behavior == AIB_STATION)
		hide_segment = Cursegp-Segments;
	else {
		if (Markedsegp != NULL)
			hide_segment = Markedsegp-Segments;
		else
			hide_segment = Cursegp-Segments;
	}

	mprintf((0, "Initializing AI object with hide segment = %i\n", hide_segment));
	init_ai_object(objp-Objects, behavior, hide_segment);

	if (behavior == AIB_STATION) {
		int	cseg, mseg;

		cseg = 0;
		mseg = 0;

		if (Cursegp != NULL)
			cseg = Cursegp-Segments;

		if (Markedsegp != NULL) {
			mseg = Markedsegp-Segments;
		}

		//objp->ctype.ai_info.follow_path_start_seg = Cursegp-Segments;
		//objp->ctype.ai_info.follow_path_end_seg = Markedsegp-Segments;
	}
}

//-------------------------------------------------------------------------
// Called when user presses "Next Type" button.  This only works for polygon
// objects and it just selects the next polygon model for the current object.
//-------------------------------------------------------------------------
int RobotNextType()
{
	if (Cur_object_index > -1 )	{
		if ( Objects[Cur_object_index].type == OBJ_ROBOT )	{
			object * obj = &Objects[Cur_object_index];
			obj->id++;
			if (obj->id >= N_robot_types )
				obj->id = 0;

			//Set polygon-object-specific data
			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//set Physics info
			obj->mtype.phys_info.flags |= (PF_LEVELLING);
			obj->shields = Robot_info[obj->id].strength;
			call_init_ai_object(obj, AIB_NORMAL);

			Cur_object_id = obj->id;
		}
	}
	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

//-------------------------------------------------------------------------
// Called when user presses "Prev Type" button.  This only works for polygon
// objects and it just selects the prev polygon model for the current object.
//-------------------------------------------------------------------------
int RobotPrevType()
{
	if (Cur_object_index > -1 )	{
		if ( Objects[Cur_object_index].type == OBJ_ROBOT )	{
			object * obj = &Objects[Cur_object_index];
			if (obj->id == 0 ) 
				obj->id = N_robot_types-1;
			else
				obj->id--;

			//Set polygon-object-specific data
			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//set Physics info
			obj->mtype.phys_info.flags |= (PF_LEVELLING);
			obj->shields = Robot_info[obj->id].strength;
			call_init_ai_object(obj, AIB_NORMAL);

			Cur_object_id = obj->id;
		}
	}
	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

//-------------------------------------------------------------------------
// Dummy function for Mike to write.
//-------------------------------------------------------------------------
int med_set_ai_path()
{
	mprintf( (0, "med-set-ai-path called -- it does nothing, paths automatically set!\n" ));

	return 1;
}

// #define OBJ_NONE		255	//unused object
// #define OBJ_WALL		0		//A wall... not really an object, but used for collisions
// #define OBJ_FIREBALL	1		//a fireball, part of an explosion
// #define OBJ_ROBOT		2		//an evil enemy
// #define OBJ_HOSTAGE	3		//a hostage you need to rescue
// #define OBJ_PLAYER	4		//the player on the console
// #define OBJ_WEAPON	5		//a laser, missile, etc
// #define OBJ_CAMERA	6		//a camera to slew around with
// #define OBJ_POWERUP	7		//a powerup you can pick up
// #define OBJ_DEBRIS	8		//a piece of robot
// #define OBJ_CNTRLCEN	9		//the control center
// #define OBJ_FLARE		10		//the control center
// #define MAX_OBJECT_TYPES	11


#define	GOODY_TYPE_MAX	MAX_OBJECT_TYPES
#define	GOODY_X	6
#define	GOODY_Y	132

//#define	GOODY_ID_MAX_ROBOT	6
//#define	GOODY_ID_MAX_POWERUP	9
#define	GOODY_COUNT_MAX	4

int		Cur_goody_type = OBJ_POWERUP;
int		Cur_goody_id = 0;
int		Cur_goody_count = 0;

void update_goody_info(void)
{
	if (Cur_object_index > -1 )	{
		if ( Objects[Cur_object_index].type == OBJ_ROBOT )	{
			object * obj = &Objects[Cur_object_index];

			obj->contains_type = Cur_goody_type;
			obj->contains_id = Cur_goody_id;
			obj->contains_count = Cur_goody_count;
		}
	}
}

// #define OBJ_WALL		0		//A wall... not really an object, but used for collisions
// #define OBJ_FIREBALL	1		//a fireball, part of an explosion
// #define OBJ_ROBOT		2		//an evil enemy
// #define OBJ_HOSTAGE	3		//a hostage you need to rescue
// #define OBJ_PLAYER	4		//the player on the console
// #define OBJ_WEAPON	5		//a laser, missile, etc
// #define OBJ_CAMERA	6		//a camera to slew around with
// #define OBJ_POWERUP	7		//a powerup you can pick up
// #define OBJ_DEBRIS	8		//a piece of robot
// #define OBJ_CNTRLCEN	9		//the control center
// #define OBJ_FLARE		10		//the control center
// #define MAX_OBJECT_TYPES	11


int GoodyNextType()
{
	Cur_goody_type++;
	while (!((Cur_goody_type == OBJ_ROBOT) || (Cur_goody_type == OBJ_POWERUP))) {
		if (Cur_goody_type > GOODY_TYPE_MAX)
			Cur_goody_type=0;
		else
			Cur_goody_type++;
	}

	GoodyNextID();
	GoodyPrevID();

	update_goody_info();
	return 1;
}

int GoodyPrevType()
{
	Cur_goody_type--;
	while (!((Cur_goody_type == OBJ_ROBOT) || (Cur_goody_type == OBJ_POWERUP))) {
		if (Cur_goody_type < 0)
			Cur_goody_type = GOODY_TYPE_MAX;
		else
			Cur_goody_type--;
	}

	GoodyNextID();
	GoodyPrevID();

	update_goody_info();
	return 1;
}

int GoodyNextID()
{
	Cur_goody_id++;
	if (Cur_goody_type == OBJ_ROBOT) {
		if (Cur_goody_id >= N_robot_types)
			Cur_goody_id=0;
	} else {
		if (Cur_goody_id >= N_powerup_types)
			Cur_goody_id=0;
	}

	update_goody_info();
	return 1;
}

int GoodyPrevID()
{
	Cur_goody_id--;
	if (Cur_goody_type == OBJ_ROBOT) {
		if (Cur_goody_id < 0)
			Cur_goody_id = N_robot_types-1;
	} else {
		if (Cur_goody_id < 0)
			Cur_goody_id = N_powerup_types-1;
	}

	update_goody_info();
	return 1;
}

int GoodyNextCount()
{
	Cur_goody_count++;
	if (Cur_goody_count > GOODY_COUNT_MAX)
		Cur_goody_count=0;

	update_goody_info();
	return 1;
}

int GoodyPrevCount()
{
	Cur_goody_count--;
	if (Cur_goody_count < 0)
		Cur_goody_count=GOODY_COUNT_MAX;

	update_goody_info();
	return 1;
}

int is_legal_type(int the_type)
{
	return (the_type == OBJ_ROBOT) || (the_type == OBJ_CLUTTER);
}

int is_legal_type_for_this_window(int objnum)
{
	if (objnum == -1)
		return 1;
	else
		return is_legal_type(Objects[objnum].type);
}

int LocalObjectSelectNextinSegment(void)
{
	int	rval, first_obj;

	rval = ObjectSelectNextinSegment();
	first_obj = Cur_object_index;

	if (Cur_object_index != -1) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			//mprintf((0, "Skipping object #%i of type %i\n", Cur_object_index, Objects[Cur_object_index].type));
			rval = ObjectSelectNextinSegment();
			if (first_obj == Cur_object_index)
				break;
		}

		Cur_goody_type = Objects[Cur_object_index].contains_type;
		Cur_goody_id = Objects[Cur_object_index].contains_id;
		if (Objects[Cur_object_index].contains_count < 0)
			Objects[Cur_object_index].contains_count = 0;
		Cur_goody_count = Objects[Cur_object_index].contains_count;
	}

	if (Cur_object_index != first_obj)
		set_view_target_from_segment(Cursegp);

	return rval;
}

int LocalObjectSelectNextinMine(void)
{
	int	rval, first_obj;

	rval = ObjectSelectNextInMine();

	first_obj = Cur_object_index;

	if (Cur_object_index != -1) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			//mprintf((0, "Skipping object #%i of type %i\n", Cur_object_index, Objects[Cur_object_index].type));
			ObjectSelectNextInMine();
			if (Cur_object_index == first_obj)
				break;
		}

		Cur_goody_type = Objects[Cur_object_index].contains_type;
		Cur_goody_id = Objects[Cur_object_index].contains_id;
		if (Objects[Cur_object_index].contains_count < 0)
			Objects[Cur_object_index].contains_count = 0;
		Cur_goody_count = Objects[Cur_object_index].contains_count;
	}

	if (Cur_object_index != first_obj)
		set_view_target_from_segment(Cursegp);

	return rval;
}

int LocalObjectSelectPrevinMine(void)
{
	int	rval, first_obj;

	rval = ObjectSelectPrevInMine();

	first_obj = Cur_object_index;

	if (Cur_object_index != -1) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			//mprintf((0, "Skipping object #%i of type %i\n", Cur_object_index, Objects[Cur_object_index].type));
			ObjectSelectPrevInMine();
			if (first_obj == Cur_object_index)
				break;
		}

		Cur_goody_type = Objects[Cur_object_index].contains_type;
		Cur_goody_id = Objects[Cur_object_index].contains_id;
		if (Objects[Cur_object_index].contains_count < 0)
			Objects[Cur_object_index].contains_count = 0;
		Cur_goody_count = Objects[Cur_object_index].contains_count;
	}

	if (Cur_object_index != first_obj)
		set_view_target_from_segment(Cursegp);

	return rval;
}

int LocalObjectDelete(void)
{
	int	rval;

	rval = ObjectDelete();

	if (Cur_object_index != -1) {
		Cur_goody_type = Objects[Cur_object_index].contains_type;
		Cur_goody_id = Objects[Cur_object_index].contains_id;
		Cur_goody_count = Objects[Cur_object_index].contains_count;
	}

	set_view_target_from_segment(Cursegp);

	return rval;
}

int LocalObjectPlaceObject(void)
{
	int	rval;

	Cur_goody_count = 0;

	if (Cur_object_type != OBJ_ROBOT)
	{
		Cur_object_type = OBJ_ROBOT;
		Cur_object_id = 3;	// class 1 drone
		Num_object_subtypes = N_robot_types;
	}

	rval = ObjectPlaceObject();
	if (rval == -1)
		return -1;

	Objects[Cur_object_index].contains_type = Cur_goody_type;
	Objects[Cur_object_index].contains_id = Cur_goody_id;
	Objects[Cur_object_index].contains_count = Cur_goody_count;

	set_view_target_from_segment(Cursegp);

	return rval;
}

void close_all_windows(void)
{
	close_trigger_window();
	close_wall_window();
	close_centers_window();
	hostage_close_window();
	robot_close_window();
}


//-------------------------------------------------------------------------
// Called from the editor... does one instance of the robot dialog box
//-------------------------------------------------------------------------
int do_robot_dialog()
{
	int i;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows
	close_all_windows();
	Cur_goody_count = 0;

	// Open a window with a quit button
	MainWindow = ui_open_window( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );
	QuitButton = ui_add_gadget_button( MainWindow, 20, 286, 40, 32, "Done", NULL );

	ui_add_gadget_button( MainWindow, GOODY_X+50, GOODY_Y-3, 25, 22, "<<", GoodyPrevType );
	ui_add_gadget_button( MainWindow, GOODY_X+80, GOODY_Y-3, 25, 22, ">>", GoodyNextType );

	ui_add_gadget_button( MainWindow, GOODY_X+50, GOODY_Y+21, 25, 22, "<<", GoodyPrevID );
	ui_add_gadget_button( MainWindow, GOODY_X+80, GOODY_Y+21, 25, 22, ">>", GoodyNextID );

	ui_add_gadget_button( MainWindow, GOODY_X+50, GOODY_Y+45, 25, 22, "<<", GoodyPrevCount );
	ui_add_gadget_button( MainWindow, GOODY_X+80, GOODY_Y+45, 25, 22, ">>", GoodyNextCount );

	InitialMode[0] = ui_add_gadget_radio( MainWindow,  6, 58, 16, 16, 0, "Hover" );
	InitialMode[1] = ui_add_gadget_radio( MainWindow, 76, 58, 16, 16, 0, "Normal" );
	InitialMode[2] = ui_add_gadget_radio( MainWindow,  6, 78, 16, 16, 0, "(hide)" );
	InitialMode[3] = ui_add_gadget_radio( MainWindow, 76, 78, 16, 16, 0, "Avoid" );
	InitialMode[4] = ui_add_gadget_radio( MainWindow,  6, 98, 16, 16, 0, "Follow" );
	InitialMode[5] = ui_add_gadget_radio( MainWindow, 76, 98, 16, 16, 0, "Station" );

	// The little box the robots will spin in.
	RobotViewBox = ui_add_gadget_userbox( MainWindow,155, 5, 150, 125 );

	// The little box the robots will spin in.
	ContainsViewBox = ui_add_gadget_userbox( MainWindow,10, 202, 100, 80 );

	// A bunch of buttons...
	i = 135;
	ui_add_gadget_button( MainWindow,190,i,53, 26, "<<Typ", 			RobotPrevType );
	ui_add_gadget_button( MainWindow,247,i,53, 26, "Typ>>", 			RobotNextType );							i += 29;		
	ui_add_gadget_button( MainWindow,190,i,110, 26, "Next in Seg", LocalObjectSelectNextinSegment );	i += 29;		

	ui_add_gadget_button( MainWindow,190,i,53, 26, "<<Obj",		 	LocalObjectSelectPrevinMine );
	ui_add_gadget_button( MainWindow,247,i,53, 26, ">>Obj",			LocalObjectSelectNextinMine ); 		i += 29;		

	ui_add_gadget_button( MainWindow,190,i,110, 26, "Delete", 		LocalObjectDelete );						i += 29;		
	ui_add_gadget_button( MainWindow,190,i,110, 26, "Create New", 	LocalObjectPlaceObject );				i += 29;		
	ui_add_gadget_button( MainWindow,190,i,110, 26, "Set Path", 	med_set_ai_path );
	
	Time = timer_get_fixed_seconds();

	old_object = -2;		// Set to some dummy value so everything works ok on the first frame.

	if ( Cur_object_index == -1 )
		LocalObjectSelectNextinMine();

	return 1;

}

void robot_close_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_window( MainWindow );
		MainWindow = NULL;
	}

}

#define	STRING_LENGTH	8

void do_robot_window()
{
	int	i;
	fix	DeltaTime, Temp;
	int	first_object_index;

	if ( MainWindow == NULL ) return;

	first_object_index = Cur_object_index;
	while (!is_legal_type_for_this_window(Cur_object_index)) {
		LocalObjectSelectNextinMine();
		if (first_object_index == Cur_object_index) {
			break;
		}
	}

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MainWindow);

	//------------------------------------------------------------
	// If we change objects, we need to reset the ui code for all
	// of the radio buttons that control the ai mode.  Also makes
	// the current AI mode button be flagged as pressed down.
	//------------------------------------------------------------
	if (old_object != Cur_object_index )	{
		if ( Cur_object_index > -1 ) {
			int	behavior = Objects[Cur_object_index].ctype.ai_info.behavior;
			if ( !((behavior >= MIN_BEHAVIOR) && (behavior <= MAX_BEHAVIOR))) {
				mprintf((0, "Object #%i behavior id (%i) out of bounds, setting to AIB_NORMAL.\n", Cur_object_index, behavior));
				Objects[Cur_object_index].ctype.ai_info.behavior = AIB_NORMAL;
				behavior = AIB_NORMAL;
			}
			ui_radio_set_value(InitialMode[behavior - MIN_BEHAVIOR], 1);
		}
	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the cooresponding AI state.
	//------------------------------------------------------------
	for (	i=0; i < NUM_BOXES; i++ )	{
		if ( InitialMode[i]->flag == 1 )	
			if (Objects[Cur_object_index].ctype.ai_info.behavior != MIN_BEHAVIOR+i) {
				Objects[Cur_object_index].ctype.ai_info.behavior = MIN_BEHAVIOR+i;		// Set the ai_state to the cooresponding radio button
				call_init_ai_object(&Objects[Cur_object_index], MIN_BEHAVIOR+i);
			}
	}

	//------------------------------------------------------------
	// A simple frame time counter for spinning the objects...
	//------------------------------------------------------------
	Temp = timer_get_fixed_seconds();
	DeltaTime = Temp - Time;
	Time = Temp;

	//------------------------------------------------------------
	// Redraw the object in the little 64x64 box
	//------------------------------------------------------------
	if (Cur_object_index > -1 )	{
		object *obj = &Objects[Cur_object_index];

		gr_set_current_canvas( RobotViewBox->canvas );
		draw_object_picture(obj->id, &angles, obj->type );
		angles.h += fixmul(0x1000, DeltaTime );
	} else {
		// no object, so just blank out
		gr_set_current_canvas( RobotViewBox->canvas );
		gr_clear_canvas( CGREY );

//		LocalObjectSelectNextInMine();
	}

	//------------------------------------------------------------
	// Redraw the contained object in the other little box
	//------------------------------------------------------------
	if ((Cur_object_index > -1 ) && (Cur_goody_count > 0))	{
		gr_set_current_canvas( ContainsViewBox->canvas );
		if ( Cur_goody_id > -1 )
			draw_object_picture(Cur_goody_id, &goody_angles, Cur_goody_type);
		else
			gr_clear_canvas( CGREY );
		goody_angles.h += fixmul(0x1000, DeltaTime );
	} else {
		// no object, so just blank out
		gr_set_current_canvas( ContainsViewBox->canvas );
		gr_clear_canvas( CGREY );

//		LocalObjectSelectNextInMine();
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------

	if (ui_button_any_drawn || (old_object != Cur_object_index) )	{
		int	i;
		char	type_text[STRING_LENGTH+1],id_text[STRING_LENGTH+1];

		if (Cur_object_index != -1) {
			Cur_goody_type = Objects[Cur_object_index].contains_type;
			Cur_goody_id = Objects[Cur_object_index].contains_id;
			if (Objects[Cur_object_index].contains_count < 0)
				Objects[Cur_object_index].contains_count = 0;
			Cur_goody_count = Objects[Cur_object_index].contains_count;
		}

		ui_wprintf_at( MainWindow, GOODY_X, GOODY_Y,    " Type:");
		ui_wprintf_at( MainWindow, GOODY_X, GOODY_Y+24, "   ID:");
		ui_wprintf_at( MainWindow, GOODY_X, GOODY_Y+48, "Count:");

		for (i=0; i<STRING_LENGTH; i++)
			id_text[i] = ' ';
		id_text[i] = 0;

		switch (Cur_goody_type) {
			case OBJ_ROBOT:
				strcpy(type_text, "Robot  ");
				strncpy(id_text, Robot_names[Cur_goody_id], strlen(Robot_names[Cur_goody_id]));
				break;
			case OBJ_POWERUP:
				strcpy(type_text, "Powerup");
				strncpy(id_text, Powerup_names[Cur_goody_id], strlen(Powerup_names[Cur_goody_id]));
				break;
			default:
				editor_status("Illegal contained object type (%i), changing to powerup.", Cur_goody_type);
				Cur_goody_type = OBJ_POWERUP;
				Cur_goody_id = 0;
				strcpy(type_text, "Powerup");
				strncpy(id_text, Powerup_names[Cur_goody_id], strlen(Powerup_names[Cur_goody_id]));
				break;
		}

		ui_wprintf_at( MainWindow, GOODY_X+108, GOODY_Y, type_text);
		ui_wprintf_at( MainWindow, GOODY_X+108, GOODY_Y+24, id_text);
		ui_wprintf_at( MainWindow, GOODY_X+108, GOODY_Y+48, "%i", Cur_goody_count);

		if ( Cur_object_index > -1 )	{
			int	id = Objects[Cur_object_index].id;
			char	id_text[12];
			int	i;

			for (i=0; i<STRING_LENGTH; i++)
				id_text[i] = ' ';
			id_text[i] = 0;

			strncpy(id_text, Robot_names[id], strlen(Robot_names[id]));

			ui_wprintf_at( MainWindow, 12,  6, "Robot: %3d ", Cur_object_index );
			ui_wprintf_at( MainWindow, 12, 22, "   Id: %3d", id);
			ui_wprintf_at( MainWindow, 12, 38, " Name: %8s", id_text);

		}	else {
			ui_wprintf_at( MainWindow, 12,  6, "Robot: none" );
			ui_wprintf_at( MainWindow, 12, 22, " Type: ?  "  );
			ui_wprintf_at( MainWindow, 12, 38, " Name: ________" );
		}
		Update_flags |= UF_WORLD_CHANGED;
	}

	if ( QuitButton->pressed || (last_keypress==KEY_ESC))	{
		robot_close_window();
		return;
	}		

	old_object = Cur_object_index;
}

//	--------------------------------------------------------------------------------------------------------------------------
#define	NUM_MATT_THINGS	2

#define	MATT_LEN				20

static UI_WINDOW 				*MattWindow = NULL;

void object_close_window()
{
	if ( MattWindow!=NULL )	{
		ui_close_window( MattWindow );
		MattWindow = NULL;
	}

}


UI_GADGET_INPUTBOX	*Xtext, *Ytext, *Ztext;

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the object dialog box
//-------------------------------------------------------------------------
int do_object_dialog()
{
	char	Xmessage[MATT_LEN], Ymessage[MATT_LEN], Zmessage[MATT_LEN];
	object *obj=&Objects[Cur_object_index];

	if (obj->type == OBJ_ROBOT)		//don't do this for robots
		return 0;

	// Only open 1 instance of this window...
	if ( MattWindow != NULL )
		return 0;
	
	Cur_goody_count = 0;

	// Open a window with a quit button
	MattWindow = ui_open_window( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );
	QuitButton = ui_add_gadget_button( MattWindow, 20, 286, 40, 32, "Done", NULL );

	QuitButton->hotkey = KEY_ENTER;

	// These are the radio buttons for each mode
	InitialMode[0] = ui_add_gadget_radio( MattWindow, 10, 50, 16, 16, 0, "None" );
	InitialMode[1] = ui_add_gadget_radio( MattWindow, 80, 50, 16, 16, 0, "Spinning" );

	InitialMode[obj->movement_type == MT_SPINNING?1:0]->flag = 1;

	sprintf(Xmessage,"%.2f",f2fl(obj->mtype.spin_rate.x));
	sprintf(Ymessage,"%.2f",f2fl(obj->mtype.spin_rate.y));
	sprintf(Zmessage,"%.2f",f2fl(obj->mtype.spin_rate.z));

	ui_wprintf_at( MattWindow, 10, 132,"&X:" );
	Xtext = ui_add_gadget_inputbox( MattWindow, 30, 132, MATT_LEN, MATT_LEN, Xmessage );

	ui_wprintf_at( MattWindow, 10, 162,"&Y:" );
	Ytext = ui_add_gadget_inputbox( MattWindow, 30, 162, MATT_LEN, MATT_LEN, Ymessage );

	ui_wprintf_at( MattWindow, 10, 192,"&Z:" );
	Ztext = ui_add_gadget_inputbox( MattWindow, 30, 192, MATT_LEN, MATT_LEN, Zmessage );

	ui_gadget_calc_keys(MattWindow);

	MattWindow->keyboard_focus_gadget = (UI_GADGET *) InitialMode[0];

	mprintf((0, "X = %08x, Y = %08x, Z = %08x\n", atoi(Xmessage), atoi(Ymessage), atoi(Zmessage)));

	return 1;

}

void do_object_window()
{
	object *obj=&Objects[Cur_object_index];

	if ( MattWindow == NULL ) return;

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MattWindow);


	if ( QuitButton->pressed || (last_keypress==KEY_ESC))	{

		if (InitialMode[0]->flag) obj->movement_type = MT_NONE;
		if (InitialMode[1]->flag) obj->movement_type = MT_SPINNING;

		obj->mtype.spin_rate.x = fl2f(atof(Xtext->text));
		obj->mtype.spin_rate.y = fl2f(atof(Ytext->text));
		obj->mtype.spin_rate.z = fl2f(atof(Ztext->text));

		object_close_window();
		return;
	}		

	old_object = Cur_object_index;
}

void set_all_modes_to_hover(void)
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].control_type == CT_AI)
			Objects[i].ctype.ai_info.behavior = AIB_STILL;
}

