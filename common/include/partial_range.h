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
#include "backports-ranges.h"

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

#define REPORT_FORMAT_STRING	 "%s:%u: %s %lu past %p end %lu \"%s\""
/* Given the length of a filename (in `NF`) and the length of an
 * expression to report (in `NE`), compute the required size of a buffer
 * that can hold the result of applying REPORT_FORMAT_STRING.  This
 * overestimates the size since sizeof(REPORT_FORMAT_STRING) is used
 * without accounting for the %-escapes being replaced with the expanded
 * text, and all expansions are assumed to be of maximum length.
 *
 * Round reporting into large buckets.  Code size is more important than
 * stack space on a cold path.
 */
template <std::size_t NF, std::size_t NE>
constexpr std::size_t required_buffer_size = ((sizeof(REPORT_FORMAT_STRING) + sizeof("65535") + (sizeof("18446744073709551615") * 2) + sizeof("0x0000000000000000") + (NF + NE + sizeof("begin"))) | 0xff) + 1;

template <std::size_t N>
	/* This is only used on a path that will throw an exception, so mark
	 * it as cold.  In a program with no bugs, which is given no
	 * ill-formed data on input, these exceptions never happen.
	 */
__attribute_cold
void prepare_error_string(std::array<char, N> &buf, unsigned long d, const char *estr, const char *file, unsigned line, const char *desc, unsigned long expr, const uintptr_t t)
{
	std::snprintf(buf.data(), buf.size(), REPORT_FORMAT_STRING, file, line, desc, expr, reinterpret_cast<const void *>(t), d, estr);
}
#undef REPORT_FORMAT_STRING

template <typename>
void range_index_type(...);

template <
	typename range_type,
	/* If `range_type::index_type` is not defined, fail.
	 * If `range_type::index_type` is void, fail.
	 */
	typename index_type = typename std::remove_reference<typename std::remove_reference<range_type>::type::index_type &>::type>
	/* If `range_type::index_type` is not a suitable argument to
	 * range_type::operator[](), fail.
	 */
	requires(
		requires(range_type &r, index_type i) {
			r.operator[](i);
		}
	)
index_type range_index_type(std::nullptr_t);

}

#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
struct partial_range_error;
#endif

template <typename range_iterator, typename range_index_type>
class partial_range_t : ranges::subrange<range_iterator>
{
	using base_type = ranges::subrange<range_iterator>;
public:
	static_assert(!std::is_reference<range_iterator>::value);
	using iterator = range_iterator;
	using index_type = range_index_type;
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
	using base_type::base_type;
	partial_range_t(const partial_range_t &) = default;
	partial_range_t(partial_range_t &&) = default;
	partial_range_t &operator=(const partial_range_t &) = default;
	using base_type::begin;
	using base_type::end;
	using base_type::empty;
	using base_type::size;
	[[nodiscard]]
	std::reverse_iterator<iterator> rbegin() const
	{
		return std::reverse_iterator<iterator>{end()};
	}
	[[nodiscard]]
	std::reverse_iterator<iterator> rend() const
	{
		return std::reverse_iterator<iterator>{begin()};
	}
	[[nodiscard]]
	partial_range_t<std::reverse_iterator<iterator>, index_type> reversed() const
	{
		return {rbegin(), rend()};
	}
};

template <typename range_iterator, typename range_index_type>
inline constexpr bool std::ranges::enable_borrowed_range<partial_range_t<range_iterator, range_index_type>> = std::ranges::enable_borrowed_range<::ranges::subrange<range_iterator>>;

#if DXX_PARTIAL_RANGE_MINIMIZE_ERROR_TYPE
struct partial_range_error
#else
template <typename range_iterator, typename range_index_type>
struct partial_range_t<range_iterator, range_index_type>::partial_range_error
#endif
	final : std::out_of_range
{
	using std::out_of_range::out_of_range;
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

template <typename range_exception, std::size_t required_buffer_size>
__attribute_always_inline()
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
	(DXX_CONSTANT_TRUE(EXPR > d) && (DXX_ALWAYS_ERROR_FUNCTION(#S " will always throw"), 0))
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
__attribute_always_inline()
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

/* If no range_type::index_type is defined, then allow any index type. */
std::true_type check_range_index_type_vs_provided_index_type(...);

template <
	typename range_type,
	typename provided_index_type,
	/* If `range_type::index_type` is not defined, fail.
	 * If `range_type::index_type` is void, fail.
	 */
	typename range_index_type = typename std::remove_reference<typename range_type::index_type &>::type
	>
std::is_same<provided_index_type, range_index_type> check_range_index_type_vs_provided_index_type(const range_type *, const provided_index_type *);

template <typename range_type, typename index_type>
requires(std::is_enum<index_type>::value)
std::size_t cast_index_to_size(const index_type i)
{
	static_assert(std::is_unsigned<typename std::underlying_type<index_type>::type>::value, "underlying_type of index_type must be unsigned");
	/* If an enumerated index type is used, require that it be the type that
	 * the underlying range uses.
	 */
	static_assert(decltype(check_range_index_type_vs_provided_index_type(static_cast<typename std::remove_reference<range_type>::type *>(nullptr), static_cast<index_type *>(nullptr)))::value);
	return static_cast<std::size_t>(i);
}

template <typename range_type, typename index_type>
requires(std::is_integral<index_type>::value)
std::size_t cast_index_to_size(const index_type i)
{
	static_assert(std::is_unsigned<index_type>::value, "integral index_type must be unsigned");
	/* Omit enforcement of range_type::index_type vs index_type, since many
	 * call sites provide numeric length limits.
	static_assert(decltype(check_range_index_type_vs_provided_index_type<range_type, index_type>(nullptr))::value);
	 */
	/* Use an implicit conversion, so that an index_type of std::size_t does
	 * not cause a useless cast.
	 */
	return i;
}

}

template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename index_type,
	typename iterator_type
	>
[[nodiscard]]
inline partial_range_t<iterator_type, index_type> unchecked_partial_range_advance(
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
		DXX_ALWAYS_ERROR_FUNCTION("offset never less than length");
#endif
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	/* Avoid iterator dereference if range is empty */
	if (index_end)
	{
		partial_range_detail::check_range_object_size<typename partial_range_t<iterator_type, index_type>::partial_range_error, required_buffer_size>(file, line, estr, *range_begin, index_begin, index_end);
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
	typename range_type,
	typename index_begin_type,
	typename index_end_type,
	typename iterator_type = decltype(std::begin(std::declval<range_type &>())),
	/* This is in the template signature so that an `iterator_type`
	 * which does not provide `operator*()` will trigger an error
	 * and remove this overload from the resolution set.
	 */
	typename reference = decltype(*std::declval<iterator_type>())
	>
[[nodiscard]]
__attribute_always_inline()
inline auto (unchecked_partial_range)(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	range_type &range, const index_begin_type &index_begin, const index_end_type &index_end)
{
	static_assert(!std::is_void<reference>::value, "dereference of iterator must not be void");
	return unchecked_partial_range_advance<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		decltype(partial_range_detail::range_index_type<range_type>(nullptr)), iterator_type>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			std::begin(range),
			partial_range_detail::cast_index_to_size<range_type, index_begin_type>(index_begin),
			partial_range_detail::cast_index_to_size<range_type, index_end_type>(index_end)
		);
}

template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename iterator_type,
	typename index_begin_type,
	typename index_end_type,
	/* C arrays (`int a[5];`) can match both the overload that calls
	 * `std::begin(range)` and the overload that calls
	 * `operator*(range)`, leading to an ambiguity.  Some supporting
	 * libraries define C arrays, which callers use partial_range on, so
	 * the array use cannot be converted to std::array.  Use
	 * std::enable_if to disable this overload in the case of a C array.
	 *
	 * C++ std::array does not permit `operator*(range)`, and so does
	 * not match this overload, regardless of whether std::enable_if is
	 * used.
	 */
	typename reference = typename std::enable_if<!std::is_array<iterator_type>::value, decltype(*std::declval<iterator_type>())>::type
	>
[[nodiscard]]
inline auto (unchecked_partial_range)(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	iterator_type iterator, const index_begin_type &index_begin, const index_end_type &index_end)
{
	/* Require unsigned length */
	static_assert(std::is_unsigned<index_begin_type>::value, "offset to partial_range must be unsigned");
	static_assert(std::is_unsigned<index_end_type>::value, "length to partial_range must be unsigned");
	static_assert(!std::is_void<reference>::value, "dereference of iterator must not be void");
	/* Do not try to guess an index_type when supplied an iterator.  Use
	 * `void` to state that no index_type is available.  Callers which
	 * need a defined `index_type` should use the range-based
	 * unchecked_partial_range() instead, which can extract an
	 * index_type from the range_type.
	 */
	return unchecked_partial_range_advance<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		void, iterator_type>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			std::move(iterator), index_begin, index_end
	);
}

/* Take either a range or an iterator, and forward it to an appropriate
 * function, adding an index_begin={} along the way.
 */
template <
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	std::size_t required_buffer_size,
#endif
	typename iterable,
	typename index_end_type
	>
[[nodiscard]]
inline auto (unchecked_partial_range)(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
	const char *const file, const unsigned line, const char *const estr,
#endif
	iterable &&it, const index_end_type &index_end)
{
	return unchecked_partial_range<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size
#endif
		>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			std::forward<iterable>(it), index_end_type{}, index_end
		);
}

template <
	std::size_t required_buffer_size,
	typename range_type,
	typename index_begin_type,
	typename index_end_type,
	typename iterator_type = decltype(std::begin(std::declval<range_type &>()))
	>
[[nodiscard]]
inline auto (partial_range)(const char *const file, const unsigned line, const char *const estr, range_type &range, const index_begin_type &index_begin, const index_end_type &index_end)
{
	const auto sz_begin = partial_range_detail::cast_index_to_size<range_type, index_begin_type>(index_begin);
	const auto sz_end = partial_range_detail::cast_index_to_size<range_type, index_end_type>(index_end);
	partial_range_detail::check_range_bounds<
		typename partial_range_t<iterator_type, decltype(partial_range_detail::range_index_type<range_type>(nullptr))>::partial_range_error,
		required_buffer_size
	>(file, line, estr, reinterpret_cast<uintptr_t>(std::addressof(range)), sz_begin, sz_end, std::size(range));
	return unchecked_partial_range<
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
		required_buffer_size,
#endif
		range_type>(
#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
			file, line, estr,
#endif
			range, sz_begin, sz_end
		);
}

template <
	std::size_t required_buffer_size,
	typename range_type,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_range)(const char *const file, const unsigned line, const char *const estr, range_type &range, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, range_type, index_end_type, index_end_type>(file, line, estr, range, index_end_type{}, index_end);
}

template <
	std::size_t required_buffer_size,
	typename range_type,
	typename index_begin_type,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_const_range)(const char *const file, const unsigned line, const char *const estr, const range_type &range, const index_begin_type &index_begin, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, const range_type, index_begin_type, index_end_type>(file, line, estr, range, index_begin, index_end);
}

template <
	std::size_t required_buffer_size,
	typename range_type,
	typename index_end_type
	>
[[nodiscard]]
inline auto (partial_const_range)(const char *const file, const unsigned line, const char *const estr, const range_type &range, const index_end_type &index_end)
{
	return partial_range<required_buffer_size, const range_type, index_end_type>(file, line, estr, range, index_end);
}

/* Explicitly block use on rvalue t because returned partial_range_t<I>
 * will outlive the rvalue.
 */
template <typename T,
	typename index_begin_type,
	typename index_end_type
	>
partial_range_t<decltype(begin(std::declval<T &&>()))> (partial_const_range)(const char *file, const unsigned line, const char *estr, const T &&t, const index_begin_type &index_begin, const index_end_type &index_end) = delete;

template <typename T,
	typename index_end_type
	>
partial_range_t<decltype(begin(std::declval<T &&>()))> (partial_const_range)(const char *file, const unsigned line, const char *estr, const T &&t, const index_end_type &index_end) = delete;

#ifdef DXX_HAVE_BUILTIN_OBJECT_SIZE
/* This macro is conditionally defined, because in the #else case, a
 * bare invocation supplies all required parameters.  In the #else case:
 * - `required_buffer_size` is not a template parameter
 * - all other template parameters can be deduced
 * - the file, line, and string form of the expression are not function
 *   parameters.
 */
#define unchecked_partial_range(T,...)	unchecked_partial_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
#endif
#define partial_range(T,...)	partial_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
#define partial_const_range(T,...)	partial_const_range<partial_range_detail::required_buffer_size<sizeof(__FILE__), sizeof(#T)>>(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
