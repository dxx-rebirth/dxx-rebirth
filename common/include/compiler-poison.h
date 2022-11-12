#pragma once

#if DXX_HAVE_POISON
#include <algorithm>
#endif

#include <cstdint>
#include <memory>
#include <span>

#if DXX_HAVE_POISON_VALGRIND
#include <valgrind/memcheck.h>
#endif

namespace {

/* Inform Valgrind that the memory range contains writable but
 * unreadable bytes.
 *
 * On non-Valgrind runs, this is a no-op.
 */
[[maybe_unused]]
static void DXX_MAKE_MEM_UNDEFINED(const std::span<std::byte> s)
{
	(void)s;
#if DXX_HAVE_POISON_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(s.data(), s.size());
#define DXX_HAVE_POISON_UNDEFINED 1
#endif
}

template <typename T, std::size_t Extent>
requires(!std::is_same<T, std::byte>::value)
static inline void DXX_MAKE_MEM_UNDEFINED(const std::span<T, Extent> s)
{
	DXX_MAKE_MEM_UNDEFINED(std::as_writable_bytes(s));
}

[[maybe_unused]]
static void DXX_CHECK_MEM_IS_DEFINED(const std::span<const std::byte> s)
{
	(void)s;
#if DXX_HAVE_POISON_VALGRIND
	VALGRIND_CHECK_MEM_IS_DEFINED(s.data(), s.size());
#endif
}

template <typename T, std::size_t Extent>
requires(!std::is_same<T, std::byte>::value)
static inline void DXX_CHECK_MEM_IS_DEFINED(const std::span<const T, Extent> s)
{
	DXX_CHECK_MEM_IS_DEFINED(std::as_bytes(s));
}

/* Convenience function to invoke
 * `DXX_CHECK_MEM_IS_DEFINED(T *, unsigned long)` for exactly one instance
 * of `T`.
 */
template <typename T>
static void DXX_CHECK_VAR_IS_DEFINED(const T &b)
{
	const auto sb = std::as_bytes(std::span<const T, 1>(std::addressof(b), 1));
	DXX_CHECK_MEM_IS_DEFINED(sb);
#ifdef __SANITIZE_ADDRESS__
	constexpr bool sanitize_read_range = true;
#else
	constexpr bool sanitize_read_range = false;
#endif
	if constexpr (DXX_HAVE_POISON && sanitize_read_range)
	{
#define DXX_READ_BUILTIN_TYPE_IF_SIZE(TYPE)	\
	if constexpr (sizeof(T) == sizeof(TYPE))	\
	{	\
		asm("" :: "rm" (*reinterpret_cast<const TYPE *>(sb.data())));	\
	}	\
	else	\

	DXX_READ_BUILTIN_TYPE_IF_SIZE(uintptr_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint64_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint32_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint16_t)
	DXX_READ_BUILTIN_TYPE_IF_SIZE(uint8_t)
#undef DXX_READ_BUILTIN_TYPE_IF_SIZE
	{
		for (const auto &i : sb)
			asm("" :: "rm" (i));
	}
	}
}

/* Convenience function to invoke
 * `DXX_MAKE_MEM_UNDEFINED(T *, unsigned long)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_MAKE_VAR_UNDEFINED(T &b)
{
	DXX_MAKE_MEM_UNDEFINED(std::span<T, 1>(std::addressof(b), 1));
}

/* If enabled, overwrite the specified memory range with copies of value `v`.
 * In poison=overwrite builds, this is always enabled.
 * In poison=valgrind builds, this is enabled if running under Valgrind.
 *
 * If disabled, this is a no-op.
 */
[[maybe_unused]]
static void DXX_POISON_MEMORY_RANGE(const std::span<std::byte> sb, const std::byte v)
{
	if constexpr (DXX_HAVE_POISON)
	{
		if constexpr (!DXX_HAVE_POISON_OVERWRITE)
		{
#if DXX_HAVE_POISON_VALGRIND
			if (!RUNNING_ON_VALGRIND)
#endif
			{
				return;
			}
		}
		std::fill_n(sb.data(), sb.size(), v);
	}
	else
		(void)sb, (void)v;
}

/* Poison a memory range, but do not mark it as unreadable.  This is
 * necessary when poisoning a buffer that will be written to the file or
 * to a network peer and some of the poisoned fields are expected not to
 * be updated (but cannot be removed from the source because the layout
 * is mandatory).
 */
template <typename T, typename V, std::size_t Extent>
requires(!std::is_same<T, std::byte>::value)
static inline void DXX_POISON_MEMORY_RANGE(const std::span<T, Extent> st, const V &v)
{
	DXX_POISON_MEMORY_RANGE(std::as_writable_bytes(st), std::byte{v});
}

/* Poison a memory range, then mark it unreadable.
 */
template <typename T, std::size_t Extent>
static void DXX_POISON_MEMORY(const std::span<T, Extent> b, const uint8_t v)
{
	DXX_POISON_MEMORY_RANGE(b, std::byte{v});
	DXX_MAKE_MEM_UNDEFINED(b);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY(T &, unsigned long, V)` for exactly one instance
 * of `T`.
 */
template <typename T>
static inline void DXX_POISON_VAR(T &b, const uint8_t v)
{
	DXX_POISON_MEMORY(std::as_writable_bytes(std::span<T, 1>(std::addressof(b), 1)), v);
}

/* Convenience function to invoke
 * `DXX_POISON_MEMORY_RANGE(T &, V)` for exactly one
 * instance of `T`.
 */
template <typename T>
static inline void DXX_POISON_DEFINED_VAR(T &b, const unsigned char v)
{
	DXX_POISON_MEMORY_RANGE(std::as_writable_bytes(std::span<T, 1>(std::addressof(b), 1)), std::byte{v});
}

}

#ifndef DXX_HAVE_POISON_UNDEFINED
#define DXX_HAVE_POISON_UNDEFINED	0
#endif
