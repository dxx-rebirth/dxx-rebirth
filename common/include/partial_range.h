/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <array>
#include <stdexcept>
#include <iterator>
#include <cstdio>
#include <string>
#include <type_traits>
#include "fwd-partial_range.h"
#include <memory>
#include "dxxsconf.h"

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

namespace {

#define REPORT_FORMAT_STRING	 "%s:%u: %s %lu past %p end %lu \"%s\""
/* Round reporting into large buckets.  Code size is more
 * important than stack space on a cold path.
 */
template <std::size_t NF, std::size_t NE>
constexpr std::size_t required_buffer_size = ((sizeof(REPORT_FORMAT_STRING) + sizeof("65535") + (sizeof("18446744073709551615") * 2) + sizeof("0x0000000000000000") + (NF + NE + sizeof("begin"))) | 0xff) + 1;

template <std::size_t N>
__attribute_cold
void prepare_error_string(std::array<char, N> &buf, unsigned long d, const char *estr, const char *file, unsigned line, const char *desc, unsigned long expr, const uintptr_t t)
{
	std::snprintf(buf.data(), buf.size(), REPORT_FORMAT_STRING, file, line, desc, expr, reinterpret_cast<const void *>(t), d, estr);
}
#undef REPORT_FORMAT_STRING

/*
 * These are placed out of line from the class and used as an
 * indirection, so that the compiler can pick std::begin/std::end or ADL
 * appropriate alternatives, if they exist.
 */
template <typename T>
inline auto adl_begin(T &t)
{
	using std::begin;
	return begin(t);
}

template <typename T>
inline auto adl_end(T &t)
{
	using std::end;
	return end(t);
}

}

}

#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
struct partial_range_error;
#endif

template <typename I>
class partial_range_t
{
public:
	using range_owns_iterated_storage = std::false_type;
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
		partial_range_t(T &&t) :
			m_begin(partial_range_detail::adl_begin(t)), m_end(partial_range_detail::adl_end(t))
	{
		/* If `T &&`, after reference collapsing, is an lvalue
		 * reference, then the object referenced by `t` will remain in
		 * scope after the statement that called this constructor, and
		 * there is no need to check whether the object `t` owns the
		 * iterated range.
		 *
		 * Otherwise, if `t` is an rvalue reference, assert that `t` is
		 * a view onto a range, rather than owning the range.  A `t`
		 * that owns the storage would leave the iterators dangling.
		 *
		 * These checks are not precise.  It is possible to have a type
		 * T that remains in scope, but frees its storage early, and
		 * leaves the range dangling.  It is possible to have a type T
		 * that owns the storage, and is not destroyed after the
		 * containing statement terminates.  Neither are good designs,
		 * and neither can be handled here.  These checks attempt to
		 * catch obvious mistakes.
		 */
		if constexpr (!std::is_lvalue_reference<T &&>::value)
			static_assert(!T::range_owns_iterated_storage::value, "rvalue reference to range requires that the range is a view, not an owner");
	}
	[[nodiscard]]
	iterator begin() const { return m_begin; }
	[[nodiscard]]
	iterator end() const { return m_end; }
	[[nodiscard]]
	bool empty() const
	{
		return m_begin == m_end;
	}
	[[nodiscard]]
	std::size_t size() const { return std::distance(m_begin, m_end); }
	[[nodiscard]]
	std::reverse_iterator<iterator> rbegin() const
	{
		return std::reverse_iterator<iterator>{m_end};
	}
	[[nodiscard]]
	std::reverse_iterator<iterator> rend() const
	{
		return std::reverse_iterator<iterator>{m_begin};
	}
	[[nodiscard]]
	partial_range_t<std::reverse_iterator<iterator>> reversed() const
	{
		return {rbegin(), rend()};
	}
};

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
		[[noreturn]]
		__attribute_cold
	static void report(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const uintptr_t t, unsigned long d)
	{
		std::array<char, N> buf;
		partial_range_detail::prepare_error_string(buf, d, estr, file, line, desc, expr, t);
		throw partial_range_error(buf.data());
	}
};

namespace partial_range_detail
{

namespace {

template <typename range_exception, std::size_t required_buffer_size>
inline void check_range_bounds(const char *file, unsigned line, const char *estr, const uintptr_t t, const std::size_t index_begin, const std::size_t index_end, const std::size_t d)
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
	((EXPR > d) && (range_exception::template report<required_buffer_size>(file, line, estr, #S, EXPR, t, d), 0))
	PARTIAL_RANGE_CHECK_BOUND(index_begin, begin);
	PARTIAL_RANGE_CHECK_BOUND(index_end, end);
#undef PARTIAL_RANGE_CHECK_BOUND
#undef PARTIAL_RANGE_COMPILE_CHECK_BOUND
}

#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
template <typename range_exception, std::size_t required_buffer_size, typename P>
inline void check_range_object_size(const char *file, unsigned line, const char *estr, P &ref, const std::size_t index_begin, const std::size_t index_end)
{
	const auto ptr = std::addressof(ref);
	/* Get the size of the object, according to the compiler's data
	 * analysis.  If a size is known, check that the index values are in
	 * range.  If no size is known, do nothing.
	 *
	 * The compiler may return a value of "unknown" here if the target
	 * object is dynamically allocated with unknown extents, or if the
	 * optimizer is not able to identify a specific object behind the
	 * pointer.  The latter could happen if the object has a fixed size
	 * assigned in a different translation unit, and only the pointer to
	 * the object is visible in the translation unit evaluating this
	 * function.
	 */
	const std::size_t bos = __builtin_object_size(ptr, 1);
	if (bos != static_cast<std::size_t>(-1))
		check_range_bounds<range_exception, required_buffer_size>(file, line, estr, reinterpret_cast<uintptr_t>(ptr), index_begin, index_end, bos / sizeof(P));
}

/* When P refers to a temporary, this overload is picked.  Temporaries
 * have no useful address, so they cannot be checked.  A temporary would
 * be present if iterator.operator*() returns a proxy object, rather
 * than a reference to an element in the container.
 */
template <typename range_exception, std::size_t required_buffer_size, typename P>
void check_range_object_size(const char *, unsigned, const char *, const P &&, std::size_t, std::size_t) {}
#endif

}

}

namespace {

template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename iterator_type
	>
[[nodiscard]]
inline partial_range_t<iterator_type> unchecked_partial_range_advance(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	iterator_type range_begin, const std::size_t index_begin, const std::size_t index_end)
{
#ifdef DXX_CONSTANT_TRUE
	/* Compile-time only check.  Runtime handles (index_begin >
	 * index_end) correctly, and it can happen in a correct program.  If
	 * it is guaranteed to happen, then the range is always empty, which
	 * likely indicates a bug.
	 */
	if (DXX_CONSTANT_TRUE(!(index_begin < index_end)))
		DXX_ALWAYS_ERROR_FUNCTION(partial_range_is_always_empty, "offset never less than length");
#endif
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	/* Avoid iterator dereference if range is empty */
	if (index_end)
	{
		partial_range_detail::check_range_object_size<typename partial_range_t<iterator_type>::partial_range_error, required_buffer_size>(file, line, estr, *range_begin, index_begin, index_end);
	}
#endif
	auto range_end = range_begin;
	/* Use <= so that (index_begin == 0) makes the expression
	 * always-true, so the compiler will optimize out the test.
	 */
	if (index_begin <= index_end)
	{
		using std::advance;
		advance(range_begin, index_begin);
		advance(range_end, index_end);
	}
	return {range_begin, range_end};
}

template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename I,
	typename index_begin_type,
	typename index_end_type
	>
[[nodiscard]]
inline auto (unchecked_partial_range)(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	const I range_begin, const index_begin_type &index_begin, const index_end_type &index_end)
{
	/* Require unsigned length */
	static_assert(std::is_unsigned<index_begin_type>::value, "offset to partial_range must be unsigned");
	static_assert(std::is_unsigned<index_end_type>::value, "length to partial_range must be unsigned");
	return unchecked_partial_range_advance<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		I>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			range_begin, index_begin, index_end
		);
}

template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename I,
	typename index_end_type
	>
[[nodiscard]]
inline auto (unchecked_partial_range)(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	const I range_begin, const index_end_type &index_end)
{
	return unchecked_partial_range<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		I, index_end_type, index_end_type>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			range_begin, index_end_type{}, index_end
		);
}

template <
	std::size_t required_buffer_size,
	typename T,
	typename index_begin_type,
	typename index_end_type,
	typename I = decltype(std::begin(std::declval<T &>()))
	>
[[nodiscard]]
inline auto (partial_range)(const char *const file, const unsigned line, const char *const estr, T &t, const index_begin_type &index_begin, const index_end_type &index_end)
{
	partial_range_detail::check_range_bounds<typename partial_range_t<I>::partial_range_error, required_buffer_size>(file, line, estr, reinterpret_cast<uintptr_t>(std::addressof(t)), index_begin, index_end, std::size(t));
	return unchecked_partial_range<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		I, index_begin_type, index_end_type>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			std::begin(t), index_begin, index_end
		);
}

template <
	std::size_t required_buffer_size,
	typename T,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_range)(const char *const file, const unsigned line, const char *const estr, T &t, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, T, index_end_type, index_end_type>(file, line, estr, t, index_end_type{}, index_end);
}

template <
	std::size_t required_buffer_size,
	typename T,
	typename index_begin_type,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_const_range)(const char *const file, const unsigned line, const char *const estr, const T &t, const index_begin_type &index_begin, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, const T, index_begin_type, index_end_type>(file, line, estr, t, index_begin, index_end);
}

template <
	std::size_t required_buffer_size,
	typename T,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_const_range)(const char *const file, const unsigned line, const char *const estr, const T &t, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, const T, index_end_type>(file, line, estr, t, index_end);
}

template <typename T, typename I = decltype(std::begin(std::declval<T &>()))>
[[nodiscard]]
inline partial_range_t<I> (make_range)(T &t)
{
	return t;
}

}

/* Explicitly block use on rvalue t because returned partial_range_t<I>
 * will outlive the rvalue.
 */
template <typename T,
	typename index_begin_type,
	typename index_end_type,
	typename I = decltype(begin(std::declval<T &&>()))
	>
partial_range_t<I> (partial_const_range)(const char *file, const unsigned line, const char *estr, const T &&t, const index_begin_type &index_begin, const index_end_type &index_end) = delete;

template <typename T,
	typename index_end_type,
	typename I = decltype(begin(std::declval<T &&>()))
	>
partial_range_t<I> (partial_const_range)(const char *file, const unsigned line, const char *estr, const T &&t, const index_end_type &index_end) = delete;

#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
#define unchecked_partial_range(T,...)	unchecked_partial_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
#endif
#define partial_range(T,...)	partial_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
#define partial_const_range(T,...)	partial_const_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
