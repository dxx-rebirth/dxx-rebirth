/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

/* Define a utility macro `cf_assert` that, like `assert`, is meant to
 * check invariants.  Unlike `assert`, it has a small but configurable
 * effect on NDEBUG builds.
 *
 * This macro is mainly used to try to hint to gcc that it is
 * excessively cautious in determining whether a
 * -Wformat-truncation warning is appropriate.
 *
 * Unless proved otherwise by control flow analysis, gcc will assume
 * that a variable to be formatted could have any value that fits in the
 * underlying type, and then gcc will warn if any of those values does
 * not fit.  In most cases, program logic constrains the value to a
 * smaller range than the underlying type supports.  Correct placement
 * of this macro can inform the compiler that a variable's actual range
 * is less than the full supported range of the underlying type.  For
 * example, a player counter should never exceed MAX_PLAYERS, but
 * MAX_PLAYERS is far less than MAX_UCHAR.
 *
 * Unfortunately, in tested versions of gcc-8, the compiler's range
 * propagation pass is hampered by strange rules in the flow control
 * logic.  Consider:

	unsigned var = unconstrained_expression();
	// var can now be anything in [0, UINT_MAX]
	cf_assert(var <= 8);
	// If execution gets here, `var <= 8` is true.
	snprintf(..., var);

 * Suppose cf_assert(X) is defined as `((X) || (assert(X),
 * __builtin_unreachable()))`, so the above becomes:

	unsigned var = unconstrained_expression();
	if (var <= 8) {
	}
	else {
		assert_fail(...);
		__builtin_unreachable();
	}
	snprintf(..., var);

 * In testing, gcc deleted __builtin_unreachable (probably because
 * assert_fail is noreturn), then warned because, without the
 * __builtin_unreachable, flow control decides the snprintf is reachable
 * (even though assert_fail is noreturn) on the else path and would
 * misbehave if reached.  Remove the call to `assert` so that you have
 * `else { __builtin_unreachable(); }` and gcc will retain the
 * `__builtin_unreachable` and not warn.
 *
 * For an even more bizarre result:

	if (var <= 8) {
		snprintf(..., var);
	} else {
		assert(var <= 8);	// always fails
		__builtin_unreachable();
	}

 * This block warns that `var` is out of range.  Comment out the
 * `assert`, which cannot influence whether `snprintf` is reached, and
 * the warning goes away.
 *
 * --
 *
 * Leave cf_assert set as an alias for (X || __builtin_unreachable())
 * unless you know what you are doing and are prepared for the warnings
 * that will arise.
 */
#include <cassert>
#if defined(DXX_CF_ASSERT_ASSERT)
#define cf_assert	assert
#else
#ifdef DXX_CF_ASSERT_TRAP
#define cf_assert_fail	__builtin_trap
#else
#define cf_assert_fail	__builtin_unreachable
#endif
#define cf_assert(X)	((X) ? static_cast<void>(0) : (cf_assert_fail()))
#endif
