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
 * Stuff for video clips.
 *
 */


#ifndef _VCLIP_H
#define _VCLIP_H

#include "gr.h"
#include "object.h"
#include "piggy.h"

#define VCLIP_SMALL_EXPLOSION       2
#define VCLIP_PLAYER_HIT            1
#define VCLIP_MORPHING_ROBOT        10
#define VCLIP_PLAYER_APPEARANCE     61
#define VCLIP_POWERUP_DISAPPEARANCE 62
#define VCLIP_VOLATILE_WALL_HIT     5
#define VCLIP_WATER_HIT             84
#define VCLIP_AFTERBURNER_BLOB      95
#define VCLIP_MONITOR_STATIC        99

#define VCLIP_MAXNUM                110
#define VCLIP_MAX_FRAMES            30

// vclip flags
#define VF_ROD      1       // draw as a rod, not a blob

typedef struct {
	fix             play_time;          // total time (in seconds) of clip
	int             num_frames;
	fix             frame_time;         // time (in seconds) of each frame
	int             flags;
	short           sound_num;
	bitmap_index    frames[VCLIP_MAX_FRAMES];
	fix             light_value;
} __pack__ vclip;

extern int Num_vclips;
extern vclip Vclip[VCLIP_MAXNUM];

// draw an object which renders as a vclip.
void draw_vclip_object(object *obj, fix timeleft, int lighted, int vclip_num);
extern void draw_weapon_vclip(object *obj);

/*
 * reads n vclip structs from a PHYSFS_file
 */
extern int vclip_read_n(vclip *vc, int n, PHYSFS_file *fp);

#endif /* _VCLIP_H */
