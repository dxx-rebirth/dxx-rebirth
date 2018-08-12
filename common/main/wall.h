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
#include "segment.h"

#ifdef __cplusplus
#include "fwd-wall.h"
#include "pack.h"

namespace dcx {

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct WALL_IS_DOORWAY_mask_t
{
	unsigned value;
	template <unsigned F>
		constexpr WALL_IS_DOORWAY_mask_t(WALL_IS_DOORWAY_FLAG<F>) :
			value(F)
	{
	}
};

struct WALL_IS_DOORWAY_result_t
{
	unsigned value;
	template <unsigned F>
		constexpr WALL_IS_DOORWAY_result_t(WALL_IS_DOORWAY_sresult_t<F>) :
			value(F)
	{
	}
	template <unsigned F>
		unsigned operator&(WALL_IS_DOORWAY_FLAG<F>) const
		{
			return value & F;
		}
	template <unsigned F>
		WALL_IS_DOORWAY_result_t operator|=(WALL_IS_DOORWAY_FLAG<F>)
		{
			value |= F;
			return *this;
		}
	template <unsigned F>
	bool operator==(WALL_IS_DOORWAY_sresult_t<F>) const
	{
		return value == F;
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
#endif

struct stuckobj : public prohibit_void_ptr<stuckobj>
{
	objnum_t objnum = object_none;
	wallnum_t wallnum = wall_none;
};

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
	int linked_wall;            // number of linked wall
};

class d_level_unique_stuck_object_state
{
protected:
	unsigned Num_stuck_objects = 0;
	array<stuckobj, 32> Stuck_objects;
public:
	void init_stuck_objects();
};

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
	void add_stuck_object(fvcwallptr &, vmobjptridx_t objp, const vcsegptr_t segp, unsigned sidenum);
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
	ubyte   keys;               // which keys are required
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
	array<wallnum_t, 2>   front_wallnum;   // front wall numbers for this door
	array<wallnum_t, 2>   back_wallnum;    // back wall numbers for this door
	fix     time;               // how long been opening, closing, waiting
};

DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(active_door, actdoor, ActiveDoors);

}

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
struct cloaking_wall : public prohibit_void_ptr<cloaking_wall>
{
	wallnum_t       front_wallnum;  // front wall numbers for this door
	wallnum_t       back_wallnum;   // back wall numbers for this door
	array<fix, 4> front_ls;     // front wall saved light values
	array<fix, 4> back_ls;      // back wall saved light values
	fix     time;               // how long been cloaking or decloaking
};
DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(cloaking_wall, clwall, CloakingWalls);
#endif

DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(wall, wall, Walls);

struct wclip : public prohibit_void_ptr<wclip>
{
	fix     play_time;
	uint16_t num_frames;
	union {
		array<int16_t, MAX_CLIP_FRAMES> frames;
		array<int16_t, MAX_CLIP_FRAMES_D1> d1_frames;
	};
	short   open_sound;
	short   close_sound;
	short   flags;
	array<char, 13> filename;
};

constexpr std::integral_constant<uint16_t, 0xffff> wclip_frames_none{};

static inline WALL_IS_DOORWAY_result_t WALL_IS_DOORWAY(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, fvcwallptr &vcwallptr, const shared_segment &sseg, const unique_segment &useg, const uint_fast32_t side)
{
	/* For now, useg is useless.  It will become useful when
	 * shared_segment::sides splits into shared_side and unique_side.
	 */
	(void)useg;
	const auto child = sseg.children[side];
	if (unlikely(child == segment_none))
		return WID_WALL;
	if (unlikely(child == segment_exit))
		return WID_EXTERNAL;
	auto &sside = sseg.sides[side];
	if (likely(sside.wall_num == wall_none))
		return WID_NO_WALL;
	return wall_is_doorway(GameBitmaps, Textures, vcwallptr, sside, sside);
}
}
#endif
#endif
