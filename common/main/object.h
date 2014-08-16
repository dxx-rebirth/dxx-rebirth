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
 * object system definitions
 *
 */

#ifndef _OBJECT_H
#define _OBJECT_H

#include "pstypes.h"
#include "vecmat.h"
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "aistruct.h"
#endif
#include "polyobj.h"
#include "laser.h"

#ifdef __cplusplus
#include <cassert>
#include <cstdint>
#include "dxxsconf.h"
#include "compiler-array.h"
#include "valptridx.h"
#include "objnum.h"
#include "segnum.h"
#include <vector>
#include <stdexcept>
#include "compiler-type_traits.h"

struct segment;

/*
 * CONSTANTS
 */

#define MAX_OBJECTS     350 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world
#define MAX_USED_OBJECTS	(MAX_OBJECTS-20)

// Object types
enum object_type_t
{
	OBJ_NONE	= 255, // unused object
	OBJ_WALL	= 0,   // A wall... not really an object, but used for collisions
	OBJ_FIREBALL	= 1,   // a fireball, part of an explosion
	OBJ_ROBOT	= 2,   // an evil enemy
	OBJ_HOSTAGE	= 3,   // a hostage you need to rescue
	OBJ_PLAYER	= 4,   // the player on the console
	OBJ_WEAPON	= 5,   // a laser, missile, etc
	OBJ_CAMERA	= 6,   // a camera to slew around with
	OBJ_POWERUP	= 7,   // a powerup you can pick up
	OBJ_DEBRIS	= 8,   // a piece of robot
	OBJ_CNTRLCEN	= 9,   // the control center
	OBJ_CLUTTER	= 11,  // misc objects
	OBJ_GHOST	= 12,  // what the player turns into when dead
	OBJ_LIGHT	= 13,  // a light source, & not much else
	OBJ_COOP	= 14,  // a cooperative player object.
#if defined(DXX_BUILD_DESCENT_II)
	OBJ_MARKER	= 15,  // a map marker
#endif
};

// WARNING!! If you add a type here, add its name to Object_type_names
// in object.c
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_OBJECT_TYPES	15
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_OBJECT_TYPES    16
#endif

// Result types
#define RESULT_NOTHING  0   // Ignore this collision
#define RESULT_CHECK    1   // Check for this collision

// Control types - what tells this object what do do
#define CT_NONE         0   // doesn't move (or change movement)
#define CT_AI           1   // driven by AI
#define CT_EXPLOSION    2   // explosion sequencer
#define CT_FLYING       4   // the player is flying
#define CT_SLEW         5   // slewing
#define CT_FLYTHROUGH   6   // the flythrough system
#define CT_WEAPON       9   // laser, etc.
#define CT_REPAIRCEN    10  // under the control of the repair center
#define CT_MORPH        11  // this object is being morphed
#define CT_DEBRIS       12  // this is a piece of debris
#define CT_POWERUP      13  // animating powerup blob
#define CT_LIGHT        14  // doesn't actually do anything
#define CT_REMOTE       15  // controlled by another net player
#define CT_CNTRLCEN     16  // the control center/main reactor

// Movement types
#define MT_NONE         0   // doesn't move
#define MT_PHYSICS      1   // moves by physics
#define MT_SPINNING     3   // this object doesn't move, just sits and spins

// Render types
#define RT_NONE         0   // does not render
#define RT_POLYOBJ      1   // a polygon model
#define RT_FIREBALL     2   // a fireball
#define RT_LASER        3   // a laser
#define RT_HOSTAGE      4   // a hostage
#define RT_POWERUP      5   // a powerup
#define RT_MORPH        6   // a robot being morphed
#define RT_WEAPON_VCLIP 7   // a weapon that renders as a vclip

// misc object flags
#define OF_EXPLODING        1   // this object is exploding
#define OF_SHOULD_BE_DEAD   2   // this object should be dead, so next time we can, we should delete this object.
#define OF_DESTROYED        4   // this has been killed, and is showing the dead version
#define OF_SILENT           8   // this makes no sound when it hits a wall.  Added by MK for weapons, if you extend it to other types, do it completely!
#define OF_ATTACHED         16  // this object is a fireball attached to another object
#if defined(DXX_BUILD_DESCENT_II)
#define OF_PLAYER_DROPPED   64  // this object was dropped by the player...
#endif

// Different Weapon ID types...
#define WEAPON_ID_LASER         0
#define WEAPON_ID_MISSLE        1
#define WEAPON_ID_CANNONBALL    2

// Object Initial shields...
#define OBJECT_INITIAL_SHIELDS F1_0/2

// physics flags
#define PF_TURNROLL         0x01    // roll when turning
#define PF_LEVELLING        0x02    // level object with closest side
#define PF_BOUNCE           0x04    // bounce (not slide) when hit will
#define PF_WIGGLE           0x08    // wiggle while flying
#define PF_STICK            0x10    // object sticks (stops moving) when hits wall
#define PF_PERSISTENT       0x20    // object keeps going even after it hits another object (eg, fusion cannon)
#define PF_USES_THRUST      0x40    // this object uses its thrust
#if defined(DXX_BUILD_DESCENT_II)
#define PF_BOUNCED_ONCE     0x80    // Weapon has bounced once.
#define PF_FREE_SPINNING    0x100   // Drag does not apply to rotation of this object
#define PF_BOUNCES_TWICE    0x200   // This weapon bounces twice, then dies

#define PF_SPAT_BY_PLAYER   1   //this powerup was spat by the player
#endif

#define IMMORTAL_TIME   0x3fffffff  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.

/*
 * STRUCTURES
 */

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern const char Object_type_names[MAX_OBJECT_TYPES][9];
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_CONTROLCEN_GUNS     4
#define MAX_RENDERED_WINDOWS    1
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_CONTROLCEN_GUNS     8
#define MAX_RENDERED_WINDOWS    3
#endif

struct reactor_static {
	/* Location of the gun on the reactor object */
	vms_vector	gun_pos[MAX_CONTROLCEN_GUNS];
	/* Orientation of the gun on the reactor object */
	vms_vector	gun_dir[MAX_CONTROLCEN_GUNS];
};
#endif

// A compressed form for sending crucial data
struct shortpos
{
	sbyte   bytemat[9];
	short   xo,yo,zo;
	short   segment;
	short   velx, vely, velz;
} __pack__;

// Another compressed form for object position, velocity, orientation and rotvel using quaternion
struct quaternionpos
{
	vms_quaternion orient;
	vms_vector pos;
	short segment;
	vms_vector vel;
	vms_vector rotvel;
} __pack__;

// This is specific to the shortpos extraction routines in gameseg.c.
#define RELPOS_PRECISION    10
#define MATRIX_PRECISION    9
#define MATRIX_MAX          0x7f    // This is based on MATRIX_PRECISION, 9 => 0x7f

// information for physics sim for an object
struct physics_info : public prohibit_void_ptr<physics_info>
{
	vms_vector  velocity;   // velocity vector of this object
	vms_vector  thrust;     // constant force applied to this object
	fix         mass;       // the mass of this object
	fix         drag;       // how fast this slows down
	vms_vector  rotvel;     // rotational velecity (angles)
	vms_vector  rotthrust;  // rotational acceleration
	fixang      turnroll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
};

struct physics_info_rw
{
	vms_vector  velocity;   // velocity vector of this object
	vms_vector  thrust;     // constant force applied to this object
	fix         mass;       // the mass of this object
	fix         drag;       // how fast this slows down
	fix         obsolete_brakes;     // how much brakes applied
	vms_vector  rotvel;     // rotational velecity (angles)
	vms_vector  rotthrust;  // rotational acceleration
	fixang      turnroll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
} __pack__;

// stuctures for different kinds of simulation

struct laser_info : public prohibit_void_ptr<laser_info>
{
	struct hitobj_list_t : public prohibit_void_ptr<hitobj_list_t>
	{
		template <typename T>
			struct tproxy
			{
				T *addr;
				uint8_t mask;
				tproxy(T *a, uint8_t m) :
					addr(a), mask(m)
				{
					assert(m);
					assert(!(m & (m - 1)));
				}
				dxx_explicit_operator_bool operator bool() const
				{
					return *addr & mask;
				}
			};
		typedef tproxy<const uint8_t> cproxy;
		struct proxy : public tproxy<uint8_t>
		{
			proxy(uint8_t *a, uint8_t m) :
				tproxy<uint8_t>(a, m)
			{
			}
			proxy &operator=(bool b)
			{
				if (b)
					*addr |= mask;
				else
					*addr &= ~mask;
				return *this;
			}
			template <typename T>
				void operator=(T) DXX_CXX11_EXPLICIT_DELETE;
		};
		array<uint8_t, (MAX_OBJECTS + 7) / 8> mask;
		proxy operator[](objnum_t i)
		{
			return make_proxy<proxy>(mask, i);
		}
		cproxy operator[](objnum_t i) const
		{
			return make_proxy<cproxy>(mask, i);
		}
		void clear()
		{
			mask.fill(0);
		}
		template <typename T1, typename T2>
		static T1 make_proxy(T2 &mask, objnum_t i)
		{
			typename T2::size_type index = i / 8, bitshift = i % 8;
			if (!(index < mask.size()))
				throw std::out_of_range("index exceeds object range");
			return T1(&mask[index], 1 << bitshift);
		}
	};
	short   parent_type;        // The type of the parent of this object
	objnum_t   parent_num;         // The object's parent's number
	int     parent_signature;   // The object's parent's signature...
	fix64   creation_time;      // Absolute time of creation.
	hitobj_list_t hitobj_list;	// list of all objects persistent weapon has already damaged (useful in case it's in contact with two objects at the same time)
	objnum_t   last_hitobj;        // For persistent weapons (survive object collision), object it most recently hit.
	objnum_t   track_goal;         // Object this object is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
	fix     track_turn_time;
#if defined(DXX_BUILD_DESCENT_II)
	fix64	last_afterburner_time;	//	Time at which this object last created afterburner blobs.
#endif
};

// Same as above but structure Savegames/Multiplayer objects expect
struct laser_info_rw
{
	short   parent_type;        // The type of the parent of this object
	short   parent_num;         // The object's parent's number
	int     parent_signature;   // The object's parent's signature...
	fix     creation_time;      // Absolute time of creation.
	short   last_hitobj;        // For persistent weapons (survive object collision), object it most recently hit.
	short   track_goal;         // Object this object is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
} __pack__;

struct explosion_info : public prohibit_void_ptr<explosion_info>
{
    fix     spawn_time;         // when lifeleft is < this, spawn another
    fix     delete_time;        // when to delete object
    objnum_t   delete_objnum;      // and what object to delete
    objnum_t   attach_parent;      // explosion is attached to this object
    objnum_t   prev_attach;        // previous explosion in attach list
    objnum_t   next_attach;        // next explosion in attach list
};

struct explosion_info_rw
{
    fix     spawn_time;         // when lifeleft is < this, spawn another
    fix     delete_time;        // when to delete object
    short   delete_objnum;      // and what object to delete
    short   attach_parent;      // explosion is attached to this object
    short   prev_attach;        // previous explosion in attach list
    short   next_attach;        // next explosion in attach list
} __pack__;

struct light_info : public prohibit_void_ptr<light_info>
{
    fix     intensity;          // how bright the light is
};

struct light_info_rw
{
    fix     intensity;          // how bright the light is
} __pack__;

struct powerup_info : public prohibit_void_ptr<powerup_info>
{
	int     count;          // how many/much we pick up (vulcan cannon only?)
#if defined(DXX_BUILD_DESCENT_II)
	int     flags;          // spat by player?
	fix64   creation_time;  // Absolute time of creation.
#endif
};

struct powerup_info_rw
{
	int     count;          // how many/much we pick up (vulcan cannon only?)
#if defined(DXX_BUILD_DESCENT_II)
// Same as above but structure Savegames/Multiplayer objects expect
	fix     creation_time;  // Absolute time of creation.
	int     flags;          // spat by player?
#endif
} __pack__;

struct vclip_info : public prohibit_void_ptr<vclip_info>
{
	int     vclip_num;
	fix     frametime;
	sbyte   framenum;
};

struct vclip_info_rw
{
	int     vclip_num;
	fix     frametime;
	sbyte   framenum;
} __pack__;

// structures for different kinds of rendering

struct polyobj_info : public prohibit_void_ptr<polyobj_info>
{
	int     model_num;          // which polygon model
	vms_angvec anim_angles[MAX_SUBMODELS]; // angles for each subobject
	int     subobj_flags;       // specify which subobjs to draw
	int     tmap_override;      // if this is not -1, map all face to this
	int     alt_textures;       // if not -1, use these textures instead
};

struct polyobj_info_rw
{
	int     model_num;          // which polygon model
	vms_angvec anim_angles[MAX_SUBMODELS]; // angles for each subobject
	int     subobj_flags;       // specify which subobjs to draw
	int     tmap_override;      // if this is not -1, map all face to this
	int     alt_textures;       // if not -1, use these textures instead
} __pack__;

struct object;
struct object_rw;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct object {
	int     signature;      // Every object ever has a unique signature...
	ubyte   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
#ifdef WORDS_NEED_ALIGNMENT
	short   pad;
#endif
	objnum_t   next,prev;      // id of next and previous connected object in Objects, -1 = no connection
	ubyte   control_type;   // how this object is controlled
	ubyte   movement_type;  // how this object moves
	ubyte   render_type;    // how this object renders
	ubyte   flags;          // misc flags
	segnum_t   segnum;         // segment number containing object
	objnum_t   attached_obj;   // number of attached fireball object
	vms_vector pos;         // absolute x,y,z coordinate of center of object
	vms_matrix orient;      // orientation of object in world
	fix     size;           // 3d size of object - for collision detection
	fix     shields;        // Starts at maximum, when <0, object dies..
	vms_vector last_pos;    // where object was last frame
	sbyte   contains_type;  // Type of object this object contains (eg, spider contains powerup)
	sbyte   contains_id;    // ID of object this object contains (eg, id = blue type = key)
	sbyte   contains_count; // number of objects of type:id this object contains
	sbyte   matcen_creator; // Materialization center that created this object, high bit set if matcen-created
	fix     lifeleft;       // how long until goes away, or 7fff if immortal
	// -- Removed, MK, 10/16/95, using lifeleft instead: int     lightlevel;

	// movement info, determined by MOVEMENT_TYPE
	union {
		physics_info phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
	} mtype;

	// render info, determined by RENDER_TYPE
	union {
		struct polyobj_info    pobj_info;      // polygon model
		struct vclip_info      vclip_info;     // vclip
	} rtype;

	// control info, determined by CONTROL_TYPE
	union {
		struct laser_info      laser_info;
		struct explosion_info  expl_info;      // NOTE: debris uses this also
		struct light_info      light_info;     // why put this here?  Didn't know what else to do with it.
		struct powerup_info    powerup_info;
		struct ai_static       ai_info;
		struct reactor_static  reactor_info;
	} ctype;

#ifdef WORDS_NEED_ALIGNMENT
	short   pad2;
#endif
};

static inline ubyte get_hostage_id(const struct object *o)
{
	return o->id;
}

static inline ubyte get_player_id(const struct object *o)
{
	return o->id;
}

static inline ubyte get_powerup_id(const struct object *o)
{
	return o->id;
}

static inline ubyte get_reactor_id(const struct object *o)
{
	return o->id;
}

static inline ubyte get_robot_id(const struct object *o)
{
	return o->id;
}

static inline enum weapon_type_t get_weapon_id(const struct object *o)
{
	return (enum weapon_type_t) o->id;
}

static inline void set_hostage_id(struct object *o, ubyte id)
{
	o->id = id;
}

static inline void set_player_id(struct object *o, ubyte id)
{
	o->id = id;
}

static inline void set_powerup_id(struct object *o, ubyte id)
{
	o->id = id;
}

static inline void set_robot_id(struct object *o, ubyte id)
{
	o->id = id;
}

static inline void set_weapon_id(struct object *o, enum weapon_type_t id)
{
	o->id = id;
}

// Same as above but structure Savegames/Multiplayer objects expect
struct object_rw
{
	int     signature;      // Every object ever has a unique signature...
	ubyte   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
#ifdef WORDS_NEED_ALIGNMENT
	short   pad;
#endif
	short   next,prev;      // id of next and previous connected object in Objects, -1 = no connection
	ubyte   control_type;   // how this object is controlled
	ubyte   movement_type;  // how this object moves
	ubyte   render_type;    // how this object renders
	ubyte   flags;          // misc flags
	short   segnum;         // segment number containing object
	short   attached_obj;   // number of attached fireball object
	vms_vector pos;         // absolute x,y,z coordinate of center of object
	vms_matrix orient;      // orientation of object in world
	fix     size;           // 3d size of object - for collision detection
	fix     shields;        // Starts at maximum, when <0, object dies..
	vms_vector last_pos;    // where object was last frame
	sbyte   contains_type;  // Type of object this object contains (eg, spider contains powerup)
	sbyte   contains_id;    // ID of object this object contains (eg, id = blue type = key)
	sbyte   contains_count; // number of objects of type:id this object contains
	sbyte   matcen_creator; // Materialization center that created this object, high bit set if matcen-created
	fix     lifeleft;       // how long until goes away, or 7fff if immortal
	// -- Removed, MK, 10/16/95, using lifeleft instead: int     lightlevel;

	// movement info, determined by MOVEMENT_TYPE
	union {
		physics_info_rw phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
	} __pack__ mtype ;

	// control info, determined by CONTROL_TYPE
	union {
		laser_info_rw   laser_info;
		explosion_info_rw  expl_info;      // NOTE: debris uses this also
		ai_static_rw    ai_info;
		light_info_rw      light_info;     // why put this here?  Didn't know what else to do with it.
		powerup_info_rw powerup_info;
	} __pack__ ctype ;

	// render info, determined by RENDER_TYPE
	union {
		polyobj_info_rw    pobj_info;      // polygon model
		vclip_info_rw      vclip_info;     // vclip
	} __pack__ rtype;

#ifdef WORDS_NEED_ALIGNMENT
	short   pad2;
#endif
} __pack__;
#endif

struct obj_position
{
	vms_vector  pos;        // absolute x,y,z coordinate of center of object
	vms_matrix  orient;     // orientation of object in world
	segnum_t       segnum;     // segment number containing object
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct window_rendered_data
{
#if defined(DXX_BUILD_DESCENT_II)
	fix64   time;
	object  *viewer;
	int     rear_view;
#endif
	std::vector<objnum_t> rendered_robots;
};

extern window_rendered_data Window_rendered_data[MAX_RENDERED_WINDOWS];

typedef array<ubyte, MAX_OBJECT_TYPES> collision_inner_array_t;
typedef array<collision_inner_array_t, MAX_OBJECT_TYPES> collision_outer_array_t;
extern const collision_outer_array_t CollisionResult;
// ie CollisionResult[a][b]==  what happens to a when it collides with b

struct object_array_t : public array<object, MAX_OBJECTS>
{
	int highest;
#define Highest_object_index Objects.highest
	typedef array<object, MAX_OBJECTS> array_t;
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, reference>::type operator[](T n)
		{
			return array_t::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, const_reference>::type operator[](T n) const
		{
			return array_t::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<!tt::is_integral<T>::value, reference>::type operator[](T) const DXX_CXX11_EXPLICIT_DELETE;
	object_array_t() = default;
	object_array_t(const object_array_t &) = delete;
	object_array_t &operator=(const object_array_t &) = delete;
};
extern object_array_t Objects;

DEFINE_VALPTRIDX_SUBTYPE(objptridx, object, objnum_t, Objects);
#endif

/*
 * VARIABLES
 */

extern int Object_next_signature;   // The next signature for the next newly created object

extern int num_objects;

extern int Num_robot_types;

extern object *ConsoleObject;       // pointer to the object that is the player
extern object *Viewer;              // which object we are seeing from
extern object *Dead_player_camera;

extern int Player_is_dead;          // !0 means player is dead!
extern int Player_exploded;
extern int Player_eggs_dropped;
extern int Death_sequence_aborted;
extern objnum_t Player_fired_laser_this_frame;
extern int Drop_afterburner_blob_flag;		//ugly hack

/*
 * FUNCTIONS
 */


// do whatever setup needs to be done
void init_objects();

// returns segment number object is in.  Searches out from object's current
// seg, so this shouldn't be called if the object has "jumped" to a new seg
int obj_get_new_seg(object *obj);

// when an object has moved into a new segment, this function unlinks it
// from its old segment, and links it into the new segment
void obj_relink(vobjptridx_t objnum,segnum_t newsegnum);

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all(void);

// links an object into a segment's list of objects.
// takes object number and segment number
void obj_link(vobjptridx_t objnum,segnum_t segnum);

// unlinks an object from a segment's list of objects
void obj_unlink(vobjptridx_t objnum);

// initialize a new object.  adds to the list for the given segment
// returns the object number
objptridx_t obj_create(enum object_type_t type, ubyte id, segnum_t segnum, const vms_vector *pos,
               const vms_matrix *orient, fix size,
               ubyte ctype, ubyte mtype, ubyte rtype);

// make a copy of an object. returs num of new object
objptridx_t obj_create_copy(objnum_t objnum, vms_vector *new_pos, segnum_t newsegnum);

// remove object from the world
void obj_delete(vobjptridx_t objnum);

// called after load.  Takes number of objects, and objects should be
// compressed
void reset_objects(int n_objs);

// make object array non-sparse
void compress_objects(void);

// Render an object.  Calls one of several routines based on type
void render_object(vobjptridx_t obj);

// Draw a blob-type object, like a fireball
void draw_object_blob(object &obj, bitmap_index bitmap);

// draw an object that is a texture-mapped rod
void draw_object_tmap_rod(vobjptridx_t obj, bitmap_index bitmap, int lighted);

// Deletes all objects that have been marked for death.
void obj_delete_all_that_should_be_dead();

// Toggles whether or not lock-boxes draw.
void object_toggle_lock_targets();

// move all objects for the current frame
void object_move_all();     // moves all objects

// set viewer object to next object in array
void object_goto_next_viewer();

// draw target boxes for nearby robots
void object_render_targets(void);

// move an object for the current frame
void object_move_one(vobjptridx_t obj);

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
segnum_t find_object_seg(object * obj );

// go through all objects and make sure they have the correct segment
// numbers used when debugging is on
void fix_object_segs();

// Drops objects contained in objp.
objnum_t object_create_egg(object *objp);

// Interface to object_create_egg, puts count objects of type type, id
// = id in objp and then drops them.
objnum_t call_object_create_egg(object *objp, int count, int type, int id);

extern void dead_player_end(void);

// Extract information from an object (objp->orient, objp->pos,
// objp->segnum), stuff in a shortpos structure.  See typedef
// shortpos.
extern void create_shortpos(shortpos *spp, object *objp, int swap_bytes);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->segnum
void extract_shortpos(vobjptridx_t objp, shortpos *spp, int swap_bytes);

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
void create_quaternionpos(quaternionpos * qpp, object * objp, int swap_bytes);
void extract_quaternionpos(vobjptridx_t objp, quaternionpos *qpp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all);

// Returns a new, unique signature for a new object
int obj_get_signature();

// returns the number of a free object, updating Highest_object_index.
// Generally, obj_create() should be called to get an object, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
objptridx_t obj_allocate();

// frees up an object.  Generally, obj_delete() should be called to
// get rid of an object.  This function deallocates the object entry
// after the object has been unlinked
void obj_free(objnum_t objnum);

// after calling init_object(), the network code has grabbed specific
// object slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void special_reset_objects(void);

// attaches an object, such as a fireball, to another object, such as
// a robot
void obj_attach(objptridx_t parent,objptridx_t sub);

void create_small_fireball_on_object(vobjptridx_t objp, fix size_scale, int sound_flag);
void dead_player_frame(void);

#if defined(DXX_BUILD_DESCENT_II)
// returns object number
objnum_t drop_marker_object(vms_vector *pos, segnum_t segnum, vms_matrix *orient, int marker_num);

extern void wake_up_rendered_objects(objptridx_t gmissp, int window_num);

void fuelcen_check_for_goal (segment *);
#endif
struct object *obj_find_first_of_type (int);

extern void object_rw_swap(struct object_rw *obj_rw, int swap);
void reset_player_object(void);

#endif

#endif
