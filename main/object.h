/* $Id: object.h,v 1.5 2003-10-04 03:14:47 btb Exp $ */
/*
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
 * Old Log:
 * Revision 1.6  1995/09/20  14:24:45  allender
 * swap bytes on extractshortpos
 *
 * Revision 1.5  1995/09/14  14:11:42  allender
 * fix_object_segs returns void
 *
 * Revision 1.4  1995/08/12  12:02:44  allender
 * added flag to create_shortpos
 *
 * Revision 1.3  1995/07/12  12:55:08  allender
 * move structures back to original form as found on PC because
 * of network play
 *
 * Revision 1.2  1995/06/19  07:55:06  allender
 * rearranged structure members for possible better alignment
 *
 * Revision 1.1  1995/05/16  16:00:40  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/31  12:24:10  john
 * I had changed alt_textures from a pointer to a byte. This hosed old
 * saved games, so I restored it to an int.
 *
 * Revision 2.0  1995/02/27  11:26:47  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.122  1995/02/22  12:35:53  allender
 * remove anonymous unions
 *
 * Revision 1.121  1995/02/06  20:43:25  rob
 * Extern'ed Dead_player_camera so it can be reset by multi.c
 *
 * Revision 1.120  1995/02/01  16:34:07  john
 * Linted.
 *
 * Revision 1.119  1995/01/29  13:46:42  mike
 * adapt to new create_small_fireball_on_object prototype.
 *
 * Revision 1.118  1995/01/26  22:11:27  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.117  1995/01/24  12:09:29  mike
 * Boost MAX_OBJECTS from 250 to 350.
 *
 * Revision 1.116  1995/01/13  19:39:51  rob
 * Removed outdated remote_info structure.  (looking for cause of bugs
 *
 * Revision 1.115  1995/01/12  12:09:38  yuan
 * Added coop object capability.
 *
 * Revision 1.114  1994/12/15  13:04:20  mike
 * Replace Players[Player_num].time_total references with GameTime.
 *
 * Revision 1.113  1994/12/12  17:18:09  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.112  1994/12/09  14:58:42  matt
 * Added system to attach a fireball to another object for rendering purposes,
 * so the fireball always renders on top of (after) the object.
 *
 * Revision 1.111  1994/12/08  12:35:35  matt
 * Added new object allocation & deallocation functions so other code
 * could stop messing around with internal object data structures.
 *
 * Revision 1.110  1994/11/21  17:30:21  matt
 * Increased max number of objects
 *
 * Revision 1.109  1994/11/18  23:41:52  john
 * Changed some shorts to ints.
 *
 * Revision 1.108  1994/11/10  14:02:45  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.107  1994/11/08  12:19:27  mike
 * Small explosions on objects.
 *
 * Revision 1.106  1994/10/25  10:51:17  matt
 * Vulcan cannon powerups now contain ammo count
 *
 * Revision 1.105  1994/10/21  12:19:41  matt
 * Clear transient objects when saving (& loading) games
 *
 * Revision 1.104  1994/10/21  11:25:04  mike
 * Add IMMORTAL_TIME.
 *
 * Revision 1.103  1994/10/17  21:34:54  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.102  1994/10/14  18:12:28  mike
 * Make egg dropping return object number.
 *
 * Revision 1.101  1994/10/12  21:07:19  matt
 * Killed unused field in object structure
 *
 * Revision 1.100  1994/10/12  10:38:24  mike
 * Add field OF_SILENT to obj->flags.
 *
 * Revision 1.99  1994/10/11  20:35:48  matt
 * Clear "transient" objects (weapons,explosions,etc.) when starting a level
 *
 * Revision 1.98  1994/10/03  20:56:13  rob
 * Added velocity to shortpos strucutre.
 *
 * Revision 1.97  1994/09/30  18:24:00  rob
 * Added new control type CT_REMOTE for remote controlled objects.
 * Also added a union struct 'remote_info' for this type.
 *
 * Revision 1.96  1994/09/28  09:23:05  mike
 * Prototype Object_type_names.
 *
 * Revision 1.95  1994/09/25  23:32:37  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been
 * incorporated into the object array.  Also, timeleft is now a property
 * of all objects, and the object structure has been otherwise cleaned up.
 *
 * Revision 1.94  1994/09/25  15:45:28  matt
 * Added OBJ_LIGHT, a type of object that casts light
 * Added generalized lifeleft, and moved it to local_object
 *
 * Revision 1.93  1994/09/24  17:41:19  mike
 * Add stuff to Local_object structure for materialization centers.
 *
 * Revision 1.92  1994/09/24  13:16:50  matt
 * Added (hacked in, really) support for overriding the bitmaps used on to
 * texture map a polygon object, and using a new bitmap for all the faces.
 *
 * Revision 1.91  1994/09/22  19:02:14  mike
 * Prototype functions extract_shortpos and create_shortpos which reside in
 * gameseg.c, but are prototyped here to prevent circular dependencies.
 *
 * Revision 1.90  1994/09/15  21:47:14  mike
 * Prototype dead_player_end().
 *
 * Revision 1.89  1994/09/15  16:34:47  mike
 * Add danger_laser_num and danger_laser_signature to object_local to
 * enable robots to efficiently (too efficiently!) avoid player fire.
 *
 * Revision 1.88  1994/09/11  22:46:19  mike
 * Death_sequence_aborted prototyped.
 *
 * Revision 1.87  1994/09/09  20:04:30  mike
 * Add vclips for weapons.
 *
 * Revision 1.86  1994/09/09  14:20:54  matt
 * Added flag that says object uses thrust
 *
 * Revision 1.85  1994/09/08  14:51:32  mike
 * Make a crucial name change to a field of local_object struct.
 *
 * Revision 1.84  1994/09/07  19:16:45  mike
 * Homing missile.
 *
 * Revision 1.83  1994/09/06  17:05:43  matt
 * Added new type for dead player
 *
 * Revision 1.82  1994/09/02  11:56:09  mike
 * Add persistency (PF_PERSISTENT) to physics_info.
 *
 * Revision 1.81  1994/08/28  19:10:28  mike
 * Add Player_is_dead.
 *
 * Revision 1.80  1994/08/18  15:11:44  mike
 * powerup stuff.
 *
 * Revision 1.79  1994/08/15  15:24:54  john
 * Made players know who killed them; Disabled cheat menu
 * during net player; fixed bug with not being able to turn
 * of invulnerability; Made going into edit/starting new leve
 * l drop you out of a net game; made death dialog box.
 *
 * Revision 1.78  1994/08/14  23:15:12  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.77  1994/08/13  14:58:27  matt
 * Finished adding support for miscellaneous objects
 *
 * Revision 1.76  1994/08/09  16:04:13  john
 * Added network players to editor.
 *
 * Revision 1.75  1994/08/03  21:06:19  matt
 * Added prototype for fix_object_segs(), and renamed now-unused spawn_pos
 *
 * Revision 1.74  1994/08/02  12:30:27  matt
 * Added support for spinning objects
 *
 * Revision 1.73  1994/07/27  20:53:25  matt
 * Added rotational drag & thrust, so turning now has momemtum like moving
 *
 * Revision 1.72  1994/07/27  19:44:21  mike
 * Objects containing objects.
 *
 * Revision 1.71  1994/07/22  20:43:29  matt
 * Fixed flares, by adding a physics flag that makes them stick to walls.
 *
 * Revision 1.70  1994/07/21  12:42:10  mike
 * Prototype new find_object_seg and update_object_seg.
 *
 * Revision 1.69  1994/07/19  15:26:39  mike
 * New ai_static structure.
 *
 * Revision 1.68  1994/07/13  00:15:06  matt
 * Moved all (or nearly all) of the values that affect player movement to
 * bitmaps.tbl
 *
 * Revision 1.67  1994/07/12  12:40:12  matt
 * Revamped physics system
 *
 * Revision 1.66  1994/07/06  15:26:23  yuan
 * Added chase mode.
 *
 *
 *
 */

#ifndef _OBJECT_H
#define _OBJECT_H

#include "pstypes.h"
#include "vecmat.h"
#include "segment.h"
#include "gameseg.h"
#include "aistruct.h"
#include "gr.h"
#include "piggy.h"

/*
 * CONSTANTS
 */

#define MAX_OBJECTS     350 // increased on 01/24/95 for multiplayer. --MK;  total number of objects in world

// Object types
#define OBJ_NONE        255 // unused object
#define OBJ_WALL        0   // A wall... not really an object, but used for collisions
#define OBJ_FIREBALL    1   // a fireball, part of an explosion
#define OBJ_ROBOT       2   // an evil enemy
#define OBJ_HOSTAGE     3   // a hostage you need to rescue
#define OBJ_PLAYER      4   // the player on the console
#define OBJ_WEAPON      5   // a laser, missile, etc
#define OBJ_CAMERA      6   // a camera to slew around with
#define OBJ_POWERUP     7   // a powerup you can pick up
#define OBJ_DEBRIS      8   // a piece of robot
#define OBJ_CNTRLCEN    9   // the control center
#define OBJ_FLARE       10  // a flare
#define OBJ_CLUTTER     11  // misc objects
#define OBJ_GHOST       12  // what the player turns into when dead
#define OBJ_LIGHT       13  // a light source, & not much else
#define OBJ_COOP        14  // a cooperative player object.
#define OBJ_MARKER      15  // a map marker

// WARNING!! If you add a type here, add its name to Object_type_names
// in object.c
#define MAX_OBJECT_TYPES    16

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
#define OF_HARMLESS         32  // this object does no damage.  Added to make quad lasers do 1.5 damage as normal lasers.
#define OF_PLAYER_DROPPED   64  // this object was dropped by the player...

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
#define PF_BOUNCED_ONCE     0x80    // Weapon has bounced once.
#define PF_FREE_SPINNING    0x100   // Drag does not apply to rotation of this object
#define PF_BOUNCES_TWICE    0x200   // This weapon bounces twice, then dies

#define IMMORTAL_TIME   0x3fffffff  // Time assigned to immortal objects, about 32768 seconds, or about 9 hours.
#define ONE_FRAME_TIME  0x3ffffffe  // Objects with this lifeleft will live for exactly one frame

extern char Object_type_names[MAX_OBJECT_TYPES][9];

// List of objects rendered last frame in order.  Created at render
// time, used by homing missiles in laser.c
#define MAX_RENDERED_OBJECTS    50

/*
 * STRUCTURES
 */

// A compressed form for sending crucial data about via slow devices,
// such as modems and buggies.
typedef struct shortpos {
	sbyte   bytemat[9];
	short   xo,yo,zo;
	short   segment;
	short   velx, vely, velz;
} __pack__ shortpos;

// This is specific to the shortpos extraction routines in gameseg.c.
#define RELPOS_PRECISION    10
#define MATRIX_PRECISION    9
#define MATRIX_MAX          0x7f    // This is based on MATRIX_PRECISION, 9 => 0x7f

// information for physics sim for an object
typedef struct physics_info {
	vms_vector  velocity;   // velocity vector of this object
	vms_vector  thrust;     // constant force applied to this object
	fix         mass;       // the mass of this object
	fix         drag;       // how fast this slows down
	fix         brakes;     // how much brakes applied
	vms_vector  rotvel;     // rotational velecity (angles)
	vms_vector  rotthrust;  // rotational acceleration
	fixang      turnroll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
} __pack__ physics_info;

// stuctures for different kinds of simulation

typedef struct laser_info {
	short   parent_type;        // The type of the parent of this object
	short   parent_num;         // The object's parent's number
	int     parent_signature;   // The object's parent's signature...
	fix     creation_time;      // Absolute time of creation.
	short   last_hitobj;        // For persistent weapons (survive object collision), object it most recently hit.
	short   track_goal;         // Object this object is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
} __pack__ laser_info;

typedef struct explosion_info {
    fix     spawn_time;         // when lifeleft is < this, spawn another
    fix     delete_time;        // when to delete object
    short   delete_objnum;      // and what object to delete
    short   attach_parent;      // explosion is attached to this object
    short   prev_attach;        // previous explosion in attach list
    short   next_attach;        // next explosion in attach list
} __pack__ explosion_info;

typedef struct light_info {
    fix     intensity;          // how bright the light is
} __pack__ light_info;

#define PF_SPAT_BY_PLAYER   1   //this powerup was spat by the player

typedef struct powerup_info {
	int     count;          // how many/much we pick up (vulcan cannon only?)
	fix     creation_time;  // Absolute time of creation.
	int     flags;          // spat by player?
} __pack__ powerup_info;

typedef struct vclip_info {
	int     vclip_num;
	fix     frametime;
	sbyte   framenum;
} __pack__ vclip_info;

// structures for different kinds of rendering

typedef struct polyobj_info {
	int     model_num;          // which polygon model
	vms_angvec anim_angles[MAX_SUBMODELS]; // angles for each subobject
	int     subobj_flags;       // specify which subobjs to draw
	int     tmap_override;      // if this is not -1, map all face to this
	int     alt_textures;       // if not -1, use these textures instead
} __pack__ polyobj_info;

typedef struct object {
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
		physics_info phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
	} __pack__ mtype;

	// control info, determined by CONTROL_TYPE
	union {
		laser_info      laser_info;
		explosion_info  expl_info;      // NOTE: debris uses this also
		ai_static       ai_info;
		light_info      light_info;     // why put this here?  Didn't know what else to do with it.
		powerup_info    powerup_info;
	} __pack__ ctype;

	// render info, determined by RENDER_TYPE
	union {
		polyobj_info    pobj_info;      // polygon model
		vclip_info      vclip_info;     // vclip
	} __pack__ rtype;

#ifdef WORDS_NEED_ALIGNMENT
	short   pad2;
#endif
} __pack__ object;

typedef struct obj_position {
	vms_vector  pos;        // absolute x,y,z coordinate of center of object
	vms_matrix  orient;     // orientation of object in world
	short       segnum;     // segment number containing object
} obj_position;

typedef struct {
	int     frame;
	object  *viewer;
	int     rear_view;
	int     user;
	int     num_objects;
	short   rendered_objects[MAX_RENDERED_OBJECTS];
} window_rendered_data;

#define MAX_RENDERED_WINDOWS    3

extern window_rendered_data Window_rendered_data[MAX_RENDERED_WINDOWS];

/*
 * VARIABLES
 */

extern int Object_next_signature;   // The next signature for the next newly created object

extern ubyte CollisionResult[MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
// ie CollisionResult[a][b]==  what happens to a when it collides with b

extern object Objects[];
extern int Highest_object_index;    // highest objnum

extern char *robot_names[];         // name of each robot

extern int Num_robot_types;

extern object *ConsoleObject;       // pointer to the object that is the player
extern object *Viewer;              // which object we are seeing from
extern object *Dead_player_camera;

extern object Follow;
extern int Player_is_dead;          // !0 means player is dead!
extern int Player_exploded;
extern int Death_sequence_aborted;
extern int Player_fired_laser_this_frame;

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
void obj_relink(int objnum,int newsegnum);

// move an object from one segment to another. unlinks & relinks
void obj_set_new_seg(int objnum,int newsegnum);

// links an object into a segment's list of objects.
// takes object number and segment number
void obj_link(int objnum,int segnum);

// unlinks an object from a segment's list of objects
void obj_unlink(int objnum);

// initialize a new object.  adds to the list for the given segment
// returns the object number
int obj_create(ubyte type, ubyte id, int segnum, vms_vector *pos,
               vms_matrix *orient, fix size,
               ubyte ctype, ubyte mtype, ubyte rtype);

// make a copy of an object. returs num of new object
int obj_create_copy(int objnum, vms_vector *new_pos, int newsegnum);

// remove object from the world
void obj_delete(int objnum);

// called after load.  Takes number of objects, and objects should be
// compressed
void reset_objects(int n_objs);

// make object array non-sparse
void compress_objects(void);

// Render an object.  Calls one of several routines based on type
void render_object(object *obj);

// Draw a blob-type object, like a fireball
void draw_object_blob(object *obj, bitmap_index bitmap);

// draw an object that is a texture-mapped rod
void draw_object_tmap_rod(object *obj, bitmap_index bitmap, int lighted);

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
void object_move_one(object * obj);

// make object0 the player, setting all relevant fields
void init_player_object();

// check if object is in object->segnum.  if not, check the adjacent
// segs.  if not any of these, returns false, else sets obj->segnum &
// returns true callers should really use find_vector_intersection()
// Note: this function is in gameseg.c
extern int update_object_seg(struct object *obj);


// Finds what segment *obj is in, returns segment number.  If not in
// any segment, returns -1.  Note: This function is defined in
// gameseg.h, but object.h depends on gameseg.h, and object.h is where
// object is defined...get it?
extern int find_object_seg(object * obj );

// go through all objects and make sure they have the correct segment
// numbers used when debugging is on
void fix_object_segs();

// Drops objects contained in objp.
int object_create_egg(object *objp);

// Interface to object_create_egg, puts count objects of type type, id
// = id in objp and then drops them.
int call_object_create_egg(object *objp, int count, int type, int id);

extern void dead_player_end(void);

// Extract information from an object (objp->orient, objp->pos,
// objp->segnum), stuff in a shortpos structure.  See typedef
// shortpos.
extern void create_shortpos(shortpos *spp, object *objp, int swap_bytes);

// Extract information from a shortpos, stuff in objp->orient
// (matrix), objp->pos, objp->segnum
extern void extract_shortpos(object *objp, shortpos *spp, int swap_bytes);

// delete objects, such as weapons & explosions, that shouldn't stay
// between levels if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all);

// returns the number of a free object, updating Highest_object_index.
// Generally, obj_create() should be called to get an object, since it
// fills in important fields and does the linking.  returns -1 if no
// free objects
int obj_allocate(void);

// frees up an object.  Generally, obj_delete() should be called to
// get rid of an object.  This function deallocates the object entry
// after the object has been unlinked
void obj_free(int objnum);

// after calling init_object(), the network code has grabbed specific
// object slots without allocating them.  Go though the objects &
// build the free list, then set the apporpriate globals Don't call
// this function if you don't know what you're doing.
void special_reset_objects(void);

// attaches an object, such as a fireball, to another object, such as
// a robot
void obj_attach(object *parent,object *sub);

extern void create_small_fireball_on_object(object *objp, fix size_scale, int sound_flag);

// returns object number
int drop_marker_object(vms_vector *pos, int segnum, vms_matrix *orient, int marker_num);

extern void wake_up_rendered_objects(object *gmissp, int window_num);

extern void AdjustMineSpawn();

void reset_player_object(void);

#endif
