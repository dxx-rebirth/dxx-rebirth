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
 * Triggers and Switches.
 *
 */

#pragma once

#include <physfs.h>
#include "maths.h"

#ifdef __cplusplus
#include "pack.h"
#include "fwdobject.h"
#include "fwdsegment.h"
#include "compiler-array.h"

#define MAX_TRIGGERS        100
#define MAX_WALLS_PER_LINK  10

#define TT_OPEN_DOOR        0   // Open a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on

#if defined(DXX_BUILD_DESCENT_II)
// Trigger types

#define TT_CLOSE_DOOR       1   // Close a door
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a wall open
#define TT_CLOSE_WALL       10  // Makes a wall closed
#define TT_ILLUSORY_WALL    11  // Makes a wall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn a light on
#define NUM_TRIGGER_TYPES   14

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only trigger once
#define TF_DISABLED         4   // Set after one-shot fires

//old trigger structs

struct v29_trigger
{
	sbyte   type;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   num_links;
	array<segnum_t, MAX_WALLS_PER_LINK>   seg;
	array<short, MAX_WALLS_PER_LINK>   side;
} __pack__;

struct v30_trigger
{
	short   flags;
	sbyte   num_links;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	array<segnum_t, MAX_WALLS_PER_LINK>   seg;
	array<short, MAX_WALLS_PER_LINK>   side;
} __pack__;
#endif

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_MATCEN            64    // Trigger for materialization centers
#define TRIGGER_ILLUSION_OFF     128    // Switch Illusion OFF trigger
#define TRIGGER_SECRET_EXIT      256    // Exit to secret level
#define TRIGGER_ILLUSION_ON      512    // Switch Illusion ON trigger
#if defined(DXX_BUILD_DESCENT_II)
#define TRIGGER_UNLOCK_DOORS    1024    // Unlocks a door
#define TRIGGER_OPEN_WALL       2048    // Makes a wall open
#define TRIGGER_CLOSE_WALL      4096    // Makes a wall closed
#define TRIGGER_ILLUSORY_WALL   8192    // Makes a wall illusory
#endif

//the trigger really should have both a type & a flags, since most of the
//flags bits are exclusive of the others.
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct trigger : public prohibit_void_ptr<trigger>
{
#if defined(DXX_BUILD_DESCENT_I)
	short		flags;
#elif defined(DXX_BUILD_DESCENT_II)
	ubyte   type;       //what this trigger does
	ubyte   flags;      //currently unused
	uint8_t   num_links;  //how many doors, etc. linked to this
#endif
	fix     value;
	fix     time;
#if defined(DXX_BUILD_DESCENT_I)
	int8_t		link_num;
	uint16_t 	num_links;
#endif
	array<segnum_t, MAX_WALLS_PER_LINK>   seg;
	array<short, MAX_WALLS_PER_LINK>   side;
};

const uint8_t trigger_none = -1;

extern unsigned Num_triggers;
extern array<trigger, MAX_TRIGGERS> Triggers;

extern void trigger_init();
void check_trigger(vcsegptridx_t seg, short side, vcobjptridx_t objnum, int shot);
extern int check_trigger_sub(int trigger_num, int player_num,int shot);
extern void triggers_frame_process();

static inline int trigger_is_exit(const trigger *t)
{
#if defined(DXX_BUILD_DESCENT_I)
	return t->flags & TRIGGER_EXIT;
#elif defined(DXX_BUILD_DESCENT_II)
	return t->type == TT_EXIT;
#endif
}

static inline int trigger_is_matcen(const trigger &t)
{
#if defined(DXX_BUILD_DESCENT_I)
	return t.flags & TRIGGER_MATCEN;
#elif defined(DXX_BUILD_DESCENT_II)
	return t.type == TT_MATCEN;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
void v25_trigger_read(PHYSFS_file *fp, trigger *);
void v26_trigger_read(PHYSFS_file *fp, trigger &);
#endif

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a v29_trigger structure from a PHYSFS_file
 */
extern void v29_trigger_read(v29_trigger *t, PHYSFS_file *fp);

/*
 * reads a v30_trigger structure from a PHYSFS_file
 */
extern void v30_trigger_read(v30_trigger *t, PHYSFS_file *fp);
#endif

/*
 * reads a trigger structure from a PHYSFS_file
 */
extern void trigger_read(trigger *t, PHYSFS_file *fp);
void v29_trigger_read_as_v31(PHYSFS_File *fp, trigger &t);
void v30_trigger_read_as_v31(PHYSFS_File *fp, trigger &t);

/*
 * reads n trigger structs from a PHYSFS_file and swaps if specified
 */
void trigger_read(PHYSFS_file *fp, trigger &t);
void trigger_write(PHYSFS_file *fp, const trigger &t);

void v29_trigger_write(PHYSFS_file *fp, const trigger &t);
void v30_trigger_write(PHYSFS_file *fp, const trigger &t);
void v31_trigger_write(PHYSFS_file *fp, const trigger &t);
#endif

#endif
