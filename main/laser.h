/* $Id: laser.h,v 1.3 2003-10-10 00:30:28 btb Exp $ */
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
 * Definitions for the laser code.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:58:43  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:27  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.50  1995/02/01  21:03:44  john
 * Lintified.
 *
 * Revision 1.49  1995/02/01  16:34:11  john
 * Linted.
 *
 * Revision 1.48  1995/01/31  16:16:16  mike
 * Separate smart blobs for robot and player.
 *
 * Revision 1.47  1995/01/27  17:02:55  mike
 * Add LASER_ID -- why wasn't this added in June?
 *
 * Revision 1.46  1995/01/25  10:53:27  mike
 * make badass damage go through grates.
 *
 * Revision 1.45  1994/12/12  17:17:32  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.44  1994/12/04  16:17:23  mike
 * spruce up homing missile behavior.
 *
 * Revision 1.43  1994/12/03  12:48:30  mike
 * make homing missile tracking not frame rate dependent (more-or-less)
 *
 * Revision 1.42  1994/10/12  08:04:54  mike
 * Clean up proximity/homing mess.
 *
 * Revision 1.41  1994/10/09  20:07:04  rob
 * Change prototype for do_laser_firing
 *
 * Revision 1.40  1994/10/09  00:15:48  mike
 * Add constants for super mech missile, regular mech missile, silent spreadfire.
 *
 * Revision 1.39  1994/10/08  19:52:09  rob
 * Added new weapon fire flags.
 *
 * Revision 1.38  1994/10/07  15:31:12  mike
 * Prototypes for new laser functions which don't necessarily make sound.
 *
 * Revision 1.37  1994/10/05  17:08:29  matt
 * Fixed a couple of small bugs, and made homing missiles alternate sides
 *
 * Revision 1.36  1994/09/28  14:28:55  rob
 * Added firing of missiles on networks/serial.
 *
 * Revision 1.35  1994/09/24  14:17:02  mike
 * Prototype do_laser_firing.
 *
 * Revision 1.34  1994/09/23  11:36:49  mike
 * Prototype Laser_create_new_easy.
 *
 * Revision 1.33  1994/09/20  11:55:01  mike
 * Fix bug.
 *
 * Revision 1.32  1994/09/20  11:48:34  mike
 * Change spreadfire laser to use new bitmap. (Define SPREADFIRE_ID)
 *
 * Revision 1.31  1994/09/15  16:31:28  mike
 * Prototype object_to_object_visibility.
 *
 * Revision 1.30  1994/09/10  17:31:40  mike
 * Add thrust to weapons.
 *
 * Revision 1.29  1994/09/08  14:49:44  mike
 * Bunch of IDs for new weapon types.
 *
 * Revision 1.28  1994/09/07  19:16:40  mike
 * Homing missile.
 *
 * Revision 1.27  1994/09/07  15:59:47  mike
 * Kill FLARE_MAX_TIME (now defined in bitmaps.tbl), add PROXIMITY_ID (shame!), prototype do_laser_firing, do_missile_firing.
 *
 * Revision 1.26  1994/09/03  15:22:41  mike
 * Kill Projectile_player_fire prototype.
 *
 * Revision 1.25  1994/09/02  16:39:00  mike
 * IDs for primary weapons.
 *
 * Revision 1.24  1994/09/02  11:55:54  mike
 * Define some illegal constants.
 *
 * Revision 1.23  1994/08/25  18:12:06  matt
 * Made player's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.22  1994/08/19  15:22:28  mike
 * Define constant for MAX_LASER_LEVEL.
 *
 * Revision 1.21  1994/08/13  12:20:47  john
 * Made the networking uise the Players array.
 *
 * Revision 1.20  1994/08/10  10:44:05  john
 * Made net players fire..
 *
 * Revision 1.19  1994/06/27  18:30:57  mike
 * Add flares.
 *
 * Revision 1.18  1994/06/09  15:32:37  mike
 * Muzzle flash
 *
 * Revision 1.17  1994/05/19  09:09:00  mike
 * Move a bunch of laser variables to bm.h, I think.
 * Also, added Robot_laser_speed, instead of hard-coding Laser_speed/4.
 *
 * Revision 1.16  1994/05/14  17:16:20  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.15  1994/05/13  20:27:39  john
 * Version II of John's new object code.
 *
 * Revision 1.14  1994/04/20  15:06:47  john
 * Neatend laser code and fixed some laser bugs.
 *
 * Revision 1.13  1994/04/01  13:35:15  matt
 * Cleaned up laser code a bit; moved some code here object.c to laser.c
 *
 * Revision 1.12  1994/04/01  11:14:24  yuan
 * Added multiple bitmap functionality to all objects...
 * (hostages, powerups, lasers, etc.)
 * Hostages and powerups are implemented in the object system,
 * just need to finish function call to "affect" player.
 *
 * Revision 1.11  1994/03/31  09:10:09  matt
 * Added #define to turn crosshair off
 *
 * Revision 1.10  1994/02/17  11:33:15  matt
 * Changes in object system
 *
 * Revision 1.9  1994/01/06  11:56:01  john
 * Made lasers be lines, not purple blobs
 *
 * Revision 1.8  1994/01/05  10:53:35  john
 * New object code by John.
 *
 * Revision 1.7  1993/12/08  14:21:36  john
 * Added ExplodeObject
 *
 * Revision 1.6  1993/12/08  11:28:54  john
 * Made lasers look like bolts.
 *
 * Revision 1.5  1993/12/01  13:12:40  john
 * made lasers frame-rate independant
 *
 * Revision 1.4  1993/11/30  19:00:42  john
 * lasers working kinda
 *
 * Revision 1.3  1993/11/29  19:44:53  john
 * *** empty log message ***
 *
 * Revision 1.2  1993/11/29  17:44:55  john
 * *** empty log message ***
 *
 * Revision 1.1  1993/11/29  17:19:19  john
 * Initial revision
 *
 *
 */

#ifndef _LASER_H
#define _LASER_H

#define LASER_ID        0   //0..3 are lasers
#define CONCUSSION_ID   8
#define FLARE_ID        9   //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define VULCAN_ID       11  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define SPREADFIRE_ID   12  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define PLASMA_ID       13  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define FUSION_ID       14  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define HOMING_ID       15
#define PROXIMITY_ID    16
#define SMART_ID        17
#define MEGA_ID         18

#define PLAYER_SMART_HOMING_ID  19
#define SUPER_MECH_MISS         21
#define REGULAR_MECH_MISS       22
#define SILENT_SPREADFIRE_ID    23
#define ROBOT_SMART_HOMING_ID   29
#define EARTHSHAKER_MEGA_ID     54

#define SUPER_LASER_ID          30  // 30,31 are super lasers (level 5,6)

#define GAUSS_ID                32  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define HELIX_ID                33  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define PHOENIX_ID              34  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define OMEGA_ID                35  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.

#define FLASH_ID                36
#define GUIDEDMISS_ID           37
#define SUPERPROX_ID            38
#define MERCURY_ID              39
#define EARTHSHAKER_ID          40

#define SMART_MINE_HOMING_ID        47
#define ROBOT_SMART_MINE_HOMING_ID  49
#define ROBOT_SUPERPROX_ID          53
#define ROBOT_EARTHSHAKER_ID        58

#define PMINE_ID                    51  //the mine that the designers can place

#define OMEGA_MULTI_LIFELEFT    (F1_0/6)

// These are new defines for the value of 'flags' passed to do_laser_firing.
// The purpose is to collect other flags like QUAD_LASER and Spreadfire_toggle
// into a single 8-bit quantity so it can be easily used in network mode.

#define LASER_QUAD                  1
#define LASER_SPREADFIRE_TOGGLED    2
#define LASER_HELIX_FLAG0           4   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG1           8   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG2           16  // helix uses 3 bits for angle

#define LASER_HELIX_SHIFT       2   // how far to shift count to put in flags
#define LASER_HELIX_MASK        7   // must match number of bits in flags

#define MAX_LASER_LEVEL         3   // Note, laser levels are numbered from 0.
#define MAX_SUPER_LASER_LEVEL   5   // Note, laser levels are numbered from 0.

#define MAX_LASER_BITMAPS   6

// For muzzle firing casting light.
#define MUZZLE_QUEUE_MAX    8

// Constants governing homing missile behavior.
// MIN_TRACKABLE_DOT gets inversely scaled by FrameTime and stuffed in
// Min_trackable_dot
#define MIN_TRACKABLE_DOT               (7*F1_0/8)
#define MAX_TRACKABLE_DIST              (F1_0*250)
#define HOMING_MISSILE_STRAIGHT_TIME    (F1_0/8)    //  Changed as per request of John, Adam, Yuan, but mostly John

extern fix Min_trackable_dot;   //  MIN_TRACKABLE_DOT inversely scaled by FrameTime

extern object *Guided_missile[];
extern int Guided_missile_sig[];

void Laser_render(object *obj);
void Laser_player_fire(object * obj, int type, int gun_num, int make_sound, int harmless_flag);
void Laser_player_fire_spread(object *obj, int laser_type, int gun_num, fix spreadr, fix spreadu, int make_sound, int harmless);
void Laser_do_weapon_sequence(object *obj);
void Flare_create(object *obj);
int laser_are_related(int o1, int o2);

extern int do_laser_firing_player(void);
extern void do_missile_firing(int do_autoselect);
extern void net_missile_firing(int player, int weapon, int flags);

int Laser_create_new(vms_vector * direction, vms_vector * position, int segnum, int parent, int type, int make_sound);

// Fires a laser-type weapon (a Primary weapon)
// Fires from object objnum, weapon type weapon_id.
// Assumes that it is firing from a player object, so it knows which
// gun to fire from.
// Returns the number of shots actually fired, which will typically be
// 1, but could be higher for low frame rates when rapidfire weapons,
// such as vulcan or plasma are fired.
extern int do_laser_firing(int objnum, int weapon_id, int level, int flags, int nfires);

// Easier to call than Laser_create_new because it determines the
// segment containing the firing point and deals with it being stuck
// in an object or through a wall.
// Fires a laser of type "weapon_type" from an object (parent) in the
// direction "direction" from the position "position"
// Returns object number of laser fired or -1 if not possible to fire
// laser.
int Laser_create_new_easy(vms_vector * direction, vms_vector * position, int parent, int weapon_type, int make_sound);

// creates a weapon object
int create_weapon_object(int weapon_type,int segnum,vms_vector *position);

// give up control of the guided missile
void release_guided_missile(int player_num);

extern void create_smart_children(object *objp, int count);
extern int object_to_object_visibility(object *obj1, object *obj2, int trans_type);

extern int Muzzle_queue_index;

typedef struct muzzle_info {
	fix         create_time;
	short       segnum;
	vms_vector  pos;
} muzzle_info;

extern muzzle_info Muzzle_data[MUZZLE_QUEUE_MAX];

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
extern fix Omega_charge;
// NOTE: OMEGA_CHARGE_SCALE moved to laser.c to avoid long rebuilds if changed

#endif /* _LASER_H */
