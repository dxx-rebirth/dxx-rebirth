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
 * Code for handling robots
 *
 */


#include "dxxerror.h"
#include "inferno.h"
#include "robot.h"
#include "object.h"
#include "polyobj.h"
#include "physfsx.h"

#include "compiler-range_for.h"
#include "partial_range.h"

namespace dcx {
unsigned N_robot_types;
unsigned N_robot_joints;
}

namespace dsx {
//	Robot stuff
array<robot_info, MAX_ROBOT_TYPES> Robot_info;

//Big array of joint positions.  All robots index into this array
array<jointpos, MAX_ROBOT_JOINTS> Robot_joints;
}

#if 0
static inline void PHYSFSX_writeAngleVec(PHYSFS_File *fp, const vms_angvec &v)
{
	PHYSFS_writeSLE16(fp, v.p);
	PHYSFS_writeSLE16(fp, v.b);
	PHYSFS_writeSLE16(fp, v.h);
}
#endif

namespace dsx {

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(vms_vector &gun_point, const object_base &obj, unsigned gun_num)
{
	Assert(obj.render_type == RT_POLYOBJ || obj.render_type == RT_MORPH);
	Assert(get_robot_id(obj) < N_robot_types);

	const auto &r = Robot_info[get_robot_id(obj)];
	const auto &pm = Polygon_models[r.model_num];

	if (gun_num >= r.n_guns)
	{
		gun_num = 0;
	}

	auto pnt = r.gun_points[gun_num];

	//instance up the tree for this gun
	auto &anim_angles = obj.rtype.pobj_info.anim_angles;
	for (unsigned mn = r.gun_submodels[gun_num]; mn != 0; mn = pm.submodel_parents[mn])
	{
		const auto &&m = vm_transposed_matrix(vm_angles_2_matrix(anim_angles[mn]));
		const auto tpnt = vm_vec_rotate(pnt,m);

		vm_vec_add(pnt, tpnt, pm.submodel_offsets[mn]);
	}

	//now instance for the entire object

	const auto &&m = vm_transposed_matrix(obj.orient);
	vm_vec_rotate(gun_point,pnt,m);
	vm_vec_add2(gun_point, obj.pos);
}

//fills in ptr to list of joints, and returns the number of joints in list
//takes the robot type (object id), gun number, and desired state
partial_range_t<const jointpos *> robot_get_anim_state(const array<robot_info, MAX_ROBOT_TYPES> &robot_info, const array<jointpos, MAX_ROBOT_JOINTS> &robot_joints, const unsigned robot_type, const unsigned gun_num, const unsigned state)
{
	auto &rirt = robot_info[robot_type];
	assert(gun_num <= rirt.n_guns);
	auto &as = rirt.anim_states[gun_num][state];
	const unsigned o = as.offset;
	return partial_range(robot_joints, o, o + as.n_joints);
}

#ifndef NDEBUG
//for test, set a robot to a specific state
static void set_robot_state(object_base &obj, const unsigned state) __attribute_used;
static void set_robot_state(object_base &obj, const unsigned state)
{
	int g,j,jo;
	jointlist *jl;

	assert(obj.type == OBJ_ROBOT);

	auto &ri = Robot_info[get_robot_id(obj)];

	for (g = 0; g < ri.n_guns + 1; g++)
	{

		jl = &ri.anim_states[g][state];

		jo = jl->offset;

		for (j=0;j<jl->n_joints;j++,jo++) {
			int jn;

			jn = Robot_joints[jo].jointnum;

			obj.rtype.pobj_info.anim_angles[jn] = Robot_joints[jo].angles;
		}
	}
}
#endif

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(robot_info *r,polymodel *pm,array<array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES> &angs)
{
	int g,state;
	array<int, MAX_SUBMODELS> gun_nums;			//which gun each submodel is part of

	range_for (auto &m, partial_range(gun_nums, 1u, pm->n_models))
		m = r->n_guns;		//assume part of body...

	gun_nums[0] = -1;		//body never animates, at least for now

	for (g=0;g<r->n_guns;g++) {
		auto m = r->gun_submodels[g];

		while (m != 0) {
			gun_nums[m] = g;				//...unless we find it in a gun
			m = pm->submodel_parents[m];
		}
	}

	for (g=0;g<r->n_guns+1;g++) {

		for (state=0;state<N_ANIM_STATES;state++) {

			r->anim_states[g][state].n_joints = 0;
			r->anim_states[g][state].offset = N_robot_joints;

			for (unsigned m = 0; m < pm->n_models; ++m)
			{
				if (gun_nums[m] == g) {
					Robot_joints[N_robot_joints].jointnum = m;
					Robot_joints[N_robot_joints].angles = angs[state][m];
					r->anim_states[g][state].n_joints++;
					N_robot_joints++;
					Assert(N_robot_joints < MAX_ROBOT_JOINTS);
				}
			}
		}
	}

}

}

/*
 * reads n jointlist structs from a PHYSFS_File
 */
static void jointlist_read(PHYSFS_File *fp, array<jointlist, N_ANIM_STATES> &jl)
{
	range_for (auto &i, jl)
	{
		i.n_joints = PHYSFSX_readShort(fp);
		i.offset = PHYSFSX_readShort(fp);
	}
}

namespace dsx {
/*
 * reads n robot_info structs from a PHYSFS_File
 */
void robot_info_read(PHYSFS_File *fp, robot_info &ri)
{
	ri.model_num = PHYSFSX_readInt(fp);
#if defined(DXX_BUILD_DESCENT_I)
	ri.n_guns = PHYSFSX_readInt(fp);
#endif
	range_for (auto &j, ri.gun_points)
		PHYSFSX_readVector(fp, j);
	range_for (auto &j, ri.gun_submodels)
		j = PHYSFSX_readByte(fp);

	ri.exp1_vclip_num = PHYSFSX_readShort(fp);
	ri.exp1_sound_num = PHYSFSX_readShort(fp);

	ri.exp2_vclip_num = PHYSFSX_readShort(fp);
	ri.exp2_sound_num = PHYSFSX_readShort(fp);

#if defined(DXX_BUILD_DESCENT_I)
	ri.weapon_type = static_cast<weapon_id_type>(PHYSFSX_readShort(fp));
#elif defined(DXX_BUILD_DESCENT_II)
	ri.weapon_type = static_cast<weapon_id_type>(PHYSFSX_readByte(fp));
	ri.weapon_type2 = static_cast<weapon_id_type>(PHYSFSX_readByte(fp));
	ri.n_guns = PHYSFSX_readByte(fp);
#endif
	ri.contains_id = PHYSFSX_readByte(fp);

	ri.contains_count = PHYSFSX_readByte(fp);
	ri.contains_prob = PHYSFSX_readByte(fp);
	ri.contains_type = PHYSFSX_readByte(fp);
#if defined(DXX_BUILD_DESCENT_I)
	ri.score_value = PHYSFSX_readInt(fp);
#elif defined(DXX_BUILD_DESCENT_II)
	ri.kamikaze = PHYSFSX_readByte(fp);

	ri.score_value = PHYSFSX_readShort(fp);
	ri.badass = PHYSFSX_readByte(fp);
	ri.energy_drain = PHYSFSX_readByte(fp);
#endif

	ri.lighting = PHYSFSX_readFix(fp);
	ri.strength = PHYSFSX_readFix(fp);

	ri.mass = PHYSFSX_readFix(fp);
	ri.drag = PHYSFSX_readFix(fp);

	range_for (auto &j, ri.field_of_view)
		j = PHYSFSX_readFix(fp);
	range_for (auto &j, ri.firing_wait)
		j = PHYSFSX_readFix(fp);
#if defined(DXX_BUILD_DESCENT_II)
	range_for (auto &j, ri.firing_wait2)
		j = PHYSFSX_readFix(fp);
#endif
	range_for (auto &j, ri.turn_time)
		j = PHYSFSX_readFix(fp);
#if defined(DXX_BUILD_DESCENT_I)
	for (unsigned j = 0; j < NDL * 2; j++)
			PHYSFSX_readFix(fp);
#endif
	range_for (auto &j, ri.max_speed)
		j = PHYSFSX_readFix(fp);
	range_for (auto &j, ri.circle_distance)
		j = PHYSFSX_readFix(fp);
	range_for (auto &j, ri.rapidfire_count)
		j = PHYSFSX_readByte(fp);
	range_for (auto &j, ri.evade_speed)
		j = PHYSFSX_readByte(fp);

	ri.cloak_type = PHYSFSX_readByte(fp);
	ri.attack_type = PHYSFSX_readByte(fp);
#if defined(DXX_BUILD_DESCENT_I)
	ri.boss_flag = PHYSFSX_readByte(fp);
#endif

	ri.see_sound = PHYSFSX_readByte(fp);
	ri.attack_sound = PHYSFSX_readByte(fp);
	ri.claw_sound = PHYSFSX_readByte(fp);
#if defined(DXX_BUILD_DESCENT_II)
	ri.taunt_sound = PHYSFSX_readByte(fp);

	ri.boss_flag = PHYSFSX_readByte(fp);
	ri.companion = PHYSFSX_readByte(fp);
	ri.smart_blobs = PHYSFSX_readByte(fp);
	ri.energy_blobs = PHYSFSX_readByte(fp);

	ri.thief = PHYSFSX_readByte(fp);
	ri.pursuit = PHYSFSX_readByte(fp);
	ri.lightcast = PHYSFSX_readByte(fp);
	ri.death_roll = PHYSFSX_readByte(fp);

	ri.flags = PHYSFSX_readByte(fp);
	array<char, 3> pad;
	PHYSFS_read(fp, pad, pad.size(), 1);

	ri.deathroll_sound = PHYSFSX_readByte(fp);
	ri.glow = PHYSFSX_readByte(fp);
	ri.behavior = static_cast<ai_behavior>(PHYSFSX_readByte(fp));
	ri.aim = PHYSFSX_readByte(fp);
#endif

	range_for (auto &j, ri.anim_states)
		jointlist_read(fp, j);

	ri.always_0xabcd = PHYSFSX_readInt(fp);
}
}

/*
 * reads n jointpos structs from a PHYSFS_File
 */
void jointpos_read(PHYSFS_File *fp, jointpos &jp)
{
	jp.jointnum = PHYSFSX_readShort(fp);
	PHYSFSX_readAngleVec(&jp.angles, fp);
}

#if 0
void jointpos_write(PHYSFS_File *fp, const jointpos &jp)
{
	PHYSFS_writeSLE16(fp, jp.jointnum);
	PHYSFSX_writeAngleVec(fp, jp.angles);
}
#endif
