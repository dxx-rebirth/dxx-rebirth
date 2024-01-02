/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Some simple physfs extensions
 *
 */

#pragma once

#include <cstddef>
#include <cstring>
#include <memory>
#include <ranges>
#include <span>
#include <stdarg.h>
#include <type_traits>

// When PhysicsFS can *easily* be built as a framework on Mac OS X,
// the framework form will be supported again -kreatordxx
#if 1	//!(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "fmtcheck.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "dxxerror.h"
#include "vecmat.h"
#include "byteutil.h"

#include <stdexcept>
#include "u_mem.h"
#include "pack.h"
#include "ntstring.h"
#include "fwd-partial_range.h"
#include <array>
#include <memory>
#include "backports-ranges.h"

#ifdef DXX_CONSTANT_TRUE
#define _DXX_PHYSFS_CHECK_SIZE(S,C,v)	DXX_CONSTANT_TRUE((std::size_t{S} * std::size_t{C}) > (v))
#define DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S,C,v)	\
	(void)(__builtin_object_size(v, 1) != static_cast<size_t>(-1) && _DXX_PHYSFS_CHECK_SIZE(S,C,__builtin_object_size(v, 1)) && (DXX_ALWAYS_ERROR_FUNCTION("read size exceeds element size"), 0))
#define DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S,C)	\
	(void)(_DXX_PHYSFS_CHECK_SIZE(S,C,sizeof(v)) && (DXX_ALWAYS_ERROR_FUNCTION("read size exceeds array size"), 0))
#define DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S,C,v)	\
	(void)(__builtin_object_size(v, 1) != static_cast<size_t>(-1) && _DXX_PHYSFS_CHECK_SIZE(S,C,__builtin_object_size(v, 1)) && (DXX_ALWAYS_ERROR_FUNCTION("write size exceeds element size"), 0))
#define DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C)	\
	((void)(dxx_builtin_constant_p(S) || dxx_builtin_constant_p(C) || \
		(DXX_ALWAYS_ERROR_FUNCTION("array element size is not constant"), 0)))
#define DXX_PHYSFS_CHECK_WRITE_SIZE_ARRAY_SIZE(S,C)	\
	((void)(_DXX_PHYSFS_CHECK_SIZE(S,C,sizeof(v)) && \
		(DXX_ALWAYS_ERROR_FUNCTION("write size exceeds array size"), 0)))
#else
#define DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S,C,v)	((void)0)
#define DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S,C)	((void)0)
#define DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S,C,v)	((void)0)
#define DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C) ((void)0)
#define DXX_PHYSFS_CHECK_WRITE_SIZE_ARRAY_SIZE(S,C) ((void)0)
#endif

#define DXX_PHYSFS_CHECK_WRITE_CONSTANTS(S,C)	\
	((void)(DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C),	\
	DXX_PHYSFS_CHECK_WRITE_SIZE_ARRAY_SIZE(S,C), 0))	\

namespace dcx {

template <typename V>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, V *v, const PHYSFS_uint32 S, const PHYSFS_uint32 C)
{
	static_assert(std::is_standard_layout<V>::value && std::is_trivial<V>::value, "non-POD value read");
	DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_read(file, v, S, C);
}

template <typename V, std::size_t N>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, std::array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_standard_layout<V>::value && std::is_trivial<V>::value, "C++ array of non-POD elements read");
	DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S, C);
	return PHYSFSX_check_read(file, &v[0], S, C);
}

template <typename V, typename D>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, const std::unique_ptr<V, D> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_read(file, v.get(), S, C);
}

template <typename V>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, const V *v, const PHYSFS_uint32 S, const PHYSFS_uint32 C)
{
	static_assert(std::is_standard_layout<V>::value && std::is_trivial<V>::value, "non-POD value written");
	if constexpr (std::is_integral<V>::value)
		DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C);
	DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_write(file, v, S, C);
}

template <typename V, std::size_t N>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, const std::array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_standard_layout<V>::value && std::is_trivial<V>::value, "C++ array of non-POD elements written");
	DXX_PHYSFS_CHECK_WRITE_CONSTANTS(S,C);
	return PHYSFSX_check_write(file, &v[0], S, C);
}

template <typename T, typename D>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, const std::unique_ptr<T, D> &p, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_write(file, p.get(), S, C);
}

template <typename V>
PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, exact_type<V> v, PHYSFS_uint32 S, PHYSFS_uint32 C) = delete;
template <typename V>
PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, exact_type<V> v, PHYSFS_uint32 S, PHYSFS_uint32 C) = delete;

template <typename V>
PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, V **v, PHYSFS_uint32 S, PHYSFS_uint32 C) = delete;
template <typename V>
PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, V **v, PHYSFS_uint32 S, PHYSFS_uint32 C) = delete;
#define PHYSFS_read(F,V,S,C)	PHYSFSX_check_read(F,V,S,C)
#define PHYSFS_write(F,V,S,C)	PHYSFSX_check_write(F,V,S,C)

static inline PHYSFS_sint16 PHYSFSX_readSXE16(PHYSFS_File *file, int swap)
{
	PHYSFS_sint16 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPSHORT(val) : val;
}

static inline PHYSFS_sint32 PHYSFSX_readSXE32(PHYSFS_File *file, int swap)
{
	PHYSFS_sint32 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPINT(val) : val;
}

static inline int PHYSFSX_writeU8(PHYSFS_File *file, PHYSFS_uint8 val)
{
	return PHYSFS_write(file, &val, 1, 1);
}

static inline int PHYSFSX_writeString(PHYSFS_File *file, const char *s)
{
	return PHYSFS_write(file, s, 1, strlen(s) + 1);
}

static inline auto PHYSFSX_puts(PHYSFS_File *file, const std::span<const char> s)
{
	return PHYSFS_write(file, s.data(), 1, s.size());
}

static inline auto PHYSFSX_puts_literal(PHYSFS_File *file, const std::span<const char> s)
{
	return PHYSFS_write(file, s.data(), 1, s.size() - 1);
}

static inline int PHYSFSX_fgetc(PHYSFS_File *const fp)
{
	unsigned char c;

	if (PHYSFS_read(fp, &c, 1, 1) != 1)
		return EOF;

	return c;
}

static inline int PHYSFSX_fseek(PHYSFS_File *fp, long int offset, int where)
{
	int c, goal_position;

	switch(where)
	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = PHYSFS_tell(fp) + offset;
		break;
	case SEEK_END:
		goal_position = PHYSFS_fileLength(fp) + offset;
		break;
	default:
		return 1;
	}
	c = PHYSFS_seek(fp, goal_position);
	return !c;
}

template <std::size_t N>
struct PHYSFSX_gets_line_t
{
	PHYSFSX_gets_line_t() = default;
	PHYSFSX_gets_line_t(const PHYSFSX_gets_line_t &) = delete;
	PHYSFSX_gets_line_t &operator=(const PHYSFSX_gets_line_t &) = delete;
	using line_t = std::array<char, N>;
#if DXX_HAVE_POISON
	/* Force onto heap to improve checker accuracy */
	std::unique_ptr<line_t> m_line;
	const line_t &line() const { return *m_line.get(); }
	line_t &line() { return *m_line.get(); }
	std::span<char, N> next()
	{
		m_line = std::make_unique<line_t>();
		return *m_line.get();
	}
#else
	line_t m_line;
	const line_t &line() const { return m_line; }
	line_t &line() { return m_line; }
	std::span<char, N> next() { return m_line; }
#endif
	operator line_t &() { return line(); }
	operator const line_t &() const { return line(); }
	operator char *() { return line().data(); }
	operator const char *() const { return line().data(); }
	typename line_t::reference operator[](typename line_t::size_type i) { return line()[i]; }
	typename line_t::reference operator[](int i) { return operator[](static_cast<typename line_t::size_type>(i)); }
	typename line_t::const_reference operator[](typename line_t::size_type i) const { return line()[i]; }
	typename line_t::const_reference operator[](int i) const { return operator[](static_cast<typename line_t::size_type>(i)); }
	constexpr std::size_t size() const { return N; }
	typename line_t::const_iterator begin() const { return line().begin(); }
	typename line_t::const_iterator end() const { return line().end(); }
};

template <>
struct PHYSFSX_gets_line_t<0>
{
#define DXX_ALLOCATE_PHYSFS_LINE(n)	std::make_unique<char[]>(n)
#if !DXX_HAVE_POISON
	const
#endif
	std::unique_ptr<char[]> m_line;
	const std::size_t m_length;
	PHYSFSX_gets_line_t(const std::size_t n) :
#if !DXX_HAVE_POISON
		m_line(DXX_ALLOCATE_PHYSFS_LINE(n)),
#endif
		m_length(n)
	{
	}
	char *line() { return m_line.get(); }
	const char *line() const { return m_line.get(); }
	std::span<char> next()
	{
#if DXX_HAVE_POISON
		/* Reallocate to tell checker to undefine the buffer */
		m_line = DXX_ALLOCATE_PHYSFS_LINE(m_length);
#endif
		return std::span<char>(m_line.get(), m_length);
	}
	std::size_t size() const { return m_length; }
	operator const char *() const { return m_line.get(); }
	const char *begin() const { return *this; }
	const char *end() const { return begin() + m_length; }
	operator const void *() const = delete;
#undef DXX_ALLOCATE_PHYSFS_LINE
};

class PHYSFSX_fgets_t
{
	[[nodiscard]]
	static char *get(std::span<char> buf, PHYSFS_File *const fp);
	template <std::size_t Extent>
		[[nodiscard]]
		static char *get(const std::span<char, Extent> buf, std::size_t offset, PHYSFS_File *const fp)
	{
		if (offset > buf.size())
			throw std::invalid_argument("offset too large");
		return get(buf.subspan(offset), fp);
	}
public:
	template <std::size_t n>
		[[nodiscard]]
		__attribute_nonnull()
		char *operator()(PHYSFSX_gets_line_t<n> &buf, PHYSFS_File *const fp, std::size_t offset = 0) const
		{
			return get(buf.next(), offset, fp);
		}
	template <std::size_t n>
		[[nodiscard]]
		__attribute_nonnull()
		char *operator()(ntstring<n> &buf, PHYSFS_File *const fp, std::size_t offset = 0) const
		{
			auto r = get(std::span(buf), offset, fp);
			buf.back() = 0;
			return r;
		}
};

constexpr PHYSFSX_fgets_t PHYSFSX_fgets{};

int PHYSFSX_printf(PHYSFS_File *file, const char *format) = delete;

__attribute_format_printf(2, 3)
static inline int PHYSFSX_printf(PHYSFS_File *file, const char *format, ...)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
	const std::size_t len = std::max(vsnprintf(buffer, sizeof(buffer), format, args), 0);
	va_end(args);

	return PHYSFSX_puts(file, {buffer, len});
}

#define PHYSFSX_writeFix	PHYSFS_writeSLE32
#define PHYSFSX_writeFixAng	PHYSFS_writeSLE16

static inline int PHYSFSX_writeVector(PHYSFS_File *file, const vms_vector &v)
{
	if (PHYSFSX_writeFix(file, v.x) < 1 ||
		PHYSFSX_writeFix(file, v.y) < 1 ||
		PHYSFSX_writeFix(file, v.z) < 1)
		return 0;

	return 1;
}

[[noreturn]]
__attribute_cold
void PHYSFSX_read_helper_report_error(const char *const filename, const unsigned line, const char *const func, PHYSFS_File *const file);

template <typename T, auto F>
struct PHYSFSX_read_helper
{
	T operator()(PHYSFS_File *file, const char *filename = __builtin_FILE(), unsigned line = __builtin_LINE(), const char *func = __builtin_FUNCTION()) const;
};

template <typename T, auto F>
T PHYSFSX_read_helper<T, F>::operator()(PHYSFS_File *const file, const char *const filename, const unsigned line, const char *const func) const
{
	T i;
	if (!F(file, &i))
		PHYSFSX_read_helper_report_error(filename, line, func, file);
	return i;
}

template <typename T1, int (*F)(PHYSFS_File *, T1 *), typename T2, T1 T2::*m1, T1 T2::*m2, T1 T2::*m3>
static void PHYSFSX_read_sequence_helper(const char *const filename, const unsigned line, const char *const func, PHYSFS_File *const file, T2 *const i)
{
	if (unlikely(!F(file, &(i->*m1)) ||
		!F(file, &(i->*m2)) ||
		!F(file, &(i->*m3))))
		PHYSFSX_read_helper_report_error(filename, line, func, file);
}

static inline int PHYSFSX_readS8(PHYSFS_File *const file, int8_t *const b)
{
	return (PHYSFS_read(file, b, sizeof(*b), 1) == 1);
}

static constexpr PHYSFSX_read_helper<int8_t, PHYSFSX_readS8> PHYSFSX_readByte{};
static constexpr PHYSFSX_read_helper<int16_t, PHYSFS_readSLE16> PHYSFSX_readShort{};
static constexpr PHYSFSX_read_helper<int32_t, PHYSFS_readSLE32> PHYSFSX_readInt{};
static constexpr PHYSFSX_read_helper<fix, PHYSFS_readSLE32> PHYSFSX_readFix{};
static constexpr PHYSFSX_read_helper<fixang, PHYSFS_readSLE16> PHYSFSX_readFixAng{};
#define PHYSFSX_readVector(F,V)	(PHYSFSX_read_sequence_helper<fix, PHYSFS_readSLE32, vms_vector, &vms_vector::x, &vms_vector::y, &vms_vector::z>(__FILE__, __LINE__, __func__, (F), &(V)))
#define PHYSFSX_readAngleVec(V,F)	(PHYSFSX_read_sequence_helper<fixang, PHYSFS_readSLE16, vms_angvec, &vms_angvec::p, &vms_angvec::b, &vms_angvec::h>(__FILE__, __LINE__, __func__, (F), (V)))

static inline void PHYSFSX_readMatrix(const char *const filename, const unsigned line, const char *const func, vms_matrix *const m, PHYSFS_File *const file)
{
	auto &PHYSFSX_readVector = PHYSFSX_read_sequence_helper<fix, PHYSFS_readSLE32, vms_vector, &vms_vector::x, &vms_vector::y, &vms_vector::z>;
	(PHYSFSX_readVector)(filename, line, func, file, &m->rvec);
	(PHYSFSX_readVector)(filename, line, func, file, &m->uvec);
	(PHYSFSX_readVector)(filename, line, func, file, &m->fvec);
}

#define PHYSFSX_readMatrix(M,F)	((PHYSFSX_readMatrix)(__FILE__, __LINE__, __func__, (M), (F)))

class PHYSFS_File_deleter
{
public:
	int operator()(PHYSFS_File *fp) const
	{
		return PHYSFS_close(fp);
	}
};

class RAIIPHYSFS_File : public std::unique_ptr<PHYSFS_File, PHYSFS_File_deleter>
{
	typedef std::unique_ptr<PHYSFS_File, PHYSFS_File_deleter> base_t;
public:
	using base_t::base_t;
	using base_t::operator bool;
	operator PHYSFS_File *() const && = delete;
	operator PHYSFS_File *() const &
	{
		return get();
	}
	int close()
	{
		/* Like reset(), but returns result */
		int r = get_deleter()(get());
		if (r)
			release();
		return r;
	}
	template <typename T>
		bool operator==(T) const = delete;
	template <typename T>
		bool operator!=(T) const = delete;
};

typedef char file_extension_t[5];

[[nodiscard]]
__attribute_nonnull()
int PHYSFSX_checkMatchingExtension(const char *filename, const ranges::subrange<const file_extension_t *> range);

enum class physfs_search_path : int
{
	prepend,
	append,
};

PHYSFS_ErrorCode PHYSFSX_addRelToSearchPath(char *relname, std::array<char, PATH_MAX> &realPath, physfs_search_path);
void PHYSFSX_removeRelFromSearchPath(const char *relname);
extern int PHYSFSX_fsize(const char *hogname);
extern void PHYSFSX_listSearchPathContent();
[[nodiscard]]
int PHYSFSX_getRealPath(const char *stdPath, std::array<char, PATH_MAX> &realPath);

class PHYSFS_unowned_storage_mount_deleter
{
public:
	void operator()(const char *const p) const noexcept
	{
		PHYSFS_unmount(p);
	}
};

class PHYSFS_computed_path_mount_deleter : PHYSFS_unowned_storage_mount_deleter, std::default_delete<std::array<char, PATH_MAX>>
{
public:
	using element_type = std::array<char, PATH_MAX>;
	PHYSFS_computed_path_mount_deleter() = default;
	PHYSFS_computed_path_mount_deleter(const PHYSFS_computed_path_mount_deleter &) = default;
	PHYSFS_computed_path_mount_deleter(PHYSFS_computed_path_mount_deleter &&) = default;
	PHYSFS_computed_path_mount_deleter(std::default_delete<element_type> &&d) :
		std::default_delete<element_type>{std::move(d)}
	{
	}
	PHYSFS_computed_path_mount_deleter &operator=(PHYSFS_computed_path_mount_deleter &&) = default;
	void operator()(element_type *const p) const noexcept
	{
		PHYSFS_unowned_storage_mount_deleter::operator()(p->data());
		std::default_delete<element_type>::operator()(p);
	}
};

/* RAIIPHYSFS_LiteralMount takes a pointer to storage, but does not take
 * ownership of the underlying storage.  On destruction, it will pass
 * that pointer to PHYSFS_unmount.  The pointer must remain valid until
 * RAIIPHYSFS_LiteralMount is destroyed.
 */
using RAIIPHYSFS_LiteralMount = std::unique_ptr<const char, PHYSFS_unowned_storage_mount_deleter>;
/* RAIIPHYSFS_ComputedPathMount owns a pointer to allocated storage.  On
 * destruction, it will pass that pointer to PHYSFS_unmount, then free
 * the pointer.
 */
using RAIIPHYSFS_ComputedPathMount = std::unique_ptr<typename PHYSFS_computed_path_mount_deleter::element_type, PHYSFS_computed_path_mount_deleter>;

[[nodiscard]]
RAIIPHYSFS_ComputedPathMount make_PHYSFSX_ComputedPathMount(char *const name, physfs_search_path position);

extern int PHYSFSX_rename(const char *oldpath, const char *newpath);

#define PHYSFSX_exists(F,I)	((I) ? PHYSFSX_exists_ignorecase(F) : PHYSFS_exists(F))
int PHYSFSX_exists_ignorecase(const char *filename);
std::pair<RAIIPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openReadBuffered(const char *filename);
std::pair<RAIIPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openWriteBuffered(const char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();
}

#ifdef dsx
namespace dsx {

bool PHYSFSX_init(int argc, char *argv[]);
int PHYSFSX_checkSupportedArchiveTypes();
#if defined(DXX_BUILD_DESCENT_II)
RAIIPHYSFS_ComputedPathMount make_PHYSFSX_ComputedPathMount(char *name1, char *name2, physfs_search_path);
#endif

}
#endif
