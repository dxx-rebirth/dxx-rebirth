/* $Id: switch.h,v 1.4 2003-10-04 03:14:48 btb Exp $ */
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
 * Triggers and Switches.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:03:48  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:26:52  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.19  1995/01/12  17:00:36  rob
 * Fixed a problem with switches and secret levels.
 *
 * Revision 1.18  1994/10/06  21:24:40  matt
 * Added switch for exit to secret level
 *
 * Revision 1.17  1994/09/29  17:05:52  matt
 * Removed unused constant
 *
 * Revision 1.16  1994/09/24  17:10:07  yuan
 * Added Matcen triggers.
 *
 * Revision 1.15  1994/08/15  18:06:39  yuan
 * Added external trigger.
 *
 * Revision 1.14  1994/06/16  16:20:52  john
 * Made player start out in physics mode; Neatend up game loop a bit.
 *
 * Revision 1.13  1994/05/30  20:22:08  yuan
 * New triggers.
 *
 * Revision 1.12  1994/05/27  10:32:44  yuan
 * New dialog boxes (Walls and Triggers) added.
 *
 *
 * Revision 1.11  1994/05/25  18:06:32  yuan
 * Making new dialog box controls for walls and triggers.
 *
 * Revision 1.10  1994/04/28  18:04:40  yuan
 * Gamesave added.
 * Trigger problem fixed (seg pointer is replaced by index now.)
 *
 * Revision 1.9  1994/04/26  11:19:01  yuan
 * Make it so a trigger can only be triggered every 5 seconds.
 *
 */

#ifndef _SWITCH_H
#define _SWITCH_H

#include "inferno.h"
#include "segment.h"

#define MAX_TRIGGERS        100
#define MAX_WALLS_PER_LINK  10

// Trigger types

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a wall open
#define TT_CLOSE_WALL       10  // Makes a wall closed
#define TT_ILLUSORY_WALL    11  // Makes a wall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn s light on
#define NUM_TRIGGER_TYPES   14

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only trigger once
#define TF_DISABLED         4   // Set after one-shot fires

//old trigger structs

typedef struct v29_trigger {
	sbyte   type;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   num_links;
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
} __pack__ v29_trigger;

typedef struct v30_trigger {
	short   flags;
	sbyte   num_links;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
} __pack__ v30_trigger;

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_MATCEN            64    // Trigger for materialization centers
#define TRIGGER_ILLUSION_OFF     128    // Switch Illusion OFF trigger
#define TRIGGER_SECRET_EXIT      256    // Exit to secret level
#define TRIGGER_ILLUSION_ON      512    // Switch Illusion ON trigger
#define TRIGGER_UNLOCK_DOORS    1024    // Unlocks a door
#define TRIGGER_OPEN_WALL       2048    // Makes a wall open
#define TRIGGER_CLOSE_WALL      4096    // Makes a wall closed
#define TRIGGER_ILLUSORY_WALL   8192    // Makes a wall illusory

//the trigger really should have both a type & a flags, since most of the
//flags bits are exclusive of the others.
typedef struct trigger {
	ubyte   type;       //what this trigger does
	ubyte   flags;      //currently unused
	sbyte   num_links;  //how many doors, etc. linked to this
	sbyte   pad;        //keep alignment
	fix     value;
	fix     time;
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
} __pack__ trigger;

extern trigger Triggers[MAX_TRIGGERS];

extern int Num_triggers;

extern void trigger_init();
extern void check_trigger(segment *seg, short side, short objnum,int shot);
extern int check_trigger_sub(int trigger_num, int player_num,int shot);
extern void triggers_frame_process();

#ifdef FAST_FILE_IO
#define v29_trigger_read(t, fp) cfread(t, sizeof(v29_trigger), 1, fp)
#define v30_trigger_read(t, fp) cfread(t, sizeof(v30_trigger), 1, fp)
#define trigger_read(t, fp) cfread(t, sizeof(trigger), 1, fp)
#else
/*
 * reads a v29_trigger structure from a CFILE
 */
extern void v29_trigger_read(v29_trigger *t, CFILE *fp);

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read(v30_trigger *t, CFILE *fp);

/*
 * reads a trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp);
#endif

#endif
