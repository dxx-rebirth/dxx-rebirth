/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <type_traits>

#ifndef DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION
#ifdef NDEBUG
#define DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION	0
#else
#define DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION	1
#endif
#endif

#if DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION
template <typename T>
struct strong_typedef;
#endif

/* Use a C++11 user-defined literal to convert a string literal into a
 * type, so that it can be used as a template type parameter.
 */
template <typename T, T... v>
struct literal_as_type {};

template <typename T, T... v>
constexpr literal_as_type<T, v...> operator""_literal_as_type();

/* Given two types representing literals, return a type representing the
 * concatenation of those literals.  This function is never defined, and
 * can only be used in unevaluated contexts.
 */
template <typename T, T... a, T... b>
constexpr literal_as_type<T, a..., b...> concatenate(literal_as_type<T, a...> &&, literal_as_type<T, b...> &&);

/* valptridx_specialized_types is never defined, but is specialized to
 * define a typedef for a type-specific class suitable for use as a base
 * of valptridx<T>.
 */
template <typename managed_type>
struct valptridx_specialized_types;

namespace valptridx_detail {

/* This type is never defined, but explicit specializations of it are
 * defined to provide a mapping from literal_as_type<char, 'X'> to
 * report_error_style::X, for each member X of report_error_style.
 */
template <typename>
struct literal_type_to_policy;

/* Given a C identifier, stringize it, then pass the string to
 * operator""_literal_as_type() to produce a specialization of
 * literal_as_type<...>, which can be used as a template type argument.
 */
#define DXX_VALPTRIDX_LITERAL_TO_TYPE2(A)	#A##_literal_as_type
#define DXX_VALPTRIDX_LITERAL_TO_TYPE(A)	decltype(DXX_VALPTRIDX_LITERAL_TO_TYPE2(A))

/* Generate all the template parameters to one instantiation of
 * error_style_dispatch.  A macro is used due to the repeated occurrence
 * of various boilerplate identifiers.
 */
#define DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_PARAMETERS(MC,TYPE)	\
	DXX_VALPTRIDX_LITERAL_TO_TYPE(TYPE),	\
	DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_##MC##_), DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_##MC##_##TYPE),	\
	DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_), DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_##TYPE),	\
	DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_##MC##_default),	\
	DXX_VALPTRIDX_LITERAL_TO_TYPE(DXX_VALPTRIDX_REPORT_ERROR_STYLE_default)	\

class untyped_utilities
{
public:
	/* Given a C identifier as PREFIX, and a C type identifier as TYPE,
	 * generate `concatenate(literal_as_type<PREFIX>,
	 * literal_as_type<TYPE>)` and `literal_as_type<PREFIX##TYPE>`.
	 * Compare them for type equality.  If `PREFIX##TYPE` matches an
	 * active macro, it will be expanded to the value of the macro, but
	 * the concatenation of literal_as_type<PREFIX> and
	 * literal_as_type<TYPE> will not be expanded.  This allows the
	 * template to detect whether `PREFIX##TYPE` is a defined macro
	 * (unless `PREFIX##TYPE` is defined as a macro that expands to its
	 * own name), and choose a branch of `std::conditional` accordingly.
	 * The true branch is chosen if `PREFIX##TYPE` is _not_ a macro, and
	 * is implemented by expanding to the third argument.  The false
	 * branch is chosen if `PREFIX##TYPE` is a macro, and is implemented
	 * as a reference to `literal_type_to_policy`.
	 */
#define DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_BRANCH(PREFIX,TYPE,BRANCH_TRUE,...)	\
	typename std::conditional<	\
	std::is_same<	\
		decltype(concatenate(	\
			std::declval<PREFIX>(), std::declval<TYPE>()	\
		)), PREFIX##TYPE>::value,	\
			BRANCH_TRUE, ## __VA_ARGS__, literal_type_to_policy<PREFIX##TYPE>	\
>::type

	/* Given specializations of `literal_as_type`, find the most
	 * specific error-reporting style and evaluate to
	 * `literal_type_to_policy` specialized on that style.  Consumers
	 * can then access `error_style_dispatch<...>::value` to obtain the
	 * chosen error-reporting style as an enum member of
	 * report_error_style.
	 */
	template <typename managed_type,
			 typename style_qualified_, typename style_qualified_managed_type,
			 typename style_default_, typename style_default_managed_type,
			 typename style_qualified_default,
			 typename style_default>
	using error_style_dispatch =
		DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_BRANCH(style_qualified_, managed_type,
			DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_BRANCH(style_default_, managed_type,
				typename std::conditional<
					std::is_same<
						decltype(
							concatenate(
								std::declval<style_qualified_>(),
								"default"_literal_as_type
							)
						),
						style_qualified_default
					>::value,
					literal_type_to_policy<style_default>,
					literal_type_to_policy<style_qualified_default>
				>::type
			)
		);
#undef DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_BRANCH
	/* The style can be selected on a per-const-qualified-type basis at
	 * compile-time by defining a DXX_VALPTRIDX_REPORT_ERROR_STYLE_*
	 * macro.  See the banner comment by the definition of
	 * `DXX_VALPTRIDX_REPORT_ERROR_STYLE_default` for details.
	 */
	enum class report_error_style
	{
		/* Do not report the error at all.  This produces the smallest
		 * program, but the program will exhibit undefined behavior if
		 * an error condition occurs.  The program may crash
		 * immediately, crash at a later stage when some other function
		 * becomes confused by the invalid data propagated by the
		 * undefined behavior, continue to run but behave incorrectly,
		 * or seem to work normally.
		 *
		 * This mode is not recommended for use in normal environments,
		 * but is included because it is the only mode which can
		 * completely optimize out the error detection code.
		 *
		 * Debugging may be very difficult, since the variables are not
		 * preserved and the undefined behavior may not halt the program
		 * until much later, if at all.
		 */
		undefined,
		/* Report the error by executing an architecture-specific
		 * trapping instruction.  No context is explicitly preserved.
		 * This produces the second smallest program.  This is the
		 * smallest choice that prevents undefined behavior.
		 *
		 * This is recommended for production in constrained
		 * environments and for environments in which no debugging will
		 * be attempted.
		 *
		 * Debugging will require reading the surrounding code to find
		 * relevant variables.  The variables might have been
		 * overwritten by the time of the trap.
		 */
		trap_terse,
		/* Report the error by executing an architecture-specific
		 * trapping instruction.  Context data is stored into
		 * registers/memory immediately before the trap.
		 * This produces a larger program than `trap_terse`, but smaller
		 * than `exception`.
		 *
		 * This is recommended for constrained environments where
		 * debugging is expected.
		 *
		 * Debugging should be easier because the relevant variables
		 * were kept alive and explicitly stored into registers/memory
		 * immediately before the trap.  Although the compiler may
		 * generate code that will overwrite those locations before the
		 * trap executes, it has no reason to do so, and likely will not
		 * do so.
		 */
		trap_verbose,
		/* Report the error by throwing an exception.  This produces the
		 * largest program, but produces the most informative output in
		 * case of an error.
		 *
		 * This is recommended for environments where debugging is
		 * expected and the space overhead is acceptable.
		 *
		 * Debugging should be easier because the relevant variables are
		 * formatted into a string, which is passed to the exception
		 * constructor.  The variables should also be visible in the
		 * call to the function which throws the exception.
		 */
		exception,
	};

#if DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION
	template <typename T>
		using wrapper = strong_typedef<T>;
#else
	template <typename T>
		using wrapper = T;
#endif

protected:
	/* These classes contain a static method `report` called when an
	 * error occurs.  The report method implements the error reporting
	 * (if any) for this type of error.  See `report_error_style`
	 * members for a description.
	 *
	 * Reporting as undefined and reporting as terse use a single helper
	 * for all error classes, since those methods do not depend on any
	 * error-type specific context data.
	 *
	 * Reporting as a trap with context is broken out on a per-error
	 * basis, but all types share the same implementation.
	 *
	 * Reporting as an exception is broken out and is not shared, so
	 * that the exception type reports the managed_type that failed the
	 * test.  See `valptridx<T>` for the exception classes.
	 */
	class report_error_undefined;
	class report_error_trap_terse;
	class index_mismatch_trap_verbose;
	class index_range_trap_verbose;
	class null_pointer_trap_verbose;

	/* This is a template switch to map each error reporting style to
	 * its corresponding class.  Modes `undefined` and `trap_terse` map
	 * to one type each.  Modes `trap_verbose` and `exception` map to
	 * varied types, so must be supplied as template arguments.
	 *
	 * Type `array_managed_type` and styles `report_const_error_mode`,
	 * `report_mutable_error_mode` are only used to to choose
	 * `report_error_mode`, which should never be specified by the
	 * caller.  Style `report_error_mode` is then used as the key of the
	 * switch.
	 *
	 * This switch yields a type that is one of the four error report
	 * classes, or `void` if the input is invalid.
	 */
	template <
		typename array_managed_type,
		report_error_style report_const_error_mode,
		report_error_style report_mutable_error_mode,
		typename report_error_trap_verbose,
		typename report_error_exception,
		report_error_style report_error_mode = std::is_const<array_managed_type>::value ? report_const_error_mode : report_mutable_error_mode
		>
		using dispatch_mc_report_error_type = typename std::conditional<
			report_error_mode == report_error_style::undefined,
			report_error_undefined,
			typename std::conditional<
				report_error_mode == report_error_style::trap_terse,
				report_error_trap_terse,
				typename std::conditional<
					report_error_mode == report_error_style::trap_verbose,
					report_error_trap_verbose,
					typename std::conditional<
						report_error_mode == report_error_style::exception,
						report_error_exception,
						/* Error, no match - use void to force a
						 * compilation error when the caller tries to
						 * access a static method of the resulting type.
						 */
						void
					>::type
				>::type
			>::type
		>::type;

	class allow_end_construction;
	class allow_none_construction {};
	class assume_nothrow_index;
	class rebind_policy;
};

/* Map the four reporting styles from their `literal_as_type`
 * representation to their `report_error_style` enumerated value.
 */
template <>
struct literal_type_to_policy<decltype("undefined"_literal_as_type)> : std::integral_constant<untyped_utilities::report_error_style, untyped_utilities::report_error_style::undefined>
{
};

template <>
struct literal_type_to_policy<decltype("trap_terse"_literal_as_type)> : std::integral_constant<untyped_utilities::report_error_style, untyped_utilities::report_error_style::trap_terse>
{
};

template <>
struct literal_type_to_policy<decltype("trap_verbose"_literal_as_type)> : std::integral_constant<untyped_utilities::report_error_style, untyped_utilities::report_error_style::trap_verbose>
{
};

template <>
struct literal_type_to_policy<decltype("exception"_literal_as_type)> : std::integral_constant<untyped_utilities::report_error_style, untyped_utilities::report_error_style::exception>
{
};

template <
	typename INTEGRAL_TYPE,
	std::size_t array_size_value,
	untyped_utilities::report_error_style report_const_error_value,
	untyped_utilities::report_error_style report_mutable_error_value
	>
class specialized_type_parameters : public untyped_utilities
{
public:
	using integral_type = INTEGRAL_TYPE;
	static constexpr std::integral_constant<std::size_t, array_size_value> array_size{};
	using report_error_uses_exception = std::integral_constant<bool,
		report_const_error_value == untyped_utilities::report_error_style::exception ||
		report_mutable_error_value == untyped_utilities::report_error_style::exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_index_mismatch_error = typename untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename untyped_utilities::index_mismatch_trap_verbose,
			report_error_exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_index_range_error = typename untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename untyped_utilities::index_range_trap_verbose,
			report_error_exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_null_pointer_error = typename untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename untyped_utilities::null_pointer_trap_verbose,
			report_error_exception
			>;
};

}

/* If not otherwise defined, set the default reporting style for all
 * valptridx errors.  The macro value must be equal to the suffix of one
 * of the four members of `report_error_style`.
 *
 * For finer control, valptridx inspects four values and picks the first
 * defined value.  Undefined styles are ignored and later values are
 * searched.  Invalid values produce a compile-time error.
 *
 * For const inputs, the four values are:
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_<TYPE>
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_<TYPE>
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_default
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_default
 *
 * For mutable inputs, the four values are:
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_<TYPE>
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_<TYPE>
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_default
 *	- DXX_VALPTRIDX_REPORT_ERROR_STYLE_default
 *
 * In all cases, <TYPE> is the second argument passed to
 * DXX_VALPTRIDX_DECLARE_SUBTYPE.  This is normally the
 * non-namespace-qualified name of the type, such as `segment`,
 * `object`, or `wall`.  Namespace qualification is not permitted since
 * colon is not valid in a macro name.
 *
 * For example, to make all const lookups use `trap_terse`, mutable
 * segment use `trap_verbose`, and all others retain default, define two
 * macros:
 *
 *	#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_default trap_terse
 *	#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_segment trap_verbose
 */
#ifndef DXX_VALPTRIDX_REPORT_ERROR_STYLE_default
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE_default	exception
#endif

#define DXX_VALPTRIDX_DECLARE_SUBTYPE(NS_TYPE,MANAGED_TYPE,INTEGRAL_TYPE,ARRAY_SIZE_VALUE)	\
	template <>	\
	struct valptridx_specialized_types<NS_TYPE MANAGED_TYPE> {	\
		using type = valptridx_detail::specialized_type_parameters<	\
			INTEGRAL_TYPE,	\
			ARRAY_SIZE_VALUE,	\
			valptridx_detail::untyped_utilities::error_style_dispatch<DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_PARAMETERS(const, MANAGED_TYPE)>::value,	\
			valptridx_detail::untyped_utilities::error_style_dispatch<DXX_VALPTRIDX_ERROR_STYLE_DISPATCH_PARAMETERS(mutable, MANAGED_TYPE)>::value	\
		>;	\
	}
