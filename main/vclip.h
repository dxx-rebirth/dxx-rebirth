/* $Id: vclip.h,v 1.4 2003-10-10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.2  1995/09/14  14:14:45  allender
 * return void in draw_vclip_object
 *
 * Revision 1.1  1995/05/16  16:04:35  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:42  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.33  1994/11/21  11:17:57  adam
 * *** empty log message ***
 *
 * Revision 1.32  1994/10/12  13:07:07  mike
 * Add VCLIP_POWERUP_DISAPPEARANCE.
 *
 * Revision 1.31  1994/10/12  10:23:54  adam
 * *** empty log message ***
 *
 * Revision 1.30  1994/10/11  12:25:21  matt
 * Added "hot rocks" that create badass explosion when hit by weapons
 *
 * Revision 1.29  1994/10/06  14:10:07  matt
 * Added constant for player appearence effect
 *
 * Revision 1.28  1994/09/26  08:40:30  adam
 * *** empty log message ***
 *
 * Revision 1.27  1994/09/10  17:30:57  mike
 * move a prototype here, had been defined in object.c
 *
 * Revision 1.26  1994/09/09  20:04:25  mike
 * Add weapon_vclip.
 *
 * Revision 1.25  1994/08/31  19:27:09  mike
 * Increase max number of Vclips for new pile of weapon powerups.
 *
 * Revision 1.24  1994/08/14  23:14:43  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.23  1994/07/23  19:56:39  matt
 * Took out unused constant
 *
 * Revision 1.22  1994/07/22  10:42:10  adam
 * upped max no. of vclips
 *
 * Revision 1.21  1994/06/14  21:15:14  matt
 * Made rod objects draw lighted or not depending on a parameter, so the
 * materialization effect no longer darkens.
 *
 * Revision 1.20  1994/06/09  19:38:16  john
 * Made each Vclip have its own sound... but only used in the
 * materialization center.
 *
 * Revision 1.19  1994/06/09  11:46:26  john
 * Took out unused vclip defines.
 *
 * Revision 1.18  1994/06/08  18:16:33  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 *
 * Revision 1.17  1994/06/08  12:49:01  mike
 * Add light_value to vclip.
 *
 * Revision 1.16  1994/06/08  11:43:28  mike
 * Allow 20 vclips, I think (anyway, more than it used to be, probably 12).
 *
 * Revision 1.15  1994/06/03  10:48:22  matt
 * Made vclips (used by explosions) which can be either rods or blobs, as
 * specified in BITMAPS.TBL.  (This is for the materialization center effect).
 *
 * Revision 1.14  1994/06/01  17:21:08  john
 * Added muzzle flash
 *
 * Revision 1.13  1994/06/01  10:34:02  john
 * Added robot morphing effect.
 *
 * Revision 1.12  1994/05/16  16:17:38  john
 * Bunch of stuff on my Inferno Task list May16-23
 *
 * Revision 1.11  1994/05/10  18:32:50  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/04/29  14:35:52  matt
 * Added second kind of fireball
 *
 * Revision 1.9  1994/04/11  10:36:31  yuan
 * Started adding types for exploding hostages.
 *
 * Revision 1.8  1994/04/07  16:27:43  yuan
 * Added SUPERPIG the robot with 200 hit points.
 * Now robots can take multiple hits before blowing up.
 *
 * Revision 1.7  1994/04/07  13:45:58  yuan
 * Defined Pclips... maybe shouldn't be in this file.
 *
 * Revision 1.6  1994/03/28  20:58:22  yuan
 * Added blood vclip constant
 *
 * Revision 1.5  1994/03/15  16:31:56  yuan
 * Cleaned up bm-loading code.
 * (And structures)
 *
 * Revision 1.4  1994/03/04  17:09:43  yuan
 * New wall stuff
 *
 * Revision 1.3  1994/01/11  10:59:01  yuan
 * Added effects
 *
 * Revision 1.2  1994/01/06  17:13:15  john
 * Added Video clip functionality
 *
 * Revision 1.1  1994/01/06  15:10:15  john
 * Initial revision
 *
 *
 */


#ifndef _VCLIP_H
#define _VCLIP_H

#include "gr.h"
#include "object.h"
#include "piggy.h"
#include "cfile.h"

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

#ifdef FAST_FILE_IO
#define vclip_read_n(vc, n, fp) cfread(vc, sizeof(vclip), n, fp)
#else
/*
 * reads n vclip structs from a CFILE
 */
extern int vclip_read_n(vclip *vc, int n, CFILE *fp);
#endif

#endif /* _VCLIP_H */
