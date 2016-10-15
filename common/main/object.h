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

#include "dsx-ns.h"
#ifdef dsx

#include "pstypes.h"
#include "vecmat.h"
#include "aistruct.h"
#include "polyobj.h"
#include "laser.h"

#ifdef __cplusplus
#include <bitset>
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
#include "fwd-object.h"
#include "weapon.h"
#include "powerup.h"
#include "poison.h"
#include "player-flags.h"

namespace dcx {

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
	OBJ_MARKER	= 15,  // a map marker
};

}

namespace dsx {

/*
 * STRUCTURES
 */

struct reactor_static {
	/* Location of the gun on the reactor object */
	array<vms_vector, MAX_CONTROLCEN_GUNS>	gun_pos,
	/* Orientation of the gun on the reactor object */
		gun_dir;
};

struct player_info
{
	fix     energy;                 // Amount of energy remaining.
	fix     homing_object_dist;     // Distance of nearest homing object.
#if defined(DXX_BUILD_DESCENT_II)
	fix Omega_charge;
#endif
	player_flags powerup_flags;
	objnum_t   killer_objnum;          // Who killed me.... (-1 if no one)
	uint16_t vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_I)
	using primary_weapon_flag_type = uint8_t;
#elif defined(DXX_BUILD_DESCENT_II)
	using primary_weapon_flag_type = uint16_t;
#endif
	primary_weapon_flag_type primary_weapon_flags;
	bool FakingInvul;
	bool lavafall_hiss_playing;
	uint8_t missile_gun;
	player_selected_weapon<primary_weapon_index_t> Primary_weapon;
	player_selected_weapon<secondary_weapon_index_t> Secondary_weapon;
	stored_laser_level laser_level;
	array<uint8_t, MAX_SECONDARY_WEAPONS>  secondary_ammo; // How much ammo of each type.
#if defined(DXX_BUILD_DESCENT_II)
	array<uint8_t, SUPER_WEAPON> Primary_last_was_super;
	array<uint8_t, SUPER_WEAPON> Secondary_last_was_super;
#endif
	union {
		struct {
			int score;				// Current score.
		} mission;
	};
	fix64   cloak_time;             // Time cloaked
	fix64   invulnerable_time;      // Time invulnerable
	fix64 Next_flare_fire_time;
	fix64 Next_laser_fire_time;
	fix64 Next_missile_fire_time;
	fix64 Last_bumped_local_player;
};

}

namespace dcx {

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

struct laser_parent
{
	short   parent_type;        // The type of the parent of this object
	objnum_t   parent_num;         // The object's parent's number
	object_signature_t     parent_signature;   // The object's parent's signature...
	constexpr laser_parent() :
		parent_type{}, parent_num{}, parent_signature(0)
	{
	}
};

}

namespace dsx {

struct laser_info : prohibit_void_ptr<laser_info>, laser_parent
{
	struct hitobj_list_t : public prohibit_void_ptr<hitobj_list_t>
	{
		typedef std::bitset<MAX_OBJECTS> mask_type;
		mask_type mask;
		struct proxy
		{
			mask_type &mask;
			std::size_t i;
			proxy(mask_type &m, std::size_t s) :
				mask(m), i(s)
			{
			}
			explicit operator bool() const
			{
				return mask.test(i);
			}
			proxy &operator=(bool b)
			{
				mask.set(i, b);
				return *this;
			}
			template <typename T>
				void operator=(T) = delete;
		};
		proxy operator[](objnum_t i)
		{
			return proxy(mask, i);
		}
		bool operator[](objnum_t i) const
		{
			return mask.test(i);
		}
		void clear()
		{
			mask.reset();
		}
	};
	fix64   creation_time;      // Absolute time of creation.
	hitobj_list_t hitobj_list;	// list of all objects persistent weapon has already damaged (useful in case it's in contact with two objects at the same time)
	objnum_t   last_hitobj;        // For persistent weapons (survive object collision), object it most recently hit.
	objnum_t   track_goal;         // Object this object is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
#if defined(DXX_BUILD_DESCENT_II)
	fix64	last_afterburner_time;	//	Time at which this object last created afterburner blobs.
#endif
	constexpr laser_info() :
		creation_time{}, last_hitobj{}, track_goal{}, multiplier{}
#if defined(DXX_BUILD_DESCENT_II)
		, last_afterburner_time{}
#endif
	{
	}
};

}

namespace dcx {

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
	int     flags;          // spat by player?
	fix64   creation_time;  // Absolute time of creation.
};

}

namespace dsx {

struct powerup_info_rw
{
	int     count;          // how many/much we pick up (vulcan cannon only?)
#if defined(DXX_BUILD_DESCENT_II)
// Same as above but structure Savegames/Multiplayer objects expect
	fix     creation_time;  // Absolute time of creation.
	int     flags;          // spat by player?
#endif
} __pack__;

}

namespace dcx {

struct vclip_info : prohibit_void_ptr<vclip_info>
{
	int     vclip_num;
	fix     frametime;
	uint8_t framenum;
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
	array<vms_angvec, MAX_SUBMODELS> anim_angles; // angles for each subobject
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

struct object_base
{
	object_signature_t signature;
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
	union movement_info {
		physics_info phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
		constexpr movement_info() :
			phys_info{}
		{
			static_assert(sizeof(phys_info) == sizeof(*this), "insufficient initialization");
		}
	} mtype;

	// render info, determined by RENDER_TYPE
	union render_info {
		struct polyobj_info    pobj_info;      // polygon model
		struct vclip_info      vclip_info;     // vclip
		constexpr render_info() :
			pobj_info{}
		{
			static_assert(sizeof(pobj_info) == sizeof(*this), "insufficient initialization");
		}
	} rtype;
};

}

namespace dsx {

struct object : public ::dcx::object_base
{
	// control info, determined by CONTROL_TYPE
	union control_info {
		constexpr control_info() :
			ai_info{}
		{
			static_assert(sizeof(ai_info) == sizeof(*this), "insufficient initialization");
		}
		struct laser_info      laser_info;
		struct explosion_info  expl_info;      // NOTE: debris uses this also
		struct light_info      light_info;     // why put this here?  Didn't know what else to do with it.
		struct powerup_info    powerup_info;
		struct ai_static       ai_info;
		struct reactor_static  reactor_info;
		struct player_info     player_info;
	} ctype;
};

}

namespace dcx {

// Same as above but structure Savegames/Multiplayer objects expect
struct object_rw
{
	int     signature;      // Every object ever has a unique signature...
	ubyte   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
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
} __pack__;

struct obj_position
{
	vms_vector  pos;        // absolute x,y,z coordinate of center of object
	vms_matrix  orient;     // orientation of object in world
	segnum_t       segnum;     // segment number containing object
};

}

#define Highest_object_index (Objects.get_count() - 1)

namespace dsx {

DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(object, obj);

static inline powerup_type_t get_powerup_id(const object_base &o)
{
	return static_cast<powerup_type_t>(o.id);
}

static inline weapon_id_type get_weapon_id(const object_base &o)
{
	return static_cast<weapon_id_type>(o.id);
}

#if defined(DXX_BUILD_DESCENT_II)
static inline uint8_t get_marker_id(const object_base &o)
{
	return o.id;
}
#endif

void set_powerup_id(object_base &o, powerup_type_t id);

static inline void set_weapon_id(object_base &o, weapon_id_type id)
{
	o.id = static_cast<uint8_t>(id);
}

}

namespace dcx {

static inline uint8_t get_hostage_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_player_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_reactor_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_fireball_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_robot_id(const object_base &o)
{
	return o.id;
}

static inline void set_hostage_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

static inline void set_player_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

static inline void set_reactor_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

static inline void set_robot_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

void check_warn_object_type(const object_base &, object_type_t, const char *file, unsigned line);
#define get_hostage_id(O)	(check_warn_object_type(O, OBJ_HOSTAGE, __FILE__, __LINE__), get_hostage_id(O))
#define get_player_id(O)	(check_warn_object_type(O, OBJ_PLAYER, __FILE__, __LINE__), get_player_id(O))
#define get_powerup_id(O)	(check_warn_object_type(O, OBJ_POWERUP, __FILE__, __LINE__), get_powerup_id(O))
#define get_reactor_id(O)	(check_warn_object_type(O, OBJ_CNTRLCEN, __FILE__, __LINE__), get_reactor_id(O))
#define get_ghost_id(O)	(check_warn_object_type(O, OBJ_GHOST, __FILE__, __LINE__), (get_player_id)(O))
#define get_fireball_id(O)	(check_warn_object_type(O, OBJ_FIREBALL, __FILE__, __LINE__), get_fireball_id(O))
#define get_robot_id(O)	(check_warn_object_type(O, OBJ_ROBOT, __FILE__, __LINE__), get_robot_id(O))
#define get_weapon_id(O)	(check_warn_object_type(O, OBJ_WEAPON, __FILE__, __LINE__), get_weapon_id(O))
#if defined(DXX_BUILD_DESCENT_II)
#define get_marker_id(O)	(check_warn_object_type(O, OBJ_MARKER, __FILE__, __LINE__), get_marker_id(O))
#endif
#define set_hostage_id(O,I)	(check_warn_object_type(O, OBJ_HOSTAGE, __FILE__, __LINE__), set_hostage_id(O, I))
#define set_player_id(O,I)	(check_warn_object_type(O, OBJ_PLAYER, __FILE__, __LINE__), set_player_id(O, I))
#define set_reactor_id(O,I)	(check_warn_object_type(O, OBJ_CNTRLCEN, __FILE__, __LINE__), set_reactor_id(O, I))
#define set_robot_id(O,I)	(check_warn_object_type(O, OBJ_ROBOT, __FILE__, __LINE__), set_robot_id(O, I))
#define set_weapon_id(O,I)	(check_warn_object_type(O, OBJ_WEAPON, __FILE__, __LINE__), set_weapon_id(O, I))
#ifdef DXX_CONSTANT_TRUE
#define check_warn_object_type(O,T,F,L)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		const object_base &dxx_check_warn_o = (O);	\
		const auto dxx_check_warn_actual_type = dxx_check_warn_o.type;	\
		const auto dxx_check_warn_expected_type = (T);	\
		/* If the type is always right, omit the runtime check. */	\
		DXX_CONSTANT_TRUE(dxx_check_warn_actual_type == dxx_check_warn_expected_type) || (	\
			/* If the type is always wrong, force a compile-time error. */	\
			DXX_CONSTANT_TRUE(dxx_check_warn_actual_type != dxx_check_warn_expected_type)	\
			? DXX_ALWAYS_ERROR_FUNCTION(dxx_error_object_type_mismatch, "object type mismatch")	\
			: (	\
				check_warn_object_type(dxx_check_warn_o, dxx_check_warn_expected_type, F, L)	\
			)	\
		, 0);	\
	} DXX_END_COMPOUND_STATEMENT )
#endif

}
#endif

#endif
