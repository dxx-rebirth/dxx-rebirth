#pragma once

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
 * `DXX_MAKE_MEM_UNDEFINED(T *, unsigned long)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_MAKE_VAR_UNDEFINED(T &b)
{
	unsigned char *const bc = reinterpret_cast<unsigned char *>(&b);
	DXX_MAKE_MEM_UNDEFINED(bc, sizeof(T));
}

/* If enabled, overwrite the specified memory range with copies of value `v`.
 * In poison=overwrite builds, this is always enabled.
 * In poison=valgrind builds, this is enabled if running under Valgrind.
 *
 * If disabled, this is a no-op.
 */
template <typename T, typename V>
static inline void _DXX_POISON_MEMORY_RANGE(T b, T e, const V &v)
{
#if DXX_HAVE_POISON
	int store = DXX_HAVE_POISON_OVERWRITE;
#if DXX_HAVE_POISON_VALGRIND
	if (!store)
		store |= RUNNING_ON_VALGRIND;
#endif
	if (!store)
		return;
	for (; b != e; ++b)
		*b = v;
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
	_DXX_POISON_MEMORY_RANGE(b, b + l, v);
}

/* Poison a memory range, then mark it unreadable.
 */
template <typename T, typename V>
static inline void DXX_POISON_MEMORY(T b, unsigned long l, const V &v)
{
	DXX_POISON_DEFINED_MEMORY(b, l, v);
	DXX_MAKE_MEM_UNDEFINED(b, l);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY(T &, unsigned long, V)`
 */
template <typename T, typename V>
static inline void DXX_POISON_MEMORY(T b, T e, const V &v)
{
	_DXX_POISON_MEMORY_RANGE(b, e, v);
	DXX_MAKE_MEM_UNDEFINED(b, e);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY(T &, unsigned long, V)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_POISON_VAR(T &b, const unsigned char v)
{
	DXX_POISON_MEMORY(reinterpret_cast<unsigned char *>(&b), sizeof(T), v);
}

/* Convenience function to invoke
 * `DXX_POISON_DEFINED_MEMORY(T &, unsigned long, V)` for exactly one
 * instance of `T`.
 */
template <typename T>
static inline void DXX_POISON_DEFINED_VAR(T &b, const unsigned char v)
{
	DXX_POISON_DEFINED_MEMORY(reinterpret_cast<unsigned char *>(&b), sizeof(T), v);
}

#ifndef DXX_HAVE_POISON_UNDEFINED
#define DXX_HAVE_POISON_UNDEFINED	0
#endif
