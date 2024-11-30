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

#pragma once

#include "d_array.h"
#include "dsx-ns.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-weapon.h"
#include "polyobj.h"
#include "vecmat.h"
#include <cstddef>
#include <physfs.h>
#include <type_traits>

namespace dcx {

enum class robot_gun_number : uint8_t;
enum class robot_id : uint8_t;

constexpr std::integral_constant<std::size_t, 8> MAX_GUNS{};      //should be multiple of 4 for ubyte array

enum class robot_animation_state : uint8_t;
#define N_ANIM_STATES   5

#define RI_CLOAKED_ALWAYS           1

struct jointpos;
struct jointlist;

constexpr std::integral_constant<unsigned, 16> ROBOT_NAME_LENGTH{};

struct d_level_shared_robot_joint_state;

}

#ifdef DXX_BUILD_DESCENT
namespace dsx {

#if DXX_BUILD_DESCENT == 2
//robot info flags
#define RIF_BIG_RADIUS  1   //pad the radius to fix robots firing through walls
#define RIF_THIEF       2   //this guy steals!
#endif

//  Robot information
struct robot_info;

#if DXX_BUILD_DESCENT == 1
// maximum number of robot types
constexpr std::integral_constant<unsigned, 30> MAX_ROBOT_TYPES{};
constexpr std::integral_constant<unsigned, 600> MAX_ROBOT_JOINTS{};
#elif DXX_BUILD_DESCENT == 2
// maximum number of robot types
constexpr std::integral_constant<unsigned, 85> MAX_ROBOT_TYPES{};
constexpr std::integral_constant<unsigned, 1600> MAX_ROBOT_JOINTS{};
#endif

using d_robot_info_array = enumerated_array<robot_info, MAX_ROBOT_TYPES, robot_id>;

//the array of robots types
struct d_level_shared_robot_info_state;
extern d_level_shared_robot_info_state LevelSharedRobotInfoState;

#if DXX_USE_EDITOR
using robot_names_array = enumerated_array<std::array<char, ROBOT_NAME_LENGTH>, MAX_ROBOT_TYPES, robot_id>;
extern robot_names_array Robot_names;
#endif

/* Robot joints can be customized by hxm files, which are per-level.
 */
struct d_level_shared_robot_joint_state;
extern d_level_shared_robot_joint_state LevelSharedRobotJointState;

//given an object and a gun number, return position in 3-space of gun
//fills in gun_point
void calc_gun_point(const robot_info &, vms_vector &gun_point, const object_base &obj, robot_gun_number gun_num);

/*
 * reads n robot_info structs from a PHYSFS_File
 */
void robot_info_read(NamedPHYSFS_File fp, robot_info &r);
void robot_set_angles(robot_info &r, polymodel &pm, enumerated_array<std::array<vms_angvec, MAX_SUBMODELS>, N_ANIM_STATES, robot_animation_state> &angs);
weapon_id_type get_robot_weapon(const robot_info &ri, const robot_gun_number gun_num);

}
#endif

/*
 * reads n jointpos structs from a PHYSFS_File
 */
void jointpos_read(NamedPHYSFS_File fp, jointpos &jp);
