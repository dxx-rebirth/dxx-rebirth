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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/3d/clipper.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:05 $
 * 
 * Header for clipper.c
 * 
 * $Log: clipper.h,v $
 * Revision 1.1.1.1  2006/03/17 19:39:05  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 21:57:51  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.1  1995/05/05  08:50:13  allender
 * Initial revision
 *
 * Revision 1.1  1995/04/17  19:56:58  matt
 * Initial revision
 * 
 * 
 */



#ifndef _CLIPPER_H
#define _CLIPPER_H

extern void free_temp_point(g3s_point *p);
extern g3s_point **clip_polygon(g3s_point **src,g3s_point **dest,int *nv,g3s_codes *cc);
extern void init_free_points(void);
extern void clip_line(g3s_point **p0,g3s_point **p1,ubyte codes_or);

#endif
