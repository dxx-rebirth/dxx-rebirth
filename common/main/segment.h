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
 * Include file for functions which need to access segment data structure.
 *
 */

#pragma once

#include <physfs.h>
#include <type_traits>
#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "dxxsconf.h"
#include "dsx-ns.h"

#ifdef __cplusplus
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include "fwd-segment.h"
#include "countarray.h"
#include "valptridx.h"
#include "objnum.h"
#include "pack.h"


#ifdef dsx
namespace dsx {
// Returns true if segnum references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
static inline bool IS_CHILD(segnum_t s)
{
	return s != segment_none && s != segment_exit;
}
}
#endif

namespace dcx {

//Structure for storing u,v,light values.
//NOTE: this structure should be the same as the one in 3d.h
struct uvl
{
	fix u, v, l;
};

enum class side_type : uint8_t
{
	quad = 1,	// render side as quadrilateral
	tri_02 = 2,	// render side as two triangles, triangulated along edge from 0 to 2
	tri_13 = 3,	// render side as two triangles, triangulated along edge from 1 to 3
};

#if 0
struct wallnum_t : prohibit_void_ptr<wallnum_t>
{
	typedef int16_t integral_type;
	integral_type value;
	wallnum_t() = default;
	wallnum_t(const integral_type &v) : value(v)
	{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
		if (__builtin_constant_p(v))
			DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_constant_wall, "constant wall number constructed");
#endif
	}
	wallnum_t &operator=(integral_type v)
	{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
		if (__builtin_constant_p(v))
			DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_constant_wall, "constant wall number assigned");
#endif
		value = v;
		return *this;
	}
	template <integral_type I>
		wallnum_t &operator=(const wall_magic_constant_t<I> &)
		{
			value = I;
			return *this;
		}
	bool operator==(const wallnum_t &v) const { return value == v.value; }
	bool operator==(const int &v) const
	{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
		if (__builtin_constant_p(v))
			DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_constant_wall, "constant wall number compared");
#endif
		return value == v;
	}
	template <integral_type I>
		bool operator==(const wall_magic_constant_t<I> &) const { return value == I; }
	template <typename T>
		bool operator!=(const T &v) const { return !(*this == v); }
	template <typename T>
		bool operator==(const T &v) const = delete;
	template <typename T>
		bool operator<(const T &v) const
		{
			static_assert(std::is_integral<T>::value, "non-integral wall number compared");
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
			if (__builtin_constant_p(v))
				DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_constant_wall, "constant wall number compared");
#endif
			return value < v;
		}
	template <typename T>
		bool operator>(const T &v) const
		{
			return v < *this;
		}
	template <typename T>
		bool operator<=(const T &) const = delete;
	template <typename T>
		bool operator>=(const T &) const = delete;
	constexpr operator integral_type() const { return value; }
	operator integral_type &() { return value; }
};
#endif

struct shared_side
{
	struct illegal_type;
	side_type m_type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	const side_type &get_type() const { return m_type; }
	void set_type(side_type t) { m_type = t; }
	wallnum_t wall_num;
	std::array<vms_vector, 2> normals;  // 2 normals, if quadrilateral, both the same.
};

enum class texture1_value : uint16_t
{
	None,
};

/* texture2_value uses the low 14 bits for the array index and the high
 * 2 bits for texture2_rotation_high
 */
enum class texture2_value : uint16_t
{
	None,
};

enum class texture2_rotation_low : uint8_t
{
	Normal = 0,
	_1 = 1,
	_2 = 2,
	_3 = 3,
};

#define TEXTURE2_ROTATION_SHIFT	14u
#define TEXTURE2_ROTATION_INDEX_MASK	((1u << TEXTURE2_ROTATION_SHIFT) - 1u)

enum class texture2_rotation_high : uint16_t
{
	Normal = 0,
	_1 = static_cast<uint16_t>(texture2_rotation_low::_1) << TEXTURE2_ROTATION_SHIFT,
	_2 = static_cast<uint16_t>(texture2_rotation_low::_2) << TEXTURE2_ROTATION_SHIFT,
	_3 = static_cast<uint16_t>(texture2_rotation_low::_3) << TEXTURE2_ROTATION_SHIFT,
};

static constexpr texture2_rotation_high &operator++(texture2_rotation_high &t)
{
	/* Cast to uint32, step to the next value, cast back.  The cast back
	 * will truncate to the low 16 bits, causing
	 * texture2_rotation_high::_3 to roll over to
	 * texture2_rotation_high::Normal.
	 */
	return (t = static_cast<texture2_rotation_high>(static_cast<uint32_t>(t) + (1u << TEXTURE2_ROTATION_SHIFT)));
}

static constexpr texture_index get_texture_index(const texture1_value t)
{
	return static_cast<texture_index>(t);
}

static constexpr texture_index get_texture_index(const texture2_value t)
{
	return static_cast<texture_index>(static_cast<uint16_t>(t) & TEXTURE2_ROTATION_INDEX_MASK);
}

static constexpr texture2_rotation_high get_texture_rotation_high(const texture2_value t)
{
	return static_cast<texture2_rotation_high>(static_cast<uint16_t>(t) & ~TEXTURE2_ROTATION_INDEX_MASK);
}

static constexpr texture2_rotation_low get_texture_rotation_low(const texture2_rotation_high t)
{
	return static_cast<texture2_rotation_low>(static_cast<uint16_t>(t) >> TEXTURE2_ROTATION_SHIFT);
}

static constexpr texture2_rotation_low get_texture_rotation_low(const texture2_value t)
{
	return get_texture_rotation_low(get_texture_rotation_high(t));
}

static constexpr texture1_value build_texture1_value(const texture_index t)
{
	return static_cast<texture1_value>(t);
}

static constexpr texture2_value build_texture2_value(const texture_index t, const texture2_rotation_high rotation)
{
	return static_cast<texture2_value>(static_cast<uint16_t>(t) | static_cast<uint16_t>(rotation));
}

#undef TEXTURE2_ROTATION_SHIFT
#undef TEXTURE2_ROTATION_INDEX_MASK

struct unique_side
{
	texture1_value tmap_num;
	texture2_value tmap_num2;
	std::array<uvl, 4>     uvls;
};

#ifdef dsx
struct shared_segment
{
#if DXX_USE_EDITOR
	segnum_t   segnum;     // segment number, not sure what it means
	short   group;      // group number to which the segment belongs.
#endif
	std::array<segnum_t, MAX_SIDES_PER_SEGMENT>   children;    // indices of 6 children segments, front, left, top, right, bottom, back
	std::array<vertnum_t, MAX_VERTICES_PER_SEGMENT> verts;    // vertex ids of 4 front and 4 back vertices
	uint8_t special;    // what type of center this is
	int8_t matcen_num; // which center segment is associated with.
	uint8_t station_idx;
	/* if DXX_BUILD_DESCENT_II */
	uint8_t s2_flags;
	/* endif */
	std::array<shared_side, MAX_SIDES_PER_SEGMENT> sides;
};

struct unique_segment
{
	objnum_t objects;    // pointer to objects in this segment
	//      If bit n (1 << n) is set, then side #n in segment has had light subtracted from original (editor-computed) value.
	uint8_t light_subtracted;
	/* if DXX_BUILD_DESCENT_II */
	uint8_t slide_textures;
	/* endif */
	fix     static_light;
	std::array<unique_side, MAX_SIDES_PER_SEGMENT> sides;
};

struct segment : unique_segment, shared_segment
{
};

template <typename S, typename U>
struct susegment
{
	using qualified_segment = typename std::conditional<std::is_const<S>::value && std::is_const<U>::value, const segment, segment>::type;
	S &s;
	U &u;
	constexpr susegment(qualified_segment &b) :
		s(b), u(b)
	{
	}
	constexpr susegment(const susegment &) = default;
	constexpr susegment(susegment &&) = default;
	template <typename S2, typename U2, typename std::enable_if<std::is_convertible<S2 &, S &>::value && std::is_convertible<U2 &, U &>::value, int>::type = 0>
		constexpr susegment(const susegment<S2, U2> &r) :
			s(r.s), u(r.u)
	{
	}
	template <typename T, typename std::enable_if<std::is_convertible<T &&, qualified_segment &>::value, int>::type = 0>
		constexpr susegment(T &&t) :
			susegment(static_cast<qualified_segment &>(t))
	{
	}
	operator S &() const
	{
		return s;
	}
	operator U &() const
	{
		return u;
	}
};
#endif

struct count_segment_array_t : public count_array_t<segnum_t, MAX_SEGMENTS> {};

struct group
{
	struct segment_array_type_t : public count_segment_array_t {};
	struct vertex_array_type_t : public count_array_t<vertnum_t, MAX_VERTICES> {};
	segment_array_type_t segments;
	vertex_array_type_t vertices;
	void clear()
	{
		segments.clear();
		vertices.clear();
	}
};

#ifdef dsx
#define Highest_segment_index (Segments.get_count() - 1)
DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(segment, seg, Segments);
#endif

// Globals from mglobal.c
struct vertex : vms_vector
{
	vertex() = default;
	vertex(const fix &a, const fix &b, const fix &c) :
		vms_vector{a, b, c}
	{
	}
	explicit vertex(const vms_vector &v) :
		vms_vector(v)
	{
	}
};

static constexpr vertnum_t operator++(vertnum_t &v)
{
	return (v = static_cast<vertnum_t>(static_cast<unsigned>(v) + 1));
}
}

#ifdef dsx
struct shared_side::illegal_type : std::runtime_error
{
	const shared_segment *const m_segment;
	const shared_side *const m_side;
	illegal_type(const shared_segment &seg, const shared_side &s) :
		runtime_error("illegal side type"),
		m_segment(&seg), m_side(&s)
	{
	}
	illegal_type(const shared_side &s) :
		runtime_error("illegal side type"),
		m_segment(nullptr), m_side(&s)
	{
	}
};

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
struct delta_light : prohibit_void_ptr<delta_light>
{
	segnum_t   segnum;
	uint8_t   sidenum;
	std::array<ubyte, 4>   vert_light;
};

// Light at segnum:sidenum casts light on count sides beginning at index (in array Delta_lights)
struct dl_index {
	segnum_t   segnum;
	uint8_t   sidenum;
	uint8_t count;
	uint16_t index;
	bool operator<(const dl_index &rhs) const
	{
		if (segnum < rhs.segnum)
			return true;
		if (segnum > rhs.segnum)
			return false;
		return sidenum < rhs.sidenum;
	}
};

struct d_level_shared_destructible_light_state
{
	valptridx<dl_index>::array_managed_type Dl_indices;
	d_delta_light_array Delta_lights;
};
#endif

}
#endif

namespace dcx {

#ifdef dsx
struct d_level_shared_vertex_state
{
#if DXX_USE_EDITOR
	/* Number of elements in Vertex_active which have a nonzero value.
	 * This can be less than the index of the highest defined vertex if
	 * there are unused vertices.
	 */
	unsigned Num_vertices;
#endif
private:
	vertex_array Vertices;
#if DXX_USE_EDITOR
	enumerated_array<uint8_t, MAX_VERTICES, vertnum_t> Vertex_active; // !0 means vertex is in use, 0 means not in use.
#endif
public:
	auto &get_vertices()
	{
		return Vertices;
	}
	const auto &get_vertices() const
	{
		return Vertices;
	}
#if DXX_USE_EDITOR
	auto &get_vertex_active()
	{
		return Vertex_active;
	}
	const auto &get_vertex_active() const
	{
		return Vertex_active;
	}
#endif
};

struct d_level_shared_segment_state
{
	unsigned Num_segments;
	d_level_shared_vertex_state LevelSharedVertexState;
	auto &get_segments()
	{
		return Segments;
	}
	const auto &get_segments() const
	{
		return Segments;
	}
	auto &get_vertex_state()
	{
		return LevelSharedVertexState;
	}
	const auto &get_vertex_state() const
	{
		return LevelSharedVertexState;
	}
};

struct d_level_unique_automap_state
{
	std::array<uint8_t, MAX_SEGMENTS> Automap_visited;
};

struct d_level_unique_segment_state
{
	auto &get_segments()
	{
		return Segments;
	}
	const auto &get_segments() const
	{
		return Segments;
	}
};

extern d_level_unique_automap_state LevelUniqueAutomapState;
extern d_level_unique_segment_state LevelUniqueSegmentState;
#endif

template <unsigned bits>
class visited_segment_mask_base
{
	static_assert(bits == 1 || bits == 2 || bits == 4, "bits must align in bytes");
protected:
	enum
	{
		divisor = 8 / bits,
	};
	using array_t = std::array<uint8_t, (MAX_SEGMENTS + (divisor - 1)) / divisor>;
	typedef typename array_t::size_type size_type;
	array_t a;
	struct maskproxy_shift_count_type
	{
		using bitmask_low_aligned = std::integral_constant<unsigned, (1 << bits) - 1>;
		const unsigned m_shift;
		unsigned shift() const
		{
			return m_shift;
		}
		typename array_t::value_type mask() const
		{
			return bitmask_low_aligned() << shift();
		}
		maskproxy_shift_count_type(const unsigned s) :
			m_shift(s * bits)
		{
		}
	};
	template <typename R>
	struct maskproxy_byte_reference : public maskproxy_shift_count_type
	{
		R m_byte;
		maskproxy_byte_reference(R byte, const unsigned s) :
			maskproxy_shift_count_type(s), m_byte(byte)
		{
		}
		operator uint8_t() const
		{
			return (this->m_byte >> this->shift()) & typename maskproxy_shift_count_type::bitmask_low_aligned();
		}
	};
	template <typename R>
	struct maskproxy_assignable_type : maskproxy_byte_reference<R>
	{
		DXX_INHERIT_CONSTRUCTORS(maskproxy_assignable_type, maskproxy_byte_reference<R>);
		using typename maskproxy_shift_count_type::bitmask_low_aligned;
		maskproxy_assignable_type &operator=(const uint8_t u)
		{
			assert(!(u & ~bitmask_low_aligned()));
			auto &byte = this->m_byte;
			byte = (byte & ~this->mask()) | (u << this->shift());
			return *this;
		}
		maskproxy_assignable_type &operator|=(const uint8_t u)
		{
			assert(!(u & ~bitmask_low_aligned()));
			this->m_byte |= (u << this->shift());
			return *this;
		}
	};
	template <typename R, typename A>
	static R make_maskproxy(A &a, const size_type segnum)
	{
		const size_type idx = segnum / divisor;
		if (idx >= a.size())
			throw std::out_of_range("index exceeds segment range");
		const size_type bit = segnum % divisor;
		return R(a[idx], bit);
	}
public:
	/* Explicitly invoke the default constructor for `a` so that the
	 * storage is cleared.
	 */
	visited_segment_mask_base() :
		a()
	{
	}
	void clear()
	{
		a = {};
	}
};

template <>
template <typename R>
struct visited_segment_mask_base<1>::maskproxy_assignable_type : maskproxy_byte_reference<R>
{
	DXX_INHERIT_CONSTRUCTORS(maskproxy_assignable_type, maskproxy_byte_reference<R>);
	maskproxy_assignable_type &operator=(const bool b)
	{
		const auto m = this->mask();
		auto &byte = this->m_byte;
		if (b)
			byte |= m;
		else
			byte &= ~m;
		return *this;
	}
};

/* This type must be separate so that its members are defined after the
 * specialization of maskproxy_assignable_type.  If these are defined
 * inline in visited_segment_mask_base, gcc crashes.
 * Known bad:
 *	gcc-4.9.4
 *	gcc-5.4.0
 *	gcc-6.4.0
 *	gcc-7.2.0
 */
template <unsigned bits>
class visited_segment_mask_t : visited_segment_mask_base<bits>
{
	using base_type = visited_segment_mask_base<bits>;
public:
	auto operator[](const typename base_type::size_type segnum)
	{
		return this->template make_maskproxy<typename base_type::template maskproxy_assignable_type<typename base_type::array_t::reference>>(this->a, segnum);
	}
	auto operator[](const typename base_type::size_type segnum) const
	{
		return this->template make_maskproxy<typename base_type::template maskproxy_assignable_type<typename base_type::array_t::const_reference>>(this->a, segnum);
	}
};

}

#ifdef dsx
namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
struct d_level_shared_segment_state : ::dcx::d_level_shared_segment_state
{
	d_level_shared_destructible_light_state DestructibleLights;
	segnum_t Secret_return_segment;
	vms_matrix Secret_return_orient;
};
#endif

extern d_level_shared_segment_state LevelSharedSegmentState;

}
#endif
#endif
