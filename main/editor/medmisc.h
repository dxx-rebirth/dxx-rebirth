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
 * $Source: /cvs/cvsroot/d2x/main/editor/medmisc.h,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Defn'tns for medmisc.c
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:39  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:40  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.3  1994/07/21  17:25:28  matt
 * Took out unused func medlisp_create_new_mine() and its prototype
 * 
 * Revision 1.2  1993/12/17  12:05:04  john
 * Took stuff out of med.c; moved into medsel.c, meddraw.c, medmisc.c
 * 
 * Revision 1.1  1993/12/17  08:45:23  john
 * Initial revision
 * 
 * 
 */



#ifndef _MEDMISC_H
#define _MEDMISC_H

void GetMouseRotation( int idx, int idy, vms_matrix * RotMat );
extern int Gameview_lockstep;                         //In medmisc.c
int ToggleLockstep();
int medlisp_delete_segment(void);
int medlisp_scale_segment(void);
int medlisp_rotate_segment(void);
int ToggleLockViewToCursegp(void);
int ToggleDrawAllSegments();
int IncreaseDrawDepth(void);
int DecreaseDrawDepth(void);
int ToggleCoordAxes();
extern int    Big_depth;
void set_chase_matrix(segment *sp);
void set_view_target_from_segment(segment *sp);


#endif
