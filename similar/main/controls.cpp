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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Code for controlling player movement
 *
 */


#include <algorithm>
#include <stdlib.h>
#include "key.h"
#include "joy.h"
#include "timer.h"
#include "dxxerror.h"
#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"
#include "weapon.h"
#include "bm.h"
#include "controls.h"
#include "render.h"
#include "args.h"
#include "palette.h"
#include "mouse.h"
#include "kconfig.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "laser.h"
#include "multi.h"
#include "vclip.h"
#include "fireball.h"

//look at keyboard, mouse, joystick, CyberMan, whatever, and set 
//physics vars rotvel, velocity

fix Afterburner_charge=f1_0;

#define AFTERBURNER_USE_SECS	3				//use up in 3 seconds
#define DROP_DELTA_TIME			(f1_0/15)	//drop 3 per second
#endif

using std::min;
using std::max;

namespace dsx {
void read_flying_controls(object &obj)
{
	fix	forward_thrust_time;

	Assert(FrameTime > 0); 		//Get MATT if hit this!

#if defined(DXX_BUILD_DESCENT_II)
	if (obj.type != OBJ_PLAYER || get_player_id(obj) != Player_num)
		return;	//references to player_ship require that this obj be the player

	const auto control_guided_missile = [&] {
		const auto gm = Guided_missile[Player_num];
		if (!gm)
			return false;
		const auto &&m = vmobjptr(gm);
		if (m->type != OBJ_WEAPON)
			return false;
		if (m->signature != Guided_missile_sig[Player_num])
			return false;
		vms_angvec rotangs;
		//this is a horrible hack.  guided missile stuff should not be
		//handled in the middle of a routine that is dealing with the player

		vm_vec_zero(obj.mtype.phys_info.rotthrust);

		rotangs.p = Controls.pitch_time / 2 + Seismic_tremor_magnitude/64;
		rotangs.b = Controls.bank_time / 2 + Seismic_tremor_magnitude/16;
		rotangs.h = Controls.heading_time / 2 + Seismic_tremor_magnitude/64;

		const auto &&rotmat = vm_angles_2_matrix(rotangs);
		m->orient = vm_matrix_x_matrix(m->orient, rotmat);

		const auto speed = Weapon_info[get_weapon_id(*m)].speed[Difficulty_level];

		vm_vec_copy_scale(m->mtype.phys_info.velocity, m->orient.fvec, speed);
		if (Game_mode & GM_MULTI)
			multi_send_guided_info(m, 0);
		return true;
	};
	if (!control_guided_missile())
#endif
	{
		auto &rotthrust = obj.mtype.phys_info.rotthrust;
		rotthrust.x = Controls.pitch_time;
		rotthrust.y = Controls.heading_time;
		rotthrust.z = Controls.bank_time;
	}

	forward_thrust_time = Controls.forward_thrust_time;

#if defined(DXX_BUILD_DESCENT_II)
	auto &player_info = obj.ctype.player_info;
	if (player_info.powerup_flags & PLAYER_FLAGS_AFTERBURNER)
	{
		if (Controls.state.afterburner) {			//player has key down
			{
				fix afterburner_scale;
				int old_count,new_count;
	
				//add in value from 0..1
				afterburner_scale = f1_0 + min(f1_0/2,Afterburner_charge) * 2;
	
				forward_thrust_time = fixmul(FrameTime,afterburner_scale);	//based on full thrust
	
				old_count = (Afterburner_charge / (DROP_DELTA_TIME/AFTERBURNER_USE_SECS));

				Afterburner_charge -= FrameTime/AFTERBURNER_USE_SECS;

				if (Afterburner_charge < 0)
					Afterburner_charge = 0;

				new_count = (Afterburner_charge / (DROP_DELTA_TIME/AFTERBURNER_USE_SECS));

				if (old_count != new_count)
					Drop_afterburner_blob_flag = 1;	//drop blob (after physics called)
			}
		}
		else {
			fix cur_energy,charge_up;
	
			//charge up to full
			charge_up = min(FrameTime/8,f1_0 - Afterburner_charge);	//recharge over 8 seconds
	
			auto &energy = player_info.energy;
			cur_energy = max(energy - i2f(10), 0);	//don't drop below 10

			//maybe limit charge up by energy
			charge_up = min(charge_up,cur_energy/10);
	
			Afterburner_charge += charge_up;
	
			energy -= charge_up * 100 / 10;	//full charge uses 10% of energy
		}
	}
#endif

	// Set object's thrust vector for forward/backward
	vm_vec_copy_scale(obj.mtype.phys_info.thrust, obj.orient.fvec, forward_thrust_time );
	
	// slide left/right
	vm_vec_scale_add2(obj.mtype.phys_info.thrust, obj.orient.rvec, Controls.sideways_thrust_time );

	// slide up/down
	vm_vec_scale_add2(obj.mtype.phys_info.thrust, obj.orient.uvec, Controls.vertical_thrust_time );

	if (obj.mtype.phys_info.flags & PF_WIGGLE)
	{
		auto swiggle = fix_fastsin(static_cast<fix>(GameTime64));
		if (FrameTime < F1_0) // Only scale wiggle if getting at least 1 FPS, to avoid causing the opposite problem.
			swiggle = fixmul(swiggle*DESIGNATED_GAME_FPS, FrameTime); //make wiggle fps-independent (based on pre-scaled amount of wiggle at DESIGNATED_GAME_FPS)
		vm_vec_scale_add2(obj.mtype.phys_info.velocity, obj.orient.uvec, fixmul(swiggle, Player_ship->wiggle));
	}

	// As of now, obj.mtype.phys_info.thrust & obj->mtype.phys_info.rotthrust are 
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

		vm_vec_scale(obj.mtype.phys_info.thrust, fixdiv(Player_ship->max_thrust, ft));

		if ((ft < F1_0/2) && (ft << 15 <= Player_ship->max_rotthrust)) {
			ft = (Player_ship->max_thrust >> 15) + 1;
		}

		vm_vec_scale(obj.mtype.phys_info.rotthrust, fixdiv(Player_ship->max_rotthrust, ft));
	}

	// moved here by WraithX
	if (Player_dead_state != player_dead_state::no)
	{
		//vm_vec_zero(&obj.mtype.phys_info.rotthrust); // let dead players rotate, changed by WraithX
		vm_vec_zero(obj.mtype.phys_info.thrust);  // don't let dead players move, changed by WraithX
		return;
	}// end if

}
}
