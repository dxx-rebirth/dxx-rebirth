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
 * $Source: /cvs/cvsroot/d2x/main/editor/seguvs.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 02:27:17 $
 * 
 * Header for seguvs.c
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:40  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:58  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.3  1994/08/03  10:32:28  mike
 * Add stretch_uvs_from_curedge.
 * 
 * Revision 1.2  1994/05/14  18:00:58  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.1  1994/05/14  17:27:26  matt
 * Initial revision
 * 
 * 
 */



#ifndef _SEGUVS_H
#define _SEGUVS_H

extern void assign_light_to_side(segment *sp, int sidenum);
extern void assign_default_lighting_all(void);
extern void stretch_uvs_from_curedge(segment *segp, int side);

#endif
