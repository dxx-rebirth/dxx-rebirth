/* $Id: bm.h,v 1.11 2003-10-04 03:14:47 btb Exp $ */
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
 * Bitmap and Palette loading functions.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:54:39  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:59  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.60  1994/12/06  13:24:58  matt
 * Made exit model come out of bitmaps.tbl
 *
 * Revision 1.59  1994/11/10  14:02:48  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.58  1994/10/27  13:32:17  john
 * Made TmapList only be in if editor
 *
 * Revision 1.57  1994/10/11  12:25:20  matt
 * Added "hot rocks" that create badass explosion when hit by weapons
 *
 * Revision 1.56  1994/09/29  12:38:13  matt
 * Took out include of digi.h, saving hours of unneeded compiles
 *
 * Revision 1.55  1994/09/25  00:39:51  matt
 * Added the ability to make eclips (monitors, fans) which can be blown up
 *
 * Revision 1.54  1994/09/17  01:40:59  matt
 * Added status bar/sizable window mode, and in the process revamped the
 * whole cockpit mode system.
 *
 * Revision 1.53  1994/09/08  16:20:48  matt
 * Gave textures the ability to cause damage when scraped
 *
 * Revision 1.52  1994/08/30  22:23:43  matt
 * Added cabability for player ship to have alternate model to use to
 * create debris.
 *
 * Revision 1.51  1994/08/26  15:35:57  matt
 * Made eclips usable on more than one object at a time
 *
 * Revision 1.50  1994/08/23  16:59:51  john
 * Added 3 cockpuits
 *
 * Revision 1.49  1994/08/19  20:09:43  matt
 * Added end-of-level cut scene with external scene
 *
 * Revision 1.48  1994/08/12  22:20:45  matt
 * Generalized polygon objects (such as control center)
 *
 * Revision 1.47  1994/08/09  16:04:21  john
 * Added network players to editor.
 *
 * Revision 1.46  1994/08/09  09:01:31  john
 * Increase MAX_TEXTURES
 *
 * Revision 1.45  1994/07/13  00:14:57  matt
 * Moved all (or nearly all) of the values that affect player movement to
 * bitmaps.tbl
 *
 * Revision 1.44  1994/06/20  21:33:15  matt
 * Made bm.h not include sounds.h, to reduce dependencies
 *
 * Revision 1.43  1994/06/15  11:55:58  matt
 * Added 3d model for player
 *
 * Revision 1.42  1994/06/13  16:09:11  adam
 * increased max textures to 600
 *
 * Revision 1.41  1994/06/06  15:38:58  john
 * Made fullscreen view be just another cockpit, but the "hole"
 * in the cockpit is just bigger.
 *
 * Revision 1.40  1994/05/26  21:08:59  matt
 * Moved robot stuff out of polygon model and into robot_info struct
 * Made new file, robot.c, to deal with robots
 *
 * Revision 1.39  1994/05/18  11:00:05  mike
 * Add robot_info stuff.
 *
 * Revision 1.38  1994/05/17  14:44:56  mike
 * Get object type and id from ObjType and ObjId.
 *
 * Revision 1.37  1994/05/17  12:04:45  mike
 * Deal with little known fact that polygon object != robot.
 *
 * Revision 1.36  1994/05/16  16:17:35  john
 * Bunch of stuff on my Inferno Task list May16-23
 *
 * Revision 1.35  1994/04/27  11:43:42  john
 * First version of sound! Yay!
 *
 * Revision 1.34  1994/04/22  10:53:48  john
 * Increased MAX_TEXTURES to 500.
 *
 * Revision 1.33  1994/04/01  11:15:05  yuan
 * Added multiple bitmap functionality to all objects...
 * (hostages, powerups, lasers, etc.)
 * Hostages and powerups are implemented in the object system,
 * just need to finish function call to "affect" player.
 *
 * Revision 1.32  1994/03/25  17:30:37  yuan
 * Checking in hostage stuff.
 *
 * Revision 1.31  1994/03/17  18:07:28  yuan
 * Removed switch code... Now we just have Walls, Triggers, and Links...
 *
 * Revision 1.30  1994/03/15  17:03:51  yuan
 * Added Robot/object bitmap capability
 *
 * Revision 1.29  1994/03/15  16:32:58  yuan
 * Cleaned up bm-loading code.
 * (Fixed structures too)
 *
 * Revision 1.28  1994/03/04  17:09:13  yuan
 * New door stuff.
 *
 * Revision 1.27  1994/01/31  14:50:09  yuan
 * Added Robotex
 *
 * Revision 1.26  1994/01/31  12:27:14  yuan
 * Added demo stuff (menu, etc.)
 *
 * Revision 1.25  1994/01/25  17:11:43  john
 * New texmaped lasers.
 *
 * Revision 1.24  1994/01/24  11:48:06  yuan
 * Lighting stuff
 *
 * Revision 1.23  1994/01/22  13:40:15  yuan
 * Modified the bmd_bitmap structure a bit.
 * (Saves some memory, and added reflection)
 *
 * Revision 1.22  1994/01/11  10:58:38  yuan
 * Added effects system
 *
 * Revision 1.21  1994/01/06  17:13:12  john
 * Added Video clip functionality
 *
 * Revision 1.20  1993/12/21  20:00:15  john
 * moved selector stuff to grs_bitmap
 *
 * Revision 1.19  1993/12/21  19:33:58  john
 * Added selector to bmd_bitmap.
 *
 * Revision 1.18  1993/12/07  12:28:48  john
 * moved bmd_palette to gr_palette
 *
 * Revision 1.17  1993/12/06  18:40:37  matt
 * Changed object loading & handling
 *
 * Revision 1.16  1993/12/05  23:05:03  matt
 * Added include of gr.h
 *
 * Revision 1.15  1993/12/03  17:38:04  yuan
 * Ooops. meant to say:
 * Moved MAX variables to bm.c, Arrays left open.
 *
 * Revision 1.14  1993/12/03  17:37:26  yuan
 * Added Asserts.
 *
 * Revision 1.13  1993/12/02  17:22:54  yuan
 * New global var. Num_object_types
 *
 * Revision 1.12  1993/12/02  16:34:39  yuan
 * Added fireball hack stuff.
 *
 * Revision 1.11  1993/12/02  15:45:14  yuan
 * Added a buncha constants, variables, and function prototypes
 * for the new bitmaps.tbl format.
 *
 * Revision 1.10  1993/12/01  11:25:11  yuan
 * Changed MALLOC'd buffers for filename and type in
 * the bmd_bitmap structure into arrays... Saves time
 * at load up.
 *
 * Revision 1.9  1993/12/01  00:28:09  yuan
 * New bitmap system structure.
 *
 * Revision 1.8  1993/11/03  11:34:08  john
 * made it use bitmaps.tbl
 *
 * Revision 1.7  1993/10/26  18:11:03  john
 * made all palette data be statically allocated
 *
 * Revision 1.6  1993/10/19  12:17:51  john
 * *** empty log message ***
 *
 * Revision 1.5  1993/10/16  20:02:41  matt
 * Changed name of backdrop bitmap file
 *
 * Revision 1.4  1993/10/12  15:08:52  matt
 * Added a bunch of new textures
 *
 * Revision 1.3  1993/10/12  12:30:41  john
 * *** empty log message ***
 *
 * Revision 1.2  1993/10/12  11:27:58  john
 * added more bitmaps
 *
 * Revision 1.1  1993/09/23  13:09:10  john
 * Initial revision
 *
 *
 */

#ifndef _BM_H
#define _BM_H

#include "gr.h"
#include "piggy.h"

#define MAX_TEXTURES    1200

//tmapinfo flags
#define TMI_VOLATILE    1   //this material blows up when hit
#define TMI_WATER       2   //this material is water
#define TMI_FORCE_FIELD 4   //this is force field - flares don't stick
#define TMI_GOAL_BLUE   8   //this is used to remap the blue goal
#define TMI_GOAL_RED    16  //this is used to remap the red goal
#define TMI_GOAL_HOARD  32  //this is used to remap the goals

typedef struct {
	ubyte   flags;     //values defined above
	ubyte   pad[3];    //keep alignment
	fix     lighting;  //how much light this casts
	fix     damage;    //how much damage being against this does (for lava)
	short   eclip_num; //the eclip that changes this, or -1
	short   destroyed; //bitmap to show when destroyed, or -1
	short   slide_u,slide_v;    //slide rates of texture, stored in 8:8 fix
	#ifdef EDITOR
	char    filename[13];       //used by editor to remap textures
	char    pad2[3];
	#endif
} __pack__ tmap_info;

extern int Num_object_types;

#define N_COCKPIT_BITMAPS 6
extern int Num_cockpits;
extern bitmap_index cockpit_bitmap[N_COCKPIT_BITMAPS];

extern int Num_tmaps;
#ifdef EDITOR
extern int TmapList[MAX_TEXTURES];
#endif

extern tmap_info TmapInfo[MAX_TEXTURES];

//for each model, a model number for dying & dead variants, or -1 if none
extern int Dying_modelnums[];
extern int Dead_modelnums[];

//the model number of the marker object
extern int Marker_model_num;

// Initializes the palette, bitmap system...
int bm_init();
void bm_close();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

#define OL_ROBOT            1
#define OL_HOSTAGE          2
#define OL_POWERUP          3
#define OL_CONTROL_CENTER   4
#define OL_PLAYER           5
#define OL_CLUTTER          6   //some sort of misc object
#define OL_EXIT             7   //the exit model for external scenes
#define OL_WEAPON           8   //a weapon that can be placed

#define MAX_OBJTYPE         140

extern int  Num_total_object_types;     // Total number of object types, including robots, hostages, powerups, control centers, faces
extern sbyte ObjType[MAX_OBJTYPE];      // Type of an object, such as Robot, eg if ObjType[11] == OL_ROBOT, then object #11 is a robot
extern sbyte ObjId[MAX_OBJTYPE];        // ID of a robot, within its class, eg if ObjType[11] == 3, then object #11 is the third robot
extern fix  ObjStrength[MAX_OBJTYPE];   // initial strength of each object

#define MAX_OBJ_BITMAPS     610

extern bitmap_index ObjBitmaps[MAX_OBJ_BITMAPS];
extern ushort ObjBitmapPtrs[MAX_OBJ_BITMAPS];
extern int First_multi_bitmap_num;

// Initializes all bitmaps from BITMAPS.TBL file.
int bm_init_use_tbl(void);

extern void bm_read_all(CFILE * fp);

#define D1_MAX_TEXTURES 800
extern short *d1_Texture_indices; // descent 1 texture bitmap indicies
extern void free_d1_texture_indices();
extern void bm_read_d1_texture_indices(CFILE *d1pig);

int load_exit_models();

#endif /* _BM_H */
