/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <type_traits>
#include "dsx-ns.h"
#include "objnum.h"
#include "fwd-piggy.h"
#include "fwd-vecmat.h"
#include "fwd-segment.h"
#include "fwd-window.h"
#include "fwd-valptridx.h"
#include "polyobj.h"
#include <array>

namespace dcx {

// Movement types
constexpr std::integral_constant<std::size_t, 350> MAX_OBJECTS{};
constexpr std::integral_constant<std::size_t, MAX_OBJECTS - 20> MAX_USED_OBJECTS{};
struct d_level_unique_control_center_state;

// Render types
enum class render_type : uint8_t;
enum class gun_num_t : uint8_t;

}

#ifdef dsx
namespace dsx {
struct object;
struct d_level_unique_object_state;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, object, objnum_t, MAX_OBJECTS);

namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(object, obj);

static constexpr valptridx<object>::magic_constant<0xffff> object_none{};
static constexpr valptridx<object>::magic_constant<0> object_first{};

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 15> MAX_OBJECT_TYPES{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 16> MAX_OBJECT_TYPES{};
struct d_level_unique_control_center_state;
#endif

}
#endif

// misc object flags
typedef unsigned object_flag_t;
constexpr std::integral_constant<object_flag_t, 1> OF_EXPLODING{};   // this object is exploding
constexpr std::integral_constant<object_flag_t, 2> OF_SHOULD_BE_DEAD{};   // this object should be dead, so next time we can, we should delete this object.
constexpr std::integral_constant<object_flag_t, 4> OF_DESTROYED{};   // this has been killed, and is showing the dead version
constexpr std::integral_constant<object_flag_t, 8> OF_SILENT{};   // this makes no sound when it hits a wall.  Added by MK for weapons, if you extend it to other types, do it completely!
constexpr std::integral_constant<object_flag_t, 16> OF_ATTACHED{};  // this object is a fireball attached to another object
#if defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<object_flag_t, 64> OF_PLAYER_DROPPED{};  // this object was dropped by the player...
#endif

// physics flags
typedef unsigned physics_flag_t;
constexpr std::integral_constant<physics_flag_t, 0x01> PF_TURNROLL{};    // roll when turning
constexpr std::integral_constant<physics_flag_t, 0x02> PF_LEVELLING{};    // level object with closest side
constexpr std::integral_constant<physics_flag_t, 0x04> PF_BOUNCE{};    // bounce (not slide) when hit will
constexpr std::integral_constant<physics_flag_t, 0x08> PF_WIGGLE{};    // wiggle while flying
constexpr std::integral_constant<physics_flag_t, 0x10> PF_STICK{};    // object sticks (stops moving) when hits wall
constexpr std::integral_constant<physics_flag_t, 0x20> PF_PERSISTENT{};    // object keeps going even after it hits another object (eg, fusion cannon)
constexpr std::integral_constant<physics_flag_t, 0x40> PF_USES_THRUST{};    // this object uses its thrust
#if defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<physics_flag_t, 0x80> PF_BOUNCED_ONCE{};    // Weapon has bounced once.
constexpr std::integral_constant<physics_flag_t, 0x100> PF_FREE_SPINNING{};   // Drag does not apply to rotation of this object
constexpr std::integral_constant<physics_flag_t, 0x200> PF_BOUNCES_TWICE{};   // This weapon bounces twice, then dies
#endif

namespace dcx {
enum object_type_t : uint8_t;
struct object_base;

typedef unsigned powerup_flag_t;
constexpr std::integral_constant<powerup_flag_t, 1> PF_SPAT_BY_PLAYER{};   //this powerup was spat by the player

constexpr std::integral_constant<unsigned, 0x3fffffff> IMMORTAL_TIME{};  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.

struct shortpos;
struct quaternionpos;

// This is specific to the shortpos extraction routines in gameseg.c.
constexpr std::integral_constant<unsigned, 10> RELPOS_PRECISION{};
constexpr std::integral_constant<unsigned, 9> MATRIX_PRECISION{};

struct physics_info;
struct physics_info_rw;

struct laser_parent;
struct laser_info_rw;

struct explosion_info;
struct explosion_info_rw;

struct light_info;
struct light_info_rw;

struct powerup_info;

struct vclip_info;
struct vclip_info_rw;

struct polyobj_info;
struct polyobj_info_rw;

struct obj_position;

enum class collision_result : bool
{
	ignore = 0,	// Ignore this collision
	check = 1,	// Check for this collision
};

}

#ifdef dsx
namespace dsx {

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 4> MAX_CONTROLCEN_GUNS{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 8> MAX_CONTROLCEN_GUNS{};
struct d_unique_buddy_state;
#endif

struct object_rw;
struct powerup_info_rw;
struct window_rendered_data;
struct reactor_static;

using collision_inner_array_t = std::array<collision_result, MAX_OBJECT_TYPES>;
using collision_outer_array_t = std::array<collision_inner_array_t, MAX_OBJECT_TYPES>;
extern const collision_outer_array_t CollisionResult;

}
#endif

namespace dcx {
extern int Num_robot_types;
}

#ifdef dsx
namespace dsx {
extern object *ConsoleObject;       // pointer to the object that is the player
extern const object *Viewer;              // which object we are seeing from
extern object *Dead_player_camera;
}
#endif

enum class player_dead_state : uint8_t
{
	no,
	yes,
	exploded,
};

namespace dcx {
extern player_dead_state Player_dead_state;          // !0 means player is dead!
extern objnum_t Player_fired_laser_this_frame;
}

#ifdef dsx
namespace dsx {
// Draw a blob-type object, like a fireball
void draw_object_blob(GameBitmaps_array &GameBitmaps, const object_base &Viewer, grs_canvas &, const object_base &obj, bitmap_index bitmap);

// do whatever setup needs to be done
void init_objects();

// when an object has moved into a new segment, this function unlinks it
// from its old segment, and links it into the new segment
void obj_relink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, vmobjptridx_t objnum, vmsegptridx_t newsegnum);

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all();

/* Link an object without checking whether the object is currently
 * unlinked.  This should be used only in cases where the caller is
 * intentionally overriding the normal linking rules (such as loading
 * objects from demos or from the network).
 */
void obj_link_unchecked(fvmobjptr &vmobjptr, vmobjptridx_t obj, vmsegptridx_t segnum);

// unlinks an object from a segment's list of objects
void obj_unlink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, object_base &obj);

// make a copy of an object. returs num of new object
imobjptridx_t obj_create_copy(const object &srcobj, vmsegptridx_t newsegnum);

// remove object from the world
void obj_delete(d_level_unique_object_state &LevelUniqueObjectState, segment_array &Segments, vmobjptridx_t objnum);

// called after load.  Takes number of objects, and objects should be
// compressed
void reset_objects(d_level_unique_object_state &, unsigned n_objs);

#if DXX_USE_EDITOR
// make object array non-sparse
void compress_objects();
#endif

// set viewer object to next object in array
void object_goto_next_viewer(const object_array &Objects, const object *&viewer);

// make object0 the player, setting all relevant fields
void init_player_object(const d_level_shared_polygon_model_state &LevelSharedPolygonModelState, object_base &console);

// check if object is in object->segnum.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->segnum &
// returns true callers should really use find_vector_intersection()
// Note: this function is in gameseg.c
int update_object_seg(fvmobjptr &vmobjptr, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, vmobjptridx_t obj);

// Finds what segment *obj is in, returns segment number.  If not in
// any segment, returns -1.  Note: This function is defined in
// gameseg.h, but object.h depends on gameseg.h, and object.h is where
// object is defined...get it?
imsegptridx_t find_object_seg(const d_level_shared_segment_state &, d_level_unique_segment_state &, const object_base &obj);

// go through all objects and make sure they have the correct segment
// numbers used when debugging is on
void fix_object_segs();

void dead_player_end();

// Extract information from an object (objp->orient, objp->pos,
// objp->segnum), stuff in a shortpos structure.  See typedef
// shortpos.
void create_shortpos_little(const d_level_shared_segment_state &, shortpos &spp, const object_base &objp);
void create_shortpos_native(const d_level_shared_segment_state &, shortpos &spp, const object_base &objp);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->segnum
void extract_shortpos_little(vmobjptridx_t objp, const shortpos *spp);

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
[[nodiscard]]
quaternionpos build_quaternionpos(const object_base &objp);
void extract_quaternionpos(vmobjptridx_t objp, quaternionpos &qpp);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all);

// returns the number of a free object, updating Highest_object_index.
// Generally, obj_create() should be called to get an object, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
[[nodiscard]]
imobjptridx_t obj_allocate(d_level_unique_object_state &);

// attaches an object, such as a fireball, to another object, such as
// a robot
void obj_attach(object_array &Objects, vmobjptridx_t parent, vmobjptridx_t sub);

void create_small_fireball_on_object(vmobjptridx_t objp, fix size_scale, int sound_flag);

#if defined(DXX_BUILD_DESCENT_II)
extern int Drop_afterburner_blob_flag;		//ugly hack
enum class game_marker_index : uint8_t;
enum class player_marker_index : uint8_t;
// returns object number
imobjptridx_t drop_marker_object(const vms_vector &pos, vmsegptridx_t segnum, const vms_matrix &orient, game_marker_index marker_num);

void wake_up_rendered_objects(const object &gmissp, window_rendered_data &window);

void fuelcen_check_for_goal(object &plrobj, const shared_segment &segp);
#endif
imobjptridx_t obj_find_first_of_type(fvmobjptridx &, object_type_t type);

void object_rw_swap(struct object_rw *obj_rw, int swap);
void reset_player_object(object_base &);

}
#endif
