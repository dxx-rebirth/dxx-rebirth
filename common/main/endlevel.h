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
 * Header for endlevel.c
 *
 */

#pragma once

#ifdef __cplusplus
#include <array>
#include "vecmat.h"
#include "fwd-segment.h"
#include "gr.h"
#include "fwd-window.h"

namespace dcx {

struct d_unique_endlevel_state
{
	using starfield_type = std::array<vms_vector, 500>;
	starfield_type stars;
};

}

extern int Endlevel_sequence;
#ifdef dsx
namespace dsx {
window_event_result do_endlevel_frame();
}
#endif
window_event_result stop_endlevel_sequence();
#ifdef dsx
namespace dsx {
window_event_result start_endlevel_sequence();
}
#endif
void render_endlevel_frame(grs_canvas &, fix eye_offset);

void draw_exit_model(grs_canvas &);
void free_endlevel_data();

extern grs_bitmap *terrain_bitmap;  //*satellite_bitmap,*station_bitmap,
extern segnum_t exit_segnum;

//@@extern vms_vector mine_exit_point;
//@@extern object external_explosion;
//@@extern int ext_expl_playing;

//called for each level to load & setup the exit sequence
#ifdef dsx
namespace dsx {
void load_endlevel_data(int level_num);

}
#endif
extern unsigned exit_modelnum, destroyed_exit_modelnum;
extern vms_matrix surface_orient;

#endif
