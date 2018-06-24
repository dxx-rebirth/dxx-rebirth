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

enum side_type : uint8_t
{
	SIDE_IS_QUAD = 1,		// render side as quadrilateral
	SIDE_IS_TRI_02 = 2,	// render side as two triangles, triangulated along edge from 0 to 2
	SIDE_IS_TRI_13 = 3,	 // render side as two triangles, triangulated along edge from 1 to 3
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
	inline void set_type(unsigned t);
	wallnum_t wall_num;
	array<vms_vector, 2> normals;  // 2 normals, if quadrilateral, both the same.
};

struct unique_side
{
	int16_t tmap_num;
	int16_t tmap_num2;
	array<uvl, 4>     uvls;
};

struct side : unique_side, shared_side
{
};

#ifdef dsx
struct shared_segment
{
#if DXX_USE_EDITOR
	segnum_t   segnum;     // segment number, not sure what it means
	short   group;      // group number to which the segment belongs.
#endif
	array<segnum_t, MAX_SIDES_PER_SEGMENT>   children;    // indices of 6 children segments, front, left, top, right, bottom, back
	array<unsigned, MAX_VERTICES_PER_SEGMENT> verts;    // vertex ids of 4 front and 4 back vertices
	uint8_t special;    // what type of center this is
	int8_t matcen_num; // which center segment is associated with.
	uint8_t station_idx;
	/* if DXX_BUILD_DESCENT_II */
	uint8_t s2_flags;
	/* endif */
	array<side, MAX_SIDES_PER_SEGMENT>    sides;       // 6 sides
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
};

struct segment : unique_segment, shared_segment
{
};
#endif

struct count_segment_array_t : public count_array_t<segnum_t, MAX_SEGMENTS> {};

struct group
{
	struct segment_array_type_t : public count_segment_array_t {};
	struct vertex_array_type_t : public count_array_t<unsigned, MAX_VERTICES> {};
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

DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(vertex, vert, Vertices);
#define Highest_vertex_index (Vertices.get_count() - 1)
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

void shared_side::set_type(const unsigned t)
{
	switch (t)
	{
		case SIDE_IS_QUAD:
		case SIDE_IS_TRI_02:
		case SIDE_IS_TRI_13:
			set_type(static_cast<side_type>(t));
			break;
		default:
			throw illegal_type(*this);
	}
}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
struct delta_light : prohibit_void_ptr<delta_light>
{
	segnum_t   segnum;
	uint8_t   sidenum;
	array<ubyte, 4>   vert_light;
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

DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(dl_index, dlindex, Dl_indices);
#endif

}
#endif

namespace dcx {

template <unsigned bits>
class visited_segment_mask_base
{
	static_assert(bits == 1 || bits == 2 || bits == 4, "bits must align in bytes");
protected:
	enum
	{
		divisor = 8 / bits,
	};
	using array_t = array<uint8_t, (MAX_SEGMENTS + (divisor - 1)) / divisor>;
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
#endif
