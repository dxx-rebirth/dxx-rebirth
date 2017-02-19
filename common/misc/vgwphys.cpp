#define DXX_VG_DECLARE_EXTERN_C(F)	\
	decltype(F) __real_##F, __wrap_##F

#define DXX_VG_DEFINE_WRITE(F,V)	\
	int __wrap_##F(PHYSFS_File *const file, V val) {	\
		dxx_vg_wrap_check_value(__builtin_return_address(0), &val, sizeof(val));	\
		return __real_##F(file, val);	\
	}

#define DXX_VG_DECLARE_WRITE_HELPER	\
static void dxx_vg_wrap_check_value(const void *, const void *, unsigned long);

#include "vg-wrap-physfs.h"

#if DXX_ENABLE_wrap_PHYSFS_write
#include "console.h"
#include "dxxerror.h"
#include "compiler-poison.h"

static void dxx_vg_wrap_check_value(const void *const ret, const void *const val, const unsigned long vlen)
{
	if (const auto o = VALGRIND_CHECK_MEM_IS_DEFINED(val, vlen))
		con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "BUG: ret=%p VALGRIND_CHECK_MEM_IS_DEFINED(%p, %lu)=%p (offset=%p)"), ret, val, vlen, reinterpret_cast<void *>(o), reinterpret_cast<void *>(o - reinterpret_cast<uintptr_t>(val)));
}

PHYSFS_sint64 __wrap_PHYSFS_write(PHYSFS_File *const handle, const void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(buffer);
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
