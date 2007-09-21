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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/terrain.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:03 $
 * 
 * Header for terrain.c
 * 
 * $Log: terrain.h,v $
 * Revision 1.1.1.1  2006/03/17 19:44:03  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:12  donut
 * Import of d1x 1.37 source.
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
void render_terrain(vms_vector *org,int org_i,int org_j);


#endif
