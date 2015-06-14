/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <physfs.h>
#include "fwdvalptridx.h"

struct side;

#if defined(DXX_BUILD_DESCENT_I)
const unsigned MAX_WALLS = 175;	// Maximum number of walls
const std::size_t MAX_WALL_ANIMS = 30;		// Maximum different types of doors
const std::size_t MAX_DOORS = 50;		// Maximum number of open doors
#elif defined(DXX_BUILD_DESCENT_II)
const unsigned MAX_WALLS = 254; // Maximum number of walls
const std::size_t MAX_WALL_ANIMS = 60;  // Maximum different types of doors
const std::size_t MAX_DOORS = 90;  // Maximum number of open doors
#endif

enum wall_type_t : uint8_t
{
	WALL_NORMAL = 0,   // Normal wall
	WALL_BLASTABLE = 1,   // Removable (by shooting) wall
	WALL_DOOR = 2,   // Door
	WALL_ILLUSION = 3,   // Wall that appears to be there, but you can fly thru
	WALL_OPEN = 4,   // Just an open side. (Trigger)
	WALL_CLOSED = 5,   // Wall.  Used for transparent walls.
#if defined(DXX_BUILD_DESCENT_II)
	WALL_OVERLAY = 6,   // Goes over an actual solid side.  For triggers
	WALL_CLOAKED = 7,   // Can see it, and see through it
#endif
};

typedef unsigned wall_flag_t;
const wall_flag_t WALL_BLASTED = 1;   // Blasted out wall.
const wall_flag_t WALL_DOOR_OPENED = 2;   // Open door.
const wall_flag_t WALL_DOOR_LOCKED = 8;   // Door is locked.
const wall_flag_t WALL_DOOR_AUTO = 16;  // Door automatically closes after time.
const wall_flag_t WALL_ILLUSION_OFF = 32;  // Illusionary wall is shut off.
#if defined(DXX_BUILD_DESCENT_II)
const wall_flag_t WALL_WALL_SWITCH = 64;  // This wall is openable by a wall switch.
const wall_flag_t WALL_BUDDY_PROOF = 128; // Buddy assumes he cannot get through this wall.
#endif

typedef unsigned wall_state_t;
const wall_state_t WALL_DOOR_CLOSED = 0;       // Door is closed
const wall_state_t WALL_DOOR_OPENING = 1;       // Door is opening.
const wall_state_t WALL_DOOR_WAITING = 2;       // Waiting to close
const wall_state_t WALL_DOOR_CLOSING = 3;       // Door is closing
#if defined(DXX_BUILD_DESCENT_II)
const wall_state_t WALL_DOOR_OPEN = 4;       // Door is open, and staying open
const wall_state_t WALL_DOOR_CLOAKING = 5;       // Wall is going from closed -> open
const wall_state_t WALL_DOOR_DECLOAKING = 6;       // Wall is going from open -> closed
#endif

enum wall_key_t : uint8_t
{
	KEY_NONE = 1,
	KEY_BLUE = 2,
	KEY_RED = 4,
	KEY_GOLD = 8,
};

const fix WALL_HPS = 100*F1_0;    // Normal wall's hp
const fix WALL_DOOR_INTERVAL = 5*F1_0;      // How many seconds a door is open

const fix DOOR_OPEN_TIME = i2f(2);      // How long takes to open
const fix DOOR_WAIT_TIME = i2f(5);      // How long before auto door closes

#if defined(DXX_BUILD_DESCENT_I)
const std::size_t MAX_CLIP_FRAMES = 20;
#elif defined(DXX_BUILD_DESCENT_II)
const std::size_t MAX_CLIP_FRAMES = 50;
#endif

template <unsigned value>
struct WALL_IS_DOORWAY_FLAG
{
	constexpr operator unsigned() const { return value; }
	template <unsigned F2>
		constexpr WALL_IS_DOORWAY_FLAG<value | F2> operator|(WALL_IS_DOORWAY_FLAG<F2>) const
		{
			return {};
		}
	void *operator &() const = delete;
};

template <unsigned F>
struct WALL_IS_DOORWAY_sresult_t
{
};

template <unsigned F>
static inline constexpr WALL_IS_DOORWAY_sresult_t<F> WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG<F>)
{
	return {};
}

struct WALL_IS_DOORWAY_result_t;

// WALL_IS_DOORWAY flags.
const WALL_IS_DOORWAY_FLAG<1> WID_FLY_FLAG{};
const WALL_IS_DOORWAY_FLAG<2> WID_RENDER_FLAG{};
const WALL_IS_DOORWAY_FLAG<4> WID_RENDPAST_FLAG{};
const WALL_IS_DOORWAY_FLAG<8> WID_EXTERNAL_FLAG{};
#if defined(DXX_BUILD_DESCENT_II)
const WALL_IS_DOORWAY_FLAG<16> WID_CLOAKED_FLAG{};
#endif

//  WALL_IS_DOORWAY return values          F/R/RP
const auto WID_WALL                = WALL_IS_DOORWAY_sresult(WID_RENDER_FLAG);   // 0/1/0        wall
const auto WID_TRANSPARENT_WALL    = WALL_IS_DOORWAY_sresult(WID_RENDER_FLAG | WID_RENDPAST_FLAG);   // 0/1/1        transparent wall
const auto WID_ILLUSORY_WALL       = WALL_IS_DOORWAY_sresult(WID_FLY_FLAG | WID_RENDER_FLAG);   // 1/1/0        illusory wall
const auto WID_TRANSILLUSORY_WALL  = WALL_IS_DOORWAY_sresult(WID_FLY_FLAG | WID_RENDER_FLAG | WID_RENDPAST_FLAG);   // 1/1/1        transparent illusory wall
const auto WID_NO_WALL             = WALL_IS_DOORWAY_sresult(WID_FLY_FLAG | WID_RENDPAST_FLAG);   //  1/0/1       no wall, can fly through
const auto WID_EXTERNAL            = WALL_IS_DOORWAY_sresult(WID_EXTERNAL_FLAG);   // 0/0/0/1  don't see it, dont fly through it
#if defined(DXX_BUILD_DESCENT_II)
const auto WID_CLOAKED_WALL        = WALL_IS_DOORWAY_sresult(WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_CLOAKED_FLAG);
#endif

template <int16_t I>
struct wall_magic_constant_t
{
	constexpr operator int16_t() const { return I; }
};

const wall_magic_constant_t<-1> wall_none{};

const std::size_t MAX_STUCK_OBJECTS = 32;

struct stuckobj;
struct v16_wall;
struct v19_wall;
struct wall;
struct active_door;

#if defined(DXX_BUILD_DESCENT_II)
struct cloaking_wall;
#endif

typedef unsigned wall_clip_flag_t;
const wall_clip_flag_t WCF_EXPLODES = 1;       //door explodes when opening
const wall_clip_flag_t WCF_BLASTABLE = 2;       //this is a blastable wall
const wall_clip_flag_t WCF_TMAP1 = 4;       //this uses primary tmap, not tmap2
const wall_clip_flag_t WCF_HIDDEN = 8;       //this uses primary tmap, not tmap2

struct wclip;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
const std::size_t MAX_CLIP_FRAMES_D1 = 20;

#if defined(DXX_BUILD_DESCENT_II)
const std::size_t MAX_CLOAKING_WALLS = 10;
extern array<cloaking_wall, MAX_CLOAKING_WALLS> CloakingWalls;
extern unsigned Num_cloaking_walls;
#endif

extern array<wall, MAX_WALLS> Walls;           // Master walls array
extern unsigned Num_walls;                   // Number of walls

extern array<active_door, MAX_DOORS> ActiveDoors;  //  Master doors array
extern unsigned Num_open_doors;              // Number of open doors

extern unsigned Num_wall_anims;
extern array<wclip, MAX_WALL_ANIMS> WallAnims;
#endif

#ifdef EDITOR
void wall_init();
#endif

// Automatically checks if a there is a doorway (i.e. can fly through)
WALL_IS_DOORWAY_result_t wall_is_doorway (const side &side);

// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void wall_damage(vsegptridx_t seg, int side, fix damage);

// Destroys a blastable wall. (So it is an opening afterwards)
void wall_destroy(vsegptridx_t seg, int side);

void wall_illusion_on(vsegptridx_t seg, int side);
void wall_illusion_off(vsegptridx_t seg, int side);

// Opens a door, including animation and other processing.
void do_door_open(int door_num);

// Closes a door, including animation and other processing.
void do_door_close(int door_num);

// Opens a door
void wall_open_door(vsegptridx_t seg, int side);

#if defined(DXX_BUILD_DESCENT_I)
// Closes a door (called after given interval)
void wall_close_door(int wall_num);
#elif defined(DXX_BUILD_DESCENT_II)
// Closes a door
void wall_close_door(vsegptridx_t seg, int side);
#endif

//return codes for wall_hit_process()
enum wall_hit_process_t
{
	WHP_NOT_SPECIAL = 0,       //wasn't a quote-wall-unquote
	WHP_NO_KEY = 1,       //hit door, but didn't have key
	WHP_BLASTABLE = 2,       //hit blastable wall
	WHP_DOOR = 3,       //a door (which will now be opening)
};

// Determines what happens when a wall is shot
//obj is the object that hit...either a weapon or the player himself
wall_hit_process_t wall_hit_process(vsegptridx_t seg, int side, fix damage, int playernum, vobjptr_t obj);

// Opens/destroys specified door.
void wall_toggle(vsegptridx_t segnum, unsigned side);

// Tidy up Walls array for load/save purposes.
void reset_walls();

// Called once per frame..
void wall_frame_process();

//  An object got stuck in a door (like a flare).
//  Add global entry.
void add_stuck_object(vobjptridx_t objp, segnum_t segnum, int sidenum);
void remove_obsolete_stuck_objects();

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(vsegptridx_t seg,int side,vsegptridx_t csegp,int cside,int anim_num,int frame_num);

// Remove any flares from a wall
void kill_stuck_objects(int wallnum);

#if defined(DXX_BUILD_DESCENT_II)
//start wall open <-> closed transitions
void start_wall_cloak(vsegptridx_t seg, int side);
void start_wall_decloak(vsegptridx_t seg, int side);

void cloaking_wall_read(cloaking_wall &cw, PHYSFS_file *fp);
void cloaking_wall_write(const cloaking_wall &cw, PHYSFS_file *fp);
#endif

void wclip_read(PHYSFS_file *, wclip &wc);
void wclip_write(PHYSFS_file *, const wclip &);

void v16_wall_read(PHYSFS_file *fp, v16_wall &w);
void v19_wall_read(PHYSFS_file *fp, v19_wall &w);
void wall_read(PHYSFS_file *fp, wall &w);

void active_door_read(PHYSFS_file *fp, active_door &ad);
void active_door_write(PHYSFS_file *fp, const active_door &ad);

void wall_write(PHYSFS_file *fp, const wall &w, short version);
void wall_close_door_num(int door_num);
void init_stuck_objects();
void clear_stuck_objects();
void blast_nearby_glass(vobjptr_t objp, fix damage);
