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
 * AI path forming stuff.
 *
 */

#include <numeric>
#include <random>
#include <stdio.h>		//	for printf()
#include <stdlib.h>		// for d_rand() and qsort()
#include <string.h>		// for memset()

#include "inferno.h"
#include "console.h"
#include "3d.h"

#include "object.h"
#include "dxxerror.h"
#include "ai.h"
#include "robot.h"
#include "fvi.h"
#include "gameseg.h"
#include "physics.h"
#include "wall.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif
#include "player.h"
#include "game.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include "d_levelstate.h"

//	Length in segments of avoidance path
#define	AVOID_SEG_LENGTH	7

#ifdef NDEBUG
#define	PATH_VALIDATION	0
#else
#define	PATH_VALIDATION	1
#endif

namespace dsx {
namespace {
static void ai_path_set_orient_and_vel(const d_robot_info_array &Robot_info, object &objp, const vms_vector &goal_point
#if defined(DXX_BUILD_DESCENT_II)
								, player_visibility_state player_visibility, const vms_vector *vec_to_player
#endif
								);
static void maybe_ai_path_garbage_collect(void);
static void ai_path_garbage_collect(void);
#if PATH_VALIDATION
static void validate_all_paths();
static int validate_path(int, point_seg* psegs, uint_fast32_t num_points);
#endif

//	-----------------------------------------------------------------------------------------------------------
//	Insert the point at the center of the side connecting two segments between the two points.
// This is messy because we must insert into the list.  The simplest (and not too slow) way to do this is to start
// at the end of the list and go backwards.
static uint_fast32_t insert_center_points(segment_array &segments, point_seg *psegs, uint_fast32_t count)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcsegptr = segments.vcptr;
	auto &vcsegptridx = segments.vcptridx;
	if (count < 2)
		return count;
	uint_fast32_t last_point = count - 1;
	for (uint_fast32_t i = last_point; i; --i)
	{
		psegs[2*i] = psegs[i];
		const auto &&seg1 = vcsegptr(psegs[i-1].segnum);
		auto connect_side = find_connect_side(vcsegptridx(psegs[i].segnum), seg1);
		Assert(connect_side != side_none);	//	Impossible!  These two segments must be connected, they were created by create_path_points (which was created by mk!)
		if (connect_side == side_none)			//	Try to blow past the assert, this should at least prevent a hang.
			connect_side = sidenum_t::WLEFT;
		auto &vcvertptr = Vertices.vcptr;
		const auto &&center_point = compute_center_point_on_side(vcvertptr, seg1, connect_side);
		auto new_point = vm_vec_sub(psegs[i-1].point, center_point);
		new_point.x /= 16;
		new_point.y /= 16;
		new_point.z /= 16;
		vm_vec_sub(psegs[2*i-1].point, center_point, new_point);
#if defined(DXX_BUILD_DESCENT_II)
		const auto &&segp = segments.imptridx(psegs[2*i].segnum);
		const auto &&temp_segnum = find_point_seg(LevelSharedSegmentState, psegs[2*i-1].point, segp);
		if (temp_segnum == segment_none) {
			psegs[2*i-1].point = center_point;
			find_point_seg(LevelSharedSegmentState, psegs[2*i-1].point, segp);
		}
#endif

		psegs[2*i-1].segnum = psegs[2*i].segnum;
		count++;
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Now, remove unnecessary center points.
	//	A center point is unnecessary if it is close to the line between the two adjacent points.
	//	MK, OPTIMIZE!  Can get away with about half the math since every vector gets computed twice.
	for (uint_fast32_t i = 1; i < count - 1; i += 2)
	{
		vms_vector	temp1, temp2;
		const auto dot = vm_vec_dot(vm_vec_sub(temp1, psegs[i].point, psegs[i-1].point), vm_vec_sub(temp2, psegs[i+1].point, psegs[i].point));
		if (dot * 9/8 > fixmul(vm_vec_mag(temp1), vm_vec_mag(temp2)))
			psegs[i].segnum = segment_none;
	}

	//	Now, scan for points with segnum == -1
	auto predicate = [](const point_seg &p) { return p.segnum == segment_none; };
	count = std::distance(psegs, std::remove_if(psegs, psegs + count, predicate));
#endif
	return count;
}

#if defined(DXX_BUILD_DESCENT_II)
//	-----------------------------------------------------------------------------------------------------------
//	Move points halfway to outside of segment.
static void move_towards_outside(const d_level_shared_segment_state &LevelSharedSegmentState, point_seg *const psegs, const unsigned num_points, const vmobjptridx_t objp, const create_path_random_flag rand_flag)
{
	int	i;
	std::array<point_seg, 200> new_psegs;
	assert(num_points < new_psegs.size());

	auto &Segments = LevelSharedSegmentState.get_segments();
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	for (i = 1; i < num_points - 1; ++i)
	{
		fix			segment_size;
		segnum_t			segnum;
		vms_vector	e;
		int			count;
		const auto &&temp_segnum = find_point_seg(LevelSharedSegmentState, psegs[i].point, Segments.vcptridx(psegs[i].segnum));
		Assert(temp_segnum != segment_none);
		psegs[i].segnum = temp_segnum;
		segnum = psegs[i].segnum;

		//	I don't think we can use quick version here and this is _very_ rarely called. --MK, 07/03/95
		const auto a = vm_vec_normalized_quick(vm_vec_sub(psegs[i].point, psegs[i-1].point));
		const auto b = vm_vec_normalized_quick(vm_vec_sub(psegs[i+1].point, psegs[i].point));
		const auto c = vm_vec_sub(psegs[i+1].point, psegs[i-1].point);
		if (abs(vm_vec_dot(a, b)) > 3*F1_0/4 ) {
			if (abs(a.z) < F1_0/2) {
				if (rand_flag != create_path_random_flag::nonrandom)
				{
					e.x = (d_rand()-16384)/2;
					e.y = (d_rand()-16384)/2;
					e.z = abs(e.x) + abs(e.y) + 1;
					vm_vec_normalize_quick(e);
				} else {
					e.x = 0;
					e.y = 0;
					e.z = F1_0;
				}
			} else {
				if (rand_flag != create_path_random_flag::nonrandom)
				{
					e.y = (d_rand()-16384)/2;
					e.z = (d_rand()-16384)/2;
					e.x = abs(e.y) + abs(e.z) + 1;
					vm_vec_normalize_quick(e);
				} else {
					e.x = F1_0;
					e.y = 0;
					e.z = 0;
				}
			}
		} else {
			const auto d{vm_vec_cross(a, b)};
			e = vm_vec_cross(c, d);
			vm_vec_normalize_quick(e);
		}

		if (vm_vec_mag_quick(e) < F1_0/2)
	Int3();

		const shared_segment &segp = *vcsegptr(segnum);
		segment_size = vm_vec_dist_quick(vcvertptr(segp.verts[segment_relative_vertnum::_0]), vcvertptr(segp.verts[segment_relative_vertnum::_6]));
		if (segment_size > F1_0*40)
			segment_size = F1_0*40;

		auto goal_pos = vm_vec_scale_add(psegs[i].point, e, segment_size/4);

		count = 3;
		while (count) {
			fvi_info		hit_data;
			auto &p0 = psegs[i].point;
			const auto hit_type = find_vector_intersection(fvi_query{
				p0,
				goal_pos,
				fvi_query::unused_ignore_obj_list,
				fvi_query::unused_LevelUniqueObjectState,
				fvi_query::unused_Robot_info,
				0,
				objp,
			}, psegs[i].segnum, objp->size, hit_data);
	
			if (hit_type == fvi_hit_type::None)
				count = 0;
			else {
				if (count == 3 && hit_type == fvi_hit_type::BadP0)
					Int3();
				goal_pos.x = (p0.x + hit_data.hit_pnt.x)/2;
				goal_pos.y = (p0.y + hit_data.hit_pnt.y)/2;
				goal_pos.z = (p0.z + hit_data.hit_pnt.z)/2;
				count--;
				if (count == 0) {	//	Couldn't move towards outside, that's ok, sometimes things can't be moved.
					goal_pos = psegs[i].point;
				}
			}
		}

		//	Only move towards outside if remained inside segment.
		const auto &&new_segnum = find_point_seg(LevelSharedSegmentState, goal_pos, Segments.vcptridx(psegs[i].segnum));
		if (new_segnum == psegs[i].segnum) {
			new_psegs[i].point = goal_pos;
			new_psegs[i].segnum = new_segnum;
		} else {
			new_psegs[i].point = psegs[i].point;
			new_psegs[i].segnum = psegs[i].segnum;
		}

	}

	for (i = 1; i < num_points - 1; ++i)
		psegs[i] = new_psegs[i];
}
#endif

}

//	-----------------------------------------------------------------------------------------------------------
//	Create a path from objp->pos to the center of end_seg.
//	Return a list of (segment_num, point_locations) at psegs
//	Return number of points in *num_points.
//	if max_depth == -1, then there is no maximum depth.
//	If unable to create path, return -1, else return 0.
//	If random_flag !0, then introduce randomness into path by looking at sides in random order.  This means
//	that a path between two segments won't always be the same, unless it is unique.
//	If safety_flag is set, then additional points are added to "make sure" that points are reachable.  I would
//	like to say that it ensures that the object can move between the points, but that would require knowing what
//	the object is (which isn't passed, right?) and making fvi calls (slow, right?).  So, consider it the more_or_less_safe_flag.
//	If end_seg == -2, then end seg will never be found and this routine will drop out due to depth (probably called by create_n_segment_path).
std::pair<create_path_result, unsigned> create_path_points(const vmobjptridx_t objp, const robot_info *const robptr, const vcsegidx_t start_seg, icsegidx_t end_seg, point_seg_array_t::iterator psegs, const unsigned max_depth, create_path_random_flag random_flag, const create_path_safety_flag safety_flag, icsegidx_t avoid_seg)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
#endif
	segnum_t		cur_seg;
	int		qtail = 0, qhead = 0;
	int		i;
	std::array<seg_seg, MAX_SEGMENTS> seg_queue;
	int		cur_depth;
	point_seg_array_t::iterator	original_psegs = psegs;
	unsigned l_num_points = 0;

#if PATH_VALIDATION
	validate_all_paths();
#endif

	auto &obj = *objp;
	if (obj.type == OBJ_ROBOT && obj.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM)
	{
	random_flag = create_path_random_flag::random;
	avoid_seg = ConsoleObject->segnum;
	// Int3();
}

	visited_segment_bitarray_t visited;
	std::array<uint16_t, MAX_SEGMENTS> depth{};

	//	If there is a segment we're not allowed to visit, mark it.
	if (avoid_seg != segment_none) {
		Assert(avoid_seg <= Highest_segment_index);
		if ((start_seg != avoid_seg) && (end_seg != avoid_seg)) {
			visited[avoid_seg] = true;
		}
	}

	cur_seg = start_seg;
	visited[cur_seg] = true;
	cur_depth = 0;

	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;

	per_side_array<sidenum_t> side_traversal_translation;
	std::iota(side_traversal_translation.begin(), side_traversal_translation.end(), sidenum_t{});
#if defined(DXX_BUILD_DESCENT_I)
	/* Descent 1 can only shuffle once, before the loop begins.
	 * Descent 2 can shuffle on every pass of the loop.
	 */
	if (random_flag != create_path_random_flag::nonrandom)
		std::shuffle(side_traversal_translation.begin(), side_traversal_translation.end(), std::minstd_rand(d_rand()));
#elif defined(DXX_BUILD_DESCENT_II)
	/* If shuffling is enabled, always shuffle on the first pass of the
	 * loop.
	 */
	unsigned shuffle_random_flag = 0;
	std::minstd_rand mrd((random_flag != create_path_random_flag::nonrandom)
		? d_rand()
		: std::minstd_rand::default_seed
	);
	std::uniform_int_distribution uid03(0, 3);
#endif
	while (cur_seg != end_seg) {
		const cscusegment &&segp = vcsegptr(cur_seg);
#if defined(DXX_BUILD_DESCENT_II)
		/* If path randomness is enabled, conditionally shuffle based on
		 * the value of shuffle_random_flag.  Since shuffle_random_flag
		 * is initialized to 0 above, it is guaranteed to shuffle on the
		 * first iteration of the while loop.  This is done since
		 * side_traversal_translation was initialized via std::iota, and
		 * is therefore predictable.  However, if path randomness is
		 * enabled by the caller, side_traversal_translation should be
		 * unpredictable, and therefore a shuffle is needed.
		 *
		 * Each subsequent iteration may or may not shuffle, depending
		 * on the value from d_rand in the most recent loop.  Regardless
		 * of whether a pass shuffles, it will reload the value so that
		 * the next pass may shuffle.  This is necessary to avoid
		 * pinning the value in no-shuffle mode after the first time
		 * d_rand returns a no-shuffle value.
		 */
		if (random_flag != create_path_random_flag::nonrandom && std::exchange(shuffle_random_flag, uid03(mrd)) == 0)
		{
			std::shuffle(side_traversal_translation.begin(), side_traversal_translation.end(), mrd);
		}
#endif

		for (const auto snum : side_traversal_translation)
		{
			if (!IS_CHILD(segp.s.children[snum]))
				continue;
#if defined(DXX_BUILD_DESCENT_I)
#define AI_DOOR_OPENABLE_PLAYER_FLAGS
#elif defined(DXX_BUILD_DESCENT_II)
#define AI_DOOR_OPENABLE_PLAYER_FLAGS	player_info.powerup_flags,
			auto &player_info = get_local_plrobj().ctype.player_info;
#endif
			if ((WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, snum) & WALL_IS_DOORWAY_FLAG::fly) || ai_door_is_openable(obj, robptr, AI_DOOR_OPENABLE_PLAYER_FLAGS segp, snum))
#undef AI_DOOR_OPENABLE_PLAYER_FLAGS
			{
				const auto this_seg = segp.s.children[snum];
#if defined(DXX_BUILD_DESCENT_II)
				Assert(this_seg != segment_none);
				if (((cur_seg == avoid_seg) || (this_seg == avoid_seg)) && (ConsoleObject->segnum == avoid_seg)) {
					const auto &&center_point = compute_center_point_on_side(vcvertptr, segp, snum);
					fvi_info		hit_data;
	
					const auto hit_type = find_vector_intersection(fvi_query{
						obj.pos,
						center_point,
						fvi_query::unused_ignore_obj_list,
						fvi_query::unused_LevelUniqueObjectState,
						fvi_query::unused_Robot_info,
						0,
						objp,
					}, obj.segnum, obj.size, hit_data);
					if (hit_type != fvi_hit_type::None)
					{
						goto dont_add;
					}
				}
#endif

				if (!visited[this_seg]) {
					seg_queue[qtail].start = cur_seg;
					seg_queue[qtail].end = this_seg;
					visited[this_seg] = true;
					depth[qtail++] = cur_depth+1;
					if (depth[qtail-1] == max_depth) {
						end_seg = seg_queue[qtail-1].end;
						goto cpp_done1;
					}	// end if (depth[...
				}	// end if (!visited...
			}	// if (WALL_IS_DOORWAY(...
#if defined(DXX_BUILD_DESCENT_II)
dont_add: ;
#endif
		}

		if (qtail <= 0)
			break;

		if (qhead >= qtail) {
			//	Couldn't get to goal, return a path as far as we got, which probably acceptable to the unparticular caller.
			end_seg = seg_queue[qtail-1].end;
			break;
		}

		cur_seg = seg_queue[qhead].end;
		cur_depth = depth[qhead];
		qhead++;

cpp_done1: ;
	}	//	while (cur_seg ...

	if (qtail > 0)
	{
		//	Set qtail to the segment which ends at the goal.
		while (seg_queue[--qtail].end != end_seg)
			if (qtail < 0) {
				return std::make_pair(create_path_result::early, l_num_points);
			}
	}
	else
		qtail = -1;

#if defined(DXX_BUILD_DESCENT_I)
#if DXX_USE_EDITOR
	Selected_segs.clear();
	#endif
#endif

	while (qtail >= 0) {
		segnum_t	parent_seg, this_seg;

		this_seg = seg_queue[qtail].end;
		parent_seg = seg_queue[qtail].start;
		psegs->segnum = this_seg;
		compute_segment_center(vcvertptr, psegs->point, vcsegptr(this_seg));
		psegs++;
		l_num_points++;
#if defined(DXX_BUILD_DESCENT_I)
#if DXX_USE_EDITOR
		Selected_segs.emplace_back(this_seg);
		#endif
#endif

		if (parent_seg == start_seg)
			break;

		while (seg_queue[--qtail].end != parent_seg)
			Assert(qtail >= 0);
	}

	psegs->segnum = start_seg;
	compute_segment_center(vcvertptr, psegs->point, vcsegptr(start_seg));
	psegs++;
	l_num_points++;

#if PATH_VALIDATION
	validate_path(1, original_psegs, l_num_points);
#endif

	//	Now, reverse point_segs in place.
	for (i=0; i< l_num_points/2; i++) {
		point_seg		temp_point_seg = *(original_psegs + i);
		*(original_psegs + i) = *(original_psegs + l_num_points - i - 1);
		*(original_psegs + l_num_points - i - 1) = temp_point_seg;
	}
#if PATH_VALIDATION
	validate_path(2, original_psegs, l_num_points);
#endif

	//	Now, if safety_flag set, then insert the point at the center of the side connecting two segments
	//	between the two points.  This is messy because we must insert into the list.  The simplest (and not too slow)
	//	way to do this is to start at the end of the list and go backwards.
	if (safety_flag != create_path_safety_flag::unsafe) {
		if (psegs - Point_segs + l_num_points + 2 > MAX_POINT_SEGS) {
			//	Ouch!  Cannot insert center points in path.  So return unsafe path.
			ai_reset_all_paths();
			return std::make_pair(create_path_result::early, l_num_points);
		} else {
			l_num_points = insert_center_points(Segments, original_psegs, l_num_points);
		}
	}

#if PATH_VALIDATION
	validate_path(3, original_psegs, l_num_points);
#endif

#if defined(DXX_BUILD_DESCENT_II)
// -- MK, 10/30/95 -- This code causes apparent discontinuities in the path, moving a point
//	into a new segment.  It is not necessarily bad, but it makes it hard to track down actual
//	discontinuity problems.
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (obj.type == OBJ_ROBOT)
		if (Robot_info[get_robot_id(obj)].companion)
			move_towards_outside(LevelSharedSegmentState, original_psegs, l_num_points, objp, create_path_random_flag::nonrandom);
#endif

#if PATH_VALIDATION
	validate_path(4, original_psegs, l_num_points);
#endif
	return std::make_pair(create_path_result::finished, l_num_points);
}

#if defined(DXX_BUILD_DESCENT_II)
//	-------------------------------------------------------------------------------------------------------
//	polish_path
//	Takes an existing path and makes it nicer.
//	Drops as many leading points as possible still maintaining direct accessibility
//	from current position to first point.
//	Will not shorten path to fewer than 3 points.
//	Returns number of points.
//	Starting position in psegs doesn't change.
//	Changed, MK, 10/18/95.  I think this was causing robots to get hung up on walls.
//				Only drop up to the first three points.
int polish_path(const vmobjptridx_t objp, point_seg *psegs, int num_points)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	int	i, first_point=0;

	if (num_points <= 4)
		return num_points;

	//	Prevent the buddy from polishing his path twice in one tick, which can cause him to get hung up.  Pretty ugly, huh?
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &obj = *objp;
	if (Robot_info[get_robot_id(obj)].companion)
	{
		if (d_tick_count == BuddyState.Last_buddy_polish_path_tick)
			return num_points;
		else
			BuddyState.Last_buddy_polish_path_tick = d_tick_count;
	}

	// -- MK: 10/18/95: for (i=0; i<num_points-3; i++)
	for (i=0; i<2; i++) {
		fvi_info		hit_data;
		const auto hit_type = find_vector_intersection(fvi_query{
			obj.pos,
			psegs[i].point,
			fvi_query::unused_ignore_obj_list,
			fvi_query::unused_LevelUniqueObjectState,
			fvi_query::unused_Robot_info,
			0,
			objp,
		}, obj.segnum, obj.size, hit_data);
	
		if (hit_type == fvi_hit_type::None)
			first_point = i+1;
		else
			break;		
	}

	if (first_point) {
		//	Scrunch down all the psegs.
		for (i=first_point; i<num_points; i++)
			psegs[i-first_point] = psegs[i];
	}

	return num_points - first_point;
}
#endif

#ifndef NDEBUG
namespace {
//	-------------------------------------------------------------------------------------------------------
//	Make sure that there are connections between all segments on path.
//	Note that if path has been optimized, connections may not be direct, so this function is useless, or worse.
//	Return true if valid, else return false.
int validate_path(int, point_seg *psegs, uint_fast32_t num_points)
{
#if PATH_VALIDATION
	auto curseg = psegs->segnum;
	if (curseg > Highest_segment_index)
	{
		Int3();		//	Contact Mike: Debug trap for elusive, nasty bug.
		return 0;
	}

	range_for (const auto &ps, unchecked_partial_range(psegs, 1u, num_points))
	{
		auto nextseg = ps.segnum;
		if (curseg != nextseg) {
			const shared_segment &csegp = vcsegptr(curseg);
			auto &children = csegp.children;
			if (std::find(children.begin(), children.end(), nextseg) == children.end())
			{
			// Assert(sidenum != MAX_SIDES_PER_SEGMENT);	//	Hey, created path is not contiguous, why!?
				Int3();
				return 0;
			}
			curseg = nextseg;
		}
	}
#endif
	return 1;

}

//	-----------------------------------------------------------------------------------------------------------
void validate_all_paths(void)
{

#if PATH_VALIDATION
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	range_for (const auto &&objp, vmobjptr)
	{
		auto &obj = *objp;
		if (obj.type == OBJ_ROBOT) {
			auto &aip = obj.ctype.ai_info;
			if (obj.control_source == object::control_type::ai)
			{
				if ((aip.hide_index != -1) && (aip.path_length > 0))
					if (!validate_path(4, &Point_segs[aip.hide_index], aip.path_length))
					{
						Int3();	//	This path is bogus!  Who corrupted it!  Danger! Danger!
									//	Contact Mike, he caused this mess.
						aip.path_length=0;	//	This allows people to resume without harm...
					}
			}
		}
	}
#endif
}

}
#endif

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment.
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
void create_path_to_segment(const vmobjptridx_t objp, const robot_info &robptr, const unsigned max_length, const create_path_safety_flag safety_flag, const icsegidx_t goal_segment)
{
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;
	ai_local *const ailp = &obj.ctype.ai_info.ail;

	ailp->time_player_seen = {GameTime64};			//	Prevent from resetting path quickly.
	ailp->goal_segment = goal_segment;

	segnum_t			start_seg;
	start_seg = obj.segnum;
	const auto end_seg = goal_segment;

	if (end_seg == segment_none) {
		;
	} else {
		aip->path_length = create_path_points(objp, &robptr, start_seg, end_seg, Point_segs_free_ptr, max_length, create_path_random_flag::random, safety_flag, segment_none).second;
#if defined(DXX_BUILD_DESCENT_II)
		aip->path_length = polish_path(objp, Point_segs_free_ptr, aip->path_length);
#endif
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#if defined(DXX_BUILD_DESCENT_I)
#ifndef NDEBUG
		validate_path(6, Point_segs_free_ptr, aip->path_length);
#endif
#endif
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: This is stupid.  Should call maybe_ai_garbage_collect before the add.
			ai_reset_all_paths();
			return;
		}
//		Assert(Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
#if defined(DXX_BUILD_DESCENT_I)
		aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
#endif
		ailp->mode = ai_mode::AIM_FOLLOW_PATH;
		ailp->player_awareness_type = player_awareness_type_t::PA_NONE;		//	If robot too aware of player, will set mode to chase
	}
	maybe_ai_path_garbage_collect();
}

//	Change, 10/07/95: Used to create path to ConsoleObject->pos.  Now creates path to Believed_player_pos.
void create_path_to_believed_player_segment(const vmobjptridx_t objp, const robot_info &robptr, const unsigned max_length, const create_path_safety_flag safety_flag)
{
#if defined(DXX_BUILD_DESCENT_I)
	const auto goal_segment = ConsoleObject->segnum;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto goal_segment = Believed_player_seg;
#endif
	create_path_to_segment(objp, robptr, max_length, safety_flag, goal_segment);
}

#if defined(DXX_BUILD_DESCENT_II)
void create_path_to_guidebot_player_segment(const vmobjptridx_t objp, const robot_info &robptr, const unsigned max_length, const create_path_safety_flag safety_flag)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		return;
	auto &plrobj = *Objects.vcptr(plr.objnum);
	const auto goal_segment = plrobj.segnum;
	create_path_to_segment(objp, robptr, max_length, safety_flag, goal_segment);
}
//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the object's current segment (objp->segnum) to segment goalseg.
void create_path_to_segment(const vmobjptridx_t objp, const robot_info &robptr, segnum_t goalseg, const unsigned max_length, const create_path_safety_flag safety_flag)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	ailp->time_player_seen = {GameTime64};			//	Prevent from resetting path quickly.
	ailp->goal_segment = goalseg;

	segnum_t			start_seg, end_seg;
	start_seg = objp->segnum;
	end_seg = ailp->goal_segment;

	if (end_seg == segment_none) {
		;
	} else {
		aip->path_length = create_path_points(objp, &robptr, start_seg, end_seg, Point_segs_free_ptr, max_length, create_path_random_flag::random, safety_flag, segment_none).second;
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			ai_reset_all_paths();
			return;
		}

		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// -- UNUSED! aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->player_awareness_type = player_awareness_type_t::PA_NONE;		//	If robot too aware of player, will set mode to chase
	}

	maybe_ai_path_garbage_collect();

}
#endif

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
void create_path_to_station(const vmobjptridx_t objp, const robot_info &robptr, int max_length)
{
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;
	ai_local *const ailp = &obj.ctype.ai_info.ail;

	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = {GameTime64};			//	Prevent from resetting path quickly.

	segnum_t			start_seg, end_seg;
	start_seg = obj.segnum;
	end_seg = aip->hide_segment;

	if (end_seg == segment_none) {
		;
	} else {
		aip->path_length = create_path_points(objp, &robptr, start_seg, end_seg, Point_segs_free_ptr, max_length, create_path_random_flag::random, create_path_safety_flag::safe, segment_none).second;
#if defined(DXX_BUILD_DESCENT_II)
		aip->path_length = polish_path(objp, Point_segs_free_ptr, aip->path_length);
#endif
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#if defined(DXX_BUILD_DESCENT_I)
#ifndef NDEBUG
		validate_path(7, Point_segs_free_ptr, aip->path_length);
#endif
#endif
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: Stupid.
			ai_reset_all_paths();
			return;
		}
//		Assert(Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->mode = ai_mode::AIM_FOLLOW_PATH;
		ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
	}


	maybe_ai_path_garbage_collect();

}


//	-------------------------------------------------------------------------------------------------------
//	Create a path of length path_length for an object, stuffing info in ai_info field.
void create_n_segment_path(const vmobjptridx_t objp, const robot_info &robptr, unsigned path_length, const imsegidx_t avoid_seg)
{
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;
	ai_local *const ailp = &obj.ctype.ai_info.ail;

	const auto &&cr0 = create_path_points(objp, &robptr, obj.segnum, segment_exit, Point_segs_free_ptr, path_length, create_path_random_flag::random, create_path_safety_flag::unsafe, avoid_seg);
	aip->path_length = cr0.second;
	if (cr0.first == create_path_result::early)
	{
		Point_segs_free_ptr += aip->path_length;
		for (;;)
		{
			const auto &&crf = create_path_points(objp, &robptr, obj.segnum, segment_exit, Point_segs_free_ptr, --path_length, create_path_random_flag::random, create_path_safety_flag::unsafe, segment_none);
			aip->path_length = crf.second;
			if (crf.first != create_path_result::early)
				break;
		}
		assert(path_length);
	}

	aip->hide_index = Point_segs_free_ptr - Point_segs;
	aip->cur_path_index = 0;
#if PATH_VALIDATION
	validate_path(8, Point_segs_free_ptr, aip->path_length);
#endif
	Point_segs_free_ptr += aip->path_length;
	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		ai_reset_all_paths();
	}

	aip->PATH_DIR = 1;		//	Initialize to moving forward.
#if defined(DXX_BUILD_DESCENT_I)
	aip->SUBMODE = -1;		//	Don't know what this means.
#endif
	ailp->mode = ai_mode::AIM_FOLLOW_PATH;

#if defined(DXX_BUILD_DESCENT_II)
	//	If this robot is visible (player_visibility is not available) and it's running away, move towards outside with
	//	randomness to prevent a stream of bots from going away down the center of a corridor.
	if (player_is_visible(ailp->previous_visibility))
	{
		if (aip->path_length) {
			int	t_num_points = aip->path_length;
			move_towards_outside(LevelSharedSegmentState, &Point_segs[aip->hide_index], t_num_points, objp, create_path_random_flag::random);
			aip->path_length = t_num_points;
		}
	}
#endif

	maybe_ai_path_garbage_collect();

}

//	-------------------------------------------------------------------------------------------------------
void create_n_segment_path_to_door(const vmobjptridx_t objp, const robot_info &robptr, const unsigned path_length)
{
	create_n_segment_path(objp, robptr, path_length, segment_none);
}

#define Int3_if(cond) if (!cond) Int3();

// -- too much work -- //	----------------------------------------------------------------------------------------------------------
// -- too much work -- //	Return true if the object the companion wants to kill is reachable.
// -- too much work -- int attack_kill_object(object *objp)
// -- too much work -- {
// -- too much work -- 	object		*kill_objp;
// -- too much work -- 	fvi_info		hit_data;
// -- too much work -- 	int			fate;
// -- too much work -- 	fvi_query	fq;
// -- too much work --
// -- too much work -- 	if (Escort_kill_object == -1)
// -- too much work -- 		return 0;
// -- too much work --
// -- too much work -- 	kill_objp = &Objects[Escort_kill_object];
// -- too much work --
// -- too much work -- 	fq.p0						= &objp->pos;
// -- too much work -- 	fq.startseg				= objp->segnum;
// -- too much work -- 	fq.p1						= &kill_objp->pos;
// -- too much work -- 	fq.rad					= objp->size;
// -- too much work -- 	fq.thisobjnum			= objp-Objects;
// -- too much work -- 	fq.ignore_obj_list	= NULL;
// -- too much work -- 	fq.flags					= 0;
// -- too much work --
// -- too much work -- 	fate = find_vector_intersection(&fq,&hit_data);
// -- too much work --
// -- too much work -- 	if (fate == HIT_NONE)
// -- too much work -- 		return 1;
// -- too much work -- 	else
// -- too much work -- 		return 0;
// -- too much work -- }

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment.
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
#if defined(DXX_BUILD_DESCENT_I)
namespace {

static void create_path(const vmobjptridx_t objp, const robot_info &robptr)
{
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;

	const auto start_seg = obj.segnum;
	const auto end_seg = obj.ctype.ai_info.ail.goal_segment;

	if (end_seg == segment_none)
		create_n_segment_path(objp, robptr, 3, segment_none);

	if (end_seg == segment_none) {
		;
	} else {
		aip->path_length = create_path_points(objp, &robptr, start_seg, end_seg, Point_segs_free_ptr, MAX_PATH_LENGTH, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none).second;
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#ifndef NDEBUG
		validate_path(5, Point_segs_free_ptr, aip->path_length);
#endif
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
			ai_reset_all_paths();
		}
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		aip->SUBMODE = AISM_HIDING;		//	Pretend we are hiding, so we sit here until bothered.
	}

	maybe_ai_path_garbage_collect();
}
}
#endif


//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void ai_follow_path(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, const player_visibility_state player_visibility, const vms_vector *const vec_to_player)
{
	auto &obj = *objp;
	ai_static *const aip = &obj.ctype.ai_info;

	vms_vector	goal_point, new_goal_point;
#if defined(DXX_BUILD_DESCENT_II)
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
#endif
	auto &robptr = Robot_info[get_robot_id(obj)];
	int			forced_break, original_dir, original_index;
	ai_local *const ailp = &obj.ctype.ai_info.ail;

	if ((aip->hide_index == -1) || (aip->path_length == 0))
	{
		if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT) {
			create_n_segment_path(objp, robptr, 5, segment_none);
			//--Int3_if((aip->path_length != 0));
			ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;
		} else {
#if defined(DXX_BUILD_DESCENT_I)
			create_path(objp, robptr);
#elif defined(DXX_BUILD_DESCENT_II)
			create_n_segment_path(objp, robptr, 5, segment_none);
#endif
			//--Int3_if((aip->path_length != 0));
		}
	}

	if ((aip->hide_index + aip->path_length > Point_segs_free_ptr - Point_segs) && (aip->path_length>0)) {
#if defined(DXX_BUILD_DESCENT_II)
		Int3();	//	Contact Mike: Bad.  Path goes into what is believed to be free space.
		//	This is debugging code.  Figure out why garbage collection
		//	didn't compress this object's path information.
		ai_path_garbage_collect();
#endif
		ai_reset_all_paths();
	}

	if (aip->path_length < 2) {
#if defined(DXX_BUILD_DESCENT_I)
		if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT)
#elif defined(DXX_BUILD_DESCENT_II)
		if ((aip->behavior == ai_behavior::AIB_SNIPE) || (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT))
#endif
		{
			create_n_segment_path(objp, robptr, AVOID_SEG_LENGTH, ConsoleObject->segnum == obj.segnum ? segment_none : ConsoleObject->segnum);			//	Can't avoid segment player is in, robot is already in it! (That's what the -1 is for)
				//--Int3_if((aip->path_length != 0));
#if defined(DXX_BUILD_DESCENT_II)
			if (aip->behavior == ai_behavior::AIB_SNIPE) {
				if (robot_is_thief(robptr))
					ailp->mode = ai_mode::AIM_THIEF_ATTACK;	//	It gets bashed in create_n_segment_path
				else
					ailp->mode = ai_mode::AIM_SNIPE_FIRE;	//	It gets bashed in create_n_segment_path
			} else
#endif
			{
				ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
			}
		}
#if defined(DXX_BUILD_DESCENT_I)
		else {
			ailp->mode = ai_mode::AIM_STILL;
		}
		return;
#elif defined(DXX_BUILD_DESCENT_II)
		else if (robot_is_companion(robptr) == 0) {
			ailp->mode = ai_mode::AIM_STILL;
			aip->path_length = 0;
			return;
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_I)
	Assert((aip->PATH_DIR == -1) || (aip->PATH_DIR == 1));

	if ((aip->SUBMODE == AISM_HIDING) && (aip->behavior == ai_behavior::AIB_HIDE))
		return;
#endif

	goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;
	auto dist_to_goal = vm_vec_dist_quick(goal_point, obj.pos);

	//	If running from player, only run until can't be seen.
	if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT) {
		if (player_visibility == player_visibility_state::no_line_of_sight && ailp->player_awareness_type == player_awareness_type_t::PA_NONE)
		{
			fix	vel_scale;

			vel_scale = F1_0 - FrameTime/2;
			if (vel_scale < F1_0/2)
				vel_scale = F1_0/2;

			vm_vec_scale(obj.mtype.phys_info.velocity, vel_scale);

			return;
		} else
#if defined(DXX_BUILD_DESCENT_II)
			if (!(d_tick_count ^ ((objp) & 0x07)))
#endif
			{		//	Done 1/8 ticks.
			//	If player on path (beyond point robot is now at), then create a new path.
			point_seg	*curpsp = &Point_segs[aip->hide_index];
			auto player_segnum = ConsoleObject->segnum;
			int			i;

			//	This is probably being done every frame, which is wasteful.
			for (i=aip->cur_path_index; i<aip->path_length; i++) {
				if (curpsp[i].segnum == player_segnum) {
					create_n_segment_path(objp, robptr, AVOID_SEG_LENGTH, player_segnum != obj.segnum ? player_segnum : segment_none);
#if defined(DXX_BUILD_DESCENT_I)
					Assert(aip->path_length != 0);
#endif
					ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
					break;
				}
			}
			if (player_is_visible(player_visibility))
			{
				ailp->player_awareness_type = player_awareness_type_t::PA_NEARBY_ROBOT_FIRED;
				ailp->player_awareness_time = F1_0;
			}
		}
	}

	if (aip->cur_path_index < 0) {
		aip->cur_path_index = 0;
	} else if (aip->cur_path_index >= aip->path_length) {
		if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT) {
			create_n_segment_path(objp, robptr, AVOID_SEG_LENGTH, ConsoleObject->segnum);
			ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
#if defined(DXX_BUILD_DESCENT_II)
			Assert(aip->path_length != 0);
#endif
		} else {
			aip->cur_path_index = aip->path_length-1;
		}
	}

	goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	original_dir = aip->PATH_DIR;
	original_index = aip->cur_path_index;
	const vm_distance threshold_distance{fixmul(vm_vec_mag_quick(obj.mtype.phys_info.velocity), FrameTime)*2 + F1_0*2};

#if defined(DXX_BUILD_DESCENT_II)
	new_goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;

	//--Int3_if(((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length)));
#endif

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		//	Advance to next point on path.
		aip->cur_path_index += aip->PATH_DIR;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((aip->cur_path_index >= aip->path_length) || (aip->cur_path_index < 0)) {
#if defined(DXX_BUILD_DESCENT_II)
			//	Buddy bot.  If he's in mode to get away from player and at end of line,
			//	if player visible, then make a new path, else just return.
			if (robot_is_companion(robptr)) {
				if (BuddyState.Escort_special_goal == ESCORT_GOAL_SCRAM)
				{
					if (player_is_visible(player_visibility))
					{
						create_n_segment_path(objp, robptr, 16 + d_rand() * 16, segment_none);
						aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
						Assert(aip->path_length != 0);
						ailp->mode = ai_mode::AIM_WANDER;	//	Special buddy mode.
						//--Int3_if(((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length)));
						return;
					} else {
						ailp->mode = ai_mode::AIM_WANDER;	//	Special buddy mode.
						obj.mtype.phys_info.velocity = {};
						obj.mtype.phys_info.rotvel = {};
						//!!Assert((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length));
						return;
					}
				}
			}
#endif

#if defined(DXX_BUILD_DESCENT_I)
			if (ailp->mode == ai_mode::AIM_HIDE) {
				ailp->mode = ai_mode::AIM_STILL;
				return;		// Stay here until bonked or hit by player.
			}
#elif defined(DXX_BUILD_DESCENT_II)
			if (aip->behavior == ai_behavior::AIB_FOLLOW) {
				create_n_segment_path(objp, robptr, 10, ConsoleObject->segnum);
				//--Int3_if(((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length)));
			}
#endif
			else if (aip->behavior == ai_behavior::AIB_STATION) {
				create_path_to_station(objp, robptr, 15);
				if ((aip->hide_segment != Point_segs[aip->hide_index+aip->path_length-1].segnum)
#if defined(DXX_BUILD_DESCENT_II)
					|| (aip->path_length == 0)
#endif
					) {
					ailp->mode = ai_mode::AIM_STILL;
				}
				return;
			} else if (ailp->mode == ai_mode::AIM_FOLLOW_PATH
#if defined(DXX_BUILD_DESCENT_I)
					   && (aip->behavior != ai_behavior::AIB_FOLLOW_PATH)
#endif
					   ) {
				create_path_to_believed_player_segment(objp, robptr, 10, create_path_safety_flag::safe);
#if defined(DXX_BUILD_DESCENT_II)
				if (aip->hide_segment != Point_segs[aip->hide_index+aip->path_length-1].segnum) {
					ailp->mode = ai_mode::AIM_STILL;
					return;
				}
#endif
			} else if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT) {
				create_n_segment_path(objp, robptr, AVOID_SEG_LENGTH, ConsoleObject->segnum);
				ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
#if defined(DXX_BUILD_DESCENT_II)
				if (aip->path_length < 1) {
					create_n_segment_path(objp, robptr, AVOID_SEG_LENGTH, ConsoleObject->segnum);
					ailp->mode = ai_mode::AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
					if (aip->path_length < 1) {
						aip->behavior = ai_behavior::AIB_NORMAL;
						ailp->mode = ai_mode::AIM_STILL;
						return;
					}
				}
				//--Int3_if(((aip->cur_path_index >= 0) && (aip->cur_path_index < aip->path_length)));
#endif
			} else {
				//	Reached end of the line.  First see if opposite end point is reachable, and if so, go there.
				//	If not, turn around.
				int			opposite_end_index;
				fvi_info		hit_data;

				// See which end we're nearer and look at the opposite end point.
				if (abs(aip->cur_path_index - aip->path_length) < aip->cur_path_index) {
					//	Nearer to far end (ie, index not 0), so try to reach 0.
					opposite_end_index = 0;
				} else {
					//	Nearer to 0 end, so try to reach far end.
					opposite_end_index = aip->path_length-1;
				}

				const auto fate = find_vector_intersection(fvi_query{
					obj.pos,
					Point_segs[aip->hide_index + opposite_end_index].point,
					fvi_query::unused_ignore_obj_list,
					fvi_query::unused_LevelUniqueObjectState,
					fvi_query::unused_Robot_info,
					0, 				//what about trans walls???
					objp,
				}, obj.segnum, obj.size, hit_data);
				if (fate != fvi_hit_type::Wall)
				{
					//	We can be circular!  Do it!
					//	Path direction is unchanged.
					aip->cur_path_index = opposite_end_index;
				} else {
					aip->PATH_DIR = -aip->PATH_DIR;
#if defined(DXX_BUILD_DESCENT_II)
					aip->cur_path_index += aip->PATH_DIR;
#endif
				}
			}
			break;
		} else {
			new_goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;
			goal_point = new_goal_point;
			dist_to_goal = vm_vec_dist_quick(goal_point, obj.pos);
		}

		//	If went all the way around to original point, in same direction, then get out of here!
		if ((aip->cur_path_index == original_index) && (aip->PATH_DIR == original_dir)) {
			create_path_to_believed_player_segment(objp, robptr, 3, create_path_safety_flag::safe);
			forced_break = 1;
		}
	}	//	end while

	//	Set velocity (objp->mtype.phys_info.velocity) and orientation (objp->orient) for this object.
	ai_path_set_orient_and_vel(Robot_info, objp, goal_point
#if defined(DXX_BUILD_DESCENT_II)
							   , player_visibility, vec_to_player
#endif
							   );
}

}

namespace {

int	Last_tick_garbage_collected;

struct obj_path {
	short	path_start;
	objnum_t objnum;
};

static int path_index_compare(const void *const v1, const void *const v2)
{
	const auto i1 = reinterpret_cast<const obj_path *>(v1);
	const auto i2 = reinterpret_cast<const obj_path *>(v2);
	if (i1->path_start < i2->path_start)
		return -1;
	else if (i1->path_start == i2->path_start)
		return 0;
	else
		return 1;
}

}

namespace dsx {
namespace {

//	----------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objp based on its desire to get to a point.
void ai_path_set_orient_and_vel(const d_robot_info_array &Robot_info, object &objp, const vms_vector &goal_point
#if defined(DXX_BUILD_DESCENT_II)
								, const player_visibility_state player_visibility, const vms_vector *const vec_to_player
#endif
								)
{
	vms_vector	cur_vel = objp.mtype.phys_info.velocity;
	vms_vector	cur_pos = objp.pos;
	fix			speed_scale;
	fix			dot;
	auto &robptr = Robot_info[get_robot_id(objp)];
	fix			max_speed;

	//	If evading player, use highest difficulty level speed, plus something based on diff level
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	max_speed = robptr.max_speed[Difficulty_level];
	ai_local		*ailp = &objp.ctype.ai_info.ail;
	if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT
#if defined(DXX_BUILD_DESCENT_II)
		|| objp.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE
#endif
		)
		max_speed = max_speed*3/2;

	auto norm_vec_to_goal = vm_vec_normalized_quick(vm_vec_sub(goal_point, cur_pos));
	auto norm_cur_vel = vm_vec_normalized_quick(cur_vel);
	const auto norm_fvec = vm_vec_normalized_quick(objp.orient.fvec);

	dot = vm_vec_dot(norm_vec_to_goal, norm_fvec);

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
		norm_cur_vel.y += norm_vec_to_goal.y/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
		norm_cur_vel.z += norm_vec_to_goal.z/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
	}

	vm_vec_normalize_quick(norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= -4;

#if defined(DXX_BUILD_DESCENT_II)
	//	If in snipe mode, can move fast even if not facing that direction.
	if (objp.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
		if (dot < F1_0/2)
			dot = (dot + F1_0)/2;
#endif

	speed_scale = fixmul(max_speed, dot);
	vm_vec_scale(norm_cur_vel, speed_scale);
	objp.mtype.phys_info.velocity = norm_cur_vel;

	fix rate;
	if (ailp->mode == ai_mode::AIM_RUN_FROM_OBJECT
#if defined(DXX_BUILD_DESCENT_II)
		|| robot_is_companion(robptr) == 1 || objp.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE
#endif
		) {
#if defined(DXX_BUILD_DESCENT_II)
		if (ailp->mode == ai_mode::AIM_SNIPE_RETREAT_BACKWARDS) {
			if (player_is_visible(player_visibility) && vec_to_player)
				norm_vec_to_goal = *vec_to_player;
			else
				vm_vec_negate(norm_vec_to_goal);
		}
#endif
		rate = robptr.turn_time[Difficulty_level_type::_4] / 2;
	} else
		rate = robptr.turn_time[Difficulty_level];
	ai_turn_towards_vector(norm_vec_to_goal, objp, rate);
}

//	----------------------------------------------------------------------------------------------------------
//	Garbage colledion -- Free all unused records in Point_segs and compress all paths.
void ai_path_garbage_collect()
{
	int	free_path_index = 0;
	int	num_path_objects = 0;
	int	objind;
	obj_path		object_list[MAX_OBJECTS];

	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	auto &vmobjptridx = Objects.vmptridx;

	Last_tick_garbage_collected = d_tick_count;

#if PATH_VALIDATION
	validate_all_paths();
#endif
	//	Create a list of objects which have paths of length 1 or more.
	range_for (const auto &&objp, vcobjptridx)
	{
		if (objp->type == OBJ_ROBOT && (objp->control_source == object::control_type::ai
#if defined(DXX_BUILD_DESCENT_II)
			|| objp->control_source == object::control_type::morph
#endif
										  )) {
			const auto &aip = objp->ctype.ai_info;
			if (aip.path_length) {
				object_list[num_path_objects].path_start = aip.hide_index;
				object_list[num_path_objects++].objnum = objp;
			}
		}
	}

	qsort(object_list, num_path_objects, sizeof(object_list[0]),
			path_index_compare);

	for (objind=0; objind < num_path_objects; objind++) {
		ai_static	*aip;
		int			i;
		int			old_index;

		auto objnum = object_list[objind].objnum;
		object &objp = vmobjptridx(objnum);
		aip = &objp.ctype.ai_info;
		old_index = aip->hide_index;

		aip->hide_index = free_path_index;
		for (i=0; i<aip->path_length; i++)
			Point_segs[free_path_index++] = Point_segs[old_index++];
	}

	Point_segs_free_ptr = Point_segs.begin() + free_path_index;

#ifndef NDEBUG
	{
	auto &vcobjptr = Objects.vcptr;
	range_for (const auto &&objp, vcobjptr)
	{
		auto &obj = *objp;
		const auto &aip = obj.ctype.ai_info;

		if (obj.type == OBJ_ROBOT && obj.control_source == object::control_type::ai)
			if ((aip.hide_index + aip.path_length > Point_segs_free_ptr - Point_segs) && (aip.path_length>0))
				Int3();		//	Contact Mike: Debug trap for nasty, elusive bug.
	}

	validate_all_paths();
	}
#endif
}

//	-----------------------------------------------------------------------------
//	Do garbage collection if not been done for awhile, or things getting really critical.
void maybe_ai_path_garbage_collect(void)
{
	if (Point_segs_free_ptr - Point_segs > MAX_POINT_SEGS - MAX_PATH_LENGTH) {
		if (Last_tick_garbage_collected+1 >= d_tick_count) {
			//	This is kind of bad.  Garbage collected last frame or this frame.
			//	Just destroy all paths.  Too bad for the robots.  They are memory wasteful.
			ai_reset_all_paths();
		} else {
			//	We are really close to full, but didn't just garbage collect, so maybe this is recoverable.
			ai_path_garbage_collect();
		}
	} else if (Point_segs_free_ptr - Point_segs > 3*MAX_POINT_SEGS/4) {
		if (Last_tick_garbage_collected + 16 < d_tick_count) {
			ai_path_garbage_collect();
		}
	} else if (Point_segs_free_ptr - Point_segs > MAX_POINT_SEGS/2) {
		if (Last_tick_garbage_collected + 256 < d_tick_count) {
			ai_path_garbage_collect();
		}
	}
}
}

//	-----------------------------------------------------------------------------
//	Reset all paths.  Do garbage collection.
//	Should be called at the start of each level.
void ai_reset_all_paths(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	range_for (const auto &&objp, vmobjptr)
	{
		auto &obj = *objp;
		if (obj.type == OBJ_ROBOT && obj.control_source == object::control_type::ai)
		{
			obj.ctype.ai_info.hide_index = -1;
			obj.ctype.ai_info.path_length = 0;
		}
	}

	ai_path_garbage_collect();
}

//	---------------------------------------------------------------------------------------------------------
//	Probably called because a robot bashed a wall, getting a bunch of retries.
//	Try to resume path.
void attempt_to_resume_path(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	ai_static *aip = &objp->ctype.ai_info;
	int new_path_index;
	auto &robptr = Robot_info[get_robot_id(objp)];

	if (aip->behavior == ai_behavior::AIB_STATION
#if defined(DXX_BUILD_DESCENT_II)
		&& robptr.companion != 1
#endif
		)
		if (d_rand() > 8192) {
			ai_local		*ailp = &objp->ctype.ai_info.ail;

			aip->hide_segment = objp->segnum;
			ailp->mode = ai_mode::AIM_STILL;
		}

	new_path_index = aip->cur_path_index - aip->PATH_DIR;

	if ((new_path_index >= 0) && (new_path_index < aip->path_length)) {
		aip->cur_path_index = new_path_index;
	} else {
		// At end of line and have nowhere to go.
		move_towards_segment_center(robptr, LevelSharedSegmentState, objp);
		create_path_to_station(objp, robptr, 15);
	}
}

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS FOLLOW
//	----------------------------------------------------------------------------------------------------------

#if DXX_USE_EDITOR
namespace {

__attribute_used
static void test_create_path_many(fvmobjptridx &vmobjptridx, fimsegptridx &imsegptridx)
{
	std::array<point_seg, 200> point_segs;
	int			i;

	const unsigned Test_size = 1000;
	for (i=0; i<Test_size; i++) {
		Cursegp = imsegptridx(static_cast<segnum_t>((d_rand() * (Highest_segment_index + 1)) / D_RAND_MAX));
		Markedsegp = imsegptridx(static_cast<segnum_t>((d_rand() * (Highest_segment_index + 1)) / D_RAND_MAX));
		create_path_points(vmobjptridx(object_first), create_path_unused_robot_info, Cursegp, Markedsegp, point_segs.begin(), MAX_PATH_LENGTH, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none);
	}
}

__attribute_used
static void test_create_path(fvmobjptridx &vmobjptridx)
{
	std::array<point_seg, 200> point_segs;

	create_path_points(vmobjptridx(object_first), create_path_unused_robot_info, Cursegp, Markedsegp, point_segs.begin(), MAX_PATH_LENGTH, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none);
}

//	For all segments in mine, create paths to all segments in mine, print results.
__attribute_used
static void test_create_all_paths(fvmobjptridx &vmobjptridx, fvcsegptridx &vcsegptridx)
{
	Point_segs_free_ptr = Point_segs.begin();

	range_for (const auto &&segp0, vcsegptridx)
	{
		const shared_segment &sseg0 = segp0;
		if (sseg0.segnum != segment_none)
		{
			for (const auto &&segp1 : partial_range(vcsegptridx, segp0.get_unchecked_index(), vcsegptridx.count()))
			{
				const shared_segment &sseg1 = segp1;
				if (sseg1.segnum != segment_none)
				{
					create_path_points(vmobjptridx(object_first), create_path_unused_robot_info, segp0, segp1, Point_segs_free_ptr, MAX_PATH_LENGTH, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none);
				}
			}
		}
	}
}

short	Player_path_length=0;
int	Player_hide_index=-1;
int	Player_cur_path_index=0;
int	Player_following_path_flag=0;

//	------------------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objp based on its desire to get to a point.
static void player_path_set_orient_and_vel(object &objp, const vms_vector &goal_point)
{
	const auto &cur_vel = objp.mtype.phys_info.velocity;
	const auto &cur_pos = objp.pos;
	fix			speed_scale;
	fix			dot;

	const fix max_speed = F1_0*50;

	const auto norm_vec_to_goal = vm_vec_normalized_quick(vm_vec_sub(goal_point, cur_pos));
	auto norm_cur_vel = vm_vec_normalized_quick(cur_vel);
	const auto &&norm_fvec = vm_vec_normalized_quick(objp.orient.fvec);

	dot = vm_vec_dot(norm_vec_to_goal, norm_fvec);

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
		norm_cur_vel.y += norm_vec_to_goal.y/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
		norm_cur_vel.z += norm_vec_to_goal.z/2/(static_cast<float>(DESIGNATED_GAME_FRAMETIME)/FrameTime);
	}

	vm_vec_normalize_quick(norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= 4;

	speed_scale = fixmul(max_speed, dot);
	vm_vec_scale(norm_cur_vel, speed_scale);
	objp.mtype.phys_info.velocity = norm_cur_vel;
	physics_turn_towards_vector(norm_vec_to_goal, objp, F1_0);
}

}

//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void player_follow_path(object &objp)
{
	vms_vector	goal_point;
	int			count, forced_break, original_index;
	int			goal_seg;

	if (!Player_following_path_flag)
		return;

	if (Player_hide_index == -1)
		return;

	if (Player_path_length < 2)
		return;

	goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;
	goal_seg = Point_segs[Player_hide_index + Player_cur_path_index].segnum;
	Assert((goal_seg >= 0) && (goal_seg <= Highest_segment_index));
	(void)goal_seg;
	auto dist_to_goal = vm_vec_dist_quick(goal_point, objp.pos);

	if (Player_cur_path_index < 0)
		Player_cur_path_index = 0;
	else if (Player_cur_path_index >= Player_path_length)
		Player_cur_path_index = Player_path_length-1;

	goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;

	count=0;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	//original_dir = 1;
	original_index = Player_cur_path_index;
	const vm_distance threshold_distance{fixmul(vm_vec_mag_quick(objp.mtype.phys_info.velocity), FrameTime)*2 + F1_0*2};

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		//	----- Debug stuff -----
		if (count++ > 20) {
			break;
		}

		//	Advance to next point on path.
		Player_cur_path_index += 1;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((Player_cur_path_index >= Player_path_length) || (Player_cur_path_index < 0)) {
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		//	If went all the way around to original point, in same direction, then get out of here!
		if (Player_cur_path_index == original_index) {
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;
		dist_to_goal = vm_vec_dist_quick(goal_point, objp.pos);

	}	//	end while

	//	Set velocity (objp->mtype.phys_info.velocity) and orientation (objp->orient) for this object.
	player_path_set_orient_and_vel(objp, goal_point);
}

namespace {

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
static void create_player_path_to_segment(fvmobjptridx &vmobjptridx, segnum_t segnum)
{
	const auto objp = vmobjptridx(ConsoleObject);
	Player_hide_index=-1;
	Player_cur_path_index=0;
	Player_following_path_flag=0;

	auto &&cr = create_path_points(objp, create_path_unused_robot_info, objp->segnum, segnum, Point_segs_free_ptr, 100, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none);
	Player_path_length = cr.second;
	if (cr.first == create_path_result::early)
		con_printf(CON_DEBUG,"Unable to form path of length %i for myself", 100);

	Player_following_path_flag = 1;

	Player_hide_index = Point_segs_free_ptr - Point_segs;
	Player_cur_path_index = 0;
	Point_segs_free_ptr += Player_path_length;
	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		ai_reset_all_paths();
	}

}

}
segnum_t	Player_goal_segment = segment_none;

void check_create_player_path(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	if (Player_goal_segment != segment_none)
		create_player_path_to_segment(vmobjptridx, Player_goal_segment);

	Player_goal_segment = segment_none;
}

#endif

}

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS ENDED
//	----------------------------------------------------------------------------------------------------------

