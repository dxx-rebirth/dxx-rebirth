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

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <physfs.h>
#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "dxxsconf.h"

#ifdef __cplusplus
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include "countarray.h"
#include "objnum.h"
#include "segnum.h"
#include "pack.h"

#include "compiler-type_traits.h"

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

// Set maximum values for segment and face data structures.
static const std::size_t MAX_VERTICES_PER_SEGMENT = 8;
static const std::size_t MAX_SIDES_PER_SEGMENT = 6;
static const std::size_t MAX_VERTICES_PER_POLY = 4;
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

static const std::size_t MAX_SEGMENTS_ORIGINAL = 900;
static const std::size_t MAX_SEGMENT_VERTICES_ORIGINAL = 4*MAX_SEGMENTS_ORIGINAL;
static const std::size_t MAX_SEGMENTS = 9000;
static const std::size_t MAX_SEGMENT_VERTICES = 4*MAX_SEGMENTS;

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#ifdef EDITOR   //verts for the new segment
# define NUM_NEW_SEG_VERTICES   8
# define NEW_SEGMENT_VERTICES   (MAX_SEGMENT_VERTICES)
static const std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES + NUM_NEW_SEG_VERTICES;
#else           //No editor
static const std::size_t MAX_VERTICES = MAX_SEGMENT_VERTICES;
#endif

// Returns true if segnum references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
static inline bool IS_CHILD(segnum_t s)
{
	return s != segment_none && s != segment_exit;
}

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

struct segment;

template <int16_t I>
struct wall_magic_constant_t;

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
			static_assert(tt::is_integral<T>::value, "non-integral wall number compared");
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

struct side
{
	struct illegal_type : std::runtime_error
	{
		const segment *m_segment;
		const side *m_side;
		illegal_type(const segment *seg, const side *s) :
			std::runtime_error("illegal side type"),
			m_segment(seg), m_side(s)
		{
		}
		illegal_type(const side *s) :
			std::runtime_error("illegal side type"),
			m_segment(nullptr), m_side(s)
		{
		}
	};
	side_type m_type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	const side_type &get_type() const { return m_type; }
	void set_type(side_type t) { m_type = t; }
	void set_type(unsigned t)
	{
		switch (t)
		{
			case SIDE_IS_QUAD:
			case SIDE_IS_TRI_02:
			case SIDE_IS_TRI_13:
				set_type(static_cast<side_type>(t));
				break;
			default:
				throw illegal_type(this);
		}
	}
	wallnum_t wall_num;
	short   tmap_num;
	short   tmap_num2;
	array<uvl, 4>     uvls;
	array<vms_vector, 2> normals;  // 2 normals, if quadrilateral, both the same.
};

struct segment {
#ifdef EDITOR
	segnum_t   segnum;     // segment number, not sure what it means
	short   group;      // group number to which the segment belongs.
#endif
	objnum_t objects;    // pointer to objects in this segment
	array<segnum_t, MAX_SIDES_PER_SEGMENT>   children;    // indices of 6 children segments, front, left, top, right, bottom, back
	//      If bit n (1 << n) is set, then side #n in segment has had light subtracted from original (editor-computed) value.
	ubyte light_subtracted;
	array<side, MAX_SIDES_PER_SEGMENT>    sides;       // 6 sides
	array<int, MAX_VERTICES_PER_SEGMENT>     verts;    // vertex ids of 4 front and 4 back vertices
	ubyte   special;    // what type of center this is
	sbyte   matcen_num; // which center segment is associated with.
#if defined(DXX_BUILD_DESCENT_I)
	short   value;
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte   value;
	ubyte   s2_flags;
#endif
	fix     static_light;
};

#if defined(DXX_BUILD_DESCENT_II)
#define Segment2s Segments

#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02
#endif

//values for special field
#define SEGMENT_IS_NOTHING      0
#define SEGMENT_IS_FUELCEN      1
#define SEGMENT_IS_REPAIRCEN    2
#define SEGMENT_IS_CONTROLCEN   3
#define SEGMENT_IS_ROBOTMAKER   4
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_CENTER_TYPES        5
#elif defined(DXX_BUILD_DESCENT_II)
#define SEGMENT_IS_GOAL_BLUE    5
#define SEGMENT_IS_GOAL_RED     6
#define MAX_CENTER_TYPES        7
#endif


// Local segment data.
// This is stuff specific to a segment that does not need to get
// written to disk.  This is a handy separation because we can add to
// this structure without obsoleting existing data on disk.

#define SS_REPAIR_CENTER    0x01    // Bitmask for this segment being part of repair center.

//--repair-- typedef struct {
//--repair-- 	int     special_type;
//--repair-- 	short   special_segment; // if special_type indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

struct count_segment_array_t : public count_array_t<short, MAX_SEGMENTS> {};

struct group
{
	struct segment_array_type_t : public count_segment_array_t {};
	struct vertex_array_type_t : public count_array_t<int, MAX_VERTICES> {};
	segment_array_type_t segments;
	vertex_array_type_t vertices;
	void clear()
	{
		segments.clear();
		vertices.clear();
	}
};

struct segment_array_t : public array<segment, MAX_SEGMENTS>
{
	unsigned highest;
#define Highest_segment_index Segments.highest
	typedef array<segment, MAX_SEGMENTS> array_t;
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, reference>::type operator[](T n)
		{
			return array_t::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, const_reference>::type operator[](T n) const
		{
			return array_t::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<!tt::is_integral<T>::value, reference>::type operator[](T) const = delete;
	segment_array_t() = default;
	segment_array_t(const segment_array_t &) = delete;
	segment_array_t &operator=(const segment_array_t &) = delete;
};

// Globals from mglobal.c
#define Segment2s Segments
struct vertex : vms_vector
{
	vertex() = default;
	vertex(const fix &a, const fix &b, const fix &c) :
		/* gcc 4.7 and later support brace initializing the base class
		 * gcc 4.6 requires the explicit temporary
		 */
		vms_vector(vms_vector{a, b, c})
	{
	}
	explicit vertex(const vms_vector &v) :
		vms_vector(v)
	{
	}
};
extern array<vertex, MAX_VERTICES> Vertices;
extern segment_array_t Segments;
extern unsigned Num_segments;
extern unsigned Num_vertices;

static const std::size_t MAX_EDGES = MAX_VERTICES * 4;

static inline segnum_t operator-(const segment *s, const segment_array_t &S)
{
	return segnum_t(static_cast<uint16_t>(s - (&*S.begin())));
}

extern const sbyte Side_to_verts[MAX_SIDES_PER_SEGMENT][4];       // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const int  Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4];    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const char Side_opposite[MAX_SIDES_PER_SEGMENT];                                // Side_opposite[my_side] returns side opposite cube from my_side.

#define SEG_PTR_2_NUM(segptr) (Assert((unsigned) (segptr-Segments)<MAX_SEGMENTS),(segptr)-Segments)

#if defined(DXX_BUILD_DESCENT_II)
// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
struct delta_light {
	segnum_t   segnum;
	sbyte   sidenum;
	sbyte   dummy;
	ubyte   vert_light[4];
};

// Light at segnum:sidenum casts light on count sides beginning at index (in array Delta_lights)
struct dl_index {
	segnum_t   segnum;
	sbyte   sidenum;
	sbyte   count;
	short   index;
};

#define MAX_DL_INDICES      500
#define MAX_DELTA_LIGHTS    10000

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

extern dl_index     Dl_indices[MAX_DL_INDICES];
extern delta_light  Delta_lights[MAX_DELTA_LIGHTS];
extern int          Num_static_lights;

int subtract_light(segnum_t segnum, int sidenum);
int add_light(segnum_t segnum, int sidenum);
extern void restore_all_lights_in_mine(void);
extern void clear_light_subtracted(void);
#endif

void segment_side_wall_tmap_write(PHYSFS_file *fp, const side &side);

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the segment data structure directly.  Use these
// functions instead.  The segment data structure is GUARANTEED to
// change MANY TIMES.  If you read the segment data structure
// directly, your code will break, I PROMISE IT!

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
extern void med_get_face_vertex_list(segment *s,int side, int facenum,int *nv,int **vp);

// Set *nf = number of faces in segment s.
extern void med_get_num_faces(segment *s,int *nf);

void med_validate_segment_side(segment *sp,int side);

// Delete segment from group
extern void delete_segment_from_group(int segment_num, int group_num);

// Add segment to group
extern void add_segment_to_group(int segment_num, int group_num);

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a segment2 structure from a PHYSFS_file
 */
void segment2_read(segment *s2, PHYSFS_file *fp);

/*
 * reads a delta_light structure from a PHYSFS_file
 */
void delta_light_read(delta_light *dl, PHYSFS_file *fp);

/*
 * reads a dl_index structure from a PHYSFS_file
 */
void dl_index_read(dl_index *di, PHYSFS_file *fp);

void segment2_write(const segment *s2, PHYSFS_file *fp);
void delta_light_write(delta_light *dl, PHYSFS_file *fp);
void dl_index_write(dl_index *di, PHYSFS_file *fp);
#endif

template <typename T, unsigned bits>
class visited_segment_mask_t
{
	static_assert(bits == 1 || bits == 2 || bits == 4, "bits must align in bytes");
protected:
	enum
	{
		divisor = 8 / bits,
	};
	typedef array<ubyte, (MAX_SEGMENTS + (divisor - 1)) / divisor> array_t;
	typedef typename array_t::size_type size_type;
	array_t a;
	struct base_maskproxy_t
	{
		unsigned m_shift;
		unsigned shift() const
		{
			return m_shift * bits;
		}
		static unsigned bitmask_low_aligned()
		{
			return (1 << bits) - 1;
		}
		typename array_t::value_type mask() const
		{
			return bitmask_low_aligned() << shift();
		}
		base_maskproxy_t(unsigned shift) :
			m_shift(shift)
		{
		}
	};
	template <typename R>
	struct tmpl_maskproxy_t : public base_maskproxy_t
	{
		R m_byte;
		tmpl_maskproxy_t(R byte, unsigned shift) :
			base_maskproxy_t(shift), m_byte(byte)
		{
		}
	};
	template <typename R, typename A>
	static R make_maskproxy(A &a, size_type segnum)
	{
		size_type idx = segnum / divisor;
		if (idx >= a.size())
			throw std::out_of_range("index exceeds segment range");
		size_type bit = segnum % divisor;
		return R(a[idx], bit);
	}
public:
	visited_segment_mask_t()
	{
		clear();
	}
	void clear()
	{
		a.fill(0);
	}
};

class visited_segment_bitarray_t : public visited_segment_mask_t<bool, 1>
{
	template <typename R>
	struct tmpl_bitproxy_t : public tmpl_maskproxy_t<R>
	{
		tmpl_bitproxy_t(R byte, unsigned shift) :
			tmpl_maskproxy_t<R>(byte, shift)
		{
		}
		dxx_explicit_operator_bool operator bool() const
		{
			return !!(this->m_byte & this->mask());
		}
		operator int() const DXX_CXX11_EXPLICIT_DELETE;
	};
	struct bitproxy_t : public tmpl_bitproxy_t<array_t::reference>
	{
		bitproxy_t(array_t::reference byte, unsigned shift) :
			tmpl_bitproxy_t<array_t::reference>(byte, shift)
		{
		}
		bitproxy_t& operator=(bool b)
		{
			if (b)
				this->m_byte |= this->mask();
			else
				this->m_byte &= ~this->mask();
			return *this;
		}
		bitproxy_t& operator=(int) DXX_CXX11_EXPLICIT_DELETE;
	};
	typedef tmpl_bitproxy_t<array_t::const_reference> const_bitproxy_t;
public:
	bitproxy_t operator[](size_type segnum)
	{
		return make_maskproxy<bitproxy_t>(a, segnum);
	}
	const_bitproxy_t operator[](size_type segnum) const
	{
		return make_maskproxy<const_bitproxy_t>(a, segnum);
	}
};

template <unsigned bits>
class visited_segment_multibit_array_t : public visited_segment_mask_t<unsigned, bits>
{
	typedef typename visited_segment_mask_t<unsigned, bits>::array_t array_t;
	typedef typename visited_segment_mask_t<unsigned, bits>::size_type size_type;
	template <typename R>
	struct tmpl_multibit_proxy_t : public visited_segment_mask_t<unsigned, bits>::template tmpl_maskproxy_t<R>
	{
		tmpl_multibit_proxy_t(R byte, unsigned shift) :
			visited_segment_mask_t<unsigned, bits>::template tmpl_maskproxy_t<R>(byte, shift)
		{
		}
		dxx_explicit_operator_bool operator bool() const
		{
			return !!(this->m_byte & this->mask());
		}
		operator unsigned() const
		{
			return (this->m_byte >> this->shift()) & this->bitmask_low_aligned();
		}
	};
	struct bitproxy_t : public tmpl_multibit_proxy_t<typename array_t::reference>
	{
		bitproxy_t(typename array_t::reference byte, unsigned shift) :
			tmpl_multibit_proxy_t<typename array_t::reference>(byte, shift)
		{
		}
		bitproxy_t& operator=(unsigned u)
		{
			assert(!(u & ~this->bitmask_low_aligned()));
			this->m_byte = (this->m_byte & ~this->mask()) | (u << this->shift());
			return *this;
		}
	};
	typedef tmpl_multibit_proxy_t<typename array_t::const_reference> const_bitproxy_t;
public:
	bitproxy_t operator[](size_type segnum)
	{
		return this->template make_maskproxy<bitproxy_t>(this->a, segnum);
	}
	const_bitproxy_t operator[](size_type segnum) const
	{
		return this->template make_maskproxy<const_bitproxy_t>(this->a, segnum);
	}
};

const int side_none = -1;
const int edge_none = -1;
#endif

#endif
