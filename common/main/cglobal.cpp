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
 * Global constants for main directory
 */

#include "segment.h"

namespace dcx {
//	Translate table to get opposite side of a face on a segment.
constexpr per_side_array<sidenum_t> Side_opposite{{{
	sidenum_t::WRIGHT,
	sidenum_t::WBOTTOM,
	sidenum_t::WLEFT,
	sidenum_t::WTOP,
	sidenum_t::WFRONT,
	sidenum_t::WBACK
}}};

const per_side_array<enumerated_array<segment_relative_vertnum, 4, side_relative_vertnum>> Side_to_verts{{{
	{{{segment_relative_vertnum::_7, segment_relative_vertnum::_6, segment_relative_vertnum::_2, segment_relative_vertnum::_3}}}, 			// left
	{{{segment_relative_vertnum::_0, segment_relative_vertnum::_4, segment_relative_vertnum::_7, segment_relative_vertnum::_3}}}, 			// top
	{{{segment_relative_vertnum::_0, segment_relative_vertnum::_1, segment_relative_vertnum::_5, segment_relative_vertnum::_4}}}, 			// right
	{{{segment_relative_vertnum::_2, segment_relative_vertnum::_6, segment_relative_vertnum::_5, segment_relative_vertnum::_1}}}, 			// bottom
	{{{segment_relative_vertnum::_4, segment_relative_vertnum::_5, segment_relative_vertnum::_6, segment_relative_vertnum::_7}}}, 			// back
	{{{segment_relative_vertnum::_3, segment_relative_vertnum::_2, segment_relative_vertnum::_1, segment_relative_vertnum::_0}}}, 			// front
}}};

}
