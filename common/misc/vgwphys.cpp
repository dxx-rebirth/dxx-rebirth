#define DXX_VG_DECLARE_EXTERN_C(F)	\
	decltype(F) __real_##F, __wrap_##F

#define DXX_VG_DEFINE_READ(F,V)	\
	__attribute__((__used__,__noinline__,__noclone__))	\
	int __wrap_##F(PHYSFS_File *const file, V *const val) {	\
		dxx_vg_wrap_poison_value(*val);	\
		return __real_##F(file, val);	\
	}

#define DXX_VG_DEFINE_WRITE(F,V)	\
	__attribute__((__used__,__noinline__,__noclone__))	\
	int __wrap_##F(PHYSFS_File *const file, V val) {	\
		dxx_vg_wrap_check_value(__builtin_return_address(0), &val, sizeof(val));	\
		return __real_##F(file, val);	\
	}

#define DXX_VG_DECLARE_READ_HELPER	\
template <typename T>	\
static void dxx_vg_wrap_poison_value(T &);

#define DXX_VG_DECLARE_WRITE_HELPER	\
static void dxx_vg_wrap_check_value(const void *, const void *, unsigned long);

#include "vg-wrap-physfs.h"

/*
 * This pattern is required due to a limitation in the PCH scanning
 * logic.  That logic will copy preprocessor guard directives along with
 * the include lines, but does not copy other lines.  Thus, writing:
 *
 * ```
 *		#ifndef PREPROCESSOR_SYMBOL
 *		#define PREPROCESSOR_SYMBOL	1
 *		#endif
 *
 *		#if PREPROCESSOR_SYMBOL
 *		#include <header>
 *		#endif
 * ```
 *
 * will cause it to copy only the second block into the generated PCH
 * CPP file.  Since the generated CPP will not set a value for
 * PREPROCESSOR_SYMBOL, the #if generates an error.
 *
 * This limitation is often hidden by a special-case that drops the
 * guards if an unguarded include exists.  In this case, this file
 * includes a header (`<cstring>`) not included unguarded in any source
 * file, so the special case does not apply.
 *
 * Compensate for this limitation by using only defined() checks, not
 * value checks, for the header includes.  This causes the guarded
 * headers not to be included in the PCH.  Omission from the PCH
 * should never affect correctness.  It can affect performance, but only
 * if the omission causes multiple source files to pay the performance
 * penalty of parsing the file individually instead of using a parsed
 * copy from the PCH.  In this instance, no other source files include
 * the headers without a guard, so no other files would benefit from
 * including it in the PCH.
 */
#if defined(DXX_ENABLE_wrap_PHYSFS_read) && !DXX_ENABLE_wrap_PHYSFS_read
#undef DXX_ENABLE_wrap_PHYSFS_read
#endif

#if defined(DXX_ENABLE_wrap_PHYSFS_write) && !DXX_ENABLE_wrap_PHYSFS_write
#undef DXX_ENABLE_wrap_PHYSFS_write
#endif

#if defined(DXX_ENABLE_wrap_PHYSFS_read) || defined(DXX_ENABLE_wrap_PHYSFS_write)
#include "console.h"
#include "dxxerror.h"
#include "compiler-poison.h"

#ifdef DXX_ENABLE_wrap_PHYSFS_read
#include <cstring>

template <typename T>
static void dxx_vg_wrap_poison_value(T &val)
{
	DXX_POISON_VAR(val, 0xbd);
}

__attribute__((__used__,__noinline__,__noclone__))
PHYSFS_sint64 __wrap_PHYSFS_read(PHYSFS_File *const handle, void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	{
		auto p = reinterpret_cast<uint8_t *>(buffer);
#if DXX_HAVE_POISON_OVERWRITE
		std::memset(p, 0xbd, static_cast<size_t>(objSize) * static_cast<size_t>(objCount));
#endif
		/* Mark each object individually so that any diagnosed errors are
		 * more precise.
		 */
		for (PHYSFS_uint32 i = objCount; i--; p += objSize)
			DXX_MAKE_MEM_UNDEFINED(p, objSize);
	}
	return __real_PHYSFS_read(handle, buffer, objSize, objCount);
}
#endif

#ifdef DXX_ENABLE_wrap_PHYSFS_write
static void dxx_vg_wrap_check_value(const void *const ret, const void *const val, const unsigned long vlen)
{
	if (const auto o = VALGRIND_CHECK_MEM_IS_DEFINED(val, vlen))
		con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "BUG: ret=%p VALGRIND_CHECK_MEM_IS_DEFINED(%p, %lu)=%p (offset=%p)"), ret, val, vlen, reinterpret_cast<void *>(o), reinterpret_cast<void *>(o - reinterpret_cast<uintptr_t>(val)));
}

__attribute__((__used__,__noinline__,__noclone__))
PHYSFS_sint64 __wrap_PHYSFS_write(PHYSFS_File *const handle, const void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	auto p = reinterpret_cast<const uint8_t *>(buffer);
	/* Check each object individually so that any diagnosed errors are
	 * more precise.
	 */
	for (PHYSFS_uint32 i = 0; i != objCount; ++i, p += objSize)
	{
		if (const auto o = VALGRIND_CHECK_MEM_IS_DEFINED(p, objSize))
			con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "BUG: ret=%p buffer=%p i=%.3u/%.3u VALGRIND_CHECK_MEM_IS_DEFINED(%p, %u)=%p (offset=%p)"), __builtin_return_address(0), buffer, i, objCount, p, objSize, reinterpret_cast<void *>(o), reinterpret_cast<void *>(o - reinterpret_cast<uintptr_t>(p)));
	}
	return __real_PHYSFS_write(handle, buffer, objSize, objCount);
}
#endif
#endif
