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
 * Global variables for main directory
 *
 */

#include "maths.h"
#include "vecmat.h"
#include "inferno.h"
#include "lighting.h"
#include "cntrlcen.h"
#include "effects.h"
#include "fuelcen.h"
#include "segment.h"
#include "switch.h"
#include "object.h"
#include "player.h"
#include "bm.h"
#include "robot.h"
#include "3d.h"
#include "game.h"
#include "textures.h"
#include "valptridx.tcc"
#include "wall.h"
#include "d_levelstate.h"

namespace dcx {

template <typename T>
static void reconstruct_global_variable(T &t)
{
	t.~T();
	new(&t) T();
}

d_interface_unique_state InterfaceUniqueState;
d_game_view_unique_state GameViewUniqueState;
d_player_unique_endlevel_state PlayerUniqueEndlevelState;
d_level_unique_automap_state LevelUniqueAutomapState;
d_level_unique_fuelcenter_state LevelUniqueFuelcenterState;
d_level_unique_robot_awareness_state LevelUniqueRobotAwarenessState;
d_level_unique_segment_state LevelUniqueSegmentState;
// Global array of vertices, common to one mine.
valptridx<player>::array_managed_type Players;
valptridx<segment>::array_managed_type Segments;
}
enumerated_array<g3s_point, MAX_VERTICES, vertnum_t> Segment_points;

namespace dcx {
fix FrameTime = 0x1000;	// Time since last frame, in seconds
fix64 GameTime64 = 0;			//	Time in game, in seconds

int d_tick_count = 0; // increments every 33.33ms
int d_tick_step = 0;  // true once every 33.33ms

//	This is the global mine which create_new_mine returns.
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, Vertices, pointed to by Vp)

//	Translate table to get opposite side of a face on a segment.

const std::array<uint8_t, MAX_SIDES_PER_SEGMENT> Side_opposite{{
	WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK
}};

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

const std::array<std::array<unsigned, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int{{
	{{7,6,2,3}},			// left
	{{0,4,7,3}},			// top
	{{0,1,5,4}},			// right
	{{2,6,5,1}},			// bottom
	{{4,5,6,7}},			// back
	{{3,2,1,0}},			// front
}};

// Texture map stuff

//--unused-- fix	Laser_delay_time = F1_0/6;		//	Delay between laser fires.

static void reset_globals_for_new_game()
{
	reconstruct_global_variable(LevelSharedBossState);
	reconstruct_global_variable(LevelUniqueFuelcenterState);
	reconstruct_global_variable(LevelUniqueSegmentState);
	reconstruct_global_variable(Players);
	reconstruct_global_variable(Segments);
}

}

#if DXX_HAVE_POISON_UNDEFINED
template <typename managed_type>
valptridx<managed_type>::array_managed_type::array_managed_type()
{
	DXX_MAKE_MEM_UNDEFINED(this->begin(), this->end());
}
#endif

namespace dsx {
d_game_shared_state GameSharedState;
d_game_unique_state GameUniqueState;
d_level_shared_boss_state LevelSharedBossState;
#if defined(DXX_BUILD_DESCENT_II)
d_level_shared_control_center_state LevelSharedControlCenterState;
#endif
d_level_unique_effects_clip_state LevelUniqueEffectsClipState;
d_level_shared_segment_state LevelSharedSegmentState;
d_level_unique_object_state LevelUniqueObjectState;
d_level_unique_light_state LevelUniqueLightState;
d_level_shared_polygon_model_state LevelSharedPolygonModelState;
d_level_shared_robotcenter_state LevelSharedRobotcenterState;
d_level_shared_robot_info_state LevelSharedRobotInfoState;
d_level_shared_robot_joint_state LevelSharedRobotJointState;
d_level_unique_wall_subsystem_state LevelUniqueWallSubsystemState;
d_level_unique_tmap_info_state LevelUniqueTmapInfoState;

#if defined(DXX_BUILD_DESCENT_II)
d_level_shared_seismic_state LevelSharedSeismicState;
d_level_unique_seismic_state LevelUniqueSeismicState;
#endif

void reset_globals_for_new_game()
{
	::dcx::reset_globals_for_new_game();
	/* Skip LevelUniqueEffectsClipState because it contains some fields
	 * that are initialized from game data, and those fields should not
	 * be reconstructed.
	 */
	reconstruct_global_variable(LevelUniqueObjectState);
	reconstruct_global_variable(LevelUniqueLightState);
	reconstruct_global_variable(LevelUniqueWallSubsystemState);
	/* Same for LevelUniqueTmapInfoState */
}
}

/*
 * If not specified otherwise by the user, enable full instantiation if
 * the compiler inliner is not enabled.  This is required because
 * non-inline builds contain link time references to functions that are
 * not used.
 *
 * Force full instantiation for non-inline builds so that these
 * references are satisfied.  For inline-enabled builds, instantiate
 * only the classes that are known to be used.
 *
 * Force full instantiation for AddressSanitizer builds.  In gcc-7 (and
 * possibly other versions), AddressSanitizer inhibits an optimization
 * that deletes an unnecessary nullptr check, causing references to
 * class null_pointer_exception.
 */
#ifndef DXX_VALPTRIDX_ENABLE_FULL_TEMPLATE_INSTANTIATION
#if (defined(__NO_INLINE__) && __NO_INLINE__ > 0) || defined(__SANITIZE_ADDRESS__)
#define DXX_VALPTRIDX_ENABLE_FULL_TEMPLATE_INSTANTIATION	1
#else
#define DXX_VALPTRIDX_ENABLE_FULL_TEMPLATE_INSTANTIATION	0
#endif
#endif

#if DXX_VALPTRIDX_ENABLE_FULL_TEMPLATE_INSTANTIATION
template class valptridx<dcx::active_door>;
template class valptridx<dcx::vertex>;
#if defined(DXX_BUILD_DESCENT_II)
template class valptridx<dsx::cloaking_wall>;
#endif
template class valptridx<dsx::object>;
template class valptridx<dcx::player>;
template class valptridx<dcx::segment>;
template class valptridx<dsx::trigger>;
template class valptridx<dsx::wall>;

#else
namespace {

	/* Explicit instantiation cannot be conditional on a truth
	 * expression, but the exception types only need to be instantiated
	 * if they are used.  To reduce code bloat, use
	 * `instantiation_guard` to instantiate the real exception class if
	 * it is used (as reported by
	 * `valptridx<T>::report_error_uses_exception`), but otherwise
	 * instantiate a stub class with no members.  The stub class must be
	 * a member of a template that depends on `T` because duplicate
	 * explicit instantiations are not allowed.  If the stub did not
	 * depend on `T`, each type `T` that used the stubs would
	 * instantiate the same stub.
	 *
	 * Hide `instantiation_guard` in an anonymous namespace to encourage
	 * the compiler to optimize away any unused pieces of it.
	 */
template <typename T, bool = valptridx<T>::report_error_uses_exception::value>
struct instantiation_guard
{
	using type = valptridx<T>;
};

template <typename T>
struct instantiation_guard<T, false>
{
	struct type
	{
		class index_mismatch_exception {};
		class index_range_exception {};
		class null_pointer_exception {};
	};
};

}

template class instantiation_guard<dcx::active_door>::type::index_range_exception;
template class instantiation_guard<dcx::vertex>::type::index_range_exception;
#if defined(DXX_BUILD_DESCENT_II)
template class instantiation_guard<dsx::cloaking_wall>::type::index_range_exception;
#endif

template class instantiation_guard<dsx::object>::type::index_mismatch_exception;
template class instantiation_guard<dsx::object>::type::index_range_exception;
template class instantiation_guard<dsx::object>::type::null_pointer_exception;

template class instantiation_guard<dcx::player>::type::index_range_exception;

template class instantiation_guard<dcx::segment>::type::index_mismatch_exception;
template class instantiation_guard<dcx::segment>::type::index_range_exception;
template class instantiation_guard<dcx::segment>::type::null_pointer_exception;

template class instantiation_guard<dsx::trigger>::type::index_range_exception;
template class instantiation_guard<dsx::wall>::type::index_range_exception;

#endif
