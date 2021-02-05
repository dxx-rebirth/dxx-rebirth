/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 *
 * Functions for moving segments.
 *
 */

#include "editor.h"
#include "editor/esegment.h"
#include "kdefs.h"

static int SegOrientCommon(fixang *ang, fix val)
{
	Seg_orientation.p = 0;
	Seg_orientation.b = 0;
	Seg_orientation.h = 0;

	*ang += val;
	rotate_segment_new(Seg_orientation);
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
