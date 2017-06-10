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
namespace dsx {
struct segment;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::segment, segnum_t, MAX_SEGMENTS);
#endif

#include "fwd-valptridx.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {
constexpr std::size_t MAX_VERTICES_PER_SEGMENT = 8;
constexpr std::size_t MAX_SIDES_PER_SEGMENT = 6;
constexpr std::size_t MAX_VERTICES_PER_POLY = 4;

constexpr std::size_t MAX_SEGMENTS_ORIGINAL = 900;
constexpr std::size_t MAX_SEGMENT_VERTICES_ORIGINAL = 4*MAX_SEGMENTS_ORIGINAL;
constexpr std::size_t MAX_SEGMENT_VERTICES = 4*MAX_SEGMENTS;
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

constexpr fix DEFAULT_LIGHTING = 0;   // (F1_0/2)

#if DXX_USE_EDITOR   //verts for the new segment
constexpr std::size_t NUM_NEW_SEG_VERTICES = 8;
constexpr std::size_t NEW_SEGMENT_VERTICES = MAX_SEGMENT_VERTICES;
constexpr std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES + NUM_NEW_SEG_VERTICES;
#else           //No editor
constexpr std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES;
#endif

struct uvl;
enum side_type : uint8_t;

using wallnum_t = uint16_t;
struct side;

}

typedef unsigned segment_type_t;
constexpr segment_type_t SEGMENT_IS_NOTHING = 0;
constexpr segment_type_t SEGMENT_IS_FUELCEN = 1;
constexpr segment_type_t SEGMENT_IS_REPAIRCEN = 2;
constexpr segment_type_t SEGMENT_IS_CONTROLCEN = 3;
constexpr segment_type_t SEGMENT_IS_ROBOTMAKER = 4;
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::size_t MAX_CENTER_TYPES = 5;
#elif defined(DXX_BUILD_DESCENT_II)
typedef unsigned s2f_ambient_t;
constexpr s2f_ambient_t S2F_AMBIENT_WATER = 1;
constexpr s2f_ambient_t S2F_AMBIENT_LAVA = 2;
constexpr segment_type_t SEGMENT_IS_GOAL_BLUE = 5;
constexpr segment_type_t SEGMENT_IS_GOAL_RED = 6;
constexpr std::size_t MAX_CENTER_TYPES = 7;
#endif

namespace dcx {
struct count_segment_array_t;
struct group;

struct vertex;
extern array<vertex, MAX_VERTICES> Vertices;
extern unsigned Num_segments;
extern unsigned Num_vertices;


#define Side_to_verts Side_to_verts_int
extern const array<array<unsigned, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int;    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const array<uint8_t, MAX_SIDES_PER_SEGMENT> Side_opposite;                                // Side_opposite[my_side] returns side opposite cube from my_side.

void segment_side_wall_tmap_write(PHYSFS_File *fp, const side &side);
}
void add_segment_to_group(segnum_t segment_num, int group_num);

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct delta_light;
struct dl_index;

constexpr std::size_t MAX_DL_INDICES = 500;
constexpr std::size_t MAX_DELTA_LIGHTS = 32000; // Original D2: 10000;

constexpr fix DL_SCALE = 2048;    // Divide light to allow 3 bits integer, 5 bits fraction.

extern array<dl_index, MAX_DL_INDICES> Dl_indices;
extern array<delta_light, MAX_DELTA_LIGHTS> Delta_lights;
extern unsigned Num_static_lights;

int subtract_light(vmsegptridx_t segnum, sidenum_fast_t sidenum);
int add_light(vmsegptridx_t segnum, sidenum_fast_t sidenum);
void clear_light_subtracted();

void segment2_read(vmsegptr_t s2, PHYSFS_File *fp);
void segment2_write(vcsegptr_t s2, PHYSFS_File *fp);

void delta_light_read(delta_light *dl, PHYSFS_File *fp);
void delta_light_write(const delta_light *dl, PHYSFS_File *fp);

void dl_index_read(dl_index *di, PHYSFS_File *fp);
void dl_index_write(const dl_index *di, PHYSFS_File *fp);
}
#endif

namespace dcx {

template <typename T, unsigned bits>
class visited_segment_mask_t;
class visited_segment_bitarray_t;

template <unsigned bits>
class visited_segment_multibit_array_t;

constexpr int side_none = MAX_SIDES_PER_SEGMENT;
constexpr int edge_none = -1;

}
