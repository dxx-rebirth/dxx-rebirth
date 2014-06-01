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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Prototypes for auto-map stuff.
 *
 */

#ifndef _AUTOMAP_H
#define _AUTOMAP_H

#include "pstypes.h"
#include "segment.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "compiler-array.h"

extern int Automap_active;

extern char Marker_input[40];
extern void do_automap(int key_code);
extern void automap_clear_visited();
extern array<ubyte, MAX_SEGMENTS> Automap_visited;

#if defined(DXX_BUILD_DESCENT_II)
struct object;
struct vms_vector;

void DropBuddyMarker(object *objp);
void InitMarkerInput();
int MarkerInputMessage(int key);

#define NUM_MARKERS         16
#define MARKER_MESSAGE_LEN  40

extern char MarkerMessage[NUM_MARKERS][MARKER_MESSAGE_LEN];
extern int  MarkerObject[NUM_MARKERS];
extern vms_vector MarkerPoint[NUM_MARKERS];
extern ubyte DefiningMarkerMessage;
#endif

#endif

#endif
