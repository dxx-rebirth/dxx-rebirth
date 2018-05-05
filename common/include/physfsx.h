/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
#include <memory>
#include <string.h>
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

#ifdef __cplusplus
#include <stdexcept>
#include "u_mem.h"
#include "pack.h"
#include "ntstring.h"
#include "compiler-array.h"
#include "compiler-make_unique.h"
#include "partial_range.h"

#ifdef DXX_CONSTANT_TRUE
#define _DXX_PHYSFS_CHECK_SIZE_CONSTANT(S,v)	DXX_CONSTANT_TRUE((S) > (v))
#define _DXX_PHYSFS_CHECK_SIZE(S,C,v)	_DXX_PHYSFS_CHECK_SIZE_CONSTANT(static_cast<size_t>(S) * static_cast<size_t>(C), v)
#define DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S,C,v)	\
	(void)(__builtin_object_size(v, 1) != static_cast<size_t>(-1) && _DXX_PHYSFS_CHECK_SIZE(S,C,__builtin_object_size(v, 1)) && (DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_overwrite, "read size exceeds element size"), 0))
#define DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S,C)	\
	(void)(_DXX_PHYSFS_CHECK_SIZE(S,C,sizeof(v)) && (DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_overwrite, "read size exceeds array size"), 0))
#define DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S,C,v)	\
	(void)(__builtin_object_size(v, 1) != static_cast<size_t>(-1) && _DXX_PHYSFS_CHECK_SIZE(S,C,__builtin_object_size(v, 1)) && (DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_overwrite, "write size exceeds element size"), 0))
#define DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C)	\
	((void)(dxx_builtin_constant_p(S) || dxx_builtin_constant_p(C) || \
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_nonconstant_size, "array element size is not constant"), 0)))
#define DXX_PHYSFS_CHECK_WRITE_SIZE_ARRAY_SIZE(S,C)	\
	((void)(_DXX_PHYSFS_CHECK_SIZE(S,C,sizeof(v)) && \
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_overread, "write size exceeds array size"), 0)))
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
static inline typename std::enable_if<std::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_read(PHYSFS_File *file, V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "non-POD integral value read");
	DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_read(file, v, S, C);
}

template <typename V>
__attribute_always_inline()
static inline typename std::enable_if<!std::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_read(PHYSFS_File *file, V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "non-POD non-integral value read");
	DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_read(file, v, S, C);
}

template <typename V, std::size_t N>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_File *file, array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "C++ array of non-POD elements read");
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
static inline typename std::enable_if<std::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_write(PHYSFS_File *file, const V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "non-POD integral value written");
	DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C);
	DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_write(file, v, S, C);
}

template <typename V>
__attribute_always_inline()
static inline typename std::enable_if<!std::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_write(PHYSFS_File *file, const V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "non-POD non-integral value written");
	DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_write(file, v, S, C);
}

template <typename V, std::size_t N>
__attribute_always_inline()
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_File *file, const array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(std::is_pod<V>::value, "C++ array of non-POD elements written");
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

static inline int PHYSFSX_puts(PHYSFS_File *file, const char *s, size_t len) __attribute_nonnull();
static inline int PHYSFSX_puts(PHYSFS_File *file, const char *s, size_t len)
{
	return PHYSFS_write(file, s, 1, len);
}

template <size_t len>
static inline int PHYSFSX_puts_literal(PHYSFS_File *file, const char (&s)[len]) __attribute_nonnull();
template <size_t len>
static inline int PHYSFSX_puts_literal(PHYSFS_File *file, const char (&s)[len])
{
	return PHYSFSX_puts(file, s, len - 1);
}
#define PHYSFSX_puts(A1,S,...)	(PHYSFSX_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))

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
	PHYSFSX_gets_line_t(PHYSFSX_gets_line_t &&) = default;
	PHYSFSX_gets_line_t &operator=(PHYSFSX_gets_line_t &&) = default;
	typedef array<char, N> line_t;
#if DXX_HAVE_POISON
	/* Force onto heap to improve checker accuracy */
	std::unique_ptr<line_t> m_line;
	const line_t &line() const { return *m_line.get(); }
	line_t &line() { return *m_line.get(); }
	line_t &next()
	{
		m_line = make_unique<line_t>();
		return *m_line.get();
	}
#else
	line_t m_line;
	const line_t &line() const { return m_line; }
	line_t &line() { return m_line; }
	line_t &next() { return m_line; }
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
#define DXX_ALLOCATE_PHYSFS_LINE(n)	make_unique<char[]>(n)
	std::unique_ptr<char[]> m_line;
	std::size_t m_length;
	PHYSFSX_gets_line_t(std::size_t n) :
#if !DXX_HAVE_POISON
		m_line(DXX_ALLOCATE_PHYSFS_LINE(n)),
#endif
		m_length(n)
	{
	}
	char *line() { return m_line.get(); }
	const char *line() const { return m_line.get(); }
	char *next()
	{
#if DXX_HAVE_POISON
		/* Reallocate to tell checker to undefine the buffer */
		m_line = DXX_ALLOCATE_PHYSFS_LINE(m_length);
#endif
		return m_line.get();
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
	static char *get(char *buf, std::size_t n, PHYSFS_File *const fp);
	static char *get(char *buf, std::size_t offset, std::size_t n, PHYSFS_File *const fp)
	{
		if (offset > n)
			throw std::invalid_argument("offset too large");
		return get(&buf[offset], n - offset, fp);
	}
public:
	template <std::size_t n>
		__attribute_nonnull()
		char *operator()(PHYSFSX_gets_line_t<n> &buf, PHYSFS_File *const fp, std::size_t offset = 0) const
		{
			return get(&buf.next()[0], offset, buf.size(), fp);
		}
	template <std::size_t n>
		__attribute_nonnull()
		char *operator()(ntstring<n> &buf, PHYSFS_File *const fp, std::size_t offset = 0) const
		{
			auto r = get(&buf.data()[0], offset, buf.size(), fp);
			buf.back() = 0;
			return r;
		}
};

constexpr PHYSFSX_fgets_t PHYSFSX_fgets{};

static inline int PHYSFSX_printf(PHYSFS_File *file, const char *format, ...) __attribute_format_printf(2, 3);
static inline int PHYSFSX_printf(PHYSFS_File *file, const char *format, ...)
#define PHYSFSX_printf(A1,F,...)	dxx_call_printf_checked(PHYSFSX_printf,PHYSFSX_puts_literal,(A1),(F),##__VA_ARGS__)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
	size_t len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	return PHYSFSX_puts(file, buffer, len);
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

__attribute_cold
__attribute_noreturn
void PHYSFSX_read_helper_report_error(const char *const filename, const unsigned line, const char *const func, PHYSFS_File *const file);

template <typename T, int (*F)(PHYSFS_File *, T *)>
static T PHYSFSX_read_helper(const char *const filename, const unsigned line, const char *const func, PHYSFS_File *const file)
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

#define PHYSFSX_readByte(F)	(PHYSFSX_read_helper<int8_t, PHYSFSX_readS8>(__FILE__, __LINE__, __func__, (F)))
#define PHYSFSX_readShort(F)	(PHYSFSX_read_helper<int16_t, PHYSFS_readSLE16>(__FILE__, __LINE__, __func__, (F)))
#define PHYSFSX_readInt(F)	(PHYSFSX_read_helper<int32_t, PHYSFS_readSLE32>(__FILE__, __LINE__, __func__, (F)))
#define PHYSFSX_readFix(F)	(PHYSFSX_read_helper<fix, PHYSFS_readSLE32>(__FILE__, __LINE__, __func__, (F)))
#define PHYSFSX_readFixAng(F)	(PHYSFSX_read_helper<fixang, PHYSFS_readSLE16>(__FILE__, __LINE__, __func__, (F)))
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

#define PHYSFSX_contfile_init PHYSFSX_addRelToSearchPath
#define PHYSFSX_contfile_close PHYSFSX_removeRelFromSearchPath

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
	DXX_INHERIT_CONSTRUCTORS(RAIIPHYSFS_File, base_t);
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
__attribute_nonnull()
__attribute_warn_unused_result
int PHYSFSX_checkMatchingExtension(const char *filename, const partial_range_t<const file_extension_t *>);

template <std::size_t count>
__attribute_nonnull()
__attribute_warn_unused_result
static inline int PHYSFSX_checkMatchingExtension(const array<file_extension_t, count> &exts, const char *filename)
{
	return PHYSFSX_checkMatchingExtension(filename, exts);
}

extern int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end);
extern int PHYSFSX_removeRelFromSearchPath(const char *relname);
extern int PHYSFSX_fsize(const char *hogname);
extern void PHYSFSX_listSearchPathContent();
int PHYSFSX_getRealPath(const char *stdPath, char *realPath, std::size_t);

template <std::size_t N>
static inline int PHYSFSX_getRealPath(const char *stdPath, array<char, N> &realPath)
{
	return PHYSFSX_getRealPath(stdPath, realPath.data(), N);
}

extern int PHYSFSX_isNewPath(const char *path);
extern int PHYSFSX_rename(const char *oldpath, const char *newpath);

#define PHYSFSX_exists(F,I)	((I) ? PHYSFSX_exists_ignorecase(F) : PHYSFS_exists(F))
int PHYSFSX_exists_ignorecase(const char *filename);
RAIIPHYSFS_File PHYSFSX_openReadBuffered(const char *filename);
RAIIPHYSFS_File PHYSFSX_openWriteBuffered(const char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();
}

#ifdef dsx
namespace dsx {

bool PHYSFSX_init(int argc, char *argv[]);
int PHYSFSX_checkSupportedArchiveTypes();

}
#endif

#endif
