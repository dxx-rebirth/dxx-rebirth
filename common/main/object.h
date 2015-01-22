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

#pragma once

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
#include "fwdobject.h"

// Object types
enum object_type_t : int
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

/*
 * STRUCTURES
 */

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
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

// information for physics sim for an object
struct physics_info : prohibit_void_ptr<physics_info>
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

struct laser_info : prohibit_void_ptr<laser_info>
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

struct explosion_info : prohibit_void_ptr<explosion_info>
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

struct light_info : prohibit_void_ptr<light_info>
{
    fix     intensity;          // how bright the light is
};

struct light_info_rw
{
    fix     intensity;          // how bright the light is
} __pack__;

struct powerup_info : prohibit_void_ptr<powerup_info>
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

struct vclip_info : prohibit_void_ptr<vclip_info>
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

struct polyobj_info : prohibit_void_ptr<polyobj_info>
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

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct object {
	int     signature;      // Every object ever has a unique signature...
	ubyte   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
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
};

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
struct object_array_t : array<object, MAX_OBJECTS>
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

DEFINE_VALPTRIDX_SUBTYPE(obj, object, objnum_t, Objects);

static inline ubyte get_hostage_id(const vcobjptr_t o)
{
	return o->id;
}

static inline ubyte get_player_id(const vcobjptr_t o)
{
	return o->id;
}

static inline ubyte get_powerup_id(const vcobjptr_t o)
{
	return o->id;
}

static inline ubyte get_reactor_id(const vcobjptr_t o)
{
	return o->id;
}

static inline ubyte get_robot_id(const vcobjptr_t o)
{
	return o->id;
}

static inline weapon_type_t get_weapon_id(const vcobjptr_t o)
{
	return static_cast<weapon_type_t>(o->id);
}

static inline void set_hostage_id(const vobjptr_t o, ubyte id)
{
	o->id = id;
}

static inline void set_player_id(const vobjptr_t o, ubyte id)
{
	o->id = id;
}

static inline void set_powerup_id(const vobjptr_t o, ubyte id)
{
	o->id = id;
}

static inline void set_robot_id(const vobjptr_t o, ubyte id)
{
	o->id = id;
}

static inline void set_weapon_id(const vobjptr_t o, weapon_type_t id)
{
	o->id = id;
}
#endif

#endif
