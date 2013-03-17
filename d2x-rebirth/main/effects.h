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
 * Headerfile for effects.c
 *
 */


#ifndef _EFFECTS_H
#define _EFFECTS_H

#include "vclip.h"

#define MAX_EFFECTS 110

//flags for eclips.  If no flags are set, always plays

#define EF_CRITICAL 1   //this doesn't get played directly (only when mine critical)
#define EF_ONE_SHOT 2   //this is a special that gets played once
#define EF_STOPPED  4   //this has been stopped

#define ECLIP_NUM_FUELCEN     2
#define ECLIP_NUM_BOSS        53
#define ECLIP_NUM_FORCE_FIELD 78

typedef struct eclip {
	vclip   vc;             //imbedded vclip
	fix     time_left;      //for sequencing
	int     frame_count;    //for sequencing
	short   changing_wall_texture;      //Which element of Textures array to replace.
	short   changing_object_texture;    //Which element of ObjBitmapPtrs array to replace.
	int     flags;          //see above
	int     crit_clip;      //use this clip instead of above one when mine critical
	int     dest_bm_num;    //use this bitmap when monitor destroyed
	int     dest_vclip;     //what vclip to play when exploding
	int     dest_eclip;     //what eclip to play when exploding
	fix     dest_size;      //3d size of explosion
	int     sound_num;      //what sound this makes
	int     segnum,sidenum; //what seg & side, for one-shot clips
} __pack__ eclip;

extern int Num_effects;
extern eclip Effects[MAX_EFFECTS];

// Set up special effects.
extern void init_special_effects();

// Clear any active one-shots
void reset_special_effects();

// Function called in game loop to do effects.
extern void do_special_effects();

// Restore bitmap
extern void restore_effect_bitmap_icons();

//stop an effect from animating.  Show first frame.
void stop_effect(int effect_num);

//restart a stopped effect
void restart_effect(int effect_num);

/*
 * reads n eclip structs from a PHYSFS_file
 */
extern int eclip_read_n(eclip *ec, int n, PHYSFS_file *fp);

#endif /* _EFFECTS_H */
