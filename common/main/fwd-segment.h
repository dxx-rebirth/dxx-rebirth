/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <type_traits>
#include <physfs.h>
#include "maths.h"

#include <cstdint>
#include "cpp-valptridx.h"

namespace dcx {
constexpr std::integral_constant<std::size_t, 9000> MAX_SEGMENTS{};
using segnum_t = uint16_t;
}
#ifdef dsx
namespace dcx {
struct shared_segment;
struct unique_segment;
struct segment;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dcx::, segment, segnum_t, MAX_SEGMENTS);
#endif

#include "fwd-valptridx.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {
constexpr std::integral_constant<std::size_t, 8> MAX_VERTICES_PER_SEGMENT{};
constexpr std::integral_constant<std::size_t, 6> MAX_SIDES_PER_SEGMENT{};
constexpr std::integral_constant<std::size_t, 4> MAX_VERTICES_PER_POLY{};

constexpr std::size_t MAX_SEGMENTS_ORIGINAL = 900;
constexpr std::integral_constant<std::size_t, 4 * MAX_SEGMENTS_ORIGINAL> MAX_SEGMENT_VERTICES_ORIGINAL{};
constexpr std::integral_constant<std::size_t, 4 * MAX_SEGMENTS> MAX_SEGMENT_VERTICES{};
}
#ifdef dsx
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(segment, seg);

static constexpr valptridx<segment>::magic_constant<0xfffe> segment_exit{};
static constexpr valptridx<segment>::magic_constant<0xffff> segment_none{};
static constexpr valptridx<segment>::magic_constant<0> segment_first{};
void delete_segment_from_group(vmsegptridx_t segment_num, unsigned group_num);
}
#endif

namespace dcx {

typedef uint_fast32_t sidenum_fast_t;

enum sidenum_t : uint8_t
{
	WLEFT = 0,
	WTOP = 1,
	WRIGHT = 2,
	WBOTTOM = 3,
	WBACK = 4,
	WFRONT = 5
};

//normal everyday vertices

constexpr std::integral_constant<fix, 0> DEFAULT_LIGHTING{};   // (F1_0/2)

#if DXX_USE_EDITOR   //verts for the new segment
constexpr std::integral_constant<std::size_t, 8> NUM_NEW_SEG_VERTICES{};
constexpr std::integral_constant<unsigned, MAX_SEGMENT_VERTICES> NEW_SEGMENT_VERTICES{};
constexpr std::integral_constant<std::size_t, MAX_SEGMENT_VERTICES + NUM_NEW_SEG_VERTICES> MAX_VERTICES{};
#else           //No editor
constexpr std::integral_constant<std::size_t, MAX_SEGMENT_VERTICES> MAX_VERTICES{};
#endif

struct uvl;
enum side_type : uint8_t;

using wallnum_t = uint16_t;
struct shared_side;
struct unique_side;

struct vertex;
using vertnum_t = uint32_t;
}

/* `vertex` has only integer members, so wild reads are unlikely to
 * cause serious harm.  It is read far more than it is written, so
 * eliminating checking on reads saves substantial code space.
 *
 * Vertex indices are only taken from map data, not network data, so
 * errors are unlikely.  Report them tersely to avoid recording the
 * file+line of every access.
 */
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_vertex undefined
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_vertex trap_terse
DXX_VALPTRIDX_DECLARE_SUBTYPE(dcx::, vertex, vertnum_t, MAX_VERTICES);

typedef unsigned segment_type_t;
constexpr segment_type_t SEGMENT_IS_NOTHING = 0;
constexpr segment_type_t SEGMENT_IS_FUELCEN = 1;
constexpr segment_type_t SEGMENT_IS_REPAIRCEN = 2;
constexpr segment_type_t SEGMENT_IS_CONTROLCEN = 3;
constexpr segment_type_t SEGMENT_IS_ROBOTMAKER = 4;
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<std::size_t, 5> MAX_CENTER_TYPES{};
#elif defined(DXX_BUILD_DESCENT_II)
typedef unsigned s2f_ambient_t;
constexpr std::integral_constant<s2f_ambient_t, 1> S2F_AMBIENT_WATER{};
constexpr std::integral_constant<s2f_ambient_t, 2> S2F_AMBIENT_LAVA{};
constexpr std::integral_constant<std::size_t, 7> MAX_CENTER_TYPES{};
constexpr segment_type_t SEGMENT_IS_GOAL_BLUE = 5;
constexpr segment_type_t SEGMENT_IS_GOAL_RED = 6;
#endif

namespace dcx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(vertex, vert);
struct count_segment_array_t;
struct group;

extern unsigned Num_segments;
extern unsigned Num_vertices;


#define Side_to_verts Side_to_verts_int
extern const array<array<unsigned, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int;    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const array<uint8_t, MAX_SIDES_PER_SEGMENT> Side_opposite;                                // Side_opposite[my_side] returns side opposite cube from my_side.

void segment_side_wall_tmap_write(PHYSFS_File *fp, const shared_side &sside, const unique_side &uside);
}
void add_segment_to_group(segnum_t segment_num, int group_num);

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct delta_light;
struct dl_index;

constexpr std::integral_constant<std::size_t, 32000> MAX_DELTA_LIGHTS{}; // Original D2: 10000;

constexpr std::integral_constant<fix, 2048> DL_SCALE{};    // Divide light to allow 3 bits integer, 5 bits fraction.

extern array<delta_light, MAX_DELTA_LIGHTS> Delta_lights;

int subtract_light(vmsegptridx_t segnum, sidenum_fast_t sidenum);
int add_light(vmsegptridx_t segnum, sidenum_fast_t sidenum);
void clear_light_subtracted();

void segment2_write(vcsegptr_t s2, PHYSFS_File *fp);

void delta_light_read(delta_light *dl, PHYSFS_File *fp);
void delta_light_write(const delta_light *dl, PHYSFS_File *fp);

void dl_index_read(dl_index *di, PHYSFS_File *fp);
void dl_index_write(const dl_index *di, PHYSFS_File *fp);
using dlindexnum_t = uint16_t;
}
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_dl_index trap_terse
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, dl_index, dlindexnum_t, 500);
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(dl_index, dlindex);
}
#endif

namespace dcx {

template <unsigned bits>
class visited_segment_mask_t;
using visited_segment_bitarray_t = visited_segment_mask_t<1>;

constexpr std::integral_constant<int, MAX_SIDES_PER_SEGMENT> side_none{};
constexpr std::integral_constant<int, -1> edge_none{};

}
