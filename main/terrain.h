/* $Id: terrain.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Header for terrain.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:03:56  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:53  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.3  1994/10/27  01:03:51  matt
 * Made terrain renderer use aribtary point in height array as origin
 *
 * Revision 1.2  1994/08/19  20:09:45  matt
 * Added end-of-level cut scene with external scene
 *
 * Revision 1.1  1994/08/17  20:33:36  matt
 * Initial revision
 *
 *
 */


#ifndef _TERRAIN_H
#define _TERRAIN_H

void load_terrain(char *filename);
void render_terrain(vms_vector *org, int org_i, int org_j);

#endif /* _TERRAIN_H */
