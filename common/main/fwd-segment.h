/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <optional>
#include <type_traits>
#include <physfs.h>
#include "maths.h"

#include <cstdint>
#include "cpp-valptridx.h"
#include "d_array.h"
#include "d_crange.h"
#include "physfsx.h"

namespace dcx {
constexpr std::integral_constant<std::size_t, 9000> MAX_SEGMENTS{};
enum segnum_t : uint16_t;
struct d_level_unique_automap_state;
}
#ifdef dsx
namespace dcx {
struct shared_segment;
struct unique_segment;
struct segment;

template <typename S, typename U>
struct susegment;

using msmusegment = susegment<shared_segment, unique_segment>;
using mscusegment = susegment<shared_segment, const unique_segment>;	/* unusual, but supported */
using csmusegment = susegment<const shared_segment, unique_segment>;
using cscusegment = susegment<const shared_segment, const unique_segment>;
}
DXX_VALPTRIDX_DECLARE_SUBTYPE(dcx::, segment, segnum_t, MAX_SEGMENTS);
#endif

#include "fwd-valptridx.h"
#include "dsx-ns.h"
#include <array>

namespace dcx {
constexpr std::integral_constant<std::size_t, 8> MAX_VERTICES_PER_SEGMENT{};

constexpr std::size_t MAX_SEGMENTS_ORIGINAL = 900;
constexpr std::integral_constant<std::size_t, 4 * MAX_SEGMENTS_ORIGINAL> MAX_SEGMENT_VERTICES_ORIGINAL{};
constexpr std::integral_constant<std::size_t, 4 * MAX_SEGMENTS> MAX_SEGMENT_VERTICES{};

#ifdef dsx
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(segment, seg);

static constexpr valptridx<segment>::magic_constant<segnum_t{0xfffe}> segment_exit{};
static constexpr valptridx<segment>::magic_constant<segnum_t{0xffff}> segment_none{};
static constexpr valptridx<segment>::magic_constant<segnum_t{0}> segment_first{};
#endif
}
#ifdef dsx
namespace dsx {
void delete_segment_from_group(vmsegptridx_t segment_num, unsigned group_num);
}
#endif

namespace dcx {

enum class materialization_center_number : uint8_t;
enum class station_number : uint8_t;

enum class sidenum_t : uint8_t;
enum class sidemask_t : uint8_t;

[[nodiscard]]
std::optional<sidenum_t> build_sidenum_from_untrusted(uint8_t untrusted);
constexpr constant_xrange<sidenum_t, sidenum_t{0}, sidenum_t{6}> MAX_SIDES_PER_SEGMENT{};
constexpr std::integral_constant<sidenum_t, MAX_SIDES_PER_SEGMENT.value> side_none{};

using texture_index = uint16_t;
enum class texture1_value : uint16_t;
enum class texture2_value : uint16_t;
enum class texture2_rotation_low : uint8_t;
enum class texture2_rotation_high : uint16_t;

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
enum class side_type : uint8_t;

enum class wallnum_t : uint16_t;
struct shared_side;
struct unique_side;

struct vertex;
enum class vertnum_t : uint32_t;
enum class side_relative_vertnum : uint8_t;
enum class segment_relative_vertnum : uint8_t;
enum class segment_special : uint8_t;
constexpr constant_xrange<side_relative_vertnum, side_relative_vertnum{0}, side_relative_vertnum{4}> MAX_VERTICES_PER_SIDE{};
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

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<std::size_t, 5> MAX_CENTER_TYPES{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<std::size_t, 7> MAX_CENTER_TYPES{};
#endif

namespace dcx {

DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(vertex, vert);
struct count_segment_array_t;
struct group;

struct d_level_shared_vertex_state;
struct d_level_shared_segment_state;
struct d_level_unique_segment_state;

template <typename T>
	using per_side_array = enumerated_array<T, static_cast<std::size_t>(MAX_SIDES_PER_SEGMENT.value), sidenum_t>;
extern const per_side_array<enumerated_array<segment_relative_vertnum, 4, side_relative_vertnum>>  Side_to_verts; // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const per_side_array<sidenum_t> Side_opposite; // Side_opposite[my_side] returns side opposite cube from my_side.

void segment_side_wall_tmap_write(PHYSFS_File *fp, const shared_side &sside, const unique_side &uside);
}
void add_segment_to_group(segnum_t segment_num, int group_num);

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
struct delta_light;
struct dl_index;
struct d_level_shared_destructible_light_state;
struct d_level_shared_segment_state;

constexpr std::integral_constant<std::size_t, 32000> MAX_DELTA_LIGHTS{}; // Original D2: 10000;

constexpr std::integral_constant<fix, 2048> DL_SCALE{};    // Divide light to allow 3 bits integer, 5 bits fraction.

enum class delta_light_index : uint16_t;
using d_delta_light_array = enumerated_array<delta_light, MAX_DELTA_LIGHTS, delta_light_index>;

void clear_light_subtracted();

void segment2_write(const cscusegment s2, PHYSFS_File *fp);

void delta_light_read(delta_light *dl, NamedPHYSFS_File fp);
void delta_light_write(const delta_light *dl, PHYSFS_File *fp);

void dl_index_read(dl_index *di, NamedPHYSFS_File fp);
void dl_index_write(const dl_index *di, PHYSFS_File *fp);
enum class dlindexnum_t : uint16_t;
}
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_dl_index trap_terse
DXX_VALPTRIDX_DECLARE_SUBTYPE(dsx::, dl_index, ::dsx::dlindexnum_t, 500);
namespace dsx {
DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(dl_index, dlindex);
int subtract_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, vmsegptridx_t segnum, sidenum_t sidenum);
int add_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, vmsegptridx_t segnum, sidenum_t sidenum);
}
#endif

namespace dcx {

constexpr std::integral_constant<int, -1> edge_none{};

}
