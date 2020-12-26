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
	return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct WALL_IS_DOORWAY_mask_t
{
	uint8_t value;
	constexpr WALL_IS_DOORWAY_mask_t(WALL_IS_DOORWAY_FLAG F) :
		value(static_cast<uint8_t>(F))
	{
	}
};

struct WALL_IS_DOORWAY_result_t
{
	uint8_t value;
	constexpr WALL_IS_DOORWAY_result_t(const WALL_IS_DOORWAY_sresult_t f) :
		value(static_cast<unsigned>(f))
	{
	}
	unsigned operator&(WALL_IS_DOORWAY_FLAG f) const
	{
		return value & static_cast<uint8_t>(f);
	}
	WALL_IS_DOORWAY_result_t &operator|=(WALL_IS_DOORWAY_FLAG f)
	{
		value |= static_cast<uint8_t>(f);
		return *this;
	}
	bool operator==(WALL_IS_DOORWAY_sresult_t F) const
	{
		return value == static_cast<unsigned>(F);
	}
	bool operator&(WALL_IS_DOORWAY_mask_t m) const
	{
		return value & m.value;
	}
	bool operator==(WALL_IS_DOORWAY_result_t) const = delete;
	template <typename T>
		bool operator!=(const T &t) const
		{
			return !(*this == t);
		}
};

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

#ifdef dsx
class d_level_unique_stuck_object_state
{
protected:
	unsigned Num_stuck_objects = 0;
	std::array<stuckobj, 32> Stuck_objects;
public:
	void init_stuck_objects();
};
#endif

}

//End old wall structures

#ifdef dsx
namespace dsx {

/* No shared state is possible for this structure, but include the
 * `unique` qualifier to document its status.
 */
class d_level_unique_stuck_object_state : public ::dcx::d_level_unique_stuck_object_state
{
public:
	void add_stuck_object(fvcwallptr &, vmobjptridx_t objp, const shared_segment &segp, unsigned sidenum);
	void remove_stuck_object(vcobjidx_t);
	void kill_stuck_objects(fvmobjptr &, vcwallidx_t wallnum);
};

extern d_level_unique_stuck_object_state LevelUniqueStuckObjectState;

struct wall : public prohibit_void_ptr<wall>
{
	segnum_t segnum;
	uint8_t  sidenum;     // Seg & side for this wall
	uint8_t type;               // What kind of special wall.
	fix     hps;                // "Hit points" of the wall.
	uint16_t explode_time_elapsed;
	wallnum_t linked_wall;        // number of linked wall
	ubyte   flags;              // Flags for the wall.
	ubyte   state;              // Opening, closing, etc.
	uint8_t   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	wall_key keys;               // which keys are required
#if defined(DXX_BUILD_DESCENT_II)
	sbyte   controlling_trigger;// which trigger causes something to happen here.  Not like "trigger" above, which is the trigger on this wall.
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
#if defined(DXX_BUILD_DESCENT_II)
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
#if defined(DXX_BUILD_DESCENT_II)
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
	short   open_sound;
	short   close_sound;
	short   flags;
	std::array<char, 13> filename;
};

constexpr std::integral_constant<uint16_t, 0xffff> wclip_frames_none{};

}
#endif
