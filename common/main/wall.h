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
 * Header for wall.c
 *
 */

#pragma once

#include "dsx-ns.h"
#include "fwd-segment.h"
#include "fwd-wall.h"
#include "fwd-object.h"
#include "pack.h"
#include "switch.h"
#include "d_underlying_value.h"

namespace dcx {

enum class wall_key : uint8_t
{
	none = 1,
	blue = 2,
	red = 4,
	gold = 8,
};

constexpr uint8_t operator&(const wall_key a, const wall_key b)
{
	return underlying_value(a) & underlying_value(b);
}

#ifdef DXX_BUILD_DESCENT
enum class wall_is_doorway_mask : uint8_t
{
	None = 0,
	fly = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::fly),
	rendpast = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::rendpast),
	fly_rendpast = fly | rendpast,
};

enum class wall_is_doorway_result : uint8_t
{
	//  WALL_IS_DOORWAY return values          F/R/RP
	wall                = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::render),   // 0/1/0        wall
	transparent_wall    = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::render) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::rendpast),   // 0/1/1        transparent wall
	illusory_wall       = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::fly) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::render),   // 1/1/0        illusory wall
	transillusory_wall  = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::fly) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::render) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::rendpast),   // 1/1/1        transparent illusory wall
	no_wall             = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::fly) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::rendpast),   //  1/0/1       no wall, can fly through
	external            = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::external),   // 0/0/0/1  do not see it, do not fly through it
	/* if DXX_BUILD_DESCENT == 2 */
	cloaked_wall        = static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::render) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::rendpast) | static_cast<uint8_t>(WALL_IS_DOORWAY_FLAG::cloaked),
	/* endif */
};

[[nodiscard]]
static constexpr bool operator&(const wall_is_doorway_result r, const WALL_IS_DOORWAY_FLAG f)
{
	return static_cast<uint8_t>(r) & static_cast<uint8_t>(f);
}

[[nodiscard]]
static constexpr bool operator&(const wall_is_doorway_result r, const wall_is_doorway_mask m)
{
	return static_cast<uint8_t>(r) & static_cast<uint8_t>(m);
}

static inline wall_is_doorway_result &operator|=(wall_is_doorway_result &r, const WALL_IS_DOORWAY_FLAG f)
{
	return r = static_cast<wall_is_doorway_result>(static_cast<uint8_t>(r) | static_cast<uint8_t>(f));
}

struct stuckobj : public prohibit_void_ptr<stuckobj>
{
	objnum_t objnum = object_none;
	wallnum_t wallnum = wall_none;
};
#endif

//Start old wall structures

struct v16_wall : public prohibit_void_ptr<v16_wall>
{
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	uint8_t   trigger;            // Which trigger is associated with the wall.
	fix     hps;                // "Hit points" of the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
};

struct v19_wall : public prohibit_void_ptr<v19_wall>
{
	segnum_t     segnum;
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	int	sidenum;     // Seg & side for this wall
	fix     hps;                // "Hit points" of the wall.
	uint8_t   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
	wallnum_t linked_wall;            // number of linked wall
};

#ifdef DXX_BUILD_DESCENT
class d_level_unique_stuck_object_state
{
protected:
	unsigned Num_stuck_objects{0};
	std::array<stuckobj, 32> Stuck_objects;
public:
	void init_stuck_objects();
};
#endif

enum class wall_flag : uint8_t
{
	blasted = 1,	// Blasted out wall.
	door_opened = 1u << 1,	// Open door.
	door_locked = 1u << 3,	// Door is locked.
	door_auto = 1u << 4,	// Door automatically closes after time.
	illusion_off = 1u << 5,	// Illusionary wall is shut off.
	exploding = 1u << 6,
	/* if DXX_BUILD_DESCENT == 2 */
	buddy_proof = 1u << 7,	// Buddy assumes he cannot get through this wall.
	/* endif */
};

enum class wall_flags : uint8_t;

enum class wall_state : uint8_t
{
	closed,		// Door is closed
	opening,	// Door is opening.
	waiting,	// Waiting to close
	closing,	// Door is closing
	/* if DXX_BUILD_DESCENT == 2 */
	open,		// Door is open, and staying open
	cloaking,	// Wall is going from closed -> open
	decloaking,	// Wall is going from open -> closed
	/* endif */
};

static constexpr auto &operator|=(wall_flags &wall, const wall_flag f)
{
	return wall = static_cast<wall_flags>(underlying_value(wall) | underlying_value(f));
}

static constexpr auto &operator&=(wall_flags &wall, const wall_flag f)
{
	return wall = static_cast<wall_flags>(underlying_value(wall) & underlying_value(f));
}

static constexpr auto operator~(const wall_flag f)
{
	return static_cast<wall_flag>(~underlying_value(f));
}

static constexpr auto operator&(const wall_flags wall, const wall_flag f)
{
	return underlying_value(wall) & underlying_value(f);
}

static constexpr auto operator|(const wall_flag f1, const wall_flag f2)
{
	return static_cast<wall_flag>(underlying_value(f1) | underlying_value(f2));
}

}

//End old wall structures

#ifdef DXX_BUILD_DESCENT
namespace dsx {

/* No shared state is possible for this structure, but include the
 * `unique` qualifier to document its status.
 */
class d_level_unique_stuck_object_state : public ::dcx::d_level_unique_stuck_object_state
{
public:
	void add_stuck_object(fvcwallptr &, vmobjptridx_t objp, const shared_segment &segp, sidenum_t sidenum);
	void remove_stuck_object(vcobjidx_t);
	void kill_stuck_objects(fvmobjptr &, vcwallidx_t wallnum);
};

extern d_level_unique_stuck_object_state LevelUniqueStuckObjectState;

struct wall : public prohibit_void_ptr<wall>
{
	segnum_t segnum;
	sidenum_t sidenum;     // Seg & side for this wall
	uint8_t type;               // What kind of special wall.
	fix     hps;                // "Hit points" of the wall.
	uint16_t explode_time_elapsed;
	wallnum_t linked_wall;        // number of linked wall
	wall_flags flags;              // Flags for the wall.
	wall_state state;              // Opening, closing, etc.
	trgnum_t trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	wall_key keys;               // which keys are required
#if DXX_BUILD_DESCENT == 2
	trgnum_t controlling_trigger;// which trigger causes something to happen here.  Not like "trigger" above, which is the trigger on this wall.
                                //  Note: This gets stuffed at load time in gamemine.c.  Don't try to use it in the editor.  You will be sorry!
	sbyte   cloak_value;        // if this wall is cloaked, the fade value
#endif
};

}

namespace dcx {

struct active_door : public prohibit_void_ptr<active_door>
{
	unsigned n_parts;            // for linked walls
	std::array<wallnum_t, 2>   front_wallnum;   // front wall numbers for this door
	std::array<wallnum_t, 2>   back_wallnum;    // back wall numbers for this door
	fix     time;               // how long been opening, closing, waiting
};

struct d_level_unique_active_door_state
{
	active_door_array ActiveDoors;
};

}

namespace dsx {
#if DXX_BUILD_DESCENT == 2
struct cloaking_wall : public prohibit_void_ptr<cloaking_wall>
{
	wallnum_t       front_wallnum;  // front wall numbers for this door
	wallnum_t       back_wallnum;   // back wall numbers for this door
	std::array<fix, 4> front_ls;     // front wall saved light values
	std::array<fix, 4> back_ls;      // back wall saved light values
	fix     time;               // how long been cloaking or decloaking
};

struct d_level_unique_cloaking_wall_state
{
	cloaking_wall_array CloakingWalls;
};
#endif

struct d_level_unique_wall_state
{
	wall_array Walls;
};

struct d_level_unique_wall_subsystem_state :
	d_level_unique_active_door_state,
	d_level_unique_trigger_state,
	d_level_unique_wall_state
#if DXX_BUILD_DESCENT == 2
	, d_level_unique_cloaking_wall_state
#endif
{
};

extern d_level_unique_wall_subsystem_state LevelUniqueWallSubsystemState;

struct wclip : public prohibit_void_ptr<wclip>
{
	fix     play_time;
	uint16_t num_frames;
	union {
		std::array<uint16_t, MAX_CLIP_FRAMES> frames;
		std::array<uint16_t, MAX_CLIP_FRAMES_D1> d1_frames;
	};
	sound_effect open_sound;
	sound_effect close_sound;
	short   flags;
	std::array<char, 13> filename;
};

constexpr std::integral_constant<uint16_t, 0xffff> wclip_frames_none{};

}
#endif
