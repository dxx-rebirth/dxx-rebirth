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
 * Routines stripped from med.c for segment selection
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "gr.h"
#include "ui.h"
#include "u_mem.h"
#include "inferno.h"
#include "gameseg.h"
#include "editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "segment.h"
#include "object.h"
#include "medsel.h"
#include "kdefs.h"

#include "dxxsconf.h"
#include "compiler-range_for.h"

//find the distance between a segment and a point
static fix compute_dist(const shared_segment &seg, const vms_vector &pos)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto delta = compute_segment_center(vcvertptr, seg);
	vm_vec_sub2(delta,pos);

	return vm_vec_mag(delta);

}

void sort_seg_list(count_segment_array_t &segnumlist,const vms_vector &pos)
{
	std::array<fix, MAX_SEGMENTS> dist;
	range_for (const auto &ss, segnumlist)
		dist[ss] = compute_dist(vcsegptr(ss), pos);
	auto predicate = [&dist](count_segment_array_t::const_reference a, count_segment_array_t::const_reference b) {
		return dist[a] < dist[b];
	};
	std::sort(segnumlist.begin(), segnumlist.end(), predicate);
}

int SortSelectedList(void)
{
	sort_seg_list(Selected_segs,ConsoleObject->pos);
	editor_status_fmt("%i element selected list sorted.", Selected_segs.size());

	return 1;
}

int SelectNextFoundSeg(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (++Found_seg_index >= Found_segs.size())
		Found_seg_index = 0;

	Cursegp = imsegptridx(Found_segs[Found_seg_index]);
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}

	editor_status("Curseg assigned to next found segment.");

	return 1;
}

int SelectPreviousFoundSeg(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (Found_seg_index > 0)
		Found_seg_index--;
	else
		Found_seg_index = Found_segs.size() - 1;

	Cursegp = imsegptridx(Found_segs[Found_seg_index]);
	med_create_new_segment_from_cursegp();

	Update_flags |= UF_WORLD_CHANGED;

	if (Lock_view_to_cursegp)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}

	editor_status("Curseg assigned to previous found segment.");

	return 1;
}

