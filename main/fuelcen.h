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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Definitions for fueling centers.
 *
 */

#ifndef _FUELCEN_H
#define _FUELCEN_H

#include "segment.h"
#include "object.h"

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
// * Call fuelcen_replentish_all() to fill 'em all up, like when
//   a new game is started.
// * When an object that needs to be refueled is in a segment, call
//   fuelcen_give_fuel(segp) to get fuel. (Call once for any refueling
//   object once per frame with the object's current segment.) This
//   will return a value between 0 and 100 that tells how much fuel
//   he got.


// Destroys all fuel centers, clears segment backpointer array.
void fuelcen_reset();
// Create materialization center
int create_matcen( segment * segp );
// Makes a segment a fuel center.
void fuelcen_create( segment * segp);
// Makes a fuel center active... needs to be called when
// a segment is loaded from disk.
void fuelcen_activate( segment * segp, int station_type );
// Deletes a segment as a fuel center.
void fuelcen_delete( segment * segp );

// Charges all fuel centers to max capacity.
void fuelcen_replentish_all();

// Create a matcen robot
extern object *create_morph_robot(segment *segp, vms_vector *object_pos, int object_id);

// Returns the amount of fuel/shields this segment can give up.
// Can be from 0 to 100.
fix fuelcen_give_fuel(segment *segp, fix MaxAmountCanTake );
fix repaircen_give_shields(segment *segp, fix MaxAmountCanTake );

// Call once per frame.
void fuelcen_update_all();

// Called when hit by laser.
void fuelcen_damage(segment *segp, fix AmountOfDamage );

// Called to repair an object
//--repair-- int refuel_do_repair_effect( object * obj, int first_time, int repair_seg );

#define MAX_NUM_FUELCENS    70

extern char Special_names[MAX_CENTER_TYPES][11];

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(object *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(object *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

// An array of pointers to segments with fuel centers.
typedef struct FuelCenter {
	int     Type;
	int     segnum;
	sbyte   Flag;
	sbyte   Enabled;
	sbyte   Lives;          // Number of times this can be enabled.
	sbyte   dum1;
	fix     Capacity;
	fix     MaxCapacity;
	fix     Timer;          // used in matcen for when next robot comes out
	fix     Disable_time;   // Time until center disabled.
	//object  *last_created_obj;
	//int     last_created_sig;
	vms_vector Center;
} __pack__ FuelCenter;

// The max number of robot centers per mine.
#define MAX_ROBOT_CENTERS  20

extern int Num_robot_centers;

typedef struct  {
	int     robot_flags[1];    // Up to 32 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   segnum;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ d1_matcen_info;

typedef struct matcen_info {
	int     robot_flags[2]; // Up to 64 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   segnum;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ matcen_info;

extern matcen_info RobotCenters[MAX_ROBOT_CENTERS];

//--repair-- extern object *RepairObj;  // which object getting repaired, or NULL

// Called when a materialization center gets triggered by the player
// flying through some trigger!
extern void trigger_matcen(int segnum);

extern void disable_matcens(void);

extern FuelCenter Station[MAX_NUM_FUELCENS];
extern int Num_fuelcenters;

extern void init_all_matcens(void);

extern fix EnergyToCreateOneRobot;

void fuelcen_check_for_hoard_goal(segment *segp);

/*
 * reads an d1_matcen_info structure from a PHYSFS_file
 */
void d1_matcen_info_read(d1_matcen_info *mi, PHYSFS_file *fp);

/*
 * reads a matcen_info structure from a PHYSFS_file
 */
void matcen_info_read(matcen_info *ps, PHYSFS_file *fp);

/*
 * reads n matcen_info structs from a PHYSFS_file and swaps if specified
 */
void matcen_info_read_n_swap(matcen_info *mi, int n, int swap, PHYSFS_file *fp);

void matcen_info_write(matcen_info *mi, short version, PHYSFS_file *fp);

/*
 * reads n Station structs from a PHYSFS_file and swaps if specified
 */
void fuelcen_read_n_swap(FuelCenter *fc, int n, int swap, PHYSFS_file *fp);

#endif
