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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/controls.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:28 $
 * 
 * Code for controlling player movement
 * 
 * $Log: controls.c,v $
 * Revision 1.1.1.1  2006/03/17 19:43:28  zicodxx
 * initial import
 *
 * Revision 1.4  2003/07/11 21:32:28  donut
 * make ship bobbing fps-independant
 *
 * Revision 1.3  2000/04/19 21:30:00  sekmu
 * movable death-cam from WraithX
 *
 * Revision 1.2  1999/11/21 14:05:00  sekmu
 * observer mode
 *
 * Revision 1.1.1.1  1999/06/14 22:05:52  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:27:11  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.50  1995/02/22  14:11:19  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.49  1994/12/15  13:04:10  mike
 * Replace Players[Player_num].time_total references with GameTime.
 * 
 * Revision 1.48  1994/11/27  23:12:13  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.47  1994/11/25  22:15:54  matt
 * Added asserts to try to trap frametime < 0 bug
 * 
 * Revision 1.46  1994/11/16  11:25:40  matt
 * Took out int3's since I determined that the errors are caused by neg frametime
 * 
 * Revision 1.45  1994/11/15  13:07:23  matt
 * Added int3's to try to trap bug
 * 
 * Revision 1.44  1994/10/14  16:18:12  john
 * Made Assert that the object was player just nicely exit
 * the function.
 * 
 * Revision 1.43  1994/10/13  11:35:25  john
 * Made Thrustmaster FCS Hat work.  Put a background behind the
 * keyboard configure.  Took out turn_sensitivity.  Changed sound/config
 * menu to new menu. Made F6 be calibrate joystick.
 * 
 * Revision 1.42  1994/09/29  11:22:02  mike
 * Zero thrust when player dies.
 * 
 * Revision 1.41  1994/09/16  13:10:30  mike
 * Hook in afterburner stuff.
 * 
 * Revision 1.40  1994/09/14  22:21:54  matt
 * Avoid post-death assert
 * 
 * Revision 1.39  1994/09/11  20:30:27  matt
 * Cleaned up thrust vars, changing a few names
 * 
 * Revision 1.38  1994/09/10  15:46:31  john
 * First version of new keyboard configuration.
 * 
 * Revision 1.37  1994/09/07  15:58:12  mike
 * Check for player dead in controls so you can't fire or move after dead, logical, huh?
 * 
 * Revision 1.36  1994/09/06  14:51:56  john
 * Added sensitivity adjustment, fixed bug with joystick button not
 * staying down.
 * 
 * Revision 1.35  1994/09/01  15:43:26  john
 * Put pitch bak like it was.
 * 
 * Revision 1.34  1994/08/31  18:59:35  john
 * Made rotthrust back like it was.
 * 
 * Revision 1.33  1994/08/31  18:49:17  john
 * Slowed Maxrothrust a bit,
 * ..
 * 
 * Revision 1.32  1994/08/31  18:32:05  john
 * Lower max rotational thrust
 * 
 * Revision 1.31  1994/08/29  21:18:27  john
 * First version of new keyboard/oystick remapping stuff.
 * 
 * Revision 1.30  1994/08/29  16:18:30  mike
 * trap divide overflow.
 * 
 * Revision 1.29  1994/08/26  14:40:45  john
 * *** empty log message ***
 * 
 * Revision 1.28  1994/08/26  12:23:50  john
 * MAde joystick read up to 15 times per second max.
 * 
 * Revision 1.27  1994/08/26  10:50:59  john
 * Took out Controls_always_stopped.
 * 
 * Revision 1.26  1994/08/26  10:47:27  john
 * *** empty log message ***
 * 
 * Revision 1.25  1994/08/26  10:46:50  john
 * New version of controls.
 * 
 * Revision 1.24  1994/08/25  19:41:44  john
 * *** empty log message ***
 * 
 * Revision 1.23  1994/08/25  18:44:55  john
 * *** empty log message ***
 * 
 * Revision 1.22  1994/08/25  18:43:46  john
 * First revision of new control code.
 * 
 * Revision 1.21  1994/08/24  20:02:46  john
 * Added cyberman support; made keys work key_down_time
 * returning seconds instead of milliseconds,.
 * 
 * 
 * Revision 1.20  1994/08/24  19:00:27  john
 * Changed key_down_time to return fixed seconds instead of
 * milliseconds.
 * 
 * Revision 1.19  1994/08/19  15:22:12  mike
 * Fix divide overflow in sliding.
 * 
 * Revision 1.18  1994/08/19  14:42:50  john
 * Added joystick sensitivity.
 * 
 * Revision 1.17  1994/08/17  16:50:01  john
 * Added damaging fireballs, missiles.
 * 
 * Revision 1.16  1994/08/12  22:41:54  john
 * Took away Player_stats; added Players array.
 * 
 * Revision 1.15  1994/08/09  16:03:56  john
 * Added network players to editor.
 * 
 * Revision 1.14  1994/07/28  12:33:31  matt
 * Made sliding use thrust, rather than changing velocity directly
 * 
 * Revision 1.13  1994/07/27  20:53:21  matt
 * Added rotational drag & thrust, so turning now has momemtum like moving
 * 
 * Revision 1.12  1994/07/25  10:24:06  john
 * Victor stuff.
 * 
 * Revision 1.11  1994/07/22  17:53:16  john
 * Added better victormax support
 * 
 * Revision 1.10  1994/07/21  21:31:29  john
 * First cheapo version of VictorMaxx tracking.
 * 
 * Revision 1.9  1994/07/15  15:16:18  john
 * Fixed some joystick stuff.
 * 
 * Revision 1.8  1994/07/15  09:32:09  john
 * Changes player movement.
 * 
 * Revision 1.7  1994/07/13  00:14:58  matt
 * Moved all (or nearly all) of the values that affect player movement to 
 * bitmaps.tbl
 * 
 * Revision 1.6  1994/07/12  12:40:14  matt
 * Revamped physics system
 * 
 * Revision 1.5  1994/07/02  13:50:39  matt
 * Cleaned up includes
 * 
 * Revision 1.4  1994/07/01  10:55:25  john
 * Added analog joystick throttle
 * 
 * Revision 1.3  1994/06/30  20:04:28  john
 * Added -joydef support.
 * 
 * Revision 1.2  1994/06/30  19:01:58  matt
 * Moved flying controls code from physics.c to controls.c
 * 
 * Revision 1.1  1994/06/30  18:41:25  matt
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: controls.c,v 1.1.1.1 2006/03/17 19:43:28 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdlib.h>

#include "mono.h"
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "render.h"
#include "args.h"
#include "palette.h"
#include "mouse.h"
#include "kconfig.h"

//added on 11/20/99 by Victor Rachels for observer mode
#include "observer.h"
//end this section addition - VR

//look at keyboard, mouse, joystick, CyberMan, whatever, and set 
//physics vars rotvel, velocity

void read_flying_controls( object * obj )
{
	fix	afterburner_thrust;

	Assert(FrameTime > 0); 		//Get MATT if hit this!

//this section commented and moved to the bottom by WraithX
//	if (Player_is_dead) {
//		vm_vec_zero(&obj->mtype.phys_info.rotthrust);
//		vm_vec_zero(&obj->mtype.phys_info.thrust);
//		return;
//	}
//end of section to be moved.

//added/edited on 11/20/99 by Victor Rachels to add observer mode
//        if ((obj->type!=OBJ_PLAYER) || (obj->id!=Player_num)) return;   //references to player_ship require that this obj be the player
        if(obj->id!=Player_num || (obj->type==OBJ_GHOST && !I_am_observer)) return;
//end this section edit - VR

	//	Couldn't the "50" in the next three lines be changed to "64" with no ill effect?
	obj->mtype.phys_info.rotthrust.x = Controls.pitch_time;
	obj->mtype.phys_info.rotthrust.y = Controls.heading_time;
	obj->mtype.phys_info.rotthrust.z = Controls.bank_time;

//	mprintf( (0, "Rot thrust = %.3f,%.3f,%.3f\n", f2fl(obj->mtype.phys_info.rotthrust.x),f2fl(obj->mtype.phys_info.rotthrust.y), f2fl(obj->mtype.phys_info.rotthrust.z) ));

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
		fix_fastsincos(GameTime, &swiggle, NULL);
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
			mprintf((0, "Preventing divide overflow in controls.c for Max_thrust!\n"));
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale( &obj->mtype.phys_info.thrust, fixdiv(Player_ship->max_thrust,ft) );

		if ((ft < F1_0/2) && (ft << 15 <= Player_ship->max_rotthrust)) {
			mprintf((0, "Preventing divide overflow in controls.c for max_rotthrust!\n"));
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
