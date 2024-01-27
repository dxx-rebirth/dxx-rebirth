#define DXX_VG_DECLARE_EXTERN_C(F)	\
	decltype(F) __real_##F

#define DXX_VG_DEFINE_READ(F,V)	\
	int __real_##F(PHYSFS_File *const file, V *const val) {	\
		return (F)(file, val);	\
	}

#define DXX_VG_DEFINE_WRITE(F,V)	\
	int __real_##F(PHYSFS_File *const file, V val) {	\
		return (F)(file, val);	\
	}

#include "vg-wrap-physfs.h"

#if DXX_ENABLE_wrap_PHYSFS_read
PHYSFS_sint64 __real_PHYSFS_read(PHYSFS_File *const handle, void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	return (PHYSFS_read)(handle, buffer, objSize, objCount);
}

PHYSFS_sint64 __real_PHYSFS_readBytes(PHYSFS_File *const handle, void *const buffer, const PHYSFS_uint64 len)
{
	return (PHYSFS_readBytes)(handle, buffer, len);
}
#endif

#if DXX_ENABLE_wrap_PHYSFS_write
PHYSFS_sint64 __real_PHYSFS_write(PHYSFS_File *const handle, const void *const buffer, const PHYSFS_uint32 objSize, const PHYSFS_uint32 objCount)
{
	return (PHYSFS_write)(handle, buffer, objSize, objCount);
}
#endif
