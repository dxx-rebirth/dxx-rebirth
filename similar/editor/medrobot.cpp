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
 * Dialog box to edit robot properties.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "segment.h"
#include "event.h"
#include "editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "timer.h"
#include "objpage.h"
#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"
#include "object.h"
#include "robot.h"
#include "game.h"
#include "powerup.h"
#include "ai.h"
#include "eobject.h"
#include "medwall.h"
#include "medrobot.h"
#include "eswitch.h"
#include "ehostage.h"
#include "key.h"
#include "centers.h"
#include "bm.h"
#include "u_mem.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_enumerate.h"
#include <memory>

static int GoodyNextID();
static int GoodyPrevID();

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
namespace dsx {

namespace {

struct robot_dialog : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	std::unique_ptr<UI_GADGET_USERBOX> robotViewBox, containsViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, prev_powerup_type, next_powerup_type, prev_powerup_id, next_powerup_id, prev_powerup_count, next_powerup_count, prev_robot_type, next_robot_type, next_segment, prev_object, next_object, delete_object, new_object, set_path;
	std::array<std::unique_ptr<UI_GADGET_RADIO>, 6> initialMode;			//	Number of boxes, AI modes
	fix64 time;
	vms_angvec angles = {}, goody_angles = {};
	int old_object;
	virtual window_event_result callback_handler(const d_event &) override;
};

static robot_dialog *MainWindow;

static void call_init_ai_object(vmobjptridx_t objp, ai_behavior behavior)
{
	segnum_t	hide_segment;

	if (behavior == ai_behavior::AIB_STATION)
		hide_segment = Cursegp;
	else {
		if (Markedsegp != segment_none)
			hide_segment = Markedsegp;
		else
			hide_segment = Cursegp;
	}

	init_ai_object(objp, behavior, hide_segment);
}

//-------------------------------------------------------------------------
// Called when user presses "Next Type" button.  This only works for polygon
// objects and it just selects the next polygon model for the current object.
//-------------------------------------------------------------------------
static int RobotNextType()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (Cur_object_index != object_none )	{
		const auto &&obj = vmobjptridx(Cur_object_index);
		if (obj->type == OBJ_ROBOT)
		{
			obj->id++;
			if (obj->id >= LevelSharedRobotInfoState.N_robot_types)
				obj->id = 0;

			//Set polygon-object-specific data
			obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//set Physics info
			obj->mtype.phys_info.flags |= (PF_LEVELLING);
			obj->shields = Robot_info[get_robot_id(obj)].strength;
			call_init_ai_object(obj, ai_behavior::AIB_NORMAL);

			Cur_object_id = get_robot_id(obj);
		}
	}
	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

//-------------------------------------------------------------------------
// Called when user presses "Prev Type" button.  This only works for polygon
// objects and it just selects the prev polygon model for the current object.
//-------------------------------------------------------------------------
static int RobotPrevType()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (Cur_object_index != object_none )	{
		const auto &&obj = vmobjptridx(Cur_object_index);
		if (obj->type == OBJ_ROBOT)
		{
			if (obj->id == 0 ) 
				obj->id = LevelSharedRobotInfoState.N_robot_types - 1;
			else
				obj->id--;

			//Set polygon-object-specific data
			obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
			obj->rtype.pobj_info.subobj_flags = 0;
			//set Physics info
			obj->mtype.phys_info.flags |= (PF_LEVELLING);
			obj->shields = Robot_info[get_robot_id(obj)].strength;
			call_init_ai_object(obj, ai_behavior::AIB_NORMAL);

			Cur_object_id = get_robot_id(obj);
		}
	}
	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

}

}

//-------------------------------------------------------------------------
// Dummy function for Mike to write.
//-------------------------------------------------------------------------
static int med_set_ai_path()
{
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

static void update_goody_info(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (Cur_object_index != object_none )	{
		auto &obj = *vmobjptr(Cur_object_index);
		if (obj.type == OBJ_ROBOT)
		{
			obj.contains_type = Cur_goody_type;
			obj.contains_id = Cur_goody_id;
			obj.contains_count = Cur_goody_count;
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


static int GoodyNextType()
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

static int GoodyPrevType()
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
		if (Cur_goody_id >= LevelSharedRobotInfoState.N_robot_types)
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
			Cur_goody_id = LevelSharedRobotInfoState.N_robot_types - 1;
	} else {
		if (Cur_goody_id < 0)
			Cur_goody_id = N_powerup_types-1;
	}

	update_goody_info();
	return 1;
}

static int GoodyNextCount()
{
	Cur_goody_count++;
	if (Cur_goody_count > GOODY_COUNT_MAX)
		Cur_goody_count=0;

	update_goody_info();
	return 1;
}

static int GoodyPrevCount()
{
	Cur_goody_count--;
	if (Cur_goody_count < 0)
		Cur_goody_count=GOODY_COUNT_MAX;

	update_goody_info();
	return 1;
}

static int is_legal_type(int the_type)
{
	return (the_type == OBJ_ROBOT) || (the_type == OBJ_CLUTTER);
}

static int is_legal_type_for_this_window(const imobjidx_t objnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (objnum == object_none)
		return 1;
	else
		return is_legal_type(vmobjptr(objnum)->type);
}

static int LocalObjectSelectNextinSegment(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int	rval, first_obj;

	rval = ObjectSelectNextinSegment();
	first_obj = Cur_object_index;

	if (Cur_object_index != object_none) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			rval = ObjectSelectNextinSegment();
			if (first_obj == Cur_object_index)
				break;
		}

		const auto &&objp = vmobjptr(Cur_object_index);
		Cur_goody_type = objp->contains_type;
		Cur_goody_id = objp->contains_id;
		if (objp->contains_count < 0)
			objp->contains_count = 0;
		Cur_goody_count = objp->contains_count;
	}

	if (Cur_object_index != first_obj)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}

	return rval;
}

static int LocalObjectSelectNextinMine(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int	rval, first_obj;

	rval = ObjectSelectNextInMine();

	first_obj = Cur_object_index;

	if (Cur_object_index != object_none) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			ObjectSelectNextInMine();
			if (Cur_object_index == first_obj)
				break;
		}

		const auto &&objp = vmobjptr(Cur_object_index);
		Cur_goody_type = objp->contains_type;
		Cur_goody_id = objp->contains_id;
		if (objp->contains_count < 0)
			objp->contains_count = 0;
		Cur_goody_count = objp->contains_count;
	}

	if (Cur_object_index != first_obj)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}

	return rval;
}

static int LocalObjectSelectPrevinMine(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int	rval, first_obj;

	rval = ObjectSelectPrevInMine();

	first_obj = Cur_object_index;

	if (Cur_object_index != object_none) {
		while (!is_legal_type_for_this_window(Cur_object_index)) {
			ObjectSelectPrevInMine();
			if (first_obj == Cur_object_index)
				break;
		}

		const auto &&objp = vmobjptr(Cur_object_index);
		Cur_goody_type = objp->contains_type;
		Cur_goody_id = objp->contains_id;
		if (objp->contains_count < 0)
			objp->contains_count = 0;
		Cur_goody_count = objp->contains_count;
	}

	if (Cur_object_index != first_obj)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}

	return rval;
}

static int LocalObjectDelete(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptr = Objects.vcptr;
	int	rval;

	rval = ObjectDelete();

	if (Cur_object_index != object_none) {
		auto &objp = *vcobjptr(Cur_object_index);
		Cur_goody_type = objp.contains_type;
		Cur_goody_id = objp.contains_id;
		Cur_goody_count = objp.contains_count;
	}

	auto &vcvertptr = Vertices.vcptr;
	set_view_target_from_segment(vcvertptr, Cursegp);

	return rval;
}

static int LocalObjectPlaceObject(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int	rval;

	Cur_goody_count = 0;

	if (Cur_object_type != OBJ_ROBOT)
	{
		Cur_object_type = OBJ_ROBOT;
		Cur_object_id = 3;	// class 1 drone
		Num_object_subtypes = LevelSharedRobotInfoState.N_robot_types;
	}

	rval = ObjectPlaceObject();
	if (rval == -1)
		return -1;

	const auto &&objp = vmobjptr(Cur_object_index);
	objp->contains_type = Cur_goody_type;
	objp->contains_id = Cur_goody_id;
	objp->contains_count = Cur_goody_count;

	auto &vcvertptr = Vertices.vcptr;
	set_view_target_from_segment(vcvertptr, Cursegp);

	return rval;
}

void close_all_windows(void)
{
	close_trigger_window();
	close_wall_window();
	close_centers_window();
#if defined(DXX_BUILD_DESCENT_I)
	hostage_close_window();
#endif
	robot_close_window();
}


//-------------------------------------------------------------------------
// Called from the editor... does one instance of the robot dialog box
//-------------------------------------------------------------------------
int do_robot_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows
	close_all_windows();
	Cur_goody_count = 0;

	// Open a window with a quit button
	MainWindow = window_create<robot_dialog>(TMAPBOX_X + 20, TMAPBOX_Y + 20, 765 - TMAPBOX_X, 545 - TMAPBOX_Y, DF_DIALOG);
	return 1;
}

static window_event_result robot_dialog_created(robot_dialog *const r)
{
	r->quitButton = ui_add_gadget_button(*r, 20, 286, 40, 32, "Done", NULL);
	r->prev_powerup_type = ui_add_gadget_button(*r, GOODY_X+50, GOODY_Y-3, 25, 22, "<<", GoodyPrevType);
	r->next_powerup_type = ui_add_gadget_button(*r, GOODY_X+80, GOODY_Y-3, 25, 22, ">>", GoodyNextType);
	r->prev_powerup_id = ui_add_gadget_button(*r, GOODY_X+50, GOODY_Y+21, 25, 22, "<<", GoodyPrevID);
	r->next_powerup_id = ui_add_gadget_button(*r, GOODY_X+80, GOODY_Y+21, 25, 22, ">>", GoodyNextID);
	r->prev_powerup_count = ui_add_gadget_button(*r, GOODY_X+50, GOODY_Y+45, 25, 22, "<<", GoodyPrevCount);
	r->next_powerup_count = ui_add_gadget_button(*r, GOODY_X+80, GOODY_Y+45, 25, 22, ">>", GoodyNextCount);
	r->initialMode[0] = ui_add_gadget_radio(*r,  6, 58, 16, 16, 0, "Hover");
	r->initialMode[1] = ui_add_gadget_radio(*r, 76, 58, 16, 16, 0, "Normal");
	r->initialMode[2] = ui_add_gadget_radio(*r,  6, 78, 16, 16, 0, "(hide)");
	r->initialMode[3] = ui_add_gadget_radio(*r, 76, 78, 16, 16, 0, "Avoid");
	r->initialMode[4] = ui_add_gadget_radio(*r,  6, 98, 16, 16, 0, "Follow");
	r->initialMode[5] = ui_add_gadget_radio(*r, 76, 98, 16, 16, 0, "Station");
	// The little box the robots will spin in.
	r->robotViewBox = ui_add_gadget_userbox(*r, 155, 5, 150, 125);
	// The little box the robots will spin in.
	r->containsViewBox = ui_add_gadget_userbox(*r, 10, 202, 100, 80);
	// A bunch of buttons...
	int i = 135;
	r->prev_robot_type = ui_add_gadget_button(*r, 190, i, 53, 26, "<<Typ", 			RobotPrevType);
	r->next_robot_type = ui_add_gadget_button(*r, 247, i, 53, 26, "Typ>>", 			RobotNextType);							i += 29;		
	r->next_segment = ui_add_gadget_button(*r, 190, i, 110, 26, "Next in Seg", LocalObjectSelectNextinSegment);	i += 29;		
	r->prev_object = ui_add_gadget_button(*r, 190, i, 53, 26, "<<Obj",		 	LocalObjectSelectPrevinMine);
	r->next_object = ui_add_gadget_button(*r, 247, i, 53, 26, ">>Obj",			LocalObjectSelectNextinMine); 		i += 29;		
	r->delete_object = ui_add_gadget_button(*r, 190, i, 110, 26, "Delete", 		LocalObjectDelete);						i += 29;		
	r->new_object = ui_add_gadget_button(*r, 190, i, 110, 26, "Create New", 	LocalObjectPlaceObject);				i += 29;		
	r->set_path = ui_add_gadget_button(*r, 190, i, 110, 26, "Set Path", 	med_set_ai_path);
	r->time = timer_query();
	r->old_object = -2;		// Set to some dummy value so everything works ok on the first frame.
	if ( Cur_object_index == object_none)
		LocalObjectSelectNextinMine();
	return window_event_result::handled;
}

void robot_close_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog(*std::exchange(MainWindow, nullptr));
	}

}

namespace dsx {

namespace {

window_event_result robot_dialog::callback_handler(const d_event &event)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return robot_dialog_created(this);
		case EVENT_WINDOW_CLOSE:
			MainWindow = NULL;
			return window_event_result::ignored;
		default:
			break;
	}
	fix	DeltaTime;
	fix64	Temp;
	int	first_object_index;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
		
	Assert(MainWindow != NULL);

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

	//------------------------------------------------------------
	// If we change objects, we need to reset the ui code for all
	// of the radio buttons that control the ai mode.  Also makes
	// the current AI mode button be flagged as pressed down.
	//------------------------------------------------------------
	if (old_object != Cur_object_index )	{
		range_for (auto &i, initialMode)
			ui_radio_set_value(*i, 0);
		if ( Cur_object_index != object_none ) {
			auto &behavior = vmobjptr(Cur_object_index)->ctype.ai_info.behavior;
			switch (behavior)
			{
				case ai_behavior::AIB_STILL:
				case ai_behavior::AIB_NORMAL:
				case ai_behavior::AIB_RUN_FROM:
				case ai_behavior::AIB_STATION:
#if defined(DXX_BUILD_DESCENT_I)
				case ai_behavior::AIB_HIDE:
				case ai_behavior::AIB_FOLLOW_PATH:
#elif defined(DXX_BUILD_DESCENT_II)
				case ai_behavior::AIB_BEHIND:
				case ai_behavior::AIB_SNIPE:
				case ai_behavior::AIB_FOLLOW:
#endif
					break;
				default:
					behavior = ai_behavior::AIB_NORMAL;
					break;
			}
			ui_radio_set_value(*initialMode[static_cast<std::size_t>(behavior) - MIN_BEHAVIOR], 1);
		}
	}

	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the cooresponding AI state.
	//------------------------------------------------------------
	range_for (auto &&eim, enumerate(initialMode))
	{
		auto &im = eim.value;
		if (GADGET_PRESSED(im.get()))
		{
			const auto i = eim.idx;
			const auto b = static_cast<ai_behavior>(MIN_BEHAVIOR + i);
			const auto &&objp = vmobjptridx(Cur_object_index);
			auto &behavior = objp->ctype.ai_info.behavior;
			if (behavior != b) {
				behavior = b;		// Set the ai_state to the cooresponding radio button
				call_init_ai_object(objp, b);
				rval = window_event_result::handled;
			}
		}
	}

	//------------------------------------------------------------
	// Redraw the object in the little 64x64 box
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		// A simple frame time counter for spinning the objects...
		Temp = timer_query();
		DeltaTime = Temp - time;
		time = Temp;

		gr_set_current_canvas(robotViewBox->canvas);
		if (Cur_object_index != object_none )	{
			const auto &&obj = vmobjptr(Cur_object_index);

			draw_object_picture(*grd_curcanv, obj->id, angles, obj->type);
			angles.h += fixmul(0x1000, DeltaTime );
		} else {
			// no object, so just blank out
			gr_clear_canvas(*grd_curcanv, CGREY);

	//		LocalObjectSelectNextInMine();
		}

	//------------------------------------------------------------
	// Redraw the contained object in the other little box
	//------------------------------------------------------------
		gr_set_current_canvas(containsViewBox->canvas);
		if ((Cur_object_index != object_none ) && (Cur_goody_count > 0))	{
			if ( Cur_goody_id > -1 )
				draw_object_picture(*grd_curcanv, Cur_goody_id, goody_angles, Cur_goody_type);
			else
				gr_clear_canvas(*grd_curcanv, CGREY);
			goody_angles.h += fixmul(0x1000, DeltaTime );
		} else {
			// no object, so just blank out
			gr_clear_canvas(*grd_curcanv, CGREY);

	//		LocalObjectSelectNextInMine();
		}
	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------

		const char *id_text;
		const char *type_text;

		if (Cur_object_index != object_none) {
			const auto &&obj = vmobjptr(Cur_object_index);
			Cur_goody_type = obj->contains_type;
			Cur_goody_id = obj->contains_id;
			if (obj->contains_count < 0)
				obj->contains_count = 0;
			Cur_goody_count = obj->contains_count;
		}

		ui_dprintf_at( MainWindow, GOODY_X, GOODY_Y,    " Type:");
		ui_dprintf_at( MainWindow, GOODY_X, GOODY_Y+24, "   ID:");
		ui_dprintf_at( MainWindow, GOODY_X, GOODY_Y+48, "Count:");

		switch (Cur_goody_type) {
			case OBJ_ROBOT:
				type_text = "Robot  ";
				id_text = Robot_names[Cur_goody_id].data();
				break;
			default:
				editor_status_fmt("Illegal contained object type (%i), changing to powerup.", Cur_goody_type);
				Cur_goody_type = OBJ_POWERUP;
				Cur_goody_id = 0;
				DXX_BOOST_FALLTHROUGH;
			case OBJ_POWERUP:
				type_text = "Powerup";
				id_text = Powerup_names[Cur_goody_id].data();
				break;
		}

		ui_dputs_at( MainWindow, GOODY_X+108, GOODY_Y, type_text);
		ui_dprintf_at( MainWindow, GOODY_X+108, GOODY_Y+24, "%-8s", id_text);
		ui_dprintf_at( MainWindow, GOODY_X+108, GOODY_Y+48, "%i", Cur_goody_count);

		if ( Cur_object_index != object_none )	{
			const auto id = get_robot_id(vcobjptr(Cur_object_index));

			ui_dprintf_at( MainWindow, 12,  6, "Robot: %3d ", Cur_object_index );
			ui_dprintf_at( MainWindow, 12, 22, "   Id: %3d", id);
			ui_dprintf_at( MainWindow, 12, 38, " Name: %-8s", Robot_names[id].data());

		}	else {
			ui_dprintf_at( MainWindow, 12,  6, "Robot: none" );
			ui_dprintf_at( MainWindow, 12, 22, " Type: ?  "  );
			ui_dprintf_at( MainWindow, 12, 38, " Name: ________" );
		}
	}
	
	if (ui_button_any_drawn || (old_object != Cur_object_index) )
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(quitButton.get()) || keypress == KEY_ESC)
	{
		return window_event_result::close;
	}		

	old_object = Cur_object_index;
	return rval;
}

}

}

//	--------------------------------------------------------------------------------------------------------------------------
#define	NUM_MATT_THINGS	2

#define	MATT_LEN				20

namespace {

struct object_dialog : UI_DIALOG
{
	explicit object_dialog(short x, short y, short w, short h, enum dialog_flags flags, object &obj);
	struct creation_context
	{
		vmobjptr_t obj;
		creation_context(vmobjptr_t o) :
			obj(o)
		{
		}
	};
	std::unique_ptr<UI_GADGET_INPUTBOX> xtext, ytext, ztext;
	std::array<std::unique_ptr<UI_GADGET_RADIO>, 2> initialMode;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton;
	virtual window_event_result callback_handler(const d_event &) override;
};

static object_dialog *MattWindow;

}

void object_close_window()
{
	if ( MattWindow!=NULL )	{
		ui_close_dialog(*std::exchange(MattWindow, nullptr));
	}

}


//-------------------------------------------------------------------------
// Called from the editor... does one instance of the object dialog box
//-------------------------------------------------------------------------
int do_object_dialog()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (Cur_object_index == object_none)
		Cur_object_index = object_first;

	auto obj = vmobjptr(Cur_object_index);
	if (obj->type == OBJ_ROBOT)		//don't do this for robots
		return 0;

	// Only open 1 instance of this window...
	if ( MattWindow != NULL )
		return 0;
	
	Cur_goody_count = 0;

	// Open a window with a quit button
	MattWindow = window_create<object_dialog>(TMAPBOX_X + 20, TMAPBOX_Y + 20, 765 - TMAPBOX_X, 545 - TMAPBOX_Y, DF_DIALOG, *obj);
	return 1;
}

object_dialog::object_dialog(short x, short y, short w, short h, enum dialog_flags flags, object &obj) :
	UI_DIALOG(x, y, w, h, flags)
{
	quitButton = ui_add_gadget_button(*this, 20, 286, 40, 32, "Done", NULL );
	quitButton->hotkey = KEY_ENTER;
	// These are the radio buttons for each mode
	initialMode[0] = ui_add_gadget_radio(*this, 10, 50, 16, 16, 0, "None" );
	initialMode[1] = ui_add_gadget_radio(*this, 80, 50, 16, 16, 0, "Spinning" );
	initialMode[obj.movement_source == object::movement_type::spinning ? 1 : 0]->flag = 1;
	char message[MATT_LEN];
	snprintf(message, sizeof(message), "%.2f", f2fl(obj.mtype.spin_rate.x));
	xtext = ui_add_gadget_inputbox<MATT_LEN>(*this, 30, 132, message);
	snprintf(message, sizeof(message), "%.2f", f2fl(obj.mtype.spin_rate.y));
	ytext = ui_add_gadget_inputbox<MATT_LEN>(*this, 30, 162, message);
	snprintf(message, sizeof(message), "%.2f", f2fl(obj.mtype.spin_rate.z));
	ztext = ui_add_gadget_inputbox<MATT_LEN>(*this, 30, 192, message);
	ui_gadget_calc_keys(*this);
	keyboard_focus_gadget = initialMode[0].get();
}

window_event_result object_dialog::callback_handler(const d_event &event)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	switch(event.type)
	{
		case EVENT_WINDOW_CLOSE:
			MattWindow = NULL;
			return window_event_result::ignored;
		default:
			break;
	}
	const auto &&obj = vmobjptr(Cur_object_index);
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;


	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dprintf_at(this, 10, 132,"&X:" );
		ui_dprintf_at(this, 10, 162,"&Y:" );
		ui_dprintf_at(this, 10, 192,"&Z:" );
	}
	
	if (GADGET_PRESSED(quitButton.get()) || keypress == KEY_ESC)
	{
		if (initialMode[0]->flag)
			obj->movement_source = object::movement_type::None;
		if (initialMode[1]->flag)
			obj->movement_source = object::movement_type::spinning;

		obj->mtype.spin_rate.x = fl2f(atof(xtext->text.get()));
		obj->mtype.spin_rate.y = fl2f(atof(ytext->text.get()));
		obj->mtype.spin_rate.z = fl2f(atof(ztext->text.get()));
		return window_event_result::close;
	}
	return rval;
}
