/* $Id: mglobal.c,v 1.4 2003-10-10 09:36:35 btb Exp $ */
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
 * Global variables for main directory
 *
 * Old Log:
 * Revision 1.1  1995/12/05  16:03:10  allender
 * Initial revision
 *
 * Revision 1.3  1995/10/10  11:49:41  allender
 * removed malloc of static data now in ai module
 *
 * Revision 1.2  1995/07/12  12:48:52  allender
 * malloc out edge_list global here, not static in automap.c
 *
 * Revision 1.1  1995/05/16  15:27:40  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/14  18:24:37  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.1  1995/03/06  16:47:23  mike
 * destination saturn
 *
 * Revision 2.0  1995/02/27  11:30:00  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.43  1995/01/19  17:00:53  john
 * Made save game work between levels.
 *
 * Revision 1.42  1994/12/05  14:23:53  adam
 * changed default detail to max, not custom
 *
 * Revision 1.41  1994/11/19  15:15:07  mike
 * remove unused code and data
 *
 * Revision 1.40  1994/11/03  10:13:19  yuan
 * Added #include "game.h"
 *
 * Revision 1.39  1994/11/03  10:09:59  matt
 * Properly initialize detail & difficulty levels
 *
 * Revision 1.38  1994/10/30  14:11:21  mike
 * rip out local segments stuff.
 *
 * Revision 1.37  1994/10/26  15:21:30  mike
 * detail level.
 *
 * Revision 1.36  1994/09/22  10:46:12  mike
 * Add difficulty levels.
 *
 * Revision 1.35  1994/09/13  11:19:11  mike
 * Add Next_missile_fire_time.
 *
 * Revision 1.34  1994/08/31  19:25:34  mike
 * GameTime and laser-firing limiting stuff added.
 *
 * Revision 1.33  1994/08/11  18:58:53  mike
 * Add Side_to_verts_int.
 *
 * Revision 1.32  1994/07/21  19:01:38  mike
 * Add Lsegment.
 *
 * Revision 1.31  1994/07/21  13:11:24  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables object movement and game options from menu.
 *
 * Revision 1.30  1994/06/17  18:06:48  matt
 * Made password be treated as lowercase, since cmdline parsing converts
 * everything to lowercase.
 *
 * Revision 1.29  1994/03/15  16:33:04  yuan
 * Cleaned up bm-loading code.
 * (Fixed structures too)
 *
 * Revision 1.28  1994/02/17  11:32:45  matt
 * Changes in object system
 *
 * Revision 1.27  1994/02/16  17:08:43  matt
 * Added needed include of 3d.h
 *
 * Revision 1.26  1994/02/16  13:47:58  mike
 * fix bugs so editor can compile out.
 *
 * Revision 1.25  1994/02/11  21:52:13  matt
 * Made password protection selectable by #define (and thus INFERNO.INI)
 *
 * Revision 1.24  1994/02/10  15:35:56  matt
 * Various changes to make editor compile out.
 *
 * Revision 1.23  1994/02/02  12:34:29  mike
 * take out BATS encryption.
 *
 * Revision 1.22  1994/01/21  16:08:11  matt
 * Added FrameCount variable
 *
 * Revision 1.21  1994/01/06  17:13:10  john
 * Added Video clip functionality
 *
 * Revision 1.20  1993/12/08  17:45:08  matt
 * Changed password again
 *
 * Revision 1.19  1993/12/08  17:41:05  matt
 * Changed password
 *
 * Revision 1.18  1993/12/08  10:55:10  mike
 * Add free_obj_list
 *
 * Revision 1.17  1993/12/07  13:46:38  john
 * Added Explosion bitmap array
 *
 * Revision 1.16  1993/12/06  18:40:35  matt
 * Changed object loading & handling
 *
 * Revision 1.15  1993/12/05  22:47:48  matt
 * Reworked include files in an attempt to cut down on build times
 *
 * Revision 1.14  1993/12/01  11:44:11  matt
 * Chagned Frfract to FrameTime
 *
 * Revision 1.13  1993/12/01  00:27:11  yuan
 * Implemented new bitmap structure system...
 * overall bitmap scheme still needs some work.
 *
 * Revision 1.12  1993/11/19  17:21:59  matt
 * Changed the bitmap number of object class UNICLASS
 * Removed static initialization for objects
 *
 * Revision 1.11  1993/11/18  13:51:47  mike
 * Add Classes, Class_views, Objects
 *
 * Revision 1.10  1993/11/04  18:52:36  matt
 * Made Vertices[] and Segment_points[] use same constant for size, since
 * they must be the same size anyway
 *
 * Revision 1.9  1993/11/04  14:01:06  matt
 * Mucked with include files
 *
 * Revision 1.8  1993/10/26  13:58:42  mike
 * Add password protection.
 *
 * Revision 1.7  1993/10/14  18:05:50  mike
 * Change Side_to_verts to use MAX_SIDES_PER_SEGMENT in place of CONNECTIVITY
 *
 * Revision 1.6  1993/10/12  13:57:19  john
 * added texture[]
 *
 * Revision 1.5  1993/10/12  09:58:15  mike
 * Move Side_to_verts here from eglobal.c, since it is needed in the game.
 *
 * Revision 1.4  1993/10/09  15:52:30  mike
 * Move test_pos, test_orient here from render.c.
 *
 * Revision 1.3  1993/10/02  18:15:45  mike
 * Killed include of segment.h, which gets included by inferno.h.
 *
 * Revision 1.2  1993/09/23  17:54:24  mike
 * Add Segment_points
 *
 * Revision 1.1  1993/09/23  15:01:50  mike
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: mglobal.c,v 1.4 2003-10-10 09:36:35 btb Exp $";
#endif

#include "fix.h"
#include "vecmat.h"
#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "bm.h"
#include "3d.h"
#include "game.h"


// Global array of vertices, common to one mine.
vms_vector Vertices[MAX_VERTICES];
g3s_point Segment_points[MAX_VERTICES];

fix FrameTime = 0x1000;	// Time since last frame, in seconds
fix GameTime = 0;			//	Time in game, in seconds

//How many frames we've rendered
int FrameCount = 0;

//	This is the global mine which create_new_mine returns.
segment	Segments[MAX_SEGMENTS];
segment2	Segment2s[MAX_SEGMENTS];
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, Vertices, pointed to by Vp)
int		Num_vertices = 0;
int		Num_segments = 0;

int		Highest_vertex_index=0;
int		Highest_segment_index=0;

//	Translate table to get opposite side of a face on a segment.
char	Side_opposite[MAX_SIDES_PER_SEGMENT] = {WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK};

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

#ifdef PASSWORD
#define encrypt(a,b,c,d)	a ^ TOLOWER((((int) PASSWORD)>>24)&255), \
									b ^ TOLOWER((((int) PASSWORD)>>16)&255), \
									c ^ TOLOWER((((int) PASSWORD)>>8)&255), \
									d ^ TOLOWER((((int) PASSWORD))&255)
#else
#define encrypt(a,b,c,d) a,b,c,d
#endif

sbyte Side_to_verts[MAX_SIDES_PER_SEGMENT][4] = {
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
};		

//	Note, this MUST be the same as Side_to_verts, it is an int for speed reasons.
int Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4] = {
			{ encrypt(7,6,2,3) },			// left
			{ encrypt(0,4,7,3) },			// top
			{ encrypt(0,1,5,4) },			// right
			{ encrypt(2,6,5,1) },			// bottom
			{ encrypt(4,5,6,7) },			// back
			{ encrypt(3,2,1,0) },			// front
};		

// Texture map stuff

int NumTextures = 0;
bitmap_index Textures[MAX_TEXTURES];		// All textures.

fix	Next_laser_fire_time;			//	Time at which player can next fire his selected laser.
fix	Next_missile_fire_time;			//	Time at which player can next fire his selected missile.
//--unused-- fix	Laser_delay_time = F1_0/6;		//	Delay between laser fires.

#if defined(MACINTOSH) && defined(APPLE_DEMO)
#define DEFAULT_DIFFICULTY		0		// trainee for apple demo
#else
#define DEFAULT_DIFFICULTY		1
#endif

int	Difficulty_level=DEFAULT_DIFFICULTY;	//	Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
int	Detail_level=NUM_DETAIL_LEVELS-2;		//	Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest

