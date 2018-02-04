/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <type_traits>

/* valptridx_specialized_types is never defined, but is specialized to
 * define a typedef for a type-specific class suitable for use as a base
 * of valptridx<T>.
 */
template <typename managed_type>
struct valptridx_specialized_types;

class valptridx_untyped_utilities
{
public:
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
	class allow_none_construction;
	class assume_nothrow_index;
};

template <
	typename INTEGRAL_TYPE,
	std::size_t array_size_value,
	valptridx_untyped_utilities::report_error_style report_const_error_value,
	valptridx_untyped_utilities::report_error_style report_mutable_error_value
	>
class valptridx_specialized_type_parameters : public valptridx_untyped_utilities
{
public:
	using integral_type = INTEGRAL_TYPE;
	static constexpr std::integral_constant<std::size_t, array_size_value> array_size{};
	using report_error_uses_exception = std::integral_constant<bool,
		report_const_error_value == valptridx_untyped_utilities::report_error_style::exception ||
		report_mutable_error_value == valptridx_untyped_utilities::report_error_style::exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_index_mismatch_error = typename valptridx_untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename valptridx_untyped_utilities::index_mismatch_trap_verbose,
			report_error_exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_index_range_error = typename valptridx_untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename valptridx_untyped_utilities::index_range_trap_verbose,
			report_error_exception
		>;
	template <typename array_managed_type, typename report_error_exception>
		using dispatch_null_pointer_error = typename valptridx_untyped_utilities::template dispatch_mc_report_error_type<
			array_managed_type,
			report_const_error_value,
			report_mutable_error_value,
			typename valptridx_untyped_utilities::null_pointer_trap_verbose,
			report_error_exception
			>;
};

/* These defines must expand to a value of the form (**comma**, *style*,
 * **comma**).  They are not required to use the _style_ name as a
 * trailing suffix, but that convention makes the code easier to
 * understand.
 *
 * *style* must be a member of
 * `valptridx_untyped_utilities::report_error_style`.
 */
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_undefined	, undefined,
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_trap_terse	, trap_terse,
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_trap_verbose	, trap_verbose,
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_exception	, exception,

/* If not otherwise defined, set the default reporting style for all
 * valptridx errors.  The macro value must be equal to the suffix of one
 * of the four DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_* macros above.
 * For convenience, those suffixes match the members of
 * `report_error_style`.
 *
 * For finer control, valptridx inspects four values and picks the first
 * value defined to a valid style.  Invalid styles are ignored and later
 * values are searched.  Clever use of a constexpr comparison could
 * detect invalid styles, but until someone needs such detection, this
 * is left as a sharp edge for simplicity.
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

#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH(A)	DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_##A
/* This approximates the ability to test defined-ness of a macro
 * argument, even though `#if defined(A)` cannot portably be used when
 * `A` is a macro parameter.  A Python function that does the same (but
 * more cleanly and safely) as this macro would be written as:
 *
 * ```
 *	def DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DISPATCH(A,B,C,D):
 *		for candidate in (A, B, C, D):
 *			if cpp_macro_defined(candidate):
 *				return cpp_macro_expand(candidate)
 *		return ''
 * ```
 *
 * Each of the four arguments evaluates, after macro expansion, to one of five values:
 * - `undefined`
 * - `trap_terse`
 * - `trap_verbose`
 * - `exception`
 * - anything else
 *
 * Pass each argument to a separate
 * DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH.  If the argument is one of
 * the four recognized values, then after pasting, it will be one of the
 * four DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_* macros, and will
 * expand to contain a leading and trailing comma.  If it is
 * unrecognized, it will not expand[1] and will therefore contain no
 * commas.  After expansion, the arguments to
 * DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_UNPACK_ARGS are:
 *
 * -	0-or-more garbage non-comma tokens resulting from arguments that
 *		were "anything else"
 * -	comma from the first matched argument
 * -	token from the first matched argument (which will be one of the
 *		four accepted types)
 * -	another comma from the first matched argument
 * -	0-or-more tokens from later arguments, which may or may not have
 *		been "anything else" and may or may not include more commas
 *
 * The call to UNPACK_ARGS then drops everything up to and including the
 * first comma from the first matched argument and everything at and
 * after the second comma from the first matched argument.  It passes
 * through the non-comma token from the first matched argument, yielding
 * one of the four recognized types.
 *
 * [1] Assuming no definitions for
 * DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH_* other than the four
 * above.  For proper operation, no other macros should be defined with
 * that prefix.
 */
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DISPATCH(A,B,C,D)	\
	DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_UNPACK_ARGS(	\
		DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH(A)	\
		DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH(B)	\
		DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH(C)	\
		DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_BRANCH(D)	\
	,)
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_UNPACK_ARGS(A,...)	\
	DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DROP1(A, ## __VA_ARGS__)
#define DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DROP1(DROP,TAKE,...)	TAKE

#define DXX_VALPTRIDX_DECLARE_SUBTYPE(NS_TYPE,MANAGED_TYPE,INTEGRAL_TYPE,ARRAY_SIZE_VALUE)	\
	template <>	\
	struct valptridx_specialized_types<NS_TYPE MANAGED_TYPE> {	\
		using type = valptridx_specialized_type_parameters<	\
			INTEGRAL_TYPE,	\
			ARRAY_SIZE_VALUE,	\
			valptridx_untyped_utilities::report_error_style::	\
				DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DISPATCH(	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_##MANAGED_TYPE,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_##MANAGED_TYPE,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_const_default,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_default),	\
			valptridx_untyped_utilities::report_error_style::	\
				DXX_VALPTRIDX_INTERNAL_ERROR_STYLE_DISPATCH(	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_##MANAGED_TYPE,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_default_##MANAGED_TYPE,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_mutable_default,	\
					DXX_VALPTRIDX_REPORT_ERROR_STYLE_default)	\
		>;	\
	}
