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
 * Defn'tns for medmisc.c
 *
 */

#pragma once

#ifdef __cplusplus
#include "fwd-segment.h"
#include "fwd-vecmat.h"

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
#ifdef DXX_BUILD_DESCENT
void set_view_target_from_segment(fvcvertptr &, const shared_segment &sp);
#endif

#endif
