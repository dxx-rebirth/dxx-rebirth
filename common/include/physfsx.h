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

#ifndef PHYSFSX_H
#define PHYSFSX_H

#include <cstddef>
#include <memory>
#include <string.h>
#include <stdarg.h>

// When PhysicsFS can *easily* be built as a framework on Mac OS X,
// the framework form will be supported again -kreatordxx
#if 1	//!(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "fmtcheck.h"
#include "dxxsconf.h"
#include "dxxerror.h"
#include "vecmat.h"
#include "byteutil.h"

#ifdef __cplusplus
#include "u_mem.h"
#include "pack.h"
#include "compiler-array.h"
#include "compiler-static_assert.h"
#include "compiler-type_traits.h"

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define _DXX_PHYSFS_CHECK_SIZE_CONSTANT(S,v)	(__builtin_constant_p((S) > v) && (S) > v)
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

template <typename V>
static inline typename tt::enable_if<tt::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_read(PHYSFS_file *file, V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "non-POD integral value read");
	DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_read(file, v, S, C);
}

template <typename V>
static inline typename tt::enable_if<!tt::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_read(PHYSFS_file *file, V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "non-POD non-integral value read");
	DXX_PHYSFS_CHECK_READ_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_read(file, v, S, C);
}

template <typename V>
static inline typename tt::enable_if<tt::is_array<V>::value, PHYSFS_sint64>::type PHYSFSX_check_read(PHYSFS_file *file, V &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	typedef typename tt::remove_extent<V>::type V0;
	static_assert(tt::is_pod<V0>::value, "C array of non-POD elements read");
	DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S, C);
	return PHYSFSX_check_read(file, &v[0], S, C);
}

template <typename V, std::size_t N>
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_file *file, array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "C++ array of non-POD elements read");
	DXX_PHYSFS_CHECK_READ_SIZE_ARRAY_SIZE(S, C);
	return PHYSFSX_check_read(file, &v[0], S, C);
}

template <typename V, typename D>
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_file *file, const std::unique_ptr<V, D> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_read(file, v.get(), S, C);
}

template <typename V>
static inline PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_file *file, const RAIIdmem<V> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_read(file, static_cast<V *>(v), S, C);
}

template <typename V>
static inline typename tt::enable_if<tt::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_write(PHYSFS_file *file, const V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "non-POD integral value written");
	DXX_PHYSFS_CHECK_WRITE_ELEMENT_SIZE_CONSTANT(S,C);
	DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_write(file, v, S, C);
}

template <typename V>
static inline typename tt::enable_if<!tt::is_integral<V>::value, PHYSFS_sint64>::type PHYSFSX_check_write(PHYSFS_file *file, const V *v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "non-POD non-integral value written");
	DXX_PHYSFS_CHECK_WRITE_SIZE_OBJECT_SIZE(S, C, v);
	return PHYSFS_write(file, v, S, C);
}

template <typename V, std::size_t N>
static inline typename tt::enable_if<tt::is_array<V>::value, PHYSFS_sint64>::type PHYSFSX_check_write(PHYSFS_file *file, const V (&v)[N], PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	typedef typename tt::remove_extent<V>::type V0;
	static_assert(tt::is_pod<V0>::value, "C array of non-POD elements written");
	DXX_PHYSFS_CHECK_WRITE_CONSTANTS(S,C);
	return PHYSFSX_check_write(file, &v[0], S, C);
}

template <typename V, std::size_t N>
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_file *file, const array<V, N> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	static_assert(tt::is_pod<V>::value, "C++ array of non-POD elements written");
	DXX_PHYSFS_CHECK_WRITE_CONSTANTS(S,C);
	return PHYSFSX_check_write(file, &v[0], S, C);
}

template <typename T, typename D>
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_file *file, const std::unique_ptr<T, D> &p, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_write(file, p.get(), S, C);
}

template <typename V>
static inline PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_file *file, const RAIIdmem<V> &v, PHYSFS_uint32 S, PHYSFS_uint32 C)
{
	return PHYSFSX_check_write(file, static_cast<V *>(v), S, C);
}

template <typename V>
PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_file *file, exact_type<V> v, PHYSFS_uint32 S, PHYSFS_uint32 C) DXX_CXX11_EXPLICIT_DELETE;
template <typename V>
PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_file *file, exact_type<V> v, PHYSFS_uint32 S, PHYSFS_uint32 C) DXX_CXX11_EXPLICIT_DELETE;

template <typename V>
PHYSFS_sint64 PHYSFSX_check_read(PHYSFS_file *file, V **v, PHYSFS_uint32 S, PHYSFS_uint32 C) DXX_CXX11_EXPLICIT_DELETE;
template <typename V>
PHYSFS_sint64 PHYSFSX_check_write(PHYSFS_file *file, V **v, PHYSFS_uint32 S, PHYSFS_uint32 C) DXX_CXX11_EXPLICIT_DELETE;
#define PHYSFS_read(F,V,S,C)	PHYSFSX_check_read(F,V,S,C)
#define PHYSFS_write(F,V,S,C)	PHYSFSX_check_write(F,V,S,C)

extern void PHYSFSX_init(int argc, char *argv[]);

static inline PHYSFS_sint16 PHYSFSX_readSXE16(PHYSFS_file *file, int swap)
{
	PHYSFS_sint16 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPSHORT(val) : val;
}

static inline PHYSFS_sint32 PHYSFSX_readSXE32(PHYSFS_file *file, int swap)
{
	PHYSFS_sint32 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPINT(val) : val;
}

static inline void PHYSFSX_readVectorX(PHYSFS_file *file, vms_vector *v, int swap)
{
	v->x = PHYSFSX_readSXE32(file, swap);
	v->y = PHYSFSX_readSXE32(file, swap);
	v->z = PHYSFSX_readSXE32(file, swap);
}

static inline void PHYSFSX_readAngleVecX(PHYSFS_file *file, vms_angvec *v, int swap)
{
	v->p = PHYSFSX_readSXE16(file, swap);
	v->b = PHYSFSX_readSXE16(file, swap);
	v->h = PHYSFSX_readSXE16(file, swap);
}

static inline int PHYSFSX_writeU8(PHYSFS_file *file, PHYSFS_uint8 val)
{
	return PHYSFS_write(file, &val, 1, 1);
}

static inline int PHYSFSX_writeString(PHYSFS_file *file, const char *s)
{
	return PHYSFS_write(file, s, 1, strlen(s) + 1);
}

static inline int PHYSFSX_puts(PHYSFS_file *file, const char *s, size_t len) __attribute_nonnull();
static inline int PHYSFSX_puts(PHYSFS_file *file, const char *s, size_t len)
{
	return PHYSFS_write(file, s, 1, len);
}

template <size_t len>
static inline int PHYSFSX_puts_literal(PHYSFS_file *file, const char (&s)[len]) __attribute_nonnull();
template <size_t len>
static inline int PHYSFSX_puts_literal(PHYSFS_file *file, const char (&s)[len])
{
	return PHYSFSX_puts(file, s, len - 1);
}
#define PHYSFSX_puts(A1,S,...)	(PHYSFSX_puts(A1,S, _dxx_call_puts_parameter2(1, ## __VA_ARGS__, strlen(S))))

static inline int PHYSFSX_putc(PHYSFS_file *file, int c)
{
	unsigned char ch = (unsigned char)c;

	if (PHYSFS_write(file, &ch, 1, 1) < 1)
		return -1;
	else
		return (int)c;
}

static inline int PHYSFSX_fgetc(PHYSFS_file *const fp)
{
	unsigned char c;

	if (PHYSFS_read(fp, &c, 1, 1) != 1)
		return EOF;

	return c;
}

static inline int PHYSFSX_fseek(PHYSFS_file *fp, long int offset, int where)
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
	struct line_t
	{
		char buf[N];
	};
#ifdef DXX_HAVE_POISON
	/* Force onto heap to improve checker accuracy */
	std::unique_ptr<line_t> m_line;
	decltype(line_t::buf) &line() { return m_line->buf; }
	decltype(line_t::buf) &next()
	{
		m_line.reset(new line_t);
		return line();
	}
#else
	line_t m_line;
	decltype(line_t::buf) &line() { return m_line.buf; }
	decltype(line_t::buf) &next() { return line(); }
#endif
	operator decltype(line_t::buf) &() { return line(); }
	operator const decltype(line_t::buf) &() const { return line(); }
	std::size_t size() const { return N; }
};

template <>
struct PHYSFSX_gets_line_t<0>
{
	std::unique_ptr<char[]> m_line;
	std::size_t m_length;
	PHYSFSX_gets_line_t(std::size_t n) :
		m_line(new char[n]),
		m_length(n)
	{
	}
	char *line() { return m_line.get(); }
	char *next()
	{
#ifdef DXX_HAVE_POISON
		m_line.reset(new char[m_length]);
#endif
		return line();
	}
	std::size_t size() const { return m_length; }
	operator const char *() const { return m_line.get(); }
	const char *begin() const { return *this; }
	const char *end() const { return begin() + m_length; }
};

char *PHYSFSX_fgets(char *buf, size_t n, PHYSFS_file *const fp);

template <std::size_t n>
static inline char * PHYSFSX_fgets(PHYSFSX_gets_line_t<n> &buf, PHYSFS_file *const fp, std::size_t offset = 0)
{
	return PHYSFSX_fgets(buf.next() + offset, buf.size() - offset, fp);
}

template <size_t n>
static inline char * PHYSFSX_fgets(char (&buf)[n], PHYSFS_file *const fp)
{
	return PHYSFSX_fgets(buf, n, fp);
}

static inline int PHYSFSX_printf(PHYSFS_file *file, const char *format, ...) __attribute_format_printf(2, 3);
static inline int PHYSFSX_printf(PHYSFS_file *file, const char *format, ...)
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

static inline int PHYSFSX_writeVector(PHYSFS_file *file, vms_vector *v)
{
	if (PHYSFSX_writeFix(file, v->x) < 1 ||
	 PHYSFSX_writeFix(file, v->y) < 1 ||
	 PHYSFSX_writeFix(file, v->z) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeAngleVec(PHYSFS_file *file, vms_angvec *v)
{
	if (PHYSFSX_writeFixAng(file, v->p) < 1 ||
	 PHYSFSX_writeFixAng(file, v->b) < 1 ||
	 PHYSFSX_writeFixAng(file, v->h) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeMatrix(PHYSFS_file *file, vms_matrix *m)
{
	if (PHYSFSX_writeVector(file, &m->rvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->uvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->fvec) < 1)
		return 0;

	return 1;
}

#define define_read_helper(T,N,F)	\
	static inline T N(const char *func, const unsigned line, PHYSFS_file *file)	\
	{	\
		T i;	\
		if (!(F)(file, &i))	\
		{	\
			(Error)(func, line, "reading " #T " in " #N "() at %lu", (unsigned long)((PHYSFS_tell)(file)));	\
		}	\
		return i;	\
	}

static inline sbyte PHYSFSX_readS8(PHYSFS_file *file, sbyte *b)
{
	return (PHYSFS_read(file, b, sizeof(*b), 1) == 1);
}

define_read_helper(sbyte, PHYSFSX_readByte, PHYSFSX_readS8);
#define PHYSFSX_readByte(F)	((PHYSFSX_readByte)(__func__, __LINE__, (F)))

define_read_helper(int, PHYSFSX_readInt, PHYSFS_readSLE32);
#define PHYSFSX_readInt(F)	((PHYSFSX_readInt)(__func__, __LINE__, (F)))

define_read_helper(int16_t, PHYSFSX_readShort, PHYSFS_readSLE16);
#define PHYSFSX_readShort(F)	((PHYSFSX_readShort)(__func__, __LINE__, (F)))

define_read_helper(fix, PHYSFSX_readFix, PHYSFS_readSLE32);
#define PHYSFSX_readFix(F)	((PHYSFSX_readFix)(__func__, __LINE__, (F)))

define_read_helper(fixang, PHYSFSX_readFixAng, PHYSFS_readSLE16);
#define PHYSFSX_readFixAng(F)	((PHYSFSX_readFixAng)(__func__, __LINE__, (F)))

static inline void PHYSFSX_readVector(const char *func, const unsigned line, vms_vector *v, PHYSFS_file *file)
{
	v->x = (PHYSFSX_readFix)(func, line, file);
	v->y = (PHYSFSX_readFix)(func, line, file);
	v->z = (PHYSFSX_readFix)(func, line, file);
}
#define PHYSFSX_readVector(V,F)	((PHYSFSX_readVector(__func__, __LINE__, (V), (F))))

static inline void PHYSFSX_readAngleVec(const char *func, const unsigned line, vms_angvec *v, PHYSFS_file *file)
{
	v->p = (PHYSFSX_readFixAng)(func, line, file);
	v->b = (PHYSFSX_readFixAng)(func, line, file);
	v->h = (PHYSFSX_readFixAng)(func, line, file);
}
#define PHYSFSX_readAngleVec(V,F)	((PHYSFSX_readAngleVec(__func__, __LINE__, (V), (F))))

static inline void PHYSFSX_readMatrix(const char *func, const unsigned line, vms_matrix *m,PHYSFS_file *file)
{
	(PHYSFSX_readVector)(func, line, &m->rvec,file);
	(PHYSFSX_readVector)(func, line, &m->uvec,file);
	(PHYSFSX_readVector)(func, line, &m->fvec,file);
}

#define PHYSFSX_readMatrix(M,F)	((PHYSFSX_readMatrix)(__func__, __LINE__, (M), (F)))

#define PHYSFSX_contfile_init PHYSFSX_addRelToSearchPath
#define PHYSFSX_contfile_close PHYSFSX_removeRelFromSearchPath

typedef char file_extension_t[5];
int PHYSFSX_checkMatchingExtension(const file_extension_t *exts, const char *filename) __attribute_nonnull();
extern int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end);
extern int PHYSFSX_removeRelFromSearchPath(const char *relname);
extern int PHYSFSX_fsize(const char *hogname);
extern void PHYSFSX_listSearchPathContent();
extern int PHYSFSX_checkSupportedArchiveTypes();
extern int PHYSFSX_getRealPath(const char *stdPath, char *realPath);
extern int PHYSFSX_isNewPath(const char *path);
extern int PHYSFSX_rename(const char *oldpath, const char *newpath);
extern char **PHYSFSX_findFiles(const char *path, const file_extension_t *exts) __attribute_nonnull();
extern char **PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const file_extension_t *exts) __attribute_nonnull();
extern PHYSFS_sint64 PHYSFSX_getFreeDiskSpace();
extern int PHYSFSX_exists(const char *filename, int ignorecase);
extern PHYSFS_file *PHYSFSX_openReadBuffered(const char *filename);
extern PHYSFS_file *PHYSFSX_openWriteBuffered(const char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();

#endif

#endif /* PHYSFSX_H */
