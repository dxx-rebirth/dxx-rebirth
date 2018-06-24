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
#include "fwd-segment.h"
#include "fwd-vecmat.h"

//------------------------------------------------------------
// A refueling center is one segment... to identify it in the
// segment structure, the "special" field is set to
// SEGMENT_IS_FUELCEN.  The "value" field is then used for how
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
void fuelcen_delete(vmsegptr_t segp);

// Create a matcen robot
imobjptridx_t create_morph_robot(vmsegptridx_t segp, const vms_vector &object_pos, int object_id);

// Returns the amount of fuel/shields this segment can give up.
// Can be from 0 to 100.
fix fuelcen_give_fuel(const shared_segment &segp, fix MaxAmountCanTake);
}

// Call once per frame.
void fuelcen_update_all();

// Called to repair an object
//--repair-- int refuel_do_repair_effect( object * obj, int first_time, int repair_seg );

namespace dsx {
#define MAX_NUM_FUELCENS 128 // Original D1: 50, Original D2: 70
#if defined(DXX_BUILD_DESCENT_II)
fix repaircen_give_shields(vcsegptr_t segp, fix MaxAmountCanTake);
#endif
}
#endif

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(object *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(object *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

// An array of pointers to segments with fuel centers.
struct FuelCenter : public prohibit_void_ptr<FuelCenter>
{
	segnum_t     segnum;
	uint8_t     Type;
	sbyte   Flag;
	sbyte   Enabled;
	sbyte   Lives;          // Number of times this can be enabled.
	fix     Capacity;
	fix     Timer;          // used in matcen for when next robot comes out
	fix     Disable_time;   // Time until center disabled.
};

// The max number of robot centers per mine.
#define MAX_ROBOT_CENTERS  128 // Original D1/D2: 20

struct d1_matcen_info : public prohibit_void_ptr<d1_matcen_info>
{
	array<unsigned, 1>     robot_flags;    // Up to 32 different robots
	segnum_t   segnum;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
};

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
typedef d1_matcen_info matcen_info;
void matcen_info_read(PHYSFS_File *fp, matcen_info &ps, int version);
#elif defined(DXX_BUILD_DESCENT_II)
struct matcen_info : public prohibit_void_ptr<matcen_info>
{
	array<unsigned, 2>     robot_flags; // Up to 64 different robots
	segnum_t   segnum;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
};

void matcen_info_read(PHYSFS_File *fp, matcen_info &ps);
#endif

extern const char Special_names[MAX_CENTER_TYPES][11];

extern array<matcen_info, MAX_ROBOT_CENTERS> RobotCenters;
extern array<FuelCenter, MAX_NUM_FUELCENS> Station;

// Called when a materialization center gets triggered by the player
// flying through some trigger!
void trigger_matcen(vmsegptridx_t segnum);

extern void disable_matcens(void);

extern void init_all_matcens(void);

/*
 * reads a matcen_info structure from a PHYSFS_File
 */
#if defined(DXX_BUILD_DESCENT_II)
void fuelcen_check_for_hoard_goal(vcsegptr_t segp);

/*
 * reads an d1_matcen_info structure from a PHYSFS_File
 */
void d1_matcen_info_read(PHYSFS_File *fp, matcen_info &mi);
#endif

void matcen_info_write(PHYSFS_File *fp, const matcen_info &mi, short version);
}

namespace dcx {
constexpr std::integral_constant<uint8_t, 0xff> station_none{};
extern unsigned Num_robot_centers;
extern unsigned Num_fuelcenters;
extern const fix EnergyToCreateOneRobot;
}
#endif

void fuelcen_read(PHYSFS_File *fp, FuelCenter &fc);
void fuelcen_write(PHYSFS_File *fp, const FuelCenter &fc);

#endif
