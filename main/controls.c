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
 * Code for controlling player movement
 *
 */


#include <stdlib.h>
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"
#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"
#include "controls.h"
#include "render.h"
#include "args.h"
#include "palette.h"
#include "mouse.h"
#include "kconfig.h"

//look at keyboard, mouse, joystick, CyberMan, whatever, and set 
//physics vars rotvel, velocity

void read_flying_controls( object * obj )
{
	fix	afterburner_thrust;

	Assert(FrameTime > 0); 		//Get MATT if hit this!

	//	Couldn't the "50" in the next three lines be changed to "64" with no ill effect?
	obj->mtype.phys_info.rotthrust.x = Controls.pitch_time;
	obj->mtype.phys_info.rotthrust.y = Controls.heading_time;
	obj->mtype.phys_info.rotthrust.z = Controls.bank_time;

	afterburner_thrust = 0;
	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		afterburner_thrust = FrameTime;

	// Set object's thrust vector for forward/backward
	vm_vec_copy_scale(&obj->mtype.phys_info.thrust,&obj->orient.fvec, Controls.forward_thrust_time + afterburner_thrust );
	
	// slide left/right
	vm_vec_scale_add2(&obj->mtype.phys_info.thrust,&obj->orient.rvec, Controls.sideways_thrust_time );

	// slide up/down
	vm_vec_scale_add2(&obj->mtype.phys_info.thrust,&obj->orient.uvec, Controls.vertical_thrust_time );

	if (obj->mtype.phys_info.flags & PF_WIGGLE) {
		fix swiggle;
		fix_fastsincos(((fix)GameTime64), &swiggle, NULL);
		if (FrameTime < F1_0) // Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
			swiggle = fixmul(swiggle*20, FrameTime); //make wiggle fps-independant (based on pre-scaled amount of wiggle at 20 FPS)
		vm_vec_scale_add2(&obj->mtype.phys_info.velocity,&obj->orient.uvec,fixmul(swiggle,Player_ship->wiggle));
	}

	// As of now, obj->mtype.phys_info.thrust & obj->mtype.phys_info.rotthrust are 
	// in units of time... In other words, if thrust==FrameTime, that
	// means that the user was holding down the Max_thrust key for the
	// whole frame.  So we just scale them up by the max, and divide by
	// FrameTime to make them independant of framerate

	//	Prevent divide overflows on high frame rates.
	//	In a signed divide, you get an overflow if num >= div<<15
	{
		fix	ft = FrameTime;

		//	Note, you must check for ft < F1_0/2, else you can get an overflow  on the << 15.
		if ((ft < F1_0/2) && (ft << 15 <= Player_ship->max_thrust)) {
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale( &obj->mtype.phys_info.thrust, fixdiv(Player_ship->max_thrust,ft) );

		if ((ft < F1_0/2) && (ft << 15 <= Player_ship->max_rotthrust)) {
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale( &obj->mtype.phys_info.rotthrust, fixdiv(Player_ship->max_rotthrust,ft) );
	}

	//moved here by WraithX
	if (Player_is_dead) {
		//vm_vec_zero(&obj->mtype.phys_info.rotthrust); //let dead players rotate, changed by WraithX
		vm_vec_zero(&obj->mtype.phys_info.thrust);  //don't let dead players move, changed by WraithX
		return;
	}//end if


}
