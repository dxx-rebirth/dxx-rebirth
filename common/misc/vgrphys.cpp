#define DXX_VG_DECLARE_EXTERN_C(F)	\
	decltype(F) __real_##F

#define DXX_VG_DEFINE_WRITE(F,V)	\
	int __real_##F(PHYSFS_File *const file, V val) {	\
		return (F)(file, val);	\
	}

#include "vg-wrap-physfs.h"

#if DXX_ENABLE_wrap_PHYSFS_write
PHYSFS_sint64 __real_PHYSFS_write(PHYSFS_File *const handle, const void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	return (PHYSFS_write)(handle, buffer, objSize, objCount);
}
#endif
