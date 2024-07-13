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

#include "dxxsconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ranges>

#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"		// For FrameTime
#include "dxxerror.h"
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
#include "gamesave.h"
#include "player.h"
#include "collide.h"
#include "multi.h"
#include "multibot.h"
#include "escort.h"
#include "compiler-poison.h"
#include "d_enumerate.h"
#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "partial_range.h"
#include "segiter.h"

// The max number of fuel stations per mine.

namespace dcx {
constexpr fix Fuelcen_give_amount = i2f(25);
constexpr fix Fuelcen_max_amount = i2f(100);

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
constexpr fix EnergyToCreateOneRobot = i2f(1);

constexpr std::integral_constant<int, 15> Num_extry_robots{};
}
namespace dsx {
#if DXX_USE_EDITOR
const enumerated_array<char[11], MAX_CENTER_TYPES, segment_special> Special_names = {{{
	{"NOTHING"},
	{"FUELCEN"},
	{"REPAIRCEN"},
	{"CONTROLCEN"},
	{"ROBOTMAKER"},
#if defined(DXX_BUILD_DESCENT_II)
	{"GOAL_RED"},
	{"GOAL_BLUE"},
#endif
}}};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void fuelcen_reset()
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	DXX_MAKE_MEM_UNDEFINED(std::span(Station));
	DXX_MAKE_MEM_UNDEFINED(std::span(RobotCenters));
	LevelSharedRobotcenterState.Num_robot_centers = 0;
	LevelUniqueFuelcenterState.Num_fuelcenters = 0;
	range_for (shared_segment &i, Segments)
		i.special = segment_special::nothing;
}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
__attribute_used
static void reset_all_robot_centers()
{
	// Remove all materialization centers
	range_for (shared_segment &i, partial_range(Segments, LevelSharedSegmentState.Num_segments))
		if (i.special == segment_special::robotmaker)
		{
			i.special = segment_special::nothing;
			i.matcen_num = materialization_center_number::None;
		}
}
#endif

//------------------------------------------------------------
// Turns a segment into a fully charged up fuel center...
void fuelcen_create(const vmsegptridx_t segp)
{
	auto &Station = LevelUniqueFuelcenterState.Station;
	auto station_type = segp->special;
	switch(station_type)
	{
		case segment_special::nothing:
#if defined(DXX_BUILD_DESCENT_II)
		case segment_special::goal_blue:
		case segment_special::goal_red:
#endif
		return;
		case segment_special::fuelcen:
		case segment_special::repaircen:
		case segment_special::controlcen:
		case segment_special::robotmaker:
		break;
	default:
			Error("Invalid station type %d in fuelcen.c\n", underlying_value(station_type));
	}

	const auto next_fuelcenter_idx = static_cast<station_number>(LevelUniqueFuelcenterState.Num_fuelcenters++);
	segp->station_idx = next_fuelcenter_idx;
	auto &station = Station.at(next_fuelcenter_idx);
	station.Type = station_type;
	station.Capacity = Fuelcen_max_amount;
	station.segnum = segp;
	station.Timer = -1;
	station.Flag = 0;
}

namespace {

//------------------------------------------------------------
// Adds a matcen that already is a special type into the Station array.
// This function is separate from other fuelcens because we don't want values reset.
static void matcen_create(const vmsegptridx_t segp)
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	const auto station_type = segp->special;

	assert(station_type == segment_special::robotmaker);

	const auto next_fuelcenter_idx = static_cast<station_number>(LevelUniqueFuelcenterState.Num_fuelcenters++);
	segp->station_idx = next_fuelcenter_idx;
	auto &station = Station.at(next_fuelcenter_idx);

	station.Type = station_type;
	station.Capacity = i2f(underlying_value(GameUniqueState.Difficulty_level) + 3);
	station.segnum = segp;
	station.Timer = -1;
	station.Flag = 0;

	const auto next_robot_center_idx = static_cast<materialization_center_number>(LevelSharedRobotcenterState.Num_robot_centers++);
	segp->matcen_num = next_robot_center_idx;
	auto &robotcenter = RobotCenters[next_robot_center_idx];
	robotcenter.fuelcen_num = next_fuelcenter_idx;
	robotcenter.segnum = segp;
}

}

//------------------------------------------------------------
// Adds a segment that already is a special type into the Station array.
void fuelcen_activate(const vmsegptridx_t segp)
{
	if (segp->special == segment_special::robotmaker)
		matcen_create( segp);
	else
		fuelcen_create( segp);
}

//------------------------------------------------------------
//	Trigger (enable) the materialization center in segment segnum
void trigger_matcen(const vmsegptridx_t segp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	FuelCenter	*robotcen;

	assert(segp->special == segment_special::robotmaker);
	assert(underlying_value(segp->matcen_num) < LevelUniqueFuelcenterState.Num_fuelcenters);

	robotcen = &Station[RobotCenters[segp->matcen_num].fuelcen_num];

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

	const auto Difficulty_level = GameUniqueState.Difficulty_level;
#if defined(DXX_BUILD_DESCENT_II)
	//	MK: 11/18/95, At insane, matcens work forever!
	if (Difficulty_level != Difficulty_level_type::_4)
#endif
		robotcen->Lives--;

	robotcen->Timer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f(underlying_value(Difficulty_level) + 3);
//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
	const auto MATCEN_LIFE = i2f(30 - 2 * underlying_value(Difficulty_level));
	robotcen->Disable_time = MATCEN_LIFE;

	//	Create a bright object in the segment.
	auto &vcvertptr = Vertices.vcptr;
	auto pos{compute_segment_center(vcvertptr, segp)};
	const auto &&delta{vm_vec_sub(vcvertptr(segp->verts.front()), pos)};
	vm_vec_scale_add2(pos, delta, F1_0/2);
	const auto &&objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_LIGHT, 0, segp, pos, nullptr, 0, object::control_type::light, object::movement_type::None, render_type::RT_NONE);
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
void fuelcen_delete(shared_segment &segp)
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	auto Num_fuelcenters = LevelUniqueFuelcenterState.Num_fuelcenters;
Restart: ;
	segp.special = segment_special::nothing;

	for (auto &&[i, fi] : enumerate(partial_range(Station, Num_fuelcenters)))
	{
		if (vmsegptr(fi.segnum) == &segp)
		{

			auto &Num_robot_centers = LevelSharedRobotcenterState.Num_robot_centers;
			// If Robot maker is deleted, fix Segments and RobotCenters.
			if (fi.Type == segment_special::robotmaker)
			{
				if (!Num_robot_centers)
				{
					con_printf(CON_URGENT, "%s:%u: error: Num_robot_centers=0 while deleting robot maker", __FILE__, __LINE__);
					return;
				}
				const auto &&range = partial_range(RobotCenters, underlying_value(segp.matcen_num), Num_robot_centers--);

				std::move(std::next(range.begin()), range.end(), range.begin());
				range_for (auto &fj, partial_const_range(Station, Num_fuelcenters))
				{
					if (fj.Type == segment_special::robotmaker)
					{
						shared_segment &sfj = vmsegptr(fj.segnum);
						if (sfj.matcen_num > segp.matcen_num)
							sfj.matcen_num = static_cast<materialization_center_number>(underlying_value(sfj.matcen_num) - 1);
					}
				}
			}

			//fix RobotCenters so they point to correct fuelcenter
			range_for (auto &j, partial_range(RobotCenters, Num_robot_centers))
				if (j.fuelcen_num > i)		//this robotcenter's fuelcen is changing
					j.fuelcen_num = static_cast<station_number>(underlying_value(j.fuelcen_num) - 1);
			Assert(Num_fuelcenters > 0);
			Num_fuelcenters--;
			for (auto &&[j, fj] : enumerate(partial_range(Station, underlying_value(i), Num_fuelcenters)))
			{
				fj = std::move(Station[static_cast<station_number>(underlying_value(j) + 1)]);
				Segments[fj.segnum].station_idx = j;
			}
			goto Restart;
		}
	}
	LevelUniqueFuelcenterState.Num_fuelcenters = Num_fuelcenters;
}
#endif

#define	ROBOT_GEN_TIME (i2f(5))

imobjptridx_t create_morph_robot(const d_robot_info_array &Robot_info, const vmsegptridx_t segp, const vms_vector &object_pos, const robot_id object_id)
{
	ai_behavior default_behavior;
	auto &robptr = Robot_info[object_id];
#if defined(DXX_BUILD_DESCENT_I)
	default_behavior = ai_behavior::AIB_NORMAL;
	if (object_id == robot_id::toaster)						//	This is a toaster guy!
		default_behavior = ai_behavior::AIB_RUN_FROM;
#elif defined(DXX_BUILD_DESCENT_II)
	default_behavior = robptr.behavior;
#endif

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	auto obj = robot_create(Robot_info, object_id, segp, object_pos,
				&vmd_identity_matrix, Polygon_models[robptr.model_num].rad,
				default_behavior);

	if (obj == object_none)
	{
		Int3();
		return object_none;
	}

	++LevelUniqueObjectState.accumulated_robots;
	++GameUniqueState.accumulated_robots;
	//Set polygon-object-specific data

	obj->rtype.pobj_info.model_num = robptr.model_num;
	obj->rtype.pobj_info.subobj_flags = 0;

	//set Physics info

	obj->mtype.phys_info.mass = robptr.mass;
	obj->mtype.phys_info.drag = robptr.drag;

	obj->mtype.phys_info.flags |= (PF_LEVELLING);

	obj->shields = robptr.strength;

	create_n_segment_path(obj, robptr, 6, segment_none);		//	Create a 6 segment path from creation point.

#if defined(DXX_BUILD_DESCENT_I)
	if (default_behavior == ai_behavior::AIB_RUN_FROM)
		obj->ctype.ai_info.ail.mode = ai_mode::AIM_RUN_FROM_OBJECT;
#elif defined(DXX_BUILD_DESCENT_II)
	obj->ctype.ai_info.ail.mode = ai_behavior_to_mode(default_behavior);
#endif

	return obj;
}

}

namespace {

//	----------------------------------------------------------------------------------------------------------
static void robotmaker_proc(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, fvmsegptridx &vmsegptridx, FuelCenter *const robotcen, const station_number numrobotcen)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptridx = Objects.vmptridx;
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
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
	const auto matcen_num = segp->matcen_num;

	if (!RobotCenters.valid_index(matcen_num))
		return;

	matcen_info *mi = &RobotCenters[matcen_num];
	for (unsigned i = 0;; ++i)
	{
		if (i >= mi->robot_flags.size())
			return;
		if (mi->robot_flags[i])
			break;
	}

	// Wait until we have a free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	{
		auto &plr = get_local_player();
		if (LevelUniqueObjectState.accumulated_robots - plr.num_kills_level >= Gamesave_num_org_robots + Num_extry_robots)
		{
		return;
		}
	}

	robotcen->Timer += FrameTime;

	auto &vcvertptr = Vertices.vcptr;
	switch( robotcen->Flag )	{
	case 0:		// Wait until next robot can generate
		if (Game_mode & GM_MULTI)
		{
			top_time = ROBOT_GEN_TIME;	
		}
		else
		{
			const auto center{compute_segment_center(vcvertptr, segp)};
			const auto dist_to_player = vm_vec_dist_quick( ConsoleObject->pos, center );
			top_time = dist_to_player/64 + d_rand() * 2 + F1_0*2;
			if ( top_time > ROBOT_GEN_TIME )
				top_time = ROBOT_GEN_TIME + d_rand();
			if ( top_time < F1_0*2 )
				top_time = F1_0*3/2 + d_rand()*2;
		}

		if (robotcen->Timer > top_time )	{
			int	count=0;
			const auto biased_matcen_creator = underlying_value(numrobotcen) ^ 0x80;

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			for (auto &obj : vcobjptr)
			{
				if (obj.type == OBJ_ROBOT)
					if (obj.matcen_creator == biased_matcen_creator)
						count++;
			}
			if (count > underlying_value(GameUniqueState.Difficulty_level) + 3)
			{
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
					collide_robot_and_materialization_center(Robot_info, objp);
					robotcen->Timer = top_time/2;
					return;
				} else if (objp->type==OBJ_PLAYER ) {
					collide_player_and_materialization_center(objp);
					robotcen->Timer = top_time/2;
					return;
				}
			}

			const auto cur_object_loc{compute_segment_center(vcvertptr, csegp)};
			const auto &&robotcen_segp = vmsegptridx(robotcen->segnum);
			// HACK!!! The 10 under here should be something equal to the 1/2 the size of the segment.
			auto obj = object_create_explosion_without_damage(Vclip, robotcen_segp, cur_object_loc, i2f(10), vclip_index::morphing_robot);

			if (obj != object_none)
				extract_orient_from_segment(vcvertptr, obj->orient, robotcen_segp);

			digi_link_sound_to_pos(Vclip[vclip_index::morphing_robot].sound_num, robotcen_segp, sidenum_t::WLEFT, cur_object_loc, 0, F1_0);
			robotcen->Flag	= 1;
			robotcen->Timer = 0;

		}
		break;
	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > Vclip[vclip_index::morphing_robot].play_time / 2)
		{

			robotcen->Capacity -= EnergyToCreateOneRobot;
			robotcen->Flag = 0;

			robotcen->Timer = 0;
			const auto cur_object_loc{compute_segment_center(vcvertptr, vcsegptr(robotcen->segnum))};

			// If this is the first materialization, set to valid robot.
			{
				std::array<robot_id, sizeof(mi->robot_flags) * 8> legal_types;   // the width of robot_flags[].
				int	num_types;

				num_types = 0;
				for (unsigned i = 0;; ++i)
				{
					if (i >= mi->robot_flags.size())
						break;
					uint32_t flags = mi->robot_flags[i];
					for (unsigned j = 0; flags && j < 8 * sizeof(flags); ++j)
					{
						if (flags & 1)
							legal_types[num_types++] = static_cast<robot_id>((i * 32) + j);
						flags >>= 1;
					}
				}

				const robot_id type = (num_types == 1)
					? legal_types[0]
					: legal_types[(d_rand() * num_types) / 32768];

				const auto &&obj = create_morph_robot(Robot_info, vmsegptridx(robotcen->segnum), cur_object_loc, type );
				if (obj != object_none) {
					if (Game_mode & GM_MULTI)
						multi_send_create_robot(numrobotcen, obj, type);
					obj->matcen_creator = underlying_value(numrobotcen) | 0x80;

					// Make object faces player...
					vm_vector_to_matrix_u(obj->orient, vm_vec_sub(ConsoleObject->pos, obj->pos), obj->orient.uvec);
	
					morph_start(LevelUniqueMorphObjectState, LevelSharedPolygonModelState, obj);
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

}

//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void fuelcen_update_all(const d_robot_info_array &Robot_info)
{
	auto &Station = LevelUniqueFuelcenterState.Station;
	for (auto &&[idx, i] : enumerate(partial_range(Station, LevelUniqueFuelcenterState.Num_fuelcenters)))
	{
		if (i.Type == segment_special::robotmaker)
		{
			if (! (Game_suspended & SUSP_ROBOTS))
				robotmaker_proc(Robot_info, Vclip, vmsegptridx, &i, idx);
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
	static fix64 last_play_time{};

	if (segp.special == segment_special::fuelcen)
	{
		fix amount;

#if defined(DXX_BUILD_DESCENT_II)
		detect_escort_goal_fuelcen_accomplished();
#endif

		if (MaxAmountCanTake <= 0 )	{
			return 0;
		}

		amount = fixmul(FrameTime,Fuelcen_give_amount);

		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;

		if (last_play_time + FUELCEN_SOUND_DELAY < GameTime64 || last_play_time > GameTime64)
		{
			last_play_time = {GameTime64};
			multi_digi_play_sample(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
		}
		return amount;
	} else {
		return 0;
	}
}

}

namespace dcx {

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix repaircen_give_shields(const shared_segment &segp, const fix MaxAmountCanTake)
{
	static fix64 last_play_time{};

	if (segp.special == segment_special::repaircen)
	{
		fix amount;
		if (MaxAmountCanTake <= 0 ) {
			return 0;
		}
		amount = fixmul(FrameTime,Fuelcen_give_amount);
		if (amount > MaxAmountCanTake )
			amount = MaxAmountCanTake;
		if (last_play_time > GameTime64)
			last_play_time = 0;
		if (GameTime64 > last_play_time+FUELCEN_SOUND_DELAY) {
			last_play_time = {GameTime64};
			multi_digi_play_sample(SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
		}
		return amount;
	} else {
		return 0;
	}
}

}

namespace dsx {

//	--------------------------------------------------------------------------------------------
void disable_matcens(void)
{
	auto &Station = LevelUniqueFuelcenterState.Station;
	range_for (auto &s, partial_range(Station, LevelUniqueFuelcenterState.Num_fuelcenters))
		if (s.Type == segment_special::robotmaker)
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
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	const auto Num_fuelcenters = LevelUniqueFuelcenterState.Num_fuelcenters;
	const auto &&robot_range = partial_const_range(RobotCenters, LevelSharedRobotcenterState.Num_robot_centers);
	for (auto &&[i, station] : enumerate(partial_range(Station, Num_fuelcenters)))
		if (station.Type == segment_special::robotmaker)
		{
			station.Lives = 3;
			station.Enabled = 0;
			station.Disable_time = 0;
			//	Make sure this fuelcen is pointed at by a matcen.
			if (std::ranges::find(robot_range, i, &matcen_info::fuelcen_num) == robot_range.end())
			{
				station.Lives = 0;
				LevelError("Station %i has type robotmaker, but no robotmaker uses it; ignoring.", underlying_value(i));
			}
		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	range_for (auto &i, robot_range)
	{
		auto	fuelcen_num = i.fuelcen_num;
		assert(underlying_value(fuelcen_num) < Num_fuelcenters);
		assert(Station[fuelcen_num].Type == segment_special::robotmaker);
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

#define D1_MATCEN_V25_MEMBERLIST	(p.m->robot_flags[0], serial::pad<sizeof(fix) * 2>(), p.m->segnum, p.m->fuelcen_num, serial::pad<1>())
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

#define D1_MATCEN_V26_MEMBERLIST	(p.m->robot_flags[0], serial::pad<sizeof(uint32_t)>(), serial::pad<sizeof(fix) * 2>(), p.m->segnum, p.m->fuelcen_num, serial::pad<1>())
DEFINE_SERIAL_UDT_TO_MESSAGE(d1mi_v26, p, D1_MATCEN_V26_MEMBERLIST);
DEFINE_SERIAL_UDT_TO_MESSAGE(d1cmi_v26, p, D1_MATCEN_V26_MEMBERLIST);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(d1mi_v26, 20);

void matcen_info_read(const NamedPHYSFS_File fp, matcen_info &mi, int version)
{
	if (version > 25)
		PHYSFSX_serialize_read<const d1mi_v26>(fp, mi);
	else
		PHYSFSX_serialize_read<const d1mi_v25>(fp, mi);
}
#elif defined(DXX_BUILD_DESCENT_II)
void fuelcen_check_for_goal(object &plrobj, const shared_segment &segp)
{
	assert(game_mode_capture_flag(Game_mode));

	team_number check_team;
	powerup_type_t powerup_to_drop;
	switch(segp.special)
	{
		case segment_special::goal_blue:
			check_team = team_number::blue;
			powerup_to_drop = powerup_type_t::POW_FLAG_RED;
			break;
		case segment_special::goal_red:
			check_team = team_number::red;
			powerup_to_drop = powerup_type_t::POW_FLAG_BLUE;
			break;
		default:
			return;
	}
	if (get_team(Player_num) != check_team)
		return;
	auto &player_info = plrobj.ctype.player_info;
	if (player_info.powerup_flags & PLAYER_FLAGS_FLAG)
	{
		player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;
				multi_send_capture_bonus (Player_num);
		maybe_drop_net_powerup(powerup_to_drop, 1, 0);
	}
}

void fuelcen_check_for_hoard_goal(object &plrobj, const shared_segment &segp)
{
	assert(game_mode_hoard(Game_mode));

   if (Player_dead_state != player_dead_state::no)
		return;

	const auto special = segp.special;
	if (special == segment_special::goal_blue || special == segment_special::goal_red)
	{
		auto &player_info = plrobj.ctype.player_info;
		if (auto &hoard_orbs = player_info.hoard.orbs)
		{
				player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;
			multi_send_orb_bonus(Player_num, std::exchange(hoard_orbs, 0));
      }
	}

}


/*
 * reads an d1_matcen_info structure from a PHYSFS_File
 */
void d1_matcen_info_read(const NamedPHYSFS_File fp, matcen_info &mi)
{
	PHYSFSX_serialize_read<const d1mi_v25>(fp, mi);
	mi.robot_flags[1] = 0;
}

DEFINE_SERIAL_UDT_TO_MESSAGE(matcen_info, m, (m.robot_flags, serial::pad<sizeof(fix) * 2>(), m.segnum, m.fuelcen_num, serial::pad<1>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(matcen_info, 20);

void matcen_info_read(const NamedPHYSFS_File fp, matcen_info &mi)
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

DEFINE_SERIAL_UDT_TO_MESSAGE(FuelCenter, fc, (fc.Type, serial::pad<3>(), serial::sign_extend<int>(fc.segnum), fc.Flag, fc.Enabled, fc.Lives, serial::pad<1>(), fc.Capacity, serial::pad<sizeof(fix)>(), fc.Timer, fc.Disable_time, serial::pad<3 * sizeof(fix)>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(FuelCenter, 40);

void fuelcen_read(const NamedPHYSFS_File fp, FuelCenter &fc)
{
	PHYSFSX_serialize_read(fp, fc);
}

void fuelcen_write(PHYSFS_File *fp, const FuelCenter &fc)
{
	PHYSFSX_serialize_write(fp, fc);
}
