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
 * $Source: /cvs/cvsroot/d2x/main/editor/meddraw.h,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Defnts for med drawing stuff
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:38  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:12  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.3  1994/07/06  16:36:54  mike
 * Prototype for draw_mine_all.
 * 
 * Revision 1.2  1993/12/17  12:05:09  john
 * Took stuff out of med.c; moved into medsel.c, meddraw.c, medmisc.c
 * 
 * Revision 1.1  1993/12/17  08:55:14  john
 * Initial revision
 * 
 * 
 */



#ifndef _MEDDRAW_H
#define _MEDDRAW_H


void meddraw_init_views( grs_canvas * canvas);
void draw_world(grs_canvas *screen_canvas,editor_view *v,segment *mine_ptr,int depth);
void find_segments(short x,short y,grs_canvas *screen_canvas,editor_view *v,segment *mine_ptr,int depth);

//    segp = pointer to segments array, probably always Segments.
//    automap_flag = 1 if this render is for the automap, else 0 (for editor)
extern void draw_mine_all(segment *segp, int automap_flag);

#endif
