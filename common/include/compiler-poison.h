#pragma once

#if DXX_HAVE_POISON
#include <algorithm>
#endif

#include <memory>

#if DXX_HAVE_POISON_VALGRIND
#include <valgrind/memcheck.h>
#endif

/* Inform Valgrind that the memory range contains writable but
 * unreadable bytes.
 *
 * On non-Valgrind runs, this is a no-op.
 */
template <typename T>
static inline void DXX_MAKE_MEM_UNDEFINED(T *b, unsigned long l)
{
	(void)b;(void)l;
#if DXX_HAVE_POISON_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(b, l);
#define DXX_HAVE_POISON_UNDEFINED 1
#endif
}

template <typename T>
static inline void DXX_CHECK_MEM_IS_DEFINED(const T *const b, unsigned long l)
{
	(void)b;(void)l;
#if DXX_HAVE_POISON_VALGRIND
	VALGRIND_CHECK_MEM_IS_DEFINED(b, l);
#endif
}

/* Convenience function to invoke
 * `DXX_MAKE_MEM_UNDEFINED(T *, unsigned long)`
 */
template <typename T>
static inline void DXX_MAKE_MEM_UNDEFINED(T *b, T *e)
{
	unsigned char *bc = reinterpret_cast<unsigned char *>(b);
	DXX_MAKE_MEM_UNDEFINED(bc, reinterpret_cast<unsigned char *>(e) - bc);
}

/* Convenience function to invoke
 * `DXX_CHECK_MEM_IS_DEFINED(T *, unsigned long)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_CHECK_VAR_IS_DEFINED(const T &b)
{
	const unsigned char *const bc = reinterpret_cast<const unsigned char *>(std::addressof(b));
	DXX_CHECK_MEM_IS_DEFINED(bc, sizeof(T));
#if DXX_HAVE_POISON
#ifdef __SANITIZE_ADDRESS__
#define DXX_READ_BUILTIN_TYPE_IF_SIZE(TYPE)	\
	if constexpr (sizeof(T) == sizeof(TYPE))	\
	{	\
		asm("" :: "rm" (*reinterpret_cast<const TYPE *>(bc)));	\
	}	\
	else	\

	DXX_READ_BUILTIN_TYPE_IF_SIZE(uintptr_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint64_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint32_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint16_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint8_t)
#undef DXX_READ_BUILTIN_TYPE_IF_SIZE
	{
		auto i = bc;
		auto e = reinterpret_cast<const unsigned char *>(bc + sizeof(T));
		for (; i != e; ++i)
			asm("" :: "rm" (*(i)));
	}
#endif
#endif
}

/* Convenience function to invoke
 * `DXX_MAKE_MEM_UNDEFINED(T *, unsigned long)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_MAKE_VAR_UNDEFINED(T &b)
{
	unsigned char *const bc = reinterpret_cast<unsigned char *>(std::addressof(b));
	DXX_MAKE_MEM_UNDEFINED(bc, sizeof(T));
}

/* If enabled, overwrite the specified memory range with copies of value `v`.
 * In poison=overwrite builds, this is always enabled.
 * In poison=valgrind builds, this is enabled if running under Valgrind.
 *
 * If disabled, this is a no-op.
 */
template <typename T, typename V>
static inline void DXX_POISON_MEMORY_RANGE(T b, T e, const V &v)
{
#if DXX_HAVE_POISON
	int store = DXX_HAVE_POISON_OVERWRITE;
#if DXX_HAVE_POISON_VALGRIND
	if (!store)
		store |= RUNNING_ON_VALGRIND;
#endif
	if (!store)
		return;
	std::fill(b, e, v);
#else
	(void)b;(void)e;(void)v;
#endif
}

/* Poison a memory range, but do not mark it as unreadable.  This is
 * necessary when poisoning a buffer that will be written to the file or
 * to a network peer and some of the poisoned fields are expected not to
 * be updated (but cannot be removed from the source because the layout
 * is mandatory).
 */
template <typename T, typename V>
static inline void DXX_POISON_DEFINED_MEMORY(T b, unsigned long l, const V &v)
{
	DXX_POISON_MEMORY_RANGE(b, b + l, v);
}

/* Poison a memory range, then mark it unreadable.
 */
template <typename T>
static inline void DXX_POISON_MEMORY(T b, unsigned long l, const uint8_t v)
{
	DXX_POISON_DEFINED_MEMORY(b, l, v);
	DXX_MAKE_MEM_UNDEFINED(b, l);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY(T &, unsigned long, V)`
 */
template <typename T>
static inline void DXX_POISON_MEMORY(T b, T e, const uint8_t &&v)
{
	DXX_POISON_MEMORY_RANGE(b, e, v);
	DXX_MAKE_MEM_UNDEFINED(b, e);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY(T &, unsigned long, V)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_POISON_VAR(T &b, const uint8_t v)
{
	DXX_POISON_MEMORY(reinterpret_cast<unsigned char *>(std::addressof(b)), sizeof(T), v);
}

/* Convenience function to invoke
 * `DXX_POISON_DEFINED_MEMORY(T &, unsigned long, V)` for exactly one
 * instance of `T`.
 */
template <typename T>
static inline void DXX_POISON_DEFINED_VAR(T &b, const unsigned char v)
{
	DXX_POISON_DEFINED_MEMORY(reinterpret_cast<unsigned char *>(std::addressof(b)), sizeof(T), v);
}

#ifndef DXX_HAVE_POISON_UNDEFINED
#define DXX_HAVE_POISON_UNDEFINED	0
#endif
