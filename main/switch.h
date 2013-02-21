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
#define TT_LIGHT_ON         13  // Turn a light on
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

/*
 * reads a v29_trigger structure from a PHYSFS_file
 */
extern void v29_trigger_read(v29_trigger *t, PHYSFS_file *fp);

/*
 * reads a v30_trigger structure from a PHYSFS_file
 */
extern void v30_trigger_read(v30_trigger *t, PHYSFS_file *fp);

/*
 * reads a trigger structure from a PHYSFS_file
 */
extern void trigger_read(trigger *t, PHYSFS_file *fp);

/*
 * reads n trigger structs from a PHYSFS_file and swaps if specified
 */
extern void trigger_read_n_swap(trigger *t, int n, int swap, PHYSFS_file *fp);

extern void trigger_write(trigger *t, short version, PHYSFS_file *fp);

#endif
