/* $Id: lighting.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Lighting system prototypes, structures, etc.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:58:51  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:52  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.6  1994/11/28  21:50:56  mike
 * optimizations.
 *
 * Revision 1.5  1994/06/07  16:51:58  matt
 * Made object lighting work correctly; changed name of Ambient_light to
 * Dynamic_light; cleaned up polygobj object rendering a little.
 *
 * Revision 1.4  1994/05/31  18:41:35  matt
 * Added comments
 *
 * Revision 1.3  1994/05/23  15:00:08  mike
 * Change MIN_LIGHT_DIST.
 *
 * Revision 1.2  1994/05/22  15:30:09  mike
 * First version.
 *
 * Revision 1.1  1994/05/22  15:16:44  mike
 * Initial revision
 *
 *
 */


#ifndef _LIGHTING_H
#define _LIGHTING_H

#define MAX_LIGHT       0x10000     // max value

#define MIN_LIGHT_DIST  (F1_0*4)

extern fix Beam_brightness;
extern fix Dynamic_light[MAX_VERTICES];

extern void set_dynamic_light(void);

// Compute the lighting from the headlight for a given vertex on a face.
// Takes:
//  point - the 3d coords of the point
//  face_light - a scale factor derived from the surface normal of the face
// If no surface normal effect is wanted, pass F1_0 for face_light
fix compute_headlight_light(vms_vector *point,fix face_light);

// compute the average dynamic light in a segment.  Takes the segment number
fix compute_seg_dynamic_light(int segnum);

// compute the lighting for an object.  Takes a pointer to the object,
// and possibly a rotated 3d point.  If the point isn't specified, the
// object's center point is rotated.
fix compute_object_light(object *obj,vms_vector *rotated_pnt);

// turn headlight boost on & off
void toggle_headlight_active(void);

#endif /* _LIGHTING_H */
