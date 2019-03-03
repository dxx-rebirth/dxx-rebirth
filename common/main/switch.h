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

#include <type_traits>
#include <physfs.h>
#include "maths.h"

#ifdef __cplusplus
#include "pack.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-valptridx.h"
#include "valptridx.h"
#include "compiler-array.h"
#include "dsx-ns.h"
#include "fwd-player.h"
#include "fwd-window.h"

namespace dcx {
constexpr std::integral_constant<std::size_t, 100> MAX_TRIGGERS{};
constexpr std::integral_constant<unsigned, 10> MAX_WALLS_PER_LINK{};
}

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

enum TRIGGER_FLAG : uint16_t
{
//flags for V30 & below triggers
	CONTROL_DOORS =      1,    // Control Trigger
	SHIELD_DAMAGE =      2,    // Shield Damage Trigger
	ENERGY_DRAIN =       4,    // Energy Drain Trigger
	EXIT =               8,    // End of level Trigger
	ON =                16,    // Whether Trigger is active
	ONE_SHOT =          32,    // If Trigger can only be triggered once
	MATCEN =            64,    // Trigger for materialization centers
	ILLUSION_OFF =     128,    // Switch Illusion OFF trigger
	SECRET_EXIT =      256,    // Exit to secret level
	ILLUSION_ON =      512,    // Switch Illusion ON trigger
#if defined(DXX_BUILD_DESCENT_II)
	UNLOCK_DOORS =    1024,    // Unlocks a door
	OPEN_WALL =       2048,    // Makes a wall open
	CLOSE_WALL =      4096,    // Makes a wall closed
	ILLUSORY_WALL =   8192,    // Makes a wall illusory
#endif
};

#define TRIGGER_CONTROL_DOORS TRIGGER_FLAG::CONTROL_DOORS
#define TRIGGER_SHIELD_DAMAGE TRIGGER_FLAG::SHIELD_DAMAGE
#define TRIGGER_ENERGY_DRAIN TRIGGER_FLAG::ENERGY_DRAIN
#define TRIGGER_EXIT TRIGGER_FLAG::EXIT
#define TRIGGER_ON TRIGGER_FLAG::ON
#define TRIGGER_ONE_SHOT TRIGGER_FLAG::ONE_SHOT
#define TRIGGER_MATCEN TRIGGER_FLAG::MATCEN
#define TRIGGER_ILLUSION_OFF TRIGGER_FLAG::ILLUSION_OFF
#define TRIGGER_SECRET_EXIT TRIGGER_FLAG::SECRET_EXIT
#define TRIGGER_ILLUSION_ON TRIGGER_FLAG::ILLUSION_ON
#if defined(DXX_BUILD_DESCENT_II)
#define TRIGGER_UNLOCK_DOORS TRIGGER_FLAG::UNLOCK_DOORS
#define TRIGGER_OPEN_WALL TRIGGER_FLAG::OPEN_WALL
#define TRIGGER_CLOSE_WALL TRIGGER_FLAG::CLOSE_WALL
#define TRIGGER_ILLUSORY_WALL TRIGGER_FLAG::ILLUSORY_WALL
#endif

//the trigger really should have both a type & a flags, since most of the
//flags bits are exclusive of the others.
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
typedef uint8_t trgnum_t;

namespace dsx {

struct trigger : public prohibit_void_ptr<trigger>
{
#if defined(DXX_BUILD_DESCENT_I)
	uint16_t flags;
	uint16_t num_links;
#elif defined(DXX_BUILD_DESCENT_II)
	ubyte   type;       //what this trigger does
	ubyte   flags;      //currently unused
	uint8_t   num_links;  //how many doors, etc. linked to this
#endif
	fix     value;
	array<segnum_t, MAX_WALLS_PER_LINK>   seg;
	array<short, MAX_WALLS_PER_LINK>   side;
};

}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, trigger, trgnum_t, MAX_TRIGGERS);
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(trigger, trg);

struct d_level_unique_trigger_state
{
	valptridx<trigger>::array_managed_type Triggers;
};
}

constexpr std::integral_constant<uint8_t, 0xff> trigger_none{};

extern void trigger_init();
namespace dsx {
window_event_result check_trigger(vcsegptridx_t seg, unsigned side, object &plrobj, vcobjptridx_t objnum, int shot);
window_event_result check_trigger_sub(object &, trgnum_t trigger_num, playernum_t player_num, unsigned shot);

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
void v25_trigger_read(PHYSFS_File *fp, trigger *);
void v26_trigger_read(PHYSFS_File *fp, trigger &);
#endif

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a v29_trigger structure from a PHYSFS_File
 */
extern void v29_trigger_read(v29_trigger *t, PHYSFS_File *fp);

/*
 * reads a v30_trigger structure from a PHYSFS_File
 */
extern void v30_trigger_read(v30_trigger *t, PHYSFS_File *fp);
#endif

/*
 * reads a trigger structure from a PHYSFS_File
 */
extern void trigger_read(trigger *t, PHYSFS_File *fp);
void v29_trigger_read_as_v31(PHYSFS_File *fp, trigger &t);
void v30_trigger_read_as_v31(PHYSFS_File *fp, trigger &t);
}

/*
 * reads n trigger structs from a PHYSFS_File and swaps if specified
 */
void trigger_read(PHYSFS_File *fp, trigger &t);
void trigger_write(PHYSFS_File *fp, const trigger &t);

#ifdef dsx
namespace dsx {
void v29_trigger_write(PHYSFS_File *fp, const trigger &t);
void v30_trigger_write(PHYSFS_File *fp, const trigger &t);
void v31_trigger_write(PHYSFS_File *fp, const trigger &t);
}
#endif
#endif

#endif
