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
 * $Source: /cvs/cvsroot/d2x/main/editor/ksegmove.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions for moving segments.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:29  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:33:37  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.5  1993/12/02  12:39:36  matt
 * Removed extra includes
 * 
 * Revision 1.4  1993/11/12  16:40:23  mike
 * Use rotate_segment_new in place of med_rotate_segment_ang.
 * 
 * Revision 1.3  1993/11/05  17:32:54  john
 * added funcs
 * .,
 * 
 * Revision 1.2  1993/10/26  11:28:41  mike
 * Write common routine SegOrientCommon so all movement can pass
 * through the same routine to check for concavity, among other things.
 * 
 * Revision 1.1  1993/10/13  18:53:21  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: ksegmove.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

//#include <stdio.h>
//#include <stdlib.h>
//#include <math.h>
//#include <string.h>

#include "inferno.h"
#include "editor.h"

// -- old -- int SegOrientCommon(fixang *ang, fix val)
// -- old -- {
// -- old -- 	*ang += val;
// -- old -- 	med_rotate_segment_ang(Cursegp,&Seg_orientation);
// -- old -- 	Update_flags |= UF_WORLD_CHANGED;
// -- old -- 	mine_changed = 1;
// -- old -- 	warn_if_concave_segment(Cursegp);
// -- old -- 	return 1;
// -- old -- }

int SegOrientCommon(fixang *ang, fix val)
{
	Seg_orientation.p = 0;
	Seg_orientation.b = 0;
	Seg_orientation.h = 0;

	*ang += val;
	rotate_segment_new(&Seg_orientation);
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	warn_if_concave_segment(Cursegp);
	return 1;
}

// ---------- segment orientation control ----------

int DecreaseHeading()
{
	// decrease heading
	return SegOrientCommon(&Seg_orientation.h,-512);
}

int IncreaseHeading()
{
	return SegOrientCommon(&Seg_orientation.h,+512);
}

int DecreasePitch()
{
	return SegOrientCommon(&Seg_orientation.p,-512);
}

int IncreasePitch()
{
	return SegOrientCommon(&Seg_orientation.p,+512);
}

int DecreaseBank()
{
	return SegOrientCommon(&Seg_orientation.b,-512);
}

int IncreaseBank()
{
	return SegOrientCommon(&Seg_orientation.b,+512);
}
