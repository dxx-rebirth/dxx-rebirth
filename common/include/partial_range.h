/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <stdexcept>
#include <iterator>
#include <cstdio>
#include <string>
#include <type_traits>
#include "fwd-partial_range.h"
#include "compiler-addressof.h"

/* If no value was specified for DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE,
 * then define it to true for NDEBUG builds and false for debug builds.
 *
 * When DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE is true, the partial_range
 * exception is a global scope structure.
 *
 * When DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE is false, the
 * partial_range exception is nested in partial_range_t<I>, so that the
 * exception type encodes the range type.
 *
 * Encoding the range type in the exception produces a more descriptive
 * message when the program aborts for an uncaught exception, but
 * produces larger debug information and many nearly-redundant
 * instantiations of the report function.  Using the global scope
 * structure avoids those redundant instances, but makes the exception
 * description less descriptive.
 *
 * Choose false if you expect to debug partial_range exceptions or
 * report them to someone who will debug them for you.  Choose true if
 * you want a smaller program.
 */
#ifndef DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
#ifdef NDEBUG
#define DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE	1
#else
#define DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE	0
#endif
#endif

namespace partial_range_detail
{

template <typename T>
static inline auto adl_begin(T &t) -> decltype(begin(t))
{
	return begin(t);
}

template <typename T>
static inline auto adl_end(T &t) -> decltype(end(t))
{
	return end(t);
}

}

#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
struct partial_range_error;
#endif

template <typename I>
class partial_range_t
{
public:
	typedef I iterator;
	/* When using the unminimized type, forward declare a structure.
	 *
	 * When using the minimized type, add a typedef here so that later
	 * code can unconditionally use the qualified type name.  Using a
	 * typedef here instead of a preprocessor macro at the usage sites
	 * causes the debug information to be very slightly bigger, but has
	 * no effect on the size of the generated code and is easier to
	 * maintain.
	 */
#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
	using partial_range_error =
#endif
	struct partial_range_error;
	iterator m_begin, m_end;
	partial_range_t(iterator b, iterator e) :
		m_begin(b), m_end(e)
	{
	}
	partial_range_t(const partial_range_t &) = default;
	partial_range_t(partial_range_t &&) = default;
	partial_range_t &operator=(const partial_range_t &) = default;
	template <typename T>
		partial_range_t(T &t) :
			m_begin(partial_range_detail::adl_begin(t)), m_end(partial_range_detail::adl_end(t))
	{
	}
	__attribute_warn_unused_result
	iterator begin() const { return m_begin; }
	__attribute_warn_unused_result
	iterator end() const { return m_end; }
	bool empty() const __attribute_warn_unused_result
	{
		return m_begin == m_end;
	}
	__attribute_warn_unused_result
	std::size_t size() const { return std::distance(m_begin, m_end); }
	std::reverse_iterator<iterator> rbegin() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_end}; }
	std::reverse_iterator<iterator> rend() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_begin}; }
	partial_range_t<std::reverse_iterator<iterator>> reversed() const __attribute_warn_unused_result
	{
		return {rbegin(), rend()};
	}
};

namespace partial_range_detail
{
#define REPORT_FORMAT_STRING	 "%s:%u: %s %lu past %p end %lu \"%s\""
	template <std::size_t NF, std::size_t NE>
		/* Round reporting into large buckets.  Code size is more
		 * important than stack space.
		 */
		using required_buffer_size = std::integral_constant<std::size_t, ((sizeof(REPORT_FORMAT_STRING) + sizeof("65535") + (sizeof("18446744073709551615") * 2) + sizeof("0x0000000000000000") + (NF + NE + sizeof("begin"))) | 0xff) + 1>;
	template <std::size_t N>
		__attribute_cold
	static void prepare_error_string(char (&buf)[N], unsigned long d, const char *estr, const char *file, unsigned line, const char *desc, unsigned long expr, const void *t)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, file, line, desc, expr, t, d, estr);
	}
#undef REPORT_FORMAT_STRING
}

#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
struct partial_range_error
#else
template <typename I>
struct partial_range_t<I>::partial_range_error
#endif
	final : std::out_of_range
{
	DXX_INHERIT_CONSTRUCTORS(partial_range_error, out_of_range);
	template <std::size_t N>
		__attribute_cold
		__attribute_noreturn
	static void report(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const void *t, unsigned long d)
	{
		char buf[N];
		partial_range_detail::prepare_error_string<N>(buf, d, estr, file, line, desc, expr, t);
		throw partial_range_error(buf);
	}
};

namespace partial_range_detail
{

template <typename I, std::size_t required_buffer_size>
static inline void check_range_bounds(const char *file, unsigned line, const char *estr, const void *t, const std::size_t o, const std::size_t l, const std::size_t d)
{
#ifdef DXX_CONSTANT_TRUE
	/*
	 * If EXPR and d are compile-time constant, and the (EXPR > d)
	 * branch is optimized out, then the expansion of
	 * PARTIAL_RANGE_COMPILE_CHECK_BOUND is optimized out, preventing
	 * the compile error.
	 *
	 * If EXPR and d are compile-time constant, and the (EXPR > d)
	 * branch is not optimized out, then this function is guaranteed to
	 * throw if it is ever called.  In that case, the compile fails,
	 * since the program is guaranteed not to work as the programmer
	 * intends.
	 *
	 * If they are not compile-time constant, but the compiler can
	 * optimize based on constants, then it will optimize out the
	 * expansion of PARTIAL_RANGE_COMPILE_CHECK_BOUND, preventing the
	 * compile error.  The function might throw on invalid inputs,
	 * including constant inputs that the compiler failed to recognize
	 * as compile-time constant.
	 *
	 * If the compiler cannot optimize based on the result of
	 * __builtin_constant_p (such as at -O0), then configure tests do
	 * not define DXX_CONSTANT_TRUE and the macro expands to nothing.
	 */
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	\
	(DXX_CONSTANT_TRUE(EXPR > d) && (DXX_ALWAYS_ERROR_FUNCTION(partial_range_will_always_throw_##S, #S " will always throw"), 0))
#else
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	static_cast<void>(0)
#endif
#define PARTIAL_RANGE_CHECK_BOUND(EXPR,S)	\
	PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S),	\
	((EXPR > d) && (I::template report<required_buffer_size>(file, line, estr, #S, EXPR, t, d), 0))
	PARTIAL_RANGE_CHECK_BOUND(o, begin);
	PARTIAL_RANGE_CHECK_BOUND(l, end);
#undef PARTIAL_RANGE_CHECK_BOUND
#undef PARTIAL_RANGE_COMPILE_CHECK_BOUND
}

/* C arrays lack a size method, but have a constant size */
template <typename T, std::size_t d>
static constexpr std::integral_constant<std::size_t, d> get_range_size(T (&)[d])
{
	return {};
}

template <typename T>
static constexpr std::size_t get_range_size(T &t)
{
	return t.size();
}

template <typename I, std::size_t N, typename T>
static inline void check_partial_range(const char *file, unsigned line, const char *estr, const T &t, const std::size_t o, const std::size_t l)
{
	check_range_bounds<I, N>(file, line, estr, addressof(t), o, l, get_range_size(t));
}

#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
template <typename I, std::size_t required_buffer_size, typename P>
static inline void check_range_object_size(const char *file, unsigned line, const char *estr, P &ref, const std::size_t o, const std::size_t l)
{
	const auto ptr = addressof(ref);
	const std::size_t bos = __builtin_object_size(ptr, 1);
	if (bos != static_cast<std::size_t>(-1))
		check_range_bounds<I, required_buffer_size>(file, line, estr, ptr, o, l, bos / sizeof(P));
}

/* When P refers to a temporary, this overload is picked.  Temporaries
 * have no useful address, so they cannot be checked.  A temporary would
 * be present if iterator.operator*() returns a proxy object, rather
 * than a reference to an element in the container.
 */
template <typename I, std::size_t required_buffer_size, typename P>
static inline void check_range_object_size(const char *, unsigned, const char *, const P &&, const std::size_t, const std::size_t) {}
#endif

}

template <typename I, std::size_t required_buffer_size>
__attribute_warn_unused_result
static inline partial_range_t<I> (unchecked_partial_range)(const char *file, unsigned line, const char *estr, I range_begin, const std::size_t o, const std::size_t l, std::true_type)
{
#ifdef DXX_CONSTANT_TRUE
	/* Compile-time only check.  Runtime handles (o > l) correctly, and
	 * it can happen in a correct program.  If it is guaranteed to
	 * happen, then the range is always empty, which likely indicates a
	 * bug.
	 */
	if (DXX_CONSTANT_TRUE(!(o < l)))
		DXX_ALWAYS_ERROR_FUNCTION(partial_range_is_always_empty, "offset never less than length");
#endif
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	/* Avoid iterator dereference if range is empty */
	if (l)
	{
		partial_range_detail::check_range_object_size<typename partial_range_t<I>::partial_range_error, required_buffer_size>(file, line, estr, *range_begin, o, l);
	}
#else
	(void)file;
	(void)line;
	(void)estr;
#endif
	auto range_end = range_begin;
	/* Use <= so that (o == 0) makes the expression always-true, so the
	 * compiler will optimize out the test.
	 */
	if (o <= l)
	{
		using std::advance;
		advance(range_begin, o);
		advance(range_end, l);
	}
	return {range_begin, range_end};
}

template <typename I, std::size_t N>
static inline partial_range_t<I> (unchecked_partial_range)(const char *, unsigned, const char *, I, std::size_t, std::size_t, std::false_type) = delete;

template <typename I, typename UO, typename UL, std::size_t NF, std::size_t NE>
__attribute_warn_unused_result
static inline partial_range_t<I> (unchecked_partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], I range_begin, const UO &o, const UL &l)
{
	/* Require unsigned length */
	return unchecked_partial_range<I, partial_range_detail::required_buffer_size<NF, NE>::value>(
		file, line, estr, range_begin, o, l,
		typename std::conditional<std::is_unsigned<UO>::value, std::is_unsigned<UL>, std::false_type>::type()
	);
}

template <typename I, typename UL, std::size_t NF, std::size_t NE>
__attribute_warn_unused_result
static inline partial_range_t<I> (unchecked_partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], I range_begin, const UL &l)
{
	return unchecked_partial_range<I, UL, UL>(file, line, estr, range_begin, 0, l);
}

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UO &o, const UL &l)
{
	partial_range_detail::check_partial_range<typename partial_range_t<I>::partial_range_error, partial_range_detail::required_buffer_size<NF, NE>::value, T>(file, line, estr, t, o, l);
	return unchecked_partial_range<I, UO, UL>(file, line, estr, begin(t), o, l);
}

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UL &l)
{
	return partial_range<T, UL, UL, NF, NE, I>(file, line, estr, t, 0, l);
}

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_const_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UO &o, const UL &l)
{
	return partial_range<const T, UO, UL, NF, NE, I>(file, line, estr, t, o, l);
}

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_const_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UL &l)
{
	return partial_range<const T, UL, NF, NE, I>(file, line, estr, t, l);
}

template <typename T, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (make_range)(T &t)
{
	return t;
}
