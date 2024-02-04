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
 * Definitions for fueling centers.
 *
 */

#pragma once

#ifdef __cplusplus
#include "pack.h"
#include "fwd-object.h"
#include "fwd-robot.h"
#include "fwd-segment.h"
#include "fwd-vecmat.h"
#include "d_array.h"
#include "physfsx.h"

//------------------------------------------------------------
// A refueling center is one segment... to identify it in the
// segment structure, the "special" field is set to
// segment_special::fuelcen.  The "value" field is then used for how
// much fuel the center has left, with a maximum value of 100.

//-------------------------------------------------------------
// To hook into Inferno:
// * When all segents are deleted or before a new mine is created
//   or loaded, call fuelcen_reset().
// * Add call to fuelcen_create(segment * segp) to make a segment
//   which isn't a fuel center be a fuel center.
// * When a mine is loaded call fuelcen_activate(segp) with each
//   new segment as it loads. Always do this.
// * When a segment is deleted, always call fuelcen_delete(segp).
// * When an object that needs to be refueled is in a segment, call
//   fuelcen_give_fuel(segp) to get fuel. (Call once for any refueling
//   object once per frame with the object's current segment.) This
//   will return a value between 0 and 100 that tells how much fuel
//   he got.


#ifdef dsx
namespace dsx {
// Destroys all fuel centers, clears segment backpointer array.
void fuelcen_reset();

// Makes a segment a fuel center.
void fuelcen_create( vmsegptridx_t segp);
// Makes a fuel center active... needs to be called when
// a segment is loaded from disk.
void fuelcen_activate(vmsegptridx_t segp);
// Deletes a segment as a fuel center.
void fuelcen_delete(shared_segment &segp);

// Returns the amount of fuel/shields this segment can give up.
// Can be from 0 to 100.
fix fuelcen_give_fuel(const shared_segment &segp, fix MaxAmountCanTake);
}

// Call once per frame.
void fuelcen_update_all(const d_robot_info_array &Robot_info);
#endif

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(object *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(object *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

namespace dcx {

// An array of pointers to segments with fuel centers.
struct FuelCenter : public prohibit_void_ptr<FuelCenter>
{
	segnum_t     segnum;
	segment_special     Type;
	sbyte   Flag;
	sbyte   Enabled;
	sbyte   Lives;          // Number of times this can be enabled.
	fix     Capacity;
	fix     Timer;          // used in matcen for when next robot comes out
	fix     Disable_time;   // Time until center disabled.
};

// The max number of robot centers per mine.
struct d1_matcen_info : public prohibit_void_ptr<d1_matcen_info>
{
	std::array<unsigned, 1>     robot_flags;    // Up to 32 different robots
	segnum_t   segnum;         // Segment this is attached to.
	station_number fuelcen_num;    // Index in fuelcen array.
};

struct d_level_unique_fuelcenter_state
{
	unsigned Num_fuelcenters;
	// Original D1 size: 50, Original D2 size: 70
	enumerated_array<FuelCenter, 128, station_number> Station;
};

extern d_level_unique_fuelcenter_state LevelUniqueFuelcenterState;

#ifdef dsx
fix repaircen_give_shields(const shared_segment &segp, fix MaxAmountCanTake);
#endif
}

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
typedef d1_matcen_info matcen_info;
void matcen_info_read(NamedPHYSFS_File fp, matcen_info &ps, int version);
#elif defined(DXX_BUILD_DESCENT_II)
struct matcen_info : public prohibit_void_ptr<matcen_info>
{
	std::array<unsigned, 2>     robot_flags; // Up to 64 different robots
	segnum_t   segnum;         // Segment this is attached to.
	station_number fuelcen_num;    // Index in fuelcen array.
};

void matcen_info_read(NamedPHYSFS_File fp, matcen_info &ps);
#endif

#if DXX_USE_EDITOR
extern const enumerated_array<char[11], MAX_CENTER_TYPES, segment_special> Special_names;
#endif

struct d_level_shared_robotcenter_state
{
	unsigned Num_robot_centers;
	// Original D1/D2 size: 20
	enumerated_array<matcen_info, 128, materialization_center_number> RobotCenters;
};

extern d_level_shared_robotcenter_state LevelSharedRobotcenterState;

// Called when a materialization center gets triggered by the player
// flying through some trigger!
void trigger_matcen(vmsegptridx_t segnum);

extern void disable_matcens(void);

extern void init_all_matcens(void);

/*
 * reads a matcen_info structure from a PHYSFS_File
 */
#if defined(DXX_BUILD_DESCENT_II)
void fuelcen_check_for_hoard_goal(object &plrobj, const shared_segment &segp);

/*
 * reads an d1_matcen_info structure from a PHYSFS_File
 */
void d1_matcen_info_read(NamedPHYSFS_File fp, matcen_info &mi);
#endif

void matcen_info_write(PHYSFS_File *fp, const matcen_info &mi, short version);
}

namespace dcx {
extern const fix EnergyToCreateOneRobot;
}
#endif

void fuelcen_read(NamedPHYSFS_File fp, FuelCenter &fc);
void fuelcen_write(PHYSFS_File *fp, const FuelCenter &fc);

#endif
