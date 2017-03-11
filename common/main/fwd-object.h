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
#include "cpp-valptridx.h"
#include "fwd-vecmat.h"
#include "fwd-segment.h"
#include "fwd-window.h"

struct bitmap_index;

namespace dcx {

// Movement types
enum movement_type_t : uint8_t;
constexpr std::size_t MAX_OBJECTS = 350;
constexpr std::integral_constant<std::size_t, MAX_OBJECTS - 20> MAX_USED_OBJECTS{};

}

#ifdef dsx
namespace dsx {
struct object;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::object, objnum_t, MAX_OBJECTS);
#endif

#include "fwd-valptridx.h"

#ifdef dsx
namespace dsx {
DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(object, obj, Objects);

static constexpr valptridx<object>::magic_constant<0xfffe> object_guidebot_cannot_reach{};
static constexpr valptridx<object>::magic_constant<0xffff> object_none{};
static constexpr valptridx<object>::magic_constant<0> object_first{};

#if defined(DXX_BUILD_DESCENT_I)
constexpr unsigned MAX_OBJECT_TYPES = 15;
#elif defined(DXX_BUILD_DESCENT_II)
constexpr unsigned MAX_OBJECT_TYPES = 16;
#endif

}
#endif

// Result types
typedef unsigned result_type_t;
constexpr result_type_t RESULT_NOTHING = 0;   // Ignore this collision
constexpr result_type_t RESULT_CHECK = 1;   // Check for this collision

// Control types - what tells this object what do do
typedef unsigned control_type_t;
constexpr control_type_t CT_NONE = 0;		// doesn't move (or change movement)
constexpr control_type_t CT_AI = 1;			// driven by AI
constexpr control_type_t CT_EXPLOSION = 2;	// explosion sequencer
constexpr control_type_t CT_FLYING = 4;		// the player is flying
constexpr control_type_t CT_SLEW = 5;		// slewing
constexpr control_type_t CT_FLYTHROUGH = 6;	// the flythrough system
constexpr control_type_t CT_WEAPON = 9;		// laser, etc.
constexpr control_type_t CT_REPAIRCEN = 10;	// under the control of the repair center
constexpr control_type_t CT_MORPH = 11;		// this object is being morphed
constexpr control_type_t CT_DEBRIS = 12;	// this is a piece of debris
constexpr control_type_t CT_POWERUP = 13;	// animating powerup blob
constexpr control_type_t CT_LIGHT = 14;		// doesn't actually do anything
constexpr control_type_t CT_REMOTE = 15;	// controlled by another net player
constexpr control_type_t CT_CNTRLCEN = 16;	// the control center/main reactor

// Render types
typedef unsigned render_type_t;
constexpr render_type_t RT_NONE = 0;   // does not render
constexpr render_type_t RT_POLYOBJ = 1;   // a polygon model
constexpr render_type_t RT_FIREBALL = 2;   // a fireball
constexpr render_type_t RT_LASER = 3;   // a laser
constexpr render_type_t RT_HOSTAGE = 4;   // a hostage
constexpr render_type_t RT_POWERUP = 5;   // a powerup
constexpr render_type_t RT_MORPH = 6;   // a robot being morphed
constexpr render_type_t RT_WEAPON_VCLIP = 7;   // a weapon that renders as a vclip

// misc object flags
typedef unsigned object_flag_t;
constexpr object_flag_t OF_EXPLODING = 1;   // this object is exploding
constexpr object_flag_t OF_SHOULD_BE_DEAD = 2;   // this object should be dead, so next time we can, we should delete this object.
constexpr object_flag_t OF_DESTROYED = 4;   // this has been killed, and is showing the dead version
constexpr object_flag_t OF_SILENT = 8;   // this makes no sound when it hits a wall.  Added by MK for weapons, if you extend it to other types, do it completely!
constexpr object_flag_t OF_ATTACHED = 16;  // this object is a fireball attached to another object
#if defined(DXX_BUILD_DESCENT_II)
constexpr object_flag_t OF_PLAYER_DROPPED = 64;  // this object was dropped by the player...
#endif

// physics flags
typedef unsigned physics_flag_t;
constexpr physics_flag_t PF_TURNROLL = 0x01;    // roll when turning
constexpr physics_flag_t PF_LEVELLING = 0x02;    // level object with closest side
constexpr physics_flag_t PF_BOUNCE = 0x04;    // bounce (not slide) when hit will
constexpr physics_flag_t PF_WIGGLE = 0x08;    // wiggle while flying
constexpr physics_flag_t PF_STICK = 0x10;    // object sticks (stops moving) when hits wall
constexpr physics_flag_t PF_PERSISTENT = 0x20;    // object keeps going even after it hits another object (eg, fusion cannon)
constexpr physics_flag_t PF_USES_THRUST = 0x40;    // this object uses its thrust
#if defined(DXX_BUILD_DESCENT_II)
constexpr physics_flag_t PF_BOUNCED_ONCE = 0x80;    // Weapon has bounced once.
constexpr physics_flag_t PF_FREE_SPINNING = 0x100;   // Drag does not apply to rotation of this object
constexpr physics_flag_t PF_BOUNCES_TWICE = 0x200;   // This weapon bounces twice, then dies
#endif

namespace dcx {
enum object_type_t : uint8_t;
struct object_base;

typedef unsigned powerup_flag_t;
constexpr powerup_flag_t PF_SPAT_BY_PLAYER = 1;   //this powerup was spat by the player

constexpr unsigned IMMORTAL_TIME = 0x3fffffff;  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.

struct shortpos;
struct quaternionpos;

// This is specific to the shortpos extraction routines in gameseg.c.
constexpr unsigned RELPOS_PRECISION = 10;
constexpr unsigned MATRIX_PRECISION = 9;

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
constexpr unsigned MAX_CONTROLCEN_GUNS = 4;
#elif defined(DXX_BUILD_DESCENT_II)
constexpr unsigned MAX_CONTROLCEN_GUNS = 8;
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

extern int num_objects;

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
void obj_relink(vobjptridx_t objnum,vsegptridx_t newsegnum);

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all();

// links an object into a segment's list of objects.
// takes object number and segment number
void obj_link(vobjptridx_t objnum,vsegptridx_t segnum);
/* Link an object without checking whether the object is currently
 * unlinked.  This should be used only in cases where the caller is
 * intentionally overriding the normal linking rules (such as loading
 * objects from demos or from the network).
 */
void obj_link_unchecked(vobjptridx_t obj, vsegptridx_t segnum);

// unlinks an object from a segment's list of objects
void obj_unlink(object_base &objnum);

// initialize a new object.  adds to the list for the given segment
// returns the object number
objptridx_t obj_create(object_type_t type, ubyte id, vsegptridx_t segnum, const vms_vector &pos, const vms_matrix *orient, fix size, ubyte ctype, ubyte mtype, ubyte rtype);

// make a copy of an object. returs num of new object
objptridx_t obj_create_copy(const object &srcobj, const vms_vector &new_pos, vsegptridx_t newsegnum);

// remove object from the world
void obj_delete(vobjptridx_t objnum);

// called after load.  Takes number of objects, and objects should be
// compressed
void reset_objects(int n_objs);

// make object array non-sparse
void compress_objects();

// Render an object.  Calls one of several routines based on type
void render_object(grs_canvas &, vobjptridx_t obj);

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
int update_object_seg(vobjptridx_t obj);


// Finds what segment *obj is in, returns segment number.  If not in
// any segment, returns -1.  Note: This function is defined in
// gameseg.h, but object.h depends on gameseg.h, and object.h is where
// object is defined...get it?
segptridx_t find_object_seg(vobjptr_t obj);

// go through all objects and make sure they have the correct segment
// numbers used when debugging is on
void fix_object_segs();

// Drops objects contained in objp.
objptridx_t object_create_robot_egg(vobjptr_t objp);
objptridx_t object_create_robot_egg(int type, int id, int num, const vms_vector &init_vel, const vms_vector &pos, const vsegptridx_t segnum);

// Interface to object_create_egg, puts count objects of type type, id
// = id in objp and then drops them.
objptridx_t call_object_create_egg(const object_base &objp, unsigned count, int id);

void dead_player_end();

// Extract information from an object (objp->orient, objp->pos,
// objp->segnum), stuff in a shortpos structure.  See typedef
// shortpos.
void create_shortpos_little(shortpos *spp, vcobjptr_t objp);
void create_shortpos_native(shortpos *spp, vcobjptr_t objp);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->segnum
void extract_shortpos_little(vobjptridx_t objp, const shortpos *spp);

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
void create_quaternionpos(quaternionpos * qpp, vobjptr_t objp, int swap_bytes);
void extract_quaternionpos(vobjptridx_t objp, quaternionpos *qpp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all);

// Returns a new, unique signature for a new object
object_signature_t obj_get_signature();

// returns the number of a free object, updating Highest_object_index.
// Generally, obj_create() should be called to get an object, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
objptridx_t obj_allocate();

// after calling init_object(), the network code has grabbed specific
// object slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void special_reset_objects();

// attaches an object, such as a fireball, to another object, such as
// a robot
void obj_attach(vobjptridx_t parent,vobjptridx_t sub);

void create_small_fireball_on_object(vobjptridx_t objp, fix size_scale, int sound_flag);
window_event_result dead_player_frame();

#if defined(DXX_BUILD_DESCENT_II)
extern int Drop_afterburner_blob_flag;		//ugly hack
// returns object number
objptridx_t drop_marker_object(const vms_vector &pos, vsegptridx_t segnum, const vms_matrix &orient, int marker_num);

void wake_up_rendered_objects(vobjptr_t gmissp, window_rendered_data &window);

void fuelcen_check_for_goal(vcsegptr_t);
#endif
objptridx_t obj_find_first_of_type(int type);

void object_rw_swap(struct object_rw *obj_rw, int swap);
void reset_player_object();

}
#endif
