/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <type_traits>
#include <physfs.h>
#include "piggy.h"
#include "maths.h"
#include "textures.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "cpp-valptridx.h"

namespace dcx {
enum class actdoornum_t : uint8_t;
constexpr std::integral_constant<std::size_t, 255> MAX_WALLS{}; // Maximum number of walls
constexpr std::integral_constant<std::size_t, 90> MAX_DOORS{};  // Maximum number of open doors
struct active_door;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dcx::, active_door, actdoornum_t, MAX_DOORS);

#include "fwd-valptridx.h"

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<std::size_t, 30> MAX_WALL_ANIMS{};		// Maximum different types of doors
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<std::size_t, 60> MAX_WALL_ANIMS{};  // Maximum different types of doors
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
}

namespace dcx {
typedef unsigned wall_flag_t;
constexpr std::integral_constant<wall_flag_t, 1> WALL_BLASTED{};   // Blasted out wall.
constexpr std::integral_constant<wall_flag_t, 2> WALL_DOOR_OPENED{};   // Open door.
constexpr std::integral_constant<wall_flag_t, 8> WALL_DOOR_LOCKED{};   // Door is locked.
constexpr std::integral_constant<wall_flag_t, 16> WALL_DOOR_AUTO{};  // Door automatically closes after time.
constexpr std::integral_constant<wall_flag_t, 32> WALL_ILLUSION_OFF{};  // Illusionary wall is shut off.
constexpr std::integral_constant<wall_flag_t, 64> WALL_EXPLODING{};
}
#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
constexpr std::integral_constant<wall_flag_t, 128> WALL_BUDDY_PROOF{}; // Buddy assumes he cannot get through this wall.
}
#endif

namespace dcx {
typedef unsigned wall_state_t;
constexpr std::integral_constant<wall_state_t, 0> WALL_DOOR_CLOSED{};       // Door is closed
constexpr std::integral_constant<wall_state_t, 1> WALL_DOOR_OPENING{};       // Door is opening.
constexpr std::integral_constant<wall_state_t, 2> WALL_DOOR_WAITING{};       // Waiting to close
constexpr std::integral_constant<wall_state_t, 3> WALL_DOOR_CLOSING{};       // Door is closing
}
#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
constexpr std::integral_constant<wall_state_t, 4> WALL_DOOR_OPEN{};       // Door is open, and staying open
constexpr std::integral_constant<wall_state_t, 5> WALL_DOOR_CLOAKING{};       // Wall is going from closed -> open
constexpr std::integral_constant<wall_state_t, 6> WALL_DOOR_DECLOAKING{};       // Wall is going from open -> closed
}
#endif

namespace dcx {
enum class wall_key : uint8_t;

constexpr std::integral_constant<fix, 100 * F1_0> WALL_HPS{};    // Normal wall's hp
constexpr std::integral_constant<fix, 5 * F1_0> WALL_DOOR_INTERVAL{};      // How many seconds a door is open

constexpr fix DOOR_OPEN_TIME = i2f(2);      // How long takes to open
constexpr fix DOOR_WAIT_TIME = i2f(5);      // How long before auto door closes
}

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<std::size_t, 20> MAX_CLIP_FRAMES{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<std::size_t, 50> MAX_CLIP_FRAMES{};
#endif
}

namespace dcx {

enum class WALL_IS_DOORWAY_FLAG : uint8_t
{
	None = 0,
	fly = 1,
	render = 2,
	rendpast = 4,
	external = 8,
	/* if DXX_BUILD_DESCENT_II */
	cloaked = 16,
	/* endif */
};

static constexpr WALL_IS_DOORWAY_FLAG operator|(const WALL_IS_DOORWAY_FLAG a, const WALL_IS_DOORWAY_FLAG b)
{
	return static_cast<WALL_IS_DOORWAY_FLAG>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

enum class WALL_IS_DOORWAY_sresult_t : unsigned;

static constexpr WALL_IS_DOORWAY_sresult_t WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG F)
{
	return WALL_IS_DOORWAY_sresult_t{static_cast<unsigned>(F)};
}

struct WALL_IS_DOORWAY_result_t;

//  WALL_IS_DOORWAY return values          F/R/RP
constexpr auto WID_WALL                = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::render);   // 0/1/0        wall
constexpr auto WID_TRANSPARENT_WALL    = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::render | WALL_IS_DOORWAY_FLAG::rendpast);   // 0/1/1        transparent wall
constexpr auto WID_ILLUSORY_WALL       = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::fly | WALL_IS_DOORWAY_FLAG::render);   // 1/1/0        illusory wall
constexpr auto WID_TRANSILLUSORY_WALL  = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::fly | WALL_IS_DOORWAY_FLAG::render | WALL_IS_DOORWAY_FLAG::rendpast);   // 1/1/1        transparent illusory wall
constexpr auto WID_NO_WALL             = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::fly | WALL_IS_DOORWAY_FLAG::rendpast);   //  1/0/1       no wall, can fly through
constexpr auto WID_EXTERNAL            = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::external);   // 0/0/0/1  don't see it, dont fly through it
}
#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
constexpr auto WID_CLOAKED_WALL        = WALL_IS_DOORWAY_sresult(WALL_IS_DOORWAY_FLAG::render | WALL_IS_DOORWAY_FLAG::rendpast | WALL_IS_DOORWAY_FLAG::cloaked);
}
#endif
#endif

namespace dcx {

struct stuckobj;
struct v16_wall;
struct v19_wall;

typedef unsigned wall_clip_flag_t;
constexpr std::integral_constant<wall_clip_flag_t, 1> WCF_EXPLODES{};       //door explodes when opening
constexpr std::integral_constant<wall_clip_flag_t, 2> WCF_BLASTABLE{};       //this is a blastable wall
constexpr std::integral_constant<wall_clip_flag_t, 4> WCF_TMAP1{};       //this uses primary tmap, not tmap2
constexpr std::integral_constant<wall_clip_flag_t, 8> WCF_HIDDEN{};       //this uses primary tmap, not tmap2
}

#ifdef dsx
namespace dsx {
struct wall;
struct wclip;
constexpr std::integral_constant<std::size_t, 20> MAX_CLIP_FRAMES_D1{};
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct cloaking_wall;
constexpr std::integral_constant<std::size_t, 10> MAX_CLOAKING_WALLS{};
using clwallnum_t = uint8_t;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, cloaking_wall, clwallnum_t, dsx::MAX_CLOAKING_WALLS);
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(cloaking_wall, clwall);
}
#endif

DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, wall, wallnum_t, dcx::MAX_WALLS);
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(wall, wall);
using wall_animations_array = std::array<wclip, MAX_WALL_ANIMS>;
constexpr valptridx<wall>::magic_constant<wallnum_t{0xffffu}> wall_none{};
}

namespace dcx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(active_door, actdoor);
extern unsigned Num_wall_anims;
}
#endif

#if DXX_USE_EDITOR
#ifdef dsx
namespace dsx {
void wall_init();
}
#endif
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
// Automatically checks if a there is a doorway (i.e. can fly through)
#ifdef dsx
namespace dsx {
WALL_IS_DOORWAY_result_t WALL_IS_DOORWAY(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, fvcwallptr &vcwallptr, cscusegment seg, const uint_fast32_t side);

// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
}
#endif
void wall_damage(vmsegptridx_t seg, unsigned side, fix damage);

// Destroys a blastable wall. (So it is an opening afterwards)
void wall_destroy(vmsegptridx_t seg, unsigned side);

#ifdef dsx
namespace dsx {

void wall_illusion_on(fvmwallptr &, vcsegptridx_t seg, unsigned side);
void wall_illusion_off(fvmwallptr &, vcsegptridx_t seg, unsigned side);

// Opens a door
void wall_open_door(vmsegptridx_t seg, unsigned side);

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
// Closes a door
void wall_close_door(wall_array &Walls, vmsegptridx_t seg, unsigned side);
#endif
}
#endif

//return codes for wall_hit_process()
enum class wall_hit_process_t : unsigned
{
	WHP_NOT_SPECIAL = 0,       //wasn't a quote-wall-unquote
	WHP_NO_KEY = 1,       //hit door, but didn't have key
	WHP_BLASTABLE = 2,       //hit blastable wall
	WHP_DOOR = 3,       //a door (which will now be opening)
};

// Determines what happens when a wall is shot
//obj is the object that hit...either a weapon or the player himself
#ifdef dsx
class player_flags;
namespace dsx {
wall_hit_process_t wall_hit_process(player_flags, vmsegptridx_t seg, unsigned side, fix damage, unsigned playernum, const object &obj);

// Opens/destroys specified door.
void wall_toggle(fvmwallptr &vmwallptr, vmsegptridx_t segnum, unsigned side);

// Called once per frame..
void wall_frame_process();

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(const wclip &, vmsegptridx_t seg, unsigned side, vmsegptridx_t csegp, unsigned cside, unsigned frame_num);
void wclip_read(PHYSFS_File *, wclip &wc);
void wall_read(PHYSFS_File *fp, wall &w);
void wall_write(PHYSFS_File *fp, const wall &w, short version);

void wall_close_door_ref(fvmsegptridx &vmsegptridx, wall_array &Walls, const wall_animations_array &WallAnims, active_door &);
}
#endif

#if defined(DXX_BUILD_DESCENT_II)
//start wall open <-> closed transitions
namespace dsx {
void start_wall_cloak(vmsegptridx_t seg, unsigned side);
void start_wall_decloak(vmsegptridx_t seg, unsigned side);

void cloaking_wall_read(cloaking_wall &cw, PHYSFS_File *fp);
void cloaking_wall_write(const cloaking_wall &cw, PHYSFS_File *fp);
void blast_nearby_glass(const object &objp, fix damage);
}
#endif
#endif

void v16_wall_read(PHYSFS_File *fp, v16_wall &w);
void v19_wall_read(PHYSFS_File *fp, v19_wall &w);

void active_door_read(PHYSFS_File *fp, active_door &ad);
void active_door_write(PHYSFS_File *fp, const active_door &ad);
#endif
