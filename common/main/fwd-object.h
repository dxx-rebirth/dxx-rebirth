/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <type_traits>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"
#include "objnum.h"
#include "fwd-vecmat.h"
#include "fwd-segment.h"
#include "fwd-window.h"
#include "fwd-valptridx.h"

struct bitmap_index;

namespace dcx {

// Movement types
enum movement_type_t : uint8_t;
constexpr std::integral_constant<std::size_t, 350> MAX_OBJECTS{};
constexpr std::integral_constant<std::size_t, MAX_OBJECTS - 20> MAX_USED_OBJECTS{};

}

#ifdef dsx
namespace dsx {
struct object;
struct d_level_object_state;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, object, objnum_t, MAX_OBJECTS);

namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(object, obj);

static constexpr valptridx<object>::magic_constant<0xfffe> object_guidebot_cannot_reach{};
static constexpr valptridx<object>::magic_constant<0xffff> object_none{};
static constexpr valptridx<object>::magic_constant<0> object_first{};

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 15> MAX_OBJECT_TYPES{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 16> MAX_OBJECT_TYPES{};
#endif

}
#endif

// Result types
typedef unsigned result_type_t;
constexpr std::integral_constant<result_type_t, 0> RESULT_NOTHING{};   // Ignore this collision
constexpr std::integral_constant<result_type_t, 1> RESULT_CHECK{};   // Check for this collision

// Control types - what tells this object what do do
typedef unsigned control_type_t;
constexpr std::integral_constant<control_type_t, 0> CT_NONE{};		// doesn't move (or change movement)
constexpr std::integral_constant<control_type_t, 1> CT_AI{};			// driven by AI
constexpr std::integral_constant<control_type_t, 2> CT_EXPLOSION{};	// explosion sequencer
constexpr std::integral_constant<control_type_t, 4> CT_FLYING{};		// the player is flying
constexpr std::integral_constant<control_type_t, 5> CT_SLEW{};		// slewing
constexpr std::integral_constant<control_type_t, 6> CT_FLYTHROUGH{};	// the flythrough system
constexpr std::integral_constant<control_type_t, 9> CT_WEAPON{};		// laser, etc.
constexpr std::integral_constant<control_type_t, 10> CT_REPAIRCEN{};	// under the control of the repair center
constexpr std::integral_constant<control_type_t, 11> CT_MORPH{};		// this object is being morphed
constexpr std::integral_constant<control_type_t, 12> CT_DEBRIS{};	// this is a piece of debris
constexpr std::integral_constant<control_type_t, 13> CT_POWERUP{};	// animating powerup blob
constexpr std::integral_constant<control_type_t, 14> CT_LIGHT{};		// doesn't actually do anything
constexpr std::integral_constant<control_type_t, 15> CT_REMOTE{};	// controlled by another net player
constexpr std::integral_constant<control_type_t, 16> CT_CNTRLCEN{};	// the control center/main reactor

// Render types
typedef unsigned render_type_t;
constexpr std::integral_constant<render_type_t, 0> RT_NONE{};   // does not render
constexpr std::integral_constant<render_type_t, 1> RT_POLYOBJ{};   // a polygon model
constexpr std::integral_constant<render_type_t, 2> RT_FIREBALL{};   // a fireball
constexpr std::integral_constant<render_type_t, 3> RT_LASER{};   // a laser
constexpr std::integral_constant<render_type_t, 4> RT_HOSTAGE{};   // a hostage
constexpr std::integral_constant<render_type_t, 5> RT_POWERUP{};   // a powerup
constexpr std::integral_constant<render_type_t, 6> RT_MORPH{};   // a robot being morphed
constexpr std::integral_constant<render_type_t, 7> RT_WEAPON_VCLIP{};   // a weapon that renders as a vclip

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
struct object_rw;

}

#ifdef dsx
namespace dsx {

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 4> MAX_CONTROLCEN_GUNS{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 8> MAX_CONTROLCEN_GUNS{};
#endif

struct powerup_info_rw;
struct window_rendered_data;
struct reactor_static;

typedef array<uint8_t, MAX_OBJECT_TYPES> collision_inner_array_t;
typedef array<collision_inner_array_t, MAX_OBJECT_TYPES> collision_outer_array_t;
extern const collision_outer_array_t CollisionResult;

}
#endif

namespace dcx {
extern int Object_next_signature;   // The next signature for the next newly created object

extern int Num_robot_types;
}

#ifdef dsx
namespace dsx {
extern object *ConsoleObject;       // pointer to the object that is the player
extern object *Viewer;              // which object we are seeing from
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
extern int Death_sequence_aborted;
extern objnum_t Player_fired_laser_this_frame;

// Draw a blob-type object, like a fireball
void draw_object_blob(grs_canvas &, const object_base &obj, bitmap_index bitmap);
}

#ifdef dsx
namespace dsx {
// do whatever setup needs to be done
void init_objects();

// when an object has moved into a new segment, this function unlinks it
// from its old segment, and links it into the new segment
void obj_relink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, vmobjptridx_t objnum, vmsegptridx_t newsegnum);

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all();

// links an object into a segment's list of objects.
// takes object number and segment number
void obj_link(fvmobjptr &vmobjptr, vmobjptridx_t objnum, vmsegptridx_t segnum);
/* Link an object without checking whether the object is currently
 * unlinked.  This should be used only in cases where the caller is
 * intentionally overriding the normal linking rules (such as loading
 * objects from demos or from the network).
 */
void obj_link_unchecked(fvmobjptr &vmobjptr, vmobjptridx_t obj, vmsegptridx_t segnum);

// unlinks an object from a segment's list of objects
void obj_unlink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, object_base &obj);

// initialize a new object.  adds to the list for the given segment
// returns the object number
imobjptridx_t obj_create(object_type_t type, ubyte id, vmsegptridx_t segnum, const vms_vector &pos, const vms_matrix *orient, fix size, ubyte ctype, ubyte mtype, ubyte rtype);

// make a copy of an object. returs num of new object
imobjptridx_t obj_create_copy(const object &srcobj, const vms_vector &new_pos, vmsegptridx_t newsegnum);

// remove object from the world
void obj_delete(vmobjptridx_t objnum);

// called after load.  Takes number of objects, and objects should be
// compressed
void reset_objects(d_level_object_state &, unsigned n_objs);

// make object array non-sparse
void compress_objects();

// Render an object.  Calls one of several routines based on type
void render_object(grs_canvas &, vmobjptridx_t obj);

// draw an object that is a texture-mapped rod
void draw_object_tmap_rod(grs_canvas &, vcobjptridx_t obj, bitmap_index bitmap, int lighted);

// move all objects for the current frame
window_event_result object_move_all();     // moves all objects

// set viewer object to next object in array
void object_goto_next_viewer();

// make object0 the player, setting all relevant fields
void init_player_object();

// check if object is in object->segnum.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->segnum &
// returns true callers should really use find_vector_intersection()
// Note: this function is in gameseg.c
int update_object_seg(vmobjptridx_t obj);


// Finds what segment *obj is in, returns segment number.  If not in
// any segment, returns -1.  Note: This function is defined in
// gameseg.h, but object.h depends on gameseg.h, and object.h is where
// object is defined...get it?
imsegptridx_t find_object_seg(vmobjptr_t obj);

// go through all objects and make sure they have the correct segment
// numbers used when debugging is on
void fix_object_segs();

// Drops objects contained in objp.
imobjptridx_t object_create_robot_egg(vmobjptr_t objp);
imobjptridx_t object_create_robot_egg(int type, int id, int num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum);

// Interface to object_create_egg, puts count objects of type type, id
// = id in objp and then drops them.
imobjptridx_t call_object_create_egg(const object_base &objp, unsigned count, int id);

void dead_player_end();

// Extract information from an object (objp->orient, objp->pos,
// objp->segnum), stuff in a shortpos structure.  See typedef
// shortpos.
void create_shortpos_little(fvcsegptr &vcsegptr, fvcvertptr &vcvertptr, shortpos *spp, const object_base &objp);
void create_shortpos_native(fvcsegptr &vcsegptr, fvcvertptr &vcvertptr, shortpos *spp, const object_base &objp);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->segnum
void extract_shortpos_little(vmobjptridx_t objp, const shortpos *spp);

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
void create_quaternionpos(quaternionpos &qpp, const object_base &objp);
void extract_quaternionpos(vmobjptridx_t objp, quaternionpos &qpp);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all);

// Returns a new, unique signature for a new object
object_signature_t obj_get_signature();

// returns the number of a free object, updating Highest_object_index.
// Generally, obj_create() should be called to get an object, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
imobjptridx_t obj_allocate(d_level_object_state &);

// after calling init_object(), the network code has grabbed specific
// object slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void special_reset_objects(d_level_object_state &);

// attaches an object, such as a fireball, to another object, such as
// a robot
void obj_attach(object_array &Objects, vmobjptridx_t parent, vmobjptridx_t sub);

void create_small_fireball_on_object(vmobjptridx_t objp, fix size_scale, int sound_flag);
window_event_result dead_player_frame();

#if defined(DXX_BUILD_DESCENT_II)
extern int Drop_afterburner_blob_flag;		//ugly hack
// returns object number
imobjptridx_t drop_marker_object(const vms_vector &pos, vmsegptridx_t segnum, const vms_matrix &orient, int marker_num);

void wake_up_rendered_objects(vmobjptr_t gmissp, window_rendered_data &window);

void fuelcen_check_for_goal(vcsegptr_t);
#endif
imobjptridx_t obj_find_first_of_type(int type);

void object_rw_swap(struct object_rw *obj_rw, int swap);
void reset_player_object();

}
#endif
