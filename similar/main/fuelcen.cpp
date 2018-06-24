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
 * Functions for refueling centers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"		// For FrameTime
#include "dxxerror.h"
#include "gauges.h"
#include "vclip.h"
#include "fireball.h"
#include "robot.h"
#include "powerup.h"

#include "sounds.h"
#include "morph.h"
#include "3d.h"
#include "physfs-serial.h"
#include "bm.h"
#include "polyobj.h"
#include "ai.h"
#include "gamemine.h"
#include "gamesave.h"
#include "player.h"
#include "collide.h"
#include "laser.h"
#include "multi.h"
#include "multibot.h"
#include "escort.h"
#include "compiler-poison.h"
#include "d_enumerate.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

// The max number of fuel stations per mine.

namespace dcx {
constexpr fix Fuelcen_give_amount = i2f(25);
constexpr fix Fuelcen_max_amount = i2f(100);

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
constexpr fix EnergyToCreateOneRobot = i2f(1);

unsigned Num_robot_centers;
unsigned Num_fuelcenters;

static int Num_extry_robots = 15;
}
namespace dsx {
array<matcen_info, MAX_ROBOT_CENTERS> RobotCenters;

array<FuelCenter, MAX_NUM_FUELCENS> Station;

#if DXX_USE_EDITOR
const char	Special_names[MAX_CENTER_TYPES][11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
#if defined(DXX_BUILD_DESCENT_II)
	"GOAL_RED",
	"GOAL_BLUE",
#endif
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void fuelcen_reset()
{
	DXX_MAKE_MEM_UNDEFINED(Station.begin(), Station.end());
	DXX_MAKE_MEM_UNDEFINED(RobotCenters.begin(), RobotCenters.end());
	Num_fuelcenters = 0;
	range_for (auto &i, Segments)
		i.special = SEGMENT_IS_NOTHING;

	Num_robot_centers = 0;

}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
static void reset_all_robot_centers() __attribute_used;
static void reset_all_robot_centers()
{
	// Remove all materialization centers
	range_for (auto &i, partial_range(Segments, Num_segments))
		if (i.special == SEGMENT_IS_ROBOTMAKER)
		{
			i.special = SEGMENT_IS_NOTHING;
			i.matcen_num = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a segment into a fully charged up fuel center...
void fuelcen_create(const vmsegptridx_t segp)
{
	int	station_type;

	station_type = segp->special;

	switch( station_type )	{
	case SEGMENT_IS_NOTHING:
#if defined(DXX_BUILD_DESCENT_II)
	case SEGMENT_IS_GOAL_BLUE:
	case SEGMENT_IS_GOAL_RED:
#endif
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
		break;
	default:
		Error( "Invalid station type %d in fuelcen.c\n", station_type );
	}

	Assert( Num_fuelcenters < MAX_NUM_FUELCENS );

	segp->station_idx = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].Capacity = Fuelcen_max_amount;
	Station[Num_fuelcenters].segnum = segp;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;
	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a matcen that already is a special type into the Station array.
// This function is separate from other fuelcens because we don't want values reset.
static void matcen_create(const vmsegptridx_t segp)
{
	int	station_type = segp->special;

	Assert(station_type == SEGMENT_IS_ROBOTMAKER);

	Assert( Num_fuelcenters < MAX_NUM_FUELCENS );

	segp->station_idx = Num_fuelcenters;
	Station[Num_fuelcenters].Type = station_type;
	Station[Num_fuelcenters].Capacity = i2f(Difficulty_level + 3);

	Station[Num_fuelcenters].segnum = segp;
	Station[Num_fuelcenters].Timer = -1;
	Station[Num_fuelcenters].Flag = 0;

	segp->matcen_num = Num_robot_centers;
	Num_robot_centers++;

	RobotCenters[segp->matcen_num].segnum = segp;
	RobotCenters[segp->matcen_num].fuelcen_num = Num_fuelcenters;

	Num_fuelcenters++;
}

//------------------------------------------------------------
// Adds a segment that already is a special type into the Station array.
void fuelcen_activate(const vmsegptridx_t segp)
{
	if (segp->special == SEGMENT_IS_ROBOTMAKER)
		matcen_create( segp);
	else
		fuelcen_create( segp);
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f(30-2*Difficulty_level))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in segment segnum
void trigger_matcen(const vmsegptridx_t segnum)
{
	const auto &segp = segnum;
	FuelCenter	*robotcen;

	Assert(segp->special == SEGMENT_IS_ROBOTMAKER);
	Assert(segp->matcen_num < Num_fuelcenters);
	Assert((segp->matcen_num >= 0) && (segp->matcen_num <= Highest_segment_index));

	robotcen = &Station[RobotCenters[segp->matcen_num].fuelcen_num];

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

#if defined(DXX_BUILD_DESCENT_II)
	//	MK: 11/18/95, At insane, matcens work forever!
	if (Difficulty_level+1 < NDL)
#endif
		robotcen->Lives--;

	robotcen->Timer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f(Difficulty_level + 3);
	robotcen->Disable_time = MATCEN_LIFE;

	//	Create a bright object in the segment.
	auto pos = compute_segment_center(vcvertptr, segp);
	const auto delta = vm_vec_sub(vcvertptr(segnum->verts[0]), pos);
	vm_vec_scale_add2(pos, delta, F1_0/2);
	auto objnum = obj_create( OBJ_LIGHT, 0, segnum, pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE );
	if (objnum != object_none) {
		objnum->lifeleft = MATCEN_LIFE;
		objnum->ctype.light_info.intensity = i2f(8);	//	Light cast by a fuelcen.
	} else {
		Int3();
	}
}

#if DXX_USE_EDITOR
//------------------------------------------------------------
// Takes away a segment's fuel center properties.
//	Deletes the segment point entry in the FuelCenter list.
void fuelcen_delete(const vmsegptr_t segp)
{
Restart: ;
	segp->special = 0;

	for (uint_fast32_t i = 0; i < Num_fuelcenters; i++ )	{
		FuelCenter &fi = Station[i];
		if (vmsegptr(fi.segnum) == segp)
		{

			// If Robot maker is deleted, fix Segments and RobotCenters.
			if (fi.Type == SEGMENT_IS_ROBOTMAKER) {
				Assert(Num_robot_centers > 0);
				const auto &&range = partial_range(RobotCenters, static_cast<unsigned>(segp->matcen_num), Num_robot_centers);
				Num_robot_centers--;

				std::move(std::next(range.begin()), range.end(), range.begin());
				range_for (auto &fj, partial_const_range(Station, Num_fuelcenters))
				{
					if ( fj.Type == SEGMENT_IS_ROBOTMAKER )
						if ( Segments[fj.segnum].matcen_num > segp->matcen_num )
							Segments[fj.segnum].matcen_num--;
				}
			}

#if defined(DXX_BUILD_DESCENT_II)
			//fix RobotCenters so they point to correct fuelcenter
			range_for (auto &j, partial_range(RobotCenters, Num_robot_centers))
				if (j.fuelcen_num > i)		//this robotcenter's fuelcen is changing
					j.fuelcen_num--;
#endif
			Assert(Num_fuelcenters > 0);
			Num_fuelcenters--;
			for (uint_fast32_t j = i; j < Num_fuelcenters; j++ )	{
				Station[j] = Station[j+1];
				Segments[Station[j].segnum].station_idx = j;
			}
			goto Restart;
		}
	}

}
#endif

#define	ROBOT_GEN_TIME (i2f(5))

imobjptridx_t  create_morph_robot( const vmsegptridx_t segp, const vms_vector &object_pos, int object_id)
{
	auto &plr = get_local_player();
	plr.num_robots_level++;
	plr.num_robots_total++;

	ai_behavior default_behavior;
#if defined(DXX_BUILD_DESCENT_I)
	default_behavior = ai_behavior::AIB_NORMAL;
	if (object_id == 10)						//	This is a toaster guy!
		default_behavior = ai_behavior::AIB_RUN_FROM;
#elif defined(DXX_BUILD_DESCENT_II)
	default_behavior = Robot_info[object_id].behavior;
#endif

	auto obj = robot_create(object_id, segp, object_pos,
				&vmd_identity_matrix, Polygon_models[Robot_info[object_id].model_num].rad,
				default_behavior);

	if (obj == object_none)
	{
		Int3();
		return object_none;
	}

	//Set polygon-object-specific data

	obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
	obj->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	obj->mtype.phys_info.mass = Robot_info[get_robot_id(obj)].mass;
	obj->mtype.phys_info.drag = Robot_info[get_robot_id(obj)].drag;

	obj->mtype.phys_info.flags |= (PF_LEVELLING);

	obj->shields = Robot_info[get_robot_id(obj)].strength;

	create_n_segment_path(obj, 6, segment_none);		//	Create a 6 segment path from creation point.

#if defined(DXX_BUILD_DESCENT_I)
	if (default_behavior == ai_behavior::AIB_RUN_FROM)
		obj->ctype.ai_info.ail.mode = ai_mode::AIM_RUN_FROM_OBJECT;
#elif defined(DXX_BUILD_DESCENT_II)
	obj->ctype.ai_info.ail.mode = ai_behavior_to_mode(default_behavior);
#endif

	return obj;
}

}

//	----------------------------------------------------------------------------------------------------------
static void robotmaker_proc(fvmsegptridx &vmsegptridx, FuelCenter *const robotcen, const unsigned numrobotcen)
{
	int		matcen_num;
	fix		top_time;

	if (robotcen->Enabled == 0)
		return;

	if (robotcen->Disable_time > 0) {
		robotcen->Disable_time -= FrameTime;
		if (robotcen->Disable_time <= 0) {
			robotcen->Enabled = 0;
		}
	}

	//	No robot making in multiplayer mode.
	if ((Game_mode & GM_MULTI) && (!(Game_mode & GM_MULTI_ROBOTS) || !multi_i_am_master()))
		return;

	// Wait until transmorgafier has capacity to make a robot...
	if ( robotcen->Capacity <= 0 ) {
		return;
	}

	const auto &&segp = vmsegptr(robotcen->segnum);
	matcen_num = segp->matcen_num;

	if ( matcen_num == -1 ) {
		return;
	}

	matcen_info *mi = &RobotCenters[matcen_num];
	for (unsigned i = 0;; ++i)
	{
		if (i >= (sizeof(mi->robot_flags) / sizeof(mi->robot_flags[0])))
			return;
		if (mi->robot_flags[i])
			break;
	}

	// Wait until we have a free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	{
		auto &plr = get_local_player();
		if ((plr.num_robots_level - plr.num_kills_level) >= (Gamesave_num_org_robots + Num_extry_robots))
		{
		return;
		}
	}

	robotcen->Timer += FrameTime;

	switch( robotcen->Flag )	{
	case 0:		// Wait until next robot can generate
		if (Game_mode & GM_MULTI)
		{
			top_time = ROBOT_GEN_TIME;	
		}
		else
		{
			const auto center = compute_segment_center(vcvertptr, segp);
			const auto dist_to_player = vm_vec_dist_quick( ConsoleObject->pos, center );
			top_time = dist_to_player/64 + d_rand() * 2 + F1_0*2;
			if ( top_time > ROBOT_GEN_TIME )
				top_time = ROBOT_GEN_TIME + d_rand();
			if ( top_time < F1_0*2 )
				top_time = F1_0*3/2 + d_rand()*2;
		}

		if (robotcen->Timer > top_time )	{
			int	count=0;
			int	my_station_num = numrobotcen;

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			range_for (const auto &&objp, vcobjptr)
			{
				auto &obj = *objp;
				if (obj.type == OBJ_ROBOT)
					if ((obj.matcen_creator ^ 0x80) == my_station_num)
						count++;
			}
			if (count > Difficulty_level + 3) {
				robotcen->Timer /= 2;
				return;
			}

			//	Whack on any robot or player in the matcen segment.
			count=0;
			auto segnum = robotcen->segnum;
			const auto &&csegp = vmsegptr(segnum);
			range_for (const auto objp, objects_in(csegp, vmobjptridx, vmsegptr))
			{
				count++;
				if ( count > MAX_OBJECTS )	{
					Int3();
					return;
				}
				if (objp->type==OBJ_ROBOT) {
					collide_robot_and_materialization_center(objp);
					robotcen->Timer = top_time/2;
					return;
				} else if (objp->type==OBJ_PLAYER ) {
					collide_player_and_materialization_center(objp);
					robotcen->Timer = top_time/2;
					return;
				}
			}

			const auto &&cur_object_loc = compute_segment_center(vcvertptr, csegp);
			const auto &&robotcen_segp = vmsegptridx(robotcen->segnum);
			// HACK!!! The 10 under here should be something equal to the 1/2 the size of the segment.
			auto obj = object_create_explosion(robotcen_segp, cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT);

			if (obj != object_none)
				extract_orient_from_segment(&obj->orient, robotcen_segp);

			if ( Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1 )		{
				digi_link_sound_to_pos(Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen_segp, 0, cur_object_loc, 0, F1_0);
			}
			robotcen->Flag	= 1;
			robotcen->Timer = 0;

		}
		break;
	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > (Vclip[VCLIP_MORPHING_ROBOT].play_time/2) )	{

			robotcen->Capacity -= EnergyToCreateOneRobot;
			robotcen->Flag = 0;

			robotcen->Timer = 0;
			const auto &&cur_object_loc = compute_segment_center(vcvertptr, vcsegptr(robotcen->segnum));

			// If this is the first materialization, set to valid robot.
			{
				int	type;
				ubyte   legal_types[sizeof(mi->robot_flags) * 8];   // the width of robot_flags[].
				int	num_types;

				num_types = 0;
				for (unsigned i = 0;; ++i)
				{
					if (i >= (sizeof(mi->robot_flags) / sizeof(mi->robot_flags[0])))
						break;
					uint32_t flags = mi->robot_flags[i];
					for (unsigned j = 0; flags && j < 8 * sizeof(flags); ++j)
					{
						if (flags & 1)
							legal_types[num_types++] = (i * 32) + j;
						flags >>= 1;
					}
				}

				if (num_types == 1)
					type = legal_types[0];
				else
					type = legal_types[(d_rand() * num_types) / 32768];

				const auto &&obj = create_morph_robot(vmsegptridx(robotcen->segnum), cur_object_loc, type );
				if (obj != object_none) {
					if (Game_mode & GM_MULTI)
						multi_send_create_robot(numrobotcen, obj, type);
					obj->matcen_creator = (numrobotcen) | 0x80;

					// Make object faces player...
					const auto direction = vm_vec_sub(ConsoleObject->pos,obj->pos );
					vm_vector_2_matrix( obj->orient, direction, &obj->orient.uvec, nullptr);
	
					morph_start( obj );
					//robotcen->last_created_obj = obj;
					//robotcen->last_created_sig = robotcen->last_created_obj->signature;
				}
			}

		}
		break;
	default:
		robotcen->Flag = 0;
		robotcen->Timer = 0;
	}
}

//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void fuelcen_update_all()
{
	range_for (auto &&e, enumerate(partial_range(Station, Num_fuelcenters)))
	{
		auto &i = e.value;
		if (i.Type == SEGMENT_IS_ROBOTMAKER)
		{
			if (! (Game_suspended & SUSP_ROBOTS))
				robotmaker_proc(vmsegptridx, &i, e.idx);
		}
	}
}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, F1_0 / 3> FUELCEN_SOUND_DELAY{};
#elif defined(DXX_BUILD_DESCENT_II)
//play every half second
constexpr std::integral_constant<unsigned, F1_0 / 4> FUELCEN_SOUND_DELAY{};
#endif

//-------------------------------------------------------------
fix fuelcen_give_fuel(const shared_segment &segp, fix MaxAmountCanTake)
{
	static fix64 last_play_time = 0;

	if (segp.special == SEGMENT_IS_FUELCEN)
	{
		fix amount;

#if defined(DXX_BUILD_DESCENT_II)
		detect_escort_goal_fuelcen_accomplished();
#endif

//		if (Station[segp->value].Capacity<=0)	{
//			HUD_init_message(HM_DEFAULT, "Fuelcenter %d is empty.", segp->value );
//			return 0;
//		}

		if (MaxAmountCanTake <= 0 )	{
//			//gauge_message( "Fueled up!");
			return 0;
		}

		amount = fixmul(FrameTime,Fuelcen_give_amount);

		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;

//		if (!(Game_mode & GM_MULTI))
//			if ( Station[segp->value].Capacity < amount  )	{
//				amount = Station[segp->value].Capacity;
//				Station[segp->value].Capacity = 0;
//			} else {
//				Station[segp->value].Capacity -= amount;
//			}

		if (last_play_time + FUELCEN_SOUND_DELAY < GameTime64 || last_play_time > GameTime64)
		{
			last_play_time = GameTime64;
			multi_digi_play_sample(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
		}
		return amount;
	} else {
		return 0;
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix repaircen_give_shields(const vcsegptr_t segp, fix MaxAmountCanTake)
{
	static fix last_play_time=0;

	if (segp->special==SEGMENT_IS_REPAIRCEN) {
		fix amount;
//             detect_escort_goal_accomplished(-4);    //      UGLY! Hack! -4 means went through fuelcen.
//             if (Station[segp->value].Capacity<=0)   {
//                     HUD_init_message(HM_DEFAULT, "Repaircenter %d is empty.", segp->value );
//                     return 0;
//             }
		if (MaxAmountCanTake <= 0 ) {
			//gauge_message( "Shields restored!");
			return 0;
		}
		amount = fixmul(FrameTime,Fuelcen_give_amount);
		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;
//        if (!(Game_mode & GM_MULTI))
//                     if ( Station[segp->value].Capacity < amount  )  {
//                             amount = Station[segp->value].Capacity;
//                             Station[segp->value].Capacity = 0;
//                     } else {
//                             Station[segp->value].Capacity -= amount;
//                     }
		if (last_play_time > GameTime64)
			last_play_time = 0;
		if (GameTime64 > last_play_time+FUELCEN_SOUND_DELAY) {
			multi_digi_play_sample(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
			last_play_time = GameTime64;
		}
		return amount;
	} else {
		return 0;
	}
}
#endif

//	--------------------------------------------------------------------------------------------
void disable_matcens(void)
{
	range_for (auto &s, partial_range(Station, Num_fuelcenters))
		if (s.Type == SEGMENT_IS_ROBOTMAKER)
		{
			s.Enabled = 0;
			s.Disable_time = 0;
		}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void init_all_matcens(void)
{
	const auto &&robot_range = partial_const_range(RobotCenters, Num_robot_centers);
	for (uint_fast32_t i = 0; i < Num_fuelcenters; i++)
		if (Station[i].Type == SEGMENT_IS_ROBOTMAKER) {
			Station[i].Lives = 3;
			Station[i].Enabled = 0;
			Station[i].Disable_time = 0;
			//	Make sure this fuelcen is pointed at by a matcen.
			if (std::find_if(robot_range.begin(), robot_range.end(), [i](const matcen_info &mi) {
				return mi.fuelcen_num == i;
			}) == robot_range.end())
			{
				Station[i].Lives = 0;
				LevelError("Station %" PRIuFAST32 " has type robotmaker, but no robotmaker uses it; ignoring.", i);
			}
		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	range_for (auto &i, robot_range)
	{
		auto	fuelcen_num = i.fuelcen_num;
		Assert(fuelcen_num < Num_fuelcenters);
		Assert(Station[fuelcen_num].Type == SEGMENT_IS_ROBOTMAKER);
	}
#endif

}

}

struct d1mi_v25
{
	matcen_info *m;
	d1mi_v25(matcen_info &mi) : m(&mi) {}
};

struct d1cmi_v25
{
	const matcen_info *m;
	d1cmi_v25(const matcen_info &mi) : m(&mi) {}
};

#define D1_MATCEN_V25_MEMBERLIST	(p.m->robot_flags[0], serial::pad<sizeof(fix) * 2>(), p.m->segnum, p.m->fuelcen_num)
DEFINE_SERIAL_UDT_TO_MESSAGE(d1mi_v25, p, D1_MATCEN_V25_MEMBERLIST);
DEFINE_SERIAL_UDT_TO_MESSAGE(d1cmi_v25, p, D1_MATCEN_V25_MEMBERLIST);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(d1mi_v25, 16);

namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
struct d1mi_v26
{
	matcen_info *m;
	d1mi_v26(matcen_info &mi) : m(&mi) {}
};

struct d1cmi_v26
{
	const matcen_info *m;
	d1cmi_v26(const matcen_info &mi) : m(&mi) {}
};

#define D1_MATCEN_V26_MEMBERLIST	(p.m->robot_flags[0], serial::pad<sizeof(uint32_t)>(), serial::pad<sizeof(fix) * 2>(), p.m->segnum, p.m->fuelcen_num)
DEFINE_SERIAL_UDT_TO_MESSAGE(d1mi_v26, p, D1_MATCEN_V26_MEMBERLIST);
DEFINE_SERIAL_UDT_TO_MESSAGE(d1cmi_v26, p, D1_MATCEN_V26_MEMBERLIST);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(d1mi_v26, 20);

void matcen_info_read(PHYSFS_File *fp, matcen_info &mi, int version)
{
	if (version > 25)
		PHYSFSX_serialize_read<const d1mi_v26>(fp, mi);
	else
		PHYSFSX_serialize_read<const d1mi_v25>(fp, mi);
}
#elif defined(DXX_BUILD_DESCENT_II)
void fuelcen_check_for_goal(const vcsegptr_t segp)
{
	Assert (game_mode_capture_flag());

	unsigned check_team;
	powerup_type_t powerup_to_drop;
	switch(segp->special)
	{
		case SEGMENT_IS_GOAL_BLUE:
			check_team = TEAM_BLUE;
			powerup_to_drop = POW_FLAG_RED;
			break;
		case SEGMENT_IS_GOAL_RED:
			check_team = TEAM_RED;
			powerup_to_drop = POW_FLAG_BLUE;
			break;
		default:
			return;
	}
	if (get_team(Player_num) != check_team)
		return;
	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	if (player_info.powerup_flags & PLAYER_FLAGS_FLAG)
	{
		player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;
				multi_send_capture_bonus (Player_num);
		maybe_drop_net_powerup(powerup_to_drop, 1, 0);
	}
}

void fuelcen_check_for_hoard_goal(const vcsegptr_t segp)
{
	Assert (game_mode_hoard());

   if (Player_dead_state != player_dead_state::no)
		return;

	if (segp->special==SEGMENT_IS_GOAL_BLUE || segp->special==SEGMENT_IS_GOAL_RED  )	
	{
		auto &player_info = get_local_plrobj().ctype.player_info;
		if (auto &hoard_orbs = player_info.hoard.orbs)
		{
				player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;
			multi_send_orb_bonus(Player_num, exchange(hoard_orbs, 0));
      }
	}

}


/*
 * reads an d1_matcen_info structure from a PHYSFS_File
 */
void d1_matcen_info_read(PHYSFS_File *fp, matcen_info &mi)
{
	PHYSFSX_serialize_read<const d1mi_v25>(fp, mi);
	mi.robot_flags[1] = 0;
}

DEFINE_SERIAL_UDT_TO_MESSAGE(matcen_info, m, (m.robot_flags, serial::pad<sizeof(fix) * 2>(), m.segnum, m.fuelcen_num));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(matcen_info, 20);

void matcen_info_read(PHYSFS_File *fp, matcen_info &mi)
{
	PHYSFSX_serialize_read(fp, mi);
}
#endif

void matcen_info_write(PHYSFS_File *fp, const matcen_info &mi, short version)
{
	if (version >= 27)
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_serialize_write<d1cmi_v26>(fp, mi);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_serialize_write(fp, mi);
#endif
	else
		PHYSFSX_serialize_write<d1cmi_v25>(fp, mi);
}
}

DEFINE_SERIAL_UDT_TO_MESSAGE(FuelCenter, fc, (serial::sign_extend<int>(fc.Type), serial::sign_extend<int>(fc.segnum), fc.Flag, fc.Enabled, fc.Lives, serial::pad<1>(), fc.Capacity, serial::pad<sizeof(fix)>(), fc.Timer, fc.Disable_time, serial::pad<3 * sizeof(fix)>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(FuelCenter, 40);

void fuelcen_read(PHYSFS_File *fp, FuelCenter &fc)
{
	PHYSFSX_serialize_read(fp, fc);
}

void fuelcen_write(PHYSFS_File *fp, const FuelCenter &fc)
{
	PHYSFSX_serialize_write(fp, fc);
}
