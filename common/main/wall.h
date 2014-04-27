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

#ifndef _WALL_H
#define _WALL_H

#include "segment.h"

struct object;
struct objptridx_t;

#ifdef __cplusplus

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_WALLS					175u	// Maximum number of walls
#define MAX_WALL_ANIMS			30		// Maximum different types of doors
#define MAX_DOORS					50		// Maximum number of open doors
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_WALLS               254u // Maximum number of walls
#define MAX_WALL_ANIMS          60  // Maximum different types of doors
#define MAX_DOORS               90  // Maximum number of open doors
#endif

// Various wall types.
#define WALL_NORMAL             0   // Normal wall
#define WALL_BLASTABLE          1   // Removable (by shooting) wall
#define WALL_DOOR               2   // Door
#define WALL_ILLUSION           3   // Wall that appears to be there, but you can fly thru
#define WALL_OPEN               4   // Just an open side. (Trigger)
#define WALL_CLOSED             5   // Wall.  Used for transparent walls.
#if defined(DXX_BUILD_DESCENT_II)
#define WALL_OVERLAY            6   // Goes over an actual solid side.  For triggers
#define WALL_CLOAKED            7   // Can see it, and see through it
#endif

// Various wall flags.
#define WALL_BLASTED            1   // Blasted out wall.
#define WALL_DOOR_OPENED        2   // Open door.
#define WALL_DOOR_LOCKED        8   // Door is locked.
#define WALL_DOOR_AUTO          16  // Door automatically closes after time.
#define WALL_ILLUSION_OFF       32  // Illusionary wall is shut off.
#if defined(DXX_BUILD_DESCENT_II)
#define WALL_WALL_SWITCH        64  // This wall is openable by a wall switch.
#define WALL_BUDDY_PROOF        128 // Buddy assumes he cannot get through this wall.
#endif

// Wall states
#define WALL_DOOR_CLOSED        0       // Door is closed
#define WALL_DOOR_OPENING       1       // Door is opening.
#define WALL_DOOR_WAITING       2       // Waiting to close
#define WALL_DOOR_CLOSING       3       // Door is closing
#if defined(DXX_BUILD_DESCENT_II)
#define WALL_DOOR_OPEN          4       // Door is open, and staying open
#define WALL_DOOR_CLOAKING      5       // Wall is going from closed -> open
#define WALL_DOOR_DECLOAKING    6       // Wall is going from open -> closed
#endif

//note: a door is considered opened (i.e., it has WALL_OPENED set) when it
//is more than half way open.  Thus, it can have any of OPENING, CLOSING,
//or WAITING bits set when OPENED is set.

#define KEY_NONE                1
#define KEY_BLUE                2
#define KEY_RED                 4
#define KEY_GOLD                8

#define WALL_HPS                100*F1_0    // Normal wall's hp
#define WALL_DOOR_INTERVAL      5*F1_0      // How many seconds a door is open

#define DOOR_OPEN_TIME          i2f(2)      // How long takes to open
#define DOOR_WAIT_TIME          i2f(5)      // How long before auto door closes

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_CLIP_FRAMES			20
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_CLIP_FRAMES         50
#endif

// WALL_IS_DOORWAY flags.
#define WID_FLY_FLAG            1
#define WID_RENDER_FLAG         2
#define WID_RENDPAST_FLAG       4
#define WID_EXTERNAL_FLAG       8
#if defined(DXX_BUILD_DESCENT_II)
#define WID_CLOAKED_FLAG        16
#endif

//  WALL_IS_DOORWAY return values          F/R/RP
#define WID_WALL                    2   // 0/1/0        wall
#define WID_TRANSPARENT_WALL        6   // 0/1/1        transparent wall
#define WID_ILLUSORY_WALL           3   // 1/1/0        illusory wall
#define WID_TRANSILLUSORY_WALL      7   // 1/1/1        transparent illusory wall
#define WID_NO_WALL                 5   //  1/0/1       no wall, can fly through
#define WID_EXTERNAL                8   // 0/0/0/1  don't see it, dont fly through it

#define MAX_STUCK_OBJECTS   32

struct stuckobj
{
	short   objnum, wallnum;
	int     signature;
};

//Start old wall structures

struct v16_wall
{
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	fix     hps;                // "Hit points" of the wall.
	sbyte   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
};

struct v19_wall
{
	int     segnum,sidenum;     // Seg & side for this wall
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	fix     hps;                // "Hit points" of the wall.
	sbyte   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
	int linked_wall;            // number of linked wall
};

//End old wall structures

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct wall
{
	short     segnum;
	int sidenum;     // Seg & side for this wall
	fix     hps;                // "Hit points" of the wall.
	int     linked_wall;        // number of linked wall
	ubyte   type;               // What kind of special wall.
	ubyte   flags;              // Flags for the wall.
	ubyte   state;              // Opening, closing, etc.
	sbyte   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	ubyte   keys;               // which keys are required
#if defined(DXX_BUILD_DESCENT_I)
	short	pad;					// keep longword aligned
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte   controlling_trigger;// which trigger causes something to happen here.  Not like "trigger" above, which is the trigger on this wall.
                                //  Note: This gets stuffed at load time in gamemine.c.  Don't try to use it in the editor.  You will be sorry!
	sbyte   cloak_value;        // if this wall is cloaked, the fade value
#endif
};
#endif

struct active_door
{
	int     n_parts;            // for linked walls
	array<short, 2>   front_wallnum;   // front wall numbers for this door
	array<short, 2>   back_wallnum;    // back wall numbers for this door
	fix     time;               // how long been opening, closing, waiting
};

#if defined(DXX_BUILD_DESCENT_II)
struct cloaking_wall
{
	short       front_wallnum;  // front wall numbers for this door
	short       back_wallnum;   // back wall numbers for this door
	array<fix, 4> front_ls;     // front wall saved light values
	array<fix, 4> back_ls;      // back wall saved light values
	fix     time;               // how long been cloaking or decloaking
};
#endif

//wall clip flags
#define WCF_EXPLODES    1       //door explodes when opening
#define WCF_BLASTABLE   2       //this is a blastable wall
#define WCF_TMAP1       4       //this uses primary tmap, not tmap2
#define WCF_HIDDEN      8       //this uses primary tmap, not tmap2

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define MAX_CLIP_FRAMES_D1 20

struct wclip {
	fix     play_time;
	short   num_frames;
	union {
		array<int16_t, MAX_CLIP_FRAMES> frames;
		array<int16_t, MAX_CLIP_FRAMES_D1> d1_frames;
	};
	short   open_sound;
	short   close_sound;
	short   flags;
	array<char, 13> filename;
};

extern const char Wall_names[7][10];

//#define WALL_IS_DOORWAY(seg,side) wall_is_doorway(seg, side)

#if defined(DXX_BUILD_DESCENT_II)
#define MAX_CLOAKING_WALLS 10
extern array<cloaking_wall, MAX_CLOAKING_WALLS> CloakingWalls;
extern unsigned Num_cloaking_walls;
#endif

extern array<wall, MAX_WALLS> Walls;           // Master walls array
extern unsigned Num_walls;                   // Number of walls

static inline ssize_t operator-(wall *w, array<wall, MAX_WALLS> &W)
{
	return w - static_cast<wall *>(&*W.begin());
}

extern array<active_door, MAX_DOORS> ActiveDoors;  //  Master doors array
extern unsigned Num_open_doors;              // Number of open doors

extern unsigned Num_wall_anims;
extern array<wclip, MAX_WALL_ANIMS> WallAnims;

extern int walls_bm_num[MAX_WALL_ANIMS];
#else
struct wall;
struct wclip;
#endif

// Initializes all walls (i.e. no special walls.)
extern void wall_init();

// Automatically checks if a there is a doorway (i.e. can fly through)
extern int wall_is_doorway ( segment *seg, int side );

static inline int WALL_IS_DOORWAY(segment *seg, int side)
{
	if (seg->children[side] == segment_none)
		return WID_WALL;
	if (seg->children[side] == segment_exit)
		return WID_EXTERNAL;
	if (seg->sides[side].wall_num == -1)
		return WID_NO_WALL;
	return wall_is_doorway(seg, side);
}

// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
extern void wall_damage(segment *seg, int side, fix damage);

// Destroys a blastable wall. (So it is an opening afterwards)
extern void wall_destroy(segment *seg, int side);

void wall_illusion_on(segment *seg, int side);

void wall_illusion_off(segment *seg, int side);

// Opens a door, including animation and other processing.
void do_door_open(int door_num);

// Closes a door, including animation and other processing.
void do_door_close(int door_num);

// Opens a door
extern void wall_open_door(segment *seg, int side);

#if defined(DXX_BUILD_DESCENT_I)
// Closes a door (called after given interval)
extern void wall_close_door(int wall_num);
#elif defined(DXX_BUILD_DESCENT_II)
// Closes a door
extern void wall_close_door(segment *seg, int side);
#endif

//return codes for wall_hit_process()
#define WHP_NOT_SPECIAL     0       //wasn't a quote-wall-unquote
#define WHP_NO_KEY          1       //hit door, but didn't have key
#define WHP_BLASTABLE       2       //hit blastable wall
#define WHP_DOOR            3       //a door (which will now be opening)

// Determines what happens when a wall is shot
//obj is the object that hit...either a weapon or the player himself
extern int wall_hit_process(segment *seg, int side, fix damage, int playernum, object *obj );

// Opens/destroys specified door.
extern void wall_toggle(int segnum, int side);

// Tidy up Walls array for load/save purposes.
extern void reset_walls();

// Called once per frame..
void wall_frame_process();

extern stuckobj Stuck_objects[MAX_STUCK_OBJECTS];

//  An object got stuck in a door (like a flare).
//  Add global entry.
void add_stuck_object(objptridx_t objp, int segnum, int sidenum);
extern void remove_obsolete_stuck_objects(void);

//set the tmap_num or tmap_num2 field for a wall/door
extern void wall_set_tmap_num(segment *seg,int side,segment *csegp,int cside,int anim_num,int frame_num);

// Remove any flares from a wall
void kill_stuck_objects(int wallnum);

#if defined(DXX_BUILD_DESCENT_II)
//start wall open <-> closed transitions
void start_wall_cloak(segment *seg, int side);
void start_wall_decloak(segment *seg, int side);

void wclip_read_d1(PHYSFS_file *fp, wclip &wc);

void cloaking_wall_read(cloaking_wall &cw, PHYSFS_file *fp);
void cloaking_wall_write(const cloaking_wall &cw, PHYSFS_file *fp);
#endif

/*
 * reads n wclip structs from a PHYSFS_file
 */
void wclip_read(PHYSFS_file *, wclip &wc);
void wclip_write(PHYSFS_file *, const wclip &);

/*
 * reads a v16_wall structure from a PHYSFS_file
 */
void v16_wall_read(PHYSFS_file *fp, v16_wall &w);

/*
 * reads a v19_wall structure from a PHYSFS_file
 */
void v19_wall_read(PHYSFS_file *fp, v19_wall &w);

/*
 * reads a wall structure from a PHYSFS_file
 */
void wall_read(PHYSFS_file *fp, wall &w);

/*
 * reads an active_door structure from a PHYSFS_file
 */
void active_door_read(PHYSFS_file *fp, active_door &ad);
void active_door_write(PHYSFS_file *fp, const active_door &ad);

void wall_write(PHYSFS_file *fp, const wall &w, short version);
void wall_close_door_num(int door_num);
void init_stuck_objects(void);
void clear_stuck_objects(void);
void blast_nearby_glass(struct object *objp, fix damage);


#endif

#endif
